/*
    rtjack.c:

    Copyright (C) 2005 Istvan Varga

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
/* no #ifdef, should always have these on systems where JACK is available */
#include <unistd.h>
#include <stdint.h>
#include "csdl.h"
#include "soundio.h"

#ifdef LINUX
#include <sched.h>
#endif

typedef struct {
    int                 active;         /* 0: unused, 1: active, 2: running */
    csRtAudioParams     parm;           /* parameters (sample rate etc.)    */
    int                 (*SampleConvFunc)(int, int, int, int, void*, void*);
    void                **ringbuffers;  /* nChannels array of ring buffers  */
    int                 rb_readpos;     /* ring buffer read position        */
    int                 rb_writepos;    /* ring buffer write position       */
    int                 sleep_time;     /* sleep time in microseconds       */
    jack_port_t         **jackPorts;    /* nChannels array of JACK ports    */
    int                 frames_ahead;   /* difference between ring buffer   */
} JackStreamParams_t;                   /*   write and read position        */

typedef struct {
    void                *csound;        /* Csound instance pointer          */
    jack_client_t       *client;        /* JACK client pointer              */
    int                 active;         /* non-zero: running                */
    JackStreamParams_t  r;              /* input (record) stream            */
    JackStreamParams_t  p;              /* output (playback) stream         */
} CsoundJackClient_t;

/* sample conversion routines */

static int float_to_float_r(int nSmps, int nChannels, int bufSize, int ofs,
                            float **inBufs, float *outBuf);
static int float_to_double_r(int nSmps, int nChannels, int bufSize, int ofs,
                             float **inBufs, double *outBuf);
static int double_to_float_r(int nSmps, int nChannels, int bufSize, int ofs,
                             double **inBufs, float *outBuf);
static int double_to_double_r(int nSmps, int nChannels, int bufSize, int ofs,
                              double **inBufs, double *outBuf);

static int float_to_float_p(int nSmps, int nChannels, int bufSize, int ofs,
                            float *inBuf, float **outBufs);
static int float_to_double_p(int nSmps, int nChannels, int bufSize, int ofs,
                             float *inBuf, double **outBufs);
static int double_to_float_p(int nSmps, int nChannels, int bufSize, int ofs,
                             double *inBuf, float **outBufs);
static int double_to_double_p(int nSmps, int nChannels, int bufSize, int ofs,
                              double *inBuf, double **outBufs);

/* callback functions */

static int bufSizeCallback(jack_nframes_t nframes, void *arg)
{
    CsoundJackClient_t  *client;
    CSOUND              *p;

    client = (CsoundJackClient_t*) arg;
    p = (CSOUND*) (client->csound);
    if ((client->r.parm.bufSamp_SW > 0 &&
         client->r.parm.bufSamp_SW != (int) nframes) ||
        (client->p.parm.bufSamp_SW > 0 &&
         client->p.parm.bufSamp_SW != (int) nframes)) {
      p->Message(p, " *** error: JACK buffer size does not match "
                    "Csound period size\n");
      return -1;
    }
    return 0;
}

static int sampleRateCallback(jack_nframes_t nframes, void *arg)
{
    CsoundJackClient_t  *client;
    CSOUND              *p;

    client = (CsoundJackClient_t*) arg;
    p = (CSOUND*) (client->csound);
    if (((int) (client->r.parm.sampleRate + 0.5f) > 0 &&
         (int) (client->r.parm.sampleRate + 0.5f) != (int) nframes) ||
        ((int) (client->r.parm.sampleRate + 0.5f) > 0 &&
         (int) (client->r.parm.sampleRate + 0.5f) != (int) nframes)) {
      p->Message(p, " *** error: JACK sample rate does not match "
                    "Csound sample rate\n");
      return -1;
    }
    return 0;
}

static void freeWheelCallback(int starting, void *arg)
{
    CsoundJackClient_t  *client;
    CSOUND              *p;

    client = (CsoundJackClient_t*) arg;
    p = (CSOUND*) (client->csound);
#ifdef LINUX
    if (starting) {
      if (sched_getscheduler(0) != SCHED_OTHER) {
        struct sched_param sp;
        p->Message(p, " *** WARNING: disabling --sched in freewheel mode\n");
        memset(&sp, 0, sizeof(struct sched_param));
        sp.sched_priority = 0;
        sched_setscheduler(0, SCHED_OTHER, &sp);
      }
    }
#endif
}

