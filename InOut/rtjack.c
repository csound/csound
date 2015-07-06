/*
  rtjack.c:

  Copyright (C) 2005, 2006 Istvan Varga

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

#include <jack/jack.h>
#include <ctype.h>
/* no #ifdef, should always have these on systems where JACK is available */
#include <unistd.h>
#include <stdint.h>
#ifdef LINUX
#include <pthread.h>
#endif
#include "csdl.h"
#include "soundio.h"
#ifdef LINUX
#include <sched.h>
#endif

#include "cs_jack.h"

#ifdef LINUX

static inline int rtJack_CreateLock(CSOUND *csound, pthread_mutex_t *p)
{
    (void) csound;
    return (pthread_mutex_init(p, (pthread_mutexattr_t*) NULL));
}

static inline void rtJack_Lock(CSOUND *csound, pthread_mutex_t *p)
{
    (void) csound;
    pthread_mutex_lock(p);
}

static inline int rtJack_LockTimeout(CSOUND *csound, pthread_mutex_t *p,
                                     size_t milliseconds)
{
      struct timeval  tv;
      struct timespec ts;
      register size_t n, s;
      register int retval = pthread_mutex_trylock(p);
      if (!retval)
        return retval;
      if (!milliseconds)
        return retval;
      gettimeofday(&tv, NULL);
      s = milliseconds / (size_t) 1000;
      n = milliseconds - (s * (size_t) 1000);
      s += (size_t) tv.tv_sec;
      n = (size_t) (((int) n * 1000 + (int) tv.tv_usec) * 1000);
      ts.tv_nsec = (long) (n < (size_t) 1000000000 ? n : n - 1000000000);
      ts.tv_sec = (time_t) (n < (size_t) 1000000000 ? s : s + 1);
      return pthread_mutex_timedlock(p, &ts);

}

static inline int rtJack_TryLock(CSOUND *csound, pthread_mutex_t *p)
{
    (void) csound;
    return (pthread_mutex_trylock(p));
}

static inline void rtJack_Unlock(CSOUND *csound, pthread_mutex_t *p)
{
    (void) csound;
    pthread_mutex_unlock(p);
}

static inline void rtJack_DestroyLock(CSOUND *csound, pthread_mutex_t *p)
{
    (void) csound;
    pthread_mutex_unlock(p);
    pthread_mutex_destroy(p);
}

#else   /* LINUX */

static inline int rtJack_CreateLock(CSOUND *csound, void **p)
{
    *p = csound->CreateThreadLock();
    return (*p != NULL ? 0 : -1);
}

static inline void rtJack_Lock(CSOUND *csound, void **p)
{
    csound->WaitThreadLockNoTimeout(*p);
}

static inline int rtJack_LockTimeout(CSOUND *csound, void **p, size_t timeout)
{
  return csound->WaitThreadLock(*p, timeout);
}


static inline int rtJack_TryLock(CSOUND *csound, void **p)
{
    return (csound->WaitThreadLock(*p, (size_t) 0));
}

static inline void rtJack_Unlock(CSOUND *csound, void **p)
{
    csound->NotifyThreadLock(*p);
}

static inline void rtJack_DestroyLock(CSOUND *csound, void **p)
{
    csound->NotifyThreadLock(*p);
    csound->DestroyThreadLock(*p);
    *p = NULL;
}

#endif  /* !LINUX */

/* print error message, close connection, and terminate performance */

static CS_NORETURN void rtJack_Error(CSOUND *, int errCode, const char *msg);

static int processCallback(jack_nframes_t nframes, void *arg);

/* callback functions */

static int sampleRateCallback(jack_nframes_t nframes, void *arg)
{
    RtJackGlobals *p = (RtJackGlobals*) arg;

    if (p->sampleRate != (int) nframes)
      p->jackState = 1;
    return 0;
}

static int bufferSizeCallback(jack_nframes_t nframes, void *arg)
{
    RtJackGlobals *p = (RtJackGlobals*) arg;

    (void) nframes;
    /* invalidate output port buffer pointer cache */
    if (p->outPortBufs != NULL)
      p->outPortBufs[0] = (jack_default_audio_sample_t*) NULL;
    return 0;
}

#ifdef LINUX
static void freeWheelCallback(int starting, void *arg)
{
    RtJackGlobals *p;
    CSOUND *csound;

    p = (RtJackGlobals*) arg;
    csound = p->csound;
    if (starting) {
      if (UNLIKELY(sched_getscheduler(0) != SCHED_OTHER)) {
        struct sched_param sp;
        csound->Message(csound, Str(" *** WARNING: "
                                    "disabling --sched in freewheel mode\n"));
        memset(&sp, 0, sizeof(struct sched_param));
        sp.sched_priority = 0;
        sched_setscheduler(0, SCHED_OTHER, &sp);
      }
    }
}
#endif

static int xrunCallback(void *arg)
{
    RtJackGlobals *p = (RtJackGlobals*) arg;

    p->xrunFlag = 1;
    return 0;
}

static void shutDownCallback(void *arg)
{
    RtJackGlobals *p = (RtJackGlobals*) arg;

    p->jackState = 2;
    if (p->bufs != NULL) {
      int   i;
      for (i = 0; i < p->nBuffers; i++) {
        if (p->bufs[i] != NULL &&
            (p->bufs[i]->inBufs != NULL || p->bufs[i]->outBufs != NULL))
          rtJack_Unlock(p->csound, &(p->bufs[i]->csndLock));
      }
    }
}

