/*
  rtcoreaudio.c:

  Copyright (C) 2005 Victor Lazzarini

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

/* this code is deprecated, has been substituted by the rtauhal.c module */


#include <CoreAudio.h>
#include <unistd.h>
#include <stdint.h>
#include "csdl.h"
#include "soundio.h"


typedef struct devparams_ {
    AudioDeviceID dev;
    float **inbuffs;
    float **outbuffs;
    AudioStreamBasicDescription format;
    unsigned int bufframes;
    unsigned int buffnos;
    unsigned int outcurbuff;
    unsigned int incurbuff;
    unsigned int iocurbuff;
    unsigned int outcount;
    unsigned int incount;
    int    *inused;
    int    *outused;
    float   srate;
    int     nchns;
    int     isNInterleaved;
#if defined(MAC_OS_X_VERSION_10_5) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_5)
    AudioDeviceIOProcID     procID;
#else
  int procID;
#endif
} DEVPARAMS;

/* module interface functions */

int csoundModuleCreate(CSOUND *csound)
{
    int     min, max, *def;
    CSOUND *p = csound;

    min = 2;
    max = 32;

    p->CreateGlobalVariable(csound, "::cabuffnos", sizeof(int));
    def = (int *) (p->QueryGlobalVariable(csound, "::cabuffnos"));
    if (def == NULL)
      p->Message(csound, Str("warning... could not create global var\n"));
    else
      *def = 4;
    p->CreateConfigurationVariable(csound, "buffnos", def,
                                   CSOUNDCFG_INTEGER, 0, &min, &max,
                                   "coreaudio IO buffer numbers", NULL);

    p->CreateGlobalVariable(csound, "::cainterleaved", sizeof(int));
    def = (int *) (p->QueryGlobalVariable(csound, "::cainterleaved"));
    if (def == NULL)
      p->Message(csound, Str("warning... could not create global var\n"));
    else
      *def = 0;
    p->CreateConfigurationVariable(csound, "noninterleaved", def,
                                   CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                   "deprecated, noninterleaved audio is automatically detected",
                                   NULL);

    if (csound->oparms->msglevel & 0x400)
      p->Message(csound, Str("CoreAudio real-time audio module for Csound\n"
                             "by Victor Lazzarini\n"));
    return 0;
}

static int playopen_(CSOUND *, const csRtAudioParams *);
static int recopen_(CSOUND *, const csRtAudioParams *);
static void rtplay_(CSOUND *, const MYFLT *, int);
static int rtrecord_(CSOUND *, MYFLT *, int);
static void rtclose_(CSOUND *);

int csoundModuleInit(CSOUND *csound)
{
    char   *drv;

    drv = (char *) csound->QueryGlobalVariable(csound, "_RTAUDIO");
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "coreaudio") == 0 || strcmp(drv, "CoreAudio") == 0 ||
          strcmp(drv, "COREAUDIO") == 0))
      return 0;
    if (csound->oparms->msglevel & 0x400)
      csound->Message(csound, Str("rtaudio: CoreAudio module enabled\n"));
    csound->SetPlayopenCallback(csound, playopen_);
    csound->SetRecopenCallback(csound, recopen_);
    csound->SetRtplayCallback(csound, rtplay_);
    csound->SetRtrecordCallback(csound, rtrecord_);
    csound->SetRtcloseCallback(csound, rtclose_);
    return 0;
}

