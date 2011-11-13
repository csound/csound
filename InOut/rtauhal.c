/*
  rtauhal.c:

  Copyright (C) 2011 Victor Lazzarini

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

#include <CoreMidi/CoreMidi.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/AudioHardware.h>
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <stdint.h>
#include "csdl.h"
#include "soundio.h"

#if !defined(MAC_OS_X_VERSION_10_6)
/* the API was changed for 10.6, these make it backwards compatible  */
typedef ComponentInstance AudioComponentInstance;
typedef Component AudioComponent;
typedef ComponentDescription AudioComponentDescription;
#define AudioComponentFindNext FindNextComponent
#define AudioComponentInstanceNew OpenAComponent
#define  AudioComponentInstanceDispose CloseComponent
typedef float AudioUnitSampleType;
#endif


typedef struct csdata_ {
  AudioDeviceID dev;
  AudioStreamBasicDescription format;
  int         inBufSamples;
  int         outBufSamples;
  int         currentInputIndex;
  int         currentOutputIndex;
  MYFLT       *inputBuffer;
  MYFLT       *outputBuffer;
  void        *auLock_in;                /* thread lock for au callback  */
  void        *clientLock_in;            /* thread lock for rtrecord     */
  void        *auLock_out;               /* thread lock for au callback  */
  void        *clientLock_out;           /* thread lock for rtplay       */
  csRtAudioParams *inParm;
  csRtAudioParams *outParm;
  int onchnls, inchnls;
  AudioComponentInstance outunit;
  AudioComponentInstance inunit;
  CSOUND *csound;
  AudioBufferList *inputdata;
  int disp;
  int isInputRunning;
  int isOutputRunning;
  AudioDeviceID defdevin;
  AudioDeviceID defdevout;
} csdata;

OSStatus  Csound_Input(void *inRefCon,
                       AudioUnitRenderActionFlags *ioActionFlags,
                       const AudioTimeStamp *inTimeStamp,
                       UInt32 inBusNumber,
                       UInt32 inNumberFrames,
                       AudioBufferList *ioData);

OSStatus  Csound_Render(void *inRefCon,
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp,
                        UInt32 dump,
                        UInt32 inNumberFrames,
                        AudioBufferList *ioData);