static CS_NOINLINE void rtJack_PrintPortName(CSOUND *csound,
                                             const char *portName,
                                             int nChannels)
{
    if (nChannels > 0 && portName[0] != (char) 0) {
      if ((int) strlen(portName) < 16)
        csound->Message(csound, "      \"%s\"\t\t", portName);
      else
        csound->Message(csound, "      \"%s\"\t", portName);
      if (nChannels > 1)
        csound->Message(csound, Str("(%d channels)\n"), nChannels);
      else
        csound->Message(csound, Str("(1 channel)\n"));
    }
}

static int portname_cmp_func(const void *p1, const void *p2)
{
    return (strcmp(*((char**) p1), *((char**) p2)));
}

/* Print the list of available device names for -i adc (if isOutput is zero) */
/* or -o dac, returning zero on success. */

static CS_NOINLINE int rtJack_ListPorts(CSOUND *csound,
                                        jack_client_t *jackClient,
                                        const char *clientName,
                                        int isOutput)
{
    char            **portNames = (char**) NULL;
    char            clientNameBuf[MAX_NAME_LEN + 2];
    char            *prvPortName = (char*) NULL, *curPortName = (char*) NULL;
    unsigned long   portFlags;
    int             i, nChn, maxChn, len, nPorts, retval = -1;

    portFlags = (isOutput ? (unsigned long) JackPortIsInput
                 : (unsigned long) JackPortIsOutput);
    len = jack_port_name_size();
    prvPortName = (char*) malloc((size_t) len);
    if (UNLIKELY(prvPortName == (char*) NULL))
      goto err_return;
    curPortName = (char*) malloc((size_t) len);
    if (UNLIKELY(curPortName == (char*) NULL))
      goto err_return;
    portNames = (char**) jack_get_ports(jackClient,
                                        (char*) NULL,
                                        JACK_DEFAULT_AUDIO_TYPE,
                                        portFlags);
    if (UNLIKELY(portNames == (char**) NULL))
      goto err_return;
    retval = 0;
    csound->Message(csound, Str("The available JACK %s devices are:\n"),
                    (isOutput ? Str("output") : Str("input")));
    /* count the number of ports, and sort port names in alphabetical order */
    for (nPorts = 0; portNames[nPorts] != NULL; nPorts++)
      ;
    qsort((void*) portNames, (size_t) nPorts, sizeof(char*), portname_cmp_func);
    snprintf(&(clientNameBuf[0]), MAX_NAME_LEN + 2, "%s:", clientName);
    len = strlen(&(clientNameBuf[0]));
    prvPortName[0] = (char) 0;
    maxChn = nChn = 0;
    for (i = 0; portNames[i] != NULL; i++) {
      int     n, chn_;
      /* skip ports owned by this client */
      if (strncmp(portNames[i], &(clientNameBuf[0]), (size_t) len) == 0)
        goto nextPortName;
      n = (int) strlen(portNames[i]);
      do {
        n--;
      } while (n > 0 && isdigit(portNames[i][n]));
      n++;
      if (n < 2 || n == (int) strlen(portNames[i]))
        goto nextPortName;
      chn_ = (int) atoi(&(portNames[i][n]));
      if (chn_ == 0)
        goto nextPortName;
      strncpy(curPortName, portNames[i], (size_t) n);
      curPortName[n] = (char) 0;
      if (strcmp(curPortName, prvPortName) != 0) {
        if (nChn == maxChn)
          rtJack_PrintPortName(csound, prvPortName, nChn);
        strcpy(prvPortName, curPortName);
        nChn = 1;
        maxChn = chn_;
      }
      else {
        nChn++;
        if (chn_ > maxChn)
          maxChn = chn_;
      }
      continue;
    nextPortName:
      if (nChn == maxChn)
        rtJack_PrintPortName(csound, prvPortName, nChn);
      prvPortName[0] = (char) 0;
      maxChn = nChn = 0;
    }
    if (nChn == maxChn)
      rtJack_PrintPortName(csound, prvPortName, nChn);

 err_return:
    if (portNames != (char**) NULL)
      free((void*) portNames);
    if (curPortName != (char*) NULL)
      free((void*) curPortName);
    if (prvPortName != (char*) NULL)
      free((void*) prvPortName);
    return retval;
}

static inline size_t rtJack_AlignData(size_t ofs)
{
    return ((ofs + (size_t) 15) & (~((size_t) 15)));
}

/* allocate ring buffers */

