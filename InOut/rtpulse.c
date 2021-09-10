/*
  rtpulse.c:

  Copyright (C) 2008 Victor Lazzarini

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

#include <csdl.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <string.h>

typedef struct _pulse_params {
  pa_simple *ps;
  pa_sample_spec spec;
  float *buf;
} pulse_params;

typedef struct _pulse_globals {
  char server[64];
  char oname[32];
  char iname[32];
} pulse_globals;

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    pulse_globals *p;
    int siz = 64;
    OPARMS oparms;
    csound->GetOParms(csound, &oparms);

    if (oparms.msglevel & 0x400)
      csound->Message(csound, Str("PulseAudio client RT IO module for Csound"
                                  "by Victor Lazzarini\n"));

    if (UNLIKELY(csound->CreateGlobalVariable(csound, "_pulse_globals",
                                              sizeof(pulse_globals)) != 0)) {
      csound->ErrorMsg(csound, Str(" *** rtpulse: error allocating globals"));
      return -1;
    }
    p = (pulse_globals*) csound->QueryGlobalVariableNoCheck(csound,
                                                            "_pulse_globals");
    strcpy(&(p->server[0]), "default");

    csound->CreateConfigurationVariable(
        csound,"server", (void*) &(p->server[0]),
        CSOUNDCFG_STRING, 0, NULL, &siz,
        "PulseAudio server name (default: default server)", NULL);

    strcpy(&(p->oname[0]), "csound-out");

    siz = 32;

    csound->CreateConfigurationVariable(
        csound,"output_stream", (void*) &(p->oname[0]),
        CSOUNDCFG_STRING, 0, NULL, &siz,
        "PulseAudio output stream name (default: csound-out)", NULL);

    strcpy(&(p->iname[0]), "csound-in");

    csound->CreateConfigurationVariable(
        csound,"input_stream", (void*) &(p->iname[0]),
        CSOUNDCFG_STRING, 0, NULL, &siz,
        "PulseAudio input stream name (default: csound-in)", NULL);


    return 0;
}


PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}

static int pulse_playopen(CSOUND *csound, const csRtAudioParams *parm)
{
    pulse_params *pulse;
    pulse_globals *pg;
    const char *server;
    /* pa_buffer_attr attr */
    int pulserror;

    pulse = (pulse_params *) csound->Malloc(csound, sizeof(pulse_params));
    *(csound->GetRtPlayUserData(csound))  = (void *) pulse;
    pulse->spec.rate = csound->GetSr(csound);
    pulse->spec.channels = csound->GetNchnls(csound);
    pulse->spec.format = PA_SAMPLE_FLOAT32;
    pulse->buf =
      (float *) csound->Malloc(csound,
                               sizeof(float)*parm->bufSamp_SW*pulse->spec.channels);

    if(!pa_sample_spec_valid(&(pulse->spec))) {
        csound->ErrorMsg(csound,Str("Pulse audio module error: invalid sample spec, "
                                    "check number of output channels (%d)\n"),
                         csound->GetNchnls(csound));
        return -1;
    }
    /*
      attr.maxlength = parm->bufSamp_HW;
      attr.prebuf = parm->bufSamp_SW;
      attr.tlength = parm->bufSamp_SW;
      attr.minreq = parm->bufSamp_SW;
      attr.fragsize = parm->bufSamp_SW;
    */

    pg = (pulse_globals*) csound->QueryGlobalVariableNoCheck(csound,
                                                             "_pulse_globals");

    if (!strcmp(pg->server,"default")){
      server = NULL;
      csound->Message(csound, Str("PulseAudio output server: default\n"));
    }
    else {
      server = pg->server;
      csound->Message(csound, Str("PulseAudio output server %s\n"), server);
    }

    pulse->ps = pa_simple_new (server,
                               "csound",             // client name
                               PA_STREAM_PLAYBACK,   // stream direction
                               parm->devName,        // device name
                               &(pg->oname[0]),      // stream name
                               &(pulse->spec),       // sample spec
                               NULL,                 // channel map (NULL=default)
                               /*&attrib*/ NULL,     // buffer attribute
                               &pulserror
                               ) ;

    if (LIKELY(pulse->ps)){
      csound->Message(csound, Str("pulseaudio output open\n"));
      return 0;
    }
    else {
      csound->ErrorMsg(csound,Str("Pulse audio module error: %s\n"),
                       pa_strerror(pulserror));
      return -1;
    }

}

