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
int dconvset(DCONV *p)
{
    FUNC *ftp;

    p->len = (int)*p->isize;
    if ((ftp = ftfind(p->ifn)) != NULL) {         /* find table */
      p->ftp = ftp;
      if ((unsigned)ftp->flen < p->len)
        p->len = ftp->flen; /* correct len if flen shorter */
    }
    else {
      return initerror(Str(X_1557,"No table for dconv"));
    }
    if (p->sigbuf.auxp == NULL || p->sigbuf.size < (int)(p->len*sizeof(MYFLT)))
      auxalloc(p->len*sizeof(MYFLT), &p->sigbuf);
    p->curp = (MYFLT *)p->sigbuf.auxp;
    return OK;
}

int dconv(DCONV *p)
{
    long i = 0;
    int nsmps = ksmps;
    long len = p->len;
    MYFLT *ar, *ain, *ftp, *startp, *endp, *curp;
    MYFLT sum;

    ain = p->ain;                               /* read saved values */
    ar = p->ar;
    ftp = p->ftp->ftable;
    startp = (MYFLT *) p->sigbuf.auxp;
    endp = startp + len;
    curp = p->curp;

    do {
      *curp = *ain++;                           /* get next input sample */
      i = 1, sum = *curp++ * *ftp;
      while (curp<endp)
        sum += (*curp++ * *(ftp + i++));        /* start the convolution */
      curp = startp;                            /* correct the ptr */
      while (i<len)
        sum += (*curp++ * *(ftp + i++));        /* finish the convolution */
      if (--curp < startp)
        curp += len;                            /* correct for last curp++ */
      *ar++ = sum;
    } while (--nsmps);

    p->curp = curp;                             /* save state */
    return OK;
}

int and_kk(AOP *p)
{
    long input1 = (long)(*p->a + FL(0.5));
    long input2 = (long)(*p->b + FL(0.5));
    *p->r = (MYFLT)(input1 & input2);
    return OK;
}

int and_aa(AOP *p)
{
    MYFLT *r           = p->r;
    MYFLT *in1         = p->a;
    MYFLT *in2         = p->b;
    unsigned int nsmps = ksmps;
    long input1, input2;

    do {
      input1 = (long)(*in1++ + FL(0.5));
      input2 = (long)(*in2++ + FL(0.5));
      *r++ = (MYFLT)(input1 & input2);
    } while (--nsmps);
    return OK;
}

int and_ak(AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    unsigned int nsmps = ksmps;
    long input2 = (long)(*p->b + FL(0.5)), input1;

    do {
      input1 = (long)(*in1++ + FL(0.5));
      *r++ = (MYFLT)(input1 & input2);
    } while (--nsmps);
    return OK;
}

int and_ka(AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    unsigned int nsmps = ksmps;
    long input2, input1 = (long)(*p->a + FL(0.5));

    do {
      input2 = (long)(*in2++ + FL(0.5));
      *r++ = (MYFLT)(input1 & input2);
    } while (--nsmps);
    return OK;
}

int or_kk(AOP *p)
{
    long input1 = (long)(*p->a + FL(0.5));
    long input2 = (long)(*p->b + FL(0.5));
    *p->r = (MYFLT)(input1 | input2);
    return OK;
}

int or_aa(AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    unsigned int nsmps = ksmps;
    long input2, input1;

    do {
      input1 = (long)(*in1++ + FL(0.5));
      input2 = (long)(*in2++ + FL(0.5));
      *r++ = (MYFLT)(input1 | input2);
    } while (--nsmps);
    return OK;
}

int or_ak(AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    unsigned int nsmps = ksmps;
    long input2 = (long)(*p->b + FL(0.5)), input1;

    do {
      input1 = (long)(*in1++ + FL(0.5));
      *r++ = (MYFLT)(input1 | input2);
    } while (--nsmps);
    return OK;
}

int or_ka(AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    unsigned int nsmps = ksmps;
    long input2, input1 = (long)(*p->a + FL(0.5));

    do {
      input2 = (long)(*in2++ + FL(0.5));
      *r++ = (MYFLT)(input1 | input2);
    } while (--nsmps);
    return OK;
}

int xor_kk(AOP *p)
{
    long input1 = (long)(*p->a + FL(0.5));
    long input2 = (long)(*p->b + FL(0.5));
    *p->r = (MYFLT)(input1 ^ input2);
    return OK;
}

