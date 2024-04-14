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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA

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
    MYFLT   *xndx, *xfn, *kinterp, *ixmode, *outargs[VARGMAX];
} MTABLEI;

typedef struct {
    OPDS    h;
    MYFLT   *xndx, *xfn, *kinterp, *ixmode, *outargs[VARGMAX];
    int32_t     nargs;
    MYFLT   xbmul;
    int64_t    pfn, len;
    MYFLT   *ftable;
} MTABLE;

typedef struct {
    OPDS    h;
    MYFLT   *xndx, *xfn, *ixmode, *inargs[VARGMAX];
} MTABLEIW;

typedef struct {
    OPDS    h;
    MYFLT   *xndx, *xfn, *ixmode, *inargs[VARGMAX];
    int32_t     nargs;
    MYFLT   xbmul;
    int64_t    pfn, len;
    MYFLT   *ftable;
} MTABLEW;

typedef struct {
    OPDS    h;
    MYFLT   *xndx, *xfn, *inargs[VARGMAX];
} MTABIW;

typedef struct {
    OPDS    h;
    MYFLT   *xndx, *xfn, *inargs[VARGMAX];
    int32_t     nargs;
 /* MYFLT   xbmul; */
    int64_t    pfn, len;
    MYFLT   *ftable;
} MTABW;

typedef struct {
    OPDS    h;
    MYFLT   *xndx, *xfn, *outargs[VARGMAX];
} MTABI;

typedef struct {
    OPDS    h;
    MYFLT   *xndx, *xfn, *outargs[VARGMAX];
    int32_t     nargs;
 /* MYFLT   xbmul; */
    int64_t    pfn, len;
    MYFLT   *ftable;
} MTAB;

/* The following from CSoundAV/vectorial.h */
typedef struct {
    OPDS    h;
    MYFLT   *ifn, *kval, *kelements, *kdstoffset, *kverbose;
    int64_t    /*elements,*/ len/*, dstoffset*/;
    MYFLT   *vector;
} VECTOROP;

typedef struct {
  OPDS    h;
  MYFLT   *ifn, *kval, *ielements, *idstoffset;
  int64_t    /*elements,*/ len;
  MYFLT   *vector;
} VECTOROPI;

typedef struct {
  OPDS    h;
  MYFLT   *ifn1, *ifn2, *kelements, *kdstoffset, *ksrcoffset, *kverbose;
  int32_t     /*elements,*/ len1, len2/*, dstoffset, srcoffset*/;
  MYFLT   *vector1, *vector2;
} VECTORSOP;

typedef struct {
  OPDS    h;
  MYFLT   *ifn1, *ifn2, *ielements, *idstoffset, *isrcoffset;
  int32_t     /*elements,*/ len1, len2;
  MYFLT   *vector1, *vector2;
} VECTORSOPI;

typedef struct {
    OPDS    h;
    MYFLT   *ifn, *kmin, *kmax, *ielements;
    int32_t     elements;
    MYFLT   *vector;
} VLIMIT;

typedef struct {
    OPDS    h;
    MYFLT   *ifn, *krange, *kcps, *ielements, *idstoffset, *iseed, *isize, *ioffset;
    AUXCH   auxch;
    MYFLT   *vector;
    int32_t     elements;
    int32_t     offset;
    int64_t    phs;
    MYFLT   *num1;
    int64_t   rand;
} VRANDH;

typedef struct {
    OPDS    h;
    MYFLT   *ifn, *krange, *kcps, *ielements, *idstoffset, *iseed, *isize, *ioffset;
    AUXCH   auxch;
    MYFLT   *vector;
    int32_t     elements;
    int32_t     offset;
    int64_t    phs;
    MYFLT   *num1, *num2, *dfdmax;
    int64_t   rand;
} VRANDI;

/*  TSEG definition from H/vpvoc.h */
typedef struct {
    FUNC    *function, *nxtfunction;
    MYFLT   d;
    int64_t    cnt;
} TSEG;

typedef struct {
    OPDS    h;
    MYFLT   *ioutfunc,*ielements,*argums[VARGMAX];
    TSEG    *cursegp;
    MYFLT   *vector;
    int32_t     elements;
    int64_t    nsegs;
    AUXCH   auxch;
} VSEG;

typedef struct {
    OPDS    h;
    MYFLT   *ifn, *khtim, *ielements, *ifnInit;
    MYFLT   c1, c2, *yt1, *vector, prvhtim;
    int32_t     elements;
    AUXCH   auxch;
} VPORT;

typedef struct {
    OPDS    h;
    MYFLT   *ifnOut, *ifnIn, *ifnDel, *ielements, *imaxd, *istod;
    AUXCH   aux;
    MYFLT   **buf, *outvec, *invec, *dlyvec;
    int32   *left, maxd;
    int32_t     elements;
} VECDEL;

typedef struct {
    FUNC    *function, *nxtfunction;
    double  d;
} TSEG2;

typedef struct {
    OPDS    h;
    MYFLT   *kphase, *ioutfunc, *ielements,*argums[VARGMAX];
    TSEG2   *cursegp;
    MYFLT   *vector;
    int32_t     elements;
    int64_t    nsegs;
    AUXCH   auxch;
} VPSEG;

typedef struct {
    OPDS    h;
    MYFLT   *kr, *kin, *kdel, *imaxd, *istod, *interp;
    AUXCH   aux;
    int64_t    left, maxd;
} KDEL;

typedef struct {
    OPDS    h;
    MYFLT   *ktrig, *kreinit, *ioutFunc, *initStateFunc,
            *iRuleFunc, *ielements, *irulelen, *iradius;
    MYFLT   *currLine, *outVec, *initVec, *ruleVec;
    int32_t     elements, NewOld, ruleLen;
    AUXCH   auxch;
} CELLA;

/* from uggab.h for vrandi, vrandh */
/*
#define oneUp31Bit      (double) (4.656612875245796924105750827168e-10)

#define randGab   (MYFLT) ((double)     \
    (((csound->holdrand = csound->holdrand * 214013 + 2531011) >> 1)  \
     & 0x7fffffff) * oneUp31Bit)
#define BiRandGab (MYFLT) ((double)     \
    (csound->holdrand = csound->holdrand * -214013 + 2531011) * oneUp31Bit)*/

#endif

