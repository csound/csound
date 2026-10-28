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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/AudioHardware.h>
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <stdint.h>
#include "csdl.h"
#include "rt_audio_fade.h"
#include "soundio.h"

/* Modified from BSD sources for strlcpy */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
/* modifed for speed -- JPff */
char *
strNcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit or until NULL */
    if (n != 0) {
      while (--n != 0) {
        if ((*d++ = *s++) == '\0')
          break;
      }
    }

    /* Not enough room in dst, add NUL */
    if (n == 0) {
      if (siz != 0)
        *d = '\0';                /* NUL-terminate dst */

      //while (*s++) ;
    }
    return dst;        /* count does not include NUL */
}

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
  int32_t outchannels;
  int32_t inchannels;
  int32_t indevnum;
  int32_t outdevnum;
} Device_Info;


typedef struct csdata_ {
  AudioDeviceID dev;
  AudioStreamBasicDescription format;
  int32_t         inBufSamples;
  int32_t         outBufSamples;
  int32_t         currentInputIndex;
  int32_t         currentOutputIndex;
  cs_float       *inputBuffer;
  cs_float       *outputBuffer;
  csRtAudioParams *inParm;
  csRtAudioParams *outParm;
  int32_t onchnls, inchnls;
  AudioComponentInstance outunit;
  AudioComponentInstance inunit;
  CSOUND *csound;
  AudioBufferList *inputdata;
  int32_t disp;
  AudioDeviceID defdevin;
  AudioDeviceID defdevout;
  int32_t devnos;
  int32_t devin;
  int32_t devout;
  void *incb;
  void *outcb;
  cs_float sr;
  int32_t complete;          /* set at close: fade out, then stop  */
  RT_AUDIO_FADE closeFade;
  int32_t outRing;           /* output circular buffer size, in frames */
  int32_t inRender;          /* input render buffer size, in frames */
  int32_t outLargest, inLargest; /* largest callback requests, in frames */
  int32_t warned;            /* warnings already issued, bit flags */
  int32_t stalled;           /* device stopped: 1 output, 2 input */
} csdata;


static OSStatus  Csound_Input(void *inRefCon,
                       AudioUnitRenderActionFlags *ioActionFlags,
                       const AudioTimeStamp *inTimeStamp,
                       UInt32 inBusNumber,
                       UInt32 inNumberFrames,
                       AudioBufferList *ioData);

static OSStatus  Csound_Render(void *inRefCon,
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp,
                        UInt32 dump,
                        UInt32 inNumberFrames,
                        AudioBufferList *ioData);

static void DAC_channels(CSOUND *csound, int32_t chans){
    int32_t *dachans = (int32_t *) csound->QueryGlobalVariable(csound, "_DAC_CHANNELS_");
    if (dachans == NULL) {
      if (csound->CreateGlobalVariable(csound, "_DAC_CHANNELS_",
                                       sizeof(int32_t)) != 0)
        return;
      dachans = (int32_t *) csound->QueryGlobalVariable(csound, "_DAC_CHANNELS_");
      *dachans = chans;
    }
}

static void ADC_channels(CSOUND *csound, int32_t chans){
    int32_t *dachans = (int32_t *) csound->QueryGlobalVariable(csound, "_ADC_CHANNELS_");
    if (dachans == NULL) {
      if (csound->CreateGlobalVariable(csound, "_ADC_CHANNELS_",
                                       sizeof(int32_t)) != 0)
        return;
      dachans = (int32_t *) csound->QueryGlobalVariable(csound, "_ADC_CHANNELS_");
      *dachans = chans;
    }
}

