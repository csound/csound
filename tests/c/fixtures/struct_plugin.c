/* Example plugin: declare, return and accept a UDT.
   SPDX-License-Identifier: LGPL-2.1-or-later */
#include "csdl.h"

typedef struct {
  OPDS h;
  CS_STRUCT_VAR *out;
  cs_float *note;
  cs_float *velocity;
} NOTE;

typedef struct {
  OPDS h;
  cs_float *out;
  CS_STRUCT_VAR *note;
} NOTE_PITCH;

static int32_t make_note(CSOUND *csound, NOTE *p)
{
  (void)csound;
  p->out->members[0]->value = *p->note;
  p->out->members[1]->value = *p->velocity;
  return OK;
}

static int32_t note_pitch(CSOUND *csound, NOTE_PITCH *p)
{
  (void)csound;
  *p->out = p->note->members[0]->value;
  return OK;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  const CSOUND_STRUCT_MEMBER fields[] = {{"inote", "i"}, {"ivelocity", "i"}};
  if (csound->RegisterStruct == NULL) return NOTOK;
  return csound->RegisterStruct(csound, "NoteTuplet", fields, 2) ? OK : NOTOK;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
  static OENTRY opcodes[] = {
    {"plugin_note", sizeof(NOTE), 0, ":NoteTuplet;", "ii",
     (SUBR)make_note, NULL, NULL},
    {"plugin_note_pitch", sizeof(NOTE_PITCH), 0, "i", ":NoteTuplet;",
     (SUBR)note_pitch, NULL, NULL}
  };
  return csound->AppendOpcodes(csound, opcodes,
                              (int32_t)(sizeof(opcodes) / sizeof(opcodes[0])));
}

PUBLIC int32_t csoundModuleInfo(void) { return CSOUND_MODULE_INFO; }
