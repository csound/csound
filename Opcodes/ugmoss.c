/*
    ugmoss.c:

    Copyright (C) 2001 William 'Pete' Moss

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

                                                        /* ugmoss.c */
#include "stdopcod.h"
#include "ugmoss.h"
#include "aops.h"
#include <math.h>

/******************************************************************************
  all this code was written by william 'pete' moss. <petemoss@petemoss.org>
  no copyright, since it seems silly to copyright algorithms.
  do what you want with the code, and credit me if you get the chance
******************************************************************************/

/* rewritten code for dconv, includes speedup tip from
   Moore: Elements of Computer Music */
static int32_t dconvset(CSOUND *csound, DCONV *p)
{
    FUNC *ftp;
    double len = *p->isize;
    size_t nbytes;

    if (UNLIKELY(!(len >= 1.0)))
      return csound->InitError(csound, "%s", Str("dconv: isize must be at least 1"));
    if (LIKELY((ftp = csound->FTFind(csound,
                                        p->ifn)) != NULL)) {   /* find table */
      p->ftp = ftp;
      /* Limit to the table before converting a possibly large request. */
      p->len = len >= ftp->flen ? ftp->flen : (uint32_t)len;
    }
    else {
      return csound->InitError(csound, "%s", Str("No table for dconv"));
    }
    nbytes = (size_t)p->len * sizeof(MYFLT);
    if (p->sigbuf.auxp == NULL || p->sigbuf.size < nbytes)
      csound->AuxAlloc(csound, nbytes, &p->sigbuf);
    else
      memset(p->sigbuf.auxp, '\0', nbytes);
    p->curp = (MYFLT *)p->sigbuf.auxp;
    return OK;
}

static int32_t dconv(CSOUND *csound, DCONV *p)
{
    IGN(csound);
    uint32_t i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    uint32_t len = p->len;
    MYFLT *ar, *ain, *ftp, *startp, *endp, *curp;
    MYFLT sum;

    ain = p->ain;                               /* read saved values */
    ar = p->ar;
    ftp = p->ftp->ftable;
    startp = (MYFLT *) p->sigbuf.auxp;
    endp = startp + len;
    curp = p->curp;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      *curp = ain[n];                           /* get next input sample */
      i = 1, sum = *curp++ * ftp[0];
      while (curp<endp)
        sum += (*curp++ * ftp[i++]);            /* start the convolution */
      curp = startp;                            /* correct the ptr */
      while (i<len)
        sum += (*curp++ * ftp[i++]);            /* finish the convolution */
      if (curp == startp)
        curp = endp;
      --curp;                                  /* stay within the buffer */
      ar[n] = sum;
    }

    p->curp = curp;                             /* save state */
    return OK;
}

static int32_t and_kk(CSOUND *csound, AOP *p)
{
    IGN(csound);
#ifndef USE_DOUBLE    
    int32_t input1 = MYFLT2LRND(*p->a);
    int32_t input2 = MYFLT2LRND(*p->b);
#else
    int64_t input1 = MYFLT2LRND64(*p->a);
    int64_t input2 = MYFLT2LRND64(*p->b);
#endif    
    *p->r = (MYFLT)(input1 & input2);
    return OK;
}

static int32_t and_aa(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r    = p->r;
    MYFLT *in1  = p->a;
    MYFLT *in2  = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t   n, nsmps = CS_KSMPS;


    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE        
      int32_t  input1, input2;
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
#else
      int64_t  input1, input2;
      input1 = MYFLT2LRND64(in1[n]);
      input2 = MYFLT2LRND64(in2[n]);
#endif      
      r[n] = (MYFLT) (input1 & input2);
    }
    return OK;
}

static int32_t and_ak(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(*p->b);
#else
      int64_t input1 = MYFLT2LRND64(*p->b);
#endif   


    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input2 = MYFLT2LRND(in1[n]);
#else
      int64_t input2 = MYFLT2LRND64(in1[n]);
#endif
      r[n] = (MYFLT)(input1 & input2);
    }
    return OK;
}

static int32_t and_ka(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(*p->a);
#else
      int64_t input1 = MYFLT2LRND64(*p->a);
#endif   

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input2 = MYFLT2LRND(in2[n]);
#else
      int64_t input2 = MYFLT2LRND64(in2[n]);
#endif
      r[n] = (MYFLT)(input1 & input2);
    }
    return OK;
}

