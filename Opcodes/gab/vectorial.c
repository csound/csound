/*   Copyright (C) 2002-2004 Gabriel Maldonado

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
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
     02111-1307 USA

    Ported to csound5 by:Andres Cabrera andres@geminiflux.com
    This file includes the vectorial table opcodes from newopcodes.c
*/
#include "csdl.h"
#include "vectorial.h"
#include <math.h>


#ifdef MAKEDLL
/* #define PUBLIC __declspec(dllexport) */
#define DIR_SEP '\\'
#else
/* #define PUBLIC */
#define DIR_SEP '/'
#endif


int mtable_i(ENVIRON *csound,MTABLEI *p)
{
    FUNC *ftp;
    int j, nargs;
    MYFLT *table, xbmul = FL(0.0), **out = p->outargs;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "mtablei: incorrect table number");
    }
    table = ftp->ftable;
    nargs = p->INOCOUNT-4;
    if (*p->ixmode)
      xbmul = (MYFLT) (ftp->flen / nargs);

    if (*p->kinterp) {
      MYFLT     v1, v2 ;
      MYFLT fndx = (*p->ixmode) ? *p->xndx * xbmul : *p->xndx;
      long indx = (long) fndx;
      MYFLT fract = fndx - indx;
      for (j=0; j < nargs; j++) {
        v1 = table[indx * nargs + j];
        v2 = table[(indx + 1) * nargs + j];
        **out++ = v1 + (v2 - v1) * fract;
      }
    }
    else {
      long indx = (*p->ixmode) ? (long)(*p->xndx * xbmul) : (long) *p->xndx;
      for (j=0; j < nargs; j++)
        **out++ =  table[indx * nargs + j];
    }
    return OK;
}

int mtable_set(ENVIRON *csound,MTABLE *p)        /*  mtab by G.Maldonado */
{
    FUNC *ftp;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "mtable: incorrect table number");
    }
    p->ftable = ftp->ftable;
    p->nargs = p->INOCOUNT-4;
    p->len = ftp->flen / p->nargs;
    p->pfn = (long) *p->xfn;
    if (*p->ixmode)
      p->xbmul = (MYFLT) ftp->flen / p->nargs;
    return OK;
}

int mtable_k(ENVIRON *csound,MTABLE *p)
{
    int j, nargs = p->nargs;
    MYFLT **out = p->outargs;
    MYFLT *table;
    long len;
    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, "mtable: incorrect table number");
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
      if (*p->ixmode)
        p->xbmul = (MYFLT) ftp->flen / nargs;
    }
    table= p->ftable;
    len = p->len;
    if (*p->kinterp) {
      MYFLT fndx;
      long indx;
      MYFLT fract;
      long indxp1;
      MYFLT     v1, v2 ;
      fndx = (*p->ixmode) ? *p->xndx * p->xbmul : *p->xndx;
      if (fndx >= len)
        fndx = (MYFLT) fmod(fndx, len);
      indx = (long) fndx;
      fract = fndx - indx;
      indxp1 = (indx < len-1) ? (indx+1) * nargs : 0;
      indx *=nargs;
      for (j=0; j < nargs; j++) {
        v1 = table[indx + j];
        v2 = table[indxp1 + j];
        **out++ = v1 + (v2 - v1) * fract;
      }
    }
    else {
      long indx = (*p->ixmode) ? ((long)(*p->xndx * p->xbmul) % len) * nargs :
                                 ((long) *p->xndx % len ) * nargs ;
      for (j=0; j < nargs; j++)
        **out++ =  table[indx + j];
    }
    return OK;
}

int mtable_a(ENVIRON *csound,MTABLE *p)
{
    int j, nargs = p->nargs;
    int nsmps = ksmps, ixmode = (int) *p->ixmode, k=0;
    MYFLT **out = p->outargs;
    MYFLT *table;
    MYFLT *xndx = p->xndx, xbmul;
    long len;

    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, "mtable: incorrect table number");
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
      if (ixmode)
        p->xbmul = (MYFLT) ftp->flen / nargs;
    }
    table = p->ftable;
    len = p->len;
    xbmul = p->xbmul;
    if (*p->kinterp) {
      MYFLT fndx;
      long indx;
      MYFLT fract;
      long indxp1;
      do {
        MYFLT   v1, v2 ;
        fndx = (ixmode) ? *xndx++ * xbmul : *xndx++;
        if (fndx >= len)
          fndx = (MYFLT) fmod(fndx, len);
        indx = (long) fndx;
        fract = fndx - indx;
        indxp1 = (indx < len-1) ? (indx+1) * nargs : 0;
        indx *=nargs;
        for (j=0; j < nargs; j++) {
          v1 = table[indx + j];
          v2 = table[indxp1 + j];
          out[j][k] = v1 + (v2 - v1) * fract;

        }
        k++;
      } while(--nsmps);

    }
    else {
      do {
        long indx = (ixmode) ? ((long)(*xndx++ * xbmul)%len) * nargs :
                               ((long) *xndx++ %len) * nargs;
        for (j=0; j < nargs; j++) {
          out[j][k] =  table[indx + j];
        }
        k++;
      } while(--nsmps);
    }
    return OK;
}


int mtab_i(ENVIRON *csound,MTABI *p)
{
    FUNC *ftp;
    int j, nargs;
    long indx;
    MYFLT *table, **out = p->outargs;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "mtabi: incorrect table number");
    }
    table = ftp->ftable;
    nargs = p->INOCOUNT-2;

    indx = (long) *p->xndx;
    for (j=0; j < nargs; j++)
      **out++ =  table[indx * nargs + j];
    return OK;
}

