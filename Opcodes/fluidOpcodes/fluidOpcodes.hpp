#ifndef FLUIDOPCODE_H
#define  FLUIDOPCODE_H

#include <fluidsynth.h>
#include "csoundCore.h"

typedef struct {
  OPDS    h;
  // OUTPUTS
  MYFLT   *iEngineNum;

} FLUIDENGINE;

typedef struct {
  OPDS    h;
  // OUTPUTS
  MYFLT   *iInstrumentNumber;
  // INPUTS
  MYFLT   *filename, *iEngineNum, *iListPresets;
} FLUIDLOAD;

typedef struct {
  OPDS  h;
  // INPUTS
  MYFLT         *iEngineNumber, *iChannelNumber, *iInstrumentNumber, *iBankNumber;
  MYFLT *iPresetNumber;
} FLUID_PROGRAM_SELECT;

typedef struct {
  OPDS  h;
  // INPUTS
  MYFLT *iEngineNumber, *iChannelNumber, *iControllerNumber, *kVal;
  int   priorMidiValue;
} FLUID_CC;

typedef struct {
  OPDS    h;
  // INPUTS
  MYFLT   *iEngineNumber, *iChannelNumber, *iMidiKeyNumber, *iVelocity;
  int     initDone;
  bool    released;
} FLUID_NOTE;

typedef struct {
  OPDS    h;
  MYFLT   *aLeftOut, *aRightOut;
  MYFLT   *iEngineNum;
  int     blockSize;
} FLUIDOUT;

typedef struct {
  OPDS    h;
  MYFLT   *aLeftOut, *aRightOut;
  int     blockSize;
} FLUIDALLOUT;

#endif
