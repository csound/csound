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

#include "cs.h"         /*                                      AOPS.C  */
#include "aops.h"
#include <math.h>
#include <time.h>

#define EIPT3       (25.0/3.0)
#define LOGTWO      (0.69314718056)
#define STEPS       (32768)
#define INTERVAL    (4.0)
#define ONEdLOG2    (1.4426950408889634074)

/* static lookup tables, initialised once at start-up */
        MYFLT   cpsocfrc[OCTRES];
static  MYFLT   powerof2[4096];

/* initialise the tables, called by csoundInitialize() */

void aops_init_tables(void)
{
    int   i;

    for (i = 0; i < OCTRES; i++)
      cpsocfrc[i] = (MYFLT) (pow(2.0, (double) i / (double) OCTRES) * ONEPT);
    for (i = 0; i < 4096; i++)
      powerof2[i] = (MYFLT) (pow(2.0, (double) i * (1.0 / 4096.0) - 15.0));
}

static inline MYFLT pow2(MYFLT a)
{
    int n = (int) (a * FL(4096.0) + FL(61440.5));   /* 4096 * 15 + 0.5 */
    return ((MYFLT) (1 << (n >> 12)) * powerof2[n & 4095]);
}

int rassign(ENVIRON *csound, ASSIGN *p)
{
    /* already assigned by otran */
    return OK;
}

int assign(ENVIRON *csound, ASSIGN *p)
{
    *p->r = *p->a;
    return OK;
}

int aassign(ENVIRON *csound, ASSIGN *p)
{
    /* the orchestra parser converts '=' to 'upsamp' if input arg is k-rate, */
    /* and skips the opcode if outarg == inarg */
    memcpy(p->r, p->a, csound->ksmps * sizeof(MYFLT));
    return OK;
}

int init(ENVIRON *csound, ASSIGN *p)
{
    *p->r = *p->a;
    return OK;
}

int ainit(ENVIRON *csound, ASSIGN *p)
{
    MYFLT aa = *p->a;
    int   n;

    for (n = 0; n < csound->ksmps; n++)
      p->r[n] = aa;
    return OK;
}

#define RELATN(OPNAME,OP) \
    int OPNAME(ENVIRON *csound, RELAT *p) \
    { *p->rbool = (*p->a OP *p->b) ? 1 : 0; return OK; }

RELATN(gt,>)
RELATN(ge,>=)
RELATN(lt,<)
RELATN(le,<=)
RELATN(eq,==)
RELATN(ne,!=)

#define LOGCLX(OPNAME,OP) \
  int OPNAME(ENVIRON *csound, LOGCL *p) \
  { *p->rbool = (*p->ibool OP *p->jbool) ? 1 : 0; return OK; }

LOGCLX(and,&&)
LOGCLX(or,||)

#define KK(OPNAME,OP) \
  int OPNAME(ENVIRON *csound, AOP *p) { *p->r = *p->a OP *p->b; return OK; }

KK(addkk,+)
KK(subkk,-)
KK(mulkk,*)
KK(divkk,/)

MYFLT MOD(MYFLT a, MYFLT bb)
{
    if (bb==FL(0.0)) return FL(0.0);
    else {
      MYFLT b = (bb<0 ? -bb : bb);
      int d = (int)(a / b);
      a -= d * b;
      while (a>b) a -= b;
      while (-a>b) a += b;
      return a;
    }
}

int modkk(ENVIRON *csound, AOP *p)
{
    *p->r = MOD(*p->a, *p->b);
    return OK;
}

#define KA(OPNAME,OP) int OPNAME(ENVIRON *csound, AOP *p) {      \
        int     n;                              \
        MYFLT   *r, a, *b;                      \
        int nsmps = csound->ksmps;              \
        r = p->r;                               \
        a = *p->a;                              \
        b = p->b;                               \
        for (n=0; n<nsmps; n++)                 \
          r[n] = a OP b[n];                     \
        return OK;                              \
}

KA(addka,+)
KA(subka,-)
KA(mulka,*)
KA(divka,/)

int modka(ENVIRON *csound, AOP *p)
{
    int n;
    MYFLT       *r, a, *b;
    int nsmps = csound->ksmps;
    r = p->r;
    a = *p->a;
    b = p->b;
    for (n=0; n<nsmps; n++)
      r[n] = MOD(a,b[n]);
    return OK;
}

#define AK(OPNAME,OP) int OPNAME(ENVIRON *csound, AOP *p) {      \
        int     n;                              \
        MYFLT   *r, *a, b;                      \
        int nsmps = csound->ksmps;              \
        r = p->r;                               \
        a = p->a;                               \
        b = *p->b;                              \
        for (n=0; n<nsmps; n++)                 \
          r[n] = a[n] OP b;                     \
        return OK;                              \
}

AK(addak,+)
AK(subak,-)
AK(mulak,*)
AK(divak,/)