int AuHAL_open(CSOUND *csound, const csRtAudioParams * parm,
               csdata *cdata, int isInput)
{
    UInt32  psize, devnum, devnos;
    AudioDeviceID *sysdevs;
    AudioDeviceID dev;
    AudioStreamBasicDescription format;
    int     i;
    UInt32  bufframes, nchnls;
    double srate;
    UInt32 enableIO, maxFPS;
    AudioComponent HALOutput;
    AudioComponentInstance *aunit;
    AudioComponentDescription cd = {kAudioUnitType_Output,
                                    kAudioUnitSubType_HALOutput,
                                    kAudioUnitManufacturer_Apple, 0, 0};
    AudioObjectPropertyAddress prop = {
      kAudioObjectPropertyName,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster
    };
    CFStringRef devName;
    CFStringEncoding defaultEncoding = CFStringGetSystemEncoding();

    prop.mSelector = (isInput ?
                      kAudioHardwarePropertyDefaultInputDevice :
                      kAudioHardwarePropertyDefaultOutputDevice );

    psize = sizeof(AudioDeviceID);
    AudioObjectGetPropertyData(kAudioObjectSystemObject,
                               &prop, 0, NULL, &psize, &dev);
    
    if(isInput) cdata->defdevin = dev;
    else cdata->defdevout = dev;

    prop.mSelector = kAudioHardwarePropertyDevices;

    AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                 &prop, 0, NULL, &psize);
    devnos = psize / sizeof(AudioDeviceID);
    sysdevs = (AudioDeviceID *) malloc(psize);

    AudioObjectGetPropertyData(kAudioObjectSystemObject,
                               &prop, 0, NULL, &psize, sysdevs);
    if(cdata->disp){
      csound->Message(csound,
                 "==========================================================\n"
                 "AuHAL Module: found %d device(s):\n", (int) devnos);
      for (i = 0; i < devnos; i++) {
        psize = sizeof(CFStringRef);
        prop.mSelector = kAudioObjectPropertyName;
        AudioObjectGetPropertyData(sysdevs[i],
                                   &prop, 0, NULL, &psize, &devName);
        csound->Message(csound, "=> AuHAL device %d: %s \n", i,
                   CFStringGetCStringPtr(devName, defaultEncoding));
        CFRelease(devName);

      }
      cdata->disp = 0;
    }
    if (parm->devName != NULL) {
      devnum = atoi(parm->devName);
      if (devnum >= 0 && devnum < devnos)
        dev  = sysdevs[devnum];
      prop.mSelector = (isInput ?
                        kAudioHardwarePropertyDefaultInputDevice :
                        kAudioHardwarePropertyDefaultOutputDevice );
      AudioObjectSetPropertyData(kAudioObjectSystemObject, &prop,
                                 0, NULL, sizeof(AudioDeviceID), &dev);

      free(sysdevs);
    }
    psize = sizeof(CFStringRef);
    prop.mSelector = kAudioObjectPropertyName;
    AudioObjectGetPropertyData(dev,
                               &prop, 0, NULL, &psize, &devName);
    if(isInput)
      csound->Message(csound, Str("selected input device: %s \n"),
                 CFStringGetCStringPtr(devName, defaultEncoding));
    else
      csound->Message(csound, Str("selected output device: %s \n"),
                 CFStringGetCStringPtr(devName, defaultEncoding));

    CFRelease(devName);

    srate = csound->GetSr(csound);
    if(!isInput){
      nchnls =cdata->onchnls = parm->nChannels;
      bufframes = csound->GetOutputBufferSize(csound)/nchnls;
    }
    else {
      nchnls = cdata->inchnls = parm->nChannels;
      bufframes = csound->GetInputBufferSize(csound)/nchnls;
    }

    /* although the SR is set in the stream properties, 
       we also need to set the device to match */
    prop.mSelector = kAudioDevicePropertyNominalSampleRate;
    psize = sizeof(double);
    AudioObjectSetPropertyData(dev, &prop, 0, NULL, psize, &srate);
    
    double sr;
    AudioObjectGetPropertyData(dev, &prop, 0, NULL, &psize, &sr);
    if(sr != srate) {
       csound->Die(csound, 
            "could not set SR, tried %.1f, got %.1f \n", srate, sr);
    } 

    HALOutput = AudioComponentFindNext(NULL, &cd);
    if(isInput){
      AudioComponentInstanceNew(HALOutput, &(cdata->inunit));    
      enableIO = 1;
      AudioUnitSetProperty(cdata->inunit, kAudioOutputUnitProperty_EnableIO,
                           kAudioUnitScope_Input, 1, &enableIO, sizeof(enableIO));
      enableIO = 0;
      AudioUnitSetProperty(cdata->inunit, kAudioOutputUnitProperty_EnableIO,
                           kAudioUnitScope_Output, 0, &enableIO, sizeof(enableIO));
      psize = sizeof(AudioDeviceID);
      /* for input, select device AFTER enabling IO */
      AudioUnitSetProperty(cdata->inunit,kAudioOutputUnitProperty_CurrentDevice,
                           kAudioUnitScope_Global, isInput, &dev, psize); 
      aunit = &(cdata->inunit);
    } else {
      AudioComponentInstanceNew(HALOutput, &(cdata->outunit));
      psize = sizeof(AudioDeviceID);
      /* for output, select device BEFORE enabling IO */
      AudioUnitSetProperty(cdata->outunit, kAudioOutputUnitProperty_CurrentDevice,
      kAudioUnitScope_Global, isInput, &dev, psize);
      enableIO = 1;
      AudioUnitSetProperty(cdata->outunit, kAudioOutputUnitProperty_EnableIO,
                           kAudioUnitScope_Output, 0, &enableIO, sizeof(enableIO));
      enableIO = 0;
      AudioUnitSetProperty(cdata->outunit, kAudioOutputUnitProperty_EnableIO,
                           kAudioUnitScope_Input, 1, &enableIO, sizeof(enableIO));
      aunit = &(cdata->outunit);
    }
    /* now set the buffer size */
    psize = sizeof(AudioDeviceID);
    AudioUnitGetProperty(*aunit, kAudioOutputUnitProperty_CurrentDevice,
                         kAudioUnitScope_Global, isInput, &dev, &psize);
    prop.mSelector = kAudioDevicePropertyBufferFrameSize;
    psize = 4;
    AudioObjectSetPropertyData(dev, &prop, 0, NULL, psize, &bufframes);
    psize = sizeof(maxFPS);
    AudioUnitGetProperty(*aunit, kAudioUnitProperty_MaximumFramesPerSlice,
                         kAudioUnitScope_Global, isInput, &maxFPS, &psize);
    AudioUnitSetProperty(*aunit, kAudioUnitProperty_MaximumFramesPerSlice,
                         kAudioUnitScope_Global, isInput, &bufframes,
                         sizeof(UInt32));
    /* set the stream properties */
    psize = sizeof(AudioStreamBasicDescription);
    AudioUnitGetProperty(*aunit, kAudioUnitProperty_StreamFormat,
                         (isInput ? kAudioUnitScope_Output : kAudioUnitScope_Input),
                         isInput, &format, &psize);
    format.mSampleRate    = srate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagsCanonical |
      kLinearPCMFormatFlagIsNonInterleaved;
    format.mBytesPerPacket = sizeof(AudioUnitSampleType);
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = sizeof(AudioUnitSampleType);
    format.mChannelsPerFrame = nchnls;
    format.mBitsPerChannel = sizeof(AudioUnitSampleType)*8;
    AudioUnitSetProperty(*aunit, kAudioUnitProperty_StreamFormat,
                         (isInput ? kAudioUnitScope_Output : kAudioUnitScope_Input),
                         isInput, &format,
                         sizeof(AudioStreamBasicDescription));
    /* set the callbacks and open the device */
    if(!isInput) {
      AURenderCallbackStruct output;
      output.inputProc = Csound_Render;
      output.inputProcRefCon = cdata;
      AudioUnitSetProperty(*aunit, kAudioUnitProperty_SetRenderCallback,
                           kAudioUnitScope_Input, isInput, &output, sizeof(output));
      AudioUnitInitialize(*aunit);
      AudioOutputUnitStart(*aunit);

      csound->Message(csound,
                 "AuHAL module: output device open with %d buffer frames\n"
                 "======================================================\n",
                 bufframes);
    }
    else {
      AURenderCallbackStruct input;
      AudioBufferList *CAInputData =
        (AudioBufferList*)malloc(sizeof(UInt32)
                                 + cdata->inchnls * sizeof(AudioBuffer));
      CAInputData->mNumberBuffers = cdata->inchnls;
      for (i = 0; i < cdata->inchnls; i++) {
        CAInputData->mBuffers[i].mNumberChannels = 1;
        CAInputData->mBuffers[i].mDataByteSize =
          bufframes * sizeof(AudioUnitSampleType);
        CAInputData->mBuffers[i].mData =
          calloc(bufframes, sizeof(AudioUnitSampleType));
      }
      cdata->inputdata = CAInputData;

      input.inputProc = Csound_Input;
      input.inputProcRefCon = cdata;
      AudioUnitSetProperty(*aunit, kAudioOutputUnitProperty_SetInputCallback,
                           kAudioUnitScope_Input, isInput, &input, sizeof(input));
      AudioUnitInitialize(*aunit);
      AudioOutputUnitStart(*aunit);
      csound->Message(csound,
                 "AuHAL module: input device open with %d buffer frames\n"
                 "==============================================\n",
                 bufframes);

    }

    return 0;

}