#define GETVAR(x)   (csound->QueryGlobalVariable(csound, x))

static void closeJackStream(CsoundJackClient_t *client, int playback)
{
    JackStreamParams_t  *pp;
    int                 i;

    if (playback)
      pp = &(client->p);
    else
      pp = &(client->r);
    /* unregister and free all ports */
    if (pp->jackPorts != NULL) {
      for (i = 0; i < pp->parm.nChannels; i++) {
        if (pp->jackPorts[i] != NULL)
          jack_port_unregister(client->client, pp->jackPorts[i]);
      }
      free(pp->jackPorts);
    }
    /* free ring buffers */
    if (pp->ringbuffers != NULL) {
      for (i = 0; i < pp->parm.nChannels; i++) {
        if (pp->ringbuffers[i] != NULL)
          free(pp->ringbuffers[i]);
      }
      free(pp->ringbuffers);
    }
    /* reset structure */
    memset(pp, 0, sizeof(JackStreamParams_t));
    pp->ringbuffers = NULL;
    pp->jackPorts = NULL;
}

static int openJackStream(CsoundJackClient_t *client, csRtAudioParams *parm,
                          int playback)
{
    JackStreamParams_t  *pp;
    CSOUND              *p, *csound;
    int                 i, j;
    char                buf[128], buf1[128];

    p = csound = (CSOUND*) (client->csound);
    if (playback)
      pp = &(client->p);
    else
      pp = &(client->r);
    memcpy(&(pp->parm), parm, sizeof(csRtAudioParams));
    pp->ringbuffers = NULL;
    pp->sleep_time = *((int*) GETVAR("::JACK::sleepTime"));
    pp->jackPorts = NULL;
    /* select sample conversion function */
    if (playback) {
      if (sizeof(jack_default_audio_sample_t) == sizeof(float)) {
        if (p->GetSizeOfMYFLT() == (int) sizeof(float))
          pp->SampleConvFunc =
            (int (*)(int, int, int, int, void*, void*)) float_to_float_p;
        else
          pp->SampleConvFunc =
            (int (*)(int, int, int, int, void*, void*)) double_to_float_p;
      }
      else {
        if (p->GetSizeOfMYFLT() == (int) sizeof(float))
          pp->SampleConvFunc =
            (int (*)(int, int, int, int, void*, void*)) float_to_double_p;
        else
          pp->SampleConvFunc =
            (int (*)(int, int, int, int, void*, void*)) double_to_double_p;
      }
    }
    else {
      if (sizeof(jack_default_audio_sample_t) == sizeof(float)) {
        if (p->GetSizeOfMYFLT() == (int) sizeof(float))
          pp->SampleConvFunc =
            (int (*)(int, int, int, int, void*, void*)) float_to_float_r;
        else
          pp->SampleConvFunc =
            (int (*)(int, int, int, int, void*, void*)) float_to_double_r;
      }
      else {
        if (p->GetSizeOfMYFLT() == (int) sizeof(float))
          pp->SampleConvFunc =
            (int (*)(int, int, int, int, void*, void*)) double_to_float_r;
        else
          pp->SampleConvFunc =
            (int (*)(int, int, int, int, void*, void*)) double_to_double_r;
      }
    }
    /* check buffer sizes */
    if (pp->parm.bufSamp_HW < 64 || pp->parm.bufSamp_HW > 65536) {
      p->Message(p, " *** openJackStream(): invalid buffer size %d\n",
                    pp->parm.bufSamp_HW);
      return CSOUND_ERROR;
    }
    if (pp->parm.bufSamp_HW & (pp->parm.bufSamp_HW - 1)) {
      p->Message(p, " *** openJackStream(): invalid buffer size %d: "
                    "must be power of two\n", pp->parm.bufSamp_HW);
      return CSOUND_ERROR;
    }
    if (pp->parm.bufSamp_SW < 64 || pp->parm.bufSamp_SW > 65536) {
      p->Message(p, " *** openJackStream(): invalid period size %d\n",
                    pp->parm.bufSamp_SW);
      return CSOUND_ERROR;
    }
    if (pp->parm.bufSamp_SW & (pp->parm.bufSamp_SW - 1)) {
      p->Message(p, " *** openJackStream(): invalid period size %d: "
                    "must be power of two\n", pp->parm.bufSamp_SW);
      return CSOUND_ERROR;
    }
    if (pp->parm.bufSamp_SW >= pp->parm.bufSamp_HW) {
      p->Message(p, " *** openJackStream(): period size must be less than "
                    "buffer size\n");
      return CSOUND_ERROR;
    }
    if (pp->parm.bufSamp_SW != (int) jack_get_buffer_size(client->client)) {
      if (jack_set_buffer_size(client->client,
                               (jack_nframes_t) pp->parm.bufSamp_SW) != 0) {
        p->Message(p,
                   " *** openJackStream(): error setting JACK buffer size\n");
        return CSOUND_ERROR;
      }
      if (pp->parm.bufSamp_SW != (int) jack_get_buffer_size(client->client)) {
        p->Message(p, " *** openJackStream(): period size %d does not match "
                      "JACK buffer size %d\n", pp->parm.bufSamp_SW,
                      (int) jack_get_buffer_size(client->client));
        return CSOUND_ERROR;
      }
    }
    if (!playback && (((pp->parm.bufSamp_SW / p->GetKsmps(p)) * p->GetKsmps(p))
                      != pp->parm.bufSamp_SW)) {
      p->Message(p, " *** openJackStream(): period size %d is not an integer "
                    "multiple of ksmps (%d)\n",
                    pp->parm.bufSamp_SW, p->GetKsmps(p));
      return CSOUND_ERROR;
    }
    /* check sample rate */
    if (pp->parm.sampleRate < 1000.0f || pp->parm.sampleRate > 768000.0f) {
      p->Message(p, " *** openJackStream(): invalid sample rate %f\n",
                    (double) pp->parm.sampleRate);
      return CSOUND_ERROR;
    }
    if (pp->parm.sampleRate != (float) ((int) (pp->parm.sampleRate + 0.5f))) {
      p->Message(p, " *** openJackStream(): invalid sample rate %f: "
                    "must be integer\n", (double) pp->parm.sampleRate);
      return CSOUND_ERROR;
    }
    if ((int) (pp->parm.sampleRate + 0.5f)
        != (int) jack_get_sample_rate(client->client)) {
      p->Message(p, " *** openJackStream(): sample rate %d does not match "
                    "JACK sample rate %d\n", (int) (pp->parm.sampleRate + 0.5f),
                    (int) jack_get_sample_rate(client->client));
      return CSOUND_ERROR;
    }
    /* register callback functions */
    if (jack_set_buffer_size_callback(client->client,
                                      (JackBufferSizeCallback) bufSizeCallback,
                                      (void*) client) != 0) {
      p->Message(p, " *** openJackStream(): error setting buffer size "
                    "callback function\n");
      return CSOUND_ERROR;
    }
    if (jack_set_sample_rate_callback(client->client, (JackSampleRateCallback)
                                                        sampleRateCallback,
                                                      (void*) client) != 0) {
      p->Message(p, " *** openJackStream(): error setting sample rate "
                    "callback function\n");
      return CSOUND_ERROR;
    }
    if (jack_set_freewheel_callback(client->client,
                                    (JackFreewheelCallback) freeWheelCallback,
                                    (void*) client) != 0) {
      p->Message(p, " *** openJackStream(): error setting freewheel "
                    "callback function\n");
      return CSOUND_ERROR;
    }
    /* allocate ring buffers */
    pp->ringbuffers = (void**) malloc(sizeof(void*) * pp->parm.nChannels);
    if (pp->ringbuffers == NULL) {
      sprintf(buf, " *** openJackStream(): memory allocation failure\n");
      goto err_return;
    }
    for (i = 0; i < pp->parm.nChannels; i++)
      pp->ringbuffers[i] = NULL;
    i = 0;
    do {
      pp->ringbuffers[i] = (void*) malloc(sizeof(jack_default_audio_sample_t)
                                          * pp->parm.bufSamp_HW);
      if (pp->ringbuffers[i] == NULL) {
        sprintf(buf, " *** openJackStream(): memory allocation failure\n");
        goto err_return;
      }
      for (j = 0; j < pp->parm.bufSamp_HW; j++)
        ((jack_default_audio_sample_t**) pp->ringbuffers)[i][j] =
          (jack_default_audio_sample_t) 0;
    } while (++i < pp->parm.nChannels);
    pp->rb_readpos = 0;
    pp->rb_writepos = pp->parm.bufSamp_HW >> 1;
    pp->frames_ahead = pp->parm.bufSamp_HW >> 1;
    /* register ports */
    pp->jackPorts = (jack_port_t**) malloc(sizeof(jack_port_t*)
                                           * pp->parm.nChannels);
    if (pp->jackPorts == NULL) {
      sprintf(buf, " *** openJackStream(): memory allocation failure\n");
      goto err_return;
    }
    for (i = 0; i < pp->parm.nChannels; i++)
      pp->jackPorts[i] = NULL;
    for (i = 0; i < pp->parm.nChannels; i++) {
      if (playback) {
        sprintf(buf1, "%s%d", (char*) GETVAR("::JACK::outputPortName"), i + 1);
        pp->jackPorts[i] = jack_port_register(client->client, &(buf1[0]),
                                              JACK_DEFAULT_AUDIO_TYPE,
                                              JackPortIsOutput, 0UL);
        if (pp->jackPorts[i] == (jack_port_t*) NULL) {
          sprintf(buf, " *** openJackStream(): error registering port %s\n",
                       &(buf1[0]));
          goto err_return;
        }
      }
      else {
        sprintf(buf1, "%s%d", (char*) GETVAR("::JACK::inputPortName"), i + 1);
        pp->jackPorts[i] = jack_port_register(client->client, &(buf1[0]),
                                              JACK_DEFAULT_AUDIO_TYPE,
                                              JackPortIsInput, 0UL);
        if (pp->jackPorts[i] == (jack_port_t*) NULL) {
          sprintf(buf, " *** openJackStream(): error registering port %s\n",
                       &(buf1[0]));
          goto err_return;
        }
      }
    }
    /* mark stream as active and report success */
    pp->active = 1;
    return CSOUND_SUCCESS;

 err_return:
    closeJackStream(client, playback);
    p->Message(p, &(buf[0]));
    return CSOUND_ERROR;
}

