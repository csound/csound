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
extern ENVIRON cenviron;

static  double  eipt3=8.3333333, oct;
#define logtwo  (0.69314718056)

int rassign(ASSIGN *p)
{
    *p->r = *p->a;  /* still not quite right */
    return OK;
}

int assign(ASSIGN *p)
{
    *p->r = *p->a;
    return OK;
}

int aassign(ASSIGN *p)
{
    MYFLT       *r, *a;
    int nsmps = ksmps;

    r = p->r;
    a = p->a;
    if (p->XINCODE) {
      do {
        *r++ = *a++;
      } while (--nsmps);
    }
    else {
      do {
        *r++ = *a;
      } while (--nsmps);
    }
    return OK;
}

int init(ASSIGN *p)
{
    *p->r = *p->a;
    return OK;
}

int ainit(ASSIGN *p)
{
    MYFLT       *r, *a;
    int nsmps = ksmps;

    r = p->r;
    a = p->a;
    do {
      *r++ = *a;
    } while (--nsmps);
    return OK;
}

#define RELATN(OPNAME,OP) \
    int OPNAME(RELAT *p) { *p->rbool = (*p->a OP *p->b) ? 1 : 0; return OK; }

RELATN(gt,>)
RELATN(ge,>=)
RELATN(lt,<)
RELATN(le,<=)
RELATN(eq,==)
RELATN(ne,!=)

#define LOGCLX(OPNAME,OP) \
  int OPNAME(LOGCL *p) { *p->rbool = (*p->ibool OP *p->jbool) ? 1 : 0; return OK; }

LOGCLX(and,&&)
LOGCLX(or,||)

#define KK(OPNAME,OP) \
  int OPNAME(AOP *p) { *p->r = *p->a OP *p->b; return OK; }

KK(addkk,+)
KK(subkk,-)
KK(mulkk,*)
KK(divkk,/)

MYFLT MOD(MYFLT a, MYFLT bb)
{
    if (bb==FL(0.0)) return FL(0.0);
    else {
      MYFLT b = (bb<0 ? -bb : bb);
/*        MYFLT b100 = b*100.0f; */
/*        MYFLT b10000 = b*10000.0f; */
/*        MYFLT b1000000 = b*1000000.0f; */
/*        printf("MOD(%f,%f)=", a, b); fflush(stdout); */
/*        while (a>b1000000) a -= b1000000; */
/*        while (a>b10000) a -= b10000; */
/*        while (a>b100) a -= b100; */
/*        while (a>b) a -= b; */
/*        while (-a>b1000000) a += b1000000; */
/*        while (-a>b10000) a += b10000; */
/*        while (-a>b100) a += b100; */
/*        while (-a>b) a += b; */
      int d = (int)(a / b);
/*        printf("MOD(%f,%f)=[d=%d]", a, b, d); fflush(stdout); */
      a -= d * b;
      while (a>b) a -= b;
      while (-a>b) a += b;
/*        printf("%f\n", a); */
      return a;
    }
}

int modkk(AOP *p)
{
    *p->r = MOD(*p->a, *p->b);
    return OK;
}

#define KA(OPNAME,OP) int OPNAME(AOP *p) {      \
        int     nsmps = ksmps;                  \
        MYFLT   *r, a, *b;                      \
        r = p->r;                               \
        a = *p->a;                              \
        b = p->b;                               \
        do {                                    \
          *r++ = a OP *b++;                     \
        } while (--nsmps);                      \
        return OK;                              \
}

KA(addka,+)
KA(subka,-)
KA(mulka,*)
KA(divka,/)

int modka(AOP *p)
{
    int nsmps = ksmps;
    MYFLT       *r, a, *b;
    r = p->r;
    a = *p->a;
    b = p->b;
    do {
      *r++ = MOD(a,*b++);
    } while (--nsmps);
    return OK;
}

#define AK(OPNAME,OP) int OPNAME(AOP *p) {      \
        int     nsmps = ksmps;                  \
        MYFLT   *r, *a, b;                      \
        r = p->r;                               \
        a = p->a;                               \
        b = *p->b;                              \
        do {                                    \
          *r++ = *a++ OP b;                     \
        } while (--nsmps);                      \
        return OK;                              \
}

AK(addak,+)
AK(subak,-)
AK(mulak,*)
AK(divak,/)

