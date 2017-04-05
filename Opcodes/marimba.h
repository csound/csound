/*
    mandolin.c:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*******************************************/
/*  Marimba SubClass of Modal4 Instrument, */
/*  by Perry R. Cook, 1995-96              */
/*                                         */
/*   Controls:               stickHardness */
/*                           strikePosition*/
/*                           vibFreq       */
/*                           vibAmt        */
/*******************************************/

#if !defined(__Marimba_h)
#define __Marimba_h
#include "modal4.h"

typedef struct Marimba {
    OPDS        h;
    MYFLT       *ar;                  /* Output */
    MYFLT       *amplitude, *frequency;
    MYFLT       *hardness, *spos, *ifn;
    MYFLT       *vibFreq, *vibAmt, *ivfn, *dettack;
    MYFLT       *doubles, *triples;
/* Modal4 */
    Modal4      m4;
    int         multiStrike;
    MYFLT       strikePosition;
    MYFLT       stickHardness;
    int         first;
    int         kloop;
} MARIMBA;

#endif