/* open for audio input */
static int recopen_(CSOUND *csound, const csRtAudioParams * parm)
{
    csdata  *cdata;

    if (csound->rtRecord_userdata != NULL)
      return 0;

    /* allocate structure */

    if(csound->rtPlay_userdata != NULL)
      cdata = (csdata *) csound->rtPlay_userdata;
    else {
      cdata = (csdata *) calloc(1, sizeof(csdata));
      cdata->disp = 1;
    }

    cdata->inunit = NULL;
    cdata->auLock_in = csound->CreateThreadLock();
    cdata->clientLock_in = csound->CreateThreadLock();
    csound->WaitThreadLock(cdata->auLock_in, (size_t) 500);
    csound->WaitThreadLock(cdata->clientLock_in, (size_t) 500);
    csound->rtRecord_userdata = (void *) cdata;
    cdata->inParm =  (csRtAudioParams *) parm;
    cdata->csound = cdata->csound;
    cdata->inputBuffer =
      (MYFLT *) calloc (csound->GetInputBufferSize(csound), sizeof(MYFLT));
    cdata->isInputRunning = 1;
    return AuHAL_open(csound, parm, cdata, 1);
}

/* open for audio output */
static int playopen_(CSOUND *csound, const csRtAudioParams * parm)
{
    csdata  *cdata;
    if(csound->rtRecord_userdata != NULL)
      cdata = (csdata *) csound->rtRecord_userdata;
    else {
      cdata = (csdata *) calloc(1, sizeof(csdata));
      cdata->disp = 1;
    }
    cdata->outunit = NULL;
    cdata->auLock_out = csound->CreateThreadLock();
    cdata->clientLock_out = csound->CreateThreadLock();
    csound->WaitThreadLock(cdata->auLock_out, (size_t) 500);
    csound->WaitThreadLock(cdata->clientLock_out, (size_t) 500);

    csound->rtPlay_userdata = (void *) cdata;
    cdata->outParm =  (csRtAudioParams *) parm;
    cdata->csound = csound;
    cdata->outputBuffer =
      (MYFLT *) calloc (csound->GetOutputBufferSize(csound), sizeof(MYFLT));
    cdata->isOutputRunning = 1;
    return AuHAL_open(csound, parm, cdata, 0);
}

