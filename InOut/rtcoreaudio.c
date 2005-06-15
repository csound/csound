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

#include <CoreAudio.h>
#include <unistd.h>
#include <stdint.h>
#include "csoundCore.h"
#include "csound.h"
#include "soundio.h"

typedef struct devparams_ {
  AudioDeviceID dev;
  float** inbuffs;
  float** outbuffs;
  AudioStreamBasicDescription format;
  unsigned int bufframes;
  unsigned int buffnos;
  unsigned int outcurbuff;
  unsigned int incurbuff;
  unsigned int iocurbuff;
  unsigned int outcount;
  unsigned int incount;
  int*     inused;
  int*     outused;
  float    srate;
  int      nchns;
} DEVPARAMS;

/* module interface functions */

int csoundModuleCreate(void *csound)
{
    int min, max, *def;
    ENVIRON *p = csound;
    min = 2;
    max = 32;

    p->CreateGlobalVariable(csound, "::cabuffnos", sizeof(int));
    def =(int*) (p->QueryGlobalVariable(csound, "::cabuffnos"));
    if(def==NULL) p->Message(csound, "warning... could not create global var\n");
    else *def = 4;
    p->CreateConfigurationVariable(csound, "buffnos", def,
                                   CSOUNDCFG_INTEGER, 0, &min, &max,
                                   "coreaudio IO buffer numbers", NULL);

    p->Message(csound, "CoreAudio real-time audio module for Csound\n"
               "by Victor Lazzarini\n");
    return 0;
}

static int playopen_(void*, csRtAudioParams*);
static int recopen_(void*, csRtAudioParams*);
static void rtplay_(void*, void*, int);
static int rtrecord_(void*, void*, int);
static void rtclose_(void*);

int csoundModuleInit(void *csound)
{
    ENVIRON *p;
    char    *drv;

    p = (ENVIRON*) csound;
    drv = (char*) (p->QueryGlobalVariable(csound, "_RTAUDIO"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "coreaudio") == 0 || strcmp(drv, "CoreAudio") == 0 ||
          strcmp(drv, "COREAUDIO") == 0))
      return 0;
    p->Message(csound, "rtaudio: CoreAudio module enabled\n");
    p->SetPlayopenCallback(csound, playopen_);
    p->SetRecopenCallback(csound, recopen_);
    p->SetRtplayCallback(csound, rtplay_);
    p->SetRtrecordCallback(csound, rtrecord_);
    p->SetRtcloseCallback(csound, rtclose_);
    return 0;
}

OSStatus
ADIOProc(const AudioBufferList *input,
         AudioBufferList *output,
         DEVPARAMS* cdata){
  int i,j,cnt;
  int chans = cdata->nchns;
  int cachans = input->mBuffers[0].mNumberChannels;
  int items = cdata->bufframes*cachans;
  int buff = cdata->iocurbuff;
  output->mNumberBuffers = 1;
  output->mBuffers[0].mDataByteSize = items*sizeof(float);
  output->mBuffers[0].mNumberChannels = cachans;
  float *outp = (float *) output->mBuffers[0].mData;
  float *inp = (float *) input->mBuffers[0].mData;
  float *ibufp = cdata->inbuffs[buff];
  float *obufp = cdata->outbuffs[buff];

  for(i = 0, cnt = 0; i < items; i+=cachans){
    for(j=0; j < cachans; j++)
      if(j < chans){
        outp[i+j]  = obufp[cnt];
        if(inp!=NULL) ibufp[cnt] = inp[i+j];
        cnt++;
      }

  }

  cdata->outused[buff] = cdata->inused[buff] = 1;
  buff++;
  buff %= cdata->buffnos;
  cdata->iocurbuff = buff;

  return 0;
}

OSStatus Csound_IOProcEntry(AudioDeviceID indev,
                            const AudioTimeStamp *inN,
                            const AudioBufferList *input,
                            const AudioTimeStamp *inT, AudioBufferList *output,
                            const AudioTimeStamp *inOT, void* cdata){

    return ADIOProc(input,output,(DEVPARAMS *)cdata);

}

