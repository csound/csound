/*
    filter.c:

    Copyright (C) 1997 Michael A. Casey, John ffitch

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

/* filter.c */

/* Author: Michael A. Casey
 * Language: C
 * Copyright (C) 1997 Michael A. Casey, MIT Media Lab, All Rights Reserved
 *
 * Implementation of filter opcode for general purpose filtering.
 * This opcode implements the following difference equation:
 *
 * (1)*y(n) = b(0)*x(n) + b(1)*x(n-1) + ... + b(nb)*x(n-nb)
 *                      - a(1)*y(n-1) - ... - a(na)*y(n-na)
 *
 * whose system function is represented by:
 *
 *                             -1                -nb
 *        jw  B(z)   b(0) + b(1)z + .... + b(nb)z
 *     H(e) = ---- = ----------------------------
 *                               -1              -na
 *            A(z)    1   + a(1)z + .... + a(na)z
 *
 * The syntax is as follows:
 *
 *  xsig2 xfilter xsig1, nb, na, b(0), b(1), ..., b(nb), a(1), a(2), ..., a(na)
 *
 * xsig is either a k-rate or a-rate signal and xfilter is
 * the corresponding opcode: kfilter or filter respectively.
 * b(n) and a(n) are either i or k-rate arguments
 * nb and na are i-time arguments with the following limits:
 *
 *   1 <= nb <= 51
 *   0 <= na <= 50
 *
 * The filter is implemented with a direct form-II digital filter lattice
 * which has the advantage of requiring a single Nth-order delay line
 * [N = max(na,nb-1)], as well as negating the need for pole-zero
 * factorization which is required of type III and IV digital filter
 * lattice structures.
 *
 *
 * Examples of digital filtering using filter
 * ==========================================
 *
 * A first-order IIR filter with k-rate gain kG and k-rate pole coefficient
 * ka1 has the following syntax:
 *
 *   asig2    filter asig1, 1, 1, kG, ka1
 *
 * A first-order allpass IIR filter with i-var coefficient c has the
 * following syntax:
 *
 *   asig2   filter asig1, 2, 1, -ic, 1, ic
 *
 * A first-order linear-phase lowpass FIR filter operating on a kvar signal
 * has the following syntax:
 *
 *   ksig2   kfilter ksig1, 2, 0, 0.5, 0.5
 *
 * Potential applications of filter
 * ================================
 *
 * Since filter allows the implementation of generalised recursive filters,
 * it can be used to specify a large range of general DSP algorithms- very
 * efficiently.
 *
 * For example, a digital waveguide can be implemented for musical instrument
 * modeling using a pair of delayr and delayw opcodes in conjunction with the
 * filter opcode.
 *
 *
 * The zfilter opcode
 * ==================
 *
 * Whereas the above two opcodes, filter and kfilter, are static linear
 * time-invariant (LTI) filters, zfilter implements two pole-warping
 * operations to effect control over the filter at the k-rate. The
 * operations are radial pole-shear and angular pole-warp respectively.
 *
 * Pole shearing increases the magnitude of poles along radial lines in
 * the Z-plane. This has the affect of altering filter ring times. The
 * variable kmag is the parameters (-1:1)
 *
 * Pole warping changes the frequency of poles by moving them along angular
 * paths in the Z plane. This operation leaves the shape of the magnitude
 * response unchanged but alters the frequencies by a constant factor
 * (preserving 0 and pi). This alters the resonant frequencies of the filter
 * but leaves the "timbre" of the filter intact.
 *
 * The following example implements a forth-order all-pole model with pole
 * shear and warp factors kmag and kfreq and i-time IIR coefficient ia1...ia4.
 *
 * asig2  zfilter kmag, kfreq, 1, 4, 1, ia1, ia2, ia3, ia4
 *
 * Notice
 * ======
 *
 * This code is provided as is for the express purpose of enhancing Csound
 * with a general-purpose filter routine. No guarantees are made as to its
 * effectiveness for a particular application. The author reserves the right
 * to withdraw this code from the Csound distribution at any time without
 * notice. This code is not to be used for any purposes other than those
 * covered by the public Csound license agreement. For any other use,
 * including distribution in whole or part, specific prior permission
 * must be obtained from MIT.
 *
 */

