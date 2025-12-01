/* array_ops.h: array operators

   Copyright (C) 2011,2012,2025 John ffitch, Steven Yi, Victor Lazzarini

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA
*/
#ifndef __ARRAY_OPS_H__
#define __ARRAY_OPS_H__
#include "csoundCore.h"
#include "arrays.h"

MYFLT MOD(MYFLT a, MYFLT bb);

typedef struct {
  OPDS    h;
  ARRAYDAT  *arrayDat;
} ARRAYDEL;

typedef struct {
  OPDS    h;
  ARRAYDAT* arrayDat;
  MYFLT   *isizes[VARGMAX];
} ARRAYINIT;

typedef struct {
  OPDS    h;
  ARRAYDAT* ans;
  MYFLT   *iargs[VARGMAX];
} TABFILL;

typedef struct {
  OPDS    h;
  ARRAYDAT* ans;
  STRINGDAT *fname;
} TABFILLF;

typedef struct {
  OPDS    h;
  ARRAYDAT* arrayDat;
  void    *value;
  MYFLT   *indexes[VARGMAX];
} ARRAY_SET;

typedef struct {
  OPDS    h;
  MYFLT*   out;
  ARRAYDAT* arrayDat;
  MYFLT   *indexes[VARGMAX];
} ARRAY_GET;

typedef struct {
  OPDS h;
  ARRAYDAT *ans, *left, *right;
} TABARITH;

typedef struct {
  OPDS h;
  ARRAYDAT *ans, *left;
  MYFLT *right;
} TABARITH1;

typedef struct {
  OPDS h;
  ARRAYDAT *ans;
  MYFLT *left;
  ARRAYDAT *right;
} TABARITH2;

typedef struct {
  OPDS h;
  ARRAYDAT *ans;
  ARRAYDAT *right;
} TABARITHIN;

typedef struct {
  OPDS h;
  ARRAYDAT *ans;
  MYFLT *right;
} TABARITHIN1;

typedef struct {
  OPDS h;
  MYFLT  *ans, *pos;
  ARRAYDAT *tab;
} TABQUERY;

typedef struct {
  OPDS h;
  MYFLT  *ans;
  ARRAYDAT *tab;
  MYFLT  *opt;
} TABQUERY1;

typedef struct {
  OPDS h;
  ARRAYDAT *tab;
  MYFLT  *kfn;
} TABCOPY;

typedef struct {
  OPDS h;
  ARRAYDAT *tab;
  MYFLT  *kfn;
  MYFLT  *offset;
} TABCOPY2;


typedef struct {
  OPDS h;
  ARRAYDAT *tab;
  MYFLT  *kmin, *kmax;
  MYFLT  *kstart, *kend;
} TABSCALE;

typedef struct {
  OPDS h;
  ARRAYDAT *tab;
} TABCLEAR;

typedef struct {
  OPDS     h;
  ARRAYDAT *dst;
  ARRAYDAT *src;
  int32_t      len;
} TABCPY;

typedef struct {
  OPDS h;
  ARRAYDAT *tab;
  MYFLT *start, *end, *incr;
  int32_t    len;
} TABGEN;

typedef struct {
  OPDS h;
  ARRAYDAT  *tab;
  MYFLT     *size;
} TRIM;

typedef struct {
  OPDS h;
  ARRAYDAT *tab, *tabin;
  MYFLT    *start, *end, *inc;
  int32_t   len;
} TABSLICE;

typedef struct {
  OPDS h;
  ARRAYDAT *tab, *tabin;
  STRINGDAT *str;
  int32_t    len;
  const OENTRY *opc;
} TABMAP;

typedef struct {
  OPDS h;
  ARRAYDAT *res;
  MYFLT *asig;
} A2ARR;

typedef struct {
  OPDS h;
  MYFLT *asig;
  ARRAYDAT *karr;
} ARR2A;

int32_t array_init(CSOUND *csound, ARRAYINIT *p);
int32_t tabfill(CSOUND *csound, TABFILL *p);
int32_t tabfillf(CSOUND* csound, TABFILLF* p);
int32_t tabsfill(CSOUND *csound, TABFILLF *p);
int32_t array_err(CSOUND* csound, ARRAY_SET *p);
int32_t array_set(CSOUND* csound, ARRAY_SET *p);
int32_t array_get(CSOUND* csound, ARRAY_GET *p);
int32_t tabarithset(CSOUND *csound, TABARITH *p);
int32_t tabarithset1(CSOUND *csound, TABARITH1 *p);
int32_t tabarithset2(CSOUND *csound, TABARITH2 *p);
int32_t tabadd(CSOUND *csound, TABARITH *p);
int32_t tabsub(CSOUND *csound, TABARITH *p);
int32_t tabmult(CSOUND *csound, TABARITH *p);
int32_t tabdiv(CSOUND *csound, TABARITH *p);
int32_t tabrem(CSOUND *csound, TABARITH *p);
int32_t tabpow(CSOUND *csound, TABARITH *p);
int32_t tabaiadd(CSOUND *csound, TABARITH1 *p);
int32_t tabiaadd(CSOUND *csound, TABARITH2 *p);
int32_t addinAA(CSOUND *csound, TABARITHIN1 *p);
int32_t mulinAA(CSOUND *csound, TABARITHIN1 *p);
int32_t tabaddinkk(CSOUND *csound, TABARITHIN *p);
int32_t tabmulinkk(CSOUND *csound, TABARITHIN *p);
int32_t tabaaddin(CSOUND *csound, TABARITHIN *p);
int32_t tabamulin(CSOUND *csound, TABARITHIN *p);
int32_t taba1mulin(CSOUND *csound, TABARITHIN1 *p);
int32_t taba1addin(CSOUND *csound, TABARITHIN1 *p);
int32_t taba1subin(CSOUND *csound, TABARITHIN1 *p);
int32_t taba1divin(CSOUND *csound, TABARITHIN1 *p);