int modak(ENVIRON *csound, AOP *p)
{
    int n;
    MYFLT       *r, *a, b;
    int nsmps = csound->ksmps;
    r = p->r;
    a = p->a;
    b = *p->b;
    for (n=0; n<nsmps; n++)
      r[n] = MOD(a[n], b);
    return OK;
}


#define AA(OPNAME,OP) int OPNAME(ENVIRON *csound, AOP *p) {      \
        int     n;                              \
        MYFLT   *r, *a, *b;                     \
        int nsmps = csound->ksmps;              \
        r = p->r;                               \
        a = p->a;                               \
        b = p->b;                               \
        for (n=0; n<nsmps; n++)                 \
          r[n] = a[n] OP b[n];                  \
        return OK;                              \
}

AA(addaa,+)
AA(subaa,-)
AA(mulaa,*)
AA(divaa,/)

int modaa(ENVIRON *csound, AOP *p)
{
    int n;
    MYFLT       *r, *a, *b;
    int nsmps = csound->ksmps;
    r = p->r;
    a = p->a;
    b = p->b;
    for (n=0; n<nsmps; n++)
      r[n] = MOD(a[n], b[n]);
    return OK;
}

int divzkk(ENVIRON *csound, DIVZ *p)
{
    *p->r = (*p->b != FL(0.0) ? *p->a / *p->b : *p->def);
    return OK;
}

int divzka(ENVIRON *csound, DIVZ *p)
{
    int         n;
    MYFLT       *r, a, *b, def;
    int nsmps = csound->ksmps;
    r = p->r;
    a = *p->a;
    b = p->b;
    def = *p->def;
    for (n=0; n<nsmps; n++)
      r[n] = (b[n]==FL(0.0) ? def : a / b[n]);
    return OK;
}

int divzak(ENVIRON *csound, DIVZ *p)
{
    int         n;
    MYFLT       *r, *a, b, def;
    int nsmps = csound->ksmps;
    r = p->r;
    a = p->a;
    b = *p->b;
    def = *p->def;
    if (b==FL(0.0)) {
      for (n=0; n<nsmps; n++) r[n] = def;
    }
    else {
      for (n=0; n<nsmps; n++) r[n] = a[n] / b;
    }
    return OK;
}

int divzaa(ENVIRON *csound, DIVZ *p)
{
    int n;
    MYFLT       *r, *a, *b, def;
    int nsmps = csound->ksmps;
    r = p->r;
    a = p->a;
    b = p->b;
    def = *p->def;
    for (n=0; n<nsmps; n++)
      r[n] = (b[n]==FL(0.0) ? def : a[n] / b[n]);
    return OK;
}

int conval(ENVIRON *csound, CONVAL *p)
{
    if (*p->cond)
      *p->r = *p->a;
    else
      *p->r = *p->b;
    return OK;
}

int aconval(ENVIRON *csound, CONVAL *p)
{
    MYFLT       *r, *s;

    r = p->r;
    if (*p->cond)
      s = p->a;
    else s = p->b;
    if (r!=s) memcpy(r, s, csound->ksmps*sizeof(MYFLT));
    return OK;
}

int int1(ENVIRON *csound, EVAL *p)              /* returns signed whole no. */
{
    double intpart;
    modf((double)*p->a, &intpart);
    *p->r = (MYFLT)intpart;
    return OK;
}

int int1a(ENVIRON *csound, EVAL *p)             /* returns signed whole no. */
{
    double intpart;
    int    n;
    for (n = 0; n < csound->ksmps; n++) {
      modf((double) p->a[n], &intpart);
      p->r[n] = (MYFLT) intpart;
    }
    return OK;
}

int frac1(ENVIRON *csound, EVAL *p)             /* returns positive frac part */
{
    double intpart, fracpart;
    fracpart = modf((double)*p->a, &intpart);
    *p->r = (MYFLT)fracpart;
    return OK;
}

int frac1a(ENVIRON *csound, EVAL *p)            /* returns positive frac part */
{
    double intpart, fracpart;
    int    n;
    for (n = 0; n < csound->ksmps; n++) {
      fracpart = modf((double) p->a[n], &intpart);
      p->r[n] = (MYFLT) fracpart;
    }
    return OK;
}

#ifdef RNDINT
#undef RNDINT
#endif
#define RNDINT(x) ((long) ((double) (x) + ((double) (x) >= 0.0 ? 0.5 : -0.5)))

#ifdef FLOOR
#undef FLOOR
#endif
#define FLOOR(x) ((long) ((double) (x) >= 0.0 ? (x) : (x) - 0.99999999))

#ifdef CEIL
#undef CEIL
#endif
#define CEIL(x) ((long) ((double) (x) >= 0.0 ? (x) + 0.99999999 : (x)))

int int1_round(ENVIRON *csound, EVAL *p)        /* round to nearest integer */
{
    *p->r = (MYFLT) (RNDINT(*p->a));
    return OK;
}

