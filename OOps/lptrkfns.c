/*
    lptrkfns.c:

    Copyright (C) 1992 Barry Vercoe, John ffitch

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

#include <stdio.h>                                        /*    LPTRKFNS.C   */
#include "cs.h"
#include "lpc.h"
#include <math.h>

#define FREQS  50
#define NN     5
#define NP     6  /* NN+1 */
#define HWIN   50 /* MAXWINDIN/20, Max Hwind */

static MYFLT   *tphi[FREQS],*tpsi[FREQS]; /* prv tphi[50][5][25],tpsi[50][6][25] */
static MYFLT   *tgamph[FREQS], *tgamps[FREQS], freq[FREQS];
          /* prv tgamph[50][5],  tgamps[50][6] */
static MYFLT    NYQ10;
static int      Windsiz, Windsiz2;         /* settable windowsize, halfthat */
static int      Dwind, Hwind;              /* settable downsamp10, halfthat */

typedef MYFLT (*phi_typ)[HWIN];
typedef MYFLT (*psi_typ)[HWIN];
static  void   trigpo(MYFLT, phi_typ, psi_typ, MYFLT *, MYFLT *, int);
static  MYFLT  lowpass(MYFLT);
static  MYFLT  search(MYFLT *fm, MYFLT qsum, MYFLT g[], MYFLT h[]);

void ptable(MYFLT fmin, MYFLT fmax, MYFLT sr, int windsiz)
{
    int   i, n;
    MYFLT omega, fstep, tpidsrd10;

    if ((n = HWIN * 20) != MAXWINDIN)
      die(Str(X_324,"LPTRKFNS: inconsistent MAXWindow defines"));
    NYQ10   = sr/FL(20.0);
    Windsiz = windsiz;              /* current windin size */
    Windsiz2 = windsiz/2;           /* half of that        */
    Dwind   = windsiz/10;           /* downsampled windsiz */
    Hwind   = (Dwind+1)/2;          /* half of that        */
    if (Hwind > HWIN)
      die(Str(X_323,"LPTRKFNS: called with excessive Windsiz"));
    tpidsrd10 = TWOPI_F / (sr/FL(10.0));
    fstep = (fmax - fmin) / FREQS;    /* alloc & init each MYFLT array  */
    for (i=0;  i<FREQS; ++i) {        /*   as if MAX dimension of Hwind */
      tphi[i] = (MYFLT *) mcalloc((long)NN * HWIN * sizeof(MYFLT));
      tpsi[i] = (MYFLT *) mcalloc((long)NP * HWIN * sizeof(MYFLT));
      tgamph[i] = (MYFLT *) mcalloc((long)NN * sizeof(MYFLT));
      tgamps[i] = (MYFLT *) mcalloc((long)NP * sizeof(MYFLT));
      freq[i] = fmin + (MYFLT)i * fstep;
      n = (int)(NYQ10 / freq[i]);
      if (n > NN)  n = NN;
      omega = freq[i] * tpidsrd10;
      trigpo(omega,(phi_typ)tphi[i],(psi_typ)tpsi[i],tgamph[i],tgamps[i],n);
    }
}

static void trigpo(MYFLT omega,
                   phi_typ phi, psi_typ psi, MYFLT *gamphi, MYFLT *gampsi, int n)