static void rtJack_AllocateBuffers(RtJackGlobals *p)
{
    CSOUND *csound = p->csound;
    void    *ptr;
    size_t  i, j, m, nBytes, nBytesPerBuf, ofs1, ofs2, ofs3;

    m = (size_t) ((p->inputEnabled ? 1 : 0) + (p->outputEnabled ? 1 : 0));
    if (!m)
      return;
    /* calculate the number of bytes to allocate */
    ofs1 = rtJack_AlignData(sizeof(RtJackBuffer*) * (size_t) p->nBuffers);
    ofs2 = rtJack_AlignData(sizeof(RtJackBuffer));
    ofs3 = rtJack_AlignData(sizeof(jack_default_audio_sample_t*)
                            * (size_t) p->nChannels * m);
    nBytesPerBuf = ofs2 + ofs3;
    nBytesPerBuf += rtJack_AlignData(sizeof(jack_default_audio_sample_t)
                                     * (size_t) p->nChannels
                                     * (size_t) p->bufSize
                                     * m);
    nBytes = ofs1 + (nBytesPerBuf * (size_t) p->nBuffers);
    /* allocate memory */
    ptr = (RtJackBuffer**) malloc(nBytes);
    if (UNLIKELY(ptr == NULL))
      rtJack_Error(csound, CSOUND_MEMORY, Str("memory allocation failure"));
    p->bufs = (RtJackBuffer**) ptr;
    memset((void*) ptr, 0, nBytes);
    /* set pointer to each buffer */
    ptr = (void*) ((char*) ptr + (long) ofs1);
    for (i = (size_t) 0; i < (size_t) p->nBuffers; i++) {
      p->bufs[i] = ptr;
      ptr = (void*) ((char*) ptr + (long) nBytesPerBuf);
    }
    for (i = (size_t) 0; i < (size_t) p->nBuffers; i++) {
      /* create lock for signaling when the process callback is done */
      /* with the buffer */
      if (UNLIKELY(rtJack_CreateLock(csound, &(p->bufs[i]->csndLock)) != 0))
        rtJack_Error(csound, CSOUND_MEMORY, Str("memory allocation failure"));
      /* create lock for signaling when the Csound thread is done */
      /* with the buffer */
      if (UNLIKELY(rtJack_CreateLock(csound, &(p->bufs[i]->jackLock)) != 0)) {
        rtJack_DestroyLock(csound, &(p->bufs[i]->csndLock));
        rtJack_Error(csound, CSOUND_MEMORY, Str("memory allocation failure"));
      }
      ptr = (void*) p->bufs[i];
      ptr = (void*) ((char*) ptr + (long) ofs2);
      /* set pointers to input/output buffers */
      if (p->inputEnabled) {
        p->bufs[i]->inBufs = (jack_default_audio_sample_t**) ptr;
        ptr = (void*) &(p->bufs[i]->inBufs[p->nChannels]);
      }
      if (p->outputEnabled)
        p->bufs[i]->outBufs = (jack_default_audio_sample_t**) ptr;
      ptr = (void*) p->bufs[i];
      ptr = (void*) ((char*) ptr + (long) (ofs2 + ofs3));
      for (j = (size_t) 0; j < (size_t) p->nChannels; j++) {
        if (p->inputEnabled) {
          p->bufs[i]->inBufs[j] = (jack_default_audio_sample_t*) ptr;
          ptr = (void*) &(p->bufs[i]->inBufs[j][p->bufSize]);
        }
        if (p->outputEnabled) {
          p->bufs[i]->outBufs[j] = (jack_default_audio_sample_t*) ptr;
          ptr = (void*) &(p->bufs[i]->outBufs[j][p->bufSize]);
        }
      }
    }
}

/* register JACK ports */

static void rtJack_RegisterPorts(RtJackGlobals *p)
{
    char          portName[MAX_NAME_LEN + 4];
    unsigned long flags = 0UL;
    int           i;
    CSOUND *csound = p->csound;

    if (!(p->inputEnabled && p->outputEnabled))
      flags = (unsigned long) JackPortIsTerminal;
    if (p->inputEnabled) {
      /* register input ports */
      for (i = 0; i < p->nChannels; i++) {
        snprintf(portName, MAX_NAME_LEN + 4, "%s%d", p->inputPortName, i + 1);
        p->inPorts[i] = jack_port_register(p->client, &(portName[0]),
                                           JACK_DEFAULT_AUDIO_TYPE,
                                           flags | JackPortIsInput, 0UL);
        if (UNLIKELY(p->inPorts[i] == NULL))
          rtJack_Error(csound, -1, Str("error registering input ports"));
      }
    }
    if (p->outputEnabled) {
      /* register output ports */
      for (i = 0; i < p->nChannels; i++) {
        snprintf(portName, MAX_NAME_LEN + 4, "%s%d", p->outputPortName, i + 1);
        p->outPorts[i] = jack_port_register(p->client, &(portName[0]),
                                            JACK_DEFAULT_AUDIO_TYPE,
                                            flags | JackPortIsOutput, 0UL);
        if (UNLIKELY(p->outPorts[i] == NULL))
          rtJack_Error(csound, -1, Str("error registering output ports"));
      }
    }
}
static int listDevices(CSOUND *csound, CS_AUDIODEVICE *list, int isOutput);
/* connect to JACK server, set up ports and ring buffers, */
/* activate client, and connect ports if requested */

