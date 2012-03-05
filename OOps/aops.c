/*
  aops.c:

  Copyright (C) 1991 Barry Vercoe, John ffitch, Gabriel Maldonado

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

#include "csoundCore.h" /*                                      AOPS.C  */
#include "csound.h"
#include "aops.h"
#include <math.h>
#include <time.h>

#define POW2TABSIZI 4096
#define POW2MAX   24.0
#define EIPT3       (25.0/3.0)
#define LOGTWO      (0.69314718055994530942)
#define STEPS       (32768)
#define INTERVAL    (4.0)
#define ONEdLOG2    FL(1.4426950408889634074)
#define MIDINOTE0   (3.00)  /* Lowest midi note is 3.00 in oct & pch formats */

/* static lookup tables, initialised once at start-up; also used by midi ops */
/* MYFLT   cpsocfrc[OCTRES] = { FL(0.0) }; */
/* static  MYFLT   powerof2[POW2TABSIZI]; */

/* initialise the tables, called by csoundInitialize() */

/*void aops_init_tables(void)
{
    int   i;
    for (i = 0; i < OCTRES; i++)
      cpsocfrc[i] = POWER(FL(2.0), (MYFLT)i / OCTRES) * ONEPT;
    for (i = 0; i < POW2TABSIZI; i++)
      powerof2[i] = POWER(FL(2.0), (MYFLT)i * (MYFLT)(1.0/POW2TABSIZI) - FL(POW2MAX));
}*/

/* initialise the tables, called by csoundPreCompile() */
void csound_aops_init_tables(CSOUND *csound)
{
    int   i;
    if(csound->cpsocfrc==NULL)csound->cpsocfrc = (MYFLT *) csound->Malloc(csound, sizeof(MYFLT)*OCTRES);
    if(csound->powerof2==NULL) csound->powerof2 = (MYFLT *) csound->Malloc(csound, sizeof(MYFLT)*POW2TABSIZI);
    for (i = 0; i < OCTRES; i++)
      csound->cpsocfrc[i] = POWER(FL(2.0), (MYFLT)i / OCTRES) * ONEPT;
    for (i = 0; i < POW2TABSIZI; i++)
      csound->powerof2[i] = POWER(FL(2.0), (MYFLT)i * (MYFLT)(1.0/POW2TABSIZI) - FL(POW2MAX));
}


MYFLT csoundPow2(CSOUND *csound, MYFLT a){

  if(a > POW2MAX) a = POW2MAX;
  else if(a < -POW2MAX) a = -POW2MAX;
    int n = (int)MYFLT2LRND(a * FL(POW2TABSIZI)) + POW2MAX*POW2TABSIZI;   /* 4096 * 15 */
    return ((MYFLT) (1UL << (n >> 12)) * csound->powerof2[n & (POW2TABSIZI-1)]);

}


/*static inline MYFLT pow2(MYFLT a)
{
    int n = (int)MYFLT2LRND(a * FL(POW2TABSIZI)) + POW2MAX*POW2TABSIZI;
    return ((MYFLT) (1 << (n >> 12)) * powerof2[n & (POW2TABSIZI-1)]);
}*/

int rassign(CSOUND *csound, ASSIGN *p)
{
    /* already assigned by otran */
    return OK;
}

int assign(CSOUND *csound, ASSIGN *p)
{
    *p->r = *p->a;
    return OK;
}

int aassign(CSOUND *csound, ASSIGN *p)
{
    /* the orchestra parser converts '=' to 'upsamp' if input arg is k-rate, */
    /* and skips the opcode if outarg == inarg */
    memcpy(p->r, p->a, csound->ksmps * sizeof(MYFLT));
    return OK;
}

int ainit(CSOUND *csound, ASSIGN *p)
{
    MYFLT aa = *p->a;
    int   n, nsmps = csound->ksmps;

    for (n = 0; n < nsmps; n++)
      p->r[n] = aa;
    return OK;
}

int minit(CSOUND *csound, ASSIGNM *p)
{
    int nargs = p->INCOUNT;
    int i;
    MYFLT *tmp;
    if (UNLIKELY(nargs > p->OUTOCOUNT))
      return csound->InitError(csound,
                               Str("Cannot be more In arguments than Out in "
                                   "init (%d,%d)"),p->OUTOCOUNT, nargs);
    if (p->OUTOCOUNT==1) {
      *p->r[0] =  *p->a[0];
      return OK;
    }
    tmp = (MYFLT*)malloc(sizeof(MYFLT)*p->OUTOCOUNT);
    for (i=0; i<nargs; i++)
      tmp[i] =  *p->a[i];
    for (; i<p->OUTOCOUNT; i++)
      tmp[i] =  *p->a[nargs-1];
    for (i=0; i<p->OUTOCOUNT; i++)
      *p->r[i] = tmp[i];
    free(tmp);
    return OK;
}

int mainit(CSOUND *csound, ASSIGNM *p)
{
    int nargs = p->INCOUNT;
    int   i, n, nsmps = csound->ksmps;
    MYFLT aa = FL(0.0);
    if (UNLIKELY(nargs > p->OUTOCOUNT))
      return csound->InitError(csound,
                               Str("Cannot be more In arguments than Out in "
                                   "init (%d,%d)"),p->OUTOCOUNT, nargs);
    for (i=0; i<nargs; i++) {
      aa = *p->a[i];
      MYFLT *r =p->r[i];
      for (n = 0; n < nsmps; n++)
        r[n] = aa;
    }
    for (; i<p->OUTOCOUNT; i++) {
      MYFLT *r =p->r[i];
      for (n = 0; n < nsmps; n++)
        r[n] = aa;
    }

    return OK;
}

typedef struct {
    OPDS    h;
    TABDAT  *tab;
} TABDEL;

static int tabdel(CSOUND *csound, void *p)
{
    TABDAT *t = ((TABDEL*)p)->tab;
    mfree(csound, t->data);
    mfree(csound, p);
    return OK;
}

int tinit(CSOUND *csound, INITT *p)
{
    int size = MYFLT2LRND(*p->size);
    MYFLT val = *p->value;
    TABDAT *t = p->a;
    int i;

    t->size = size;
    mfree(csound, t->data);
    t->data = mmalloc(csound, sizeof(MYFLT)*(size+1));
    for (i=0; i<=size; i++) t->data[i] = val;
    { // Need to recover space eventually
      TABDEL *op = (TABDEL*) mmalloc(csound, sizeof(TABDEL));
      op->h.insdshead = ((OPDS*) p)->insdshead;
      op->tab = t;
      csound->RegisterDeinitCallback(csound, op, tabdel);
    }
    return OK;
}

