/*
    fareygen.c:

    Copyright (C) 2010 Georg Boenn

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

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include <math.h>

typedef struct _rat {
    int32_t p;
    int32_t q;
} RATIO;

static int32_t EulerPhi (int32_t n);
static int32_t FareyLength (int32_t n);
static void GenerateFarey (int32_t n, RATIO flist[], int32_t size);

static int32_t fareytable (FGDATA *ff, FUNC *ftp)
{
    /*
      This Gen routine calculates a Farey Sequence F_n of the integer n.
      A Farey Sequence F_n of order n is a list of fractions in their lowest
      terms between 0 and 1 and in ascending order. Their denominators do not
      exceed n.
      This means a fraction a/b belongs to F_n if 0 <= a <= b <= n.
      In F_n, the numerator and denominator of each fraction is always coprime.
      0 and 1 are included in F_n as the fractions 0/1 and 1/1.
      For example F_5 = {0/1, 1/5, 1/4, 1/3, 2/5, 1/2, 3/5, 2/3, 3/4, 4/5, 1/1}
      Some properties of the Farey Sequence:
      1. If a/b and c/d are two successive terms of F_n, then bc - ad = 1.
      2. If a/b, c/d, e/f are three successive terms of F_n, then:
      c/d = (a+e) / (b+f)
      c/d is called the mediant fraction between a/b and e/f.
      3. If n > 1, then no two successive terms of F_n have the same denominator.

      The length of any Farey Sequence F_n is determined by
      |F_n| = 1 + SUM (phi(m)) FOR m=1, m<=n, m++
      where phi(m) is Euler's totient function, which gives the number of
      integers <= m that are coprime to m.

      References:
      Hardy, G.M. and Wright, E.M. (1960), An Introduction to the Theory of
                               Numbers, Oxford, 4th Edition, Chapter 3, p.23
      http://mathworld.wolfram.com/FareySequence.html
      http://en.wikipedia.org/wiki/Totient

      Implementation Notes:

      The sequence length uses integer totients so the count is exact in
      both float and double builds.

      Important: The length of the table declared by the user does not
      have to be equal to the length of the Farey Sequence. If the
      table is smaller, then only a part of the sequence is copied. If
      it is longer, then zeros are padded.
    */

    int32_t j, fareyseq, nvals, nargs, farey_length, mode;
    MYFLT   *fp = ftp->ftable, *pp, *pp2;
    CSOUND  *csound = ff->csound;
    RATIO *flist;

    nvals = ff->flen;
    nargs = ff->e.pcnt - 4;
    if (UNLIKELY(nargs < 2)) {
      return csound->FtError(ff, "%s", Str("insufficient arguments for fareytable"));
    }
    ff->e.p[4] *= -1;
    pp = &(ff->e.p[5]);
    if (UNLIKELY(!(*pp >= FL(1.0) && (double)*pp <= INT32_MAX)))
      return csound->FtError(ff, Str("farey: invalid sequence order"));
    fareyseq = (int32_t)*pp;
    pp2 = &(ff->e.p[6]);
    if (UNLIKELY(!(*pp2 >= FL(0.0) && *pp2 < FL(5.0))))
      return csound->FtError(ff, Str("farey: mode must be between 0 and 4"));
    mode = (int32_t)*pp2;
    farey_length = FareyLength(fareyseq);
    if (UNLIKELY(farey_length == 0 ||
                 (size_t)farey_length > SIZE_MAX / sizeof(RATIO)))
      return csound->FtError(ff, Str("farey: sequence is too large"));
    if (ff->flen <= 0) return csound->FtError(ff, "%s", Str("Illegal table size"));
    flist = (RATIO*)csound->Calloc(csound, (size_t)farey_length * sizeof(RATIO));

    GenerateFarey (fareyseq, flist, farey_length);

    switch (mode) {
    default:
    case 0: /* output float elements of F_n */
      for (j = 0;  j < nvals; j++) {
        if (j < farey_length)
          fp[j] = (MYFLT) flist[j].p / (MYFLT) flist[j].q;
      }
      break;
    case 1: /* output delta values of successive elements of F_n */
      {
        MYFLT last = FL(0.0);
        int32_t i = 1;
        for (j = 0; j < nvals; j++, i++) {
          if (i < farey_length) {
            MYFLT current = (MYFLT) flist[i].p / (MYFLT) flist[i].q;
            fp[j] = current - last;
            last = current;
          }
        }
        break;
      }
    case 2: /* output only the denominators of the integer ratios */
      for (j = 0; j < nvals; j++) {
        if (j < farey_length)
          fp[j] = (MYFLT) flist[j].q;
      }
      break;
    case 3: /* output the normalised denominators of the integer ratios */
      {
        MYFLT farey_scale = (MYFLT) 1 / (MYFLT) fareyseq;
        for (j = 0; j < nvals; j++) {
          if (j < farey_length)
            fp[j] = (MYFLT) flist[j].q * farey_scale;
        }
        break;
      }
    case 4: /* output float elements of F_n + 1 for tuning tables*/
      for (j = 0; j < nvals; j++) {
        if (j < farey_length)
          fp[j] = FL(1.0) + (MYFLT) flist[j].p / (MYFLT) flist[j].q;
      }
      break;
    }
    csound->Free(csound,flist);
    return OK;
}

/* utility functions. See the comments above. */
static int32_t EulerPhi (int32_t n)
{
    int32_t prime, result = n;
    for (prime = 2; prime <= n / prime; prime++) {
      if (n % prime == 0) {
        result -= result / prime;
        do {
          n /= prime;
        } while (n % prime == 0);
      }
    }
    if (n > 1)
      result -= result / n;
    return result;
}

static int32_t FareyLength (int32_t n)
{
    int32_t i, result = 1;
    for (i = 1; i <= n; i++) {
      int32_t phi = EulerPhi(i);
      if (phi > INT32_MAX - result)
        return 0;
      result += phi;
    }
    return result;
}

static void GenerateFarey (int32_t n, RATIO flist[], int32_t size)
{
    int32_t a = 0, b = 1, c = 1, d = n, i;
    flist[0].p = a;
    flist[0].q = b;
    for (i = 1; i < size; i++) {
      int32_t k, nextp, nextq;
      flist[i].p = c;
      flist[i].q = d;
      if (c == d)
        break;
      k = (n + b) / d;
      nextp = k * c - a;
      nextq = k * d - b;
      a = c; b = d;
      c = nextp; d = nextq;
    }
}


static NGFENS farey_fgens[] = {
  { "farey", fareytable },
  { NULL, NULL }
};

FLINKAGE_BUILTIN(farey_fgens)
