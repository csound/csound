#include "osc_blob.h"

#include <math.h>
#include <string.h>

static void clear_myflt_view(OSC_MYFLT_BLOB_VIEW *view)
{
    if (view != NULL) {
      view->data = NULL;
      view->count = 0;
    }
}

int32_t osc_blob_parse_myflts(const void *payload, size_t payloadBytes,
                              OSC_MYFLT_BLOB_VIEW *view)
{
    clear_myflt_view(view);
    if (view == NULL || (payload == NULL && payloadBytes != 0) ||
        payloadBytes % sizeof(MYFLT) != 0) {
      return NOTOK;
    }
    view->data = (const unsigned char *)payload;
    view->count = payloadBytes / sizeof(MYFLT);
    return OK;
}

int32_t osc_blob_parse_audio(const void *payload, size_t payloadBytes,
                             size_t sampleLimit,
                             OSC_MYFLT_BLOB_VIEW *view)
{
    const unsigned char *bytes = (const unsigned char *)payload;
    MYFLT advertised;
    uint32_t advertisedCount;
    size_t available;

    clear_myflt_view(view);
    if (view == NULL || payload == NULL || payloadBytes < sizeof(MYFLT) ||
        (payloadBytes - sizeof(MYFLT)) % sizeof(MYFLT) != 0) {
      return NOTOK;
    }
    memcpy(&advertised, bytes, sizeof(advertised));
    if (!isfinite((double)advertised) || advertised < FL(0.0) ||
        advertised > (MYFLT)UINT32_MAX) {
      return NOTOK;
    }
    advertisedCount = (uint32_t)advertised;
    if ((MYFLT)advertisedCount != advertised) {
      return NOTOK;
    }
    available = (payloadBytes - sizeof(MYFLT)) / sizeof(MYFLT);
    view->count = (size_t)advertisedCount;
    if (view->count > available) {
      view->count = available;
    }
    if (view->count > sampleLimit) {
      view->count = sampleLimit;
    }
    view->data = bytes + sizeof(MYFLT);
    return OK;
}

int32_t osc_blob_array_size(const OSC_ARRAY_BLOB_VIEW *view, int32_t index,
                            int32_t *size)
{
    if (view == NULL || size == NULL || view->sizes == NULL ||
        index < 0 || index >= view->dimensions) {
      return NOTOK;
    }
    memcpy(size, view->sizes + (size_t)index * sizeof(int32_t), sizeof(*size));
    return OK;
}

int32_t osc_blob_parse_array(const void *payload, size_t payloadBytes,
                             OSC_ARRAY_BLOB_VIEW *view)
{
    /* Array blobs contain a dimension count, dimension sizes, then MYFLTs. */
    const unsigned char *bytes = (const unsigned char *)payload;
    size_t headerBytes;
    size_t valueCount = 1;
    size_t valueBytes;
    int32_t dimensions;
    int32_t i;

    if (view != NULL) {
      view->dimensions = 0;
      view->sizes = NULL;
      clear_myflt_view(&view->values);
    }
    if (view == NULL || payload == NULL || payloadBytes < sizeof(int32_t)) {
      return NOTOK;
    }
    memcpy(&dimensions, bytes, sizeof(dimensions));
    if (dimensions <= 0 ||
        (size_t)dimensions >
          (payloadBytes - sizeof(int32_t)) / sizeof(int32_t)) {
      return NOTOK;
    }
    headerBytes = sizeof(int32_t) +
      (size_t)dimensions * sizeof(int32_t);
    view->dimensions = dimensions;
    view->sizes = bytes + sizeof(int32_t);
    for (i = 0; i < dimensions; i++) {
      int32_t dimensionSize;
      if (osc_blob_array_size(view, i, &dimensionSize) != OK ||
          dimensionSize < 0) {
        return NOTOK;
      }
      if (dimensionSize == 0) {
        valueCount = 0;
      }
      else if (valueCount != 0) {
        if ((size_t)dimensionSize > SIZE_MAX / valueCount) {
          return NOTOK;
        }
        valueCount *= (size_t)dimensionSize;
      }
    }
    if (valueCount > SIZE_MAX / sizeof(MYFLT)) {
      return NOTOK;
    }
    valueBytes = valueCount * sizeof(MYFLT);
    if (payloadBytes - headerBytes != valueBytes) {
      return NOTOK;
    }
    view->values.data = bytes + headerBytes;
    view->values.count = valueCount;
    return OK;
}
