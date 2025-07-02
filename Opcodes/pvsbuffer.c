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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
    if (p->handmem.auxp == NULL)
      csound->AuxAlloc(csound, sizeof(FSIG_HANDLE), &p->handmem);
    p->handle = (FSIG_HANDLE *) p->handmem.auxp;
    p->handle->header.N = N = p->fin->N;
    p->handle->header.overlap = hop = p->fin->overlap;
    p->handle->header.winsize = p->fin->winsize;
    p->handle->header.wintype = p->fin->wintype;
    p->handle->header.format  = p->fin->format;
    p->handle->header.framecount = p->fin->framecount;
    p->nframes = p->handle->frames = (*p->len) * CS_ESR/hop;
    if (p->buffer.auxp == NULL ||
        p->buffer.size < sizeof(float) * (N + 2) * p->nframes)
      csound->AuxAlloc(csound, (N + 2) * sizeof(float) * p->nframes, &p->buffer);
    else
      memset(p->buffer.auxp, 0, (N + 2) * sizeof(float) * p->nframes);

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
     float *fin = p->fin->frame.auxp;

    if (p->lastframe < p->fin->framecount) {
      int32 framesize = p->fin->N + 2, i;
      float *fout = (float *) p->buffer.auxp;
      fout += framesize*p->cframes;
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
  MYFLT iclear, optr;
  FSIG_HANDLE *handle;
  uint32_t scnt;
} PVSBUFFERREAD;

static int32_t pvsbufreadset(CSOUND *csound, PVSBUFFERREAD *p)
{
    int32_t N;
    FSIG_HANDLE *handle=NULL, **phandle;
    char varname[32];

    snprintf(varname, 32, "::buffer%d", (int32_t)(*p->hptr));
    /* csound->Message(csound, "%s:\n", varname); */
    phandle = (FSIG_HANDLE **) csound->QueryGlobalVariable(csound,varname);
    if (phandle == NULL)
      return csound->InitError(csound,
                               "%s", Str("error... could not read handle from "
                                   "global variable\n"));
    else
      handle = *phandle;
    p->optr = *p->hptr;

    if (handle != NULL) {
      p->fout->N = N = handle->header.N;
      p->fout->overlap = handle->header.overlap;
      p->fout->winsize = handle->header.winsize;
      p->fout->wintype = handle->header.wintype;
      p->fout->format  = handle->header.format;
      p->fout->framecount = 1;
    }
    else {
      p->fout->N = N = 1024;
      p->fout->overlap = 256;
      p->fout->winsize = 1024;
      p->fout->wintype = 1;
      p->fout->format  = PVS_AMP_FREQ;
      p->fout->framecount = 1;
    }

    if (p->fout->frame.auxp == NULL ||
         p->fout->frame.size < sizeof(float) * (N + 2))
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);

    p->fout->sliding = 0;
    p->scnt = p->fout->overlap;
    p->handle = handle;
    return OK;
}

 static int32_t pvsbufreadproc(CSOUND *csound, PVSBUFFERREAD *p){

    uint32_t posi, frames;
    MYFLT pos, sr = CS_ESR, frac;
    FSIG_HANDLE *handle =  p->handle, **phandle;
    float *fout, *buffer;
    int32_t strt = *p->strt, end = *p->end, i, N;
    uint32_t overlap;
    p->iclear = *p->clear;

   if (*p->hptr != p->optr) {
     char varname[32];
     snprintf(varname, 32, "::buffer%d", (int32_t)(*p->hptr));
     phandle = (FSIG_HANDLE **) csound->QueryGlobalVariable(csound,varname);
     if (phandle == NULL)
       csound->PerfError(csound, &(p->h),
                         "%s", Str("error... could not read handle "
                             "from global variable\n"));
     else
       handle = *phandle;
   }

   if (handle == NULL) goto err1;

   fout = (float *) p->fout->frame.auxp,
     buffer = handle->data;
   N = p->fout->N;
   overlap = p->fout->overlap;

   if (p->scnt >= overlap){
     float *frame1, *frame2;
     strt /= (sr/N);
     end /= (sr/N);
     strt = (int32_t)(strt < 0 ? 0 : strt > N/2 ? N/2 : strt);
     end = (int32_t)(end <= strt ? N/2 + 2 : end > N/2 + 2 ? N/2 + 2 : end);
     frames = handle->frames-1;
     pos = *p->ktime*(sr/overlap);

     if (p->iclear) memset(fout, 0, sizeof(float)*(N+2));
     while (pos >= frames) pos -= frames;
     while (pos < 0) pos += frames;
     posi = (int32_t) pos;
     if (N == handle->header.N &&
         overlap == (uint32_t)handle->header.overlap){
       frame1 = buffer + (N + 2) * posi;
       frame2 = buffer + (N + 2)*(posi != frames-1 ? posi+1 : 0);
       frac = pos - posi;

       for (i=strt; i < end; i+=2){
         fout[i] = frame1[i] + frac*(frame2[i] - frame1[i]);
         fout[i+1] = frame1[i+1] + frac*(frame2[i+1] - frame1[i+1]);
       }
     }
     else
       for (i=0; i < N+2; i+=2){
         fout[i] = 0.0f;
         fout[i+1] = 0.0f;
       }
     p->scnt -= overlap;
     p->fout->framecount++;
   }
   p->scnt += CS_KSMPS;

   return OK;
 err1:
   return csound->PerfError(csound, &(p->h),
                             "%s", Str("Invalid buffer handle"));
  }


