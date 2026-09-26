#ifndef CSOUND_ARRAYS_INTERNAL_H
#define CSOUND_ARRAYS_INTERNAL_H

#include "csoundCore.h"
#include "arrays.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  CSOUND_ARRAY_COPY_NO_ALLOCATION,
  CSOUND_ARRAY_COPY_ALLOW_ALLOCATION
} CSOUND_ARRAY_COPY_MODE;

int32_t csound_array_copy_independent(CSOUND *csound, ARRAYDAT *destination,
                                      const ARRAYDAT *source, INSDS *ctx,
                                      CSOUND_ARRAY_COPY_MODE mode);
int32_t csound_array_prepare_write_impl(CSOUND *csound, ARRAYDAT *array,
                                        INSDS *ctx, int32_t allowAllocation);
int32_t csound_array_prepare_opcode_write_impl(
    CSOUND *csound, ARRAYDAT *array, OPDS *opds, int32_t initializing,
    const char *errorMessage);
int32_t csound_array_ensure_capacity_impl(CSOUND *csound, ARRAYDAT *array,
                                          size_t capacity, INSDS *ctx);
int32_t csound_array_initialize_element_range(
    CSOUND *csound, ARRAYDAT *array, size_t dataBytes, size_t begin,
    size_t end, INSDS *ctx);
void csound_free_array_storage(CSOUND *csound, ARRAYDAT *array);
size_t csound_array_allocated_bytes(CSOUND *csound, const ARRAYDAT *array);
int32_t csound_array_storage_matches(CSOUND *csound,
                                     const ARRAYDAT *array);

#ifdef __cplusplus
}
#endif

#endif