/* Contains modifications by John ffitch, which have no restrictions attached
 * Main changes are to work in double precision internally */

#include <stdlib.h>

#include "stdopcod.h"
#include "filter.h"
#include <math.h>

typedef struct FCOMPLEX {double r,i;} fcomplex;

static double readFilter(FILTER*, int32_t);
static void insertFilter(FILTER*,double);

#ifndef MAX
#define MAX(a,b) ((a>b)?(a):(b))
#define MIN(a,b) ((a>b)?(b):(a))
#endif

/*#define POLEISH (1) */     /* 1=poleish pole roots after Laguer root finding */

typedef struct FPOLAR {double mag,ph;} fpolar;

/* Routines associated with pole control */
static void expandPoly(fcomplex[], double[], int32_t);
static void complex2polar(fcomplex[],fpolar[], int32_t);
static void polar2complex(fpolar[],fcomplex[], int32_t);
static void sortRoots(fcomplex roots[], int32_t dim);
static int32_t sortfun(fpolar *a, fpolar *b);
static void nudgeMags(fpolar a[], fcomplex b[], int32_t dim, double fact);
static void nudgePhases(fpolar a[], fcomplex b[], int32_t dim, double fact);

static void zroots(CSOUND*, fcomplex [], int32_t, fcomplex []);
static fcomplex Cadd(fcomplex, fcomplex);
static fcomplex Csub(fcomplex, fcomplex);
static fcomplex Cmul(fcomplex, fcomplex);
static fcomplex Cdiv(fcomplex, fcomplex);
static fcomplex Complex(double, double);
static double Cabs(fcomplex);
static fcomplex Csqrt(fcomplex);
static fcomplex RCmul(double, fcomplex);

/* Filter initialization routine */
static int32_t ifilter(CSOUND *csound, FILTER* p)
{
    int32_t i;

    /* since i-time arguments are not guaranteed to propegate to p-time
     * we must copy the i-vars into the p structure.
     */

    p->numa = (int32_t)*p->na;
    p->numb = (int32_t)*p->nb;

    /* First check bounds on initialization arguments */
    if (UNLIKELY((p->numb<1) || (p->numb>(MAXZEROS+1)) ||
                 (p->numa<0) || (p->numa>MAXPOLES)))
      return csound->InitError(csound, "%s", Str("Filter order out of bounds: "
                                           "(1 <= nb < 51, 0 <= na <= 50)"));

    /* Calculate the total delay in samples and allocate memory for it */
    p->ndelay = MAX(p->numb-1,p->numa);

    csound->AuxAlloc(csound, p->ndelay * sizeof(double), &p->delay);

    /* Initialize the delay line for safety  ***NOT NEEDED AS AUXALLOC DOES THAT */
    /* for (i=0;i<p->ndelay;i++) */
    /*   ((double*)p->delay.auxp)[i] = 0.0; */

    /* Set current position pointer to beginning of delay */
    p->currPos = (double*)p->delay.auxp;

    for (i=0; i<p->numb+p->numa; i++)
      p->dcoeffs[i] = (double)*p->coeffs[i];
    return OK;
}