OSStatus
ADIOProc(const AudioBufferList * input,
           AudioBufferList * output, DEVPARAMS * cdata)
{
    int     i, j, cnt;
    int     chans = cdata->nchns;
    int     cachans = output->mBuffers[0].mNumberChannels;
    int     inchans = input->mBuffers[0].mNumberChannels;
    int     items;
    int     buff = cdata->iocurbuff;
    float  *outp, *inp;
    float  *ibufp = cdata->inbuffs[buff];
    float  *obufp = cdata->outbuffs[buff];

    if (input->mNumberBuffers > 1) cdata->isNInterleaved = 1;

    if (cdata->isNInterleaved) {

      int     nibuffs = input->mNumberBuffers, buffs;
      int     nobuffs = output->mNumberBuffers;

      items = cdata->bufframes * chans;
      buffs = nibuffs > nobuffs ? nibuffs : nobuffs;
      output->mNumberBuffers = buffs;
      chans = chans > buffs ? buffs : chans;
      for (j = 0; j < chans; j++) {
        outp = (float *) output[0].mBuffers[j].mData;
        inp = (float *) input[0].mBuffers[j].mData;

        for (i = j, cnt = 0; i < items; i += chans, cnt++) {
          outp[cnt] = obufp[i];
          obufp[i] = 0.0f;
          ibufp[i] = inp[cnt];
        }
        output->mBuffers[j].mDataByteSize = input[0].mBuffers[j].mDataByteSize;
        output->mBuffers[j].mNumberChannels = 1;
      }
    }
    else {
      items = cdata->bufframes * cachans;
      output->mNumberBuffers = 1;
      output->mBuffers[0].mDataByteSize = items * sizeof(float);
      output->mBuffers[0].mNumberChannels = cachans;
      outp = (float *) output->mBuffers[0].mData;
      inp = (float *) input->mBuffers[0].mData;
      for (i = 0, cnt = 0; i < items; i += cachans) {
        for (j = 0; j < cachans; j++)
          if (j < chans) {
            outp[i + j] = obufp[cnt];
            obufp[cnt] = 0.0f;
            if (inp != NULL && j < inchans)
              ibufp[cnt] = inp[i + j];
            cnt++;
          }
          else
            outp[i + j] = 0.0f;
      }
    }
    cdata->outused[buff] = cdata->inused[buff] = 1;
    buff++;
    buff %= cdata->buffnos;
    cdata->iocurbuff = buff;

    return 0;
}

OSStatus Csound_IOProcEntry(AudioDeviceID indev,
                              const AudioTimeStamp * inN,
                              const AudioBufferList * input,
                              const AudioTimeStamp * inT,
                              AudioBufferList * output,
                              const AudioTimeStamp * inOT, void *cdata)
{
    return ADIOProc(input, output, (DEVPARAMS *) cdata);
}

