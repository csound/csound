/* Native Wasm opcode loader test fixture. */

#include <csdl.h>

#include "wasm_opcode_abi.h"

typedef struct {
  OPDS h;
  MYFLT *out;
  MYFLT *in;
  MYFLT *cutoff;
  MYFLT state;
} VELVETLP_FIXTURE;

typedef struct {
  OPDS h;
  MYFLT *out;
} RETRY_FIXTURE;

static uint32_t retry_call_count = 0;

static int32_t velvetlp_init(CSOUND *csound, VELVETLP_FIXTURE *opcode)
{
  (void) csound;
  opcode->state = 0.0;
  return OK;
}

static int32_t velvetlp_perf(CSOUND *csound, VELVETLP_FIXTURE *opcode)
{
  uint32_t offset = opcode->h.insdshead->ksmps_offset;
  uint32_t early = opcode->h.insdshead->ksmps_no_end;
  uint32_t end = opcode->h.insdshead->ksmps - early;
  MYFLT coefficient = *opcode->cutoff / opcode->h.insdshead->esr;
  uint32_t index;

  (void) csound;
  if (coefficient < 0.0)
    coefficient = 0.0;
  else if (coefficient > 1.0)
    coefficient = 1.0;
  for (index = offset; index < end; index++) {
    opcode->state += coefficient * (opcode->in[index] - opcode->state);
    opcode->out[index] = opcode->state;
  }
  return OK;
}

static int32_t retry_once_perf(CSOUND *csound, RETRY_FIXTURE *opcode)
{
  uint32_t offset = opcode->h.insdshead->ksmps_offset;
  uint32_t early = opcode->h.insdshead->ksmps_no_end;
  uint32_t end = opcode->h.insdshead->ksmps - early;
  uint32_t index;

  (void) csound;
  retry_call_count++;
  if (retry_call_count == 1)
    return NOTOK;
  for (index = offset; index < end; index++)
    opcode->out[index] = 0.0;
  return OK;
}

static int32_t retry_count_init(CSOUND *csound, RETRY_FIXTURE *opcode)
{
  (void) csound;
  *opcode->out = retry_call_count;
  return OK;
}

PUBLIC int64_t csound_opcode_init(CSOUND *csound, OENTRY **entries_out)
{
  OENTRY *entries = (OENTRY *) csound->Calloc(csound, 3 * sizeof(*entries));

  *entries_out = entries;
  if (entries == NULL)
    return 0;

  entries[0].opname = "velvetlp";
  entries[0].dsblksiz = sizeof(VELVETLP_FIXTURE);
  entries[0].outypes = "a";
  entries[0].intypes = "ak";
  entries[0].init = (SUBR) velvetlp_init;
  entries[0].perf = (SUBR) velvetlp_perf;

  entries[1].opname = "wasmretryonce";
  entries[1].dsblksiz = sizeof(RETRY_FIXTURE);
  entries[1].outypes = "a";
  entries[1].intypes = "";
  entries[1].perf = (SUBR) retry_once_perf;

  entries[2].opname = "wasmretrycount";
  entries[2].dsblksiz = sizeof(RETRY_FIXTURE);
  entries[2].outypes = "i";
  entries[2].intypes = "";
  entries[2].init = (SUBR) retry_count_init;

  return 3 * (int64_t) sizeof(*entries);
}

PUBLIC int32_t csoundModuleInfo(void)
{
  return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT));
}