/* izfilter - initialize z-plane controllable filter */
static int32_t izfilter(CSOUND *csound, ZFILTER *p)
{
    fcomplex a[MAXPOLES];
    fcomplex *roots;
    double *coeffs;
    int32_t i, dim;

    /* since i-time arguments are not guaranteed to propagate to p-time
     * we must copy the i-vars into the p structure.
     */

    p->numa = (int32_t)*p->na;
    p->numb = (int32_t)*p->nb;

    /* First check bounds on initialization arguments */
    if (UNLIKELY((p->numb<1) || (p->numb>(MAXZEROS+1)) ||
                 (p->numa<0) || (p->numa>MAXPOLES)))
      return csound->InitError(csound, "%s", Str("Filter order out of bounds: "
                                           "(1 <= nb < 51, 0 <= na <= 50)"));

    /* Calculate the total delay in samples and allocate memory for it */
    p->ndelay = MAX(p->numb-1,p->numa);

    csound->AuxAlloc(csound, p->ndelay * sizeof(double), &p->delay);

    /* Set current position pointer to beginning of delay */
    p->currPos = (double*)p->delay.auxp;

    for (i=0; i<p->numb+p->numa; i++)
      p->dcoeffs[i] = (double)*p->coeffs[i];

    /* Add auxillary root memory */
    csound->AuxAlloc(csound, p->numa * sizeof(fcomplex), &p->roots);
    roots = (fcomplex*) p->roots.auxp;
    dim = p->numa;

    coeffs = p->dcoeffs + p->numb;

    /* Reverse coefficient order for root finding */
    a[dim] = Complex(1.0,0.0);
    for (i=dim-1; i>=0; i--)
      a[i] = Complex(coeffs[dim-i-1],0.0);

    /* NRIC root finding routine, a[0..M] roots[1..M] */
    zroots(csound, a, dim,  roots-1/*POLEISH*/);

    /* Sort roots into descending order of magnitudes */
    sortRoots(roots, dim);
    return OK;
}

/* a-rate filter routine
 *
 * Implements the following difference equation
 *
 * (1)*y(n) = b(0)*x(n) + b(1)*x(n-1) + ... + b(nb)*x(n-nb)
 *                      - a(1)*y(n-1) - ... - a(na)*y(n-na)
 *
 */
