/*
    ambicode.c:

    Copyright (C) 2005 Samuel Groner,
    Institute for Computer Music and Sound Technology, www.icst.net

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

#include "csdl.h"
#include <math.h>

typedef struct {
    OPDS    h;                                      /* required header */
    MYFLT   *mw, *mx, *my, *mz, *mr, *ms, *mt, *mu, *mv, *mk,
            *ml, *mm, *mn, *mo, *mp, *mq;           /* addr outarg */
  MYFLT   *asig, *kalpha, *kbeta, *kin[4];          /* addr inargs */
    /* private dataspace */
    double  w, x, y, z, r, s, t, u, v, k, l, m, n, o, p, q;
} AMBIC;

typedef struct {
    OPDS    h;                                      /* required header */
    MYFLT   *m0, *m1, *m2, *m3, *m4, *m5, *m6, *m7; /* addr outarg */
    MYFLT   *isetup, *aw, *ax, *ay, *a[VARGMAX];    /* addr inargs */
    /* private dataspace */
    double  w[8], x[8], y[8], z[8], r[8], s[8], t[8], u[8],
            v[8], k[8], l[8], m[8], n[8], o[8], p[8], q[8];
} AMBID;

static int iambicode(CSOUND *csound, AMBIC *p)
{
    csound->Warning(csound,
                    Str("bformenc is deprecated; use bformenc1 instead\n"));
    /* check correct number of input and output arguments */
    switch (p->OUTOCOUNT) {
      case 4:
/*         { */
/*           /\* 2nd order *\/ */
/*           if (p->INOCOUNT != 5) { */
/*             return csound->InitError(csound, */
/*                                      Str("Wrong number of input arguments! " */
/*                                          "5 needed!")); */
/*           } */
/*           break; */
/*         } */

      case 9:
/*         { */
/*           /\* 3rd order *\/ */
/*           if (p->INOCOUNT != 6) { */
/*             return csound->InitError(csound, */
/*                                      Str("Wrong number of input arguments! " */
/*                                          "6 needed!")); */
/*           } */
/*           break; */
/*         } */

      case 16:
/*         { */
/*           /\* 4th order *\/ */
/*           if (p->INOCOUNT != 7) { */
/*             return csound->InitError(csound, */
/*                                      Str("Wrong number of input arguments! " */
/*                                          "7 needed!")); */
/*           } */
/*           break; */
/*         } */
        break;
      default:
        {
          return csound->InitError(csound,
                                   Str("Wrong number of output arguments! "
                                       "4, 9 or 16 needed!"));
        }
    }
    return OK;
}

static void ambicode_set_coefficients(AMBIC *p)
{
    /* convert degrees to radian */
    /* 0.017 = pi/180 */
    double kalpha_rad = (double)(*p->kalpha)*0.0174532925199432957692369076848861;
    double kbeta_rad = (double)(*p->kbeta)*0.0174532925199432957692369076848861;

    /* calculate ambisonic coefficients (Furse-Malham-set) */

    /* 0th order */
    p->w = 0.707106781186547524400844362104849 /* (1.0 / sqrt(2.0)) */ ;

    /* 1st order */
    {
      double ck = cos(kbeta_rad);
      p->x = cos(kalpha_rad) * ck;
      p->y = sin(kalpha_rad) * ck;
      p->z = sin(kbeta_rad);
    }

    /* 2nd order */
    p->r = 0.5 * (3.0 * p->z * p->z - 1.0);
    p->s = 2.0 * p->x * p->z;
    p->t = 2.0 * p->y * p->z;
    p->u = p->x * p->x - p->y * p->y;
    p->v = 2.0 * p->x * p->y;

    /* 3rd order */
    p->k = 0.5 * p->z * (5.0 * p->z * p->z - 3.0);
    p->l = (8.0 / 11.0) * p->y * (5.0 * p->z * p->z - 1.0);
    p->m = (8.0 / 11.0) * p->x * (5.0 * p->z * p->z - 1.0);
    p->n = 2.0 * p->x * p->y * p->z;
    p->o = p->z * (p->x * p->x - p->y * p->y);
    p->p = 3.0 * p->y * (3.0 * p->x * p->x - p->y * p->y);
    p->q = 3.0 * p->x * (p->x * p->x - 3.0 * p->y * p->y);
}