static void pulse_play(CSOUND *csound, const MYFLT *outbuf, int nbytes){

  int i, bufsiz, pulserror;
  float *buf;
  pulse_params *pulse = (pulse_params*) *(csound->GetRtPlayUserData(csound));
  //MYFLT norm = csound->e0dbfs;
  bufsiz = nbytes/sizeof(MYFLT);
  buf = pulse->buf;
  for (i=0;i<bufsiz;i++) buf[i] = outbuf[i];
  if (UNLIKELY(pa_simple_write(pulse->ps, buf,
                               bufsiz*sizeof(float), &pulserror) < 0))
    csound->ErrorMsg(csound,Str("Pulse audio module error: %s\n"),
                     pa_strerror(pulserror));

}


static void pulse_close(CSOUND *csound)
{
    int error;
    pulse_params *pulse = (pulse_params*) *(csound->GetRtPlayUserData(csound));

    if (pulse != NULL){
      pa_simple_drain(pulse->ps, &error);
      pa_simple_free(pulse->ps);
      csound->Free(csound,pulse->buf);
    }

    pulse = (pulse_params*) *(csound->GetRtRecordUserData(csound)) ;

    if (pulse != NULL){
      pa_simple_free(pulse->ps);
      csound->Free(csound,pulse->buf);
    }
    csound->DestroyGlobalVariable(csound, "pulse_globals");
}

static int pulse_recopen(CSOUND *csound, const csRtAudioParams *parm)
{
    pulse_params *pulse;
    pulse_globals *pg;
    const char *server;
    /*pa_buffer_attr attr;*/
    int pulserror;
    pulse = (pulse_params *) csound->Malloc(csound, sizeof(pulse_params));
    *(csound->GetRtRecordUserData(csound))  = (void *) pulse;
    pulse->spec.rate = csound->GetSr(csound);
    pulse->spec.channels = csound->GetNchnls_i(csound);
    pulse->spec.format = PA_SAMPLE_FLOAT32;
    pulse->buf =
      (float *) csound->Malloc(csound,
                               sizeof(float)*parm->bufSamp_SW*pulse->spec.channels);
    /*
      attr.maxlength = parm->bufSamp_HW;
      attr.prebuf = 0;
      attr.tlength = parm->bufSamp_SW;
      attr.minreq = parm->bufSamp_SW;
      attr.fragsize = parm->bufSamp_SW;
    */

    pg = (pulse_globals*) csound->QueryGlobalVariableNoCheck(csound,
                                                             "_pulse_globals");

    if (!strcmp(pg->server,"default")){
      server = NULL;
      csound->Message(csound, Str("PulseAudio input server: default\n"));
    }
    else {
      server = pg->server;
      csound->Message(csound, Str("PulseAudio input server %s\n"), server);
    }

    pulse->ps = pa_simple_new (server,
                               "csound",          // client name
                               PA_STREAM_RECORD,  // stream direction
                               parm->devName,     // device name
                               &(pg->iname[0]),   // stream name
                               &(pulse->spec),    // sample spec
                               NULL,              // channel map (NULL=default)
                               /*&attr*/ NULL,    // buffer attribute
                               &pulserror );

    if (LIKELY(pulse->ps)) return 0;
    else {
      csound->ErrorMsg(csound,Str("Pulse audio module error: %s\n"),
                       pa_strerror(pulserror));
      return -1;
    }

}

static int pulse_record(CSOUND *csound, MYFLT *inbuf, int nbytes)
{
    int i, bufsiz,pulserror;
    float *buf;
    pulse_params *pulse = (pulse_params*) *(csound->GetRtRecordUserData(csound)) ;
    //MYFLT norm = csound->e0dbfs;
    bufsiz = nbytes/sizeof(MYFLT);
    buf = pulse->buf;

    if (UNLIKELY(pa_simple_read(pulse->ps, buf,
                                bufsiz*sizeof(float), &pulserror) < 0)) {
      csound->ErrorMsg(csound,Str("Pulse audio module error: %s\n"),
                       pa_strerror(pulserror));
      return -1;
    }
    else {
      for (i=0;i<bufsiz;i++) inbuf[i] = buf[i];
      return nbytes;
    }

}


PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *s;
    int     i;
    char    buf[9];
    csound->module_list_add(csound, "pulse", "audio");
    s = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO");
    i = 0;
    if (s != NULL) {
      while (*s != (char) 0 && i < 8)
        buf[i++] = *(s++) | (char) 0x20;
    }
    buf[i] = (char) 0;
    if (strcmp(&(buf[0]), "pulse") == 0) {
      csound->Message(csound, Str("rtaudio: pulseaudio module enabled\n"));
      csound->SetPlayopenCallback(csound, pulse_playopen);
      csound->SetRecopenCallback(csound, pulse_recopen);
      csound->SetRtplayCallback(csound, pulse_play);
      csound->SetRtrecordCallback(csound, pulse_record);
      csound->SetRtcloseCallback(csound, pulse_close);
    }


    return 0;
}
