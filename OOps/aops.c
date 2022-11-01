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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
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
    int32_t   i;
    if (csound->cpsocfrc==NULL)
      csound->cpsocfrc = (MYFLT *) csound->Malloc(csound, sizeof(MYFLT)*OCTRES);
    /* if (csound->powerof2==NULL) */
    /*   csound->powerof2 = (MYFLT *) csound->Malloc(csound, */
    /*                                               sizeof(MYFLT)*POW2TABSIZI); */
    for (i = 0; i < OCTRES; i++)
      csound->cpsocfrc[i] = POWER(FL(2.0), (MYFLT)i / OCTRES) * ONEPT;
    /* for (i = 0; i < POW2TABSIZI; i++) { */
    /*   csound->powerof2[i] = */
    /*     POWER(FL(2.0), (MYFLT)i * (MYFLT)(1.0/POW2TABSIZI) - FL(POW2MAX)); */
    /* } */
}


MYFLT csoundPow2(CSOUND *csound, MYFLT a)
{
    /* int32_t n; */
    if (a > POW2MAX) a = POW2MAX;
    else if (a < -POW2MAX) a = -POW2MAX;
    return POWER(FL(2.0), a);
    /* 4096 * 15 */
    /* n = (int32_t)MYFLT2LRND(a * FL(POW2TABSIZI)) + POW2MAX*POW2TABSIZI; */
    /* return ((MYFLT) (1UL << (n >> 12)) * csound->powerof2[n & (POW2TABSIZI-1)]); */
}


/*static inline MYFLT pow2(MYFLT a)
{
    int32_t n = (int32_t)MYFLT2LRND(a * FL(POW2TABSIZI)) + POW2MAX*POW2TABSIZI;
    return ((MYFLT) (1 << (n >> 12)) * powerof2[n & (POW2TABSIZI-1)]);
}*/

int32_t rassign(CSOUND *csound, ASSIGN *p)
{
    /* already assigned by otran */
    IGN(csound);
    IGN(p);
    return OK;
}

int32_t assign(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    *p->r = *p->a;
    return OK;
}

int32_t aassign(CSOUND *csound, ASSIGN *p, int32_t islocal)
{
    IGN(csound);
    uint32_t nsmps = CS_KSMPS;
    if (LIKELY(nsmps!=1)) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t nsmps = CS_KSMPS;
      /* the orchestra parser converts '=' to 'upsamp' if input arg is k-rate, */
      /* and skips the opcode if outarg == inarg */
      if (UNLIKELY(islocal &&offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        if (islocal) memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
      }
      memcpy(&p->r[offset], &p->a[offset], (nsmps-offset) * sizeof(MYFLT));
    }
    else
      *p->r =*p->a;
    return OK;
}

int32_t gaassign(CSOUND *csound, ASSIGN *p)
{   return aassign(csound, p, 0); }

int32_t laassign(CSOUND *csound, ASSIGN *p)
{   return aassign(csound, p, 1); }

int32_t ainit(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT aa = *p->a;
    int32_t   n, nsmps = CS_KSMPS;
    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      p->r[n] = aa;
    return OK;
}

int32_t minit(CSOUND *csound, ASSIGNM *p)
{
    uint32_t nargs = p->INCOUNT;
    uint32_t nout = p->OUTOCOUNT;
    uint32_t i;
    MYFLT *tmp;
    if (UNLIKELY(nargs > p->OUTOCOUNT))
      return csound->InitError(csound,
                               Str("Cannot be more In arguments than Out in "
                                   "init (%d,%d)"),p->OUTOCOUNT, nargs);
    if (nout==1) {
      *p->r[0] =  *p->a[0];
      return OK;
    }
    tmp = (MYFLT*)csound->Malloc(csound, sizeof(MYFLT)*p->OUTOCOUNT);
    for (i=0; i<nargs; i++)
      tmp[i] =  *p->a[i];
    for (; i<nout; i++)
      tmp[i] =  *p->a[nargs-1];
    for (i=0; i<nout; i++)
      *p->r[i] = tmp[i];
    csound->Free(csound, tmp);
    return OK;
}

int32_t mainit(CSOUND *csound, ASSIGNM *p)
{
    uint32_t nargs = p->INCOUNT;
    uint32_t nouts = p->OUTOCOUNT;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, n, nsmps = CS_KSMPS;
    MYFLT aa = FL(0.0);
    early = nsmps - early;      /* Bit at end to ignore */
    if (UNLIKELY(nargs > nouts))
      return csound->InitError(csound,
                               Str("Cannot be more In arguments than Out in "
                                   "init (%d,%d)"),p->OUTOCOUNT, nargs);
    for (i=0; i<nargs; i++) {
      aa = *p->a[i];
      MYFLT *r =p->r[i];
      for (n = 0; n < nsmps; n++)
        r[n] = (n < offset || n > early ? FL(0.0) : aa);
    }
    for (; i<nouts; i++) {
      MYFLT *r =p->r[i];
      memset(r, '\0', nsmps*sizeof(MYFLT));
      for (n = 0; n < nsmps; n++)
        r[n] = (n < offset || n > early ? FL(0.0) : aa);
    }

    return OK;
}


int32_t signum(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    MYFLT a = *p->a;
    int32_t ans = (a==FL(0.0) ? 0 : a<FL(0.0) ? -1 : 1);
    *p->r = (MYFLT) ans;
    return OK;
}

int32_t asignum(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t   i, nsmps = CS_KSMPS;
    MYFLT *a = p->a;
    memset(p->r, '\0', nsmps*sizeof(MYFLT));
    early = nsmps-early;
    for (i=offset; i<early; i++) {
      MYFLT aa = a[i];
      int32_t ans = (aa==FL(0.0) ? 0 : aa<FL(0.0) ? -1 : 1);
      p->r[i] = (MYFLT) ans;
    }
    return OK;
}

#define RELATN(OPNAME,OP)                                \
  int32_t OPNAME(CSOUND *csound, RELAT *p)               \
  {   IGN(csound); *p->rbool = (*p->a OP *p->b) ? 1 : 0; \
       return OK; }

RELATN(gt,>)
RELATN(ge,>=)
RELATN(lt,<)
RELATN(le,<=)
RELATN(eq,==)
RELATN(ne,!=)

int32_t b_not(CSOUND *csound, LOGCL *p)
{
    IGN(csound); *p->rbool = (*p->ibool) ? 0 : 1; return OK; }

#define LOGCLX(OPNAME,OP)                                       \
  int32_t OPNAME(CSOUND *csound, LOGCL *p)                      \
  { IGN(csound);*p->rbool = (*p->ibool OP *p->jbool) ? 1 : 0; return OK; }

LOGCLX(and,&&)
LOGCLX(or,||)

#define KK(OPNAME,OP)                                           \
  int32_t OPNAME(CSOUND *csound, AOP *p)                        \
  { IGN(csound); *p->r = *p->a OP *p->b; return OK; }