int mtab_set(ENVIRON *csound,MTAB *p)    /* mtab by G.Maldonado */
{
    FUNC *ftp;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "mtable: incorrect table number");
    }
    p->ftable = ftp->ftable;
    p->nargs = p->INOCOUNT-2;
    p->len = ftp->flen / p->nargs;
    p->pfn = (long) *p->xfn;
    return OK;
}

int mtab_k(ENVIRON *csound,MTAB *p)
{
    int j, nargs = p->nargs;
    MYFLT **out = p->outargs;
    MYFLT *table;
    long len, indx;

    table= p->ftable;
    len = p->len;
    indx = ((long) *p->xndx % len ) * nargs ;
    for (j=0; j < nargs; j++)
      **out++ =  table[indx + j];
    return OK;
}

int mtab_a(ENVIRON *csound,MTAB *p)
{
    int j, nargs = p->nargs;
    int nsmps = ksmps, k=0;
    MYFLT **out = p->outargs;
    MYFLT *table;
    MYFLT *xndx = p->xndx;
    long len;
    table = p->ftable;
    len = p->len;
    do {
      long indx = ((long) *xndx++ %len) * nargs;
      for (j=0; j < nargs; j++) {
        out[j][k] =  table[indx + j];
      }
      k++;
    } while(--nsmps);
    return OK;
}

/* ////////// mtab end /////////////// */

int mtablew_i(ENVIRON *csound,MTABLEIW *p)
{
    FUNC *ftp;
    int j, nargs;
    long indx;
    MYFLT *table, xbmul = FL(0.0), **in = p->inargs;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "mtablewi: incorrect table number");
    }
    table = ftp->ftable;
    nargs = p->INOCOUNT-3;
    if (*p->ixmode)
      xbmul = (MYFLT) (ftp->flen / nargs);
    indx = (*p->ixmode) ? (long)(*p->xndx * xbmul) : (long) *p->xndx;
    for (j=0; j < nargs; j++)
      table[indx * nargs + j] = **in++;
    return OK;
}

int mtablew_set(ENVIRON *csound,MTABLEW *p)      /* mtabw by G.Maldonado */
{
    FUNC *ftp;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "mtabw: incorrect table number");
    }
    p->ftable = ftp->ftable;
    p->nargs = p->INOCOUNT-3;
    p->len = ftp->flen / p->nargs;
    p->pfn = (long) *p->xfn;
    if (*p->ixmode)
      p->xbmul = (MYFLT) ftp->flen / p->nargs;
    return OK;
}

int mtablew_k(ENVIRON *csound,MTABLEW *p)
{
    int j, nargs = p->nargs;
    MYFLT **in = p->inargs;
    MYFLT *table;
    long len, indx;
    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, "mtabw: incorrect table number");
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
      if (*p->ixmode)
        p->xbmul = (MYFLT) ftp->flen / nargs;
    }
    table= p->ftable;
    len = p->len;
    indx = (*p->ixmode) ? ((long)(*p->xndx * p->xbmul) % len) * nargs :
                          ((long) *p->xndx % len ) * nargs ;
    for (j=0; j < nargs; j++)
      table[indx + j] = **in++;
    return OK;
}

int mtablew_a(ENVIRON *csound,MTABLEW *p)
{
    int j, nargs = p->nargs;
    int nsmps = ksmps, ixmode = (int) *p->ixmode, k=0;
    MYFLT **in = p->inargs;
    MYFLT *table;
    MYFLT *xndx = p->xndx, xbmul;
    long len;

    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, "mtabw: incorrect table number");
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
      if (ixmode)
        p->xbmul = (MYFLT) ftp->flen / nargs;
    }
    table = p->ftable;
    len = p->len;
    xbmul = p->xbmul;
    do {
      long indx = (ixmode) ? ((long)(*xndx++ * xbmul)%len) * nargs :
                             ((long) *xndx++ %len) * nargs;
      for (j=0; j < nargs; j++) {
        table[indx + j] = in[j][k];
      }
      k++;
    } while(--nsmps);
    return OK;
}

/* ////////////////////////////////////// */

int mtabw_i(ENVIRON *csound,MTABIW *p)
{
    FUNC *ftp;
    int j, nargs;
    long indx;
    MYFLT *table, **in = p->inargs;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "mtabwi: incorrect table number");
    }
    table = ftp->ftable;
    nargs = p->INOCOUNT-2;
    indx = (long) *p->xndx;
    for (j=0; j < nargs; j++)
      table[indx * nargs + j] = **in++;
    return OK;
}

int mtabw_set(ENVIRON *csound,MTABW *p)  /* mtabw by G.Maldonado */
{
    FUNC *ftp;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "mtabw: incorrect table number");
    }
    p->ftable = ftp->ftable;
    p->nargs = p->INOCOUNT-2;
    p->len = ftp->flen / p->nargs;
    p->pfn = (long) *p->xfn;
    return OK;
}

int mtabw_k(ENVIRON *csound,MTABW *p)
{
    int j, nargs = p->nargs;
    MYFLT **in = p->inargs;
    MYFLT *table;
    long len, indx;
    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, "mtablew: incorrect table number");
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
    }
    table= p->ftable;
    len = p->len;
    indx = ((long) *p->xndx % len ) * nargs ;
    for (j=0; j < nargs; j++)
      table[indx + j] = **in++;
    return OK;
}

