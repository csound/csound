/* aops.c:

   Copyright (C) 1991 Barry Vercoe, John ffitch, Gabriel Maldonado
   (c) 2024 V Lazzarini

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

#include "csoundCore.h" /*                                      AOPS.C  */
#include "aops.h"
#include "arrays.h"
#include "arrays_internal.h"
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
    csound->cpsocfrc = (cs_float *) csound->Malloc(csound, sizeof(cs_float)*OCTRES);
  /* if (csound->powerof2==NULL) */
  /*   csound->powerof2 = (cs_float *) csound->Malloc(csound, */
  /*                                               sizeof(cs_float)*POW2TABSIZI); */
  for (i = 0; i < OCTRES; i++)
    csound->cpsocfrc[i] = POWER(FL(2.0), (cs_float)i / OCTRES) * ONEPT;
  /* for (i = 0; i < POW2TABSIZI; i++) { */
  /*   csound->powerof2[i] = */
  /*     POWER(FL(2.0), (cs_float)i * (cs_float)(1.0/POW2TABSIZI) - FL(POW2MAX)); */
  /* } */
}


cs_float csoundPow2(CSOUND *csound, cs_float a)
{
  /* int32_t n; */
  if (a > POW2MAX) a = POW2MAX;
  else if (a < -POW2MAX) a = -POW2MAX;
  return POWER(FL(2.0), a);
}

int32_t storei(CSOUND *csound, STOREI *p) {
  p->mem = *p->a;
  return OK;
}

int32_t retrievek(CSOUND *csound, STOREI *p) {
  *p->r = p->mem;
  return OK;
}


int32_t b2s(CSOUND *csound, ASSIGN *p){
  // B may use cs_float storage when it represents a k-rate boolean.
  *p->r = *p->a;
  return OK;
}

int32_t b2i(CSOUND *csound, ASSIGN *p){
  int32_t value;
  memcpy(&value, p->a, sizeof(value));
  *p->r = (cs_float) value;
  return OK;
}

int32_t b2b(CSOUND *csound, ASSIGN *p){
  memcpy(p->r, p->a, sizeof(int32_t));
  return OK;
}

int32_t binit(CSOUND *csound, ASSIGNM *p)
{
  uint32_t nargs = p->INOCOUNT;
  uint32_t nout = p->OUTOCOUNT;
  int32_t **r = (int32_t **) p->r;
  uint32_t i;
  int32_t *tmp;
  if (UNLIKELY(nargs > p->OUTOCOUNT))
    return csound->InitError(csound,
                             Str("Cannot be more In arguments than Out in "
                                 "init (%d,%d)"),p->OUTOCOUNT, nargs);
  if (nout==1) {
    *r[0] =  *p->a[0] != 0 ? 1 : 0;
    return OK;
  }
  tmp = (int32_t *)csound->Malloc(csound, sizeof(int32_t)*p->OUTOCOUNT);
  for (i=0; i<nargs; i++)
    tmp[i] = *p->a[i] != 0 ? 1 : 0;
  for (; i<nout; i++)
    tmp[i] = *p->a[nargs-1] != 0 ? 1 : 0;;
  for (i=0; i<nout; i++)
    *r[i] = tmp[i];
  csound->Free(csound, tmp);
  return OK;
}



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
    if (UNLIKELY(islocal &&offset)) memset(p->r, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      if (islocal) memset(&p->r[nsmps], '\0', early*sizeof(cs_float));
    }
    memcpy(&p->r[offset], &p->a[offset], (nsmps-offset) * sizeof(cs_float));
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
  cs_float aa = *p->a;
  int32_t   n, nsmps = CS_KSMPS;
  if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&p->r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++)
    p->r[n] = aa;
  return OK;
}

int32_t minit(CSOUND *csound, ASSIGNM *p)
{
  uint32_t nargs = p->INOCOUNT;
  uint32_t nout = p->OUTOCOUNT;
  uint32_t i;
  cs_float *tmp;
  if (UNLIKELY(nargs > p->OUTOCOUNT))
    return csound->InitError(csound,
                             Str("Cannot be more In arguments than Out in "
                                 "init (%d,%d)"),p->OUTOCOUNT, nargs);
  if (nout==1) {
    *p->r[0] =  *p->a[0];
    return OK;
  }
  tmp = (cs_float*)csound->Malloc(csound, sizeof(cs_float)*p->OUTOCOUNT);
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
  uint32_t nargs = p->INOCOUNT;
  uint32_t nouts = p->OUTOCOUNT;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, n, nsmps = CS_KSMPS;
  cs_float aa = FL(0.0);
  early = nsmps - early;      /* Bit at end to ignore */
  if (UNLIKELY(nargs > nouts))
    return csound->InitError(csound,
                             Str("Cannot be more In arguments than Out in "
                                 "init (%d,%d)"),p->OUTOCOUNT, nargs);
  for (i=0; i<nargs; i++) {
    aa = *p->a[i];
    cs_float *r =p->r[i];
    for (n = 0; n < nsmps; n++)
      r[n] = (n < offset || n > early ? FL(0.0) : aa);
  }
  for (; i<nouts; i++) {
    cs_float *r =p->r[i];
    memset(r, '\0', nsmps*sizeof(cs_float));
    for (n = 0; n < nsmps; n++)
      r[n] = (n < offset || n > early ? FL(0.0) : aa);
  }

  return OK;
}

int32_t mainit2(CSOUND *csound, ASSIGNM *p)
{
  uint32_t nargs = p->INOCOUNT;
  uint32_t nouts = p->OUTOCOUNT;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, n, nsmps = CS_KSMPS;
  cs_float *aa;
  early = nsmps - early;      /* Bit at end to ignore */
  if (UNLIKELY(nargs != nouts))
    return csound->InitError(csound,
                             Str("Out and in numbers not matching in "
                                 "assignment (%d,%d)"),nouts, nargs);
  for (i=0; i<nargs; i++) {
    aa = p->a[i];
    cs_float *r =p->r[i];
    for (n = 0; n < nsmps; n++)
      r[n] = (n < offset || n > early ? FL(0.0) : aa[n]);
  }

  return OK;
}


int32_t signum(CSOUND *csound, ASSIGN *p)
{
  IGN(csound);
  cs_float a = *p->a;
  int32_t ans = (a==FL(0.0) ? 0 : a<FL(0.0) ? -1 : 1);
  *p->r = (cs_float) ans;
  return OK;
}

int32_t asignum(CSOUND *csound, ASSIGN *p)
{
  IGN(csound);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t   i, nsmps = CS_KSMPS;
  cs_float *a = p->a;
  memset(p->r, '\0', nsmps*sizeof(cs_float));
  early = nsmps-early;
  for (i=offset; i<early; i++) {
    cs_float aa = a[i];
    int32_t ans = (aa==FL(0.0) ? 0 : aa<FL(0.0) ? -1 : 1);
    p->r[i] = (cs_float) ans;
  }
  return OK;
}

#define RELATN(OPNAME,OP)                                       \
  int32_t OPNAME(CSOUND *csound, RELAT *p)                      \
  {   IGN(csound); *p->rbool = (*p->a OP *p->b) ? 1 : 0; \
    return OK; }

RELATN(gt,>)
RELATN(ge,>=)
RELATN(lt,<)
RELATN(le,<=)
RELATN(eq,==)
RELATN(ne,!=)

int32_t bassign(CSOUND *csound, RELAT *p) {
  *p->rbool = ((int32_t) *p->a != 0) ? 1 : 0;
  return OK;
}


int32_t b_not(CSOUND *csound, LOGCL *p)
{
  IGN(csound); *p->rbool = (*p->ibool) ? 0 : 1; return OK; }

#define LOGCLX(OPNAME,OP)                                               \
  int32_t OPNAME(CSOUND *csound, LOGCL *p)                              \
  { IGN(csound);*p->rbool = (*p->ibool OP *p->jbool) ? 1 : 0; return OK; }

LOGCLX(and,&&)
LOGCLX(or,||)

// k-rate cs_float logical operations - both k-rate cs_float
int32_t and_kk_bool(CSOUND *csound, LOGCL_KK *p)
{
  IGN(csound);
  int32_t a_bool = (*p->a != FL(0.0)) ? 1 : 0;
  int32_t b_bool = (*p->b != FL(0.0)) ? 1 : 0;
  *p->rbool = (a_bool && b_bool) ? 1 : 0;
  return OK;
}

