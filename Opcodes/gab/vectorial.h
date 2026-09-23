/*  Copyright (C) 2002-2004 Gabriel Maldonado

    The gab library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    The gab library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with the gab library; if not, write to the Free Software
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA

   Ported to csound5 by: Andres Cabrera
*/

#ifndef GAB_VECTORIAL_H
#define GAB_VECTORIAL_H
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

/* The follwoing from CsoundAV/newopcodes.h */
typedef struct {
    OPDS    h;
    cs_float   *xndx, *xfn, *kinterp, *ixmode, *outargs[VARGMAX];
} MTABLEI;

typedef struct {
    OPDS    h;
    cs_float   *xndx, *xfn, *kinterp, *ixmode, *outargs[VARGMAX];
    int32_t     nargs;
    cs_float   xbmul;
    cs_float   pfn;
    int64_t len;
    cs_float   *ftable;
} MTABLE;

typedef struct {
    OPDS    h;
    cs_float   *xndx, *xfn, *ixmode, *inargs[VARGMAX];
} MTABLEIW;

typedef struct {
    OPDS    h;
    cs_float   *xndx, *xfn, *ixmode, *inargs[VARGMAX];
    int32_t     nargs;
    cs_float   xbmul;
    cs_float   pfn;
    int64_t len;
    cs_float   *ftable;
} MTABLEW;

typedef struct {
    OPDS    h;
    cs_float   *xndx, *xfn, *inargs[VARGMAX];
} MTABIW;

typedef struct {
    OPDS    h;
    cs_float   *xndx, *xfn, *inargs[VARGMAX];
    int32_t     nargs;
 /* cs_float   xbmul; */
    cs_float   pfn;
    int64_t len;
    cs_float   *ftable;
} MTABW;

typedef struct {
    OPDS    h;
    cs_float   *xndx, *xfn, *outargs[VARGMAX];
} MTABI;

typedef struct {
    OPDS    h;
    cs_float   *xndx, *xfn, *outargs[VARGMAX];
    int32_t     nargs;
 /* cs_float   xbmul; */
    cs_float   pfn;
    int64_t len;
    cs_float   *ftable;
} MTAB;

/* The following from CSoundAV/vectorial.h */
typedef struct {
    OPDS    h;
    cs_float   *ifn, *kval, *kelements, *kdstoffset, *kverbose;
    int64_t    /*elements,*/ len/*, dstoffset*/;
    cs_float   *vector;
} VECTOROP;

typedef struct {
  OPDS    h;
  cs_float   *ifn, *kval, *ielements, *idstoffset;
  int64_t    /*elements,*/ len;
  cs_float   *vector;
} VECTOROPI;

typedef struct {
  OPDS    h;
  cs_float   *ifn1, *ifn2, *kelements, *kdstoffset, *ksrcoffset, *kverbose;
  int32_t     /*elements,*/ len1, len2/*, dstoffset, srcoffset*/;
  cs_float   *vector1, *vector2;
} VECTORSOP;

typedef struct {
  OPDS    h;
  cs_float   *ifn1, *ifn2, *ielements, *idstoffset, *isrcoffset;
  int32_t     /*elements,*/ len1, len2;
  cs_float   *vector1, *vector2;
} VECTORSOPI;

typedef struct {
    OPDS    h;
    cs_float   *ifn, *kmin, *kmax, *ielements;
    int32_t     elements;
    cs_float   *vector;
} VLIMIT;

typedef struct {
    OPDS    h;
    cs_float   *ifn, *krange, *kcps, *ielements, *idstoffset, *iseed, *isize, *ioffset;
    AUXCH   auxch;
    cs_float   *vector;
    int32_t     elements;
    int32_t     offset;
    int64_t    phs;
    cs_float   *num1;
    int64_t   rand;
} VRANDH;

typedef struct {
    OPDS    h;
    cs_float   *ifn, *krange, *kcps, *ielements, *idstoffset, *iseed, *isize, *ioffset;
    AUXCH   auxch;
    cs_float   *vector;
    int32_t     elements;
    int32_t     offset;
    int64_t    phs;
    cs_float   *num1, *num2, *dfdmax;
    int64_t   rand;
} VRANDI;

/*  TSEG definition from H/vpvoc.h */
typedef struct {
    FUNC    *function, *nxtfunction;
    cs_float   d;
    int64_t    cnt;
} TSEG;

typedef struct {
    OPDS    h;
    cs_float   *ioutfunc,*ielements,*argums[VARGMAX];
    TSEG    *cursegp;
    cs_float   *vector;
    int32_t     elements;
    int64_t    nsegs;
    AUXCH   auxch;
} VSEG;

typedef struct {
    OPDS    h;
    cs_float   *ifn, *khtim, *ielements, *ifnInit;
    cs_float   c1, c2, *yt1, *vector, prvhtim;
    int32_t     elements;
    AUXCH   auxch;
} VPORT;

typedef struct {
    OPDS    h;
    cs_float   *ifnOut, *ifnIn, *ifnDel, *ielements, *imaxd, *istod;
    AUXCH   aux;
    cs_float   **buf, *outvec, *invec, *dlyvec;
    int32   *left, maxd;
    int32_t     elements;
} VECDEL;

typedef struct {
    FUNC    *function, *nxtfunction;
    cs_double  d;
} TSEG2;

typedef struct {
    OPDS    h;
    cs_float   *kphase, *ioutfunc, *ielements,*argums[VARGMAX];
    TSEG2   *cursegp;
    cs_float   *vector;
    int32_t     elements;
    int64_t    nsegs;
    AUXCH   auxch;
} VPSEG;

typedef struct {
    OPDS    h;
    cs_float   *kr, *kin, *kdel, *imaxd, *istod, *interp;
    AUXCH   aux;
    int64_t    left, maxd;
} KDEL;

typedef struct {
    OPDS    h;
    cs_float   *ktrig, *kreinit, *ioutFunc, *initStateFunc,
            *iRuleFunc, *ielements, *irulelen, *iradius;
    cs_float   *currLine, *outVec, *initVec, *ruleVec;
    int32_t     elements, NewOld, ruleLen;
    AUXCH   auxch;
} CELLA;

/* from uggab.h for vrandi, vrandh */
/*
#define oneUp31Bit      (double) (4.656612875245796924105750827168e-10)

#define randGab   (cs_float) ((double)     \
    (((csound->holdrand = csound->holdrand * 214013 + 2531011) >> 1)  \
     & 0x7fffffff) * oneUp31Bit)
#define BiRandGab (cs_float) ((double)     \
    (csound->holdrand = csound->holdrand * -214013 + 2531011) * oneUp31Bit)*/

#endif
