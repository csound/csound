/*  newfils.h:
    Filter opcodes

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

MOOGLADDER:
asig  moogladder  ain, kcf, kres[, istor]

A new implementation of the Moog ladder filter, based on
Antti Huovilainen's design (Huovilainen, A. "Non-linear Digital
Implementation of the Moog Ladder Filter". DAFX 2004, Napoli).

ain: input signal.
kcf: lowpass cuttoff frequency (Hz).
kres: resonance, generally < 1, but not limited to it.
Higher than 1 resonance values might cause aliasing,
analogue synths generally allow resonances to be above 1.
istor: defaults to 1, initialise the internal delay buffers
on startup. 0 means no initialisation.

STATEVAR:
ahp,alp,abp,abr  statevar  ain, kcf, kq [, iosamps, istor]

A digital implementation of the analogue state-variable filter.
This filter has four simultaneous outputs: high-pass, low-pass,
band-pass and band-reject.

ahp: high-pass output
alp: low-pass output
abp: band-pass output
abr: band-reject output

ain: input signal
kcf: filter frequency (Hz)
kq: filter Q. This value is limited internally depending on
the frequency and the number of times of oversampling used in
the process (3-times oversampling by default).
iosamps: number of times of oversampling used in the
filtering process. This will determine the maximum sharpness
of the filter resonance (Q). More oversampling allows higher Qs,
less oversampling will limit the resonance. The default is 3
times (iosamps=0).
istor: defaults to 1, initialise the internal delay buffers
on startup. 0 means no initialisation.

FOFILTER:
ar fofilter asig, kcf, kris, kdec[,istor]

This filter generates a stream of overlapping sinewave grains, when
fed with a pulse train.  Each grain is the impulse response of a
combination of two BP filters. The grains are defined by their attack
time (determining the skirtwidth of the formant region at -60dB) and
decay time (-6dB bandwidth). Overlapping will occur when 1/freq <
decay, but, unlike FOF, there is no upper limit on the number of
overlaps.  The original idea for this opcode came from J McCartney's
formlet class in SuperCollider, but this is possibly implemented
differently(?).

asig: input signal
kcf: formant center frequency (Hz)
kris: impulse response attack time (secs)
kdec: impulse response decay time (to -60dB, in secs)
istor: defaults to 1, initialise the internal delay buffers
on startup. 0 means no initialisation.

*/

#ifndef _NEWFILS_H
#define _NEWFILS_H

typedef struct _moogladder {
  OPDS    h;
  MYFLT   *out;
  MYFLT   *in;
  MYFLT   *freq;
  MYFLT   *res;
  MYFLT   *istor;

  double  delay[6];
  double  tanhstg[3];
  MYFLT   oldfreq;
  MYFLT   oldres;
  double  oldacr;
  double  oldtune;
} moogladder;

static int32_t moogladder_init(CSOUND *csound,moogladder *p);
static int32_t moogladder_process(CSOUND *csound,moogladder *p);

typedef struct _statevar {
  OPDS    h;
  MYFLT   *outhp;
  MYFLT   *outlp;
  MYFLT   *outbp;
  MYFLT   *outbr;
  MYFLT   *in;
  MYFLT   *freq;
  MYFLT   *res;
  MYFLT   *osamp;
  MYFLT   *istor;

  double  bpd;
  double  lpd;
  double  lp;
  int32_t     ostimes;
  MYFLT   oldfreq;
  MYFLT   oldres;
  double  oldq;
  double  oldf;
} statevar;

static int32_t statevar_init(CSOUND *csound,statevar *p);
static int32_t statevar_process(CSOUND *csound,statevar *p);

typedef struct _fofilter {
  OPDS    h;
  MYFLT   *out;
  MYFLT   *in;
  MYFLT   *freq;
  MYFLT   *ris;
  MYFLT   *dec;
  MYFLT   *istor;

  double  delay[4];
} fofilter;

static int32_t fofilter_init(CSOUND *csound,fofilter *p);
static int32_t
fofilter_process(CSOUND *csound,fofilter *p);

#endif