int coreaudio_open(void *csound, csRtAudioParams *parm, DEVPARAMS *dev,int isInput)
{

    ENVIRON   *p;
    UInt32 psize, devnum, devnos;
    double sr;
    AudioDeviceID *sysdevs;
    AudioStreamBasicDescription format;
    int i, bfns, *vpt;
    char* name;
    UInt32 obufframes, ibufframes, buffbytes;

    memset(dev, 0, sizeof(DEVPARAMS));
    p = (ENVIRON*) csound;
    /* set up parameters */
    vpt= (int*) (p->QueryGlobalVariable(csound, "::cabuffnos"));
    if(vpt != NULL) bfns = *vpt;
    else bfns = 4;

    p->DeleteConfigurationVariable(csound, "buffnos");
    p->DestroyGlobalVariable(csound, "::cabuffnos");

    psize = sizeof(AudioDeviceID);
    AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice,
                             &psize, &dev->dev);

    AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,
                                 &psize, NULL);
    devnos = psize/sizeof(AudioDeviceID);
    sysdevs = (AudioDeviceID *) malloc(psize);
    AudioHardwareGetProperty(kAudioHardwarePropertyDevices,
                             &psize, sysdevs);
    p->Message(csound,
               "==========================================================\n"
               "CoreAudio Module: found %d device(s):\n", (int)devnos);
    for(i=0; (unsigned int)i < devnos; i++){
      AudioDeviceGetPropertyInfo(sysdevs[i],1,false,
                                 kAudioDevicePropertyDeviceName,
                                 &psize, NULL);
      name = (char *) malloc(psize);
      AudioDeviceGetProperty(sysdevs[i],1,false, kAudioDevicePropertyDeviceName,
                             &psize, name);
      p->Message(csound, "=> CoreAudio device %d: %s \n", i, name);
      free(name);
    }
    if(parm->devName!=NULL){
      devnum = atoi(parm->devName);
      if(devnum >= 0 && devnum < devnos) dev->dev = sysdevs[devnum];
      free(sysdevs);
    }

    AudioDeviceGetPropertyInfo(dev->dev,1,false, kAudioDevicePropertyDeviceName,
                               &psize, NULL);
    name = (char *) malloc(psize);
    AudioDeviceGetProperty(dev->dev,1,false, kAudioDevicePropertyDeviceName,
                           &psize, name);
    p->Message(csound, "CoreAudio module: opening %s \n",
               name);
    free(name);

    dev->srate = (float) (parm->sampleRate);
    dev->nchns = parm->nChannels;
    dev->bufframes = parm->bufSamp_HW;
    dev->buffnos = bfns;

    psize = 4;
    /* set the buffer size
       output*/
    AudioDeviceSetProperty(dev->dev,NULL,0,false,
                           kAudioDevicePropertyBufferFrameSize,
                           psize, &dev->bufframes);
    /* input */
    AudioDeviceSetProperty(dev->dev,NULL,0,true,
                           kAudioDevicePropertyBufferFrameSize,
                           psize, &dev->bufframes);

    // check that it matches the expected size
    AudioDeviceGetProperty(dev->dev,0,true,
                           kAudioDevicePropertyBufferFrameSize,
                           &psize, &ibufframes);

    AudioDeviceGetProperty(dev->dev,0,false,
                           kAudioDevicePropertyBufferFrameSize,
                           &psize, &obufframes);

    if(ibufframes != dev->bufframes){
      if(ibufframes == obufframes)
        dev->bufframes = obufframes;
      else {
        free(dev);
        *(p->GetRtRecordUserData(csound)) = NULL;
        p->Message(csound, " *** CoreAudio: open: could not set buffer size\n");
        return -1;
      }
    }
    psize = sizeof(double);
    sr = dev->srate;
    AudioDeviceSetProperty(dev->dev, NULL, 0, true,
                           kAudioDevicePropertyNominalSampleRate, psize, &sr);
    AudioDeviceSetProperty(dev->dev, NULL, 0, false,
                           kAudioDevicePropertyNominalSampleRate, psize, &sr);

    dev->format.mSampleRate = dev->srate;
    dev->format.mFormatID = kAudioFormatLinearPCM;
    dev->format.mFormatFlags = kAudioFormatFlagIsFloat;
    dev->format.mBytesPerPacket = sizeof(float)*dev->nchns;
    dev->format.mFramesPerPacket = 1;
    dev->format.mBytesPerFrame = format.mBytesPerPacket;
    dev->format.mChannelsPerFrame = dev->nchns;
    dev->format.mBitsPerChannel = sizeof(float);

    psize = sizeof(AudioStreamBasicDescription);

    AudioDeviceSetProperty(dev->dev,NULL,0,true,
                           kAudioDevicePropertyStreamFormat,
                           psize, &dev->format);
    AudioDeviceSetProperty(dev->dev,NULL,0,false,
                           kAudioDevicePropertyStreamFormat,
                           psize, &dev->format);
    AudioDeviceGetProperty(dev->dev,0,false,
                           kAudioDevicePropertyStreamFormat,
                           &psize, &format);

    if(format.mChannelsPerFrame != (unsigned int)dev->nchns) {
      dev->format.mChannelsPerFrame = format.mChannelsPerFrame;
      p->Message(csound,
                 "CoreAudio module warning: using %d channels; "
                 "requested %d channels\n",
                 (int)dev->format.mChannelsPerFrame, (int)dev->nchns);

    }

    if(format.mSampleRate != dev->srate){
      *(p->GetRtRecordUserData(csound)) = NULL;
      p->Message(csound,
                 " *** CoreAudio: open: could not set device parameter sr: %d \n",
                 (int)dev->srate);
      p->Message(csound, " *** CoreAudio: current device sampling rate is:%d \n"
                 "     try setting the above value in your csound orchestra \n" ,
                 (int)format.mSampleRate);
      free(dev);
      return -1;
    }
    else p->Message(csound,
                    "CoreAudio module: sr set to %d with %d audio channels\n",
                    (int)dev->srate, (int) dev->format.mChannelsPerFrame);

    dev->outbuffs = (float **) malloc(sizeof(float*)*dev->buffnos);
    dev->inbuffs =  (float **) malloc(sizeof(float*)*dev->buffnos);
    dev->inused =    (int *) malloc(sizeof(int)*dev->buffnos);
    dev->outused = (int *) malloc(sizeof(int)*dev->buffnos);

    buffbytes = dev->bufframes*dev->nchns*sizeof(float);

    for(i=0; (unsigned int)i < dev->buffnos; i++){

      if((dev->inbuffs[i] = (float *) malloc(buffbytes)) == NULL){
        free(dev->outbuffs);
        free(dev->inbuffs);
        free(dev->inused);
        free(dev->outused);
        free(dev);
        *(p->GetRtRecordUserData(csound)) = NULL;
        p->Message(csound, " *** CoreAudio: open: memory allocation failure\n");
        return -1;
      }
          memset(dev->inbuffs[i], 0, buffbytes);

      if((dev->outbuffs[i] = (float *) malloc(buffbytes)) == NULL){
        free(dev->outbuffs);
        free(dev->inbuffs);
        free(dev->inused);
        free(dev->outused);
        free(dev);
        *(p->GetRtRecordUserData(csound)) = NULL;
        p->Message(csound, " *** CoreAudio: open: memory allocation failure\n");
        return -1;
      }
          memset(dev->outbuffs[i], 0, buffbytes);
      dev->inused[i] = dev->outused[i] = 1;
    }

    dev->incurbuff = dev->outcurbuff = dev->iocurbuff = 0;
    dev->incount = dev->outcount = 0;

    AudioDeviceAddIOProc(dev->dev, Csound_IOProcEntry, dev);
    AudioDeviceStart(dev->dev, Csound_IOProcEntry);

    if(isInput) *(p->GetRtPlayUserData(csound)) = (void*) dev;
    else  *(p->GetRtRecordUserData(csound)) = (void*) dev;

    p->Message(csound,
               "CoreAudio module: device open with %d buffers of %d frames\n"
               "==========================================================\n",
               dev->buffnos,dev->bufframes);
    return 0;
}

