/*  Copyright (C) 2002-2004 Gabriel Maldonado */
/*                2016 John ffitch (array changed) */

/*  The gab library is free software; you can redistribute it */
/*  and/or modify it under the terms of the GNU Lesser General Public */
/*  License as published by the Free Software Foundation; either */
/*  version 2.1 of the License, or (at your option) any later version. */

/*  The gab library is distributed in the hope that it will be useful, */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/*  GNU Lesser General Public License for more details. */

/*  You should have received a copy of the GNU Lesser General Public */
/*  License along with the gab library; if not, write to the Free Software */
/*  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA */
/*  02110-1301 USA */

/* Ported to csound5 by: Andres Cabrera */
/* This file includes the opcodes from newopcodes.c */
/* TODO: Check if the authors (Peter Neubaeker and Jens Groh) are correct */

/*printi removed? I can't find the corresponding OENTRY- */
/*Left them commented out */
/*how should exitnow be safely implemented? */
/*Check the zak opcodes */
/*changed some comments to c-style */
/*Check csystem, exitnow */
/*Check other opcodes commented out in Oentry */


#include "stdopcod.h"
#include "gab.h"
#include <math.h>
#include "interlocks.h"

#define FLT_MAX ((MYFLT)0x7fffffff)

static int32_t krsnsetx(CSOUND *csound, KRESONX *p)
  /* Gabriel Maldonado, modified for arb order  */
{
    int32_t scale;
    p->scale = scale = (int32_t) *p->iscl;
    if (UNLIKELY((p->loop = MYFLT2LRND(*p->ord)) < 1))
      p->loop = 4; /*default value*/
    if (!*p->istor && (p->aux.auxp == NULL ||
                       (uint32_t)(p->loop*2*sizeof(MYFLT)) > p->aux.size))
      csound->AuxAlloc(csound, (int64_t)(p->loop*2*sizeof(MYFLT)), &p->aux);
    p->yt1 = (MYFLT*)p->aux.auxp; p->yt2 = (MYFLT*)p->aux.auxp + p->loop;
    if (UNLIKELY(scale && scale != 1 && scale != 2)) {
      return csound->InitError(csound,Str("illegal reson iscl value, %f"),
                               *p->iscl);
    }
    if (LIKELY(!(*p->istor))) {
      memset(p->yt1, 0, p->loop*sizeof(MYFLT));
      memset(p->yt2, 0, p->loop*sizeof(MYFLT));
    }
    p->prvcf = p->prvbw = -FL(100.0);
    return OK;
}

static int32_t kresonx(CSOUND *csound, KRESONX *p) /* Gabriel Maldonado, modified */
{
    int32_t flag = 0, j;
    MYFLT       *ar, *asig;
    MYFLT       c3p1, c3t4, omc3, c2sqr;
    MYFLT *yt1, *yt2, c1,c2,c3;

    if (*p->kcf != p->prvcf) {
      p->prvcf = *p->kcf;
      p->cosf = COS(*p->kcf * csound->tpidsr * CS_KSMPS);
      flag = 1;
    }
    if (*p->kbw != p->prvbw) {
      p->prvbw = *p->kbw;
      p->c3 = EXP(*p->kbw * csound->mtpdsr * CS_KSMPS);
      flag = 1;
    }
    if (flag) {
      c3p1 = p->c3 + FL(1.0);
      c3t4 = p->c3 * FL(4.0);
      omc3 = FL(1.0)- p->c3;
      p->c2 = c3t4 * p->cosf / c3p1;            /* -B, so + below */
      c2sqr = p->c2 * p->c2;
      if (p->scale == 1)
        p->c1 = omc3 * SQRT(FL(1.0) - (c2sqr / c3t4));
      else if (p->scale == 2)
        p->c1 = SQRT((c3p1*c3p1-c2sqr) * omc3/c3p1);
      else p->c1 = FL(1.0);
    }
    c1   = p->c1;
    c2   = p->c2;
    c3   = p->c3;
    yt1  = p->yt1;
    yt2  = p->yt2;
    asig = p->asig;
    ar   = p->ar;
    for (j=0; j< p->loop; j++) {
      *ar = c1 * *asig + c2 * yt1[j] - c3 * yt2[j];
      yt2[j] = yt1[j];
      yt1[j] = *ar;
      asig= p->ar;
    }
    return OK;
}

/* /////////////////////////////////////////// */

static int32_t fastab_set(CSOUND *csound, FASTAB *p)
{
    FUNC *ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL)) {
      return csound->InitError(csound, Str("fastab: incorrect table number"));
    }
    p->table = ftp->ftable;
    p->tablen = ftp->flen;
    p->xmode = (int32_t) *p->ixmode;
    if (p->xmode)
      p->xbmul = (MYFLT) p->tablen /*- FL(0.001)*/;
    else
      p->xbmul = FL(1.0);
    return OK;
}

