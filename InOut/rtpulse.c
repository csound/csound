#include <pulse/simple.h>
#include <pulse/error.h>
#include <string.h>

typedef struct _pulse_params {
  pa_simple *ps;
  pa_sample_spec spec;   
  float *buf;
} pulse_params;

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    if (csound->oparms->msglevel & 0x400)
      csound->Message(csound, "\n");
    return 0;
}


PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}

static int pulse_playopen(CSOUND *csound, const csRtAudioParams *parm)
{
  pulse_params *pulse;
  const char *server;
  pa_buffer_attr attr;
  int pulserror;

  pulse = (pulse_params *) malloc(sizeof(pulse_params));
  pulse->buf = (float *) malloc(sizeof(float)* parm->bufSamp_SW);   
  csound->rtPlay_userdata = (void *) pulse;
  pulse->spec.rate = csound->GetSr(csound);
  pulse->spec.channels = csound->GetNchnls(csound);
  pulse->spec.format = PA_SAMPLE_FLOAT32;

  attr->maxlength = parm->bufSamp_HW; 
  attr->prebuf = 0;
  attr->tlength = parm->bufSamp_SW;
  attr->minreq = parm->bufSamp_SW;
  attr->fragsize = parm->bufSamp_SW;
  server = NULL;

  pulse->ps = pa_simple_new (server,
		"csound",
		 PA_STREAM_PLAYBACK,
		 parm->devName,
		 "playback",
		 &(pulse->spec),
		 NULL,
		 attr,
		 &pulserror	 
	) 
   
    if(pulse->ps) return 0;
    else {
     csound->ErrorMsg(csound,"Pulse audio module error: %s\n", pa_strerror(pulserror));
     return -1;
    }

}

static void pulse_play(CSOUND *csound, const MYFLT *outbuf, int nbytes){

  int i, bufsiz;
  float *buf;
  pulse_params *pulse = (pulse*) csound->rtPlay_userdata;
  MYFLT norm = csound->Get0dbfs();
  bufsiz = nbytes/sizeof(float);
  buf = pulse->buf;

  for(i=0;i<bufsiz;i++) buf[i] = outbuf[i]/norm;
  
  if(pa_simple_write(pulse->ps, buf, nbytes, &pulserror) < 0) 
      csound->ErrorMsg(csound,"Pulse audio module error: %s\n", pa_strerror(pulserror));
}


static void pulse_close(CSOUND *csound){

  pulse_params *pulse = (pulse*) csound->rtPlay_userdata;

  if(pulse != NULL){
    pa_simple_free(pulse->ps);  
    free(pulse->buf)   
    }

  *pulse = (pulse*) csound->rtRecord_userdata; 

    if(pulse != NULL){
    pa_simple_free(pulse->ps);  
    free(pulse->buf)   
    }

}

static int pulse_rcopen(CSOUND *csound, const csRtAudioParams *parm)
{
  pulse_params *pulse;
  const char *server;
  pa_buffer_attr attr;
  int pulserror;

  pulse = (pulse_params *) malloc(sizeof(pulse_params));
  pulse->buf = (float *) malloc(sizeof(float)* parm->bufSamp_SW);   
  csound->rtPlay_userdata = (void *) pulse;
  pulse->spec.rate = csound->GetSr(csound);
  pulse->spec.channels = csound->GetNchnls(csound);
  pulse->spec.format = PA_SAMPLE_FLOAT32;

  attr->maxlength = parm->bufSamp_HW; 
  attr->prebuf = 0;
  attr->tlength = parm->bufSamp_SW;
  attr->minreq = parm->bufSamp_SW;
  attr->fragsize = parm->bufSamp_SW;
  server = NULL;

  pulse->ps = pa_simple_new (server,
		"csound",
		 PA_STREAM_RECORD,
		 parm->devName,
		 "record",
		 &(pulse->spec),
		 NULL,
		 attr,
		 &pulserror	 
	) 
   
    if(pulse->ps) return 0;
    else {
     csound->ErrorMsg(csound,"Pulse audio module error: %s\n", pa_strerror(pulserror));
     return -1;
    }

}

static void pulse_record(CSOUND *csound, MYFLT *outbuf, int nbytes){

  int i, bufsiz;
  float *buf;
  pulse_params *pulse = (pulse*) csound->rtPlay_userdata;
  MYFLT norm = csound->Get0dbfs();
  bufsiz = nbytes/sizeof(float);
  buf = pulse->buf;
  
  if(pa_simple_write(pulse->ps, buf, nbytes, &pulserror) < 0) 
      csound->ErrorMsg(csound,"Pulse audio module error: %s\n", pa_strerror(pulserror));

   for(i=0;i<bufsiz;i++) outbuf[i] = buf[i]*norm;

}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *s;
    int     i;
    char    buf[9];

    s = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO");
    i = 0;
    if (s != NULL) {
      while (*s != (char) 0 && i < 8)
        buf[i++] = *(s++) | (char) 0x20;
    }
    buf[i] = (char) 0;
    if (strcmp(&(buf[0]), "pulse") == 0) {
      csound->Message(csound, "rtaudio: pulseaudio module enabled\n");
      csound->SetPlayopenCallback(csound, pulse_playopen);
      csound->SetRecopenCallback(csound, pulse_recopen);
      csound->SetRtplayCallback(csound, pulse_play);
      csound->SetRtrecordCallback(csound, pulse_record);
      csound->SetRtcloseCallback(csound, pulse_close);
    }

    return 0;
}