OSStatus  Csound_Input(void *inRefCon,
                       AudioUnitRenderActionFlags *ioActionFlags,
                       const AudioTimeStamp *inTimeStamp,
                       UInt32 inBusNumber,
                       UInt32 inNumberFrames,
                       AudioBufferList *ioData)
{
    csdata *cdata = (csdata *) inRefCon;
    CSOUND *csound = cdata->csound;
    int inchnls = cdata->inchnls;
    MYFLT *inputBuffer = cdata->inputBuffer;
    int j,k;
    AudioUnitSampleType *buffer;
    if(!cdata->isInputRunning) return 0;
    csound->WaitThreadLock(cdata->auLock_in,10);
    AudioUnitRender(cdata->inunit, ioActionFlags, inTimeStamp, inBusNumber,
                    inNumberFrames, cdata->inputdata);
    for (k = 0; k < inchnls; k++){
      buffer = (AudioUnitSampleType *) cdata->inputdata->mBuffers[k].mData;
      for(j=0; j < inNumberFrames; j++){
        inputBuffer[j*inchnls+k] = buffer[j];
      }
    }
    csound->NotifyThreadLock(cdata->clientLock_in);

    return 0;
}

static int rtrecord_(CSOUND *csound, MYFLT *inbuff_, int nbytes)
{
    csdata  *cdata;
    cdata = (csdata *) *(csound->GetRtRecordUserData(csound));
    csound->WaitThreadLock(cdata->clientLock_in, (size_t) 500);
    memcpy(inbuff_,cdata->inputBuffer,nbytes);
    csound->NotifyThreadLock(cdata->auLock_in);
    return nbytes;
}

OSStatus  Csound_Render(void *inRefCon,
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp,
                        UInt32 inBusNumber,
                        UInt32 inNumberFrames,
                        AudioBufferList *ioData)
{

    csdata *cdata = (csdata *) inRefCon;
    CSOUND *csound = cdata->csound;
    int onchnls = cdata->onchnls;;
    MYFLT *outputBuffer = cdata->outputBuffer;
    int j,k;
    AudioUnitSampleType *buffer;

    if(!cdata->isOutputRunning) return 0;
    csound->WaitThreadLock(cdata->auLock_out, 10);
    for (k = 0; k < onchnls; k++) {
      buffer = (AudioUnitSampleType *) ioData->mBuffers[k].mData;
      for(j=0; j < inNumberFrames; j++){
        buffer[j] = (AudioUnitSampleType) outputBuffer[j*onchnls+k] ;
      }
    }
    csound->NotifyThreadLock(cdata->clientLock_out);

    return 0;
}

static void rtplay_(CSOUND *csound, const MYFLT *outbuff_, int nbytes)
{

    csdata  *cdata;
    cdata = (csdata *) *(csound->GetRtPlayUserData(csound));
    csound->WaitThreadLock(cdata->clientLock_out, (size_t) 500);
    memcpy(cdata->outputBuffer,outbuff_,nbytes);
    csound->NotifyThreadLock(cdata->auLock_out);

}

/* close the I/O device entirely  */
/* called only when both complete */

