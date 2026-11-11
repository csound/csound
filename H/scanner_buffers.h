#ifndef CSOUND_SCANNER_BUFFERS_H
#define CSOUND_SCANNER_BUFFERS_H

typedef struct {
    void *current;
    /* yy_scan_string switches buffers, so retain parents for later cleanup. */
    void **parents;
    size_t count;
    size_t capacity;
} SCANNER_BUFFERS;

static inline void scanner_buffers_push(CSOUND *csound, SCANNER_BUFFERS *buffers)
{
    if (buffers->count == buffers->capacity) {
      size_t capacity = buffers->capacity ? buffers->capacity * 2 : 8;
      void **parents = (void **) csound->ReAlloc(csound, buffers->parents,
                                                capacity * sizeof(*parents));
      if (UNLIKELY(parents == NULL)) {
        csound->ErrorMsg(csound, "%s\n", Str("Memory exhausted"));
        csound->LongJmp(csound, 1);
        abort();
      }
      buffers->parents = parents;
      buffers->capacity = capacity;
    }
    buffers->parents[buffers->count++] = buffers->current;
}

#endif
