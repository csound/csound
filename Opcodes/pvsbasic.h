/*
    pvsbasic.h:
    basic opcodes for transformation of streaming PV signals

    Copyright (c) Victor Lazzarini, 2004

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

/*
   PVSMOOTH:
   fsig  pvsmooth  fsigin, kcfa, kcff

   Smooths the spectral functions of a pvs signal.

   fsigin: input signal
   kcfa:  cutoff frequency amount of LP filter applied to amplitudes
          (0 - 1)
   kcff: cutoff frequency amount of LP filter applied to frequencies
          (0 - 1)

   PVSFREEZE:
   fsig pvsfreeze fsigin, kfreeza, freezf

   Freeze a spectral frame.

   fsigin: input
   kfreeza: amplitude freezing (1=on, 0=off)
   kfreezf: frequency freezing (1=on, 0=off)

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
   {"pvsinit", S(PVSINI), 3, "f", "", pvsinit, NULL, NULL}
*/

#ifndef _PVSBASIC_H
#define _PVSBASIC_H

#include "pstream.h"

typedef struct _pvsini {
    OPDS    h;
    PVSDAT  *fout;
    MYFLT   *framesize, *olap, *winsize, *wintype, *format;
    uint32  lastframe;
} PVSINI;

typedef struct _pvsosc {
    OPDS    h;
    PVSDAT  *fout;
    MYFLT   *ka, *kf, *type;
    MYFLT   *framesize, *olap, *winsize, *wintype, *format;
    MYFLT incr;
    uint32  lastframe;
} PVSOSC;

typedef struct _pvsbin {
    OPDS    h;
    MYFLT   *kamp, *kfreq;
    PVSDAT  *fin;
    MYFLT   *kbin;
    uint32  lastframe;
} PVSBIN;

typedef struct _pvsfreez {
    OPDS    h;
    PVSDAT  *fout;
    PVSDAT  *fin;
    MYFLT   *kfra, *kfrf;
    AUXCH   freez;
    uint32  lastframe;
} PVSFREEZE;

typedef struct _pvsmooth {
    OPDS    h;
    PVSDAT  *fout;
    PVSDAT  *fin;
    MYFLT   *kfra, *kfrf;
    AUXCH   del;
    uint32  lastframe;
} PVSMOOTH;

typedef struct _pvsmix {
    OPDS    h;
    PVSDAT  *fout;
    PVSDAT  *fa;
    PVSDAT  *fb;
    uint32  lastframe;
} PVSMIX;

static int32_t pvsmixset(CSOUND *, PVSMIX *p);
static int32_t pvsmix(CSOUND *, PVSMIX *p);

typedef struct _pvsfilter {
    OPDS    h;
    PVSDAT  *fout;
    PVSDAT  *fin;
    PVSDAT  *fil;
    MYFLT   *kdepth;
    MYFLT   *gain;
    uint32  lastframe;
} PVSFILTER;

static int32_t pvsfilterset(CSOUND *, PVSFILTER *p);
static int32_t pvsfilter(CSOUND *, PVSFILTER *p);


typedef struct _pvsblur {
    OPDS    h;
    PVSDAT  *fout;
    PVSDAT  *fin;
    MYFLT   *kdel;
    MYFLT   *maxdel;
    AUXCH   delframes;
    MYFLT   frpsec;
    int32   count;
    uint32  lastframe;
} PVSBLUR;

static int32_t pvsblurset(CSOUND *, PVSBLUR *p);
static int32_t pvsblur(CSOUND *, PVSBLUR *p);

typedef struct _pvstencil {
    OPDS    h;
    PVSDAT  *fout;
    PVSDAT  *fin;
    MYFLT   *kgain;
    MYFLT   *klevel;
    MYFLT   *ifn;
    FUNC    *func;
    uint32  lastframe;
} PVSTENCIL;

static int32_t pvstencilset(CSOUND *, PVSTENCIL *p);
static int32_t
pvstencil(CSOUND *, PVSTENCIL *p);

#endif

