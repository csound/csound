/*
    Copyright (c) Victor Lazzarini, 2007

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

#include "pvs_ops.h"
#include "interlocks.h"

#include "pstream.h"

typedef struct {
  PVSDAT  header;
  float   *data;
  uint32  frames;
} FSIG_HANDLE;

typedef struct {
  OPDS h;
  MYFLT  *hptr;
  MYFLT  *ktime;
  PVSDAT *fin;
  MYFLT  *len;
  MYFLT  pos;
  uint32 nframes;
  uint32 cframes;
  AUXCH handmem;
  FSIG_HANDLE *handle;
  AUXCH  buffer;
  uint32 lastframe;
} PVSBUFFER;

static int32_t pvsbufferset(CSOUND *csound, PVSBUFFER *p)
{
    int32_t N, hop, i=0;
    char varname[32] = "::buffer0";
    FSIG_HANDLE **phandle = NULL;

    if (UNLIKELY(p->fin->sliding))
      return csound->InitError(csound, "%s", Str("SDFT case not implemented yet"));
    N = p->fin->N;
    hop = p->fin->overlap;
    if (UNLIKELY(N < 2 || N > INT32_MAX - 2 || (N & 1) || hop <= 0 ||
                 (uint64_t) N + 2 > SIZE_MAX / sizeof(float)))
      return csound->InitError(csound, "%s", Str("pvsbuffer: invalid frame size"));
    double frames = (double) *p->len * CS_ESR / hop;
    size_t frameBytes = ((size_t) N + 2) * sizeof(float);
    if (UNLIKELY(!(frames >= 1 && frames <= UINT32_MAX &&
                   frames <= SIZE_MAX / frameBytes)))
      return csound->InitError(csound, "%s", Str("pvsbuffer: invalid buffer length"));
    p->nframes = (uint32_t) frames;
    size_t bytes = frameBytes * p->nframes;
    if (p->handmem.auxp == NULL)
      csound->AuxAlloc(csound, sizeof(FSIG_HANDLE), &p->handmem);
    p->handle = (FSIG_HANDLE *) p->handmem.auxp;
    p->handle->header.N = N = p->fin->N;
    p->handle->header.overlap = hop = p->fin->overlap;
    p->handle->header.winsize = p->fin->winsize;
    p->handle->header.wintype = p->fin->wintype;
    p->handle->header.format  = p->fin->format;
    p->handle->header.framecount = p->fin->framecount;
    p->handle->frames = p->nframes;
    if (p->buffer.auxp == NULL ||
        p->buffer.size < bytes)
      csound->AuxAlloc(csound, bytes, &p->buffer);
    else
      memset(p->buffer.auxp, 0, bytes);

    p->handle->header.frame.auxp = p->buffer.auxp;
    p->handle->header.frame.size = p->buffer.size;
    p->handle->data = (float *)  p->buffer.auxp;


    while ((phandle = (FSIG_HANDLE **)csound->QueryGlobalVariable(csound,varname))
          != NULL)
      if (p->handle == *phandle) break;
      else snprintf(varname, 32, "::buffer%d", ++i);

    if (phandle == NULL) {
     csound->CreateGlobalVariable(csound, varname, sizeof(FSIG_HANDLE *));
     phandle = (FSIG_HANDLE **) csound->QueryGlobalVariable(csound,varname);
     /*csound->Message(csound, "%p -> %p \n", p->handle, phandle); */
    if (phandle == NULL)
      return
        csound->InitError(csound,
                          "%s", Str("error... could not create global var for handle\n"));
    else
      *phandle = p->handle;
     }
    *p->hptr = (MYFLT) i;

    p->lastframe = 0;
    p->cframes = 0;
    *p->ktime = p->pos = FL(0.0);
    return OK;
}

