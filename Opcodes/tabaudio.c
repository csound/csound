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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include "soundio.h"

typedef struct {
  CSOUND *csound;
  MYFLT   *samples;
  uint32_t frames;
  SNDFILE *sf;
  void     *fd;
  void     *thread;
  volatile int32_t result;
} SAVE_THREAD;

typedef struct {
  OPDS    h;
  MYFLT   *kans;
  MYFLT   *itab;
  STRINGDAT *file;
  MYFLT   *format;
  MYFLT   *beg;
  MYFLT   *end;
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
  MYFLT   *beg;
  MYFLT   *end;
  /* Local */
  SAVE_THREAD *job;
} TABAUDIOK;

static const int32_t format_table[51] = {
  /* 0 - 9 */
  (AE_FLOAT | TYP2SF(TYP_RAW)), (AE_SHORT | TYP2SF(TYP_RAW)),
  AE_SHORT, AE_ULAW, AE_SHORT, AE_LONG,
  AE_FLOAT, AE_UNCH, AE_24INT, AE_DOUBLE,
  /* 10 - 19 */
  TYP2SF(TYP_WAV), (AE_CHAR | TYP2SF(TYP_WAV)),
  (AE_ALAW | TYP2SF(TYP_WAV)), (AE_ULAW | TYP2SF(TYP_WAV)),
  (AE_SHORT | TYP2SF(TYP_WAV)), (AE_LONG | TYP2SF(TYP_WAV)),
  (AE_FLOAT | TYP2SF(TYP_WAV)), (AE_UNCH | TYP2SF(TYP_WAV)),
  (AE_24INT | TYP2SF(TYP_WAV)), (AE_DOUBLE | TYP2SF(TYP_WAV)),
  /* 20 - 29 */
  TYP2SF(TYP_AIFF), (AE_CHAR | TYP2SF(TYP_AIFF)),
  (AE_ALAW | TYP2SF(TYP_AIFF)), (AE_ULAW | TYP2SF(TYP_AIFF)),
  (AE_SHORT | TYP2SF(TYP_AIFF)), (AE_LONG | TYP2SF(TYP_AIFF)),
  (AE_FLOAT | TYP2SF(TYP_AIFF)), (AE_UNCH | TYP2SF(TYP_AIFF)),
  (AE_24INT | TYP2SF(TYP_AIFF)), (AE_DOUBLE | TYP2SF(TYP_AIFF)),
  /* 30 - 39 */
  TYP2SF(TYP_RAW), (AE_CHAR | TYP2SF(TYP_RAW)),
  (AE_ALAW | TYP2SF(TYP_RAW)), (AE_ULAW | TYP2SF(TYP_RAW)),
  (AE_SHORT | TYP2SF(TYP_RAW)), (AE_LONG | TYP2SF(TYP_RAW)),
  (AE_FLOAT | TYP2SF(TYP_RAW)), (AE_UNCH | TYP2SF(TYP_RAW)),
  (AE_24INT | TYP2SF(TYP_RAW)), (AE_DOUBLE | TYP2SF(TYP_RAW)),
  /* 40 - 49 */
  TYP2SF(TYP_IRCAM), (AE_CHAR | TYP2SF(TYP_IRCAM)),
  (AE_ALAW | TYP2SF(TYP_IRCAM)), (AE_ULAW | TYP2SF(TYP_IRCAM)),
  (AE_SHORT | TYP2SF(TYP_IRCAM)), (AE_LONG | TYP2SF(TYP_IRCAM)),
  (AE_FLOAT | TYP2SF(TYP_IRCAM)), (AE_UNCH | TYP2SF(TYP_IRCAM)),
  (AE_24INT | TYP2SF(TYP_IRCAM)), (AE_DOUBLE | TYP2SF(TYP_IRCAM)),
  /* 50 */
  (TYP2SF(TYP_OGG) | AE_VORBIS)
};


