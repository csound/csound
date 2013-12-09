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
    data->bkpt_anchor = (bkpt_node_t *) malloc(sizeof(bkpt_node_t));
    data->bkpt_anchor->line = -1;
    data->bkpt_anchor->next = NULL;
    data->debug_instr_ptr = NULL;
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

PUBLIC void csoundDebugSetMode(CSOUND *csound, debug_mode_t enabled)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    if (enabled)
    data->csdebug_on = enabled;
}

PUBLIC void csoundSetBreakpoint(CSOUND *csound, int line)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    if (line < 0) {
        csound->Warning(csound, Str("Negative line for breakpoint invalid."));
    }
    bkpt_node_t *newpoint = (bkpt_node_t *) malloc(sizeof(bkpt_node_t));
    newpoint->line = line;
    newpoint->instr = 0;
    newpoint->mode = CSDEBUG_BKPT_LINE;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint, 1);
}

PUBLIC void csoundRemoveBreakpoint(CSOUND *csound, int line)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    if (line < 0) {
        csound->Warning(csound, Str ("Negative line for breakpoint invalid."));
    }
    bkpt_node_t *newpoint = (bkpt_node_t *) malloc(sizeof(bkpt_node_t));
    newpoint->line = line;
    newpoint->instr = 0;
    newpoint->mode = CSDEBUG_BKPT_DELETE;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint, 1);
}

PUBLIC void csoundSetInstrumentBreakpoint(CSOUND *csound, MYFLT instr)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    bkpt_node_t *newpoint = (bkpt_node_t *) malloc(sizeof(bkpt_node_t));
    newpoint->line = -1;
    newpoint->instr = instr;
    newpoint->mode = CSDEBUG_BKPT_INSTR;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint, 1);
}

PUBLIC void csoundRemoveInstrumentBreakpoint(CSOUND *csound, MYFLT instr)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    bkpt_node_t *newpoint = (bkpt_node_t *) malloc(sizeof(bkpt_node_t));
    newpoint->line = -1;
    newpoint->instr = instr;
    newpoint->mode = CSDEBUG_BKPT_DELETE;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint, 1);
}

PUBLIC void csoundClearBreakpoints(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    bkpt_node_t *newpoint;
    newpoint->mode = CSDEBUG_BKPT_CLEAR_ALL;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint, 1);
}

PUBLIC void csoundSetBreakpointCallback(CSOUND *csound, breakpoint_cb_t bkpt_cb)
{

    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    data->bkpt_cb = bkpt_cb;
}

/* FIXME make command operations atomic, and make them block until message has been processed */
PUBLIC void csoundDebugStepOver(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    data->command = CSDEBUG_CMD_STEPOVER;
}

PUBLIC void csoundDebugStepInto(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    data->command = CSDEBUG_CMD_STEPINTO;
}

PUBLIC void csoundDebugNext(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    data->command = CSDEBUG_CMD_NEXT;
}

PUBLIC void csoundDebugContinue(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    data->command = CSDEBUG_CMD_CONTINUE;
}

PUBLIC void csoundDebugStop(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    data->command = CSDEBUG_CMD_STOP;
}

PUBLIC INSDS *csoundDebugGetInstrument(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    return data->debug_instr_ptr;
}