int mtabw_a(ENVIRON *csound,MTABW *p)
{
    int j, nargs = p->nargs;
    int nsmps = ksmps, k=0;
    MYFLT **in = p->inargs;
    MYFLT *table;
    MYFLT *xndx = p->xndx;
    long len;

    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTnp2Find(csound, p->xfn) ) == NULL) {
        return csound->PerfError(csound, "mtabw: incorrect table number");
      }
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
      p->len = ftp->flen / nargs;
    }
    table = p->ftable;
    len = p->len;
    do {
      long indx = ((long) *xndx++ %len) * nargs;
      for (j=0; j < nargs; j++) {
        table[indx + j] = in[j][k];
      }
      k++;
    } while(--nsmps);
    return OK;
}


/* The following opcodes come from CsoundAV/vectorial.c */

int vectorOp_set(ENVIRON *csound,VECTOROP *p)
{
    FUNC        *ftp;
    if ((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL) {
      p->vector = ftp->ftable;
      p->elements = (int) *p->ielements;
    }
    if ( p->elements > ftp->flen ) {
      return csound->InitError(csound, "vectorop: invalid num of elements");
    }
    return OK;
}

int vadd(ENVIRON *csound,VECTOROP *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector;
    MYFLT value = *p->kval;

    do {
      *vector++ += value;
    } while (--elements);
    return OK;
}

int vmult(ENVIRON *csound,VECTOROP *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector;
    MYFLT value = *p->kval;

    do {
      *vector++ *= value;
    } while (--elements);
    return OK;
}

int vpow(ENVIRON *csound,VECTOROP *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector;
    MYFLT value = *p->kval;

    do {
      *vector = (MYFLT) pow (*vector, value);
      vector++;
    } while (--elements);
    return OK;
}

int vexp(ENVIRON *csound,VECTOROP *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector;
    MYFLT value = *p->kval;

    do {
      *vector = (MYFLT) pow (value, *vector);
      vector++;
    } while (--elements);
    return OK;
}
/* ------------------------- */

int vectorsOp_set(ENVIRON *csound,VECTORSOP *p)
{
    FUNC        *ftp1, *ftp2;
    if ((ftp1 = csound->FTnp2Find(csound,p->ifn1)) != NULL) {
      p->vector1 = ftp1->ftable;
    }
    if ((ftp2 = csound->FTnp2Find(csound,p->ifn2)) != NULL) {
      p->vector2 = ftp2->ftable;
    }
    p->elements = (int) *p->ielements;
    if ( p->elements > ftp1->flen || p->elements > ftp2->flen) {
      return csound->InitError(csound, "vectorop: invalid num of elements");
    }
    return OK;
}


int vcopy(ENVIRON *csound,VECTORSOP *p)
{
    int elements = p->elements;
    MYFLT *vector1 = p->vector1, *vector2 = p->vector2;

    do {
      *vector1++ = *vector2++;
    } while (--elements);
    return OK;
}

int vcopy_i(ENVIRON *csound,VECTORSOP *p)
{
    FUNC        *ftp1, *ftp2;
    MYFLT   *vector1 = NULL, *vector2 = NULL;
    unsigned long       elements;
    if ((ftp1 = csound->FTnp2Find(csound,p->ifn1)) != NULL) {
      vector1 = ftp1->ftable;
    }
    if ((ftp2 = csound->FTnp2Find(csound,p->ifn2)) != NULL) {
      vector2 = ftp2->ftable;
    }
    elements = (unsigned long) *p->ielements;
    if ( elements > ftp1->flen || elements > ftp2->flen) {
      return csound->InitError(csound, "vcopy_i: invalid num of elements");
    }

    do {
      *vector1++ = *vector2++;
    } while (--elements);
    return OK;
}



int vaddv(ENVIRON *csound,VECTORSOP *p)
{
    int elements = p->elements;
    MYFLT *vector1 = p->vector1, *vector2 = p->vector2;

    do {
      *vector1++ += *vector2++;
    } while (--elements);
    return OK;
}

int vsubv(ENVIRON *csound,VECTORSOP *p)
{
    int elements = p->elements;
    MYFLT *vector1 = p->vector1, *vector2 = p->vector2;

    do {
      *vector1++ -= *vector2++;
    } while (--elements);
    return OK;
}

int vmultv(ENVIRON *csound,VECTORSOP *p)
{
    int elements = p->elements;
    MYFLT *vector1 = p->vector1, *vector2 = p->vector2;

    do {
      *vector1++ *= *vector2++;
    } while (--elements);
    return OK;
}

int vdivv(ENVIRON *csound,VECTORSOP *p)
{
    int elements = p->elements;
    MYFLT *vector1 = p->vector1, *vector2 = p->vector2;

    do {
      *vector1++ /= *vector2++;
    } while (--elements);
    return OK;
}

int vpowv(ENVIRON *csound,VECTORSOP *p)
{
    int elements = p->elements;
    MYFLT *vector1 = p->vector1, *vector2 = p->vector2;

    do {
      *vector1 = (MYFLT) pow (*vector1, *vector2++);
      vector1++;
    } while (--elements);
    return OK;
}

int vexpv(ENVIRON *csound,VECTORSOP *p)
{
    int elements = p->elements;
    MYFLT *vector1 = p->vector1, *vector2 = p->vector2;

    do {
      *vector1 = (MYFLT) pow (*vector1, *vector2++);
      vector1++;
    } while (--elements);
    return OK;
}

int vmap(ENVIRON *csound,VECTORSOP *p)
{
    int elements = p->elements;
    MYFLT *vector1 = p->vector1, *vector2 = p->vector2;

    do {
      *vector1 = (vector2++)[(int)*vector1];
      vector1++;
    } while (--elements);
    return OK;
}


int vlimit_set(ENVIRON *csound,VLIMIT *p)
{
    FUNC        *ftp;
    if ((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL) {
      p->vector = ftp->ftable;
      p->elements = (int) *p->ielements;
    }
    if ( p->elements >= ftp->flen ) {
      return csound->InitError(csound, "vectorop: invalid num of elements");
    }
    return OK;
}

int vlimit(ENVIRON *csound,VLIMIT *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector;
    MYFLT min = *p->kmin, max = *p->kmax;
    do {
      *vector = (*vector > min) ? ((*vector < max) ? *vector : max) : min;
      vector++;
    } while (--elements);
    return OK;
}

/*-----------------------------*/
int vport_set(ENVIRON *csound,VPORT *p)
{
    FUNC        *ftp;
    int elements;
    MYFLT *vector, *yt1,*vecInit  = NULL;

    if ((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL) {
      vector = (p->vector = ftp->ftable);
      elements = (p->elements = (int) *p->ielements);
      if ( elements >= ftp->flen )
        return csound->InitError(csound, "vport: invalid table length or num of elements");
    }
    else return csound->InitError(csound, "vport: invalid table");
    if (*p->ifnInit) {
      if ((ftp = csound->FTnp2Find(csound,p->ifnInit)) != NULL) {
        vecInit = ftp->ftable;
        if ( elements >= ftp->flen )
          return csound->InitError(csound, "vport: invalid init table length or num of elements");
      }
      else return csound->InitError(csound, "vport: invalid init table");
    }
    if (p->auxch.auxp == NULL)
      csound->AuxAlloc(csound, elements * sizeof(MYFLT), &p->auxch);
    yt1 = (p->yt1 = (MYFLT *) p->auxch.auxp);
    if (vecInit) {
      do {
        *yt1++ = *vecInit++;
      } while (--elements);
    } else {
      do {
        *yt1++ = FL(0.0);
      } while (--elements);
    }
    p->prvhtim = -FL(100.0);
    return OK;
}

int vport(ENVIRON *csound,VPORT *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector, *yt1 = p->yt1, c1, c2;
    if (p->prvhtim != *p->khtim) {
      p->c2 = (MYFLT)pow(0.5, (double)onedkr / *p->khtim);
      p->c1 = FL(1.0) - p->c2;
      p->prvhtim = *p->khtim;
    }
    c1 = p->c1;
    c2 = p->c2;
    do {
      *vector = (*yt1 = c1 * *vector + c2 * *yt1);
      vector++; yt1++;
    } while (--elements);
    return OK;
}

/*-------------------------------*/
int vwrap(ENVIRON *csound,VLIMIT *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector;
    MYFLT min = *p->kmin, max = *p->kmax;

    if (min >= max) {
      MYFLT average = (min+max)/2;
      do {
        *vector++ = average;
      } while (--elements);
    }
    else {
      do {
        if (*vector >= max) {
          *vector = min + (MYFLT) fmod(*vector - min, fabs(min-max));
          vector++;
        }
        else {
          *vector = max - (MYFLT) fmod(max - *vector, fabs(min-max));
          vector++;
        }
      } while (--elements);
    }
    return OK;
}

int vmirror(ENVIRON *csound,VLIMIT *p)
{
    int elements = p->elements;
    MYFLT *vector = p->vector;
    MYFLT min = *p->kmin, max = *p->kmax;

    if (min >= max) {
      MYFLT average = (min+max)* FL(0.50);
      do {
        *vector++ = average;
      } while (--elements);
    }
    else {
      do {
        while (!((*vector <= max) && (*vector >=min))) {
          if (*vector > max)
            *vector = max + max - *vector;
          else
            *vector = min + min - *vector;
        }
        vector++;
      } while (--elements);
    }
    return OK;
}

/* #include "newopcodes.h" */

int vrandh_set(ENVIRON *csound,VRANDH *p)
{
    FUNC        *ftp;
    int elements = 0;
    MYFLT *num1;
    if ((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL) {
      p->vector = ftp->ftable;
      elements = (p->elements = (int) *p->ielements);
    }
    else return NOTOK;
    if ( p->elements >= ftp->flen )
      return csound->InitError(csound, "vectorop: invalid num of elements");

    p->phs = 0;
    if (p->auxch.auxp == NULL)
      csound->AuxAlloc(csound, p->elements * sizeof(MYFLT), &p->auxch);
    num1 = (p->num1 = (MYFLT *) p->auxch.auxp);
    do {
      *num1++ = BiRandGab;
    } while (--elements);
    return OK;
}

int vrandh(ENVIRON *csound,VRANDH *p)
{
    MYFLT *vector = p->vector, *num1 = p->num1;
    MYFLT value = *p->krange;
    int elements = p->elements;

    do {
      *vector++ += *num1++ * value;
    } while (--elements);

    p->phs += (long)(*p->kcps * kicvt);
    if (p->phs >= MAXLEN) {
      p->phs &= PHMASK;
      elements = p->elements;
      vector = p->vector;
      num1 = p->num1;
      do {
        *num1++ = BiRandGab;
      } while (--elements);
    }
    return OK;
}

int vrandi_set(ENVIRON *csound,VRANDI *p)
{
    FUNC        *ftp;
    MYFLT fmaxlen = (MYFLT)MAXLEN;
    int elements = 0;
    MYFLT *dfdmax, *num1, *num2;
    if ((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL) {
      p->vector = ftp->ftable;
      elements = (p->elements = (int) *p->ielements);
    }
    else return NOTOK;
    if ( p->elements >= ftp->flen )
      return csound->InitError(csound, "vectorop: invalid num of elements");

    p->phs = 0;
    if (p->auxch.auxp == NULL)
      csound->AuxAlloc(csound, elements * sizeof(MYFLT) * 3, &p->auxch);
    num1 = (p->num1 = (MYFLT *) p->auxch.auxp);
    num2 = (p->num2 = &num1[elements]);
    dfdmax = (p->dfdmax = &num1[elements * 2]);

    do {
      *num1 = FL(0.0);
      *num2 = BiRandGab;
      *dfdmax++ = (*num2++ - *num1++) / fmaxlen;
    } while (--elements);
    return OK;
}


int vrandi(ENVIRON *csound,VRANDI *p)
{
    MYFLT *vector = p->vector, *num1 = p->num1, *num2, *dfdmax = p->dfdmax;
    MYFLT value = *p->krange;
    MYFLT fmaxlen = (MYFLT)MAXLEN;
    int elements = p->elements;

    do {
      *vector++ += (*num1++ + (MYFLT)p->phs * *dfdmax++) * value;
    } while (--elements);

    p->phs += (long)(*p->kcps * kicvt);
    if (p->phs >= MAXLEN) {
      p->phs &= PHMASK;
      elements = p->elements;
      vector = p->vector;
      num1 = p->num1;
      num2 = p->num2;
      dfdmax = p->dfdmax;

      do {
        *num1 = *num2;
        *num2 = BiRandGab ;
        *dfdmax++ = (*num2++ - *num1++) / fmaxlen;
      } while (--elements);
    }
    return OK;
}

int vecdly_set(ENVIRON *csound,VECDEL *p)
{
    FUNC        *ftp;
    int elements = (p->elements = (int) *p->ielements), j;
    long n;

    if ((ftp = csound->FTnp2Find(csound,p->ifnOut)) != NULL) {
      p->outvec = ftp->ftable;
      elements = (p->elements = (int) *p->ielements);
      if ( elements >= ftp->flen )
        return csound->InitError(csound, "vecdelay: invalid num of elements");
    }
    else return csound->InitError(csound, "vecdly: invalid output table");
    if ((ftp = csound->FTnp2Find(csound,p->ifnIn)) != NULL) {
      p->invec = ftp->ftable;
      if (elements >= ftp->flen)
        return csound->InitError(csound, "vecdelay: invalid num of elements");
    }
    else return csound->InitError(csound, "vecdly: invalid input table");
    if ((ftp = csound->FTnp2Find(csound,p->ifnDel)) != NULL) {
      p->dlyvec = ftp->ftable;
      if ( elements >= ftp->flen )
        return csound->InitError(csound, "vecdelay: invalid num of elements");
    }
    else return csound->InitError(csound, "vecdly: invalid delay table");

    n = (p->maxd = (long) (*p->imaxd * ekr));
    if (n == 0) n = (p->maxd = 1);

    if (!*p->istod) {
      if (p->aux.auxp == NULL ||
          (int)(elements * sizeof(MYFLT *)
                + n * elements * sizeof(MYFLT)
                + elements * sizeof(long)) > p->aux.size) {
        csound->AuxAlloc(csound, elements * sizeof(MYFLT *)
                 + n * elements * sizeof(MYFLT)
                 + elements * sizeof(long),
                 &p->aux);
        p->buf= (MYFLT **) p->aux.auxp;
        for (j = 0; j < elements; j++) {
          p->buf[j] = (MYFLT *) (p->aux.auxp + sizeof(MYFLT *)* elements
                                 +sizeof(MYFLT ) * n * j);
        }
        p->left= (long *)  (p->aux.auxp +sizeof(MYFLT *)* elements
                            +sizeof(MYFLT ) * n * elements);
      }
      else {
        MYFLT **buf= p->buf;
        for (j = 0; j < elements; j++) {
          MYFLT *temp = buf[j];
          int count = n;
          do {
            *temp++ = FL(0.0);
          } while (--count);
          p->left[j] = 0;
        }
      }
    }
    return OK;
}


int vecdly(ENVIRON *csound,VECDEL *p)
{
    long maxd = p->maxd, *indx=p->left, v1, v2;
    MYFLT **buf = p->buf, fv1, fv2, *inVec = p->invec;
    MYFLT *outVec = p->outvec, *dlyVec = p->dlyvec;
    int elements = p->elements;
    if (buf==NULL) {
      return csound->InitError(csound, "vecdly: not initialized");
    }
    do {
      (*buf)[*indx] = *inVec++;
      fv1 = *indx - *dlyVec++ * ekr;
      while (fv1 < FL(0.0))     fv1 += (MYFLT)maxd;
      while (fv1 >= (MYFLT)maxd) fv1 -= (MYFLT)maxd;
      if (fv1 < maxd - 1) fv2 = fv1 + 1;
      else                fv2 = FL(0.0);
      v1 = (long)fv1;
      v2 = (long)fv2;
      *outVec++ = (*buf)[v1] + (fv1 - v1) * ((*buf)[v2]-(*buf)[v1]);
      ++buf;
      if (++(*indx) == maxd) *indx = 0;
      ++indx;
    } while (--elements);
    return OK;
}


int vseg_set(ENVIRON *csound,VSEG *p)
{
    TSEG        *segp;
    int nsegs;
    MYFLT       **argp, dur, *vector;
    FUNC *nxtfunc, *curfunc, *ftp;
    long        flength;

    nsegs = ((p->INCOUNT-2) >> 1);      /* count segs & alloc if nec */

    if ((segp = (TSEG *) p->auxch.auxp) == NULL) {
      csound->AuxAlloc(csound, (long)(nsegs+1)*sizeof(TSEG), &p->auxch);
      p->cursegp = segp = (TSEG *) p->auxch.auxp;
      (segp+nsegs)->cnt = MAXPOS;
    }
    argp = p->argums;
    if ((nxtfunc = csound->FTnp2Find(csound,*argp++)) == NULL)
      return NOTOK;
    if ((ftp = csound->FTnp2Find(csound,p->ioutfunc)) != NULL) {
      p->vector = ftp->ftable;
      p->elements = (int) *p->ielements;
    }
    if ( p->elements >= ftp->flen )
      return csound->InitError(csound, "vlinseg/vexpseg: invalid num. of elements");
    vector = p->vector;
    flength = p->elements;

    do {
      *vector++ = FL(0.0);
    } while (--flength);

    if (**argp <= FL(0.0))  return NOTOK; /* if idur1 <= 0, skip init  */
    p->cursegp = segp;              /* else proceed from 1st seg */
    segp--;
    do {
      segp++;           /* init each seg ..  */
      curfunc = nxtfunc;
      dur = **argp++;
      if ((nxtfunc = csound->FTnp2Find(csound,*argp++)) == NULL) return NOTOK;
      if (dur > FL(0.0)) {
        segp->d = dur * ekr;
        segp->function =  curfunc;
        segp->nxtfunction = nxtfunc;
        segp->cnt = (long) (segp->d + .5);
      }
      else break;               /*  .. til 0 dur or done */
    } while (--nsegs);
    segp++;
    segp->d = FL(0.0);
    segp->cnt = MAXPOS;         /* set last cntr to infin */
    segp->function =  nxtfunc;
    segp->nxtfunction = nxtfunc;
    return OK;
}

int vlinseg(ENVIRON *csound,VSEG *p)
{
    TSEG        *segp;
    MYFLT       *curtab, *nxttab,curval, nxtval, durovercnt=FL(0.0), *vector;
    long        flength, upcnt;
    if (p->auxch.auxp==NULL) {
      /* csound->InitError(csound, Str(X_1270,"tableseg: not initialized")); */
      return csound->InitError(csound, "tableseg: not initialized");
    }
    segp = p->cursegp;
    curtab = segp->function->ftable;
    nxttab = segp->nxtfunction->ftable;
    upcnt = (long)segp->d-segp->cnt;
    if (upcnt > 0)
      durovercnt = segp->d/upcnt;
    while (--segp->cnt < 0)
      p->cursegp = ++segp;
    flength = p->elements;
    vector = p->vector;
    do {
      curval = *curtab++;
      nxtval = *nxttab++;
      if (durovercnt > FL(0.0))
        *vector++ = (curval + ((nxtval - curval) / durovercnt));
      else
        *vector++ = curval;
    } while (--flength);
    return OK;
}

int vexpseg(ENVIRON *csound,VSEG *p)
{
    TSEG        *segp;
    MYFLT       *curtab, *nxttab,curval, nxtval, cntoverdur=FL(0.0), *vector;
    long        flength, upcnt;

    if (p->auxch.auxp==NULL) {
      /* csound->InitError(csound, Str(X_1271,"tablexseg: not initialized")); */
      return csound->InitError(csound, "tablexseg: not initialized");
    }
    segp = p->cursegp;
    curtab = segp->function->ftable;
    nxttab = segp->nxtfunction->ftable;
    upcnt = (long)segp->d-segp->cnt;
    if (upcnt > 0) cntoverdur = upcnt/ segp->d;
    while(--segp->cnt < 0)
      p->cursegp = ++segp;
    flength = p->elements;
    vector = p->vector;
    cntoverdur *= cntoverdur;
    do {
      curval = *curtab++;
      nxtval = *nxttab++;
      *vector++ = (curval + ((nxtval - curval) * cntoverdur));
    } while (--flength);
    return OK;
}

/* ----------------------------------------------- */
int vphaseseg_set(ENVIRON *csound,VPSEG *p)
{
    TSEG2       *segp;
    int nsegs,j;
    MYFLT       **argp,  *vector;
    double dur, durtot = 0.0, prevphs;
    FUNC *nxtfunc, *curfunc, *ftp;
    long        flength;

    nsegs = p->nsegs =((p->INCOUNT-3) >> 1);    /* count segs & alloc if nec */

    if ((segp = (TSEG2 *) p->auxch.auxp) == NULL) {
      csound->AuxAlloc(csound, (long)(nsegs+1)*sizeof(TSEG), &p->auxch);
      p->cursegp = segp = (TSEG2 *) p->auxch.auxp;
      /* (segp+nsegs)->cnt = MAXPOS;  */
    }
    argp = p->argums;
    if ((nxtfunc = csound->FTnp2Find(csound,*argp++)) == NULL)
      return NOTOK;
    if ((ftp = csound->FTnp2Find(csound,p->ioutfunc)) != NULL) {
      p->vector = ftp->ftable;
      p->elements = (int) *p->ielements;
    }
    if ( p->elements > ftp->flen )
      return csound->InitError(csound, "vphaseseg: invalid num. of elements");
    vector = p->vector;
    flength = p->elements;

    do {
      *vector++ = FL(0.0);
    } while (--flength);

    if (**argp <= FL(0.0))  return NOTOK; /* if idur1 <= 0, skip init  */
    /* p->cursegp = tempsegp =segp; */      /* else proceed from 1st seg */

    segp--;
    do {
      segp++;           /* init each seg ..  */
      curfunc = nxtfunc;
      dur = **argp++;
      if ((nxtfunc = csound->FTnp2Find(csound,*argp++)) == NULL) return NOTOK;
      if (dur > FL(0.0)) {
        durtot+=dur;
        segp->d = dur; /* * ekr; */
        segp->function = curfunc;
        segp->nxtfunction = nxtfunc;
        /* segp->cnt = (long) (segp->d + .5);  */
      }
      else break;               /*  .. til 0 dur or done */
    } while (--nsegs);
    segp++;

    segp->function =  nxtfunc;
    segp->nxtfunction = nxtfunc;
    nsegs = p->nsegs;

    segp = p->cursegp;

    for (j=0; j< nsegs; j++)
      segp[j].d /= durtot;

    for (j=nsegs-1; j>= 0; j--)
      segp[j+1].d = segp[j].d;

    segp[0].d = prevphs = 0.0;

    for (j=0; j<= nsegs; j++) {
      segp[j].d += prevphs;
      prevphs = segp[j].d;
    }
    return OK;
}

int vphaseseg(ENVIRON *csound,VPSEG *p)
{

    TSEG2       *segp = p->cursegp;
    double phase = *p->kphase, partialPhase = 0.0;
    int j, flength;
    MYFLT       *curtab = NULL, *nxttab = NULL, curval, nxtval, *vector;

    while (phase >= 1.0) phase -= 1.0;
    while (phase < 0.0) phase = 0.0;

    for (j = 0; j < p->nsegs; j++) {
      TSEG2 *seg = &segp[j], *seg1 = &segp[j+1];
      if (phase < seg1->d) {
        curtab = seg->function->ftable;
        nxttab = seg1->function->ftable;
        partialPhase = (phase - seg->d) / (seg1->d - seg->d);
        break;
      }
    }

    flength = p->elements;
    vector = p->vector;
    do {
      curval = *curtab++;
      nxtval = *nxttab++;
      *vector++ = (MYFLT) (curval + ((nxtval - curval) * partialPhase));
    } while (--flength);
    return OK;
}

/* ------------------------- */
int kdel_set(ENVIRON *csound,KDEL *p)
{
    unsigned long n;
    MYFLT *buf;
    n = (p->maxd = (long) (*p->imaxd * ekr));
    if (n == 0) n = (p->maxd = 1);

    if (!*p->istod) {
      if (p->aux.auxp == NULL || (int)(n*sizeof(MYFLT)) > p->aux.size)
        csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux);
      else {
        buf = (MYFLT *)p->aux.auxp;
        do {
          *buf++ = FL(0.0);
        } while (--n);
      }
      p->left = 0;
    }
    return OK;
}

int kdelay(ENVIRON *csound,KDEL *p)
{
    long maxd = p->maxd, indx, v1, v2;
    MYFLT *buf = (MYFLT *)p->aux.auxp, fv1, fv2;

    if (buf==NULL) {
      return csound->InitError(csound, "vdelayk: not initialized");
    }

    indx = p->left;
    buf[indx] = *p->kin;
    fv1 = indx - *p->kdel * ekr;
    while (fv1 < FL(0.0))       fv1 += (MYFLT)maxd;
    while (fv1 >= (MYFLT)maxd) fv1 -= (MYFLT)maxd;
    if (*p->interp) { /*  no interpolation */
      *p->kr = buf[(long) fv1];
    }
    else {
      if (fv1 < maxd - 1) fv2 = fv1 + 1;
      else                fv2 = FL(0.0);
      v1 = (long)fv1;
      v2 = (long)fv2;
      *p->kr = buf[v1] + (fv1 - v1) * (buf[v2]-buf[v1]);
    }
    if (++(p->left) == maxd) p->left = 0;
    return OK;
}

/*  From fractals.c */
int ca_set(ENVIRON *csound,CELLA *p)
{
    FUNC        *ftp;
    int elements;
    MYFLT *currLine, *initVec = NULL;

    if ((ftp = csound->FTnp2Find(csound,p->ioutFunc)) != NULL) {
      p->outVec = ftp->ftable;
      elements = (p->elements = (int) *p->ielements);
      if ( elements >= ftp->flen )
        return csound->InitError(csound, "cella: invalid num of elements");
    }
    else return csound->InitError(csound, "cella: invalid output table");
    if ((ftp = csound->FTnp2Find(csound,p->initStateFunc)) != NULL) {
      initVec = (p->initVec = ftp->ftable);
      if ( elements >= ftp->flen )
        return csound->InitError(csound, "cella: invalid num of elements");
    }
    else return csound->InitError(csound, "cella: invalid initial state table");
    if ((ftp = csound->FTnp2Find(csound,p->iRuleFunc)) != NULL) {
      p->ruleVec = ftp->ftable;
    }
    else return csound->InitError(csound, "cella: invalid rule table");

    if (p->auxch.auxp == NULL)
      csound->AuxAlloc(csound, elements * sizeof(MYFLT) * 2, &p->auxch);
    currLine = (p->currLine = (MYFLT *) p->auxch.auxp);
    p->NewOld = 0;
    p->ruleLen = (int) *p->irulelen;
    do {
      *currLine++ = *initVec++;
    } while (--elements);
    return OK;
}


int ca(ENVIRON *csound,CELLA *p)
{
    if (*p->kreinit) {
      MYFLT *currLine = p->currLine, *initVec = p->initVec;
      int elements =  p->elements;
      p->NewOld = 0;
      do {
        *currLine++ = *initVec++;
      } while (--elements);
    }
    if (*p->ktrig) {
      int j, elements = p->elements, jm1, ruleLen = p->ruleLen;
      MYFLT *actual, *previous, *outVec = p->outVec, *ruleVec = p->ruleVec;
      previous = &(p->currLine[elements * p->NewOld]);
      p->NewOld += 1;
      p->NewOld %= 2;
      actual   = &(p->currLine[elements * p->NewOld]);
      if (*p->iradius == 1) {
        for (j=0; j < elements; j++) {
          jm1 = (j < 1) ? elements-1 : j-1;
          outVec[j] = previous[j];
          actual[j] = ruleVec[ (int) (previous[jm1] + previous[j] +
                                      previous[(j+1) % elements] ) % ruleLen];
        }
      } else {
        int jm2;
        for (j=0; j < elements; j++) {
          jm1 = (j < 1) ? elements-1 : j-1;
          jm2 = (j < 2) ? elements-2 : j-2;
          outVec[j] = previous[j];
          actual[j] = ruleVec[ (int) (previous[jm2] +   previous[jm1] +
                                      previous[j] + previous[(j+1) % elements] +
                                      previous[(j+2) % elements])
                               % ruleLen];
        }
      }

    } else {
      int elements =  p->elements;
      MYFLT *actual = &(p->currLine[elements * !(p->NewOld)]), *outVec = p->outVec;
      do {
        *outVec++ = *actual++ ;
      } while (--elements);
    }
    return OK;
}


#define S sizeof

static OENTRY localops[] = {
  { "vtablei", S(MTABLEI),   1, "",   "iiiim", (SUBR)mtable_i,  NULL },
  { "vtablek", S(MTABLE),    3, "",   "kkkiz", (SUBR)mtable_set, (SUBR)mtable_k, NULL },
  { "vtablea", S(MTABLE),    5, "",   "akkiy", (SUBR)mtable_set, NULL, (SUBR)mtable_a },
  { "vtablewi", S(MTABLEIW), 1, "",   "iiim", (SUBR)mtablew_i,  NULL },
  { "vtablewk", S(MTABLEW),  3, "",   "kkiz", (SUBR)mtablew_set, (SUBR)mtablew_k, NULL },
  { "vtablewa", S(MTABLEW),  5, "",   "akiy", (SUBR)mtablew_set, NULL, (SUBR)mtablew_a },
  { "vtabi", S(MTABI),       1, "",   "iim", (SUBR)mtab_i,  NULL },
  { "vtabk", S(MTAB),        3, "",   "kiz", (SUBR)mtab_set, (SUBR)mtab_k, NULL },
  { "vtaba", S(MTAB),        5, "",  "aiy", (SUBR)mtab_set, NULL, (SUBR)mtab_a },
  { "vtabwi", S(MTABIW),     1, "",  "iim", (SUBR)mtabw_i,  NULL },
  { "vtabwk", S(MTABW),      3, "",  "kiz", (SUBR)mtabw_set, (SUBR)mtabw_k, NULL },
  { "vtabwa", S(MTABW),      5, "",  "aiy", (SUBR)mtabw_set, NULL, (SUBR)mtabw_a },

  { "vadd",S(VECTOROP),      3, "",  "iki", (SUBR)vectorOp_set, (SUBR) vadd    },
  { "vmult",S(VECTOROP),     3, "",  "iki", (SUBR)vectorOp_set, (SUBR) vmult   },
  { "vpow",S(VECTOROP),      3, "",  "iki", (SUBR)vectorOp_set, (SUBR) vpow    },
  { "vexp",S(VECTOROP),      3, "",  "iki", (SUBR)vectorOp_set, (SUBR) vexp    },
  { "vaddv",S(VECTORSOP),    3, "",  "iii", (SUBR)vectorsOp_set, (SUBR) vaddv  },
  { "vsubv",S(VECTORSOP),    3, "",  "iii", (SUBR)vectorsOp_set, (SUBR) vsubv  },
  { "vmultv",S(VECTORSOP),   3, "",  "iii", (SUBR)vectorsOp_set, (SUBR) vmultv },
  { "vdivv",S(VECTORSOP),    3, "",  "iii", (SUBR)vectorsOp_set, (SUBR)vdivv   },
  { "vpowv",S(VECTORSOP),    3, "",  "iii", (SUBR)vectorsOp_set, (SUBR)vpowv   },
  { "vexpv",S(VECTORSOP),    3, "",  "iii", (SUBR)vectorsOp_set, (SUBR) vexpv  },
  { "vcopy",S(VECTORSOP),    3, "",  "iii", (SUBR)vectorsOp_set, (SUBR)vcopy   },
  { "vcopy_i",S(VECTORSOP),  1, "",  "iii", (SUBR)vcopy_i           },
  { "vmap",S(VECTORSOP),     3, "",  "iii", (SUBR)vectorsOp_set, (SUBR)vmap    },
  { "vlimit",S(VLIMIT),      3, "",  "ikki",(SUBR)vlimit_set, (SUBR)vlimit     },
  { "vwrap",S(VLIMIT),       3, "",  "ikki",(SUBR)vlimit_set, (SUBR) vwrap     },
  { "vmirror",S(VLIMIT),     3, "",  "ikki",(SUBR)vlimit_set, (SUBR)vmirror     },
  { "vlinseg",S(VSEG),       3, "",  "iin", (SUBR)vseg_set,   (SUBR)vlinseg     },
  { "vexpseg",S(VSEG),       3, "",  "iin", (SUBR)vseg_set,   (SUBR)vexpseg     },
  { "vrandh",S(VRANDH),      3, "",  "ikki",(SUBR)vrandh_set, (SUBR) vrandh     },
  { "vrandi",S(VRANDI),      3, "",  "ikki",(SUBR)vrandi_set, (SUBR)vrandi      },
  { "vport",S(VPORT),        3, "",  "ikio",(SUBR)vport_set,  (SUBR)vport       },
  { "vecdelay",S(VECDEL),    3, "",  "iiiiio",(SUBR)vecdly_set, (SUBR)vecdly    },
  { "vdelayk",S(KDEL),       3, "k", "kkioo",(SUBR)kdel_set,  (SUBR)kdelay      },
  { "vcella",S(CELLA),       3, "",  "kkiiiiip",(SUBR)ca_set, (SUBR)ca          },

};

LINKAGE

