/* Included only in the browser test artifact, never in the shipped reactor. */
#include "csoundCore.h"
#include "cs_internal.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

/* The browser test controls this storage directly while rendering or join waits.
   0: entered; 1: release (negative traps); 2: completed; 3: Csound worker. */
static _Atomic int32_t probe[4];

static int32_t hold_browser_init(CSOUND *csound, void *opcode)
{
    (void) csound;
    (void) opcode;
    atomic_store(&probe[3], csoundWasiIsWorkerThread());
    atomic_store(&probe[0], 1);
    while (!atomic_load(&probe[1]))
        __builtin_wasm_memory_atomic_wait32((int32_t *) &probe[1], 0, 1000000000LL);
    if (atomic_load(&probe[1]) < 0)
        __builtin_trap();
    atomic_store(&probe[2], 1);
    return 0;
}

typedef struct {
    OPDS h;
    cs_float *result;
} HEAP_PROBE;

static int32_t free_browser_heap(CSOUND *csound, HEAP_PROBE *opcode)
{
    (void) csound;
    /* WASI libc has no mallinfo. Probe reusable capacity without touching the
       large blocks. Run only in tests, on the background API thread. */
    /* Keep these calls observable so Clang cannot fold unused malloc/free. */
    void *(*volatile allocate)(size_t) = malloc;
    void (*volatile release)(void *) = free;
    size_t low = 0;
    size_t high = (size_t) __builtin_wasm_memory_size(0) * 16;
    while (low < high) {
      size_t middle = low + (high - low + 1) / 2;
      void *memory = allocate(middle * 4096);
      if (memory != NULL) {
        release(memory);
        low = middle;
      } else {
        high = middle - 1;
      }
    }
    *opcode->result = (cs_float) low * 4096;
    return 0;
}

static int32_t browser_macro_arguments(CSOUND *csound, HEAP_PROBE *opcode)
{
    int32_t count = 0;
    for (MACRO *macro = csound->orc_macros; macro != NULL; macro = macro->next)
      if (macro->name[0] == '`')
        count++;
    *opcode->result = count;
    return 0;
}

__attribute__((used))
uintptr_t csoundWasiInstallThreadProbe(CSOUND *csound)
{
    for (int index = 0; index < 4; ++index)
        atomic_store(&probe[index], 0);
    if (csoundAppendOpcode(csound, "test_wasi_browser_hold", sizeof(OPDS), 0,
                           "", "", hold_browser_init, NULL, NULL) != 0)
        return 0;
    if (csoundAppendOpcode(csound, "test_wasi_browser_heap_free", sizeof(HEAP_PROBE), 0,
                           "i", "", (SUBR) free_browser_heap, NULL, NULL) != 0)
        return 0;
    if (csoundAppendOpcode(csound, "test_wasi_browser_macro_args", sizeof(HEAP_PROBE), 0,
                           "i", "", (SUBR) browser_macro_arguments, NULL, NULL) != 0)
        return 0;
    return (uintptr_t) &probe[0];
}
