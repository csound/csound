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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#pragma once
#include "stdopcod.h"
#define POWER_BUFSIZE 1000

typedef struct {
   OPDS h ;

   cs_float *aout ;      /* Declare output array first  */
   cs_float *ain ;       /* Input array   */
   cs_float *kthreshold ;/* sound level threshold */
   cs_float *icomp1 ;    /* Compression factors */
   cs_float *icomp2 ;
   cs_float *rtime ;         /* Raise/Fall times */
   cs_float *ftime ;

   cs_float rspeed ;
   cs_float fspeed ;

   cs_float gain ;
   cs_double power ;    /* Limit cancellation drift in the running sum. */
   cs_float powerBuffer[POWER_BUFSIZE] ;
   cs_float *powerPos ;
   cs_float kthr;

} DAM ;

/* void daminit(DAM *p) ; */
/* void dam(DAM *p) ; */