static void openJackStreams(RtJackGlobals *p)
{
    char    buf[256];
    int     i, j, k;
    CSOUND *csound = p->csound;

    /* connect to JACK server */
    p->client = jack_client_open(&(p->clientName[0]), JackNoStartServer, NULL);
    if (UNLIKELY(p->client == NULL))
      rtJack_Error(csound, -1, Str("could not connect to JACK server"));

    csound->system_sr(csound, jack_get_sample_rate(p->client));

    /* check consistency of parameters */
    if (UNLIKELY(p->nChannels < 1 || p->nChannels > 255))
      rtJack_Error(csound, -1, Str("invalid number of channels"));
    if (UNLIKELY(p->sampleRate < 1000 || p->sampleRate > 768000))
      rtJack_Error(csound, -1, Str("invalid sample rate"));
    if (UNLIKELY(p->sampleRate != (int) jack_get_sample_rate(p->client))) {
      snprintf(&(buf[0]), 256, Str("sample rate %d does not match "
                             "JACK sample rate %d"),
              p->sampleRate, (int) jack_get_sample_rate(p->client));
      rtJack_Error(p->csound, -1, &(buf[0]));
    }
    if (UNLIKELY(p->bufSize < 8 || p->bufSize > 32768))
      rtJack_Error(csound, -1, Str("invalid period size (-b)"));
    if (p->nBuffers < 2)
      p->nBuffers = 2;
    if (UNLIKELY((unsigned int) (p->nBuffers * p->bufSize) > (unsigned int) 65536))
      rtJack_Error(csound, -1, Str("invalid buffer size (-B)"));
    if (UNLIKELY(((p->nBuffers - 1) * p->bufSize)
                 < (int) jack_get_buffer_size(p->client)))
      rtJack_Error(csound, -1, Str("buffer size (-B) is too small"));

    /* register ports */
    rtJack_RegisterPorts(p);

    /* allocate ring buffers if not done yet */
    if (p->bufs == NULL)
      rtJack_AllocateBuffers(p);

    /* initialise ring buffers */
    p->csndBufCnt = 0;
    p->csndBufPos = 0;
    p->jackBufCnt = 0;
    p->jackBufPos = 0;
    for (i = 0; i < p->nBuffers; i++) {
      rtJack_TryLock(p->csound, &(p->bufs[i]->csndLock));
      rtJack_Unlock(p->csound, &(p->bufs[i]->jackLock));
      for (j = 0; j < p->nChannels; j++) {
        if (p->inputEnabled) {
          for (k = 0; k < p->bufSize; k++)
            p->bufs[i]->inBufs[j][k] = (jack_default_audio_sample_t) 0;
        }
        if (p->outputEnabled) {
          for (k = 0; k < p->bufSize; k++)
            p->bufs[i]->outBufs[j][k] = (jack_default_audio_sample_t) 0;
        }
      }
    }

    /* output port buffer pointer cache is invalid initially */
    if (p->outputEnabled)
      p->outPortBufs[0] = (jack_default_audio_sample_t*) NULL;

    /* register callback functions */
    if (UNLIKELY(jack_set_sample_rate_callback(p->client,
                                               sampleRateCallback, (void*) p)
                 != 0))
      rtJack_Error(csound, -1, Str("error setting sample rate callback"));
    if (UNLIKELY(jack_set_buffer_size_callback(p->client,
                                               bufferSizeCallback, (void*) p)
                 != 0))
      rtJack_Error(csound, -1, Str("error setting buffer size callback"));
#ifdef LINUX
    if (UNLIKELY(jack_set_freewheel_callback(p->client,
                                             freeWheelCallback, (void*) p)
                 != 0))
      rtJack_Error(csound, -1, Str("error setting freewheel callback"));
#endif
    if (UNLIKELY(jack_set_xrun_callback(p->client, xrunCallback, (void*) p) != 0))
      rtJack_Error(csound, -1, Str("error setting xrun callback"));
    jack_on_shutdown(p->client, shutDownCallback, (void*) p);
    if (UNLIKELY(jack_set_process_callback(p->client,
                                           processCallback, (void*) p) != 0))
      rtJack_Error(csound, -1, Str("error setting process callback"));

    /* activate client */
    if (UNLIKELY(jack_activate(p->client) != 0))
      rtJack_Error(csound, -1, Str("error activating JACK client"));

    /* connect ports if requested */
    if (p->inputEnabled) {
      char dev[128], *dev_final, *sp;
      {
        int i,n = listDevices(csound,NULL,0);
        CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *)
                malloc(n*sizeof(CS_AUDIODEVICE));
        listDevices(csound,devs,0);
        for(i=0; i < n; i++)
          csound->Message(csound, " %d: %s (%s)\n",
                          i, devs[i].device_id, devs[i].device_name);
        strncpy(dev, devs[0].device_name, 128);
        free(devs);
      }
      if(p->inDevName != NULL) {
        strncpy(dev, p->inDevName, 128); dev[127]='\0';
      }
      //if (dev) {
      dev_final = dev;
      sp = strchr(dev_final, '\0');
      if (!isalpha(dev_final[0])) dev_final++;
      for (i = 0; i < p->nChannels; i++) {
        snprintf(sp, 128-(dev-sp), "%d", i + 1);
        if (UNLIKELY(jack_connect(p->client, dev_final,
                                  jack_port_name(p->inPorts[i])) != 0)) {
          //rtJack_Error(csound, -1, Str("error connecting input ports"));
          csound->Warning(csound,
                          Str("not autoconnecting input channel %d \n"
                              "(needs manual connection)"), i+1);
        }
      }
      *sp = (char) 0;
      //}

    }
    if (p->outputEnabled) {
      char dev[128], *dev_final, *sp;
      {
          int i,n = listDevices(csound,NULL,1);
          CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *)
                  malloc(n*sizeof(CS_AUDIODEVICE));
          listDevices(csound,devs,1);
          for(i=0; i < n; i++)
            csound->Message(csound, " %d: %s (%s)\n",
                            i, devs[i].device_id, devs[i].device_name);
          strncpy(dev, devs[0].device_name, 128);
          free(devs);
      }
      if (p->outDevName != NULL) {
        strncpy(dev, p->outDevName, 128); dev[127]='\0';
      }
      //if (dev) { this test is rubbish
      dev_final = dev;
      sp = strchr(dev_final, '\0');
      if(!isalpha(dev_final[0])) dev_final++;
      for (i = 0; i < p->nChannels; i++) {
        snprintf(sp, 128-(dev-sp), "%d", i + 1);
        if (jack_connect(p->client, jack_port_name(p->outPorts[i]),
                         dev_final) != 0) {
          //rtJack_Error(csound, -1, Str("error connecting output ports"));
          csound->Warning(csound, Str("not autoconnecting input channel %d \n"
                                      "(needs manual connection)"), i+1);

        }
      }
      *sp = (char) 0;
    }
    /* stream is now active */
    p->jackState = 0;
}

