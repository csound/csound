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

#include "csdebug.h"

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

PUBLIC void csoundDebuggerInit(CSOUND *csound)
{
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

PUBLIC void csoundDebuggerClean(CSOUND *csound)
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

PUBLIC void csoundDebugStart(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    data->status = CSDEBUG_STATUS_RUNNING;
}

PUBLIC void csoundSetBreakpoint(CSOUND *csound, int32_t line, int32_t instr, int32_t skip)
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

PUBLIC void csoundRemoveBreakpoint(CSOUND *csound, int32_t line, int32_t instr)
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

PUBLIC void csoundSetInstrumentBreakpoint(CSOUND *csound, MYFLT instr, int32_t skip)
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

PUBLIC void csoundRemoveInstrumentBreakpoint(CSOUND *csound, MYFLT instr)
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

PUBLIC void csoundClearBreakpoints(CSOUND *csound)
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

PUBLIC void csoundSetBreakpointCallback(CSOUND *csound,
                                       breakpoint_cb_t bkpt_cb, void *userdata)
{

    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    data->bkpt_cb = bkpt_cb;
    data->cb_data = userdata;
}

PUBLIC void csoundDebugStepOver(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    debug_command_t command = CSDEBUG_CMD_STEPOVER;
    csoundWriteCircularBuffer(csound, data->cmd_buffer, &command, 1);
}

PUBLIC void csoundDebugStepInto(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    debug_command_t command = CSDEBUG_CMD_STEPINTO;
    csoundWriteCircularBuffer(csound, data->cmd_buffer, &command, 1);
}

PUBLIC void csoundDebugNext(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    debug_command_t command = CSDEBUG_CMD_NEXT;
    csoundWriteCircularBuffer(csound, data->cmd_buffer, &command, 1);
}

PUBLIC void csoundDebugContinue(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    debug_command_t command = CSDEBUG_CMD_CONTINUE;
    csoundWriteCircularBuffer(csound, data->cmd_buffer, &command, 1);
}

PUBLIC void csoundDebugStop(CSOUND *csound)
{
    csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
    assert(data);
    debug_command_t command = CSDEBUG_CMD_STOP;
    csoundWriteCircularBuffer(csound, data->cmd_buffer, &command, 1);
}

PUBLIC debug_instr_t *csoundDebugGetInstrInstances(CSOUND *csound)
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

PUBLIC void csoundDebugFreeInstrInstances(CSOUND *csound, debug_instr_t *instr)
{
    while (instr) {
        debug_instr_t *oldinstr = instr;
        instr = instr->next;
        csound->Free(csound, oldinstr);
    }
}

PUBLIC debug_variable_t *csoundDebugGetVariables(CSOUND *csound,
                                                 debug_instr_t *instr)
{
    debug_variable_t *head = NULL;
    debug_variable_t *debug_var = head;
    CS_VARIABLE * var = instr->varPoolHead;
    while (var) {
        void * varmem = NULL;
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
        if (strcmp(debug_var->typeName, "i") == 0
                || strcmp(debug_var->typeName, "k") == 0
                || strcmp(debug_var->typeName, "a") == 0
                || strcmp(debug_var->typeName, "r") == 0
                ) {
            varmem = instr->lclbas + var->memBlockIndex;
        } else if (strcmp(debug_var->typeName, "S") == 0) {
            STRINGDAT *strdata = (STRINGDAT *) (instr->lclbas + var->memBlockIndex);
            varmem = &strdata->data[0];
        } else {
            csound->Message(csound, "csoundDebugGetVarData() unknown data type.\n");
        }
        debug_var->data = varmem;
        var = var->next;
    }
    return head;
}


PUBLIC void csoundDebugFreeVariables(CSOUND *csound, debug_variable_t *varHead)
{
    while (varHead) {
        debug_variable_t *oldvar = varHead;
        varHead = varHead->next;
        csound->Free(csound, oldvar);
    }
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
    /* There are 2 partitions of work: 1st by inso,
       2nd by inso count / thread count. */
    if (csound->multiThreadedThreadInfo != NULL) {
#ifdef PARCS
      int32_t k;
      int32_t n = csound->oparms->numThreads;
      if (csound->dag_changed)
        dag_build(csound, ip);
      else
        dag_reinit(csound); /* set to initial state */

      /* process this partition */
#ifdef PARCS_USE_LOCK_BARRIER
      csound->WaitBarrier(csound->barrier1)
#else
      ATOMIC_SET(csound->parflag,!csound->parflag);
#endif
      csound_node_perf(csound, 0, n);
      /* wait until partition is complete */
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
      /* do the mixing of thread buffers */
      for (k = 1; k < csound->oparms->numThreads; k++)
          mix_out(csound->spout_tmp, csound->spout_tmp +
                  k * csound->nspout, csound->nspout);
#endif /* PARCS */
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

  if (!data || data->status != CSDEBUG_STATUS_STOPPED)
    csound->spoutran(csound); /*      send to audio_out  */

  return 0;
}
