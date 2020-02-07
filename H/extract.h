/*
 extract.h:

 Copyright (C) 2013 Steven Yi

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

#ifndef EXTRACT_H
#define EXTRACT_H

#define INSMAX  4096

typedef struct extractStatics__ {
    char    inslst[INSMAX];         /*   values set by readxfil         */
    int     sectno, a0done;
    int     onsect, offsect;        /*      "       "       "           */
    MYFLT   onbeat, offbeat;        /*      "       "       "           */
    MYFLT   ontime, offtime;        /* set by readxfil, mod by w-stmnt  */
    SRTBLK  *frstout, *prvout;      /* links for building new outlist   */
    SRTBLK  a0;
    SRTBLK  f0;
    SRTBLK  e;
} EXTRACT_STATICS;

/* read the extract control file */
void readxfil(CSOUND *csound, EXTRACT_STATICS* extractStatics, FILE *xfp);

/* extract instr events within the time period */
void extract(CSOUND *csound, EXTRACT_STATICS* extractStatics);


#endif
