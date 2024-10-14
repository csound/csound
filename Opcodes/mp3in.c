/*
  mp3in.c:

  Copyright (C) 2009 by John ffitch, V Lazzarini

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

/* mp3in.c */
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include "mp3dec.h"


typedef struct {
  OPDS    h;
  MYFLT   *ar[2];
  MYFLT   *iFileCode;
  MYFLT   *iSkipTime;
  MYFLT   *iSampleFormat;
  MYFLT   *iSkipInit;
  MYFLT   *ibufsize;
  /* ------------------------------------- */
  mp3dec_t mpa;           /* For library */
  int32_t  r;             /* Result field */
  int32_t  initDone;
  int32_t  bufSize;       /* in sample frames, power of two */
  uint32_t bufused;
  int64_t  pos;           /* type should be defined in sysdep.h */
  uint8_t  *buf;
  AUXCH    auxch;
  FDCH     fdch;
} MP3IN;


typedef struct {
  OPDS    h;
  MYFLT   *ir;
  MYFLT   *iFileCode;

} MP3LEN;

int32_t mp3in_cleanup(CSOUND *csound, MP3IN *p)
{
  IGN(csound);
  if (LIKELY(p->mpa != NULL))
    mp3dec_uninit(p->mpa);
  p->mpa = NULL;
  return OK;
}