static int32_t AuHAL_open(CSOUND *csound, const csRtAudioParams * parm,
               csdata *cdata, int32_t isInput)
{
    UInt32  psize, devnum, devnos;
    AudioDeviceID dev;
    AudioDeviceID *sysdevs;
    AudioStreamBasicDescription format;
    int32_t     i;
    Device_Info *devinfo;
    UInt32  bufframes, nchnls;
    int32_t devouts = 0, devins = 0;
    Float64 srate;
    UInt32 enableIO, maxFPS, granted, need;
    int32_t ring;
    AudioComponent HALOutput;
    AudioComponentInstance *aunit;
    AudioComponentDescription cd = {kAudioUnitType_Output,
                                    kAudioUnitSubType_HALOutput,
                                    kAudioUnitManufacturer_Apple, 0, 0};
    AudioObjectPropertyAddress prop = {
      kAudioObjectPropertyName,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain
    };
    CFStringRef devName;



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
    sysdevs = (AudioDeviceID *) csound->Malloc(csound,psize);
    devinfo = (Device_Info *) csound->Malloc(csound,devnos*sizeof(Device_Info));
    AudioObjectGetPropertyData(kAudioObjectSystemObject,
                               &prop, 0, NULL, &psize, sysdevs);

    cdata->devnos = devnos;
    for (i = 0; (uint32_t) i < devnos; i++) {
      AudioBufferList *b;
      int32_t devchannels, k, n;
      int32_t numlists;
      psize = sizeof(CFStringRef);
      prop.mScope = kAudioObjectPropertyScopeGlobal;
      prop.mSelector = kAudioObjectPropertyName;
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, &devName);
      if(!CFStringGetCString(devName, devinfo[i].name, 128,
                             kCFStringEncodingUTF8))
        strNcpy(devinfo[i].name, "unnamed device", 127);
      CFRelease(devName);

      devchannels = 0;
      prop.mScope = kAudioDevicePropertyScopeInput;
      prop.mSelector =  kAudioDevicePropertyStreamConfiguration;
      AudioObjectGetPropertyDataSize(sysdevs[i],
                                     &prop, 0, NULL, &psize);
      b = (AudioBufferList *) csound->Malloc(csound,psize);
      numlists = psize / sizeof(AudioBufferList);
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, b);
      for(n=0; n < numlists; n++){
        for(k=0; (uint32_t) k < b[n].mNumberBuffers; k++)
          devchannels += b[n].mBuffers[k].mNumberChannels;
      }
      devinfo[i].inchannels = devchannels;
      if(devchannels) {
        devins++;
        devinfo[i].indevnum = devins;
      } else devinfo[i].indevnum = -1;
      csound->Free(csound,b);

      devchannels = 0;
      prop.mScope = kAudioDevicePropertyScopeOutput;
      AudioObjectGetPropertyDataSize(sysdevs[i],
                                     &prop, 0, NULL, &psize);
      b = (AudioBufferList *) csound->Malloc(csound,psize);
      numlists = psize /sizeof(AudioBufferList);
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, b);
      for(n=0; n < numlists; n++){
        for(k=0; (uint32_t) k < b[n].mNumberBuffers; k++)
          devchannels += b[n].mBuffers[k].mNumberChannels;
      }
      devinfo[i].outchannels = devchannels;
      if(devchannels) {
        devouts++;
        devinfo[i].outdevnum = devouts;
      } else devinfo[i].outdevnum = -1;
      csound->Free(csound,b);
    }

    const OPARMS *O;
    O = csound->GetOParms(csound) ;
    if(O->msglevel || O->odebug) {
    if (isInput)
      csound->Message(csound,
                      Str("AuHAL Module: found %d input device(s):\n"), devins);
    else csound->Message(csound,
                         Str("AuHAL Module: found %d output device(s):\n"),
                         devouts);

    for (i = 0; (uint32_t)  i < devnos; i++) {
      if (isInput) {
        if(devinfo[i].inchannels) {
          csound->Message(csound, Str("%d: %s (%d channels)\n"),
                          devinfo[i].indevnum, devinfo[i].name,
                          devinfo[i].inchannels);
        }
      }
      else {
        if(devinfo[i].outchannels)
          csound->Message(csound, Str("%d: %s (%d channels)\n"),
                          devinfo[i].outdevnum, devinfo[i].name,
                          devinfo[i].outchannels);
      }
    }
      }

    if (parm->devName != NULL) devnum = atoi(parm->devName);
    else devnum = parm->devNum;

    if (devnum > 0 && devnum < 1024) {
      int32_t CoreAudioDev = -1;
      prop.mSelector = kAudioHardwarePropertyDevices;
      if (isInput) {
        for(i=0; (uint32_t)  i < devnos; i++) {
          if((uint32_t) devinfo[i].indevnum == devnum) {
            CoreAudioDev = i;
            break;
          }
        }
        if (LIKELY(CoreAudioDev >= 0)) {
          prop.mSelector = kAudioHardwarePropertyDefaultInputDevice;
          dev  = sysdevs[CoreAudioDev];
          AudioObjectSetPropertyData(kAudioObjectSystemObject, &prop,
                                     0, NULL, sizeof(AudioDeviceID), &dev);
        }
        else {
       if(O->msglevel || O->odebug)
          csound->Warning(csound, Str("requested device %d out of range\n"),
                             devnum);
        }

      }
      else {
        prop.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
        for(i=0;(uint32_t)  i < devnos; i++) {
          if((uint32_t) devinfo[i].outdevnum == devnum)  CoreAudioDev = i;
        }
        if (LIKELY(CoreAudioDev >= 0)) {
          dev  = sysdevs[CoreAudioDev];
          AudioObjectSetPropertyData(kAudioObjectSystemObject, &prop,
                                     0, NULL, sizeof(AudioDeviceID), &dev);

        }
        else {
          if(O->msglevel || O->odebug)
          csound->Warning(csound, Str("requested device %d out of range\n"),
                             devnum);
        }
      }
    }

    for(i=0; (uint32_t)  i < devnos; i++) {
      if(sysdevs[i] == dev){
        if(isInput) {
          if(devinfo[i].inchannels < parm->nChannels) {
            if(O->msglevel || O->odebug)
            csound->ErrorMsg(csound,
                             Str(" *** CoreAudio: Device has not enough"
                                 " inputs (%d, requested %d)\n"),
                             devinfo[i].inchannels, parm->nChannels);
            return -1;
          }
          ADC_channels(csound, devinfo[i].inchannels);
        }
        else {
          if(devinfo[i].outchannels < parm->nChannels) {
            if(O->msglevel || O->odebug)
            csound->ErrorMsg(csound,
                             Str(" *** CoreAudio: Device has not enough"
                                 " outputs (%d, requested %d)\n"),
                             devinfo[i].outchannels, parm->nChannels);
            return -1;
          }
          DAC_channels(csound, devinfo[i].outchannels);
        }
      }
    }
    csound->Free(csound,sysdevs);
    csound->Free(csound,devinfo);

    psize = sizeof(CFStringRef);
    prop.mSelector = kAudioObjectPropertyName;
    AudioObjectGetPropertyData(dev,
                               &prop, 0, NULL, &psize, &devName);
    if(O->msglevel || O->odebug) {
    char name[128];
    if(!CFStringGetCString(devName, name, 128, kCFStringEncodingUTF8))
      strNcpy(name, "unnamed device", 127);
    if(isInput)
      csound->Message(csound, Str("selected input device: %s\n"), name);

    else
      csound->Message(csound, Str("selected output device: %s\n"), name);
    }
    CFRelease(devName);

    cdata->sr = srate = parm->sampleRate;
    if(!isInput){
      nchnls =cdata->onchnls = parm->nChannels;
      bufframes = (uint32_t) (csound->GetOutputBufferSize(csound)/nchnls);
    }
    else {
      nchnls = cdata->inchnls = parm->nChannels;
      bufframes =  (uint32_t) (csound->GetInputBufferSize(csound)/nchnls);
    }

    /* although the SR is set in the stream properties,
       we also need to set the device to match */
    Float64 sr = 0.0;
    prop.mSelector = kAudioDevicePropertyNominalSampleRate;
    if(!isInput){
      AudioObjectGetPropertyData(dev, &prop, 0, NULL, &psize, &sr);
      csound->GetSystemSr(csound, sr);
    }

    psize = sizeof(Float64);
    AudioObjectSetPropertyData(dev, &prop, 0, NULL, psize, &srate);
    AudioObjectGetPropertyData(dev, &prop, 0, NULL, &psize, &sr);

    if(srate < 0)
      srate  =  csound->GetSystemSr(csound, sr);
    int attempts = 0;
    while (UNLIKELY(sr != srate)) {
       if(O->odebug)
        csound->Warning(csound,
                      Str("Attempted to set device SR, tried %.1f, got %.1f\n"),
                      srate, sr);
       // wait for it
       csound->Sleep(500);
       // try again
       AudioObjectSetPropertyData(dev, &prop, 0, NULL, psize, &srate);
       AudioObjectGetPropertyData(dev, &prop, 0, NULL, &psize, &sr);
       // try another 5 times max (2.5 sec wait)
       if(++attempts > 5) {
         csound->Warning(csound, Str("could not set sr to %.1f after %d attempts\n"),
                         srate, attempts);
         break;
       }
    }
    /* the input unit does not convert the sampling rate: refuse a device
       that cannot run at ours, rather than read it at the wrong rate */
    if (isInput && sr != srate) {
      csound->ErrorMsg(csound,
                       Str(" *** CoreAudio: input device runs at %.0f Hz, "
                           "not %.0f Hz\n"), sr, srate);
      if (cdata->defdevin) {
        AudioObjectPropertyAddress def = {
          kAudioHardwarePropertyDefaultInputDevice,
          kAudioObjectPropertyScopeGlobal,
          kAudioObjectPropertyElementMain
        };
        AudioObjectSetPropertyData(kAudioObjectSystemObject, &def, 0, NULL,
                                   sizeof(AudioDeviceID), &cdata->defdevin);
      }
      return -1;
    }
    csound->Message(csound, Str("auhal: device sampling rate set to %.1f\n"),
                    sr);

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
    /* the device may not take the requested size (e.g. Bluetooth) */
    psize = sizeof(granted);
    if (AudioObjectGetPropertyData(dev, &prop, 0, NULL, &psize, &granted)
        != noErr)
      granted = bufframes;
    if (granted != bufframes)
      csound->Message(csound,
                      Str("auhal: device buffer is %u frames, requested %u\n"),
                      (unsigned) granted, (unsigned) bufframes);
    /* The unit refuses renders larger than MaximumFramesPerSlice and stops
       calling back, so never set it below the device buffer, converted to
       our sampling rate for an output at another rate. */
    need = granted;
    if (sr > 0.0 && srate > sr)
      need = (UInt32) (granted * srate / sr) + 1;
    if (need < bufframes)
      need = bufframes;
    psize = sizeof(maxFPS);
    if (AudioUnitGetProperty(*aunit, kAudioUnitProperty_MaximumFramesPerSlice,
                             kAudioUnitScope_Global, isInput, &maxFPS, &psize)
        != noErr)
      maxFPS = 0;
    if (maxFPS < need) {
      maxFPS = need;
      AudioUnitSetProperty(*aunit, kAudioUnitProperty_MaximumFramesPerSlice,
                           kAudioUnitScope_Global, isInput, &maxFPS,
                           sizeof(UInt32));
    }
    /* the circular buffer holds -B frames, or at least one callback */
    ring = need > (UInt32) parm->bufSamp_HW ? (int32_t) need : parm->bufSamp_HW;
    if (!isInput && ring != parm->bufSamp_HW)
      csound->Message(csound,
                      Str("auhal: output buffer raised to %d frames for the "
                          "device\n"), ring);
    if (isInput) {
      cdata->incb = csound->CreateCircularBuffer(csound, ring * (int32_t) nchnls,
                                                 sizeof(cs_float));
    } else {
      cdata->outRing = ring;
      cdata->outcb = csound->CreateCircularBuffer(csound, ring * (int32_t) nchnls,
                                                  sizeof(cs_float));
    }
    /* set the stream properties */
    psize = sizeof(AudioStreamBasicDescription);
    AudioUnitGetProperty(*aunit, kAudioUnitProperty_StreamFormat,
                         (isInput ? kAudioUnitScope_Output : kAudioUnitScope_Input),
                         isInput, &format, &psize);
    format.mSampleRate    = srate;
    format.mFormatID =  kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat  |
                          kAudioFormatFlagIsPacked |
                          kLinearPCMFormatFlagIsNonInterleaved;
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

      if(O->msglevel || O->odebug)
       csound->Message(csound,
                      Str("***** AuHAL module: output device open with %d "
                          "buffer frames\n"),
                          granted);
      cdata->disp = 0;
    }
    else {
      AURenderCallbackStruct input;
      AudioBufferList *CAInputData =
        (AudioBufferList*)csound->Malloc(csound,sizeof(UInt32)
                                 + cdata->inchnls * sizeof(AudioBufferList));
      CAInputData->mNumberBuffers = cdata->inchnls;
      /* a callback renders its whole request at once */
      for (i = 0; i < cdata->inchnls; i++) {
        CAInputData->mBuffers[i].mNumberChannels = 1;
        CAInputData->mBuffers[i].mDataByteSize =
          maxFPS * sizeof(Float32);
        CAInputData->mBuffers[i].mData =
          csound->Calloc(csound,maxFPS*sizeof(Float32));
      }
      cdata->inputdata = CAInputData;
      cdata->inRender = (int32_t) maxFPS;

      input.inputProc = Csound_Input;
      input.inputProcRefCon = cdata;
      AudioUnitSetProperty(*aunit, kAudioOutputUnitProperty_SetInputCallback,
                           kAudioUnitScope_Input, isInput, &input, sizeof(input));
      AudioUnitInitialize(*aunit);
      AudioOutputUnitStart(*aunit);
      if(O->msglevel || O->odebug)
       csound->Message(csound,
                      Str("***** AuHAL module: input device open with "
                          "%d buffer frames\n"),
                      (int32_t) granted);
    }

    cdata->disp = 0;
    return 0;

}