static int32_t pvsbufferproc(CSOUND *csound, PVSBUFFER *p)
{
    if (UNLIKELY(p->fin->N != p->handle->header.N ||
                 p->fin->overlap != p->handle->header.overlap ||
                 p->fin->format != p->handle->header.format || p->fin->sliding))
      return csound->PerfError(csound, &p->h, "%s",
                              Str("pvsbuffer: input format changed"));
     float *fin = p->fin->frame.auxp;

    if (p->lastframe < p->fin->framecount) {
      int32 framesize = p->fin->N + 2, i;
      float *fout = (float *) p->buffer.auxp;
      fout += (size_t) framesize*p->cframes;
      for (i=0;i < framesize; i+=2) {
        fout[i] = fin[i];
        fout[i+1] = fin[i+1];
      }
      p->handle->header.framecount = p->lastframe = p->fin->framecount;
      p->pos = p->cframes/(CS_ESR/p->fin->overlap);
      p->cframes++;
      if (p->cframes == p->nframes)p->cframes = 0;
    }
    *p->ktime = p->pos;

    return OK;
}


typedef struct {
  OPDS h;
  PVSDAT *fout;
  MYFLT  *ktime;
  MYFLT *hptr;
  MYFLT *strt;
  MYFLT *end;
  MYFLT *clear;
  MYFLT optr;
  FSIG_HANDLE *handle;
  uint32_t scnt;
} PVSBUFFERREAD;

static FSIG_HANDLE *pvsbuffer_handle(CSOUND *csound, MYFLT number)
{
    char varname[32];
    if (!((double) number >= 0 && (double) number <= INT32_MAX))
      return NULL;
    snprintf(varname, sizeof(varname), "::buffer%d", (int32_t) number);
    FSIG_HANDLE **handle = (FSIG_HANDLE **)
      csound->QueryGlobalVariable(csound, varname);
    return handle != NULL ? *handle : NULL;
}

/* Normalize frame positions in bounded time, including one-frame buffers. */
static inline double pvsbuffer_position(double pos, uint32_t frames)
{
    if (pos >= 0 && pos < frames) return pos;
    pos = fmod(pos, frames);
    if (pos < 0) pos += frames;
    if (pos >= frames) pos = 0;
    return pos;
}

static int32_t pvsbufreadset(CSOUND *csound, PVSBUFFERREAD *p)
{
    FSIG_HANDLE *handle = pvsbuffer_handle(csound, *p->hptr);
    if (UNLIKELY(handle == NULL))
      return csound->InitError(csound, "%s", Str("Invalid buffer handle"));
    int32_t N = handle->header.N;
    p->fout->N = N;
    p->fout->overlap = handle->header.overlap;
    p->fout->winsize = handle->header.winsize;
    p->fout->wintype = handle->header.wintype;
    p->fout->format = handle->header.format;
    p->fout->framecount = 1;
    size_t bytes = ((size_t) N + 2) * sizeof(float);
    if (p->fout->frame.auxp == NULL || p->fout->frame.size < bytes)
      csound->AuxAlloc(csound, bytes, &p->fout->frame);
    else
      memset(p->fout->frame.auxp, 0, bytes);
    p->fout->sliding = 0;
    p->scnt = p->fout->overlap;
    p->handle = handle;
    p->optr = *p->hptr;
    return OK;
}

static int32_t pvsbufread_handle(CSOUND *csound, PVSBUFFERREAD *p)
{
    if (*p->hptr != p->optr) {
      FSIG_HANDLE *handle = pvsbuffer_handle(csound, *p->hptr);
      if (UNLIKELY(handle == NULL))
        return csound->PerfError(csound, &p->h, "%s", Str("Invalid buffer handle"));
      p->handle = handle;
      p->optr = *p->hptr;
    }
    return OK;
}