/* module interface functions */

int csoundModuleCreate(CSOUND *csound)
{
    CSOUND *p = csound;
    void    *ptr;
    int     i, j;

    p->Message(csound, "JACK real-time audio module for Csound "
                       "by Istvan Varga\n");
    /* register options: */
    /*   client name */
    i = jack_client_name_size();
    if (i > 33) i = 33;
    p->CreateGlobalVariable(csound, "::JACK::clientName", (size_t) i);
    ptr = GETVAR("::JACK::clientName");
    strcpy((char*) ptr, "csound5");
    p->CreateConfigurationVariable(csound, "jack_client", ptr,
                                   CSOUNDCFG_STRING, 0, NULL, &i,
                                   "JACK client name (default: csound5)", NULL);
    /*   input port name */
    i = jack_port_name_size();
    if (i > 33) i = 33;
    p->CreateGlobalVariable(csound, "::JACK::inputPortName", (size_t) i);
    ptr = GETVAR("::JACK::inputPortName");
    strcpy((char*) ptr, "input");
    p->CreateConfigurationVariable(csound, "jack_inportname", ptr,
                                   CSOUNDCFG_STRING, 0, NULL, &i,
                                   "JACK input port name prefix "
                                   "(default: input)", NULL);
    /*   output port name */
    i = jack_port_name_size();
    if (i > 33) i = 33;
    p->CreateGlobalVariable(csound, "::JACK::outputPortName", (size_t) i);
    ptr = GETVAR("::JACK::outputPortName");
    strcpy((char*) ptr, "output");
    p->CreateConfigurationVariable(csound, "jack_outportname", ptr,
                                   CSOUNDCFG_STRING, 0, NULL, &i,
                                   "JACK output port name prefix "
                                   "(default: output)", NULL);
    /* sleep time */
    i = 250; j = 25000;         /* min/max value */
    p->CreateGlobalVariable(csound, "::JACK::sleepTime", sizeof(int));
    ptr = GETVAR("::JACK::sleepTime");
    *((int*) ptr) = 1000;
    p->CreateConfigurationVariable(csound, "jack_sleep_time", ptr,
                                   CSOUNDCFG_INTEGER, 0, &i, &j,
                                   "JACK sleep time in microseconds "
                                   "(default: 1000)", NULL);
    /* done */
    return 0;
}