static int aambicode(CSOUND *csound, AMBIC *p)
{
    int nn = csound->ksmps;  /* array size from orchestra */

    /* init input array pointer */
    MYFLT *inptp = p->asig;

    /* init output array pointer 0th order */
    MYFLT *rsltp_w = p->mw;

    /* init output array pointers 1th order */
    MYFLT *rsltp_x = p->mx;
    MYFLT *rsltp_y = p->my;
    MYFLT *rsltp_z = p->mz;

    /* init output array pointers 2nd order */
    MYFLT *rsltp_r = p->mr;
    MYFLT *rsltp_s = p->ms;
    MYFLT *rsltp_t = p->mt;
    MYFLT *rsltp_u = p->mu;
    MYFLT *rsltp_v = p->mv;

    /* init output array pointers 3rd order */
    MYFLT *rsltp_k = p->mk;
    MYFLT *rsltp_l = p->ml;
    MYFLT *rsltp_m = p->mm;
    MYFLT *rsltp_n = p->mn;
    MYFLT *rsltp_o = p->mo;
    MYFLT *rsltp_p = p->mp;
    MYFLT *rsltp_q = p->mq;

    /* update coefficients */
    ambicode_set_coefficients(p);

    if (p->OUTOCOUNT == 4 && p->INOCOUNT >= 5) {
      /* 1st order */
      do {
        /* 0th order */
        *rsltp_w++ = *inptp * p->w * *p->kin[0];

        /* 1st order */
        *rsltp_x++ = *inptp * p->x * *p->kin[1];
        *rsltp_y++ = *inptp * p->y * *p->kin[1];
        *rsltp_z++ = *inptp * p->z * *p->kin[1];

        /* increment input pointer */
        inptp++;
      }
      while (--nn);
    }
    else if (p->OUTOCOUNT == 9 && p->INOCOUNT >= 6) {
      /* 2nd order */

      do {
        /* 0th order */
        *rsltp_w++ = *inptp * p->w * *p->kin[0];

        /* 1st order */
        *rsltp_x++ = *inptp * p->x * *p->kin[1];
        *rsltp_y++ = *inptp * p->y * *p->kin[1];
        *rsltp_z++ = *inptp * p->z * *p->kin[1];

        /* 2nd order */
        *rsltp_r++ = *inptp * p->r * *p->kin[2];
        *rsltp_s++ = *inptp * p->s * *p->kin[2];
        *rsltp_t++ = *inptp * p->t * *p->kin[2];
        *rsltp_u++ = *inptp * p->u * *p->kin[2];
        *rsltp_v++ = *inptp * p->v * *p->kin[2];

        /* increment input pointer */
        inptp++;
      }
      while (--nn);
    }
    else if (p->OUTOCOUNT == 16 && p->INOCOUNT >= 7) {
      /* 3rd order */

      do {
        /* 0th order */
        *rsltp_w++ = *inptp * p->w * *p->kin[0];

        /* 1st order */
        *rsltp_x++ = *inptp * p->x * *p->kin[1];
        *rsltp_y++ = *inptp * p->y * *p->kin[1];
        *rsltp_z++ = *inptp * p->z * *p->kin[1];

        /* 2nd order */
        *rsltp_r++ = *inptp * p->r * *p->kin[2];
        *rsltp_s++ = *inptp * p->s * *p->kin[2];
        *rsltp_t++ = *inptp * p->t * *p->kin[2];
        *rsltp_u++ = *inptp * p->u * *p->kin[2];
        *rsltp_v++ = *inptp * p->v * *p->kin[2];

        /* 3rd order */
        *rsltp_k++ = *inptp * p->k * *p->kin[3];
        *rsltp_l++ = *inptp * p->l * *p->kin[3];
        *rsltp_m++ = *inptp * p->m * *p->kin[3];
        *rsltp_n++ = *inptp * p->n * *p->kin[3];
        *rsltp_o++ = *inptp * p->o * *p->kin[3];
        *rsltp_p++ = *inptp * p->p * *p->kin[3];
        *rsltp_q++ = *inptp * p->q * *p->kin[3];

        /* increment input pointer */
        inptp++;
      }
      while (--nn);
    }
    return OK;
}