int modak(AOP *p)
{
    int nsmps = ksmps;
    MYFLT       *r, *a, b;
    r = p->r;
    a = p->a;
    b = *p->b;
    do {
      *r++ = MOD(*a++, b);
    } while (--nsmps);
    return OK;
}


#define AA(OPNAME,OP) int OPNAME(AOP *p) {      \
        int     nsmps = ksmps;                  \
        MYFLT   *r, *a, *b;                     \
        r = p->r;                               \
        a = p->a;                               \
        b = p->b;                               \
        do {                                    \
          *r++ = *a++ OP *b++;                  \
        } while (--nsmps);                      \
        return OK;                              \
}

AA(addaa,+)
AA(subaa,-)
AA(mulaa,*)
AA(divaa,/)

int modaa(AOP *p)
{
    int nsmps = ksmps;
    MYFLT       *r, *a, *b;
    r = p->r;
    a = p->a;
    b = p->b;
    do {
      *r++ = MOD(*a++, *b++);
    } while (--nsmps);
    return OK;
}

int divzkk(DIVZ *p)
{
    *p->r = (*p->b != FL(0.0) ? *p->a / *p->b : *p->def);
    return OK;
}

int divzka(DIVZ *p)
{
    int         nsmps = ksmps;
    MYFLT       *r, a, *b, def;
    r = p->r;
    a = *p->a;
    b = p->b;
    def = *p->def;
    do {
      *r++ = (*b==FL(0.0) ? def : a / *b);
      b++;
    } while (--nsmps);
    return OK;
}

int divzak(DIVZ *p)
{
    int         nsmps = ksmps;
    MYFLT       *r, *a, b, def;
    r = p->r;
    a = p->a;
    b = *p->b;
    def = *p->def;
    if (b==FL(0.0)) {
      do {
        *r++ = def;
      } while (--nsmps);
    }
    else {
      do {
        *r++ = *a++ / b;
      } while (--nsmps);
    }
    return OK;
}

int divzaa(DIVZ *p)
{
    int nsmps = ksmps;
    MYFLT       *r, *a, *b, def;
    r = p->r;
    a = p->a;
    b = p->b;
    def = *p->def;
    do {
      *r++ = (*b==FL(0.0) ? def : *a / *b);
      a++; b++;
    } while (--nsmps);
    return OK;
}

int conval(CONVAL *p)
{
    if (*p->cond)
      *p->r = *p->a;
    else
      *p->r = *p->b;
    return OK;
}

int aconval(CONVAL *p)
{
    MYFLT       *r, *s;
    int nsmps = ksmps;

    r = p->r;
    if (*p->cond)
      s = p->a;
    else s = p->b;
    do {
      *r++ = *s++;
    } while (--nsmps);
    return OK;
}

int int1(EVAL *p)                              /* returns signed whole no. */
{
    double intpart;
    modf((double)*p->a, &intpart);
    *p->r = (MYFLT)intpart;
    return OK;
}

int frac1(EVAL *p)                             /* returns positive frac part */
{
    double intpart, fracpart;
    fracpart = modf((double)*p->a, &intpart);
    *p->r = (MYFLT)fracpart;
    return OK;
}

static double rndfrac = 0.5, rndmlt = 105.947;

int rnd1(EVAL *p)              /* returns unipolar rand(x) */
{
    double intpart;
    rndfrac = modf(rndfrac * rndmlt, &intpart);
    *p->r = *p->a * (MYFLT)rndfrac;
    return OK;
}

int birnd1(EVAL *p)            /* returns bipolar rand(x) */
{
    double intpart;
    rndfrac = modf(rndfrac * rndmlt, &intpart);
    *p->r = *p->a * (FL(2.0) * (MYFLT)rndfrac - FL(1.0));
    return OK;
}

#define LIB1(OPNAME,LIBNAME)  int OPNAME(EVAL *p)       \
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

int atan21(AOP *p)
{
    *p->r = (MYFLT)atan2((double)*p->a, (double)*p->b);
    return OK;
}

#define LIBA(OPNAME,LIBNAME) int OPNAME(EVAL *p) {                     \
                                int     nsmps = ksmps;                 \
                                MYFLT   *r, *a;                        \
                                r = p->r;                              \
                                a = p->a;                              \
                                do {                                   \
                                  *r++ = (MYFLT)LIBNAME((double)*a++); \
                                } while (--nsmps);                     \
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

