/*
    tabaudio.c:

    Copyright (C) 2018 John ffitch

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

#include "csoundCore.h"
#include "interlocks.h"
//#include <pthread.h>
#include <sndfile.h>
#include "soundio.h"

typedef struct {
    OPDS    h;
    MYFLT   *kans;
    MYFLT   *itab;
    STRINGDAT *file;
    MYFLT   *format;
    MYFLT   *sync;
    /* Local */
} TABAUDIO;

typedef struct {
    OPDS    h;
    MYFLT   *kans;
    MYFLT   *trig;
    MYFLT   *itab;
    STRINGDAT *file;
    MYFLT   *format;
    MYFLT   *sync;
    /* Local */
} TABAUDIOK;

typedef struct {
    CSOUND *csound;
    MYFLT*   t;
    uint32_t size;
    SNDFILE* ff;
    MYFLT*   ans;
    void     *thread;
    OPDS     *h;
} SAVE_THREAD;

static const int32_t format_table[51] = {
    /* 0 - 9 */
    (SF_FORMAT_FLOAT | SF_FORMAT_RAW), (SF_FORMAT_PCM_16 | SF_FORMAT_RAW),
    SF_FORMAT_PCM_16, SF_FORMAT_ULAW, SF_FORMAT_PCM_16, SF_FORMAT_PCM_32,
    SF_FORMAT_FLOAT, SF_FORMAT_PCM_U8, SF_FORMAT_PCM_24, SF_FORMAT_DOUBLE,
    /* 10 - 19 */
    SF_FORMAT_WAV, (SF_FORMAT_PCM_S8 | SF_FORMAT_WAV),
    (SF_FORMAT_ALAW | SF_FORMAT_WAV), (SF_FORMAT_ULAW | SF_FORMAT_WAV),
    (SF_FORMAT_PCM_16 | SF_FORMAT_WAV), (SF_FORMAT_PCM_32 | SF_FORMAT_WAV),
    (SF_FORMAT_FLOAT | SF_FORMAT_WAV), (SF_FORMAT_PCM_U8 | SF_FORMAT_WAV),
    (SF_FORMAT_PCM_24 | SF_FORMAT_WAV), (SF_FORMAT_DOUBLE | SF_FORMAT_WAV),
    /* 20 - 29 */
    SF_FORMAT_AIFF, (SF_FORMAT_PCM_S8 | SF_FORMAT_AIFF),
    (SF_FORMAT_ALAW | SF_FORMAT_AIFF), (SF_FORMAT_ULAW | SF_FORMAT_AIFF),
    (SF_FORMAT_PCM_16 | SF_FORMAT_AIFF), (SF_FORMAT_PCM_32 | SF_FORMAT_AIFF),
    (SF_FORMAT_FLOAT | SF_FORMAT_AIFF), (SF_FORMAT_PCM_U8 | SF_FORMAT_AIFF),
    (SF_FORMAT_PCM_24 | SF_FORMAT_AIFF), (SF_FORMAT_DOUBLE | SF_FORMAT_AIFF),
    /* 30 - 39 */
    SF_FORMAT_RAW, (SF_FORMAT_PCM_S8 | SF_FORMAT_RAW),
    (SF_FORMAT_ALAW | SF_FORMAT_RAW), (SF_FORMAT_ULAW | SF_FORMAT_RAW),
    (SF_FORMAT_PCM_16 | SF_FORMAT_RAW), (SF_FORMAT_PCM_32 | SF_FORMAT_RAW),
    (SF_FORMAT_FLOAT | SF_FORMAT_RAW), (SF_FORMAT_PCM_U8 | SF_FORMAT_RAW),
    (SF_FORMAT_PCM_24 | SF_FORMAT_RAW), (SF_FORMAT_DOUBLE | SF_FORMAT_RAW),
    /* 40 - 49 */
    SF_FORMAT_IRCAM, (SF_FORMAT_PCM_S8 | SF_FORMAT_IRCAM),
    (SF_FORMAT_ALAW | SF_FORMAT_IRCAM), (SF_FORMAT_ULAW | SF_FORMAT_IRCAM),
    (SF_FORMAT_PCM_16 | SF_FORMAT_IRCAM), (SF_FORMAT_PCM_32 | SF_FORMAT_IRCAM),
    (SF_FORMAT_FLOAT | SF_FORMAT_IRCAM), (SF_FORMAT_PCM_U8 | SF_FORMAT_IRCAM),
    (SF_FORMAT_PCM_24 | SF_FORMAT_IRCAM), (SF_FORMAT_DOUBLE | SF_FORMAT_IRCAM),
    /* 50 */
    (SF_FORMAT_OGG | SF_FORMAT_VORBIS)
};

static uintptr_t write_tab(void* pp)
{
    SAVE_THREAD *p = (SAVE_THREAD*)pp;
    MYFLT*   t = p->t;
    uint32_t size = p->size;
    SNDFILE* ff = p->ff;
    MYFLT*   ans = p->ans;
    CSOUND*  csound = p->csound;
    OPDS     *h = p->h;
    //free(pp);
    //printf("t=%p size=%d ff=%p\n", t, size, ff);
    if (sf_writef_MYFLT(ff, t, size) != size) {
      sf_close(ff);
      csound->PerfError(csound, h,
                           Str("tabaudio: failed to write data %d"),size);
      *ans = -FL(1.0);
    }
    else *ans = FL(1.0);
    sf_close(ff);
    return 0;
}

