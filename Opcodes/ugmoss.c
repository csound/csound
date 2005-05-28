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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

                                                        /* ugmoss.c */
#include "csdl.h"
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
int dconvset(ENVIRON *csound, DCONV *p)
{
    FUNC *ftp;

    p->len = (int)*p->isize;
    if ((ftp = csound->FTFind(csound, p->ifn)) != NULL) {   /* find table */
      p->ftp = ftp;
      if ((unsigned)ftp->flen < p->len)
        p->len = ftp->flen; /* correct len if flen shorter */
    }
    else {
      return csound->InitError(csound, Str("No table for dconv"));
    }
    if (p->sigbuf.auxp == NULL || p->sigbuf.size < (int)(p->len*sizeof(MYFLT)))
      csound->AuxAlloc(csound, p->len*sizeof(MYFLT), &p->sigbuf);
    p->curp = (MYFLT *)p->sigbuf.auxp;
    return OK;
}

int dconv(ENVIRON *csound, DCONV *p)
{
    long i = 0;
    int n, nsmps = csound->ksmps;
    long len = p->len;
    MYFLT *ar, *ain, *ftp, *startp, *endp, *curp;
    MYFLT sum;

    ain = p->ain;                               /* read saved values */
    ar = p->ar;
    ftp = p->ftp->ftable;
    startp = (MYFLT *) p->sigbuf.auxp;
    endp = startp + len;
    curp = p->curp;

    for (n=0; n<nsmps; n++) {
      *curp = ain[n];                           /* get next input sample */
      i = 1, sum = *curp++ * *ftp;
      while (curp<endp)
        sum += (*curp++ * *(ftp + i++));        /* start the convolution */
      curp = startp;                            /* correct the ptr */
      while (i<len)
        sum += (*curp++ * *(ftp + i++));        /* finish the convolution */
      if (--curp < startp)
        curp += len;                            /* correct for last curp++ */
      ar[n] = sum;
    }

    p->curp = curp;                             /* save state */
    return OK;
}

int and_kk(ENVIRON *csound, AOP *p)
{
    long input1 = MYFLT2LRND(*p->a);
    long input2 = MYFLT2LRND(*p->b);
    *p->r = (MYFLT)(input1 & input2);
    return OK;
}

int and_aa(ENVIRON *csound, AOP *p)
{
    MYFLT *r    = p->r;
    MYFLT *in1  = p->a;
    MYFLT *in2  = p->b;
    int   n, nsmps = csound->ksmps;
    long  input1, input2;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT) (input1 & input2);
    }
    return OK;
}

int and_ak(ENVIRON *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    int   n, nsmps = csound->ksmps;
    long  input2 = MYFLT2LRND(*p->b), input1;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      r[n] = (MYFLT)(input1 & input2);
    }
    return OK;
}

int and_ka(ENVIRON *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    int   n, nsmps = csound->ksmps;
    long  input2, input1 = MYFLT2LRND(*p->a);

    for (n = 0; n < nsmps; n++) {
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT)(input1 & input2);
    }
    return OK;
}

int or_kk(ENVIRON *csound, AOP *p)
{
    long input1 = MYFLT2LRND(*p->a);
    long input2 = MYFLT2LRND(*p->b);
    *p->r = (MYFLT)(input1 | input2);
    return OK;
}

