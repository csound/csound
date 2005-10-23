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
typedef struct
{
  OPDS h;  /* required header */
  MYFLT *mw, *mx, *my, *mz, *mr, *ms, *mt, *mu, *mv, *mk,
        *ml, *mm, *mn, *mo, *mp, *mq;  /* addr outarg */
  MYFLT *asig, *kalpha, *kin[VARGMAX];  /* addr inargs */
  double w, x, y, z, r, s, t, u, v, k, l, m, n, o, p, q;  /* private dataspace */
} AMBIC;

#include <math.h>

int iambicode(CSOUND *csound, AMBIC *p)
{
    /* check correct number of input and output arguments */
    switch (p -> OUTOCOUNT)
      {
      case 4:
        {
          /* 2nd order */
          if (p -> INOCOUNT != 5) {
            return csound->InitError(csound,
                                     "Wrong number of input arguments! "
                                     "5 needed!\n");
          }
          break;
        }

      case 9:
        {
          /* 3rd order */
          if (p -> INOCOUNT != 6) {
            return csound->InitError(csound,"Wrong number of input arguments! "
                                     "6 needed!\n");
          }
          break;
        }

      case 16:
        {
          /* 4th order */
          if (p -> INOCOUNT != 7) {
            return csound->InitError(csound,"Wrong number of input arguments! "
                                     "7 needed!\n");
          }
          break;
        }

      default:
        {
          return csound->InitError(csound,"Wrong number of output arguments! "
                                   "4, 9 or 16 needed!\n");
        }
      }
    return OK;
}

void ambicode_set_coefficients(AMBIC *p)
{
    /* convert degrees to radian */
    MYFLT kalpha_rad = (*p -> kalpha) / FL(57.295779513082320876798154814105);
    MYFLT kbeta_rad = (*p -> kin[0]) / FL(57.295779513082320876798154814105);

    /* calculate ambisonic coefficients (Furse-Malham-set) */

    /* 0th order */
    p -> w = FL(1.0) / sqrt(FL(2.0));

    /* 1st order */
    p -> x = cos(kalpha_rad) * cos(kbeta_rad);
    p -> y = sin(kalpha_rad) * cos(kbeta_rad);
    p -> z = sin(kbeta_rad);

    /* 2nd order */
    p -> r = FL(0.5) * (FL(3.0) * p -> z * p -> z - FL(1.0));
    p -> s = FL(2.0) * p -> x * p -> z;
    p -> t = FL(2.0) * p -> y * p -> z;
    p -> u = p -> x * p -> x - p -> y * p -> y;
    p -> v = FL(2.0) * p -> x * p -> y;

    /* 3rd order */
    p -> k = FL(0.5) * p -> z * (FL(5.0) * p -> z * p -> z - FL(3.0));
    p -> l = FL(8.0) / FL(11.0) * p -> y * (FL(5.0) * p -> z * p -> z - FL(1.0));
    p -> m = FL(8.0) / FL(11.0) * p -> x * (FL(5.0) * p -> z * p -> z - FL(1.0));
    p -> n = FL(2.0) * p -> x * p -> y * p -> z;
    p -> o = p -> z * (p -> x * p -> x - p -> y * p -> y);
    p -> p = FL(3.0) * p -> y * (FL(3.0) * p -> x * p -> x - p -> y * p -> y);
    p -> q = FL(3.0) * p -> x * (p -> x * p -> x - FL(3.0) * p -> y * p -> y);
}