static int32_t or_kk(CSOUND *csound, AOP *p)
{
    IGN(csound);
#ifndef USE_DOUBLE    
    int32_t input1 = MYFLT2LRND(*p->a);
    int32_t input2 = MYFLT2LRND(*p->b);
#else
    int64_t input1 = MYFLT2LRND64(*p->a);
    int64_t input2 = MYFLT2LRND64(*p->b);
#endif  
    *p->r = (MYFLT)(input1 | input2);
    return OK;
}

static int32_t or_aa(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE        
      int32_t  input1, input2;
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
#else
      int64_t  input1, input2;
      input1 = MYFLT2LRND64(in1[n]);
      input2 = MYFLT2LRND64(in2[n]);
#endif      
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

static int32_t or_ak(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(*p->b);
#else
      int64_t input1 = MYFLT2LRND64(*p->b);
#endif   

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input2 = MYFLT2LRND(in1[n]);
#else
      int64_t input2 = MYFLT2LRND64(in1[n]);
#endif
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

static int32_t or_ka(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(*p->a);
#else
      int64_t input1 = MYFLT2LRND64(*p->a);
#endif   

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input2 = MYFLT2LRND(in2[n]);
#else
      int64_t input2 = MYFLT2LRND64(in2[n]);
#endif
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

static int32_t xor_kk(CSOUND *csound, AOP *p)
{
    IGN(csound);
#ifndef USE_DOUBLE    
    int32_t input1 = MYFLT2LRND(*p->a);
    int32_t input2 = MYFLT2LRND(*p->b);
#else
    int64_t input1 = MYFLT2LRND64(*p->a);
    int64_t input2 = MYFLT2LRND64(*p->b);
#endif  
    *p->r = (MYFLT)(input1 ^ input2);
    return OK;
}

static int32_t xor_aa(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = 0; n < nsmps; n++) {
#ifndef USE_DOUBLE        
      int32_t  input1, input2;
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
#else
      int64_t  input1, input2;
      input1 = MYFLT2LRND64(in1[n]);
      input2 = MYFLT2LRND64(in2[n]);
#endif  
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

static int32_t xor_ak(CSOUND *csound, AOP *p)
{
     IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(*p->b);
#else
      int64_t input1 = MYFLT2LRND64(*p->b);
#endif   

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input2 = MYFLT2LRND(in1[n]);
#else
      int64_t input2 = MYFLT2LRND64(in1[n]);
#endif
      r[n] = (MYFLT)(input1 ^ input2);
    }
   return OK;
}

static int32_t xor_ka(CSOUND *csound, AOP *p)
{
     IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(*p->a);
#else
      int64_t input1 = MYFLT2LRND64(*p->a);
#endif   

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input2 = MYFLT2LRND(in2[n]);
#else
      int64_t input2 = MYFLT2LRND64(in2[n]);
#endif
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

/* Use unsigned left shifts so negative values and discarded high bits do not
   cause signed overflow. Mask counts to the word width, preserving the usual
   machine-shift behavior. Right shifts retain their signed arithmetic result. */
#ifdef USE_DOUBLE
typedef int64_t BITSHIFT_INT;
typedef uint64_t BITSHIFT_UINT;
#define BITSHIFT_MASK 63U
#else
typedef int32_t BITSHIFT_INT;
typedef uint32_t BITSHIFT_UINT;
#define BITSHIFT_MASK 31U
#endif
#define BITSHIFT_LEFT(value, count) \
    ((BITSHIFT_INT)((BITSHIFT_UINT)(value) << \
                   ((BITSHIFT_UINT)(count) & BITSHIFT_MASK)))
#define BITSHIFT_RIGHT(value, count) \
    ((value) >> ((BITSHIFT_UINT)(count) & BITSHIFT_MASK))

static int32_t shift_left_kk(CSOUND *csound, AOP *p)
{
    IGN(csound);
#ifndef USE_DOUBLE    
    int32_t input1 = MYFLT2LRND(*p->a);
    int32_t input2 = MYFLT2LRND(*p->b);
#else
    int64_t input1 = MYFLT2LRND64(*p->a);
    int64_t input2 = MYFLT2LRND64(*p->b);
#endif 
    *p->r = (MYFLT) BITSHIFT_LEFT(input1, input2);
    return OK;
}

static int32_t shift_left_aa(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE        
      int32_t  input1, input2;
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
#else
      int64_t  input1, input2;
      input1 = MYFLT2LRND64(in1[n]);
      input2 = MYFLT2LRND64(in2[n]);
#endif  
      r[n] = (MYFLT)BITSHIFT_LEFT(input1, input2);
    }
    return OK;
}

static int32_t shift_left_ak(CSOUND *csound, AOP *p)
{
     IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
#ifndef USE_DOUBLE      
      int32_t input2 = MYFLT2LRND(*p->b);
#else
      int64_t input2 = MYFLT2LRND64(*p->b);
#endif   

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(in1[n]);
#else
      int64_t input1 = MYFLT2LRND64(in1[n]);
#endif
      r[n] = (MYFLT)BITSHIFT_LEFT(input1, input2);
    }
    return OK;
}

static int32_t shift_left_ka(CSOUND *csound, AOP *p)
{
     IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(*p->a);
#else
      int64_t input1 = MYFLT2LRND64(*p->a);
#endif   

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input2 = MYFLT2LRND(in2[n]);
#else
      int64_t input2 = MYFLT2LRND64(in2[n]);
#endif
      r[n] = (MYFLT)BITSHIFT_LEFT(input1, input2);
    }
    return OK;
}

static int32_t shift_right_kk(CSOUND *csound, AOP *p)
{
    IGN(csound);
#ifndef USE_DOUBLE    
    int32_t input1 = MYFLT2LRND(*p->a);
    int32_t input2 = MYFLT2LRND(*p->b);
#else
    int64_t input1 = MYFLT2LRND64(*p->a);
    int64_t input2 = MYFLT2LRND64(*p->b);
#endif 
    *p->r = (MYFLT) BITSHIFT_RIGHT(input1, input2);
    return OK;
}

static int32_t shift_right_aa(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE        
      int32_t  input1, input2;
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
#else
      int64_t  input1, input2;
      input1 = MYFLT2LRND64(in1[n]);
      input2 = MYFLT2LRND64(in2[n]);
#endif  
      p->r[n] = (MYFLT) BITSHIFT_RIGHT(input1, input2);
    }
    return OK;
}