static int32_t pvsbufreadproc(CSOUND *csound, PVSBUFFERREAD *p)
{
    if (UNLIKELY(pvsbufread_handle(csound, p) != OK)) return NOTOK;
    FSIG_HANDLE *handle = p->handle;
    int32_t N = p->fout->N;
    uint32_t overlap = p->fout->overlap;
    float *fout = (float *) p->fout->frame.auxp;
    if (p->scnt >= overlap) {
      if (N == handle->header.N && overlap == (uint32_t) handle->header.overlap &&
          p->fout->format == handle->header.format) {
        double lo = (double) *p->strt * N / CS_ESR;
        double hi = (double) *p->end * N / CS_ESR;
        int32_t first = !(lo > 0) ? 0 : (lo >= N/2 ? N/2 : (int32_t) lo);
        int32_t last = !(hi > first) || hi >= N/2 ? N/2 : (int32_t) hi;
        double pos = pvsbuffer_position((double) *p->ktime * CS_ESR / overlap,
                                       handle->frames);
        if (UNLIKELY(!(pos >= 0)))
          return csound->PerfError(csound, &p->h, "%s", Str("Invalid buffer position"));
        uint32_t index = (uint32_t) pos;
        uint32_t next = index + 1 == handle->frames ? 0 : index + 1;
        float *frame1 = handle->data + ((size_t) N + 2) * index;
        float *frame2 = handle->data + ((size_t) N + 2) * next;
        MYFLT frac = pos - index;
        if (*p->clear != FL(0)) memset(fout, 0, ((size_t) N + 2) * sizeof(float));
        /* Each bin contains an amplitude and a frequency (or phase). */
        for (int32_t i = first * 2; i <= last * 2; i += 2) {
          fout[i] = frame1[i] + frac * (frame2[i] - frame1[i]);
          fout[i+1] = frame1[i+1] + frac * (frame2[i+1] - frame1[i+1]);
        }
      } else {
        memset(fout, 0, ((size_t) N + 2) * sizeof(float));
      }
      p->scnt -= overlap;
      p->fout->framecount++;
    }
    p->scnt += CS_KSMPS;
    return OK;
}

static int32_t pvsbufreadproc2(CSOUND *csound, PVSBUFFERREAD *p)
{
    if (UNLIKELY(pvsbufread_handle(csound, p) != OK)) return NOTOK;
    FSIG_HANDLE *handle = p->handle;
    int32_t N = p->fout->N;
    uint32_t overlap = p->fout->overlap;
    float *fout = (float *) p->fout->frame.auxp;
    if (p->scnt >= overlap) {
      FUNC *ftab1 = csound->FTFind(csound, p->strt);
      if (UNLIKELY(ftab1 == NULL)) return NOTOK;
      FUNC *ftab2 = csound->FTFind(csound, p->end);
      if (UNLIKELY(ftab2 == NULL)) return NOTOK;
      if (UNLIKELY(ftab1->flen < (uint32_t) N/2+1 ||
                   ftab2->flen < (uint32_t) N/2+1))
        return csound->PerfError(csound, &p->h, "%s", Str("pvsbufread2: delay table too short"));
      if (N == handle->header.N && overlap == (uint32_t) handle->header.overlap &&
          p->fout->format == handle->header.format) {
        for (int32_t i = 0; i < N+2; i++) {
          MYFLT *tab = (i & 1) ? ftab2->ftable : ftab1->ftable;
          double pos = pvsbuffer_position(((double) *p->ktime - tab[i/2]) *
                                         CS_ESR / overlap, handle->frames);
          if (UNLIKELY(!(pos >= 0)))
            return csound->PerfError(csound, &p->h, "%s", Str("Invalid buffer position"));
          uint32_t index = (uint32_t) pos;
          uint32_t next = index + 1 == handle->frames ? 0 : index + 1;
          float *frame1 = handle->data + ((size_t) N + 2) * index;
          float *frame2 = handle->data + ((size_t) N + 2) * next;
          MYFLT frac = pos - index;
          fout[i] = frame1[i] + frac * (frame2[i] - frame1[i]);
        }
      } else {
        memset(fout, 0, ((size_t) N + 2) * sizeof(float));
      }
      p->scnt -= overlap;
      p->fout->framecount++;
    }
    p->scnt += CS_KSMPS;
    return OK;
}

#define S(x)    sizeof(x)

/* static */
static OENTRY pvsbuffer_localops[] = {
  {"pvsbuffer", S(PVSBUFFER), 0,  "ik", "fi",
   (SUBR)pvsbufferset, (SUBR)pvsbufferproc, NULL},
  {"pvsbufread", S(PVSBUFFERREAD), 0,  "f", "kkOOo",
   (SUBR)pvsbufreadset, (SUBR)pvsbufreadproc, NULL},
  {"pvsbufread2", S(PVSBUFFERREAD), 0,  "f", "kkkk",
   (SUBR)pvsbufreadset, (SUBR)pvsbufreadproc2, NULL}
};

int32_t pvsbuffer_localops_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(pvsbuffer_localops[0]),
                               (int32_t) (sizeof(pvsbuffer_localops) / sizeof(OENTRY)));
}
/* LINKAGE */