static int playopen_(CSOUND*, csRtAudioParams*);
static int recopen_(CSOUND*, csRtAudioParams*);
static void rtplay_(CSOUND*, void*, int);
static int rtrecord_(CSOUND*, void*, int);
static void rtclose_(CSOUND*);

int csoundModuleInit(CSOUND *csound)
{
    char    *drv;

    drv = (char*) GETVAR("_RTAUDIO");
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "jack") == 0 || strcmp(drv, "Jack") == 0 ||
          strcmp(drv, "JACK") == 0))
      return 0;
    csound->Message(csound, "rtaudio: JACK module enabled\n");
    /* register Csound interface functions */
    csound->SetPlayopenCallback(csound, playopen_);
    csound->SetRecopenCallback(csound, recopen_);
    csound->SetRtplayCallback(csound, rtplay_);
    csound->SetRtrecordCallback(csound, rtrecord_);
    csound->SetRtcloseCallback(csound, rtclose_);

    return 0;
}

/* connect to JACK server, if not done so already */
/* returns client pointer on success, or NULL in case of an error */

static CsoundJackClient_t *jackConnectServer(CSOUND *csound)
{
    CSOUND              *p = csound;
    CsoundJackClient_t  *client;

    /* already connected ? */
    client = (CsoundJackClient_t*) GETVAR("::JACK::client");
    if (client != NULL)
      return client;
    /* no, connect to JACK server now */
    p->CreateGlobalVariable(csound, "::JACK::client",
                            sizeof(CsoundJackClient_t));
    client = (CsoundJackClient_t*) GETVAR("::JACK::client");
    if (client == NULL) {
      p->Message(csound, " *** JACK: jackConnectServer(): "
                         "memory allocation failure\n");
      return NULL;
    }
    client->csound = csound;
    client->active = 0;
    client->r.ringbuffers = NULL;
    client->r.jackPorts = NULL;
    client->p.ringbuffers = NULL;
    client->p.jackPorts = NULL;
    client->client = jack_client_new((char*) GETVAR("::JACK::clientName"));
    if (client->client == NULL) {
      p->Message(csound, " *** JACK: jackConnectServer(): "
                         "could not connect to JACK server\n");
      p->DestroyGlobalVariable(csound, "::JACK::client");
      return NULL;
    }
    /* successfully connected */
    return client;
}