static int32_t afilter(CSOUND *csound, FILTER* p)
{
     IGN(csound);
    int32_t      i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    double* a = p->dcoeffs+p->numb;
    double* b = p->dcoeffs+1;
    double  b0 = p->dcoeffs[0];

    double poleSamp, zeroSamp, inSamp;

    /* Outer loop */
    if (UNLIKELY(offset)) memset(p->out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {

      inSamp = p->in[n];
      poleSamp = inSamp;
      zeroSamp = 0.0;

      /* Inner filter loop */
      for (i=0; i< p->ndelay; i++) {

        /* Do poles first */
        /* Sum of products of a's and delays */
        if (i<p->numa)
          poleSamp += -(a[i])*readFilter(p,i+1);

        /* Now do the zeros */
        if (i<(p->numb-1))
          zeroSamp += (b[i])*readFilter(p,i+1);

      }

      p->out[n] = (MYFLT)((b0)*poleSamp + zeroSamp);
      /* update filter delay line */
      insertFilter(p, poleSamp);
    }
    return OK;
}

/* k-rate filter routine
 *
 * Implements the following difference equation at the k rate
 *
 * (1)*y(k) = b(0)*x(k) + b(1)*x(k-1) + ... + b(nb)*x(k-nb)
 *                      - a(1)*y(k-1) - ... - a(na)*y(k-na)
 *
 */
static int32_t kfilter(CSOUND *csound, FILTER* p)
{
     IGN(csound);
    int32_t i;

    double* a = p->dcoeffs+p->numb;
    double* b = p->dcoeffs+1;
    double  b0 = p->dcoeffs[0];

    double poleSamp, zeroSamp, inSamp;

    inSamp = *p->in;
    poleSamp = inSamp;
    zeroSamp = 0.0;

    /* Filter loop */
    for (i=0; i<p->ndelay; i++) {

      /* Do poles first */
      /* Sum of products of a's and delays */
      if (i<p->numa)
        poleSamp += -(a[i])*readFilter(p,i+1);

      /* Now do the zeros */
      if (i<(p->numb-1))
        zeroSamp += (b[i])*readFilter(p,i+1);
    }

    *p->out = (MYFLT)((b0)*poleSamp + zeroSamp);

    /* update filter delay line */
    insertFilter(p, poleSamp);
    return OK;
}

/* azfilter - a-rate controllable pole filter
 *
 * This filter allows control over the magnitude
 * and frequency response of the filter by efficient
 * manipulation of the poles.
 *
 * The k-rate controls are:
 *
 * kmag, kfreq
 *
 * The rest of the filter is the same as filter
 *
 */
static int32_t azfilter(CSOUND *csound, ZFILTER* p)
{
     IGN(csound);
    int32_t      i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    double* a = p->dcoeffs+p->numb;
    double* b = p->dcoeffs+1;
    double  b0 = p->dcoeffs[0];

    double poleSamp, zeroSamp, inSamp;

    fpolar B[MAXPOLES];
    fcomplex C[MAXPOLES+1];

    fcomplex *roots = (fcomplex*) p->roots.auxp;
    double kmagf = *p->kmagf; /* Mag nudge factor */
    double kphsf = *p->kphsf; /* Phs nudge factor */

    int32_t dim = p->numa;

    /* Nudge pole magnitudes */
    complex2polar(roots,B,dim);
    nudgeMags(B,roots,dim,kmagf);
    nudgePhases(B,roots,dim,kphsf);
    polar2complex(B,C,dim);
    expandPoly(C,a,dim);

    /* C now contains the complex roots of the nudged filter */
    /* and a contains their associated real coefficients. */

    /* Outer loop */
    if (UNLIKELY(offset)) memset(p->out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      inSamp = p->in[n];
      poleSamp = inSamp;
      zeroSamp = 0.0;

      /* Inner filter loop */
      for (i=0; i< p->ndelay; i++) {

        /* Do poles first */
        /* Sum of products of a's and delays */
        if (i<p->numa)
          poleSamp += -(a[i])*readFilter((FILTER*)p,i+1);

        /* Now do the zeros */
        if (i<(p->numb-1))
          zeroSamp += (b[i])*readFilter((FILTER*)p,i+1);
      }

      p->out[n] = (MYFLT)((b0)*poleSamp + zeroSamp);

      /* update filter delay line */
      insertFilter((FILTER*)p, poleSamp);
    }
    return OK;
}

/* readFilter -- delay-line access routine
 *
 * Reads sample x[n-i] from a previously established delay line.
 * With this syntax i is +ve for a time delay and -ve for a time advance.
 *
 * The use of explicit indexing rather than implicit index incrementing
 * allows multiple lattice structures to access the same delay line.
 *
 */
static double readFilter(FILTER* p, int32_t i)
{
    double* readPoint; /* Generic pointer address */

    /* Calculate the address of the index for this read */
    readPoint = p->currPos - i;

    /* Wrap around for time-delay if necessary */
    if (readPoint < ((double*)p->delay.auxp) )
      readPoint += p->ndelay;
    else
      /* Wrap for time-advance if necessary */
      if (readPoint > ((double*)p->delay.auxp + (p->ndelay-1)) )
        readPoint -= p->ndelay;

    return *readPoint; /* Dereference read address for delayed value */
}

/* insertFilter -- delay-line update routine
 *
 * Inserts the passed value into the currPos and increments the
 * currPos pointer modulo the length of the delay line.
 *
 */
static void insertFilter(FILTER* p, double val)
{
    /* Insert the passed value into the delay line */
    *p->currPos = val;

    /* Update the currPos pointer and wrap modulo the delay length */
    if (((double*) (++p->currPos)) >
        ((double*)p->delay.auxp + (p->ndelay-1)) )
      p->currPos -= p->ndelay;
}

/* Compute polynomial coefficients from the roots */
/* The expanded polynomial is computed as a[0..N] in
 * descending powers of Z
 */
static void expandPoly(fcomplex roots[], double a[], int32_t dim)
{
    int32_t j,k;
    fcomplex z[MAXPOLES],d[MAXPOLES];

    z[0] = Complex(1.0, 0.0);
    for (j=1;j<=dim;j++)
      z[j] = Complex(0.0,0.0);

    /* Recursive coefficient expansion about the roots of A(Z) */
    for (j=0;j<dim;j++) {
      for (k=0;k<dim;k++)
        d[k]=z[k]; /* Store last vector of coefficients */
      for (k=1;k<=j+1;k++)
        z[k] = Csub(z[k],Cmul(roots[j], d[k-1]));
    }
    for (j=0;j<dim;j++)
      (a[j]) = z[j+1].r;
}

#define SQR(a) (a*a)

static void complex2polar(fcomplex a[], fpolar b[], int32_t N)
{
    int32_t i;

    for (i=0; i<N; i++) {
      b[i].mag = hypot(a[i].r,a[i].i);
      b[i].ph = atan2(a[i].i,a[i].r);
    }
}

static void polar2complex(fpolar a[], fcomplex b[],int32_t N)
{
    int32_t i;

    for (i=0;i<N;i++) {
      b[i].r = a[i].mag*cos(a[i].ph);
      b[i].i = a[i].mag*sin(a[i].ph);
    }
}

/* Sort poles in decreasing order of magnitudes */
static void sortRoots(fcomplex roots[], int32_t dim)
{
    fpolar plr[MAXPOLES];

    /* Convert roots to polar form */
    complex2polar(roots, plr, dim);

    /* Sort by their magnitudes */
    qsort(plr, dim, sizeof(fpolar),
          (int32_t(*)(const void *, const void * ))sortfun);

    /* Convert back to complex form */
    polar2complex(plr,roots,dim);

}

/* Comparison function for sorting in DECREASING order */
static int32_t sortfun(fpolar *a, fpolar *b)
{
    if (a->mag<b->mag)
      return 1;
    else if (a->mag==b->mag)
      return 0;
    else
      return -1;
}

/* nudgeMags - Pole magnitude nudging routine
 *
 * Find the largest-magnitude pole off the real axis
 * and nudge all non-real poles by a factor of the distance
 * of the largest pole to the unit circle (or zero if fact is -ve).
 *
 * This has the effect of changing the time-response of the filter
 * without affecting the overall frequency response characteristic.
 *
 */
static void nudgeMags(fpolar a[], fcomplex b[], int32_t dim, double fact)
{
    double eps = .000001; /* To avoid underflow comparisons */
    double nudgefact;
    int32_t i;

    /* Check range of nudge factor */
    if (fact>0 && fact<=1) {
      /* The largest magnitude pole will be at the beginning of
       * the array since it was previously sorted by the init routine.
       */
      for (i=0;i<dim;i++)
        if (fabs(b[i].i)>eps) /* Check if pole is complex */
          break;

      nudgefact = 1 + (1/a[i].mag-1)*fact;

      /* Nudge all complex-pole magnitudes by this factor */
      for (i=dim-1;i>=0;i--)
        if (fabs(b[i].i)>eps)
          a[i].mag *= nudgefact;
    }
    else if (fact < 0 && fact >=-1) {

      nudgefact = (fact + 1);

      /* Nudge all complex-pole magnitudes by this factor */
      for (i=dim-1;i>=0;i--)
        if (fabs(b[i].i)>eps)
          a[i].mag *= nudgefact;
    }
    else {
      /* Factor is out of range, do nothing */
    }
}

/* nudgePhases - Pole phase nudging routine
 *
 * Multiply phases of all poles by factor
 */
static void nudgePhases(fpolar a[], fcomplex b[], int32_t dim, double fact)
{
    double eps = .000001; /* To avoid underflow comparisons */
    double nudgefact;
    int32_t i;
    double phmax=0.0;

    /* Check range of nudge factor */
    if (fact>0 && fact<=1) {
      /* Find the largest angled non-real pole */
      for (i=0;i<dim;i++)
        if (a[i].ph>phmax)
          phmax = a[i].ph;

      phmax /= PI; /* Normalize to radian frequency */

      nudgefact = 1 + (1-phmax)*fact;

      /* Nudge all complex-pole magnitudes by this factor */
      for (i=dim-1;i>=0;i--)
        if (fabs(b[i].i)>eps)
          a[i].ph *= nudgefact;
    }
    else if (fact < 0 && fact >=-1) {
      nudgefact = (fact + 1);

      /* Nudge all complex-pole magnitudes by this factor */
      for (i=dim-1;i>=0;i--)
        if (fabs(b[i].i)>eps)
          a[i].ph *= nudgefact;
    }
    else {
      /* Factor is out of range, do nothing */
    }
}

/* ------------------------------------------------------------ */

/* Code from Press, Teukolsky, Vettering and Flannery
 * Numerical Recipes in C, 2nd Edition, Cambridge 1992.
 */

#define EPSS (1.0e-7)
#define MR (8)
#define MT (10)
#define MAXIT (MT*MR)

/* Simple definition is sufficient */
#define FPMAX(a,b) (a>b ? a : b)

static void laguer(CSOUND *csound, fcomplex a[], int32_t m,
                   fcomplex *x, int32_t *its)
{
    int32_t iter,j;
    double abx,abp,abm,err;
    fcomplex dx,x1,b,d,f,g,h,sq,gp,gm,g2;
    static const double frac[MR+1] = {0.0,0.5,0.25,0.75,0.13,0.38,0.62,0.88,1.0};

    for (iter=1; iter<=MAXIT; iter++) {
      *its = iter;
      b = a[m];
      err = Cabs(b);
      d = f = Complex(0.0,0.0);
      abx = Cabs(*x);
      for (j=m-1; j>=0; j--) {
        f = Cadd(Cmul(*x,f),d);
        d = Cadd(Cmul(*x,d),b);
        b = Cadd(Cmul(*x,b),a[j]);
        err = Cabs(b)+abx*err;
      }
      err *= (double)EPSS;
      if (Cabs(b) <= err) return;
      g = Cdiv(d,b);
      g2 = Cmul(g,g);
      h = Csub(g2,RCmul(2.0,Cdiv(f,b)));
      sq = Csqrt(RCmul((double) (m-1),Csub(RCmul((double) m,h),g2)));
      gp = Cadd(g,sq);
      gm = Csub(g,sq);
      abp = Cabs(gp);
      abm = Cabs(gm);
      if (abp < abm) gp = gm;
      dx = ((FPMAX(abp,abm) > 0.0 ? Cdiv(Complex((double) m,0.0),gp)
           : RCmul(exp(log(1.0+abx)),
                   Complex(cos((double)iter),
                           sin((double)iter)))));
      x1 = Csub(*x,dx);
      if (x->r == x1.r && x->i == x1.i) return;
      if (iter % MT) *x = x1;
      else *x = Csub(*x,RCmul(frac[iter/MT],dx));
    }
    csound->Warning(csound, "%s", Str("too many iterations in laguer"));
    return;
}
#undef EPSS
#undef MR
#undef MT
#undef MAXIT
/* (C) Copr. 1986-92 Numerical Recipes Software *%&&"U^3. */

/* ------------------------------------------------------------ */

/* Code from Press, Teukolsky, Vettering and Flannery
 * Numerical Recipes in C, 2nd Edition, Cambridge 1992.
 */

#define EPS (2.0e-6)
#define MAXM (100)

static void zroots(CSOUND *csound,fcomplex a[], int32_t m, fcomplex roots[])
{
    int32_t i,its,j,jj;
    fcomplex x,b,c,ad[MAXM];

    for (j=0; j<=m; j++) ad[j] = a[j];
    for (j=m; j>=1; j--) {
      x = Complex(0.0,0.0);
      laguer(csound,ad,j,&x,&its);
      if (fabs(x.i) <= 2.0*EPS*fabs(x.r)) x.i = 0.0;
      roots[j] = x;
      b = ad[j];
      for (jj=j-1; jj>=0; jj--) {
        c = ad[jj];
        ad[jj] = b;
        b = Cadd(Cmul(x,b),c);
      }
    }
    /*    if (poleish) */
    for (j=1; j<=m; j++)
      laguer(csound,a,m,&roots[j],&its);
    for (j=2; j<=m; j++) {
      x = roots[j];
      for (i=j-1; i>=1; i--) {
        if (roots[i].r <= x.r) break;
        roots[i+1] = roots[i];
      }
      roots[i+1] = x;
    }
}
#undef EPS
#undef MAXM
/* (C) Copr. 1986-92 Numerical Recipes Software *%&&"U^3. */

/* Code from Press, Teukolsky, Vettering and Flannery
 * Numerical Recipes in C, 2nd Edition, Cambridge 1992.
 */

static fcomplex Cadd(fcomplex a, fcomplex b)
{
    fcomplex c;
    c.r = a.r+b.r;
    c.i = a.i+b.i;
    return c;
}

static fcomplex Csub(fcomplex a, fcomplex b)
{
    fcomplex c;
    c.r = a.r-b.r;
    c.i = a.i-b.i;
    return c;
}

static fcomplex Cmul(fcomplex a, fcomplex b)
{
    fcomplex c;
    c.r = a.r*b.r-a.i*b.i;
    c.i = a.i*b.r+a.r*b.i;
    return c;
}

static fcomplex Complex(double re, double im)
{
    fcomplex c;
    c.r = re;
    c.i = im;
    return c;
}

/* fcomplex Conjg(fcomplex z) */
/* { */
/*     fcomplex c; */
/*     c.r = z.r; */
/*     c.i = -z.i; */
/*     return c; */
/* } */

static fcomplex Cdiv(fcomplex a, fcomplex b)
{
    fcomplex c;
    double r,den;
    if (fabs(b.r) >= fabs(b.i)) {
      r   = b.i/b.r;
      den = b.r+r*b.i;
      c.r = (a.r+r*a.i)/den;
      c.i = (a.i-r*a.r)/den;
    }
    else {
      r   = b.r/b.i;
      den = b.i+r*b.r;
      c.r = (a.r*r+a.i)/den;
      c.i = (a.i*r-a.r)/den;
    }
    return c;
}

static double Cabs(fcomplex z)
{
    double x,y,ans;
    double temp;
    x = fabs(z.r);
    y = fabs(z.i);
    if (x == 0.0)
      ans  = y;
    else if (y == 0.0)
      ans  = x;
    else if (x > y) {
      temp = (y/x);
      ans  = x*sqrt(1.0+temp*temp);
    }
    else {
      temp = (x/y);
      ans  = y*sqrt(1.0+temp*temp);
    }
    return ans;
}

static fcomplex Csqrt(fcomplex z)
{
    fcomplex c;
    double w;
    double x,y,r;
    if ((z.r == 0.0) && (z.i == 0.0)) {
      c.r = 0.0;
      c.i = 0.0;
      return c;
    }
    else {
      x = fabs(z.r);
      y = fabs(z.i);
      if (x >= y) {
        r   = y/x;
        w   = sqrt(x)*sqrt(0.5*(1.0+sqrt(1.0+r*r)));
      }
      else {
        r   = x/y;
        w   = sqrt(y)*sqrt(0.5*(r+sqrt(1.0+r*r)));
      }
      if (z.r >= 0.0) {
        c.r = w;
        c.i = z.i/(2.0*w);
      } else {
        c.i = (z.i >= 0.0) ? w : -w;
        c.r = z.i/(2.0*c.i);
      }
      return c;
    }
}

static fcomplex RCmul(double x, fcomplex a)
{
    fcomplex c;
    c.r = x*a.r;
    c.i = x*a.i;
    return c;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "filter2",0xffff,                                                     },
{ "filter2.a",  S(FILTER), 0,  "a", "aiim", (SUBR)ifilter, (SUBR)afilter},
{ "filter2.k", S(FILTER), 0,   "k", "kiim", (SUBR)ifilter, (SUBR)kfilter,NULL },
{ "zfilter2", S(ZFILTER), 0,   "a", "akkiim", (SUBR)izfilter, (SUBR)azfilter}
};

int32_t filter_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