int32_t tabaasubin(CSOUND *csound, TABARITHIN *p);
int32_t tabaadivin(CSOUND *csound, TABARITHIN *p);
int32_t tabarkrddin(CSOUND *csound, TABARITHIN *p);
int32_t tabarkrmulin(CSOUND *csound, TABARITHIN *p);
int32_t tabarkrsbin(CSOUND *csound, TABARITH *p);
int32_t tabarkrdivin(CSOUND *csound, TABARITH *p);
int32_t tabakaddin(CSOUND *csound, TABARITHIN1 *p);
int32_t tabakmulin(CSOUND *csound, TABARITHIN1 *p);
int32_t tabaksubin(CSOUND *csound, TABARITHIN1 *p);
int32_t tabakdivin(CSOUND *csound, TABARITHIN1 *p);
int32_t tabaksub(CSOUND *csound, TABARITH1 *p);
int32_t subinAA(CSOUND *csound, TABARITHIN1 *p);
int32_t divinAA(CSOUND *csound, TABARITHIN1 *p);
int32_t  tabsubinkk(CSOUND *csound, TABARITHIN *p);
int32_t  tabdivinkk(CSOUND *csound, TABARITHIN *p);
int32_t tabaisub(CSOUND *csound, TABARITH1 *p);
int32_t tabiasub(CSOUND *csound, TABARITH2 *p);
int32_t tabaimult(CSOUND *csound, TABARITH1 *p);
int32_t tabiamult(CSOUND *csound, TABARITH2 *p);
int32_t tabaidiv(CSOUND *csound, TABARITH1 *p);
int32_t tabiadiv(CSOUND *csound, TABARITH2 *p);
int32_t tabairem(CSOUND *csound, TABARITH1 *p);
int32_t tabiarem(CSOUND *csound, TABARITH2 *p);
int32_t tabaipow(CSOUND *csound, TABARITH1 *p);
int32_t tabiapow(CSOUND *csound, TABARITH2 *p);
int32_t tabaadd(CSOUND *csound, TABARITH *p);
int32_t tabasub(CSOUND *csound, TABARITH *p);
int32_t tabamul(CSOUND *csound, TABARITH *p);
int32_t tabadiv(CSOUND *csound, TABARITH *p);
int32_t tabkamult(CSOUND *csound, TABARITH2 *p);
int32_t tabakmult(CSOUND *csound, TABARITH1 *p);
int32_t tabkaadd(CSOUND *csound, TABARITH2 *p);
int32_t tabakadd(CSOUND *csound, TABARITH1 *p);
int32_t tabkasub(CSOUND *csound, TABARITH2 *p);
int32_t tabkadiv(CSOUND *csound, TABARITH2 *p);
int32_t tabakdiv(CSOUND *csound, TABARITH1 *p);
int32_t tabarkrem(CSOUND *csound, TABARITH1 *p);
int32_t tabarkpow(CSOUND *csound, TABARITH1 *p);
int32_t tabaardd(CSOUND *csound, TABARITH2 *p);
int32_t tabaarsb(CSOUND *csound, TABARITH2 *p);
int32_t tabaarml(CSOUND *csound, TABARITH2 *p);
int32_t tabaardv(CSOUND *csound, TABARITH2 *p);
int32_t tabaradd(CSOUND *csound, TABARITH1 *p);
int32_t tabarasb(CSOUND *csound, TABARITH1 *p);
int32_t tabaraml(CSOUND *csound, TABARITH1 *p);
int32_t tabaradv(CSOUND *csound, TABARITH1 *p);
int32_t tabkrardd(CSOUND *csound, TABARITH *p);
int32_t tabkrarsb(CSOUND *csound, TABARITH *p);
int32_t tabkrarml(CSOUND *csound, TABARITH *p);
int32_t tabkrardv(CSOUND *csound, TABARITH *p);
int32_t tabkrarmd(CSOUND *csound, TABARITH *p);
int32_t tabarkrdd(CSOUND *csound, TABARITH *p);
int32_t tabarkrsb(CSOUND *csound, TABARITH *p);
int32_t tabarkrml(CSOUND *csound, TABARITH *p);
int32_t tabarkrdv(CSOUND *csound, TABARITH *p);
int32_t tabarkrmd(CSOUND *csound, TABARITH *p);
int32_t tabarkrpw(CSOUND *csound, TABARITH *p);
int32_t tabqset(CSOUND *csound, TABQUERY *p);
int32_t tabqset1(CSOUND *csound, TABQUERY1 *p);
int32_t tabmax(CSOUND *csound, TABQUERY *p);
int32_t tabmax1(CSOUND *csound, TABQUERY *p);
int32_t tabmin(CSOUND *csound, TABQUERY *p);
int32_t tabmin1(CSOUND *csound, TABQUERY *p);
int32_t tabsuma(CSOUND *csound, TABQUERY1 *p);
int32_t tabclearset(CSOUND *csound, TABCLEAR *p);
int32_t tabclear(CSOUND *csound, TABCLEAR *p);
int32_t tabcleark(CSOUND *csound, TABCLEAR *p);
int32_t tabsum(CSOUND *csound, TABQUERY1 *p);
int32_t tabsuma1(CSOUND *csound, TABQUERY1 *p);
int32_t tabsum1(CSOUND *csound, TABQUERY1 *p);
int32_t tabscaleset(CSOUND *csound, TABSCALE *p);
int32_t tabscale(CSOUND *csound, TABSCALE *p);
int32_t tabscale1(CSOUND *csound, TABSCALE *p);
int32_t tabcopy(CSOUND *csound, TABCPY *p);
int32_t tabcopyk_init(CSOUND *csound, TABCPY *p);
int32_t tabcopyk(CSOUND *csound, TABCPY *p);
int32_t tabcopy1(CSOUND *csound, TABCPY *p);
int32_t tabcopy2(CSOUND *csound, TABCPY *p);
int32_t tab2ftab(CSOUND *csound, TABCOPY *p);
int32_t tab2ftabi(CSOUND *csound, TABCOPY *p);
int32_t tab2ftab_offset(CSOUND *csound, TABCOPY2 *p);
int32_t tab2ftab_offset_i(CSOUND *csound, TABCOPY2 *p);
int32_t tabgen(CSOUND *csound, TABGEN *p);
int32_t ftab2tabi(CSOUND *csound, TABCOPY *p);
int32_t ftab2tab(CSOUND *csound, TABCOPY *p);
int32_t trim_i(CSOUND *csound, TRIM *p);
int32_t trim(CSOUND *csound, TRIM *p);
int32_t tabslice(CSOUND *csound, TABSLICE *p);
int32_t tabmap_set(CSOUND *csound, TABMAP *p);
int32_t tabmap_perf(CSOUND *csound, TABMAP *p);
int32_t tablength(CSOUND *csound, TABQUERY1 *p);
int32_t asig2array_init(CSOUND *csound, A2ARR *p);
int32_t asig2array_perf(CSOUND *csound, A2ARR *p);
int32_t array2asig_perf(CSOUND *csound, ARR2A *p);
int32_t arrayass(CSOUND *csound, TABCOPY *p);
int32_t scalarset(CSOUND *csound, TABCOPY *p);
int32_t taninv2_Aa(CSOUND* csound, TABARITH* p);
int32_t taninv2_A(CSOUND* csound, TABARITH* p);
int32_t taninv2_Ai(CSOUND* csound, TABARITH* p);

