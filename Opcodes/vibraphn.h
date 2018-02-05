/*
    vibraphn.h:

    Copyright (C) 1996, 1997 Perry Cook, John ffitch

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

/*******************************************/
/*  Vibraphone SubClass of Modal4          */
/*  Instrument, by Perry R. Cook, 1995-96  */
/*                                         */
/*   Controls:    stickHardness            */
/*                strikePosition           */
/*                vibFreq                  */
/*                vibAmt                   */
/*******************************************/

#if !defined(__Vibraphn_h)
#define __Vibraphn_h

typedef struct Vibraphn {
    OPDS        h;
    MYFLT       *ar;                  /* Output */
    MYFLT       *amplitude, *frequency;
    MYFLT       *hardness, *spos, *ifn;
    MYFLT       *vibFreq, *vibAmt, *ivfn, *dettack;

    Modal4      m4;
    MYFLT       strikePosition;
    MYFLT       stickHardness;
    int32_t         first;
    int32_t
    kloop;
} VIBRAPHN;

#endif
