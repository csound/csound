#ifndef CSOUND_OSC_BLOB_H
#define CSOUND_OSC_BLOB_H

#include "csdl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Keep packet data byte-aligned; callers copy into aligned storage. */
    const unsigned char *data;
    size_t count;
} OSC_MYFLT_BLOB_VIEW;

typedef struct {
    int32_t dimensions;
    const unsigned char *sizes;
    OSC_MYFLT_BLOB_VIEW values;
} OSC_ARRAY_BLOB_VIEW;

int32_t osc_blob_parse_myflts(const void *payload, size_t payloadBytes,
                              OSC_MYFLT_BLOB_VIEW *view);
int32_t osc_blob_parse_audio(const void *payload, size_t payloadBytes,
                             size_t sampleLimit,
                             OSC_MYFLT_BLOB_VIEW *view);
int32_t osc_blob_parse_array(const void *payload, size_t payloadBytes,
                             OSC_ARRAY_BLOB_VIEW *view);
int32_t osc_blob_array_size(const OSC_ARRAY_BLOB_VIEW *view, int32_t index,
                            int32_t *size);

#ifdef __cplusplus
}
#endif

#endif