static void ambideco_set_coefficients(AMBID *p, double alpha, double beta,
                                      int index)
{
    /* convert degrees to radian */
    /* 0.017... = pi/180 */
    double alpha_rad = alpha * 0.0174532925199432957692369076848861;
    double beta_rad = beta * 0.0174532925199432957692369076848861;

    /* calculate ambisonic coefficients (Furse-Malham-set) */

    /* 0th order */
    p->w[index] = 0.707106781186547524400844362104849; /* 1/sqrt(2) */

    /* 1st order */
    {
      double cbeta = cos(beta_rad);
      p->x[index] = cos(alpha_rad) * cbeta;
      p->y[index] = sin(alpha_rad) * cbeta;
      p->z[index] = sin(beta_rad);
    }

    /* 2nd order */
    p->r[index] = 0.5 * (3.0 * p->z[index] * p->z[index] - 1.0);
    p->s[index] = 2.0 * p->x[index] * p->z[index];
    p->t[index] = 2.0 * p->y[index] * p->z[index];
    p->u[index] = p->x[index] * p->x[index] - p->y[index] * p->y[index];
    p->v[index] = 2.0 * p->x[index] * p->y[index];

    /* 3rd order */
    p->k[index] = 0.5 * p->z[index] * (5.0 * p->z[index] * p->z[index] - 3.0);
    p->l[index] = (8.0/11.0) * p->y[index] * (5.0*p->z[index]* p->z[index] - 1.0);
    p->m[index] = (8.0/11.0) * p->x[index] * (5.0*p->z[index]* p->z[index] - 1.0);
    p->n[index] = 2.0 * p->x[index] * p->y[index] * p->z[index];
    p->o[index] = p->z[index] *
      (p->x[index] * p->x[index] - p->y[index] * p->y[index]);
    p->p[index] = 3.0 * p->y[index] *
      (3.0 * p->x[index] * p->x[index] - p->y[index] * p->y[index]);
    p->q[index] = 3.0 * p->x[index] *
      (p->x[index] * p->x[index] - 3.0 * p->y[index] * p->y[index]);
}

