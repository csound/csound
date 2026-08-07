/*
    csound_debug.c: csound debugger

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include <assert.h>
#include <string.h>

#include "csdebug.h"
#include "udo.h"
#include "pstream.h"   /* PVSDAT / CMPLX layout for csoundDebugSerializeFsig */

int32_t kperf(CSOUND *csound);
int32_t kperf_debug(CSOUND *csound);  
debug_instr_t *csoundDebugGetCurrentInstrInstance(CSOUND *csound);
debug_opcode_t *csoundDebugGetCurrentOpcodeList(CSOUND *csound);
void csoundDebugFreeOpcodeList(CSOUND *csound, debug_opcode_t *opcode_list);
int32_t dag_get_task(CSOUND *csound, int32_t index, int32_t numThreads,
                     int32_t next_task);
int32_t dag_end_task(CSOUND *csound, int32_t task);
void dag_build(CSOUND *csound, INSDS *chain);
void dag_reinit(CSOUND *csound);
void message_dequeue(CSOUND *csound);
int32_t sense_events(CSOUND *);
int32_t csound_node_perf(CSOUND *csound, int32_t index,
                  int32_t numThreads);

void csoundDebuggerBreakpointReached(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    debug_bkpt_info_t bkpt_info;
    bkpt_info.breakpointInstr = csoundDebugGetCurrentInstrInstance(csound);
    bkpt_info.instrListHead = csoundDebugGetInstrInstances(csound);
    bkpt_info.currentOpcode = csoundDebugGetCurrentOpcodeList(csound);
    bkpt_info.instrVarList = csoundDebugGetVariables(csound,
                                                     bkpt_info.breakpointInstr);
    if (data->bkpt_cb) {
      data->bkpt_cb(csound, &bkpt_info, data->cb_data);
    } else {
      csoundMessage(csound, Str("Breakpoint callback not set. Breakpoint Reached."));
    }
    // TODO: These free operations could be moved to a low priority context
    csoundDebugFreeInstrInstances(csound, bkpt_info.breakpointInstr);
    csoundDebugFreeInstrInstances(csound, bkpt_info.instrListHead);
    if (bkpt_info.currentOpcode) {
        csoundDebugFreeOpcodeList(csound, bkpt_info.currentOpcode);
    }
    csoundDebugFreeVariables(csound, bkpt_info.instrVarList);
}

 void csoundDebuggerInit(CSOUND *csound)
{
    /* Idempotent: do nothing if already initialized */
    if (csound->csdebug_data != NULL) return;
    csdebug_data_t *data =
      (csdebug_data_t *) csound->Malloc(csound, sizeof(csdebug_data_t));
    data->bkpt_anchor = (bkpt_node_t *) csound->Malloc(csound, sizeof(bkpt_node_t));
    data->bkpt_anchor->line = -1;
    data->bkpt_anchor->next = NULL;
    data->debug_instr_ptr = NULL;
    data->debug_opcode_ptr = NULL;
    data->bkpt_cb = NULL;
    data->status = CSDEBUG_STATUS_RUNNING;
    data->bkpt_buffer = csoundCreateCircularBuffer(csound,
                                                   64, sizeof(bkpt_node_t **));
    data->cmd_buffer = csoundCreateCircularBuffer(csound,
                                                  64, sizeof(debug_command_t));
    csound->csdebug_data = data;
    csound->kperf = kperf_debug;
}

 void csoundDebuggerClean(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    bkpt_node_t *node = data->bkpt_anchor;
    csoundDestroyCircularBuffer(csound, data->bkpt_buffer);
    csoundDestroyCircularBuffer(csound, data->cmd_buffer);
    while (node) {
        bkpt_node_t *oldnode = node;
        node = node->next;
        csound->Free(csound, oldnode);
    }
    csound->Free(csound, data);
    csound->csdebug_data = NULL;
    csound->kperf = kperf;
}

 void csoundDebugStart(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    data->status = CSDEBUG_STATUS_RUNNING;
}

 void csoundSetBreakpoint(CSOUND *csound, int32_t line, int32_t instr, int32_t skip)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    if (!data) {
      csound->Warning(csound,
                      Str("csoundSetBreakpoint: cannot set breakpoint. "
                          "Debugger is not initialised."));
      return;
    }
    if (line <= 0) {
      csound->Warning(csound, Str("csoundSetBreakpoint: line > 0 for breakpoint."));
      return;
    }
    bkpt_node_t *newpoint =
      (bkpt_node_t *) csound->Malloc(csound, sizeof(bkpt_node_t));
    newpoint->line = line;
    newpoint->instr = instr;
    newpoint->skip = skip;
    newpoint->count = skip;
    newpoint->mode = CSDEBUG_BKPT_LINE;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint,  1);
}

 void csoundRemoveBreakpoint(CSOUND *csound, int32_t line, int32_t instr)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    if (!data) {
      csound->Warning(csound,
                      Str("csoundRemoveBreakpoint: cannot remove breakpoint. "
                          "Debugger is not initialised."));
      return;
    }
    if (line < 0) {
      csound->Warning(csound, Str ("Negative line for breakpoint invalid."));
    }
    bkpt_node_t *newpoint =
      (bkpt_node_t *) csound->Malloc(csound, sizeof(bkpt_node_t));
    newpoint->line = line;
    newpoint->instr = instr;
    newpoint->mode = CSDEBUG_BKPT_DELETE;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint,  1);
}

 void csoundSetInstrumentBreakpoint(CSOUND *csound, MYFLT instr, int32_t skip)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    if (!data) {
      csound->Warning(csound,
                      Str("csoundRemoveBreakpoint: cannot remove breakpoint. "
                          "Debugger is not initialised."));
      return;
    }
    assert(data);
    bkpt_node_t *newpoint =
      (bkpt_node_t *) csound->Malloc(csound, sizeof(bkpt_node_t));
    newpoint->line = -1;
    newpoint->instr = instr;
    newpoint->skip = skip;
    newpoint->count = skip;
    newpoint->mode = CSDEBUG_BKPT_INSTR;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint,  1);
}

 void csoundRemoveInstrumentBreakpoint(CSOUND *csound, MYFLT instr)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    bkpt_node_t *newpoint =
      (bkpt_node_t *) csound->Malloc(csound, sizeof(bkpt_node_t));
    newpoint->line = -1;
    newpoint->instr = instr;
    newpoint->mode = CSDEBUG_BKPT_DELETE;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint,  1);
}

 void csoundClearBreakpoints(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    bkpt_node_t *newpoint =
      (bkpt_node_t *) csound->Malloc(csound, sizeof(bkpt_node_t));
    newpoint->line = -1;
    newpoint->instr = -1;
    newpoint->mode = CSDEBUG_BKPT_CLEAR_ALL;
    csoundWriteCircularBuffer(csound, data->bkpt_buffer, &newpoint,  1);
}

 void csoundSetBreakpointCallback(CSOUND *csound,
                                       breakpoint_cb_t bkpt_cb, void *userdata)
{

    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    data->bkpt_cb = bkpt_cb;
    data->cb_data = userdata;
}

 void csoundDebugStepOver(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    debug_command_t command = CSDEBUG_CMD_STEPOVER;
    csoundWriteCircularBuffer(csound, data->cmd_buffer, &command, 1);
}

 void csoundDebugStepInto(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    debug_command_t command = CSDEBUG_CMD_STEPINTO;
    csoundWriteCircularBuffer(csound, data->cmd_buffer, &command, 1);
}

 void csoundDebugNext(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    debug_command_t command = CSDEBUG_CMD_NEXT;
    csoundWriteCircularBuffer(csound, data->cmd_buffer, &command, 1);
}

 void csoundDebugContinue(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    debug_command_t command = CSDEBUG_CMD_CONTINUE;
    csoundWriteCircularBuffer(csound, data->cmd_buffer, &command, 1);
}

 void csoundDebugStop(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    debug_command_t command = CSDEBUG_CMD_STOP;
    csoundWriteCircularBuffer(csound, data->cmd_buffer, &command, 1);
}

 debug_instr_t *csoundDebugGetInstrInstances(CSOUND *csound)
{
    debug_instr_t *instrhead = NULL;
    debug_instr_t *debug_instr = NULL;
    INSDS *insds = csound->actanchor.nxtact;

    while (insds) {
        if (!instrhead) {
            instrhead = csound->Malloc(csound, sizeof(debug_instr_t));
            debug_instr = instrhead;
        } else {
            debug_instr->next = csound->Malloc(csound, sizeof(debug_instr_t));
            debug_instr = debug_instr->next;
        }
        debug_instr->lclbas = insds->lclbas;
        debug_instr->varPoolHead = insds->instr->varPool->head;
        debug_instr->instrptr = (void *) insds;
        debug_instr->p1 = insds->p1.value;
        debug_instr->p2 = insds->p2.value;
        debug_instr->p3 = insds->p3.value;
        debug_instr->kcounter = insds->kcounter;
        debug_instr->next = NULL;
        insds = insds->nxtact;
    }
    return instrhead;
}

