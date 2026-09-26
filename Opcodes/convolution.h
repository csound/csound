/* Output storage shared by the direct and partitioned table convolvers. */
#ifndef CSOUND_CONVOLUTION_H
#define CSOUND_CONVOLUTION_H

#include "arrays.h"
#include <math.h>

#define CONV_MAX_OUTPUTS 32
#define CONV_OUTPUT_TYPES "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"

typedef struct {
    cs_float **scalar;
    ARRAYDAT *array;
    int32_t channels;
} CONV_OUTPUT;

static inline int32_t conv_array_channels(cs_float channels)
{
    cs_double n = (cs_double) channels;
    return n >= 1 && n <= (INT32_MAX + 0.0) && n == floor(n) ? (int32_t)n : 0;
}

static inline int32_t conv_output_init(CSOUND *csound, OPDS *h,
                                        CONV_OUTPUT *out)
{
    if (out->array != NULL) {
      if (out->array->dimensions > 1)
        return csound->InitError(csound, "%s",
                                Str("convolution: output must be a one-dimensional array"));
      if (tabinit(csound, out->array, out->channels, h->insdshead) != OK)
        return csound_array_init_resize_error(csound);
    }
    return OK;
}

static inline int32_t conv_output_ready(CSOUND *csound, OPDS *h,
                                         const CONV_OUTPUT *out)
{
    const ARRAYDAT *a = out->array;
    if (a != NULL &&
        (a->data == NULL || a->dimensions != 1 || a->sizes == NULL ||
         a->sizes[0] < out->channels ||
         a->arrayMemberSize < h->insdshead->ksmps * sizeof(cs_float) ||
         a->arrayMemberSize % sizeof(cs_float) != 0 ||
         (size_t)out->channels > a->allocated / a->arrayMemberSize))
      return csound->PerfError(csound, h, "%s",
                              Str("convolution: output array is too small"));
    return OK;
}

static inline cs_float *conv_output_channel(const CONV_OUTPUT *out, int32_t ch)
{
    return out->array != NULL
        ? out->array->data + (size_t)ch *
                            (out->array->arrayMemberSize / sizeof(cs_float))
        : out->scalar[ch];
}

#endif