int int1a_round(ENVIRON *csound, EVAL *p)       /* round to nearest integer */
{
    int n;
    for (n = 0; n < csound->ksmps; n++)
      p->r[n] = (MYFLT) (RNDINT(p->a[n]));
    return OK;
}

int int1_floor(ENVIRON *csound, EVAL *p)        /* round down */
{
    *p->r = (MYFLT) (FLOOR(*p->a));
    return OK;
}

int int1a_floor(ENVIRON *csound, EVAL *p)       /* round down */
{
    int n;
    for (n = 0; n < csound->ksmps; n++)
      p->r[n] = (MYFLT) (FLOOR(p->a[n]));
    return OK;
}

int int1_ceil(ENVIRON *csound, EVAL *p)         /* round up */
{
    *p->r = (MYFLT) (CEIL(*p->a));
    return OK;
}

int int1a_ceil(ENVIRON *csound, EVAL *p)        /* round up */
{
    int n;
    for (n = 0; n < csound->ksmps; n++)
      p->r[n] = (MYFLT) (CEIL(p->a[n]));
    return OK;
}

#define rndmlt (105.947)

int rnd1(ENVIRON *csound, EVAL *p)              /* returns unipolar rand(x) */
{
    double intpart;
    csound->rndfrac = modf(csound->rndfrac * rndmlt, &intpart);
    *p->r = *p->a * (MYFLT) csound->rndfrac;
    return OK;
}

int birnd1(ENVIRON *csound, EVAL *p)            /* returns bipolar rand(x) */
{
    double intpart;
    csound->rndfrac = modf(csound->rndfrac * rndmlt, &intpart);
    *p->r = *p->a * (FL(2.0) * (MYFLT) csound->rndfrac - FL(1.0));
    return OK;
}

#define LIB1(OPNAME,LIBNAME)  int OPNAME(ENVIRON *csound, EVAL *p)       \
                     { *p->r = (MYFLT)LIBNAME((double)*p->a); return OK; }
LIB1(abs1,fabs)
LIB1(exp01,exp)
LIB1(log01,log)
LIB1(sqrt1,sqrt)
LIB1(sin1,sin)
LIB1(cos1,cos)
LIB1(tan1,tan)
LIB1(asin1,asin)
LIB1(acos1,acos)
LIB1(atan1,atan)
LIB1(sinh1,sinh)
LIB1(cosh1,cosh)
LIB1(tanh1,tanh)
LIB1(log101,log10)

int atan21(ENVIRON *csound, AOP *p)
{
    *p->r = (MYFLT)atan2((double)*p->a, (double)*p->b);
    return OK;
}

#define LIBA(OPNAME,LIBNAME) int OPNAME(ENVIRON *csound, EVAL *p) {    \
                                int     n;                             \
                                MYFLT   *r, *a;                        \
                                int nsmps = csound->ksmps;             \
                                r = p->r;                              \
                                a = p->a;                              \
                                for (n=0;n<nsmps;n++)                  \
                                  r[n] = (MYFLT)LIBNAME((double)a[n]); \
                                return OK;                             \
                              }
LIBA(absa,fabs)
LIBA(expa,exp)
LIBA(loga,log)
LIBA(sqrta,sqrt)
LIBA(sina,sin)
LIBA(cosa,cos)
LIBA(tana,tan)
LIBA(asina,asin)
LIBA(acosa,acos)
LIBA(atana,atan)
LIBA(sinha,sinh)
LIBA(cosha,cosh)
LIBA(tanha,tanh)
LIBA(log10a,log10)

int atan2aa(ENVIRON *csound, AOP *p)
{
    int n;
    MYFLT       *r, *a, *b;
    int nsmps = csound->ksmps;
    r = p->r;
    a = p->a;
    b = p->b;
    for (n=0;n<nsmps;n++)
      r[n] = (MYFLT)atan2((double)a[n], (double)b[n]);
    return OK;
}

int dbamp(ENVIRON *csound, EVAL *p)
{
    *p->r = (MYFLT)(log(fabs((double)*p->a)) / LOG10D20);
    return OK;
}

int ampdb(ENVIRON *csound, EVAL *p)
{
    *p->r = (MYFLT) exp((double)*p->a * LOG10D20);
    return OK;
}

int aampdb(ENVIRON *csound, EVAL *p)
{
    int   n;
    MYFLT *r = p->r, *a = p->a;
    int nsmps = csound->ksmps;
    for (n=0;n<nsmps;n++)
      r[n] = (MYFLT) exp((double)a[n] * LOG10D20);
    return OK;
}

int dbfsamp(ENVIRON *csound, EVAL *p)
{
    *p->r = (MYFLT)(log(fabs((double)*p->a) / csound->e0dbfs) / LOG10D20);
    return OK;
}

int ampdbfs(ENVIRON *csound, EVAL *p)
{
    *p->r =  csound->e0dbfs * (MYFLT) exp((double)*p->a * LOG10D20);
    return OK;
}