static uintptr_t write_tab(void* pp)
{
  SAVE_THREAD *p = (SAVE_THREAD*)pp;
  CSOUND *csound = p->csound;
  int32_t result = 1;

  if (csound->SndfileWrite(csound, p->sf, p->samples, p->frames)
      != p->frames) {
    csound->ErrorMsg(csound, Str("tabaudio: failed to write data: %s"),
                     csound->SndfileStrError(csound, p->sf));
    result = 0;
  }
  csound->FileClose(csound, p->fd, CSFILE_CLOSE_SYNC);
  csound->Free(csound, p->samples);
  ATOMIC_SET(p->result, result);
  return 0;
}

static int32_t on_reset_audio(CSOUND *csound, void *pp)
{
  SAVE_THREAD *p = (SAVE_THREAD *) pp;
  csound->JoinThread(p->thread);
  csound->Free(csound, p);
  return 0;
}

static inline int32_t ftaudio_range(FUNC *ftp, MYFLT beginArg, MYFLT endArg,
                                    MYFLT **samples, uint32_t *frames)
{
  int32_t begin, end;

  if (UNLIKELY(ftp->nchanls < 1 || ftp->flenfrms < 0 ||
               !(beginArg >= FL(0.0) && beginArg <= ftp->flenfrms) ||
               !(endArg <= FL(0.0) || endArg <= ftp->flenfrms)))
    return NOTOK;
  begin = MYFLT2LRND(beginArg);
  end = endArg <= FL(0.0) ? ftp->flenfrms : MYFLT2LRND(endArg);
  if (UNLIKELY(begin > end))
    return NOTOK;
  *samples = ftp->ftable + (size_t) begin * (size_t) ftp->nchanls;
  *frames = (uint32_t) (end - begin);
  return OK;
}

static int32_t tabaudiok_init(CSOUND *csound, TABAUDIOK *p)
{
  (void) csound;
  p->job = NULL;
  *p->kans = FL(0.0);
  return OK;
}

static int32_t tabaudiok(CSOUND *csound, TABAUDIOK *p)
{
  if (p->job != NULL) {
    int32_t result = ATOMIC_GET(p->job->result);

    *p->kans = (MYFLT) result;
    if (result >= 0)
      p->job = NULL;
  }
  else
    *p->kans = FL(0.0);

  if (*p->trig) {
    FUNC  *ftp;
    MYFLT *t;
    int64_t n;
    uint32_t frames;
    SNDFILE *sf;
    void *fd;
    SFLIB_INFO sfinfo;
    int32_t  format = MYFLT2LRND(*p->format);
    const OPARMS *parms;
    parms =   csound->GetOParms(csound) ;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->itab)) == NULL)) {
      return csound->PerfError(csound, &(p->h), Str("tabaudio: No table %g"), *p->itab);
    }
    if (UNLIKELY(ftaudio_range(ftp, *p->beg, *p->end, &t, &frames) != OK))
      return csound->PerfError(csound, &(p->h), "%s",
                               Str("ftaudio: illegal range"));
    memset(&sfinfo, 0, sizeof(SFLIB_INFO));
    if (format >= 51)
      sfinfo.format = AE_SHORT | TYP2SF(TYP_RAW);
    else if (format < 0) {
      sfinfo.format = FORMAT2SF(parms->outformat);
      sfinfo.format |= TYPE2SF(parms->filetyp);
    }
    else sfinfo.format = format_table[format];
    if (!SF2FORMAT(sfinfo.format))
      sfinfo.format |= FORMAT2SF(parms->outformat);
    if (!SF2TYPE(sfinfo.format))
      sfinfo.format |= TYPE2SF(parms->filetyp);
    sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
    sfinfo.channels = ftp->nchanls;
    fd = csound->FileOpen(csound, &sf, CSFILE_SND_W, p->file->data, &sfinfo, NULL,
                          csound->Type2CsfileType(parms->filetyp,
                                                 parms->outformat), 0);
    if (fd == NULL)
      return csound->PerfError(csound, &(p->h),
                               Str("tabaudio: failed to open file %s"),
                               p->file->data);
    if (*p->sync==FL(0.0)) {  /* write in perf thread */
      if ((n = csound->SndfileWrite(csound, sf, t, frames)) != frames) {
        int32_t result = csound->PerfError(
          csound, &(p->h), Str("tabaudio: failed to write data: %s"),
          csound->SndfileStrError(csound, sf));
        csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
        return result;
      }
      csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
    }
    else if (frames == 0) {
      csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
    }
    else {                    /* Use a helper thread */
      SAVE_THREAD *q = (SAVE_THREAD*)csound->Malloc(csound, sizeof(SAVE_THREAD));
      size_t sampleCount = (size_t) frames * (size_t) ftp->nchanls;
      q->samples = (MYFLT *) csound->Malloc(csound,
                                            sampleCount * sizeof(MYFLT));
      memcpy(q->samples, t, sampleCount * sizeof(MYFLT));
      q->frames = frames;
      q->sf = sf;
      q->fd = fd;
      q->csound = csound;
      q->result = -1;
      if ((q->thread = csound->CreateThread(write_tab, (void*)q)) == NULL) {
        csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
        csound->Free(csound, q->samples);
        csound->Free(csound, q);
        return csound->PerfError(csound, &(p->h),
                                 "%s", Str("Error creating thread"));
      }
      if (UNLIKELY(csound->RegisterResetCallback(csound, (void *) q,
                                                 on_reset_audio) != OK)) {
        csound->JoinThread(q->thread);
        csound->Free(csound, q);
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("Error registering thread cleanup"));
      }
      p->job = q;
      *p->kans = -FL(1.0);
    }
    if (*p->sync == FL(0.0) || frames == 0)
      *p->kans = FL(1.0);
  }
  return OK;
}

