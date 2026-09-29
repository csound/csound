/* Example plugin: JSON note data to a plugin-owned UDT array.
   SPDX-License-Identifier: LGPL-2.1-or-later */
#include "csdl.h"
#include <math.h>
#include <string.h>

typedef struct {
  OPDS h;
  ARRAYDAT *out;
  STRINGDAT *source;
} NOTES;

typedef struct {
  OPDS h;
  void *out;
  STRINGDAT *source;
} DECODE;

typedef struct {
  OPDS h;
  STRINGDAT *out;
  void *value;
} ENCODE;

static int32_t read_notes(CSOUND *csound, NOTES *p, int file)
{
  const CSOUND_JSON_API *json = csound->GetJsonAPI(CSOUND_JSON_API_VERSION);
  CSOUND_JSON_ERROR error;
  CSOUND_JSON_DOCUMENT *doc = file
    ? json->ParseFile(csound, p->source->data, 0, &error)
    : json->Parse(csound, p->source->data, strlen(p->source->data), 0, &error);
  if (doc == NULL)
    return csound->InitError(csound, "plugin_notes: %s at byte %zu",
                             error.message, error.position);
  CSOUND_JSON_VALUE *notes = json->Root(doc);
  const char *failure = "expected an array of notes";
  if (json->Kind(notes) != CSOUND_JSON_ARRAY) goto invalid;
  for (size_t i = 0; i < json->Size(notes); ++i) {
    CSOUND_JSON_VALUE *note = json->Element(notes, i);
    CSOUND_JSON_VALUE *pitch = json->Member(note, "note");
    CSOUND_JSON_VALUE *velocity = json->Member(note, "velocity");
    failure = "each note needs numeric note and velocity fields";
    if (json->Kind(note) != CSOUND_JSON_OBJECT || json->Size(note) != 2 ||
        json->Kind(pitch) != CSOUND_JSON_NUMBER ||
        json->Kind(velocity) != CSOUND_JSON_NUMBER) goto invalid;
    double n = json->Number(pitch), v = json->Number(velocity);
    failure = "note and velocity must be integers from 0 to 127";
    if (!isfinite(n) || !isfinite(v) || n < 0 || n > 127 || v < 0 || v > 127 ||
        floor(n) != n || floor(v) != v) goto invalid;
    failure = "could not rename note fields";
    if (json->Rename(doc, note, "note", "inote") != OK ||
        json->Rename(doc, note, "velocity", "ivelocity") != OK) goto invalid;
  }
  int32_t result = json->Decode(csound, p->h.insdshead, p->out, doc, 0);
  json->Free(doc);
  return result;
invalid:
  json->Free(doc);
  return csound->InitError(csound, "plugin_notes: %s", failure);
}

static int32_t notes_string(CSOUND *csound, NOTES *p)
{ return read_notes(csound, p, 0); }
static int32_t notes_file(CSOUND *csound, NOTES *p)
{ return read_notes(csound, p, 1); }
static int32_t decode_string(CSOUND *csound, DECODE *p)
{
  return csound->GetJsonAPI(1)->Unmarshal(csound, p->h.insdshead, p->out,
                                         p->source->data, 0, 0);
}
static int32_t decode_file(CSOUND *csound, DECODE *p)
{
  return csound->GetJsonAPI(1)->UnmarshalFile(csound, p->h.insdshead, p->out,
                                             p->source->data, 0, 0);
}
static int32_t encode(CSOUND *csound, ENCODE *p)
{
  return csound->GetJsonAPI(1)->Marshal(csound, p->h.insdshead, p->out,
                                       p->value, 0, 0);
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  const CSOUND_STRUCT_MEMBER fields[] = {{"inote", "i"}, {"ivelocity", "i"}};
  if (csound->RegisterStruct == NULL || csound->GetJsonAPI == NULL ||
      csound->GetJsonAPI(CSOUND_JSON_API_VERSION) == NULL) return NOTOK;
  return csound->RegisterStruct(csound, "NoteTuplet", fields, 2) ? OK : NOTOK;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
  static OENTRY opcodes[] = {
    {"plugin_notes", sizeof(NOTES), 0, ":NoteTuplet;[]", "S", (SUBR)notes_string, NULL, NULL},
    {"plugin_notesfile", sizeof(NOTES), 0, ":NoteTuplet;[]", "S", (SUBR)notes_file, NULL, NULL},
    {"plugin_jsondecode", sizeof(DECODE), 0, ".", "S", (SUBR)decode_string, NULL, NULL},
    {"plugin_jsondecodefile", sizeof(DECODE), 0, ".", "S", (SUBR)decode_file, NULL, NULL},
    {"plugin_jsonencode", sizeof(ENCODE), 0, "S", ".", (SUBR)encode, NULL, NULL}
  };
  return csound->AppendOpcodes(csound, opcodes,
                              (int32_t)(sizeof(opcodes) / sizeof(opcodes[0])));
}
PUBLIC int32_t csoundModuleInfo(void) { return CSOUND_MODULE_INFO; }