int aampdbfs(ENVIRON *csound, EVAL *p)
{
    int n;
    MYFLT       *r, *a;
    int nsmps = csound->ksmps;
    r = p->r;
    a = p->a;
    for (n=0;n<nsmps;n++)
      r[n] = csound->e0dbfs * (MYFLT) exp((double)a[n] * LOG10D20);
    return OK;
}

int ftlen(ENVIRON *csound, EVAL *p)
{
    FUNC        *ftp;

    if ((ftp = csound->FTnp2Find(csound, p->a)) != NULL)
      *p->r = (MYFLT)ftp->flen;
    else
      *p->r = -FL(1.0);      /* Return something */
    return OK;
}

int ftchnls(ENVIRON *csound, EVAL *p)
{
    FUNC *ftp;

    if ((ftp = csound->FTnp2Find(csound,p->a)) != NULL)
      *p->r = (MYFLT)ftp->nchanls;
    else
      *p->r = -FL(1.0);      /* Return something */
    return OK;
}

int ftlptim(ENVIRON *csound, EVAL *p)
{
    FUNC    *ftp;
    if ((ftp = csound->FTnp2Find(csound,p->a)) == NULL) return OK;
    if (ftp->loopmode1)
      *p->r = ftp->begin1 * csound->onedsr;
    else {
      *p->r = FL(0.0);
      if (csound->oparms->msglevel & WARNMSG)
        csound->Message(csound,Str("WARNING: non-looping sample\n"));
    }
    return OK;
}

int numsamp(ENVIRON *csound, EVAL *p)       /***** nsamp by G.Maldonado ****/
{
    FUNC        *ftp;
    if ((ftp = csound->FTFind(csound, p->a)) != NULL)
      *p->r = (MYFLT) ftp->soundend;
    else
      *p->r = FL(0.0);
    return OK;
}

int ftsr(ENVIRON *csound, EVAL *p)              /**** ftsr by G.Maldonado ****/
{
    FUNC        *ftp;
    if ((ftp = csound->FTFind(csound, p->a)) != NULL)
      *p->r = ftp->gen01args.sample_rate;
    else
      *p->r = FL(0.0);
    return OK;
}

int rtclock(ENVIRON *csound, EVAL *p)
{
    /* IV - Jan 29 2005 */
    *p->r = (MYFLT)
            timers_get_real_time(csoundQueryGlobalVariableNoCheck(csound,
                                                                  "csRtClock"));
    return OK;
}

int octpch(ENVIRON *csound, EVAL *p)
{
    double fract, oct;
    fract = modf((double)*p->a, &oct);
    fract *= EIPT3;
    *p->r = (MYFLT) (oct + fract);
    return OK;
}

int pchoct(ENVIRON *csound, EVAL *p)
{
    double fract, oct;
    fract = modf((double)*p->a, &oct);
    fract *= 0.12;
    *p->r = (MYFLT)(oct + fract);
    return OK;
}

int cpsoct(ENVIRON *csound, EVAL *p)
{
    long loct = (long)(*p->a * OCTRES);
    *p->r = (MYFLT)CPSOCTL(loct);
    return OK;
}

int acpsoct(ENVIRON *csound, EVAL *p)
{
    MYFLT       *r, *a;
    long        loct;
    int         n,nsmps = csound->ksmps;
    a = p->a;
    r = p->r;
    for (n=0; n<nsmps; n++) {
      loct = (long)(a[n] * OCTRES);
      r[n] = CPSOCTL(loct);
    }
    return OK;
}

int octcps(ENVIRON *csound, EVAL *p)
{
    *p->r = (MYFLT)(log((double)*p->a / ONEPT) / LOGTWO);
    return OK;
}

int cpspch(ENVIRON *csound, EVAL *p)
{
    double fract, oct;
    long   loct;

    fract = modf((double) *p->a, &oct);
    fract *= EIPT3;
    loct = (long) ((oct + fract) * OCTRES);
    *p->r = (MYFLT)CPSOCTL(loct);
    return OK;
}

int cpsxpch(ENVIRON *csound, XENH *p)
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
      FUNC* ftp = csound->FTFind(csound, &t);
      long len;
      if (ftp == NULL)
        return csound->PerfError(csound, Str("No tuning table %d"),
                                         -((int) *p->et));
      len = ftp->flen;
      while (fract>len) {
        fract -= len; loct++;
      }
      fract += 0.005;
      *p->r = *p->ref * *(ftp->ftable + (int)(100.0*fract)) *
        (MYFLT)pow((double)*p->cy, loct);
    }
    return OK;
}

int cps2pch(ENVIRON *csound, XENH *p)
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
      FUNC* ftp = csound->FTFind(csound, &t);
      long len;
      if (ftp == NULL)
        return csound->PerfError(csound, Str("No tuning table %d"),
                                         -((int) *p->et));
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

