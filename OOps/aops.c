/* aops.c:

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
#include "aops.h"
#include <math.h>
#include <time.h>

#define POW2TABSIZI 4096
#if ULONG_MAX == 18446744073709551615UL
#  define POW2MAX   (24.0)
#else
#  define POW2MAX   (15.0)
#endif

#define EIPT3       (25.0/3.0)
#define LOGTWO      (0.69314718055994530942)
#define STEPS       (32768)
#define INTERVAL    (4.0)
#define ONEdLOG2    FL(1.4426950408889634074)
#define MIDINOTE0   (3.00)  /* Lowest midi note is 3.00 in oct & pch formats */

/* initialise the tables, called by csoundPreCompile() */
void csound_aops_init_tables(CSOUND *csound)
{
    int   i;
    if (csound->cpsocfrc==NULL)
      csound->cpsocfrc = (MYFLT *) csound->Malloc(csound, sizeof(MYFLT)*OCTRES);
    if (csound->powerof2==NULL)
      csound->powerof2 = (MYFLT *) csound->Malloc(csound,
                                                  sizeof(MYFLT)*POW2TABSIZI);
    for (i = 0; i < OCTRES; i++)
      csound->cpsocfrc[i] = POWER(FL(2.0), (MYFLT)i / OCTRES) * ONEPT;
    for (i = 0; i < POW2TABSIZI; i++) {
      csound->powerof2[i] =
        POWER(FL(2.0), (MYFLT)i * (MYFLT)(1.0/POW2TABSIZI) - FL(POW2MAX));
    }
}


MYFLT csoundPow2(CSOUND *csound, MYFLT a)
{
    int n;
    if (a > POW2MAX) a = POW2MAX;
    else if (a < -POW2MAX) a = -POW2MAX;
    /* 4096 * 15 */
    n = (int)MYFLT2LRND(a * FL(POW2TABSIZI)) + POW2MAX*POW2TABSIZI;
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
   IGN(csound);
   IGN(p);
    return OK;
}

int assign(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    *p->r = *p->a;
    return OK;
}

int aassign(CSOUND *csound, ASSIGN *p)
{
    uint32_t nsmps = CS_KSMPS;
    if (LIKELY(nsmps!=1)) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t nsmps = CS_KSMPS;
      /* the orchestra parser converts '=' to 'upsamp' if input arg is k-rate, */
      /* and skips the opcode if outarg == inarg */
      if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
      }
      memcpy(&p->r[offset], &p->a[offset], (nsmps-offset) * sizeof(MYFLT));
    }
    else
      *p->r =*p->a;
    return OK;
}

int ainit(CSOUND *csound, ASSIGN *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT aa = *p->a;
    int   n, nsmps = CS_KSMPS;
    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      p->r[n] = aa;
    return OK;
}

int minit(CSOUND *csound, ASSIGNM *p)
{
    unsigned int nargs = p->INCOUNT;
    unsigned int i;
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
    unsigned int nargs = p->INCOUNT;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    unsigned int   i, n, nsmps = CS_KSMPS;
    MYFLT aa = FL(0.0);
    early = nsmps - early;      /* Bit at end to ignore */
    if (UNLIKELY(nargs > p->OUTOCOUNT))
      return csound->InitError(csound,
                               Str("Cannot be more In arguments than Out in "
                                   "init (%d,%d)"),p->OUTOCOUNT, nargs);
    for (i=0; i<nargs; i++) {
      aa = *p->a[i];
      MYFLT *r =p->r[i];
      for (n = 0; n < nsmps; n++)
        r[n] = (n < offset || n > early ? FL(0.0) : aa);
    }
    for (; i<p->OUTOCOUNT; i++) {
      MYFLT *r =p->r[i];
      memset(r, '\0', nsmps*sizeof(MYFLT));
      for (n = 0; n < nsmps; n++)
        r[n] = (n < offset || n > early ? FL(0.0) : aa);
    }

    return OK;
}


int signum(CSOUND *csound, ASSIGN *p)
{
    MYFLT a = *p->a;
    int ans = (a==FL(0.0) ? 0 : a<FL(0.0) ? -1 : 1);
    *p->r = (MYFLT) ans;
    return OK;
}

/* ********COULD BE IMPROVED******** */
int asignum(CSOUND *csound, ASSIGN *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    unsigned int   i, nsmps = CS_KSMPS;
    MYFLT *a = p->a;
    memset(p->r, '\0', nsmps*sizeof(MYFLT));
    early = nsmps-early;
    for (i=offset; i<early; i++) {
      int ans = (a[i]==FL(0.0) ? 0 : a[i]<FL(0.0) ? -1 : 1);
      p->r[i] = (MYFLT) ans;
    }
    return OK;
}

#define RELATN(OPNAME,OP)                               \
  int OPNAME(CSOUND *csound, RELAT *p)                  \
  { IGN(csound); *p->rbool = (*p->a OP *p->b) ? 1 : 0;  return OK; }

RELATN(gt,>)
RELATN(ge,>=)
RELATN(lt,<)
RELATN(le,<=)
RELATN(eq,==)
RELATN(ne,!=)

#define LOGCLX(OPNAME,OP)                                       \
  int OPNAME(CSOUND *csound, LOGCL *p)                          \
  {  IGN(csound);*p->rbool = (*p->ibool OP *p->jbool) ? 1 : 0; return OK; }