/* open for audio input */
static int recopen_(void *csound, csRtAudioParams *parm)
{
    ENVIRON *p;
    DEVPARAMS *dev;
    p = (ENVIRON*) csound;
    if (*(p->GetRtRecordUserData(csound)) != NULL)
      return 0;
    /* allocate structure */
    dev = (DEVPARAMS*) malloc(sizeof(DEVPARAMS));
    if (dev == NULL) {
      p->Message(csound, " *** CoreAudio: open: memory allocation failure\n");
      return -1;
    }
    *(p->GetRtRecordUserData(csound)) = (void*) dev;

    return coreaudio_open(csound, parm, dev, 1);
}

/* open for audio output */
static int playopen_(void *csound, csRtAudioParams *parm)
{
    ENVIRON *p;
    DEVPARAMS *dev;
    p = (ENVIRON*) csound;
    if (*(p->GetRtPlayUserData(csound))  != NULL)
      return 0;
    /* allocate structure */
    dev = (DEVPARAMS*) malloc(sizeof(DEVPARAMS));
    if (dev == NULL) {
      p->Message(csound, " *** CoreAudio: open: memory allocation failure\n");
      return -1;
    }
    *(p->GetRtPlayUserData(csound)) = (void*) dev;

    return coreaudio_open(csound, parm, dev, 0);
}