KK(addkk,+)
KK(subkk,-)
KK(mulkk,*)
//KK(divkk,/)
int32_t divkk(CSOUND *csound, AOP *p)
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
      //      if (d>=b || d<0)
      //   printf("**** a,b = %f, %f => D %f\n", a,b, d);
      return d;
  }
}

int32_t modkk(CSOUND *csound, AOP *p)
{
    IGN(csound);
    *p->r = MOD(*p->a, *p->b);
    return OK;
}

#define KA(OPNAME,OP)                                  \
  int32_t OPNAME(CSOUND *csound, AOP *p) {             \
    uint32_t n, nsmps = CS_KSMPS;                      \
    IGN(csound);                                       \
    if (LIKELY(nsmps!=1)) {                            \
      MYFLT   *r, a, *b;                               \
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

int32_t modka(CSOUND *csound, AOP *p)
{
    IGN(csound);
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
  int32_t OPNAME(CSOUND *csound, AOP *p) {      \
    uint32_t n, nsmps = CS_KSMPS;               \
    IGN(csound);                                \
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
int32_t divak(CSOUND *csound, AOP *p) {
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
        memset(&r[nsmps], '\0', early*sizeof(MYFLT));
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


int32_t modak(CSOUND *csound, AOP *p)
{
    IGN(csound);
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
  int32_t OPNAME(CSOUND *csound, AOP *p) {      \
  MYFLT   *r, *a, *b;                           \
  IGN(csound);                                  \
  uint32_t n, nsmps = CS_KSMPS;                 \
  if (LIKELY(nsmps!=1)) {                       \
    uint32_t offset = p->h.insdshead->ksmps_offset;  \
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
      *p->r = *p->a OP *p->b;                   \
      return OK;                                \
    }                                           \
  }

/* VL
   experimental code using SSE for operations
   needs memory alignment - 16 bytes
*/
#ifdef USE_SSE
#include "emmintrin.h"
#define AA_VEC(OPNAME,OP)                   \
int32_t OPNAME(CSOUND *csound, AOP *p){     \
  MYFLT   *r, *a, *b;                       \
  __m128d va, vb;                           \
  uint32_t n, nsmps = CS_KSMPS, end;        \
  if (LIKELY(nsmps!=1)) {                   \
  uint32_t offset = p->h.insdshead->ksmps_offset; \
  uint32_t early  = p->h.insdshead->ksmps_no_end; \
  r = p->r; a = p->a; b = p->b;             \
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT)); \
  if (UNLIKELY(early)) {                    \
      nsmps -= early;                       \
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));  \
  } \
  end = nsmps; \
  for (n=offset; n<end; n+=2) { \
   va = _mm_loadu_pd(&a[n]); \
   vb = _mm_loadu_pd(&b[n]); \
   va = OP(va,vb);\
   _mm_storeu_pd(&r[n],va); \
  }     \
  return OK; \
  } \
   else { \
     *p->r = *p->a + *p->b;\
      return OK; \
   }             \
} \

AA_VEC(addaa,_mm_add_pd)
AA_VEC(subaa,_mm_sub_pd)
AA_VEC(mulaa,_mm_mul_pd)
AA_VEC(divaa,_mm_div_pd)

#else
AA(addaa,+)
AA(subaa,-)
AA(mulaa,*)
//AA(divaa,/)
#endif

int32_t divaa(CSOUND *csound, AOP *p)
{
    MYFLT   *r, *a, *b;
    int     err = 0;
    IGN(csound);
    uint32_t n, nsmps = CS_KSMPS;
    if (LIKELY(nsmps!=1)) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      r = p->r;
      a = p->a;
      b = p->b;
      if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&r[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++ ) {
        MYFLT bb = b[n];
        if (UNLIKELY(bb==FL(0.0) && err==0)) {
          csound->Warning(csound, Str("Division by zero"));
          err = 1;
        }
        r[n] = a[n] / bb;
      }
      return OK;
    }
    else {
      if (UNLIKELY(*p->b==FL(0.0)))
        csound->Warning(csound, Str("Division by zero"));
      *p->r = *p->a / *p->b;
      return OK;
    }
}

  int32_t modaa(CSOUND *csound, AOP *p)
{
    MYFLT   *r, *a, *b;
    IGN(csound);
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

int32_t divzkk(CSOUND *csound, DIVZ *p)
{
    IGN(csound);
    *p->r = (*p->b != FL(0.0) ? *p->a / *p->b : *p->def);
    return OK;
}

int32_t divzka(CSOUND *csound, DIVZ *p)
{
    uint32_t n;
    IGN(csound);
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
     for (n=offset; n<nsmps; n++) {
      MYFLT bb = b[n];
      r[n] = (bb==FL(0.0) ? def : a / bb);
    }
    return OK;
}

int32_t divzak(CSOUND *csound, DIVZ *p)
{
    uint32_t n;
    IGN(csound);
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

int32_t divzaa(CSOUND *csound, DIVZ *p)
{
    uint32_t n;
    IGN(csound);
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
    for (n=offset; n<nsmps; n++) {
      MYFLT bb=b[n];
      r[n] = (bb==FL(0.0) ? def : a[n] / bb);
    }
    return OK;
}

int32_t conval(CSOUND *csound, CONVAL *p)
{
    IGN(csound);
    if (*p->cond)
      *p->r = *p->a;
    else
      *p->r = *p->b;
    return OK;
}

int32_t aconval(CSOUND *csound, CONVAL *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset*sizeof(MYFLT);
    uint32_t early  = p->h.insdshead->ksmps_no_end*sizeof(MYFLT);
    MYFLT   *r, *s;
    IGN(csound);

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

int32_t int1(CSOUND *csound, EVAL *p)               /* returns signed whole no. */
{
    MYFLT intpart;
    IGN(csound);
    MODF(*p->a, &intpart);
    *p->r = intpart;
    return OK;
}

int32_t int1a(CSOUND *csound, EVAL *p)              /* returns signed whole no. */
{
    MYFLT        intpart, *a=p->a, *r=p->r;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    IGN(csound);

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

int32_t frac1(CSOUND *csound, EVAL *p)              /* returns positive frac part */
{
    MYFLT intpart, fracpart;
    IGN(csound);
    fracpart = MODF(*p->a, &intpart);
    *p->r = fracpart;
    return OK;
}

int32_t frac1a(CSOUND *csound, EVAL *p)             /* returns positive frac part */
{
    MYFLT intpart, fracpart, *r = p->r, *a = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    IGN(csound);

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
#define MYFLOOR(x) ((int32_t)((double)(x) >= 0.0 ? (x) : (x) - 0.99999999))

#ifdef MYCEIL
#undef MYCEIL
#endif
#define MYCEIL(x) ((int32_t)((double)(x) >= 0.0 ? (x) + 0.99999999 : (x)))

int32_t int1_round(CSOUND *csound, EVAL *p)         /* round to nearest integer */
{
    IGN(csound);
    *p->r = (MYFLT) MYFLT2LRND(*p->a);
    return OK;
}

int32_t int1a_round(CSOUND *csound, EVAL *p)        /* round to nearest integer */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    MYFLT *r=p->r, *a=p->a;
    IGN(csound);

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      r[n] = (MYFLT)MYFLT2LRND(a[n]);
    return OK;
}

int32_t int1_floor(CSOUND *csound, EVAL *p)         /* round down */
{
    IGN(csound);
    *p->r = (MYFLT)(MYFLOOR(*p->a));
    return OK;
}

int32_t int1a_floor(CSOUND *csound, EVAL *p)        /* round down */
{
    MYFLT    *a=p->a, *r=p->r;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    IGN(csound);

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      r[n] = (MYFLT)(MYFLOOR(a[n]));
    return OK;
}

int32_t int1_ceil(CSOUND *csound, EVAL *p)          /* round up */
{
    IGN(csound);
    *p->r = (MYFLT)(MYCEIL(*p->a));
    return OK;
}

int32_t int1a_ceil(CSOUND *csound, EVAL *p)         /* round up */
{
    MYFLT    *a=p->a, *r=p->r;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    IGN(csound);

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

int32_t rnd1seed(CSOUND *csound, INM *p)
{
    double intpart;
    csound->rndfrac = modf(*p->ar, &intpart);
    return OK;
}

int32_t rnd1(CSOUND *csound, EVAL *p)               /* returns unipolar rand(x) */
{
    double intpart;
    csound->rndfrac = modf(csound->rndfrac * rndmlt, &intpart);
    *p->r = *p->a * (MYFLT)csound->rndfrac;
    return OK;
}

int32_t birnd1(CSOUND *csound, EVAL *p)             /* returns bipolar rand(x) */
{
    double intpart;
    csound->rndfrac = modf(csound->rndfrac * rndmlt, &intpart);
    *p->r = *p->a * (FL(2.0) * (MYFLT)csound->rndfrac - FL(1.0));
    return OK;
}

#define LIB1(OPNAME,LIBNAME)  int32_t OPNAME(CSOUND *csound, EVAL *p)       \
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

int32_t atan21(CSOUND *csound, AOP *p)
{
    IGN(csound);
    *p->r = ATAN2(*p->a, *p->b);
    return OK;
}

#define LIBA(OPNAME,LIBNAME) int32_t OPNAME(CSOUND *csound, EVAL *p) {      \
    IGN(csound); \
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

int32_t atan2aa(CSOUND *csound, AOP *p)
{
    MYFLT   *r, *a, *b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    IGN(csound);
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

int32_t dbamp(CSOUND *csound, EVAL *p)
{
    IGN(csound);
    *p->r = LOG(FABS(*p->a)) / LOG10D20;
    return OK;
}

int32_t ampdb(CSOUND *csound, EVAL *p)
{
    IGN(csound);
    *p->r = EXP(*p->a * LOG10D20);
    return OK;
}

int32_t aampdb(CSOUND *csound, EVAL *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS;
    MYFLT   *r = p->r, *a = p->a;
    IGN(csound);

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      r[n] = EXP(a[n] * LOG10D20);
    return OK;
}

int32_t dbfsamp(CSOUND *csound, EVAL *p)
{
    *p->r = LOG(FABS(*p->a) / csound->e0dbfs) / LOG10D20;
    return OK;
}

int32_t ampdbfs(CSOUND *csound, EVAL *p)
{
    *p->r =  csound->e0dbfs * EXP(*p->a * LOG10D20);
    return OK;
}

int32_t aampdbfs(CSOUND *csound, EVAL *p)
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

int32_t ftlen(CSOUND *csound, EVAL *p)
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->a)) == NULL)) {
      *p->r = -FL(1.0);       /* Return something */
      return NOTOK;
    }
    *p->r = (MYFLT)ftp->flen;

    return OK;
}

int32_t ftchnls(CSOUND *csound, EVAL *p)
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->a)) == NULL)) {
      *p->r = -FL(1.0);       /* Return something */
      return NOTOK;
    }
    *p->r = (MYFLT)ftp->nchanls;

    return OK;
}