int coreaudio_open(CSOUND *csound, const csRtAudioParams * parm,
                     DEVPARAMS * dev, int isInput)
{

    CSOUND *p;
    UInt32  psize, devnum, devnos;
    double  sr;
    AudioDeviceID *sysdevs;
    AudioStreamBasicDescription format;
    int     i, bfns, *vpt;
    char   *name;
    UInt32  obufframes, ibufframes, buffbytes;

    memset(dev, 0, sizeof(DEVPARAMS));
    p = (CSOUND *) csound;
    /* set up parameters */
    vpt = (int *) (p->QueryGlobalVariable(csound, "::cabuffnos"));
    if (vpt != NULL)
      bfns = *vpt;
    else
      bfns = 4;

    p->DeleteConfigurationVariable(csound, "buffnos");
    p->DestroyGlobalVariable(csound, "::cabuffnos");

    vpt = (int *) (p->QueryGlobalVariable(csound, "::cainterleaved"));

    /*
     if (vpt != NULL)
      dev->isNInterleaved = *vpt;
    else
      dev->isNInterleaved = 0;
    */

    p->DeleteConfigurationVariable(csound, "interleaved");
    p->DestroyGlobalVariable(csound, "::cainterleaved");

    psize = sizeof(AudioDeviceID);
    AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice,
                             &psize, &dev->dev);

    AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &psize, NULL);
    devnos = psize / sizeof(AudioDeviceID);
    sysdevs = (AudioDeviceID *) malloc(psize);
    AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &psize, sysdevs);
    if (csound->oparms->msglevel & 0x400) {
      p->Message(csound,
                 "==========================================================\n"
                 "CoreAudio Module: found %d device(s):\n", (int) devnos);

      for (i = 0; (unsigned int) i < devnos; i++) {

        AudioDeviceGetPropertyInfo(sysdevs[i], 1, false,
                                   kAudioDevicePropertyDeviceName, &psize, NULL);
        name = (char *) malloc(psize);
        AudioDeviceGetProperty(sysdevs[i], 1, false,
                               kAudioDevicePropertyDeviceName, &psize, name);
        p->Message(csound, "=> CoreAudio device %d: %s \n", i, name);
        free(name);
      }
      if (parm->devName != NULL) {
        devnum = atoi(parm->devName);
        if (devnum >= 0 && devnum < devnos)
          dev->dev = sysdevs[devnum];
        p->Message(csound, Str("selected device: %u \n"), (unsigned int) devnum);
        free(sysdevs);
      }

      AudioDeviceGetPropertyInfo(dev->dev, 1, false,
                               kAudioDevicePropertyDeviceName, &psize, NULL);
      name = (char *) malloc(psize);
      AudioDeviceGetProperty(dev->dev, 1, false, kAudioDevicePropertyDeviceName,
                             &psize, name);
      p->Message(csound, Str("CoreAudio module: opening %s \n"), name);
      free(name);
    }
    dev->srate = (float) (parm->sampleRate);
    dev->nchns = parm->nChannels;
    dev->bufframes = parm->bufSamp_HW;
    dev->buffnos = bfns;

    psize = 4;
    /* set the buffer size
       output */
    AudioDeviceSetProperty(dev->dev, NULL, 0, false,
                           kAudioDevicePropertyBufferFrameSize,
                           psize, &dev->bufframes);
    /* input */
    AudioDeviceSetProperty(dev->dev, NULL, 0, true,
                           kAudioDevicePropertyBufferFrameSize,
                           psize, &dev->bufframes);

    /* check that it matches the expected size */
    AudioDeviceGetProperty(dev->dev, 0, true,
                           kAudioDevicePropertyBufferFrameSize,
                           &psize, &ibufframes);

    AudioDeviceGetProperty(dev->dev, 0, false,
                           kAudioDevicePropertyBufferFrameSize,
                           &psize, &obufframes);

    if (ibufframes != dev->bufframes) {
      if (ibufframes == obufframes)
        dev->bufframes = obufframes;
      else {
        free(dev);
        csound->rtRecord_userdata = NULL;
        p->Message(csound, Str(" *** CoreAudio: open: could not set buffer size\n"));
        return -1;
      }
    }
    psize = sizeof(double);
    sr = dev->srate;
    AudioDeviceSetProperty(dev->dev, NULL, 0, true,
                           kAudioDevicePropertyNominalSampleRate, psize, &sr);
    AudioDeviceSetProperty(dev->dev, NULL, 0, false,
                           kAudioDevicePropertyNominalSampleRate, psize, &sr);

    psize = sizeof(AudioStreamBasicDescription);
    AudioDeviceGetProperty(dev->dev, 0, false,
                           kAudioDevicePropertyStreamFormat, &psize, &format);

    dev->isNInterleaved = 0;

    dev->format.mSampleRate = dev->srate;
    dev->format.mFormatID = kAudioFormatLinearPCM;
    dev->format.mFormatFlags = kAudioFormatFlagIsFloat | format.mFormatFlags;
    dev->format.mBytesPerPacket = sizeof(float) * dev->nchns;
    dev->format.mFramesPerPacket = 1;
    dev->format.mBytesPerFrame = format.mBytesPerPacket;
    dev->format.mChannelsPerFrame = dev->nchns;
    dev->format.mBitsPerChannel = sizeof(float);

    psize = sizeof(AudioStreamBasicDescription);

    AudioDeviceSetProperty(dev->dev, NULL, 0, true,
                           kAudioDevicePropertyStreamFormat,
                           psize, &dev->format);
    AudioDeviceSetProperty(dev->dev, NULL, 0, false,
                           kAudioDevicePropertyStreamFormat,
                           psize, &dev->format);
    AudioDeviceGetProperty(dev->dev, 0, false,
                           kAudioDevicePropertyStreamFormat, &psize, &format);

    if (format.mChannelsPerFrame != (unsigned int) dev->nchns &&
        !dev->isNInterleaved) {
      dev->format.mChannelsPerFrame = format.mChannelsPerFrame;
    }

    if (format.mSampleRate != dev->srate) {
      csound->rtRecord_userdata = NULL;
      p->Message(csound,
                 Str(" *** CoreAudio: open: could not set device parameter sr: %d \n"),
                 (int) dev->srate);
      p->Message(csound, Str(" *** CoreAudio: current device sampling rate is:%d \n"
                             "     try setting the above value in your csound orchestra \n"),
                 (int) format.mSampleRate);
      free(dev);
      return -1;
    }
    else{

      p->Message(csound,
                 Str("CoreAudio module: sr set to %d with %d audio channels \n"),
                 (int) dev->srate, (int) dev->nchns);
    }

    dev->outbuffs = (float **) malloc(sizeof(float *) * dev->buffnos);
    dev->inbuffs = (float **) malloc(sizeof(float *) * dev->buffnos);
    dev->inused = (int *) malloc(sizeof(int) * dev->buffnos);
    dev->outused = (int *) malloc(sizeof(int) * dev->buffnos);

    buffbytes = dev->bufframes * dev->nchns * sizeof(float);

    for (i = 0; (unsigned int) i < dev->buffnos; i++) {

      if ((dev->inbuffs[i] = (float *) malloc(buffbytes)) == NULL) {
        free(dev->outbuffs);
        free(dev->inbuffs);
        free(dev->inused);
        free(dev->outused);
        free(dev);
        csound->rtRecord_userdata = NULL;
        p->Message(csound,
                   Str(" *** CoreAudio: open: memory allocation failure\n"));
        return -1;
      }
      memset(dev->inbuffs[i], 0, buffbytes);

      if ((dev->outbuffs[i] = (float *) malloc(buffbytes)) == NULL) {
        free(dev->outbuffs);
        free(dev->inbuffs);
        free(dev->inused);
        free(dev->outused);
        free(dev);
        csound->rtRecord_userdata = NULL;
        p->Message(csound,
                   Str(" *** CoreAudio: open: memory allocation failure\n"));
        return -1;
      }
      memset(dev->outbuffs[i], 0, buffbytes);
      dev->inused[i] = dev->outused[i] = 1;
    }

    dev->incurbuff = dev->outcurbuff = dev->iocurbuff = 0;
    dev->incount = dev->outcount = 0;
    dev->procID = 0;

    //