static int rtrecord_(void *csound, void *inbuf_, int bytes_)
{
    DEVPARAMS *dev;
    ENVIRON   *p;
    int       n, i, chans, cur, icount, buffitems, buffnos, *inused, usecs;
    float **ibuffs;
    /* MYFLT norm; */
        p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (*(p->GetRtRecordUserData(csound)));
        usecs = (int) (1000*dev->bufframes/p->esr);
    n = bytes_ / sizeof(MYFLT);
    chans = dev->nchns;
    ibuffs = dev->inbuffs;
    cur = dev->incurbuff;
    inused = dev->inused;
    icount = dev->incount;
    buffnos = dev->buffnos;
    buffitems = dev->bufframes*chans;
    /* norm = p->e0dbfs;  */

    for(i = 0; i < n; i++){
      ((MYFLT *)inbuf_)[i] = ibuffs[cur][icount];
      icount++;
      if(icount == buffitems){
        inused[cur] = 0;
        cur++;
        cur %= buffnos;
        icount = 0;
                while(!inused[cur]) usleep(usecs);
      }
    } // for
    dev->incount = icount;
    dev->incurbuff = cur;

    return bytes_;

}

/* put samples to DAC */

static void rtplay_(void *csound, void *outbuf_, int bytes_)
{
    DEVPARAMS *dev;
    ENVIRON   *p;
    int       n, i, chans, cur, ocount, buffitems, buffnos, *outused, usecs;
    float **obuffs;
    /* MYFLT norm; */
    p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (*(p->GetRtRecordUserData(csound)));

    n = bytes_ / sizeof(MYFLT);
        usecs = (int) (1000*dev->bufframes/p->esr);
    chans = dev->nchns;
    obuffs = dev->outbuffs;
    cur = dev->outcurbuff;
    outused = dev->outused;
    ocount = dev->outcount;
    buffnos = dev->buffnos;
    buffitems = dev->bufframes*chans;
    /* norm = p->e0dbfs; */

    for(i = 0; i < n; i++){
      obuffs[cur][ocount] = (float)((MYFLT *)outbuf_)[i];
      ocount++;
      if(ocount == buffitems){
        outused[cur] = 0;
        cur++;
        cur %= buffnos;
        ocount = 0;
        while(!outused[cur]) usleep(usecs);
      }
    }

    dev->outcurbuff = cur;
    dev->outcount = ocount;
}

/* close the I/O device entirely  */
/* called only when both complete */

static void rtclose_(void *csound)
{
    DEVPARAMS *dev;
    ENVIRON   *p;
    p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (*(p->GetRtRecordUserData(csound)));
    if (dev != NULL) {
      p->Message(csound, "coreaudio module: closing device...\n");
      AudioDeviceStop(dev->dev, Csound_IOProcEntry);
      AudioDeviceRemoveIOProc(dev->dev, Csound_IOProcEntry);
      *(p->GetRtRecordUserData(csound)) = NULL;
      free(dev->outbuffs);
      free(dev->inbuffs);
      free(dev->inused);
      free(dev->outused);
      free(dev);
      p->Message(csound, "coreaudio module: device closed\n");
    }
}

