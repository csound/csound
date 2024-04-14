/*
  pvread.c:

  Copyright (C) 1992, 2000 Richard Karpen, Richard Dobson

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

/**************************************************************/
/***********pvread ********************************************/
/*** By Richard Karpen - July-October 1992*********************/
/**************************************************************/

#include "pvoc.h"
#include <math.h>

/*RWD 10:9:2000 read pvocex file format */
#include "pvfileio.h"
static int32_t pvocex_loadfile(CSOUND *, const char *fname, PVREAD *p);

#define WLN   1         /* time window is WLN*2*ksmps long */
#define OPWLEN (2*WLN*ksmps)    /* manifest used for final time wdw */

static void FetchInOne(
                       float   *inp,       /* pointer to input data */
                       MYFLT   *buf,       /* where to put our nice mag/pha pairs */
                       int32    fsize,      /* frame size we're working with */
                       MYFLT   pos,        /* fractional frame we want */
                       int32    mybin)
{
  float   *frame0;
  float   *frame1;
  int32    base;
  MYFLT   frac;
  int32    twmybin = mybin+mybin;  /* Always used thus */

  /***** WITHOUT INFO ON WHERE LAST FRAME IS, MAY 'INTERP' BEYOND IT ****/
  base = (int32)pos;               /* index of basis frame of interpolation */
  frac = ((MYFLT)(pos - (MYFLT)base));
  /* & how close to get to next */
  frame0 = inp + ((int32) fsize + 2L) * base + twmybin;
  frame1 = frame0 + ((int32) fsize + 2L);      /* addresses of both frames */
  if (frac != 0.0) {  /* must have 2 cases to avoid possible segmentation */
    /* violations and failed computes, else may interp  */
    /* beyond valid data */
    buf[0] = frame0[0] + frac * (frame1[0] - frame0[0]);
    buf[1] = frame0[1] + frac * (frame1[1] - frame0[1]);
  }
  else {
    /* frac is 0.0 i.e. just copy the source frame */
    buf[0] = frame0[0];
    buf[1] = frame0[1];
  }
}

int32_t pvreadset_(CSOUND *csound, PVREAD *p, int32_t stringname)
{
  char      pvfilnam[256];

  if (stringname==0){
    if (IsStringCode(*p->ifilno))
      strncpy(pvfilnam,csound->GetString(csound, *p->ifilno), MAXNAME);
    else csound->StringArg2Name(csound, pvfilnam, p->ifilno, "pvoc.",0);
  }
  else strncpy(pvfilnam, ((STRINGDAT *)p->ifilno)->data, MAXNAME);

  if (pvocex_loadfile(csound, pvfilnam, p) == OK) {
    p->prFlg = 1;
    p->mybin = MYFLT2LRND(*p->ibin);
    return OK;
  }
  return NOTOK;
}

int32_t pvreadset(CSOUND *csound, PVREAD *p){
  return pvreadset_(csound,p,0);
}

int32_t pvreadset_S(CSOUND *csound, PVREAD *p){
  return pvreadset_(csound,p,1);
}

int32_t pvread(CSOUND *csound, PVREAD *p)
{
  MYFLT  frIndx;
  MYFLT  buf[2];
  int32_t    size = pvfrsiz(p);

  if (UNLIKELY((frIndx = *p->ktimpnt * p->frPrtim) < 0)) goto err1;
  if (frIndx > p->maxFr) {  /* not past last one */
    frIndx = (MYFLT)p->maxFr;
    if (p->prFlg) {
      p->prFlg = 0;   /* false */
      csound->Warning(csound, "%s", Str("PVOC ktimpnt truncated to last frame"));
    }
  }
  FetchInOne(p->frPtr, &(buf[0]), size, frIndx, p->mybin);
  *p->kfreq = buf[1];
  *p->kamp = buf[0];
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h), "%s", Str("PVOC timpnt < 0"));
}

static int32_t pvocex_loadfile(CSOUND *csound, const char *fname, PVREAD *p)
{
  PVOCEX_MEMFILE  pp;

  if (UNLIKELY(csound->PVOCEX_LoadFile(csound, fname, &pp) != 0)) {
    return csound->InitError(csound, Str("PVREAD cannot load %s"), fname);
  }
  /* have to reject m/c files for now, until opcodes upgraded */
  if (UNLIKELY(pp.chans > 1)) {
    return csound->InitError(csound, Str("pvoc-ex file %s is not mono"), fname);
  }
  /* ignore the window spec until we can use it! */
  p->frSiz    = pp.fftsize;
  p->frPtr    = (float*) pp.data;
  p->baseFr   = 0;  /* point to first data frame */
  p->maxFr    = pp.nframes - 1;
  p->asr      = pp.srate;
  /* highest possible frame index */
  /* factor by which to mult expand phase diffs (ratio of samp spacings) */
  p->frPrtim = CS_ESR / ((MYFLT) pp.overlap);
  return OK;
}