/* open for audio input */

static int recopen_(CSOUND *csound, csRtAudioParams *parm)
{
    CsoundJackClient_t  *client;

    /* do we have a connection to the JACK server ? */
    client = jackConnectServer(csound);
    if (client == NULL)
      return -1;    /* no */
    /* open stream */
    if (openJackStream(client, parm, 0) != CSOUND_SUCCESS) {
      closeJackStream(client, 0);
      closeJackStream(client, 1);
      jack_client_close(client->client);
      csound->DestroyGlobalVariable(csound, "::JACK::client");
      return -1;
    }
    *(csound->GetRtRecordUserData(csound)) = &(client->r);
    return 0;
}

/* open for audio output */

static int playopen_(CSOUND *csound, csRtAudioParams *parm)
{
    CsoundJackClient_t  *client;

    /* do we have a connection to the JACK server ? */
    client = jackConnectServer(csound);
    if (client == NULL)
      return -1;    /* no */
    /* open stream */
    if (openJackStream(client, parm, 1) != CSOUND_SUCCESS) {
      closeJackStream(client, 0);
      closeJackStream(client, 1);
      jack_client_close(client->client);
      csound->DestroyGlobalVariable(csound, "::JACK::client");
      return -1;
    }
    *(csound->GetRtPlayUserData(csound)) = &(client->p);
    return 0;
}

