// when compiling with fPIC
// the compiler needs some help with
// some types (this file could and should be removed one day)

#include <unistd.h>
#include "csoundCore.h"
#include "csound_data_structures.h"
#include "csound.h"
#include "csoundCore.h"

#ifndef __csound_wasi___typedefs
#define __csound_wasi___typedefs

#ifndef __cplusplus

__attribute__((used))
CS_HASH_TABLE* cs_hash_table_create(CSOUND* csound);
/* int32_t cs_hash_table_create(int32_t cs); */

#endif
#endif
