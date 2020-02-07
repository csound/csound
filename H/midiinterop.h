/*
    midiinterop.h:

    Copyright (C) 2002 Michael Gogins

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

#ifndef MIDIINTEROP_H
#define MIDIINTEROP_H

#include "csoundCore.h"

/*

MIDI INTEROPERABILITY OPCODES

midinoteoff                     xkey, xvelocity
midinoteonkey                   xkey, xvelocity
midinoteoncps                   xcps, xvelocity
midinoteonoct                   xoct, xvelocity
midinoteonpch                   xpch, xvelocity
midipolyaftertouch              xpolyaftertouch, xcontrollervalue [, olow, hhigh]
midicontrolchange,              xcontroller, xcontrollervalue [, olow, hhigh]
midiprogramchange               xprogram
midichannelaftertouch           xchannelaftertouch [, olow, hhigh]
midipitchbend                   xpitchbend [, olow, hhigh]
mididefault                     xdefault, xvalue

Description

These opcodes are designed to simplify writing instruments that can be
used interchangeably for either score or MIDI input, and to make it
easier to adapt instruments originally written for score input to work
with MIDI input.

In general, it should be possible to write instrument definitions that
work identically with both scores and MIDI, including both MIDI files
and real-time MIDI input, without using any conditional statements,
and that take full advantage of MIDI voice messages.

Note that correlating Csound instruments with MIDI channel numbers is
done using the massign opcode for real-time performance.  For
file-driven performance, instrument numbers default to MIDI channel
number + 1, but the defaults are overridden by any MIDI program change
messages in the file.

Initialization

xkey - returns MIDI key during MIDI activation, remains unchanged otherwise
kcps - returns MIDI key translated to cycles per second during MIDI
       activation, remains unchanged otherwise
xoct - returns MIDI key translated to linear octaves during MIDI
       activation, remains unchanged otherwise
xpch - returns MIDI key translated to octave.pch during MIDI
       activation, remains unchanged otherwise
xvelocity - returns MIDI velocity during MIDI activation, remains
       unchanged otherwise
xpolyaftertouch - returns MIDI polyphonic aftertouch during MIDI
       activation, remains unchanged otherwise
xcontroller - specifies a MIDI controller number
xcontrollervalue - returns the value of the MIDI controller during
       MIDI activation, remains unchanged otherwise
xprogram - returns the MIDI program change value during MIDI
       activation, remains unchanged otherwise
xchannelaftertouch - returns the MIDI channel aftertouch during MIDI
       activation, remains unchanged otherwise
xpitchbend - returns the MIDI pitch bend during MIDI activation,
       remains unchanged otherwise
xdefault - specifies a default value that will be used during MIDI activation
xvalue - overwritten by xdefault during MIDI activation, remains
       unchanged otherwise
olow - optional low value after rescaling, defaults to 0.
hhigh - optional high value after rescaling, defaults to 127.

Performance

for the first 10 opcodes, if the instrument was activated by MIDI
input, the opcode overwrites the value of the "x" input variable(s)
with the corresponding value from MIDI input.  If the instrument was
NOT activated by MIDI input, the value of the "x" input variable(s)
remains unchanged.

This enables score pfields to receive MIDI input data during MIDI
activation, and score values otherwise.

For the mididefault opcode, if the instrument was activated by MIDI
input, the opcode will overwrite the value of xvalue with the value of
xdefault.  If the instrument was NOT activated by MIDI input, xvalue
will remain unchanged.

This enables score pfields to receive a default value during MIDI
activation, and score values otherwise.

To adapt an ordinary Csound instrument designed for score activation
for score/MIDI interoperability:

1.      Change all linen, linseg, and expseg opcodes to linenr,
        linsegr, and expsegr, respectively.  This will not materially
        change score-driven performance.

2.      Add the following lines at the beginning of the instrument
        definition:
        mididefault 60, p3 ; Ensures that a MIDI-activated instrument
        will have a positive p3 field.
        midinoteoncps p4, p5 ; Puts MIDI key translated to cycles per
        second into p4, and MIDI velocity into p5

        Obviously, midinoteoncps could be changed to midinoteonoct or
        any of the other options, and the choice of pfields is arbitrary.

Author: Michael Gogins
                gogins@pipeline.com

*/

typedef struct MIDINOTEON_
{
        OPDS h;
        MYFLT *xkey;
        MYFLT *xvelocity;
}
MIDINOTEON;

typedef struct MIDIPOLYAFTERTOUCH_
{
        OPDS h;
        MYFLT *xpolyaftertouch;
        MYFLT *xcontroller;
        MYFLT *olow;
        MYFLT *hhigh;
}
MIDIPOLYAFTERTOUCH;

typedef struct MIDICONTROLCHANGE_
{
        OPDS h;
        MYFLT *xcontroller;
        MYFLT *xcontrollervalue;
        MYFLT *olow;
        MYFLT *hhigh;
}
MIDICONTROLCHANGE;

typedef struct MIDIPROGRAMCHANGE_
{
        OPDS h;
        MYFLT *xprogram;
}
MIDIPROGRAMCHANGE;

typedef struct MIDICHANNELAFTERTOUCH_
{
        OPDS h;
        MYFLT *xchannelaftertouch;
        MYFLT *olow;
        MYFLT *hhigh;
}
MIDICHANNELAFTERTOUCH;

typedef struct MIDIPITCHBEND_
{
        OPDS h;
        MYFLT *xpitchbend;
        MYFLT *olow;
        MYFLT *hhigh;
}
MIDIPITCHBEND;

typedef struct MIDIDEFAULT_
{
        OPDS h;
        MYFLT *xdefault;
        MYFLT *xvalue;
}
MIDIDEFAULT;

#endif