/* Make a copy of the device name specified for -i adc or -o dac, */
/* allocating extra space for a channel number suffix. */
/* Also set up other device parameters, and check consistency. */

static void rtJack_CopyDevParams(RtJackGlobals *p, char **devName,
                                 const csRtAudioParams *parm, int isOutput)
{
    CSOUND  *csound;
    char    *s;
    size_t  nBytes;

    csound = p->csound;
    *devName = (char*) NULL;
    if (parm->devNum != 1024) {
      jack_client_t *client_;
      int           useTmpClient = 0;
      /* FIXME: a temporary JACK client is created if there is no */
      /* connection yet; this is a somewhat hackish solution... */
      if (p->client == (jack_client_t*) NULL) {
        useTmpClient = 1;
        client_ = jack_client_open(&(p->clientName[0]), JackNoStartServer, NULL);
      }
      else
        client_ = p->client;
      if (client_ != (jack_client_t*) NULL) {
        rtJack_ListPorts(csound, client_, &(p->clientName[0]), isOutput);
        if (useTmpClient)
          jack_client_close(client_);
      }
      rtJack_Error(csound, -1, Str("must specify a device name, not a number"));
    }
    if (parm->devName != NULL && parm->devName[0] != (char) 0) {
      /* NOTE: this assumes max. 999 channels (the current limit is 255) */
      nBytes = strlen(parm->devName) + 4;
      if (UNLIKELY(nBytes > (size_t) jack_port_name_size()))
        rtJack_Error(csound, -1, Str("device name is too long"));
      s = (char*) malloc(nBytes+1);
      if (UNLIKELY(s == NULL))
        rtJack_Error(csound, CSOUND_MEMORY, Str("memory allocation failure"));
      strcpy(s, parm->devName);

      *devName = s;
    }
    if (isOutput && p->inputEnabled) {
      /* full duplex audio I/O: check consistency of parameters */
      if (UNLIKELY(p->nChannels != parm->nChannels ||
                   (unsigned int)p->bufSize != parm->bufSamp_SW))
        rtJack_Error(csound, -1,
                     Str("input and output parameters are not consistent"));
      if (UNLIKELY((unsigned int)((parm->bufSamp_SW / csound->GetKsmps(csound)) *
                                  csound->GetKsmps(csound)) != parm->bufSamp_SW))
        rtJack_Error(csound, -1,
                     Str("period size (-b) must be an integer multiple of ksmps"));
    }
    p->sampleRate = (int) parm->sampleRate;
    if (UNLIKELY((float) p->sampleRate != parm->sampleRate))
      rtJack_Error(csound, -1, Str("sample rate must be an integer"));
    p->nChannels = parm->nChannels;
    p->bufSize = parm->bufSamp_SW;
    p->nBuffers = (parm->bufSamp_HW + parm->bufSamp_SW - 1) / parm->bufSamp_SW;
    
}

/* open for audio input */

static int recopen_(CSOUND *csound, const csRtAudioParams *parm)
{
    RtJackGlobals *p;

    p = (RtJackGlobals*) csound->QueryGlobalVariable(csound, "_rtjackGlobals");
    if (p == NULL)
      return -1;
    *(csound->GetRtRecordUserData(csound)) = (void*) p;
    rtJack_CopyDevParams(p, &(p->inDevName), parm, 0);
    p->inputEnabled = 1;
    /* allocate pointers to input ports */
    p->inPorts = (jack_port_t**)
      calloc((size_t) p->nChannels, sizeof(jack_port_t*));
    if (UNLIKELY(p->inPorts == NULL))
      rtJack_Error(p->csound, CSOUND_MEMORY, Str("memory allocation failure"));
    /* allocate pointers to input port buffers */
    p->inPortBufs = (jack_default_audio_sample_t**)
      calloc((size_t) p->nChannels, sizeof(jack_default_audio_sample_t*));
    if (UNLIKELY(p->inPortBufs == NULL))
      rtJack_Error(p->csound, CSOUND_MEMORY, Str("memory allocation failure"));



    return 0;
}

/* open for audio output */

static int playopen_(CSOUND *csound, const csRtAudioParams *parm)
{
    RtJackGlobals *p;

    p = (RtJackGlobals*) csound->QueryGlobalVariable(csound, "_rtjackGlobals");
    if (p == NULL)
      return -1;
    *(csound->GetRtPlayUserData(csound)) = (void*) p;
    rtJack_CopyDevParams(p, &(p->outDevName), parm, 1);

    p->outputEnabled = 1;
    /* allocate pointers to output ports */
    p->outPorts = (jack_port_t**)
      calloc((size_t) p->nChannels, sizeof(jack_port_t*));
    if (UNLIKELY(p->outPorts == NULL))
      rtJack_Error(p->csound, CSOUND_MEMORY, Str("memory allocation failure"));
    /* allocate pointers to output port buffers */
    p->outPortBufs = (jack_default_audio_sample_t**)
      calloc((size_t) p->nChannels, sizeof(jack_default_audio_sample_t*));
    if (UNLIKELY(p->outPortBufs == NULL))
      rtJack_Error(p->csound, CSOUND_MEMORY, Str("memory allocation failure"));
    /* activate client to start playback */
    openJackStreams(p);

    return 0;
}

/* the process callback is called by the JACK client thread, */
/* and copies data to the input and from the output ring buffers */