int atan2aa(AOP *p)
{
    int nsmps = ksmps;
    MYFLT       *r, *a, *b;
    r = p->r;
    a = p->a;
    b = p->b;
    do {
      *r++ = (MYFLT)atan2((double)*a++, (double)*b++);
    } while (--nsmps);
    return OK;
}

int dbamp(EVAL *p)
{
    *p->r = (MYFLT)(log(fabs((double)*p->a)) / LOG10D20);
    return OK;
}

int ampdb(EVAL *p)
{
    *p->r = (MYFLT) exp((double)*p->a * LOG10D20);
    return OK;
}

int aampdb(EVAL *p)
{
    int nsmps = ksmps;
    MYFLT       *r, *a;
    r = p->r;
    a = p->a;
    do {
      *r++ = (MYFLT) exp((double)*a++ * LOG10D20);
    } while (--nsmps);
    return OK;
}

int dbfsamp(EVAL *p)
{
    *p->r = (MYFLT)(log(fabs((double)*p->a) / e0dbfs) / LOG10D20);
    return OK;
}

int ampdbfs(EVAL *p)
{
    *p->r =  e0dbfs * (MYFLT) exp((double)*p->a * LOG10D20);
    return OK;
}

int aampdbfs(EVAL *p)
{
    int nsmps = ksmps;
    MYFLT       *r, *a;
    r = p->r;
    a = p->a;
    do {
      *r++ = e0dbfs * (MYFLT) exp((double)*a++ * LOG10D20);
    } while (--nsmps);
    return OK;
}

int ftlen(EVAL *p)
{
    FUNC        *ftp;

    if ((ftp = ftnp2find(p->a)) != NULL)
      *p->r = (MYFLT)ftp->flen;
    else
      *p->r = -FL(1.0);      /* Return something */
    return OK;
}

int ftchnls(EVAL *p)
{
    FUNC *ftp;

    if ((ftp = ftnp2find(p->a)) != NULL)
      *p->r = (MYFLT)ftp->nchanls;
    else
      *p->r = -FL(1.0);      /* Return something */
    return OK;
}

int ftlptim(EVAL *p)
{
    FUNC    *ftp;
    if ((ftp = ftnp2find(p->a)) == NULL) return OK;
    if (ftp->loopmode1)
      *p->r = ftp->begin1 * onedsr;
    else {
      *p->r = FL(0.0);
      if (O.msglevel & WARNMSG)
        printf(Str(X_1074,"WARNING: non-looping sample\n"));
    }
    return OK;
}

int numsamp(EVAL *p)           /***** nsamp by G.Maldonado ****/
{
    FUNC        *ftp;
    if ((ftp = ftfind(p->a)) != NULL)
      *p->r = (MYFLT) ftp->soundend;
    else
      *p->r = FL(0.0);
    return OK;
}

int ftsr(EVAL *p)              /**** ftsr by G.Maldonado ****/
{
    FUNC        *ftp;
    if ((ftp = ftfind(p->a)) != NULL)
      *p->r = ftp->gen01args.sample_rate;
    else
      *p->r = FL(0.0);
    return OK;
}

#ifdef HAVE_GETTIMEOFDAY
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# endif
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

int rtclock(EVAL *p)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *p->r = (MYFLT)tv.tv_sec + FL(0.000001)*(MYFLT)tv.tv_usec;
    return OK;
}
#elif defined(WIN32)
/* Need to undefine these else MSVC barfs */
#undef u_char
#undef u_short
#undef u_int
#undef u_long
#include <windows.h>
int rtclock(EVAL *p)
{
    FILETIME    t;
    GetSystemTimeAsFileTime((LPFILETIME) &t);
    *(p->r) = (FL(2147483648.0) * (MYFLT) ((unsigned long) t.dwHighDateTime) +
               (MYFLT) ((long) ((unsigned long) t.dwLowDateTime >> 1))) *
               FL(2.0e-7);
    return OK;
}
#else
int rtclock(EVAL *p)
{
    time_t realtime = time(NULL);
    *p->r = (MYFLT) realtime;
    return OK;
}
#endif

void cpsoctinit(void)           /* init the arrays, called by oload */
{
    MYFLT *fp;
    long  count;

    cpsocint = (MYFLT *) mmalloc((long)NOCTS * sizeof(MYFLT));
    cpsocfrc = (MYFLT *) mmalloc((long)OCTRES * sizeof(MYFLT));
    for (fp = cpsocint, count = 0; count < NOCTS; count++)
      *fp++ = (MYFLT) intpow(FL(2.0), count);
    for (fp = cpsocfrc, count = 0; count < OCTRES; count++)
      *fp++ = (MYFLT)(pow(2.0, (double)count / (double)OCTRES) * ONEPT);
}

