/* Included only in the browser test artifact, never in the shipped reactor. */
#include "csoundCore.h"
#include "cs_internal.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The browser test controls this storage directly while rendering or join waits.
   0: entered; 1: release (negative traps); 2: completed; 3: Csound worker. */
static _Atomic int32_t probe[4];
static int32_t freed_published_lookup;
static _Thread_local int32_t checking_channel_allocations;
static _Thread_local int32_t locked_channel_allocations;

static void check_channel_allocation(CSOUND *csound)
{
    if (checking_channel_allocations &&
        __atomic_load_n(&csound->wasi_channel_lock, __ATOMIC_RELAXED))
        locked_channel_allocations++;
}

static void *check_browser_malloc(CSOUND *csound, size_t size)
{
    check_channel_allocation(csound);
    return csoundMalloc(csound, size);
}

static void *check_browser_calloc(CSOUND *csound, size_t size)
{
    check_channel_allocation(csound);
    return csoundCalloc(csound, size);
}

static void check_browser_free(CSOUND *csound, void *memory)
{
    check_channel_allocation(csound);
    /* Observe the exact lifetime error without relying on freed heap contents
       being overwritten before the next compilation. Only the API compiler
       thread reads the variable pool here. */
    if (csoundWasiCompileJumpContext(csound) != NULL) {
      CS_VARIABLE *variable = csoundFindVariableWithName(
          csound, csound->engineState.varPool, "KeptLookup");
      if (variable != NULL && variable->memBlock != NULL) {
        OPCODEREF *reference = (OPCODEREF *) &variable->memBlock->value;
        if (memory != NULL && memory == reference->entries)
          freed_published_lookup++;
      }
    }
    csoundFree(csound, memory);
}

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

static size_t free_browser_heap_capacity(void)
{
    /* WASI libc has no mallinfo. Count every free block that can hold a link.
       A 4 KiB minimum misses small holes left by compiler allocations.
       Touch only the first word of each block to keep a list for cleanup.
       Run only in tests, on the background API thread. */
    /* Keep these calls observable so Clang cannot fold unused malloc/free. */
    void *(*volatile allocate)(size_t) = malloc;
    void (*volatile release)(void *) = free;
    void **blocks = NULL;
    size_t total = 0;
    const size_t unit = sizeof(void *);
    size_t high = (size_t) __builtin_wasm_memory_size(0) * (65536 / unit);
    while (high > 0) {
      size_t low = 0;
      while (low < high) {
        size_t middle = low + (high - low + 1) / 2;
        void *memory = allocate(middle * unit);
        if (memory != NULL) {
          release(memory);
          low = middle;
        } else {
          high = middle - 1;
        }
      }
      if (low == 0)
        break;
      void **memory = allocate(low * unit);
      if (memory == NULL)
        break;
      /* Keep each block until counting finishes. Measuring only the largest
         block mistakes fragmentation for a leak after a rejected edit. */
      *memory = blocks;
      blocks = memory;
      total += low * unit;
      high = low;
    }
    while (blocks != NULL) {
      void **next = *blocks;
      release(blocks);
      blocks = next;
    }
    return total;
}

static int32_t free_browser_heap(CSOUND *csound, HEAP_PROBE *opcode)
{
    (void) csound;
    *opcode->result = (cs_float) free_browser_heap_capacity();
    return 0;
}

static int32_t fragmented_browser_heap(CSOUND *csound, HEAP_PROBE *opcode)
{
    (void) csound;
    /* Both blocks exceed the small holes left by compilation. The second
       keeps the first from merging with the large free block at the end. */
    const size_t size = 8 * 1024 * 1024;
    void *(*volatile allocate)(size_t) = malloc;
    void (*volatile release)(void *) = free;
    void *hole = allocate(size);
    void *guard = allocate(size);
    if (hole == NULL || guard == NULL) {
      release(hole);
      release(guard);
      *opcode->result = -1;
      return 0;
    }
    size_t before = free_browser_heap_capacity();
    release(hole);
    size_t after = free_browser_heap_capacity();
    release(guard);
    *opcode->result = (cs_float) after - (cs_float) before;
    return 0;
}