debug_instr_t *csoundDebugGetCurrentInstrInstance(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    if (!data->debug_instr_ptr) {
        return NULL;
    }
    debug_instr_t *debug_instr = csound->Malloc(csound, sizeof(debug_instr_t));
    INSDS *insds = (INSDS *)data->debug_instr_ptr;
    debug_instr->lclbas = insds->lclbas;
    debug_instr->varPoolHead = insds->instr->varPool->head;
    debug_instr->instrptr = data->debug_instr_ptr;
    debug_instr->p1 = insds->p1.value;
    debug_instr->p2 = insds->p2.value;
    debug_instr->p3 = insds->p3.value;
    debug_instr->kcounter = insds->kcounter;
    debug_instr->next = NULL;
    OPDS* opstart = (OPDS*) data->debug_instr_ptr;
    if (opstart->nxtp) {
      debug_instr->line = opstart->nxtp->optext->t.linenum;
    } else {
      debug_instr->line = 0;
    }
    return debug_instr;
}


debug_opcode_t *csoundDebugGetCurrentOpcodeList(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    if (!data->debug_instr_ptr) {
        return NULL;
    }
    OPDS *op = (OPDS *)data->debug_opcode_ptr;

    if (!op) {
        return NULL;
    }
    debug_opcode_t *opcode_list = csound->Malloc(csound, sizeof(debug_opcode_t));
    strNcpy(opcode_list->opname, op->optext->t.opcod, 16);
    //opcode_list->opname[15] = '\0';
    opcode_list->line = op->optext->t.linenum;
    return opcode_list;
}

