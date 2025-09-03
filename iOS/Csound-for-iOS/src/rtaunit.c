/*
  rtaunit.c
  Audio Unit module for Csound

  Copyright (C) 2025 Victor Lazzarini.

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA

*/

#include <csoundCore.h>
#include <csound_rtaudio.h>
#include <AudioUnit/AudioUnit.h>


typedef struct  {
  CSOUND *csound;
  AudioUnit aunit;
  int32_t ret;
  int32_t nsmps;
  bool audioin;
} AUNIT_PARAMS;


OSStatus audio_callback(void *inRefCon,
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp, UInt32 dump,
                        UInt32 inNumberFrames, AudioBufferList *ioData) {
  AUNIT_PARAMS *engine = (AUNIT_PARAMS *)inRefCon;
  CSOUND *csound = engine->csound;
  int32_t ret = engine->ret;
  int32_t nchnls = csoundGetChannels(csound,0);
  int32_t inchnls = csoundGetChannels(csound,1);
  float coef = (float)INT_MAX / csoundGet0dBFS(csound);
  int32_t k;
  int32_t frame;
  int32_t ksmps = csoundGetKsmps(csound)*nchnls;
  int32_t nsmps = engine->nsmps;
  int32_t insmps = nsmps;
  const MYFLT *spout = csoundGetSpout(csound);
  MYFLT *spin = csoundGetSpin(csound);
  int32_t *buffer;
  
  for(frame = 0; frame < inNumberFrames; frame++) {

    if(engine->audioin) {
      for (k = 0; k < inchnls; k++){
	buffer = (int32_t *) ioData->mBuffers[k].mData;
	spin[insmps++] =(1./coef)*buffer[frame];
      }
    }

    for(k = 0; k < nchnls; k++) {
      buffer = (int32_t *)ioData->mBuffers[k].mData;
      buffer[frame] = (SInt32)lrintf(spout[nsmps++] * coef);
    }
    if(nsmps == ksmps) {
      if(!ret) {
	ret = csoundPerformKsmps(csound);
      }
      nsmps = insmps = 0;
    }
  }
  engine->nsmps = nsmps;
  engine->ret = ret;
  return 0;
}

static int32_t open_in(CSOUND *csound, const csRtAudioParams *parm) {

  uint32_t bufframes = parm->bufSamp_SW;
  OSStatus err;
  int32_t nchnls = csoundGetChannels(csound, 1);

  // input init always first
  void **data = csoundGetRtRecordUserData(csound);
  AUNIT_PARAMS *cdata;
  cdata = (AUNIT_PARAMS*)
    csound->Calloc(csound, sizeof(AUNIT_PARAMS));
  *data = cdata;
  cdata->csound = csound;
  cdata->audioin = true;
  
  AudioStreamBasicDescription format;
  AudioComponentDescription cd = {kAudioUnitType_Output,
				  kAudioUnitSubType_RemoteIO,kAudioUnitManufacturer_Apple, 0, 0};
  AudioComponent HALOutput = AudioComponentFindNext(NULL, &cd);
  err = AudioComponentInstanceNew(HALOutput, &cdata->aunit);
  if(!err){
    UInt32 enableIO = 1;
    AudioUnitSetProperty(cdata->aunit,
			 kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Input,
			 1, &enableIO, sizeof(enableIO));
    if(enableIO) {
      UInt32 maxFPS;
      UInt32 outsize;
      outsize = sizeof(maxFPS);
      AudioUnitGetProperty(cdata->aunit,
			   kAudioUnitProperty_MaximumFramesPerSlice,
			   kAudioUnitScope_Global, 1, &maxFPS, &outsize);
      AudioUnitSetProperty(cdata->aunit,
			   kAudioUnitProperty_MaximumFramesPerSlice,
			   kAudioUnitScope_Global, 1, (uint32_t *)&(bufframes), sizeof(uint32_t));
      outsize = sizeof(AudioStreamBasicDescription);
      AudioUnitGetProperty(cdata->aunit,
			   kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1,
			   &format, &outsize);
      format.mSampleRate = csoundGetSr(csound);
      format.mFormatID = kAudioFormatLinearPCM;
      format.mFormatFlags = kAudioFormatFlagIsSignedInteger |
        kAudioFormatFlagIsPacked | kLinearPCMFormatFlagIsNonInterleaved;
      format.mBytesPerPacket = sizeof(int32_t);
      format.mFramesPerPacket = 1;
      format.mBytesPerFrame = sizeof(int32_t);
      format.mChannelsPerFrame = nchnls;
      format.mBitsPerChannel = sizeof(int32_t) * 8;
      err = AudioUnitSetProperty(cdata->aunit,
				 kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output,
				 1, &format,sizeof(AudioStreamBasicDescription));
    }
  }
  return OK;
}