int cpstun_i(ENVIRON *csound, CPSTUNI *p)
{
    FUNC  *ftp;
    MYFLT *func;
    int notenum = (int) *p->input;
    int grade;
    int numgrades;
    int basekeymidi;
    MYFLT basefreq, factor,interval;
    if ((ftp = csound->FTFind(csound, p->tablenum)) == NULL) {
      return csound->PerfError(csound, Str("cpstun: invalid table"));
    }
    func = ftp->ftable;
    numgrades = (int) *func++;
    interval = *func++;
    basefreq = *func++;
    basekeymidi = (int) *func++;

    if (notenum < basekeymidi) {
      notenum = basekeymidi - notenum;
      grade  = (numgrades-(notenum % numgrades)) % numgrades;
      factor = - (MYFLT)(int) ((notenum+numgrades-1) / numgrades) ;
    }
    else {
      notenum = notenum - basekeymidi;
      grade  = notenum % numgrades;
      factor = (MYFLT)(int) (notenum / numgrades);
    }
    factor = (MYFLT)pow((double)interval, (double)factor);
    *p->r = func[grade] * factor * basefreq;
    return OK;
}

int cpstun(ENVIRON *csound, CPSTUN *p)
{
    if (*p->ktrig) {
      FUNC  *ftp;
      MYFLT *func;
      int notenum = (int) *p->kinput;
      int grade;
      int numgrades;
      int basekeymidi;
      MYFLT basefreq, factor,interval;
      if ((ftp = csound->FTFind(csound, p->tablenum)) == NULL) {
        return csound->PerfError(csound, Str("cpstun: invalid table"));
      }
      func = ftp->ftable;
      numgrades = (int) *func++;
      interval = *func++;
      basefreq = *func++;
      basekeymidi = (int) *func++;

      if (notenum < basekeymidi) {
        notenum = basekeymidi - notenum;
        grade  = (numgrades-(notenum % numgrades)) % numgrades;
        factor = - (MYFLT)(int) ((notenum+numgrades-1) / numgrades) ;
      }
      else {
        notenum = notenum - basekeymidi;
        grade  = notenum % numgrades;
        factor = (MYFLT)(int) (notenum / numgrades);
      }
      factor = (MYFLT)pow((double)interval, (double)factor);
      p->old_r = (*p->r = func[grade] * factor * basefreq);

    }
    else *p->r = p->old_r;
    return OK;
}

int logbasetwo_set(ENVIRON *csound, EVAL *p)
{
    if (csound->logbase2 == NULL) {
      double  x = (1.0 / INTERVAL);
      int     i;
      csound->logbase2 = (MYFLT*) csound->Malloc(csound, (STEPS + 1)
                                                         * sizeof(MYFLT));
      for (i = 0; i <= STEPS; i++) {
        csound->logbase2[i] = (MYFLT) (ONEdLOG2 * log(x));
        x += ((INTERVAL - 1.0 / INTERVAL) / (double) STEPS);
      }
    }
    return OK;
}

int powoftwo(ENVIRON *csound, EVAL *p)
{
    *p->r = pow2(*p->a);
    return OK;
}

int powoftwoa(ENVIRON *csound, EVAL *p)
{                                   /* by G.Maldonado, liberalised by JPff */
    int n;
    for (n = 0; n < csound->ksmps; n++)
      p->r[n] = pow2(p->a[n]);
    return OK;
}

#define ONEd12          (FL(0.08333333333333333333333))
#define ONEd1200        (FL(0.00083333333333333333333))

int semitone(ENVIRON *csound, EVAL *p)
{
    MYFLT a = *p->a*ONEd12;
    *p->r = pow2(a);
    return OK;
}

int asemitone(ENVIRON *csound, EVAL *p)           /* JPff */
{
    MYFLT *r, *a;
    int n;
    int nsmps = csound->ksmps;
    a = p->a;
    r = p->r;
    for (n=0; n<nsmps; n++) {
      MYFLT aa = (a[n])*ONEd12;
      r[n] = pow2(aa);
    }
    return OK;
}

int cent(ENVIRON *csound, EVAL *p)
{
    MYFLT a = *p->a*ONEd1200;
    *p->r = pow2(a);
    return OK;
}

int acent(ENVIRON *csound, EVAL *p)       /* JPff */
{
    MYFLT *r, *a;
    int n;
    int nsmps = csound->ksmps;
    a = p->a;
    r = p->r;
    for (n=0; n<nsmps; n++) {
      MYFLT aa = (a[n])*ONEd1200;
      r[n] = pow2(aa);
    }
    return OK;
}

#define LOG2_10D20      (FL(0.166096404744368117393515971474))

int db(ENVIRON *csound, EVAL *p)
{
    *p->r = pow2(*p->a*LOG2_10D20);
    return OK;
}

int dba(ENVIRON *csound, EVAL *p)         /* JPff */
{
    MYFLT *r, *a;
    int n;
    int nsmps = csound->ksmps;
    a = p->a;
    r = p->r;
    for (n=0; n<nsmps; n++) {
      MYFLT aa = a[n];
      r[n] = pow2(aa*LOG2_10D20);
    }
    return OK;
}

