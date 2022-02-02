/*
  rtjack.c:

  Copyright (C) 2005, 2006 Istvan Varga
  2016  Victor Lazzarini

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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include <jack/jack.h>
#include <jack/midiport.h>
#include <ctype.h>
#include <sys/time.h>
#include "alphanumcmp.h"

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


#include "cs_jack.h"
static int listDevices(CSOUND *csound,
                       CS_AUDIODEVICE *list,
                       int isOutput);

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
  IGN(csound);
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
        csound->Warning(csound, "%s", Str("disabling --sched in freewheel mode\n"));
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
    size_t buf_chnls = (size_t) (p->nChannels > p->nChannels_i ?
                                 p->nChannels : p->nChannels_i);

    m = (size_t) ((p->inputEnabled ? 1 : 0) + (p->outputEnabled ? 1 : 0));
    if (!m)
      return;
    /* calculate the number of bytes to allocate */
    ofs1 = rtJack_AlignData(sizeof(RtJackBuffer*) * (size_t) p->nBuffers);
    ofs2 = rtJack_AlignData(sizeof(RtJackBuffer));
    ofs3 = rtJack_AlignData(sizeof(jack_default_audio_sample_t*)
                            * buf_chnls * m);
    nBytesPerBuf = ofs2 + ofs3;
    // VL: 16-09-21 buffers need to have number of channels set to the
    // greatest, input or output
    nBytesPerBuf += rtJack_AlignData(sizeof(jack_default_audio_sample_t)
                                     * buf_chnls
                                     * (size_t) p->bufSize
                                     * m);
    nBytes = ofs1 + (nBytesPerBuf * (size_t) p->nBuffers);
    /* allocate memory */
    ptr = (RtJackBuffer**) csound->Malloc(csound, nBytes);
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
        ptr = (void*) &(p->bufs[i]->inBufs[buf_chnls]);
      }
      if (p->outputEnabled)
        p->bufs[i]->outBufs = (jack_default_audio_sample_t**) ptr;
      ptr = (void*) p->bufs[i];
      ptr = (void*) ((char*) ptr + (long) (ofs2 + ofs3));
      for (j = (size_t) 0; j < buf_chnls; j++) {
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

static void listPorts(CSOUND *csound, int isOutput){
    int i, n = listDevices(csound, NULL, isOutput);
    CS_AUDIODEVICE *devs = csound->Malloc(csound, (size_t)n*sizeof(CS_AUDIODEVICE));
    listDevices(csound, devs, isOutput);
    if(csound->GetMessageLevel(csound) || csound->GetDebug(csound)) {
      csound->Message(csound, "Jack %s ports:\n", isOutput ? "output" : "input");
      for(i=0; i < n; i++)
        csound->Message(csound, " %d: %s (%s:%s)\n",
                        i, devs[i].device_id,
                        isOutput ? "dac" : "adc",
                        devs[i].device_name);
    }
    csound->Free(csound,devs);
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
      for (i = 0; i < p->nChannels_i; i++) {
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


static int strcmp_natural (const void *s1, const void *s2)
{
    return alphanum_cmp(* (char **) s1, * (char **) s2);
}

/* connect to JACK server, set up ports and ring buffers, */
/* activate client, and connect ports if requested */

static void openJackStreams(RtJackGlobals *p)
{
    char    buf[256];
    int     i, j, k;
    CSOUND *csound = p->csound;
    OPARMS oparms;
    csound->GetOParms(csound, &oparms);


    /* connect to JACK server */
    p->client = jack_client_open(&(p->clientName[0]), JackNoStartServer, NULL);
    if (UNLIKELY(p->client == NULL))
      rtJack_Error(csound, -1, Str("could not connect to JACK server"));

    csound->system_sr(csound, jack_get_sample_rate(p->client));
    if(oparms.msglevel || oparms.odebug)
      csound->Message(csound, "system sr: %f\n", csound->system_sr(csound,0));
    if(p->sampleRate < 0) p->sampleRate = jack_get_sample_rate(p->client);

    /* check consistency of parameters */
    if (UNLIKELY(p->nChannels < 1 || p->nChannels > 255))
      rtJack_Error(csound, -1, Str("invalid number of channels"));
    if (p->inputEnabled) {
      if (UNLIKELY(p->nChannels_i < 1 || p->nChannels > 255))
        rtJack_Error(csound, -1, Str("invalid number of input channels"));
    }
    if (UNLIKELY(p->sampleRate < 1000 || p->sampleRate > 768000))
      rtJack_Error(csound, -1, Str("invalid sample rate"));
    if (UNLIKELY(p->sampleRate != (int) jack_get_sample_rate(p->client))) {
      if(oparms.sr_override != 0.) {
        p->sampleRate = (int) jack_get_sample_rate(p->client);
      } else {
        snprintf(&(buf[0]), 256, Str("sample rate %d does not match "
                                     "JACK sample rate %d"),
                 p->sampleRate, (int) jack_get_sample_rate(p->client));
        rtJack_Error(p->csound, -1, &(buf[0]));
      }
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
      if (p->inputEnabled) {
        for (j = 0; j < p->nChannels_i; j++) {
          for (k = 0; k < p->bufSize; k++)
            p->bufs[i]->inBufs[j][k] = (jack_default_audio_sample_t) 0;
        }
      }
      if (p->outputEnabled) {
        for (j = 0; j < p->nChannels; j++) {
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
    int sortPorts = 1;   // this could be a command line flag (--unsorted-devices)
    if (p->inputEnabled) {
      listPorts(csound, 0);
      if (p->inDevNum >= 0){
        int num = p->inDevNum;
        unsigned long portFlags =  JackPortIsOutput;
        char **portNames = (char**) jack_get_ports(p->client,
                                                   (char*) NULL,
                                                   JACK_DEFAULT_AUDIO_TYPE,
                                                   portFlags);
        size_t numPorts = 0;
        if (portNames != NULL)
          for(; portNames[numPorts] != NULL; numPorts++);

        for (i = 0; i < p->nChannels_i; i++) {
          if (num+i+1 >= (int)numPorts){
            csound->Warning(csound, Str("Trying to connect input channel %d but there are "
                                        "not enough ports available\n"), num+i);
            break;
          }
          if (portNames[num+i] != NULL){
            if(csound->GetMessageLevel(csound) || csound->GetDebug(csound))
              csound->Message(csound, Str("connecting channel %d to %s\n"),
                              i, portNames[num+i]);
            if (jack_connect(p->client, portNames[num+i],
                             jack_port_name(p->inPorts[i])) != 0) {
              csound->Warning(csound,
                              Str("failed to autoconnect input channel %d\n"
                                  "(needs manual connection)"), i+1);
            }
          } else
            csound->Warning(csound, Str("jack port %d not valid\n"
                                        "failed to autoconnect input channel %d\n"
                                        "(needs manual connection)"), num+i, i+1);
        }
        jack_free(portNames);

      }
      else {
        if (strcmp(p->inDevName, "null") && p->inDevName != NULL){
          char dev[128], *dev_final, *sp;
          strNcpy(dev, p->inDevName, 128); //dev[127]='\0';
          dev_final = dev;
          sp = strchr(dev_final, '\0');
          if (!isalpha(dev_final[0])) dev_final++;
          char **portNames = (char**) jack_get_ports(p->client,
                                                     p->inDevName,
                                                     JACK_DEFAULT_AUDIO_TYPE,
                                                     JackPortIsOutput);
          size_t numPorts = 0;
          if (portNames != NULL)
            for(; portNames[numPorts] != NULL; numPorts++);
          if (numPorts == 0) {
            csound->Warning(csound, Str("Failed to autoconnect, no ports match pattern %s\n"),
                                        p->inDevName);
          }
          else {
            if (sortPorts)
              qsort (portNames, numPorts, sizeof (*portNames), strcmp_natural);
            for (i = 0; i < p->nChannels_i; i++) {
              // snprintf(sp, 128-(dev-sp), "%d", i + 1);
              dev_final = portNames[i];
              if(dev_final == NULL) {
                csound->Warning(csound, Str("Not enough ports to connect all out channels,"
                                            "only connected %d channels\n"), i);
                break;
              }
              if(csound->GetMessageLevel(csound) || csound->GetDebug(csound))
                csound->Message(csound, Str("connecting channel %d to %s\n"),
                                i, dev_final);
              if (UNLIKELY(jack_connect(p->client, dev_final,
                                        jack_port_name(p->inPorts[i])) != 0)) {
                csound->Warning(csound,
                                Str("failed to autoconnect input channel %d\n"
                                    "(needs manual connection)"), i+1);
              }
            }
          }
          *sp = (char) 0;
        } else
          csound->Message(csound, "%s", Str("input ports not connected\n"));
      }

    }
    if (p->outputEnabled) {
      listPorts(csound,1);
      if (p->outDevNum >= 0) {
        int num = p->outDevNum;
        unsigned long portFlags =  JackPortIsInput;
        // giving NULL as the regex selection, all ports will be returned.
        // the NN in dacNN is used to determine the first port
        char **portNames = (char**) jack_get_ports(p->client,
                                                   (char*) NULL,
                                                   JACK_DEFAULT_AUDIO_TYPE,
                                                   portFlags);
        // jack_get_ports returns a NULL terminated array of strings
        size_t numPorts = 0;
        if (portNames != NULL)
          for(; portNames[numPorts] != NULL; numPorts++);
        for (i = 0; i < p->nChannels; i++) {
          if (num+i+1 >= (int)numPorts){
            csound->Warning(csound, Str("Trying to connect channel %d but there are "
                                        "no ports available\n"), num+i);
            break;
          }
          if (portNames[num+i] != NULL) {
            if(csound->GetMessageLevel(csound) || csound->GetDebug(csound))
              csound->Message(csound, Str("connecting channel %d to %s\n"),
                              i,portNames[num+i]);
            if (jack_connect(p->client, jack_port_name(p->outPorts[i]),
                             portNames[num+i]) != 0) {
              csound->Warning(csound,
                              Str("failed to autoconnect output channel %d\n"
                                  "(needs manual connection)"), i+1);
            }
          } else
            csound->Warning(csound, Str("jack port %d not valid\n"
                                        "failed to autoconnect output channel %d\n"
                                        "(needs manual connection)"), num+i, i+1);
        }
        jack_free(portNames);
      }
      else {
        if (p->outDevName != NULL && strcmp(p->outDevName, "null")){
          char dev[128], *dev_final, *sp;
          strNcpy(dev, p->outDevName, 128); //dev[127]='\0';
          dev_final = dev;
          sp = strchr(dev_final, '\0');
          if (!isalpha(dev_final[0])) dev_final++;
          char **portNames = (char**) jack_get_ports(p->client,
                                                     p->outDevName,
                                                     JACK_DEFAULT_AUDIO_TYPE,
                                                     JackPortIsInput);
          size_t numPorts = 0;
          if (portNames != NULL)
            for(; portNames[numPorts] != NULL; numPorts++);
          if (numPorts == 0) {
            csound->Warning(csound, Str("Failed to autoconnect, no ports match pattern %s\n"),
                                        p->outDevName);
          }
          else {
            if (sortPorts)
              qsort (portNames, numPorts, sizeof (*portNames), strcmp_natural);
            for (i = 0; i < p->nChannels; i++) {
              // snprintf(sp, 128-(dev-sp), "%d", i + 1);
              dev_final = portNames[i];
              if(dev_final == NULL) {
                csound->Warning(csound, Str("Not enough ports to connect all out channels,"
                                            "only connected %d channels\n"), i);
                break;
              }
              if(csound->GetMessageLevel(csound) || csound->GetDebug(csound))
                csound->Message(csound, Str("connecting channel %d to %s\n"),
                                i, dev_final);
              if (UNLIKELY(jack_connect(p->client, jack_port_name(p->outPorts[i]),
                                        dev_final) != 0)) {
                  csound->Warning(csound, Str("failed to autoconnect output channel "
                                              "%d\n(needs manual connection)"), i+1);
              }
            }
          }
          *sp = (char) 0;
          jack_free(portNames);
        } else
          csound->Message(csound, "%s", Str("output ports not connected\n"));
      }
    }
    /* stream is now active */
    p->jackState = 0;
}

/* Make a copy of the device name specified for -i adc or -o dac, */
/* allocating extra space for a channel number suffix. */
/* Also set up other device parameters, and check consistency. */

static void rtJack_CopyDevParams(RtJackGlobals *p,
                                 const csRtAudioParams *parm, int isOutput)
{
    CSOUND  *csound;
    char    *s;
    size_t  nBytes;

    csound = p->csound;

    if (parm->devNum != 1024) {
      if (isOutput){
        p->outDevNum = parm->devNum;
        p->outDevName = NULL;
      }
      else {
        p->inDevNum = parm->devNum;
        p->inDevName = NULL;
      }
    }
    else {
      if (parm->devName != NULL && parm->devName[0] != (char) 0) {
        /* NOTE: this assumes max. 999 channels (the current limit is 255) */
        nBytes = strlen(parm->devName) + 4;
        s = (char*) csound->Malloc(csound, nBytes+1);
        if (UNLIKELY(s == NULL))
          rtJack_Error(csound, CSOUND_MEMORY, Str("memory allocation failure"));
        strcpy(s, parm->devName);
        if (isOutput){
          p->outDevNum = -1;
          p->outDevName = s;
        }
        else {
          p->inDevName = s;
          p->inDevNum = -1;
        }
      }
      if (isOutput && p->inputEnabled) {
        /* full duplex audio I/O: check consistency of parameters */
        if (UNLIKELY(/*p->nChannels != parm->nChannels ||*/
                     (unsigned int)p->bufSize != parm->bufSamp_SW))
          rtJack_Error(csound, -1,
                       Str("input and output parameters are not consistent"));
        if (UNLIKELY((unsigned int)((parm->bufSamp_SW / csound->GetKsmps(csound)) *
                                    csound->GetKsmps(csound)) != parm->bufSamp_SW))
          rtJack_Error(csound, -1,
                       Str("period size (-b) must be an integer "
                           "multiple of ksmps"));
      }
    }
    p->sampleRate = (int) parm->sampleRate;
    if (UNLIKELY((float) p->sampleRate != parm->sampleRate))
      rtJack_Error(csound, -1, Str("sample rate must be an integer"));
    if (isOutput) p->nChannels = parm->nChannels;
    else p->nChannels_i = parm->nChannels;

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
    rtJack_CopyDevParams(p, parm, 0);
    p->inputEnabled = 1;
    /* allocate pointers to input ports */
    p->inPorts = (jack_port_t**)
      csound->Calloc(csound, (size_t) p->nChannels_i* sizeof(jack_port_t*));
    if (UNLIKELY(p->inPorts == NULL))
      rtJack_Error(p->csound, CSOUND_MEMORY, Str("memory allocation failure"));
    /* allocate pointers to input port buffers */
    p->inPortBufs = (jack_default_audio_sample_t**)
      csound->Calloc(csound,
                     (size_t)p->nChannels_i * sizeof(jack_default_audio_sample_t*));
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
    rtJack_CopyDevParams(p, parm, 1);

    p->outputEnabled = 1;
    /* allocate pointers to output ports */
    p->outPorts = (jack_port_t**)
      csound->Calloc(csound, (size_t) p->nChannels* sizeof(jack_port_t*));
    if (UNLIKELY(p->outPorts == NULL))
      rtJack_Error(p->csound, CSOUND_MEMORY, Str("memory allocation failure"));
    /* allocate pointers to output port buffers */
    p->outPortBufs = (jack_default_audio_sample_t**)
      csound->Calloc(csound,
                     (size_t) p->nChannels* sizeof(jack_default_audio_sample_t*));
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
      for (i = 0; i < p->nChannels_i; i++)
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
      if (p->inputEnabled) {
        for (j = 0; j < p->nChannels_i; j++) {
          jack_default_audio_sample_t   *srcp, *dstp;
          srcp = &(p->inPortBufs[j][i]);
          dstp = &(p->bufs[p->jackBufCnt]->inBufs[j][p->jackBufPos]);
          for (k = 0; k < l; k++)
            dstp[k] = srcp[k];
        }
      }
      if (p->outputEnabled) {
        for (j = 0; j < p->nChannels; j++) {
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

    csound->ErrorMsg(csound, "%s", Str(" *** rtjack: connection to JACK "
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
    nframes = bytes_ / (p->nChannels_i * (int) sizeof(MYFLT));
    bufpos = p->csndBufPos;
    bufcnt = p->csndBufCnt;
    for (i = j = 0; i < nframes; i++) {
      if (bufpos == 0) {
        /* wait until there is enough data in ring buffer */
        /* VL 28.03.15 -- timeout after wait for 10 buffer
           lengths */
        int ret = rtJack_LockTimeout(csound, &(p->bufs[bufcnt]->csndLock),
                                     10000*(nframes/csound->GetSr(csound)));
        if (ret) {
          memset(inbuf_, 0, bytes_);
          OPARMS oparms;
          csound->GetOParms(csound, &oparms);
          if (UNLIKELY(oparms.msglevel & 4))
            csound->Warning(csound, "%s", Str("rtjack: input audio timeout"));
          return bytes_;
        }
      }
      /* copy audio data */
      for (k = 0; k < p->nChannels_i; k++)
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
      if (UNLIKELY(oparms.msglevel & 4))
        csound->Warning(csound, "%s", Str("rtjack: xrun in real time audio"));
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
      csound->Warning(csound, "%s", Str("rtjack: xrun in real time audio"));
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
    p->csound->Free(p->csound,(void*) bufs);
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
        for (i = 0; i < p.nChannels_i; i++) {
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
      csound->Free(csound,p.inDevName);
    if (p.outDevName != NULL)
      csound->Free(csound,p.outDevName);
    /* free ports and port buffer pointers */
    if (p.inPorts != NULL)
      csound->Free(csound,p.inPorts);
    if (p.inPortBufs != NULL)
      csound->Free(csound,p.inPortBufs);
    if (p.outPorts != NULL)
      csound->Free(csound,p.outPorts);
    if (p.outPortBufs != NULL)
      csound->Free(csound,p.outPortBufs);
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

    if (p->listclient == NULL)
      p->listclient = jack_client_open("list", JackNoStartServer, NULL);

    jackClient  = p->listclient;

    if (jackClient == NULL) return 0;
    portFlags = (isOutput ? (unsigned long) JackPortIsInput
                 : (unsigned long) JackPortIsOutput);

    portNames = (char**) jack_get_ports(jackClient,
                                        (char*) NULL,
                                        JACK_DEFAULT_AUDIO_TYPE,
                                        portFlags);
    if (portNames == NULL) {
      jack_client_close(jackClient);
      p->listclient = NULL;
      return 0;
    }

    memset(port, '\0', 64);
    for(i=0; portNames[i] != NULL; i++, cnt++) {
      n = (int) strlen(portNames[i]);
      strNcpy(port, portNames[i], n+1);
      //port[n] = '\0';
      if (list != NULL) {
        strNcpy(list[cnt].device_name, port, 63);
        snprintf(list[cnt].device_id, 63, "%s%d",
                 isOutput ? "dac" : "adc", cnt);
        list[cnt].max_nchnls = 1;
        list[cnt].isOutput = isOutput;
      }
    }
    jack_free(portNames);
    jack_client_close(jackClient);
    p->listclient = NULL;
    return cnt;
}

typedef struct RtJackMIDIGlobals_ {
  char clientName[MAX_NAME_LEN];
  char inputPortName[MAX_NAME_LEN];
  char outputPortName[MAX_NAME_LEN];
} RtJackMIDIGlobals;


/* module interface functions */
PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    RtJackGlobals   *p;
    int             i, j;
    OPARMS oparms;
    csound->GetOParms(csound, &oparms);

    /* allocate and initialise globals */
    if (UNLIKELY(oparms.msglevel & 0x400))
      csound->Message(csound, "%s",
                      Str("JACK real-time audio module for Csound\n"));
    if (UNLIKELY(csound->CreateGlobalVariable(csound, "_rtjackGlobals",
                                              sizeof(RtJackGlobals)) != 0)) {
      csound->ErrorMsg(csound, "%s", Str(" *** rtjack: error allocating globals"));
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


    RtJackMIDIGlobals *pm;
    if (oparms.msglevel & 0x400)
      csound->Message(csound, "%s", Str("JACK MIDI module for Csound\n"));
    if (csound->CreateGlobalVariable(csound, "_rtjackMIDIGlobals",
                                     sizeof(RtJackMIDIGlobals)) != 0) {
      csound->ErrorMsg(csound, "%s",
                       Str(" *** rtjack MIDI: error allocating globals"));
      return -1;
    }
    pm = (RtJackMIDIGlobals*)
      csound->QueryGlobalVariableNoCheck(csound, "_rtjackMIDIGlobals");

    strcpy(&(pm->clientName[0]), "csound6-midi");
    strcpy(&(pm->inputPortName[0]), "port");
    strcpy(&(pm->outputPortName[0]), "port");
    /*   client name */
    i = jack_client_name_size();
    if (i > (MAX_NAME_LEN + 1))
      i = (MAX_NAME_LEN + 1);
    csound->CreateConfigurationVariable(csound, "jack_midi_client",
                                        (void*) &(pm->clientName[0]),
                                        CSOUNDCFG_STRING, 0, NULL, &i,
                                        Str("JACK MIDI client name prefix"
                                            " (default: csound6-midi)"),
                                        NULL);

    /*   input port name */
    i = jack_port_name_size() - 3;
    if (i > (MAX_NAME_LEN + 1))
      i = (MAX_NAME_LEN + 1);
    csound->CreateConfigurationVariable(csound, "jack_midi_inportname",
                                        (void*) &(pm->inputPortName[0]),
                                        CSOUNDCFG_STRING, 0, NULL, &i,
                                        Str("JACK MIDI input port name"
                                            "(default: port)"), NULL);
    /*   output port name */
    i = jack_port_name_size() - 3;
    if (i > (MAX_NAME_LEN + 1))
      i = (MAX_NAME_LEN + 1);
    csound->CreateConfigurationVariable(csound, "jack_midi_outportname",
                                        (void*) &(pm->outputPortName[0]),
                                        CSOUNDCFG_STRING, 0, NULL, &i,
                                        Str("JACK MIDI output port name"
                                            " (default: port)"), NULL);

    return 0;
}

#define JACK_MIDI_BUFFSIZE 1024
typedef struct jackMidiDevice_ {
  jack_client_t *client;
  jack_port_t *port;
  CSOUND *csound;
  void *cb;
} jackMidiDevice;

int MidiInProcessCallback(jack_nframes_t nframes, void *userData){

    jack_midi_event_t event;
    jackMidiDevice *dev = (jackMidiDevice *) userData;
    CSOUND *csound = dev->csound;
    int n = 0;
    while(jack_midi_event_get(&event,
                              jack_port_get_buffer(dev->port,nframes),
                              n++) == 0) {
      if (UNLIKELY(csound->WriteCircularBuffer(csound,dev->cb,
                                              event.buffer,event.size)
                  != (int) event.size)){
        csound->Warning(csound, "%s", Str("Jack MIDI module: buffer overflow"));
        return 1;
      }
    }
    return 0;
}


static int midi_in_open(CSOUND *csound,
                        void **userData,
                        const char *devName){

    jack_client_t *jack_client;
    jack_port_t  *jack_port;
    jackMidiDevice *dev;
    RtJackMIDIGlobals *pm;
    char clientName[MAX_NAME_LEN+3];

    pm =
      (RtJackMIDIGlobals*) csound->QueryGlobalVariableNoCheck(csound,
                                                              "_rtjackMIDIGlobals");

    sprintf(clientName, "%s_in", pm->clientName);
    if (UNLIKELY((jack_client =
                 jack_client_open(clientName, 0, NULL)) == NULL)){
      *userData = NULL;
      csound->ErrorMsg(csound, "%s",
                       Str("Jack MIDI module: failed to create client for input"));
      return NOTOK;
    }


    if (UNLIKELY((jack_port = jack_port_register(jack_client,pm->inputPortName,
                                                JACK_DEFAULT_MIDI_TYPE,
                                                JackPortIsInput|JackPortIsTerminal,
                                                0)) == NULL)){
      jack_client_close(jack_client);
      *userData = NULL;
      csound->ErrorMsg(csound, "%s",
                       Str("Jack MIDI module: failed to register input port"));
      return NOTOK;
    }

    dev = (jackMidiDevice *) csound->Calloc(csound,sizeof(jackMidiDevice));
    dev->client = jack_client;
    dev->port = jack_port;
    dev->csound = csound;
    dev->cb = csound->CreateCircularBuffer(csound,
                                           JACK_MIDI_BUFFSIZE,
                                           sizeof(char));

    if (UNLIKELY(jack_set_process_callback(jack_client,
                                          MidiInProcessCallback,
                                          (void*) dev) != 0)){
      jack_client_close(jack_client);
      csound->DestroyCircularBuffer(csound, dev->cb);
      csound->Free(csound, dev);
      csound->ErrorMsg(csound,
                       "%s", Str("Jack MIDI module: failed to set input"
                           " process callback"));
      return NOTOK;
    }

    if (UNLIKELY(jack_activate(jack_client) != 0)){
      jack_client_close(jack_client);
      csound->DestroyCircularBuffer(csound, dev->cb);
      csound->Free(csound, dev);
      *userData = NULL;
      csound->ErrorMsg(csound, "%s",
                       Str("Jack MIDI module: failed to activate input"));
      return NOTOK;
    }

    if (strcmp(devName,"0")){
      if (UNLIKELY(jack_connect(jack_client,devName,
                                jack_port_name(dev->port)) != 0)){
        csound->Warning(csound,  Str("Jack MIDI module: failed to connect to: %s"),
                        devName);
      }
    }

    *userData = (void *) dev;
    return OK;
}

static int midi_in_read(CSOUND *csound,
                        void *userData, unsigned char *buf, int nbytes)
{
    jackMidiDevice *dev = (jackMidiDevice *) userData;
    return csound->ReadCircularBuffer(csound,dev->cb,buf,nbytes);
}

static int midi_in_close(CSOUND *csound, void *userData){
    jackMidiDevice *dev = (jackMidiDevice *) userData;
    if(dev != NULL) {
      jack_port_disconnect(dev->client, dev->port);
      jack_client_close(dev->client);
      csound->DestroyCircularBuffer(csound, dev->cb);
      csound->Free(csound, dev);
    }
    return OK;
}

int MidiOutProcessCallback(jack_nframes_t nframes, void *userData){

    jackMidiDevice *dev = (jackMidiDevice *) userData;
    CSOUND *csound = dev->csound;
    jack_midi_data_t buf[JACK_MIDI_BUFFSIZE];
    int n;
    jack_midi_clear_buffer(jack_port_get_buffer(dev->port,nframes));
    while((n = csound->ReadCircularBuffer(csound,dev->cb,
                                          buf,
                                          JACK_MIDI_BUFFSIZE)) != 0) {
      if(UNLIKELY(jack_midi_event_write(jack_port_get_buffer(dev->port,nframes),
                                        0, buf,n) != 0)){
        csound->Warning(csound, "%s", Str("Jack MIDI module: out buffer overflow"));
        return 1;
      }
    }
    return 0;
}


static int midi_out_open(CSOUND *csound, void **userData,
                         const char *devName)
{
    jack_client_t *jack_client;
    jack_port_t  *jack_port;
    jackMidiDevice *dev;
    RtJackMIDIGlobals *pm;
    char clientName[MAX_NAME_LEN+4];

    pm =
      (RtJackMIDIGlobals*) csound->QueryGlobalVariableNoCheck(csound,
                                                              "_rtjackMIDIGlobals");
    sprintf(clientName, "%s_out", pm->clientName);
    if(UNLIKELY((jack_client =
                 jack_client_open(clientName, 0, NULL)) == NULL)){
      *userData = NULL;
      csound->ErrorMsg(csound, "%s",
                       Str("Jack MIDI module: failed to create client for output"));
      return NOTOK;
    }


    if(UNLIKELY((jack_port = jack_port_register(jack_client,pm->outputPortName,
                                                JACK_DEFAULT_MIDI_TYPE,
                                                JackPortIsOutput,
                                                0)) == NULL)){
      jack_client_close(jack_client);
      *userData = NULL;
      csound->ErrorMsg(csound, "%s",
                       Str("Jack MIDI module: failed to register output port"));
      return NOTOK;
    }

    dev = (jackMidiDevice *) csound->Calloc(csound,sizeof(jackMidiDevice));
    dev->client = jack_client;
    dev->port = jack_port;
    dev->csound = csound;
    dev->cb = csound->CreateCircularBuffer(csound,
                                           JACK_MIDI_BUFFSIZE,
                                           sizeof(char));

    if(UNLIKELY(jack_set_process_callback(jack_client,
                                          MidiOutProcessCallback,
                                          (void*) dev) != 0)){
      jack_client_close(jack_client);
      csound->DestroyCircularBuffer(csound, dev->cb);
      csound->Free(csound, dev);
      csound->ErrorMsg(csound,
                       "%s", Str("Jack MIDI module: failed to set input"
                           " process callback"));
      return NOTOK;
    }

    if(UNLIKELY(jack_activate(jack_client) != 0)){
      jack_client_close(jack_client);
      csound->DestroyCircularBuffer(csound, dev->cb);
      csound->Free(csound, dev);
      *userData = NULL;
      csound->ErrorMsg(csound, "%s",
                       Str("Jack MIDI module: failed to activate output"));
      return NOTOK;
    }

    if(strcmp(devName,"0")){
      if(UNLIKELY(jack_connect(jack_client,
                               jack_port_name(dev->port),devName) != 0)){
        csound->Warning(csound,
                         Str("Jack MIDI out module: failed to connect to: %s"),
                        devName);
      }
    }

    *userData = (void *) dev;
    return OK;
}

static int midi_out_write(CSOUND *csound,
                          void *userData, const unsigned char *buf, int nbytes)
{
    jackMidiDevice *dev = (jackMidiDevice *) userData;
    return csound->WriteCircularBuffer(csound,dev->cb,buf,nbytes);
}

static int midi_out_close(CSOUND *csound, void *userData){
    jackMidiDevice *dev = (jackMidiDevice *) userData;
    if(dev != NULL) {
      jack_port_disconnect(dev->client, dev->port);
      jack_client_close(dev->client);
      csound->DestroyCircularBuffer(csound, dev->cb);
      csound->Free(csound, dev);
    }
    return OK;
}

static int listDevicesM(CSOUND *csound, CS_MIDIDEVICE *list,
                        int isOutput){
    char            **portNames = (char**) NULL, port[64];
    unsigned long   portFlags;
    int             i, n, cnt=0;
    jack_client_t *jackClient;
    RtJackGlobals* p =
      (RtJackGlobals*) csound->QueryGlobalVariableNoCheck(csound,
                                                          "_rtjackGlobals");
    char *drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));

    if(p->listclient == NULL)
      p->listclient = jack_client_open("list", JackNoStartServer, NULL);

    jackClient  = p->listclient;

    if(jackClient == NULL) return 0;
    portFlags = (isOutput ? (unsigned long) JackPortIsInput
                 : (unsigned long) JackPortIsOutput);

    portNames = (char**) jack_get_ports(jackClient,
                                        (char*) NULL,
                                        JACK_DEFAULT_MIDI_TYPE,
                                        portFlags);
    if(portNames == NULL) {
      jack_client_close(jackClient);
      p->listclient = NULL;
      return 0;
    }

    memset(port, '\0', 64);
    for(i=0; portNames[i] != NULL; i++, cnt++) {
      n = (int) strlen(portNames[i]);
      strNcpy(port, portNames[i], n+1);
      //port[n] = '\0';
      if (list != NULL) {
        strNcpy(list[cnt].device_name, port, 64);
        snprintf(list[cnt].device_id, 63, "%d", cnt);
        list[cnt].isOutput = isOutput;
        strcpy(list[i].interface_name, "");
        strNcpy(list[i].midi_module, drv, 64);
      }
    }
    jack_free(portNames);
    jack_client_close(jackClient);
    p->listclient = NULL;
    return cnt;
    return 0;
}

/*
  static int listDevices(CSOUND *csound, CS_MIDIDEVICE *list, int isOutput){
  int i, cnt;
  PmDeviceInfo  *info;
  char tmp[64];
  char *drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));

  if (UNLIKELY(start_portmidi(csound) != 0))
  return 0;

  cnt = portMidi_getDeviceCount(isOutput);
  if (list == NULL) return cnt;
  for (i = 0; i < cnt; i++) {
  info = portMidi_getDeviceInfo(i, isOutput);
  if(info->name != NULL)
  strNcpy(list[i].device_name, info->name, 64);
  snprintf(tmp, 64, "%d", i);
  strNcpy(list[i].device_id, tmp, 64);
  list[i].isOutput = isOutput;
  if (info->interf != NULL)
  strNcpy(list[i].interface_name, info->interf, 64);
  else strcpy(list[i].interface_name, "");
  strNcpy(list[i].midi_module, drv, 64);
  }
  return cnt;
  }
*/


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
   OPARMS O;
    csound->GetOParms(csound, &O);
    csound->module_list_add(csound,"jack", "audio");
    drv = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO");
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "jack") == 0 || strcmp(drv, "Jack") == 0 ||
          strcmp(drv, "JACK") == 0))
      return 0;
    if(O.msglevel || O.odebug)
     csound->Message(csound, "%s", Str("rtaudio: JACK module enabled\n"));
    {
      /* register Csound interface functions */
      csound->SetPlayopenCallback(csound, playopen_);
      csound->SetRecopenCallback(csound, recopen_);
      csound->SetRtplayCallback(csound, rtplay_);
      csound->SetRtrecordCallback(csound, rtrecord_);
      csound->SetRtcloseCallback(csound, rtclose_);
      csound->SetAudioDeviceListCallback(csound, listDevices);
    }

    drv = (char*) csound->QueryGlobalVariable(csound, "_RTMIDI");
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "jack") == 0 || strcmp(drv, "Jack") == 0 ||
          strcmp(drv, "JACK") == 0))
      return 0;
    if(O.msglevel || O.odebug)
     csound->Message(csound, "%s", Str("rtmidi: JACK module enabled\n"));
    {
      csound->SetExternalMidiInOpenCallback(csound, midi_in_open);
      csound->SetExternalMidiReadCallback(csound, midi_in_read);
      csound->SetExternalMidiInCloseCallback(csound, midi_in_close);
      csound->SetExternalMidiOutOpenCallback(csound, midi_out_open);
      csound->SetExternalMidiWriteCallback(csound, midi_out_write);
      csound->SetExternalMidiOutCloseCallback(csound, midi_out_close);
      csound->SetMIDIDeviceListCallback(csound,listDevicesM);
    }

    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}
