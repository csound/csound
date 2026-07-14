#ifndef CSOUND_ARRAYS_INTERNAL_H
#define CSOUND_ARRAYS_INTERNAL_H

#include "csoundCore.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t csound_array_copy_independent(CSOUND *csound, ARRAYDAT *destination,
                                      const ARRAYDAT *source, INSDS *ctx);
int32_t csound_array_prepare_write_impl(CSOUND *csound, ARRAYDAT *array,
                                        INSDS *ctx, int32_t allowAllocation);
void csound_free_array_storage(CSOUND *csound, ARRAYDAT *array);
size_t csound_array_allocated_bytes(CSOUND *csound, const ARRAYDAT *array);
int32_t csound_array_storage_matches(CSOUND *csound,
                                     const ARRAYDAT *array);

#ifdef __cplusplus
}
#endif

#endif