int32_t mp3ininit_(CSOUND *csound, MP3IN *p, int32_t stringname)
{
  char    name[1024];
  FILE *f;
  mp3dec_t mpa           = NULL;
  mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_STEREO,
                             MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                             MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                             0.0 };
  mpadec_info_t mpainfo;
  int32_t buffersize =
    (*p->ibufsize<=0.0 ? /*0x1000*/ 8*1152 : (int32_t)*p->ibufsize);
  /* uint64_t maxsize; */
  int32_t r;
  int32_t skip;
  if (p->OUTOCOUNT==1) config.mode = MPADEC_CONFIG_MONO;
  /* if already open, close old file first */
  if (p->fdch.fd != NULL) {
    /* skip initialisation if requested */
    if (*(p->iSkipInit) != FL(0.0))
      return OK;
    csound->FDClose(csound, &(p->fdch));
  }
  /* set default format parameters */
  /* open file */
  p->mpa = mpa = mp3dec_init(csound);
  if (UNLIKELY(!mpa)) {
    return csound->InitError(csound, "%s", Str("Not enough memory\n"));
  }

  if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    p->mpa = NULL;
    return csound->InitError(csound, "%s", mp3dec_error(r));
  }


  /* FIXME: name can overflow with very long string -- truncates safely */
  if (stringname==0){
    if (IsStringCode(*p->iFileCode))
      strncpy(name,csound->GetString(csound, *p->iFileCode), 1023);
    else csound->StringArg2Name(csound, name, p->iFileCode, "soundin.",0);
  }
  else strncpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);


  if (UNLIKELY(mp3dec_open_file(mpa, name, &f) == NULL)) {
    mp3dec_uninit(mpa);
    return
      csound->InitError(csound, Str("mp3in: %s: failed to open file"), name);
  }
  /* HOW TO record file handle so that it will be closed at note-off */
  /* memset(&(p->fdch), 0, sizeof(FDCH)); */
  /* p->fdch.fd = fd; */
  /* fdrecord(csound, &(p->fdch)); */
  if (UNLIKELY((r = mp3dec_init_file(mpa, f, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, "%s", mp3dec_error(r));
  }
  if (UNLIKELY((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
               MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, "%s", mp3dec_error(r));
  }
  skip = (int32_t)(*p->iSkipTime*CS_ESR);

  /* maxsize = mpainfo.decoded_sample_size */
  /*          *mpainfo.decoded_frame_samples */
  /*          *mpainfo.frames; */
  /* csound->Message(csound, "maxsize = %li\n", maxsize); */
  /* print file information */
  /* if (UNLIKELY(csound->GetOParms_.msglevel & WARNMSG)) */ {
    char temp[80];            /* Could be as low as 20 */
    if (mpainfo.frequency < 16000) strcpy(temp, "MPEG-2.5 ");
    else if (mpainfo.frequency < 32000) strcpy(temp, "MPEG-2 ");
    else strcpy(temp, "MPEG-1 ");
    if (mpainfo.layer == 1) strcat(temp, "Layer I");
    else if (mpainfo.layer == 2) strcat(temp, "Layer II");
    else strcat(temp, "Layer III");
    if(csound->GetDebug(csound))
      csound->Warning(csound, "Input:  %s, %s, %d kbps, %d Hz  (%d:%02d)\n",
                    temp, ((mpainfo.channels > 1) ? "stereo" : "mono"),
                    mpainfo.bitrate, mpainfo.frequency, mpainfo.duration/60,
                    mpainfo.duration%60);
  }
  /* check number of channels in file (must equal the number of outargs) */
  /* if (UNLIKELY(sfinfo.channels != p->nChannels && */
  /*              (csound->GetOParms_.msglevel & WARNMSG) != 0)) { */
  /*   mp3dec_uninit(mpa); */
  /*   return csound->InitError(csound, */
  /*                      "%s", Str("mp3in: number of output args " */
  /*                          "inconsistent with number of file channels")); */
  /* } */
  /* skip initialisation if requested */
  if (*(p->iSkipInit) != FL(0.0))
    return OK;
  /* set file parameters from header info */
  if ((int32_t) (CS_ESR + FL(0.5)) != mpainfo.frequency) {
    csound->Warning(csound, Str("mp3in: file sample rate (%d) "
                                "!= orchestra sr (%d)\n"),
                    mpainfo.frequency, (int32_t) (CS_ESR + FL(0.5)));
  }
  /* initialise buffer */
  mp3dec_seek(mpa,0, MP3DEC_SEEK_SAMPLES);
  p->bufSize = buffersize;
  if (p->auxch.auxp == NULL || p->auxch.size < (uint32_t)buffersize)
    csound->AuxAlloc(csound, buffersize, &p->auxch);
  p->buf = (uint8_t *) p->auxch.auxp;
  p->bufused = -1;
  buffersize /= (mpainfo.decoded_sample_size);
  //printf("===%d\n", skip);
  //skip = skip - 528;
  while (skip > 0) {
    int32_t xx= skip;
    // printf("%d\n", skip);
    if (xx > buffersize) xx = buffersize;
    skip -= xx;
    r = mp3dec_decode(mpa, p->buf, mpainfo.decoded_sample_size*xx, &p->bufused);
    // printf("u %d\n", p->bufused);
  }
  //if(!skip)
  //mp3dec_seek(mpa, skip, MP3DEC_SEEK_SAMPLES);
  p->r = r;
  /* done initialisation */
  p->initDone = -1;
  p->pos = 0;

  return OK;
}

int32_t mp3ininit(CSOUND *csound, MP3IN *p){
  return mp3ininit_(csound,p,0);
}

int32_t mp3ininit_S(CSOUND *csound, MP3IN *p){
  return mp3ininit_(csound,p,1);
}


int32_t mp3in(CSOUND *csound, MP3IN *p)
{
  int32_t r       = p->r;
  mp3dec_t mpa    = p->mpa;
  uint8_t *buffer = p->buf;
  MYFLT *al       = p->ar[0];
  MYFLT *ar       = p->ar[1];
  int64_t pos     = p->pos;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t i, n, nsmps = CS_KSMPS;

  if (UNLIKELY(offset)) {
    memset(al, '\0', offset*sizeof(MYFLT));
    if(p->OUTCOUNT > 1)  memset(ar, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&al[nsmps], '\0', early*sizeof(MYFLT));
   if(p->OUTCOUNT > 1)  memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    for (i=0; i<p->OUTOCOUNT; i++) {     /* stereo */
      MYFLT xx;
      short *bb = (short*)buffer;
      while (r != MP3DEC_RETCODE_OK || 2*pos >=  (int32_t)p->bufused) {
        r = mp3dec_decode(mpa, buffer, p->bufSize, &p->bufused);
        if (UNLIKELY(p->bufused == 0)) {
          memset(&al[n], 0, (nsmps-n)*sizeof(MYFLT));
          if(p->OUTCOUNT > 1) memset(&ar[n], 0, (nsmps-n)*sizeof(MYFLT));
          goto ending;
        }
        pos = 0;
      }
      xx = ((MYFLT)bb[pos]/(MYFLT)0x7fff) * csound->Get0dBFS(csound);
      if (i==0) al[n] = xx;
      else      ar[n] = xx;
      pos++;
    }
  }
 ending:

  p->pos = pos;
  p->r = r;
  if (UNLIKELY(r != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    p->mpa = NULL;
    return NOTOK;
  }
  return OK;
}

int32_t mp3len_(CSOUND *csound, MP3LEN *p, int32_t stringname)
{
  char     name[1024];
  FILE     *f;
  mp3dec_t mpa           = NULL;
  mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_STEREO,
                             MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                             MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                             0.0 };
  mpadec_info_t mpainfo;
  int32_t  r;

  /* open file */
  mpa = mp3dec_init(csound);
  if (UNLIKELY(!mpa)) {
    return csound->InitError(csound, "%s", Str("Not enough memory\n"));
  }
  if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, "%s", mp3dec_error(r));
  }
  /* FIXME: name can overflow with very long string -- safely truncated */
  if (stringname==0){
    if (IsStringCode(*p->iFileCode))
      strncpy(name,csound->GetString(csound, *p->iFileCode), 1023);
    else csound->StringArg2Name(csound, name, p->iFileCode, "soundin.",0);
  }
  else strncpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);

  if (UNLIKELY(mp3dec_open_file(mpa, name, &f) == NULL)) {
    mp3dec_uninit(mpa);
    return
      csound->InitError(csound,  Str("mp3in: %s: failed to open file"), name);
  }
  if (UNLIKELY((r = mp3dec_init_file(mpa, f, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, "%s", mp3dec_error(r));
  }
  if (UNLIKELY((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
               MP3DEC_RETCODE_OK)) {
    fclose(f);
    mp3dec_uninit(mpa);
    return csound->InitError(csound, "%s", mp3dec_error(r));
  }
  fclose(f);
  if(!strcmp(GetOpcodeName(&p->h), "mp3len"))
    *p->ir = (MYFLT) mpainfo.duration;
  else if(!strcmp(GetOpcodeName(&p->h), "mp3sr"))
    *p->ir = (MYFLT) mpainfo.frequency;
  else if(!strcmp(GetOpcodeName(&p->h), "mp3bitrate"))
    *p->ir = (MYFLT) mpainfo.bitrate;
  else if(!strcmp(GetOpcodeName(&p->h), "mp3nchnls"))
    *p->ir = (MYFLT) mpainfo.channels;
  mp3dec_uninit(mpa);
  return OK;
}

int32_t mp3len(CSOUND *csound, MP3LEN *p){
  return mp3len_(csound,p,0);
}

int32_t mp3len_S(CSOUND *csound, MP3LEN *p){
  return mp3len_(csound,p,1);
}

#define MP3_CHNS 2
typedef struct dats{
  OPDS h;
  MYFLT *out1,*out2,*kstamp, *knum, *time,*kpitch, *kamp, *skip, *iN,
    *idecim, *klock,*kinterp;
  int32_t cnt, hsize, curframe, N, decim,tscale;
  double pos;
  MYFLT accum;
  AUXCH outframe[MP3_CHNS], win, bwin[MP3_CHNS], fwin[MP3_CHNS],
    nwin[MP3_CHNS], prev[MP3_CHNS], framecount[MP3_CHNS], fdata[MP3_CHNS], buffer;
  MYFLT *indataL[2], *indataR[2];
  MYFLT *tab[MP3_CHNS];
  char curbuf;
  mp3dec_t mpa;
  FDCH    fdch;
  MYFLT resamp;
  double tstamp, incr;
  int32_t initDone;
  uint32_t bufused;
  int32_t finished;
  char init;
  CSOUND *csound;
  // PTHREAD: change
  //pthread_t t;
  void* t;
  int32_t ti;
  char filling;
  void *fwdsetup, *invsetup;
} DATASPACE;

int32_t mp3scale_cleanup(CSOUND *csound, DATASPACE *p)
{
  IGN(csound);
  if (p->mpa != NULL)
    mp3dec_uninit(p->mpa);
  return OK;
}

#define BUFS 32
static void fillbuf(CSOUND *csound, DATASPACE *p, int32_t nsmps);
/* file-reading version of temposcal */
static int32_t sinit(CSOUND *csound, DATASPACE *p)
{
  int32_t  N =  *p->iN, ui;
  uint32_t i;
  uint32_t size;
  int32_t  decim = *p->idecim;
  /*double dtime;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    dtime = ts.tv_sec + 1e-9*ts.tv_nsec;*/
  if (N) {
    for (i=0; N; i++) {
      N >>= 1;
    }
    N = (int32_t) pow(2.0, i-1);  /* could be a shift?  1 << (i-1) */
  } else N = 2048;
  if (decim == 0) decim = 4;

  p->hsize = N/decim;
  p->cnt = p->hsize;
  p->curframe = 0;
  p->pos = 0;

  for (i=0; i < MP3_CHNS; i++) {

    size = (N+2)*sizeof(MYFLT);
    if (p->fwin[i].auxp == NULL || p->fwin[i].size < size)
      csound->AuxAlloc(csound, size, &p->fwin[i]);
    if (p->bwin[i].auxp == NULL || p->bwin[i].size < size)
      csound->AuxAlloc(csound, size, &p->bwin[i]);
    if (p->prev[i].auxp == NULL || p->prev[i].size < size)
      csound->AuxAlloc(csound, size, &p->prev[i]);
    size = decim*sizeof(int32_t);
    if (p->framecount[i].auxp == NULL || p->framecount[i].size < size)
      csound->AuxAlloc(csound, size, &p->framecount[i]);
    {
      int32_t k=0;
      for (k=0; k < decim; k++) {
        ((int32_t *)(p->framecount[i].auxp))[k] = k*N;
      }
    }
    size = decim*sizeof(MYFLT)*N;
    if (p->outframe[i].auxp == NULL || p->outframe[i].size < size)
      csound->AuxAlloc(csound, size, &p->outframe[i]);
    else
      memset(p->outframe[i].auxp,0,size);
  }
  size = N*sizeof(MYFLT);
  if (p->win.auxp == NULL || p->win.size < size)
    csound->AuxAlloc(csound, size, &p->win);

  {
    MYFLT x = FL(2.0)*PI_F/N;
    for (ui=0; ui < N; ui++)
      ((MYFLT *)p->win.auxp)[ui] = FL(0.5) - FL(0.5)*COS((MYFLT)ui*x);
  }

  p->N = N;
  p->decim = decim;

  /*clock_gettime(CLOCK_MONOTONIC, &ts);
    dtime = ts.tv_sec + 1e-9*ts.tv_nsec - dtime;
    csound->Message(csound, "SINIT time %f ms", dtime*1000);*/
  p->fwdsetup = csound->RealFFTSetup(csound,N,FFT_FWD);
  p->invsetup = csound->RealFFTSetup(csound,N,FFT_INV);
  return OK;
}

static int32_t sinit3_(CSOUND *csound, DATASPACE *p)
{
  uint32_t size;
  char *name;
  // open file
  FILE  *f;
  int32_t  r;
  mp3dec_t mpa           = NULL;
  mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_STEREO,
                             MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                             MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                             0.0 };
  mpadec_info_t mpainfo;
  /*double dtime;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    dtime = ts.tv_sec + 1e-9*ts.tv_nsec;*/
  name = ((STRINGDAT *)p->knum)->data;
  p->mpa = mpa = mp3dec_init(csound);
  if (UNLIKELY(!mpa)) {
    return csound->InitError(csound, "%s", Str("Not enough memory\n"));
  }
  if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    p->mpa = NULL;
    return csound->InitError(csound, "%s", mp3dec_error(r));
  }

  if (UNLIKELY(mp3dec_open_file(mpa, name, &f) == NULL)) {
    mp3dec_uninit(mpa);
    return
      csound->InitError(csound,  Str("mp3scale: %s: failed to open file"), name);
  }// else
  // csound->Message(csound, "%s", Str("mp3scale: open %s\n"), name);
  if (UNLIKELY((r = mp3dec_init_file(mpa, f, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, "%s", mp3dec_error(r));
  } // else
    // csound->Message(csound, "%s", Str("mp3scale: init %s\n"), name);

  if (UNLIKELY((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
               MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, "%s", mp3dec_error(r));
  }

  /* {
     char temp[80];
     if (mpainfo.frequency < 16000) strcpy(temp, "MPEG-2.5 ");
     else if (mpainfo.frequency < 32000) strcpy(temp, "MPEG-2 ");
     else strcpy(temp, "MPEG-1 ");
     if (mpainfo.layer == 1) strcat(temp, "Layer I");
     else if (mpainfo.layer == 2) strcat(temp, "Layer II");
     else strcat(temp, "Layer III");
     csound->Warning(csound, "Input:  %s, %s, %d kbps, %d Hz  (%d:%02d)\n",
     temp, ((mpainfo.channels > 1) ? "stereo" : "mono"),
     mpainfo.bitrate, mpainfo.frequency, mpainfo.duration/60,
     mpainfo.duration%60);
     }*/

  if(mpainfo.frequency != CS_ESR)
    p->resamp = mpainfo.frequency/CS_ESR;
  else
    p->resamp = 1;

  /*clock_gettime(CLOCK_MONOTONIC, &ts);
    dtime = ts.tv_sec + 1e-9*ts.tv_nsec - dtime;
    csound->Message(csound, "MP3 INIT time %f ms", dtime*1000);
    clock_gettime(CLOCK_MONOTONIC, &ts);
    dtime = ts.tv_sec + 1e-9*ts.tv_nsec;*/

  {
    char *ps;
    sinit(csound, p);
    size = p->N*sizeof(MYFLT)*BUFS;
    if (p->fdata[0].auxp == NULL || p->fdata[0].size < size)
      csound->AuxAlloc(csound, size, &p->fdata[0]);
    ps = (char *) p->fdata[0].auxp;
    p->indataL[0] = (MYFLT*) ps;
    p->indataL[1] = (MYFLT*) (ps + size/2);
    if (p->fdata[1].auxp == NULL || p->fdata[1].size < size)
      csound->AuxAlloc(csound, size, &p->fdata[1]);
    ps = (char *) p->fdata[1].auxp;
    p->indataR[0] = (MYFLT*) ps;
    p->indataR[1] = (MYFLT*) (ps + size/2);
    if (p->buffer.auxp == NULL || p->buffer.size < size)
      csound->AuxAlloc(csound, size, &p->buffer);
  }
  /*
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    fdrecord(csound, &(p->fdch));
  */
  printf("fftsize = %d\n", p->N);
  int32_t skip = (int32_t)(*p->skip*CS_ESR)*p->resamp;
  p->bufused = -1;

  /*
    int32_t buffersize = size;
    buffersize /= mpainfo.decoded_sample_size;
    while (skip > 0) {
    int32_t xx= skip;
    if (xx > buffersize) xx = buffersize;
    skip -= xx;
    r = mp3dec_decode(mpa, p->buffer.auxp,
    mpainfo.decoded_sample_size*xx, &p->bufused);
    }*/
  mp3dec_seek(mpa, skip, MP3DEC_SEEK_SAMPLES);
  // fill buffers
  p->curbuf = 0;
  fillbuf(csound,p,p->N*BUFS/2);
  p->pos = p->hsize;
  p->tscale  = 0;
  p->accum = 0;
  p->tab[0] = (MYFLT *) p->fdata[0].auxp;
  p->tab[1] = (MYFLT *) p->fdata[1].auxp;
  p->tstamp = 0;
  p->initDone = -1;
  p->finished = 0;
  p->init = 1;
  /*clock_gettime(CLOCK_MONOTONIC, &ts);
    dtime = ts.tv_sec + 1e-9*ts.tv_nsec - dtime;
    csound->Message(csound, "MP3SCAL INIT time %f ms", dtime*1000);*/
  return OK;
}

#ifdef ANDROID
void *init_thread(void *p){
  DATASPACE *pp = (DATASPACE *) p;
  sinit3_(pp->csound,pp);
  return NULL;
}

static int32_t sinit3(CSOUND *csound, DATASPACE *p){
  p->csound = csound;
  p->init = 0;
  p->ti = 0;
  csound->Message(csound, "thread start\n");
  pthread_create(&p->t, NULL, init_thread, p);
  return OK;
}
#else

static int32_t sinit3(CSOUND *csound, DATASPACE *p) {
  return sinit3_(csound,p);
}

#endif


/*
  this will read a buffer full of samples
  from disk position offset samps from the last
  call to fillbuf
*/
void fillbuf(CSOUND *csound, DATASPACE *p, int32_t nsmps){
  IGN(csound);
  short *buffer= (short *) p->buffer.auxp;
  MYFLT *data[2];
  data[0] =  p->indataL[(int32_t)p->curbuf];
  data[1] =  p->indataR[(int32_t)p->curbuf];
  int32_t i,j, end;
  memset(data[0],0,nsmps*sizeof(MYFLT));
  memset(data[1],0,nsmps*sizeof(MYFLT));
  if(!p->finished){
    mp3dec_decode(p->mpa,p->buffer.auxp,
                  MP3_CHNS*nsmps*sizeof(short),
                  &p->bufused);
    if(p->bufused == 0) p->finished = 1;
    else {
      end = p->bufused/sizeof(short);
      for(i=j=0; i < end/2; i++, j+=2){
        data[0][i] = buffer[j]/32768.0;
        data[1][i] = buffer[j+1]/32768.0;
      }
    }
  }
  p->curbuf = p->curbuf ? 0 : 1;
}

static int32_t sprocess3(CSOUND *csound, DATASPACE *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  MYFLT pitch = *p->kpitch*p->resamp, time = *p->time*p->resamp, lock = *p->klock;
  MYFLT *out;
  MYFLT *tab, **table,frac;
  int32_t N = p->N, hsize = p->hsize, cnt = p->cnt;
  int32_t  nsmps = CS_KSMPS, n;
  int32_t size = (int32_t) p->fdata[0].size/sizeof(MYFLT), post, i, j;
  double pos, spos = p->pos;
  MYFLT *fwin, *bwin;
  MYFLT in, *prev;
  MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
  MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
  int32_t *framecnt, curframe = p->curframe;
  int32_t decim               = p->decim;
  int32_t interp              = *p->kinterp;
  double tstamp               = p->tstamp, incrt = p->incr;
  double amp                  = *p->kamp*csound->Get0dBFS(csound)*(8./decim)/3.;
  int32_t curbuf              = p->curbuf;
  AUXCH *mfwin = p->fwin,
    *mbwin = p->bwin,
    *mprev = p->prev,
    *moutframe = p->outframe,
    *mframecount = p->framecount;
  MYFLT hsizepitch = hsize*pitch;
  int32_t nbytes =  p->N*sizeof(MYFLT);

  if(time < 0) time = 0.0;
  table = p->tab;

  if(!p->init){
    for (j=0; j < MP3_CHNS; j++) {
      out = j == 0 ? p->out1 : p->out2;
      memset(out, '\0', nsmps*sizeof(MYFLT));
    }
    p->ti++;
    *p->kstamp = 0;
    return OK;
  }

  if(*p->kstamp == 0) csound->Message(csound, "waited %d cycles\n", p->ti);

  if (UNLIKELY(early)) {
    nsmps -= early;
    for (j=0; j < MP3_CHNS; j++) {
      out = j == 0 ? p->out1 : p->out2;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
  }
  if (UNLIKELY(offset)) {
    for (j=0; j < MP3_CHNS; j++) {
      out = j == 0 ? p->out1 : p->out2;
      memset(out, '\0', offset*sizeof(MYFLT));
    }
  }

  for (n=offset; n < nsmps; n++) {
    if (cnt == hsize){
      spos += hsize*time;
      incrt =  time*nsmps;

      while(spos >= size) {
        spos -= size;
      }
      while(spos < 0){
        spos += size;
      }
      if (spos > size/2+hsize && curbuf == 0) {
        fillbuf(csound,p,size/2);
      } else if (spos < size/2+hsize && curbuf == 1){
        fillbuf(csound,p,size/2);
      }

      for (j = 0; j < MP3_CHNS; j++) {
        bwin = (MYFLT *) mbwin[j].auxp;
        fwin = (MYFLT *) mfwin[j].auxp;
        prev = (MYFLT *) mprev[j].auxp;
        framecnt  = (int32_t *) mframecount[j].auxp;
        outframe= (MYFLT *) moutframe[j].auxp;
        tab = table[j];
        if(pitch != 1) {
          pos = spos;
          for (i=0; i < N; i++) {
            post = (int32_t) pos;
            frac = pos  - post;
            if(post < 0) post += size;
            if(post >= size) post -= size;
            if(post+1 <  size && interp){
              in = tab[post] + frac*(tab[post+1] - tab[post]);
            }
            else
              in = tab[post];
            fwin[i] = in * win[i];

            post -= hsizepitch;
            if(post < 0) post += size;
            if(post >= size) post -= size;
            if(post+1 <  size && interp){
              in = tab[post] + frac*(tab[post+1] - tab[post]);
            }
            else
              in = tab[post];
            bwin[i] = in * win[i];
            pos += pitch;
          }
        }
        else {
          post = (int32_t) spos;
          int32_t end = post+N;
          if(end <= size)
            memcpy(fwin,&tab[post],nbytes);
          else {
            int32_t endbytes;
            endbytes = (end - size)*sizeof(MYFLT);
            end = N - (end - size);
            memcpy(fwin,&tab[post],nbytes-endbytes);
            memcpy(&fwin[end],tab,endbytes);
          }
          post -= hsize;
          if(post < 0) post += size;
          end = post+N;
          if(end < size)
            memcpy(bwin,&tab[post],nbytes);
          else {
            int32_t endbytes;
            endbytes = (end - size)*sizeof(MYFLT);
            end = N - (end - size);
            memcpy(bwin,&tab[post],nbytes-endbytes);
            memcpy(&bwin[end],tab,endbytes);
          }
          for(i=0; i < N; i++) {
            bwin[i] *= win[i];
            fwin[i] *= win[i];
          }
        }


        csound->RealFFT(csound, p->fwdsetup, bwin);
        bwin[N] = bwin[1];
        bwin[N+1] = FL(0.0);
        csound->RealFFT(csound, p->fwdsetup, fwin);
        fwin[N] = fwin[1];
        fwin[N+1] = FL(0.0);

        for (i=0; i < N + 2; i+=2) {

          div =  FL(1.0)/(HYPOT(prev[i], prev[i+1]) + 1.0e-20);
          ph_real  =    prev[i]*div;
          ph_im =       prev[i+1]*div;

          tmp_real =   bwin[i] * ph_real + bwin[i+1] * ph_im;
          tmp_im =   bwin[i] * ph_im - bwin[i+1] * ph_real;
          bwin[i] = tmp_real;
          bwin[i+1] = tmp_im;
        }

        for (i=0; i < N + 2; i+=2) {
          if (lock) {
            if (i > 0) {
              if (i < N) {
                tmp_real = bwin[i] + bwin[i-2] + bwin[i+2];
                tmp_im = bwin[i+1] + bwin[i-1] + bwin[i+3];
              }
              else {
                tmp_real = bwin[i] + bwin[i-2];
                tmp_im = FL(0.0);
              }
            }
            else {
              tmp_real = bwin[i] + bwin[i+2];
              tmp_im = FL(0.0);
            }
          }
          else {
            tmp_real = bwin[i];
            tmp_im = bwin[i+1];
          }

          tmp_real += 1e-15;
          div =  FL(1.0)/(HYPOT(tmp_real, tmp_im));

          ph_real = tmp_real*div;
          ph_im = tmp_im*div;

          tmp_real = fwin[i] * ph_real - fwin[i+1] * ph_im;
          tmp_im = fwin[i] * ph_im + fwin[i+1] * ph_real;

          prev[i] = fwin[i] = tmp_real;
          prev[i+1] = fwin[i+1] = tmp_im;
        }

        fwin[1] = fwin[N];
        csound->RealFFT(csound, p->invsetup, fwin);
        framecnt[curframe] = curframe*N;
        for (i=0;i<N;i++)
          outframe[framecnt[curframe]+i] = win[i]*fwin[i];
      }
      cnt=0;
      curframe++;
      if (curframe == decim) curframe = 0;
    }

    /* we only output as many channels as we have outs for */
    for (j=0; j < 2; j++) {
      out = j == 0 ? p->out1 : p->out2;
      framecnt  = (int32_t *) p->framecount[j].auxp;
      outframe  = (MYFLT *) p->outframe[j].auxp;
      out[n] = (MYFLT) 0;

      for (i = 0; i < decim; i++) {
        out[n] += outframe[framecnt[i]];
        framecnt[i]++;
      }
      out[n] *= amp;
    }
    cnt++;
  }
  p->cnt = cnt;
  p->curframe = curframe;
  p->pos = spos;
  p->tstamp = tstamp + incrt;
  *p->kstamp = (*p->skip + p->tstamp/CS_ESR)/p->resamp;
  p->incr = incrt;
  return OK;

}

#define S(x)    sizeof(x)

static OENTRY mp3in_localops[] =
  {
    {"mp3in",  S(MP3IN),  _QQ,  "mm", "Soooo", (SUBR) mp3ininit_S, (SUBR)mp3in, (SUBR) mp3in_cleanup},
    {"mp3in",  S(MP3IN),  _QQ,  "mm", "ioooo", (SUBR) mp3ininit, (SUBR)mp3in, (SUBR) mp3in_cleanup},
    {"mp3len", S(MP3LEN), _QQ,  "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
    {"mp3len", S(MP3LEN), _QQ,  "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
    {"mp3sr", S(MP3LEN), 0,  "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
    {"mp3sr", S(MP3LEN), 0,  "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
    {"mp3bitrate", S(MP3LEN), 0,  "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
    {"mp3bitrate", S(MP3LEN), 0,  "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
    {"mp3nchnls", S(MP3LEN), 0,  "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
    {"mp3nchnls", S(MP3LEN), 0,  "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
    {"mp3scal", sizeof(DATASPACE), 0,  "aak", "SkkkoooPP",
     (SUBR)sinit3,(SUBR)sprocess3, (SUBR) mp3scale_cleanup},
  };




#include "mp3dec.h"

static CS_NOINLINE void ftresdisp(const FGDATA *ff, FUNC *ftp)
{
  MYFLT   *fp, *finp = &ftp->ftable[ff->flen];
  MYFLT   abs, maxval;

  if (!ff->guardreq)                      /* if no guardpt yet, do it */
    ftp->ftable[ff->flen] = ftp->ftable[0];
  if (ff->e.p[4] > FL(0.0)) {             /* if genum positve, rescale */
    for (fp=ftp->ftable, maxval = FL(0.0); fp<=finp; ) {
      if ((abs = *fp++) < FL(0.0))
        abs = -abs;
      if (abs > maxval)
        maxval = abs;
    }
    if (maxval != FL(0.0) && maxval != FL(1.0))
      for (fp=ftp->ftable; fp<=finp; fp++)
        *fp /= maxval;
  }
}

static CS_NOINLINE FUNC *ftalloc(const FGDATA *ff, FUNC *ftp)
{
  CSOUND  *csound = ff->csound;
 
  if (UNLIKELY(ftp != NULL)) {
    csound->Warning(csound, Str("replacing previous ftable %d"), ff->fno);
    if (ff->flen != (int32)ftp->flen) {       /* if redraw & diff len, */
      csound->Free(csound, ftp->ftable);
      csound->Free(csound, (void*) ftp);             /*   release old space   */
    }
    else {
      /* else clear it to zero */
      MYFLT *tmp = ftp->ftable;
      memset((void*) ftp->ftable, 0, sizeof(MYFLT)*(ff->flen+1));
      memset((void*) ftp, 0, sizeof(FUNC));
      ftp->ftable = tmp; /* restore table pointer */
    }
  }
  if (ftp == NULL) {                      /*   alloc space as reqd */
    ftp = (FUNC*) csound->Calloc(csound, sizeof(FUNC));
    ftp->ftable = (MYFLT*) csound->Calloc(csound, (1+ff->flen) * sizeof(MYFLT));
  }
  ftp->fno = (int32) ff->fno;
  ftp->flen = ff->flen;
  return ftp;
}

int32_t gen49raw(FGDATA *ff, FUNC *ftp)
{
  CSOUND  *csound        = ff->csound;
  MYFLT   *fp           = ftp == NULL ? NULL: ftp->ftable;
  mp3dec_t mpa           = NULL;
  mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_AUTO,
                             MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                             MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                             0.0 };
  int32_t     skip              = 0, chan = 0, r;
  FILE    *f;
  int32_t p                     = 0;
  char    sfname[1024];
  mpadec_info_t mpainfo;
  uint32_t bufsize, bufused = 0;
  uint8_t *buffer;
  int32_t size = 0x1000;
  int32_t flen, nchanls, def = 0;

  if (UNLIKELY(ff->e.pcnt < 7)) {
    return csound->FtError(ff, "%s", Str("insufficient arguments"));
  }
  {
    int32 filno /*= (int32) MYFLT2LRND(ff->e.p[5])*/;
    if (IsStringCode(ff->e.p[5])) {
      if (ff->e.strarg[0] == '"') {
        int32_t len = (int32_t) strlen(ff->e.strarg) - 2;
        strncpy(sfname, ff->e.strarg + 1, 1024);
        if (len >= 0 && sfname[len] == '"')
          sfname[len] = '\0';
      }
      else
        strncpy(sfname, ff->e.strarg, 1024);
    }
    else if ((filno= (int32) MYFLT2LRND(ff->e.p[5])) >= 0)
      snprintf(sfname, 1024, "soundin.%d", filno);   /* soundin.filno */
  }
  chan  = (int32_t) MYFLT2LRND(ff->e.p[7]);
  if (UNLIKELY(chan < 0)) {
    return csound->FtError(ff, Str("channel %d illegal"), (int32_t) chan);
  }
  switch (chan) {
  case 0:
    config.mode = MPADEC_CONFIG_AUTO; break;
  case 1:
    config.mode = MPADEC_CONFIG_MONO; break;
  case 2:
    config.mode = MPADEC_CONFIG_STEREO; break;
  case 3:
    config.mode = MPADEC_CONFIG_CHANNEL1; break;
  case 4:
    config.mode = MPADEC_CONFIG_CHANNEL2; break;
  }
  mpa = mp3dec_init(csound);
  if (UNLIKELY(!mpa)) {
    return csound->FtError(ff, "%s", Str("Not enough memory\n"));
  }
  if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->FtError(ff,"%s", mp3dec_error(r));
  }
  (void)mp3dec_open_file(mpa, sfname, &f);
  //    fd = open(sfname, O_RDONLY); /* search paths */
  if (UNLIKELY(f < 0)) {
    mp3dec_uninit(mpa);
    return csound->FtError(ff, "sfname");
  }
  if (UNLIKELY((r = mp3dec_init_file(mpa, f, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->FtError(ff,"%s", mp3dec_error(r));
  }
  if (UNLIKELY((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
               MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->FtError(ff,"%s", mp3dec_error(r));
  }
  /* maxsize = mpainfo.decoded_sample_size */
  /*   *mpainfo.decoded_frame_samples */
  /*   *mpainfo.frames; */
  {
    char temp[80];
    if (mpainfo.frequency < 16000) strcpy(temp, "MPEG-2.5 ");
    else if (mpainfo.frequency < 32000) strcpy(temp, "MPEG-2 ");
    else strcpy(temp, "MPEG-1 ");
    if (mpainfo.layer == 1) strcat(temp, "Layer I");
    else if (mpainfo.layer == 2) strcat(temp, "Layer II");
    else strcat(temp, "Layer III");
    csound->DebugMsg(csound, "Input:  %s, %s, %d kbps, %d Hz  (%d:%02d)\n",
                     temp, ((mpainfo.channels > 1) ? "stereo" : "mono"),
                     mpainfo.bitrate, mpainfo.frequency, mpainfo.duration/60,
                     mpainfo.duration%60);
  }
  buffer = (uint8_t *)csound->Malloc(csound,size);
  bufsize = size/mpainfo.decoded_sample_size;
  skip = (int)(ff->e.p[6] * mpainfo.frequency);
  while (skip > 0) {
    uint32_t xx = skip;
    if ((uint32_t)xx > bufsize) xx = bufsize;
    //      printf("gen49: skipping xx\n", xx);
    skip -=xx;
    mp3dec_decode(mpa, buffer, mpainfo.decoded_sample_size*xx, &bufused);
  }
  //bufsize *= mpainfo.decoded_sample_size;
  r = mp3dec_decode(mpa, buffer, size, &bufused);
  nchanls = (chan == 2 && mpainfo.channels == 2 ? 2 : 1);
  if (ff->flen == 0) {    /* deferred ftalloc */
    int32_t fsize, frames;
    frames = mpainfo.frames * mpainfo.decoded_frame_samples;
    fsize  = frames * nchanls;
    if (UNLIKELY((ff->flen = fsize) <= 0))
      return csound->FtError(ff, "%s", Str("deferred size, but filesize unknown"));
    if (UNLIKELY(ff->flen > MAXLEN))
      return csound->FtError(ff, "%s", Str("illegal table length"));
    ftp = ftalloc(ff,ftp);
    ftp->lenmask  = 0L;
    ftp->flenfrms = frames;
    ftp->nchanls  = nchanls;
    fp = ftp->ftable;
    def = 1;
  }
  ftp->gen01args.sample_rate = mpainfo.frequency;
  ftp->cvtbas = mpainfo.frequency * ftp->sr; 
  flen = ftp->flen;
  //printf("gen49: flen=%d size=%d bufsize=%d\n", flen, size, bufsize);
  while ((r == MP3DEC_RETCODE_OK) && bufused) {
    uint32_t i;
    short *bb = (short*)buffer;
    //printf("gen49: p=%d bufused=%d\n", p, bufused);
    for (i=0; i<bufused*nchanls/mpainfo.decoded_sample_size; i++)  {
      if (UNLIKELY(p>=flen)) {
        csound->Free(csound,buffer);
        //printf("gen49: i=%d p=%d exit as at end of table\n", i, p);
        return ((mp3dec_uninit(mpa) == MP3DEC_RETCODE_OK) ? OK : NOTOK);
      }
      fp[p] = ((MYFLT)bb[i]/(MYFLT)0x7fff) * csound->Get0dBFS(csound);
      //printf("%d: %f %d\n", p, fp[p], bb[i]);
      p++;
    }
    if (i <= 0) break;
    //printf("gen49: new buffer\n");
    r = mp3dec_decode(mpa, buffer, size, &bufused);
  }

  csound->Free(csound, buffer);
  r |= mp3dec_uninit(mpa);
  if (def) ftresdisp(ff, ftp);
  return ((r == MP3DEC_RETCODE_OK) ? OK : NOTOK);
}

int32_t gen49(FGDATA *ff, FUNC *ftp)
{
  CSOUND *csound = ff->csound;
  if (UNLIKELY(ff->e.pcnt < 7)) {
    return csound->FtError(ff, "%s", Str("insufficient arguments"));
  }
  return gen49raw(ff, ftp);
}

static NGFENS mp3in_fgen[] = {
  { "gen49", gen49 },
  { NULL, NULL }
};

NGFENS *mp3in_fgen_init(CSOUND *csound) {
  (void) csound;
  return mp3in_fgen;
}

int32_t mp3in_localops_init(CSOUND *csound,
                            OENTRY **ep)  
{
  (void) csound;
  *ep = mp3in_localops;
  return (int32_t) sizeof(mp3in_localops);
}

#ifdef BUILD_PLUGINS
PUBLIC int64_t csound_opcode_init(CSOUND *csound, OENTRY **ep) {
  return mp3in_localops_init(csound, ep);
}

PUBLIC NGFENS *csound_fgen_init(CSOUND *csound)                         \
{
  return   mp3in_fgen_init(csound);
}

PUBLIC int32_t csoundModuleInfo(void)                                      
{
  return ((CS_VERSION << 16)
          + (CS_SUBVER << 8)
          + (int32_t) sizeof(MYFLT));
}
#endif
