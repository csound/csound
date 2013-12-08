/*
    csdebug.h:

    Copyright (C) 2013 Andres Cabrera

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

#ifndef CSDEBUG_H
#define CSDEBUG_H

#include "pthread.h"
#include "csound.h"

typedef struct bkpt_node_s {
    int line;
    struct bkpt_node_s *next;
} bkpt_node_t;

typedef struct {

    void *bkpt_buffer; /* for passing breakpoints to the running engine */
    bkpt_node_t *bkpt_anchor;
    pthread_mutex_t bkpt_mutex;
} csdebug_data_t;

PUBLIC void csoundDebuggerInit(CSOUND *csound);
PUBLIC void csoundDebuggerClean(CSOUND *csound);

PUBLIC void csoundSetBreakpoint(CSOUND *csound, int line);
PUBLIC void csoundClearBreakpoints(CSOUND *csound);

PUBLIC void csoundDebugStep(CSOUND *csound);
PUBLIC void csoundDebugContinue(CSOUND *csound);
PUBLIC void csoundDebugStop(CSOUND *csound);



#endif