LOGCLX(and,&&)
LOGCLX(or,||)

#define KK(OPNAME,OP)                                           \
  int OPNAME(CSOUND *csound, AOP *p)                            \
  { IGN(csound); *p->r = *p->a OP *p->b; return OK; }

KK(addkk,+)
KK(subkk,-)
KK(mulkk,*)
//KK(divkk,/)
int divkk(CSOUND *csound, AOP *p)
{
    MYFLT div = *p->b;
    IGN(csound);
    if (UNLIKELY(div==FL(0.0)))
      csound->Warning(csound, Str("Division by zero"));
    *p->r = *p->a / div;
    return OK;
}

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
    IGN(csound);
    *p->r = MOD(*p->a, *p->b);
    return OK;
}

#define KA(OPNAME,OP)                           \
  int OPNAME(CSOUND *csound, AOP *p) {          \
    uint32_t n, nsmps = CS_KSMPS;                    \
    if (LIKELY(nsmps!=1)) {                     \
      MYFLT   *r, a, *b;                          \
      uint32_t offset = p->h.insdshead->ksmps_offset;  \
      uint32_t early  = p->h.insdshead->ksmps_no_end;  \
      r = p->r;                                        \
      a = *p->a;                                       \
      b = p->b;                                        \
      if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT)); \
      if (UNLIKELY(early)) {                           \
        nsmps -= early;                                \
        memset(&r[nsmps], '\0', early*sizeof(MYFLT));  \
      }                                                \
      for (n=offset; n<nsmps; n++)                     \
        r[n] = a OP b[n];                              \
      return OK;                                       \
    }                                                  \
    else {                                             \
        *p->r = *p->a OP *p->b;                        \
      return OK;                                       \
    }                                                  \
  }


KA(addka,+)
KA(subka,-)
KA(mulka,*)
KA(divka,/)

/* ********COULD BE IMPROVED******** */
int modka(CSOUND *csound, AOP *p)
{
    MYFLT   *r, a, *b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    r = p->r;
    a = *p->a;
    b = p->b;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++)
      r[n] = MOD(a, b[n]);
    return OK;
}

#define AK(OPNAME,OP)                           \
  int OPNAME(CSOUND *csound, AOP *p) {          \
    uint32_t n, nsmps = CS_KSMPS;               \
    if (LIKELY(nsmps != 1)) {                   \
      MYFLT   *r, *a, b;                        \
      uint32_t offset = p->h.insdshead->ksmps_offset;  \
      uint32_t early  = p->h.insdshead->ksmps_no_end;  \
      r = p->r;                                 \
      a = p->a;                                 \
      b = *p->b;                                \
      if (UNLIKELY(offset))                     \
        memset(r, '\0', offset*sizeof(MYFLT));  \
      if (UNLIKELY(early)) {                    \
        nsmps -= early;                         \
        memset(&r[nsmps], '\0', early*sizeof(MYFLT)); \
      }                                         \
      for (n=offset; n<nsmps; n++)              \
        r[n] = a[n] OP b;                       \
      return OK;                                \
    }                                           \
    else {                                      \
      p->r[0] = p->a[0] OP *p->b;               \
      return OK;                                \
    }                                           \
}

AK(addak,+)
AK(subak,-)
AK(mulak,*)
//AK(divak,/)
int divak(CSOUND *csound, AOP *p) {
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT b = *p->b;
    if (LIKELY(nsmps != 1)) {
      MYFLT   *r, *a;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      r = p->r;
      a = p->a;
      b = *p->b;
      if (UNLIKELY(b==FL(0.0)))
        csound->Warning(csound, Str("Division by zero"));
      if (UNLIKELY(offset))
        memset(r, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&r[nsmps], '\0', early*sizeof(MYFLT)); \
      }
      for (n=offset; n<nsmps; n++)
        r[n] = a[n] / b;
      return OK;
    }
    else {
      if (UNLIKELY(b==FL(0.0)))
        csound->Warning(csound, Str("Division by zero"));
      p->r[0] = p->a[0] / b;
      return OK;
    }
}


/* ********COULD BE IMPROVED******** */
int modak(CSOUND *csound, AOP *p)
{
    MYFLT   *r, *a, b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    r = p->r;
    a = p->a;
    b = *p->b;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++)
      r[n] = MOD(a[n], b);
    return OK;
}

#define AA(OPNAME,OP)                           \
  int OPNAME(CSOUND *csound, AOP *p) {          \
  MYFLT   *r, *a, *b;                           \
  uint32_t n, nsmps = CS_KSMPS;                 \
  if (LIKELY(nsmps!=1)) {                       \
    uint32_t offset = p->h.insdshead->ksmps_offset;       \
    uint32_t early  = p->h.insdshead->ksmps_no_end;  \
    r = p->r;                                   \
    a = p->a;                                   \
    b = p->b;                                   \
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT)); \
    if (UNLIKELY(early)) {                      \
      nsmps -= early;                           \
      memset(&r[nsmps], '\0', early*sizeof(MYFLT)); \
    }                                           \
    for (n=offset; n<nsmps; n++)                \
      r[n] = a[n] OP b[n];                      \
    return OK;                                  \
  }                                             \
    else {                                      \
      *p->r = *p->a OP *p->b;                    \
      return OK;                                \
    }                                           \
  }


