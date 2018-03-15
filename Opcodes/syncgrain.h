/* syncgrain.h:
   Synchronous granular synthesis

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

SYNCGRAIN:

Syncgrain implements synchronous granular synthesis. The source sound
for the grains is obtained by reading a function table containing the
samples of the source waveform.  For sampled-sound sources, GEN01 is
used. Syncgrain will accept deferred allocation tables (with aif
files).

The grain generator has full control of frequency (grains/sec),
overall amplitude, grain pitch (a sampling increment) and grain size
(in secs), both as fixed or time-varying (signal) parameters.  An
extra parameter is the grain pointer speed (or rate), which controls
which position the generator will start reading samples in the table
for each successive grain. It is measured in fractions of grain size,
so a value of 1 (the default) will make each successive grain read
from where the previous grain should finish. A value of 0.5 will make
the next grain start at the midway position from the previous grain
start and finish, etc.. A value of 0 will make the generator read
always from a fixed position of the table (wherever the pointer was
last at).  A negative value will decrement pointer positions. This
control gives extra flexibility for creating timescale modifications
in the resynthesis.

Syncgrain will generate any number of parallel grain streams (which
will depend on grain density/frequency), up to the olaps value
(default 100). The number of streams (overlapped grains) is determined
by grainsize*grain_freq. More grain overlaps will demand more
calculations and the synthesis might not run in realtime (depending on
processor power).

Syncgrain can simulate FOF-like formant synthesis, provided that a
suitable shape is used as grain envelope and a sinewave as the grain
wave. For this use, grain sizes of around 0.04 secs can be used. The
formant centre frequency is determined by the grain pitch.  Since this
is a sampling increment, in order to use a frequency in Hz, that value
has to be scaled by tablesize/sr. Grain frequency will determine the
fundamental.

Syncgrain uses floating-point indexing, so its precision is not
affected by large-size tables.

asig  syncgrain  kamp, kfreq, kpitch, kgrsize, kprate, ifun1, ifun2, iolaps

kamp: amplitude scaling
kfreq: frequency of grain generation, or density, in grains/sec.
kpitch: grain pitch scaling (1=normal pitch, < 1 lower, > 1 higher;
        negative, backwards)
kgrsize: grain size in secs.
kprate: grain pointer rate, in relation to grainsize.
The value of 1 will advance the reading pointer 1 grain ahead in the source table.
Larger values will time-compress and smaller values will time-expand the source
signal. Negative values will cause the pointer to run backwards and zero will
        freeze it.
ifun1: source signal function table. Deferred-allocation tables are accepted.
ifun2: grain envelope function table.
iolaps: maximum number of overlaps, max(kfreq)*max(kgrsize).

The syncgrain opcode is based on an improved version of the original
SndObj library SyncGrain class.

SYNCLOOP:

A variation on syncgrain allowing for loop points to be set, as well
as sound start position

asig  syncloop kamp, kfreq, kpitch, kgrsize, kprate,kloopstart, kloopend, \
               ifun1, ifun2, iolaps [, istart]

parameters are as above, with the following additions:

kloopstart - loop start point (in secs)
kloopend - loop end point (in secs)
istart - start position (in secs), defaults to 0.

*/

#ifndef _SYNCGRAIN_H
#define _SYNCGRAIN_H

typedef struct _syncgrain {
    OPDS h;
    MYFLT *output;
    MYFLT *amp;
    MYFLT *fr;
    MYFLT *pitch;
    MYFLT *grsize;
    MYFLT *prate;
    MYFLT *ifn1;
    MYFLT *ifn2;
    MYFLT *ols;
    FUNC  *sfunc;
    FUNC  *efunc;
    int32_t count, numstreams, firststream;
    int32_t datasize, envtablesize, olaps;
    AUXCH streamon;
    AUXCH index;
    AUXCH envindex;
    AUXCH envincr;
    float start,frac;
} syncgrain;

typedef struct _syncgrainl {
    OPDS h;
    MYFLT *output;
    MYFLT *amp;
    MYFLT *fr;
    MYFLT *pitch;
    MYFLT *grsize;
    MYFLT *prate;
    MYFLT *loop_start;
    MYFLT *loop_end;
    MYFLT *ifn1;
    MYFLT *ifn2;
    MYFLT *ols;
    MYFLT *startpos;
  MYFLT *iskip;
    FUNC  *sfunc;
    FUNC  *efunc;
    int32_t count, numstreams, firststream;
    int32_t datasize, envtablesize, olaps;
    AUXCH streamon;
    AUXCH index;
    AUXCH envindex;
    float start,frac;
    int32_t firsttime;
} syncgrainloop;

static int32_t syncgrain_process(CSOUND *csound, syncgrain *p);
static int32_t
syncgrain_init(CSOUND *csound, syncgrain *p);

#endif