int logbasetwo(ENVIRON *csound, EVAL *p)
{
    int n = (int) ((*p->a -  (FL(1.0)/INTERVAL)) / (INTERVAL - FL(1.0)/INTERVAL)
                   *  STEPS + FL(0.5));
    if (n<0 || n>STEPS)
      *p->r = (MYFLT)(log((double)*p->a)*ONEdLOG2);
    else
      *p->r = csound->logbase2[n];
    return OK;
}

int logbasetwoa(ENVIRON *csound, EVAL *p)
{                                   /* by G.Maldonado liberalised by JPff */
    MYFLT *r, *a;
    int n;
    int nsmps = csound->ksmps;
    a = p->a;
    r = p->r;
    for (n=0; n<nsmps; n++) {
      MYFLT aa = a[n];
      int n = (int) ((aa - (FL(1.0)/INTERVAL)) / (INTERVAL - FL(1.0)/INTERVAL)
                     *  STEPS + FL(0.5));
      if (n<0 || n>STEPS) r[n] = (MYFLT)(log((double)aa)*ONEdLOG2);
      else                r[n] = csound->logbase2[n];
    }
    return OK;
}

int ilogbasetwo(ENVIRON *csound, EVAL *p)
{
    logbasetwo_set(csound,p);
    logbasetwo(csound,p);
    return OK;
}

int in(ENVIRON *csound, INM *p)
{
    memcpy(p->ar, csound->spin, csound->ksmps * sizeof(MYFLT));
    return OK;
}

int ins(ENVIRON *csound, INS *p)
{
    MYFLT       *sp, *ar1, *ar2;
    int n, k;
    int nsmps = csound->ksmps;

    sp = csound->spin;
    ar1 = p->ar1;
    ar2 = p->ar2;
    for (n=0, k=0; n<nsmps; n++,k+=2) {
      ar1[n] = sp[k];
      ar2[n] = sp[k+1];
    }
    return OK;
}

int inq(ENVIRON *csound, INQ *p)
{
    MYFLT       *sp, *ar1, *ar2, *ar3, *ar4;
    int n, k;
    int nsmps = csound->ksmps;

    sp = csound->spin;
    ar1 = p->ar1;
    ar2 = p->ar2;
    ar3 = p->ar3;
    ar4 = p->ar4;
    for (n=0, k=0; n<nsmps; n++,k+=4) {
      ar1[n] = sp[k];
      ar2[n] = sp[k+1];
      ar3[n] = sp[k+2];
      ar4[n] = sp[k+3];
    }
    return OK;
}

int inh(ENVIRON *csound, INH *p)
{
    MYFLT       *sp, *ar1, *ar2, *ar3, *ar4, *ar5, *ar6;
    int n, k;
    int nsmps = csound->ksmps;

    sp = csound->spin;
    ar1 = p->ar1;
    ar2 = p->ar2;
    ar3 = p->ar3;
    ar4 = p->ar4;
    ar5 = p->ar5;
    ar6 = p->ar6;
    for (n=0, k=0; n<nsmps; n++,k+=6) {
      ar1[n] = sp[k];
      ar2[n] = sp[k+1];
      ar3[n] = sp[k+2];
      ar4[n] = sp[k+3];
      ar5[n] = sp[k+4];
      ar6[n] = sp[k+5];
    }
    return OK;
}

int ino(ENVIRON *csound, INO *p)
{
    MYFLT       *sp, *ar1, *ar2, *ar3, *ar4, *ar5, *ar6, *ar7, *ar8;
    int n, k;
    int nsmps = csound->ksmps;

    sp = csound->spin;
    ar1 = p->ar1;
    ar2 = p->ar2;
    ar3 = p->ar3;
    ar4 = p->ar4;
    ar5 = p->ar5;
    ar6 = p->ar6;
    ar7 = p->ar7;
    ar8 = p->ar8;
    for (n=0, k=0; n<nsmps; n++,k+=8) {
      ar1[n] = sp[k];
      ar2[n] = sp[k+1];
      ar3[n] = sp[k+2];
      ar4[n] = sp[k+3];
      ar5[n] = sp[k+4];
      ar6[n] = sp[k+5];
      ar7[n] = sp[k+6];
      ar8[n] = sp[k+7];
    }
    return OK;
}

static int inn(ENVIRON *csound, INALL *p, int n)
{
    MYFLT *sp, **ara;
    int   m;
    int   i;
    int   nsmps = csound->ksmps;

    sp = csound->spin;
    ara = p->ar;
    for (m=0; m<nsmps; m++) {
      for (i=0; i<n; i++)
        *ara[i] = *sp++;
    }
    return OK;
}

int in16(ENVIRON *csound, INALL *p)
{
    inn(csound, p, 16);
    return OK;
}

int in32(ENVIRON *csound, INALL *p)
{
    inn(csound, p, 32);
    return OK;
}

