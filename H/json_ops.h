/* JSON conversion for declared Csound types.
   SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef CSOUND_JSON_OPS_H
#define CSOUND_JSON_OPS_H

#include "csoundCore.h"

typedef struct {
  OPDS h;
  void *out;
  STRINGDAT *source;
  MYFLT *flags;
  MYFLT *maxdepth;
} JSON_UNMARSHAL;

typedef struct {
  OPDS h;
  STRINGDAT *out;
  void *value;
  MYFLT *pretty;
  MYFLT *maxdepth;
} JSON_MARSHAL;

int32_t json_unmarshal(CSOUND *csound, JSON_UNMARSHAL *p);
int32_t json_unmarshal_file(CSOUND *csound, JSON_UNMARSHAL *p);
int32_t json_marshal(CSOUND *csound, JSON_MARSHAL *p);

#endif
