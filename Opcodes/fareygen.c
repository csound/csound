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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include <math.h>

#define MAX_PFACTOR 16
static const int32_t MAX_PRIMES = 168; /* 168 primes < 1000 */
static const int32_t primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37,
                             41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83,
                             89, 97, 101, 103, 107, 109, 113, 127, 131,
                             137, 139, 149, 151, 157, 163, 167, 173, 179,
                             181, 191, 193, 197, 199, 211, 223, 227, 229,
                             233, 239, 241, 251, 257, 263, 269, 271, 277,
                             281, 283, 293, 307, 311, 313, 317, 331, 337,
                             347, 349, 353, 359, 367, 373, 379, 383, 389,
                             397, 401, 409, 419, 421, 431, 433, 439, 443,
                             449, 457, 461, 463, 467, 479, 487, 491, 499,
                             503, 509, 521, 523, 541, 547, 557, 563, 569,
                             571, 577, 587, 593, 599, 601, 607, 613, 617,
                             619, 631, 641, 643, 647, 653, 659, 661, 673,
                             677, 683, 691, 701, 709, 719, 727, 733, 739,
                             743, 751, 757, 761, 769, 773, 787, 797, 809,
                             811, 821, 823, 827, 829, 839, 853, 857, 859,
                             863, 877, 881, 883, 887, 907, 911, 919, 929,
                             937, 941, 947, 953, 967, 971, 977, 983, 991,
                             997};

typedef struct pfactor_ {
    int32_t expon;
    int32_t base;
} PFACTOR;

typedef struct _rat {
    int32_t p;
    int32_t q;
} RATIO;

static int32_t EulerPhi (int32_t n);
static int32_t FareyLength (int32_t n);
static int32_t PrimeFactors (int32_t n, PFACTOR p[]);
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

      The size of the array of fractions for any F_n is calculated
      with the help Euler's function phi(n), also called totient
      function. For its implementation we included an array of prime
      numbers and a function to determine the prime factor composition
      of a natural integer. The primes dividing the integer are stored
      locally in a small array of ints.

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
    fareyseq = (int32_t) *pp;
    pp2 = &(ff->e.p[6]);
    mode = (int32_t) *pp2;
    farey_length = FareyLength(fareyseq);
    flist = (RATIO*) csound->Calloc(csound, farey_length*sizeof(RATIO));
    if (ff->flen <= 0) return csound->FtError(ff, "%s", Str("Illegal table size"));

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
    int32_t i;
    PFACTOR p[MAX_PFACTOR];
    MYFLT result;

    if (n == 1)
      return 1;
    if (n == 0)
      return 0;
    memset(p, 0, sizeof(PFACTOR)*MAX_PFACTOR);
    /* for (i=0; i < MAX_PFACTOR; i++) { */
    /*     p[i].expon = 0;  */
    /*     p[i].base = 0; */
    /* } */
    (void)PrimeFactors (n, p);

    result = (MYFLT) n;
    for (i = 0; i < MAX_PFACTOR; i++) {
      int32_t q = p[i].base;
      if (!q)
        break;
      result *= (FL(1.0) - FL(1.0) / (MYFLT) q);
    }
    return (int32_t) result;
}

static int32_t FareyLength (int32_t n)
{
    int32_t i = 1;
    int32_t result = 1;
    n++;
    for (; i < n; i++)
      result += EulerPhi (i);
    return result;
}


static int32_t PrimeFactors (int32_t n, PFACTOR p[])
{
    int32_t i = 0; int32_t j = 0;
    int32_t i_exp = 0;

    if (!n)
      return j;

    while (i < MAX_PRIMES)
    {
      int32_t aprime = primes[i++];
      if (j == MAX_PFACTOR || aprime > n) {
        return j;
      }
      if (n == aprime)
        {
          p[j].expon = 1;
          p[j].base = n;
          return (++j);
        }
      i_exp = 0;
      while (!(n % aprime))
        {
          i_exp++;
          n /= aprime;
        }
      if (i_exp)
        {
          p[j].expon = i_exp;
          p[j].base = aprime;
          ++j;
        }
    }
    return j;
}

static void GenerateFarey (int32_t n, RATIO flist[], int32_t size) {
    int32_t a, b, c, d, k, i;
    a = 0; b = 1, c = 1, d = n; i = 1;
    flist[0].p = a;
    flist[0].q = b;
    while (c < n) {
      k = (int32_t) ((n + b) / d);
      int32_t tempa = a;
      int32_t
        tempb = b;
      a = c; b = d;
      c = k * c - tempa;
      d = k * d - tempb;
      flist[i].p = a;
      flist[i].q = b;
      if (i < size)
        i++;
    }
}


static NGFENS farey_fgens[] = {
  { "farey", fareytable },
  { NULL, NULL }
};

FLINKAGE_BUILTIN(farey_fgens)