int32_t or_kk_bool(CSOUND *csound, LOGCL_KK *p)
{
  IGN(csound);
  int32_t a_bool = (*p->a != FL(0.0)) ? 1 : 0;
  int32_t b_bool = (*p->b != FL(0.0)) ? 1 : 0;
  *p->rbool = (a_bool || b_bool) ? 1 : 0;
  return OK;
}


#define KK(OPNAME,OP)                                   \
  int32_t OPNAME(CSOUND *csound, AOP *p)                \
  { IGN(csound); *p->r = *p->a OP *p->b; return OK; }

int32_t addkk(CSOUND *csound, AOP *p)
{
  cs_float a = *p->a, b = *p->b;
  *p->r = a + b;

  return OK;
}
KK(subkk,-)
KK(mulkk,*)
//KK(divkk,/)
int32_t divkk(CSOUND *csound, AOP *p)
{
  cs_float div = *p->b;
  IGN(csound);
  if (UNLIKELY(div==FL(0.0)))
    csound->Warning(csound, Str("Division by zero"));
  *p->r = *p->a / div;
  return OK;
}

cs_float MOD(cs_float a, cs_float bb)
{
  if (UNLIKELY(bb==FL(0.0))) return FL(0.0);
  else {
    cs_float b = (bb<0 ? -bb : bb);
    cs_float d = FMOD(a, b);
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

#define KA(OPNAME,OP)                                                   \
  int32_t OPNAME(CSOUND *csound, AOP *p) {                              \
    uint32_t n, nsmps = CS_KSMPS;                                       \
    IGN(csound);                                                        \
    if (LIKELY(nsmps!=1)) {                                             \
      cs_float   *r, a, *b;                                                \
      uint32_t offset = p->h.insdshead->ksmps_offset;                   \
      uint32_t early  = p->h.insdshead->ksmps_no_end;                   \
      r = p->r;                                                         \
      a = *p->a;                                                        \
      b = p->b;                                                         \
      if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));      \
      if (UNLIKELY(early)) {                                            \
        nsmps -= early;                                                 \
        memset(&r[nsmps], '\0', early*sizeof(cs_float));                   \
      }                                                                 \
      for (n=offset; n<nsmps; n++)                                      \
        r[n] = a OP b[n];                                               \
      return OK;                                                        \
    }                                                                   \
    else {                                                              \
      *p->r = *p->a OP *p->b;                                           \
      return OK;                                                        \
    }                                                                   \
  }


KA(addka,+)
KA(subka,-)
KA(mulka,*)
KA(divka,/)

int32_t modka(CSOUND *csound, AOP *p)
{
  IGN(csound);
  cs_float   *r, a, *b;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  r = p->r;
  a = *p->a;
  b = p->b;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++)
    r[n] = MOD(a, b[n]);
  return OK;
}

#define AK(OPNAME,OP)                                   \
  int32_t OPNAME(CSOUND *csound, AOP *p) {              \
    uint32_t n, nsmps = CS_KSMPS;                       \
    IGN(csound);                                        \
    if (LIKELY(nsmps != 1)) {                           \
      cs_float   *r, *a, b;                                \
      uint32_t offset = p->h.insdshead->ksmps_offset;   \
      uint32_t early  = p->h.insdshead->ksmps_no_end;   \
      r = p->r;                                         \
      a = p->a;                                         \
      b = *p->b;                                        \
      if (UNLIKELY(offset))                             \
        memset(r, '\0', offset*sizeof(cs_float));          \
      if (UNLIKELY(early)) {                            \
        nsmps -= early;                                 \
        memset(&r[nsmps], '\0', early*sizeof(cs_float));   \
      }                                                 \
      for (n=offset; n<nsmps; n++)                      \
        r[n] = a[n] OP b;                               \
      return OK;                                        \
    }                                                   \
    else {                                              \
      p->r[0] = p->a[0] OP *p->b;                       \
      return OK;                                        \
    }                                                   \
  }

AK(addak,+)
AK(subak,-)
AK(mulak,*)
//AK(divak,/)
int32_t divak(CSOUND *csound, AOP *p) {
  uint32_t n, nsmps = CS_KSMPS;
  cs_float b = *p->b;
  if (LIKELY(nsmps != 1)) {
    cs_float   *r, *a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    r = p->r;
    a = p->a;
    b = *p->b;
    if (UNLIKELY(b==FL(0.0)))
      csound->Warning(csound, Str("Division by zero"));
    if (UNLIKELY(offset))
      memset(r, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(cs_float));
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
  cs_float   *r, *a, b;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  r = p->r;
  a = p->a;
  b = *p->b;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++)
    r[n] = MOD(a[n], b);
  return OK;
}

#define AA(OPNAME,OP)                                                   \
  int32_t OPNAME(CSOUND *csound, AOP *p) {                              \
    cs_float   *r, *a, *b;                                                 \
    IGN(csound);                                                        \
    uint32_t n, nsmps = CS_KSMPS;                                       \
    if (LIKELY(nsmps!=1)) {                                             \
      uint32_t offset = p->h.insdshead->ksmps_offset;                   \
      uint32_t early  = p->h.insdshead->ksmps_no_end;                   \
      r = p->r;                                                         \
      a = p->a;                                                         \
      b = p->b;                                                         \
      if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));      \
      if (UNLIKELY(early)) {                                            \
        nsmps -= early;                                                 \
        memset(&r[nsmps], '\0', early*sizeof(cs_float));                   \
      }                                                                 \
      for (n=offset; n<nsmps; n++)                                      \
        r[n] = a[n] OP b[n];                                            \
      return OK;                                                        \
    }                                                                   \
    else {                                                              \
      *p->r = *p->a OP *p->b;                                           \
      return OK;                                                        \
    }                                                                   \
  }

/* VL
   experimental code using SSE for operations
   needs memory alignment - 16 bytes
*/
#ifdef USE_SSE
#include "emmintrin.h"
#define AA_VEC(OPNAME,OP)                                               \
  int32_t OPNAME(CSOUND *csound, AOP *p){                               \
    cs_float   *r, *a, *b;                                                 \
    __m128d va, vb;                                                     \
    uint32_t n, nsmps = CS_KSMPS, end;                                  \
    if (LIKELY(nsmps!=1)) {                                             \
      uint32_t offset = p->h.insdshead->ksmps_offset;                   \
      uint32_t early  = p->h.insdshead->ksmps_no_end;                   \
      r = p->r; a = p->a; b = p->b;                                     \
      if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));      \
      if (UNLIKELY(early)) {                                            \
        nsmps -= early;                                                 \
        memset(&r[nsmps], '\0', early*sizeof(cs_float));                   \
      }                                                                 \
      end = nsmps;                                                      \
      for (n=offset; n<end; n+=2) {                                     \
        va = _mm_loadu_pd(&a[n]);                                       \
        vb = _mm_loadu_pd(&b[n]);                                       \
        va = OP(va,vb);                                                 \
        _mm_storeu_pd(&r[n],va);                                        \
      }                                                                 \
      return OK;                                                        \
    }                                                                   \
    else {                                                              \
      *p->r = *p->a + *p->b;                                            \
      return OK;                                                        \
    }                                                                   \
  }                                                                     \

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
  cs_float   *r, *a, *b;
  int32_t     err = 0;
  IGN(csound);
  uint32_t n, nsmps = CS_KSMPS;
  if (LIKELY(nsmps!=1)) {
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    r = p->r;
    a = p->a;
    b = p->b;
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps; n++ ) {
      cs_float bb = b[n];
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
  cs_float   *r, *a, *b;
  IGN(csound);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  r = p->r;
  a = p->a;
  b = p->b;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
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
  cs_float    *r, a, *b, def;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS;

  r = p->r;
  a = *p->a;
  b = p->b;
  def = *p->def;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    cs_float bb = b[n];
    r[n] = (bb==FL(0.0) ? def : a / bb);
  }
  return OK;
}

