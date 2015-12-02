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


typedef struct {
  char name[128];
  int outchannels;
  int inchannels;
  int indevnum;
  int outdevnum;
} Device_Info;


typedef struct csdata_ {
  AudioDeviceID dev;
  AudioStreamBasicDescription format;
  int         inBufSamples;
  int         outBufSamples;
  int         currentInputIndex;
  int         currentOutputIndex;
  MYFLT       *inputBuffer;
  MYFLT       *outputBuffer;
  csRtAudioParams *inParm;
  csRtAudioParams *outParm;
  int onchnls, inchnls;
  AudioComponentInstance outunit;
  AudioComponentInstance inunit;
  CSOUND *csound;
  AudioBufferList *inputdata;
  int disp;
  AudioDeviceID defdevin;
  AudioDeviceID defdevout;
  int devnos;
  int devin;
  int devout;
  void *incb;
  void *outcb;
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
    AudioDeviceID dev;
    AudioDeviceID *sysdevs;
    AudioStreamBasicDescription format;
    int     i;
    Device_Info *devinfo;
    UInt32  bufframes, nchnls;
    int devouts = 0, devins = 0;
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
                      kAudioHardwarePropertyDefaultOutputDevice);

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
    devinfo = (Device_Info *) malloc(devnos*sizeof(Device_Info));
    AudioObjectGetPropertyData(kAudioObjectSystemObject,
                               &prop, 0, NULL, &psize, sysdevs);

    cdata->devnos = devnos;
    for (i = 0; (unsigned int) i < devnos; i++) {
      AudioBufferList *b;
      int devchannels, k, n;
      int numlists;
      psize = sizeof(CFStringRef);
      prop.mScope = kAudioObjectPropertyScopeGlobal;
      prop.mSelector = kAudioObjectPropertyName;
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, &devName);
      strcpy(devinfo[i].name, CFStringGetCStringPtr(devName, defaultEncoding));
      CFRelease(devName);

      devchannels = 0;
      prop.mScope = kAudioDevicePropertyScopeInput;
      prop.mSelector =  kAudioDevicePropertyStreamConfiguration;
      AudioObjectGetPropertyDataSize(sysdevs[i],
                                     &prop, 0, NULL, &psize);
      b = (AudioBufferList *) malloc(psize);
      numlists = psize / sizeof(AudioBufferList);
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, b);
      for(n=0; n < numlists; n++){
        for(k=0; (unsigned int) k < b[n].mNumberBuffers; k++)
          devchannels += b[n].mBuffers[k].mNumberChannels;
      }
      devinfo[i].inchannels = devchannels;
      if(devchannels) {
        devins++;
        devinfo[i].indevnum = devins;
      } else devinfo[i].indevnum = -1;
      free(b);

      devchannels = 0;
      prop.mScope = kAudioDevicePropertyScopeOutput;
      AudioObjectGetPropertyDataSize(sysdevs[i],
                                     &prop, 0, NULL, &psize);
      b = (AudioBufferList *) malloc(psize);
      numlists = psize /sizeof(AudioBufferList);
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, b);
      for(n=0; n < numlists; n++){
        for(k=0; (unsigned int) k < b[n].mNumberBuffers; k++)
          devchannels += b[n].mBuffers[k].mNumberChannels;
      }
      devinfo[i].outchannels = devchannels;
      if(devchannels) {
        devouts++;
        devinfo[i].outdevnum = devouts;
      } else devinfo[i].outdevnum = -1;
      free(b);
    }


    if(cdata->disp)
      csound->Message(csound,
         "==========================================================\n");

    if (isInput)
      csound->Message(csound,
                      Str("AuHAL Module: found %d input device(s):\n"), devins);
    else csound->Message(csound,
                         Str("AuHAL Module: found %d output device(s):\n"),
                         devouts);

    for (i = 0; (unsigned int)  i < devnos; i++) {
      if (isInput) {
        if(devinfo[i].inchannels) {
          csound->Message(csound, Str("%d: %s (%d channels) \n"),
                          devinfo[i].indevnum, devinfo[i].name,
                          devinfo[i].inchannels);
        }
      }
      else {
        if(devinfo[i].outchannels)
          csound->Message(csound, Str("%d: %s (%d channels) \n"),
                          devinfo[i].outdevnum, devinfo[i].name,
                          devinfo[i].outchannels);
      }
    }

    if (parm->devName != NULL) devnum = atoi(parm->devName);
    else devnum = parm->devNum;

    if (devnum > 0 && devnum < 1024) {
      int CoreAudioDev = -1;
      prop.mSelector = kAudioHardwarePropertyDevices;
      if (isInput) {
        for(i=0; (unsigned int)  i < devnos; i++) {
          if((unsigned int) devinfo[i].indevnum == devnum) CoreAudioDev = i;
        }
        if(CoreAudioDev >= 0) {
          prop.mSelector = kAudioHardwarePropertyDefaultInputDevice;
          dev  = sysdevs[CoreAudioDev];
          AudioObjectSetPropertyData(kAudioObjectSystemObject, &prop,
                                     0, NULL, sizeof(AudioDeviceID), &dev);
        }
        else csound->Warning(csound, Str("requested device %d out of range"),
                             devnum);
      }
      else {
        prop.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
        for(i=0;(unsigned int)  i < devnos; i++) {
          if((unsigned int) devinfo[i].outdevnum == devnum)  CoreAudioDev = i;
        }
        if(CoreAudioDev >= 0) {
          dev  = sysdevs[CoreAudioDev];
          AudioObjectSetPropertyData(kAudioObjectSystemObject, &prop,
                                     0, NULL, sizeof(AudioDeviceID), &dev);
        }
        else csound->Warning(csound, Str("requested device %d out of range"),
                             devnum, devinfo[CoreAudioDev].name);
      }
    }


    free(sysdevs);
    free(devinfo);

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
     double sr;
    prop.mSelector = kAudioDevicePropertyNominalSampleRate;
    if(!isInput){
      AudioObjectGetPropertyData(dev, &prop, 0, NULL, &psize, &sr);
      csound->system_sr(csound, sr);
    }

    psize = sizeof(double);
    AudioObjectSetPropertyData(dev, &prop, 0, NULL, psize, &srate);
    AudioObjectGetPropertyData(dev, &prop, 0, NULL, &psize, &sr);

    if(sr != srate) {
      csound->Warning(csound,
                      Str("Attempted to set device SR, tried %.1f, got %.1f \n"),
                      srate, sr);
    }

    HALOutput = AudioComponentFindNext(NULL, &cd);
    if (isInput) {
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
    }
    else {
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
    format.mFormatFlags = kLinearPCMFormatFlagIsNonInterleaved;
    format.mBytesPerPacket = sizeof(Float32);
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = sizeof(Float32);
    format.mChannelsPerFrame = nchnls;
    format.mBitsPerChannel = sizeof(Float32)*8;
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
                      Str("***** AuHAL module: output device open with %d "
                          "buffer frames\n"),
                          bufframes);
      cdata->disp = 0;
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
          bufframes * sizeof(Float32);
        CAInputData->mBuffers[i].mData =
          calloc(bufframes, sizeof(Float32));
      }
      cdata->inputdata = CAInputData;

      input.inputProc = Csound_Input;
      input.inputProcRefCon = cdata;
      AudioUnitSetProperty(*aunit, kAudioOutputUnitProperty_SetInputCallback,
                           kAudioUnitScope_Input, isInput, &input, sizeof(input));
      AudioUnitInitialize(*aunit);
      AudioOutputUnitStart(*aunit);
      csound->Message(csound,
              "***** AuHAL module: input device open with %d buffer frames\n",
                      bufframes);
    }
    if(!cdata->disp)
      csound->Message(csound,
              "==========================================================\n");

    cdata->disp = 0;
    return 0;

}