void csoundDebugFreeOpcodeList(CSOUND *csound, debug_opcode_t *opcode_list)
{
    csound->Free(csound, opcode_list);
}

 void csoundDebugFreeInstrInstances(CSOUND *csound, debug_instr_t *instr)
{
    while (instr) {
        debug_instr_t *oldinstr = instr;
        instr = instr->next;
        csound->Free(csound, oldinstr);
    }
}

/* Build a debug_variable_t list from a variable pool.
 *
 * For instrument-local pools (isGlobal == 0) each variable's storage is at
 * lclbas + memBlockIndex. For the global pool (isGlobal == 1) each variable
 * owns its own storage block, reached through var->memBlock->value (lclbas is
 * unused / NULL). Both paths use the same type dispatch:
 *   i/k/a/r -> MYFLT(s) read in place
 *   S       -> STRINGDAT, data points to the C string
 *   f       -> PVSDAT*,   decode with csoundDebugSerializeFsig()
 *   [       -> ARRAYDAT*, decode with csoundDebugSerializeArray()
 */
static debug_variable_t *csoundDebugBuildVarList(
    CSOUND *csound, CS_VARIABLE *varPoolHead, MYFLT *lclbas, int32_t isGlobal)
{
    debug_variable_t *head = NULL;
    debug_variable_t *debug_var = NULL;
    CS_VARIABLE *var = varPoolHead;
    while (var) {
        void *varmem = NULL;
        MYFLT *base = NULL;
        if (isGlobal) {
            if (var->memBlock != NULL) {
                base = (MYFLT *) &var->memBlock->value;
            }
        } else {
            base = lclbas + var->memBlockIndex;
        }
        if (!head) {
            head = csound->Malloc(csound, sizeof(debug_variable_t));
            debug_var = head;
        } else {
            debug_var->next = csound->Malloc(csound, sizeof(debug_variable_t));
            debug_var = debug_var->next;
        }
        debug_var->next = NULL;
        debug_var->name = var->varName;
        debug_var->typeName = var->varType->varTypeName;
        if (base == NULL) {
            varmem = NULL;
        } else if (strcmp(debug_var->typeName, "i") == 0
                || strcmp(debug_var->typeName, "k") == 0
                || strcmp(debug_var->typeName, "a") == 0
                || strcmp(debug_var->typeName, "r") == 0
                ) {
            varmem = base;
        } else if (strcmp(debug_var->typeName, "S") == 0) {
            STRINGDAT *strdata = (STRINGDAT *) base;
            varmem = &strdata->data[0];
        } else if (strcmp(debug_var->typeName, "f") == 0) {
            /* base is the PVSDAT struct itself */
            varmem = (void *) base;
        } else if (strcmp(debug_var->typeName, "[") == 0) {
            /* base is the ARRAYDAT struct itself */
            varmem = (void *) base;
        } else {
            varmem = NULL;
        }
        debug_var->data = varmem;
        var = var->next;
    }
    return head;
}

 debug_variable_t *csoundDebugGetVariables(CSOUND *csound,
                                                 debug_instr_t *instr)
{
    return csoundDebugBuildVarList(csound, instr->varPoolHead,
                                   instr->lclbas, 0);
}

debug_variable_t *csoundDebugGetGlobalVariables(CSOUND *csound)
{
    CS_VAR_POOL *pool;
    if (csound == NULL) {
        return NULL;
    }
    pool = csound->engineState.varPool;
    if (pool == NULL) {
        return NULL;
    }
    return csoundDebugBuildVarList(csound, pool->head, NULL, 1);
}

static UOPCODE *csoundDebugUdoFindSavedSibling(UOPCODE *nestedHead,
                                               INSDS *sibling_parent,
                                               int32_t *truncatedOut)
{
    INSDS *ip;
    UOPCODE *candidate;
    int32_t depth;
    enum { maxDepth = 4096 };

    if (nestedHead == NULL || sibling_parent == NULL) {
        return NULL;
    }
    ip = nestedHead->ip;
    for (depth = 0; depth < maxDepth && ip != NULL; depth++) {
        candidate = (UOPCODE *)ip->opcod_deact;
        if (candidate == NULL) {
            return NULL;
        }
        if (candidate->parent_ip == sibling_parent) {
            return candidate;
        }
        if (candidate->parent_ip == ip) {
            ip = candidate->ip;
            continue;
        }
        /* Advance along nested saved-sibling chains instead of aborting. */
        ip = candidate->ip;
    }
    if (truncatedOut != NULL) {
        *truncatedOut = 1;
    }
    return NULL;
}