int tassign(CSOUND *csound, ASSIGNT *p)
{
    TABDAT *t = p->tab;
    int ind = MYFLT2LRND(*p->ind);
    if (ind<0 || ind>t->size)
      return csound->PerfError(csound,
                               Str("Index %d out of range [0,%d] in t[]\n"),
                               ind, t->size);
    t->data[ind] = *p->val;
    return OK;
}

int tabref_check(CSOUND *csound, TABREF *p)
{
    if (UNLIKELY(p->tab->data==NULL))
      return csound->InitError(csound, Str("Vector not initialised\n"));
    return OK;
}

int tabref(CSOUND *csound, TABREF *p)
{
    int ind = MYFLT2LRND(*p->ind);
    TABDAT *t = p->tab;
     if (ind<0 || ind>t->size)
      return csound->PerfError(csound,
                               Str("Index %d out of range [0,%d] in t[]\n"),
                               ind, t->size);
     *p->ans = t->data[ind];
     return OK;
}

#define RELATN(OPNAME,OP)                               \
  int OPNAME(CSOUND *csound, RELAT *p)                  \
  { *p->rbool = (*p->a OP *p->b) ? 1 : 0;  return OK; }

RELATN(gt,>)
RELATN(ge,>=)
RELATN(lt,<)
RELATN(le,<=)
RELATN(eq,==)
RELATN(ne,!=)

#define LOGCLX(OPNAME,OP)                                       \
  int OPNAME(CSOUND *csound, LOGCL *p)                          \
  { *p->rbool = (*p->ibool OP *p->jbool) ? 1 : 0; return OK; }

LOGCLX(and,&&)
LOGCLX(or,||)

#define KK(OPNAME,OP)                                                   \
  int OPNAME(CSOUND *csound, AOP *p) { *p->r = *p->a OP *p->b; return OK; }

KK(addkk,+)
KK(subkk,-)
KK(mulkk,*)
KK(divkk,/)

MYFLT MOD(MYFLT a, MYFLT bb)
{
    if (UNLIKELY(bb==FL(0.0))) return FL(0.0);
    else {
      MYFLT b = (bb<0 ? -bb : bb);
      MYFLT d = FMOD(a, b);
      while (d>b) d -= b;
      while (-d>b) d += b;
      return d;
  }
}

int modkk(CSOUND *csound, AOP *p)
{
    *p->r = MOD(*p->a, *p->b);
    return OK;
}

#define KA(OPNAME,OP)                           \
  int OPNAME(CSOUND *csound, AOP *p) {          \
    int     n;                                  \
    MYFLT   *r, a, *b;                          \
    int nsmps = csound->ksmps;                  \
    r = p->r;                                   \
    a = *p->a;                                  \
    b = p->b;                                   \
    for (n=0; n<nsmps; n++)                     \
      r[n] = a OP b[n];                         \
    return OK;                                  \
  }

KA(addka,+)
KA(subka,-)
KA(mulka,*)
KA(divka,/)

int modka(CSOUND *csound, AOP *p)
{
    int     n;
    MYFLT   *r, a, *b;
    int     nsmps = csound->ksmps;

    r = p->r;
    a = *p->a;
    b = p->b;
    for (n=0; n<nsmps; n++)
      r[n] = MOD(a, b[n]);
    return OK;
}

#define AK(OPNAME,OP)                           \
  int OPNAME(CSOUND *csound, AOP *p) {          \
    int     n;                                  \
    MYFLT   *r, *a, b;                          \
    int nsmps = csound->ksmps;                  \
    r = p->r;                                   \
    a = p->a;                                   \
    b = *p->b;                                  \
    for (n=0; n<nsmps; n++)                     \
      r[n] = a[n] OP b;                         \
    return OK;                                  \
  }

AK(addak,+)
AK(subak,-)
AK(mulak,*)
AK(divak,/)

int modak(CSOUND *csound, AOP *p)
{
    int     n;
    MYFLT   *r, *a, b;
    int     nsmps = csound->ksmps;

    r = p->r;
    a = p->a;
    b = *p->b;
    for (n=0; n<nsmps; n++)
      r[n] = MOD(a[n], b);
    return OK;
}

#define AA(OPNAME,OP)                           \
  int OPNAME(CSOUND *csound, AOP *p) {          \
    int     n;                                  \
    MYFLT   *r, *a, *b;                         \
    int nsmps = csound->ksmps;                  \
    r = p->r;                                   \
    a = p->a;                                   \
    b = p->b;                                   \
    for (n=0; n<nsmps; n++)                     \
      r[n] = a[n] OP b[n];                      \
    return OK;                                  \
  }

AA(addaa,+)
AA(subaa,-)
AA(mulaa,*)
AA(divaa,/)

int modaa(CSOUND *csound, AOP *p)
{
    int     n;
    MYFLT   *r, *a, *b;
    int     nsmps = csound->ksmps;

    r = p->r;
    a = p->a;
    b = p->b;
    for (n=0; n<nsmps; n++)
      r[n] = MOD(a[n], b[n]);
    return OK;
}

int divzkk(CSOUND *csound, DIVZ *p)
{
    *p->r = (*p->b != FL(0.0) ? *p->a / *p->b : *p->def);
    return OK;
}

int divzka(CSOUND *csound, DIVZ *p)
{
    int     n;
    MYFLT   *r, a, *b, def;
    int     nsmps = csound->ksmps;

    r = p->r;
    a = *p->a;
    b = p->b;
    def = *p->def;
    for (n=0; n<nsmps; n++)
      r[n] = (b[n]==FL(0.0) ? def : a / b[n]);
    return OK;
}

int divzak(CSOUND *csound, DIVZ *p)
{
    int     n;
    MYFLT   *r, *a, b, def;
    int     nsmps = csound->ksmps;

    r = p->r;
    a = p->a;
    b = *p->b;
    def = *p->def;
    if (UNLIKELY(b==FL(0.0))) {
      for (n=0; n<nsmps; n++) r[n] = def;
    }
    else {
      for (n=0; n<nsmps; n++) r[n] = a[n] / b;
    }
    return OK;
}

int divzaa(CSOUND *csound, DIVZ *p)
{
    int     n;
    MYFLT   *r, *a, *b, def;
    int     nsmps = csound->ksmps;

    r = p->r;
    a = p->a;
    b = p->b;
    def = *p->def;
    for (n=0; n<nsmps; n++)
      r[n] = (b[n]==FL(0.0) ? def : a[n] / b[n]);
    return OK;
}