int or_aa(ENVIRON *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    int   n, nsmps = csound->ksmps;
    long  input2, input1;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

int or_ak(ENVIRON *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    int   n, nsmps = csound->ksmps;
    long  input2 = MYFLT2LRND(*p->b), input1;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

int or_ka(ENVIRON *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    int   n, nsmps = csound->ksmps;
    long  input2, input1 = MYFLT2LRND(*p->a);

    for (n = 0; n < nsmps; n++) {
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

int xor_kk(ENVIRON *csound, AOP *p)
{
    long input1 = MYFLT2LRND(*p->a);
    long input2 = MYFLT2LRND(*p->b);
    *p->r = (MYFLT)(input1 ^ input2);
    return OK;
}

int xor_aa(ENVIRON *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    int   n, nsmps = csound->ksmps;
    long  input2, input1;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

int xor_ak(ENVIRON *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    int   n, nsmps = csound->ksmps;
    long  input2 = MYFLT2LRND(*p->b), input1;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

int xor_ka(ENVIRON *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    int   n, nsmps = csound->ksmps;
    long  input2, input1 = MYFLT2LRND(*p->a);

    for (n = 0; n < nsmps; n++) {
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

static int shift_left_kk(ENVIRON *csound, AOP *p)
{
    long input1 = MYFLT2LRND(*p->a);
    int  input2 = (int) MYFLT2LRND(*p->b);
    *p->r = (MYFLT) (input1 << input2);
    return OK;
}

static int shift_left_aa(ENVIRON *csound, AOP *p)
{
    long  input1;
    int   input2, n, nsmps = csound->ksmps;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(p->a[n]);
      input2 = (int) MYFLT2LRND(p->b[n]);
      p->r[n] = (MYFLT) (input1 << input2);
    }
    return OK;
}

static int shift_left_ak(ENVIRON *csound, AOP *p)
{
    long  input1;
    int   input2 = MYFLT2LRND(*p->b);
    int   n, nsmps = csound->ksmps;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(p->a[n]);
      p->r[n] = (MYFLT) (input1 << input2);
    }
    return OK;
}

static int shift_left_ka(ENVIRON *csound, AOP *p)
{
    long  input1 = MYFLT2LRND(*p->a);
    int   input2, n, nsmps = csound->ksmps;

    for (n = 0; n < nsmps; n++) {
      input2 = MYFLT2LRND(p->b[n]);
      p->r[n] = (MYFLT) (input1 << input2);
    }
    return OK;
}

static int shift_right_kk(ENVIRON *csound, AOP *p)
{
    long input1 = MYFLT2LRND(*p->a);
    int  input2 = (int) MYFLT2LRND(*p->b);
    *p->r = (MYFLT) (input1 >> input2);
    return OK;
}

static int shift_right_aa(ENVIRON *csound, AOP *p)
{
    long  input1;
    int   input2, n, nsmps = csound->ksmps;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(p->a[n]);
      input2 = (int) MYFLT2LRND(p->b[n]);
      p->r[n] = (MYFLT) (input1 >> input2);
    }
    return OK;
}

static int shift_right_ak(ENVIRON *csound, AOP *p)
{
    long  input1;
    int   input2 = MYFLT2LRND(*p->b);
    int   n, nsmps = csound->ksmps;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(p->a[n]);
      p->r[n] = (MYFLT) (input1 >> input2);
    }
    return OK;
}

static int shift_right_ka(ENVIRON *csound, AOP *p)
{
    long  input1 = MYFLT2LRND(*p->a);
    int   input2, n, nsmps = csound->ksmps;

    for (n = 0; n < nsmps; n++) {
      input2 = MYFLT2LRND(p->b[n]);
      p->r[n] = (MYFLT) (input1 >> input2);
    }
    return OK;
}

int not_k(ENVIRON *csound, AOP *p)      /* Added for completeness by JPff */
{
    long input1 = MYFLT2LRND(*p->a);
    *p->r = (MYFLT)(~input1);
    return OK;
}

int not_a(ENVIRON *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    int   n, nsmps = csound->ksmps;
    long  input1;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      r[n] = (MYFLT)(~input1);
    }
    return OK;
}

/* all the vcomb and valpass stuff adapted from comb() and alpass()
   with additional insight from me (petemoss@petemoss.org)  */

int vcombset(ENVIRON *csound, VCOMB *p)
{
    long        lpsiz, nbytes;

    if (*p->insmps != FL(0.0)) {
      if ((lpsiz = (long)(FL(0.5)+*p->imaxlpt)) <= 0) {
        return csound->InitError(csound, Str("illegal loop time"));
      }
    }
    else if ((lpsiz = (long)(*p->imaxlpt * csound->esr)) <= 0) {
      return csound->InitError(csound, Str("illegal loop time"));
    }
    nbytes = lpsiz * sizeof(MYFLT);
    if (p->auxch.auxp == NULL || nbytes != p->auxch.size) {
      csound->AuxAlloc(csound, (long)nbytes, &p->auxch);
      p->pntr = (MYFLT *) p->auxch.auxp;
      if (p->pntr==NULL) {
        return csound->InitError(csound, Str("could not allocate memory"));
      }
    }
    else if (!(*p->istor)) {
      long *fp = (long *) p->auxch.auxp;
      p->pntr = (MYFLT *) fp;
      do
        *fp++ = 0;
      while (--lpsiz);
    }
    p->rvt = FL(0.0);
    p->lpt = FL(0.0);
    p->g   = FL(0.0);
    p->lpta = (XINARG3) ? 1 : 0;
    if (*p->insmps == 0) p->maxlpt = *p->imaxlpt * csound->esr;
    else p->maxlpt = *p->imaxlpt;
    return OK;
}

int vcomb(ENVIRON *csound, VCOMB *p)
{
    int n, nsmps = csound->ksmps;
    unsigned long xlpt, maxlpt = (unsigned long)p->maxlpt;
    MYFLT       *ar, *asig, *rp, *endp, *startp, *wp, *lpt;
    MYFLT       g = p->g;

    if (p->auxch.auxp==NULL) {
      return csound->PerfError(csound, Str("vcomb: not initialised"));
    }
    ar = p->ar;
    asig = p->asig;
    endp = (MYFLT *) p->auxch.endp;
    startp = (MYFLT *) p->auxch.auxp;
    wp = p->pntr;
    if (p->lpta) {                               /* if xlpt is a-rate */
      lpt = p->xlpt;
      for (n=0; n<nsmps; n++) {
        xlpt = (unsigned long)((*p->insmps != 0) ? *lpt : *lpt * csound->esr);
        if (xlpt > maxlpt) xlpt = maxlpt;
        if ((rp = wp - xlpt) < startp) rp += maxlpt;
        if ((p->rvt != *p->krvt) || (p->lpt != *lpt)) {
          p->rvt = *p->krvt, p->lpt = *lpt;
          g = p->g = (MYFLT)pow(0.001, (p->lpt / p->rvt));
        }
        lpt++;
        ar[n] = *rp++;
        *wp++ = (ar[n] * g) + asig[n];
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      }
    }
    else {                                       /* if xlpt is k-rate */
      xlpt = (unsigned long) ((*p->insmps != 0) ? *p->xlpt
                                                  : *p->xlpt * csound->esr);
      if (xlpt > maxlpt) xlpt = maxlpt;
      if ((rp = wp - xlpt) < startp) rp += maxlpt;
      if ((p->rvt != *p->krvt) || (p->lpt != *p->xlpt)) {
        p->rvt = *p->krvt, p->lpt = *p->xlpt;
        g = p->g = (MYFLT)pow(0.001, (p->lpt / p->rvt));
      }
      for (n=0; n<nsmps; n++) {
        ar[n] = *rp++;
        *wp++ = (ar[n] * g) + asig[n];
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      }
    }
    p->pntr = wp;
    return OK;
}

int valpass(ENVIRON *csound, VCOMB *p)
{
    int nsmps = csound->ksmps;
    unsigned long xlpt, maxlpt = (unsigned long)p->maxlpt;
    MYFLT       *ar, *asig, *rp, *startp, *endp, *wp, *lpt;
    MYFLT       y, z, g = p->g;

    if (p->auxch.auxp==NULL) {
      return csound->PerfError(csound, Str("valpass: not initialised"));
    }
    ar = p->ar;
    asig = p->asig;
    endp = (MYFLT *) p->auxch.endp;
    startp = (MYFLT *) p->auxch.auxp;
    wp = p->pntr;
    if (p->lpta) {                                      /* if xlpt is a-rate */
      lpt = p->xlpt;
      do {
        xlpt = (unsigned long)((*p->insmps != 0) ? *lpt : *lpt * csound->esr);
        if (xlpt > maxlpt) xlpt = maxlpt;
        if ((rp = wp - xlpt) < startp) rp += maxlpt;
        if ((p->rvt != *p->krvt) || (p->lpt != *lpt)) {
          p->rvt = *p->krvt, p->lpt = *lpt;
          g = p->g = (MYFLT)pow(0.001, (p->lpt / p->rvt));
        }
        lpt++;
        y = *rp++;
        *wp++ = z = y * g + *asig++;
        *ar++ = y - g * z;
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      } while (--nsmps);
    }
    else {                                              /* if xlpt is k-rate */
      xlpt = (unsigned long) ((*p->insmps != 0) ? *p->xlpt
                                                  : *p->xlpt * csound->esr);
      if (xlpt > maxlpt) xlpt = maxlpt;
      if ((rp = wp - xlpt) < startp) rp += maxlpt;
      if ((p->rvt != *p->krvt) || (p->lpt != *p->xlpt)) {
        p->rvt = *p->krvt, p->lpt = *p->xlpt;
        g = p->g = (MYFLT)pow(0.001, (p->lpt / p->rvt));
      }
      do {
        y = *rp++;
        *wp++ = z = y * g + *asig++;
        *ar++ = y - g * z;
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      } while (--nsmps);
    }
    p->pntr = wp;
    return OK;
}

int ftmorfset(ENVIRON *csound, FTMORF *p)
{
    FUNC *ftp;
    int j = 0;
    unsigned int len;
    /* make sure resfn exists and set it up */
    if ((ftp = csound->FTFind(csound, p->iresfn)) != NULL) {
      p->resfn = ftp, len = p->resfn->flen;
    }
    else {
      return csound->InitError(csound, Str("iresfn for ftmorf does not exist"));
    }
    /* make sure ftfn exists and set it up */
    if ((ftp = csound->FTFind(csound, p->iftfn)) != NULL) {
      p->ftfn = ftp;
    }
    else {
      return csound->InitError(csound, Str("iftfn for ftmorf doesnt exist"));
    }

    do {                /* make sure tables in ftfn exist and are right size*/
      if ((ftp = csound->FTFind(csound, p->ftfn->ftable + j)) != NULL) {
        if ((unsigned int)ftp->flen != len) {
          return csound->InitError(csound,
                                   Str("table in iftfn for ftmorf wrong size"));
        }
      }
      else {
        return csound->InitError(csound, Str("table in iftfn for ftmorf "
                                             "does not exist"));
      }
    } while (++j < p->ftfn->flen);

    p->len = len;
    p->ftndx = -FL(1.0);
    return OK;
}

int ftmorf(ENVIRON *csound, FTMORF *p)
{
    unsigned int j = 0;
    int i;
    MYFLT f;
    FUNC *ftp1, *ftp2;

    if (*p->kftndx >= p->ftfn->flen) *p->kftndx = (MYFLT)(p->ftfn->flen - 1);
    i = (int)*p->kftndx;
    f = *p->kftndx - i;
    if (p->ftndx != *p->kftndx) {
      p->ftndx = *p->kftndx;
      ftp1 = csound->FTFind(csound, p->ftfn->ftable + i++);
      ftp2 = csound->FTFind(csound, p->ftfn->ftable + i--);
      do {
        *(p->resfn->ftable + j) = (*(ftp1->ftable + j) * (1-f)) +
          (*(ftp2->ftable + j) * f);
      } while (++j < p->len);
    }
    return OK;
}

/* end of ugmoss.c */
#define S       sizeof

static OENTRY localops[] = {
{ "dconv",  S(DCONV), 5, "a", "aii",   (SUBR)dconvset, NULL, (SUBR)dconv      },
{ "vcomb", S(VCOMB),  5, "a", "akxioo", (SUBR)vcombset, NULL, (SUBR)vcomb     },
{ "valpass", S(VCOMB),5, "a", "akxioo", (SUBR)vcombset, NULL, (SUBR)valpass   },
{ "ftmorf", S(FTMORF),3, "",  "kii",  (SUBR)ftmorfset,  (SUBR)ftmorf, NULL    },
{ "and.ii",  S(AOP),  1, "i", "ii",   (SUBR)and_kk                  },
{ "and.kk",  S(AOP),  2, "k", "kk",   NULL,   (SUBR)and_kk          },
{ "and.ka",  S(AOP),  4, "a", "ka",   NULL,   NULL,   (SUBR)and_ka  },
{ "and.ak",  S(AOP),  4, "a", "ak",   NULL,   NULL,   (SUBR)and_ak  },
{ "and.aa",  S(AOP),  4, "a", "aa",   NULL,   NULL,   (SUBR)and_aa  },
{ "or.ii",   S(AOP),  1, "i", "ii",   (SUBR)or_kk                   },
{ "or.kk",   S(AOP),  2, "k", "kk",   NULL,   (SUBR)or_kk           },
{ "or.ka",   S(AOP),  4, "a", "ka",   NULL,   NULL,   (SUBR)or_ka   },
{ "or.ak",   S(AOP),  4, "a", "ak",   NULL,   NULL,   (SUBR)or_ak   },
{ "or.aa",   S(AOP),  4, "a", "aa",   NULL,   NULL,   (SUBR)or_aa   },
{ "xor.ii",  S(AOP),  1, "i", "ii",   (SUBR)xor_kk                  },
{ "xor.kk",  S(AOP),  2, "k", "kk",   NULL,   (SUBR)xor_kk          },
{ "xor.ka",  S(AOP),  4, "a", "ka",   NULL,   NULL,   (SUBR)xor_ka  },
{ "xor.ak",  S(AOP),  4, "a", "ak",   NULL,   NULL,   (SUBR)xor_ak  },
{ "xor.aa",  S(AOP),  4, "a", "aa",   NULL,   NULL,   (SUBR)xor_aa  },
{ "not.i",   S(AOP),  1, "i", "i",    (SUBR)not_k                   },
{ "not.k",   S(AOP),  2, "k", "k",    NULL,   (SUBR)not_k           },
{ "not.a",   S(AOP),  4, "a", "a",    NULL,   NULL,   (SUBR)not_a   },
{ "shl.ii",  S(AOP),  1, "i", "ii",   (SUBR) shift_left_kk          },
{ "shl.kk",  S(AOP),  2, "k", "kk",   NULL, (SUBR) shift_left_kk    },
{ "shl.ka", S(AOP), 4, "a", "ka", NULL, NULL, (SUBR) shift_left_ka  },
{ "shl.ak", S(AOP), 4, "a", "ak", NULL, NULL, (SUBR) shift_left_ak  },
{ "shl.aa", S(AOP), 4, "a", "aa", NULL, NULL, (SUBR) shift_left_aa  },
{ "shr.ii",  S(AOP),  1, "i", "ii",   (SUBR) shift_right_kk         },
{ "shr.kk",  S(AOP),  2, "k", "kk",   NULL, (SUBR) shift_right_kk   },
{ "shr.ka", S(AOP), 4, "a", "ka", NULL, NULL, (SUBR) shift_right_ka },
{ "shr.ak", S(AOP), 4, "a", "ak", NULL, NULL, (SUBR) shift_right_ak },
{ "shr.aa", S(AOP), 4, "a", "aa", NULL, NULL, (SUBR) shift_right_aa }
};

LINKAGE