static UOPCODE *csoundDebugUdoChainNext(UOPCODE *p, INSDS *parent_ip,
                                        int32_t *truncatedOut)
{
    UOPCODE *next;
    if (p == NULL || p->ip == NULL || parent_ip == NULL) {
        return NULL;
    }
    /* UDO linking stores the older sibling in p->ip->opcod_deact at call
       time, but that field becomes the head of nested UDO calls once the
       sub-instance runs. */
    next = (UOPCODE *)p->ip->opcod_deact;
    if (next == NULL) {
        return NULL;
    }
    if (next->parent_ip == parent_ip) {
        return next;
    }
    if (next->parent_ip == p->ip) {
        return csoundDebugUdoFindSavedSibling(next, parent_ip, truncatedOut);
    }
    return NULL;
}

static int32_t csoundDebugVisitedEnsureCapacity(CSOUND *csound,
                                                UOPCODE ***visited,
                                                int32_t *visitedCapacity,
                                                int32_t visitedCount,
                                                int32_t *truncatedOut)
{
    UOPCODE **newVisited;
    int32_t newCapacity;
    size_t bytes;

    if (visitedCount < *visitedCapacity) {
        return 1;
    }
    newCapacity = (*visitedCapacity > 0) ? (*visitedCapacity * 2) : 64;
    bytes = (size_t) newCapacity * sizeof(UOPCODE *);
    newVisited = (UOPCODE **) csound->Malloc(csound, bytes);
    if (newVisited == NULL) {
        if (truncatedOut != NULL) {
            *truncatedOut = 1;
        }
        return 0;
    }
    if (*visited != NULL && visitedCount > 0) {
        memcpy(newVisited, *visited, (size_t) visitedCount * sizeof(UOPCODE *));
        csound->Free(csound, *visited);
    }
    *visited = newVisited;
    *visitedCapacity = newCapacity;
    return 1;
}

static int csoundDebugUopcodeVisited(UOPCODE *p, UOPCODE **visited,
                                     int32_t visitedCount)
{
    int32_t i;
    for (i = 0; i < visitedCount; i++) {
        if (visited[i] == p) {
            return 1;
        }
    }
    return 0;
}

static const char *csoundDebugUdoName(UOPCODE *p)
{
    OPCOD_IOBUFS *buf = p->buf;
    if (buf != NULL && buf->opcode_info != NULL && buf->opcode_info->name != NULL) {
        return buf->opcode_info->name;
    }
    if (p->h.optext != NULL && p->h.optext->t.opcod != NULL) {
        return p->h.optext->t.opcod;
    }
    return "unknown";
}

static void csoundDebugAppendUdoFrame(
    CSOUND *csound,
    debug_udo_frame_t **head,
    debug_udo_frame_t **tail,
    UOPCODE *p,
    INSDS *udo_ip,
    int32_t depth,
    int32_t frameIndex)
{
    debug_udo_frame_t *frame =
        csound->Malloc(csound, sizeof(debug_udo_frame_t));
    frame->udoName = csoundDebugUdoName(p);
    frame->callLine = (p->h.optext != NULL) ? p->h.optext->t.linenum : 0;
    frame->depth = depth;
    frame->frameIndex = frameIndex;
    if (udo_ip->instr != NULL && udo_ip->instr->varPool != NULL) {
        frame->varList = csoundDebugBuildVarList(
            csound, udo_ip->instr->varPool->head, udo_ip->lclbas, 0);
    } else {
        frame->varList = NULL;
    }
    frame->next = NULL;
    if (*tail == NULL) {
        *head = *tail = frame;
    } else {
        (*tail)->next = frame;
        *tail = frame;
    }
}

static void csoundDebugCollectUdoFrames(
    CSOUND *csound,
    INSDS *ip,
    int32_t depth,
    debug_udo_frame_t **head,
    debug_udo_frame_t **tail,
    UOPCODE ***visited,
    int32_t *visitedCount,
    int32_t *visitedCapacity,
    int32_t *truncatedOut)
{
    UOPCODE *p;
    int32_t siblingIndex = 0;

    for (p = (UOPCODE *)ip->opcod_deact; p != NULL;
         p = csoundDebugUdoChainNext(p, ip, truncatedOut)) {
        INSDS *udo_ip = p->ip;
        if (udo_ip == NULL) {
            continue;
        }
        /* opcod_deact on a child instance may still hold a saved sibling
           pointer for its real parent; do not treat that as a nested call. */
        if (p->parent_ip != ip) {
            continue;
        }
        if (!ATOMIC_GET(udo_ip->init_done)) {
            continue;
        }
        if (csoundDebugUopcodeVisited(p, *visited, *visitedCount)) {
            continue;
        }
        if (!csoundDebugVisitedEnsureCapacity(csound, visited, visitedCapacity,
                                              *visitedCount, truncatedOut)) {
            return;
        }
        (*visited)[(*visitedCount)++] = p;
        csoundDebugAppendUdoFrame(csound, head, tail, p, udo_ip, depth,
                                  siblingIndex++);
        csoundDebugCollectUdoFrames(csound, udo_ip, depth + 1, head, tail,
                                    visited, visitedCount, visitedCapacity,
                                    truncatedOut);
    }
}