int32_t ftcps(CSOUND *csound, EVAL *p)
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->a)) == NULL)
        || ftp->cpscvt == FL(0.0)) {
      *p->r = -FL(1.0);       /* Return something */
      return NOTOK;
    }
    *p->r = (MYFLT)(ftp->cvtbas/ftp->cpscvt);

    return OK;
}



int32_t ftlptim(CSOUND *csound, EVAL *p)
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->a)) == NULL))
      return NOTOK;
    if (LIKELY(ftp->loopmode1))
      *p->r = ftp->begin1 * csound->onedsr;
    else {
      *p->r = FL(0.0);
      csound->Warning(csound, Str("non-looping sample"));
    }
    return OK;
}

int32_t numsamp(CSOUND *csound, EVAL *p)        /***** nsamp by G.Maldonado ****/
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->a)) == NULL)) {
      *p->r = FL(0.0);
      return NOTOK;
    }
    /* if (ftp->soundend) */
    *p->r = (MYFLT)ftp->soundend;
    /* else
     *p->r = (MYFLT)(ftp->flen + 1); */

    return OK;
}

int32_t ftsr(CSOUND *csound, EVAL *p)               /**** ftsr by G.Maldonado ****/
{
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->a)) == NULL)) {
      *p->r = FL(0.0);
      return NOTOK;
    }
    *p->r = ftp->gen01args.sample_rate;

    return OK;
}

int32_t rtclock(CSOUND *csound, EVAL *p)
{
    *p->r = (MYFLT)csoundGetRealTime(csound->csRtClock);
    return OK;
}

int32_t octpch(CSOUND *csound, EVAL *p)
{
    IGN(csound);
    double fract, oct;
    fract = modf((double)*p->a, &oct);
    fract *= EIPT3;
    *p->r = (MYFLT)(oct + fract);
    return OK;
}

int32_t pchoct(CSOUND *csound, EVAL *p)
{
    IGN(csound);
    double fract, oct;
    fract = modf((double)*p->a, &oct);
    fract *= 0.12;
    *p->r = (MYFLT)(oct + fract);
    return OK;
}

int32_t cpsoct(CSOUND *csound, EVAL *p)
{
    int32_t loct = (int32_t)(*p->a * OCTRES);
    *p->r = (MYFLT)CPSOCTL(loct);
    return OK;
}

int32_t acpsoct(CSOUND *csound, EVAL *p)
{
    MYFLT   *r, *a;
    int32_t    loct;
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
      loct = (int32_t)(a[n] * OCTRES);
      r[n] = CPSOCTL(loct);
    }
    return OK;
}

