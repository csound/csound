#include "csdl.h"

typedef struct {
  OPDS h;
  MYFLT *output;
  MYFLT *input;
} REQUESTED_OPCODE;

static int32_t requested_opcode_init(CSOUND *csound, REQUESTED_OPCODE *opcode)
{
  (void) csound;
  *opcode->output = *opcode->input;
  return OK;
}

static OENTRY localops[] = {
    {"requested_opcode_fixture", sizeof(REQUESTED_OPCODE), 0, "i", "i",
     (SUBR) requested_opcode_init, NULL, NULL}};

LINKAGE