static int32_t fastabw(CSOUND *csound, FASTAB *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    FUNC *ftp = csound->FTnp2Finde(csound, p->xfn);
    p->table = ftp->ftable;
    MYFLT *tab = p->table;
    MYFLT *rslt = p->rslt, *ndx = p->xndx;


    if (UNLIKELY(early)) nsmps -= early;
    if (p->xmode) {
      MYFLT xbmul = p->xbmul;   /* load once */
      int32_t len = p->tablen;
      for (n=offset; n<nsmps; n++)  { /* for loops compile better */
        int32_t i = (int32_t)MYFLT2LRND(ndx[n]*xbmul);
        if (UNLIKELY(i > len || i<0)) {
          csound->Message(csound, "ndx: %f\n", ndx[n]);
          return csound->PerfError(csound, &(p->h), Str("tabw off end"));
        }
        tab[i] = rslt[n];
      }
    }
    else {
      int32_t len = p->tablen;
      for (n=offset; n<nsmps; n++) {
        int32_t i = MYFLT2LRND(ndx[n]);
        if (UNLIKELY(i > len || i<0)) {
          return csound->PerfError(csound, &(p->h), Str("tabw off end"));
        }
        tab[i] = rslt[n];
      }
    }
    return OK;
}

static int32_t fastabk(CSOUND *csound, FASTAB *p)
{
    int32_t i;
    if (p->xmode)
      i = (int32_t) MYFLT2LRND(*p->xndx * p->xbmul);
    else
      i = (int32_t) MYFLT2LRND(*p->xndx);
    if (UNLIKELY(i > p->tablen || i<0)) {
      return csound->PerfError(csound, &(p->h), Str("tab off end %i"), i);
    }
    *p->rslt =  p->table[i];
    return OK;
}

static int32_t fastabkw(CSOUND *csound, FASTAB *p)
{
    int32_t i;
    if (p->xmode)
      i = (int32_t) MYFLT2LRND(*p->xndx * p->xbmul);
    else
      i = (int32_t) MYFLT2LRND(*p->xndx);
    if (UNLIKELY(i > p->tablen || i<0)) {
      return csound->PerfError(csound, &(p->h), Str("tabw off end"));
    }
    p->table[i] = *p->rslt;
    return OK;
}

static int32_t fastabi(CSOUND *csound, FASTAB *p)
{
    FUNC *ftp;
    int32 i;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL)) {
      return csound->InitError(csound, Str("tab_i: incorrect table number"));
    }
    if (*p->ixmode)
      i = (int32) MYFLT2LRND(*p->xndx * ftp->flen);
    else
      i = (int32) MYFLT2LRND(*p->xndx);
    if (UNLIKELY(i >= (int32)ftp->flen || i<0)) {
      return csound->InitError(csound, Str("tab_i off end: table number: %d\n"),
                               (int32_t) *p->xfn);
    }
    *p->rslt =  ftp->ftable[i];
    return OK;
}

static int32_t fastabiw(CSOUND *csound, FASTAB *p)
{
    FUNC *ftp;
    int32 i;
    /*ftp = csound->FTFind(p->xfn); */
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL)) {
      return csound->InitError(csound, Str("tabw_i: incorrect table number"));
    }
    if (*p->ixmode)
      i = (int32) MYFLT2LRND(*p->xndx * ftp->flen);
    else
      i = (int32) MYFLT2LRND(*p->xndx);
    if (UNLIKELY(i >= (int32)ftp->flen || i<0)) {
      return csound->PerfError(csound, &(p->h), Str("tabw_i off end"));
    }
    ftp->ftable[i] = *p->rslt;
    return OK;
}

static int32_t fastab(CSOUND *csound, FASTAB *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    FUNC *ftp = csound->FTnp2Finde(csound, p->xfn);
    p->table = ftp->ftable;
    MYFLT *tab = p->table;
    MYFLT *rslt = p->rslt, *ndx = p->xndx;
    if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->xmode) {
      MYFLT xbmul = p->xbmul;
      int32_t len = p->tablen;
      for (i=offset; i<nsmps; i++) {
        int32_t n = (int32_t) MYFLT2LRND(ndx[i] * xbmul);
        if (UNLIKELY(n > len || n<0)) {
          return csound->PerfError(csound, &(p->h), Str("tab off end %d"),n);
        }
        rslt[i] = tab[n];
      }
    }
    else {
      int32_t len = p->tablen;
      for (i=offset; i<nsmps; i++) {
        int32_t n = (int32_t) MYFLT2LRND(ndx[i]);
        if (UNLIKELY(n > len || n<0)) {
          return csound->PerfError(csound, &(p->h), Str("tab off end %d"),n);
        }
        rslt[i] = tab[n];
      }
    }
    return OK;
}