debug_udo_frame_t *csoundDebugGetUdoFrames(CSOUND *csound,
                                           debug_instr_t *instr,
                                           int32_t *truncatedOut)
{
    debug_udo_frame_t *head = NULL;
    debug_udo_frame_t *tail = NULL;
    int32_t visitedCount = 0;
    int32_t visitedCapacity = 0;
    UOPCODE **visited = NULL;
    INSDS *ip;

    if (truncatedOut != NULL) {
        *truncatedOut = 0;
    }
    if (instr == NULL || instr->instrptr == NULL) {
        return NULL;
    }
    ip = (INSDS *)instr->instrptr;
    csoundDebugCollectUdoFrames(csound, ip, 0, &head, &tail, &visited,
                                &visitedCount, &visitedCapacity, truncatedOut);
    if (visited != NULL) {
        csound->Free(csound, visited);
    }
    return head;
}

void csoundDebugFreeUdoFrames(CSOUND *csound, debug_udo_frame_t *frameHead)
{
    while (frameHead) {
        debug_udo_frame_t *old = frameHead;
        frameHead = frameHead->next;
        if (old->varList) {
            csoundDebugFreeVariables(csound, old->varList);
        }
        csound->Free(csound, old);
    }
}


 void csoundDebugFreeVariables(CSOUND *csound, debug_variable_t *varHead)
{
    while (varHead) {
        debug_variable_t *oldvar = varHead;
        varHead = varHead->next;
        csound->Free(csound, oldvar);
    }
}

int32_t csoundDebugSerializeFsig(CSOUND *csound, void *varData,
                                 float *outBuf, int32_t bufMax,
                                 debug_fsig_info_t *infoOut,
                                 int32_t localKsmps)
{
    PVSDAT *fsig = (PVSDAT *) varData;
    int32_t NB, total, n, i, activeKsmps;

    if (infoOut != NULL) {
        memset(infoOut, 0, sizeof(debug_fsig_info_t));
    }
    if (fsig == NULL) {
        return 0;
    }
    NB = (int32_t) fsig->NB;
    /* pvsanal only sets NB in the sliding-analysis path; the normal (non-sliding)
       generate_frame leaves it at 0. The frame is always N+2 floats = 2*(N/2+1),
       so derive the bin count from N when NB is unset. */
    if (NB <= 0 && fsig->N > 0) {
        NB = (int32_t) (fsig->N / 2) + 1;
    }
    if (infoOut != NULL) {
        infoOut->N         = (int32_t) fsig->N;
        infoOut->NB        = NB;
        infoOut->overlap   = (int32_t) fsig->overlap;
        infoOut->winsize   = (int32_t) fsig->winsize;
        infoOut->wintype   = (int32_t) fsig->wintype;
        infoOut->format    = (int32_t) fsig->format;
        infoOut->framecount = (uint32_t) fsig->framecount;
        infoOut->sliding   = (int32_t) fsig->sliding;
    }
    /* Frame not allocated yet (e.g. before first pvsanal run). */
    if (fsig->frame.auxp == NULL || NB <= 0) {
        if (infoOut != NULL) {
            infoOut->NB = 0;
        }
        return 0;
    }

    total = 2 * NB;   /* interleaved amp/freq */
    if (outBuf == NULL || bufMax <= 0) {
        return total;   /* report size only */
    }
    n = (total < bufMax) ? total : bufMax;

    if (fsig->sliding) {
        /* Sliding analysis: frame.auxp is CMPLX[capacity * NB]. Use the most
           recent active sub-frame. Capacity may exceed the producer's current
           local ksmps when a UDO instance is reused after setksmps. */
        size_t stride = (size_t) NB * sizeof(CMPLX);
        uint32_t capacitySubframes, activeSubframes;
        CMPLX *base;
        if (stride == 0 || fsig->frame.size < stride) {
            if (infoOut != NULL) {
                infoOut->NB = 0;
            }
            return 0;
        }
        activeKsmps = (localKsmps > 0) ? localKsmps : csound->ksmps;
        capacitySubframes = (uint32_t)(fsig->frame.size / stride);
        if (capacitySubframes < 1) {
            capacitySubframes = 1;
        }
        activeSubframes = (uint32_t) activeKsmps;
        if (activeSubframes > capacitySubframes) {
            activeSubframes = capacitySubframes;
        }
        if (activeSubframes < 1) {
            activeSubframes = 1;
        }
        base = ((CMPLX *) fsig->frame.auxp)
            + (size_t) NB * (activeSubframes - 1);
        for (i = 0; i < n; i++) {
            int32_t bin = i >> 1;
            outBuf[i] = (i & 1) ? (float) base[bin].im : (float) base[bin].re;
        }
    } else {
        /* Normal analysis: frame.auxp is float[2*NB] interleaved amp/freq. */
        float *frame = (float *) fsig->frame.auxp;
        for (i = 0; i < n; i++) {
            outBuf[i] = frame[i];
        }
    }
    return total;
}

