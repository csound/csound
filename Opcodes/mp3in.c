/*
  mp3in.c:

  Copyright (C) 2009 by John ffitch,

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

/* mp3in.c */
/* #include "csdl.h" */
#include "csoundCore.h"
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
  mp3dec_t mpa;               /* For library */
  int      r;                  /* Result field */
  int      initDone;
  int      bufSize;            /* in sample frames, power of two */
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

int mp3in_cleanup(CSOUND *csound, MP3IN *p)
{
  if (p->mpa != NULL)
    mp3dec_uninit(p->mpa);
  return OK;
}

int mp3ininit_(CSOUND *csound, MP3IN *p, int stringname)
{
  char    name[1024];
  int     fd;
  mp3dec_t mpa           = NULL;
  mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_STEREO,
                             MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                             MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                             0.0 };
  mpadec_info_t mpainfo;
  int buffersize = (*p->ibufsize<=0.0 ? 0x1000 : (int)*p->ibufsize);
  /* uint64_t maxsize; */
  int r;
  int skip;
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

  p->mpa = mpa = mp3dec_init();
  if (UNLIKELY(!mpa)) {
    return csound->InitError(csound, Str("Not enough memory\n"));
  }

  if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    p->mpa = NULL;
    return csound->InitError(csound, mp3dec_error(r));
  }


  /* FIXME: name can overflow with very long string */
  if(stringname==0){
    if (ISSTRCOD(*p->iFileCode))
      strncpy(name,get_arg_string(csound, *p->iFileCode), 1023);
    else csound->strarg2name(csound, name, p->iFileCode, "soundin.",0);
  }
  else strncpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);

  if (UNLIKELY(csound->FileOpen2(csound, &fd, CSFILE_FD_R,
                                 name, "rb", "SFDIR;SSDIR",
                                 CSFTYPE_OTHER_BINARY, 0) == NULL)) {
    mp3dec_uninit(mpa);
    return
      csound->InitError(csound, Str("mp3in: %s: failed to open file"), name);
  }
  /* HOW TO record file handle so that it will be closed at note-off */
  /* memset(&(p->fdch), 0, sizeof(FDCH)); */
  /* p->fdch.fd = fd; */
  /* fdrecord(csound, &(p->fdch)); */
  if (UNLIKELY((r = mp3dec_init_file(mpa, fd, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, mp3dec_error(r));
  }
  if (UNLIKELY((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
               MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, mp3dec_error(r));
  }
  skip = (int)(*p->iSkipTime*CS_ESR+1);
  /* maxsize = mpainfo.decoded_sample_size */
  /*          *mpainfo.decoded_frame_samples */
  /*          *mpainfo.frames; */
  /* csound->Message(csound, "maxsize = %li\n", maxsize); */
  /* print file information */
  /* if (UNLIKELY(csound->oparms_.msglevel & WARNMSG)) */ {
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
  }
  /* check number of channels in file (must equal the number of outargs) */
  /* if (UNLIKELY(sfinfo.channels != p->nChannels && */
  /*              (csound->oparms_.msglevel & WARNMSG) != 0)) { */
  /*   mp3dec_uninit(mpa); */
  /*   return csound->InitError(csound, */
  /*                      Str("mp3in: number of output args " */
  /*                          "inconsistent with number of file channels")); */
  /* } */
  /* skip initialisation if requested */
  if (*(p->iSkipInit) != FL(0.0))
    return OK;
  /* set file parameters from header info */
  if ((int) (CS_ESR + FL(0.5)) != mpainfo.frequency) {
    csound->Warning(csound, Str("mp3in: file sample rate (%d) "
                                "!= orchestra sr (%d)\n"),
                    mpainfo.frequency, (int) (CS_ESR + FL(0.5)));
  }
  /* initialise buffer */
  p->bufSize = buffersize;
  if (p->auxch.auxp == NULL || p->auxch.size < (unsigned int)buffersize)
    csound->AuxAlloc(csound, buffersize, &p->auxch);
  p->buf = (uint8_t *) p->auxch.auxp;
  p->bufused = -1;
  buffersize /= mpainfo.decoded_sample_size;
  /*while (skip > 0) {
    int xx= skip;
    if (xx > buffersize) xx = buffersize;
    skip -= xx;
    r = mp3dec_decode(mpa, p->buf, mpainfo.decoded_sample_size*xx, &p->bufused);
    }*/
  mp3dec_seek(mpa, skip, MP3DEC_SEEK_SAMPLES);
  p->r = r;
  if(p->initDone == -1)
    csound->RegisterDeinitCallback(csound, p,
                                   (int (*)(CSOUND*, void*)) mp3in_cleanup);
  /* done initialisation */
  p->initDone = -1;
  p->pos = 0;

  return OK;
}

int mp3ininit(CSOUND *csound, MP3IN *p){
  return mp3ininit_(csound,p,0);
}

int mp3ininit_S(CSOUND *csound, MP3IN *p){
  return mp3ininit_(csound,p,1);
}


int mp3in(CSOUND *csound, MP3IN *p)
{
  int r           = p->r;
  mp3dec_t mpa    = p->mpa;
  uint8_t *buffer = p->buf;
  MYFLT *al       = p->ar[0];
  MYFLT *ar       = p->ar[1];
  int pos         = p->pos;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t i, n, nsmps = CS_KSMPS;

  if (UNLIKELY(offset)) {
    memset(al, '\0', offset*sizeof(MYFLT));
    memset(ar, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&al[nsmps], '\0', early*sizeof(MYFLT));
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    for (i=0; i<p->OUTOCOUNT; i++) {     /* stereo */
      MYFLT xx;
      short *bb = (short*)buffer;
      while (r != MP3DEC_RETCODE_OK || 2*pos >=  (int)p->bufused) {
        r = mp3dec_decode(mpa, buffer, p->bufSize, &p->bufused);
        if (UNLIKELY(p->bufused == 0)) {
          memset(&al[n], 0, (nsmps-n)*sizeof(MYFLT));
          memset(&ar[n], 0, (nsmps-n)*sizeof(MYFLT));
          goto ending;
        }
        pos = 0;
      }
      xx = ((MYFLT)bb[pos]/(MYFLT)0x7fff) * csound->e0dbfs;
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

int mp3len_(CSOUND *csound, MP3LEN *p, int stringname)
{
  char    name[1024];
  int     fd;
  mp3dec_t mpa           = NULL;
  mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_STEREO,
                             MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                             MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                             0.0 };
  mpadec_info_t mpainfo;
  int r;

  /* open file */
  mpa = mp3dec_init();
  if (UNLIKELY(!mpa)) {
    return csound->InitError(csound, Str("Not enough memory\n"));
  }
  if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, mp3dec_error(r));
  }
  /* FIXME: name can overflow with very long string */

  if(stringname==0){
    if(ISSTRCOD(*p->iFileCode))
      strncpy(name,get_arg_string(csound, *p->iFileCode), 1023);
    else csound->strarg2name(csound, name, p->iFileCode, "soundin.",0);
  }
  else strncpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);
  if (UNLIKELY(csound->FileOpen2(csound, &fd, CSFILE_FD_R,
                                 name, "rb", "SFDIR;SSDIR",
                                 CSFTYPE_OTHER_BINARY, 0) == NULL)) {
    mp3dec_uninit(mpa);
    return
      csound->InitError(csound, Str("mp3in: %s: failed to open file"), name);
  }
  if (UNLIKELY((r = mp3dec_init_file(mpa, fd, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, mp3dec_error(r));
  }
  if (UNLIKELY((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
               MP3DEC_RETCODE_OK)) {
    close(fd);
    mp3dec_uninit(mpa);
    return csound->InitError(csound, mp3dec_error(r));
  }
  close(fd);

  if(!strcmp(csound->GetOpcodeName(&p->h), "mp3len"))
    *p->ir = (MYFLT)mpainfo.duration;
  else if(!strcmp(csound->GetOpcodeName(&p->h), "mp3sr"))
    *p->ir = (MYFLT) mpainfo.frequency;
  else if(!strcmp(csound->GetOpcodeName(&p->h), "mp3bitrate"))
    *p->ir = (MYFLT) mpainfo.bitrate;
  else if(!strcmp(csound->GetOpcodeName(&p->h), "mp3nchnls"))
    *p->ir = (MYFLT) mpainfo.channels;

  mp3dec_uninit(mpa);
  return OK;
}

int mp3len(CSOUND *csound, MP3LEN *p){
  return mp3len_(csound,p,0);
}

int mp3len_S(CSOUND *csound, MP3LEN *p){
  return mp3len_(csound,p,1);
}

#define MP3_CHNS 2
typedef struct dats{
  OPDS h;
  MYFLT *out1,*out2,*kstamp, *knum, *time,*kpitch, *kamp, *skip, *iN,
    *idecim, *klock,*kinterp;
  int cnt, hsize, curframe, N, decim,tscale;
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
  int initDone;
  uint32_t bufused;
  int finished;
  char init;
  CSOUND *csound;
  pthread_t t;
  int ti;
  char filling;
} DATASPACE;

int mp3scale_cleanup(CSOUND *csound, DATASPACE *p)
{
  if (p->mpa != NULL)
    mp3dec_uninit(p->mpa);
  return OK;
}

#define BUFS 4
static void fillbuf(CSOUND *csound, DATASPACE *p, int nsmps);
/* file-reading version of temposcal */
static int sinit(CSOUND *csound, DATASPACE *p)
{

  int N =  *p->iN, ui;
  unsigned int nchans, i;
  unsigned int size;
  int decim = *p->idecim;
  /*double dtime;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  dtime = ts.tv_sec + 1e-9*ts.tv_nsec;*/
  if (N) {
    for (i=0; N; i++) {
      N >>= 1;
    }
    N = (int) pow(2.0, i-1);
  } else N = 2048;
  if (decim == 0) decim = 4;

  p->hsize = N/decim;
  p->cnt = p->hsize;
  p->curframe = 0;
  p->pos = 0;

  for (i=0; i < MP3_CHNS; i++){

    size = (N+2)*sizeof(MYFLT);
    if (p->fwin[i].auxp == NULL || p->fwin[i].size < size)
      csound->AuxAlloc(csound, size, &p->fwin[i]);
    if (p->bwin[i].auxp == NULL || p->bwin[i].size < size)
      csound->AuxAlloc(csound, size, &p->bwin[i]);
    if (p->prev[i].auxp == NULL || p->prev[i].size < size)
      csound->AuxAlloc(csound, size, &p->prev[i]);
    size = decim*sizeof(int);
    if (p->framecount[i].auxp == NULL || p->framecount[i].size < size)
      csound->AuxAlloc(csound, size, &p->framecount[i]);
    {
      int k=0;
      for (k=0; k < decim; k++) {
        ((int *)(p->framecount[i].auxp))[k] = k*N;
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
  return OK;
}
static int sinit3_(CSOUND *csound, DATASPACE *p)
{
  unsigned int size,i;
  char *name;
  SF_INFO sfinfo;
  // open file
  int fd;
  int r;
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
  p->mpa = mpa = mp3dec_init();
  if (UNLIKELY(!mpa)) {
    return csound->InitError(csound, Str("Not enough memory\n"));
  }
  if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    p->mpa = NULL;
    return csound->InitError(csound, mp3dec_error(r));
  }
  if (UNLIKELY(csound->FileOpen2(csound, &fd, CSFILE_FD_R,
                                 name, "rb", "SFDIR;SSDIR",
                                 CSFTYPE_OTHER_BINARY, 0) == NULL)) {
    mp3dec_uninit(mpa);
    return
      csound->InitError(csound, Str("mp3scale: %s: failed to open file"), name);
  }// else
  // csound->Message(csound, Str("mp3scale: open %s \n"), name);
  if (UNLIKELY((r = mp3dec_init_file(mpa, fd, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, mp3dec_error(r));
  } // else
  // csound->Message(csound, Str("mp3scale: init %s \n"), name);

  if (UNLIKELY((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
               MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return csound->InitError(csound, mp3dec_error(r));
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

  sinit(csound, p);
  size = p->N*sizeof(MYFLT)*BUFS;
  if (p->fdata[0].auxp == NULL || p->fdata[0].size < size)
    csound->AuxAlloc(csound, size, &p->fdata[0]);
  p->indataL[0] = p->fdata[0].auxp;
  p->indataL[1] = p->fdata[0].auxp + size/2;
  if (p->fdata[1].auxp == NULL || p->fdata[1].size < size)
    csound->AuxAlloc(csound, size, &p->fdata[1]);
  p->indataR[0] = p->fdata[1].auxp;
  p->indataR[1] = p->fdata[1].auxp + size/2;
  if (p->buffer.auxp == NULL || p->buffer.size < size)
    csound->AuxAlloc(csound, size, &p->buffer);

  /*
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    fdrecord(csound, &(p->fdch));
  */
  printf("fftsize = %d \n", p->N);
  int buffersize = size;
  buffersize /= mpainfo.decoded_sample_size;
  int skip = (int)(*p->skip*CS_ESR)*p->resamp;
  p->bufused = -1;

  /*while (skip > 0) {
    int xx= skip;
    if (xx > buffersize) xx = buffersize;
    skip -= xx;
    r = mp3dec_decode(mpa, p->buffer.auxp, mpainfo.decoded_sample_size*xx, &p->bufused);
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
  if(p->initDone == -1)
    csound->RegisterDeinitCallback(csound, p,
                                   (int (*)(CSOUND*, void*)) mp3scale_cleanup);
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

static int sinit3(CSOUND *csound, DATASPACE *p){
  p->csound = csound;
  p->init = 0;
  p->ti = 0;
  csound->Message(csound, "thread start\n");
  pthread_create(&p->t, NULL, init_thread, p);
  return OK;
}
#else

static int sinit3(CSOUND *csound, DATASPACE *p) {
  return sinit3_(csound,p);
}

#endif


/*
  this will read a buffer full of samples
  from disk position offset samps from the last
  call to fillbuf
*/
void fillbuf(CSOUND *csound, DATASPACE *p, int nsmps){
  short *buffer= (short *) p->buffer.auxp;
  MYFLT *data[2];
  data[0] =  p->indataL[(int)p->curbuf];
  data[1] =  p->indataR[(int)p->curbuf];
  int r,i,j, end;
  memset(data[0],0,nsmps*sizeof(MYFLT));
  memset(data[1],0,nsmps*sizeof(MYFLT));
  if(!p->finished){
    r = mp3dec_decode(p->mpa,p->buffer.auxp,
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

static int sprocess3(CSOUND *csound, DATASPACE *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  MYFLT pitch = *p->kpitch*p->resamp, time = *p->time*p->resamp, lock = *p->klock;
  MYFLT *out;
  MYFLT *tab, **table,frac;
  FUNC *ft;
  int N = p->N, hsize = p->hsize, cnt = p->cnt;
  int  nsmps = CS_KSMPS, n;
  int size = p->fdata[0].size/sizeof(MYFLT), post, i, j;
  double pos, spos = p->pos;
  MYFLT *fwin, *bwin;
  MYFLT in, *nwin, *prev;
  MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
  MYFLT powrat;
  MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
  int *framecnt, curframe = p->curframe;
  int decim = p->decim;
  int interp = *p->kinterp;
  double tstamp = p->tstamp, incrt = p->incr;
  double amp = *p->kamp*csound->Get0dBFS(csound)*(8./decim)/3.;
  int curbuf = p->curbuf;
  AUXCH *mfwin = p->fwin,
    *mbwin = p->bwin,
    *mprev = p->prev,
    *moutframe = p->outframe,
    *mframecount = p->framecount;
  MYFLT hsizepitch = hsize*pitch;
  int nbytes =  p->N*sizeof(MYFLT);

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
         framecnt  = (int *) mframecount[j].auxp;
         outframe= (MYFLT *) moutframe[j].auxp;
         tab = table[j];
        if(pitch != 1) {
          pos = spos;
          for (i=0; i < N; i++) {
            post = (int) pos;
            frac = pos  - post;
            if(post < 0) post += size;
            if(post >= size) post -= size;
            if(post+1 <  size ){
              in = tab[post] + frac*(tab[post+1] - tab[post]);
            }
            else
              in = tab[post];
            fwin[i] = in * win[i];

            post -= hsizepitch;
            if(post < 0) post += size;
            if(post >= size) post -= size;
            if(post+1 <  size){
              in = tab[post] + frac*(tab[post+1] - tab[post]);
            }
            else
              in = tab[post];
            bwin[i] = in * win[i];
            pos += pitch;
          }
        }
        else {
          post = (int) spos;
          int end = post+N;
          if(end <= size)
            memcpy(fwin,&tab[post],nbytes);
          else {
            int endbytes;
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
            int endbytes;
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


        csound->RealFFT(csound, bwin, N);
        bwin[N] = bwin[1];
        bwin[N+1] = FL(0.0);
        csound->RealFFT(csound, fwin, N);
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
        csound->InverseRealFFT(csound, fwin, N);
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
      framecnt  = (int *) p->framecount[j].auxp;
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
  *p->kstamp = (*p->skip + p->tstamp/csound->GetSr(csound))/p->resamp;
  p->incr = incrt;
  return OK;

}

typedef struct _mp3scal2_ {
  int cnt, hsize, curframe, N, decim,tscale;
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
  int initDone;
  uint32_t bufused;
  int finished;
  char init;
  CSOUND *csound;
  pthread_t t,t1;
  int ti;
  MYFLT ilen;
  MYFLT skip;
  char playing;
  int nsmps;
  char filling;
  int async;
  int error;
} MP3SCAL2;

typedef struct _loader {
  OPDS h;
  STRINGDAT *res;
  STRINGDAT *name;
  MYFLT *skip, *iN, *idecim;
  MP3SCAL2 p;
} LOADER;


void *buffiller(void *pp){
  MP3SCAL2 *p = (MP3SCAL2 *) pp;
  int nsmps = p->nsmps;
  short *buffer= (short *) p->buffer.auxp;
  MYFLT *data[2];
  data[0] =  p->indataL[(int)p->curbuf];
  data[1] =  p->indataR[(int)p->curbuf];
  int r,i,j, end;
  memset(data[0],0,nsmps*sizeof(MYFLT));
  memset(data[1],0,nsmps*sizeof(MYFLT));
  if(!p->finished){
    r = mp3dec_decode(p->mpa,p->buffer.auxp,
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
  return NULL;
}

void fillbuf2(CSOUND *csound, MP3SCAL2 *p, int nsmps){
  p->nsmps = nsmps;
  if(p->async)
   pthread_create(&(p->t1), NULL, buffiller, p);
  else
    buffiller((void *) p);
}

static int meminit(CSOUND *csound, LOADER *pp)
{

  MP3SCAL2 *p = &(pp->p);
  int N =  *pp->iN, ui;
  unsigned int nchans, i;
  unsigned int size;
  int decim = *pp->idecim;
  p->error = 0;
  if (N) {
    for (i=0; N; i++) {
      N >>= 1;
    }
    N = (int) pow(2.0, i-1);
  } else N = 2048;
  if (decim == 0) decim = 4;

  p->hsize = N/decim;
  p->cnt = p->hsize;
  p->curframe = 0;
  p->pos = 0;

  for (i=0; i < MP3_CHNS; i++){

    size = (N+2)*sizeof(MYFLT);
    if (p->fwin[i].auxp == NULL || p->fwin[i].size < size)
      csound->AuxAlloc(csound, size, &p->fwin[i]);
    if (p->bwin[i].auxp == NULL || p->bwin[i].size < size)
      csound->AuxAlloc(csound, size, &p->bwin[i]);
    if (p->prev[i].auxp == NULL || p->prev[i].size < size)
      csound->AuxAlloc(csound, size, &p->prev[i]);
    size = decim*sizeof(int);
    if (p->framecount[i].auxp == NULL || p->framecount[i].size < size)
      csound->AuxAlloc(csound, size, &p->framecount[i]);
    {
      int k=0;
      for (k=0; k < decim; k++) {
        ((int *)(p->framecount[i].auxp))[k] = k*N;
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
  return OK;
}

static int filinit(CSOUND *csound, LOADER *pp)
{
  MP3SCAL2 *p = &(pp->p);
  unsigned int size,i;
  char *name;
  SF_INFO sfinfo;
  // open file
  int fd;
  int r;
  mp3dec_t mpa           = NULL;
  mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_STEREO,
                             MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                             MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                             0.0 };
  mpadec_info_t mpainfo;
  name = pp->name->data;
  p->mpa = mpa = mp3dec_init();
  if (UNLIKELY(!mpa)) {
    p->error = MPADEC_RETCODE_NOT_ENOUGH_MEMORY;
    return NOTOK;
  }
  if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    p->mpa = NULL;
    p->error = r;
    return NOTOK;
  }
  if (UNLIKELY(csound->FileOpen2(csound, &fd, CSFILE_FD_R,
                                 name, "rb", "SFDIR;SSDIR",
                                 CSFTYPE_OTHER_BINARY, 0) == NULL)) {
    mp3dec_uninit(mpa);
    p->error = -1;
    return NOTOK;
  }
  if (UNLIKELY((r = mp3dec_init_file(mpa, fd, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    p->error = r;
    return NOTOK;
  }

  if (UNLIKELY((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
               MP3DEC_RETCODE_OK)) {
    mp3dec_uninit(mpa);
    return NOTOK;
  }

  p->ilen =  (MYFLT) mpainfo.duration;
  if(mpainfo.frequency != CS_ESR)
    p->resamp = mpainfo.frequency/CS_ESR;
  else
    p->resamp = 1;
  meminit(csound, pp);
  size = p->N*sizeof(MYFLT)*BUFS;
  if (p->fdata[0].auxp == NULL || p->fdata[0].size < size)
    csound->AuxAlloc(csound, size, &p->fdata[0]);
  p->indataL[0] = p->fdata[0].auxp;
  p->indataL[1] = p->fdata[0].auxp + size/2;
  if (p->fdata[1].auxp == NULL || p->fdata[1].size < size)
    csound->AuxAlloc(csound, size, &p->fdata[1]);
  p->indataR[0] = p->fdata[1].auxp;
  p->indataR[1] = p->fdata[1].auxp + size/2;
  if (p->buffer.auxp == NULL || p->buffer.size < size)
    csound->AuxAlloc(csound, size, &p->buffer);

  int buffersize = size;
  buffersize /= mpainfo.decoded_sample_size;
  int skip = (int)(*pp->skip*CS_ESR)*p->resamp;
  p->bufused = -1;
  mp3dec_seek(mpa, skip, MP3DEC_SEEK_SAMPLES);

  // fill buffers
  p->curbuf = 0;
  p->nsmps = p->N*BUFS/2;
  buffiller((void *)p);
  p->pos = p->hsize;
  p->tscale  = 0;
  p->accum = 0;
  p->tab[0] = (MYFLT *) p->fdata[0].auxp;
  p->tab[1] = (MYFLT *) p->fdata[1].auxp;
  p->tstamp = 0;
  p->initDone = -1;
  p->finished = 0;
  p->init = 1;
  p->skip = *pp->skip;
  p->filling = 1;
  return OK;
}

void *loader_thread(void *p){
  LOADER *pp = (LOADER *) p;
  if(filinit(pp->p.csound,pp) == NOTOK) {
    if(pp->p.error > 0)
    pp->p.csound->Message(pp->p.csound, "mp3scal_load error: %s \n",
                          mp3dec_error(pp->p.error));
    else
    pp->p.csound->Message(pp->p.csound, "mp3scal_load error:"
                          "could not open %s \n", pp->name->data);
  }
  return NULL;
}

static int loader_init(CSOUND *csound, LOADER *pp){
  MP3SCAL2 *p = &(pp->p);
  p->csound = csound;
  p->init = 0;
  p->ti = 0;
  // csound->Message(csound, "loader thread start\n");
  if(p->playing == 0)
   pthread_create(&(pp->p.t), NULL, loader_thread, pp);
  else return csound->InitError(csound, "cannot load: player still active\n");
  pp->res->data = (char *) p;
  pp->res->size = sizeof(MP3SCAL2);
  return OK;
}

typedef struct _check {
  OPDS h;
  MYFLT *res;
  STRINGDAT *pp;
  MP3SCAL2 *p;
} CHECK;


static int check_init(CSOUND *csound, CHECK *p){
  if(p->pp->data != NULL &&
     p->pp->size != sizeof(MP3SCAL2)) {
    p->p = (MP3SCAL2 *) p->pp->data;
  }
  else return csound->InitError(csound, "invalid handle \n");
  return OK;
}

static int check_play(CSOUND *csound, CHECK *p){
  *p->res = p->p->init;
  return OK;
}



typedef struct _player {
  OPDS h;
  MYFLT *out1, *out2, *kstamp, *ilen;
  STRINGDAT *pp;
  MYFLT *time, *kpitch, *kamp, *klock, *kinterp, *async;
  MP3SCAL2 *p;
} PLAYER;

int mp3dec_cleanup(CSOUND *csound, PLAYER *p)
{
  if (p->p->mpa != NULL)
    mp3dec_uninit(p->p->mpa);
  return OK;
}

static int player_init(CSOUND *csound, PLAYER *p){
  if(p->pp->data != NULL &&
     p->pp->size != sizeof(MP3SCAL2)) {
    p->p = (MP3SCAL2 *) p->pp->data;
  }
  else return csound->InitError(csound, "invalid handle \n");
  *p->ilen = p->p->ilen;
  p->p->async = *p->async;

  //if(p->p->initDone == -1)
    csound->RegisterDeinitCallback(csound, p,
    (int (*)(CSOUND*, void*)) mp3dec_cleanup);

  return OK;
}

static int player_play(CSOUND *csound, PLAYER *pp)
{
  MP3SCAL2 *p = pp->p;
  uint32_t offset = pp->h.insdshead->ksmps_offset;
  uint32_t early  = pp->h.insdshead->ksmps_no_end;
  int decim = p->decim;
  MYFLT pitch = *pp->kpitch*p->resamp, time = *pp->time*p->resamp, lock = *pp->klock;
  double amp = *pp->kamp*csound->Get0dBFS(csound)*(8./decim)/3.;
  int interp = *pp->kinterp;
  MYFLT *out;
  MYFLT *tab, **table,frac;
  FUNC *ft;
  int N = p->N, hsize = p->hsize, cnt = p->cnt;
  int  nsmps = csound->GetKsmps(csound), n;
  int size = p->fdata[0].size/sizeof(MYFLT), post, i, j;
  double pos, spos = p->pos;
  MYFLT *fwin, *bwin;
  MYFLT in, *nwin, *prev;
  MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
  MYFLT powrat;
  MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
  int *framecnt, curframe = p->curframe;
  double tstamp = p->tstamp, incrt = p->incr;
  int curbuf = p->curbuf;
  AUXCH *mfwin = p->fwin,
    *mbwin = p->bwin,
    *mprev = p->prev,
    *moutframe = p->outframe,
    *mframecount = p->framecount;
  MYFLT hsizepitch = hsize*pitch;
  int nbytes =  p->N*sizeof(MYFLT);

  p->playing = 1;

  if(time < 0) time = 0.0;
  table = p->tab;

  if(!p->init){
    for (j=0; j < MP3_CHNS; j++) {
      out = j == 0 ? pp->out1 : pp->out2;
      memset(out, '\0', nsmps*sizeof(MYFLT));
    }
    p->ti++;
    *pp->kstamp = 0;
    return OK;
  } else *pp->ilen = p->ilen;

  if (UNLIKELY(early)) {
    nsmps -= early;
    for (j=0; j < MP3_CHNS; j++) {
      out = j == 0 ? pp->out1 : pp->out2;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
  }
  if (UNLIKELY(offset)) {
    for (j=0; j < MP3_CHNS; j++) {
      out = j == 0 ? pp->out1 : pp->out2;
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
      if (spos > size/2+hsize && p->curbuf == 0 && p->filling == 0) {
        fillbuf2(csound,p,size/2);
        p->filling = 1;
      } else if (spos < size/2+hsize && p->curbuf == 1 && p->filling == 1){
        fillbuf2(csound,p,size/2);
        p->filling = 0;
      }

      for (j = 0; j < MP3_CHNS; j++) {
         bwin = (MYFLT *) mbwin[j].auxp;
         fwin = (MYFLT *) mfwin[j].auxp;
         prev = (MYFLT *) mprev[j].auxp;
         framecnt  = (int *) mframecount[j].auxp;
         outframe= (MYFLT *) moutframe[j].auxp;
         tab = table[j];
        if(pitch != 1) {
          pos = spos;
          for (i=0; i < N; i++) {
            post = (int) pos;
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
          post = (int) spos;
          int end = post+N;
          if(end <= size)
            memcpy(fwin,&tab[post],nbytes);
          else {
            int endbytes;
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
            int endbytes;
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


        csound->RealFFT(csound, bwin, N);
        bwin[N] = bwin[1];
        bwin[N+1] = FL(0.0);
        csound->RealFFT(csound, fwin, N);
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
        csound->InverseRealFFT(csound, fwin, N);
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
      out = j == 0 ? pp->out1 : pp->out2;
      framecnt  = (int *) p->framecount[j].auxp;
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
  *pp->kstamp = (p->skip + p->tstamp/csound->GetSr(csound))/p->resamp;
  p->incr = incrt;
  p->playing = 0;
  return OK;

}
//#endif

#define S(x)    sizeof(x)

static OENTRY mp3in_localops[] = {
  {"mp3in",  S(MP3IN),  0, 5, "mm", "Soooo", (SUBR) mp3ininit_S, NULL, (SUBR) mp3in},
  {"mp3in.i",  S(MP3IN),  0, 5, "mm", "ioooo", (SUBR) mp3ininit, NULL, (SUBR) mp3in},
  {"mp3len", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3len.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
  {"mp3sr", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3sr.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
  {"mp3bitrate", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3bitrate.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
  {"mp3nchnls", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3nchnls.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
  {"mp3scal", sizeof(DATASPACE), 0, 5, "aak", "SkkkoooPP",
   (SUBR)sinit3, NULL,(SUBR)sprocess3 },
    {"mp3scal_load", sizeof(LOADER), 0, 1, "i", "Sooo",
   (SUBR)loader_init, NULL,NULL },
    {"mp3scal_play", sizeof(PLAYER), 0, 5, "aaki", "ikkkPPo",
   (SUBR)player_init, NULL,(SUBR)player_play},
    {"mp3scal_check", sizeof(CHECK), 0, 5, "k", "i",
   (SUBR)check_init, NULL,(SUBR)check_play}
};

LINKAGE_BUILTIN(mp3in_localops)