int32_t divzak(CSOUND *csound, DIVZ *p)
{
  uint32_t n;
  IGN(csound);
  cs_float    *r, *a, b, def;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS;

  r = p->r;
  a = p->a;
  b = *p->b;
  def = *p->def;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
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
  cs_float    *r, *a, *b, def;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS;

  r = p->r;
  a = p->a;
  b = p->b;
  def = *p->def;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    cs_float bb=b[n];
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
  uint32_t offset = p->h.insdshead->ksmps_offset*sizeof(cs_float);
  uint32_t early  = p->h.insdshead->ksmps_no_end*sizeof(cs_float);
  cs_float   *r, *s;
  IGN(csound);

  r = p->r;
  if (*p->cond)
    s = p->a;
  else s = p->b;
  if (r!=s) {
    memset(r, '\0', offset);
    memcpy(&r[offset], &s[offset], CS_KSMPS*sizeof(cs_float)-offset-early);
    memset(&r[offset-early], '\0', early);
  }
  return OK;
}

int32_t int1(CSOUND *csound, EVAL *p)               /* returns signed whole no. */
{
  cs_float intpart;
  IGN(csound);
  MODF(*p->a, &intpart);
  *p->r = intpart;
  return OK;
}

int32_t int1a(CSOUND *csound, EVAL *p)              /* returns signed whole no. */
{
  cs_float        intpart, *a=p->a, *r=p->r;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  IGN(csound);

  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++) {
    MODF(a[n], &intpart);
    r[n] = intpart;
  }
  return OK;
}

int32_t frac1(CSOUND *csound, EVAL *p)              /* returns positive frac part */
{
  cs_float intpart, fracpart;
  IGN(csound);
  fracpart = MODF(*p->a, &intpart);
  *p->r = fracpart;
  return OK;
}

int32_t frac1a(CSOUND *csound, EVAL *p)             /* returns positive frac part */
{
  cs_float intpart, fracpart, *r = p->r, *a = p->a;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  IGN(csound);

  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++) {
    fracpart = MODF(a[n], &intpart);
    r[n] = fracpart;
  }
  return OK;
}

/* Match CS_FLOAT2LRND's tie behavior without narrowing the result to int32_t. */
#if defined(USE_LRINT) || defined(MSVC) || \
    (defined(HAVE_GCC3) && defined(__i386__) && !defined(__ICC))
#  ifdef USE_DOUBLE
#    define ROUND_VALUE nearbyint
#  else
#    define ROUND_VALUE nearbyintf
#  endif
#else
#  ifdef USE_DOUBLE
#    define ROUND_VALUE round
#  else
#    define ROUND_VALUE roundf
#  endif
#endif

int32_t int1_round(CSOUND *csound, EVAL *p)         /* round to nearest integer */
{
  IGN(csound);
  *p->r = ROUND_VALUE(*p->a);
  return OK;
}

int32_t int1a_round(CSOUND *csound, EVAL *p)        /* round to nearest integer */
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  cs_float *r=p->r, *a=p->a;
  IGN(csound);

  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++)
    r[n] = ROUND_VALUE(a[n]);
  return OK;
}

int32_t int1_floor(CSOUND *csound, EVAL *p)         /* round down */
{
  IGN(csound);
  *p->r = FLOOR(*p->a);
  return OK;
}

int32_t int1a_floor(CSOUND *csound, EVAL *p)        /* round down */
{
  cs_float    *a=p->a, *r=p->r;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  IGN(csound);

  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++)
    r[n] = FLOOR(a[n]);
  return OK;
}

int32_t int1_ceil(CSOUND *csound, EVAL *p)          /* round up */
{
  IGN(csound);
  *p->r = CEIL(*p->a);
  return OK;
}

int32_t int1a_ceil(CSOUND *csound, EVAL *p)         /* round up */
{
  cs_float    *a=p->a, *r=p->r;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  IGN(csound);

  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++)
    r[n] = CEIL(a[n]);
  return OK;
}

#define rndmlt (105.947)

int32_t rnd1seed(CSOUND *csound, INM *p)
{
  cs_double intpart;
  csound->rndfrac = cs_modf(*p->ar, &intpart);
  return OK;
}

int32_t rnd1(CSOUND *csound, EVAL *p)               /* returns unipolar rand(x) */
{
  cs_double intpart;
  csound->rndfrac = cs_modf(csound->rndfrac * rndmlt, &intpart);
  *p->r = *p->a * (cs_float)csound->rndfrac;
  return OK;
}

int32_t birnd1(CSOUND *csound, EVAL *p)             /* returns bipolar rand(x) */
{
  cs_double intpart;
  csound->rndfrac = cs_modf(csound->rndfrac * rndmlt, &intpart);
  *p->r = *p->a * (FL(2.0) * (cs_float)csound->rndfrac - FL(1.0));
  return OK;
}

#define LIB1(OPNAME,LIBNAME)  int32_t OPNAME(CSOUND *csound, EVAL *p)   \
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

#define LIBA(OPNAME,LIBNAME) int32_t OPNAME(CSOUND *csound, EVAL *p) {  \
    IGN(csound);                                                        \
    uint32_t offset = p->h.insdshead->ksmps_offset;                     \
    uint32_t early  = p->h.insdshead->ksmps_no_end;                     \
    uint32_t n, nsmps =CS_KSMPS;                                        \
    cs_float   *r, *a;                                                     \
    r = p->r;                                                           \
    a = p->a;                                                           \
    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));        \
    if (UNLIKELY(early)) {                                              \
      nsmps -= early;                                                   \
      memset(&r[nsmps], '\0', early*sizeof(cs_float));                     \
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
  cs_float   *r, *a, *b;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  IGN(csound);
  r = p->r;
  a = p->a;
  b = p->b;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
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
  cs_float   *r = p->r, *a = p->a;
  IGN(csound);

  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
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
  cs_float   *r, *a;

  r = p->r;
  a = p->a;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++)
    r[n] = csound->e0dbfs * EXP(a[n] * LOG10D20);
  return OK;
}

int32_t ftlen(CSOUND *csound, EVAL *p)
{
  FUNC    *ftp;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->a)) == NULL)) {
    *p->r = -FL(1.0);       /* Return something */
    return NOTOK;
  }
  *p->r = (cs_float)ftp->flen;

  return OK;
}

int32_t ftchnls(CSOUND *csound, EVAL *p)
{
  FUNC    *ftp;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->a)) == NULL)) {
    *p->r = -FL(1.0);       /* Return something */
    return NOTOK;
  }
  *p->r = (cs_float)ftp->nchanls;

  return OK;
}

int32_t ftcps(CSOUND *csound, EVAL *p)
{
  FUNC    *ftp;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->a)) == NULL)
      || ftp->cpscvt == FL(0.0)) {
    *p->r = -FL(1.0);       /* Return something */
    return NOTOK;
  }
  *p->r = (cs_float)(ftp->cvtbas/ftp->cpscvt);

  return OK;
}



int32_t ftlptim(CSOUND *csound, EVAL *p)
{
  FUNC    *ftp;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->a)) == NULL))
    return NOTOK;
  if (LIKELY(ftp->loopmode1))
    *p->r = ftp->begin1 * CS_ONEDSR;
  else {
    *p->r = FL(0.0);
    csound->Warning(csound, Str("non-looping sample"));
  }
  return OK;
}

int32_t numsamp(CSOUND *csound, EVAL *p)        /***** nsamp by G.Maldonado ****/
{
  FUNC    *ftp;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->a)) == NULL)) {
    *p->r = FL(0.0);
    return NOTOK;
  }
  /* if (ftp->soundend) */
  *p->r = (cs_float)ftp->soundend;
  /* else
   *p->r = (cs_float)(ftp->flen + 1); */

  return OK;
}

int32_t ftsr(CSOUND *csound, EVAL *p)               /**** ftsr by G.Maldonado ****/
{
  FUNC    *ftp;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->a)) == NULL)) {
    *p->r = FL(0.0);
    return NOTOK;
  }
  *p->r = ftp->gen01args.sample_rate;

  return OK;
}

int32_t rtclock(CSOUND *csound, EVAL *p)
{
  *p->r = (cs_float)csoundGetRealTime(csound->csRtClock);
  return OK;
}

int32_t octpch(CSOUND *csound, EVAL *p)
{
  cs_double fract, oct;
  cs_double in = (cs_double)*p->a;
  fract = cs_modf(in, &oct);
  fract *= EIPT3;
  *p->r = (cs_float)(oct + fract);
  return OK;
}

int32_t pchoct(CSOUND *csound, EVAL *p)
{
  cs_double fract, oct;
  cs_double in = (cs_double)*p->a;
  fract = cs_modf(in, &oct);
  fract *= 0.12;
  *p->r = (cs_float)(oct + fract);
  return OK;
}