static int32_t tabaudioi(CSOUND *csound, TABAUDIO *p)
{
  FUNC  *ftp;
  MYFLT *t;
  int64_t n;
  uint32_t frames;
  SNDFILE *sf;
  void *fd;
  SFLIB_INFO sfinfo;
  int32_t  format = MYFLT2LRND(*p->format);

 const OPARMS *parms;
  parms =   csound->GetOParms(csound) ;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->itab)) == NULL)) {
    return csound->InitError(csound, "%s", Str("tabaudio: No table"));
  }
  *p->kans = FL(0.0);
  if (UNLIKELY(ftaudio_range(ftp, *p->beg, *p->end, &t, &frames) != OK))
    return csound->InitError(csound, "%s", Str("ftaudio: illegal range"));
  memset(&sfinfo, 0, sizeof(SFLIB_INFO));
  if (format >= 51)
    sfinfo.format = AE_SHORT | TYP2SF(TYP_RAW);
  else if (format < 0) {
    sfinfo.format = FORMAT2SF(parms->outformat);
    sfinfo.format |= TYPE2SF(parms->filetyp);
  }
  else sfinfo.format = format_table[format];
  if (!SF2FORMAT(sfinfo.format))
    sfinfo.format |= FORMAT2SF(parms->outformat);
  if (!SF2TYPE(sfinfo.format))
    sfinfo.format |= TYPE2SF(parms->filetyp);

  sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
  sfinfo.channels = ftp->nchanls;

  fd = csound->FileOpen(csound, &sf, CSFILE_SND_W, p->file->data, &sfinfo, NULL,
                        csound->Type2CsfileType(parms->filetyp,
                                               parms->outformat), 0);
  if (fd == NULL)
    return csound->InitError(csound, Str("tabaudio: failed to open file %s"),
                             p->file->data);
  if ((n = csound->SndfileWrite(csound, sf, t, frames)) != frames) {
    int32_t result = csound->InitError(
      csound, Str("tabaudio: failed to write data: %s"),
      csound->SndfileStrError(csound, sf));
    csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
    return result;
  }
  *p->kans = FL(1.0);
  csound->FileClose(csound, fd, CSFILE_CLOSE_SYNC);
  return OK;
}

#define S(x)    sizeof(x)

static OENTRY tabaudio_localops[] =
  {
   { "ftaudio.i",     S(TABAUDIO),  TR,  "i", "iSioo",   (SUBR)tabaudioi, NULL },
   { "ftaudio.k",     S(TABAUDIOK), TR,  "k", "kkSkpOO",
     (SUBR)tabaudiok_init, (SUBR)tabaudiok },
  };

LINKAGE_BUILTIN(tabaudio_localops)