int32_t octcps(CSOUND *csound, EVAL *p)
{
    IGN(csound);
    *p->r = (LOG(*p->a /(MYFLT)ONEPT) / (MYFLT)LOGTWO);
    return OK;
}

int32_t cpspch(CSOUND *csound, EVAL *p)
{
    double fract, oct;
    int32_t    loct;
    fract = modf((double)*p->a, &oct);
    fract *= EIPT3;
    loct = (int32_t)MYFLT2LRND((oct + fract) * OCTRES);
    *p->r = (MYFLT)CPSOCTL(loct);
    return OK;
}

int32_t cpsmidinn(CSOUND *csound, EVAL *p)
{
    MYFLT note = *p->a;         /* (note-69)>12* */
    if (note > 12*32+69 || note < 0)
      return csound->InitError(csound, Str("MIDI note %f out of range"), note);
    *p->r = POWER(FL(2.0),
                  (note - FL(69.0)) / FL(12.0)) * (MYFLT)(csound->A4);
    return OK;
}

int32_t octmidinn(CSOUND *csound, EVAL *p)
{
    IGN(csound);
    /* Convert Midi Note number to 8ve.decimal format */
    *p->r = (*p->a / FL(12.0)) + FL(MIDINOTE0);
    return OK;
}

int32_t pchmidinn(CSOUND *csound, EVAL *p)
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

int32_t cpsxpch(CSOUND *csound, XENH *p)
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
      FUNC* ftp = csound->FTnp2Finde(csound, &t);
      int32_t len, frt;
      if (UNLIKELY(ftp == NULL))
        return csound->PerfError(csound, &(p->h),Str("No tuning table %d"),
                                 -((int32_t)*p->et));
      len = ftp->flen;
      frt = (int32_t)(100.0*fract+0.5);
      while (frt>len) {
        frt -= len; loct++;
      }
      *p->r = *p->ref * *(ftp->ftable + frt) *
        POWER(*p->cy, (MYFLT)loct);
    }
    return OK;
}

int32_t cps2pch(CSOUND *csound, XENH *p)
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
      FUNC* ftp = csound->FTnp2Finde(csound, &t);
      int32_t len, frt;
      if (UNLIKELY(ftp == NULL))
        return csound->PerfError(csound, &(p->h),Str("No tuning table %d"),
                                 -((int32_t)*p->et));
      len = ftp->flen;
      frt = (int32_t)(100.0*fract+0.5);
      //printf("len=%d fract=%g frt=%d\n", len, fract, frt);
      while (frt>len) {
        frt -= len; loct++;
      }
      //printf("len=%d loct=%g frt=%d\n", len, loct, frt);
      *p->r = (MYFLT)(1.02197503906 * *(ftp->ftable + frt) *
                      pow(2.0, loct));
    }

    /*  double ref = 261.62561 / pow(2.0, 8.0); */
    return OK;
}

int32_t cpstun_i(CSOUND *csound, CPSTUNI *p)
{
    FUNC  *ftp;
    MYFLT *func;
    int32_t notenum = (int32_t)*p->input;
    int32_t grade;
    int32_t numgrades;
    int32_t basekeymidi;
    MYFLT basefreq, factor, interval;
    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->tablenum)) == NULL)) goto err1;
    func = ftp->ftable;
    numgrades = (int32_t)*func++;
    interval = *func++;
    basefreq = *func++;
    basekeymidi = (int32_t)*func++;

    if (notenum < basekeymidi) {
      notenum = basekeymidi - notenum;
      grade  = (numgrades-(notenum % numgrades)) % numgrades;
      factor = - (MYFLT)(int32_t)((notenum+numgrades-1) / numgrades) ;
    }
    else {
      notenum = notenum - basekeymidi;
      grade  = notenum % numgrades;
      factor = (MYFLT)(int32_t)(notenum / numgrades);
    }
    factor = POWER(interval, factor);
    *p->r = func[grade] * factor * basefreq;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),Str("cpstun: invalid table"));
}

int32_t cpstun(CSOUND *csound, CPSTUN *p)
{
    if (*p->ktrig) {
      FUNC  *ftp;
      MYFLT *func;
      int32_t notenum = (int32_t)*p->kinput;
      int32_t grade;
      int32_t numgrades;
      int32_t basekeymidi;
      MYFLT basefreq, factor, interval;
      if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->tablenum)) == NULL))
        goto err1;
      func = ftp->ftable;
      numgrades = (int32_t)*func++;
      interval = *func++;
      basefreq = *func++;
      basekeymidi = (int32_t)*func++;

      if (notenum < basekeymidi) {
        notenum = basekeymidi - notenum;
        grade  = (numgrades-(notenum % numgrades)) % numgrades;
        factor = - (MYFLT)(int32_t)((notenum+numgrades-1) / numgrades) ;
      }
      else {
        notenum = notenum - basekeymidi;
        grade  = notenum % numgrades;
        factor = (MYFLT)(int32_t)(notenum / numgrades);
      }
      factor = POWER(interval, factor);
      p->old_r = (*p->r = func[grade] * factor * basefreq);

}
    else *p->r = p->old_r;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),Str("cpstun: invalid table"));
}

int32_t logbasetwo_set(CSOUND *csound, EVAL *p)
{
    IGN(p);
    if (UNLIKELY(csound->logbase2 == NULL)) {
      double  x = (1.0 / INTERVAL);
      int32_t     i;
      csound->logbase2 = (MYFLT*) csound->Malloc(csound, (STEPS + 1)
                                                 * sizeof(MYFLT));
      for (i = 0; i <= STEPS; i++) {
        csound->logbase2[i] = ONEdLOG2 * LOG((MYFLT)x);
        x += ((INTERVAL - 1.0 / INTERVAL) / (double)STEPS);
      }
    }
    return OK;
}

int32_t powoftwo(CSOUND *csound, EVAL *p)
{
    *p->r = POWER(FL(2.0), *p->a);
    return OK;
}

int32_t powoftwoa(CSOUND *csound, EVAL *p)
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
      r[n] = POWER(FL(2.0), a[n]);
    return OK;
}

#define ONEd12          (FL(0.08333333333333333333333))
#define ONEd1200        (FL(0.00083333333333333333333))

int32_t semitone(CSOUND *csound, EVAL *p)
{
    MYFLT a = *p->a*ONEd12;
    *p->r = POWER(FL(2.0), a);
    return OK;
}

int32_t asemitone(CSOUND *csound, EVAL *p)            /* JPff */
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
      r[n] = POWER(FL(2.0), aa);
    }
    return OK;
}

int32_t cent(CSOUND *csound, EVAL *p)
{
    MYFLT a = *p->a;
    *p->r = POWER(FL(2.0), a/FL(1200.0));
    return OK;
}

int32_t acent(CSOUND *csound, EVAL *p)        /* JPff */
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
      r[n] = POWER(FL(2.0), aa);
  }
  return OK;
}