int octpch(EVAL *p)
{
    double      fract;
    fract = modf((double)*p->a, &oct);
    fract *= eipt3;
    *p->r = (MYFLT) (oct + fract);
    return OK;
}

int pchoct(EVAL *p)
{
    double fract;
    fract = modf((double)*p->a, &oct);
    fract *= 0.12;
    *p->r = (MYFLT)(oct + fract);
    return OK;
}

int cpsoct(EVAL *p)
{
    long loct = (long)(*p->a * OCTRES);
    *p->r = (MYFLT)CPSOCTL(loct);
    return OK;
}

int acpsoct(EVAL *p)
{
    MYFLT       *r, *a;
    long        loct, nsmps = ksmps;
    a = p->a;
    r = p->r;
    do {
      loct = (long)(*a++ * OCTRES);
      *r++ = CPSOCTL(loct);
    } while (--nsmps);
    return OK;
}

int octcps(EVAL *p)
{
    *p->r = (MYFLT)(log((double)*p->a / ONEPT) / logtwo);
    return OK;
}

int cpspch(EVAL *p)
{
    double      fract;
    long   loct;

    fract = modf((double) *p->a, &oct);
    fract *= eipt3;
    loct = (long) ((oct + fract) * OCTRES);
    *p->r = (MYFLT)CPSOCTL(loct);
    return OK;
}

int cpsxpch(XENH *p)
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
      FUNC* ftp = ftfind(&t);
      long len;
      if (ftp == NULL) {
        sprintf(errmsg, Str(X_387,"No tuning table %d\n"), (int)(- *p->et));
        return perferror(errmsg);
      }
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

int cps2pch(XENH *p)
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
      FUNC* ftp = ftfind(&t);
      long len;
      if (ftp == NULL) {
        sprintf(errmsg,Str(X_387,"No tuning table %d\n"), (int)(- *p->et));
        return perferror(errmsg);
      }
      len = ftp->flen;
      while (fract>len) {
        fract -= len; loct++;
      }
      fract += 0.005;
      *p->r = (MYFLT)(1.02197503906 * *(ftp->ftable +(int)(100.0*fract)) *
                      pow(2.0, loct));
    }

/*       double ref = 261.62561 / pow(2.0, 8.0); */
    return OK;
}

