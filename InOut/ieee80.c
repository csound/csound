/*  
    ieee80.c:

    Copyright (C) 1992 Bill Gardener, Dan Ellis

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

/***************************************************************\
*   ieee80.c                                                    *
*   Convert between "double" and IEEE 80 bit format             *
*   in machine independent manner.                              *
*   Assumes array of char is a continuous region of 8 bit frames*
*   Assumes (unsigned long) has 32 useable bits                 *
*   billg, dpwe @media.mit.edu                                  *
*   01aug91                                                     *
*   19aug91  aldel/dpwe  workaround top bit problem in Ultrix   *
*                        cc's double->ulong cast                *
*   05feb92  dpwe/billg  workaround top bit problem in          *
*                        THINKC4 + 68881 casting                *
\***************************************************************/

#include        "ieee80.h"
#include        <stdio.h>       /* for stderr in myDoubleToUlong */
#include        <stdlib.h>
#include        "cs.h"

/* #define MAIN 1 */       /* to compile test routines */

#define ULPOW2TO31      ((unsigned long)0x80000000L)
#define DPOW2TO31       ((double)2147483648.0)  /* 2^31 */

/* have to deal with ulong's 32nd bit conditionally as double<->ulong casts
   don't work in some C compilers */

static double myUlongToDouble(unsigned long ul)
{
    double val;

    /* in THINK_C, ulong -> double apparently goes via long, so can only
       apply to 31 bit numbers.  If 32nd bit is set, explicitly add on its
       value */
    if (ul & ULPOW2TO31)
      val = DPOW2TO31 + (ul & (~ULPOW2TO31));
    else
      val = ul;
    return val;
}

static unsigned long myDoubleToUlong(double val)
{
    unsigned long ul;

    /* cannot cast negative numbers into unsigned longs */
    if (val < 0) {
      err_printf(Str(X_296,"IEEE80:DoubleToUlong: val < 0\n"));
      longjmp(cenviron.exitjmp_,-1);
    }

    /* in ultrix 4.1's cc, double -> unsigned long loses the top bit,
       so we do the conversion only on the bottom 31 bits and set the
       last one by hand, if val is truly that big */
    /* should maybe test for val > (double)(unsigned long)0xFFFFFFFF ? */
    if (val < DPOW2TO31)
      ul = (unsigned long)val;
    else
      ul = ULPOW2TO31 | (unsigned long)(val-DPOW2TO31);
    return ul;
}


/*
 * Convert IEEE 80 bit floating point to double.
 * Should be portable to all C compilers.
 */
double ieee_80_to_double(unsigned char *p)
{
    char sign;
    short exp = 0;
    unsigned long mant1 = 0;
    unsigned long mant0 = 0;
    double val;
    exp = *p++;
    exp <<= 8;
    exp |= *p++;
    sign = (exp & 0x8000) ? 1 : 0;
    exp &= 0x7FFF;

    mant1 = *p++;
    mant1 <<= 8;
    mant1 |= *p++;
    mant1 <<= 8;
    mant1 |= *p++;
    mant1 <<= 8;
    mant1 |= *p++;

    mant0 = *p++;
    mant0 <<= 8;
    mant0 |= *p++;
    mant0 <<= 8;
    mant0 |= *p++;
    mant0 <<= 8;
    mant0 |= *p++;

    /* special test for all bits zero meaning zero
       - else pow(2,-16383) bombs */
    if (mant1 == 0 && mant0 == 0 && exp == 0 && sign == 0)
      return 0.0;
    else {
      val = myUlongToDouble(mant0) * pow(2.0,-63.0);
      val += myUlongToDouble(mant1) * pow(2.0,-31.0);
      val *= pow(2.0,((double) exp) - 16383.0);
      return sign ? -val : val;
    }
}

/*
 * Convert double to IEEE 80 bit floating point
 * Should be portable to all C compilers.
 * 19aug91 aldel/dpwe  covered for MSB bug in Ultrix 'cc'
 */

void double_to_ieee_80(double val, unsigned char *p)
{
    char sign = 0;
    short exp = 0;
    unsigned long mant1 = 0;
    unsigned long mant0 = 0;

    if (val < 0.0)      {  sign = 1;  val = -val; }

    if (val != 0.0) {   /* val identically zero -> all elements zero */
      exp = (short)(log(val)/log(2.0) + 16383.0);
      val *= pow(2.0, 31.0+16383.0-(double)exp);
      mant1 = myDoubleToUlong(val);
      val -= myUlongToDouble(mant1);
      val *= pow(2.0, 32.0);
      mant0 = myDoubleToUlong(val);
    }

    *p++ = ((sign<<7)|(exp>>8));
    *p++ = (u_char)(0xFF & exp);
    *p++ = (u_char)(0xFF & (mant1>>24));
    *p++ = (u_char)(0xFF & (mant1>>16));
    *p++ = (u_char)(0xFF & (mant1>> 8));
    *p++ = (u_char)(0xFF & (mant1));
    *p++ = (u_char)(0xFF & (mant0>>24));
    *p++ = (u_char)(0xFF & (mant0>>16));
    *p++ = (u_char)(0xFF & (mant0>> 8));
    *p++ = (u_char)(0xFF & (mant0));

}

#ifdef MAIN

static void print_hex(unsigned char *p, int n)
{
    long i;
    for (i = 0; i < n; i++) printf("%02X",*p++);
    printf(" ");
}

double tab[] = { 0, -1.0, 1.0, 2.0 , 4.0, 0.5, 0.25, 0.125,
                 3.14159265358979323846, 10000.0, 22000.0,
                 44100.0, 65536.0, 134072.768, 3540001.9, 4294967295.0};
#define NTAB (sizeof(tab)/sizeof(double))

int main(void)
{
    int i;
    double val;
    unsigned char eighty[10];
    double *p80;

    p80 = (double *)eighty;

    /* for each number in the test table, print its actual value,
       its native hex representation, the 80 bit representation and
       the back-converted value (should be the same!) */
    /* I think the hex of PI to all 80 bits is
       4000 C90F DAA2 2168 C233 */
    for (i = 0; i < NTAB; i++) {
      printf("%8f: ", tab[i]);
      print_hex((unsigned char *)(tab+i),sizeof(double));
      double_to_ieee_80((double) tab[i], eighty);
      print_hex(eighty, 10);
      val = ieee_80_to_double(eighty);
      printf("%f\n",val);
    }
}

#endif