int inall(ENVIRON *csound, INCH *p)
{
/*      int nch = (int) p->INOCOUNT; */
/*      inn(csound, p, (nch>csound->nchnls ? csound->nchnls : nch)); */
    int ch = (int)(*p->ch+FL(0.5));
    int n;
    int nsmps = csound->ksmps;
    MYFLT *sp = csound->spin+ch-1;
    MYFLT *ain = p->ar;
    if (ch>csound->nchnls) return NOTOK;
    for (n=0; n<nsmps; n++) {
      ain[n] = *sp;
      sp += csound->nchnls;
    }
    return OK;
}

int out(ENVIRON *csound, OUTM *p)
{
    int n;

    if (!csound->spoutactive) {
      memcpy(csound->spout, p->asig, csound->ksmps * sizeof(MYFLT));
      csound->spoutactive = 1;
    }
    else {
      for (n = 0; n < csound->ksmps; n++)
        csound->spout[n] += p->asig[n];
    }
    return OK;
}

int outs(ENVIRON *csound, OUTS *p)
{
    MYFLT       *sp, *ap1, *ap2;
    int nsmps = csound->ksmps;

    ap1 = p->asig1;
    ap2 = p->asig2;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++, m+=2) {
        sp[m]   = ap1[n];
        sp[m+1] = ap2[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++, m+=2) {
        sp[m]   += ap1[n];
        sp[m+1] += ap2[n];
      }
    }
    return OK;
}

int outq(ENVIRON *csound, OUTQ *p)
{
    MYFLT       *sp, *ap1, *ap2, *ap3, *ap4;
    int nsmps = csound->ksmps;

    ap1 = p->asig1;
    ap2 = p->asig2;
    ap3 = p->asig3;
    ap4 = p->asig4;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = ap1[n];
        sp[m+1] = ap2[n];
        sp[m+2] = ap3[n];
        sp[m+3] = ap4[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   += ap1[n];
        sp[m+1] += ap2[n];
        sp[m+2] += ap3[n];
        sp[m+3] += ap4[n];
      }
    }
    return OK;
}

int outs1(ENVIRON *csound, OUTM *p)
{
    MYFLT       *sp, *ap1;
    int nsmps = csound->ksmps;

    ap1 = p->asig;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++, m+=2) {
        sp[m]   = ap1[n];
        sp[m+1] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++, m+=2) {
        sp[m]   += ap1[n];
      }
    }
    return OK;
}

int outs2(ENVIRON *csound, OUTM *p)
{
    MYFLT       *sp, *ap2;
    int nsmps = csound->ksmps;

    ap2 = p->asig;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<nsmps; n++, m+=2) {
        sp[m]   = FL(0.0);
        sp[m+1] = ap2[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=1; n<nsmps; n++, m+=2) {
        sp[m] += ap2[n];
      }
    }
    return OK;
}

int outs12(ENVIRON *csound, OUTM *p)
{
    MYFLT       *sp, *ap;
    int nsmps = csound->ksmps;

    ap = p->asig;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;
      for (n=0, m=0; n<nsmps; n++, m+=2) {
        sp[m] = sp[m+1] = ap[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n,m;
      for (n=0, m=0; n<nsmps; n++, m+=2) {
        sp[m]   += ap[n];
        sp[m+1] += ap[n];
      }
    }
    return OK;
}

int outq1(ENVIRON *csound, OUTM *p)
{
    MYFLT       *sp, *ap1;
    int nsmps = csound->ksmps;

    ap1 = p->asig;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = ap1[n];
        sp[m+1] = FL(0.0);
        sp[m+2] = FL(0.0);
        sp[m+3] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      int n,m;
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   += ap1[n];
      }
    }
    return OK;
}

int outq2(ENVIRON *csound, OUTM *p)
{
    MYFLT       *sp, *ap2;
    int nsmps = csound->ksmps;

    ap2 = p->asig;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = ap2[n];
        sp[m+2] = FL(0.0);
        sp[m+3] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      int n,m;
      for (n=0, m=1; n<nsmps; n++, m+=4) {
        sp[m]   += ap2[n];
      }
    }
    return OK;
}

int outq3(ENVIRON *csound, OUTM *p)
{
    MYFLT       *sp, *ap3;
    int nsmps = csound->ksmps;

    ap3 = p->asig;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = FL(0.0);
        sp[m+2] = ap3[n];
        sp[m+3] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      int n,m;
      for (n=0, m=2; n<nsmps; n++, m+=4) {
        sp[m]   += ap3[n];
      }
    }
   return OK;
 }

int outq4(ENVIRON *csound, OUTM *p)
{
    MYFLT       *sp, *ap4;
    int nsmps = csound->ksmps;

    ap4 = p->asig;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = FL(0.0);
        sp[m+2] = FL(0.0);
        sp[m+3] = ap4[n];
      }
      csound->spoutactive = 1;
    }
    else {
      int n,m;
      for (n=0, m=3; n<nsmps; n++, m+=4) {
        sp[m]   += ap4[n];
      }
    }
    return OK;
}