int xor_aa(AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    unsigned int nsmps = ksmps;
    long input2, input1;

    do {
      input1 = (long)(*in1++ + FL(0.5));
      input2 = (long)(*in2++ + FL(0.5));
      *r++ = (MYFLT)(input1 ^ input2);
    } while (--nsmps);
    return OK;
}

int xor_ak(AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    unsigned int nsmps = ksmps;
    long input2 = (long)(*p->b + FL(0.5)), input1;

    do {
      input1 = (long)(*in1++ + FL(0.5));
      *r++ = (MYFLT)(input1 ^ input2);
    } while (--nsmps);
    return OK;
}

int xor_ka(AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    unsigned int nsmps = ksmps;
    long input2, input1 = (long)(*p->a + FL(0.5));

    do {
      input2 = (long)(*in2++ + FL(0.5));
      *r++ = (MYFLT)(input1 ^ input2);
    } while (--nsmps);
    return OK;
}

int not_k(AOP *p)              /* Added for completeness by JPff */
{
    long input1 = (long)(*p->a + FL(0.5));
    *p->r = (MYFLT)(~input1);
    return OK;
}

int not_a(AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    unsigned int nsmps = ksmps;
    long input1;

    do {
      input1 = (long)(*in1++ + FL(0.5));
      *r++ = (MYFLT)(~input1);
    } while (--nsmps);
    return OK;
}

/* all the vcomb and valpass stuff adapted from comb() and alpass()
   with additional insight from me (petemoss@petemoss.org)  */

