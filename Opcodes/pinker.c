/*
    pink.c

    New Shade of Pink
    (c) 2014 Stefan Stenzel
    stefan at waldorfmusic.de
    Terms of use:
    Use for any purpose. If used in a commercial product, you should give me one.
    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    some temporary bias required for hack on floats

    Implemented in Csound by John ffitch
    Copyright (C) 2014 Stefan Stenzel, John ffitch
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

#include "csoundCore.h"       /*                              PINKER.C         */

typedef struct {
  OPDS h;
  MYFLT *ar;
  int   inc;
  int   dec;
  int32 accu;
  int32 lfsr;
  unsigned char cnt;
  int offset;
} PINKER;

#define PINK_BIAS   440.0f

static int instance_cnt = 0;

// Let preprocessor and compiler calculate two lookup tables for 12-tap
// FIR filter with these coefficients:
// 1.190566,0.162580, 0.002208,0.025475,-0.001522,0.007322,
// 0.001774,0.004529,-0.001561,0.000776,-0.000486,0.002017

#define F(cf,m,shift)   (0.0625f*cf*(2*((m)>>shift&1)-1))

#define FA(n)   F(1.190566,n,0)+F(0.162580,n,1)+F(0.002208,n,2)+ \
                F(0.025475,n,3)+F(-0.001522,n,4)+F(0.007322,n,5)-PINK_BIAS
#define FB(n)   F(0.001774,n,0)+F(0.004529,n,1)+F(-0.001561,n,2)+ \
                F(0.000776,n,3)+F(-0.000486,n,4)+F(0.002017,n,5)

#define FA8(n)  FA(n),FA(n+1),FA(n+2),FA(n+3),FA(n+4),FA(n+5),FA(n+6),FA(n+7)
#define FB8(n)  FB(n),FB(n+1),FB(n+2),FB(n+3),FB(n+4),FB(n+5),FB(n+6),FB(n+7)

 // 1st FIR lookup table
static const float pfira[64] = {FA8(0) ,FA8(8), FA8(16),FA8(24),
                                FA8(32),FA8(40),FA8(48),FA8(56)};
 // 2nd FIR lookup table
static const float pfirb[64] = {FB8(0), FB8(8), FB8(16),FB8(24),
                                FB8(32),FB8(40),FB8(48),FB8(56)};
// bitreversed lookup table
#define PM16(n) n,0x80,0x40,0x80,0x20,0x80,0x40,0x80, \
                0x10,0x80,0x40,0x80,0x20,0x80,0x40,0x80

static const unsigned char pnmask[256] =
{
    PM16(0x00),PM16(0x08),PM16(0x04),PM16(0x08),
    PM16(0x02),PM16(0x08),PM16(0x04),PM16(0x08),
    PM16(0x01),PM16(0x08),PM16(0x04),PM16(0x08),
    PM16(0x02),PM16(0x08),PM16(0x04),PM16(0x08)
};

static const int ind[] = {     0, 0x0800, 0x0400, 0x0800,
                          0x0200, 0x0800, 0x0400, 0x0800,
                          0x0100, 0x0800, 0x0400, 0x0800,
                          0x0200, 0x0800, 0x0400, 0x0800};

static int pink_perf(CSOUND* csound, PINKER *p) /* generate samples of pink noise */
{
    int inc    =   p->inc;
    int dec    =   p->dec;
    int32 accu =   p->accu;
    int32 lfsr   =   p->lfsr;
    int cnt    =   p->cnt;
    int bit;
    int n, nn, nsmps = csound->ksmps;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int mask;
    float yy;
    MYFLT *out = p->ar;
    int loffset = p->offset;
    if (UNLIKELY(early)) {
        nsmps -= early;
      }
    for (n=offset, nn=loffset; n<nsmps; n++, nn++) {
      int k = nn%16;   /* algorithm is in 16 sample chunks */

/* bit   = lfsr >> 31;        dec &= ~0x0800; */
/* lfsr <<= 1;                dec |= inc & 0x0800; */
/* inc ^= bit  & 0x0800;      accu -= dec; */
/* lfsr ^= bit & 0x46000001;  accu += inc; */
/* leftOut[i+1] = accu * pscale + pflta[lfsr & 0x3F] + pfltb[lfsr >> 6 & 0x3F]; */


      if (k==0) mask = pnmask[cnt++];
      else mask = ind[k];
      bit = lfsr >> 31;            /* spill random to all bits        */
      dec &= ~mask;                /* blank old decrement bit         */
      lfsr <<= 1;                  /* shift lfsr                      */
      dec |= inc & mask;           /* copy increment to decrement bit */
      inc ^= bit & mask;           /* new random bit                  */
      *((int *)(&yy)) = accu;      /* save biased value as float      */
      //printf("yy = %f ", yy);
      accu += inc - dec;           /* integrate                       */
      lfsr ^= bit & 0x46000001;    /* update lfsr                     */
      yy += pfira[lfsr & 0x3F] /* add 1st half precalculated FIR  */
                + pfirb[lfsr >>6 & 0x3F];  /* add 2nd half, also corrects bias */
      //printf("out = %f a,b = %f,%f mask = %.8x dec,inc = %x,%x acc = %x\n",
      //       yy, pfira[lfsr & 0x3F], pfirb[lfsr >>6 & 0x3F],
      //       mask, dec, inc, accu);
      out[n] = yy*csound->e0dbfs;
    /* PINK(mask);   PINK(0x0800); PINK(0x0400); PINK(0x0800); */
    /* PINK(0x0200); PINK(0x0800); PINK(0x0400); PINK(0x0800); */
    /* PINK(0x0100); PINK(0x0800); PINK(0x0400); PINK(0x0800); */
    /* PINK(0x0200); PINK(0x0800); PINK(0x0400); PINK(0x0800); */
    }
    p->inc    =   inc;                        // write back variables
    p->dec    =   dec;
    p->accu   =   accu;
    p->lfsr   =   lfsr;
    p->cnt    =   cnt;
    p->offset =   nn%16;
    return OK;
};

static int pink_init(CSOUND *csund, PINKER *p)      // constructor
{
    p->lfsr  = 0x5EED41F5 + instance_cnt++;   // seed for lfsr,
                                              // decorrelate multiple instances
    *((float*)(&p->accu))  = PINK_BIAS;       // init float hack
    p->cnt = 0;                               // counter from zero
    p->inc   = 0x0CCC;                        // balance initial states to avoid DC
    p->dec   = 0x0CCC;
    p->offset =   0;
    return OK;
}

static OENTRY pinker_localops[] =
{
  { "pinker", sizeof(PINKER),0,5, "a", "", (SUBR)pink_init, NULL, (SUBR)pink_perf }
};

LINKAGE_BUILTIN(pinker_localops)