static int iambideco(CSOUND *csound, AMBID *p)
{
    int setup = (int)*p->isetup;
    csound->Warning(csound,
                    Str("bformdec is deprecated; use bformdec1 instead\n"));
    if (setup<0) setup = -setup;
    /* check correct number of input arguments */
    if (UNLIKELY((p->INOCOUNT != 5) && (p->INOCOUNT != 10) && (p->INOCOUNT != 17))) {
      return csound->InitError(csound, Str("Wrong number of input arguments!"));
    }

    switch (setup) {
      case 1:
        {
          if (UNLIKELY(p->OUTOCOUNT != 2)) {
            return csound->InitError(csound,
                                     Str("Wrong number of output cells! "
                                         "There must be 2 output cells."));
          }
          else if (*p->isetup>0) {
            ambideco_set_coefficients(p, 330.0, 0.0, 0);    /* left */
            ambideco_set_coefficients(p, 30.0, 0.0, 1);     /* right */
          }
          else {
            int i;
            static double w[] = {0.707106781186547524400844362104849,
                                 0.707106781186547524400844362104849};
/*             static double x[] = {0.0, 0.0}; */
            static double y[] = {0.5000,-0.5000};
/*             static double z[] = {0.0, 0.0}; */
/*             static double r[] = {0.0, 0.0}; */
/*             static double s[] = {0.0, 0.0}; */
/*             static double t[] = {0.0, 0.0}; */
/*             static double u[] = {0.0, 0.0}; */
/*             static double v[] = {0.0, 0.0}; */
            for (i=0; i<2; i++) {
              p->w[i] = w[i];
              p->x[i] = 0.0;
              p->y[i] = y[i];
              p->z[i] = 0.0;
              p->r[i] = 0.0;
              p->s[i] = 0.0;
              p->t[i] = 0.0;
              p->u[i] = 0.0;
              p->v[i] = 0.0;
              p->k[i] = 0.0;
              p->l[i] = 0.0;
              p->m[i] = 0.0;
              p->n[i] = 0.0;
              p->o[i] = 0.0;
              p->p[i] = 0.0;
              p->q[i] = 0.0;
            }
          }
          break;
        }

      case 2:
        {
          if (UNLIKELY(p->OUTOCOUNT != 4)) {
            return csound->InitError(csound,
                                     Str("Wrong number of output cells! "
                                         "There must be 4 output cells."));
          }
          else if (*p->isetup>0) {
            ambideco_set_coefficients(p, 45.0, 0.0, 0);
            ambideco_set_coefficients(p, 135.0, 0.0, 1);
            ambideco_set_coefficients(p, 225.0, 0.0, 2);
            ambideco_set_coefficients(p, 315.0, 0.0, 3);
          }
          else {
            int i;
            static double w[] = {0.3536, 0.3536, 0.3536, 0.3536};
            static double x[] = {0.2434,  0.2434, -0.2434, -0.2434};
            static double y[] = {0.2434,  -0.2434, -0.2434, 0.2434};
/*             static double z[] = {0.0, 0.0, 0.0, 0.0}; */
/*             static double r[] = {0.0, 0.0, 0.0, 0.0}; */
/*             static double s[] = {0.0, 0.0, 0.0, 0.0}; */
/*             static double t[] = {0.0, 0.0, 0.0, 0.0}; */
/*             static double u[] = {0.0, 0.0, 0.0, 0.0}; */
            static double v[] = {0.0964, -0.0964, 0.0964, -0.0964};
            for (i=0; i<4; i++) {
              p->w[i] = w[i];
              p->x[i] = x[i];
              p->y[i] = y[i];
              p->z[i] = 0.0;
              p->r[i] = 0.0;
              p->s[i] = 0.0;
              p->t[i] = 0.0;
              p->u[i] = 0.0;
              p->v[i] = v[i];
              p->k[i] = 0.0;
              p->l[i] = 0.0;
              p->m[i] = 0.0;
              p->n[i] = 0.0;
              p->o[i] = 0.0;
              p->p[i] = 0.0;
              p->q[i] = 0.0;
            }
          }
          break;
        }

      case 3: {
        if (UNLIKELY(p->OUTOCOUNT != 5)) {
          return csound->InitError(csound,
                                   Str("Wrong number of output cells! "
                                       "There must be 5 output cells."));
        }
        else if (*p->isetup>0) {
          ambideco_set_coefficients(p, 330.0, 0.0, 0);  /* left */
          ambideco_set_coefficients(p, 30.0, 0.0, 1);   /* right */
          ambideco_set_coefficients(p, 0.0, 0.0, 2);    /* center */
          ambideco_set_coefficients(p, 250.0, 0.0, 3);  /* surround L */
          ambideco_set_coefficients(p, 110.0, 0.0, 4);  /* surround R */
        }
        else {
          int i;
          /* Furze controlled opposites */
          static double w[] = {0.2828, 0.2828, 0.2828, 0.2828, 0.2828};
          static double x[] = {0.2227, -0.0851, -0.2753, -0.0851, 0.2227};
          static double y[] = {0.1618, 0.2619, 0.0000, -0.2619, -0.1618};
/*           static double z[] = {0.0, 0.0, 0.0, 0.0}; */
/*           static double r[] = {0.0, 0.0, 0.0, 0.0}; */
/*           static double s[] = {0.0, 0.0, 0.0, 0.0}; */
/*           static double t[] = {0.0, 0.0, 0.0, 0.0}; */
          static double u[] = {0.0238, -0.0624, 0.0771, -0.0624, 0.0238};
          static double v[] = {0.0733, -0.0453, 0.0000, 0.0453, -0.0733};

          for (i=0; i<5; i++) {
            p->w[i] = w[i];
            p->x[i] = x[i];
            p->y[i] = y[i];
            p->z[i] = 0.0;
            p->r[i] = 0.0;
            p->s[i] = 0.0;
            p->t[i] = 0.0;
            p->u[i] = u[i];
            p->v[i] = v[i];
            p->k[i] = 0.0;
            p->l[i] = 0.0;
            p->m[i] = 0.0;
            p->n[i] = 0.0;
            p->o[i] = 0.0;
            p->p[i] = 0.0;
            p->q[i] = 0.0;
          }
        }
        break;
      }

      case 4:
        {
          if (UNLIKELY(p->OUTOCOUNT != 8)) {
            return csound->InitError(csound,
                                     Str("Wrong number of output cells! "
                                         "There must be 8 output cells."));
          }
          else if (*p->isetup>0) {
            ambideco_set_coefficients(p,  22.5, 0.0, 0);
            ambideco_set_coefficients(p,  67.5, 0.0, 1);
            ambideco_set_coefficients(p, 112.5, 0.0, 2);
            ambideco_set_coefficients(p, 157.5, 0.0, 3);
            ambideco_set_coefficients(p, 202.5, 0.0, 4);
            ambideco_set_coefficients(p, 247.5, 0.0, 5);
            ambideco_set_coefficients(p, 292.5, 0.0, 6);
            ambideco_set_coefficients(p, 337.5, 0.0, 7);
          }
          else {
            int i;
            static double w[] = {0.1768, 0.1768, 0.1768, 0.1768, 0.1768, 0.1768, 0.1768, 0.1768};
            static double x[] = {0.1591, 0.0659,-0.0659,-0.1591,-0.1591,-0.0659, 0.0659, 0.1591};
            static double y[] = {0.0659, 0.1591, 0.1591, 0.0659,-0.0659,-0.1591,-0.1591,-0.0659};
/*             static double z[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; */
/*             static double r[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; */
/*             static double s[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; */
/*             static double t[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; */
            static double u[] = {0.0342,-0.0342,-0.0342, 0.0342, 0.0342,-0.0342,-0.0342, 0.0342};
            static double v[] = {0.0342, 0.0342,-0.0342,-0.0342, 0.0342, 0.0342,-0.0342,-0.0342};
            for (i=0; i<8; i++) {
              p->w[i] = w[i];
              p->x[i] = x[i];
              p->y[i] = y[i];
              p->z[i] = 0.0;
              p->r[i] = 0.0;
              p->s[i] = 0.0;
              p->t[i] = 0.0;
              p->u[i] = u[i];
              p->v[i] = v[i];
              p->k[i] = 0.0;
              p->l[i] = 0.0;
              p->m[i] = 0.0;
              p->n[i] = 0.0;
              p->o[i] = 0.0;
              p->p[i] = 0.0;
              p->q[i] = 0.0;
            }
          }
          break;
        }

      case 5:
        {
          if (UNLIKELY(p->OUTOCOUNT != 8)) {
            return csound->InitError(csound,
                                     Str("Wrong number of output cells! "
                                         "There must be 8 output cells."));
          }
          else if (*p->isetup>0) {
            ambideco_set_coefficients(p,  45.0,  0.0, 0);
            ambideco_set_coefficients(p,  45.0, 30.0, 1);
            ambideco_set_coefficients(p, 135.0,  0.0, 2);
            ambideco_set_coefficients(p, 135.0, 30.0, 3);
            ambideco_set_coefficients(p, 225.0,  0.0, 4);
            ambideco_set_coefficients(p, 225.0, 30.0, 5);
            ambideco_set_coefficients(p, 315.0,  0.0, 6);
            ambideco_set_coefficients(p, 315.0, 30.0, 7);
          }
          else {
            int i;
            static double w[] = {0.1768,0.1768,0.1768,0.1768,0.1768,0.1768,0.1768,0.1768};
            static double x[] = {0.1140, 0.1140,-0.1140,-0.1140, 0.1140, 0.1140,-0.1140,-0.1140};
            static double y[] = {0.1140,-0.1140,-0.1140, 0.1140, 0.1140,-0.1140,-0.1140, 0.1140};
            static double z[] = {-0.1140,-0.1140,-0.1140,-0.1140, 0.1140, 0.1140, 0.1140, 0.1140};
/*             static double r[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; */
            static double s[] = {-0.0369,-0.0369, 0.0369, 0.0369, 0.0369, 0.0369,-0.0369,-0.0369};
            static double t[] = {-0.0369, 0.0369, 0.0369,-0.0369, 0.0369,-0.0369,-0.0369, 0.0369};
/*             static double u[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; */
            static double v[] = { 0.0369,-0.0369, 0.0369,-0.0369, 0.0369,-0.0369, 0.0369,-0.0369};
            for (i=0; i<8; i++) {
              p->w[i] = w[i];
              p->x[i] = x[i];
              p->y[i] = y[i];
              p->z[i] = z[i];
              p->r[i] = 0.0;
              p->s[i] = s[i];
              p->t[i] = t[i];
              p->u[i] = 0.0;
              p->v[i] = v[i];
              p->k[i] = 0.0;
              p->l[i] = 0.0;
              p->m[i] = 0.0;
              p->n[i] = 0.0;
              p->o[i] = 0.0;
              p->p[i] = 0.0;
              p->q[i] = 0.0;
            }
          }
         break;
        }
      case 6: {
        if (UNLIKELY(p->OUTOCOUNT != 5)) {
          return csound->InitError(csound,
                                   Str("Wrong number of output cells! "
                                       "There must be 5 output cells."));
        }
                /*  These are Wiggins' cpefficients */
/*  L    30°    {0.4724,  0.7143,  0.7258,  0.0000,  0.3456}
    R   -30°    {0.4724,  0.7143, -0.7258,  0.0000, -0.3456}
    C   0°      {0.3226,  0.7719,  0.0000,  0.0000,  0.4724}
   LS   110°    {0.9101, -0.7834,  0.9562, -0.0806,  0.0000}
   RS   -110°   {0.9101, -0.7834, -0.9562, -0.0806,  0.0000}  */
        {
          int i;
          static double w[] = {0.4724, 0.4724, 0.3226, 0.9101, 0.9101};
          static double x[] = {0.7143, 0.7143, 0.7719,-0.7834,-0.7834};
          static double y[] = {0.7258,-0.7258, 0.0000, 0.9562,-0.9562};
          static double u[] = {0.0000,0.0000,0.0000,-0.0806,-0.0806};
          static double v[] = {0.3456,-0.3456,0.4724,0.0000,0.0000};
          for (i=0; i<5; i++) {
            p->w[i] = w[i];
            p->x[i] = x[i];
            p->y[i] = y[i];
            p->z[i] = 0.0;
            p->r[i] = 0.0;
            p->s[i] = 0.0;
            p->t[i] = 0.0;
            p->u[i] = u[i];
            p->v[i] = v[i];
            p->k[i] = 0.0;
            p->l[i] = 0.0;
            p->m[i] = 0.0;
            p->n[i] = 0.0;
            p->o[i] = 0.0;
            p->p[i] = 0.0;
            p->q[i] = 0.0;
          }
        }
        break;
      }

      default:
        return csound->InitError(csound, Str("Not supported setup number!"));
    }
    return OK;
}