int32_t cpsoct(CSOUND *csound, EVAL *p)
{
  int32_t loct = (int32_t)(*p->a * OCTRES);
  *p->r = (cs_float)CPSOCTL(loct);
  return OK;
}

int32_t acpsoct(CSOUND *csound, EVAL *p)
{
  cs_float   *r, *a;
  int32_t    loct;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;

  a = p->a;
  r = p->r;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++) {
    loct = (int32_t)(a[n] * OCTRES);
    r[n] = CPSOCTL(loct);
  }
  return OK;
}

int32_t octcps(CSOUND *csound, EVAL *p)
{
  *p->r = (LOG(*p->a /(cs_float)ONEPT) / (cs_float)LOGTWO);
  return OK;
}

int32_t cpspch(CSOUND *csound, EVAL *p)
{
  cs_double in = (cs_double)*p->a;
  cs_double fract, oct;
  int32_t loct;
  fract = cs_modf(in, &oct);
  fract *= EIPT3;
  loct = (int32_t)CS_FLOAT2LRND((oct + fract) * OCTRES);
  *p->r = (cs_float)CPSOCTL(loct);
  return OK;
}

int32_t cpsmidinn(CSOUND *csound, EVAL *p)
{
  cs_float note = *p->a;         /* (note-69)>12* */
  if (note > 12*32+69 || note < 0)
    return csound->InitError(csound, Str("MIDI note %f out of range"), note);
  *p->r = POWER(FL(2.0),
                (note - FL(69.0)) / FL(12.0)) * (cs_float)(csound->A4);
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
  cs_double fract, oct, octdec;
  /* Convert Midi Note number to 8ve.decimal format */
  octdec = ((cs_double)*p->a / 12.0) + MIDINOTE0;
  /* then convert to 8ve.pc format */
  fract = cs_modf(octdec, &oct);
  fract *= 0.12;
  *p->r = (cs_float)(oct + fract);
  return OK;
}

int32_t cpsxpch(CSOUND *csound, XENH *p)
{                               /* This may be too expensive */
  cs_double  fract;
  cs_double  loct;

  fract = cs_modf((cs_double)*p->pc, &loct); /* Get octave */
  if (*p->et > 0) {
    fract = pow((cs_double)*p->cy, loct + (100.0*fract)/((cs_double)*p->et));
    *p->r = (cs_float)fract * *p->ref;
  }
  else {                      /* Values in a table */
    cs_float t = - *p->et;
    FUNC* ftp = csound->FTFind(csound, &t);
    int32_t len, frt;
    if (UNLIKELY(ftp == NULL))
      return csound->InitError(csound, Str("No tuning table %g"), t);
    len = ftp->flen;
    frt = (int32_t)(100.0 * fract + (fract < 0.0 ? -0.5 : 0.5));
    /* Wrap scale degrees in either direction, carrying whole periods. */
    loct += frt / len;
    frt %= len;
    if (frt < 0) {
      frt += len;
      loct--;
    }
    *p->r = *p->ref * *(ftp->ftable + frt) *
      POWER(*p->cy, (cs_float)loct);
  }
  return OK;
}

int32_t cps2pch(CSOUND *csound, XENH *p)
{
  cs_double  fract;
  cs_double  loct;

  fract = cs_modf((cs_double)*p->pc, &loct);        /* Get octave */
  if (*p->et > 0) {
    fract = pow(2.0, loct + (100.0*fract)/((cs_double)*p->et));
    *p->r = (cs_float)(fract * 1.02197503906); /* Refer to base frequency */
  }
  else {
    cs_float t = - *p->et;
    FUNC* ftp = csound->FTFind(csound, &t);
    int32_t len, frt;
    if (UNLIKELY(ftp == NULL))
      return csound->InitError(csound, Str("No tuning table %g"), t);
    len = ftp->flen;
    frt = (int32_t)(100.0 * fract + (fract < 0.0 ? -0.5 : 0.5));
    /* Wrap scale degrees in either direction, carrying whole periods. */
    loct += frt / len;
    frt %= len;
    if (frt < 0) {
      frt += len;
      loct--;
    }
    *p->r = (cs_float)(1.02197503906 * *(ftp->ftable + frt) *
                    pow(2.0, loct));
  }

  /*  double ref = 261.62561 / pow(2.0, 8.0); */
  return OK;
}

int32_t cpstun_i(CSOUND *csound, CPSTUNI *p)
{
  FUNC  *ftp;
  cs_float *func;
  int32_t notenum = (int32_t)*p->input;
  int32_t grade;
  int32_t numgrades;
  int32_t basekeymidi;
  cs_float basefreq, factor, interval;
  if (UNLIKELY((ftp = csound->FTFind(csound, p->tablenum)) == NULL)) goto err1;
  func = ftp->ftable;
  numgrades = (int32_t)*func++;
  interval = *func++;
  basefreq = *func++;
  basekeymidi = (int32_t)*func++;

  if (notenum < basekeymidi) {
    notenum = basekeymidi - notenum;
    grade  = (numgrades-(notenum % numgrades)) % numgrades;
    factor = - (cs_float)(int32_t)((notenum+numgrades-1) / numgrades) ;
  }
  else {
    notenum = notenum - basekeymidi;
    grade  = notenum % numgrades;
    factor = (cs_float)(int32_t)(notenum / numgrades);
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
    cs_float *func;
    int32_t notenum = (int32_t)*p->kinput;
    int32_t grade;
    int32_t numgrades;
    int32_t basekeymidi;
    cs_float basefreq, factor, interval;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->tablenum)) == NULL))
      goto err1;
    func = ftp->ftable;
    numgrades = (int32_t)*func++;
    interval = *func++;
    basefreq = *func++;
    basekeymidi = (int32_t)*func++;

    if (notenum < basekeymidi) {
      notenum = basekeymidi - notenum;
      grade  = (numgrades-(notenum % numgrades)) % numgrades;
      factor = - (cs_float)(int32_t)((notenum+numgrades-1) / numgrades) ;
    }
    else {
      notenum = notenum - basekeymidi;
      grade  = notenum % numgrades;
      factor = (cs_float)(int32_t)(notenum / numgrades);
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
    cs_double  x = (1.0 / INTERVAL);
    int32_t     i;
    csound->logbase2 = (cs_float*) csound->Malloc(csound, (STEPS + 1)
                                               * sizeof(cs_float));
    for (i = 0; i <= STEPS; i++) {
      csound->logbase2[i] = ONEdLOG2 * LOG((cs_float)x);
      x += ((INTERVAL - 1.0 / INTERVAL) / (cs_double)STEPS);
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
  cs_float    *a=p->a, *r=p->r;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++)
    r[n] = POWER(FL(2.0), a[n]);
  return OK;
}

#define ONEd12          (FL(0.08333333333333333333333))
#define ONEd1200        (FL(0.00083333333333333333333))

int32_t semitone(CSOUND *csound, EVAL *p)
{
  cs_float a = *p->a*ONEd12;
  *p->r = POWER(FL(2.0), a);
  return OK;
}

int32_t asemitone(CSOUND *csound, EVAL *p)            /* JPff */
{
  cs_float *r, *a;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  a = p->a;
  r = p->r;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++) {
    cs_float aa = (a[n])*ONEd12;
    r[n] = POWER(FL(2.0), aa);
  }
  return OK;
}

int32_t cent(CSOUND *csound, EVAL *p)
{
  cs_float a = *p->a;
  *p->r = POWER(FL(2.0), a/FL(1200.0));
  return OK;
}