static int processCallback(jack_nframes_t nframes, void *arg)
{
    CsoundJackClient_t  *client;
    CSOUND              *p;
    jack_default_audio_sample_t *bufp, *bufp2;
    int                 i, j, k = 0;

    client = (CsoundJackClient_t*) arg;
    p = (CSOUND*) (client->csound);
    if (!client->active)
      return -1;
    if (client->r.active == 2 &&                          /* capture active */
        client->r.frames_ahead                            /* and no xrun */
          <= (client->r.parm.bufSamp_HW - client->r.parm.bufSamp_SW)) {
      for (i = 0; i < client->r.parm.nChannels; i++) {
        bufp = (jack_default_audio_sample_t*)
                 jack_port_get_buffer(client->r.jackPorts[i],
                                      (jack_nframes_t)
                                        client->r.parm.bufSamp_SW);
        bufp2 = (jack_default_audio_sample_t*) (client->r.ringbuffers[i]);
        k = client->r.rb_writepos;
        for (j = 0; j < client->r.parm.bufSamp_SW; j++) {
          bufp2[k] = bufp[j];
          k = (k + 1) & (client->r.parm.bufSamp_HW - 1);
        }
      }
      client->r.rb_writepos = k;
      client->r.frames_ahead += client->r.parm.bufSamp_SW;
    }
    if (client->p.active == 2) {            /* playback active, and no xrun */
      if (client->p.frames_ahead >= client->p.parm.bufSamp_SW) {
        for (i = 0; i < client->p.parm.nChannels; i++) {
          bufp = (jack_default_audio_sample_t*) (client->p.ringbuffers[i]);
          bufp2 = (jack_default_audio_sample_t*)
                    jack_port_get_buffer(client->p.jackPorts[i],
                                         (jack_nframes_t)
                                           client->p.parm.bufSamp_SW);
          k = client->p.rb_readpos;
          for (j = 0; j < client->p.parm.bufSamp_SW; j++) {
            bufp2[j] = bufp[k];
            k = (k + 1) & (client->p.parm.bufSamp_HW - 1);
          }
        }
        client->p.rb_readpos = k;
        client->p.frames_ahead -= client->p.parm.bufSamp_SW;
      }
      else {                                /* xrun: fill with zero samples */
        for (i = 0; i < client->p.parm.nChannels; i++) {
          bufp2 = (jack_default_audio_sample_t*)
                    jack_port_get_buffer(client->p.jackPorts[i],
                                         (jack_nframes_t)
                                           client->p.parm.bufSamp_SW);
          for (j = 0; j < client->p.parm.bufSamp_SW; j++) {
            bufp2[j] = (jack_default_audio_sample_t) 0;
          }
        }
      }
    }
    return 0;
}