#define LOG2_10D20      (FL(0.166096404744368117393515971474))

int32_t db(CSOUND *csound, EVAL *p)
{
    *p->r = POWER(FL(2.0), *p->a*LOG2_10D20);
    return OK;
}

int32_t dba(CSOUND *csound, EVAL *p)          /* JPff */
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
      r[n] = POWER(FL(2.0), aa*LOG2_10D20);
    }
    return OK;
}

int32_t logbasetwo(CSOUND *csound, EVAL *p)
{
    int32_t n = (int32_t)((*p->a -(FL(1.0)/INTERVAL)) / (INTERVAL-FL(1.0)/INTERVAL)
                  *  STEPS + FL(0.5));
    if (n<0 || n>STEPS)
      *p->r = LOG(*p->a)*ONEdLOG2;
    else
      *p->r = csound->logbase2[n];
    return OK;
}

int32_t logbasetwoa(CSOUND *csound, EVAL *p)
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
      int32_t n = (int32_t)((aa -(FL(1.0)/INTERVAL)) / (INTERVAL-FL(1.0)/INTERVAL)
                    *  STEPS + FL(0.5));
      if (n<0 || n>STEPS) r[n] = LOG(aa)*ONEdLOG2;
      else                r[n] = csound->logbase2[n];
    }
    return OK;
}

int32_t ilogbasetwo(CSOUND *csound, EVAL *p)
{
    logbasetwo_set(csound, p);
    logbasetwo(csound, p);
    return OK;
}

int32_t in(CSOUND *csound, INM *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset*sizeof(MYFLT);
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if (csound->inchnls != 1)
      return csound->PerfError(csound,
                               &(p->h),
                               "Wrong numnber of input channels\n");
    CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset)) memset(p->ar, '\0', offset);
    memcpy(&p->ar[offset], CS_SPIN, (CS_KSMPS-early) * sizeof(MYFLT)-offset);
    if (UNLIKELY(early))
      memset(&p->ar[CS_KSMPS-early], '\0', early * sizeof(MYFLT));
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int32_t inarray(CSOUND *csound, INA *p)
{
    MYFLT *data = p->tabout->data;
    uint32_t n = p->tabout->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset*sizeof(MYFLT);
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT *sp = CS_SPIN;
    uint32_t m, nsmps =CS_KSMPS, i;
    uint32_t ksmps = nsmps;

    if ((int32_t)n>csound->inchnls) n = csound->inchnls;
    CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset))
      for (i = 0; i < n; i++)
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

