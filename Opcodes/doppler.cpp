/*
    doppler.cpp

    Copyright (C) 2014 Michael Gogins

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
 */

#include "OpcodeBase.hpp"
#include <cmath>
#include <list>
#include <vector>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace csound;

class RCLowpassFilter {
public:
  void initialize(cs_float sampleRate, cs_float cutoffHz, cs_float initialValue) {
    cs_float tau = cs_float(1.0) / (cs_float(2.0) * M_PI * cutoffHz);
    alpha = cs_float(1.0) / (cs_float(1.0) + (tau * sampleRate));
    value = initialValue;
  }
  cs_float update(cs_float inputValue) {
    value += alpha * (inputValue - value);
    return value;
  }

protected:
  cs_float alpha;
  cs_float value;
};

class LinearInterpolator {
public:
  LinearInterpolator() : priorValue(cs_float(0.0)), currentValue(cs_float(0.0)) {}
  virtual void put(cs_float inputValue) {
    priorValue = currentValue;
    currentValue = inputValue;
  }
  virtual cs_float get(cs_float fraction) {
    return priorValue + (fraction * (currentValue - priorValue));
  }
  virtual ~LinearInterpolator(){};

protected:
  cs_float priorValue;
  cs_float currentValue;
};

class DelayLine : public std::vector<cs_float> {
public:
  cs_float sampleRate;
  int32_t writingFrame;
  int32_t size_;
  void initialize(size_t sampleRate_, cs_float maximumDelay = 10.0) {
    sampleRate = (cs_float)sampleRate_;
    size_ = (int32_t)std::ceil(maximumDelay * sampleRate);
    // std::cout << "DelayLine::initialize: size: " << size_ << std::endl;
    // std::cout << "DelayLine::initialize: sampleRate: " << sampleRate <<
    // std::endl;
    resize(size_);
    writingFrame = 0;
  }
  void write(cs_float value) {
    while (writingFrame >= size_) {
      writingFrame -= size_;
    }
    (*this)[(size_t)writingFrame] = value;
    // std::cout << "DelayLine::write: writingFrame: " << writingFrame <<
    // std::endl;
    writingFrame++;
  }
  cs_float delaySeconds(cs_float delaySeconds) {
    int32_t delayFrames_ = (int32_t)(delaySeconds * sampleRate);
    return delayFrames(delayFrames_);
  }
  cs_float delayFrames(int32_t delayFrames_) {
    // std::cout << "DelayLine::delayFrames: delayFrames: "
    //        << delayFrames_ << std::endl;
    int32_t readingFrame = writingFrame - delayFrames_;
    while (readingFrame < 0) {
      readingFrame += size_;
    }
    while (readingFrame >= size_) {
      readingFrame -= size_;
    }
    // std::cout << "DelayLine::delayFrames: readingFrame: "
    //           << readingFrame << std::endl;
    return (*this)[(size_t)readingFrame];
  }
};

class Doppler : public OpcodeNoteoffBase<Doppler> {
public:
  // Csound opcode outputs.
  cs_float *audioOutput;
  // Csound opcode inputs.
  cs_float *audioInput;
  cs_float *kSourcePosition;     // usually meters
  cs_float *kMicPosition;        // usually meters
  cs_float *jSpeedOfSound;       // usually meters/second
  cs_float *jUpdateFilterCutoff; // Hz
  // Doppler internal state.
  cs_float speedOfSound;          // usually meters/second
  cs_float smoothingFilterCutoff; // Hz
  cs_float sampleRate;            // Hz
  cs_float samplesPerDistance;    // usually samples/meter
  cs_float blockRate;             // Hz
  int32_t blockSize;               // samples
  RCLowpassFilter *smoothingFilter;
  LinearInterpolator *audioInterpolator;
  std::list<std::vector<cs_float> *> *audioBufferQueue;
  std::list<cs_float> *sourcePositionQueue;
  int32_t relativeIndex;
  int32_t currentIndex;

