/*
    crossfm.h:

    Copyright (C) 2005 Francois Pinot

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

/*********************************************************************/
/* Header file for the plugin opcode crossfm                         */
/*                                                                   */
/* crossfm implements a crossed frequency modulation algorithm.      */
/* The audio-rate output of oscillator #1 is used to modulate the    */
/* frequency input of oscillator #2 and the audio-rate output of     */
/* oscillator #2 is used to modulate the frequency input of          */
/* oscillator #1.                                                    */
/*                                                                   */
/* a1,a2   crossfm xfrq1, xfrq2, xndx1, xndx2, kcps, ifn1, ifn2      */
/*                 [, iphs1] [, iphs2]                               */
/*                                                                   */
/* kcps  - base frequency                                            */
/* xfrq1 - frequency multiplier for oscillator #1. Oscillator #1     */
/*         effective frequency is kcps * xfrq1                       */
/* xfrq2 - frequency multiplier for oscillator #2. Oscillator #2     */
/*         effective frequency is kcps * xfrq2                       */
/* xndx1 - index of the modulation of oscillator #2 by oscillator #1 */
/* xndx2 - index of the modulation of oscillator #1 by oscillator #2 */
/* ifn1  - function table number for oscillator #1                   */
/* ifn2  - function table number for oscillator #2                   */
/* iphs1 - optional, initial phase of waveform in table #1           */
/* iphs2 - optional, initial phase of waveform in table #2           */
/*                                                                   */
/*********************************************************************/

#pragma once

typedef struct {
  OPDS h;                                     /* common to all opcodes */

  MYFLT *aout1, *aout2;                       /* output args */
  MYFLT *xfrq1, *xfrq2, *xndx1, *xndx2;       /* input args  */
  MYFLT *kcps, *ifn1, *ifn2;                  /* input args  */
  MYFLT *iphs1, *iphs2;                       /* input args  */

  MYFLT phase1, phase2;                       /* phase of oscillators       */
  MYFLT sig1, sig2;                           /* a-rate oscillators outputs */
  MYFLT siz1, siz2;                           /* size of function tables    */
  FUNC  *ftp1, *ftp2;                         /* function table pointers    */
  short frq1adv, frq2adv, ndx1adv, ndx2adv;   /* increment values for xargs */
                                              /* pointers (0 for i-rate and */
                                              /* k-rate, 1 for a-rate args  */
} CROSSFM;