static int processCallback(jack_nframes_t nframes, void *arg)
{
    RtJackGlobals *p;
    int           i, j, k, l;

    p = (RtJackGlobals*) arg;
    /* get pointers to port buffers */
    if (p->inputEnabled) {
      for (i = 0; i < p->nChannels; i++)
        p->inPortBufs[i] = (jack_default_audio_sample_t*)
          jack_port_get_buffer(p->inPorts[i], nframes);
    }
    if (p->outputEnabled && p->outPortBufs[0] == NULL) {
      for (i = 0; i < p->nChannels; i++)
        p->outPortBufs[i] = (jack_default_audio_sample_t*)
          jack_port_get_buffer(p->outPorts[i], nframes);
    }
    i = 0;
    do {
      /* if starting new buffer: */
      if (p->jackBufPos == 0) {
        /* check for xrun: */
        if (rtJack_TryLock(p->csound, &(p->bufs[p->jackBufCnt]->jackLock))
            != 0) {
          p->xrunFlag = 1;
          /* yes, discard input and fill output with zero samples */
          if (p->outputEnabled) {
            for (j = 0; j < p->nChannels; j++)
              for (k = i; k < (int) nframes; k++)
                p->outPortBufs[j][k] = (jack_default_audio_sample_t) 0;
            return 0;
          }
        }
      }
      /* copy audio data on each channel */
      k = (int) nframes - i;
      l = p->bufSize - p->jackBufPos;
      l = (l < k ? l : k);      /* number of frames to copy */
      for (j = 0; j < p->nChannels; j++) {
        if (p->inputEnabled) {
          jack_default_audio_sample_t   *srcp, *dstp;
          srcp = &(p->inPortBufs[j][i]);
          dstp = &(p->bufs[p->jackBufCnt]->inBufs[j][p->jackBufPos]);
          for (k = 0; k < l; k++)
            dstp[k] = srcp[k];
        }
        if (p->outputEnabled) {
          jack_default_audio_sample_t   *srcp, *dstp;
          srcp = &(p->bufs[p->jackBufCnt]->outBufs[j][p->jackBufPos]);
          dstp = &(p->outPortBufs[j][i]);
          for (k = 0; k < l; k++)
            dstp[k] = srcp[k];
        }
      }
      p->jackBufPos += l;
      i += l;
      /* if done with a buffer, notify Csound thread and advance to next one */
      if (p->jackBufPos >= p->bufSize) {
        p->jackBufPos = 0;
        rtJack_Unlock(p->csound, &(p->bufs[p->jackBufCnt]->csndLock));
        if (++(p->jackBufCnt) >= p->nBuffers)
          p->jackBufCnt = 0;
      }
    } while (i < (int) nframes);
    return 0;
}

static CS_NOINLINE CS_NORETURN void rtJack_Abort(CSOUND *csound, int err)
{
    switch (err) {
    case 1:
      rtJack_Error(csound, -1, Str("JACK sample rate changed"));
      break;
    default:
      rtJack_Error(csound, -1, Str("no connection to JACK server"));
    }
}

static CS_NOINLINE void rtJack_Restart(RtJackGlobals *p)
{
    CSOUND  *csound = p->csound;

    csound->ErrorMsg(csound, Str(" *** rtjack: connection to JACK "
                                 "server was lost, reconnecting..."));
    p->jackState = -1;
    jack_client_close(p->client);
    openJackStreams(p);
}

/* get samples from ADC */

static int rtrecord_(CSOUND *csound, MYFLT *inbuf_, int bytes_)
{
    RtJackGlobals *p;
    int           i, j, k, nframes, bufpos, bufcnt;

    p = (RtJackGlobals*) *(csound->GetRtPlayUserData(csound));
    if (UNLIKELY(p==NULL)) rtJack_Abort(csound, 0);
    if (p->jackState != 0) {
      if (p->jackState < 0)
        openJackStreams(p);     /* open audio input */
      else if (p->jackState == 2)
        rtJack_Restart(p);
      else
        rtJack_Abort(csound, p->jackState);
    }
    nframes = bytes_ / (p->nChannels * (int) sizeof(MYFLT));
    bufpos = p->csndBufPos;
    bufcnt = p->csndBufCnt;
    for (i = j = 0; i < nframes; i++) {
      if (bufpos == 0) {
        /* wait until there is enough data in ring buffer */
        /* VL 28.03.15 -- timeout after wait for 10 buffer
           lengths */
        int ret = rtJack_LockTimeout(csound, &(p->bufs[bufcnt]->csndLock),
                                     10000*(nframes/csound->GetSr(csound)));
        if(ret) {
          memset(inbuf_, 0, bytes_);
          OPARMS oparms;
          csound->GetOParms(csound, &oparms);
          if (oparms.msglevel & 4)
             csound->Warning(csound, Str("rtjack: input audio timeout"));
          return bytes_;
        }
      }
      /* copy audio data */
      for (k = 0; k < p->nChannels; k++)
        inbuf_[j++] = (MYFLT) p->bufs[bufcnt]->inBufs[k][i];
      if (++bufpos >= p->bufSize) {
        bufpos = 0;
        /* notify JACK callback that this buffer has been consumed */
        if (!p->outputEnabled)
          rtJack_Unlock(csound, &(p->bufs[bufcnt]->jackLock));
        /* advance to next buffer */
        if (++bufcnt >= p->nBuffers)
          bufcnt = 0;
      }
    }
    if (!p->outputEnabled) {
      p->csndBufPos = bufpos;
      p->csndBufCnt = bufcnt;
    }
    if (p->xrunFlag) {
      p->xrunFlag = 0;
      OPARMS oparms;
      csound->GetOParms(csound, &oparms);
      if (oparms.msglevel & 4)
        csound->Warning(csound, Str("rtjack: xrun in real time audio"));
    }

    return bytes_;
}

