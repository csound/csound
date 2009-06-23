#include <OpcodeBase.hpp>
#include <cmath>
#include <iostream>
#include <list>
#include <vector>

static MYFLT pi = std::atan(1.0) * MYFLT(4.0);

class RCLowpassFilter
{
public:
  void initialize(MYFLT framesHz, MYFLT cutoffHz, MYFLT initialValue)
  {
    MYFLT tau = MYFLT(1.0) / (MYFLT(2.0) * pi * cutoffHz);
    alpha = MYFLT(1.0) / (MYFLT(1.0) + (tau * framesHz));
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
    x0(MYFLT(0.0)),
    x1(MYFLT(0.0))
  {
  }
  virtual void put(MYFLT x1_)
  {
    x0 = x1;
    x1 = x1_;
  }
  virtual MYFLT get(MYFLT q)
  {
    return x0 + (q * (x1 - x0));
  }
protected:
  MYFLT x0;
  MYFLT x1;
};

class DelayLine : public std::vector<MYFLT> 
{
public:
  MYFLT framesPerSecond;
  int writingFrame;
  int size_;
  void initialize(size_t framesPerSecond_, MYFLT maximumDelay = 1.0)
  {
    framesPerSecond = (MYFLT) framesPerSecond_;
    size_ = (size_t) std::ceil(maximumDelay * framesPerSecond);
    std::cout << "DelayLine::initialize: size: " << size_ << std::endl;
    std::cout << "DelayLine::initialize: framesPerSecond: " << framesPerSecond << std::endl;
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
    int delayFrames_ = (int) (delaySeconds * framesPerSecond);
    return delayFrames(delayFrames_);
  }
  MYFLT delayFrames(int delayFrames_)
  {
    //std::cout << "DelayLine::delayFrames: delayFrames: " << delayFrames_ << std::endl;
    int readingFrame;
    if ( writingFrame >= delayFrames_) {
      readingFrame = writingFrame - delayFrames_;
    } else {
      readingFrame = writingFrame - delayFrames_ + size_;
    }
    // std::cout << "DelayLine::delayFrames: readingFrame: " << readingFrame << std::endl;
    return (*this)[(size_t) readingFrame];
  }
};

std::list<RCLowpassFilter *> updateSmoothingFilterInstances;
std::list<DelayLine *> delayLineInstances;

class Doppler : public OpcodeBase<Doppler>
{
public:
  // Csound opcode outputs.
  MYFLT *audioOutput;
  // Csound opcode inputs.
  MYFLT *audioInput;
  MYFLT *kSourcePositionMeters;
  MYFLT *kMicPositionMeters;
  MYFLT *jSpeedOfSoundMetersPerSecond;
  MYFLT *jUpdateSmoothingFilterCutoffHz;
  // Doppler internal state.
  RCLowpassFilter *updateSmoothingFilter;
  LinearInterpolator *audioInterpolator;
  std::list< std::vector<MYFLT> *> *audioBufferQueue;
  std::list<MYFLT> *sourcePositionQueue;
  size_t currentFrame;
  size_t relativeFrame;
  // For convenience.
  MYFLT framesHz;
  size_t framesPerBlock;
  int init(CSOUND *csound)
  {
    updateSmoothingFilter = 0;
    audioInterpolator = new LinearInterpolator;
    audioBufferQueue = new std::list< std::vector<MYFLT> *>;
    sourcePositionQueue = new std::list<MYFLT>;
    // Take care of default values.
    if (*jUpdateSmoothingFilterCutoffHz == MYFLT(-1.0)) {
      *jUpdateSmoothingFilterCutoffHz = MYFLT(6.0);
    }
    if (*jSpeedOfSoundMetersPerSecond == MYFLT(-1.0)) {
      *jSpeedOfSoundMetersPerSecond = MYFLT(340.29);
    }
    currentFrame = 0;
    relativeFrame = 0;
    // Save these values for convenience.
    framesHz = csound->GetSr(csound);
    framesPerBlock = csound->GetKsmps(csound);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    // The smoothing filter cannot be initialized at i-time,
    // because it must be initialized from a k-rate variable.
    if (!updateSmoothingFilter) {
      updateSmoothingFilter = new RCLowpassFilter();
      updateSmoothingFilter->initialize(framesHz, *jUpdateSmoothingFilterCutoffHz, *kSourcePositionMeters);
      log(csound, "Doppler::kontrol: this:                           %10p\n", this);
      log(csound, "Doppler::kontrol: sizeof(MYFLT):                  %10d\n", sizeof(MYFLT));
      log(csound, "Doppler::kontrol: audioOutput:                    %10p\n", audioOutput);
      log(csound, "Doppler::kontrol: audioInput:                     %10p\n", audioInput);
      log(csound, "Doppler::kontrol: kSourcePositionMeters:          %10.3f\n", *kSourcePositionMeters);
      log(csound, "Doppler::kontrol: kMicPositionMeters:             %10.3f\n", *kMicPositionMeters);
      log(csound, "Doppler::kontrol: jSpeedOfSoundMetersPerSecond:   %10.3f\n", *jSpeedOfSoundMetersPerSecond);
      log(csound, "Doppler::kontrol: jUpdateSmoothingFilterCutoffHz: %10.3f\n", *jUpdateSmoothingFilterCutoffHz);
      log(csound, "Doppler::kontrol: framesHz:                       %10.3f\n", framesHz);
      log(csound, "Doppler::kontrol: framesPerBlock:                 %10d\n", framesPerBlock);
    }
    // Send the audio input into the audio buffer queue.
    std::vector<MYFLT> *audioBuffer = new std::vector<MYFLT>;
    audioBuffer->resize(framesPerBlock);
    for (size_t inputFrame = 0; inputFrame < framesPerBlock; inputFrame++) {
      (*audioBuffer)[inputFrame] = audioInput[inputFrame];
    }
    audioBufferQueue->push_front(audioBuffer);
    // Send the current source position into the source position queue.
    sourcePositionQueue->push_front(*kSourcePositionMeters);
    // Get the current audio buffer from the audio buffer queue.
    std::vector<MYFLT> *currentBuffer = audioBufferQueue->front();
    // Obtain the current target position.
    // This can be positive, if the source is behind the mic,
    // or negative, if the source is in front of the mic.
    MYFLT targetPositionMeters = sourcePositionQueue->front() - *kMicPositionMeters;
    // Send to the audio output the audio input,
    // which is more or less delayed in the buffer queue by the time it takes
    // for sound to travel the distance between the source and the mic.
    for (size_t outputFrame = 0; outputFrame < framesPerBlock; outputFrame++) {
      // Smooth the current position.
      MYFLT currentPositionMeters = updateSmoothingFilter->update(targetPositionMeters);
      // The distance traveled by the sound is the absolute value of the position.
      MYFLT distanceMeters = std::fabs(currentPositionMeters);
      // Obtain the distance in frames from the distance in time.
      MYFLT targetFrameReal = relativeFrame - (distanceMeters * framesHz / *jSpeedOfSoundMetersPerSecond);
      size_t targetFrame = size_t(targetFrameReal);
      relativeFrame++;
      // Pop delay buffers from the queue as required, if the delay shortens.
      while (targetFrame >= currentFrame) {
        if (currentFrame >= framesPerBlock) {
          targetFrame -= framesPerBlock;
          currentFrame -= framesPerBlock;
          relativeFrame -= framesPerBlock;
	  delete audioBufferQueue->front();
          audioBufferQueue->pop_front();
          sourcePositionQueue->pop_front();
          currentBuffer = audioBufferQueue->front();
          targetPositionMeters = sourcePositionQueue->front() - *kMicPositionMeters;
        }
        audioInterpolator->put((*currentBuffer)[currentFrame]);
        currentFrame++;
      }
      MYFLT fractionalFrame = targetFrameReal - targetFrame;
      MYFLT currentSample = audioInterpolator->get(fractionalFrame);
      audioOutput[outputFrame] = currentSample;
    }
    return OK;
  }
};

struct Doppler2 : public OpcodeBase<Doppler2>
{
  // Csound opcode outputs.
  MYFLT *audioOutput;
  // Csound opcode inputs.
  MYFLT *audioInput;
  MYFLT *kMicPositionMeters;
  MYFLT *kSourcePositionMeters;
  MYFLT *jSpeedOfSoundMetersPerSecond;
  MYFLT *jUpdateSmoothingFilterCutoffHz;
  // Doppler internal state.
  RCLowpassFilter *updateSmoothingFilter;
  DelayLine *delayLine;
  LinearInterpolator *audioInterpolator;
  // For convenience.
  MYFLT framesHz;
  size_t framesPerBlock;
  size_t framesFromStart;
  int init(CSOUND *csound)
  {
    // The smoothing filter cannot be initialized at i-time,
    // because it must be initialized from a k-rate variable.
    updateSmoothingFilter = 0;
    // Take care of default values.
    if (*jSpeedOfSoundMetersPerSecond == MYFLT(-1.0)) {
      *jSpeedOfSoundMetersPerSecond = MYFLT(340.29);
    }
    if (*jUpdateSmoothingFilterCutoffHz == MYFLT(-1.0)) {
      *jUpdateSmoothingFilterCutoffHz = MYFLT(6.0);
    }
    // Save these values for convenience.
    framesHz = csound->GetSr(csound);
    framesPerBlock = csound->GetKsmps(csound);
    delayLine = new DelayLine;
    audioInterpolator = new LinearInterpolator;
    // Typical C++ classes can't be straight member variables of OpcodeBase
    // because their destructors would not be called.
    delayLineInstances.push_back(delayLine);
    delayLine->initialize(framesHz, 10.0);
    framesFromStart = 0;
    return OK;
  }
  int kontrol(CSOUND *csound) 
  {
    // On the very first block only, initialize the smoothing filter.
    if (!updateSmoothingFilter) {
      updateSmoothingFilter = new RCLowpassFilter();
      updateSmoothingFilter->initialize(framesHz / framesPerBlock, *jUpdateSmoothingFilterCutoffHz, *kSourcePositionMeters);
      updateSmoothingFilterInstances.push_back(updateSmoothingFilter);
      log(csound, "Doppler::kontrol: sizeof(MYFLT):                  %10d\n", sizeof(MYFLT));
      log(csound, "Doppler::kontrol: this:                           %10p\n", this);
      log(csound, "Doppler::kontrol: audioOutput:                    %10p\n", audioOutput);
      log(csound, "Doppler::kontrol: audioInput:                     %10p\n", audioInput);
      log(csound, "Doppler::kontrol: jSpeedOfSoundMetersPerSecond:   %10.3f\n", *jSpeedOfSoundMetersPerSecond);
      log(csound, "Doppler::kontrol: jUpdateSmoothingFilterCutoffHz: %10.3f\n", *jUpdateSmoothingFilterCutoffHz);
      log(csound, "Doppler::kontrol: framesHz:                       %10.3f\n", framesHz);
      log(csound, "Doppler::kontrol: framesPerBlock:                 %10d\n", framesPerBlock);
      log(csound, "Doppler::kontrol: updateSmoothingFilter:          %10p\n", updateSmoothingFilter);
      log(csound, "Doppler::kontrol: delayLine:                      %10p\n", delayLine);
      log(csound, "Doppler::kontrol: kMicPositionMeters:             %10.3f\n", *kMicPositionMeters);
      log(csound, "Doppler::kontrol: kSourcePositionMeters:          %10.3f\n", *kSourcePositionMeters);
    }
    for (size_t frame = 0; frame < framesPerBlock; frame++, framesFromStart++) {
      delayLine->write(audioInput[frame]);
      MYFLT positionMeters = *kSourcePositionMeters - *kMicPositionMeters;
      MYFLT smoothedPositionMeters = updateSmoothingFilter->update(positionMeters);
      MYFLT distanceMeters = std::fabs(smoothedPositionMeters);
      MYFLT delaySeconds = distanceMeters / *jSpeedOfSoundMetersPerSecond;
      MYFLT delayFrames = delaySeconds * MYFLT(framesHz);
      MYFLT delayFramesFloor = std::floor(delayFrames);
      MYFLT delayFramesFraction = delayFrames - delayFramesFloor;
      MYFLT currentValue = delayLine->delayFrames((int) delayFrames);
      audioInterpolator->put(currentValue);
      audioOutput[frame] = audioInterpolator->get(delayFramesFraction);
    }
    return OK;
  }
};

extern "C"
{
  OENTRY oentries[] =
    {
      {
        (char*)"doppler",
        sizeof(Doppler2),
        3,
        (char*)"a",
        (char*)"akkjj",
        (SUBR) Doppler2::init_,
        (SUBR) Doppler2::kontrol_,
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
                                       oentry->dsblksiz, oentry->thread,
                                       oentry->outypes, oentry->intypes,
                                       (int (*)(CSOUND*,void*)) oentry->iopadr,
                                       (int (*)(CSOUND*,void*)) oentry->kopadr,
                                       (int (*)(CSOUND*,void*)) oentry->aopadr);
      }
    return status;
  }
  
  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    csound->Message(csound, "Deleting C++ objects from doppler...\n");
    for (std::list<RCLowpassFilter *>::iterator it = updateSmoothingFilterInstances.begin();
	 it != updateSmoothingFilterInstances.end();
	 ++it) {
      delete *it;
    }
    updateSmoothingFilterInstances.clear();
    for (std::list<DelayLine *>::iterator it = delayLineInstances.begin();
	 it != delayLineInstances.end();
	 ++it) {
      delete *it;
    }
    delayLineInstances.clear();
    return 0;
  }
}