int conval(CSOUND *csound, CONVAL *p)
{
    if (*p->cond)
      *p->r = *p->a;
    else
      *p->r = *p->b;
    return OK;
}

int aconval(CSOUND *csound, CONVAL *p)
{
    MYFLT   *r, *s;

    r = p->r;
    if (*p->cond)
      s = p->a;
    else s = p->b;
    if (r!=s) memcpy(r, s, csound->ksmps*sizeof(MYFLT));
    return OK;
}

int int1(CSOUND *csound, EVAL *p)               /* returns signed whole no. */
{
    MYFLT intpart;
    MODF(*p->a, &intpart);
    *p->r = intpart;
    return OK;
}

int int1a(CSOUND *csound, EVAL *p)              /* returns signed whole no. */
{
    MYFLT intpart;
    int    n;
    for (n = 0; n < csound->ksmps; n++) {
      MODF(p->a[n], &intpart);
      p->r[n] = intpart;
    }
    return OK;
}

int frac1(CSOUND *csound, EVAL *p)              /* returns positive frac part */
{
    MYFLT intpart, fracpart;
    fracpart = MODF(*p->a, &intpart);
    *p->r = fracpart;
    return OK;
}

int frac1a(CSOUND *csound, EVAL *p)             /* returns positive frac part */
{
    MYFLT intpart, fracpart;
    int    n;
    for (n = 0; n < csound->ksmps; n++) {
      fracpart = MODF(p->a[n], &intpart);
      p->r[n] = fracpart;
    }
    return OK;
}

#ifdef MYFLOOR
#undef MYFLOOR
#endif
#define MYFLOOR(x) ((int32)((double)(x) >= 0.0 ? (x) : (x) - 0.99999999))

#ifdef MYCEIL
#undef MYCEIL
#endif
#define MYCEIL(x) ((int32)((double)(x) >= 0.0 ? (x) + 0.99999999 : (x)))

int int1_round(CSOUND *csound, EVAL *p)         /* round to nearest integer */
{
    *p->r = (MYFLT) MYFLT2LRND(*p->a);
    return OK;
}

int int1a_round(CSOUND *csound, EVAL *p)        /* round to nearest integer */
{
    int n, nsmps = csound->ksmps;
    for (n = 0; n < nsmps; n++)
      p->r[n] = (MYFLT)MYFLT2LRND(p->a[n]);
  return OK;
}

int int1_floor(CSOUND *csound, EVAL *p)         /* round down */
{
    *p->r = (MYFLT)(MYFLOOR(*p->a));
    return OK;
}

int int1a_floor(CSOUND *csound, EVAL *p)        /* round down */
{
    int n, nsmps = csound->ksmps;
    for (n = 0; n < nsmps; n++)
      p->r[n] = (MYFLT)(MYFLOOR(p->a[n]));
    return OK;
}

int int1_ceil(CSOUND *csound, EVAL *p)          /* round up */
{
    *p->r = (MYFLT)(MYCEIL(*p->a));
    return OK;
}

int int1a_ceil(CSOUND *csound, EVAL *p)         /* round up */
{
    int n, nsmps = csound->ksmps;
    for (n = 0; n < nsmps; n++)
      p->r[n] = (MYFLT)(MYCEIL(p->a[n]));
    return OK;
}

#define rndmlt (105.947)

int rnd1(CSOUND *csound, EVAL *p)               /* returns unipolar rand(x) */
{
    double intpart;
    csound->rndfrac = modf(csound->rndfrac * rndmlt, &intpart);
    *p->r = *p->a * (MYFLT)csound->rndfrac;
    return OK;
}

int birnd1(CSOUND *csound, EVAL *p)             /* returns bipolar rand(x) */
{
    double intpart;
    csound->rndfrac = modf(csound->rndfrac * rndmlt, &intpart);
    *p->r = *p->a * (FL(2.0) * (MYFLT)csound->rndfrac - FL(1.0));
    return OK;
}

#define LIB1(OPNAME,LIBNAME)  int OPNAME(CSOUND *csound, EVAL *p)       \
  { *p->r = LIBNAME(*p->a); return OK; }
LIB1(abs1,FABS)
LIB1(exp01,EXP)
LIB1(log01,LOG)
LIB1(sqrt1,SQRT)
LIB1(sin1,SIN)
LIB1(cos1,COS)
LIB1(tan1,TAN)
LIB1(asin1,ASIN)
LIB1(acos1,ACOS)
LIB1(atan1,ATAN)
LIB1(sinh1,SINH)
LIB1(cosh1,COSH)
LIB1(tanh1,TANH)
LIB1(log101,LOG10)

int atan21(CSOUND *csound, AOP *p)
{
    *p->r = ATAN2(*p->a, *p->b);
    return OK;
}

#define LIBA(OPNAME,LIBNAME) int OPNAME(CSOUND *csound, EVAL *p) {      \
    int     n;                                                          \
    MYFLT   *r, *a;                                                     \
    int nsmps = csound->ksmps;                                          \
    r = p->r;                                                           \
    a = p->a;                                                           \
    for (n=0;n<nsmps;n++)                                               \
      r[n] = LIBNAME(a[n]);                                             \
    return OK;                                                          \
  }
LIBA(absa,FABS)
LIBA(expa,EXP)
LIBA(loga,LOG)
LIBA(sqrta,SQRT)
LIBA(sina,SIN)
LIBA(cosa,COS)
LIBA(tana,TAN)
LIBA(asina,ASIN)
LIBA(acosa,ACOS)
LIBA(atana,ATAN)
LIBA(sinha,SINH)
LIBA(cosha,COSH)
LIBA(tanha,TANH)
LIBA(log10a,LOG10)

int atan2aa(CSOUND *csound, AOP *p)
{
    int     n;
    MYFLT   *r, *a, *b;
    int     nsmps = csound->ksmps;

    r = p->r;
    a = p->a;
    b = p->b;
    for (n = 0; n < nsmps; n++)
      r[n] = ATAN2(a[n], b[n]);
    return OK;
}

int dbamp(CSOUND *csound, EVAL *p)
{
    *p->r = LOG(FABS(*p->a)) / LOG10D20;
    return OK;
}

int ampdb(CSOUND *csound, EVAL *p)
{
    *p->r = EXP(*p->a * LOG10D20);
    return OK;
}