static int32_t small_fragmented_browser_heap(CSOUND *csound, HEAP_PROBE *opcode)
{
    (void) csound;
    /* Keep alternating allocations live so the freed KiB blocks cannot
       coalesce. Together they exceed the compiler leak check's tolerance. */
    void *blocks[256] = {0};
    void *(*volatile allocate)(size_t) = malloc;
    void (*volatile release)(void *) = free;
    *opcode->result = -1;
    for (size_t index = 0; index < 256; index++) {
      blocks[index] = allocate(1024);
      if (blocks[index] == NULL)
        goto cleanup;
    }
    size_t before = free_browser_heap_capacity();
    for (size_t index = 0; index < 256; index += 2) {
      release(blocks[index]);
      blocks[index] = NULL;
    }
    *opcode->result = (cs_float) free_browser_heap_capacity() - (cs_float) before;
cleanup:
    for (size_t index = 0; index < 256; index++)
      release(blocks[index]);
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

static int32_t browser_freed_lookup(CSOUND *csound, HEAP_PROBE *opcode)
{
    (void) csound;
    *opcode->result = freed_published_lookup;
    return 0;
}

static int32_t browser_channel_allocations(CSOUND *csound, HEAP_PROBE *opcode)
{
    controlChannelHints_t hints = {0}, copied = {0};
    controlChannelInfo_t *channels = NULL;
    char attributes[4097];
    int32_t result = 0;
    memset(attributes, 'x', sizeof(attributes) - 1);
    attributes[sizeof(attributes) - 1] = '\0';
    hints.behav = CSOUND_CONTROL_CHANNEL_LIN;
    hints.max = 1;
    hints.attributes = attributes;
    locked_channel_allocations = 0;
    checking_channel_allocations = 1;
    /* The host runs this before starting dynamic-name channel opcodes. The
       only registry operations during these hooks belong to this thread. */
    for (int32_t index = 0; index < 256; index++) {
        char name[48];
        void *value;
        snprintf(name, sizeof(name), "allocation-probe-%03d", index);
        if (csoundGetChannelPtr(csound, &value, name,
                               CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL) ||
            csoundSetControlChannelHints(csound, name, hints) ||
            csoundGetControlChannelHints(csound, name, &copied)) {
            result = -1;
            break;
        }
        if (strcmp(copied.attributes, attributes) != 0)
            result = -1;
        csoundDeleteControlChannelHints(csound, &copied);
    }
    int32_t count = csoundListChannels(csound, &channels);
    if (count < 256)
        result = -1;
    csoundDeleteChannelList(csound, channels);
    checking_channel_allocations = 0;
    *opcode->result = result != 0 ? result : locked_channel_allocations;
    return 0;
}

__attribute__((used))
uintptr_t csoundWasiInstallThreadProbe(CSOUND *csound)
{
    freed_published_lookup = 0;
    csound->Malloc = check_browser_malloc;
    csound->Calloc = check_browser_calloc;
    csound->Free = check_browser_free;
    for (int index = 0; index < 4; ++index)
        atomic_store(&probe[index], 0);
    if (csoundAppendOpcode(csound, "test_wasi_browser_hold", sizeof(OPDS), 0,
                           "", "", hold_browser_init, NULL, NULL) != 0)
        return 0;
    if (csoundAppendOpcode(csound, "test_wasi_browser_heap_free", sizeof(HEAP_PROBE), 0,
                           "i", "", (SUBR) free_browser_heap, NULL, NULL) != 0)
        return 0;
    if (csoundAppendOpcode(csound, "test_wasi_browser_heap_fragment", sizeof(HEAP_PROBE), 0,
                           "i", "", (SUBR) fragmented_browser_heap, NULL, NULL) != 0)
        return 0;
    if (csoundAppendOpcode(csound, "test_wasi_browser_heap_small_fragments", sizeof(HEAP_PROBE), 0,
                           "i", "", (SUBR) small_fragmented_browser_heap, NULL, NULL) != 0)
        return 0;
    if (csoundAppendOpcode(csound, "test_wasi_browser_macro_args", sizeof(HEAP_PROBE), 0,
                           "i", "", (SUBR) browser_macro_arguments, NULL, NULL) != 0)
        return 0;
    if (csoundAppendOpcode(csound, "test_wasi_browser_freed_lookup", sizeof(HEAP_PROBE), 0,
                           "i", "", (SUBR) browser_freed_lookup, NULL, NULL) != 0)
        return 0;
    if (csoundAppendOpcode(csound, "test_wasi_browser_channel_allocs", sizeof(HEAP_PROBE), 0,
                           "i", "", (SUBR) browser_channel_allocations, NULL, NULL) != 0)
        return 0;
    return (uintptr_t) &probe[0];
}
