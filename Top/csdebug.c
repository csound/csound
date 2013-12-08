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

#include <assert.h>

#include "csdebug.h"

#include "csoundCore.h"

PUBLIC void csoundDebuggerInit(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *)malloc(sizeof(csdebug_data_t));
    data->bkpt_anchor = (bkpt_node_t *) malloc(sizeof(bkpt_node_t *));
    data->bkpt_anchor->line = -1;
    data->bkpt_anchor->next = NULL;

    data->bkpt_buffer = csoundCreateCircularBuffer(csound, 64, sizeof(bkpt_node_t **));
    csound->csdebug_data = data;
}

PUBLIC void csoundDebuggerClean(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    bkpt_node_t *node = data->bkpt_anchor;
    csoundDestroyCircularBuffer(csound, data->bkpt_buffer);
    while (node) {
        bkpt_node_t *oldnode = node;
        node = node->next;
        free(oldnode);
    }
    free(data);
    csound->csdebug_data = NULL;
}

PUBLIC void csoundSetBreakpoint(CSOUND *csound, int line)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    if (line < 0) {
        csound->Warning(csound, Str("Negative line for breakpoint invalid."));
    }
    bkpt_node_t newpoint;
    newpoint.line = line;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint, 1);
}

PUBLIC void csoundClearBreakpoints(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    bkpt_node_t newpoint;
    newpoint.line = -1;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint, 1);
}
