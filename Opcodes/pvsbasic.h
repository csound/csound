/* pvsbasic.h:
   basic opcodes for transformation of streaming PV signals

   (c) Victor Lazzarini, 2004

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

/*
   PVSMIX:
   fsig  pvsmix  fsigin1, fsigin2

   Mix 'seamlessly' two pv signals.

   fsigin1, fsigin2: input signals.

   PVSFILTER:
   fsig pvsfilter  fsigin, fsigfil, kdepth[, igain]

   Multiply amplitudes of fsigin by those of fsigfil, according to kdepth

   fsigin: input signal
   fsigfil: filtering signal
   kdepth: depth of filtering
   igain: amplitude scaling

   PVSCALE:
   fsig pvscale  fsigin, kscal[, ikeepform, igain]

   Scale the frequency components of a pv signal, resulting
   in pitch shift.

   fsigin: input signal
   kscal: scaling ratio
   ikeepform: attempt to keep input signal formants; 0: do not keep formants;
   1: keep formants by imposing original amps; 2: keep formants by filtering using
   the original spec envelope (defaults to 0)
   igain: amplitude scaling (defaults to 1)

   PVSHIFT:
   fsig pvshift  fsigin, kshift, klowest [,ikeepform, igain]

   Shift the frequency components of a pv signal.

   fsigin: input signal
   kshift: shift amount (in Hz)
   klowest: lowest freq affected by the process.
   ikeepform: attempt to keep input signal formants; 0: do not keep formants;
   1: keep formants by imposing original amps; 2: keep formants by filtering using
   the original spec envelope (defaults to 0)
   igain: amplitude scaling (defaults to 1)


   PVBLUR:
   fsig pvsblur  fsigin, kblurtime, imaxdel

   Average the amp/freq time functions of each analysis channel for
   a specified time (truncated to number of frames).
   The input signal will be delayed by that amount.

   fsigin: input signal
   kblurtime: time in secs during which windows will be averaged.
   imaxdel: max delay time, used for allocating memory for the averaging
   operation.

   PVSTENCIL:
   fsig pvstencil  fsigin, kgain, klevel, iftable

   Transforms a signal according to a masking function table; if the
   signal normalised amplitude is below the value of the function for
   a specific PV channel, it applies a gain to that channel.

   fsigin: input signal
   kgain: 'stencil' gain
   klevel: mask function level (ftable is scaled by this value prior to 'stenciling')
   iftable: masking function table


   opcode table entries:
   {"pvscale", S(PVSSCALE), 3,"f", "fkop", pvsscaleset, pvsscale, NULL },
   {"pvshift", S(PVSSHIFT), 3,"f", "fkopo", pvsshiftset, pvsshift, NULL },
   {"pvsmix", S(PVSMIX), 3, "f", "ff", pvsmixset, pvsmix, NULL},
   {"pvsfilter", S(PVSFILTER), 3, "f", "ffkp", pvsfilterset, pvsfilter, NULL},
   {"pvsblur", S(PVSBLUR), 3, "f", "fki", pvsblurset, pvsblur, NULL},
   {"pvstencil", S(PVSTENCIL), 3, "f", "fkki", pvstencilset, pvstencil, NULL}


*/

#ifndef _PVSBASIC_H
#define _PVSBASIC_H

#include "pstream.h"


typedef struct _pvsmix {
        OPDS    h;
        PVSDAT  *fout;
        PVSDAT  *fa;
        PVSDAT  *fb;
        unsigned long   lastframe;
} PVSMIX;

int pvsmixset(PVSMIX *p);
int pvsmix(PVSMIX *p);

typedef struct _pvsfilter {
        OPDS h;
        PVSDAT  *fout;
        PVSDAT  *fin;
        PVSDAT  *fil;
        MYFLT   *kdepth;
        MYFLT   *gain;
        unsigned long   lastframe;
} PVSFILTER;

int pvsfilterset(PVSFILTER *p);
int pvsfilter(PVSFILTER *p);

typedef struct _pvscale {
        OPDS h;
        PVSDAT  *fout;
        PVSDAT  *fin;
        MYFLT   *kscal;
        MYFLT   *keepform;
        MYFLT   *gain;
        unsigned long   lastframe;
} PVSSCALE;

int pvsscaleset(PVSSCALE *p);
int pvsscale(PVSSCALE *p);

typedef struct _pvshift {
        OPDS h;
        PVSDAT  *fout;
        PVSDAT  *fin;
        MYFLT   *kshift;
        MYFLT   *lowest;
        MYFLT   *keepform;
        MYFLT   *gain;
        unsigned long   lastframe;
} PVSSHIFT;

int pvsshiftset(PVSSHIFT *p);
int pvsshift(PVSSHIFT *p);

typedef struct _pvsblur {
        OPDS h;
        PVSDAT  *fout;
        PVSDAT  *fin;
        MYFLT   *kdel;
        MYFLT   *maxdel;
        AUXCH   delframes;
        MYFLT   frpsec;
        long    count;
        unsigned long   lastframe;
} PVSBLUR;

int pvsblurset(PVSBLUR *p);
int pvsblur(PVSBLUR *p);

typedef struct _pvstencil {
        OPDS    h;
        PVSDAT  *fout;
        PVSDAT  *fin;
        MYFLT   *kgain;
        MYFLT   *klevel;
        MYFLT   *ifn;
        FUNC    *func;
        unsigned long   lastframe;
} PVSTENCIL;

int pvstencilset(PVSTENCIL *p);
int pvstencil(PVSTENCIL *p);


#endif