static int aambideco(CSOUND *csound, AMBID *p)
{
    int nn = csound->ksmps;  /* array size from orchestra */
    int i = 0;

    /* init input array pointer 0th order */
    MYFLT *inptp_w = p->aw;

    /* init input array pointer 1st order */
    MYFLT *inptp_x = p->ax;
    MYFLT *inptp_y = p->ay;
    MYFLT *inptp_z = p->a[0];

    /* init input array pointer 2nd order */
    MYFLT *inptp_r = p->a[1];
    MYFLT *inptp_s = p->a[2];
    MYFLT *inptp_t = p->a[3];
    MYFLT *inptp_u = p->a[4];
    MYFLT *inptp_v = p->a[5];

    /* init input array pointer 3rd order */
    MYFLT *inptp_k = p->a[6];
    MYFLT *inptp_l = p->a[7];
    MYFLT *inptp_m = p->a[8];
    MYFLT *inptp_n = p->a[9];
    MYFLT *inptp_o = p->a[10];
    MYFLT *inptp_p = p->a[11];
    MYFLT *inptp_q = p->a[12];

    /* init output array pointer */
    MYFLT *rsltp[8];

    rsltp[0] = p->m0;
    rsltp[1] = p->m1;
    rsltp[2] = p->m2;
    rsltp[3] = p->m3;
    rsltp[4] = p->m4;
    rsltp[5] = p->m5;
    rsltp[6] = p->m6;
    rsltp[7] = p->m7;
    /* L = 0.5 * (0.9397*W + 0.1856*X - j*0.342*W + j*0.5099*X + 0.655*Y)

       R = 0.5 * (0.9397*W+ 0.1856*X + j*0.342*W - j*0.5099*X - 0.655*Y) */
    if (p->INOCOUNT == 5) {
      do {
        /* 1st order */
        for (i = 0; i < p->OUTOCOUNT; i++) {
          /* calculate output for every used loudspeaker */
          *rsltp[i]++ = *inptp_w * p->w[i] + *inptp_x * p->x[i] +
                        *inptp_y * p->y[i] + *inptp_z * p->z[i];
        }

        /* increment input array pointer 0th order */
        inptp_w++;

        /* increment input array pointer 1st order */
        inptp_x++;
        inptp_y++;
        inptp_z++;

      } while (--nn);
    }
    else if (p->INOCOUNT == 10) {
      do {
        /* 2nd order */
        for (i = 0; i < p->OUTOCOUNT; i++) {
          /* calculate output for every used loudspeaker */
          *rsltp[i]++ = *inptp_w * p->w[i] + *inptp_x * p->x[i] +
                        *inptp_y * p->y[i] + *inptp_z * p->z[i] +
                        *inptp_r * p->r[i] + *inptp_s * p->s[i] +
                        *inptp_t * p->t[i] + *inptp_u * p->u[i] +
                        *inptp_v * p->v[i];
        }

        /* increment input array pointer 0th order */
        inptp_w++;

        /* increment input array pointer 1st order */
        inptp_x++;
        inptp_y++;
        inptp_z++;

        /* increment input array pointer 2nd order */
        inptp_r++;
        inptp_s++;
        inptp_t++;
        inptp_u++;
        inptp_v++;

      } while (--nn);
    }
    else if (p->INOCOUNT == 17) {
      do {
        /* 3rd order */
        for (i = 0; i < p->OUTOCOUNT; i++) {
          /* calculate output for every used loudspeaker */
          *rsltp[i]++ = *inptp_w * p->w[i] + *inptp_x * p->x[i] +
                        *inptp_y * p->y[i] + *inptp_z * p->z[i] +
                        *inptp_r * p->r[i] + *inptp_s * p->s[i] +
                        *inptp_t * p->t[i] + *inptp_u * p->u[i] +
                        *inptp_v * p->v[i] + *inptp_k * p->k[i] +
                        *inptp_l * p->l[i] + *inptp_m * p->m[i] +
                        *inptp_n * p->n[i] + *inptp_o * p->o[i] +
                        *inptp_p * p->p[i] + *inptp_q * p->q[i];
        }

        /* increment input array pointer 0th order */
        inptp_w++;

        /* increment input array pointer 1st order */
        inptp_x++;
        inptp_y++;
        inptp_z++;

        /* increment input array pointer 2nd order */
        inptp_r++;
        inptp_s++;
        inptp_t++;
        inptp_u++;
        inptp_v++;

        /* increment input array pointer 3rd order */
        inptp_k++;
        inptp_l++;
        inptp_m++;
        inptp_n++;
        inptp_o++;
        inptp_p++;
        inptp_q++;
      } while (--nn);
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "bformenc", S(AMBIC), DP|5, "mmmmmmmmmmmmmmmm", "akkPPPP",
                            (SUBR)iambicode, NULL, (SUBR)aambicode },
  { "bformdec", S(AMBID), DP|5, "mmmmmmmm", "iaaay",
                            (SUBR)iambideco, NULL, (SUBR)aambideco }
};

int ambicode_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