/* dimensions:   phi[NN][HWIN], psi[NP][HWIN], gamphi[NN], gampsi[NP]  */
{
    int    j=0, k, np;
    double alpha, beta, gamma, wcos[HWIN], wsin[HWIN];
    double p, z, a, b, yy;

    np = n+1;
    for (k=0;  k<Hwind;  ++k) {
      yy = omega * (MYFLT)k;
      wcos[k] = cos(yy);
      wsin[k] = sin(yy);
    }
    beta = gamma = 0.0;
    for (k=0;  k<Hwind;  ++k) {
      p = wsin[k];
      z = p * p;
      beta += z * wcos[k];
      gamma += z;
      phi[0][k] = (MYFLT)p;
    }
    gamphi[0] = (MYFLT)gamma;
    a = 2.0 * beta/gamma;
    alpha = beta = gamma = 0.0;
    for (k=0;  k<Hwind;  ++k) {
      p = (2.0 * wcos[k]-a) * phi[0][k];
      alpha += wcos[k] * p * phi[0][k];
      beta += wcos[k] * ( p * p );
      gamma +=  p * p;
      phi[1][k] = (MYFLT)p;
    }
    gamphi[1] = (MYFLT)gamma;
    a = 2.0 * beta/gamma;
    b = 2.0 *alpha/gamphi[0];
    for (j=2;  j<n;  ++j) {
      alpha = beta = gamma = 0.0;
      for (k=0;  k< Hwind;  ++k)  {
        p = (2.0 * wcos[k] - a ) * phi[j-1][k] - b * phi[j-2][k];
        alpha += wcos[k] * p * phi[j-1][k];
        beta += wcos[k] * (p * p);
        gamma += (p * p);
        phi[j][k] = (MYFLT)p;
      }
      gamphi[j] = (MYFLT)gamma;
      a = 2.0 * beta/gamma;
      b = 2.0 *alpha/gamphi[j-1];
    }
    beta = 0.0;
    gamma = (double) Hwind;
    for ( k=0; k < Hwind;  ++k) {
      beta += wcos[k];
      psi[0][k] = FL(1.0);
    }
    gampsi[0] = (MYFLT)gamma;
    a = beta/gamma;
    alpha = beta = gamma = 0.0;
    for ( k=0;  k < Hwind;  ++k) {
      p = wcos[k]-a;
      alpha += wcos[k] * p*psi[0][k];
      beta += wcos[k] * ( p * p );
      gamma += (p * p);
      psi[1][k] = (MYFLT)p;
    }
    gampsi[1] = (MYFLT)gamma;
    a = 2.0 * beta / gamma;
    b = 2.0 * alpha / gampsi[0];
    for (j=2;  j<np; ++j) {
      alpha = beta = gamma = 0.0;
      for (k=0; k < Hwind;  ++k) {
        p = (2.0 * wcos[k]-a)* psi[j-1][k]-b*psi[j-2][k];
        alpha += wcos[k]*p*psi[j-1][k];
        beta += wcos[k]* (p*p);
        gamma += (p*p);
        psi[j][k] = (MYFLT)p;
      }
      gampsi[j] = (MYFLT)gamma;
      a = 2.0 * beta/gamma;
      b = 2.0 * alpha/gampsi[j-1];
    }
}

MYFLT getpch(MYFLT *sigbuf)
{
static  int   firstcall = 1, tencount = 0;
static  MYFLT *Dwind_dbuf, *Dwind_end1;    /* double buffer for downsamps   */
static  MYFLT *dbp1, *dbp2;
        MYFLT g[HWIN], h[HWIN], fm, qsum, y, *inp;
        int   n;

        if (firstcall) {                   /* on first call, alloc dbl dbuf  */
          Dwind_dbuf = (MYFLT *) mcalloc((long)Dwind * 2 * sizeof(MYFLT));
          Dwind_end1 = Dwind_dbuf + Dwind;
          dbp1 = Dwind_dbuf;             /*   init the local Dsamp pntrs */
          dbp2 = Dwind_end1;             /*   & process the whole inbuf  */
          for (inp = sigbuf, n = Windsiz; n--; ) {
            y = lowpass(*inp++);            /* lowpass every sample  */
            if (++tencount == 10) {
              tencount = 0;
              *dbp1++ = y;                /*    & save every 10th  */
              *dbp2++ = y;
              if (dbp1 >= Dwind_end1) {
                dbp1 = Dwind_dbuf;
                dbp2 = Dwind_end1;
              }
            }
          }
          firstcall = 0;
        }
        else {                           /* other calls: process only inbuf2  */
          for (inp = sigbuf+Windsiz2, n = Windsiz2; n--; ) {
            y = lowpass(*inp++);            /* lowpass every sample  */
            if (++tencount == 10) {
              tencount = 0;
              *dbp1++ = y;                /*    & save every 10th  */
              *dbp2++ = y;
              if (dbp1 >= Dwind_end1) {
                dbp1 = Dwind_dbuf;
                dbp2 = Dwind_end1;
              }
            }
          }
        }
        {
          MYFLT *gp, *hp, *sp1, *sp2;
          qsum = FL(0.0);
          gp = g; hp = h;
          sp1 = sp2 = dbp1 + Hwind - 1;
          for (n = Hwind; n--; gp++, hp++, sp1++, sp2-- ) {
            *gp = FL(0.5) * (*sp1 - *sp2);        /* get sum & diff pairs */
            *hp = FL(0.5) * (*sp1 + *sp2);
            qsum += *gp * *gp + *hp * *hp;   /* accum sum of squares */
          }
        }
        return ( search(&fm, qsum, g, h) );
}

