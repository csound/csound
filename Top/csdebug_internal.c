#include "csdebug_internal.h"

#include <assert.h>              // for assert
#include <stddef.h>              // for NULL

#include "csdebug.h"             // for csdebug_data_t, debug_instr_t, debug...
#include "csdebug_internal.h"
#include "csoundCore_internal.h"          // for INSDS, OPDS, CSOUND_, OPTXT, TEXT
#include "csound_type_system.h"  // for CS_VAR_MEM, CS_VAR_POOL
#include "sysdep.h"              // for strNcpy

debug_instr_t *csoundDebugGetCurrentInstrInstance(CSOUND *csound) {
  csdebug_data_t *data = (csdebug_data_t *)csound->csdebug_data;
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
  OPDS *opstart = (OPDS *)data->debug_instr_ptr;
  if (opstart->nxtp) {
    debug_instr->line = opstart->nxtp->optext->t.linenum;
  } else {
    debug_instr->line = 0;
  }
  return debug_instr;
}

debug_opcode_t *csoundDebugGetCurrentOpcodeList(CSOUND *csound) {
  csdebug_data_t *data = (csdebug_data_t *)csound->csdebug_data;
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
  // opcode_list->opname[15] = '\0';
  opcode_list->line = op->optext->t.linenum;
  return opcode_list;
}

void csoundDebugFreeOpcodeList(CSOUND *csound, debug_opcode_t *opcode_list) {
  csound->Free(csound, opcode_list);
}

void csoundDebuggerBreakpointReached(CSOUND *csound) {
  csdebug_data_t *data = (csdebug_data_t *)csound->csdebug_data;
  debug_bkpt_info_t bkpt_info;
  bkpt_info.breakpointInstr = csoundDebugGetCurrentInstrInstance(csound);
  bkpt_info.instrListHead = csoundDebugGetInstrInstances(csound);
  bkpt_info.currentOpcode = csoundDebugGetCurrentOpcodeList(csound);
  bkpt_info.instrVarList =
      csoundDebugGetVariables(csound, bkpt_info.breakpointInstr);
  if (data->bkpt_cb) {
    data->bkpt_cb(csound, &bkpt_info, data->cb_data);
  } else {
    csoundMessage(csound,
                  Str("Breakpoint callback not set. Breakpoint Reached."));
  }
  // TODO: These free operations could be moved to a low priority context
  csoundDebugFreeInstrInstances(csound, bkpt_info.breakpointInstr);
  csoundDebugFreeInstrInstances(csound, bkpt_info.instrListHead);
  if (bkpt_info.currentOpcode) {
    csoundDebugFreeOpcodeList(csound, bkpt_info.currentOpcode);
  }
  csoundDebugFreeVariables(csound, bkpt_info.instrVarList);
}