static int jackClientActivate(CSOUND *csound)
{
    CsoundJackClient_t  *client;
    CSOUND              *p = csound;
    int                 i;
    char                buf1[128], buf2[256];

    client = (CsoundJackClient_t*) GETVAR("::JACK::client");
    if (client == NULL)
      return -1;
    if (client->p.active == 0 && client->r.active == 0)
      goto err_return;
    if (client->active)
      return 0;
    /* register process callback function */
    if (jack_set_process_callback(client->client,
                                  (JackProcessCallback) processCallback,
                                  (void*) client) != 0) {
      p->Message(p, " *** jackClientActivate(): "
                    "error registering process callback function\n");
      goto err_return;
    }
    /* activate */
    if (jack_activate(client->client) != 0) {
      p->Message(p, " *** jackClientActivate(): "
                    "error activating JACK client\n");
      goto err_return;
    }
    /* connect ports if requested */
    if (client->r.parm.devName != NULL && client->r.parm.devName[0] != '\0') {
      if ((int) strlen(client->r.parm.devName) > 64) {
        p->Message(p, " *** jackClientActivate(): "
                      "input device name is too long\n");
        goto err_return;
      }
      for (i = 1; i <= client->r.parm.nChannels; i++) {
        sprintf(buf1, "%s%d", client->r.parm.devName, i);
        sprintf(buf2, "%s:%s%d", (char*) GETVAR("::JACK::clientName"),
                                 (char*) GETVAR("::JACK::inputPortName"), i);
        if (jack_connect(client->client, &(buf1[0]), &(buf2[0])) != 0) {
          p->Message(p, " *** jackClientActivate(): cannot connect %s to %s\n",
                        &(buf1[0]), &(buf2[0]));
          goto err_return;
        }
      }
    }
    if (client->p.parm.devName != NULL && client->p.parm.devName[0] != '\0') {
      if ((int) strlen(client->p.parm.devName) > 64) {
        p->Message(p, " *** jackClientActivate(): "
                      "output device name is too long\n");
        goto err_return;
      }
      for (i = 1; i <= client->p.parm.nChannels; i++) {
        sprintf(buf1, "%s:%s%d", (char*) GETVAR("::JACK::clientName"),
                                 (char*) GETVAR("::JACK::outputPortName"), i);
        sprintf(buf2, "%s%d", client->p.parm.devName, i);
        if (jack_connect(client->client, &(buf1[0]), &(buf2[0])) != 0) {
          p->Message(p, " *** jackClientActivate(): cannot connect %s to %s\n",
                        &(buf1[0]), &(buf2[0]));
          goto err_return;
        }
      }
    }
    /* now active */
    if (client->r.active)
      client->r.active = 2;
    if (client->p.active)
      client->p.active = 2;
    client->active = 1;

    return 0;

 err_return:
    closeJackStream(client, 0);
    closeJackStream(client, 1);
    jack_client_close(client->client);
    p->DestroyGlobalVariable(csound, "::JACK::client");
    *(p->GetRtRecordUserData(csound)) = NULL;
    *(p->GetRtPlayUserData(csound)) = NULL;
    return -1;
}

/* get samples from ADC */

static int rtrecord_(CSOUND *csound, void *inbuf_, int bytes_)
{
    JackStreamParams_t  *pp;
    volatile int        *frames_ahead;
    int                 nframes;

    pp = *((JackStreamParams_t**) (csound->GetRtRecordUserData(csound)));
    if (pp == NULL || pp->active == 0) {
      csound->Message(csound, " *** real time audio input stopped\n");
      memset(inbuf_, 0, (size_t) bytes_);
      usleep(100000);
      return bytes_;
    }
    if (pp->active == 1) {
      if (jackClientActivate(csound) != 0) {
        memset(inbuf_, 0, (size_t) bytes_);
        usleep(100000);
        return bytes_;
      }
    }
    /* wait until there is enough data in ring buffer */
    frames_ahead = &(pp->frames_ahead);
    while (*frames_ahead < pp->parm.bufSamp_SW)
      usleep(pp->sleep_time);
    /* copy from ring buffer */
    nframes = bytes_ / (pp->parm.nChannels * (int) (csound->GetSizeOfMYFLT()));
    pp->rb_readpos =
      pp->SampleConvFunc(nframes, pp->parm.nChannels,
                         pp->parm.bufSamp_HW, pp->rb_readpos,
                         (void*) pp->ringbuffers, (void*) inbuf_);
    (*frames_ahead) -= nframes;

    return bytes_;
}

/* put samples to DAC */

static void rtplay_(CSOUND *csound, void *outbuf_, int bytes_)
{
    JackStreamParams_t  *pp;
    volatile int        *frames_ahead;
    int                 nframes;

    pp = *((JackStreamParams_t**) (csound->GetRtPlayUserData(csound)));
    if (pp == NULL || pp->active == 0) {
      csound->Message(csound, " *** real time audio output stopped\n");
      usleep(100000);
      return;
    }
    if (pp->active == 1) {
      if (jackClientActivate(csound) != 0) {
        usleep(100000);
        return;
      }
    }
    /* wait until there is enough free space in ring buffer */
    frames_ahead = &(pp->frames_ahead);
    while (*frames_ahead > (pp->parm.bufSamp_HW - pp->parm.bufSamp_SW))
      usleep(pp->sleep_time);
    /* copy to ring buffer */
    nframes = bytes_ / (pp->parm.nChannels * (int) (csound->GetSizeOfMYFLT()));
    pp->rb_writepos =
      pp->SampleConvFunc(nframes, pp->parm.nChannels,
                         pp->parm.bufSamp_HW, pp->rb_writepos,
                         (void*) outbuf_, (void*) pp->ringbuffers);
    (*frames_ahead) += nframes;
}