#define IIARRAY_(opcode,fn)                              \
  int32_t opcode(CSOUND *csound, TABARITH *p);

IIARRAY_(tabaddi,tabadd)
IIARRAY_(tabsubi,tabsub)
IIARRAY_(tabmulti,tabmult)
IIARRAY_(tabdivi,tabdiv)
IIARRAY_(tabremi,tabrem)
IIARRAY_(tabpowi,tabpow)


#define IiARRAY_(opcode,fn)                              \
  int32_t opcode(CSOUND *csound, TABARITH1 *p);

IiARRAY_(tabaiaddi,tabaiadd)
IiARRAY_(tabaisubi,tabaisub)
IiARRAY_(tabaimulti,tabaimult)
IiARRAY_(tabaidivi,tabaidiv)
IiARRAY_(tabairemi,tabairem)
IiARRAY_(tabaipowi,tabaipow)

#define iIARRAY_(opcode,fn)                              \
  int32_t opcode(CSOUND *csound, TABARITH2 *p);

iIARRAY_(tabiaaddi,tabiaadd)
iIARRAY_(tabiasubi,tabiasub)
iIARRAY_(tabiamulti,tabiamult)
iIARRAY_(tabiadivi,tabiadiv)
iIARRAY_(tabiaremi,tabiarem)
iIARRAY_(tabiapowi,tabiapow)


#endif