static int32_t pvsbufreadproc2(CSOUND *csound, PVSBUFFERREAD *p)
{
    uint32_t posi, frames;
    MYFLT pos, sr = CS_ESR;
    FSIG_HANDLE *handle =  p->handle, **phandle;
    MYFLT    frac, *tab1, *tab2, *tab;
    FUNC     *ftab;
    float    *fout, *buffer;
    uint32_t overlap, i;
    int32_t      N;

    if (*p->hptr != p->optr){
      char varname[32];
      snprintf(varname, 32, "::buffer%d", (int32_t)(*p->hptr));
      phandle = (FSIG_HANDLE **) csound->QueryGlobalVariable(csound,varname);
      if (phandle == NULL)
        csound->PerfError(csound, &(p->h),
                          "%s", Str("error... could not read handle from "
                              "global variable\n"));
      else
        handle = *phandle;
    }

    if (UNLIKELY(handle == NULL)) goto err1;
    fout = (float *) p->fout->frame.auxp,
    buffer = handle->data;
    N = p->fout->N;
    overlap = p->fout->overlap;
    if (p->scnt >= overlap) {
      float *frame1, *frame2;
      frames = handle->frames-1;
      ftab = csound->FTFind(csound, p->strt);
      if (UNLIKELY((int32_t)ftab->flen < N/2+1))
        csound->PerfError(csound, &(p->h),
                          Str("table length too small: needed %d, got %d\n"),
                          N/2+1, ftab->flen);
      tab = tab1 = ftab->ftable;
      ftab = csound->FTFind(csound, p->end);
      if (UNLIKELY((int32_t)ftab->flen < N/2+1))
        csound->PerfError(csound, &(p->h),
                          Str("table length too small: needed %d, got %d\n"),
                          N/2+1, ftab->flen);
      tab2 = ftab->ftable;
      for (i=0; i < (uint32_t)N+2; i++){
        pos = (*p->ktime - tab[i/2])*(sr/overlap);
           while(pos >= frames) {
             pos -= frames;
           }
           while(pos < 0){
             pos += frames;
           }
           posi = (int32_t) pos;
        if (N == handle->header.N &&
            overlap == (uint32_t)handle->header.overlap) {
           frame1 = buffer + (N + 2) * posi;
           frame2 = buffer + (N + 2)*(posi != frames-1 ? posi+1 : 0);
           frac = pos - posi;
           fout[i] = frame1[i] + frac*(frame2[i] - frame1[i]);
        } else
          fout[i] = 0.0f;
        if (tab == tab1) tab = tab2;
          else tab = tab1;
      }
      p->scnt -= overlap;
      p->fout->framecount++;
    }
    p->scnt += CS_KSMPS;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("Invalid buffer handle"));
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
