/*
    diskin.h:

    Copyright (C) 1998, 2001 matt ingalls, Richard Dobson, John ffitch

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

#define SNDINEWBUFSIZ  (4096)
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

typedef struct {
  OPDS          h;
  MYFLT         *r1, *r2, *r3, *r4;
  MYFLT         *ifilno, *ktransp, *iskptim, *ilooping, *iformat, *skipinit;
  short         format, channel, nchanls, sampframsiz, filetyp;
  short         analonly, endfile, begfile;
  long          sr, audrem, audsize;
  AIFFDAT       *aiffdata;
  FDCH          fdch;
  MYFLT         *inbufp, *bufend, *guardpt;
  MYFLT         inbuf[SNDINEWBUFSIZ];
  double        phs;
  long          filepos, firstsampinfile;
  /*RWD 3:2000*/
  float         fscalefac;
  long          do_floatscaling;
} SOUNDINEW;