int32_t csoundDebugSerializeArray(CSOUND *csound, void *varData,
                                  MYFLT *outBuf, int32_t bufMax,
                                  debug_array_info_t *infoOut)
{
    ARRAYDAT *adat = (ARRAYDAT *) varData;
    const char *elemType = NULL;
    int32_t total, n, i, d, elements, perMember;
    (void) csound;

    if (infoOut != NULL) {
        memset(infoOut, 0, sizeof(debug_array_info_t));
    }
    if (adat == NULL || adat->arrayType == NULL) {
        return 0;
    }
    elemType = adat->arrayType->varTypeName;
    if (infoOut != NULL) {
        infoOut->dimensions = (int32_t) adat->dimensions;
        infoOut->arrayMemberSize = (int32_t) adat->arrayMemberSize;
        if (elemType != NULL) {
            strncpy(infoOut->elementTypeName, elemType,
                    sizeof(infoOut->elementTypeName) - 1);
        }
    }
    /* Only numeric arrays (i/k/a) serialize to flat MYFLT. S[]/f[] return 0. */
    if (elemType == NULL ||
        (strcmp(elemType, "i") != 0 && strcmp(elemType, "k") != 0 &&
         strcmp(elemType, "a") != 0)) {
        return 0;
    }
    if (adat->data == NULL || adat->sizes == NULL || adat->dimensions <= 0) {
        return 0;
    }

    elements = 1;
    for (d = 0; d < adat->dimensions; d++) {
        elements *= adat->sizes[d];
    }
    perMember = (int32_t) (adat->arrayMemberSize / (int32_t) sizeof(MYFLT));
    if (perMember < 1) {
        perMember = 1;
    }
    total = elements * perMember;
    if (infoOut != NULL) {
        infoOut->totalElements = total;
    }
    if (outBuf == NULL || bufMax <= 0) {
        return total;   /* report size only */
    }
    n = (total < bufMax) ? total : bufMax;
    for (i = 0; i < n; i++) {
        outBuf[i] = adat->data[i];
    }
    return total;
}

inline static void mix_out(MYFLT *out, MYFLT *in, uint32_t smps) {
  uint32_t i;
  for (i = 0; i < smps; i++)
    out[i] += in[i];
}

static inline void opcode_perf_debug(CSOUND *csound, csdebug_data_t *data,
                                     INSDS *ip) {
  OPDS *opstart = (OPDS *)ip;
  while ((opstart = opstart->nxtp) != NULL) {
    /* check if we have arrived at a line breakpoint */
    bkpt_node_t *bp_node = data->bkpt_anchor->next;
    if (data->debug_opcode_ptr) {
      opstart = data->debug_opcode_ptr;
      data->debug_opcode_ptr = NULL;
    }
    int32_t linenum = opstart->optext->t.linenum;
    while (bp_node) {
      if (bp_node->instr == ip->p1.value || (bp_node->instr == 0)) {
        if ((bp_node->line) == linenum) { /* line matches */
          if (bp_node->count < 2) { /* skip of 0 or 1 has the same effect */
            if (data->debug_opcode_ptr != opstart) { /* did we just stop here */
              data->debug_instr_ptr = ip;
              data->debug_opcode_ptr = opstart;
              data->status = CSDEBUG_STATUS_STOPPED;
              data->cur_bkpt = bp_node;
              csoundDebuggerBreakpointReached(csound);
              bp_node->count = bp_node->skip;
              return;
            } else {
              data->debug_opcode_ptr = NULL; /* if just stopped here-continue */
            }
          } else {
            bp_node->count--;
          }
        }
      }
      bp_node = bp_node->next;
    }
    opstart->insdshead->pds = opstart;
    csound->mode = 2;
    (*opstart->perf)(csound, opstart); /* run each opcode */
    opstart = opstart->insdshead->pds;
    csound->mode = 0;
  }
  mix_out(csound->spout, ip->spout, ip->ksmps * csound->nchnls);
}