/* VL
   experimental code using SIMD for operations
*/
typedef double v2d __attribute__ ((vector_size (128)));
#define AA_VECTOR(OPNAME,OP)                           \
  int OPNAME(CSOUND *csound, AOP *p) {          \
  MYFLT   *r, *a, *b;                           \
  v2d     rv, av, bv;                           \
  uint32_t n, nsmps = CS_KSMPS;                 \
  if (LIKELY(nsmps!=1)) {                       \
    uint32_t offset = p->h.insdshead->ksmps_offset;       \
    uint32_t early  = p->h.insdshead->ksmps_no_end;  \
    r = p->r;                                   \
    a = p->a;                                   \
    b = p->b;                                   \
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT)); \
    if (UNLIKELY(early)) {                      \
      nsmps -= early;                           \
      memset(&r[nsmps], '\0', early*sizeof(MYFLT)); \
    }                                           \
    for (n=offset; n<nsmps; n+=2){              \
       memcpy(&av,&a[n],2*sizeof(MYFLT));       \
       memcpy(&bv,&b[n],2*sizeof(MYFLT));       \
       rv = av OP bv;                  \
       memcpy(&r[n],&rv,2*sizeof(MYFLT));       \
    }                                           \
    return OK;                                  \
  }                                             \
    else {                                      \
      *p->r = *p->a OP *p->b;                    \
      return OK;                                \
    }                                           \
  }

AA(addaa,+)
AA(subaa,-)
AA(mulaa,*)
AA(divaa,/)

/* ********COULD BE IMPROVED******** */
int modaa(CSOUND *csound, AOP *p)
{
    MYFLT   *r, *a, *b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    r = p->r;
    a = p->a;
    b = p->b;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++)
      r[n] = MOD(a[n], b[n]);
    return OK;
}

int divzkk(CSOUND *csound, DIVZ *p)
{
    IGN(csound);
    *p->r = (*p->b != FL(0.0) ? *p->a / *p->b : *p->def);
    return OK;
}
/* ********COULD BE IMPROVED******** */

int divzka(CSOUND *csound, DIVZ *p)
{
    uint32_t n;
    MYFLT    *r, a, *b, def;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    r = p->r;
    a = *p->a;
    b = p->b;
    def = *p->def;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++)
      r[n] = (b[n]==FL(0.0) ? def : a / b[n]);
    return OK;
}

/* ********COULD BE IMPROVED******** */
int divzak(CSOUND *csound, DIVZ *p)
{
    uint32_t n;
    MYFLT    *r, *a, b, def;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    r = p->r;
    a = p->a;
    b = *p->b;
    def = *p->def;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (UNLIKELY(b==FL(0.0))) {
      for (n=offset; n<nsmps; n++) r[n] = def;
    }
    else {
      for (n=offset; n<nsmps; n++) r[n] = a[n] / b;
    }
    return OK;
}

/* ********COULD BE IMPROVED******** */
int divzaa(CSOUND *csound, DIVZ *p)
{
    uint32_t n;
    MYFLT    *r, *a, *b, def;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    r = p->r;
    a = p->a;
    b = p->b;
    def = *p->def;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++)
      r[n] = (b[n]==FL(0.0) ? def : a[n] / b[n]);
    return OK;
}

int conval(CSOUND *csound, CONVAL *p)
{
    IGN(csound);
    if (*p->cond)
      *p->r = *p->a;
    else
      *p->r = *p->b;
    return OK;
}

/* ********COULD BE IMPROVED******** */
int aconval(CSOUND *csound, CONVAL *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset*sizeof(MYFLT);
    uint32_t early  = p->h.insdshead->ksmps_no_end*sizeof(MYFLT);
    MYFLT   *r, *s;

    r = p->r;
    if (*p->cond)
      s = p->a;
    else s = p->b;
    if (r!=s) {
      memset(r, '\0', offset);
      memcpy(&r[offset], &s[offset], CS_KSMPS*sizeof(MYFLT)-offset-early);
      memset(&r[offset-early], '\0', early);
    }
    return OK;
}

int int1(CSOUND *csound, EVAL *p)               /* returns signed whole no. */
{
    MYFLT intpart;
    IGN(csound);
    MODF(*p->a, &intpart);
    *p->r = intpart;
    return OK;
}

/* ********COULD BE IMPROVED******** */
int int1a(CSOUND *csound, EVAL *p)              /* returns signed whole no. */
{
    MYFLT        intpart, *a=p->a, *r=p->r;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      MODF(a[n], &intpart);
      r[n] = intpart;
    }
    return OK;
}

int frac1(CSOUND *csound, EVAL *p)              /* returns positive frac part */
{
    MYFLT intpart, fracpart;
    IGN(csound);
    fracpart = MODF(*p->a, &intpart);
    *p->r = fracpart;
    return OK;
}

/* ********COULD BE IMPROVED******** */
int frac1a(CSOUND *csound, EVAL *p)             /* returns positive frac part */
{
    MYFLT intpart, fracpart, *r = p->r, *a = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      fracpart = MODF(a[n], &intpart);
      r[n] = fracpart;
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
    IGN(csound);
    *p->r = (MYFLT) MYFLT2LRND(*p->a);
    return OK;
}