/* close the I/O device entirely  */
/* called only when both complete */

static void rtclose_(CSOUND *csound)
{
    CsoundJackClient_t  *client;

    client = (CsoundJackClient_t*) GETVAR("::JACK::client");
    if (client == NULL)
      return;       /* already closed */
    closeJackStream(client, 0);
    closeJackStream(client, 1);
    jack_client_close(client->client);
    csound->DestroyGlobalVariable(csound, "::JACK::client");
    *(csound->GetRtRecordUserData(csound)) = NULL;
    *(csound->GetRtPlayUserData(csound)) = NULL;
}

/* sample conversion routines */

static int float_to_float_r(int nSmps, int nChannels, int bufSize, int ofs,
                            float **inBufs, float *outBuf)
{
    int i, j, c;

    i = c = 0;
    j = ofs;
    while (nSmps) {
      outBuf[i++] = inBufs[c++][j];
      if (c >= nChannels) {
        c = 0; j = (j + 1) & (bufSize - 1); nSmps--;
      }
    }
    return j;
}

static int float_to_double_r(int nSmps, int nChannels, int bufSize, int ofs,
                             float **inBufs, double *outBuf)
{
    int i, j, c;

    i = c = 0;
    j = ofs;
    while (nSmps) {
      outBuf[i++] = (double) inBufs[c++][j];
      if (c >= nChannels) {
        c = 0; j = (j + 1) & (bufSize - 1); nSmps--;
      }
    }
    return j;
}

static int double_to_float_r(int nSmps, int nChannels, int bufSize, int ofs,
                             double **inBufs, float *outBuf)
{
    int i, j, c;

    i = c = 0;
    j = ofs;
    while (nSmps) {
      outBuf[i++] = (float) inBufs[c++][j];
      if (c >= nChannels) {
        c = 0; j = (j + 1) & (bufSize - 1); nSmps--;
      }
    }
    return j;
}

static int double_to_double_r(int nSmps, int nChannels, int bufSize, int ofs,
                              double **inBufs, double *outBuf)
{
    int i, j, c;

    i = c = 0;
    j = ofs;
    while (nSmps) {
      outBuf[i++] = inBufs[c++][j];
      if (c >= nChannels) {
        c = 0; j = (j + 1) & (bufSize - 1); nSmps--;
      }
    }
    return j;
}

static int float_to_float_p(int nSmps, int nChannels, int bufSize, int ofs,
                            float *inBuf, float **outBufs)
{
    int i, j, c;

    i = c = 0;
    j = ofs;
    while (nSmps) {
      outBufs[c++][j] = inBuf[i++];
      if (c >= nChannels) {
        c = 0; j = (j + 1) & (bufSize - 1); nSmps--;
      }
    }
    return j;
}

static int float_to_double_p(int nSmps, int nChannels, int bufSize, int ofs,
                             float *inBuf, double **outBufs)
{
    int i, j, c;

    i = c = 0;
    j = ofs;
    while (nSmps) {
      outBufs[c++][j] = (double) inBuf[i++];
      if (c >= nChannels) {
        c = 0; j = (j + 1) & (bufSize - 1); nSmps--;
      }
    }
    return j;
}

static int double_to_float_p(int nSmps, int nChannels, int bufSize, int ofs,
                             double *inBuf, float **outBufs)
{
    int i, j, c;

    i = c = 0;
    j = ofs;
    while (nSmps) {
      outBufs[c++][j] = (float) inBuf[i++];
      if (c >= nChannels) {
        c = 0; j = (j + 1) & (bufSize - 1); nSmps--;
      }
    }
    return j;
}

static int double_to_double_p(int nSmps, int nChannels, int bufSize, int ofs,
                              double *inBuf, double **outBufs)
{
    int i, j, c;

    i = c = 0;
    j = ofs;
    while (nSmps) {
      outBufs[c++][j] = inBuf[i++];
      if (c >= nChannels) {
        c = 0; j = (j + 1) & (bufSize - 1); nSmps--;
      }
    }
    return j;
}

