/* Included only in the browser test artifact, never in the shipped reactor. */
#include "csoundCore.h"
#include "cs_internal.h"
#include <stdatomic.h>
#include <stdint.h>

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

__attribute__((used))
uintptr_t csoundWasiInstallThreadProbe(CSOUND *csound)
{
    for (int index = 0; index < 4; ++index)
        atomic_store(&probe[index], 0);
    if (csoundAppendOpcode(csound, "test_wasi_browser_hold", sizeof(OPDS), 0,
                           "", "", hold_browser_init, NULL, NULL) != 0)
        return 0;
    return (uintptr_t) &probe[0];
}