int on_reset_audio(CSOUND *csound, void *pp)
{
    SAVE_THREAD *p =  (SAVE_THREAD *) pp;
    csound->JoinThread(p->thread);
    return 0;
}

static int32_t tabaudiok(CSOUND *csound, TABAUDIOK *p)
{
    if (*p->trig) {
      FUNC  *ftp;
      MYFLT *t;
      uint32_t size, n;
      SNDFILE *ff;
      SF_INFO sfinfo;
      int32_t  format = MYFLT2LRND(*p->format);
  
      if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->itab)) == NULL)) {
        return csound->PerfError(csound, &(p->h), Str("tabaudio: No table %g"), *p->itab);
      }
      *p->kans = FL(0.0);
      t = ftp->ftable;
      size = ftp->flenfrms;
      memset(&sfinfo, 0, sizeof(SF_INFO));
      if (format >= 51)
        sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
      else if (format < 0) {
        sfinfo.format = FORMAT2SF(csound->oparms->outformat);
        sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
      }
      else sfinfo.format = format_table[format];
      if (!SF2FORMAT(sfinfo.format))
        sfinfo.format |= FORMAT2SF(csound->oparms->outformat);
      if (!SF2TYPE(sfinfo.format))
        sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
      sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
      sfinfo.channels = ftp->nchanls;
      ff = sf_open(p->file->data, SFM_WRITE, &sfinfo);
      if (ff==NULL)
        return csound->PerfError(csound, &(p->h),
                                 Str("tabaudio: failed to open file %s"),
                                 p->file->data);
      if (*p->sync==FL(0.0)) {  /* write in perf thread */
        if ((n=sf_writef_MYFLT(ff, t, size)) != size) {
          printf("%s\n", sf_strerror(ff));
          sf_close(ff);
          return csound->PerfError(csound, &(p->h),
                                   Str("tabaudio: failed to write data %d %d"),
                                   n,size);
        }
        sf_close(ff);
      }
      else {                    /* Use a helper thread */
        SAVE_THREAD *q = (SAVE_THREAD*)csound->Malloc(csound, sizeof(SAVE_THREAD));
        q->t = t;
        q->size = size;
        q->ff = ff;
        q->ans = p->kans;
        q->csound = csound;
        q->h = &(p->h);
        if ((q->thread = csound->CreateThread(write_tab, (void*)q))==NULL) {
          OPDS * i = q->h;
          free(q);
          return csound->PerfError(csound, i,
                                   Str("Error creating thread"));
        }
        csound->RegisterResetCallback(csound, (void*)q, on_reset_audio);
        /* if (fork() == 0) { */
        /*   ff = sf_open(p->file->data, SFM_WRITE, &sfinfo); */
        /*   if (ff==NULL) { */
        /*     printf(Str("tabaudio: failed to open file %s"), p->file->data); */
        /*     exit(1); */
        /*   } */
        /*   if ((n=sf_writef_MYFLT(ff, t, size)) != size) { */
        /*     sf_close(ff); */
        /*     printf("%s %s", Str("tabaudio: failed to write data:"), */
        /*            sf_strerror(ff)); */
        /*     exit(1); */
        /*   } */
        /*   sf_close(ff); */
        /*   exit(0); */
        /* } */
      }
      *p->kans = FL(1.0);
    }
    else *p->kans = FL(0.0);
    return OK;
}

static int32_t tabaudioi(CSOUND *csound, TABAUDIO *p)
{
    FUNC  *ftp;
    MYFLT *t;
    uint32_t size, n;
    SNDFILE *ff;
    SF_INFO sfinfo;
    int32_t  format = MYFLT2LRND(*p->format);
      
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->itab)) == NULL)) {
      return csound->InitError(csound, Str("tabaudio: No table"));
    }
    *p->kans = FL(0.0);
    t = ftp->ftable;
    size = ftp->flenfrms;
    memset(&sfinfo, 0, sizeof(SF_INFO));
    if (format >= 51)
      sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
    else if (format < 0) {
      sfinfo.format = FORMAT2SF(csound->oparms->outformat);
      sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
    }
    else sfinfo.format = format_table[format];
    if (!SF2FORMAT(sfinfo.format))
      sfinfo.format |= FORMAT2SF(csound->oparms->outformat);
    if (!SF2TYPE(sfinfo.format))
      sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
    sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
    sfinfo.channels = ftp->nchanls;
      
    ff = sf_open(p->file->data, SFM_WRITE, &sfinfo);
    if (ff==NULL)
      return csound->InitError(csound, Str("tabaudio: failed to open file %s"),
                               p->file->data);
    if ((n=sf_writef_MYFLT(ff, t, size)) != size) {
      printf("%s\n", sf_strerror(ff));
      sf_close(ff);
      return csound->InitError(csound, Str("tabaudio: failed to write data: %s"),
                               sf_strerror(ff));
    }
    *p->kans = FL(1.0);
    sf_close(ff);
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY tabaudio_localops[] =
  {
   { "ftaudio.i",     S(TABAUDIO),  TR, 1, "i", "iSki",   (SUBR)tabaudioi, NULL },
   { "ftaudio.k",     S(TABAUDIOK), TR, 2, "k", "kkSkp",  NULL, (SUBR)tabaudiok },
  };

LINKAGE_BUILTIN(tabaudio_localops)