static int32_t open_out(CSOUND *csound, const csRtAudioParams *parm) {

  int32_t bufframes = parm->bufSamp_SW;
  OSStatus err;
  int32_t nchnls = csoundGetChannels(csound, 1);
  void **data = csoundGetRtRecordUserData(csound);
  AUNIT_PARAMS *cdata;
  if(*data == NULL) {
    cdata = (AUNIT_PARAMS*)
      csound->Calloc(csound, sizeof(AUNIT_PARAMS));
    *data = cdata;
    cdata->csound = csound;
  }
  else cdata = *data;
  data = csoundGetRtPlayUserData(csound);
  *data = cdata;

  AudioStreamBasicDescription format;
  AudioComponentDescription cd = {kAudioUnitType_Output,
				  kAudioUnitSubType_RemoteIO,kAudioUnitManufacturer_Apple, 0, 0};
  AudioComponent HALOutput = AudioComponentFindNext(NULL, &cd);
  err = AudioComponentInstanceNew(HALOutput, &cdata->aunit);
  if(!err){
    UInt32 enableIO = 1;
    AudioUnitSetProperty(cdata->aunit,
			 kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Output,
			 0, &enableIO, sizeof(enableIO));
    if(enableIO) {
      UInt32 maxFPS;
      UInt32 outsize;
      outsize = sizeof(maxFPS);
      AudioUnitGetProperty(cdata->aunit,
			   kAudioUnitProperty_MaximumFramesPerSlice,
			   kAudioUnitScope_Global, 0, &maxFPS, &outsize);
      AudioUnitSetProperty(cdata->aunit,
			   kAudioUnitProperty_MaximumFramesPerSlice,
			   kAudioUnitScope_Global, 0, (UInt32 *)&(bufframes), sizeof(UInt32));
      outsize = sizeof(AudioStreamBasicDescription);
      AudioUnitGetProperty(cdata->aunit,
			   kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0,
			   &format, &outsize);
      format.mSampleRate = csoundGetSr(csound);
      format.mFormatID = kAudioFormatLinearPCM;
      format.mFormatFlags = kAudioFormatFlagIsSignedInteger |
        kAudioFormatFlagIsPacked | kLinearPCMFormatFlagIsNonInterleaved;
      format.mBytesPerPacket = sizeof(SInt32);
      format.mFramesPerPacket = 1;
      format.mBytesPerFrame = sizeof(SInt32);
      format.mChannelsPerFrame = nchnls;
      format.mBitsPerChannel = sizeof(SInt32) * 8;
      err = AudioUnitSetProperty(cdata->aunit,
				 kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
				 0, &format,sizeof(AudioStreamBasicDescription));
    }
  }
  AURenderCallbackStruct output;
  output.inputProc = audio_callback;
  output.inputProcRefCon = cdata;
  AudioUnitSetProperty(cdata->aunit,
		       kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,
                       0, &output, sizeof(output));
  AudioUnitInitialize(cdata->aunit);
  AudioOutputUnitStart(cdata->aunit);
  return OK;
}

static void close_io(CSOUND *csound) {

  AUNIT_PARAMS *cdata = (AUNIT_PARAMS *)
    *(csound->GetRtPlayUserData(csound));  
  if(cdata) {
    AudioOutputUnitStop(cdata->aunit);
    AudioUnitUninitialize(cdata->aunit);
    AudioComponentInstanceDispose(cdata->aunit);
  }
  if(cdata) csound->Free(csound, cdata);
  cdata = NULL;
}

static void  audio_output(CSOUND *csound, const MYFLT *outbuff, int32_t nbytes) {
  // nothing to do
}

static int audio_input(CSOUND *csound, MYFLT *inbuff, int32_t nbytes) {
  // nothing to do
}

void aunit_setup(CSOUND *csound) {
  csoundSetPlayopenCallback(csound, open_out);
  csoundSetRecopenCallback(csound, open_in);
  csoundSetRtplayCallback(csound, audio_output);
  csoundSetRtrecordCallback(csound, audio_input);
  csoundSetRtcloseCallback(csound, close_io);
}
