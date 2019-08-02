#ifndef EMUGENS_COMMON_H
#define EMUGENS_COMMON_H

#include "csdl.h"


#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
#define MSG(m) (csound->Message(csound, m))
#define MSGF(fmt, ...) (csound->Message(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))


#define CHECKARR1D(arr)           \
    if((arr)->dimensions != 1)    \
        return INITERRF(Str("Array should be of 1D, but has %d dimensions"), \
                        (arr)->dimensions);

// This must be called for each array at each perf pass
// (at init call tabinit)
#define ARRAY_ENSURESIZE(csound, arr, size) tabcheck(csound, arr, size, &(p->h))


// This is deprecated, we use the new methods (6.13) tabinit and tabcheck
// as defined in arrays.h

// Ensure the existence and size of the array at i-time ONLY if
// array has not been initialized, so we don't need to check intialization
// at k-time
static inline void
tabensure_init(CSOUND *csound, ARRAYDAT *p, int size)
{
    size_t ss;
    if (p->dimensions==0) {
        p->dimensions = 1;
        p->sizes = (int32_t*)csound->Malloc(csound, sizeof(int32_t));
    }
    if (p->data == NULL) {
        CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
        p->arrayMemberSize = var->memBlockSize;
        ss = p->arrayMemberSize*size;
        p->data = (MYFLT*)csound->Calloc(csound, ss);
        p->allocated = ss;
    } else if( (ss = p->arrayMemberSize*size) > p->allocated) {
        p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
        p->allocated = ss;
    }
    p->sizes[0] = size;
    p->dimensions = 1;
}



/*
#define TABENSURE_PERF(csound, arr, size) {
    if(UNLIKELY(arr->sizes[0] != size)) {                                                  \
        size_t _ss = arr->arrayMemberSize*size;                                            \
        if (_ss > arr->allocated) {                                                        \
            return PERFERRF(Str("Array is too small (allocated %zu < needed %zu), but cannot " \
                            "allocate during performance pass. Allocate a bigger array "   \
                            "at init time"),                                               \
                            arr->allocated, _ss);                                          \
        }                                                                                  \
        arr->sizes[0] = size;                                                              \
    }}

*/

#endif
