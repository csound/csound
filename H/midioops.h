/*
    midioops.h:

    Copyright (C) 1997 John ffitch

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

/* midioops.h */
/*  John ffitch -- August 1997 */

/* **********************************************************************
 * Defines etc for the basic MIDI output codes
 * **********************************************************************/

#define MD_NOTEOFF      (0x80)
#define MD_NOTEON       (0x90)
#define MD_POLYAFTER    (0xa0)
#define MD_CNTRLCHG     (0xb0)
#define MD_PGMCHG       (0xc0)
#define MD_CHANPRESS    (0xd0)
#define MD_PTCHBENDCHG  (0xe0)