static MYFLT search(MYFLT *fm, MYFLT qsum, MYFLT g[], MYFLT h[])
{
    MYFLT fun[FREQS], funmin = FL(1.e10);
    MYFLT sum, f1, f2, f3, x0, x1, x2, x3, a, b, c, ftemp;
    int   i, istar = 0, n, np, j, k;

    for (i=0;  i < FREQS;  ++i) {
      MYFLT (*tphii)[HWIN], (*tpsii)[HWIN];
      MYFLT  *tgamphi, *tgampsi;
      tphii = (MYFLT (*)[HWIN]) tphi[i];    /* dim [][NN][HWIN] */
      tpsii = (MYFLT (*)[HWIN]) tpsi[i];    /* dim [][NP][HWIN] */
      tgamphi = tgamph[i];                  /* dim [][NN]       */
      tgampsi = tgamps[i];                  /* dim [][NP]       */
      n = (int)(NYQ10 / freq[i]);
      if (n > NN)  n = NN;
      np = n+1;
      sum = FL(0.0);
      for (j=0;  j < n;  ++j) {
        c = FL(0.0);
        for (k=0;  k< Hwind;  ++k)
          c += g[k] * tphii[j][k];
        sum += (c*c) / tgamphi[j];
      }
      for (j=0;  j<np;  ++j) {
        c = FL(0.0);
        for (k=0;  k < Hwind;  ++k)
          c += h[k] * tpsii[j][k];
        sum += (c*c) / tgampsi[j];
      }
      fun[i] = ftemp = qsum - sum;      /* store the least sqr vals */
      /*    printf("qsum %f  sum %f  ftemp %f\n", qsum, sum, ftemp);   */
      if (ftemp < funmin) {
        funmin = ftemp;               /*   but remember minimum   */
        istar = i;
      }
    }
    if (istar == 0 || istar == 49) {
      *fm = fun[istar];
      return (freq[istar]);
    }
    x1 = freq[istar-1];
    f1 = fun[istar-1];
    x2 = freq[istar];
    f2 = fun[istar];
    x3 = freq[istar+1];
    f3 = fun[istar+1];
    a = f3/((x3-x1)*(x3-x2));
    b = f2/((x2-x1)*(x2-x3));
    c = f1/((x1-x2)*(x1-x3));
    x0 = FL(0.5)*(a*(x1+x2)+b*(x1+x3)+c*(x2+x3))/(a+b+c);
    *fm = a*(x0-x1)*(x0-x2)+b*(x0-x1)*(x0-x3)+c*(x0-x2)*(x0-x3);
    return (x0);
}

static MYFLT lowpass(MYFLT x)           /* x now MYFLT */
{
#define c FL(0.00048175311)

#define a1 -FL(1.89919924)
#define c1 -FL(1.92324804)
#define d1 FL(0.985720370)

#define a2 -FL(1.86607670)
#define c2 -FL(1.90075003)
#define d2 FL(0.948444690)

#define a3 -FL(1.66423461)
#define c3 -FL(1.87516686)
#define d3 FL(0.896241023)

#define c4 -FL(0.930449120)

static  MYFLT w1 = FL(0.0), w11 = FL(0.0), w12 = FL(0.0);
static  MYFLT w2 = FL(0.0), w21 = FL(0.0), w22 = FL(0.0);
static  MYFLT w3 = FL(0.0), w31 = FL(0.0), w32 = FL(0.0);
static  MYFLT w4 = FL(0.0), w41 = FL(0.0), w42 = FL(0.0);
        MYFLT temp,y;

        w1 = c*x - c1*w11 - d1*w12;
        temp = w1 + a1*w11 + w12;
        w12 = w11;
        w11 = w1;
        w2 = temp - c2*w21 - d2*w22;
        temp = w2 + a2*w21 + w22;
        w22 = w21;
        w21 = w2;
        w3 = temp - c3*w31 - d3*w32;
        temp = w3 + a3*w31 + w32;
        w32 = w31;
        w31 = w3;
        w4 = temp - c4*w41;
        y = w4 + w41;
        w42 = w41;   /* w42 set but not used in lowpass */
        w41 = w4;
        return(y);
}