int outh(ENVIRON *csound, OUTH *p)
{
    MYFLT       *sp, *ap1, *ap2, *ap3, *ap4, *ap5, *ap6;
    int nsmps = csound->ksmps;

    ap1 = p->asig1;
    ap2 = p->asig2;
    ap3 = p->asig3;
    ap4 = p->asig4;
    ap5 = p->asig5;
    ap6 = p->asig6;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;
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
      int n,m;
      for (n=0, m=0; n<nsmps; n++, m+=6) {
        sp[m]   += ap1[n];
        sp[m+1] += ap2[n];
        sp[m+2] += ap3[n];
        sp[m+3] += ap4[n];
        sp[m+4] += ap5[n];
        sp[m+5] += ap6[n];
      }
    }
    return OK;
}

int outo(ENVIRON *csound, OUTO *p)
{
    MYFLT       *sp, *ap1, *ap2, *ap3, *ap4, *ap5, *ap6, *ap7, *ap8;
    int nsmps = csound->ksmps;

    ap1 = p->asig1;
    ap2 = p->asig2;
    ap3 = p->asig3;
    ap4 = p->asig4;
    ap5 = p->asig5;
    ap6 = p->asig6;
    ap7 = p->asig7;
    ap8 = p->asig8;
    sp = csound->spout;
    if (!csound->spoutactive) {
      int n,m;
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
      int n,m;
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
    return OK;
}

static void outn(ENVIRON *csound, int n, OUTX *p)
{
    MYFLT *sp, *ap[64];
    int   nsmps = csound->ksmps;
    int   i;

    for (i=0; i<n; i++) ap[i] = p->asig[i];
    sp = csound->spout;
    if (!csound->spoutactive) {
      do {
        for (i=0; i<n; i++) {
          *sp = *ap[i]++;       sp++;
        }
        for (i=n+1; i < csound->nchnls; i++) {
          *sp = FL(0.0); sp++;
        }
      }
      while (--nsmps);
      csound->spoutactive = 1;
    }
    else {
      do {
        for (i=0; i<n; i++) {
          *sp += *ap[i]++;      sp++;
        }
        for (i=n+1; i < csound->nchnls; i++) {
          sp++;
        }
      }
      while (--nsmps);
    }
}

int outx(ENVIRON *csound, OUTX *p)
{
    outn(csound, 16, p);
    return OK;
}

int outX(ENVIRON *csound, OUTX *p)
{
    outn(csound, 32, p);
    return OK;
}

int outall(ENVIRON *csound, OUTX *p)            /* Output a list of channels */
{
    int nch = (int) p->INOCOUNT;
    outn(csound, (nch <= csound->nchnls ? nch : csound->nchnls), p);
    return OK;
}

int outch(ENVIRON *csound, OUTCH *p)
{
    int         ch;
    int         i, j;
    MYFLT       *sp, *apn;
    int         nsmps = csound->ksmps;
    int         count = (int) p->INOCOUNT;
    MYFLT       **args = p->args;

    for (j=0; j<count; j +=2) {
      nsmps = csound->ksmps;
      ch = (int)(*args[j]+FL(0.5));
      apn = args[j+1];
      if (ch>csound->nchnls) continue;
      if (!csound->spoutactive) {
        sp = csound->spout;
        do {
          for (i=1; i<=csound->nchnls; i++) {
            *sp = ((i==ch) ? *apn++ : FL(0.0));
            sp++;
          }
        } while (--nsmps);
        csound->spoutactive = 1;
      }
      else {
        sp = csound->spout + ch-1;
        do {
          *sp += *apn++;
          sp += csound->nchnls;
        }
        while (--nsmps);
      }
    }
    return OK;
}

/* k-rate i/o opcodes */
/* invalue and outvalue are used with the csoundAPI */
int invalset(ENVIRON *csound, INVAL *p)
{
    if (p->XSTRCODE)
      strcpy(p->channelName, (char*) p->valID);
    else
      sprintf(p->channelName, "%d", (int)(*p->valID+FL(0.5)));
    return OK;
}

int kinval(ENVIRON *csound, INVAL *p)
{
    extern void InputValue(ENVIRON*, char*, MYFLT*);
    InputValue(csound, p->channelName, p->value);  /* in csound.c */
    return OK;
}

int outvalset(ENVIRON *csound, OUTVAL *p)
{
    if (p->XSTRCODE)
      strcpy(p->channelName, (char*) p->valID);
    else
      sprintf(p->channelName, "%d", (int)(*p->valID+FL(0.5)));
    return OK;
}

int koutval(ENVIRON *csound, OUTVAL *p)
{
    extern void OutputValue(ENVIRON*, char*, MYFLT);
    OutputValue(csound, p->channelName, *p->value);  /* in csound.c */
    return OK;
}