/* ********COULD BE IMPROVED******** */
int int1a_round(CSOUND *csound, EVAL *p)        /* round to nearest integer */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    MYFLT *r=p->r, *a=p->a;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      r[n] = (MYFLT)MYFLT2LRND(a[n]);
    return OK;
}

int int1_floor(CSOUND *csound, EVAL *p)         /* round down */
{
    IGN(csound);
    *p->r = (MYFLT)(MYFLOOR(*p->a));
    return OK;
}

/* ********COULD BE IMPROVED******** */
int int1a_floor(CSOUND *csound, EVAL *p)        /* round down */
{
    MYFLT    *a=p->a, *r=p->r;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      r[n] = (MYFLT)(MYFLOOR(a[n]));
    return OK;
}

int int1_ceil(CSOUND *csound, EVAL *p)          /* round up */
{
    IGN(csound);
    *p->r = (MYFLT)(MYCEIL(*p->a));
    return OK;
}

/* ********COULD BE IMPROVED******** */
int int1a_ceil(CSOUND *csound, EVAL *p)         /* round up */
{
    MYFLT    *a=p->a, *r=p->r;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      r[n] = (MYFLT)(MYCEIL(a[n]));
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
  {  IGN(csound); *p->r = LIBNAME(*p->a); return OK; }
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
LIB1(log21,LOG2)

int atan21(CSOUND *csound, AOP *p)
{
    IGN(csound);
    *p->r = ATAN2(*p->a, *p->b);
    return OK;
}

/* ********COULD BE IMPROVED******** */
#define LIBA(OPNAME,LIBNAME) int OPNAME(CSOUND *csound, EVAL *p) {      \
    uint32_t offset = p->h.insdshead->ksmps_offset;                     \
    uint32_t early  = p->h.insdshead->ksmps_no_end;                     \
    uint32_t n, nsmps =CS_KSMPS;                                        \
    MYFLT   *r, *a;                                                     \
    r = p->r;                                                           \
    a = p->a;                                                           \
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));        \
    if (UNLIKELY(early)) {                                              \
      nsmps -= early;                                                   \
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));                     \
    }                                                                   \
    for (n = offset; n < nsmps; n++)                                    \
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
LIBA(log2a,LOG2)

/* ********COULD BE IMPROVED******** */
int atan2aa(CSOUND *csound, AOP *p)
{
    MYFLT   *r, *a, *b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;

    r = p->r;
    a = p->a;
    b = p->b;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      r[n] = ATAN2(a[n], b[n]);
    return OK;
}

int dbamp(CSOUND *csound, EVAL *p)
{
    IGN(csound);
    *p->r = LOG(FABS(*p->a)) / LOG10D20;
    return OK;
}

int ampdb(CSOUND *csound, EVAL *p)
{
    IGN(csound);
    *p->r = EXP(*p->a * LOG10D20);
    return OK;
}

int aampdb(CSOUND *csound, EVAL *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    MYFLT   *r = p->r, *a = p->a;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
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

/* ********COULD BE IMPROVED******** */
int aampdbfs(CSOUND *csound, EVAL *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    MYFLT   *r, *a;

    r = p->r;
    a = p->a;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
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
   IGN(csound);
    double fract, oct;
    fract = modf((double)*p->a, &oct);
    fract *= EIPT3;
    *p->r = (MYFLT)(oct + fract);
    return OK;
}

int pchoct(CSOUND *csound, EVAL *p)
{
   IGN(csound);
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
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;

    a = p->a;
    r = p->r;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      loct = (int)(a[n] * OCTRES);
      r[n] = CPSOCTL(loct);
    }
    return OK;
}

int octcps(CSOUND *csound, EVAL *p)
{
   IGN(csound);
    *p->r = (LOG(*p->a /(MYFLT)ONEPT) / (MYFLT)LOGTWO);
    return OK;
}

int cpspch(CSOUND *csound, EVAL *p)
{
    double fract, oct;
    int    loct;
    fract = modf((double)*p->a, &oct);
    fract *= EIPT3;
    loct = (int)MYFLT2LRND((oct + fract) * OCTRES);
    *p->r = (MYFLT)CPSOCTL(loct);
    return OK;
}

int cpsmidinn(CSOUND *csound, EVAL *p)
{
    IGN(csound);
    *p->r = pow(FL(2.0), (*p->a - FL(69.0)) / FL(12.0)) * FL(440.0);
    return OK;
}

int octmidinn(CSOUND *csound, EVAL *p)
{
    IGN(csound);
    /* Convert Midi Note number to 8ve.decimal format */
    *p->r = (*p->a / FL(12.0)) + FL(MIDINOTE0);
    return OK;
}