static CS_NOINLINE int32_t tab_init(CSOUND *csound, TB_INIT *p, int32_t ndx)
{
    MYFLT             *ft;
    STDOPCOD_GLOBALS  *pp;
    if (UNLIKELY(csoundGetTable(csound, &ft, MYFLT2LRND(*p->ifn)) < 0))
      return csound->InitError(csound, Str("tab_init: incorrect table number"));
    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    pp->tb_ptrs[ndx] = ft;
    return OK;
}

static CS_NOINLINE int32_t tab_perf(CSOUND *csound, FASTB *p)
{
     IGN(csound);
     *p->r = (*p->tb_ptr)[(int32_t) MYFLT2LRND(*p->ndx)];
    return OK;
}

static CS_NOINLINE int32_t tab_i_tmp(CSOUND *csound, FASTB *p, int32_t ndx)
{
    STDOPCOD_GLOBALS  *pp;
    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    p->tb_ptr = &(pp->tb_ptrs[ndx]);
    p->h.iopadr = (SUBR) tab_perf;
    return tab_perf(csound, p);
}

static CS_NOINLINE int32_t tab_k_tmp(CSOUND *csound, FASTB *p, int32_t ndx)
{
    STDOPCOD_GLOBALS  *pp;
    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    p->tb_ptr = &(pp->tb_ptrs[ndx]);
    p->h.opadr = (SUBR) tab_perf;
    return tab_perf(csound, p);
}

#ifdef TAB_MACRO
#undef TAB_MACRO
#endif

#define TAB_MACRO(W,X,Y,Z)                  \
static int32_t W(CSOUND *csound, TB_INIT *p)    \
{   return tab_init(csound, p, Z);  }       \
static int32_t X(CSOUND *csound, FASTB *p)      \
{   return tab_i_tmp(csound, p, Z); }       \
static int32_t Y(CSOUND *csound, FASTB *p)      \
{   return tab_k_tmp(csound, p, Z); }

TAB_MACRO(tab0_init, tab0_i_tmp, tab0_k_tmp, 0)
TAB_MACRO(tab1_init, tab1_i_tmp, tab1_k_tmp, 1)
TAB_MACRO(tab2_init, tab2_i_tmp, tab2_k_tmp, 2)
TAB_MACRO(tab3_init, tab3_i_tmp, tab3_k_tmp, 3)
TAB_MACRO(tab4_init, tab4_i_tmp, tab4_k_tmp, 4)
TAB_MACRO(tab5_init, tab5_i_tmp, tab5_k_tmp, 5)
TAB_MACRO(tab6_init, tab6_i_tmp, tab6_k_tmp, 6)
TAB_MACRO(tab7_init, tab7_i_tmp, tab7_k_tmp, 7)
TAB_MACRO(tab8_init, tab8_i_tmp, tab8_k_tmp, 8)
TAB_MACRO(tab9_init, tab9_i_tmp, tab9_k_tmp, 9)
TAB_MACRO(tab10_init, tab10_i_tmp, tab10_k_tmp, 10)
TAB_MACRO(tab11_init, tab11_i_tmp, tab11_k_tmp, 11)
TAB_MACRO(tab12_init, tab12_i_tmp, tab12_k_tmp, 12)
TAB_MACRO(tab13_init, tab13_i_tmp, tab13_k_tmp, 13)
TAB_MACRO(tab14_init, tab14_i_tmp, tab14_k_tmp, 14)
TAB_MACRO(tab15_init, tab15_i_tmp, tab15_k_tmp, 15)

/* ====================== */
/* opcodes from Jens Groh */
/* ====================== */

static int32_t nlalp_set(CSOUND *csound, NLALP *p)
{
    IGN(csound);
    if (LIKELY(!(*p->istor))) {
      p->m0 = 0.0;
      p->m1 = 0.0;
    }
    return OK;
}