static int32_t shift_right_ak(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
#ifndef USE_DOUBLE      
      int32_t input2 = MYFLT2LRND(*p->b);
#else
      int64_t input2 = MYFLT2LRND64(*p->b);
#endif   

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(in1[n]);
#else
      int64_t input1 = MYFLT2LRND64(in1[n]);
#endif
      p->r[n] = (MYFLT) BITSHIFT_RIGHT(input1, input2);
    }
    return OK;
}

static int32_t shift_right_ka(CSOUND *csound, AOP *p)
{
     IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(*p->a);
#else
      int64_t input1 = MYFLT2LRND64(*p->a);
#endif   

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input2 = MYFLT2LRND(in2[n]);
#else
      int64_t input2 = MYFLT2LRND64(in2[n]);
#endif
      p->r[n] = (MYFLT) BITSHIFT_RIGHT(input1, input2);
    }
    return OK;
}

static int32_t not_k(CSOUND *csound, AOP *p)    /* Added for completeness by JPff */
{
     IGN(csound);
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(*p->a);
#else
      int64_t input1 = MYFLT2LRND64(*p->a);
#endif
    *p->r = (MYFLT)(~input1);
    return OK;
}

static int32_t not_a(CSOUND *csound, AOP *p)
{
    IGN(csound);
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
     for (n = offset; n < nsmps; n++) {
#ifndef USE_DOUBLE      
      int32_t input1 = MYFLT2LRND(in1[n]);
#else
      int64_t input1 = MYFLT2LRND64(in1[n]);
#endif
      r[n] = (MYFLT)(~input1);
    }
    return OK;
}

/* all the vcomb and valpass stuff adapted from comb() and alpass()
   with additional insight from me (petemoss@petemoss.org)  */

