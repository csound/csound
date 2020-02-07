/*
    fgens.h:

    Copyright (C) 1991 Barry Vercoe

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
                                                /*      FGENS.H         */
#ifndef CSOUND_FGENS_H
#define CSOUND_FGENS_H

#define MAXFNUM 100
#define GENMAX  60

/**
 * Create ftable using evtblk data, and store pointer to new table in *ftpp.
 * If mode is zero, a zero table number is ignored, otherwise a new table
 * number is automatically assigned.
 * Returns zero on success.
 */
int hfgens(CSOUND *csound, FUNC **ftpp, const EVTBLK *evtblkp, int mode);

/**
 * Allocates space for 'tableNum' with a length (not including the guard
 * point) of 'len' samples. The table data is not cleared to zero.
 * Return value is zero on success.
 */
int csoundFTAlloc(CSOUND *csound, int tableNum, int len);

/**
 * Deletes a function table.
 * Return value is zero on success.
 */
int csoundFTDelete(CSOUND *csound, int tableNum);

#endif  /* CSOUND_FGENS_H */

