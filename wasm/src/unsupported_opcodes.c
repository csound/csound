#include "csdl.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct { OPDS h; } NOOP;

#define S sizeof(NOOP)


static int32_t osc_unsupported_warning(CSOUND *csound, NOOP *p) {
  (void) csound->ErrorMsg(csound,Str("OSC opcodes are currently not supported in WASM %s\n"), "");
  return CSOUND_ERROR;
}

static OENTRY OSC_localops[] =
  {{ "OSCsend_lo", S, 0, "", "kSkSS*",
     (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning },
   { "OSCinit", S, 0, "i", "i",
     (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning },
   { "OSCinitM", S, 0, "i", "Si",
     (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning },
   { "OSClisten", S,0, "k", "iSS*",
     (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning },
   { "OSClisten", S, 0, "k", "iSS",
     (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning },
   { "OSClisten", S,0, "kk[]", "iSS",
     (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning},
   { "OSCcount", S, 0, "k", "",
     (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning},
   { "OSCsend", S, 0, "", "kSkSS*",
     (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning},
   { "OSCbundle", S, 0, "", "kSkS[]S[]k[][]o",
     (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning, (SUBR)osc_unsupported_warning }};

int32_t unsupported_opdoces_init_(CSOUND *csound) {
  return csound->AppendOpcodes(csound, &(OSC_localops[0]), (int32_t) (sizeof(OSC_localops) / sizeof(OENTRY)));
}