int32_t ins(CSOUND *csound, INS *p)
{
    MYFLT       *sp, *ar1, *ar2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS, k;
    if (UNLIKELY(csound->inchnls != 2))
      return csound->PerfError(csound, &(p->h),
                               "Wrong numnber of input channels\n");
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
      memset(&p->ar1[nsmps], '\0', early * sizeof(MYFLT));
      memset(&p->ar2[nsmps], '\0', early * sizeof(MYFLT));
    }
    for (n=offset, k=0; n<nsmps; n++, k+=2) {
      ar1[n] = sp[k];
      ar2[n] = sp[k+1];
    }
    CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int32_t inq(CSOUND *csound, INQ *p)
{
    MYFLT       *sp = CS_SPIN, *ar1 = p->ar1, *ar2 = p->ar2,
                               *ar3 = p->ar3, *ar4 = p->ar4;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS, k;
    if (UNLIKELY(csound->inchnls != 4))
      return csound->PerfError(csound,
                               &(p->h),
                               "Wrong numnber of input channels\n");
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

int32_t inh(CSOUND *csound, INH *p)
{
    MYFLT       *sp = CS_SPIN, *ar1 = p->ar1, *ar2 = p->ar2, *ar3 = p->ar3,
                               *ar4 = p->ar4, *ar5 = p->ar5, *ar6 = p->ar6;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS, k;
    if (UNLIKELY(csound->inchnls != 6))
      return csound->PerfError(csound,
                               &(p->h),
                               "Wrong numnber of input channels\n");
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

int32_t ino(CSOUND *csound, INO *p)
{
    MYFLT       *sp = CS_SPIN, *ar1 = p->ar1, *ar2 = p->ar2, *ar3 = p->ar3,
                               *ar4 = p->ar4, *ar5 = p->ar5, *ar6 = p->ar6,
                               *ar7 = p->ar7, *ar8 = p->ar8;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps =CS_KSMPS, k;
    if (UNLIKELY(csound->inchnls != 8))
      return csound->PerfError(csound,
                               &(p->h),
                               "Wrong numnber of input channels\n");
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

static int32_t inn(CSOUND *csound, INALL *p, uint32_t n)
{
    MYFLT *sp = CS_SPIN, **ara = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t m, nsmps =CS_KSMPS, i;
    if (UNLIKELY(csound->inchnls != (int32_t) n))
      return csound->PerfError(csound,
                               &(p->h),
                               "Wrong numnber of input channels\n");

    CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset))
      for (i = 0; i < n; i++)
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

int32_t in16(CSOUND *csound, INALL *p)
{
    return inn(csound, p, 16u);
}

int32_t in32(CSOUND *csound, INALL *p)
{
    return inn(csound, p, 32u);
}

int32_t inch1_set(CSOUND *csound, INCH1 *p)
{
    IGN(csound);
    p->init = 1;
    return OK;
}

int32_t inch_opcode1(CSOUND *csound, INCH1 *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS, ch;
    MYFLT *sp, *ain;

    ch = MYFLT2LRND(*p->ch);
    if (UNLIKELY(ch > (uint32_t)csound->inchnls)) {
      if (p->init)
        csound->Message(csound, Str("Input channel %d too large; ignored\n"), ch);
      memset(p->ar, 0, sizeof(MYFLT)*nsmps);
      p->init = 0;
      //        return OK;
    } else if (UNLIKELY(ch < 1)) {
      if (p->init)
        csound->Message(csound, Str("Input channel %d is invalid; ignored"), ch);
      memset(p->ar, 0, sizeof(MYFLT)*nsmps);
      p->init = 0;
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

int32_t inch_set(CSOUND *csound, INCH *p)
{
    IGN(csound);
    p->init = 1;
    return OK;
}

int32_t inch_opcode(CSOUND *csound, INCH *p)
{                               /* Rewritten to allow multiple args upto 40 */
    uint32_t nc, nChannels = p->INCOUNT;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS, ch;
    MYFLT *sp, *ain;
    if (UNLIKELY(nChannels != p->OUTOCOUNT))
      return
        csound->PerfError(csound, &(p->h),
                          Str("Input and output argument count differs in inch"));
    for (nc=0; nc<nChannels; nc++) {
      ch = MYFLT2LRND(*p->ch[nc]);
      if (UNLIKELY(ch > (uint32_t)csound->inchnls)) {
        if (p->init)
          csound->Warning(csound, Str("Input channel %d too large; ignored"), ch);
        memset(p->ar[nc], 0, sizeof(MYFLT)*nsmps);
        p->init = 0;
        //        return OK;
      } else if (UNLIKELY(ch < 1)) {
        if (UNLIKELY(p->init))
          csound->Warning(csound, Str("Input channel %d is invalid; ignored"), ch);
        memset(p->ar[nc], 0, sizeof(MYFLT)*nsmps);
        p->init = 0;
      } else {
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


int32_t inall_opcode(CSOUND *csound, INALL *p)
{
    uint32_t n = (int32_t)p->OUTOCOUNT, m;
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

int32_t outs1(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp=  CS_SPOUT /*csound->spraw*/, *ap1= p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;

    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      if (offset) memset(sp, '\0', offset*sizeof(MYFLT));
      memcpy(&sp[offset], &ap1[offset], (early-offset)*sizeof(MYFLT));
      if (early!=nsmps) memset(&sp[early], '\0', (nsmps-early)*sizeof(MYFLT));
      /* for (n=0; n<nsmps; n++) { */
      /*   sp[n] = (n<offset || n>early) ? FL(0.0) : ap1[n]; */
      /* } */
      if (csound->nchnls>1)
        memset(&sp[nsmps], '\0', nsmps*(csound->nchnls-1)*sizeof(MYFLT));
      csound->spoutactive = 1;
    }
    else {
      for (n=0; n<early; n++) {
        if (n>=offset) sp[n]   += ap1[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

#define OUTCN(n)  if (n>csound->nchnls) return          \
                                          csound->InitError(csound, "%s", \
                                              Str("Channel greater than nchnls")); \
  return OK;

int32_t och2(CSOUND *csound, OUTM *p) { IGN(p); OUTCN(2) }
int32_t och3(CSOUND *csound, OUTM *p) { IGN(p); OUTCN(3) }
int32_t och4(CSOUND *csound, OUTM *p) { IGN(p); OUTCN(4) }

int32_t outs2(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp =  CS_SPOUT /*csound->spraw*/, *ap2 = p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;

    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      memset(sp, '\0', nsmps*sizeof(MYFLT));
      sp +=nsmps;
      if (offset) memset(sp, '\0', offset*sizeof(MYFLT));
      memcpy(&sp[offset], &ap2[offset], (early-offset)*sizeof(MYFLT));
      if (early!=nsmps) memset(&sp[early], '\0', (nsmps-early)*sizeof(MYFLT));
      if (csound->nchnls>2)
        memset(&sp[nsmps], '\0', (csound->nchnls-2)*sizeof(MYFLT));
      csound->spoutactive = 1;
    }
    else {
      sp +=nsmps;
      for (n=offset; n<early; n++) {
        sp[n] += ap2[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t outq3(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = CS_SPOUT /*csound->spraw*/, *ap3 = p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
       memset(sp, '\0', 2*nsmps*sizeof(MYFLT));
      sp += 2*nsmps;
      if (offset) memset(sp, '\0', offset*sizeof(MYFLT));
      memcpy(&sp[offset], &ap3[offset], (early-offset)*sizeof(MYFLT));
      if (early!=nsmps) memset(&sp[early], '\0', (nsmps-early)*sizeof(MYFLT));
      if (csound->nchnls>3)
        memset(&sp[nsmps], '\0', (csound->nchnls-3)*sizeof(MYFLT));
      csound->spoutactive = 1;
    }
    else {
      sp += 2*nsmps;
      for (n=offset; n<early; n++) {
        sp[n]   += ap3[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t outq4(CSOUND *csound, OUTM *p)
{
    MYFLT       *sp = CS_SPOUT /*csound->spraw*/, *ap4 = p->asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t nsmps =CS_KSMPS,  n;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;
    CSOUND_SPOUT_SPINLOCK
    if (!csound->spoutactive) {
      memset(sp, '\0', 3*nsmps*sizeof(MYFLT));
      sp += 3*nsmps;
      if (offset) memset(sp, '\0', offset*sizeof(MYFLT));
      memcpy(&sp[offset], &ap4[offset], (early-offset)*sizeof(MYFLT));
      if (early!=nsmps) memset(&sp[early], '\0', (nsmps-early)*sizeof(MYFLT));
      if (csound->nchnls>4)
        memset(&sp[nsmps], '\0', (csound->nchnls-4)*sizeof(MYFLT));
      csound->spoutactive = 1;
    }
    else {
      sp += 3*nsmps;
      for (n=offset; n<early; n++) {
        sp[n]   += ap4[n];
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

inline static int32_t outn(CSOUND *csound, uint32_t n, OUTX *p)
{
    uint32_t nsmps = CS_KSMPS,  i, j, k=0;
    MYFLT *spout = CS_SPOUT; ///csound->spraw;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    //        if (UNLIKELY((offset|early))) {
    //          printf("OUT; spout=%p early=%d offset=%d\n", spout, early, offset);}
    early = nsmps - early;
    CSOUND_SPOUT_SPINLOCK

    if (!csound->spoutactive) {
      //printf("inactive: spout=%p size=%ld\n",
      //       spout, csound->nspout*sizeof(MYFLT));
      memset(spout, '\0', csound->nspout*sizeof(MYFLT));
      for (i=0; i<n; i++) {
        //printf("        : spout=%p size=%ld\n",
        //       spout+k+offset, (early-offset)*sizeof(MYFLT));
        memcpy(&spout[k+offset], p->asig[i]+offset, (early-offset)*sizeof(MYFLT));
        k += nsmps;
      }
      csound->spoutactive = 1;
    }
    else {
      for (i=0; i<n; i++) {
        for (j=offset; j<early; j++) {
          //printf("active: spout=%p k=%d j=%d\n", spout, k, j);
          spout[k + j] += p->asig[i][j];
        }
        k += nsmps;
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
        //    }
    /* else { */
    /*   CSOUND_SPOUT_SPINLOCK */

    /*   if (!csound->spoutactive) { */
    /*     for (i=0; i<n; i++) { */
    /*       memcpy(&spout[k], p->asig[i], nsmps*sizeof(MYFLT)); */
    /*       k += nsmps; */
    /*     } */
    /*     if (csound->nchnls>n+1) { */
    /*       printf("nchnks, n = %d,%d\n", csound->nchnls, n); */
    /*       memset(&spout[k], '\0', (nsmps*(csound->nchnls-n))*sizeof(MYFLT)); */
    /*     } */
    /*     csound->spoutactive = 1; */
    /*   } */
    /*   else { */
    /*     for (i=0; i<n; i++) { */
    /*       for (j=0; j<nsmps; j++) { */
    /*         spout[k + j] += p->asig[i][j]; */
    /*       } */
    /*       k += nsmps; */
    /*     } */
    /*   } */
    /*   CSOUND_SPOUT_SPINUNLOCK */
    /* } */
    return OK;
}

int32_t ochn(CSOUND *csound, OUTX *p)
{
    uint32_t nch = p->INOCOUNT;
    if (nch>csound->nchnls)
      csound->Warning(csound, Str("Excess channels ignored\n"));
    return OK;
}

int32_t outall(CSOUND *csound, OUTX *p)             /* Output a list of channels */
{
    uint32_t nch = p->INOCOUNT;
    return outn(csound, (nch <= csound->nchnls ? nch : csound->nchnls), p);
}

int32_t outarr_init(CSOUND *csound, OUTARRAY *p)
{
    IGN(csound);
    p->nowarn = 0;
    return OK;
}

int32_t outarr(CSOUND *csound, OUTARRAY *p)
{
    uint32_t nsmps =CS_KSMPS,  i, j;
    uint32_t ksmps = nsmps;
    uint32_t n = p->tabin->sizes[0];
    MYFLT *data = p->tabin->data;
    MYFLT *spout = CS_SPOUT; //csound->spraw;
    if (n>csound->nchnls) {
      if (p->nowarn==0) {
        csound->Warning(csound,
                        Str("out: number of channels truncated from %d to %d"),
                        n, csound->nchnls);
      }
      n = csound->nchnls;
      p->nowarn = 1;
    }
    if (csound->oparms->sampleAccurate) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;

      CSOUND_SPOUT_SPINLOCK
      if (!csound->spoutactive) {
        memset(spout, '\0', csound->nspout*sizeof(MYFLT));
        for (i=0; i<n; i++) {
          for (j=offset; j<early; j++) {
            spout[j+i*ksmps] = data[j+i*ksmps];
          }
        }
        csound->spoutactive = 1;
      }
      else {
        /* no need to offset data is already offset in the buffer*/
        for (i=0; i<n; i++) {
          for (j=offset; j<early; j++) {
            spout[j+i*ksmps] += data[j+i*ksmps];
          }
        }
      }
      CSOUND_SPOUT_SPINUNLOCK
    }
    else {
      CSOUND_SPOUT_SPINLOCK
      if (!csound->spoutactive) {
        memcpy(spout, data, n*ksmps*sizeof(MYFLT));
        if (csound->nchnls>n)
          memset(&spout[n*ksmps], '\0', (csound->nchnls-n)*ksmps*sizeof(MYFLT));
        csound->spoutactive = 1;
      }
      else {
        for (i=0; i<n*nsmps; i++) {
          spout[i] += data[i];
        }
      }
      CSOUND_SPOUT_SPINUNLOCK
    }
    return OK;
}

int32_t outch(CSOUND *csound, OUTCH *p)
{
    uint32_t    ch;
    MYFLT       *sp, *apn;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    nsmps = CS_KSMPS, j, n;
    uint32_t    early = nsmps-p->h.insdshead->ksmps_no_end;
    uint32_t    count = p->INOCOUNT;
    MYFLT       **args = p->args;
    uint32_t    nchnls = csound->nchnls;
    MYFLT *spout = CS_SPOUT;
    if (UNLIKELY((count&1)!=0))
      return
        csound->PerfError(csound, &(p->h),
                          Str("outch must have an even number of arguments"));
    CSOUND_SPOUT_SPINLOCK
    for (j = 0; j < count; j += 2) {
      ch = MYFLT2LRND(*args[j]);
      if (ch < 1) ch = 1;
      apn = args[j + 1];
      if (ch > nchnls) continue;
      if (!csound->spoutactive) {
        ch--;
        memset(spout, '\0', csound->nspout*sizeof(MYFLT));
        memcpy(&spout[offset+ch*nsmps], apn, (early-offset)*sizeof(MYFLT));
        csound->spoutactive = 1;
      }
      else {
        sp = spout + (ch - 1)*nsmps;
        for (n=offset; n<early; n++) {
          sp[n] += apn[n];
        }
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t outrep(CSOUND *csound, OUTM *p)
{
    uint32_t nsmps = CS_KSMPS,  i, j, k=0;
    uint32_t n = csound->nchnls;
    MYFLT *spout = CS_SPOUT; ///csound->spraw;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    //if (UNLIKELY((offset|early))) {
    //  printf("OUT; spout=%p early=%d offset=%d\n", spout, early, offset);}
    early = nsmps - early;
    CSOUND_SPOUT_SPINLOCK

    if (!csound->spoutactive) {
      //printf("inactive: spout=%p size=%ld\n",
      //       spout, csound->nspout*sizeof(MYFLT));
      memset(spout, '\0', csound->nspout*sizeof(MYFLT));
      for (i=0; i<n; i++) {
        //printf("        : spout=%p size=%ld\n",
        //       spout+k+offset, (early-offset)*sizeof(MYFLT));
        memcpy(&spout[k+offset], p->asig+offset, (early-offset)*sizeof(MYFLT));
        k += nsmps;
      }
      csound->spoutactive = 1;
    }
    else {
      for (i=0; i<n; i++) {
        for (j=offset; j<early; j++) {
          //printf("active: spout=%p k=%d j=%d\n", spout, k, j);
          spout[k + j] += p->asig[j];
        }
        k += nsmps;
      }
    }
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

/* For parallel mixin template */
int32_t addina(CSOUND *csound, ASSIGN *p)
{
    MYFLT* val = p->a;
    MYFLT* ans = p->r;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    nsmps = CS_KSMPS, n;
    uint32_t    early = nsmps-p->h.insdshead->ksmps_no_end;

    CSOUND_SPOUT_SPINLOCK
    for (n=offset; n<early; n++)
      ans[n] += val[n];
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t addinak(CSOUND *csound, ASSIGN *p)
{
    MYFLT val;
    MYFLT* ans = p->r;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    nsmps = CS_KSMPS, n;
    uint32_t    early = nsmps-p->h.insdshead->ksmps_no_end;

    CSOUND_SPOUT_SPINLOCK
    val = *p->a;
    for (n=offset; n<early; n++)
      ans[n] += val;
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t addin(CSOUND *csound, ASSIGN *p)
{
    CSOUND_SPOUT_SPINLOCK
    *p->r += *p->a;
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t subin(CSOUND *csound, ASSIGN *p)
{
    CSOUND_SPOUT_SPINLOCK
    *p->r -= *p->a;
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t subina(CSOUND *csound, ASSIGN *p)
{
    MYFLT* val = p->a;
    MYFLT* ans = p->r;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    nsmps = CS_KSMPS, n;
    uint32_t    early = nsmps-p->h.insdshead->ksmps_no_end;

    CSOUND_SPOUT_SPINLOCK
    for (n=offset; n<early; n++)
      ans[n] -= val[n];
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t subinak(CSOUND *csound, ASSIGN *p)
{
    MYFLT val;
    MYFLT* ans = p->r;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    nsmps = CS_KSMPS, n;
    uint32_t    early = nsmps-p->h.insdshead->ksmps_no_end;

    CSOUND_SPOUT_SPINLOCK
    val = *p->a;
    for (n=offset; n<early; n++)
      ans[n] -= val;
    CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

/**
 * Identifies both signaling NaN (sNaN) and quiet NaN (qNaN).
 * 
 * According to the IEEE 754 standard, all NaN have the sign bit set to 0 and 
 * all exponent bits set to 1. qNaN has the most significant bit of the
 * fractional set to 1, while sNaN has most the significant bit of the 
 * fraction set to 0 -- but the NEXT most significant bit of the fraction must 
 * be set to 1! This is necessary in order to distinguish sNaN from positive 
 * infinity. Hence, there are 2 bit masks to test. Doubles have the most 
 * significant bit of the fraction in (0-based) bit 52, floats have the most 
 * significant bit of the fraction in bit 22.
 * double qNaN:
 * 0111111111110000000000000000000000000000000000000000000000000000
 * 0x7FF0000000000000ULL
 * double sNaN:
 * 0111111111101000000000000000000000000000000000000000000000000000
 * 0x7FE8000000000000ULL
 * float qNaN:
 * 01111111110000000000000000000000  
 * 0x7FC00000
 * float sNaN:
 * 01111111101000000000000000000000  
 * 0x7FA00000
 * NOTE: Not all compilers permit type casting a type-punned pointer. So, we 
 * must explicitly copy rather than assign the data to test.
 */
static inline int _isnan(MYFLT x) {
    #ifdef USE_DOUBLE
        uint64_t bits;
        memcpy(&bits, &x, sizeof(MYFLT));
        if ((bits & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) {
            return 1;
        } 
        if ((bits & 0x7FE8000000000000ULL) == 0x7FE8000000000000ULL) {
            return 1;
        } 
        return 0;
    #else
        uint32_t bits;
        memcpy(&bits, &x, sizeof(MYFLT));
        if ((bits & 0x7FC00000) == 0x7FC00000) {
            return 1;
        } 
        if ((bits & 0x7FA00000) == 0x7FA00000) {
            return 1;
        } 
        return 0;
    #endif
 }


int32_t is_NaN(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    // *p->r = isnan(*p->a);
    *p->r = _isnan(*p->a);
	return OK;
}

/* ********COULD BE IMPROVED******** */
int32_t is_NaNa(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t k, nsmps = CS_KSMPS;
    uint32_t early  = nsmps - p->h.insdshead->ksmps_no_end;
    MYFLT *a = p->a;
    *p->r = FL(0.0);
    for (k=offset; k<early; k++)
      // *p->r += isnan(a[k]);
      *p->r += _isnan(a[k]);
    return OK;
}

int32_t is_inf(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    *p->r = isinf(*p->a);
    return OK;
}

/* ********COULD BE IMPROVED******** */
int32_t is_infa(CSOUND *csound, ASSIGN *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t k, nsmps = CS_KSMPS;
    uint32_t early  = nsmps-p->h.insdshead->ksmps_no_end;
    MYFLT *a = p->a;
    MYFLT ans = FL(0.0);
    int32_t sign = 1;
    for (k=offset; k<early; k++) {
      if (isinf(a[k]))
        if (ans==FL(0.0)) sign = (int32_t)isinf(a[k]);
      ans++;
    }
    *p->r = ans*sign;
    return OK;
}

int32_t error_fn(CSOUND *csound, ERRFN *p)
{
    IGN(p);
    return csound->InitError(csound, Str("Unknown function called"));
}

 /* ------------------------------------------------------------------------ */

int32_t monitor_opcode_perf(CSOUND *csound, MONITOR_OPCODE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, j, nsmps = CS_KSMPS, nchnls = csound->GetNchnls(csound);
    MYFLT *spout = csound->spraw;

    if (csound->spoutactive) {
      for (j = 0; j<nchnls; j++) {
        for (i = 0; i<nsmps; i++) {
          if (i<offset||i>nsmps-early)
            p->ar[j][i] = FL(0.0);
          else
            p->ar[j][i] = spout[i+j*nsmps];
        }
      }
    }
    else {
      for (j = 0; j<nchnls; j++) {
        memset(p->ar[j], '\0', nsmps*sizeof(MYFLT));
      }
    }
    return OK;
}

int32_t monitor_opcode_init(CSOUND *csound, MONITOR_OPCODE *p)
{
    if (UNLIKELY(csound->GetOutputArgCnt(p) != (int32_t)csound->GetNchnls(csound)))
      return csound->InitError(csound, Str("number of arguments != nchnls"));
    p->h.opadr = (SUBR) monitor_opcode_perf;
    return OK;
}

/* -------------------------------------------------------------------- */

int32_t outRange_i(CSOUND *csound, OUTRANGE *p)
{
    IGN(csound);
    p->narg = p->INOCOUNT-1;

    return OK;
}

int32_t outRange(CSOUND *csound, OUTRANGE *p)
{
    int32_t j;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    //int32_t nchnls = csound->GetNchnls(csound);
    MYFLT *ara[VARGMAX];
    int32_t startChan = (int32_t) *p->kstartChan -1;
    MYFLT *sp = csound->spraw + startChan*nsmps;
    int32_t narg = p->narg;

    if (UNLIKELY(startChan < 0))
      return csound->PerfError(csound, &(p->h),
                               Str("outrg: channel number cannot be < 1 "
                                   "(1 is the first channel)"));

    for (j = 0; j < narg; j++)
      ara[j] = p->argums[j];

    if (!csound->spoutactive) {
      memset(csound->spraw, '\0', csound->nspout * sizeof(MYFLT));
      /* no need to offset ?? why ?? */
      int32_t i;
      for (i=0; i < narg; i++) {
        memcpy(sp, ara[i], nsmps*sizeof(MYFLT));
        sp += nsmps;
      }
      csound->spoutactive = 1;
    }
    else {
      int32_t i;
      for (i=0; i < narg; i++) {
        for (n=offset; n<nsmps-early; n++) {
          sp[n] += ara[i][n];
        }
        sp += nsmps;
      }
    }
    return OK;
}
/* -------------------------------------------------------------------- */

int32_t hw_channels(CSOUND *csound, ASSIGN *p){

    int32_t *dachans =
      (int32_t *) csound->QueryGlobalVariable(csound, "_DAC_CHANNELS_");
    if (UNLIKELY(dachans == NULL)) {
      csound->Warning(csound, Str("number of hardware output channels"
                                  " not currently available"));
    }
    else *p->r = *dachans;
    dachans = (int32_t *) csound->QueryGlobalVariable(csound, "_ADC_CHANNELS_");
    if (UNLIKELY(dachans == NULL)) {
      csound->Warning(csound, Str("number of hardware input channels"
                                  " not currently available"));
    }
    else *p->a = *dachans;
    return OK;
}