static inline void process_debug_buffers(CSOUND *csound, csdebug_data_t *data) {
  bkpt_node_t *bkpt_node;
  while (csoundReadCircularBuffer(csound, data->bkpt_buffer, &bkpt_node, 1) ==
         1) {
    if (bkpt_node->mode == CSDEBUG_BKPT_CLEAR_ALL) {
      bkpt_node_t *n;
      while (data->bkpt_anchor->next) {
        n = data->bkpt_anchor->next;
        data->bkpt_anchor->next = n->next;
        csound->Free(csound, n); /* TODO this should be moved from kperf to a
                                    non-realtime context */
      }
      csound->Free(csound, bkpt_node);
    } else if (bkpt_node->mode == CSDEBUG_BKPT_DELETE) {
      bkpt_node_t *n = data->bkpt_anchor->next;
      bkpt_node_t *prev = data->bkpt_anchor;
      while (n) {
        if (n->line == bkpt_node->line && n->instr == bkpt_node->instr) {
          prev->next = n->next;
          if (data->cur_bkpt == n)
            data->cur_bkpt = n->next;
          csound->Free(csound, n); /* TODO this should be moved from kperf to a
                                      non-realtime context */
          n = prev->next;
          continue;
        }
        prev = n;
        n = n->next;
      }
      //        csound->Free(csound, bkpt_node); /* TODO move to non rt context
      //        */
    } else {
      // FIXME sort list to optimize
      bkpt_node->next = data->bkpt_anchor->next;
      data->bkpt_anchor->next = bkpt_node;
    }
  }
}

