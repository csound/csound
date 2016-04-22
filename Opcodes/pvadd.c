/*
    pvadd.c:

    Copyright (C) 1998 Richard Karpen

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

/******************************************/
/* The applications in this file were     */
/* designed and coded by Richard Karpen   */
/* University of Washington, Seattle 1998 */
/******************************************/
/*    PVADD.C        */

#include "pvoc.h"
#include <math.h>

static int pvx_loadfile(CSOUND *csound, const char *fname, PVADD *p);

/* This is used in pvadd instead of the Fetch() from dsputil.c */
void FetchInForAdd(float *inp, MYFLT *buf, int32 fsize,
                   MYFLT pos, int binoffset, int maxbin, int binincr)
{
    int32    j;
    float   *frame0, *frame1;
    int32    base;
    MYFLT   frac;

    base = (int32)pos;
    frac = pos - (MYFLT)base;
    /* & how close to get to next */
    frame0 = inp + ((int32)fsize+2L)*base;
    frame1 = frame0 + ((int32)fsize+2L);
    if (frac != FL(0.0)) {
      for (j = binoffset; j < maxbin; j+=binincr) {
        buf[2L*j] = frame0[2L*j] + frac*(frame1[2L*j]-frame0[2L*j]);
        buf[2L*j+1L] = frame0[2L*j+1L]
          + frac*(frame1[2L*j+1L]-frame0[2L*j+1L]);
      }
    }
    else {
      for (j = binoffset; j < maxbin; j+=binincr) {
        buf[2L*j] = frame0[2L*j];
        buf[2L*j+1] = frame0[2L*j+1L];
      }
    }
}