int aampdb(CSOUND *csound, EVAL *p)
{
    int     n;
    MYFLT   *r = p->r, *a = p->a;
    int     nsmps = csound->ksmps;

    for (n = 0; n < nsmps; n++)
      r[n] = EXP(a[n] * LOG10D20);
    return OK;
}

int dbfsamp(CSOUND *csound, EVAL *p)
{
    *p->r = LOG(FABS(*p->a) / csound->e0dbfs) / LOG10D20;
    return OK;
}

int ampdbfs(CSOUND *csound, EVAL *p)
{
    *p->r =  csound->e0dbfs * EXP(*p->a * LOG10D20);
    return OK;
}

int aampdbfs(CSOUND *csound, EVAL *p)
{
    int     n;
    MYFLT   *r, *a;
    int     nsmps = csound->ksmps;

    r = p->r;
    a = p->a;
    for (n = 0; n < nsmps; n++)
      r[n] = csound->e0dbfs * EXP(a[n] * LOG10D20);
    return OK;
}

int ftlen(CSOUND *csound, EVAL *p)
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->a)) == NULL)) {
      *p->r = -FL(1.0);       /* Return something */
      return NOTOK;
    }
    *p->r = (MYFLT)ftp->flen;

    return OK;
}

int ftchnls(CSOUND *csound, EVAL *p)
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->a)) == NULL)) {
      *p->r = -FL(1.0);       /* Return something */
      return NOTOK;
    }
    *p->r = (MYFLT)ftp->nchanls;

    return OK;
}

int ftcps(CSOUND *csound, EVAL *p)
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->a)) == NULL)
        || ftp->cpscvt == FL(0.0)) {
      *p->r = -FL(1.0);       /* Return something */
      return NOTOK;
    }
    *p->r = (MYFLT)(ftp->cvtbas/ftp->cpscvt);

    return OK;
}



int ftlptim(CSOUND *csound, EVAL *p)
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->a)) == NULL))
      return NOTOK;
    if (LIKELY(ftp->loopmode1))
      *p->r = ftp->begin1 * csound->onedsr;
    else {
      *p->r = FL(0.0);
      csound->Warning(csound, Str("non-looping sample"));
    }
    return OK;
}

int numsamp(CSOUND *csound, EVAL *p)        /***** nsamp by G.Maldonado ****/
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->a)) == NULL)) {
      *p->r = FL(0.0);
      return NOTOK;
    }
    /* if (ftp->soundend) */
    *p->r = (MYFLT)ftp->soundend;
    /* else
     *p->r = (MYFLT)(ftp->flen + 1); */

    return OK;
}

int ftsr(CSOUND *csound, EVAL *p)               /**** ftsr by G.Maldonado ****/
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->a)) == NULL)) {
      *p->r = FL(0.0);
      return NOTOK;
    }
    *p->r = ftp->gen01args.sample_rate;

    return OK;
}

int rtclock(CSOUND *csound, EVAL *p)
{
    *p->r = (MYFLT)csoundGetRealTime(csound->csRtClock);
    return OK;
}

int octpch(CSOUND *csound, EVAL *p)
{
    double fract, oct;
    fract = modf((double)*p->a, &oct);
    fract *= EIPT3;
    *p->r = (MYFLT)(oct + fract);
    return OK;
}

int pchoct(CSOUND *csound, EVAL *p)
{
    double fract, oct;
    fract = modf((double)*p->a, &oct);
    fract *= 0.12;
    *p->r = (MYFLT)(oct + fract);
    return OK;
}

int cpsoct(CSOUND *csound, EVAL *p)
{
    int loct = (int)(*p->a * OCTRES);
    *p->r = (MYFLT)CPSOCTL(loct);
    return OK;
}

int acpsoct(CSOUND *csound, EVAL *p)
{
    MYFLT   *r, *a;
    int    loct;
    int     n, nsmps = csound->ksmps;

    a = p->a;
    r = p->r;
    for (n=0; n<nsmps; n++) {
      loct = (int)(a[n] * OCTRES);
      r[n] = CPSOCTL(loct);
    }
    return OK;
}

int octcps(CSOUND *csound, EVAL *p)
{
    *p->r = (LOG(*p->a /(MYFLT)ONEPT) / (MYFLT)LOGTWO);
    return OK;
}

int cpspch(CSOUND *csound, EVAL *p)
{
    double fract, oct;
    int    loct;

    fract = modf((double)*p->a, &oct);
    fract *= EIPT3;
    loct = (int)((oct + fract) * OCTRES);
    *p->r = (MYFLT)CPSOCTL(loct);
    return OK;
}

int cpsmidinn(CSOUND *csound, EVAL *p)
{
    /* Convert Midi Note number to 8ve.decimal format */
    MYFLT oct = (*p->a / FL(12.0)) + FL(MIDINOTE0);
    /* Lookup in cpsoct table */
    int32 loct = (int32)(oct * OCTRES);
    *p->r = (MYFLT)CPSOCTL(loct);
    return OK;
}

int octmidinn(CSOUND *csound, EVAL *p)
{
    /* Convert Midi Note number to 8ve.decimal format */
    *p->r = (*p->a / FL(12.0)) + FL(MIDINOTE0);
    return OK;
}

int pchmidinn(CSOUND *csound, EVAL *p)
{
    double fract, oct, octdec;
    /* Convert Midi Note number to 8ve.decimal format */
    octdec = ((double)*p->a / 12.0) + MIDINOTE0;
    /* then convert to 8ve.pc format */
    fract = modf(octdec, &oct);
    fract *= 0.12;
    *p->r = (MYFLT)(oct + fract);
    return OK;
}

int cpsxpch(CSOUND *csound, XENH *p)
{                               /* This may be too expensive */
    double  fract;
    double  loct;

    fract = modf((double)*p->pc, &loct); /* Get octave */
    if (*p->et > 0) {
      fract = pow((double)*p->cy, loct + (100.0*fract)/((double)*p->et));
      *p->r = (MYFLT)fract * *p->ref;
    }
    else {                      /* Values in a table */
      MYFLT t = - *p->et;
      FUNC* ftp = csound->FTnp2Find(csound, &t);
      int32 len;
      if (UNLIKELY(ftp == NULL))
        return csound->PerfError(csound, Str("No tuning table %d"),
                                 -((int)*p->et));
      len = ftp->flen;
      while (fract>len) {
        fract -= len; loct++;
      }
      fract += 0.005;
      *p->r = *p->ref * *(ftp->ftable + (int)(100.0*fract)) *
        POWER(*p->cy, (MYFLT)loct);
    }
    return OK;
}