#if defined(MAC_OS_X_VERSION_10_5) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_5)
    AudioDeviceCreateIOProcID(dev->dev,Csound_IOProcEntry,dev,&dev->procID);
#else
    AudioDeviceAddIOProc(dev->dev, Csound_IOProcEntry, dev);
#endif
    AudioDeviceStart(dev->dev,Csound_IOProcEntry);


    if (isInput)
      csound->rtPlay_userdata = (void *) dev;
    else
      csound->rtRecord_userdata = (void *) dev;

    p->Message(csound,
               Str("CoreAudio module: device open with %d buffers of %d frames\n"
                   "==========================================================\n"),
               dev->buffnos, dev->bufframes);
    return 0;
}

/* open for audio input */
static int recopen_(CSOUND *csound, const csRtAudioParams * parm)
{
    CSOUND *p;
    DEVPARAMS *dev;

    p = (CSOUND *) csound;
    if (csound->rtRecord_userdata != NULL)
      return 0;
    /* allocate structure */
    dev = (DEVPARAMS *) malloc(sizeof(DEVPARAMS));
    if (dev == NULL) {
      p->Message(csound, Str(" *** CoreAudio: open: memory allocation failure\n"));
      return -1;
    }
    csound->rtRecord_userdata = (void *) dev;

    return coreaudio_open(csound, parm, dev, 1);
}

