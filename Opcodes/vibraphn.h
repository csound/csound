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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
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
    cs_float       *ar;                  /* Output */
    cs_float       *amplitude, *frequency;
    cs_float       *hardness, *spos, *ifn;
    cs_float       *vibFreq, *vibAmt, *ivfn, *dettack;

    Modal4      m4;
    cs_float       strikePosition;
    cs_float       stickHardness;
    int32_t         first;
    cs_double      kloop;
} VIBRAPHN;

#endif
