#include <OpcodeBase.hpp>
#include <cmath>
#include <iostream>
#include <list>
#include <vector>

/* ***************  does not deal with unaligned signals ************** */
// Why not use the constants already defined?
static MYFLT pi = std::atan(1.0) * MYFLT(4.0);

class RCLowpassFilter
{
public:
  void initialize(MYFLT sampleRate, MYFLT cutoffHz, MYFLT initialValue)
  {
    MYFLT tau = MYFLT(1.0) / (MYFLT(2.0) * pi * cutoffHz);
    alpha = MYFLT(1.0) / (MYFLT(1.0) + (tau * sampleRate));
    value = initialValue;
  }
  MYFLT update(MYFLT inputValue)
  {
    value += alpha * (inputValue - value);
    return value;
  }
protected:
  MYFLT alpha;
  MYFLT value;
};

class LinearInterpolator
{
public:
  LinearInterpolator() :
    priorValue(MYFLT(0.0)),
    currentValue(MYFLT(0.0))
  {
  }
  virtual void put(MYFLT inputValue)
  {
    priorValue = currentValue;
    currentValue = inputValue;
  }
  virtual MYFLT get(MYFLT fraction)
  {
    return priorValue + (fraction * (currentValue - priorValue));
  }
protected:
  MYFLT priorValue;
  MYFLT currentValue;
};

class DelayLine : public std::vector<MYFLT>
{
public:
  MYFLT sampleRate;
  int writingFrame;
  int size_;
  void initialize(size_t sampleRate_, MYFLT maximumDelay = 10.0)
  {
    sampleRate = (MYFLT) sampleRate_;
    size_ = (int) std::ceil(maximumDelay * sampleRate);
    //std::cout << "DelayLine::initialize: size: " << size_ << std::endl;
    //std::cout << "DelayLine::initialize: sampleRate: " << sampleRate << std::endl;
    resize(size_);
    writingFrame = 0;
  }
  void write(MYFLT value)
  {
    while (writingFrame >= size_) {
      writingFrame -= size_;
    }
    (*this)[(size_t) writingFrame] = value;
    //std::cout << "DelayLine::write: writingFrame: " << writingFrame << std::endl;
    writingFrame++;
  }
  MYFLT delaySeconds(MYFLT delaySeconds)
  {
    int delayFrames_ = (int) (delaySeconds * sampleRate);
    return delayFrames(delayFrames_);
  }
  MYFLT delayFrames(int delayFrames_)
  {
    //std::cout << "DelayLine::delayFrames: delayFrames: "
      //        << delayFrames_ << std::endl;
    int readingFrame = writingFrame - delayFrames_;
    while (readingFrame < 0) {
      readingFrame += size_;
    }
    while (readingFrame >= size_) {
      readingFrame -= size_;
    }
    // std::cout << "DelayLine::delayFrames: readingFrame: "
    //           << readingFrame << std::endl;
    return (*this)[(size_t) readingFrame];
  }
};

std::list<RCLowpassFilter *> smoothingFilterInstances;
std::list<DelayLine *> delayLineInstances;

class Doppler : public OpcodeNoteoffBase<Doppler>
{
public:
  // Csound opcode outputs.
  MYFLT *audioOutput;
  // Csound opcode inputs.
  MYFLT *audioInput;
  MYFLT *kSourcePosition;     // usually meters
  MYFLT *kMicPosition;        // usually meters
  MYFLT *jSpeedOfSound;       // usually meters/second
  MYFLT *jUpdateFilterCutoff; // Hz
  // Doppler internal state.
  MYFLT speedOfSound;         // usually meters/second
  MYFLT smoothingFilterCutoff;   // Hz
  MYFLT sampleRate;           // Hz
  MYFLT samplesPerDistance;   // usually samples/meter
  MYFLT blockRate;            // Hz
  int blockSize;              // samples
  RCLowpassFilter *smoothingFilter;
  LinearInterpolator *audioInterpolator;
  std::list< std::vector<MYFLT> *> *audioBufferQueue;
  std::list<MYFLT> *sourcePositionQueue;
  int relativeIndex;
  int currentIndex;