/* open for audio output */
static int playopen_(CSOUND *csound, const csRtAudioParams * parm)
{
    CSOUND *p;
    DEVPARAMS *dev;

    p = (CSOUND *) csound;
    if (csound->rtPlay_userdata != NULL)
      return 0;
    /* allocate structure */
    dev = (DEVPARAMS *) malloc(sizeof(DEVPARAMS));
    if (dev == NULL) {
      p->Message(csound, Str(" *** CoreAudio: open: memory allocation failure\n"));
      return -1;
    }
    csound->rtPlay_userdata = (void *) dev;

    return coreaudio_open(csound, parm, dev, 0);
}

static int rtrecord_(CSOUND *csound, MYFLT *inbuf_, int bytes_)
{
    DEVPARAMS *dev;
    CSOUND *p;
    int     n, i, chans, cur, icount, buffitems, buffnos, *inused, usecs;
    float **ibuffs;

    /* MYFLT norm; */
    p = (CSOUND *) csound;
    dev = (DEVPARAMS *) (csound->rtRecord_userdata);
    usecs = (int) (1000 * dev->bufframes / p->esr);
    n = bytes_ / sizeof(MYFLT);
    chans = dev->nchns;
    ibuffs = dev->inbuffs;
    cur = dev->incurbuff;
    inused = dev->inused;
    icount = dev->incount;
    buffnos = dev->buffnos;
    buffitems = dev->bufframes * chans;
    /* norm = p->e0dbfs;  */

    for (i = 0; i < n; i++) {
      ((MYFLT *) inbuf_)[i] = ibuffs[cur][icount];
      icount++;
      if (icount == buffitems) {
        inused[cur] = 0;
        cur++;
        cur %= buffnos;
        icount = 0;
        while (!inused[cur])
          usleep(usecs);
      }
    }                           /* for */
    dev->incount = icount;
    dev->incurbuff = cur;

    return bytes_;

}

/* put samples to DAC */

static void rtplay_(CSOUND *csound, const MYFLT *outbuf_, int bytes_)
{
    DEVPARAMS *dev;
    CSOUND *p;
    int     n, i, chans, cur, ocount, buffitems, buffnos, *outused, usecs;
    float **obuffs;

    /* MYFLT norm; */
    p = (CSOUND *) csound;
    dev = (DEVPARAMS *) (csound->rtRecord_userdata);

    n = bytes_ / sizeof(MYFLT);
    usecs = (int) (1000 * dev->bufframes / p->esr);
    chans = dev->nchns;
    obuffs = dev->outbuffs;
    cur = dev->outcurbuff;
    outused = dev->outused;
    ocount = dev->outcount;
    buffnos = dev->buffnos;
    buffitems = dev->bufframes * chans;
    /* norm = p->e0dbfs; */

    for (i = 0; i < n; i++) {
      obuffs[cur][ocount] = (float) outbuf_[i];
      ocount++;
      if (ocount == buffitems) {
        outused[cur] = 0;
        cur++;
        cur %= buffnos;
        ocount = 0;
        while (!outused[cur])
          usleep(usecs);
      }
    }

    dev->outcurbuff = cur;
    dev->outcount = ocount;
}

/* close the I/O device entirely  */
/* called only when both complete */

static void rtclose_(CSOUND *csound)
{
    DEVPARAMS *dev;
    CSOUND *p;

    p = (CSOUND *) csound;
    dev = (DEVPARAMS *) (csound->rtRecord_userdata);
    if (dev != NULL) {
      p->Message(csound, Str("coreaudio module: closing device...\n"));
      sleep(1);
      AudioDeviceStop(dev->dev,Csound_IOProcEntry );
#if defined(MAC_OS_X_VERSION_10_5) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_5)
      AudioDeviceDestroyIOProcID(dev->dev, dev->procID);
#else
      AudioDeviceRemoveIOProc(dev->dev, Csound_IOProcEntry);
#endif
      csound->rtRecord_userdata = NULL;
      free(dev->outbuffs);
      free(dev->inbuffs);
      free(dev->inused);
      free(dev->outused);
      free(dev);
      p->Message(csound, Str("coreaudio module: device closed\n"));
    }
}