int32_t acent(CSOUND *csound, EVAL *p)        /* JPff */
{
  cs_float *r, *a;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  a = p->a;
  r = p->r;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++) {
    cs_float aa = (a[n])*ONEd1200;
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
  cs_float *r, *a;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  a = p->a;
  r = p->r;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++) {
    cs_float aa = a[n];
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
  cs_float *r, *a;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps =CS_KSMPS;
  a = p->a;
  r = p->r;
  if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&r[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++) {
    cs_float aa = a[n];
    int32_t indx = (int32_t)((aa -(FL(1.0)/INTERVAL)) / (INTERVAL-FL(1.0)/INTERVAL)
                             *  STEPS + FL(0.5));
    if (indx<0 || indx>STEPS) r[n] = LOG(aa)*ONEdLOG2;
    else                     r[n] = csound->logbase2[indx];
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
  if(CS_ESR != csound->esr)
    return csound->InitError(csound,
                             "local sampling rate not supported\n");

  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS - early;
  if (csound->inchnls != 1)
    return csound->PerfError(csound,
                             &(p->h),
                             "Wrong numnber of input channels\n");
  CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset)) memset(p->ar, '\0', offset*sizeof(cs_float));
  memcpy(&p->ar[offset], &CS_SPIN[offset],
         (nsmps-offset) * sizeof(cs_float));
  if (UNLIKELY(early))
    memset(&p->ar[nsmps], '\0', early * sizeof(cs_float));
  CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int32_t inarray_set(CSOUND *csound, INA *p){
  if(CS_ESR != csound->esr)
    return csound->InitError(csound,
                             "local sampling rate not supported\n");
  /* Reused instances may now need more samples per array element. */
  if (p->tabout->data != NULL &&
      (size_t)p->tabout->arrayMemberSize < CS_KSMPS * sizeof(cs_float))
    csound_free_array_storage(csound, p->tabout);
  if (UNLIKELY(tabinit(csound, p->tabout, csound->inchnls,
                       p->h.insdshead) != OK))
    return csound_array_init_resize_error(csound);
  return OK;
}

int32_t inarray(CSOUND *csound, INA *p)
{
  cs_float *data = p->tabout->data;
  uint32_t n = p->tabout->sizes[0];
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t m, nsmps =CS_KSMPS, i;
  size_t ksmps = p->tabout->arrayMemberSize / sizeof(cs_float);

  if ((int32_t)n>csound->inchnls) n = csound->inchnls;
  CSOUND_SPIN_SPINLOCK
    if (UNLIKELY(offset))
      for (i = 0; i < n; i++)
        memset(&data[i*ksmps], '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    for (i = 0; i < n; i++)
      memset(&data[nsmps+i*ksmps], '\0', early*sizeof(cs_float));
  }
  for (m = offset; m < nsmps; m++) {
    cs_float *frame = &CS_SPIN[m*csound->inchnls];
    for (i = 0; i < n; i++)
      data[m+i*ksmps] = frame[i];
  }
  CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int32_t ins(CSOUND *csound, INS *p)
{
  cs_float       *sp, *ar1, *ar2;
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
    memset(ar1, '\0', offset*sizeof(cs_float));
    memset(ar2, '\0', offset*sizeof(cs_float));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&p->ar1[nsmps], '\0', early * sizeof(cs_float));
    memset(&p->ar2[nsmps], '\0', early * sizeof(cs_float));
  }
  for (n=offset, k=offset*2; n<nsmps; n++, k+=2) {
    ar1[n] = sp[k];
    ar2[n] = sp[k+1];
  }
  CSOUND_SPIN_SPINUNLOCK
    return OK;
}