static void rtclose_(CSOUND *csound)
{
    csdata *cdata;
    cdata = (csdata *) *(csound->GetRtRecordUserData(csound));
    if(cdata == NULL)
      cdata = (csdata *) *(csound->GetRtPlayUserData(csound));

    if (cdata != NULL) {
      usleep(1000*csound->GetOutputBufferSize(csound)/
             (csound->GetSr(csound)*csound->GetNchnls(csound)));

      if(cdata->inunit != NULL){
        AudioOutputUnitStop(cdata->inunit);
        AudioUnitUninitialize(cdata->inunit);
        AudioComponentInstanceDispose(cdata->inunit);
      }
      cdata->isInputRunning = 0;

      if(cdata->outunit != NULL){
        AudioOutputUnitStop(cdata->outunit);
        AudioUnitUninitialize(cdata->outunit);
        AudioComponentInstanceDispose(cdata->outunit);
      }
      cdata->isOutputRunning = 0;

      if (cdata->clientLock_in != NULL) {
        csound->NotifyThreadLock(cdata->clientLock_in);
        csound->DestroyThreadLock(cdata->clientLock_in);
        cdata->clientLock_in = NULL;
      }
      if (cdata->auLock_in != NULL) {
        csound->NotifyThreadLock(cdata->auLock_in);
        csound->DestroyThreadLock(cdata->auLock_in);
        cdata->auLock_in = NULL;
      }

      if (cdata->clientLock_out != NULL) {
        csound->NotifyThreadLock(cdata->clientLock_out);
        csound->DestroyThreadLock(cdata->clientLock_out);
        cdata->clientLock_in = NULL;
      }

      if (cdata->auLock_out != NULL) {
        csound->NotifyThreadLock(cdata->auLock_out);
        csound->DestroyThreadLock(cdata->auLock_out);
        cdata->auLock_out = NULL;
      }


      if (cdata->outputBuffer != NULL) {
        free(cdata->outputBuffer);
        cdata->outputBuffer = NULL;
      }
      if (cdata->inputBuffer != NULL) {
        free(cdata->inputBuffer);
        cdata->inputBuffer = NULL;
      }
      csound->rtRecord_userdata = NULL;
      csound->rtPlay_userdata = NULL;

      if(cdata->inputdata) {
        int i;
        for (i = 0; i < cdata->inchnls; i++)
          free(cdata->inputdata->mBuffers[i].mData);
        free(cdata->inputdata);
      }

      if(cdata->defdevin) {
      AudioObjectPropertyAddress prop = {
      kAudioHardwarePropertyDefaultInputDevice,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster
      };
      UInt32 psize = sizeof(AudioDeviceID);
      AudioObjectSetPropertyData(kAudioObjectSystemObject,
                               &prop, 0, NULL, psize, &cdata->defdevin); 
      }
      if(cdata->defdevout) { 
      AudioObjectPropertyAddress prop = {
     kAudioHardwarePropertyDefaultOutputDevice,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster
      };   
      UInt32 psize = sizeof(AudioDeviceID);
      AudioObjectSetPropertyData(kAudioObjectSystemObject,
                               &prop, 0, NULL, psize, &cdata->defdevout);
    }
      free(cdata);
      csound->Message(csound, Str("AuHAL module: device closed\n"));

    }
}

int csoundModuleInit(CSOUND *csound)
{
    char   *drv;
    drv = (char *) csound->QueryGlobalVariable(csound, "_RTAUDIO");
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "auhal") == 0 || strcmp(drv, "AuHal") == 0 ||
          strcmp(drv, "AUHAL") == 0 ||
          strcmp(drv, "coreaudio") == 0 || strcmp(drv, "CoreAudio") == 0 ||
          strcmp(drv, "COREAUDIO") == 0))
      return 0;
    if (csound->oparms->msglevel & 0x400)
      csound->Message(csound, Str("rtaudio: coreaaudio-AuHAL module enabled\n"));
    csound->SetPlayopenCallback(csound, playopen_);
    csound->SetRecopenCallback(csound, recopen_);
    csound->SetRtplayCallback(csound, rtplay_);
    csound->SetRtrecordCallback(csound, rtrecord_);
    csound->SetRtcloseCallback(csound, rtclose_);
    return 0;
}

int csoundModuleCreate(CSOUND *csound)
{
    CSOUND *p = csound;
    return 0;
}