int vcombset(VCOMB *p)
{
    long        lpsiz, nbytes;

    if (*p->insmps != FL(0.0)) {
      if ((lpsiz = (long)(FL(0.5)+*p->imaxlpt)) <= 0) {
        return initerror(Str(X_867,"illegal loop time"));
      }
    }
    else if ((lpsiz = (long)(*p->imaxlpt * esr)) <= 0) {
      return initerror(Str(X_867,"illegal loop time"));
    }
    nbytes = lpsiz * sizeof(MYFLT);
    if (p->auxch.auxp == NULL || nbytes != p->auxch.size) {
      auxalloc((long)nbytes, &p->auxch);
      p->pntr = (MYFLT *) p->auxch.auxp;
      if (p->pntr==NULL) {
        return initerror(Str(X_668,"could not allocate memory"));
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
    if (*p->insmps == 0) *p->imaxlpt *= esr;
    return OK;
}

int vcomb(VCOMB *p)
{
    int nsmps = ksmps;
    unsigned long xlpt, maxlpt = (unsigned long)*p->imaxlpt;
    MYFLT       *ar, *asig, *rp, *endp, *startp, *wp, *lpt;
    MYFLT       g = p->g;

    if (p->auxch.auxp==NULL) {
      return perferror(Str(X_1685,"vcomb: not initialised"));
    }
    ar = p->ar;
    asig = p->asig;
    endp = (MYFLT *) p->auxch.endp;
    startp = (MYFLT *) p->auxch.auxp;
    wp = p->pntr;
    if (p->lpta) {                               /* if xlpt is a-rate */
      lpt = p->xlpt;
      do {
        xlpt = (unsigned long)((*p->insmps != 0) ? *lpt : *lpt * esr);
        if (xlpt > maxlpt) xlpt = maxlpt;
        if ((rp = wp - xlpt) < startp) rp += maxlpt;
        if ((p->rvt != *p->krvt) || (p->lpt != *lpt)) {
          p->rvt = *p->krvt, p->lpt = *lpt;
          g = p->g = (MYFLT)pow(0.001, (p->lpt / p->rvt));
        }
        lpt++;
        *ar = *rp++;
        *wp++ = (*ar++ * g) + *asig++;
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      } while (--nsmps);
    }
    else {                                       /* if xlpt is k-rate */
      xlpt = (unsigned long)((*p->insmps != 0) ? *p->xlpt : *p->xlpt * esr);
      if (xlpt > maxlpt) xlpt = maxlpt;
      if ((rp = wp - xlpt) < startp) rp += maxlpt;
      if ((p->rvt != *p->krvt) || (p->lpt != *p->xlpt)) {
        p->rvt = *p->krvt, p->lpt = *p->xlpt;
        g = p->g = (MYFLT)pow(0.001, (p->lpt / p->rvt));
      }
      do {
        *ar = *rp++;
        *wp++ = (*ar++ * g) + *asig++;
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      } while (--nsmps);
    }
    p->pntr = wp;
    return OK;
}

int valpass(VCOMB *p)
{
    int nsmps = ksmps;
    unsigned long xlpt, maxlpt = (unsigned long)*p->imaxlpt;
    MYFLT       *ar, *asig, *rp, *startp, *endp, *wp, *lpt;
    MYFLT       y, z, g = p->g;

    if (p->auxch.auxp==NULL) {
      return perferror(Str(X_1686,"valpass: not initialised"));
    }
    ar = p->ar;
    asig = p->asig;
    endp = (MYFLT *) p->auxch.endp;
    startp = (MYFLT *) p->auxch.auxp;
    wp = p->pntr;
    if (p->lpta) {                                      /* if xlpt is a-rate */
      lpt = p->xlpt;
      do {
        xlpt = (unsigned long)((*p->insmps != 0) ? *lpt : *lpt * esr);
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
      xlpt = (unsigned long)((*p->insmps != 0) ? *p->xlpt : *p->xlpt * esr);
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

int ftmorfset(FTMORF *p)
{
    FUNC *ftp;
    int j = 0;
    unsigned int len;

    if ((ftp = ftfind(p->iresfn)) != NULL) {    /* make sure resfn exists */
      p->resfn = ftp, len = p->resfn->flen;     /* and set it up */
    }
    else {
      return initerror(Str(X_1687,"iresfn for ftmorf does not exist"));
    }

    if ((ftp = ftfind(p->iftfn)) != NULL) {     /* make sure ftfn exists */
      p->ftfn = ftp;                            /* and set it up */
    }
    else {
      return initerror(Str(X_1688,"iftfn for ftmorf doesnt exist"));
    }

    do {                /* make sure tables in ftfn exist and are right size*/
      if ((ftp = ftfind(p->ftfn->ftable + j)) != NULL) {
        if ((unsigned int)ftp->flen != len) {
          return initerror(Str(X_1689,"table in iftfn for ftmorf wrong size"));
        }
      }
      else {
        return initerror(Str(X_1690,"table in iftfn for ftmorf does not exist"));
      }
    } while (++j < p->ftfn->flen);

    p->len = len;
    p->ftndx = -FL(1.0);
    return OK;
}

int ftmorf(FTMORF *p)
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
      ftp1 = ftfind(p->ftfn->ftable + i++);
      ftp2 = ftfind(p->ftfn->ftable + i--);
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
{ "and_ii",  S(AOP),  1, "i", "ii",   (SUBR)and_kk                  },
{ "and_kk",  S(AOP),  2, "k", "kk",   NULL,   (SUBR)and_kk          },
{ "and_ka",  S(AOP),  4, "a", "ka",   NULL,   NULL,   (SUBR)and_ka  },
{ "and_ak",  S(AOP),  4, "a", "ak",   NULL,   NULL,   (SUBR)and_ak  },
{ "and_aa",  S(AOP),  4, "a", "aa",   NULL,   NULL,   (SUBR)and_aa  },
{ "or_ii",   S(AOP),  1, "i", "ii",   (SUBR)or_kk                   },
{ "or_kk",   S(AOP),  1, "i", "kk",   (SUBR)or_kk                   },
{ "or_ka",   S(AOP),  4, "a", "ka",   NULL,   (SUBR)or_kk,          },
{ "or_ak",   S(AOP),  4, "a", "ak",   NULL,   NULL,   (SUBR)or_ak   },
{ "or_aa",   S(AOP),  4, "a", "aa",   NULL,   NULL,   (SUBR)or_aa   },
{ "xor_ii",  S(AOP),  1, "i", "ii",   (SUBR)xor_kk                  },
{ "xor_kk",  S(AOP),  2, "k", "kk",   NULL,   (SUBR)xor_kk          },
{ "xor_ka",  S(AOP),  4, "a", "ka",   NULL,   NULL,   (SUBR)xor_ka  },
{ "xor_ak",  S(AOP),  4, "a", "ak",   NULL,   NULL,   (SUBR)xor_ak  },
{ "xor_aa",  S(AOP),  4, "a", "aa",   NULL,   NULL,   (SUBR)xor_aa  },
{ "not_i",   S(AOP),  1, "i", "i",    (SUBR)not_k                   },
{ "not_k",   S(AOP),  2, "k", "k",    NULL,   (SUBR)not_k           },
{ "not_a",   S(AOP),  4, "a", "a",    NULL,   NULL,   (SUBR)not_a   },
};

LINKAGE
