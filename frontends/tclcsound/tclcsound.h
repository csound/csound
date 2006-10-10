/*
  Copyright (C) 2005 Victor Lazzarini

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

#ifndef TCLCSOUND_H
#define TCLCSOUND_H

#include <stdio.h>
#include <string.h>
#include <csound.h>
#include <string.h>
#include <stdlib.h>
#include <tcl.h>
#include <tk.h>

/* Csound performance status

CS_READY: ready for compilation, effectively stopped
CS_COMPILED: ready for performance, but not yet running
CS_RUNNING: running, producing audio
CS_PAUSED: paused, but ready for performance

Csound will switch from CS_RUNNING to CS_COMPILED once
the loaded score has finished playing.
*/

typedef struct __ctlchn {
    char   *name;
    double  value;
    struct __ctlchn *next;
} ctlchn;

typedef struct __pvsctlchn {
    int n;
    PVSDATEXT data;
    struct __pvsctlchn *next;
} pvsctlchn;

typedef struct __csdata {
    CSOUND *instance;           /* csound object */
    int     result;             /* action result */
    void   *threadID;           /* processing thread ID */
    int     status;             /* perf status */
    ctlchn *inchan;
    ctlchn *outchan;
    Tcl_Interp *interp;
    char *mbuf;                /* message buffer */
    char mess[50];             /* message recipient name */
    pvsctlchn *pvsinchan;
    pvsctlchn *pvsoutchan;
    void  *threadlock;
  void *messlock;
} csdata;

extern int tclcsound_initialise(Tcl_Interp * interp);

#endif

