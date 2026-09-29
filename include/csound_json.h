/* JSON services for plugins. SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef CSOUND_JSON_H
#define CSOUND_JSON_H
#include "csound.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
struct insds;
typedef struct csound_json_document CSOUND_JSON_DOCUMENT;
typedef struct csound_json_value CSOUND_JSON_VALUE;
typedef enum {
  CSOUND_JSON_INVALID, CSOUND_JSON_NULL, CSOUND_JSON_BOOLEAN,
  CSOUND_JSON_NUMBER, CSOUND_JSON_STRING, CSOUND_JSON_ARRAY, CSOUND_JSON_OBJECT
} CSOUND_JSON_KIND;
typedef struct {
  char message[256];
  size_t position; /* byte offset for parse errors */
} CSOUND_JSON_ERROR;

#define CSOUND_JSON_API_VERSION 1

/** Obtain with csound->GetJsonAPI(CSOUND_JSON_API_VERSION).
 * All calls allocate or inspect initialization-time data, not real-time data.
 * Documents own their values and strings. Free documents with Free(). Never
 * mix values from different documents. NULL lookups have kind INVALID.
 * Read flags: 1 allows comments, 2 allows trailing commas. Depth is capped at
 * 256. Strings/keys passed to mutation calls are copied. Mutation returns
 * OK/NOTOK; Rename rejects an existing destination key. Number/String/Boolean
 * require the corresponding Kind; otherwise they return 0/NULL/0.
 * Conversion calls accept initialized Csound opcode arguments (including their
 * type headers), report InitError on failure and leave outputs unchanged.
 * depth=0 selects 256. Unmarshal/File/Decode accept UDTs or 1D typed arrays.
 * pretty is 0 or 1. Csound owns STRINGDAT outputs. */
typedef struct {
  uint32_t version;
  size_t size;
  CSOUND_JSON_DOCUMENT *(*Parse)(CSOUND *, const char *, size_t, uint32_t,
                                 CSOUND_JSON_ERROR *);
  CSOUND_JSON_DOCUMENT *(*ParseFile)(CSOUND *, const char *, uint32_t,
                                     CSOUND_JSON_ERROR *);
  void (*Free)(CSOUND_JSON_DOCUMENT *);
  CSOUND_JSON_VALUE *(*Root)(CSOUND_JSON_DOCUMENT *);
  CSOUND_JSON_KIND (*Kind)(const CSOUND_JSON_VALUE *);
  size_t (*Size)(const CSOUND_JSON_VALUE *);
  CSOUND_JSON_VALUE *(*Member)(const CSOUND_JSON_VALUE *, const char *);
  CSOUND_JSON_VALUE *(*Element)(const CSOUND_JSON_VALUE *, size_t);
  double (*Number)(const CSOUND_JSON_VALUE *);
  const char *(*String)(const CSOUND_JSON_VALUE *);
  int32_t (*Boolean)(const CSOUND_JSON_VALUE *);
  int32_t (*Rename)(CSOUND_JSON_DOCUMENT *, CSOUND_JSON_VALUE *,
                     const char *, const char *);
  int32_t (*SetNumber)(CSOUND_JSON_VALUE *, double);
  int32_t (*SetString)(CSOUND_JSON_DOCUMENT *, CSOUND_JSON_VALUE *, const char *);
  int32_t (*Remove)(CSOUND_JSON_VALUE *, const char *);
  int32_t (*Write)(CSOUND *, CSOUND_JSON_DOCUMENT *, STRINGDAT *, uint32_t);
  int32_t (*Decode)(CSOUND *, struct insds *, void *,
                    CSOUND_JSON_DOCUMENT *, uint32_t);
  int32_t (*Unmarshal)(CSOUND *, struct insds *, void *, const char *,
                       uint32_t, uint32_t);
  int32_t (*UnmarshalFile)(CSOUND *, struct insds *, void *, const char *,
                           uint32_t, uint32_t);
  int32_t (*Marshal)(CSOUND *, struct insds *, STRINGDAT *, void *,
                     uint32_t, uint32_t);
} CSOUND_JSON_API;
PUBLIC const CSOUND_JSON_API *csoundGetJsonAPI(uint32_t version);
#ifdef __cplusplus
}
#endif
#endif