  int init(CSOUND *csound)
  {
    sampleRate = csound->GetSr(csound);
    blockRate = opds.insdshead->ekr;
    blockSize = opds.insdshead->ksmps;
    // Take care of default values.
    if (*jSpeedOfSound == MYFLT(-1.0)) {
      speedOfSound = MYFLT(340.29);
    }
    else speedOfSound = *jSpeedOfSound;
    if (*jUpdateFilterCutoff == MYFLT(-1.0)) {
//    MYFLT blockRateNyquist = blockRate / MYFLT(2.0);
//    *jUpdateFilterCutoff = blockRateNyquist / MYFLT(2.0);
      smoothingFilterCutoff = MYFLT(6.0); // very conservative
    }
    else smoothingFilterCutoff = *jUpdateFilterCutoff;
    samplesPerDistance = sampleRate / speedOfSound;
    audioInterpolator = new LinearInterpolator;
    smoothingFilter = NULL;
    audioBufferQueue = new std::list< std::vector<MYFLT> *>;
    sourcePositionQueue = new std::list<MYFLT>;
    currentIndex = 0;
    relativeIndex = 0;
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    MYFLT sourcePosition = *kSourcePosition;
    MYFLT micPosition = *kMicPosition;

    std::vector<MYFLT> *sourceBuffer = new std::vector<MYFLT>;
    sourceBuffer->resize(blockSize);
    for (size_t inputFrame = 0; inputFrame<(unsigned int)blockSize; inputFrame++) {
      (*sourceBuffer)[inputFrame] = audioInput[inputFrame];
    }
    audioBufferQueue->push_back(sourceBuffer);
    sourcePositionQueue->push_back(sourcePosition);

    std::vector<MYFLT> *currentBuffer = audioBufferQueue->front();
    MYFLT targetPosition = sourcePositionQueue->front() - micPosition;

    // The smoothing filter cannot be initialized at i-time,
    // because it must be initialized from a k-rate variable.
    if (!smoothingFilter) {
      smoothingFilter = new RCLowpassFilter();
      smoothingFilter->initialize(sampleRate,
                                  smoothingFilterCutoff, targetPosition);
      warn(csound, "Doppler::kontrol: sizeof(MYFLT):         %10d\n", sizeof(MYFLT));
      warn(csound, "Doppler::kontrol: PI:                    %10.3f\n", pi);
      warn(csound, "Doppler::kontrol: this:                  %10p\n", this);
      warn(csound, "Doppler::kontrol: sampleRate:            %10.3f\n", sampleRate);
      warn(csound, "Doppler::kontrol: blockSize:             %10.3f\n", blockSize);
      warn(csound, "Doppler::kontrol: blockRate:             %10.3f\n", blockRate);
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

    for (size_t outputFrame = 0;
         outputFrame < (unsigned int)blockSize;
         outputFrame++) {
      MYFLT position = smoothingFilter->update(targetPosition);
      MYFLT distance = std::fabs(position);
      MYFLT sourceTime = relativeIndex - (distance * samplesPerDistance);
      int targetIndex = int(sourceTime);
      MYFLT fraction = sourceTime - targetIndex;
      relativeIndex++;
      for ( ; targetIndex >= currentIndex; currentIndex++) {
        if (currentIndex >= blockSize) {
          relativeIndex -= blockSize;
          currentIndex -= blockSize;
          targetIndex -= blockSize;
                delete audioBufferQueue->front();
          audioBufferQueue->pop_front();
          sourcePositionQueue->pop_front();
          currentBuffer = audioBufferQueue->front();
          targetPosition = sourcePositionQueue->front() - micPosition;
        }
        audioInterpolator->put((*currentBuffer)[currentIndex]);
      }
      MYFLT currentSample = audioInterpolator->get(fraction);
      audioOutput[outputFrame] = currentSample;
    }
    return OK;
  }
    int noteoff(CSOUND *csound)
    {
        int result = OK;
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

extern "C"
{
  OENTRY oentries[] =
    {
      {
        (char*)"doppler",
        sizeof(Doppler),
        0,
        3,
        (char*)"a",
        (char*)"akkjj",
        (SUBR) Doppler::init_,
        (SUBR) Doppler::kontrol_,
        0,
      },
      {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
      }
    };

  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
    return 0;
  }

  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
    int status = 0;
    for(OENTRY *oentry = &oentries[0]; oentry->opname; oentry++)
      {
        status |= csound->AppendOpcode(csound, oentry->opname,
                                       oentry->dsblksiz, oentry->flags,
                                       oentry->thread,
                                       oentry->outypes, oentry->intypes,
                                       (int (*)(CSOUND*,void*)) oentry->iopadr,
                                       (int (*)(CSOUND*,void*)) oentry->kopadr,
                                       (int (*)(CSOUND*,void*)) oentry->aopadr);
      }
    return status;
  }

  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    //csound->Message(csound, "Deleting C++ objects from doppler...\n");
    for (std::list<RCLowpassFilter *>::iterator it=smoothingFilterInstances.begin();
         it != smoothingFilterInstances.end();
         ++it) {
      delete *it;
    }
    smoothingFilterInstances.clear();
    for (std::list<DelayLine *>::iterator it = delayLineInstances.begin();
         it != delayLineInstances.end();
         ++it) {
      delete *it;
    }
    delayLineInstances.clear();
    return 0;
  }
}
