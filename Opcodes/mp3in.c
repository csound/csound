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
    while (skip > 0) {
      int xx= skip;
      if (xx > buffersize) xx = buffersize;
      skip -= xx;
      r = mp3dec_decode(mpa, p->buf, mpainfo.decoded_sample_size*xx, &p->bufused);
    }
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
      for (i=0; i<2; i++) {     /* stereo */
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

#define S(x)    sizeof(x)

static OENTRY mp3in_localops[] = {
  {"mp3in",  S(MP3IN),  0, 5, "aa", "Soooo", (SUBR) mp3ininit_S, NULL, (SUBR) mp3in},
  {"mp3in.i",  S(MP3IN),  0, 5, "aa", "ioooo", (SUBR) mp3ininit, NULL, (SUBR) mp3in},
  {"mp3len", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3len.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
  {"mp3sr", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3sr.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
    {"mp3bitrate", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3bitrate.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL},
    {"mp3nchnls", S(MP3LEN), 0, 1, "i",  "S",     (SUBR) mp3len_S,    NULL,  NULL},
  {"mp3nchnls.i", S(MP3LEN), 0, 1, "i",  "i",     (SUBR) mp3len,    NULL,  NULL}
};

LINKAGE_BUILTIN(mp3in_localops)