  int32_t init(CSOUND *csound) {
    // Reinitialization replaces the delay history as well as the parameters.
    noteoff(csound);
    sampleRate = opds.insdshead->esr;
    blockRate = opds.insdshead->ekr;
    blockSize = opds.insdshead->ksmps;
    // Take care of default values.
    if (*jSpeedOfSound == cs_float(-1.0)) {
      speedOfSound = cs_float(340.29);
    } else
      speedOfSound = *jSpeedOfSound;
    if (*jUpdateFilterCutoff == cs_float(-1.0)) {
      //    cs_float blockRateNyquist = blockRate / cs_float(2.0);
      //    *jUpdateFilterCutoff = blockRateNyquist / cs_float(2.0);
      smoothingFilterCutoff = cs_float(6.0); // very conservative
    } else
      smoothingFilterCutoff = *jUpdateFilterCutoff;
    if (!(speedOfSound > cs_float(0.0)))
      return csound->InitError(csound, "doppler: speed of sound must be positive");
    if (!(smoothingFilterCutoff >= cs_float(0.0)))
      return csound->InitError(csound, "doppler: filter cutoff must be nonnegative");
    samplesPerDistance = sampleRate / speedOfSound;
    audioInterpolator = new LinearInterpolator;
    smoothingFilter = NULL;
    audioBufferQueue = new std::list<std::vector<cs_float> *>;
    sourcePositionQueue = new std::list<cs_float>;
    currentIndex = 0;
    relativeIndex = 0;
    return OK;
  }
  int32_t kontrol(CSOUND *csound) {
    uint32_t offset = opds.insdshead->ksmps_offset;
    uint32_t end = blockSize - opds.insdshead->ksmps_no_end;
    if (offset) memset(audioOutput, 0, offset * sizeof(cs_float));
    if (end < (uint32_t)blockSize)
      memset(audioOutput + end, 0, (blockSize - end) * sizeof(cs_float));
    if (offset >= end) return OK;
    cs_float sourcePosition = *kSourcePosition;
    cs_float micPosition = *kMicPosition;

    std::vector<cs_float> *sourceBuffer = new std::vector<cs_float>;
    sourceBuffer->resize(end - offset);
    for (uint32_t inputFrame = offset; inputFrame < end; inputFrame++) {
      (*sourceBuffer)[inputFrame - offset] = audioInput[inputFrame];
    }
    audioBufferQueue->push_back(sourceBuffer);
    sourcePositionQueue->push_back(sourcePosition);

    std::vector<cs_float> *currentBuffer = audioBufferQueue->front();
    int32_t currentSize = (int32_t)currentBuffer->size();
    cs_float targetPosition = sourcePositionQueue->front() - micPosition;

    // The smoothing filter cannot be initialized at i-time,
    // because it must be initialized from a k-rate variable.
    if (!smoothingFilter) {
      smoothingFilter = new RCLowpassFilter();
      smoothingFilter->initialize(sampleRate, smoothingFilterCutoff,
                                  targetPosition);
      warn(csound, "Doppler::kontrol: sizeof(cs_float):         %10zu\n",
           sizeof(cs_float));
      warn(csound, "Doppler::kontrol: PI:                    %10.3f\n", M_PI);
      warn(csound, "Doppler::kontrol: this:                  %10p\n", (void *)this);
      warn(csound, "Doppler::kontrol: sampleRate:            %10.3f\n",
           sampleRate);
      warn(csound, "Doppler::kontrol: blockSize:             %10d\n",
           blockSize);
      warn(csound, "Doppler::kontrol: blockRate:             %10.3f\n",
           blockRate);
      warn(csound, "Doppler::kontrol: speedOfSound:          %10.3f\n",
           speedOfSound);
      warn(csound, "Doppler::kontrol: samplesPerDistance:    %10.3f\n",
           samplesPerDistance);
      warn(csound, "Doppler::kontrol: smoothingFilterCutoff: %10.3f\n",
           smoothingFilterCutoff);
      warn(csound, "Doppler::kontrol: kMicPosition:          %10.3f\n",
           *kMicPosition);
      warn(csound, "Doppler::kontrol: kSourcePosition:       %10.3f\n",
           *kSourcePosition);
    }


    for (uint32_t outputFrame = offset;
         outputFrame < end;
         outputFrame++) {
      cs_float position = smoothingFilter->update(targetPosition);
      cs_float distance = std::fabs(position);
      cs_float sourceTime = relativeIndex - (distance * samplesPerDistance);
      int32_t targetIndex = int32_t(sourceTime);
      cs_float fraction = sourceTime - targetIndex;
      relativeIndex++;
      for (; targetIndex >= currentIndex; currentIndex++) {
        if (currentIndex >= currentSize) {
          relativeIndex -= currentSize;
          currentIndex -= currentSize;
          targetIndex -= currentSize;
          delete audioBufferQueue->front();
          audioBufferQueue->pop_front();
          sourcePositionQueue->pop_front();
          currentBuffer = audioBufferQueue->front();
          currentSize = (int32_t)currentBuffer->size();
          targetPosition = sourcePositionQueue->front() - micPosition;
        }
        audioInterpolator->put((*currentBuffer)[currentIndex]);
      }
      cs_float currentSample = audioInterpolator->get(fraction);
      audioOutput[outputFrame] = currentSample;
    }
    return OK;
  }
  int32_t noteoff(CSOUND *csound) {
    IGN(csound);
    int32_t result = OK;
    if (audioBufferQueue) {
      while (!audioBufferQueue->empty()) {
        delete audioBufferQueue->front();
        audioBufferQueue->pop_front();
      }
      delete audioBufferQueue;
      audioBufferQueue = 0;
    }
    if (sourcePositionQueue) {
      delete sourcePositionQueue;
      sourcePositionQueue = 0;
    }
    if (audioInterpolator) {
      delete audioInterpolator;
      audioInterpolator = 0;
    }
    if (smoothingFilter) {
      delete smoothingFilter;
      smoothingFilter = 0;
    }
    return result;
  }
};

extern "C" {
OENTRY oentries[] = {{
                         (char *)"doppler", sizeof(Doppler), 0, (char *)"a",
                         (char *)"akkjj", (SUBR)Doppler::init_,
                         (SUBR)Doppler::kontrol_, (SUBR)Doppler::deinit_,
                     },
                     {
                         0,  0, 0, 0, 0, 0, 0, 0, 0,
                     }};

PUBLIC int32_t csoundModuleInit_doppler(CSOUND *csound) {
  int32_t status = 0;
  for (OENTRY *oentry = &oentries[0]; oentry->opname; oentry++) {
    status |= csound->AppendOpcode(csound, oentry->opname, oentry->dsblksiz,
                                   oentry->flags,
                                   oentry->outypes, oentry->intypes,
                                   (int32_t (*)(CSOUND *, void *))oentry->init,
                                   (int32_t (*)(CSOUND *, void *))oentry->perf,
                                   (int32_t (*)(CSOUND *, void *))oentry->deinit);
  }
  return status;
}
  
#ifdef BUILD_PLUGINS
PUBLIC int32_t csoundModuleInfo(void) {
  return CSOUND_MODULE_INFO;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound) {
  IGN(csound);
  return 0;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound) {
  return csoundModuleInit_doppler(csound);
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) {
  IGN(csound);
  return 0;
}
#endif
}
