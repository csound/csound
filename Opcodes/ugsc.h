/*
    ugsc.h:

    Copyright (C) 1999 Sean M. Costello

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

/* svfilter.h
 *
 * Copyright 1999, by Sean M. Costello
 *
 * svfilter is an implementation of Hal Chamberlin's state variable filter
 * algorithm, from "Musical Applications of Microprocessors" (Hayden Books,
 * Indianapolis, Indiana, 1985), 2nd. edition, pp. 489-492. It implements
 * a second-order resonant filter, with lowpass, highpass and bandpass
 * outputs.
 *
 */

typedef struct {
        OPDS h;
  MYFLT *low, *high, *band, *in, *kfco, *kq, *iscl, *iskip;
        MYFLT ynm1, ynm2;
} SVF;

/* hilbert.h
 *
 * Copyright 1999, by Sean M. Costello
 *
 * hilbert is an implementation of an IIR Hilbert transformer.
 * The structure is based on two 6th-order allpass filters in
 * parallel, with a constant phase difference of 90 degrees
 * (+- some small amount of error) between the two outputs.
 * Allpass coefficients are calculated at i-time.
 */

typedef struct {
        OPDS h;
        MYFLT *out1, *out2, *in;
        MYFLT xnm1[12], ynm1[12], coef[12];
} HILBERT;

/* resonrz.h
 *
 * Copyright 1999, by Sean M. Costello
 *
 * resonr and resonz are implementations of second-order
 * bandpass resonators, with added zeroes in the transfer function.
 * The algorithms are based upon the work of Julius O. Smith and
 * John Stautner at Stanford, and Ken Steiglitz at Princeton.
 *
 */

typedef struct {
        OPDS h;
        MYFLT *out, *in, *kcf, *kbw, *iscl, *istor;
        double xnm1, xnm2, ynm1, ynm2;
        int32_t scaletype, aratemod;
} RESONZ;

/* Structure for cascade of 2nd order allpass filters */
typedef struct {
        OPDS h;
        MYFLT *out, *in, *kbf, *kbw, *order, *mode, *ksep, *fbgain;
        int32_t loop, modetype;
        MYFLT *nm1, *nm2, feedback;
        AUXCH aux1, aux2;
} PHASER2;

/* Structure for cascade of 1st order allpass filters*/
typedef struct {
        OPDS h;
        MYFLT *out, *in, *kcoef, *iorder, *fbgain, *istor;
        int32_t
        loop;
        MYFLT *xnm1, *ynm1, feedback;
        AUXCH auxx, auxy;
} PHASER1;

/* Structure for lowpass filter */
typedef struct {
        OPDS h;
        MYFLT *out, *in, *kfco, *kres, *istor;
        double ynm1, ynm2;
} LP2;