int cpstun_i(CPSTUNI *p)
{
    FUNC  *ftp;
    MYFLT *func;
    int notenum = (int) *p->input;
    int grade;
    int numgrades;
    int basekeymidi;
    MYFLT basefreq, factor,interval;
    if ((ftp = ftfind(p->tablenum)) == NULL) {
      return perferror(Str(X_1666,"cpstun: invalid table"));
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

int cpstun(CPSTUN *p)
{
    if (*p->ktrig) {
      FUNC  *ftp;
      MYFLT *func;
      int notenum = (int) *p->kinput;
      int grade;
      int numgrades;
      int basekeymidi;
      MYFLT basefreq, factor,interval;
      if ((ftp = ftfind(p->tablenum)) == NULL) {
        return perferror(Str(X_1666,"cpstun: invalid table"));
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

#define STEPS 32768
#define OCTAVES 5.0
MYFLT *powerof2 = NULL; /* gab-A1 for fast power of two table  */
MYFLT *logbase2 = NULL; /* gab-A1 for fast log base 2 table */
static void init_powers(void)
{
    double incr, exp;
    int count;
    MYFLT *fp;

    if (powerof2!=NULL) return;
    incr = (OCTAVES +OCTAVES) / (double)STEPS;
    exp = -OCTAVES;

    powerof2 = (MYFLT *) mmalloc((long)(STEPS+1) * sizeof(MYFLT));
    fp =  powerof2;
    for ( count = 0; count <= STEPS; count++, exp += incr)
      *fp++ = (MYFLT) pow(2.0, exp);
    powerof2 += STEPS/2;
}


#define INTERVAL 4.0
static void init_logs(void)
{                               /* gab for fast log base two table */
    double incr, first;
    double conv;
    MYFLT *fp;
    int count;

    if (logbase2!=NULL) return;
    incr = (INTERVAL - 1/INTERVAL) / (double)STEPS;
    first = 1.0/(double)INTERVAL;
    conv = 1.44269504089 /* 1.0/log(2.0) */;
    logbase2  = (MYFLT *) mmalloc((long)(STEPS+1) * sizeof(MYFLT));
    fp = logbase2;
    for (count = 0; count <= STEPS; count++, first +=incr)
      *fp++ = (MYFLT) (conv * log(first));
}

int powoftwo_set(EVAL *p)
{
    if (powerof2==NULL) init_powers();
    return OK;
}

int logbasetwo_set(EVAL *p)
{
    if (logbase2==NULL) init_logs();
    return OK;
}

int powoftwo(EVAL *p)
{
    int n = (int) (*p->a * (STEPS/(OCTAVES + OCTAVES)) + FL(0.5));
    if (n<-STEPS/2 || n>STEPS/2)
      *p->r = (MYFLT)pow(2.0, (double)*p->a);
    else
      *p->r = powerof2[n];
    return OK;
}

int powoftwoa(EVAL *p)           /* by G.Maldonado, liberalised by JPff */
{
    MYFLT *r, *a;
    long nsmps=ksmps;
    a = p->a;
    r = p->r;
    do {
      MYFLT aa = *a++;
      int n = (int) (aa * (STEPS/(OCTAVES + OCTAVES)) + FL(0.5));
      if (n<-STEPS/2 || n>STEPS/2)
        *r++ = (MYFLT)pow(2.0, (double)aa);
      else
        *r++ = powerof2[n] ;
    } while (--nsmps);
    return OK;
}

MYFLT pow2(MYFLT a)
{
    int n = (int) (a * (STEPS/(OCTAVES + OCTAVES)) + FL(0.5));
    if (n<-STEPS/2 || n>STEPS/2)
      return (MYFLT)pow(2.0, (double)a);
    else
      return(powerof2[n]);
}

#define ONEd12          (FL(0.08333333333333333333333))
#define ONEd1200        (FL(0.00083333333333333333333))
int semitone(EVAL *p)
{
    MYFLT a = *p->a*ONEd12;
    *p->r = pow2(a);
    return OK;
}

int asemitone(EVAL *p)           /* JPff */
{
    MYFLT *r, *a;
    long nsmps = ksmps;
    a = p->a;
    r = p->r;
    do  {
      MYFLT aa = (*a++)*ONEd12;
      *r++ = pow2(aa);
    } while (--nsmps);
    return OK;
}

int cent(EVAL *p)
{
    MYFLT a = *p->a*ONEd1200;
    *p->r = pow2(a);
    return OK;
}

int acent(EVAL *p)       /* JPff */
{
    MYFLT *r, *a;
    long nsmps = ksmps;
    a = p->a;
    r = p->r;
    do {
      MYFLT aa = (*a++)*ONEd1200;
      *r++ = pow2(aa);
    } while (--nsmps);
    return OK;
}

int isemitone(EVAL *p)
{
    powoftwo_set(p);
    semitone(p);
    return OK;
}

int icent(EVAL *p)
{
    powoftwo_set(p);
    cent(p);
    return OK;
}

#define LOG2_10D20      (FL(0.166096404744368117393515971474))
int db(EVAL *p)
{
    *p->r = pow2(*p->a*LOG2_10D20);
    return OK;
}

int dbi(EVAL *p)
{
    powoftwo_set(p);
    *p->r = pow2(*p->a*LOG2_10D20);
    return OK;
}

int dba(EVAL *p)         /* JPff */
{
    MYFLT *r, *a;
    long nsmps = ksmps;
    a = p->a;
    r = p->r;
    do  {
      MYFLT aa = *a++;
      *r++ = pow2(aa*LOG2_10D20);
    } while (--nsmps);
    return OK;
}

#define ONEdLOG2        FL(1.4426950408889634074)
int logbasetwo(EVAL *p)
{
    int n = (int) ((*p->a -  (FL(1.0)/INTERVAL)) / (INTERVAL - FL(1.0)/INTERVAL)
                   *  STEPS + FL(0.5));
    if (n<0 || n>STEPS)
      *p->r = (MYFLT)(log((double)*p->a)*ONEdLOG2);
    else
      *p->r = logbase2[n] ;
    return OK;
}

int logbasetwoa(EVAL *p)       /* by G.Maldonado liberalised by JPff */
{
    MYFLT *r, *a;
    long nsmps=ksmps;
    a = p->a;
    r = p->r;
    do {
      MYFLT aa = *a++;
      int n = (int) ((aa - (FL(1.0)/INTERVAL)) / (INTERVAL - FL(1.0)/INTERVAL)
                     *  STEPS + FL(0.5));
      if (n<0 || n>STEPS) *r++ = (MYFLT)(log((double)aa)*ONEdLOG2);
      else                *r++ = logbase2[n] ;
    } while (--nsmps);
    return OK;
}

int ipowoftwo(EVAL *p)
{
    powoftwo_set(p);
    powoftwo(p);
    return OK;
}

int ilogbasetwo(EVAL *p)
{
    logbasetwo_set(p);
    logbasetwo(p);
    return OK;
}

int in(INM *p)
{
    MYFLT       *sp, *ar;
    int nsmps = ksmps;

    sp = spin;
    ar = p->ar;
    do {
      *ar++ = *sp++;
    } while (--nsmps);
    return OK;
}

int ins(INS *p)
{
    MYFLT       *sp, *ar1, *ar2;
    int nsmps = ksmps;

    sp = spin;
    ar1 = p->ar1;
    ar2 = p->ar2;
    do {
      *ar1++ = *sp++;
      *ar2++ = *sp++;
    }
    while (--nsmps);
    return OK;
}

int inq(INQ *p)
{
    MYFLT       *sp, *ar1, *ar2, *ar3, *ar4;
    int nsmps = ksmps;

    sp = spin;
    ar1 = p->ar1;
    ar2 = p->ar2;
    ar3 = p->ar3;
    ar4 = p->ar4;
    do {
      *ar1++ = *sp++;
      *ar2++ = *sp++;
      *ar3++ = *sp++;
      *ar4++ = *sp++;
    }
    while (--nsmps);
    return OK;
}

int inh(INH *p)
{
    MYFLT       *sp, *ar1, *ar2, *ar3, *ar4, *ar5, *ar6;
    int nsmps = ksmps;

    sp = spin;
    ar1 = p->ar1;
    ar2 = p->ar2;
    ar3 = p->ar3;
    ar4 = p->ar4;
    ar5 = p->ar5;
    ar6 = p->ar6;
    do {
      *ar1++ = *sp++;
      *ar2++ = *sp++;
      *ar3++ = *sp++;
      *ar4++ = *sp++;
      *ar5++ = *sp++;
      *ar6++ = *sp++;
    }
    while (--nsmps);
    return OK;
}

int ino(INO *p)
{
    MYFLT       *sp, *ar1, *ar2, *ar3, *ar4, *ar5, *ar6, *ar7, *ar8;
    int nsmps = ksmps;

    sp = spin;
    ar1 = p->ar1;
    ar2 = p->ar2;
    ar3 = p->ar3;
    ar4 = p->ar4;
    ar5 = p->ar5;
    ar6 = p->ar6;
    ar7 = p->ar7;
    ar8 = p->ar8;
    do {
      *ar1++ = *sp++;
      *ar2++ = *sp++;
      *ar3++ = *sp++;
      *ar4++ = *sp++;
      *ar5++ = *sp++;
      *ar6++ = *sp++;
      *ar7++ = *sp++;
      *ar8++ = *sp++;
    }
    while (--nsmps);
    return OK;
}

int inn(INALL *p, int n)
{
    MYFLT       *sp, **ara;
    int         nsmps = ksmps;
    int         i;

    sp = spin;
    ara = p->ar;
    do {
      for (i=0; i<n; i++)
        *ara[i] = *sp++;
    }
    while (--nsmps);
    return OK;
}

int in16(INALL *p)
{
    inn(p, 16);
    return OK;
}

int in32(INALL *p)
{
    inn(p, 32);
    return OK;
}

int inall(INCH *p)
{
/*      int nch = (int) p->INOCOUNT; */
/*      inn(p, (nch>nchnls ? nchnls : nch)); */
    int ch = (int)(*p->ch+FL(0.5));
    int nsmps = ksmps;
    MYFLT *sp = spin+ch-1;
    MYFLT *ain = p->ar;
    if (ch>nchnls) return NOTOK;
    do {
      *ain++ = *sp;
      sp += nchnls;
    } while (--nsmps);
    return OK;
}

int out(OUTM *p)
{
    MYFLT       *sp, *ap;
    int n;

    ap = p->asig;
    sp = spout;
    if (!spoutactive) {
      for (n=0; n<ksmps; n++)
        sp[n] = ap[n];
      spoutactive = 1;
    }
    else {
      for (n=0; n<ksmps; n++)
        sp[n] += ap[n];
    }
    return OK;
}

int outs(OUTS *p)
{
    MYFLT       *sp, *ap1, *ap2;

    ap1 = p->asig1;
    ap2 = p->asig2;
    sp = spout;
    if (!spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=2) {
        sp[m]   = ap1[n];
        sp[m+1] = ap2[n];
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=2) {
        sp[m]   += ap1[n];
        sp[m+1] += ap2[n];
      }
    }
    return OK;
}

int outq(OUTQ *p)
{
    MYFLT       *sp, *ap1, *ap2, *ap3, *ap4;

    ap1 = p->asig1;
    ap2 = p->asig2;
    ap3 = p->asig3;
    ap4 = p->asig4;
    sp = spout;
    if (!spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=4) {
        sp[m]   = ap1[n];
        sp[m+1] = ap2[n];
        sp[m+2] = ap3[n];
        sp[m+3] = ap4[n];
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=4) {
        sp[m]   += ap1[n];
        sp[m+1] += ap2[n];
        sp[m+2] += ap3[n];
        sp[m+3] += ap4[n];
      }
    }
    return OK;
}

int outs1(OUTM *p)
{
    MYFLT       *sp, *ap1;

    ap1 = p->asig;
    sp = spout;
    if (!spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=2) {
        sp[m]   = ap1[n];
        sp[m+1] = FL(0.0);
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=2) {
        sp[m]   += ap1[n];
      }
    }
    return OK;
}

int outs2(OUTM *p)
{
    MYFLT       *sp, *ap2;

    ap2 = p->asig;
    sp = spout;
    if (!spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=2) {
        sp[m]   = FL(0.0);
        sp[m+1] = ap2[n];
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=1; n<ksmps; n++, m+=2) {
        sp[m] += ap2[n];
      }
    }
    return OK;
}

int outs12(OUTM *p)
{
    MYFLT       *sp, *ap;

    ap = p->asig;
    sp = spout;
    if (!spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=2) {
        sp[m] = sp[m+1] = ap[n];
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=2) {
        sp[m]   += ap[n];
        sp[m+1] += ap[n];
      }
    }
    return OK;
}

