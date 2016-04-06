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
#define MAXOUTS 2
#define MAXOUTS 2

typedef struct dats{
  OPDS h;
  MYFLT *out1,*out2,*kstamp, *knum, *time,*kpitch, *kamp, *skip, *iN,
    *idecim, *klock;
  int cnt, hsize, curframe, N, decim,tscale;
  unsigned int nchans;
  double pos;
  MYFLT accum;
  AUXCH outframe[MAXOUTS], win, bwin[MAXOUTS], fwin[MAXOUTS],
    nwin[MAXOUTS], prev[MAXOUTS], framecount[MAXOUTS], fdata, buffer;
  MYFLT *indata[2];
  MYFLT *tab;
  int curbuf;
  mp3dec_t mpa;
  FDCH    fdch;
  MYFLT resamp;
  double tstamp, incr;
  int initDone;
  uint32_t bufused;
  int finished;
} DATASPACE;

int mp3scale_cleanup(CSOUND *csound, DATASPACE *p)
{
    if (p->mpa != NULL)
      mp3dec_uninit(p->mpa);
    return OK;
}

#define BUFS 8
static void fillbuf(CSOUND *csound, DATASPACE *p, int nsmps);
/* file-reading version of temposcal */
static int sinit(CSOUND *csound, DATASPACE *p)
{

    int N =  *p->iN, ui;
    unsigned int nchans, i;
    unsigned int size;
    int decim = *p->idecim;

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

    nchans = p->nchans;

  if (UNLIKELY(nchans < 1 || nchans > MAXOUTS))
      csound->InitError(csound, Str("invalid number of output arguments"));
    p->nchans = nchans;

    for (i=0; i < nchans; i++){

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
static int sinit3(CSOUND *csound, DATASPACE *p)
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
    } else
       csound->Message(csound, Str("mp3scale: open %s \n"), name);
    if (UNLIKELY((r = mp3dec_init_file(mpa, fd, 0, FALSE)) != MP3DEC_RETCODE_OK)) {
      mp3dec_uninit(mpa);
      return csound->InitError(csound, mp3dec_error(r));
    } else
      csound->Message(csound, Str("mp3scale: init %s \n"), name);

    if (UNLIKELY((r = mp3dec_get_info(mpa, &mpainfo, MPADEC_INFO_STREAM)) !=
                 MP3DEC_RETCODE_OK)) {
      mp3dec_uninit(mpa);
      return csound->InitError(csound, mp3dec_error(r));
    }

    {
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

    if(mpainfo.frequency != CS_ESR)
      p->resamp = mpainfo.frequency/CS_ESR;
    else
     p->resamp = 1;
    p->nchans = 2;


   sinit(csound, p);
   size = p->N*sizeof(MYFLT)*BUFS;
   if (p->fdata.auxp == NULL || p->fdata.size < size)
      csound->AuxAlloc(csound, size, &p->fdata);
   p->indata[0] = p->fdata.auxp;
   p->indata[1] = p->fdata.auxp + size/2;
   size = p->N*sizeof(short)*BUFS/2;
   if (p->buffer.auxp == NULL || p->buffer.size < size)
      csound->AuxAlloc(csound, size, &p->buffer);

   /*
   memset(&(p->fdch), 0, sizeof(FDCH));
   p->fdch.fd = fd;
   fdrecord(csound, &(p->fdch));
   */

   int buffersize = size;
   buffersize /= mpainfo.decoded_sample_size;
   int skip = (int)(*p->skip*CS_ESR)*p->resamp;
   p->bufused = -1;
   mp3dec_seek(mpa, skip, MP3DEC_SEEK_SAMPLES);

   // fill buffers
    p->curbuf = 0;
    fillbuf(csound,p,p->N*BUFS/2);
    p->pos = p->hsize;
    p->tscale  = 0;
    p->accum = 0;
    p->tab = (MYFLT *) p->fdata.auxp;
    p->tstamp = 0;
    if(p->initDone == -1)
       csound->RegisterDeinitCallback(csound, p,
                                   (int (*)(CSOUND*, void*)) mp3scale_cleanup);
    p->initDone = -1;
    p->finished = 0;
    return OK;
}

/*
 this will read a buffer full of samples
 from disk position offset samps from the last
 call to fillbuf
*/

void fillbuf(CSOUND *csound, DATASPACE *p, int nsmps){
     short *buffer= (short *) p->buffer.auxp;
     MYFLT *data =  p->indata[p->curbuf];
     int r,i;
     if(!p->finished){
     r = mp3dec_decode(p->mpa,p->buffer.auxp, nsmps*sizeof(short), &p->bufused);
     for(i=0; i < nsmps;i++)
       data[i] = p->bufused ? buffer[i]/32768.0 : 0.0;
     if(p->bufused == 0) p->finished = 1;
     } else memset(data,0,nsmps*sizeof(MYFLT));
     p->curbuf = p->curbuf ? 0 : 1;
}

static int sprocess3(CSOUND *csound, DATASPACE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT pitch = *p->kpitch*p->resamp, time = *p->time*p->resamp, lock = *p->klock;
    MYFLT *out, amp =*p->kamp;
    MYFLT *tab,frac;
    FUNC *ft;
    int N = p->N, hsize = p->hsize, cnt = p->cnt, sizefrs, nchans = p->nchans;
    int  nsmps = CS_KSMPS, n;
    int size, post, i, j;
    double pos, spos = p->pos;
    MYFLT *fwin, *bwin;
    MYFLT in, *nwin, *prev;
    MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
    MYFLT powrat;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int *framecnt, curframe = p->curframe;
    int decim = p->decim;
    double tstamp = p->tstamp, incrt = p->incr;

    int outnum = csound->GetOutputArgCnt(p);
    double _0dbfs = csound->Get0dBFS(csound);

    if(time < 0) time = 0.0;

    /*if(p->finished){
      for (j=0; j < nchans; j++) {
         out = j == 0 ? p->out1 : p->out2;
        memset(out, '\0', nsmps*sizeof(MYFLT));
     }
      return OK;
      }*/

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nchans; j++) {
        out = j == 0 ? p->out1 : p->out2;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
      }
    }
    if (UNLIKELY(offset)) {
     for (j=0; j < nchans; j++) {
         out = j == 0 ? p->out1 : p->out2;
        memset(out, '\0', offset*sizeof(MYFLT));
     }
     }

    for (n=offset; n < nsmps; n++) {

      if (cnt == hsize){
        tab = p->tab;
        size = p->fdata.size/sizeof(MYFLT);
        spos += hsize*time;
        incrt =  time*nsmps;

        sizefrs = size/nchans;

        while(spos > sizefrs) {
          spos -= sizefrs;
        }
        while(spos <= 0){
          spos += sizefrs;
        }
        if (spos > sizefrs/2 && p->curbuf == 0) {
          fillbuf(csound,p,size/2);
        } else if (spos < sizefrs/2 && p->curbuf == 1){
          fillbuf(csound,p,size/2);
        }

        for (j = 0; j < nchans; j++) {
          pos = spos;
          bwin = (MYFLT *) p->bwin[j].auxp;
          fwin = (MYFLT *) p->fwin[j].auxp;
          prev = (MYFLT *)p->prev[j].auxp;
          framecnt  = (int *)p->framecount[j].auxp;
          outframe= (MYFLT *) p->outframe[j].auxp;

          for (i=0; i < N; i++) {
            post = (int) pos;
            frac = pos  - post;
            post *= nchans;
            post += j;

           while(post < 0) post += size;
           while(post >= size) post -= size;
           if(post+nchans <  size)
            in = tab[post] + frac*(tab[post+nchans] - tab[post]);
           else {
             in = tab[post];
           }

            fwin[i] = in * win[i];

            post = (int) (pos - hsize*pitch);
            post *= nchans;
            post += j;
            while(post < 0) post += size;
            while(post >= size) post -= size;
            if(post+nchans <  size)
            in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];
            bwin[i] = in * win[i];
            pos += pitch;
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

            tmp_real =   fwin[i] * ph_real - fwin[i+1] * ph_im;
            tmp_im =   fwin[i] * ph_im + fwin[i+1] * ph_real;

            prev[i] = fwin[i] = tmp_real;
            prev[i+1] = fwin[i+1] = tmp_im;
          }

          fwin[1] = fwin[N];
          csound->InverseRealFFT(csound, fwin, N);

          framecnt[curframe] = curframe*N;

          for (i=0;i<N;i++) outframe[framecnt[curframe]+i] = win[i]*fwin[i];

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
        out[n] *= _0dbfs*amp*(2./3.);
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
  {"mp3scal", sizeof(DATASPACE), 0, 5, "aak", "Skkkooop",
                                               (SUBR)sinit3, NULL,(SUBR)sprocess3 },
};

LINKAGE_BUILTIN(mp3in_localops)
