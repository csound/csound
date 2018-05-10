/*
    3Dug.h:

    Copyright (C) 1995, 2001 Eli Breder, David McIntyre

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

#ifndef _3DUG_H_
#define _3DUG_H_

#define SAMP_RATE 44100  /* sampling rate of HRTFs */

#define FILT_LEN 128L
#define FILT_LENm1 (FILT_LEN-1)
#define BUF_LEN (FILT_LEN*2)
#define BLOCK_SIZE 1024

#define MIN_ELEV        -40
#define MAX_ELEV        90
#define ELEV_INC        10
#define N_ELEV          (((MAX_ELEV - MIN_ELEV) / ELEV_INC) + 1)
#define MIN_AZIM        -180
#define MAX_AZIM        180

/*
 * This array gives the total number of azimuths measured
 * per elevation, and hence the AZIMUTH INCREMENT. Note that
 * only azimuths up to and including 180 degrees actually
 * exist in file system (because the data is symmetrical.
 */

/* int32_t elevation_data[N_ELEV] = {56, 60, 72, 72, 72, 72, 72, 60, 56, 45, */
/*                               36, 24, 12, 1 };                        */

typedef struct {
    MYFLT left[256];  /* left and right will hold FFTed values of HRTFs */
    MYFLT right[256];
} HRTF_DATUM;

#endif