int outq1(OUTM *p)
{
    MYFLT       *sp, *ap1;

    ap1 = p->asig;
    sp = spout;
    if (!spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=4) {
        sp[m]   = ap1[n];
        sp[m+1] = FL(0.0);
        sp[m+2] = FL(0.0);
        sp[m+3] = FL(0.0);
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=4) {
        sp[m]   += ap1[n];
      }
    }
    return OK;
}

int outq2(OUTM *p)
{
    MYFLT       *sp, *ap2;

    ap2 = p->asig;
    sp = spout;
    if (!spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = ap2[n];
        sp[m+2] = FL(0.0);
        sp[m+3] = FL(0.0);
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=1; n<ksmps; n++, m+=4) {
        sp[m]   += ap2[n];
      }
    }
    return OK;
}

int outq3(OUTM *p)
{
    MYFLT       *sp, *ap3;

    ap3 = p->asig;
    sp = spout;
   if (!spoutactive) {
       int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = FL(0.0);
        sp[m+2] = ap3[n];
        sp[m+3] = FL(0.0);
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=2; n<ksmps; n++, m+=4) {
        sp[m]   += ap3[n];
      }
    }
   return OK;
 }

int outq4(OUTM *p)
{
    MYFLT       *sp, *ap4;

    ap4 = p->asig;
    sp = spout;
    if (!spoutactive) {
       int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = FL(0.0);
        sp[m+2] = FL(0.0);
        sp[m+3] = ap4[n];
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=3; n<ksmps; n++, m+=4) {
        sp[m]   += ap4[n];
      }
    }
    return OK;
}