int32_t kperf_debug(CSOUND *csound) {
  INSDS *ip;
  csdebug_data_t *data = (csdebug_data_t *)csound->csdebug_data;
  int32_t lksmps = csound->ksmps;
  /* call message_dequeue to run API calls */
  message_dequeue(csound);

  if (!data || data->status != CSDEBUG_STATUS_STOPPED) {
    /* update orchestra time */
    csound->kcounter = ++(csound->global_kcounter);
    csound->icurTimeSamples += csound->ksmps;
    csound->curBeat += csound->curBeat_inc;
  }

  /* if skipping time on request by 'a' score statement: */
  if (UNLIKELY(csound->advanceCnt)) {
    csound->advanceCnt--;
    return 1;
  }
  /* if i-time only, return now */
  if (UNLIKELY(csound->initonly))
    return 1;

  if (data) { /* process debug commands*/
    process_debug_buffers(csound, data);
  }

  if (!data || data->status == CSDEBUG_STATUS_RUNNING) {
    /* for one kcnt: */
    if (csound->oparms_.sfread) /*   if audio_infile open  */
      csound->spinrecv(csound); /*      fill the spin buf  */
    /* clear spout */
    memset(csound->spout, 0, csound->nspout * sizeof(MYFLT));
    memset(csound->spout_tmp, 0, csound->nspout * sizeof(MYFLT));
  }

  ip = csound->actanchor.nxtact;
  /* Process debugger commands */
  debug_command_t command = CSDEBUG_CMD_NONE;
  if (data) {
    csoundReadCircularBuffer(csound, data->cmd_buffer, &command, 1);
    if (command == CSDEBUG_CMD_STOP && data->status != CSDEBUG_STATUS_STOPPED) {
      data->debug_instr_ptr = ip;
      data->status = CSDEBUG_STATUS_STOPPED;
      csoundDebuggerBreakpointReached(csound);
    }
    if (command == CSDEBUG_CMD_CONTINUE &&
        data->status == CSDEBUG_STATUS_STOPPED) {
      if (data->cur_bkpt && data->cur_bkpt->skip <= 2)
        data->cur_bkpt->count = 2;
      data->status = CSDEBUG_STATUS_RUNNING;
      if (data->debug_instr_ptr) {
        /* if not NULL, resume from last active */
        ip = data->debug_instr_ptr;
        data->debug_instr_ptr = NULL;
      }
    }
    if (command == CSDEBUG_CMD_NEXT && data->status == CSDEBUG_STATUS_STOPPED) {
      data->status = CSDEBUG_STATUS_NEXT;
    }
  }
  if (ip != NULL && data != NULL && (data->status != CSDEBUG_STATUS_STOPPED)) {
    if (csound->multiThreadedThreadInfo != NULL) {
#ifdef ONE_FINE_DAY
      /* DEBUGGER disabled for multicore performance */
      int32_t k;
      int32_t n = csound->oparms->numThreads;
      if (csound->dag_changed)
        dag_build(csound, ip);
      else
        dag_reinit(csound);

#ifdef PARCS_USE_LOCK_BARRIER
      csound->WaitBarrier(csound->barrier1)
#else
      ATOMIC_SET(csound->parflag,!csound->parflag);
#endif
      csound_node_perf(csound, 0, n);
#ifdef PARCS_USE_LOCK_BARRIER
      csound->WaitBarrier(csound->barrier2);
#else
      {
        int32_t i, sum;
        do {
          for(i = 1, sum = 1; i < n; i++)
            sum += csound->taskflag[i];
        } while(sum < n);
      }
#endif
      for (k = 1; k < csound->oparms->numThreads; k++)
          mix_out(csound->spout_tmp, csound->spout_tmp +
                  k * csound->nspout, csound->nspout);
#else /* currently disabled */
      csoundDie(csound, "csound debugger cannot run in multiple threads\n");
#endif
      csound->multiThreadedDag = NULL;
    } else {
      int32_t done;
      double time_end = (csound->ksmps + csound->icurTimeSamples) / csound->esr;

      while (ip != NULL) { /* for each instr active:  */
        if (UNLIKELY(csound->oparms->sampleAccurate && ip->offtim > 0 &&
                     time_end > ip->offtim)) {
          /* this is the last cycle of performance */
          //   csound->Message(csound, "last cycle %d: %f %f %d\n",
          //       ip->insno, csound->icurTimeSamples/csound->esr,
          //          ip->offtim, ip->no_end);
          ip->ksmps_no_end = ip->no_end;
        }
        done = ATOMIC_GET(ip->init_done);
        if (done == 1) { /* if init-pass has been done */
          /* check if next command pending and we are on the
             first instrument in the chain */
          /* coverity says data already dereferenced by here */
          if (/*data &&*/ data->status == CSDEBUG_STATUS_NEXT) {
            if (data->debug_instr_ptr == NULL) {
              data->debug_instr_ptr = ip;
              data->debug_opcode_ptr = NULL;
              data->status = CSDEBUG_STATUS_STOPPED;
              csoundDebuggerBreakpointReached(csound);
              return 0;
            } else {
              ip = data->debug_instr_ptr;
              data->debug_instr_ptr = NULL;
            }
          }
          /* check if we have arrived at an instrument breakpoint */
          bkpt_node_t *bp_node = data->bkpt_anchor->next;
          while (bp_node && data->status != CSDEBUG_STATUS_NEXT) {
            if (bp_node->instr == ip->p1.value && (bp_node->line == -1)) {
              if (bp_node->count < 2) {
                /* skip of 0 or 1 has the same effect */
                data->debug_instr_ptr = ip;
                data->debug_opcode_ptr = NULL;
                data->cur_bkpt = bp_node;
                data->status = CSDEBUG_STATUS_STOPPED;
                csoundDebuggerBreakpointReached(csound);
                bp_node->count = bp_node->skip;
                return 0;
              } else {
                bp_node->count--;
              }
            }
            bp_node = bp_node->next;
          }
          ip->spin = csound->spin;
          ip->spout = csound->spout_tmp;
          ip->kcounter = csound->kcounter;
          if (ip->ksmps == csound->ksmps) {
            opcode_perf_debug(csound, data, ip);
          } else { /* when instrument has local ksmps */
            int32_t i, n = csound->nspout, start = 0;
            lksmps = ip->ksmps;
            int32_t incr = csound->nchnls * lksmps;
            int32_t offset = ip->ksmps_offset;
            int32_t early = ip->ksmps_no_end;
            ip->spin = csound->spin;
            ip->kcounter = csound->kcounter * csound->ksmps / lksmps;

            /* we have to deal with sample-accurate code
               whole CS_KSMPS blocks are offset here, the
               remainder is left to each opcode to deal with.
            */
            while (offset >= lksmps) {
              offset -= lksmps;
              start += csound->nchnls;
            }
            ip->ksmps_offset = offset;
            if (UNLIKELY(early)) {
              n -= (early * csound->nchnls);
              ip->ksmps_no_end = early % lksmps;
            }

            for (i = start; i < n;
                 i += incr, ip->spin += incr, ip->spout += incr) {
              opcode_perf_debug(csound, data, ip);
              ip->kcounter++;
            }
          }
        }
        ip->ksmps_offset = 0; /* reset sample-accuracy offset */
        ip->ksmps_no_end = 0; /* reset end of loop samples */
        ip = ip->nxtact;      /* but this does not allow for all deletions */
        if (/*data &&*/ data->status == CSDEBUG_STATUS_NEXT) {
          data->debug_instr_ptr = ip; /* we have reached the next
                                         instrument. Break */
          data->debug_opcode_ptr = NULL;
          if (ip != NULL) { /* must defer break until next kperf */
            data->status = CSDEBUG_STATUS_STOPPED;
            csoundDebuggerBreakpointReached(csound);
            return 0;
          }
        }
      }
    }
  }

  /* fire per-k-cycle debug callback if set (only when not stopped at a breakpoint) */
  if (csound->debug_cb && (!data || data->status != CSDEBUG_STATUS_STOPPED)) {
    csound->debug_cb(csound, csound->debug_cb_data);
  }
  if (!data || data->status != CSDEBUG_STATUS_STOPPED)
    csound->spoutran(csound); /*      send to audio_out  */

  return 0;
}

/* ---- per-k-cycle debug callback API ---- */

void csoundSetDebugCallback(CSOUND *csound,
                            void (*cb)(CSOUND *, void *),
                            void *userdata)
{
  csound->debug_cb      = cb;
  csound->debug_cb_data = userdata;
}

void csoundRemoveDebugCallback(CSOUND *csound)
{
  csound->debug_cb      = NULL;
  csound->debug_cb_data = NULL;
}
