/* Native Wasm opcode loader test fixture. */

#include <csdl.h>

#include "wasm_opcode_abi.h"

typedef struct {
  OPDS h;
  cs_float *out;
  cs_float *in;
  cs_float *cutoff;
  cs_float state;
} VELVETLP_FIXTURE;

typedef struct {
  OPDS h;
  cs_float *out;
} RETRY_FIXTURE;

typedef struct {
  OPDS h;
  cs_float *out;
  cs_float init_count;
} STATE_FIXTURE;

typedef struct {
  OPDS h;
  cs_float *trap;
} DEINIT_ERROR_FIXTURE;

typedef struct {
  OPDS h;
  cs_float *out;
  cs_float *in;
} ALIAS_FIXTURE;

static uint32_t retry_call_count = 0;
static uint32_t deinit_count = 0;

enum { OPCODE_COUNT = 10 };

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
  cs_float coefficient = *opcode->cutoff / opcode->h.insdshead->esr;
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

static int32_t state_init(CSOUND *csound, STATE_FIXTURE *opcode)
{
  (void) csound;
  *opcode->out = ++opcode->init_count;
  return OK;
}

static int32_t state_deinit(CSOUND *csound, STATE_FIXTURE *opcode)
{
  (void) csound;
  deinit_count += opcode->init_count != 0;
  return OK;
}

static int32_t init_error(CSOUND *csound, STATE_FIXTURE *opcode)
{
  (void) csound;
  opcode->init_count = 1;
  return NOTOK;
}

static int32_t deinit_count_init(CSOUND *csound, RETRY_FIXTURE *opcode)
{
  (void) csound;
  *opcode->out = deinit_count;
  return OK;
}

static int32_t add_one(CSOUND *csound, RETRY_FIXTURE *opcode)
{
  (void) csound;
  *opcode->out += 1;
  return OK;
}

static int32_t deinit_error(CSOUND *csound, DEINIT_ERROR_FIXTURE *opcode)
{
  deinit_count++;
  if (*opcode->trap != 0)
    csound->Message(csound, "This host call is not supported\n");
  return NOTOK;
}

static int32_t alias_perf(CSOUND *csound, ALIAS_FIXTURE *opcode)
{
  (void) csound;
  *opcode->out = 99;
  *opcode->out = (*opcode->in == 99);
  return OK;
}

PUBLIC int64_t csound_opcode_init(CSOUND *csound, OENTRY **entries_out)
{
  OENTRY *entries;

  /* A supported host callback must have a non-null function pointer. */
  if (csound->Calloc == NULL)
    return 0;
  entries = (OENTRY *) csound->Calloc(csound, OPCODE_COUNT * sizeof(*entries));

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

  entries[3].opname = "wasmstate";
  entries[3].dsblksiz = sizeof(STATE_FIXTURE);
  entries[3].outypes = "i";
  entries[3].intypes = "";
  entries[3].init = (SUBR) state_init;
  entries[3].deinit = (SUBR) state_deinit;

  entries[4].opname = "wasmdeinitcount";
  entries[4].dsblksiz = sizeof(RETRY_FIXTURE);
  entries[4].outypes = "i";
  entries[4].intypes = "";
  entries[4].init = (SUBR) deinit_count_init;

  entries[5].opname = "wasmnoinit";
  entries[5].dsblksiz = sizeof(RETRY_FIXTURE);
  entries[5].outypes = "k";
  entries[5].intypes = "";
  entries[5].perf = (SUBR) add_one;

  entries[6].opname = "wasminitadd";
  entries[6].dsblksiz = sizeof(RETRY_FIXTURE);
  entries[6].outypes = "i";
  entries[6].intypes = "";
  entries[6].init = (SUBR) add_one;

  entries[7].opname = "wasmdeiniterror";
  entries[7].dsblksiz = sizeof(DEINIT_ERROR_FIXTURE);
  entries[7].outypes = "";
  entries[7].intypes = "i";
  entries[7].deinit = (SUBR) deinit_error;

  entries[8].opname = "wasmalias";
  entries[8].dsblksiz = sizeof(ALIAS_FIXTURE);
  entries[8].outypes = "k";
  entries[8].intypes = "k";
  entries[8].perf = (SUBR) alias_perf;

  entries[9].opname = "wasminiterror";
  entries[9].dsblksiz = sizeof(STATE_FIXTURE);
  entries[9].outypes = "i";
  entries[9].intypes = "";
  entries[9].init = (SUBR) init_error;
  entries[9].deinit = (SUBR) state_deinit;

  return OPCODE_COUNT * (int64_t) sizeof(*entries);
}

PUBLIC int32_t csoundModuleInfo(void)
{
  return CSOUND_MODULE_INFO;
}