int outh(OUTH *p)
{
    MYFLT       *sp, *ap1, *ap2, *ap3, *ap4, *ap5, *ap6;

    ap1 = p->asig1;
    ap2 = p->asig2;
    ap3 = p->asig3;
    ap4 = p->asig4;
    ap5 = p->asig5;
    ap6 = p->asig6;
    sp = spout;
    if (!spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=6) {
        sp[m]   = ap1[n];
        sp[m+1] = ap2[n];
        sp[m+2] = ap3[n];
        sp[m+3] = ap4[n];
        sp[m+4] = ap5[n];
        sp[m+5] = ap6[n];
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=6) {
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

int outo(OUTO *p)
{
    MYFLT       *sp, *ap1, *ap2, *ap3, *ap4, *ap5, *ap6, *ap7, *ap8;

    ap1 = p->asig1;
    ap2 = p->asig2;
    ap3 = p->asig3;
    ap4 = p->asig4;
    ap5 = p->asig5;
    ap6 = p->asig6;
    ap7 = p->asig7;
    ap8 = p->asig8;
    sp = spout;
    if (!spoutactive) {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=8) {
        sp[m]   = ap1[n];
        sp[m+1] = ap2[n];
        sp[m+2] = ap3[n];
        sp[m+3] = ap4[n];
        sp[m+4] = ap5[n];
        sp[m+5] = ap6[n];
        sp[m+6] = ap7[n];
        sp[m+7] = ap8[n];
      }
      spoutactive = 1;
    }
    else {
      int n,m;                    /* Amazingly this compiles better!!! */
      for (n=0, m=0; n<ksmps; n++, m+=8) {
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

static void outn(int n, OUTX *p)
{
    MYFLT       *sp, *ap[64];
    int nsmps = ksmps;
    int i;

    for (i=0; i<n; i++) ap[i] = p->asig[i];
    sp = spout;
    if (!spoutactive) {
      do {
        for (i=0; i<n; i++) {
          *sp = *ap[i]++;       sp++;
        }
        for (i=n+1; i<nchnls; i++) {
          *sp = FL(0.0); sp++;
        }
      }
      while (--nsmps);
      spoutactive = 1;
    }
    else {
      do {
        for (i=0; i<n; i++) {
          *sp += *ap[i]++;      sp++;
        }
        for (i=n+1; i<nchnls; i++) {
          sp++;
        }
      }
      while (--nsmps);
    }
}

int outx(OUTX *p)
{
    outn(16, p);
    return OK;
}

int outX(OUTX *p)
{
    outn(32, p);
    return OK;
}

int outall(OUTX *p)            /* Output a list of channels */
{
    int nch = (int) p->INOCOUNT;
    outn((nch <= nchnls ? nch : nchnls), p);
    return OK;
}

int outch(OUTCH *p)
{
    int         ch;
    int         i, j;
    MYFLT       *sp, *apn;
    int         nsmps = ksmps;
    int         count = (int) p->INOCOUNT;
    MYFLT       **args = p->args;

    for (j=0; j<count; j +=2) {
      nsmps = ksmps;
      ch = (int)(*args[j]+FL(0.5));
      apn = args[j+1];
      if (ch>nchnls) continue;
      if (!spoutactive) {
        sp = spout;
        do {
          for (i=1; i<=nchnls; i++) {
            *sp = ((i==ch) ? *apn++ : FL(0.0));
            sp++;
          }
        } while (--nsmps);
        spoutactive = 1;
      }
      else {
        sp = spout + ch-1;
        do {
          *sp += *apn++;
          sp += nchnls;
        }
        while (--nsmps);
      }
    }
    return OK;
}

/* k-rate i/o opcodes */
/* invalue and outvalue are used with the csoundAPI */
int invalset(INVAL *p)
{
    if (*p->valID == SSTRCOD && p->STRARG != NULL)
      strcpy(p->channelName, unquote(p->STRARG));
    else
      sprintf(p->channelName, "%d", (int)(*p->valID+FL(0.5)));
    return OK;
}

int kinval(INVAL *p)
{
    extern void InputValue(char *, MYFLT *);
    InputValue(p->channelName, p->value);  /* in csound.c */
    return OK;
}


int outvalset(OUTVAL *p)
{
    if (*p->valID == SSTRCOD && p->STRARG != NULL)
      strcpy(p->channelName, unquote(p->STRARG));
    else
      sprintf(p->channelName, "%d", (int)(*p->valID+FL(0.5)));
    return OK;
}

int koutval(OUTVAL *p)
{
    extern void OutputValue(char *, MYFLT);
    OutputValue(p->channelName, *p->value);  /* in csound.c */
    return OK;
}