static int32_t nlalp(CSOUND *csound, NLALP *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *rp;
    MYFLT *ip;
    double m0;
    double m1;
    double tm0;
    double tm1;
    double klfact;
    double knfact;

    rp = p->aresult;
    ip = p->ainsig;
    klfact = (double)*p->klfact;
    knfact = (double)*p->knfact;
    tm0 = p->m0;
    tm1 = p->m1;
    if (UNLIKELY(offset)) memset(rp, '\0', offset*sizeof(MYFLT));
     if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rp[nsmps], '\0', early*sizeof(MYFLT));
    }
   if (knfact == 0.0) { /* linear case */
     if (UNLIKELY(klfact == 0.0)) { /* degenerated linear case */
        m0 = (double)ip[0] - tm1;
        rp[offset] = (MYFLT)(tm0);
        for (n=offset+1; n<nsmps; n++) {
          rp[n] = (MYFLT)(m0);
          m0 = (double)ip[n];
        }
        tm1 = 0.0;
        tm0 = m0;
      }
      else { /* normal linear case */
        for (n=offset; n<nsmps; n++) {
          m0 = (double)ip[n] - tm1;
          m1 = m0 * klfact;
          rp[n] = (MYFLT)(tm0 + m1);
          tm1 = m1;
          tm0 = m0;
        }
      }
    } else { /* non-linear case */
     if (UNLIKELY(klfact == 0.0)) { /* simplified non-linear case */
        for (n=offset; n<nsmps; n++) {
          m0 = (double)ip[n] - tm1;
          m1 = fabs(m0) * knfact;
          rp[n] = (MYFLT)(tm0 + m1);
          tm1 = m1;
          tm0 = m0;
        }
      } else { /* normal non-linear case */
        for (n=offset; n<nsmps; n++) {
          m0 = (double)ip[n] - tm1;
          m1 = m0 * klfact + fabs(m0) * knfact;
          rp[n] = (MYFLT)(tm0 + m1);
          tm1 = m1;
          tm0 = m0;
        }
      }
    }
    p->m0 = tm0;
    p->m1 = tm1;
    return OK;
}

/* ----------------------------------------------- */

static int32_t adsynt2_set(CSOUND *csound,ADSYNT2 *p)
{
    FUNC    *ftp;
    uint32_t count;
    int32   *lphs;
    MYFLT    iphs = *p->iphs;

    p->inerr = 0;

    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
    }
    else {
      p->inerr = 1;
      return csound->InitError(csound, Str("adsynt2: wavetable not found!"));
    }

    count = (uint32_t)*p->icnt;
    if (UNLIKELY(count < 1)) count = 1;
    p->count = count;

    if (LIKELY((ftp = csound->FTnp2Find(csound, p->ifreqtbl)) != NULL)) {
      p->freqtp = ftp;
    }
    else {
      p->inerr = 1;
      return csound->InitError(csound, Str("adsynt2: freqtable not found!"));
    }
    if (UNLIKELY(ftp->flen < count)) {
      p->inerr = 1;
      return csound->InitError(csound,
                               Str("adsynt2: partial count is greater "
                                   "than freqtable size!"));
    }

    if (LIKELY((ftp = csound->FTnp2Find(csound, p->iamptbl)) != NULL)) {
      p->amptp = ftp;
    }
    else {
      p->inerr = 1;
      return csound->InitError(csound, Str("adsynt2: amptable not found!"));
    }
    if (UNLIKELY(ftp->flen < count)) {
      p->inerr = 1;
      return csound->InitError(csound,
                               Str("adsynt2: partial count is greater "
                                   "than amptable size!"));
    }

    if (p->lphs.auxp==NULL ||
        p->lphs.size < sizeof(int32)*count)
      csound->AuxAlloc(csound, sizeof(int32)*count, &p->lphs);
    lphs = (int32*)p->lphs.auxp;

    if (iphs > 1) {
      uint32_t c;
      for (c=0; c<count; c++) {
        lphs[c] = ((int32)
                   ((MYFLT) ((double) (csound->Rand31(&(csound->randSeed1)) - 1)
                             / 2147483645.0) * FMAXLEN)) & PHMASK;
      }
    }
    else if (iphs >= 0) {
      uint32_t c;
      for (c=0; c<count; c++) {
        lphs[c] = ((int32)(iphs * FMAXLEN)) & PHMASK;
      }
    }
    if (p->pamp.auxp==NULL ||
        p->pamp.size < (uint32_t)(sizeof(MYFLT)*p->count))
      csound->AuxAlloc(csound, sizeof(MYFLT)*p->count, &p->pamp);
    else  if (iphs >= 0)        /* AuxAlloc clear anyway */
      memset(p->pamp.auxp, 0, sizeof(MYFLT)*p->count);
    return OK;
}