int pchmidinn(CSOUND *csound, EVAL *p)
{
   IGN(csound);
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
        return csound->PerfError(csound, p->h.insdshead,Str("No tuning table %d"),
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
        return csound->PerfError(csound, p->h.insdshead,Str("No tuning table %d"),
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
    return csound->PerfError(csound, p->h.insdshead,Str("cpstun: invalid table"));
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
    return csound->PerfError(csound, p->h.insdshead,Str("cpstun: invalid table"));
}

int logbasetwo_set(CSOUND *csound, EVAL *p)
{
    IGN(p);
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

/* ********COULD BE IMPROVED******** */
int powoftwoa(CSOUND *csound, EVAL *p)
{                                   /* by G.Maldonado, liberalised by JPff */
    MYFLT    *a=p->a, *r=p->r;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      r[n] = csound->Pow2(csound,a[n]);
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
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    a = p->a;
    r = p->r;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
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

/* ********COULD BE IMPROVED******** */
int acent(CSOUND *csound, EVAL *p)        /* JPff */
{
    MYFLT *r, *a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    a = p->a;
    r = p->r;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
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

/* ********COULD BE IMPROVED******** */
int dba(CSOUND *csound, EVAL *p)          /* JPff */
{
    MYFLT *r, *a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    a = p->a;
    r = p->r;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
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
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    a = p->a;
    r = p->r;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
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
    uint32_t offset = p->h.insdshead->ksmps_offset*sizeof(MYFLT);
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset)) memset(p->ar, '\0', offset);
    memcpy(&p->ar[offset], CS_SPIN, (CS_KSMPS-early) * sizeof(MYFLT)-offset);
    if (UNLIKELY(early))
      memset(&p->ar[CS_KSMPS-early], '\0', early * sizeof(MYFLT));
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int inarray(CSOUND *csound, INA *p)
{
    MYFLT *data = p->tabout->data;
    uint32_t n = p->tabout->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset*sizeof(MYFLT);
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT *sp = CS_SPIN;
    uint32_t m, nsmps =CS_KSMPS, i;
    uint32_t ksmps = nsmps;

    if ((int)n>csound->inchnls) n = csound->inchnls;
    CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset)) for (i = 0; i < n; i++)
                  memset(&data[i*ksmps], '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      for (i = 0; i < n; i++)
        memset(&data[nsmps+i*ksmps], '\0', early*sizeof(MYFLT));
    }
    for (m = offset; m < nsmps; m++) {
      for (i = 0; i < n; i++)
        data[m+i*ksmps] = *sp++;
    }
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int ins(CSOUND *csound, INS *p)
{
    MYFLT       *sp, *ar1, *ar2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS, k;
    CSOUND_SPIN_SPINLOCK
    sp = CS_SPIN;
    ar1 = p->ar1;
    ar2 = p->ar2;
    if (UNLIKELY(offset)) {
      memset(ar1, '\0', offset*sizeof(MYFLT));
      memset(ar2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->ar1[nsmps],'\0',  early * sizeof(MYFLT));
      memset(&p->ar2[nsmps], '\0', early * sizeof(MYFLT));
    }
    for (n=offset, k=0; n<nsmps; n++, k+=2) {
      ar1[n] = sp[k];
      ar2[n] = sp[k+1];
    }
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int inq(CSOUND *csound, INQ *p)
{
    MYFLT       *sp = CS_SPIN, *ar1 = p->ar1, *ar2 = p->ar2,
                                    *ar3 = p->ar3, *ar4 = p->ar4;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS, k;
    CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset)) {
      memset(ar1, '\0', offset*sizeof(MYFLT));
      memset(ar2, '\0', offset*sizeof(MYFLT));
      memset(ar3, '\0', offset*sizeof(MYFLT));
      memset(ar4, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar1[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar2[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar3[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar4[nsmps], '\0', early * sizeof(MYFLT));
    }
    for (n=offset, k=0; n<nsmps; n++, k+=4) {
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
    MYFLT       *sp = CS_SPIN, *ar1 = p->ar1, *ar2 = p->ar2, *ar3 = p->ar3,
                                    *ar4 = p->ar4, *ar5 = p->ar5, *ar6 = p->ar6;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS, k;
    CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset)) {
      memset(ar1, '\0', offset*sizeof(MYFLT));
      memset(ar2, '\0', offset*sizeof(MYFLT));
      memset(ar3, '\0', offset*sizeof(MYFLT));
      memset(ar4, '\0', offset*sizeof(MYFLT));
      memset(ar5, '\0', offset*sizeof(MYFLT));
      memset(ar6, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar1[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar2[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar3[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar4[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar5[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar6[nsmps], '\0', early * sizeof(MYFLT));
    }
    for (n=offset, k=0; n<nsmps; n++, k+=6) {
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
    MYFLT       *sp = CS_SPIN, *ar1 = p->ar1, *ar2 = p->ar2, *ar3 = p->ar3,
                                    *ar4 = p->ar4, *ar5 = p->ar5, *ar6 = p->ar6,
                                    *ar7 = p->ar7, *ar8 = p->ar8;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS, k;
    CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset)) {
      memset(ar1, '\0', offset*sizeof(MYFLT));
      memset(ar2, '\0', offset*sizeof(MYFLT));
      memset(ar3, '\0', offset*sizeof(MYFLT));
      memset(ar4, '\0', offset*sizeof(MYFLT));
      memset(ar5, '\0', offset*sizeof(MYFLT));
      memset(ar6, '\0', offset*sizeof(MYFLT));
      memset(ar7, '\0', offset*sizeof(MYFLT));
      memset(ar8, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar1[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar2[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar3[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar4[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar5[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar6[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar7[nsmps], '\0', early * sizeof(MYFLT));
      memset(&ar8[nsmps], '\0', early * sizeof(MYFLT));
    }
    for (n=offset, k=0; n<nsmps; n++, k+=8) {
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

static int inn(CSOUND *csound, INALL *p, uint32_t n)
{
    MYFLT *sp = CS_SPIN, **ara = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t m, nsmps =CS_KSMPS, i;

    CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset)) for (i = 0; i < n; i++)
                  memset(ara[i], '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      for (i = 0; i < n; i++)
        memset(ara[i], '\0', early*sizeof(MYFLT));
    }
    for (m = offset; m < nsmps; m++) {
      for (i = 0; i < n; i++)
        *ara[i] = *sp++;
    }
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int in16(CSOUND *csound, INALL *p)
{
    return inn(csound, p, 16u);
}

int in32(CSOUND *csound, INALL *p)
{
    return inn(csound, p, 32u);
}

int inch_opcode1(CSOUND *csound, INCH1 *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS, ch;
    MYFLT *sp, *ain;

      ch = ((int)*p->ch + FL(0.5));
      if (UNLIKELY(ch > (uint32_t)csound->inchnls)) {
        csound->Message(csound, Str("Input channel %d too large; ignored"), ch);
        memset(p->ar, 0, sizeof(MYFLT)*nsmps);
        //        return OK;
      }
      else {
        sp = CS_SPIN + (ch - 1);
        ain = p->ar;
        if (UNLIKELY(offset)) memset(ain, '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          nsmps -= early;
          memset(&ain[nsmps], '\0', early*sizeof(MYFLT));
        }
        for (n = offset; n < nsmps; n++) {
          ain[n] = *sp;
          sp += csound->inchnls;
        }
      }

    return OK;
}


int inch_opcode(CSOUND *csound, INCH *p)
{                               /* Rewritten to allow multiple args upto 40 */
    uint32_t nc, nChannels = p->INCOUNT;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS, ch;
    MYFLT *sp, *ain;
    if (UNLIKELY(nChannels != p->OUTOCOUNT))
      return
        csound->PerfError(csound, p->h.insdshead,
                          Str("Input and output argument count differs in inch"));
    for (nc=0; nc<nChannels; nc++) {
      ch = (int)(*p->ch[nc] + FL(0.5));
      if (UNLIKELY(ch > (uint32_t)csound->inchnls)) {
        csound->Message(csound, Str("Input channel %d too large; ignored"), ch);
        memset(p->ar[nc], 0, sizeof(MYFLT)*nsmps);
        //        return OK;
      }
      else {
        sp = CS_SPIN + (ch - 1);
        ain = p->ar[nc];
        if (UNLIKELY(offset)) memset(ain, '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          nsmps -= early;
          memset(&ain[nsmps], '\0', early*sizeof(MYFLT));
        }
        for (n = offset; n < nsmps; n++) {
          ain[n] = *sp;
          sp += csound->inchnls;
        }
      }
    }
    return OK;
}


int inall_opcode(CSOUND *csound, INALL *p)
{
    uint32_t n = (int)p->OUTOCOUNT, m;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t    i,j = 0, k = 0, nsmps = CS_KSMPS;
    uint32_t early  = nsmps - p->h.insdshead->ksmps_no_end;
    MYFLT *spin = CS_SPIN;
    CSOUND_SPIN_SPINLOCK
    m = (n < (uint32_t)csound->inchnls ? n : (uint32_t)csound->inchnls);
    for (j=0; j<nsmps; j++)
      if (j<offset || j>early) {
        for (i=0 ; i < n; i++)
          p->ar[i][j] = FL(0.0);
      }
      else {
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

int outs1(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp= CS_SPOUT, *ap1= p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n, m;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;

    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      for (n=0, m=0; n<nsmps; n++) {
        sp[m++] = (n<offset || n>early) ? FL(0.0) : ap1[n];
        sp[m++] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      for (n=0, m=0; n<early; n++, m+=2) {
        if (n>=offset) sp[m]   += ap1[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outs2(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = CS_SPOUT, *ap2 = p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n, m;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;

    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      for (n=0, m=0; n<nsmps; n++) {
        sp[m++] = FL(0.0);
        sp[m++] = (n<offset||n>early) ? FL(0.0) : ap2[n];
      }
      csound->spoutactive = 1;
    }
    else {
      for (n=0, m=1; n<early; n++, m+=2) {
        if (n>=offset) sp[m] += ap2[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outs12(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = CS_SPOUT, *ap = p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n, m;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;
    CSOUND_SPOUT_SPINLOCK

    if (!csound->spoutactive) {
      for (n=0, m=0; n<nsmps; n++, m+=2) {
        sp[m] = sp[m+1] = (n<offset||n>early) ? FL(0.0) : ap[n];
      }
      csound->spoutactive = 1;
    }
    else {
      for (n=0, m=0; n<early; n++) {
        if (n<offset) m+=2;
        else {
          sp[m++] += ap[n];
          sp[m++] += ap[n];
        }
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outq1(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = CS_SPOUT, *ap1 = p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n, m;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = (n<offset||n>early) ? FL(0.0) : ap1[n];
        sp[m+1] = FL(0.0);
        sp[m+2] = FL(0.0);
        sp[m+3] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      for (n=0, m=0; n<early; n++, m+=4) {
        if (n>=offset) sp[m]   += ap1[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outq2(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = CS_SPOUT, *ap2 = p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n, m;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = (n<offset||n>early) ? FL(0.0) : ap2[n];
        sp[m+2] = FL(0.0);
        sp[m+3] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      for (n=0, m=1; n<early; n++, m+=4) {
        if (n>=offset) sp[m]   += ap2[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outq3(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = CS_SPOUT, *ap3 = p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n, m;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = FL(0.0);
        sp[m+2] = (n<offset||n>early) ? FL(0.0) : ap3[n];
        sp[m+3] = FL(0.0);
      }
      csound->spoutactive = 1;
    }
    else {
      for (n=0, m=2; n<early; n++, m+=4) {
        if (n>=offset) sp[m]   += ap3[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int outq4(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = CS_SPOUT, *ap4 = p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n, m;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      for (n=0, m=0; n<nsmps; n++, m+=4) {
        sp[m]   = FL(0.0);
        sp[m+1] = FL(0.0);
        sp[m+2] = FL(0.0);
        sp[m+3] = (n<offset||n>early) ? FL(0.0) : ap4[n];
      }
      csound->spoutactive = 1;
    }
    else {
      for (n=0, m=3; n<early; n++, m+=4) {
        if (n>=offset) sp[m]   += ap4[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

inline static int outn(CSOUND *csound, uint32_t n, OUTX *p)
{
    uint32_t nsmps =CS_KSMPS,  i, j, k=0;
    if (csound->oparms->sampleAccurate) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;

      CSOUND_SPOUT_SPINLOCK
      if (!csound->spoutactive) {

        for (j=0; j<nsmps; j++) {
          for (i=0; i<n; i++) {
            CS_SPOUT[k + i] = (j<offset||j>early) ? FL(0.0) : p->asig[i][j];
          }
          for ( ; i < csound->nchnls; i++) {
            CS_SPOUT[k + i] = FL(0.0);
          }
          k += csound->nchnls;
        }
        csound->spoutactive = 1;
      }
      else {
        //if(offset) printf("offset = %d, %d nsmps\n", offset, nsmps);
        // no need to offset as the data is already offset in the asig
        for (j=0; j<early; j++) {
          for (i=0; i<n; i++) {
            CS_SPOUT[k + i] += p->asig[i][j];
          }
          k += csound->nchnls;
        }
      }
      CSOUND_SPOUT_SPINUNLOCK
    }
    else {
      CSOUND_SPOUT_SPINLOCK

      if (!csound->spoutactive) {
        for (j=0; j<nsmps; j++) {
          for (i=0; i<n; i++) {
            CS_SPOUT[k + i] = p->asig[i][j];
          }
          for ( ; i < csound->nchnls; i++) {
            CS_SPOUT[k + i] = FL(0.0);
          }
          k += csound->nchnls;
        }
        csound->spoutactive = 1;
      }
      else {
        for (j=0; j<nsmps; j++) {
          for (i=0; i<n; i++) {
            CS_SPOUT[k + i] += p->asig[i][j];
          }
          k += csound->nchnls;
        }
      }
      CSOUND_SPOUT_SPINUNLOCK
    }
    return OK;
}

int outall(CSOUND *csound, OUTX *p)             /* Output a list of channels */
{
    uint32_t nch = p->INOCOUNT;

    return outn(csound, (nch <= csound->nchnls ? nch : csound->nchnls), p);
}

int outarr(CSOUND *csound, OUTARRAY *p)
{
    uint32_t nsmps =CS_KSMPS,  i, j, k=0;
    uint32_t ksmps = nsmps;
    uint32_t n = p->tabin->sizes[0];
    MYFLT *data = p->tabin->data;
    if (csound->oparms->sampleAccurate) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;

      CSOUND_SPOUT_SPINLOCK
      if (!csound->spoutactive) {
        for (j=0; j<nsmps; j++) {
          for (i=0; i<n; i++) {
            CS_SPOUT[k + i] = (j<offset||j>early) ? FL(0.0) : data[j+i*ksmps];
          }
          for ( ; i < csound->nchnls; i++) {
            CS_SPOUT[k + i] = FL(0.0);
          }
          k += csound->nchnls;
        }
        csound->spoutactive = 1;
      }
      else {
        /* no need to offset data is already offset in the buffer*/
        for (j=0; j<early; j++) {
          for (i=0; i<n; i++) {
            CS_SPOUT[k + i] += data[j+i*ksmps];
          }
          k += csound->nchnls;
        }
      }
      CSOUND_SPOUT_SPINUNLOCK
    }
    else {
      CSOUND_SPOUT_SPINLOCK

      if (!csound->spoutactive) {
        for (j=0; j<nsmps; j++) {
          for (i=0; i<n; i++) {
            CS_SPOUT[k + i] = data[j+i*ksmps];
          }
          for ( ; i < csound->nchnls; i++) {
            CS_SPOUT[k + i] = FL(0.0);
          }
          k += csound->nchnls;
        }
        csound->spoutactive = 1;
      }
      else {
        for (j=0; j<nsmps; j++) {
          for (i=0; i<n; i++) {
            CS_SPOUT[k + i] += data[j+i*ksmps];
          }
          k += csound->nchnls;
        }
      }
      CSOUND_SPOUT_SPINUNLOCK
    }
    return OK;
}

int outch(CSOUND *csound, OUTCH *p)
{
    uint32_t    ch;
    MYFLT       *sp, *apn;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    nsmps = CS_KSMPS,  i, j, n;
    uint32_t    early = nsmps-p->h.insdshead->ksmps_no_end;
    uint32_t    count = p->INOCOUNT;
    MYFLT       **args = p->args;
    uint32_t    nchnls = csound->nchnls;
    CSOUND_SPOUT_SPINLOCK
    for (j = 0; j < count; j += 2) {
      ch = (int)(*args[j] + FL(0.5));
      apn = args[j + 1];
      if (ch > nchnls) continue;
      if (!csound->spoutactive) {
        sp = CS_SPOUT;
        for (n=0; n<nsmps; n++) {
          for (i = 1; i <= nchnls; i++) {
            *sp = ((i == ch && n>=offset && n<early) ? apn[n] : FL(0.0));
            sp++;
          }
        }
        csound->spoutactive = 1;
      }
      else {
        sp = CS_SPOUT + (ch - 1);
        /* no need to offset */
        for (n=0; n<early; n++) {
          /* if (n>=offset)*/ *sp += apn[n];
          sp += nchnls;
        }
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int is_NaN(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    *p->r = isnan(*p->a);
    return OK;
}

/* ********COULD BE IMPROVED******** */
int is_NaNa(CSOUND *csound, ASSIGN *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t k, nsmps = CS_KSMPS;
    uint32_t early  = nsmps - p->h.insdshead->ksmps_no_end;
    MYFLT *a = p->a;
    *p->r = FL(0.0);
    for (k=offset; k<early; k++)
      *p->r += isnan(a[k]);
    return OK;
}

int is_inf(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    *p->r = isinf(*p->a);
    return OK;
}

/* ********COULD BE IMPROVED******** */
int is_infa(CSOUND *csound, ASSIGN *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t k, nsmps = CS_KSMPS;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;
    MYFLT *a = p->a;
    MYFLT ans = FL(0.0);
    int sign = 1;
    for (k=offset; k<early; k++) {
      if (isinf(a[k]))
        if (ans==FL(0.0)) sign = (int)isinf(a[k]);
      ans++;
    }
    *p->r = ans*sign;
    return OK;
}

int error_fn(CSOUND *csound, ERRFN *p)
{
   IGN(p);
    return csound->InitError(csound, Str("Unknown function called"));
}

 /* ------------------------------------------------------------------------ */

int monitor_opcode_perf(CSOUND *csound, MONITOR_OPCODE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, j, nsmps = CS_KSMPS;

    if (csound->spoutactive) {
      int   k = 0;
      for (i = 0; i<nsmps; i++) {
        for (j = 0; j<csound->GetNchnls(csound); j++) {
          if (i<offset||i>nsmps-early)
            p->ar[j][i] = FL(0.0);
          else
            p->ar[j][i] = CS_SPOUT[k];
          k++;
        }
      }
    }
    else {
      for (j = 0; j<csound->GetNchnls(csound); j++) {
        for (i = 0; i<CS_KSMPS; i++) {
          p->ar[j][i] = FL(0.0);
        }
      }
    }
    return OK;
}

int monitor_opcode_init(CSOUND *csound, MONITOR_OPCODE *p)
{
    if (UNLIKELY(csound->GetOutputArgCnt(p) != (int)csound->GetNchnls(csound)))
      return csound->InitError(csound, Str("number of arguments != nchnls"));
    p->h.opadr = (SUBR) monitor_opcode_perf;
    return OK;
}

/* -------------------------------------------------------------------- */

int outRange_i(CSOUND *csound, OUTRANGE *p)
{
   IGN(csound);
    p->narg = p->INOCOUNT-1;

    return OK;
}

int outRange(CSOUND *csound, OUTRANGE *p)
{
    int j;
    //uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int nchnls = csound->GetNchnls(csound);
    MYFLT *ara[VARGMAX];
    int startChan = (int) *p->kstartChan -1;
    MYFLT *sp = CS_SPOUT + startChan;
    int narg = p->narg;


    if (startChan < 0)
      return csound->PerfError(csound, p->h.insdshead,
                               Str("outrg: channel number cannot be < 1 "
                                   "(1 is the first channel)"));

    for (j = 0; j < narg; j++)
      ara[j] = p->argums[j];

    if (!csound->spoutactive) {
      memset(CS_SPOUT, 0, nsmps * nchnls * sizeof(MYFLT));
      /* no need to offset */
      for (n=0; n<nsmps-early; n++) {
        int i;
        MYFLT *sptemp = sp;
        for (i=0; i < narg; i++)
          sptemp[i] = ara[i][n];
        sp += nchnls;
      }
      csound->spoutactive = 1;
    }
    else {
      for (n=0; n<nsmps-early; n++) {
        int i;
        MYFLT *sptemp = sp;
        for (i=0; i < narg; i++)
          sptemp[i] += ara[i][n];
        sp += nchnls;
      }
    }
    return OK;
}
/* -------------------------------------------------------------------- */