int32_t inq(CSOUND *csound, INQ *p)
{
  cs_float       *sp = CS_SPIN, *ar1 = p->ar1, *ar2 = p->ar2,
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
      memset(ar1, '\0', offset*sizeof(cs_float));
      memset(ar2, '\0', offset*sizeof(cs_float));
      memset(ar3, '\0', offset*sizeof(cs_float));
      memset(ar4, '\0', offset*sizeof(cs_float));
    }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar1[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar2[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar3[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar4[nsmps], '\0', early * sizeof(cs_float));
  }
  for (n=offset, k=offset*4; n<nsmps; n++, k+=4) {
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
  cs_float *sp = CS_SPIN, *ar1 = p->ar1, *ar2 = p->ar2, *ar3 = p->ar3,
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
      memset(ar1, '\0', offset*sizeof(cs_float));
      memset(ar2, '\0', offset*sizeof(cs_float));
      memset(ar3, '\0', offset*sizeof(cs_float));
      memset(ar4, '\0', offset*sizeof(cs_float));
      memset(ar5, '\0', offset*sizeof(cs_float));
      memset(ar6, '\0', offset*sizeof(cs_float));
    }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar1[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar2[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar3[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar4[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar5[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar6[nsmps], '\0', early * sizeof(cs_float));
  }
  for (n=offset, k=offset*6; n<nsmps; n++, k+=6) {
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
  cs_float       *sp = CS_SPIN, *ar1 = p->ar1, *ar2 = p->ar2, *ar3 = p->ar3,
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
      memset(ar1, '\0', offset*sizeof(cs_float));
      memset(ar2, '\0', offset*sizeof(cs_float));
      memset(ar3, '\0', offset*sizeof(cs_float));
      memset(ar4, '\0', offset*sizeof(cs_float));
      memset(ar5, '\0', offset*sizeof(cs_float));
      memset(ar6, '\0', offset*sizeof(cs_float));
      memset(ar7, '\0', offset*sizeof(cs_float));
      memset(ar8, '\0', offset*sizeof(cs_float));
    }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar1[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar2[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar3[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar4[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar5[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar6[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar7[nsmps], '\0', early * sizeof(cs_float));
    memset(&ar8[nsmps], '\0', early * sizeof(cs_float));
  }
  for (n=offset, k=offset*8; n<nsmps; n++, k+=8) {
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

static int32_t in_fixed_channels(CSOUND *csound, INALL *p, uint32_t channels)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  uint32_t m, nsmps = CS_KSMPS, i;
  cs_float *sp, **ara = p->ar;
  if (UNLIKELY(csound->inchnls != (int32_t)channels))
    return csound->PerfError(csound, &(p->h),
                             "Wrong numnber of input channels\n");
  sp = &CS_SPIN[offset*channels];
  CSOUND_SPIN_SPINLOCK
  if (UNLIKELY(offset))
    for (i = 0; i < channels; i++)
      memset(ara[i], '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    for (i = 0; i < channels; i++)
      memset(&ara[i][nsmps], '\0', early*sizeof(cs_float));
  }
  for (m = offset; m < nsmps; m++)
    for (i = 0; i < channels; i++)
      ara[i][m] = *sp++;
  CSOUND_SPIN_SPINUNLOCK
  return OK;
}

int32_t in16(CSOUND *csound, INALL *p)
{
  return in_fixed_channels(csound, p, 16u);
}

int32_t in32(CSOUND *csound, INALL *p)
{
  return in_fixed_channels(csound, p, 32u);
}

int32_t inch1_set(CSOUND *csound, INCH1 *p)
{
  if(CS_ESR != csound->esr)
    return csound->InitError(csound,
                             "local sampling rate not supported\n");
  p->init = 1;
  return OK;
}

int32_t inch_opcode1(CSOUND *csound, INCH1 *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS, ch;
  cs_float *sp, *ain;

  ch = CS_FLOAT2LRND(*p->ch);
  if (UNLIKELY(ch > (uint32_t)csound->inchnls)) {
    if (p->init)
      csound->Message(csound, Str("Input channel %d too large; ignored\n"), ch);
    memset(p->ar, 0, sizeof(cs_float)*nsmps);
    p->init = 0;
    //        return OK;
  } else if (UNLIKELY(ch < 1)) {
    if (p->init)
      csound->Message(csound, Str("Input channel %d is invalid; ignored"), ch);
    memset(p->ar, 0, sizeof(cs_float)*nsmps);
    p->init = 0;
  }
  else {
    sp = CS_SPIN + offset*csound->inchnls + (ch - 1);
    ain = p->ar;
    if (UNLIKELY(offset)) memset(ain, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ain[nsmps], '\0', early*sizeof(cs_float));
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
  if(CS_ESR != csound->esr)
    return csound->InitError(csound,
                             "local sampling rate not supported\n");
  p->init = 1;
  return OK;
}

int32_t inch_opcode(CSOUND *csound, INCH *p)
{                               /* Rewritten to allow multiple args upto 40 */
  uint32_t nc, nChannels = p->INOCOUNT;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS, end = nsmps - early, ch;
  cs_float *sp, *ain;
  if (UNLIKELY(nChannels != p->OUTOCOUNT))
    return
      csound->PerfError(csound, &(p->h),
                        Str("Input and output argument count differs in inch"));
  for (nc=0; nc<nChannels; nc++) {
    ch = CS_FLOAT2LRND(*p->ch[nc]);
    if (UNLIKELY(ch > (uint32_t)csound->inchnls)) {
      if (p->init)
        csound->Warning(csound, Str("Input channel %d too large; ignored"), ch);
      memset(p->ar[nc], 0, sizeof(cs_float)*nsmps);
      p->init = 0;
      //        return OK;
    } else if (UNLIKELY(ch < 1)) {
      if (UNLIKELY(p->init))
        csound->Warning(csound, Str("Input channel %d is invalid; ignored"), ch);
      memset(p->ar[nc], 0, sizeof(cs_float)*nsmps);
      p->init = 0;
    } else {
      sp = CS_SPIN + offset*csound->inchnls + (ch - 1);
      ain = p->ar[nc];
      if (UNLIKELY(offset)) memset(ain, '\0', offset*sizeof(cs_float));
      if (UNLIKELY(early))
        memset(&ain[end], '\0', early*sizeof(cs_float));
      for (n = offset; n < end; n++) {
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
  uint32_t i, j = 0, nsmps = CS_KSMPS;
  uint32_t end = nsmps - p->h.insdshead->ksmps_no_end;
  cs_float *spin = CS_SPIN;

  CSOUND_SPIN_SPINLOCK
    m = (n < (uint32_t)csound->inchnls ? n : (uint32_t)csound->inchnls);
  for (j=0; j<nsmps; j++)
    if (j<offset || j>=end) {
      for (i=0 ; i < n; i++)
        p->ar[i][j] = FL(0.0);
    }
    else {
      for (i=0; i<m; i++) {
        p->ar[i][j] = spin[j*csound->inchnls + i];
      }
      for ( ; i < n; i++)
        p->ar[i][j] = FL(0.0);
    }
  CSOUND_SPIN_SPINUNLOCK
    return OK;
}

inline static int32_t outn(CSOUND *csound, uint32_t k,
                           uint32_t n, cs_float **asig,
                           INSDS *p, ARRAYDAT *arr)
{
  uint32_t nsmps = p->ksmps, ksmps = csound->ksmps,  i, j;
  cs_float *spout = p->spout;
  uint32_t offset = p->ksmps_offset;
  uint32_t early  = p->ksmps_no_end;
  early = nsmps - early;
  n -= k;
  k *= ksmps;
  for (i=0; i<n; i++) {
    /* The source array stride may differ from the global output stride. */
    cs_float *p = asig ? asig[i] :
      (cs_float *)((char *)arr->data + (size_t)i * arr->arrayMemberSize);
    for (j=offset; j < early; j++) {
      spout[k+j] += p[j];
    }
    // k always jumps by global ksmps
    k += ksmps;
  }
  return OK;
}

int32_t outs1(CSOUND *csound, OUTM *p)
{
  int32_t ret;
  ret  = outn(csound, 0, 1, &(p->asig),
              p->h.insdshead, NULL);
  return ret;
}

#define OUTCN(n)  if (n>csound->nchnls) return \
 csound->InitError(csound, "%s", \
 Str("Channel greater than nchnls")); \
  return OK;

int32_t och2(CSOUND *csound, OUTM *p) { IGN(p); OUTCN(2) }
int32_t och3(CSOUND *csound, OUTM *p) { IGN(p); OUTCN(3) }
int32_t och4(CSOUND *csound, OUTM *p) { IGN(p); OUTCN(4) }

/* using outn now */
int32_t outs2(CSOUND *csound, OUTM *p) {
  int32_t ret;
  ret = outn(csound, 1, 2, &(p->asig),
             p->h.insdshead, NULL);
  return ret;
}

int32_t outq3(CSOUND *csound, OUTM *p)
{
  int32_t ret;
  ret = outn(csound, 2, 3, &(p->asig),
             p->h.insdshead, NULL);
  return ret;
}

int32_t outq4(CSOUND *csound, OUTM *p)
{
  int32_t ret;
  ret = outn(csound, 3, 4, &(p->asig),
             p->h.insdshead, NULL);
  return ret;
}

int32_t outch(CSOUND *csound, OUTCH *p)
{
  uint32_t count = p->INOCOUNT, n, ch, nchnls = csound->nchnls;
  int32_t ret = OK;
  if (UNLIKELY((count&1)!=0))
    return csound->PerfError(csound, &(p->h),
             Str("outch must have an even number of arguments"));
  for(n=0; n < count; n+=2) {
    cs_float channel = *p->args[n];
    if (channel >= FL(1.0) && (cs_double)channel < (cs_double)nchnls + 1) {
      ch = (uint32_t)channel - 1;
      ret = outn(csound, ch, ch+1, &p->args[n+1],
                 p->h.insdshead, NULL);
    }
  }
  return ret;
}


int32_t ochn(CSOUND *csound, OUTX *p)
{
  uint32_t nch = p->INOCOUNT;
  if(CS_ESR != csound->esr)
    return csound->InitError(csound,
                             "local sampling rate not supported\n");
  if (nch>csound->nchnls)
    csound->Warning(csound, Str("Excess channels ignored"));
  return OK;
}

int32_t outall(CSOUND *csound, OUTX *p) /* Output a list of channels */
{
  uint32_t nch = p->INOCOUNT;
  int32_t ret = outn(csound, 0,
                     (nch <= csound->nchnls ? nch : csound->nchnls),
                     p->asig, p->h.insdshead, NULL);
  return ret;
}

int32_t outarr_init(CSOUND *csound, OUTARRAY *p)
{
  if(CS_ESR != csound->esr)
    return csound->InitError(csound,
                             "local sampling rate not supported\n");
  p->nowarn = 0;
  return OK;
}

int32_t outarr(CSOUND *csound, OUTARRAY *p)
{
  uint32_t n = p->tabin->sizes[0];
  int32_t ret;
  if (n>csound->nchnls) {
    if (p->nowarn==0) {
      csound->Warning(csound,
                      Str("out: number of channels truncated from %d to %d"),
                      n, csound->nchnls);
    }
    n = csound->nchnls;
    p->nowarn = 1;
  }
  ret = outn(csound, 0, n, NULL, p->h.insdshead, p->tabin);
  return ret;
}


int32_t outrep(CSOUND *csound, OUTM *p)
{
  uint32_t n = csound->nchnls, i;
  int32_t ret;
  for(i = 0; i < n; i++)
    ret = outn(csound, i, i+1, &p->asig,
               p->h.insdshead, NULL);
  return ret;
}

/* For parallel mixin template */
int32_t addina(CSOUND *csound, ASSIGN *p)
{
  cs_float* val = p->a;
  cs_float* ans = p->r;
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
  cs_float val;
  cs_float* ans = p->r;
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
  cs_float* val = p->a;
  cs_float* ans = p->r;
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
  cs_float val;
  cs_float* ans = p->r;
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

int32_t mulina(CSOUND *csound, ASSIGN *p)
{
  cs_float* val = p->a;
  cs_float* ans = p->r;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    nsmps = CS_KSMPS, n;
  uint32_t    early = nsmps-p->h.insdshead->ksmps_no_end;

  CSOUND_SPOUT_SPINLOCK
    for (n=offset; n<early; n++)
      ans[n] *= val[n];
  CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t mulinak(CSOUND *csound, ASSIGN *p)
{
  cs_float val;
  cs_float* ans = p->r;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    nsmps = CS_KSMPS, n;
  uint32_t    early = nsmps-p->h.insdshead->ksmps_no_end;

  CSOUND_SPOUT_SPINLOCK
    val = *p->a;
  for (n=offset; n<early; n++)
    ans[n] *= val;
  CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t mulin(CSOUND *csound, ASSIGN *p)
{
  CSOUND_SPOUT_SPINLOCK
    *p->r *= *p->a;
  CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t divin(CSOUND *csound, ASSIGN *p)
{
  CSOUND_SPOUT_SPINLOCK
    *p->r /= *p->a;
  CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t divina(CSOUND *csound, ASSIGN *p)
{
  cs_float* val = p->a;
  cs_float* ans = p->r;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    nsmps = CS_KSMPS, n;
  uint32_t    early = nsmps-p->h.insdshead->ksmps_no_end;

  CSOUND_SPOUT_SPINLOCK
    for (n=offset; n<early; n++)
      ans[n] /= val[n];
  CSOUND_SPOUT_SPINUNLOCK
    return OK;
}

int32_t divinak(CSOUND *csound, ASSIGN *p)
{
  cs_float val;
  cs_float* ans = p->r;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    nsmps = CS_KSMPS, n;
  uint32_t    early = nsmps-p->h.insdshead->ksmps_no_end;

  CSOUND_SPOUT_SPINLOCK
    val = *p->a;
  for (n=offset; n<early; n++)
    ans[n] /= val;
  CSOUND_SPOUT_SPINUNLOCK
    return OK;
}



int32_t is_NaN(CSOUND *csound, ASSIGN *p)
{
  IGN(csound);
  *p->r = isnan(*p->a) ? FL(1.0) : FL(0.0);
  return OK;
}

int32_t is_NaNa(CSOUND *csound, ASSIGN *p)
{
  IGN(csound);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t k, nsmps = CS_KSMPS;
  uint32_t early = nsmps - p->h.insdshead->ksmps_no_end;
  cs_float *a = p->a, *r = p->r, count = FL(0.0);

  /* Count before writing: the input and output may share a buffer. */
  for (k=offset; k<early; k++)
    if (isnan(a[k])) count += FL(1.0);

  /* These audio overloads return the block count at every active sample. */
  if (UNLIKELY(offset)) memset(r, 0, offset * sizeof(cs_float));
  if (UNLIKELY(early < nsmps))
    memset(r + early, 0, (nsmps - early) * sizeof(cs_float));
  for (k=offset; k<early; k++) r[k] = count;
  return OK;
}

int32_t is_inf(CSOUND *csound, ASSIGN *p)
{
  IGN(csound);
  cs_float value = *p->a;
  /* C only guarantees that isinf returns nonzero, not the infinity's sign. */
  *p->r = isinf(value) ? (value < FL(0.0) ? -FL(1.0) : FL(1.0)) : FL(0.0);
  return OK;
}

int32_t is_infa(CSOUND *csound, ASSIGN *p)
{
  IGN(csound);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t k, nsmps = CS_KSMPS;
  uint32_t early = nsmps - p->h.insdshead->ksmps_no_end;
  cs_float *a = p->a, *r = p->r, count = FL(0.0), sign = FL(1.0);

  for (k=offset; k<early; k++) {
    if (isinf(a[k])) {
      if (count == FL(0.0) && a[k] < FL(0.0)) sign = -FL(1.0);
      count += FL(1.0);
    }
  }
  count *= sign;
  if (UNLIKELY(offset)) memset(r, 0, offset * sizeof(cs_float));
  if (UNLIKELY(early < nsmps))
    memset(r + early, 0, (nsmps - early) * sizeof(cs_float));
  for (k=offset; k<early; k++) r[k] = count;
  return OK;
}

int32_t error_fn(CSOUND *csound, ERRFN *p)
{
  IGN(p);
  return csound->InitError(csound, Str("Unknown function called"));
}

/* ------------------------------------------------------------------------ */

static inline cs_float monitor_spout_sample(CSOUND *csound, INSDS *instance,
                                         uint32_t channel, uint32_t frame)
{
  cs_float *spout = instance->spout;
  uintptr_t base = (uintptr_t)csound->spout_tmp;
  uintptr_t current = (uintptr_t)spout;
  size_t samples = (size_t)csound->nspout * csound->oparms->numThreads;

  if (current < base || current - base >= samples * sizeof(cs_float))
    return spout[channel * csound->ksmps + frame];

  size_t offset = ((current - base) / sizeof(cs_float)) % csound->nspout;
  size_t index = offset + (size_t)channel * csound->ksmps + frame;
  cs_float value = csound->spout_tmp[index];

  if (csound->multiThreadedThreadInfo != NULL) {
    int32_t thread;
    for (thread = 1; thread < csound->oparms->numThreads; thread++)
      value += csound->spout_tmp[thread * (size_t)csound->nspout + index];
  }
  return value;
}

int32_t monitor_opcode_perf(CSOUND *csound, MONITOR_OPCODE *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t end = CS_KSMPS - p->h.insdshead->ksmps_no_end;
  uint32_t i, j, nsmps = CS_KSMPS, nchnls = csound->GetNchnls(csound);

  for (j = 0; j<nchnls; j++) {
    for (i = 0; i<nsmps; i++) {
      if (i < offset || i >= end)
        p->ar[j][i] = FL(0.0);
      else
        p->ar[j][i] = monitor_spout_sample(csound, p->h.insdshead, j, i);
    }
  }
  return OK;
}

int32_t monitor_opcode_init(CSOUND *csound, MONITOR_OPCODE *p)
{
  if(CS_ESR != csound->esr)
    return csound->InitError(csound,
                             "local sampling rate not supported\n");
  if (UNLIKELY(GetOutputArgCnt((OPDS *)p)
               != (int32_t)csound->GetNchnls(csound)))
    return csound->InitError(csound,
                             Str("number of arguments != nchnls"));
  p->h.perf = (SUBR) monitor_opcode_perf;
  return OK;
}

/* -------------------------------------------------------------------- */

int32_t outRange_i(CSOUND *csound, OUTRANGE *p)
{
  if(CS_ESR != csound->esr)
    return csound->InitError(csound,
                             "local sampling rate not supported\n");
  p->narg = p->INOCOUNT-1;

  return OK;
}

int32_t outRange(CSOUND *csound, OUTRANGE *p)
{
  cs_double start = (cs_double)*p->kstartChan;
  uint32_t narg = p->narg, nchnls = csound->nchnls, first;

  /* Validate before converting; fractional channel numbers still truncate. */
  if (UNLIKELY(narg == 0 || narg > nchnls ||
               !(start >= 1.0 && start < (cs_double)(nchnls - narg) + 2.0)))
    return csound->PerfError(csound, &p->h, "%s",
                             Str("outrg: channel range is outside output channels"));
  first = (uint32_t)start - 1;
  /* Use the instrument's output buffer and the global channel stride,
     just like out and outch, including local ksmps and subinstruments. */
  return outn(csound, first, first + narg, p->argums,
              p->h.insdshead, NULL);
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

int32_t inRange_i(CSOUND *csound, INRANGE *p)
{
  if(CS_ESR != csound->esr)
    return csound->InitError(csound,
                             "local sampling rate not supported\n");
  p->narg = p->INOCOUNT-1;
  if (UNLIKELY(!csound->GetOParms(csound)->sfread))
    return csound->InitError(csound, "%s", Str("inrg: audio input is not enabled"));
  p->numChans = csound->inchnls;
  return OK;
}

int32_t inRange(CSOUND *csound, INRANGE *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t j, nsmps = CS_KSMPS;
  int32_t i;
  cs_float *ara[VARGMAX];
  cs_double start = (cs_double)*p->kstartChan;
  int32_t narg = p->narg, numchans = p->numChans;
  int32_t startChan;
  cs_float *sp;

  /* Check the whole range before converting or forming an input pointer.
     Fractional channel numbers still truncate, as in the original opcode. */
  if (UNLIKELY(narg < 1 || narg > numchans ||
               !(start >= 1.0 && start < (cs_double)(numchans - narg) + 2.0)))
    return csound->PerfError(csound, &(p->h), "%s",
                             Str("inrg: channel range is outside input channels"));
  startChan = (int32_t)start - 1;

  if (UNLIKELY(early)) nsmps -= early;
  for (i = 0; i < narg; i++) {
    ara[i] = p->argums[i];
    if (UNLIKELY(offset)) memset(ara[i], '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) memset(&ara[i][nsmps], '\0', early*sizeof(cs_float));
    ara[i] += offset;
  }
  if (UNLIKELY(offset >= nsmps)) return OK;
  /* CS_SPIN includes the local-ksmps block offset. */
  sp = CS_SPIN + (size_t)offset*numchans + startChan;
  for (j=offset; j<nsmps; j++)  {
    for (i=0; i<narg; i++) {
      *ara[i]++ = sp[i];
    }
    sp += numchans;
  }
  return OK;

}

/* ***************************************************************** */
/* ***************************************************************** */
/* ***************************************************************** */
/* ***************************************************************** */
/*     icnt    pcnt */
/*     ival    pfld indx */
int32_t pcount(CSOUND *csound, PFIELD *p)
{
  if(csound->init_event != NULL)
     *p->ians = (cs_float) csound->init_event->pcnt;
  else *p->ians = 3;
  return OK;
}

int32_t pvalue(CSOUND *csound, PFIELD *p)
{
  if (UNLIKELY(csound->init_event == NULL ||
               !(*p->index >= FL(1.0) &&
                 (cs_double)*p->index < (cs_double)csound->init_event->pcnt + 1.0))) {
    return csound->InitError(csound, "%s", Str("invalid p field index"));
  }
  int32_t n = (int32_t)(*p->index);
  *p->ians = csound->init_event->p[n];
  return OK;
}

char *get_string_arg_from_evt(CSOUND *csound, cs_float p, EVTBLK *evt);

int32_t pvaluestr(CSOUND *csound, PFIELDSTR *p)
{
  if (UNLIKELY(csound->init_event == NULL ||
               !(*p->index >= FL(1.0) &&
                 (cs_double)*p->index < (cs_double)csound->init_event->pcnt + 1.0))) {
    return csound->InitError(csound, "%s", Str("invalid p field index"));
  }
  int32_t n = (int32_t)(*p->index);
  cs_float value = csound->init_event->p[n];
  if (UNLIKELY(!IsStringCode(value)))
    return csound->InitError(csound, Str("pindex: p-field %d is not a string"), n);

  /* The value and its string storage must come from the same event, including
     in subinstruments where pindex reads the host's p-fields. */
  char *str = csound->Strdup(csound,
                           get_string_arg_from_evt(csound, value,
                                                   csound->init_event));
  if (p->ians->data!=NULL) csound->Free(csound, p->ians->data);
  p->ians->data = str;
  p->ians->size = strlen(str) + 1;
  return OK;
}

int32_t pinit(CSOUND *csound, PINIT *p)
{
  if(csound->init_event != NULL) {
    int32_t n;
    int32_t nargs = p->OUTOCOUNT;
    int32_t pargs = csound->init_event->pcnt;
    int32_t start = (int32_t)(*p->start);
    /* Check for out-of-range start values */
    if (UNLIKELY(start < 1 || start > pargs)) {
      return csound->InitError(csound, "%s", Str("start value out of range"));
    }
    /* Should check that inits exist> */
    int32_t k = (int32_t)(*p->end);
    if (*p->end!=FL(0.0) && k < pargs) {
      pargs = k;
    }
    if (UNLIKELY(nargs > (pargs - start + 1)))
      csound->Warning(csound, "%s", Str("More arguments than p fields"));
    const int32_t last = (*p->end!=FL(0.0) ? pargs : pargs);
    const int32_t limit = last - start + 1;
    const int32_t upto = (nargs < limit ? nargs : limit);
    for (n=0; n<upto; n++) {
      // Use proper type checking to determine if output is string
      CS_TYPE *outType = GetTypeForArg(p->inits[n]);
      int isStringOutput = (outType != NULL &&
                           strcmp(outType->varTypeName, "S") == 0);

      if (IsStringCode(csound->init_event->p[n+start])) {
        // Source is string
        if (isStringOutput) {
          // String to string - safe assignment
          STRINGDAT *strOut = (STRINGDAT *)p->inits[n];
          if (strOut->data != NULL) {
            csound->Free(csound, strOut->data);
            strOut->data = NULL;
            strOut->size = 0;
          }
          const char *srcStr = get_string_arg_from_evt(
              csound, csound->init_event->p[n+start], csound->init_event);
          if (srcStr != NULL) {
            strOut->data = csound->Strdup(csound, srcStr);
            strOut->size = strlen(strOut->data) + 1;
          }
        } else {
          // String to numeric - store string code directly
          *p->inits[n] = csound->init_event->p[n+start];
        }
      } else {
        // Source is numeric
        if (isStringOutput) {
          // Numeric to string - convert to string representation
          STRINGDAT *strOut = (STRINGDAT *)p->inits[n];
          if (strOut->data != NULL) {
            csound->Free(csound, strOut->data);
            strOut->data = NULL;
            strOut->size = 0;
          }
          char numStr[32];
          snprintf(numStr, sizeof(numStr), "%.6f", csound->init_event->p[n+start]);
          strOut->data = csound->Strdup(csound, numStr);
          strOut->size = strlen(strOut->data) + 1;
        } else {
          // Numeric to numeric - direct assignment
          *p->inits[n] = csound->init_event->p[n+start];
        }
      }
    }
  } else return csoundInitError(csound, "no pfields available\n");
  return OK;
}


int32_t painit(CSOUND *csound, PAINIT *p)
{
 if(csound->init_event != NULL) {
  int32_t n;
  int32_t    pargs = csound->init_event->pcnt;
  int32_t    start = (int32_t)(*p->start);
  int32_t    k = (int32_t)(*p->end);
  if (*p->end!=FL(0.0)) {
    if (k<pargs) pargs = k;
  }
  if (UNLIKELY(tabinit(csound, p->inits, pargs-start+1,
                       p->h.insdshead) != OK))
    return csound_array_init_resize_error(csound);
  for (n=0; n<=pargs-start; n++) {
    ((cs_float*)p->inits->data)[n] = csound->init_event->p[n+start];
  }
  } else return csoundInitError(csound, "no pfields available\n");
  return OK;
}

int32_t init_instr_ref(CSOUND *csound, IREF_INIT *p) {
  INSTRTXT **instrs = csound->GetInstrumentList(csound);
  if(!p->out->readonly) { // can write to it
    if (UNLIKELY(!(*p->in >= FL(0.0) &&
                   (cs_double)*p->in < (cs_double)csound->engineState.maxinsno + 1)))
      return csound->InitError(csound, "%s",
                              Str("init: instrument number out of range"));
    p->out->instr = instrs[(int32_t)*p->in];
    if (UNLIKELY(p->out->instr == NULL))
      return csound->InitError(csound, "%s",
                              Str("init: instrument is not defined"));
  }
  else csound->Warning(csound, "instr ref var %s is read-only: cannot copy",
                              GetOutputArgName(&(p->h),0));
  return OK;
}

int32_t instr_num(CSOUND *csound, INSTRTXT *instr) {
   int32_t inum = 0;
   INSTRTXT **instrs = csound->GetInstrumentList(csound);
   int32_t max_instrs = csound->engineState.maxinsno + 1;
   while(inum < max_instrs && instrs[inum] != instr) inum++;
   return inum;
}


int32_t get_instr_num(CSOUND *csound, IREF_NUM *p) {
  if (UNLIKELY(p->in->instr == NULL)) {
    return csound->InitError(csound,
      Str("instrnum/nstrnum: instrument reference is not initialized"));
  }
  int32_t result = instr_num(csound, p->in->instr);
  *p->out = result + *p->offs;
  return OK;
}


int32_t get_instr_name(CSOUND *csound, IREF_NUM *p) {
  if (UNLIKELY(p->in->instr == NULL)) {
    return csound->InitError(csound,
      Str("str: instrument reference is not initialized"));
  }
  const char *name = p->in->instr->insname;
  if (name == NULL) name = "";
  STRINGDAT *out = (STRINGDAT *) p->out;
  if(strlen(name) >= out->size) {
    csound->Free(csound, out->data);
    out->data = csoundStrdup(csound, name);
    out->size = strlen(name) + 1;
  } else strNcpy(out->data, name, out->size);
  return OK;
}

int32_t monitora_perf(CSOUND *csound, MONITOR_A *p)
{
  ARRAYDAT *aa = p->tabin;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t nsmps = CS_KSMPS;
  uint32_t end = nsmps - p->h.insdshead->ksmps_no_end;
  uint32_t i, j;
  cs_float *data = aa->data;
  uint32_t len = (uint32_t)p->len;

  memset(data, '\0', len * nsmps * sizeof(cs_float));
  for (j = 0; j < len; j++) {
    for (i = offset; i < end; i++) {
      data[i+j*nsmps] = monitor_spout_sample(csound, p->h.insdshead,
                                             j, i);
    }
  }
  return OK;
}

int32_t monitora_init(CSOUND *csound, MONITOR_A *p)
{
  if(CS_ESR != csound->esr)
    return csound->InitError(csound,
                             "local sampling rate not supported\n");
  ARRAYDAT *aa = p->tabin;
  // should call ensure here but it is a-rate
  aa->dimensions = 1;
  if (aa->sizes) csound->Free(csound, aa->sizes);
  if (aa->data) csound->Free(csound, aa->data);
  aa->sizes = (int32_t*)csound->Malloc(csound, sizeof(int32_t));
  aa->sizes[0] = p->len = csound->GetNchnls(csound);
  aa->data = (cs_float*)
    csound->Malloc(csound, CS_KSMPS*sizeof(cs_float)*p->len);
  aa->arrayMemberSize = CS_KSMPS*sizeof(cs_float);
  return OK;
}