int pvaddset_(CSOUND *csound, PVADD *p, int stringname)
{
    int      ibins;
    char     pvfilnam[MAXNAME];
    int      size;
    FUNC     *ftp = NULL, *AmpGateFunc = NULL;
    int32     memsize;

    if (*p->ifn > FL(0.0))
      if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
        return NOTOK;
    p->ftp = ftp;

    if (*p->igatefun > FL(0.0))
      if (UNLIKELY((AmpGateFunc = csound->FTnp2Find(csound, p->igatefun)) == NULL))
        return NOTOK;
    p->AmpGateFunc = AmpGateFunc;

    if (stringname==0){
      if (csound->ISSTRCOD(*p->ifilno))
        strncpy(pvfilnam,get_arg_string(csound, *p->ifilno), MAXNAME-1);
      else csound->strarg2name(csound, pvfilnam, p->ifilno, "pvoc.",0);
    }
    else strncpy(pvfilnam, ((STRINGDAT *)p->ifilno)->data, MAXNAME-1);

    if (UNLIKELY(pvx_loadfile(csound, pvfilnam, p) != OK))
      return NOTOK;

    memsize = (int32) (MAXBINS + PVFFTSIZE + PVFFTSIZE);
    if (*p->imode == 1 || *p->imode == 2) {
      int32  n= (int32) ((p->frSiz + 2L) * (p->maxFr + 2L));
#ifdef USE_DOUBLE
      n = (n + 1L) * (int32) sizeof(float) / (int32) sizeof(double);
#endif
      memsize += n;
    }

    if (p->auxch.auxp == NULL || memsize != p->mems) {
      MYFLT *fltp;
      csound->AuxAlloc(csound, (memsize * sizeof(MYFLT)), &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->oscphase = fltp;
      fltp += MAXBINS;
      p->buf = fltp;
      if (*p->imode == 1 || *p->imode == 2) {
        fltp += PVFFTSIZE * 2;
        p->pvcopy = (float*) ((void*) fltp);
      }
    }
    p->mems = memsize;

    size = pvfrsiz(p);
    p->prFlg = 1;    /* true */

   if (*p->igatefun > 0)
     p->PvMaxAmp = PvocMaxAmp(p->frPtr, size, p->maxFr);

   if (*p->imode == 1 || *p->imode == 2) {
     SpectralExtract(p->frPtr, p->pvcopy, size, p->maxFr,
                     (int) *p->imode, *p->ifreqlim);
     p->frPtr = (float*) p->pvcopy;
   }

    memset(p->oscphase, 0, MAXBINS*sizeof(MYFLT));

    ibins = (*p->ibins <= FL(0.0) ? (size / 2) : (int) *p->ibins);
    p->maxbin = ibins + (int) *p->ibinoffset;
    p->maxbin = (p->maxbin > (size / 2) ? (size / 2) : p->maxbin);

    return OK;
}

int pvadd(CSOUND *csound, PVADD *p)
{
    MYFLT   *ar, *ftab;
    MYFLT   frIndx;
    int     size = pvfrsiz(p);
    int     i, binincr = (int) *p->ibinincr;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   amp, frq, v1, fract, *oscphase;
    int32    phase, incr;
    FUNC    *ftp;
    int32    lobits;

    if (UNLIKELY(p->auxch.auxp == NULL)) goto err1;
    ftp = p->ftp;
    if (UNLIKELY((frIndx = *p->ktimpnt * p->frPrtim) < 0)) goto err2;

    if (frIndx > p->maxFr) { /* not past last one */
      frIndx = (MYFLT) p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;   /* false */
        csound->Warning(csound, Str("PVADD ktimpnt truncated to last frame"));
      }
    }
    FetchInForAdd(p->frPtr, p->buf, size, frIndx,
                  (int) *p->ibinoffset, p->maxbin, binincr);

    if (*p->igatefun > 0)
      PvAmpGate(p->buf, p->maxbin*2, p->AmpGateFunc, p->PvMaxAmp);

    ar = p->rslt;
    memset(ar, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;
    oscphase = p->oscphase;
    for (i = (int) *p->ibinoffset; i < p->maxbin; i += binincr) {
      lobits = ftp->lobits;
      phase = (int32) *oscphase;
      frq = p->buf[i * 2 + 1] * *p->kfmod;
      if (p->buf[i * 2 + 1] == FL(0.0) || frq >= CS_ESR * FL(0.5)) {
        incr = 0;               /* Hope then does not matter */
        amp = FL(0.0);
      }
      else {
        MYFLT tmp = frq * csound->sicvt;
        incr = (int32) MYFLT2LONG(tmp);
        amp = p->buf[i * 2];
      }
      for (n=offset;n<nsmps;n++) {
        fract = PFRAC(phase);
        ftab = ftp->ftable + (phase >> lobits);
        v1 = *ftab++;
        ar[n] += (v1 + (*ftab - v1) * fract) * amp;
        phase += incr;
        phase &= PHMASK;
      }
      *oscphase = (MYFLT) phase;
      oscphase++;
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead, Str("pvadd: not initialised"));
 err2:
    return csound->PerfError(csound, p->h.insdshead, Str("PVADD timpnt < 0"));
}

int pvaddset(CSOUND *csound, PVADD *p){
    return pvaddset_(csound, p, 0);
}

int pvaddset_S(CSOUND *csound, PVADD *p){
    return pvaddset_(csound, p, 1);
}


static int pvx_loadfile(CSOUND *csound, const char *fname, PVADD *p)
{
    PVOCEX_MEMFILE  pp;

    if (UNLIKELY(csound->PVOCEX_LoadFile(csound, fname, &pp) != 0)) {
      return csound->InitError(csound, Str("PVADD cannot load %s"), fname);
    }
    /* fft size must be <= PVFRAMSIZE (=8192) for Csound */
    if (UNLIKELY(pp.fftsize > PVFRAMSIZE)) {
      return csound->InitError(csound, Str("pvoc-ex file %s: "
                                           "FFT size %d too large for Csound"),
                               fname, (int) pp.fftsize);
    }
    if (UNLIKELY(pp.fftsize < 128)) {
      return csound->InitError(csound, Str("PV frame %ld seems too small in %s"),
                               (int32) pp.fftsize, fname);
    }
    /* have to reject m/c files for now, until opcodes upgraded */
    if (UNLIKELY(pp.chans > 1)) {
      return csound->InitError(csound, Str("pvoc-ex file %s is not mono"), fname);
    }
    /* ignore the window spec until we can use it! */
    p->frSiz    = pp.fftsize;
    p->frPtr    = (float*) pp.data;
    p->maxFr    = pp.nframes - 1;
    p->asr      = pp.srate;
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */
    p->frPrtim = CS_ESR / (MYFLT) pp.overlap;
    return OK;
}