int cps2pch(CSOUND *csound, XENH *p)
{
    double  fract;
    double  loct;

    fract = modf((double)*p->pc, &loct);        /* Get octave */
    if (*p->et > 0) {
      fract = pow(2.0, loct + (100.0*fract)/((double)*p->et));
      *p->r = (MYFLT)(fract * 1.02197503906); /* Refer to base frequency */
    }
    else {
      MYFLT t = - *p->et;
      FUNC* ftp = csound->FTnp2Find(csound, &t);
      int32 len;
      if (UNLIKELY(ftp == NULL))
        return csound->PerfError(csound, Str("No tuning table %d"),
                                 -((int)*p->et));
      len = ftp->flen;
      while (fract>len) {
        fract -= len; loct++;
      }
      fract += 0.005;
      *p->r = (MYFLT)(1.02197503906 * *(ftp->ftable +(int)(100.0*fract)) *
                      pow(2.0, loct));
    }

    /*  double ref = 261.62561 / pow(2.0, 8.0); */
    return OK;
}

int cpstun_i(CSOUND *csound, CPSTUNI *p)
{
    FUNC  *ftp;
    MYFLT *func;
    int notenum = (int)*p->input;
    int grade;
    int numgrades;
    int basekeymidi;
    MYFLT basefreq, factor, interval;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->tablenum)) == NULL)) goto err1;
    func = ftp->ftable;
    numgrades = (int)*func++;
    interval = *func++;
    basefreq = *func++;
    basekeymidi = (int)*func++;

    if (notenum < basekeymidi) {
      notenum = basekeymidi - notenum;
      grade  = (numgrades-(notenum % numgrades)) % numgrades;
      factor = - (MYFLT)(int)((notenum+numgrades-1) / numgrades) ;
    }
    else {
      notenum = notenum - basekeymidi;
      grade  = notenum % numgrades;
      factor = (MYFLT)(int)(notenum / numgrades);
    }
    factor = POWER(interval, factor);
    *p->r = func[grade] * factor * basefreq;
    return OK;
 err1:
    return csound->PerfError(csound, Str("cpstun: invalid table"));
}

int cpstun(CSOUND *csound, CPSTUN *p)
{
    if (*p->ktrig) {
      FUNC  *ftp;
      MYFLT *func;
      int notenum = (int)*p->kinput;
      int grade;
      int numgrades;
      int basekeymidi;
      MYFLT basefreq, factor, interval;
      if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->tablenum)) == NULL))
        goto err1;
      func = ftp->ftable;
      numgrades = (int)*func++;
      interval = *func++;
      basefreq = *func++;
      basekeymidi = (int)*func++;

      if (notenum < basekeymidi) {
        notenum = basekeymidi - notenum;
        grade  = (numgrades-(notenum % numgrades)) % numgrades;
        factor = - (MYFLT)(int)((notenum+numgrades-1) / numgrades) ;
      }
      else {
        notenum = notenum - basekeymidi;
        grade  = notenum % numgrades;
        factor = (MYFLT)(int)(notenum / numgrades);
      }
      factor = POWER(interval, factor);
      p->old_r = (*p->r = func[grade] * factor * basefreq);

    }
    else *p->r = p->old_r;
    return OK;
 err1:
    return csound->PerfError(csound, Str("cpstun: invalid table"));
}

int logbasetwo_set(CSOUND *csound, EVAL *p)
{
    if (UNLIKELY(csound->logbase2 == NULL)) {
      double  x = (1.0 / INTERVAL);
      int     i;
      csound->logbase2 = (MYFLT*) csound->Malloc(csound, (STEPS + 1)
                                                 * sizeof(MYFLT));
      for (i = 0; i <= STEPS; i++) {
        csound->logbase2[i] = ONEdLOG2 * LOG((MYFLT)x);
        x += ((INTERVAL - 1.0 / INTERVAL) / (double)STEPS);
      }
    }
    return OK;
}

int powoftwo(CSOUND *csound, EVAL *p)
{
    *p->r = csound->Pow2(csound,*p->a);
    return OK;
}

int powoftwoa(CSOUND *csound, EVAL *p)
{                                   /* by G.Maldonado, liberalised by JPff */
    int n, nsmps = csound->ksmps;
    for (n = 0; n < nsmps; n++)
      p->r[n] = csound->Pow2(csound,p->a[n]);
    return OK;
}

#define ONEd12          (FL(0.08333333333333333333333))
#define ONEd1200        (FL(0.00083333333333333333333))

int semitone(CSOUND *csound, EVAL *p)
{
    MYFLT a = *p->a*ONEd12;
    *p->r = csound->Pow2(csound,a);
    return OK;
}

int asemitone(CSOUND *csound, EVAL *p)            /* JPff */
{
    MYFLT *r, *a;
    int n;
    int nsmps = csound->ksmps;
    a = p->a;
    r = p->r;
    for (n=0; n<nsmps; n++) {
      MYFLT aa = (a[n])*ONEd12;
      r[n] = csound->Pow2(csound,aa);
    }
    return OK;
}

int cent(CSOUND *csound, EVAL *p)
{
    MYFLT a = *p->a*ONEd1200;
    *p->r = csound->Pow2(csound,a);
    return OK;
}

int acent(CSOUND *csound, EVAL *p)        /* JPff */
{
    MYFLT *r, *a;
    int n;
    int nsmps = csound->ksmps;
    a = p->a;
    r = p->r;
    for (n=0; n<nsmps; n++) {
      MYFLT aa = (a[n])*ONEd1200;
      r[n] = csound->Pow2(csound,aa);
  }
  return OK;
}

#define LOG2_10D20      (FL(0.166096404744368117393515971474))

int db(CSOUND *csound, EVAL *p)
{
    *p->r = csound->Pow2(csound,*p->a*LOG2_10D20);
    return OK;
}

int dba(CSOUND *csound, EVAL *p)          /* JPff */
{
    MYFLT *r, *a;
    int n;
    int nsmps = csound->ksmps;
    a = p->a;
    r = p->r;
    for (n=0; n<nsmps; n++) {
      MYFLT aa = a[n];
      r[n] = csound->Pow2(csound,aa*LOG2_10D20);
    }
    return OK;
}

int logbasetwo(CSOUND *csound, EVAL *p)
{
    int n = (int)((*p->a -  (FL(1.0)/INTERVAL)) / (INTERVAL - FL(1.0)/INTERVAL)
                  *  STEPS + FL(0.5));
    if (n<0 || n>STEPS)
      *p->r = LOG(*p->a)*ONEdLOG2;
    else
      *p->r = csound->logbase2[n];
    return OK;
}