static int32_t vcombset(CSOUND *csound, VCOMB *p)
{
    MYFLT samples = *p->insmps != FL(0) ? *p->imaxlpt : *p->imaxlpt * CS_ESR;
    if (UNLIKELY(!((double) samples >= 1.0 &&
                   (double) samples <= INT32_MAX &&
                   (double) samples <= SIZE_MAX / sizeof(MYFLT)))) {
      return csound->InitError(csound, "%s", Str("illegal loop time"));
    }
    uint32_t lpsiz = (uint32_t) samples;
    size_t nbytes = (size_t) lpsiz * sizeof(MYFLT);
    if (p->auxch.auxp == NULL || nbytes != p->auxch.size) {
      csound->AuxAlloc(csound, nbytes, &p->auxch);
      p->pntr = (MYFLT *) p->auxch.auxp;
      if (UNLIKELY(p->pntr==NULL)) {
        return csound->InitError(csound, "%s", Str("could not allocate memory"));
      }
    }
    else if (!(*p->istor)) {
      p->pntr = (MYFLT *) p->auxch.auxp;
      memset(p->pntr, 0, nbytes);
    }
    p->rvt = FL(0.0);
    p->lpt = 0;
    p->g   = FL(0.0);
    p->lpta = IS_ASIG_ARG(p->xlpt) ? 1 : 0;
    p->maxlpt = lpsiz;
    return OK;
}

/* Feedback requires at least one sample of delay. Bound before conversion. */
#define VCOMB_DELAY(value, scale, maximum, result) do {                   \
    MYFLT delaySamples = (value) * (scale);                              \
    (result) = !(delaySamples >= FL(1)) ? 1 :                             \
      ((double) delaySamples >= (maximum) ? (maximum) :                   \
       (uint32_t) delaySamples);                                        \
  } while (0)

/* Wrap an integer index without first forming a pointer before the array. */
#define VCOMB_READ(write, start, size, delay, read) do {                   \
    uint32_t writeIndex = (uint32_t) ((write) - (start));                 \
    (read) = (start) + (writeIndex >= (delay) ? writeIndex - (delay) :     \
                       (size) - ((delay) - writeIndex));                \
  } while (0)

static int32_t vcomb(CSOUND *csound, VCOMB *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    uint32_t      xlpt, maxlpt = p->maxlpt;
    MYFLT       *ar, *asig, *rp, *endp, *startp, *wp, *lpt;
    MYFLT       g = p->g;
    MYFLT sampleScale = *p->insmps != 0 ? FL(1.0) : CS_ESR;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;
    ar = p->ar;
    asig = p->asig;
    endp = (MYFLT *) p->auxch.endp;
    startp = (MYFLT *) p->auxch.auxp;
    wp = p->pntr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->lpta) {                               /* if xlpt is a-rate */
      lpt = p->xlpt + offset;
      for (n=offset; n<nsmps; n++) {
        VCOMB_DELAY(*lpt, sampleScale, maxlpt, xlpt);
        VCOMB_READ(wp, startp, maxlpt, xlpt, rp);
        if ((p->rvt != *p->krvt) || (p->lpt != xlpt)) {
          p->rvt = *p->krvt, p->lpt = xlpt;
          g = p->g = p->rvt == FL(0) ? FL(0) :
            POWER(FL(0.001), (p->lpt * CS_ONEDSR / p->rvt));
        }
        lpt++;
        MYFLT output = *rp++;
        *wp++ = (output * g) + asig[n];
        ar[n] = output;
        if (wp >= endp) wp = startp;
      }
    }
    else {                                       /* if xlpt is k-rate */
      VCOMB_DELAY(*p->xlpt, sampleScale, maxlpt, xlpt);
      VCOMB_READ(wp, startp, maxlpt, xlpt, rp);
      if ((p->rvt != *p->krvt) || (p->lpt != xlpt)) {
        p->rvt = *p->krvt, p->lpt = xlpt;
        g = p->g = p->rvt == FL(0) ? FL(0) :
            POWER(FL(0.001), (p->lpt * CS_ONEDSR / p->rvt));
      }
      for (n=offset; n<nsmps; n++) {
        MYFLT output = *rp++;
        *wp++ = (output * g) + asig[n];
        ar[n] = output;
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      }
    }
    p->pntr = wp;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("vcomb: not initialised"));
}