int listDevices(CSOUND *csound, CS_AUDIODEVICE *list, int isOutput){
    UInt32  psize, devnos;
    AudioDeviceID *sysdevs;
    Device_Info *devinfo;
    int     i;
    int devouts = 0, devins = 0;

    AudioObjectPropertyAddress prop = {
      kAudioObjectPropertyName,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster
    };
    CFStringRef devName;
    CFStringEncoding defaultEncoding = CFStringGetSystemEncoding();
    prop.mSelector = kAudioHardwarePropertyDevices;
    AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                   &prop, 0, NULL, &psize);
    devnos = psize / sizeof(AudioDeviceID);
    sysdevs = (AudioDeviceID *) malloc(psize);
    devinfo = (Device_Info *) malloc(devnos*sizeof*devinfo);
    AudioObjectGetPropertyData(kAudioObjectSystemObject,
                               &prop, 0, NULL, &psize, sysdevs);

    for (i = 0; (unsigned int) i < devnos; i++) {
      AudioBufferList *b;
      int devchannels, k, n;
      int numlists;
      psize = sizeof(CFStringRef);
      prop.mScope = kAudioObjectPropertyScopeGlobal;
      prop.mSelector = kAudioObjectPropertyName;
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, &devName);
      strcpy(devinfo[i].name, CFStringGetCStringPtr(devName, defaultEncoding));
      CFRelease(devName);

      devchannels = 0;
      prop.mScope = kAudioDevicePropertyScopeInput;
      prop.mSelector =  kAudioDevicePropertyStreamConfiguration;
      AudioObjectGetPropertyDataSize(sysdevs[i],
                                     &prop, 0, NULL, &psize);
      b = (AudioBufferList *) malloc(psize);
      numlists = psize / sizeof(AudioBufferList);
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, b);
      for(n=0; n < numlists; n++){
        for(k=0; (unsigned int)  k < b[n].mNumberBuffers; k++)
          devchannels += b[n].mBuffers[k].mNumberChannels;
      }
      devinfo[i].inchannels = devchannels;
      if(devchannels) {
        devins++;
        devinfo[i].indevnum = devins;
      } else devinfo[i].indevnum = -1;
      free(b);

      devchannels = 0;
      prop.mScope = kAudioDevicePropertyScopeOutput;
      AudioObjectGetPropertyDataSize(sysdevs[i],
                                     &prop, 0, NULL, &psize);
      b = (AudioBufferList *) malloc(psize);
      numlists = psize /sizeof(AudioBufferList);
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, b);
      for(n=0; n < numlists; n++){
        for(k=0; (unsigned int) k < b[n].mNumberBuffers; k++)
          devchannels += b[n].mBuffers[k].mNumberChannels;
      }
      devinfo[i].outchannels = devchannels;
      if(devchannels) {
        devouts++;
        devinfo[i].outdevnum = devouts;
      } else devinfo[i].outdevnum = -1;
      free(b);
    }
    if(list==NULL){
      return (isOutput ? devouts : devins);
    } else {

      char tmp[64], *s;
      int n=0, i;

      if ((s = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO")) == NULL)
        return 0;

      if(!isOutput){
        for(i=0; (unsigned int)  i < devnos; i++) {
          if(devinfo[i].inchannels) {
            strncpy(list[n].device_name,  devinfo[i].name, 63);
            snprintf(tmp, 64, "adc%d", devinfo[i].indevnum);
            strncpy(list[n].device_id, tmp, 63);
            strncpy(list[n].rt_module, s, 63);
            list[n].max_nchnls = devinfo[i].inchannels;
            list[n].isOutput = 0;
            n++;
          }
        }
        return n;
      } else {
        for(i=0;(unsigned int) i < devnos; i++){
          if(devinfo[i].outchannels) {
            strncpy(list[n].device_name,  devinfo[i].name, 63);
            snprintf(tmp, 64, "dac%d", devinfo[i].outdevnum);
            strncpy(list[n].device_id, tmp, 63);
            strncpy(list[n].rt_module, s, 63);
            list[n].max_nchnls = devinfo[i].outchannels;
            list[n].isOutput = 1;
            n++;
          }
        }
        return n;
      }
    }
}

/* open for audio input */
static int recopen_(CSOUND *csound, const csRtAudioParams * parm)
{
    csdata  *cdata;
    void **recordata = csound->GetRtRecordUserData(csound);
    if (*(csound->GetRtRecordUserData(csound)) != NULL)
      return 0;

    /* allocate structure */

    if(*(csound->GetRtPlayUserData(csound) )!= NULL)
       cdata = (csdata *) *(csound->GetRtPlayUserData(csound));
    else {
       cdata = (csdata *) calloc(1, sizeof(csdata));
      cdata->disp = 1;
    }

    cdata->inunit = NULL;
    *recordata = (void *) cdata;
    cdata->inParm =  (csRtAudioParams *) parm;
    cdata->csound = cdata->csound;
    cdata->inputBuffer =
      (MYFLT *) calloc (csound->GetInputBufferSize(csound), sizeof(MYFLT));
    cdata->incb =
      csound->CreateCircularBuffer(csound,
                                   parm->bufSamp_HW*parm->nChannels, sizeof(MYFLT));
    int ret = AuHAL_open(csound, parm, cdata, 1);
    return ret;
}

/* open for audio output */
static int playopen_(CSOUND *csound, const csRtAudioParams * parm)
{
    csdata  *cdata;
    void    **playdata = csound->GetRtPlayUserData(csound);

    if(*(csound->GetRtRecordUserData(csound)) != NULL)
      cdata = (csdata *) *(csound->GetRtRecordUserData(csound));
    else {
      cdata = (csdata *) calloc(1, sizeof(csdata));
      cdata->disp = 1;
    }
    cdata->outunit = NULL;
    *playdata = (void *) cdata;
    cdata->outParm =  (csRtAudioParams *) parm;
    cdata->csound = csound;
    cdata->outputBuffer =
      (MYFLT *) calloc (csound->GetOutputBufferSize(csound), sizeof(MYFLT));
    memset(cdata->outputBuffer, 0,
           csound->GetOutputBufferSize(csound)*sizeof(MYFLT));
    cdata->outcb =
      csound->CreateCircularBuffer(csound,
                                   parm->bufSamp_HW*parm->nChannels, sizeof(MYFLT));
    return AuHAL_open(csound, parm,cdata,0);
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
    Float32 *buffer;
    int n = inNumberFrames*inchnls;
    int l;
    IGN(ioData);

    AudioUnitRender(cdata->inunit, ioActionFlags, inTimeStamp, inBusNumber,
                    inNumberFrames, cdata->inputdata);
    for (k = 0; k < inchnls; k++){
      buffer = (Float32 *) cdata->inputdata->mBuffers[k].mData;
      for(j=0; (unsigned int) j < inNumberFrames; j++){
        inputBuffer[j*inchnls+k] = buffer[j];
      }
    }
    l = csound->WriteCircularBuffer(csound, cdata->incb,inputBuffer,n);
    return 0;
}

#define MICROS 1000000
static int rtrecord_(CSOUND *csound, MYFLT *inbuff_, int nbytes)
{
    csdata  *cdata;
    int n = nbytes/sizeof(MYFLT);
    int m = 0, l, w = n;
    MYFLT sr = csound->GetSr(csound);
    cdata = (csdata *) *(csound->GetRtRecordUserData(csound));
    do{
      l = csound->ReadCircularBuffer(csound,cdata->incb,&inbuff_[m],n);
      m += l;
      n -= l;
      if(n) usleep(MICROS*w/sr);
    } while(n);
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
    int onchnls = cdata->onchnls;
    MYFLT *outputBuffer = cdata->outputBuffer;
    int j,k;
    Float32 *buffer;
    int n = inNumberFrames*onchnls;
    IGN(ioActionFlags);
    IGN(inTimeStamp);
    IGN(inBusNumber);


    n = csound->ReadCircularBuffer(csound,cdata->outcb,outputBuffer,n);
    for (k = 0; k < onchnls; k++) {
      buffer = (Float32 *) ioData->mBuffers[k].mData;
      for(j=0; (unsigned int) j < inNumberFrames; j++){
        buffer[j] = (Float32) outputBuffer[j*onchnls+k] ;
        outputBuffer[j*onchnls+k] = FL(0.0);
      }
    }
    return 0;
}

static void rtplay_(CSOUND *csound, const MYFLT *outbuff_, int nbytes)
{
    csdata  *cdata;
    int n = nbytes/sizeof(MYFLT);
    int m = 0, l, w = n;
    MYFLT sr = csound->GetSr(csound);
    cdata = (csdata *) *(csound->GetRtPlayUserData(csound));
    do {
      l = csound->WriteCircularBuffer(csound, cdata->outcb,&outbuff_[m],n);
      m += l;
      n -= l;
      if(n) usleep(MICROS*w/sr);
    } while(n);
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

      if(cdata->outunit != NULL){
        AudioOutputUnitStop(cdata->outunit);
        AudioUnitUninitialize(cdata->outunit);
        AudioComponentInstanceDispose(cdata->outunit);
      }

      if (cdata->outputBuffer != NULL) {
        free(cdata->outputBuffer);
        cdata->outputBuffer = NULL;
      }
      if (cdata->inputBuffer != NULL) {
        free(cdata->inputBuffer);
        cdata->inputBuffer = NULL;
      }

      *(csound->GetRtRecordUserData(csound)) = NULL;
      *(csound->GetRtPlayUserData(csound)) = NULL;

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
      csound->DestroyCircularBuffer(csound, cdata->incb);
      csound->DestroyCircularBuffer(csound, cdata->outcb);
      free(cdata);
      csound->Message(csound, Str("AuHAL module: device closed\n"));
    }
}

int csoundModuleInit(CSOUND *csound)
{
    char   *drv;
    csound->module_list_add(csound, "auhal", "audio");
    drv = (char *) csound->QueryGlobalVariable(csound, "_RTAUDIO");
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "auhal") == 0 || strcmp(drv, "AuHal") == 0 ||
          strcmp(drv, "AUHAL") == 0 ||
          strcmp(drv, "coreaudio") == 0 || strcmp(drv, "CoreAudio") == 0 ||
          strcmp(drv, "COREAUDIO") == 0))
      return 0;
    //if (csound->oparms->msglevel & 0x400)
    csound->Message(csound, Str("rtaudio: coreaaudio-AuHAL module enabled\n"));
    csound->SetPlayopenCallback(csound, playopen_);
    csound->SetRecopenCallback(csound, recopen_);
    csound->SetRtplayCallback(csound, rtplay_);
    csound->SetRtrecordCallback(csound, rtrecord_);
    csound->SetRtcloseCallback(csound, rtclose_);
    csound->SetAudioDeviceListCallback(csound, listDevices);
    return 0;
}

int csoundModuleCreate(CSOUND *csound)
{
    IGN(csound);
    return 0;
}