int logbasetwoa(CSOUND *csound, EVAL *p)
{                                   /* by G.Maldonado liberalised by JPff */
    MYFLT *r, *a;
    int n;
    int nsmps = csound->ksmps;
    a = p->a;
    r = p->r;
    for (n=0; n<nsmps; n++) {
      MYFLT aa = a[n];
      int n = (int)((aa - (FL(1.0)/INTERVAL)) / (INTERVAL - FL(1.0)/INTERVAL)
                    *  STEPS + FL(0.5));
      if (n<0 || n>STEPS) r[n] = LOG(aa)*ONEdLOG2;
      else                r[n] = csound->logbase2[n];
    }
    return OK;
}

int ilogbasetwo(CSOUND *csound, EVAL *p)
{
    logbasetwo_set(csound, p);
    logbasetwo(csound, p);
    return OK;
}

int in(CSOUND *csound, INM *p)
{
    CSOUND_SPIN_SPINLOCK
    memcpy(p->ar, csound->spin, csound->ksmps * sizeof(MYFLT));
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int ins(CSOUND *csound, INS *p)
{
    MYFLT       *sp, *ar1, *ar2;
    int n, k;
    int nsmps = csound->ksmps;
    CSOUND_SPIN_SPINLOCK
    sp = csound->spin;
    ar1 = p->ar1;
    ar2 = p->ar2;
    for (n=0, k=0; n<nsmps; n++, k+=2) {
      ar1[n] = sp[k];
      ar2[n] = sp[k+1];
    }
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int inq(CSOUND *csound, INQ *p)
{
    MYFLT       *sp = csound->spin, *ar1 = p->ar1, *ar2 = p->ar2,
                                    *ar3 = p->ar3, *ar4 = p->ar4;
    int n, k;
    int nsmps = csound->ksmps;
    CSOUND_SPIN_SPINLOCK
    for (n=0, k=0; n<nsmps; n++, k+=4) {
      ar1[n] = sp[k];
      ar2[n] = sp[k+1];
      ar3[n] = sp[k+2];
      ar4[n] = sp[k+3];
    }
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int inh(CSOUND *csound, INH *p)
{
    MYFLT       *sp = csound->spin, *ar1 = p->ar1, *ar2 = p->ar2, *ar3 = p->ar3,
                                    *ar4 = p->ar4, *ar5 = p->ar5, *ar6 = p->ar6;
    int n, k;
    int nsmps = csound->ksmps;
    CSOUND_SPIN_SPINLOCK
    for (n=0, k=0; n<nsmps; n++, k+=6) {
      ar1[n] = sp[k];
      ar2[n] = sp[k+1];
      ar3[n] = sp[k+2];
      ar4[n] = sp[k+3];
      ar5[n] = sp[k+4];
      ar6[n] = sp[k+5];
    }
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int ino(CSOUND *csound, INO *p)
{
    MYFLT       *sp = csound->spin, *ar1 = p->ar1, *ar2 = p->ar2, *ar3 = p->ar3,
                                    *ar4 = p->ar4, *ar5 = p->ar5, *ar6 = p->ar6,
                                    *ar7 = p->ar7, *ar8 = p->ar8;
    int n, k;
    int nsmps = csound->ksmps;
    CSOUND_SPIN_SPINLOCK
    for (n=0, k=0; n<nsmps; n++, k+=8) {
      ar1[n] = sp[k];
      ar2[n] = sp[k+1];
      ar3[n] = sp[k+2];
      ar4[n] = sp[k+3];
      ar5[n] = sp[k+4];
      ar6[n] = sp[k+5];
      ar7[n] = sp[k+6];
      ar8[n] = sp[k+7];
    }
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

static int inn(CSOUND *csound, INALL *p, int n)
{
    MYFLT *sp = csound->spin, **ara = p->ar;
    int   m;
    int   i;
    int   nsmps = csound->ksmps;
    CSOUND_SPIN_SPINLOCK
    for (m = 0; m < nsmps; m++) {
      for (i = 0; i < n; i++)
        *ara[i] = *sp++;
    }
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int in16(CSOUND *csound, INALL *p)
{
    return inn(csound, p, 16);
}

int in32(CSOUND *csound, INALL *p)
{
    return inn(csound, p, 32);
}

int inch_opcode(CSOUND *csound, INCH *p)
{                               /* Rewritten to allow multiple args upto 40 */
    int nc, nChannels = p->INCOUNT;
    int   ch, n, nsmps = csound->ksmps;
    MYFLT *sp, *ain;
    if (UNLIKELY(nChannels != p->OUTOCOUNT))
      return
        csound->PerfError(csound,
                          Str("Input and output argument count differs in inch"));
    for (nc=0; nc<nChannels; nc++) {
      ch = (int)(*p->ch[nc] + FL(0.5));
      if (UNLIKELY(ch > csound->inchnls)) {
        csound->Message(csound, Str("Input channel %d too large; ignored"), ch);
        memset(p->ar[nc], 0, sizeof(MYFLT)*nsmps);
        //        return OK;
      }
      else {
        sp = csound->spin + (ch - 1);
        ain = p->ar[nc];
        for (n = 0; n < nsmps; n++) {
          ain[n] = *sp;
          sp += csound->inchnls;
        }
      }
    }
    return OK;
}

/* int inch_opcode(CSOUND *csound, INCH *p) */
/* { */
/*     int   ch = (int)(*p->ch + FL(0.5)); */
/*     int   n; */
/*     int   nsmps = csound->ksmps; */
/*     MYFLT *sp = csound->spin + (ch - 1); */
/*     MYFLT *ain = p->ar; */
/*     if (UNLIKELY(ch > csound->inchnls)) { */
/*       return NOTOK; */
/*     } */
/*     /\* Are APINLOCKS really necessary for reading?? *\/ */
/*     CSOUND_SPIN_SPINLOCK */
/*     for (n = 0; n < nsmps; n++) { */
/*       ain[n] = *sp; */
/*       sp += csound->inchnls; */
/*     } */
/*     CSOUND_SPIN_SPINUNLOCK */
/*     return OK; */
/* } */

int inall_opcode(CSOUND *csound, INALL *p)
{
    int   n = (int)p->OUTOCOUNT;
    int   m;
    int   i, j = 0, k = 0, nsmps = csound->ksmps;
    MYFLT *spin = csound->spin;
    CSOUND_SPIN_SPINLOCK
    m = (n < csound->inchnls ? n : csound->inchnls);
    for (j=0; j<nsmps; j++) {
      for (i=0; i<m; i++) {
        p->ar[i][j] = spin[k + i];
      }
      for ( ; i < n; i++)
        p->ar[i][j] = FL(0.0);
      k += csound->inchnls;
    }
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

#if 0
int out(CSOUND *csound, OUTM *p)
{
    int n;
    int nsmps = csound->ksmps;
    MYFLT *smp = p->asig;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      memcpy(csound->spout, smp, nsmps * sizeof(MYFLT));
      csound->spoutactive = 1;
    }
    else {
      for (n=0; n<nsmps; n++)
        csound->spout[n] += smp[n];
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outs(CSOUND *csound, OUTS *p)
{
    MYFLT       *sp= csound->spout, *ap1 = p->asig1, *ap2= p->asig2;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      int n, m;                   /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++) {
        sp[m++] = ap1[n];
        sp[m++] = ap2[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;                   /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++) {
        sp[m++] += ap1[n];
        sp[m++] += ap2[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outq(CSOUND *csound, OUTQ *p)
{
    MYFLT       *sp= csound->spout, *ap1= p->asig1, *ap2= p->asig2,
                *ap3= p->asig3, *ap4= p->asig4;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n, m;                   /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = ap1[n];
        sp[m+1] = ap2[n];
        sp[m+2] = ap3[n];
        sp[m+3] = ap4[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;                   /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   += ap1[n];
        sp[m+1] += ap2[n];
        sp[m+2] += ap3[n];
        sp[m+3] += ap4[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}
#endif

int outs1(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp= csound->spout, *ap1= p->asig;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      int n, m;                   /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++) {
        sp[m++] = ap1[n];
        sp[m++] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;                   /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++, m+=2) {
        sp[m]   += ap1[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outs2(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = csound->spout, *ap2 = p->asig;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      int n, m;                   /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++) {
        sp[m++] = FL(0.0);
        sp[m++] = ap2[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;                   /* Amazingly this compiles better!!! */
      for (n=0, m=1; n<nsmps; n++, m+=2) {
        sp[m] += ap2[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outs12(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = csound->spout, *ap = p->asig;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK

    if (!csound->spoutactive) {
      int n, m;
      for (n=0, m=0; n<nsmps; n++, m+=2) {
        sp[m] = sp[m+1] = ap[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;
      for (n=0, m=0; n<nsmps; n++) {
        sp[m++] += ap[n];
        sp[m++] += ap[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outq1(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = csound->spout, *ap1 = p->asig;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      int n, m;
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = ap1[n];
        sp[m+1] = FL(0.0);
        sp[m+2] = FL(0.0);
        sp[m+3] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   += ap1[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outq2(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = csound->spout, *ap2 = p->asig;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      int n, m;
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = ap2[n];
        sp[m+2] = FL(0.0);
        sp[m+3] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;
      for (n=0, m=1; n<nsmps; n++, m+=4) {
        sp[m]   += ap2[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outq3(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = csound->spout, *ap3 = p->asig;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      int n, m;
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = FL(0.0);
        sp[m+2] = ap3[n];
        sp[m+3] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;
      for (n=0, m=2; n<nsmps; n++, m+=4) {
        sp[m]   += ap3[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outq4(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = csound->spout, *ap4 = p->asig;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      int n, m;
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = FL(0.0);
        sp[m+2] = FL(0.0);
        sp[m+3] = ap4[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;
      for (n=0, m=3; n<nsmps; n++, m+=4) {
        sp[m]   += ap4[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

#if 0
int outh(CSOUND *csound, OUTH *p)
{
    MYFLT *sp = csound->spout, *ap1 = p->asig1, *ap2 = p->asig2, *ap3 = p->asig3,
                               *ap4 = p->asig4, *ap5 = p->asig5, *ap6 = p->asig6;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      int n, m;
      for (n=0, m=0; n<nsmps; n++, m+=6) {
        sp[m]   = ap1[n];
        sp[m+1] = ap2[n];
        sp[m+2] = ap3[n];
        sp[m+3] = ap4[n];
        sp[m+4] = ap5[n];
        sp[m+5] = ap6[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;
      for (n=0, m=0; n<nsmps; n++, m+=6) {
        sp[m]   += ap1[n];
        sp[m+1] += ap2[n];
        sp[m+2] += ap3[n];
        sp[m+3] += ap4[n];
        sp[m+4] += ap5[n];
        sp[m+5] += ap6[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outo(CSOUND *csound, OUTO *p)
{
    MYFLT *sp = csound->spout, *ap1 = p->asig1, *ap2 = p->asig2, *ap3 = p->asig3,
                               *ap4 = p->asig4, *ap5 = p->asig5, *ap6 = p->asig6,
                               *ap7 = p->asig7, *ap8 = p->asig8;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      int n, m;
      for (n=0, m=0; n<nsmps; n++, m+=8) {
        sp[m]   = ap1[n];
        sp[m+1] = ap2[n];
        sp[m+2] = ap3[n];
        sp[m+3] = ap4[n];
        sp[m+4] = ap5[n];
        sp[m+5] = ap6[n];
        sp[m+6] = ap7[n];
        sp[m+7] = ap8[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n, m;
      for (n=0, m=0; n<nsmps; n++, m+=8) {
        sp[m]   += ap1[n];
        sp[m+1] += ap2[n];
        sp[m+2] += ap3[n];
        sp[m+3] += ap4[n];
        sp[m+4] += ap5[n];
        sp[m+5] += ap6[n];
        sp[m+6] += ap7[n];
        sp[m+7] += ap8[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}
#endif

static int outn(CSOUND *csound, int n, OUTX *p)
{
    int   i, j = 0, k = 0;
    int nsmps = csound->ksmps;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      for (j=0; j<nsmps; j++) {
        for (i=0; i<n; i++) {
          csound->spout[k + i] = p->asig[i][j];
        }
        for ( ; i < csound->nchnls; i++) {
          csound->spout[k + i] = FL(0.0);
        }
        k += csound->nchnls;
      }
      csound->spoutactive = 1;
    }
    else {
      for (j=0; j<nsmps; j++) {
        for (i=0; i<n; i++) {
          csound->spout[k + i] += p->asig[i][j];
        }
        k += csound->nchnls;
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

#if 0
int outx(CSOUND *csound, OUTX *p)
{
    return outn(csound, 16, p);
}

int outX(CSOUND *csound, OUTX *p)
{
    return outn(csound, 32, p);
}
#endif

int outall(CSOUND *csound, OUTX *p)             /* Output a list of channels */
{
    int nch = (int)p->INOCOUNT;
    return outn(csound, (nch <= csound->nchnls ? nch : csound->nchnls), p);
}

int outch(CSOUND *csound, OUTCH *p)
{
    int         ch;
    int         i, j;
    MYFLT       *sp, *apn;
    int         n, nsmps = csound->ksmps;
    int         count = (int)p->INOCOUNT;
    MYFLT       **args = p->args;
    int         nchnls = csound->nchnls;
    CSOUND_SPOUT_SPINLOCK
    for (j = 0; j < count; j += 2) {
      ch = (int)(*args[j] + FL(0.5));
      apn = args[j + 1];
      if (ch > nchnls) continue;
      if (!csound->spoutactive) {
        sp = csound->spout;
        for (n=0; n<nsmps; n++) {
          for (i = 1; i <= nchnls; i++) {
            *sp = ((i == ch) ? apn[n] : FL(0.0));
            sp++;
          }
        }
        csound->spoutactive = 1;
      }
      else {
        sp = csound->spout + (ch - 1);
        for (n=0; n<nsmps; n++) {
          *sp += apn[n];
          sp += nchnls;
        }
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

/* k-rate and string i/o opcodes */
/* invalue and outvalue are used with the csoundAPI */
/*     ma++ ingalls      matt@sonomatics.com */

int kinval(CSOUND *csound, INVAL *p)
{
    if (csound->InputValueCallback_)
      csound->InputValueCallback_(csound,
                                  (char*) p->channelName.auxp, p->value);
    else
      *(p->value) = FL(0.0);

    return OK;
}

int invalset(CSOUND *csound, INVAL *p)
{
    if (p->XSTRCODE) {
      const char  *s = (char*) p->valID;

      /* check for starting with a $, which will confuse hosts
         -- pretty unlikely given that the parser thinks
         "$string" is a macro -- but just in case: */
      if (UNLIKELY(*s == '$'))
        return csound->InitError(csound, Str("k-rate invalue ChannelName "
                                             "cannot start with $"));
      /* allocate the space used to pass a string during the k-pass */
      csound->AuxAlloc(csound, strlen(s) + 1, &p->channelName);
      sprintf((char*) p->channelName.auxp, "%s", s);
    }
    else {
      /* convert numerical channel to string name */
      csound->AuxAlloc(csound, 64, &p->channelName);
      sprintf((char*) p->channelName.auxp, "%d", (int)MYFLT2LRND(*p->valID));
    }

    /* grab input now for use during i-pass */
    kinval(csound, p);

    return OK;
}

int kinval_S(CSOUND *csound, INVAL *p)
{
    ((char*) p->value)[0] = (char) 0;
    /* make sure the output is null-terminated with old hosts that */
    /* are not aware of string channels */
    ((char*) p->value)[sizeof(MYFLT)] = (char) 0;

    if (csound->InputValueCallback_)
      csound->InputValueCallback_(csound,
                                  (char*) p->channelName.auxp, p->value);

    return OK;
}

int invalset_S(CSOUND *csound, INVAL *p)
{
    if (p->XSTRCODE) {
      const char  *s = (char*) p->valID;
      csound->AuxAlloc(csound, strlen(s) + 2, &p->channelName);
      sprintf((char*) p->channelName.auxp, "$%s", s);
    }
    else {
      csound->AuxAlloc(csound, 64, &p->channelName);
      sprintf(p->channelName.auxp, "$%d", (int)MYFLT2LRND(*p->valID));
    }

    /* grab input now for use during i-pass */
    kinval_S(csound, p);

    return OK;
}

int koutval(CSOUND *csound, OUTVAL *p)
{
    char    *chan = (char*)p->channelName.auxp;

    if (csound->OutputValueCallback_) {
      if (p->XSTRCODE & 2) {
        /* a hack to support strings */
        int32  len = strlen(chan);
        strcat(chan, (char*)p->value);
        csound->OutputValueCallback_(csound, chan, (MYFLT)len);
        chan[len] = '\0';   /* clear for next time */
      }
      else
        csound->OutputValueCallback_(csound, chan, *(p->value));
    }

    return OK;
}

int outvalset(CSOUND *csound, OUTVAL *p)
{
    if (p->XSTRCODE & 1) {
      const char  *s = (char*) p->valID;
      if (p->XSTRCODE & 2) {
        /* allocate the space used to pass a string during the k-pass */
        /* FIXME: string constants may use more space than strVarMaxLen */
        csound->AuxAlloc(csound, strlen(s) + csound->strVarMaxLen + 2,
                         &p->channelName);
        sprintf((char*) p->channelName.auxp, "$%s$", s);
      }
      else {
        csound->AuxAlloc(csound, strlen(s) + 1, &p->channelName);
        strcpy((char*) p->channelName.auxp, s);
      }
    }
    else {
      /* convert numerical channel to string name */
      csound->AuxAlloc(csound, 64, &p->channelName);
      sprintf((char*)p->channelName.auxp, (p->XSTRCODE & 2 ? "$%d" : "%d"),
              (int)MYFLT2LRND(*p->valID));
    }

    /* send output now for use during i-pass */
    koutval(csound, p);

    return OK;
}

int is_NaN(CSOUND *csound, ASSIGN *p)
{
    *p->r = isnan(*p->a);
    return OK;
}

int is_NaNa(CSOUND *csound, ASSIGN *p)
{
    int k, nsmps = csound->ksmps;
    MYFLT *a = p->a;
    *p->r = FL(0.0);
    for (k=0; k<nsmps; k++)
      *p->r += isnan(a[k]);
    return OK;
}

int is_inf(CSOUND *csound, ASSIGN *p)
{
    *p->r = isinf(*p->a);
    return OK;
}

int is_infa(CSOUND *csound, ASSIGN *p)
{
    int k, nsmps = csound->ksmps;
    MYFLT *a = p->a;
    MYFLT ans = FL(0.0);
    int sign = 1;
    for (k=0; k<nsmps; k++) {
      if (isinf(a[k]))
        if (ans==FL(0.0)) sign = (int)isinf(a[k]);
      ans++;
    }
    *p->r = ans*sign;
    return OK;
}

int error_fn(CSOUND *csound, ERRFN *p)
{
    return csound->InitError(csound, Str("Unknown functuon called"));
}
