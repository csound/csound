/*
    dam.h:

    Copyright (C) 1997 Marc Resibois

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

#include "csdl.h"

#define POWER_BUFSIZE 1000

typedef struct {
   OPDS h ;

   MYFLT *aout ;      /* Declare output array first  */
   MYFLT *ain ;       /* Input array   */
   MYFLT *kthreshold ;/* sound level threshold */
   MYFLT *icomp1 ;    /* Compression factors */
   MYFLT *icomp2 ;
   MYFLT *rtime ;         /* Raise/Fall times */
   MYFLT *ftime ;

   MYFLT rspeed ;
   MYFLT fspeed ;

   MYFLT gain ;
   MYFLT power ;
   MYFLT powerBuffer[POWER_BUFSIZE] ;
   MYFLT *powerPos ;
   MYFLT kthr;

} DAM ;

/* void daminit(DAM *p) ; */
/* void dam(DAM *p) ; */

