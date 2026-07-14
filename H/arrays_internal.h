#ifndef CSOUND_ARRAYS_INTERNAL_H
#define CSOUND_ARRAYS_INTERNAL_H

#include "csoundCore.h"
#include "arrays.h"

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

/* Engine mode 2 is the performance pass. Mixed init/perf writers may clone
   during initialization, but must only claim unique storage at performance. */
static inline int32_t csound_array_prepare_write_for_mode(
    CSOUND *csound, ARRAYDAT *array, INSDS *ctx)
{
    if (csound != NULL && csound->mode == 2) {
        return csound_array_try_prepare_write(csound, array, ctx);
    }
    return csound_array_prepare_write(csound, array, ctx);
}

#ifdef __cplusplus
}
#endif

#endif