int aambicode(CSOUND *csound, AMBIC *p)
{
    int nn = csound->ksmps;  /* array size from orchestra */

    /* update coefficients */
    ambicode_set_coefficients(p);

    /* init input array pointer */
    MYFLT *inptp = p -> asig;

    /* init output array pointer 0th order */
    MYFLT *rsltp_w = p -> mw;

    /* init output array pointers 1th order */
    MYFLT *rsltp_x = p -> mx;
    MYFLT *rsltp_y = p -> my;
    MYFLT *rsltp_z = p -> mz;

    /* init output array pointers 2nd order */
    MYFLT *rsltp_r = p -> mr;
    MYFLT *rsltp_s = p -> ms;
    MYFLT *rsltp_t = p -> mt;
    MYFLT *rsltp_u = p -> mu;
    MYFLT *rsltp_v = p -> mv;

    /* init output array pointers 3rd order */
    MYFLT *rsltp_k = p -> mk;
    MYFLT *rsltp_l = p -> ml;
    MYFLT *rsltp_m = p -> mm;
    MYFLT *rsltp_n = p -> mn;
    MYFLT *rsltp_o = p -> mo;
    MYFLT *rsltp_p = p -> mp;
    MYFLT *rsltp_q = p -> mq;

    if (p -> OUTOCOUNT == 4 && p -> INOCOUNT == 5) {
      /* 1st order */

      do {
        /* 0th order */
        *rsltp_w++ = *inptp * p -> w * *p -> kin[1];

        /* 1st order */
        *rsltp_x++ = *inptp * p -> x * *p -> kin[2];
        *rsltp_y++ = *inptp * p -> y * *p -> kin[2];
        *rsltp_z++ = *inptp * p -> z * *p -> kin[2];

        /* increment input pointer */
        inptp++;
      }
      while (--nn);
    }

    else if (p -> OUTOCOUNT == 9 && p -> INOCOUNT == 6) {
      /* 2nd order */

      do {
        /* 0th order */
        *rsltp_w++ = *inptp * p -> w * *p -> kin[1];

        /* 1st order */
        *rsltp_x++ = *inptp * p -> x * *p -> kin[2];
        *rsltp_y++ = *inptp * p -> y * *p -> kin[2];
        *rsltp_z++ = *inptp * p -> z * *p -> kin[2];

        /* 2nd order */
        *rsltp_r++ = *inptp * p -> r * *p -> kin[3];
        *rsltp_s++ = *inptp * p -> s * *p -> kin[3];
        *rsltp_t++ = *inptp * p -> t * *p -> kin[3];
        *rsltp_u++ = *inptp * p -> u * *p -> kin[3];
        *rsltp_v++ = *inptp * p -> v * *p -> kin[3];

        /* increment input pointer */
        inptp++;
      }
      while (--nn);
    }

    else if (p -> OUTOCOUNT == 16 && p -> INOCOUNT == 7) {
      /* 3rd order */

      do {
        /* 0th order */
        *rsltp_w++ = *inptp * p -> w * *p -> kin[1];

        /* 1st order */
        *rsltp_x++ = *inptp * p -> x * *p -> kin[2];
        *rsltp_y++ = *inptp * p -> y * *p -> kin[2];
        *rsltp_z++ = *inptp * p -> z * *p -> kin[2];

        /* 2nd order */
        *rsltp_r++ = *inptp * p -> r * *p -> kin[3];
        *rsltp_s++ = *inptp * p -> s * *p -> kin[3];
        *rsltp_t++ = *inptp * p -> t * *p -> kin[3];
        *rsltp_u++ = *inptp * p -> u * *p -> kin[3];
        *rsltp_v++ = *inptp * p -> v * *p -> kin[3];

        /* 3rd order */
        *rsltp_k++ = *inptp * p -> k * *p -> kin[4];
        *rsltp_l++ = *inptp * p -> l * *p -> kin[4];
        *rsltp_m++ = *inptp * p -> m * *p -> kin[4];
        *rsltp_n++ = *inptp * p -> n * *p -> kin[4];
        *rsltp_o++ = *inptp * p -> o * *p -> kin[4];
        *rsltp_p++ = *inptp * p -> p * *p -> kin[4];
        *rsltp_q++ = *inptp * p -> q * *p -> kin[4];

        /* increment input pointer */
        inptp++;
      }
      while (--nn);
    }
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
  { "ambicode", S(AMBIC), 5, "mmmmmmmmmmmmmmmm", "akz",
                          (SUBR)iambicode, NULL, (SUBR)aambicode }
};

LINKAGE