static int32_t valpass(CSOUND *csound, VCOMB *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    uint32_t xlpt, maxlpt = p->maxlpt;
    MYFLT       *ar, *asig, *rp, *startp, *endp, *wp, *lpt;
    MYFLT       y, z, g = p->g;
    MYFLT sampleScale = *p->insmps != 0 ? FL(1.0) : CS_ESR;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;
    ar = p->ar;
    asig = p->asig;
    endp = (MYFLT *) p->auxch.endp;
    startp = (MYFLT *) p->auxch.auxp;
    wp = p->pntr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->lpta) {                                      /* if xlpt is a-rate */
      lpt = p->xlpt;
      for (n=offset; n<nsmps; n++) {
        VCOMB_DELAY(lpt[n], sampleScale, maxlpt, xlpt);
        VCOMB_READ(wp, startp, maxlpt, xlpt, rp);
        if ((p->rvt != *p->krvt) || (p->lpt != xlpt)) {
          p->rvt = *p->krvt, p->lpt = xlpt;
          g = p->g = p->rvt == FL(0) ? FL(0) :
            POWER(FL(0.001), (p->lpt * CS_ONEDSR / p->rvt));
        }
        y = *rp++;
        *wp++ = z = y * g + asig[n];
        ar[n] = y - g * z;
        if (wp >= endp) wp = startp;
      }
    }
    else {                                              /* if xlpt is k-rate */
      VCOMB_DELAY(*p->xlpt, sampleScale, maxlpt, xlpt);
      VCOMB_READ(wp, startp, maxlpt, xlpt, rp);
      if ((p->rvt != *p->krvt) || (p->lpt != xlpt)) {
        p->rvt = *p->krvt, p->lpt = xlpt;
        g = p->g = p->rvt == FL(0) ? FL(0) :
            POWER(FL(0.001), (p->lpt * CS_ONEDSR / p->rvt));
      }
      for (n=offset; n<nsmps; n++) {
        y = *rp++;
        *wp++ = z = y * g + asig[n];
        ar[n] = y - g * z;
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      }
    }
    p->pntr = wp;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("valpass: not initialised"));
}

#undef VCOMB_DELAY
#undef VCOMB_READ

static int32_t ftmorfset(CSOUND *csound, FTMORF *p)
{
    FUNC *ftp;
    int32_t j = 0;
    uint32_t len;
    /* make sure resfn exists and set it up */
    if (LIKELY((ftp = csound->FTFind(csound, p->iresfn)) != NULL)) {
      p->resfn = ftp, len = p->resfn->flen;
    }
    else {
      return csound->InitError(csound, "%s", Str("iresfn for ftmorf does not exist"));
    }
    /* make sure ftfn exists and set it up */
    if (LIKELY((ftp = csound->FTFind(csound, p->iftfn)) != NULL)) {
      p->ftfn = ftp;
    }
    else {
      return csound->InitError(csound, "%s", Str("iftfn for ftmorf does not exist"));
    }

    do {                /* make sure tables in ftfn exist and are right size*/
      if (LIKELY((ftp = csound->FTFind(csound, p->ftfn->ftable + j)) != NULL)) {
        if (UNLIKELY((uint32_t)ftp->flen != len)) {
          return csound->InitError(csound,
                                   "%s", Str("table in iftfn for ftmorf wrong size"));
        }
      }
      else {
        return csound->InitError(csound, "%s", Str("table in iftfn for ftmorf "
                                             "does not exist"));
      }
    } while (++j < (int32_t)p->ftfn->flen);

    p->len = len;
    p->ftndx = -FL(1.0);
    return OK;
}

static int32_t ftmorf(CSOUND *csound, FTMORF *p)
{
    uint32_t j, i;
    double ndx = *p->kftndx;
    MYFLT f;
    FUNC *ftp1, *ftp2;

    /* Clamp locally: the input can also be used by other opcodes. */
    if (ndx < 0.0)
      ndx = 0.0;
    else if (ndx > (double)p->ftfn->flen - 1.0)
      ndx = (double)p->ftfn->flen - 1.0;
    else if (UNLIKELY(!(ndx >= 0.0)))
      return csound->PerfError(csound, &(p->h),
                              "%s", Str("ftmorf: invalid index"));
    if (p->ftndx != ndx) {
      i = (uint32_t)ndx;
      f = (MYFLT)(ndx - i);
      ftp1 = csound->FTFind(csound, &p->ftfn->ftable[i]);
      ftp2 = f != FL(0.0) ?
        csound->FTFind(csound, &p->ftfn->ftable[i + 1]) : ftp1;
      /* The table list may have changed since initialisation. */
      if (UNLIKELY(ftp1 == NULL || ftp2 == NULL))
        return csound->PerfError(csound, &(p->h),
                                "%s", Str("ftmorf: source table does not exist"));
      if (UNLIKELY(ftp1->flen != p->len || ftp2->flen != p->len))
        return csound->PerfError(csound, &(p->h),
                                "%s", Str("ftmorf: source table has wrong size"));
      /* Interpolating readers also need the source tables' guard points. */
      for (j = 0; j <= p->len; j++)
        p->resfn->ftable[j] = ftp1->ftable[j] * (FL(1.0) - f) +
                             ftp2->ftable[j] * f;
      p->ftndx = (MYFLT)ndx;
    }
    return OK;
}