/* put samples to DAC */

static void rtplay_(CSOUND *csound, const MYFLT *outbuf_, int bytes_)
{
    RtJackGlobals *p;
    int           i, j, k, nframes;

    p = (RtJackGlobals*) *(csound->GetRtPlayUserData(csound));
    if (p == NULL)
      return;
    if (p->jackState != 0) {
      if (p->jackState == 2)
        rtJack_Restart(p);
      else
        rtJack_Abort(csound, p->jackState);
      return;
    }
    nframes = bytes_ / (p->nChannels * (int) sizeof(MYFLT));
    for (i = j = 0; i < nframes; i++) {
      if (p->csndBufPos == 0) {
        /* wait until there is enough free space in ring buffer */
        if (!p->inputEnabled)
          /* **** COVERITY: claims this is a double lock **** */
          rtJack_Lock(csound, &(p->bufs[p->csndBufCnt]->csndLock));
      }
      /* copy audio data */
      for (k = 0; k < p->nChannels; k++)
        p->bufs[p->csndBufCnt]->outBufs[k][i] =
          (jack_default_audio_sample_t) outbuf_[j++];
      if (++(p->csndBufPos) >= p->bufSize) {
        p->csndBufPos = 0;
        /* notify JACK callback that this buffer is now filled */
        rtJack_Unlock(csound, &(p->bufs[p->csndBufCnt]->jackLock));
        /* advance to next buffer */
        if (++(p->csndBufCnt) >= p->nBuffers)
          p->csndBufCnt = 0;
      }
    }
    if (p->xrunFlag) {
      p->xrunFlag = 0;
      csound->Warning(csound, Str("rtjack: xrun in real time audio"));
    }
}

/* release ring buffers */

static void rtJack_DeleteBuffers(RtJackGlobals *p)
{
    RtJackBuffer  **bufs;
    size_t        i;

    if (p->bufs == (RtJackBuffer**) NULL)
      return;
    bufs = p->bufs;
    p->bufs = (RtJackBuffer**) NULL;
    for (i = (size_t) 0; i < (size_t) p->nBuffers; i++) {
      if (bufs[i]->inBufs == (jack_default_audio_sample_t**) NULL &&
          bufs[i]->outBufs == (jack_default_audio_sample_t**) NULL)
        continue;
      rtJack_DestroyLock(p->csound, &(bufs[i]->csndLock));
      rtJack_DestroyLock(p->csound, &(bufs[i]->jackLock));
    }
    free((void*) bufs);
}

/* close the I/O device entirely  */
/* called only when both complete */

static CS_NOINLINE void rtclose_(CSOUND *csound)
{
    RtJackGlobals p;
    RtJackGlobals *pp;
    int           i;

    pp = (RtJackGlobals*) csound->QueryGlobalVariable(csound, "_rtjackGlobals");
    if (pp == NULL)
      return;
    *(csound->GetRtPlayUserData(csound))  = NULL;
    *(csound->GetRtRecordUserData(csound))  = NULL;
    memcpy(&p, pp, sizeof(RtJackGlobals));
    /* free globals */

    if (p.client != (jack_client_t*) NULL) {
      /* deactivate client */
      //if (p.jackState != 2) {
      //if (p.jackState == 0)
      //  csound->Sleep((size_t)
      //                ((int) ((double) (p.bufSize * p.nBuffers)
      //                        * 1000.0 / (double) p.sampleRate + 0.999)));
      jack_deactivate(p.client);
      //}
      csound->Sleep((size_t) 50);
      /* unregister and free all ports */
      if (p.inPorts != NULL) {
        for (i = 0; i < p.nChannels; i++) {
          if (p.inPorts[i] != NULL && p.jackState != 2)
            jack_port_unregister(p.client, p.inPorts[i]);
        }
      }
      if (p.outPorts != NULL) {
        for (i = 0; i < p.nChannels; i++) {
          if (p.outPorts[i] != NULL && p.jackState != 2)
            jack_port_unregister(p.client, p.outPorts[i]);
        }
      }
      /* close connection */
      if (p.jackState != 2) {
        jack_client_close(p.client);
      }
    }
    /* free copy of input and output device name */
    if (p.inDevName != NULL)
      free(p.inDevName);
    if (p.outDevName != NULL)
      free(p.outDevName);
    /* free ports and port buffer pointers */
    if (p.inPorts != NULL)
      free(p.inPorts);
    if (p.inPortBufs != NULL)
      free(p.inPortBufs);
    if (p.outPorts != NULL)
      free(p.outPorts);
    if (p.outPortBufs != NULL)
      free(p.outPortBufs);
    /* free ring buffers */
    rtJack_DeleteBuffers(&p);
    csound->DestroyGlobalVariable(csound, "_rtjackGlobals");
}

/* print error message, close connection, and terminate performance */

static CS_NORETURN void rtJack_Error(CSOUND *csound,
                                     int errCode, const char *msg)
{
    csound->ErrorMsg(csound, " *** rtjack: %s", msg);
    rtclose_(csound);
    csound->LongJmp(csound, errCode);
}

