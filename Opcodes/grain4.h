/*
    grain4.h:

    Copyright (C) 1994, 1995 Allan S C Lee, John ffitch

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

#pragma once

#define MAXVOICE 128

typedef struct {
        OPDS    h;
        MYFLT   *ar, *xamp, *ivoice, *iratio;
        MYFLT   *imode, *ithd, *ifn, *ipshift;
        MYFLT   *igskip;
        MYFLT   *igskip_os;
        MYFLT   *ilength, *kgap, *igap_os, *kgsize, *igsize_os, *iatt, *idec;
        MYFLT   *iseed, *ipitch1, *ipitch2, *ipitch3, *ipitch4, *ifnenv;
        int32   fpnt[MAXVOICE], cnt[MAXVOICE], gskip[MAXVOICE], gap[MAXVOICE];
        int32   gsize[MAXVOICE], stretch[MAXVOICE], mode[MAXVOICE];
        MYFLT   pshift[MAXVOICE], phs[MAXVOICE];
        int16   grnd;
        //int32   clock;
        int32   gskip_os;
        int32_t gstart, gend, glength;
        MYFLT   gap_os, gsize_os;
        FUNC    *ftp, *ftp_env;
} GRAINV4;

/*
ar - audio results
xamp - amplitude
ivoice - number of voices
iratio - ratio of the speed of the gskip pointer relative to audio sample rate.
   0.5 will be half speed.
imode - +1 grain pointer move forward (same direction of the gskip pointer),
   -1 backward (oppose direction fo the gskip pointer) or 0 for random
ithd - threshold, if the sample signal is smaller then ithd, it will be skiped
ifn - function table number
ipshift - pitch shift control. If ipshift set to 0, pitch will be set randomly
   up and dow an octave. If ipshift set to 1, 2, 3 or 4, up to four different
   pitches can be set amount all the voices set in ivoice, optional parmaters
   ipitch1, ipitch2, ipitch3 and ipitch4 will be used.
igskip - initial skip from the beginning of the function table in sec.
igskip_os - gskip pointer random offset in sec, 0 will be no offset.
ilength - length of the table to be used start from igskip in sec.
kgap - gap in sec.
igap_os - gap random offset in % of the gap size, 0 will be no offset.
kgsize - grain size in sec.
igsize_os - grain size random offset in % of grain size.
iatt - attack in % of grain size.
idec - decade in % of grain size.
[iseed] - optional, seed for the random number generator, default is 0.5.
[ipitch1], [ipitch2], [ipitch3], [ipitch4] - optional, pitch shift parameter
   if ipshift set to 1, 2, 3 or 4. Range: 0.1 (down 10 octave) to
   10 (up 10 octave). Default value is 1, original pitch.
[ifnenv] Optional function table number to be used to generate the
   shape of the rise and decade of envlop.
*/