static int32_t adsynt2(CSOUND *csound,ADSYNT2 *p)
{
    FUNC    *ftp, *freqtp, *amptp;
    MYFLT   *ar, *ftbl, *freqtbl, *amptbl, *prevAmp;
    MYFLT   amp0, amp, cps0, cps, ampIncr, amp2;
    int32   phs, inc, lobits;
    int32   *lphs;
    int32_t     c, count;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    /* I believe this can never happen as InitError will remove instance */
    /* The check should be on p->amptp and p->freqtp  -- JPff            */
    if (UNLIKELY(p->inerr || p->amptp==NULL || p->freqtp==NULL)) {
      return csound->InitError(csound, Str("adsynt2: not initialised"));
    }
    ftp = p->ftp;
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    freqtp = p->freqtp;
    freqtbl = freqtp->ftable;
    amptp = p->amptp;
    amptbl = amptp->ftable;
    lphs = (int32*)p->lphs.auxp;
    prevAmp = (MYFLT*)p->pamp.auxp;

    cps0 = *p->kcps;
    amp0 = *p->kamp;
    count = p->count;

    ar = p->sr;
    memset(ar, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (c=0; c<count; c++) {
      amp2 = prevAmp[c];
      amp = amptbl[c] * amp0;
      cps = freqtbl[c] * cps0;
      inc = (int32) (cps * csound->sicvt);
      phs = lphs[c];
      ampIncr = (amp - amp2) * CS_ONEDKSMPS;
      for (n=offset; n<nsmps; n++) {
        ar[n] += *(ftbl + (phs >> lobits)) * amp2;
        phs += inc;
        phs &= PHMASK;
        amp2 += ampIncr;
      }
      prevAmp[c] = amp;
      lphs[c] = phs;
    }
    return OK;
}

static int32_t exitnow(CSOUND *csound, EXITNOW *p)
{
    (void) p;
    csound->LongJmp(csound, MYFLT2LRND(*p->retval));
    return OK;  /* compiler only */
}

static int32_t tabrec_set(CSOUND *csound,TABREC *p)
{
    IGN(csound);
    p->recording = 0;
    p->currtic = 0;
    p->ndx = 0;
    p->numins = p->INOCOUNT-4;
    return OK;
}

static int32_t tabrec_k(CSOUND *csound,TABREC *p)
{
    if (*p->ktrig_start) {
      if (*p->kfn != p->old_fn) {
        int32_t flen;
        if (UNLIKELY((flen = csoundGetTable(csound,&(p->table),
                                            (int32_t)*p->kfn)) < 0))
          return csound->PerfError(csound, &(p->h),
                                   Str("Invalid ftable no. %f"), *p->kfn);
        p->tablen = (int64_t) flen;
        *(p->table++) = *p->numtics;
        p->old_fn = *p->kfn;
      }
      p->recording = 1;
      p->ndx = 0;
      p->currtic = 0;
    }
    if (*p->ktrig_stop) {

      if (p->currtic >= *p->numtics) {
        p->recording = 0;
        return OK;
      }
      p->currtic++;
    }
    if (p->recording) {
      int32_t j, curr_frame = p->ndx * p->numins;

      MYFLT *table = p->table;
      MYFLT **inargs = p->inargs;
      if (curr_frame + p->numins < p->tablen) {
        /* record only if table is not full */
        for (j = 0; j < p->numins; j++)
          table[curr_frame + j] = *inargs[j];
      }
      (p->ndx)++;
    }
    return OK;
}
/*-------------------------*/
static int32_t tabplay_set(CSOUND *csound,TABPLAY *p)
{
    /*   FUNC *ftp; */
    /* if ((ftp = csound->FTFind(p->ifn)) == NULL) { */
    /*   csound->InitError(csound, Str("tabplay: incorrect table number")); */
    /*   return; */
    /* } */
    /*  p->table = ftp->ftable; */
    /*  p->tablen = ftp->flen; */
    IGN(csound);
    p->playing = 0;
    p->currtic = 0;
    p->ndx = 0;
    p->numouts = p->INOCOUNT-3;
    return OK;
}

static int32_t tabplay_k(CSOUND *csound,TABPLAY *p)
{
    if (*p->ktrig) {
      if (*p->kfn != p->old_fn) {
        int32_t flen;
        if (UNLIKELY((flen = csoundGetTable(csound, &(p->table),
                                            (int32_t)*p->kfn)) < 0))
          return csound->PerfError(csound, &(p->h),
                                   Str("Invalid ftable no. %f"), *p->kfn);
        p->tablen = (int64_t) flen;
        p->currtic = 0;
        p->ndx = 0;
        *(p->table++) = *p->numtics;
        p->old_fn = *p->kfn;
      }
      p->playing = 1;
      if (p->currtic == 0)
        p->ndx = 0;
      if (p->currtic >= *p->numtics) {
        p->playing = 0;
        return OK;
      }
      p->currtic++;
      p->currtic %= (int64_t) *p->numtics;

    }
    if (p->playing) {
      int32_t j, curr_frame = p->ndx * p->numouts;
      MYFLT *table = p->table;
      MYFLT **outargs = p->outargs;
      if (UNLIKELY(curr_frame + p->numouts < p->tablen)) {
        /* play only if ndx is inside table */
        for (j = 0; j < p->numouts; j++)
          *outargs[j] = table[curr_frame+j];
      }
      (p->ndx)++;
    }
    return OK;
}

static int32_t isChanged_set(CSOUND *csound,ISCHANGED *p)
{
     IGN(csound);
    p->numargs = p->INOCOUNT;
    memset(p->old_inargs, 0, sizeof(MYFLT)*p->numargs); /* Initialise */
    p->cnt = 1;
    return OK;
}

static int32_t isChanged(CSOUND *csound,ISCHANGED *p)
{
    IGN(csound);
    MYFLT **inargs = p->inargs;
    MYFLT *old_inargs = p->old_inargs;
    int32_t numargs = p->numargs, ktrig = 0, j;

    if (LIKELY(p->cnt))
      for (j =0; j< numargs; j++) {
        if (*inargs[j] != old_inargs[j]) {
          ktrig = 1;
          break;
        }
      }

    if (ktrig || p->cnt==0) {
      for (j =0; j< numargs; j++) {
        old_inargs[j] = *inargs[j];
      }
    }
    *p->ktrig = (MYFLT) ktrig;
    p->cnt++;
    return OK;
}

static int32_t isChanged2_set(CSOUND *csound,ISCHANGED *p)
{
    int32_t res = isChanged_set(csound,p);
    p->cnt = 0;
    return res;
}

/* changed in array */
static int32_t isAChanged_set(CSOUND *csound, ISACHANGED *p)
{
    int32_t size = 0, i;
    ARRAYDAT *arr = p->chk;
    //char *tmp;
    for (i=0; i<arr->dimensions; i++) size += arr->sizes[i];
    size *= arr->arrayMemberSize;
    csound->AuxAlloc(csound, size, &p->old_chk);
    /* tmp = (char*)p->old_chk.auxp; */
    /* for (i=0; i<size; i++) tmp[i]=rand()&0xff; */
    /* memset(p->old_chk.auxp, '\0', size); */
    p->size = size;
    p->cnt = 0;
    return OK;
}


static int32_t isAChanged(CSOUND *csound,ISACHANGED *p)
{
     IGN(csound);
    ARRAYDAT *chk = p->chk;
    void *old_chk = p->old_chk.auxp;
    int32_t size = p->size;
    int32_t ktrig = memcmp(chk->data, old_chk, size);
    memcpy(old_chk, chk->data, size);
    *p->ktrig = (p->cnt && ktrig)?FL(1.0):FL(0.0);
    p->cnt++;
    return OK;
}


/* ------------------------- */

static int32_t partial_maximum_set(CSOUND *csound,P_MAXIMUM *p)
{
     IGN(csound);
    int32_t flag = (int32_t) *p->imaxflag;
    switch (flag) {
    case 1:
      p->max = 0; break;
    case 2:
      p->max = -FLT_MAX; break;
    case 3:
      p->max = FLT_MAX; break;
    case 4:
      p->max = 0; break;
    }
    p->counter = 0;
    return OK;
}

static int32_t partial_maximum(CSOUND *csound,P_MAXIMUM *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t flag = (int32_t) *p->imaxflag;
    MYFLT *a = p->asig;
    MYFLT max = p->max;
    if (UNLIKELY(early)) nsmps -= early;
    switch(flag) {
    case 1: /* absolute maximum */
      for (n=offset; n<nsmps; n++) {
        MYFLT temp;
        if ((temp = FABS(a[n])) > max) max = temp;
      }
      if (max > p->max) p->max = max;
      break;
    case 2: /* actual maximum */
      for (n=offset; n<nsmps; n++) {
        if (a[n] > max) max = a[n];
      }
      if (max > p->max) p->max = max;
      break;
    case 3: /* actual minimum */
      for (n=offset; n<nsmps; n++) {
        if (a[n] < max) max = a[n];
      }
      if (max < p->max) p->max = max;
      break;
    case 4: { /* average */
        MYFLT temp = FL(0.0);
        p->counter += nsmps;
        for (n=offset; n<nsmps; n++) {
          temp += a[n];
        }
        p->max += temp;
      }
      break;
    default:
      return csound->PerfError(csound, &(p->h),
                               Str("max_k: invalid imaxflag value"));
    }
    if (*p->ktrig) {
      switch (flag) {
      case 4:
        *p->kout = p->max / (MYFLT) p->counter;
        p->counter = 0;
        p->max = FL(0.0);
      break;
      case 1:
        *p->kout = p->max;
        p->max = 0; break;
      case 2:
        *p->kout = p->max;
        p->max = -FLT_MAX; break;
      case 3:
        *p->kout = p->max;
        p->max = FLT_MAX; break;
      }
    }
    return OK;
}

/* From fractals.c */
/* mandelbrot set scanner  */
static int32_t mandel_set(CSOUND *csound,MANDEL *p)
{
     IGN(csound);
    p->oldx=-99999; /*probably unused values  */
    p->oldy=-99999;
    p->oldCount = -1;
    return OK;
}

static int32_t mandel(CSOUND *csound,MANDEL *p)
{
     IGN(csound);
    MYFLT px=*p->kx, py=*p->ky;
    if (*p->ktrig && (px != p->oldx || py != p->oldy)) {
      int32_t maxIter = (int32_t) *p->kmaxIter, j;
      MYFLT x=FL(0.0), y=FL(0.0), newx, newy;
      for (j=0; j<maxIter; j++) {
        newx = x*x - y*y + px;
        newy = FL(2.0)*x*y + py;
        x=newx;
        y=newy;
        if (x*x+y*y >= FL(4.0)) break;
      }
      p->oldx = px;
      p->oldy = py;
      if (p->oldCount != j) *p->koutrig = FL(1.0);
      else *p->koutrig = FL(0.0);
      *p->kr = (MYFLT) (p->oldCount = j);
    }
    else {
      *p->kr = (MYFLT) p->oldCount;
      *p->koutrig = FL(0.0);
    }
    return OK;
}

#define S(x)    sizeof(x)

OENTRY gab_localops[] = {
  {"resonxk", S(KRESONX),    0, 3,   "k",    "kkkooo",
                            (SUBR) krsnsetx, (SUBR) kresonx, NULL, NULL},
  { "tab_i",S(FASTAB),       TR, 1,   "i",    "iio", (SUBR) fastabi, NULL, NULL, NULL},
  { "tab",S(FASTAB),         TR, 3,   "a",    "xio",
                            (SUBR) fastab_set, (SUBR) fastab, NULL, NULL},
  { "tab.k",S(FASTAB),       TR, 3,   "k",    "kio",
                            (SUBR) fastab_set, (SUBR)fastabk, NULL, NULL},
  { "tabw_i",S(FASTAB),      TW, 1,   "",    "iiio", (SUBR) fastabiw, NULL, NULL, NULL},
  { "tabw",S(FASTAB),        TW, 3,   "",    "kkio",
                            (SUBR)fastab_set, (SUBR)fastabkw, NULL, NULL},
  { "tabw",S(FASTAB),        TW, 3,   "",    "aaio",
                            (SUBR)fastab_set, (SUBR)fastabw, NULL, NULL},
  { "tb0_init", S(TB_INIT),  _QQ, 1,   "",      "i",    (SUBR)tab0_init, NULL, NULL, NULL},
  { "tb1_init", S(TB_INIT),  _QQ, 1,   "",      "i",    (SUBR)tab1_init, NULL, NULL, NULL},
  { "tb2_init", S(TB_INIT),  _QQ, 1,   "",      "i",    (SUBR)tab2_init, NULL, NULL, NULL},
  { "tb3_init", S(TB_INIT),  _QQ, 1,   "",      "i",    (SUBR)tab3_init, NULL, NULL, NULL},
  { "tb4_init", S(TB_INIT),  _QQ, 1,   "",      "i",    (SUBR)tab4_init, NULL, NULL, NULL},
  { "tb5_init", S(TB_INIT),  _QQ, 1,   "",      "i",    (SUBR)tab5_init, NULL, NULL, NULL},
  { "tb6_init", S(TB_INIT),  _QQ, 1,   "",      "i",    (SUBR)tab6_init, NULL, NULL, NULL},
  { "tb7_init", S(TB_INIT),  _QQ, 1,   "",      "i",    (SUBR)tab7_init, NULL, NULL, NULL},
  { "tb8_init", S(TB_INIT),  _QQ, 1,   "",      "i",    (SUBR)tab8_init, NULL, NULL, NULL},
  { "tb9_init", S(TB_INIT),  _QQ, 1,   "",      "i",    (SUBR)tab9_init, NULL, NULL, NULL},
  { "tb10_init", S(TB_INIT), _QQ, 1,   "",      "i",    (SUBR)tab10_init, NULL, NULL, NULL},
  { "tb11_init", S(TB_INIT), _QQ, 1,   "",      "i",    (SUBR)tab11_init, NULL, NULL, NULL},
  { "tb12_init", S(TB_INIT), _QQ, 1,   "",      "i",    (SUBR)tab12_init, NULL, NULL, NULL},
  { "tb13_init", S(TB_INIT), _QQ, 1,   "",      "i",    (SUBR)tab13_init, NULL, NULL, NULL},
  { "tb14_init", S(TB_INIT), _QQ, 1,   "",      "i",    (SUBR)tab14_init, NULL, NULL, NULL},
  { "tb15_init", S(TB_INIT), _QQ, 1,   "",      "i",    (SUBR)tab15_init, NULL, NULL, NULL},
  /* tbx_t (t-rate version removed here) */
  { "tb0.i",      S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab0_i_tmp, NULL, NULL, NULL},
  { "tb1.i",      S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab1_i_tmp, NULL, NULL, NULL},
  { "tb2.i",      S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab2_i_tmp, NULL, NULL, NULL},
  { "tb3.i",      S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab3_i_tmp, NULL, NULL, NULL},
  { "tb4.i",      S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab4_i_tmp, NULL, NULL, NULL},
  { "tb5.i",      S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab5_i_tmp, NULL, NULL, NULL},
  { "tb6.i",      S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab6_i_tmp, NULL, NULL, NULL},
  { "tb7.i",      S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab7_i_tmp, NULL, NULL, NULL},
  { "tb8.i",      S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab8_i_tmp, NULL, NULL, NULL},
  { "tb9.i",      S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab9_i_tmp, NULL, NULL, NULL},
  { "tb10.i",     S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab10_i_tmp, NULL, NULL, NULL},
  { "tb11.i",     S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab11_i_tmp, NULL, NULL, NULL},
  { "tb12.i",     S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab12_i_tmp, NULL, NULL, NULL},
  { "tb13.i",     S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab13_i_tmp, NULL, NULL, NULL},
  { "tb14.i",     S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab14_i_tmp, NULL, NULL, NULL},
  { "tb15.i",     S(FASTB), _QQ|TR, 1,    "i",     "i", (SUBR) tab15_i_tmp, NULL, NULL, NULL},
  { "tb0.k",  S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab0_k_tmp, NULL, NULL},
  { "tb1.k",  S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab1_k_tmp, NULL, NULL},
  { "tb2.k",  S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab2_k_tmp, NULL, NULL},
  { "tb3.k",  S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab3_k_tmp, NULL, NULL},
  { "tb4.k",  S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab4_k_tmp, NULL, NULL},
  { "tb5.k",  S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab5_k_tmp, NULL, NULL},
  { "tb6.k",  S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab6_k_tmp, NULL, NULL},
  { "tb7.k",  S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab7_k_tmp, NULL, NULL},
  { "tb8.k",  S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab8_k_tmp, NULL, NULL},
  { "tb9.k",  S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab9_k_tmp, NULL, NULL},
  { "tb10.k", S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab10_k_tmp, NULL, NULL},
  { "tb11.k", S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab11_k_tmp, NULL, NULL},
  { "tb12.k", S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab12_k_tmp, NULL, NULL},
  { "tb13.k", S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab13_k_tmp, NULL, NULL},
  { "tb14.k", S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab14_k_tmp, NULL, NULL},
  { "tb15.k", S(FASTB), _QQ|TR, 2,  "k",    "k",    NULL, (SUBR) tab15_k_tmp, NULL, NULL},
  { "nlalp",  S(NLALP), 0,  3,  "a",    "akkoo",
                            (SUBR) nlalp_set, (SUBR) nlalp, NULL, NULL},
  { "adsynt2",S(ADSYNT2),TR, 3,    "a",     "kkiiiio",
                            (SUBR) adsynt2_set, (SUBR)adsynt2, NULL, NULL},
  { "exitnow",S(EXITNOW),   0, 1,    "",  "o", (SUBR) exitnow, NULL, NULL, NULL},
/* { "zr_i",  S(ZKR),     0, 1,  "i",  "i",  (SUBR)zread, NULL, NULL}, */
/* { "zr_k",  S(ZKR),     0, 2,  "k",  "k",  NULL, (SUBR)zread, NULL}, */
/* { "zr_a",  S(ZAR),     0, 3,  "a",  "a",  (SUBR)zaset, (SUBR)zar}, */
/* { "k_i",   S(ASSIGN),  0, 1,  "k",  "i",  (SUBR)assign}, */
/* { "k_t",   S(ASSIGN),  0, 2,  "k",  "t",  NULL, (SUBR)assign}, */
/* { "a_k",   S(INDIFF),  0, 3,  "a",  "k",  (SUBR)a_k_set, (SUBR)interp }, */
  { "tabrec",   S(TABREC),  TW, 3,     "",      "kkkkz",
                            (SUBR) tabrec_set, (SUBR) tabrec_k, NULL, NULL},
  { "tabplay",  S(TABPLAY), TR, 3,     "",      "kkkz",
                            (SUBR) tabplay_set, (SUBR) tabplay_k, NULL, NULL},
  { "changed.k", S(ISCHANGED),  0, 3,     "k",     "z",
                            (SUBR) isChanged_set, (SUBR)isChanged, NULL, NULL},
  { "changed2.k", S(ISCHANGED), 0, 3,     "k",     "z",
                            (SUBR) isChanged2_set, (SUBR)isChanged, NULL, NULL},
  { "changed2.A", S(ISACHANGED), 0, 3,     "k",     ".[]",
                            (SUBR) isAChanged_set, (SUBR)isAChanged, NULL, NULL},
  { "max_k",  S(P_MAXIMUM), 0, 3,      "k",    "aki",
            (SUBR) partial_maximum_set, (SUBR) partial_maximum, NULL, NULL},
  { "mandel",S(MANDEL),     0, 3,      "kk",    "kkkk",
                            (SUBR) mandel_set, (SUBR) mandel, NULL, NULL}
};

int32_t gab_gab_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(gab_localops[0]),
                                 (int32_t) (sizeof(gab_localops) / sizeof(OENTRY)));
}

