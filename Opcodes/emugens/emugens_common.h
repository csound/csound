#ifndef EMUGENS_COMMON_H
#define EMUGENS_COMMON_H

#include "csdl.h"


#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
// VL this is needed for -Wformat-security
#define MSG(m) (csound->Message(csound, "%s", m))
#define MSGF(fmt, ...) (csound->Message(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))


#define CHECKARR1D(arr)           \
    if((arr)->dimensions != 1)    \
        return INITERRF(Str("Array should be of 1D, but has %d dimensions"), \
                        (arr)->dimensions);

// This must be called for each array at each perf pass
// (at init call tabinit)
#define ARRAY_ENSURESIZE_PERF(csound, arr, size) tabcheck(csound, arr, size, &(p->h))


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
  if(UNLIKELY(arr->sizes[0] != size)) {                                 \
        size_t _ss = arr->arrayMemberSize*size;                           \
        if (_ss > arr->allocated) {                                       \
            return PERFERRF(Str("Array is too small (allocated %zu "      \
                                "< needed %zu), but cannot allocate  "    \
                                "during performance pass. Allocate a "    \
                                "bigger array "at init time"),            \
                            arr->allocated, _ss);                         \
        }                                                                 \
        arr->sizes[0] = size;                                             \
    }}

*/


static inline
int em_isnan(MYFLT d) {
  union {
    unsigned long long l;
    double d;
  } u;
  u.d=d;
  return (u.l==0x7FF8000000000000ll ||
          u.l==0x7FF0000000000000ll ||
          u.l==0xfff8000000000000ll);
}

static inline
int em_isinf(MYFLT d) {
  union {
    unsigned long long l;
    double d;
  } u;
  u.d=d;
  return (u.l==0x7FF0000000000000ll?1:u.l==0xFFF0000000000000ll?-1:0);
}

static inline
int em_isinfornan(MYFLT d) {
    union {
      unsigned long long l;
      double d;
    } u;
    u.d=d;
    return (u.l==0x7FF8000000000000ll ||
            u.l==0x7FF0000000000000ll ||
            u.l==0xfff8000000000000ll ||
            u.l==0xFFF0000000000000ll);
}


unsigned long next_power_of_two(unsigned long v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}


// like strncpy but really makes sure that the dest str is 0 terminated
// dest should have allocated at least n+1
static inline
void strncpy0(char *dest, const char *src, size_t n) {
    strncpy(dest, src, n);
    dest[n] = '\0';
}


// make sure that the out string has enough allocated space to hold
// a string of `size` chars (the \0 should be taken into account)
// This can be run only at init time
static void
string_ensure(CSOUND *csound, STRINGDAT *s, size_t size) {
    if (s->size >= (int)size)
        return;
    size = next_power_of_two(size);
    s->data = csound->ReAlloc(csound, s->data, size);
    s->size = (int)size;
}


static void
stringdat_copy_cstr(CSOUND *csound, STRINGDAT *dest, const char *src, size_t len) {
    string_ensure(csound, dest, len+1);
    strncpy0(dest->data, src, len);
}


static void
stringdat_clear(CSOUND *csound, STRINGDAT *s) {
    string_ensure(csound, s, 1);
    s->data[0] = '\0';
}

#endif