int listDevices(CSOUND *csound, CS_AUDIODEVICE *list, int isOutput){

    char            **portNames = (char**) NULL, port[64];
    unsigned long   portFlags;
    int             i, n, cnt=0;
    jack_client_t *jackClient;
    RtJackGlobals* p =
      (RtJackGlobals*) csound->QueryGlobalVariableNoCheck(csound,
                                                          "_rtjackGlobals");

    if(p->listclient == NULL)
      p->listclient = jack_client_open("list", JackNoStartServer, NULL);

    jackClient  = p->listclient;

    if(jackClient == NULL) return 0;
    portFlags = (isOutput ? (unsigned long) JackPortIsInput
                 : (unsigned long) JackPortIsOutput);

    portNames = (char**) jack_get_ports(jackClient,
                                        (char*) NULL,
                                        JACK_DEFAULT_AUDIO_TYPE,
                                        portFlags);
    if(portNames == NULL) {
      jack_client_close(jackClient);
      p->listclient = NULL;
      return 0;
    }

    memset(port, '\0', 64);
    for(i=0; portNames[i] != NULL; i++) {
      n = (int) strlen(portNames[i]);
      do {
        n--;
      } while (n > 0 && isdigit(portNames[i][n]));
      n++;
      if(strncmp(portNames[i], port, n)==0) continue;
      strncpy(port, portNames[i], n);
      port[n] = '\0';
      if (list != NULL) {
        strncpy(list[cnt].device_name, port, 63);
        snprintf(list[cnt].device_id, 63, "%s%s",
                 isOutput ? "dac:" : "adc:",port);
        list[cnt].max_nchnls = -1;
        list[cnt].isOutput = isOutput;
      }
      cnt++;
    }
    jack_client_close(jackClient);
    p->listclient = NULL;
    return cnt;
}

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    RtJackGlobals   *p;
    int             i, j;
    OPARMS oparms;
    csound->GetOParms(csound, &oparms);

    /* allocate and initialise globals */
    if (oparms.msglevel & 0x400)
      csound->Message(csound, Str("JACK real-time audio module for Csound "
                                  "by Istvan Varga\n"));
    if (csound->CreateGlobalVariable(csound, "_rtjackGlobals",
                                     sizeof(RtJackGlobals)) != 0) {
      csound->ErrorMsg(csound, Str(" *** rtjack: error allocating globals"));
      return -1;
    }
    p = (RtJackGlobals*) csound->QueryGlobalVariableNoCheck(csound,
                                                            "_rtjackGlobals");
    p->csound = csound;
    p->jackState = -1;
    strcpy(&(p->clientName[0]), "csound6");
    strcpy(&(p->inputPortName[0]), "input");
    strcpy(&(p->outputPortName[0]), "output");
    p->sleepTime = 1000;        /* this is not actually used */
    p->inDevName = (char*) NULL;
    p->outDevName = (char*) NULL;
    p->client = (jack_client_t*) NULL;
    p->inPorts = (jack_port_t**) NULL;
    p->inPortBufs = (jack_default_audio_sample_t**) NULL;
    p->outPorts = (jack_port_t**) NULL;
    p->outPortBufs = (jack_default_audio_sample_t**) NULL;
    p->bufs = (RtJackBuffer**) NULL;
    /* register options: */
    /*   client name */
    i = jack_client_name_size();
    if (i > (MAX_NAME_LEN + 1))
      i = (MAX_NAME_LEN + 1);
    csound->CreateConfigurationVariable(csound, "jack_client",
                                        (void*) &(p->clientName[0]),
                                        CSOUNDCFG_STRING, 0, NULL, &i,
                                        Str("JACK client name (default: csound6)"),
                                        NULL);
    /*   input port name */
    i = jack_port_name_size() - 3;
    if (i > (MAX_NAME_LEN + 1))
      i = (MAX_NAME_LEN + 1);
    csound->CreateConfigurationVariable(csound, "jack_inportname",
                                        (void*) &(p->inputPortName[0]),
                                        CSOUNDCFG_STRING, 0, NULL, &i,
                                        Str("JACK input port name prefix "
                                            "(default: input)"), NULL);
    /*   output port name */
    i = jack_port_name_size() - 3;
    if (i > (MAX_NAME_LEN + 1))
      i = (MAX_NAME_LEN + 1);
    csound->CreateConfigurationVariable(csound, "jack_outportname",
                                      (void*) &(p->outputPortName[0]),
                                        CSOUNDCFG_STRING, 0, NULL, &i,
                                        Str("JACK output port name prefix"
                                            " (default: output)"), NULL);
  /* sleep time */
    i = 250; j = 25000;         /* min/max value */
    csound->CreateConfigurationVariable(csound, "jack_sleep_time",
                                        (void*) &(p->sleepTime),
                                        CSOUNDCFG_INTEGER, 0, &i, &j,
                                        Str("Deprecated"), NULL);
    /* done */
    p->listclient = NULL;

    return 0;
}



PUBLIC int csoundModuleDestroy(CSOUND *csound)
{
    RtJackGlobals* p =
      (RtJackGlobals*) csound->QueryGlobalVariableNoCheck(csound,
                                                          "_rtjackGlobals");
    if(p && p->listclient) {
      jack_client_close(p->listclient);
      p->listclient = NULL;
    }
    return OK;
}



PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *drv;
    csound->module_list_add(csound,"jack", "audio");
    drv = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO");
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "jack") == 0 || strcmp(drv, "Jack") == 0 ||
          strcmp(drv, "JACK") == 0))
      return 0;
    csound->Message(csound, Str("rtaudio: JACK module enabled\n"));
    {
      /* register Csound interface functions */
      csound->SetPlayopenCallback(csound, playopen_);
      csound->SetRecopenCallback(csound, recopen_);
      csound->SetRtplayCallback(csound, rtplay_);
      csound->SetRtrecordCallback(csound, rtrecord_);
      csound->SetRtcloseCallback(csound, rtclose_);
      csound->SetAudioDeviceListCallback(csound, listDevices);
    }
    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}
