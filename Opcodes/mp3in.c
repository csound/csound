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
    int32_t fd;
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

    p->mpa = mpa = mp3dec_init();
    if (UNLIKELY(!mpa)) {
      return csound->InitError(csound, Str("Not enough memory\n"));
    }

    if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
      mp3dec_uninit(mpa);
      p->mpa = NULL;
      return csound->InitError(csound, "%s", mp3dec_error(r));
    }


    /* FIXME: name can overflow with very long string -- truncates safely */
    if (stringname==0){
      if (csound->ISSTRCOD(*p->iFileCode))
        strNcpy(name,get_arg_string(csound, *p->iFileCode), 1023);
      else csound->strarg2name(csound, name, p->iFileCode, "soundin.",0);
    }
    else strNcpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);

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
    /* if (UNLIKELY(csound->oparms_.msglevel & WARNMSG)) */ {
      char temp[80];            /* Could be as low as 20 */
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
    if (p->initDone == 0)
      csound->RegisterDeinitCallback(csound, p,
                                     (int32_t (*)(CSOUND*, void*)) mp3in_cleanup);
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
    int32_t pos     = p->pos;
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
        while (r != MP3DEC_RETCODE_OK || 2*pos >=  (int32_t)p->bufused) {
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

int32_t mp3len_(CSOUND *csound, MP3LEN *p, int32_t stringname)
{
    char     name[1024];
    int32_t  fd;
    mp3dec_t mpa           = NULL;
    mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_STEREO,
                               MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                               MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                               0.0 };
    mpadec_info_t mpainfo;
    int32_t  r;

    /* open file */
    mpa = mp3dec_init();
    if (UNLIKELY(!mpa)) {
      return csound->InitError(csound, "%s", Str("Not enough memory\n"));
    }
    if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
      mp3dec_uninit(mpa);
      return csound->InitError(csound, "%s", mp3dec_error(r));
    }
    /* FIXME: name can overflow with very long string -- safely truncated */
    if (stringname==0){
      if (csound->ISSTRCOD(*p->iFileCode))
        strNcpy(name,get_arg_string(csound, *p->iFileCode), 1023);
      else csound->strarg2name(csound, name, p->iFileCode, "soundin.",0);
    }
    else strNcpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);
    if (UNLIKELY(csound->FileOpen2(csound, &fd, CSFILE_FD_R,
                                   name, "rb", "SFDIR;SSDIR",
                                   CSFTYPE_OTHER_BINARY, 0) == NULL)) {
      mp3dec_uninit(mpa);
      return
        csound->InitError(csound, Str("mp3in: %s: failed to open file"), name);
    }
    if (UNLIKELY((r = mp3dec_init_file(mpa, fd, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
      mp3dec_uninit(mpa);
      return csound->InitError(csound, "%s", mp3dec_error(r));
    }
    if (UNLIKELY((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
                 MP3DEC_RETCODE_OK)) {
      close(fd);
      mp3dec_uninit(mpa);
      return csound->InitError(csound, "%s", mp3dec_error(r));
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
    p->fwdsetup = csound->RealFFT2Setup(csound,N,FFT_FWD);
    p->invsetup = csound->RealFFT2Setup(csound,N,FFT_INV);
    return OK;
}

static int32_t sinit3_(CSOUND *csound, DATASPACE *p)
{
    uint32_t size;
    char *name;
    // open file
    int32_t  fd;
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
    p->mpa = mpa = mp3dec_init();
    if (UNLIKELY(!mpa)) {
      return csound->InitError(csound, Str("Not enough memory\n"));
    }
    if (UNLIKELY((r = mp3dec_configure(mpa, &config)) != MP3DEC_RETCODE_OK)) {
      mp3dec_uninit(mpa);
      p->mpa = NULL;
      return csound->InitError(csound, "%s", mp3dec_error(r));
    }
    if (UNLIKELY(csound->FileOpen2(csound, &fd, CSFILE_FD_R,
                                   name, "rb", "SFDIR;SSDIR",
                                   CSFTYPE_OTHER_BINARY, 0) == NULL)) {
      mp3dec_uninit(mpa);
      return
        csound->InitError(csound, Str("mp3scale: %s: failed to open file"), name);
    }// else
    // csound->Message(csound, Str("mp3scale: open %s\n"), name);
    if (UNLIKELY((r = mp3dec_init_file(mpa, fd, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
      mp3dec_uninit(mpa);
      return csound->InitError(csound, "%s", mp3dec_error(r));
    } // else
    // csound->Message(csound, Str("mp3scale: init %s\n"), name);

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
    if(p->initDone == -1)
      csound->RegisterDeinitCallback(csound, p,
                                     (int32_t (*)(CSOUND*, void*))mp3scale_cleanup);
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
    int32_t size = p->fdata[0].size/sizeof(MYFLT), post, i, j;
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


          csound->RealFFT2(csound, p->fwdsetup, bwin);
          bwin[N] = bwin[1];
          bwin[N+1] = FL(0.0);
          csound->RealFFT2(csound, p->fwdsetup, fwin);
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
          csound->RealFFT2(csound, p->invsetup, fwin);
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
    *p->kstamp = (*p->skip + p->tstamp/csound->GetSr(csound))/p->resamp;
    p->incr = incrt;
    return OK;

}

#ifdef ANDROID

typedef struct _mp3scal2_ {
  int32_t cnt, hsize, curframe, N, decim,tscale;
  double pos;
  MYFLT accum;
  AUXCH outframe[MP3_CHNS], win, bwin[MP3_CHNS], fwin[MP3_CHNS],
    nwin[MP3_CHNS], prev[MP3_CHNS], framecount[MP3_CHNS], fdata[MP3_CHNS], buffer;
  MYFLT *indataL[8], *indataR[8];
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
  pthread_t t,t1;
  int32_t ti;
  MYFLT ilen;
  MYFLT skip;
  char playing;
  int32_t nsmps;
  char filling;
  int32_t async;
  int32_t error;
  MYFLT orsr;
  char lock;
  uint64_t end;
} MP3SCAL2;

typedef struct _loader {
  OPDS h;
  STRINGDAT *res;
  STRINGDAT *name;
  MYFLT *skip, *iN, *idecim, *bfs;
  MP3SCAL2 p;
} LOADER;


void *buffiller(void *pp){
    MP3SCAL2 *p = (MP3SCAL2 *) pp;
    int32_t nsmps = p->nsmps;
    short *buffer= (short *) p->buffer.auxp;
    MYFLT *data[2];
    p->lock = 1;
    if(p->mpa != NULL) {
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
          end = (p->bufused/sizeof(short))/MP3_CHNS;
          for(i=j=0; i < end; i++, j+=2){
            data[0][i] = buffer[j]/32768.0;
            data[1][i] = buffer[j+1]/32768.0;
          }
        }
      }
      p->curbuf = (p->curbuf+1)%8;
    }
    p->lock = 0;
    return NULL;
}

void fillbuf2(CSOUND *csound, MP3SCAL2 *p, int32_t nsmps){
    p->nsmps = nsmps;
    if(p->async) {
      int32_t policy;
      struct sched_param param;
      pthread_create(&(p->t1), NULL, buffiller, p);
      pthread_getschedparam(p->t1, &policy,
                            &param);
      param.sched_priority = 1;
      //policy = SCHED_OTHER;
      if(pthread_setschedparam(p->t1, policy,
                               &param) != 0)
        csound->Message(csound, Str("could not set priority\n"));
    }
    else
      buffiller((void *) p);
}


static int32_t meminit(CSOUND *csound, LOADER *pp)
{

    MP3SCAL2 *p = &(pp->p);
    int32_t N =  *pp->iN, ui;
    uint32_t i;
    uint32_t size;
    int32_t decim = *pp->idecim;
    p->N = N;
    p->error = 0;
    if (N) {
      for (i=0; N; i++) {
        N >>= 1;
      }
      N = (int32_t) pow(2.0, i-1);
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
    return OK;
}

int32_t mp3dec_cleanup(CSOUND *csound, LOADER *p)
{
    while(p->p.lock)
      usleep(1000);
    if (p->p.mpa != NULL)
      mp3dec_uninit(p->p.mpa);
    p->p.mpa = NULL;
    return OK;
}

void decode_seek(CSOUND *csound, mp3dec_t mpa, int32_t skip){
    unsigned char buffer[1152*4];
    mp3dec_seek(mpa, 0, MP3DEC_SEEK_SAMPLES);
    while (skip > 0) {
      mp3dec_decode(mpa,buffer, 1152*4,NULL);
      skip -= 1152;
    }
}


static int32_t filinit(CSOUND *csound, LOADER *pp)
{
    MP3SCAL2 *p = &(pp->p);
    uint32_t size;
    char *name;
    // open file
    int32_t  fd;
    int32_t  r;
    mp3dec_t mpa           = NULL;
    mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_STEREO,
                               MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                               MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                               0.0 };
    mpadec_info_t mpainfo;
    int32_t buffsize = 32768;

    p->init = 0;
    if(*pp->bfs) buffsize = *pp->bfs*8;

    name = pp->name->data;
    if(p->mpa != NULL) mp3dec_uninit(p->mpa);
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
    if (UNLIKELY((r = mp3dec_init_file(mpa, fd, 0, TRUE)) != MP3DEC_RETCODE_OK)) {
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
    size = buffsize*sizeof(MYFLT);
    if (p->fdata[0].auxp == NULL || p->fdata[0].size < size)
      csound->AuxAlloc(csound, size, &p->fdata[0]);
    p->indataL[0] = p->fdata[0].auxp;
    p->indataL[1] = p->fdata[0].auxp + size/8;
    p->indataL[2] = p->fdata[0].auxp + size/4;
    p->indataL[3] = p->fdata[0].auxp + 3*size/8;
    p->indataL[4] = p->fdata[0].auxp + size/2;
    p->indataL[5] = p->fdata[0].auxp + 5*size/8;
    p->indataL[6] = p->fdata[0].auxp + 3*size/4;
    p->indataL[7] = p->fdata[0].auxp + 7*size/8;
    memset(p->indataL[7], 0, size/8);
    if (p->fdata[1].auxp == NULL || p->fdata[1].size < size)
      csound->AuxAlloc(csound, size, &p->fdata[1]);
    p->indataR[0] = p->fdata[1].auxp;
    p->indataR[1] = p->fdata[1].auxp + size/8;
    p->indataR[2] = p->fdata[1].auxp + size/4;
    p->indataR[3] = p->fdata[1].auxp + 3*size/8;
    p->indataR[4] = p->fdata[1].auxp + size/2;
    p->indataR[5] = p->fdata[1].auxp + 5*size/8;
    p->indataR[6] = p->fdata[1].auxp + 3*size/4;
    p->indataR[7] = p->fdata[1].auxp + 7*size/8;
    memset(p->indataR[7], 0, size/8);
    size =  buffsize*sizeof(short)/4;
    if (p->buffer.auxp == NULL || p->buffer.size < size)
      csound->AuxAlloc(csound, size, &p->buffer);
    /*double dtime;
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      dtime = ts.tv_sec + 1e-9*ts.tv_nsec;
    */

    int32_t skip = (int32_t)(*pp->skip*mpainfo.frequency);
    p->bufused = -1;
    /* mp3_seek operates on multiples of 1152 frames */
    int32_t frmsiz = mpainfo.decoded_frame_samples;
    if(skip > 0) {
      int32_t ftbr = skip/frmsiz;
      int32_t skips = ftbr*frmsiz;
      MYFLT fsize = (FL(125.0)*mpainfo.bitrate*frmsiz)/mpainfo.frequency;
      if(fsize - (int32_t) fsize > FL(0.0)){
        char dat[4];
        int32_t byts, pad, i;
        mp3dec_seek(mpa, 0, MP3DEC_SEEK_SAMPLES);
        for(i=0; i < ftbr; i++){
          byts = read(fd,dat,4);
          pad = dat[2] & 0x02 ? 1 : 0;
          lseek(fd,(int32_t)fsize + pad - 4, SEEK_CUR);
          //printf("skip %d pad %d\n", i, pad);
        }
      } else
        mp3dec_seek(mpa, skips, MP3DEC_SEEK_SAMPLES);
      skip = skip - skips;
    } else mp3dec_seek(mpa, 0, MP3DEC_SEEK_SAMPLES);

    /*clock_gettime(CLOCK_MONOTONIC, &ts);
      dtime = ts.tv_sec + 1e-9*ts.tv_nsec - dtime;
      csound->Message(csound, "skip time %f\n", dtime);*/

    // fill buffers
    p->orsr = mpainfo.frequency;
    p->curbuf = 0;
    p->finished = 0;
    p->nsmps = buffsize/8;
    buffiller((void *)p);
    buffiller((void *)p);
    buffiller((void *)p);
    buffiller((void *)p);
    buffiller((void *)p);
    buffiller((void *)p);
    buffiller((void *)p);
    buffiller((void *)p);

    p->pos = skip*csound->GetSr(csound)/p->orsr;
    p->tscale  = 0;
    p->accum = 0;
    p->tab[0] = (MYFLT *) p->fdata[0].auxp;
    p->tab[1] = (MYFLT *) p->fdata[1].auxp;
    p->tstamp = 0;
    p->finished = 0;
    p->skip = *pp->skip;
    p->filling = 0;
    p->init = 1;

    return OK;
}

void *loader_thread(void *p){
    LOADER *pp = (LOADER *) p;
    if(filinit(pp->p.csound,pp) != OK) {
      if(pp->p.error > 0)
        pp->p.csound->Message(pp->p.csound, Str("mp3scal_load error: %s\n"),
                              mp3dec_error(pp->p.error));
      else
        pp->p.csound->Message(pp->p.csound,
                              Str("mp3scal_load error: could not open %s\n"),
                              pp->name->data);
    }
    return NULL;
}

static int32_t loader_init(CSOUND *csound, LOADER *pp){
    MP3SCAL2 *p = &(pp->p);
    p->csound = csound;
    p->init = 0;
    p->ti = 0;
    if(p->playing == 0){
      if(pthread_create(&(pp->p.t), NULL, loader_thread, pp) != 0)
        csound->Message(csound, Str("failed to start thread\n"));
      struct sched_param param;
      int32_t policy;
      pthread_getschedparam((pp->p.t), &policy,
                            &param);
      param.sched_priority = 0;
      policy = SCHED_OTHER;
      if(pthread_setschedparam((pp->p.t), policy,
                               &param) != 0)
        csound->Message(csound, Str("could not set priority\n"));
    }
    else return csound->InitError(csound,
                                  Str("cannot load: player still active\n"));
    pp->res->data = (char *) p;
    pp->res->size = sizeof(MP3SCAL2);
    // if(p->initDone == 0)
    //csound->RegisterDeinitCallback(csound, pp,
    // (int32_t (*)(CSOUND*, void*)) mp3dec_cleanup);
    //p->initDone = 1;
    return OK;
}

typedef struct _check {
  OPDS h;
  MYFLT *res;
  STRINGDAT *pp;
  MP3SCAL2 *p;
} CHECK;


static int32_t check_init(CSOUND *csound, CHECK *p){
    if(p->pp->data != NULL &&
       p->pp->size != sizeof(MP3SCAL2)) {
      p->p = (MP3SCAL2 *) p->pp->data;
    }
    else return csound->InitError(csound, Str("invalid handle\n"));
    return OK;
}

static int32_t check_play(CSOUND *csound, CHECK *p){
    *p->res = p->p->init;
    return OK;
}

#ifdef HAVE_NEON
#include <pffft.h>
#endif

typedef struct _player {
  OPDS h;
  MYFLT *out1, *out2, *kstamp, *ilen;
  STRINGDAT *pp;
  MYFLT *time, *kpitch, *kamp, *klock, *kinterp, *async;
  MP3SCAL2 *p;
  char start_flag;
#ifdef HAVE_NEON
  PFFFT_Setup *setup;
  float *bw,*fw;
#else
  void *fwdsetup, *invsetup;
#endif
} PLAYER;

int32_t mp3dec_cleanup_player(CSOUND *csound,  PLAYER  *p)
{
    while(p->p->lock)
      usleep(1000);
    pthread_join(p->p->t1, NULL);
    if (p->p->mpa != NULL)
      mp3dec_uninit(p->p->mpa);
    p->p->mpa = NULL;
#ifdef HAVE_NEON
    pffft_destroy_setup(p->setup);
    pffft_aligned_free(p->bw);
    pffft_aligned_free(p->fw);
#endif
    return OK;
}

static int32_t player_init(CSOUND *csound, PLAYER *p){
    if(p->pp->data != NULL &&
       p->pp->size != sizeof(MP3SCAL2)) {
      p->p = (MP3SCAL2 *) p->pp->data;
    }
    else return csound->InitError(csound, Str("invalid handle\n"));
    *p->ilen = p->p->ilen;
    p->p->async = *p->async;
    if(p->p->initDone == 0)
      csound->RegisterDeinitCallback(csound, p,
                                 (int32_t (*)(CSOUND*,void*))mp3dec_cleanup_player);
    p->p->initDone = 1;

    int32_t policy;
    struct sched_param param;
    pthread_getschedparam(pthread_self(), &policy,
                          &param);
    /*if(policy == SCHED_OTHER)
      csound->Message(csound, "POLICY: SCHED_OTHER");
      struct sched_param param;
      pthread_getschedparam(pthread_self(), &policy,
      &param);
      if(policy == SCHED_OTHER)
      csound->Message(csound, "POLICY: SCHED_OTHER");
      else if(policy == SCHED_FIFO)
      csound->Message(csound, "POLICY: SCHED_FIFO, %d", param.sched_priority);*/

#ifdef HAVE_NEON
    while(!p->p->N) usleep(1000);
    p->setup = pffft_new_setup(p->p->N,PFFFT_REAL);
    p->bw = pffft_aligned_malloc(p->p->N*sizeof(float));
    p->fw = pffft_aligned_malloc(p->p->N*sizeof(float));
#else
    while(!p->p->N) usleep(1000);
    p->fwdsetup = csound->RealFFT2Setup(csound,p->p->N,FFT_FWD);
    p->invsetup = csound->RealFFT2Setup(csound,p->p->N,FFT_INV);
#endif
    p->start_flag = 1;
    return OK;
}




#include <stdbool.h>
#ifdef HAVE_NEON
#include <arm_neon.h>
static
inline
void
cmplx_multiply_scal(MYFLT *ans_r, MYFLT *ans_i,
                    MYFLT  *in1, MYFLT  *in2,
                    MYFLT div, bool neg){
    float32x4_t op1, op2, ans;
    float32x2_t tmp1, tmp2;
    float32_t vans[4];
    tmp1 = vld1_f32(in1);
    tmp2 = vld1_f32(in2);
    op1 = vcombine_f32(tmp1,tmp1);
    op2 = vcombine_f32(tmp2,vrev64_f32(tmp2));
    /* ac - bd + i(ad + bc) */
    ans = vmulq_n_f32(op2,div);
    op2 = vmulq_f32(op1,ans);
    vst1q_f32(vans, op2);
    *ans_r =  vans[0] - (neg ? -vans[1] : vans[1]);
    *ans_i =  vans[2] + (neg ? -vans[3] : vans[3]);
}
static
inline
MYFLT
invsqrt(MYFLT x)
{
    int64_t i;
    float x2 = x*0.5f;
    i = * (int64_t *) &x;
    i = 0x5f3759df - (i >> 1);
    x = *(float *) &i;
    x = x*(1.5f - (x2*x*x));
    return x;
}

static
inline
MYFLT
inv_mag_(MYFLT *a){
    float32x2_t ans, op;
    float32_t vans[2];
    op = vld1_f32(a);
    ans = vmul_f32(op,op);
    vst1_f32(vans, ans);
    return invsqrt(vans[0]+vans[1]);
}

#else

static
inline
void
cmplx_multiply_scal(MYFLT *ans_r, MYFLT *ans_i,
                    MYFLT* in1, MYFLT* in2,
                    MYFLT div, bool neg){

    MYFLT r = in2[0] * div, i = in2[1] * div;
    *ans_r = in1[0]*r - (neg ? -in1[1]*i : in1[1]*i);
    *ans_i = in1[0]*i + (neg ? -in1[1]*r : in1[1]*r);
}

#endif

static
inline
MYFLT
inv_mag(MYFLT *a){
    return FL(1.0)/(hypot(a[0], a[1])+1.0e-15);
    //return invsqrt(a[0]*a[0]+a[1]*a[1]);
}
static
inline
MYFLT
inv_mag2(MYFLT *a){  // FIXME: use HYPOT
    a[0] += 1.0e-15;
    return FL(1.0)/(hypot(a[0],a[1]));
    //return invsqrt(a[0]*a[0]+a[1]*a[1]);
}



#define FTOINT(x)  ((int32_t)x)

static int32_t player_play(CSOUND *csound, PLAYER *pp)
{
    MP3SCAL2 *p = pp->p;
    uint32_t offset = pp->h.insdshead->ksmps_offset;
    uint32_t early  = pp->h.insdshead->ksmps_no_end;
    int32_t decim = p->decim;
    MYFLT pitch = *pp->kpitch*p->resamp, time = *pp->time*p->resamp,
      lock = *pp->klock;
    double amp = *pp->kamp*csound->Get0dBFS(csound)*(8./decim)/3.;
    int32_t interp = *pp->kinterp;
#ifdef __clang__
    MYFLT *restrict out;
    MYFLT *restrict fwin;
    MYFLT *restrict bwin;
    MYFLT *restrict prev;
    MYFLT *restrict win = (MYFLT *) p->win.auxp, *restrict outframe;
    int32_t *restrict framecnt, curframe = p->curframe;
    MYFLT *restrict tab, **table,frac;
    MYFLT *restrict phs;
#else
    MYFLT *out;
    MYFLT *fwin;
    MYFLT *bwin;
    MYFLT *prev;
    MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
    int32_t *framecnt, curframe = p->curframe;
    MYFLT *tab, **table,frac;
    MYFLT *phs;
#endif
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt;
    int32_t  nsmps = csound->GetKsmps(csound), n;
    int32_t size = p->fdata[0].size/sizeof(MYFLT), post, i, j;
    double pos, spos = p->pos;
    MYFLT in;
    MYFLT div;
    double tstamp = p->tstamp, incrt = p->incr;
    AUXCH *mfwin = p->fwin,
      *mbwin = p->bwin,
      *mprev = p->prev,
      *moutframe = p->outframe,
      *mframecount = p->framecount;
    MYFLT hsizepitch = hsize*pitch;
    int32_t nbytes =  p->N*sizeof(MYFLT);
    int32_t start_flag = pp->start_flag;
#ifdef HAVE_NEON
    float *restrict bw =  pp->bw, *restrict fw = pp->fw;
#endif
    p->playing = 1;

    if(time < 0) time = 0.0;
    table = p->tab;

    if(!p->init){
      for (j=0; j < MP3_CHNS; j++) {
        out = j == 0 ? pp->out1 : pp->out2;
        memset(out, '\0', nsmps*sizeof(MYFLT));
      }
      csound->Message(csound, Str("not init\n"));
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
        while(spos >= size) {
          spos -= size;
        }
        while(spos < 0){
          spos += size;
        }

        if (spos > size/8+hsize && p->curbuf == 0 && p->filling == 0) {
          fillbuf2(csound,p,size/8);
          p->filling = 1;
        } else if (spos > size/4+hsize && p->curbuf == 1 && p->filling == 1){
          fillbuf2(csound,p,size/8);
          p->filling = 2;
        }
        else if (spos > 3*size/8+hsize && p->curbuf == 2 && p->filling == 2){
          fillbuf2(csound,p,size/8);
          p->filling = 3;
        }
        else if (spos > size/2+hsize && p->curbuf == 3 && p->filling == 3){
          fillbuf2(csound,p,size/8);
          p->filling = 4;
        }
        else if (spos > 5*size/8+hsize && p->curbuf == 4 && p->filling == 4){
          fillbuf2(csound,p,size/8);
          p->filling = 5;
        }
        else if (spos > 3*size/4+hsize && p->curbuf == 5 && p->filling == 5){
          fillbuf2(csound,p,size/8);
          p->filling = 6;
        }
        else if (spos > 7*size/8+hsize && p->curbuf == 6 && p->filling == 6){
          fillbuf2(csound,p,size/8);
          p->filling = 7;
        }
        else if (spos < size/8+hsize && p->curbuf == 7 && p->filling == 7){
          fillbuf2(csound,p,size/8);
          p->filling = 0;
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

#ifndef HAVE_NEON
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
#ifdef HAVE_NEON
              fw[i] = in * win[i];
#else
              fwin[i] = in * win[i];
#endif

              post -= hsizepitch;
              if(post < 0) post += size;
              if(post >= size) post -= size;
              if(post+1 <  size && interp){
                in = tab[post] + frac*(tab[post+1] - tab[post]);
              }
              else
                in = tab[post];
#ifdef HAVE_NEON
              bw[i] = in * win[i];
#else
              bwin[i] = in * win[i];
#endif
              pos += pitch;
            }
#else
            float32x4_t bsm1,bsm2,ans;
            float tmpf[5],fracv[4];
            float tmpos;
            int32_t   tmposi;
            for (i=0; i < N; i+=4) {
              tmpos = pos;
              if(tmpos < 0) tmpos += size;
              if(tmpos >= size) tmpos -= size;
              tmposi = FTOINT(tmpos);
              tmpf[0] = tab[tmposi];
              fracv[0] = tmpos - tmposi;
              tmpos += pitch;
              tmposi = FTOINT(tmpos);
              if(tmposi < size){
                tmpf[1] = tab[tmposi];
                fracv[1] = tmpos -tmposi;
                tmpos += pitch;
                tmposi = FTOINT(tmpos);
                if(tmposi < size){
                  tmpf[2] = tab[tmposi];
                  fracv[2] = tmpos - tmposi;
                  tmpos += pitch;
                  tmposi = FTOINT(tmpos);
                  if(tmposi < size){
                    tmpf[3] = tab[tmposi];
                    fracv[3] = tmpos - tmposi;
                    tmpos += pitch;
                    tmposi = FTOINT(tmpos);
                    if(tmposi < size)
                      tmpf[4] = tab[tmposi];
                    else {
                      tmpos -= size;
                      tmposi = FTOINT(tmpos);
                      tmpf[4] = tab[tmposi];
                    }
                  }
                  else {
                    tmpos -= size;
                    tmposi = FTOINT(tmpos);
                    tmpf[3] = tab[tmposi];
                    fracv[3] = tmpos -tmposi;
                    tmpos += pitch;
                    tmposi = FTOINT(tmpos);
                    tmpf[4] = tab[tmposi];
                  }
                }
                else {
                  tmpos -= size;
                  tmposi = FTOINT(tmpos);
                  tmpf[2] = tab[tmposi];
                  fracv[2] = tmpos - tmposi;
                  tmpos += pitch;
                  tmposi = FTOINT(tmpos);
                  tmpf[3] = tab[tmposi];
                  fracv[3] = tmpos - tmposi;
                  tmpos += pitch;
                  tmposi = FTOINT(tmpos);
                  tmpf[4] = tab[tmposi];
                }
              }
              else {
                tmpos -= size;
                tmposi = FTOINT(tmpos);
                tmpf[1] = tab[tmposi];
                fracv[1] = tmpos -tmposi;
                tmpos += pitch;
                tmposi = FTOINT(tmpos);
                tmpf[2] = tab[tmposi];
                fracv[2] = tmpos - tmposi;
                tmpos += pitch;
                tmposi = FTOINT(tmpos);
                tmpf[3] = tab[tmposi];
                fracv[3] = tmpos - tmposi;
                tmpos += pitch;
                tmposi = FTOINT(tmpos);
                tmpf[4] = tab[tmposi];
              }

              bsm1 = vld1q_f32(&tmpf[0]);
              bsm2 = vld1q_f32(&tmpf[1]);
              ans = vsubq_f32(bsm2,bsm1);
              bsm2 = vld1q_f32(fracv);
              bsm2 = vmulq_f32(ans,bsm2);
              ans = vaddq_f32(bsm1,bsm2);
              bsm1 = vld1q_f32(&win[i]);
              bsm2 = vmulq_f32(ans,bsm1);
              vst1q_f32(&fw[i],bsm2);

              tmpos = pos - hsize*pitch;
              if(tmpos < 0) tmpos += size;
              if(tmpos >= size) tmpos -= size;
              if(tmpos < 0) tmpos += size;
              if(tmpos >= size) tmpos -= size;
              tmposi = FTOINT(tmpos);
              tmpf[0] = tab[tmposi];
              fracv[0] = tmpos - tmposi;
              tmpos += pitch;
              tmposi = FTOINT(tmpos);
              if(tmposi < size){
                tmpf[1] = tab[tmposi];
                fracv[1] = tmpos -tmposi;
                tmpos += pitch;
                tmposi = FTOINT(tmpos);
                if(tmposi < size){
                  tmpf[2] = tab[tmposi];
                  fracv[2] = tmpos - tmposi;
                  tmpos += pitch;
                  tmposi = FTOINT(tmpos);
                  if(tmposi < size){
                    tmpf[3] = tab[tmposi];
                    fracv[3] = tmpos - tmposi;
                    tmpos += pitch;
                    tmposi = FTOINT(tmpos);
                    if(tmposi < size)
                      tmpf[4] = tab[tmposi];
                    else {
                      tmpos -= size;
                      tmposi = FTOINT(tmpos);
                      tmpf[4] = tab[tmposi];
                    }
                  }
                  else {
                    tmpos -= size;
                    tmposi = FTOINT(tmpos);
                    tmpf[3] = tab[tmposi];
                    fracv[3] = tmpos -tmposi;
                    tmpos += pitch;
                    tmposi = FTOINT(tmpos);
                    tmpf[4] = tab[tmposi];
                  }
                }
                else {
                  tmpos -= size;
                  tmposi = FTOINT(tmpos);
                  tmpf[2] = tab[tmposi];
                  fracv[2] = tmpos - tmposi;
                  tmpos += pitch;
                  tmposi = FTOINT(tmpos);
                  tmpf[3] = tab[tmposi];
                  fracv[3] = tmpos - tmposi;
                  tmpos += pitch;
                  tmposi = FTOINT(tmpos);
                  tmpf[4] = tab[tmposi];
                }
              }
              else {
                tmpos -= size;
                tmposi = FTOINT(tmpos);
                tmpf[1] = tab[tmposi];
                fracv[1] = tmpos -tmposi;
                tmpos += pitch;
                tmposi = FTOINT(tmpos);
                tmpf[2] = tab[tmposi];
                fracv[2] = tmpos - tmposi;
                tmpos += pitch;
                tmposi = FTOINT(tmpos);
                tmpf[3] = tab[tmposi];
                fracv[3] = tmpos - tmposi;
                tmpos += pitch;
                tmposi = FTOINT(tmpos);
                tmpf[4] = tab[tmposi];
              }

              bsm1 = vld1q_f32(&tmpf[0]);
              bsm2 = vld1q_f32(&tmpf[1]);
              ans = vsubq_f32(bsm2,bsm1);
              bsm2 = vld1q_f32(fracv);
              bsm2 = vmulq_f32(ans,bsm2);
              ans = vaddq_f32(bsm1,bsm2);
              bsm1 = vld1q_f32(&win[i]);
              bsm2 = vmulq_f32(ans,bsm1);
              vst1q_f32(&bw[i],bsm2);
              pos += pitch*4;
            }
#endif
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

#ifdef HAVE_NEON
            for(i=0; i < N; i++) {
              bw[i] = bwin[i]*win[i];
              fw[i] = fwin[i]*win[i];
            }
#else
            for(i=0; i < N; i++) {
              bwin[i] *= win[i];
              fwin[i] *= win[i];
            }
#endif
          }

#ifdef HAVE_NEON
          pffft_transform_ordered(pp->setup,bw,bw,NULL,PFFFT_FORWARD);
          pffft_transform_ordered(pp->setup,fw,fw,NULL,PFFFT_FORWARD);
          memcpy(bwin,bw,N*sizeof(float));
          memcpy(fwin,fw,N*sizeof(float));
          bwin[N] = bw[1];
          fwin[N] = fw[1];
#else
          csound->RealFFT2(csound, pp->fwdsetup, bwin);
          csound->RealFFT2(csound, pp->fwdsetup, fwin);
#endif
          bwin[N] = bwin[1];
          bwin[N+1] = FL(0.0);
          fwin[N] = fwin[1];
          fwin[N+1] = FL(0.0);

          if(start_flag){
            memcpy(prev, bwin, sizeof(MYFLT)*(N+2));
            pp->start_flag = 0;
          }

          for (i=0; i < N; i+=2) {
            div =  inv_mag(&prev[i]);
            cmplx_multiply_scal(&prev[i],&prev[i+1],
                                &bwin[i],&prev[i],
                                div, true);
          }

          if (lock) {
            phs = prev;
            for(i = 2; i < N; i++)
              bwin[i] = phs[i];
            for(i = 2; i < N; i++)
              bwin[i] += phs[i-2];
            for(i = 2; i < N; i++)
              bwin[i] += phs[i+2];
            bwin[0] = prev[i] + prev[i-2];
            bwin[N] = prev[i] + prev[i+2];
          }
          else memcpy(bwin,prev,sizeof(MYFLT)*(N+2));

          for (i=0; i < N; i+=2) {
            div =  inv_mag2(&bwin[i]);
            cmplx_multiply_scal(&prev[i],&prev[i+1],
                                &fwin[i], &bwin[i],
                                div, false);
          }
#ifdef HAVE_NEON
          for(i=0;i<N;i++)
            fw[i] = prev[i]/N;
          fw[1] = prev[N]/N;
          pffft_transform_ordered(pp->setup,fw,fw,NULL,PFFFT_BACKWARD);
#else


          for(i=0; i < N+2; i++)
            fwin[i] = prev[i];
          fwin[1] = fwin[N];
          csound->RealFFT2(csound, pp->invsetup, fwin);
#endif
          framecnt[curframe] = curframe*N;
          for (i=0;i<N;i++)
#ifdef HAVE_NEON
            outframe[framecnt[curframe]+i] = win[i]*fw[i];
#else
          outframe[framecnt[curframe]+i] = win[i]*fwin[i];
#endif
        }
        cnt=0;
        curframe++;
        if (curframe == decim) curframe = 0;

        /* increment position according to timescale */
        spos += hsize*time;
        incrt =  time*nsmps;

      }

      /* we only output as many channels as we have outs for */
      for (j=0; j < 2; j++) {
        out = j == 0 ? pp->out1 : pp->out2;
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
    *pp->kstamp = (p->skip + p->tstamp/p->orsr);
    p->tstamp = tstamp + incrt;
    p->incr = incrt;
    p->playing = 0;
    return OK;

}
#endif

#ifdef ONE_FINE_DAY
static int32_t meminit2(CSOUND *csound, LOADER *pp)
{

    MP3SCAL2 *p = &(pp->p);
    int32_t N =  *pp->iN, ui;
    uint32_t i;
    uint32_t size;
    int32_t decim = *pp->idecim;
    p->N = N;
    p->error = 0;
    if (N) {
      for (i=0; N; i++) {
        N >>= 1;
      }
      N = (int32_t) pow(2.0, i-1);
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
    return OK;
}

static int32_t filinit2(CSOUND *csound, LOADER *pp)
{
    MP3SCAL2 *p = &(pp->p);
    uint32_t size;
    char *name;
    // open file
    int32_t fd;
    int32_t r;
    mp3dec_t mpa           = NULL;
    mpadec_config_t config = { MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_STEREO,
                               MPADEC_CONFIG_16BIT, MPADEC_CONFIG_LITTLE_ENDIAN,
                               MPADEC_CONFIG_REPLAYGAIN_NONE, TRUE, TRUE, TRUE,
                               0.0 };
    mpadec_info_t mpainfo;
    int32_t buffsize = 32768;
    if(*pp->bfs) buffsize = *pp->bfs*8;

    name = pp->name->data;
    if(p->mpa != NULL) mp3dec_uninit(p->mpa);
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
    meminit2(csound, pp);

    int32_t extrasmps = p->N+p->hsize+528;
    size = (mpainfo.duration*mpainfo.frequency + extrasmps)*sizeof(MYFLT);
    if (p->fdata[0].auxp == NULL || p->fdata[0].size < size)
      csound->AuxAlloc(csound, size, &p->fdata[0]);
    if (p->fdata[1].auxp == NULL || p->fdata[1].size < size)
      csound->AuxAlloc(csound, size, &p->fdata[1]);

    int32_t frmsiz = mpainfo.decoded_frame_samples;
    int32_t skip = (int32_t)(*pp->skip*mpainfo.frequency);
    p->bufused = -1;
    p->orsr = mpainfo.frequency;
    p->curbuf = 0;
    p->nsmps = buffsize/8;
    p->pos = p->hsize + skip - 528;
    p->tscale  = 0;
    p->accum = 0;
    p->tab[0] = (MYFLT *) p->fdata[0].auxp;
    p->tab[1] = (MYFLT *) p->fdata[1].auxp;
    p->tstamp = 0;
    p->finished = 0;
    p->init = 0;
    p->skip = *pp->skip;
    p->filling = 7;

    // fill buffers
    mp3dec_seek(mpa,0, MP3DEC_SEEK_SAMPLES);
    int32_t end = mpainfo.duration*mpainfo.frequency;
    short decbuffer[frmsiz*sizeof(short)*MP3_CHNS];
    p->end = end;
    MYFLT *left = (MYFLT *)  p->fdata[0].auxp;
    MYFLT *right = (MYFLT *) p->fdata[1].auxp;
    left += p->hsize;
    right += p->hsize;
    int32_t i,j,k;
    uint32_t bufused = 0;
    for(i=0;i<end;i+=frmsiz){
      mp3dec_decode(mpa, (unsigned char *)decbuffer,
                    frmsiz*sizeof(short)*MP3_CHNS,&bufused);
      //int32_t ss = (bufused/sizeof(short))/MP3_CHNS;
      for(k=j=0;k<frmsiz;j+=2,k++){
        left[i+k] = decbuffer[j]/32768.;
        right[i+k] = decbuffer[j+1]/32768.;
      }
      if(i > p->pos) {
        p->init = 1;
      }
    }
    return OK;
}

void *loader_thread2(void *p){
    LOADER *pp = (LOADER *) p;
    if(filinit2(pp->p.csound,pp) != OK) {
      if(pp->p.error > 0)
        pp->p.csound->Message(pp->p.csound, Str("mp3scal_load error: %s\n"),
                              mp3dec_error(pp->p.error));
      else
        pp->p.csound->Message(pp->p.csound,
                              Str("mp3scal_load error: could not open %s\n"),
                              pp->name->data);
    }
    return NULL;
}


static int32_t loader_init2(CSOUND *csound, LOADER *pp){
    MP3SCAL2 *p = &(pp->p);
    p->csound = csound;
    p->init = 0;
    p->ti = 0;
    if(p->playing == 0){
      pthread_create(&(pp->p.t), NULL, loader_thread2, pp);
      struct sched_param param;
      int32_t policy;
      pthread_getschedparam((pp->p.t), &policy,
                            &param);
      param.sched_priority = 0;
      policy = SCHED_OTHER;
      if(pthread_setschedparam((pp->p.t), policy,
                               &param) != 0)
        csound->Message(csound, "could not set priority\n");
    }
    else return csound->InitError(csound,
                                  Str("cannot load: player still active\n"));
    pp->res->data = (char *) p;
    pp->res->size = sizeof(MP3SCAL2);
    return OK;
}

static int32_t player_init2(CSOUND *csound, PLAYER *p){
    if(p->pp->data != NULL &&
       p->pp->size != sizeof(MP3SCAL2)) {
      p->p = (MP3SCAL2 *) p->pp->data;
    }
    else return csound->InitError(csound, "invalid handle\n");
    *p->ilen = p->p->ilen;
    p->p->async = *p->async;
    if(p->p->initDone == 0)
      csound->RegisterDeinitCallback(csound, p,
                              (int32_t (*)(CSOUND*, void*)) mp3dec_cleanup_player);
    p->p->initDone = 1;

#ifdef HAVE_NEON
    while(!p->p->N) usleep(1000);
    p->setup = pffft_new_setup(p->p->N,PFFFT_REAL);
    p->bw = pffft_aligned_malloc(p->p->N*sizeof(float));
    p->fw = pffft_aligned_malloc(p->p->N*sizeof(float));
#else
    while(!p->p->N) usleep(1000);
    p->fwdsetup = csound->RealFFT2Setup(csound,p->p->N,FFT_FWD);
    p->invsetup = csound->RealFFT2Setup(csound,p->p->N,FFT_INV);
#endif
    p->start_flag = 1;
    return OK;
}



static int32_t player_play2(CSOUND *csound, PLAYER *pp)
{
    MP3SCAL2 *p = pp->p;
    uint32_t offset = pp->h.insdshead->ksmps_offset;
    uint32_t early  = pp->h.insdshead->ksmps_no_end;
    int32_t decim = p->decim;
    MYFLT pitch = *pp->kpitch*p->resamp, time = *pp->time*p->resamp,
      lock = *pp->klock;
    double amp = *pp->kamp*csound->Get0dBFS(csound)*(8.0/decim)/3.0;
    int32_t interp = *pp->kinterp;
#ifdef __clang__
    MYFLT *restrict out;
    MYFLT *restrict fwin;
    MYFLT *restrict bwin;
    MYFLT *restrict prev;
    MYFLT *restrict win = (MYFLT *) p->win.auxp, *restrict outframe;
    int32_t *restrict framecnt, curframe = p->curframe;
    MYFLT *restrict tab;
    MYFLT frac;
    MYFLT *restrict phs;
#else
    MYFLT *out;
    MYFLT *fwin;
    MYFLT *bwin;
    MYFLT *prev;
    MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
    int32_t *framecnt, curframe = p->curframe;
    MYFLT *tab;
    MYFLT frac;
    MYFLT *phs;
#endif
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt;
    int32_t  nsmps = csound->GetKsmps(csound), n;
    int32_t post, i, j;
    double pos, spos = p->pos;
    MYFLT in;
    MYFLT div;
    double tstamp = p->tstamp, incrt = p->incr;
    AUXCH *mfwin = p->fwin,
      *mbwin = p->bwin,
      *mprev = p->prev,
      *moutframe = p->outframe,
      *mframecount = p->framecount;
    MYFLT hsizepitch = hsize*pitch;
    int32_t nbytes =  p->N*sizeof(MYFLT);
    int32_t start_flag = pp->start_flag;
#ifdef HAVE_NEON
    float *restrict bw =  pp->bw, *restrict fw = pp->fw;
#endif
    if(spos >= p->end) {
      for (j=0; j < MP3_CHNS; j++) {
        out = j == 0 ? pp->out1 : pp->out2;
        memset(out, '\0', nsmps*sizeof(MYFLT));
      }
      return OK;
    }
    p->playing = 1;

    if(time < 0) time = 0.0;



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
      if(cnt == hsize){
        for (j = 0; j < MP3_CHNS; j++) {
          bwin = (MYFLT *) mbwin[j].auxp;
          fwin = (MYFLT *) mfwin[j].auxp;
          prev = (MYFLT *) mprev[j].auxp;
          framecnt  = (int32_t *) mframecount[j].auxp;
          outframe= (MYFLT *) moutframe[j].auxp;
          tab = (MYFLT *) p->fdata[j].auxp;
          if(pitch != 1) {
            pos = spos;
#ifndef HAVE_NEON
            for (i=0; i < N; i++) {
              post = (int32_t) pos;
              frac = pos  - post;
              if(interp){
                in = tab[post] + frac*(tab[post+1] - tab[post]);
              }
              else
                in = tab[post];
#ifdef HAVE_NEON
              fw[i] = in * win[i];
#else
              fwin[i] = in * win[i];
#endif

              post -= hsizepitch;
              if(interp){
                in = tab[post] + frac*(tab[post+1] - tab[post]);
              }
              else
                in = tab[post];
#ifdef HAVE_NEON
              bw[i] = in * win[i];
#else
              bwin[i] = in * win[i];
#endif
              pos += pitch;
            }
#else
            float32x4_t bsm1,bsm2,ans;
            float tmpf[5],fracv[4];
            float tmpos;
            int32_t   tmposi;
            for (i=0; i < N; i+=4) {
              tmpos = pos;
              tmposi = FTOINT(tmpos);
              tmpf[0] = tab[tmposi];
              fracv[0] = tmpos - tmposi;
              tmpos += pitch;
              tmposi = FTOINT(tmpos);
              tmpf[1] = tab[tmposi];
              fracv[1] = tmpos -tmposi;
              tmpos += pitch;
              tmposi = FTOINT(tmpos);
              tmpf[2] = tab[tmposi];
              fracv[2] = tmpos - tmposi;
              tmpos += pitch;
              tmposi = FTOINT(tmpos);
              tmpf[3] = tab[tmposi];
              fracv[3] = tmpos - tmposi;
              tmpos += pitch;
              tmposi = FTOINT(tmpos);
              tmpf[4] = tab[tmposi];

              bsm1 = vld1q_f32(&tmpf[0]);
              bsm2 = vld1q_f32(&tmpf[1]);
              ans = vsubq_f32(bsm2,bsm1);
              bsm2 = vld1q_f32(fracv);
              bsm2 = vmulq_f32(ans,bsm2);
              ans = vaddq_f32(bsm1,bsm2);
              bsm1 = vld1q_f32(&win[i]);
              bsm2 = vmulq_f32(ans,bsm1);
              vst1q_f32(&fw[i],bsm2);

              tmpos = pos - hsize*pitch;
              tmposi = FTOINT(tmpos);
              tmpf[0] = tab[tmposi];
              fracv[0] = tmpos - tmposi;
              tmpos += pitch;
              tmposi = FTOINT(tmpos);
              tmpf[1] = tab[tmposi];
              fracv[1] = tmpos -tmposi;
              tmpos += pitch;
              tmposi = FTOINT(tmpos);
              tmpf[2] = tab[tmposi];
              fracv[2] = tmpos - tmposi;
              tmpos += pitch;
              tmposi = FTOINT(tmpos);
              tmpf[3] = tab[tmposi];
              fracv[3] = tmpos - tmposi;
              tmpos += pitch;
              tmposi = FTOINT(tmpos);
              tmpf[4] = tab[tmposi];

              bsm1 = vld1q_f32(&tmpf[0]);
              bsm2 = vld1q_f32(&tmpf[1]);
              ans = vsubq_f32(bsm2,bsm1);
              bsm2 = vld1q_f32(fracv);
              bsm2 = vmulq_f32(ans,bsm2);
              ans = vaddq_f32(bsm1,bsm2);
              bsm1 = vld1q_f32(&win[i]);
              bsm2 = vmulq_f32(ans,bsm1);
              vst1q_f32(&bw[i],bsm2);
              pos += pitch*4;
            }
#endif
          }
          else {
            post = (int32_t) spos;
            memcpy(fwin,&tab[post],nbytes);
            post -= hsize;
            memcpy(bwin,&tab[post],nbytes);
#ifdef HAVE_NEON
            for(i=0; i < N; i++) {
              bw[i] = bwin[i]*win[i];
              fw[i] = fwin[i]*win[i];
            }
#else
            for(i=0; i < N; i++) {
              bwin[i] *= win[i];
              fwin[i] *= win[i];
            }
#endif
          }

          //       if(time != FL(1.0) || pitch != FL(1.0)){

#ifdef HAVE_NEON
          pffft_transform_ordered(pp->setup,bw,bw,NULL,PFFFT_FORWARD);
          pffft_transform_ordered(pp->setup,fw,fw,NULL,PFFFT_FORWARD);
          memcpy(bwin,bw,N*sizeof(float));
          memcpy(fwin,fw,N*sizeof(float));
          bwin[N] = bw[1];
          fwin[N] = fw[1];
#else
          csound->RealFFT2(csound, pp->fwdsetup, bwin);
          csound->RealFFT2(csound, pp->fwdsetup, fwin);
#endif
          bwin[N] = bwin[1];
          bwin[N+1] = FL(0.0);
          fwin[N] = fwin[1];
          fwin[N+1] = FL(0.0);

          if(start_flag){
            memcpy(prev, bwin, sizeof(MYFLT)*(N+2));
            pp->start_flag = 0;
          }

          for (i=0; i < N; i+=2) {
            div =  inv_mag(&prev[i]);
            cmplx_multiply_scal(&prev[i],&prev[i+1],
                                &bwin[i],&prev[i],
                                div, true);
          }

          if (lock) {
            phs = prev;
            for(i = 2; i < N; i++)
              bwin[i] = phs[i];
            for(i = 2; i < N; i++)
              bwin[i] += phs[i-2];
            for(i = 2; i < N; i++)
              bwin[i] += phs[i+2];
            bwin[0] = prev[i] + prev[i-2];
            bwin[N] = prev[i] + prev[i+2];
          }
          else memcpy(bwin,prev,sizeof(MYFLT)*(N+2));

          for (i=0; i < N; i+=2) {
            div =  inv_mag(&bwin[i]);
            cmplx_multiply_scal(&prev[i],&prev[i+1],
                                &fwin[i], &bwin[i],
                                div, false);
          }
#ifdef HAVE_NEON
          for(i=0;i<N;i++)
            fw[i] = prev[i]/N;
          fw[1] = prev[N]/N;
          pffft_transform_ordered(pp->setup,fw,fw,NULL,PFFFT_BACKWARD);
#else

#endif
          //} else pp->start_flag = 1;

          framecnt[curframe] = curframe*N;
          for (i=0;i<N;i++)
#ifdef HAVE_NEON
            outframe[framecnt[curframe]+i] = win[i]*fw[i];
#else
          outframe[framecnt[curframe]+i] = win[i]*fwin[i];
#endif
        }
        cnt=0;
        curframe++;
        if (curframe == decim) curframe = 0;

        /* increment position according to timescale */
        spos += hsize*time;
        incrt =  time*nsmps;
      }

      /* we only output as many channels as we have outs for */
      for (j=0; j < 2; j++) {
        out = j == 0 ? pp->out1 : pp->out2;
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
    *pp->kstamp = (p->skip + p->tstamp/p->orsr);
    p->tstamp = tstamp + incrt;
    p->incr = incrt;
    p->playing = 0;
    return OK;

}

#endif


#define S(x)    sizeof(x)

static OENTRY mp3in_localops[] =
{
  {"mp3in",  S(MP3IN),  0, 3, "mm", "Soooo", (SUBR) mp3ininit_S, (SUBR)mp3in},
  {"mp3in.i",  S(MP3IN),  0, 3, "mm", "ioooo", (SUBR) mp3ininit, (SUBR)mp3in},
  {"mp3len", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3len.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
  {"mp3sr", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3sr.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
  {"mp3bitrate", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3bitrate.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
  {"mp3nchnls", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3nchnls.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
  {"mp3scal", sizeof(DATASPACE), 0, 3, "aak", "SkkkoooPP",
   (SUBR)sinit3,(SUBR)sprocess3 },
#ifdef ANDROID
  {"mp3scal_load", sizeof(LOADER), 0, 1, "i", "Soooo",
   (SUBR)loader_init, NULL,NULL },
  {"mp3scal_play", sizeof(PLAYER), 0, 3, "aaki", "ikkkPPo",
   (SUBR)player_init,(SUBR)player_play},
  {"mp3scal_check", sizeof(CHECK), 0, 3, "k", "i",
   (SUBR)check_init,(SUBR)check_play},
 #endif
  /*
  {"mp3scal_load2", sizeof(LOADER), 0, 1, "i", "Soooo",
   (SUBR)loader_init2, NULL,NULL },
  {"mp3scal_play2", sizeof(PLAYER), 0, 3, "aaki", "ikkkPPo",
   (SUBR)player_init2,(SUBR)player_play2}
  */
};

LINKAGE_BUILTIN(mp3in_localops)
