/*
    ieee80.h:

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
*   IEEE80.h                                                    *
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

/* Prototype argument wrapper */
/* make fn protos like   void fn PARG((int arg1, char arg2));  */
#ifndef PARG
#ifdef __STDC__
#define PARG(a)         a
#else /* !__STDC__ */
#define PARG(a)         ()
#endif /* __STDC__ */
#endif /* PARG */

#include <math.h>

double ieee_80_to_double(unsigned char *p);
void   double_to_ieee_80(double val, unsigned char *p);