/* end of ugmoss.c */
#define S(x)    sizeof(x)

static OENTRY localops[] =
  {
   { "dconv",  S(DCONV), TR, "a", "aii",   (SUBR)dconvset, (SUBR)dconv },
   { "vcomb", S(VCOMB),  0, "a", "akxioo", (SUBR)vcombset, (SUBR)vcomb   },
   { "valpass", S(VCOMB),0, "a", "akxioo", (SUBR)vcombset, (SUBR)valpass },
   { "ftmorf", S(FTMORF),TR, "",  "kii",  (SUBR)ftmorfset,  (SUBR)ftmorf,    },
   { "##and.ii",  S(AOP),  0, "i", "ii",   (SUBR)and_kk                  },
   { "##and.kk",  S(AOP),  0, "k", "kk",   NULL,   (SUBR)and_kk          },
   { "##and.ka",  S(AOP),  0, "a", "ka",   NULL,   (SUBR)and_ka  },
   { "##and.ak",  S(AOP),  0, "a", "ak",   NULL,   (SUBR)and_ak  },
   { "##and.aa",  S(AOP),  0, "a", "aa",   NULL,   (SUBR)and_aa  },
   { "##or.ii",   S(AOP),  0, "i", "ii",   (SUBR)or_kk                   },
   { "##or.kk",   S(AOP),  0, "k", "kk",   NULL,   (SUBR)or_kk           },
   { "##or.ka",   S(AOP),  0, "a", "ka",   NULL,   (SUBR)or_ka   },
   { "##or.ak",   S(AOP),  0, "a", "ak",   NULL,   (SUBR)or_ak   },
   { "##or.aa",   S(AOP),  0, "a", "aa",   NULL,   (SUBR)or_aa   },
   { "##xor.ii",  S(AOP),  0, "i", "ii",   (SUBR)xor_kk                  },
   { "##xor.kk",  S(AOP),  0, "k", "kk",   NULL,   (SUBR)xor_kk          },
   { "##xor.ka",  S(AOP),  0, "a", "ka",   NULL,   (SUBR)xor_ka  },
   { "##xor.ak",  S(AOP),  0, "a", "ak",   NULL,   (SUBR)xor_ak  },
   { "##xor.aa",  S(AOP),  0, "a", "aa",   NULL,   (SUBR)xor_aa  },
   { "##not.i",   S(AOP),  0, "i", "i",    (SUBR)not_k                   },
   { "##not.k",   S(AOP),  0, "k", "k",    NULL,   (SUBR)not_k           },
   { "##not.a",   S(AOP),  0, "a", "a",    NULL,   (SUBR)not_a   },
   { "##shl.ii",  S(AOP),  0, "i", "ii",   (SUBR) shift_left_kk          },
   { "##shl.kk",  S(AOP),  0, "k", "kk",   NULL, (SUBR) shift_left_kk    },
   { "##shl.ka", S(AOP), 0, "a", "ka", NULL, (SUBR) shift_left_ka  },
   { "##shl.ak", S(AOP), 0, "a", "ak", NULL, (SUBR) shift_left_ak  },
   { "##shl.aa", S(AOP), 0, "a", "aa", NULL, (SUBR) shift_left_aa  },
   { "##shr.ii",  S(AOP),  0, "i", "ii",   (SUBR) shift_right_kk         },
   { "##shr.kk",  S(AOP),  0, "k", "kk",   NULL, (SUBR) shift_right_kk   },
   { "##shr.ka", S(AOP), 0, "a", "ka", NULL, (SUBR) shift_right_ka },
   { "##shr.ak", S(AOP), 0, "a", "ak", NULL, (SUBR) shift_right_ak },
   { "##shr.aa", S(AOP), 0, "a", "aa", NULL, (SUBR) shift_right_aa }
};

int32_t ugmoss_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