int32_t listDevices(CSOUND *csound, CS_AUDIODEVICE *list, int32_t isOutput){
    UInt32  psize, devnos;
    AudioDeviceID *sysdevs;
    Device_Info *devinfo;
    int32_t     i;
    int32_t devouts = 0, devins = 0;

    AudioObjectPropertyAddress prop = {
      kAudioObjectPropertyName,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain
    };
    CFStringRef devName;
    prop.mSelector = kAudioHardwarePropertyDevices;
    AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                   &prop, 0, NULL, &psize);
    devnos = psize / sizeof(AudioDeviceID);
    sysdevs = (AudioDeviceID *) csound->Malloc(csound,psize);
    devinfo = (Device_Info *) csound->Malloc(csound,devnos*sizeof*devinfo);
    AudioObjectGetPropertyData(kAudioObjectSystemObject,
                               &prop, 0, NULL, &psize, sysdevs);

    for (i = 0; (uint32_t) i < devnos; i++) {
      AudioBufferList *b;
      int32_t devchannels, k, n;
      int32_t numlists;
      psize = sizeof(CFStringRef);
      prop.mScope = kAudioObjectPropertyScopeGlobal;
      prop.mSelector = kAudioObjectPropertyName;
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, &devName);
      memset(devinfo[i].name,0,128);
      if(!CFStringGetCString(devName, devinfo[i].name, 128,
                             kCFStringEncodingUTF8))
        devinfo[i].name[0] = '\0';
      CFRelease(devName);

      devchannels = 0;
      prop.mScope = kAudioDevicePropertyScopeInput;
      prop.mSelector =  kAudioDevicePropertyStreamConfiguration;
      AudioObjectGetPropertyDataSize(sysdevs[i],
                                     &prop, 0, NULL, &psize);
      b = (AudioBufferList *) csound->Malloc(csound,psize);
      numlists = psize / sizeof(AudioBufferList);
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, b);
      for(n=0; n < numlists; n++){
        for(k=0; (uint32_t)  k < b[n].mNumberBuffers; k++)
          devchannels += b[n].mBuffers[k].mNumberChannels;
      }
      devinfo[i].inchannels = devchannels;
      if(devchannels) {
        devins++;
        devinfo[i].indevnum = devins;
      } else devinfo[i].indevnum = -1;
      csound->Free(csound,b);

      devchannels = 0;
      prop.mScope = kAudioDevicePropertyScopeOutput;
      AudioObjectGetPropertyDataSize(sysdevs[i],
                                     &prop, 0, NULL, &psize);
      b = (AudioBufferList *) csound->Malloc(csound,psize);
      numlists = psize /sizeof(AudioBufferList);
      AudioObjectGetPropertyData(sysdevs[i],
                                 &prop, 0, NULL, &psize, b);
      for(n=0; n < numlists; n++){
        for(k=0; (uint32_t) k < b[n].mNumberBuffers; k++)
          devchannels += b[n].mBuffers[k].mNumberChannels;
      }
      devinfo[i].outchannels = devchannels;
      if(devchannels) {
        devouts++;
        devinfo[i].outdevnum = devouts;
      } else devinfo[i].outdevnum = -1;
      csound->Free(csound,b);
    }
    if(list==NULL){
      return (isOutput ? devouts : devins);
    } else {

      char tmp[64], *s;
      int32_t n=0, i;

      if ((s = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO")) == NULL)
        return 0;

      if(!isOutput){
        for(i=0; (uint32_t)  i < devnos; i++) {
          if(devinfo[i].inchannels) {
            strNcpy(list[n].device_name,  devinfo[i].name, 63);
            snprintf(tmp, 64, "adc%d", devinfo[i].indevnum);
            strNcpy(list[n].device_id, tmp, 63);
            strNcpy(list[n].rt_module, s, 63);
            list[n].max_nchnls = devinfo[i].inchannels;
            list[n].isOutput = 0;
            n++;
          }
        }
        return n;
      } else {
        for(i=0;(uint32_t) i < devnos; i++){
          if(devinfo[i].outchannels) {
            strNcpy(list[n].device_name,  devinfo[i].name, 63);
            snprintf(tmp, 64, "dac%d", devinfo[i].outdevnum);
            strNcpy(list[n].device_id, tmp, 63);
            strNcpy(list[n].rt_module, s, 63);
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
static int32_t recopen_(CSOUND *csound, const csRtAudioParams * parm)
{
    csdata  *cdata;
    void **recordata = csound->GetRtRecordUserData(csound);
    if (*(csound->GetRtRecordUserData(csound)) != NULL)
      return 0;

    /* allocate structure */

    if(*(csound->GetRtPlayUserData(csound) )!= NULL)
       cdata = (csdata *) *(csound->GetRtPlayUserData(csound));
    else {
       cdata = (csdata *) csound->Calloc(csound, sizeof(csdata));
      cdata->disp = 1;
    }

    cdata->inunit = NULL;
    *recordata = (void *) cdata;
    cdata->inParm =  (csRtAudioParams *) parm;
    cdata->csound = csound;
    cdata->inBufSamples = (int32_t) csound->GetInputBufferSize(csound);
    cdata->inputBuffer =
      (cs_float *) csound->Calloc(csound,
                               cdata->inBufSamples * sizeof(cs_float));
    /* the circular buffer is created by AuHAL_open */
    int32_t ret = AuHAL_open(csound, parm, cdata, 1);
    return ret;
}

/* open for audio output */
static int32_t playopen_(CSOUND *csound, const csRtAudioParams * parm)
{
    csdata  *cdata;
    void    **playdata = csound->GetRtPlayUserData(csound);

    if(*(csound->GetRtRecordUserData(csound)) != NULL)
      cdata = (csdata *) *(csound->GetRtRecordUserData(csound));
    else {
      cdata = (csdata *) csound->Calloc(csound, sizeof(csdata));
      cdata->disp = 1;
    }
    cdata->outunit = NULL;
    *playdata = (void *) cdata;
    cdata->outParm =  (csRtAudioParams *) parm;
    cdata->csound = csound;
    cdata->outBufSamples = (int32_t) csound->GetOutputBufferSize(csound);
    cdata->outputBuffer =
      (cs_float *) csound->Calloc(csound,
                               cdata->outBufSamples*sizeof(cs_float));
    memset(cdata->outputBuffer, 0,
           cdata->outBufSamples*sizeof(cs_float));

    /* the circular buffer is created by AuHAL_open */
    ATOMIC_SET(cdata->complete, 0);
    ATOMIC_SET(cdata->outLargest, 0);
    rt_audio_fade_reset(&cdata->closeFade);

    return AuHAL_open(csound, parm,cdata,0);
}


static OSStatus  Csound_Input(void *inRefCon,
                       AudioUnitRenderActionFlags *ioActionFlags,
                       const AudioTimeStamp *inTimeStamp,
                       UInt32 inBusNumber,
                       UInt32 inNumberFrames,
                       AudioBufferList *ioData){
    csdata *cdata = (csdata *) inRefCon;
    CSOUND *csound = cdata->csound;
    int32_t inchnls = cdata->inchnls;
    cs_float *inputBuffer = cdata->inputBuffer;
    /* The whole request is rendered at once and copied out in -b blocks;
       a request too large for the render buffers is recorded as silence. */
    UInt32 most = (UInt32) (cdata->inBufSamples/inchnls);
    UInt32 room = (UInt32) cdata->inRender;
    UInt32 done, frames;
    OSStatus err = -1;
    int32_t j,k,i,chns;
    Float32 *buffer;

    if ((int32_t) inNumberFrames > ATOMIC_GET(cdata->inLargest))
      ATOMIC_SET(cdata->inLargest, (int32_t) inNumberFrames);
    ioData = cdata->inputdata;
    if (inNumberFrames <= room) {
      /* AudioUnitRender leaves mDataByteSize at what it rendered */
      for (i = 0; i < ioData->mNumberBuffers; i++)
        ioData->mBuffers[i].mDataByteSize = inNumberFrames*sizeof(Float32);
      err = AudioUnitRender(cdata->inunit, ioActionFlags, inTimeStamp,
                            inBusNumber, inNumberFrames, ioData);
    }
    if (err != noErr) {
      memset(inputBuffer, 0, sizeof(cs_float)*cdata->inBufSamples);
      for (done = 0; done < inNumberFrames; done += frames) {
        frames = inNumberFrames - done < most ? inNumberFrames - done : most;
        csound->WriteCircularBuffer(csound, cdata->incb, inputBuffer,
                                    frames*inchnls);
      }
      return 0;
    }

    chns = ioData->mBuffers[0].mNumberChannels;
    for (done = 0; done < inNumberFrames; done += frames) {
      frames = inNumberFrames - done < most ? inNumberFrames - done : most;
      if(chns == 1) { // non-interleaved
        for (i = 0; i < ioData->mNumberBuffers; i++) {
          buffer = (Float32 *) ioData->mBuffers[i].mData + done;
          for(j = 0, k = 0; (uint32_t) k < frames; j+=inchnls, k++)
            inputBuffer[j+i] = (cs_float) buffer[k];
        }
      } else { // interleaved
        buffer = (Float32 *) ioData->mBuffers[0].mData + done*inchnls;
        for(k = 0; (uint32_t) k < frames*inchnls; k++)
          inputBuffer[k] = (cs_float) buffer[k];
      }
      csound->WriteCircularBuffer(csound, cdata->incb, inputBuffer,
                                  frames*inchnls);
    }
    return 0;
}
#define slt 100

/* Waits of slt us for the device in rtplay_ (which = 1) and rtrecord_ (2):
   1 s, 5 s before its first callback. Once it has stalled, one block's
   duration, so the performance keeps time and can still be stopped; none for
   an input when an output paces the performance. */
static int32_t naps(csdata *cdata, int32_t samples, int32_t chnls, int32_t which)
{
    int32_t called = which == 1 ? ATOMIC_GET(cdata->outLargest)
                                : ATOMIC_GET(cdata->inLargest);
    if (!(cdata->stalled & which) || cdata->sr <= 0.0 || chnls <= 0)
      return (called ? 1 : 5) * (1000000 / slt);
    if (which == 2 && cdata->outunit != NULL)
      return 0;
    return (int32_t) (1e6 * samples / chnls / cdata->sr / slt) + 1;
}

static int32_t rtrecord_(CSOUND *csound, cs_float *inbuff_, int32_t nbytes)
{
    csdata  *cdata;
    int32_t n = nbytes/sizeof(cs_float);
    int32_t m = 0, l, left;
    cdata = (csdata *) *(csound->GetRtRecordUserData(csound));
    if (!(cdata->warned & 2) && ATOMIC_GET(cdata->inLargest) > cdata->inRender) {
      cdata->warned |= 2;
      csound->Warning(csound,
                      Str("auhal: input callback of %d frames exceeds the "
                          "render buffer (%d), recording silence\n"),
                      ATOMIC_GET(cdata->inLargest), cdata->inRender);
    }
    left = naps(cdata, n, cdata->inchnls, 2);
    do{
      l = csound->ReadCircularBuffer(csound,cdata->incb,&inbuff_[m],n);
      m += l;
      n -= l;
      if (l > 0)
        cdata->stalled &= ~2;
      if(n) {
        if (--left < 0) {
          memset(&inbuff_[m], 0, n*sizeof(cs_float));
          if (!(cdata->stalled & 2)) {
            cdata->stalled |= 2;
            if (!(cdata->warned & 8)) {
              cdata->warned |= 8;
              csound->Warning(csound,
                              Str("auhal: input device stopped, recording "
                                  "silence\n"));
            }
          }
          break;
        }
        usleep(slt);
      }
    } while(n);

    return nbytes;
}

static OSStatus Csound_Render(void *inRefCon,
    /* Matches the AudioUnit callback signature. */
    /* NOLINTNEXTLINE(readability-non-const-parameter) */
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp,
                        UInt32 inBusNumber,
                        UInt32 inNumberFrames,
                        AudioBufferList *ioData) {
    csdata *cdata = (csdata *) inRefCon;
    CSOUND *csound = cdata->csound;
    int32_t onchnls = cdata->onchnls;
    cs_float *outputBuffer = cdata->outputBuffer;
    /* the device may request more than -b frames: fill in -b blocks */
    UInt32 most = (UInt32) (cdata->outBufSamples/onchnls);
    UInt32 done, frames;
    int32_t closing = ATOMIC_GET(cdata->complete);
    int32_t j,k,i,n,chns;
    Float32 *buffer;
    IGN(ioActionFlags);
    IGN(inTimeStamp);
    IGN(inBusNumber);

    if ((int32_t) inNumberFrames > ATOMIC_GET(cdata->outLargest))
      ATOMIC_SET(cdata->outLargest, (int32_t) inNumberFrames);
    chns = ioData->mBuffers[0].mNumberChannels;
    for (done = 0; done < inNumberFrames; done += frames) {
      frames = inNumberFrames - done < most ? inNumberFrames - done : most;
      memset(outputBuffer, 0, sizeof(cs_float)*frames*onchnls);
      /* Once closing, fade the audio still queued out to silence, as
         Ardour does when stopping its engine, so the stream never ends
         on a non-zero sample. The fade spans the queued audio and so
         reaches zero exactly when the buffer drains. */
      if (closing && cdata->closeFade.lengthFrames == 0) {
        int32_t queued = csound->CheckCircularBuffer(csound, cdata->outcb, 0);
        rt_audio_fade_begin(&cdata->closeFade, queued, onchnls);
      }
      n = csound->ReadCircularBuffer(csound,cdata->outcb,outputBuffer,
                                     frames*onchnls);
      if (closing && n > 0)
        rt_audio_fade_apply(&cdata->closeFade, outputBuffer, n, onchnls);

      if(chns == 1) { // non-interleaved
        for (i = 0; i < ioData->mNumberBuffers; i++) {
          buffer = (Float32 *) ioData->mBuffers[i].mData + done;
          for(j = 0, k = 0; (uint32_t) k < frames; j+=onchnls, k++)
            buffer[k] = (Float32) outputBuffer[j+i];
        }
      } else { // interleaved
        buffer = (Float32 *) ioData->mBuffers[0].mData + done*onchnls;
        for(k = 0; (uint32_t) k < frames*onchnls; k++)
          buffer[k] = (Float32) outputBuffer[k];
      }
    }
    return 0;
}

static void rtplay_(CSOUND *csound, const cs_float *outbuff_, int32_t nbytes)
{
    csdata  *cdata;
    int32_t n = nbytes/sizeof(cs_float);
    int32_t m = 0, l, left;
    cdata = (csdata *) *(csound->GetRtPlayUserData(csound));
    if (!(cdata->warned & 1) && ATOMIC_GET(cdata->outLargest) > cdata->outRing) {
      cdata->warned |= 1;
      csound->Warning(csound,
                      Str("auhal: device requested %d frames, more than the "
                          "output buffer (%d), expect dropouts\n"),
                      ATOMIC_GET(cdata->outLargest), cdata->outRing);
    }
    left = naps(cdata, n, cdata->onchnls, 1);
    do {
      l = csound->WriteCircularBuffer(csound, cdata->outcb,&outbuff_[m],n);
      m += l;
      n -= l;
      if (l > 0)
        cdata->stalled &= ~1;
      if(n) {
        if (--left < 0) {
          if (!(cdata->stalled & 1)) {
            cdata->stalled |= 1;
            if (!(cdata->warned & 4)) {
              cdata->warned |= 4;
              csound->Warning(csound,
                              Str("auhal: output device stopped, dropping "
                                  "output\n"));
            }
          }
          break;
        }
        usleep(slt);
      }
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
      if (cdata->outunit != NULL) {
        /* Mark closing so the render callback fades the queued audio out. */
        ATOMIC_SET(cdata->complete, 1);

        /* Wait until the callback has consumed the circular buffer.  The
           timeout covers the requested queue and the largest render
           buffer, while the second wait lets the final render block
           reach the device. */
        int32_t waitMs = 100;
        int32_t queued = csound->CheckCircularBuffer(csound, cdata->outcb, 0);
        cs_double renderMs = 0.0;
        if (cdata->sr > 0.0 && cdata->onchnls > 0) {
          int32_t render = ATOMIC_GET(cdata->outLargest);
          cs_double queueMs = 1000.0 * (cs_double) queued /
                           ((cs_double) cdata->onchnls * (cs_double) cdata->sr);
          if (render < cdata->outBufSamples/cdata->onchnls)
            render = cdata->outBufSamples/cdata->onchnls;
          renderMs = 1000.0 * (cs_double) render / (cs_double) cdata->sr;
          waitMs = (int32_t) (queueMs + renderMs + 20.0);
          if (waitMs < 50)
            waitMs = 50;
        }
        do {
          if (queued == 0)
            break;
          csound->Sleep(1);
          queued = csound->CheckCircularBuffer(csound, cdata->outcb, 0);
        } while (--waitMs > 0);
        if (queued != 0)
          queued = csound->CheckCircularBuffer(csound, cdata->outcb, 0);
        if (queued == 0)
          csound->Sleep((size_t) (renderMs + 10.0));
      }

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
        csound->Free(csound,cdata->outputBuffer);
        cdata->outputBuffer = NULL;
      }
      if (cdata->inputBuffer != NULL) {
        csound->Free(csound,cdata->inputBuffer);
        cdata->inputBuffer = NULL;
      }

      *(csound->GetRtRecordUserData(csound)) = NULL;
      *(csound->GetRtPlayUserData(csound)) = NULL;

      if(cdata->inputdata) {
        int32_t i;
        for (i = 0; i < cdata->inchnls; i++)
          csound->Free(csound,cdata->inputdata->mBuffers[i].mData);
        csound->Free(csound,cdata->inputdata);
      }

      if(cdata->defdevin) {
        AudioObjectPropertyAddress prop = {
          kAudioHardwarePropertyDefaultInputDevice,
          kAudioObjectPropertyScopeGlobal,
          kAudioObjectPropertyElementMain
        };
        UInt32 psize = sizeof(AudioDeviceID);
        AudioObjectSetPropertyData(kAudioObjectSystemObject,
                                   &prop, 0, NULL, psize, &cdata->defdevin);
      }
      if(cdata->defdevout) {
        AudioObjectPropertyAddress prop = {
          kAudioHardwarePropertyDefaultOutputDevice,
          kAudioObjectPropertyScopeGlobal,
          kAudioObjectPropertyElementMain
        };
        UInt32 psize = sizeof(AudioDeviceID);
        AudioObjectSetPropertyData(kAudioObjectSystemObject,
                                   &prop, 0, NULL, psize, &cdata->defdevout);
      }
      csound->DestroyCircularBuffer(csound, cdata->incb);
      csound->DestroyCircularBuffer(csound, cdata->outcb);
      csound->Free(csound,cdata);
      csound->DebugMsg(csound, "%s\n", Str("AuHAL module: device closed\n"));
    }
}

int32_t csoundModuleInit(CSOUND *csound)
{
    char   *drv;
    csound->ModuleListAdd(csound, "auhal", "audio");
    drv = (char *) csound->QueryGlobalVariable(csound, "_RTAUDIO");
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "auhal") == 0 || strcmp(drv, "AuHal") == 0 ||
          strcmp(drv, "AUHAL") == 0 ||
          strcmp(drv, "coreaudio") == 0 || strcmp(drv, "CoreAudio") == 0 ||
          strcmp(drv, "COREAUDIO") == 0))
      return 0;
    csound->DebugMsg(csound, "%s\n", Str("rtaudio: coreaaudio-AuHAL module enabled\n"));
    csound->SetPlayopenCallback(csound, playopen_);
    csound->SetRecopenCallback(csound, recopen_);
    csound->SetRtplayCallback(csound, rtplay_);
    csound->SetRtrecordCallback(csound, rtrecord_);
    csound->SetRtcloseCallback(csound, rtclose_);
    csound->SetAudioDeviceListCallback(csound, listDevices);
    return 0;
}

PUBLIC int32_t csoundModuleInfo(void)
{
  return CSOUND_MODULE_INFO;
}

int32_t csoundModuleCreate(CSOUND *csound)
{
    IGN(csound);
    return 0;
}
