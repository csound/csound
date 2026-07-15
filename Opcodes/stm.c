/*
    stm.c:

    Copyright (C) 2026 Pasquale Mainolfi

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/*
   STM is a family of Csound opcodes for building stateful computational
   graphs inspired by LangGraph (https://github.com/langchain-ai/langgraph).

   stmcreate builds a mutable topology, stmcompile freezes it into an immutable
   definition, and stminstance creates a mutable runner from that definition.
   Multiple runners can share one definition while keeping current node,
   requests, clocks and transition events independent. Node computation remains
   ordinary Csound code, typically written as user-defined opcodes operating on
   a shared struct passed by reference.

   At k-rate, the orchestra asks the runner for the current node with stmcurrent,
   dispatches explicitly to the matching node implementation, then calls
   stmadvance. During its execution, a node may request a transition with
   stmnext. stmadvance validates that request against the graph edges and applies
   it only if it is legal.

   The practical runtime loop is:

   stmcurrent -> dispatch current node -> node updates shared state calls stmnext -> stmadvance

   This keeps the state-machine control flow explicit and deterministic while
   leaving DSP, analysis, and musical behavior in the orchestra.

   ADVANCE STATUS

   stmadvance returns three k-rate values: status, source node ID and target
   node ID. The status values are STM_NO_REQUEST (0), STM_CHANGED (1),
   STM_ILLEGAL_EDGE (2), STM_SELF_TRANSITION (3) and STM_CONFLICT (4). The
   target is -1 when there was no unique target (no request or conflict).

   Requests share one pending slot. The first target requested before an
   advance is retained; repeating that target is idempotent, while requesting
   a different target marks a conflict. A conflicted advance applies neither
   target. This detects competing intentions without trying to identify the
   writers; a single writer changing its request also counts as a conflict.

   ENTER/EXIT EVENTS

   stmonenter and stmonexit report runner-level transition events, not an
   observer-local rising/falling edge. Each opcode instance records the graph
   event sequence that was current when it was initialized; it only triggers
   for a later event where the named node was entered or exited. Therefore an
   observer scheduled after node B is already current does not receive a fake
   stmonenter(B) on its first pass.

   These opcodes observe the latest graph event seen by that opcode call. If a
   runner is advanced through multiple transitions between two passes of a slow
   observer, intermediate events may be missed. For process lifecycle binding,
   the deterministic pattern is to run the enter/exit checks in the same
   supervisor loop that advances the graph, and bind the initial state
   explicitly if it needs an initial process.

   stmevent provides loss detection and ordered consumption through a bounded
   per-runner ring. Each opcode instance owns an independent read cursor; an
   overflow output reports when unread events were overwritten.

   GRAPH CLOCK

   Every runner carries its own clock, read with three k-rate opcodes:

     stmtick     - k-cycles elapsed since stminstance or stmreset
     stmtime     - graph time in seconds, advanced sample frames / sr
     stmnodetime - seconds elapsed since the current node became current

   The clock is driven by stmadvance, not by the global k-counter: one tick is
   one call to stmadvance. It is the state machine's own notion of time, so it
   only moves when the machine is actually stepped. If the instrument driving
   the graph is turned off, skipped, or simply not scheduled for a cycle, the
   graph clock does not move either. stmtime is therefore NOT wall-clock time
   and will diverge from times(), and stmtick is NOT timeinstk(). Use those
   opcodes instead when performance time is what is wanted.

   Two rules follow from an advance-driven clock:

     1. call stmadvance exactly once per runner per k-cycle. Multiple calls by
        its writer tick it multiple times; overlapping writers are rejected.
     2. read the time opcodes BEFORE stmadvance. After it they already report
        the values of the next cycle.

   Times are derived from integer sample-frame counters rather than accumulated
   floating-point seconds, so they cannot drift over long runs. Each stmadvance
   adds the advancing instrument's current CS_KSMPS to the graph's total sample
   count, which means successive drivers with different local setksmps values
   contribute their real control-period lengths without retroactively rescaling
   earlier steps. stmtick is counted internally in a uint64 and returned as
   MYFLT, hence exact up to 2^53 in a double build and 2^24 in a float one.

   Node time is measured from the sample-frame count at which the current node
   was entered:

     - it reads 0 on the node's own first cycle
     - a transition rejected by stmadvance (no such edge) leaves it untouched,
       since the current node never changed
     - stmreset restarts tick, graph time and node time at 0, exactly the way
       stminstance does

   REGISTRY HANDLES

   Builder, definition and runner handles are numeric MYFLT values because
   orchestra code cannot safely carry raw C pointers. One typed registry stores
   all three object kinds in reusable slots and encodes both the slot index and
   a generation counter into each handle. Every lookup validates generation and
   expected object type before casting, so a builder handle cannot be used as a
   definition or runner handle.

   The handle encoding is intentionally kept within 2^24, so all handle
   integers are exact even in single-precision MYFLT builds. With the current
   constants this allows 4096 live registry slots per CSOUND instance and 4096
   generations per slot, for up to 16,777,216 graph lifetimes before every slot
   would be exhausted.

   CONCURRENCY MODEL

   Immutable definitions may be shared freely. Each runner permits one active
   writer instrument instance and any number of observers. Multiple mutating
   opcodes in that writer are allowed, and a later writer may claim the runner
   after the previous one deinitializes; overlapping writer instruments are
   rejected during opcode initialization.

   Each opcode resolves and retains its runner during initialization, so the
   performance path does not touch the registry mutex. The writer publishes
   compound runner updates through an atomic sequence counter; observers retry
   if an update overlaps their snapshot. On platforms without compiler atomic
   builtins, the same contract uses a per-runner fallback mutex.
*/



#include "stm.h"
#include "coreDefs.h"
#include "csound.h"
#include "sysdep.h"
#include <stdio.h>
/* udo.h carries OPCODINFO pointers, while plugin builds do not expose the
   private definition from cs_internal.h.  STM only needs the type to be
   declared in order to use OPCOD_IOBUFS. */
typedef struct opcodinfo OPCODINFO;
#include "udo.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>


/* ------------------------------------------------------------------ */
/* graph registry (per csound instance)                               */
/* ------------------------------------------------------------------ */

/* Query-only: never creates the registry (safe in deinit). */
static STM_REGISTRY *stm_registry_query(CSOUND *csound) {
    return (STM_REGISTRY *) csound->QueryGlobalVariable(csound, STM_REGISTRY_NAME);
}

static void stm_registry_lock(CSOUND *csound, STM_REGISTRY *reg) {
    if (reg != NULL && reg->mutex != NULL)
        csound->LockMutex(reg->mutex);
}

static void stm_registry_unlock(CSOUND *csound, STM_REGISTRY *reg) {
    if (reg != NULL && reg->mutex != NULL)
        csound->UnlockMutex(reg->mutex);
}

static void stm_free_nodes(CSOUND *csound, GRAPH_NODE *nodes, uint32_t node_count) {
    if (nodes == NULL) return;
    for (uint32_t i = 0; i < node_count; i++) {
        if (nodes[i].edges != NULL) csound->Free(csound, nodes[i].edges);
        if (nodes[i].name != NULL) csound->Free(csound, nodes[i].name);
    }
    csound->Free(csound, nodes);
}

static uint32_t stm_ref_decrement(uint32_t *refcount) {
#if defined(MSVC)
    return (uint32_t) InterlockedDecrement((LONG *) refcount);
#elif defined(HAVE_ATOMIC_BUILTIN)
    return __atomic_sub_fetch(refcount, 1, __ATOMIC_ACQ_REL);
#else
    return --(*refcount);
#endif
}

static uint32_t stm_ref_increment(uint32_t *refcount) {
#if defined(MSVC)
    return (uint32_t) InterlockedIncrement((LONG *) refcount);
#elif defined(HAVE_ATOMIC_BUILTIN)
    return __atomic_add_fetch(refcount, 1, __ATOMIC_ACQ_REL);
#else
    return ++(*refcount);
#endif
}

static uint32_t stm_ref_load(uint32_t *refcount) {
#if defined(MSVC)
    return (uint32_t) InterlockedCompareExchange((LONG *) refcount, 0, 0);
#elif defined(HAVE_ATOMIC_BUILTIN)
    return __atomic_load_n(refcount, __ATOMIC_ACQUIRE);
#else
    return *refcount;
#endif
}

/* stm_ref_* are lock-free with MSVC Interlocked or compiler atomic builtins;
   otherwise refcounts must be adjusted under the registry mutex. */
#if defined(MSVC) || defined(HAVE_ATOMIC_BUILTIN)
#define STM_REFCOUNT_IS_ATOMIC 1
#else
#define STM_REFCOUNT_IS_ATOMIC 0
#endif

/* Type-generic loads/stores on runner state: acquire/release atomics where
   available, plain accesses (guarded by the fallback mutex) otherwise. */
#if defined(HAVE_ATOMIC_BUILTIN)
#define STM_LOAD(p)     __atomic_load_n((p), __ATOMIC_ACQUIRE)
#define STM_STORE(p, v) __atomic_store_n((p), (v), __ATOMIC_RELEASE)
#else
#define STM_LOAD(p)     (*(p))
#define STM_STORE(p, v) (*(p) = (v))
#endif

static void stm_increment_state_version(uint32_t *value) {
#if defined(HAVE_ATOMIC_BUILTIN)
    __atomic_add_fetch(value, 1U, __ATOMIC_SEQ_CST);
#else
    ++(*value);
#endif
}

static void stm_state_read_lock(CSOUND *csound, GRAPH_RUNNER *runner) {
#if !defined(HAVE_ATOMIC_BUILTIN)
    if (runner->state_mutex != NULL) csound->LockMutex(runner->state_mutex);
#else
    (void) csound;
    (void) runner;
#endif
}

static void stm_state_read_unlock(CSOUND *csound, GRAPH_RUNNER *runner) {
#if !defined(HAVE_ATOMIC_BUILTIN)
    if (runner->state_mutex != NULL) csound->UnlockMutex(runner->state_mutex);
#else
    (void) csound;
    (void) runner;
#endif
}

static uint32_t stm_state_read_begin(GRAPH_RUNNER *runner) {
    uint32_t version;
    do {
        version = STM_LOAD(&runner->state_version);
    } while ((version & 1U) != 0U);
    return version;
}

static int32_t stm_state_read_retry(GRAPH_RUNNER *runner, uint32_t version) {
#if defined(HAVE_ATOMIC_BUILTIN)
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
#endif
    return STM_LOAD(&runner->state_version) != version;
}

static uint32_t stm_snapshot_u32(CSOUND *csound, GRAPH_RUNNER *runner, const uint32_t *value) {
    uint32_t snapshot;
    stm_state_read_lock(csound, runner);
    snapshot = STM_LOAD(value);
    stm_state_read_unlock(csound, runner);
    return snapshot;
}

static uint64_t stm_snapshot_u64(CSOUND *csound, GRAPH_RUNNER *runner, const uint64_t *value) {
    uint64_t snapshot;
    stm_state_read_lock(csound, runner);
    snapshot = STM_LOAD(value);
    stm_state_read_unlock(csound, runner);
    return snapshot;
}

/* Coherent values exposed to opcode implementations. The seqlock retry
   protocol remains private to the snapshot helpers below. */

static STM_LATEST_EVENT_SNAPSHOT stm_latest_event_snapshot(CSOUND *csound, GRAPH_RUNNER *runner) {
    STM_LATEST_EVENT_SNAPSHOT snapshot;
    uint32_t version;

    stm_state_read_lock(csound, runner);
    do {
        version = stm_state_read_begin(runner);
        snapshot.sequence = STM_LOAD(&runner->node_event_seq);
        snapshot.entered_node = STM_LOAD(&runner->event_entered_node);
        snapshot.exited_node = STM_LOAD(&runner->event_exited_node);
    } while (stm_state_read_retry(runner, version));
    stm_state_read_unlock(csound, runner);

    return snapshot;
}

static STM_NODE_TIME_SNAPSHOT stm_node_time_snapshot(CSOUND *csound, GRAPH_RUNNER *runner) {
    STM_NODE_TIME_SNAPSHOT snapshot;
    uint32_t version;

    stm_state_read_lock(csound, runner);
    do {
        version = stm_state_read_begin(runner);
        snapshot.total_frames = STM_LOAD(&runner->total_sample_frames);
        snapshot.node_enter = STM_LOAD(&runner->node_sample_on_enter);
    } while (stm_state_read_retry(runner, version));
    stm_state_read_unlock(csound, runner);

    return snapshot;
}

static STM_TRANSITION_SNAPSHOT stm_transition_snapshot(CSOUND *csound, GRAPH_RUNNER *runner, uint64_t requested_sequence) {
    STM_TRANSITION_SNAPSHOT snapshot = { 0 };
    uint32_t version;

    stm_state_read_lock(csound, runner);
    do {
        uint32_t transition_count;
        uint32_t write_index;
        uint64_t latest_sequence;
        uint64_t next_sequence = requested_sequence;

        version = stm_state_read_begin(runner);
        transition_count = STM_LOAD(&runner->transition_count);
        write_index = STM_LOAD(&runner->tndx_write);
        latest_sequence = STM_LOAD(&runner->event_seq);
        snapshot.available = 0;
        snapshot.overflow = 0;
        snapshot.oldest_sequence = 0;

        if (transition_count != 0 && next_sequence <= latest_sequence) {
            uint32_t oldest_index = (write_index + TRANSITION_BUFFER_CAPACITY - transition_count) % TRANSITION_BUFFER_CAPACITY;
            snapshot.oldest_sequence = STM_LOAD(&runner->transitions[oldest_index].sequence);

            if (next_sequence < snapshot.oldest_sequence) {
                snapshot.overflow = 1;
                next_sequence = snapshot.oldest_sequence;
            }

            uint64_t offset = next_sequence - snapshot.oldest_sequence;
            if (offset < transition_count) {
                uint32_t index = (oldest_index + (uint32_t) offset) % TRANSITION_BUFFER_CAPACITY;
                GRAPH_TRANSITION_EVENT *event = &runner->transitions[index];
                snapshot.event.sequence = STM_LOAD(&event->sequence);
                snapshot.event.from = STM_LOAD(&event->from);
                snapshot.event.to = STM_LOAD(&event->to);
                snapshot.event.status = STM_LOAD(&event->status);
                snapshot.available = 1;
            }
        }
    } while (stm_state_read_retry(runner, version));
    stm_state_read_unlock(csound, runner);

    return snapshot;
}

static void stm_runner_update_begin(CSOUND *csound, GRAPH_RUNNER *runner) {
    stm_state_read_lock(csound, runner);
    stm_increment_state_version(&runner->state_version);
}

static void stm_runner_update_end(CSOUND *csound, GRAPH_RUNNER *runner) {
    stm_increment_state_version(&runner->state_version);
    stm_state_read_unlock(csound, runner);
}

static void stm_definition_release(CSOUND *csound, GRAPH_DEFINITION *definition) {
    if (definition == NULL) return;
    if (stm_ref_decrement(&definition->refcount) == 0) {
        stm_free_nodes(csound, definition->nodes, definition->node_count);
        csound->Free(csound, definition);
    }
}

static void stm_runner_release(CSOUND *csound, GRAPH_RUNNER *runner) {
    if (runner == NULL) return;
    if (stm_ref_decrement(&runner->refcount) == 0) {
        if (runner->transitions != NULL) {
            csound->Free(csound, runner->transitions);
        }
        if (runner->checkpoints != NULL) {
            csound->Free(csound, runner->checkpoints);
        }
#if !defined(HAVE_ATOMIC_BUILTIN)
        if (runner->state_mutex != NULL) {
            csound->DestroyMutex(runner->state_mutex);
        }
#endif
        stm_definition_release(csound, runner->definition);
        csound->Free(csound, runner);
    }
}

static void stm_destroy_object(CSOUND *csound, STM_OBJECT_TYPE type, STM_OBJECT_POINTER object) {
    switch (type) {
        case STM_OBJECT_BUILDER:
            if (object.builder != NULL) {
                stm_free_nodes(csound, object.builder->nodes, object.builder->node_count);
                csound->Free(csound, object.builder);
            }
            break;
        case STM_OBJECT_DEFINITION:
            stm_definition_release(csound, object.definition);
            break;
        case STM_OBJECT_RUNNER:
            stm_runner_release(csound, object.runner);
            break;
        case STM_OBJECT_NONE:
        default:
            break;
    }
}

static int32_t stm_registry_reset(CSOUND *csound, void *userData) {
    STM_REGISTRY *reg = (STM_REGISTRY *) userData;
    if (reg == NULL) reg = stm_registry_query(csound);
    if (reg == NULL) return OK;

    if (reg->slots != NULL) {
        for (uint32_t i = 0; i < reg->count; i++) {
            STM_REGISTRY_SLOT *slot = &reg->slots[i];
            stm_destroy_object(csound, slot->type, slot->object);
            slot->object.ptr = NULL;
            slot->type = STM_OBJECT_NONE;
        }
        csound->Free(csound, reg->slots);
        reg->slots = NULL;
    }
    if (reg->mutex != NULL) {
        csound->DestroyMutex(reg->mutex);
        reg->mutex = NULL;
    }
    reg->count = 0;
    reg->capacity = 0;
    csound->DestroyGlobalVariable(csound, STM_REGISTRY_NAME);
    return OK;
}

static uint32_t stm_make_handle(uint32_t slot, uint32_t generation) {
    if (slot >= STM_HANDLE_SLOT_BASE || generation == 0 ||
            generation > STM_HANDLE_GENERATION_MAX) {
        return 0;
    }
    return (generation - 1) * STM_HANDLE_SLOT_BASE + slot + 1;
}

static int32_t stm_decode_handle(MYFLT handle, uint32_t *slot, uint32_t *generation) {
    if (!(handle >= FL(1.0)) || handle > (MYFLT) STM_HANDLE_MAX_EXACT) return 0;

    uint32_t raw = (uint32_t) handle;
    if ((MYFLT) raw != handle) return 0;

    uint32_t value = raw - 1;
    *slot = value % STM_HANDLE_SLOT_BASE;
    *generation = (value / STM_HANDLE_SLOT_BASE) + 1;
    return 1;
}

static int32_t stm_myflt_to_uint32(MYFLT value, uint32_t max_exclusive, uint32_t *out) {
    /* Check range before casting: out-of-range float-to-int conversion is
       undefined, and the negated >= rejects NaN. */
    if (!(value >= FL(0.0)) || value >= (MYFLT) max_exclusive) return 0;

    uint32_t converted = (uint32_t) value;
    if ((MYFLT) converted != value) return 0;

    *out = converted;
    return 1;
}

/* Create the shared typed registry on first use. */
static STM_REGISTRY *stm_registry_get_or_create(CSOUND *csound) {
    STM_REGISTRY *reg = stm_registry_query(csound);
    if (reg == NULL) {
        if (csound->CreateGlobalVariable(csound, STM_REGISTRY_NAME, sizeof(STM_REGISTRY)) != 0) {
            return NULL;
        }
        reg = stm_registry_query(csound);
        if (reg == NULL) {
            csound->DestroyGlobalVariable(csound, STM_REGISTRY_NAME);
            return NULL;
        }
        reg->capacity = INITIAL_GRAPH_CAPACITY;
        reg->count = 0;
        reg->slots = csound->Calloc(csound, sizeof(STM_REGISTRY_SLOT) * reg->capacity);
        if (reg->slots == NULL) {
            csound->DestroyGlobalVariable(csound, STM_REGISTRY_NAME);
            return NULL;
        }
        reg->mutex = csound->Create_Mutex(0);
        if (reg->mutex == NULL) {
            csound->Free(csound, reg->slots);
            reg->slots = NULL;
            csound->DestroyGlobalVariable(csound, STM_REGISTRY_NAME);
            return NULL;
        }
        if (csound->RegisterResetCallback(csound, (void *) reg,
                stm_registry_reset) != 0) {
            stm_registry_reset(csound, (void *) reg);
            return NULL;
        }
    }
    return reg;
}

static uint32_t stm_register_object(CSOUND *csound, STM_OBJECT_TYPE type, STM_OBJECT_POINTER object, STM_OWNER_TOKEN *owner) {
    STM_REGISTRY *reg = stm_registry_get_or_create(csound);
    if (reg == NULL || type == STM_OBJECT_NONE || object.ptr == NULL) return 0;

    stm_registry_lock(csound, reg);

    /* reuse free slot; generation 0 means the slot exhausted its handle space */
    for (uint32_t i = 0; i < reg->count; i++) {
        STM_REGISTRY_SLOT *slot = &reg->slots[i];
        if (slot->type == STM_OBJECT_NONE && slot->object.ptr == NULL && slot->generation > 0 && slot->generation <= STM_HANDLE_GENERATION_MAX) {
            slot->object = object;
            slot->type = type;
            owner->slot = i;
            owner->generation = slot->generation;
            owner->type = type;
            uint32_t handle = stm_make_handle(i, slot->generation);
            stm_registry_unlock(csound, reg);
            return handle;
        }
    }

    if (reg->count == reg->capacity) {
        if (reg->capacity >= STM_HANDLE_SLOT_BASE) {
            stm_registry_unlock(csound, reg);
            return 0;
        }
        uint32_t newcap = reg->capacity * 2;
        if (newcap > STM_HANDLE_SLOT_BASE) newcap = STM_HANDLE_SLOT_BASE;
        STM_REGISTRY_SLOT *grown = csound->ReAlloc(csound, reg->slots, sizeof(STM_REGISTRY_SLOT) * newcap);
        if (grown == NULL) {
            stm_registry_unlock(csound, reg);
            return 0;
        }
        reg->slots = grown;
        reg->capacity = newcap;
    }

    uint32_t slot = reg->count++;
    reg->slots[slot].object = object;
    reg->slots[slot].generation = 1;
    reg->slots[slot].type = type;
    owner->slot = slot;
    owner->generation = 1;
    owner->type = type;
    uint32_t handle = stm_make_handle(slot, 1);
    stm_registry_unlock(csound, reg);
    return handle;
}

static void *stm_handle_to_object_locked(STM_REGISTRY *reg, MYFLT handle, STM_OBJECT_TYPE expected_type) {
    uint32_t slot;
    uint32_t generation;
    if (!stm_decode_handle(handle, &slot, &generation)) return NULL;
    if (slot >= reg->count) return NULL;
    STM_REGISTRY_SLOT *entry = &reg->slots[slot];
    if (entry->generation != generation || entry->type != expected_type || entry->object.ptr == NULL) {
        return NULL;
    }

    return entry->object.ptr;
}

static int32_t stm_release_owner(CSOUND *csound, STM_OWNER_TOKEN *owner) {
    if (owner->type == STM_OBJECT_NONE) return OK;
    STM_REGISTRY *reg = stm_registry_query(csound);
    if (reg == NULL) {
        owner->type = STM_OBJECT_NONE;
        return OK;
    }

    stm_registry_lock(csound, reg);
    if (owner->slot < reg->count) {
        STM_REGISTRY_SLOT *slot = &reg->slots[owner->slot];
        if (slot->generation == owner->generation && slot->type == owner->type && slot->object.ptr != NULL) {
            STM_OBJECT_POINTER object = slot->object;
            STM_OBJECT_TYPE type = slot->type;
            slot->object.ptr = NULL;
            slot->type = STM_OBJECT_NONE;
            if (slot->generation >= STM_HANDLE_GENERATION_MAX) {
                slot->generation = 0;
            } else {
                slot->generation++;
            }
            stm_destroy_object(csound, type, object);
        }
    }
    stm_registry_unlock(csound, reg);
    owner->slot = 0;
    owner->generation = 0;
    owner->type = STM_OBJECT_NONE;
    return OK;
}

static int32_t stm_get_object_init_locked(CSOUND *csound, OPDS *opds, MYFLT handle, STM_OBJECT_TYPE expected_type, const char *msg, STM_REGISTRY **reg_out, void **object_out) {
    STM_REGISTRY *reg = stm_registry_query(csound);
    if (reg == NULL) {
        return csound->InitError(csound, "%s", msg);
    }

    stm_registry_lock(csound, reg);
    void *object = stm_handle_to_object_locked(reg, handle, expected_type);
    if (object == NULL) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "%s", msg);
    }

    (void) opds;
    *reg_out = reg;
    *object_out = object;
    return OK;
}

static int32_t stm_runner_ref_deinit(CSOUND *csound, OPDS *opds, STM_RUNNER_REF *ref);

static INSDS *stm_top_level_instrument(OPDS *opds) {
    INSDS *owner = opds->insdshead;
    while (owner != NULL && owner->opcod_iobufs != NULL) {
        owner = ((OPCOD_IOBUFS *) owner->opcod_iobufs)->parent_ip;
    }
    return owner;
}

static int32_t stm_runner_ref_init(CSOUND *csound, OPDS *opds, MYFLT handle, STM_RUNNER_ACCESS access, const char *msg, STM_RUNNER_REF *ref) {
    /* Csound may rerun an init callback without invoking deinit first. */
    if (ref->runner != NULL) {
        stm_runner_ref_deinit(csound, opds, ref);
    }

    STM_REGISTRY *reg;
    GRAPH_RUNNER *runner;
    if (STM_GET_RUNNER_INIT(csound, opds, handle, msg, &reg, &runner) != OK) {
        return NOTOK;
    }

    if (access == STM_RUNNER_WRITER) {
        INSDS *owner = stm_top_level_instrument(opds);
        if (runner->writer_owner != NULL && runner->writer_owner != owner) {
            stm_registry_unlock(csound, reg);
            return csound->InitError(csound, "[stm] runner already has another active writer");
        }
        if (runner->writer_claims == UINT32_MAX) {
            stm_registry_unlock(csound, reg);
            return csound->InitError(csound, "[stm] runner writer claim limit reached");
        }
        runner->writer_owner = owner;
        runner->writer_claims++;
        ref->writer_owner = owner;
        ref->writer_claimed = 1;
    } else {
        ref->writer_owner = NULL;
        ref->writer_claimed = 0;
    }
    if (stm_ref_load(&runner->refcount) == UINT32_MAX) {
        if (access == STM_RUNNER_WRITER) {
            runner->writer_claims--;
            if (runner->writer_claims == 0) runner->writer_owner = NULL;
            ref->writer_owner = NULL;
            ref->writer_claimed = 0;
        }
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] runner reference limit reached");
    }
    stm_ref_increment(&runner->refcount);
    ref->runner = runner;
    stm_registry_unlock(csound, reg);
    return OK;
}

static int32_t stm_runner_ref_deinit(CSOUND *csound, OPDS *opds, STM_RUNNER_REF *ref) {
    (void) opds;
    GRAPH_RUNNER *runner = ref->runner;
    if (runner == NULL) return OK;

    STM_REGISTRY *reg = stm_registry_query(csound);
    stm_registry_lock(csound, reg);
    if (ref->writer_claimed && runner->writer_owner == ref->writer_owner && runner->writer_claims > 0) {
        runner->writer_claims--;
        if (runner->writer_claims == 0) runner->writer_owner = NULL;
    }
    if (!STM_REFCOUNT_IS_ATOMIC && reg != NULL) {
        stm_runner_release(csound, runner);
    }
    stm_registry_unlock(csound, reg);

    ref->runner = NULL;
    ref->writer_owner = NULL;
    ref->writer_claimed = 0;
    /* with a non-atomic refcount the release above already ran under the
       registry mutex; without a registry there is no mutex to take anyway */
    if (STM_REFCOUNT_IS_ATOMIC || reg == NULL) {
        stm_runner_release(csound, runner);
    }
    return OK;
}

/* ------------------------------------------------------------------ */
/* graph helpers                                                      */
/* ------------------------------------------------------------------ */

static int32_t graph_find_node(const GRAPH_NODE *nodes, uint32_t node_count, const char *name) {
    for (uint32_t i = 0; i < node_count; i++) {
        if (strcmp(nodes[i].name, name) == 0)
            return (int32_t) i;
    }
    return -1;
}

/* returns 1 if edge from n to t exists, 0 otherwise */
static int32_t graph_has_edge(GRAPH_NODE *n, uint32_t t) {
    for (uint32_t i = 0; i < n->edge_count; i++) {
        if (n->edges[i] == t) return 1;
    }
    return 0;
}

static int32_t add_edge_helper(CSOUND *csound, GRAPH_BUILDER *builder, const char *from_node_name, const char *to_node_name) {
    int32_t from = graph_find_node(builder->nodes, builder->node_count, from_node_name);
    int32_t to = graph_find_node(builder->nodes, builder->node_count, to_node_name);

    if (from < 0 || to < 0) { return NOTOK; }

    GRAPH_NODE *from_node = &builder->nodes[from];

    // ignore duplicate edges
    if (graph_has_edge(from_node, (uint32_t) to)) return OK;

    if (from_node->edge_count == from_node->edge_capacity) {
        uint32_t newcap = from_node->edge_capacity * 2;
        uint32_t *new_edges = csound->ReAlloc(csound, from_node->edges, sizeof(uint32_t) * newcap);

        if (new_edges == NULL) { return NOTOK; }

        from_node->edges = new_edges;
        from_node->edge_capacity = newcap;
    }

    from_node->edges[from_node->edge_count] = (uint32_t) to;
    from_node->edge_count++;

    return OK;
}

/* Keep the first pending target. Repeating it is idempotent; requesting a
   different target before stmadvance is a conflict and applies neither. */
static void graph_request_node(GRAPH_RUNNER *g, uint32_t target) {
    if (g->requested_node == STM_NO_NODE) {
        g->requested_node = target;
        g->request_conflict = STM_REQUEST_OK;
    } else if (g->requested_node != target) {
        g->request_conflict = STM_REQUEST_CONFLICT;
    }
}

/* ------------------------------------------------------------------ */
/* opcodes                                                            */
/* ------------------------------------------------------------------ */

int32_t graph_create_deinit(CSOUND *csound, GRAPH_CREATE *p) {
    *p->handle = FL(0.0);
    return stm_release_owner(csound, &p->owner);
}

int32_t graph_create(CSOUND *csound, GRAPH_CREATE *p) {
    /* Csound may rerun init without deinit; drop any previous object first. */
    int32_t rc = graph_create_deinit(csound, p);
    if (rc != OK) {
        return rc;
    }

    GRAPH_BUILDER *builder = csound->Calloc(csound, sizeof(GRAPH_BUILDER));
    if (builder == NULL) {
        return csound->InitError(csound, "[stm] stmcreate: builder memory error");
    }

    builder->node_capacity = INITIAL_NODE_CAPACITY;
    builder->nodes = csound->Calloc(csound, sizeof(GRAPH_NODE) * builder->node_capacity);
    if (builder->nodes == NULL) {
        csound->Free(csound, builder);
        return csound->InitError(csound, "[stm] stmcreate: builder memory error");
    }

    builder->start_node = STM_NO_NODE;
    STM_OBJECT_POINTER object = { .builder = builder };
    uint32_t handle = stm_register_object(csound, STM_OBJECT_BUILDER, object, &p->owner);
    if (handle == 0) {
        stm_free_nodes(csound, builder->nodes, builder->node_count);
        csound->Free(csound, builder);
        return csound->InitError(csound, "[stm] stmcreate: registry error");
    }

    *p->handle = (MYFLT) handle;
    return OK;
}

int32_t graph_add_node(CSOUND *csound, GRAPH_ADD_NODE *p) {
    STM_REGISTRY *reg;
    GRAPH_BUILDER *builder;
    if (STM_GET_BUILDER_INIT(csound, &(p->h), *p->handle, "[stm] stmaddnode: invalid builder", &reg, &builder) != OK) {
        return NOTOK;
    }

    if (builder->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddnode: builder already compiled (immutable)");
    }

    if (graph_find_node(builder->nodes, builder->node_count, p->node_name->data) >= 0) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddnode: duplicate node name '%s'", p->node_name->data);
    }

    if (builder->node_count == builder->node_capacity) {
        uint32_t newcap = builder->node_capacity * 2;
        GRAPH_NODE *grown = csound->ReAlloc(csound, builder->nodes, sizeof(GRAPH_NODE) * newcap);
        if (grown == NULL) {
            stm_registry_unlock(csound, reg);
            return csound->InitError(csound, "[stm] stmaddnode: memory error");
        }
        builder->nodes = grown;
        builder->node_capacity = newcap;
    }
    uint32_t id = builder->node_count;

    GRAPH_NODE *node = &builder->nodes[id];
    memset(node, 0, sizeof(GRAPH_NODE));

    node->id = id;
    size_t namelen = strlen(p->node_name->data);
    node->name = csound->Calloc(csound, namelen + 1);
    if (node->name == NULL) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddnode: memory error");
    }
    memcpy(node->name, p->node_name->data, namelen);

    node->edge_count = 0;
    node->edge_capacity = INITIAL_EDGE_CAPACITY;
    node->edges = csound->Calloc(csound, sizeof(uint32_t) * node->edge_capacity);
    if (node->edges == NULL) {
        csound->Free(csound, node->name);
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddnode: memory error");
    }

    builder->node_count++;
    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_add_edge(CSOUND *csound, GRAPH_ADD_EDGE *p) {
    STM_REGISTRY *reg;
    GRAPH_BUILDER *builder;
    if (STM_GET_BUILDER_INIT(csound, &(p->h), *p->handle, "[stm] stmaddedge: invalid builder", &reg, &builder) != OK) {
        return NOTOK;
    }

    if (builder->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddedge: graph already compiled (immutable)");
    }

    if (add_edge_helper(csound, builder, p->from->data, p->to->data) == NOTOK) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddedge, something went wrong");
    }

    stm_registry_unlock(csound, reg);
    return OK;
}

static GRAPH_DEFINITION *graph_copy_definition(CSOUND *csound, const GRAPH_BUILDER *builder) {
    GRAPH_DEFINITION *definition = csound->Calloc(csound, sizeof(GRAPH_DEFINITION));
    if (definition == NULL) return NULL;

    definition->node_count = builder->node_count;
    definition->start_node = builder->start_node == STM_NO_NODE ? 0 : builder->start_node;
    definition->refcount = 1;
    definition->nodes = csound->Calloc(csound, sizeof(GRAPH_NODE) * definition->node_count);
    if (definition->nodes == NULL) {
        csound->Free(csound, definition);
        return NULL;
    }

    for (uint32_t i = 0; i < definition->node_count; i++) {
        const GRAPH_NODE *source = &builder->nodes[i];
        GRAPH_NODE *target = &definition->nodes[i];
        target->id = source->id;
        target->edge_count = source->edge_count;
        target->edge_capacity = source->edge_count;

        size_t name_size = strlen(source->name) + 1;
        target->name = csound->Malloc(csound, name_size);
        if (target->name == NULL) goto memory_error;
        memcpy(target->name, source->name, name_size);

        if (source->edge_count > 0) {
            target->edges = csound->Malloc(csound, sizeof(uint32_t) * source->edge_count);
            if (target->edges == NULL) goto memory_error;
            memcpy(target->edges, source->edges, sizeof(uint32_t) * source->edge_count);
        }
    }
    return definition;

memory_error:
    stm_free_nodes(csound, definition->nodes, definition->node_count);
    csound->Free(csound, definition);
    return NULL;
}

int32_t graph_compile_deinit(CSOUND *csound, GRAPH_COMPILE *p) {
    *p->def_handle = FL(0.0);
    return stm_release_owner(csound, &p->owner);
}

int32_t graph_compile(CSOUND *csound, GRAPH_COMPILE *p) {
    int32_t rc = graph_compile_deinit(csound, p);
    if (rc != OK) return rc;

    STM_REGISTRY *reg;
    GRAPH_BUILDER *builder;

    if (STM_GET_BUILDER_INIT(csound, &(p->h), *p->bld_handle, "[stm] stmcompile: invalid builder", &reg, &builder) != OK) {
        return NOTOK;
    }

    if (builder->compiled) {
        return csound->InitError(csound, "[stm] stmcompile: graph already compiled");
    }

    if (builder->node_count == 0) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmcompile: graph has no nodes");
    }

    for (uint32_t i = 0; i < builder->node_count; i++) {
        GRAPH_NODE *n = &builder->nodes[i];
        for (uint32_t e = 0; e < n->edge_count; e++) {
            if (n->edges[e] >= builder->node_count) {
                stm_registry_unlock(csound, reg);
                return csound->InitError(csound, "[stm] stmcompile: invalid edge");
            }
        }
    }

    GRAPH_DEFINITION *definition = graph_copy_definition(csound, builder);
    if (definition == NULL) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmcompile: definition memory error");
    }
    builder->compiled = 1;

    stm_registry_unlock(csound, reg);

    STM_OBJECT_POINTER object = { .definition = definition };
    uint32_t handle = stm_register_object(csound, STM_OBJECT_DEFINITION, object, &p->owner);
    if (handle == 0) {
        stm_definition_release(csound, definition);
        return csound->InitError(csound, "[stm] stmcompile: registry error");
    }
    *p->def_handle = (MYFLT) handle;
    return OK;
}

int32_t graph_instance_deinit(CSOUND *csound, GRAPH_INSTANCE *p) {
    *p->runner_handle = FL(0.0);
    return stm_release_owner(csound, &p->owner);
}

int32_t graph_instance(CSOUND *csound, GRAPH_INSTANCE *p) {
    int32_t rc = graph_instance_deinit(csound, p);
    if (rc != OK) return rc;

    STM_REGISTRY *reg;
    GRAPH_DEFINITION *definition;
    if (STM_GET_DEFINITION_INIT(csound, &(p->h), *p->def_handle, "[stm] stminstance: invalid definition", &reg, &definition) != OK) {
        return NOTOK;
    }
    if (stm_ref_load(&definition->refcount) == UINT32_MAX) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stminstance: definition reference limit reached");
    }

    GRAPH_RUNNER *runner = csound->Calloc(csound, sizeof(GRAPH_RUNNER));
    if (runner == NULL) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stminstance: runner memory error");
    }
    runner->transitions = csound->Calloc(csound, sizeof(GRAPH_TRANSITION_EVENT) * TRANSITION_BUFFER_CAPACITY);
    if (runner->transitions == NULL) {
        csound->Free(csound, runner);
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stminstance: runner memory error");
    }

    runner->checkpoints = csound->Calloc(csound, sizeof(STM_CHECKPOINT) * CHECKPOINT_BUFFER_CAPACITY);
    if (runner->checkpoints == NULL) {
        csound->Free(csound, runner->transitions);
        csound->Free(csound, runner);
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stminstance: runner memory error");
    }

#if !defined(HAVE_ATOMIC_BUILTIN)
    runner->state_mutex = csound->Create_Mutex(0);
    if (runner->state_mutex == NULL) {
        csound->Free(csound, runner->transitions);
        csound->Free(csound, runner->checkpoints);
        csound->Free(csound, runner);
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stminstance: runner mutex error");
    }
#endif

    stm_ref_increment(&definition->refcount);
    runner->definition = definition;
    runner->refcount = 1;
    runner->current_node = definition->start_node;
    runner->previous_node = STM_NO_NODE;
    runner->requested_node = STM_NO_NODE;
    runner->request_conflict = STM_REQUEST_OK;
    runner->event_seq = 1;
    runner->node_event_seq = 1;
    runner->event_entered_node = runner->current_node;
    runner->event_exited_node = STM_NO_NODE;
    runner->run_state = STM_RUNNER_RUNNING;
    stm_registry_unlock(csound, reg);

    STM_OBJECT_POINTER object = { .runner = runner };
    uint32_t handle = stm_register_object(csound, STM_OBJECT_RUNNER, object, &p->owner);
    if (handle == 0) {
        stm_destroy_object(csound, STM_OBJECT_RUNNER, object);
        return csound->InitError(csound, "[stm] stminstance: registry error");
    }

    *p->runner_handle = (MYFLT) handle;
    return OK;
}

static void record_gevent(GRAPH_RUNNER *g, uint32_t from, uint32_t to, int32_t status, int32_t is_transition) {
    uint32_t write_index = STM_LOAD(&g->tndx_write);
    uint32_t count       = STM_LOAD(&g->transition_count);
    uint64_t sequence    = STM_LOAD(&g->event_seq) + 1U;

    GRAPH_TRANSITION_EVENT *event = &g->transitions[write_index];

    STM_STORE(&event->from, from);
    STM_STORE(&event->to, to);
    STM_STORE(&event->status, status);
    STM_STORE(&event->sequence, sequence);

    STM_STORE(&g->tndx_write, (write_index + 1U) % TRANSITION_BUFFER_CAPACITY);
    if (count < TRANSITION_BUFFER_CAPACITY) {
        STM_STORE(&g->transition_count, count + 1U);
    }

    if (is_transition) {
        STM_STORE(&g->event_exited_node, from);
        STM_STORE(&g->event_entered_node, to);
        STM_STORE(&g->node_event_seq, sequence);
    }

    STM_STORE(&g->event_seq, sequence);
}

static int32_t record_gcheckpoint(GRAPH_RUNNER *g, const char *name) {
    STM_CHECKPOINT *checkpoints = g->checkpoints;

    if (name == NULL || name[0] == '\0') return NOTOK;
    if (strlen(name) >= sizeof(checkpoints[0].name)) return NOTOK;

    for (uint32_t i = 0; i < CHECKPOINT_BUFFER_CAPACITY; ++i) {
        if (!checkpoints[i].is_valid) continue;
        if (strcmp(name, checkpoints[i].name) == 0) return NOTOK;
    }

    STM_CHECKPOINT *gc = &checkpoints[g->cndx_write];
    int32_t was_valid = gc->is_valid;

    if (!was_valid) g->ckp_count++;

    snprintf(gc->name, sizeof(gc->name), "%s", name);

    gc->current_node         = STM_LOAD(&g->current_node);
    gc->previous_node        = STM_LOAD(&g->previous_node);
    gc->requested_node       = STM_LOAD(&g->requested_node);
    gc->request_conflict     = STM_LOAD(&g->request_conflict);
    gc->graph_tick           = STM_LOAD(&g->graph_tick);
    gc->total_sample_frames  = STM_LOAD(&g->total_sample_frames);
    gc->node_sample_on_enter = STM_LOAD(&g->node_sample_on_enter);

    gc->is_valid = 1;
    g->cndx_write = (g->cndx_write + 1U) % CHECKPOINT_BUFFER_CAPACITY;

    return OK;
}

static int32_t resume_gcheckpoint(GRAPH_RUNNER *g, const char *name) {
    STM_CHECKPOINT *checkpoints = g->checkpoints;

    if (name == NULL || name[0] == '\0') return NOTOK;
    if (strlen(name) >= sizeof(checkpoints[0].name)) return NOTOK;

    int32_t cndx = -1;
    for (int32_t i = 0; i < CHECKPOINT_BUFFER_CAPACITY; ++i) {
        if (!checkpoints[i].is_valid) continue;
        if (strcmp(name, checkpoints[i].name) == 0) {
            cndx = i;
            break;
        }
    }

    if (cndx < 0) return NOTOK;

    STM_CHECKPOINT *gc = &checkpoints[cndx];

    uint32_t old_node = STM_LOAD(&g->current_node);

    STM_STORE(&g->current_node, gc->current_node);
    STM_STORE(&g->previous_node, gc->previous_node);
    STM_STORE(&g->requested_node, gc->requested_node);
    STM_STORE(&g->request_conflict, gc->request_conflict);
    STM_STORE(&g->graph_tick, gc->graph_tick);
    STM_STORE(&g->total_sample_frames, gc->total_sample_frames);
    STM_STORE(&g->node_sample_on_enter, gc->node_sample_on_enter);

    /* A reset cycle is tied to an absolute engine k-counter and cannot be
       resumed later.  Resume is instead published as a new, monotonic event;
       the transition ring and its reader cursors remain valid. */
    STM_STORE(&g->reset_pcycle, 0);
    STM_STORE(&g->has_reset_cycle, 0);
    record_gevent(g, old_node, gc->current_node, STM_EVENT_RECALL, 1);

    return OK;
}

/* Every k-rate runner opcode resolves and retains its runner at init time and
   drops it at deinit; generate those identical thin wrappers. */
#define STM_RUNNER_REF_OPCODE(fname, TYPE, ACCESS, MSG)                              \
    static int32_t fname##_init(CSOUND *csound, TYPE *p) {                           \
        return stm_runner_ref_init(csound, &p->h, *p->handle, ACCESS, MSG, &p->ref); \
    }                                                                                \
    static int32_t fname##_deinit(CSOUND *csound, TYPE *p) {                         \
        return stm_runner_ref_deinit(csound, &p->h, &p->ref);                        \
    }

STM_RUNNER_REF_OPCODE(graph_current,    GRAPH_CURRENT,      STM_RUNNER_READER, "[stm] stmcurrent: invalid runner")
STM_RUNNER_REF_OPCODE(graph_current_id, GRAPH_RUNNER_QUERY, STM_RUNNER_READER, "[stm] stmcurrentid: invalid runner")
STM_RUNNER_REF_OPCODE(graph_advance,    GRAPH_ADVANCE,      STM_RUNNER_WRITER, "[stm] stmadvance: invalid runner")
STM_RUNNER_REF_OPCODE(graph_next,       GRAPH_NEXT,         STM_RUNNER_WRITER, "[stm] stmnext: invalid runner")
STM_RUNNER_REF_OPCODE(graph_next_id,    GRAPH_NEXT_ID,      STM_RUNNER_WRITER, "[stm] stmnextid: invalid runner")
STM_RUNNER_REF_OPCODE(graph_node_id,    GRAPH_NODE_ID,      STM_RUNNER_READER, "[stm] stmnodeid: invalid runner")
STM_RUNNER_REF_OPCODE(graph_node_name,  GRAPH_NODE_NAME,    STM_RUNNER_READER, "[stm] stmnodename: invalid runner")
STM_RUNNER_REF_OPCODE(graph_node_count, GRAPH_RUNNER_QUERY, STM_RUNNER_READER, "[stm] stmnodecount: invalid runner")
STM_RUNNER_REF_OPCODE(graph_edge_count, GRAPH_RUNNER_QUERY, STM_RUNNER_READER, "[stm] stmedgecount: invalid runner")
STM_RUNNER_REF_OPCODE(graph_reset,      GRAPH_RESET,        STM_RUNNER_WRITER, "[stm] stmreset: invalid runner")
STM_RUNNER_REF_OPCODE(graph_time,       GRAPH_RUNNER_QUERY, STM_RUNNER_READER, "[stm] invalid runner for time query")

/* return the current node name (for orchestra-side dispatch) */
int32_t graph_current(CSOUND *csound, GRAPH_CURRENT *p) {
    GRAPH_RUNNER *g = p->ref.runner;

    GRAPH_DEFINITION *definition = g->definition;
    uint32_t current_node = stm_snapshot_u32(csound, g, &g->current_node);
    if (current_node >= definition->node_count) {
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrent: invalid current node");
    }

    const char *name = definition->nodes[current_node].name;
    size_t len = strlen(name);
    if (len >= p->cur->size) {
        void *newp = csound->ReAlloc(csound, p->cur->data, len + 1);
        if (newp == NULL) {
            return csound->PerfError(csound, &(p->h), "[stm] stmcurrent: memory error");
        }
        p->cur->data = newp;
        p->cur->size = len + 1;
    }
    memcpy(p->cur->data, name, len + 1);
    return OK;
}

/* return the current node id (for orchestra-side dispatch) */
int32_t graph_current_id(CSOUND *csound, GRAPH_RUNNER_QUERY *p) {
    GRAPH_RUNNER *g = p->ref.runner;
    uint32_t current_node = stm_snapshot_u32(csound, g, &g->current_node);
    if (current_node >= g->definition->node_count) {
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrentid: invalid current node");
    }

    *p->out = current_node;
    return OK;
}

/* Consume the pending request and report its outcome together with the source
   and target node IDs. A conflict applies no transition. */
int32_t graph_advance(CSOUND *csound, GRAPH_ADVANCE *p) {
    GRAPH_RUNNER *g = p->ref.runner;

    GRAPH_DEFINITION *definition = g->definition;
    uint32_t current_node = STM_LOAD(&g->current_node);
    if (current_node >= definition->node_count) {
        return csound->PerfError(csound, &(p->h), "[stm] stmadvance: invalid current node");
    }

    stm_runner_update_begin(csound, g);

    if (STM_LOAD(&g->run_state) == STM_RUNNER_PAUSED) {
        *p->status = STM_PAUSED;
        *p->id_from = (MYFLT) STM_LOAD(&g->current_node);
        *p->id_to = FL(-1.0);
        stm_runner_update_end(csound, g);
        return OK;
    }

    uint32_t source = STM_LOAD(&g->current_node);
    uint32_t target = g->requested_node;
    int32_t status = STM_NO_REQUEST;
    int32_t accepted = 0;

    *p->id_from = (MYFLT) source;
    *p->id_to = FL(-1.0);

    if (g->request_conflict) {
        status = STM_CONFLICT;
    } else if (target != STM_NO_NODE) {
        *p->id_to = (MYFLT) target;

        if (target >= definition->node_count) {
            g->requested_node = STM_NO_NODE;
            g->request_conflict = STM_REQUEST_OK;
            stm_runner_update_end(csound, g);
            return csound->PerfError(csound, &(p->h), "[stm] stmadvance: invalid requested node");
        }

        GRAPH_NODE *node = &definition->nodes[source];
        if (!graph_has_edge(node, target)) {
            status = STM_ILLEGAL_EDGE;
        } else {
            int32_t is_self = target == source;
            status = is_self ? STM_SELF_TRANSITION : STM_CHANGED;
            accepted = 1;
            STM_STORE(&g->previous_node, source);
            STM_STORE(&g->current_node, target);
            int32_t estatus = is_self ? STM_EVENT_SELF_TRANSITION : STM_EVENT_CHANGED;
            record_gevent(g, source, target, estatus, 1);
        }
    }

    g->requested_node = STM_NO_NODE;
    g->request_conflict = STM_REQUEST_OK;

    uint64_t curr_pcycle = csound->GetEngineKcounter(csound);
    int is_reset = g->has_reset_cycle && curr_pcycle == g->reset_pcycle;

    if (!is_reset) {
        uint64_t total_frames = STM_LOAD(&g->total_sample_frames) + (uint64_t) CS_KSMPS;
        STM_STORE(&g->graph_tick, STM_LOAD(&g->graph_tick) + 1U);
        STM_STORE(&g->total_sample_frames, total_frames);
        if (accepted) {
            STM_STORE(&g->node_sample_on_enter, total_frames);
        }
    }

    if (g->has_reset_cycle && curr_pcycle != g->reset_pcycle) {
        g->has_reset_cycle = 0;
    }

    *p->status = (MYFLT) status;
    stm_runner_update_end(csound, g);
    return OK;
}

int32_t graph_next(CSOUND *csound, GRAPH_NEXT *p) {
    GRAPH_RUNNER *g = p->ref.runner;

    if (STM_LOAD(&g->run_state) == STM_RUNNER_PAUSED) {
        return OK;
    }

    int32_t requested_node = graph_find_node(g->definition->nodes, g->definition->node_count, p->next_node->data);
    if (requested_node < 0) {
        return csound->PerfError(csound, &(p->h), "[stm] stmnext: node '%s' not found", p->next_node->data);
    }

    graph_request_node(g, (uint32_t) requested_node);
    return OK;
}

int32_t graph_next_id(CSOUND *csound, GRAPH_NEXT_ID *p) {
    GRAPH_RUNNER *g = p->ref.runner;

    if (STM_LOAD(&g->run_state) == STM_RUNNER_PAUSED) {
        return OK;
    }

    uint32_t requested_node;
    if (!stm_myflt_to_uint32(*p->next_node, g->definition->node_count, &requested_node)) {
        return csound->PerfError(csound, &(p->h), "[stm] stmnextid: invalid node id");
    }

    graph_request_node(g, requested_node);
    return OK;
}

int32_t graph_add_cond_edge(CSOUND *csound, GRAPH_ADD_COND_EDGE *p) {
    STM_REGISTRY *reg;
    GRAPH_BUILDER *builder;
    if (STM_GET_BUILDER_INIT(csound, &(p->h), *p->handle, "[stm] stmaddcondedge: invalid builder", &reg, &builder) != OK) {
        return NOTOK;
    }

    if (builder->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddcondedge: graph already compiled (immutable)");
    }

    STRINGDAT *items = (STRINGDAT *) p->targets->data;
    int32_t ntargets = p->targets->sizes[0];
    for (int32_t i = 0; i < ntargets; i++) {
        if (add_edge_helper(csound, builder, p->from->data, items[i].data) == NOTOK) {
            stm_registry_unlock(csound, reg);
            return csound->InitError(csound, "[stm] stmaddcondedge, something went wrong");
        }
    }

    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_on_ee_init(CSOUND *csound, GRAPH_ON_EE *p) {
    if (stm_runner_ref_init(csound, &p->h, *p->handle, STM_RUNNER_READER, "[stm] on enter/exit: invalid runner", &p->ref) != OK) {
        return NOTOK;
    }

    GRAPH_RUNNER *g = p->ref.runner;

    int32_t n = graph_find_node(g->definition->nodes, g->definition->node_count, p->node->data);
    if (n < 0) {
        stm_runner_ref_deinit(csound, &(p->h), &p->ref);
        return csound->InitError(csound, "[stm] on enter/exit : node '%s' not found", p->node->data);
    }

    p->node_id = n;
    *p->trig = FL(0.0);
    p->last_seen_event_seq = stm_snapshot_u64(csound, g, &g->node_event_seq);
    return OK;
}

static int32_t graph_on_ee_deinit(CSOUND *csound, GRAPH_ON_EE *p) {
    return stm_runner_ref_deinit(csound, &(p->h), &p->ref);
}

/* Runner-level enter event trigger: 1 only for a new transition event where
   this node became current. */
int32_t graph_on_enter(CSOUND *csound, GRAPH_ON_EE *p) {
    STM_LATEST_EVENT_SNAPSHOT event = stm_latest_event_snapshot(csound, p->ref.runner);
    int32_t entered = event.sequence > p->last_seen_event_seq && event.entered_node == (uint32_t) p->node_id;
    *p->trig = entered ? FL(1.0) : FL(0.0);
    p->last_seen_event_seq = event.sequence;
    return OK;
}

/* Runner-level exit event trigger: 1 only for a new transition event where
   this node stopped being current. */
int32_t graph_on_exit(CSOUND *csound, GRAPH_ON_EE *p) {
    STM_LATEST_EVENT_SNAPSHOT event = stm_latest_event_snapshot(csound, p->ref.runner);
    int32_t exited = event.sequence > p->last_seen_event_seq && event.exited_node == (uint32_t) p->node_id;
    *p->trig = exited ? FL(1.0) : FL(0.0);
    p->last_seen_event_seq = event.sequence;
    return OK;
}


int32_t graph_node_id(CSOUND *csound, GRAPH_NODE_ID *p) {
    GRAPH_RUNNER *g = p->ref.runner;
    int32_t n = graph_find_node(g->definition->nodes, g->definition->node_count, p->node_name->data);
    if (n < 0) {
        return csound->PerfError(csound, &(p->h), "[stm] stmnodeid: node not found");
    }

    *p->node_id = (MYFLT) n;
    return OK;
}

int32_t graph_node_name(CSOUND *csound, GRAPH_NODE_NAME *p) {
    GRAPH_RUNNER *g = p->ref.runner;
    GRAPH_DEFINITION *definition = g->definition;
    uint32_t node_id;
    if (!stm_myflt_to_uint32(*p->node_id, definition->node_count, &node_id)) {
        return csound->PerfError(csound, &(p->h), "[stm] stmnodename: invalid node id");
    }

    const char *name = definition->nodes[node_id].name;
    size_t len = strlen(name);
    if (len >= p->node_name->size) {
        void *newp = csound->ReAlloc(csound, p->node_name->data, len + 1);
        if (newp == NULL) {
            return csound->PerfError(csound, &(p->h), "[stm] stmnodename: memory error");
        }
        p->node_name->data = newp;
        p->node_name->size = len + 1;
    }

    memcpy(p->node_name->data, name, len + 1);
    return OK;
}

int32_t graph_node_count(CSOUND *csound, GRAPH_RUNNER_QUERY *p) {
    GRAPH_RUNNER *g = p->ref.runner;
    *p->out = g->definition->node_count;
    return OK;
}

int32_t graph_edge_count(CSOUND *csound, GRAPH_RUNNER_QUERY *p) {
    GRAPH_RUNNER *g = p->ref.runner;

    uint32_t edge_count = 0;
    for (uint32_t i = 0; i < g->definition->node_count; i++) {
        edge_count += g->definition->nodes[i].edge_count;
    }

    *p->out = edge_count;
    return OK;
}

int32_t graph_reset(CSOUND *csound, GRAPH_RESET *p) {
    GRAPH_RUNNER *g = p->ref.runner;

    stm_runner_update_begin(csound, g);
    uint32_t old_node = STM_LOAD(&g->current_node);
    uint32_t start_node = g->definition->start_node;
    STM_STORE(&g->current_node, start_node);
    STM_STORE(&g->previous_node, STM_NO_NODE);
    g->requested_node = STM_NO_NODE;
    g->request_conflict = STM_REQUEST_OK;
    STM_STORE(&g->graph_tick, 0);
    STM_STORE(&g->total_sample_frames, 0);
    STM_STORE(&g->node_sample_on_enter, 0);
    g->reset_pcycle = csound->GetEngineKcounter(csound);
    g->has_reset_cycle = 1;
    if (old_node != start_node) {
        record_gevent(g, old_node, start_node, STM_EVENT_RESET, 1);
    }

    stm_runner_update_end(csound, g);
    return OK;
}

int32_t graph_entry(CSOUND *csound, GRAPH_ENTRY *p) {
    STM_REGISTRY *reg;
    GRAPH_BUILDER *builder;
    if (STM_GET_BUILDER_INIT(csound, &(p->h), *p->handle, "[stm] stmentry: invalid builder", &reg, &builder) != OK) {
        return NOTOK;
    }

    if (builder->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmentry: graph already compiled. Move entry before stmcompile");
    }

    int32_t n = graph_find_node(builder->nodes, builder->node_count, p->entry_node->data);
    if (n < 0) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmentry: node not found");
    }

    builder->start_node = (uint32_t) n;
    stm_registry_unlock(csound, reg);
    return OK;
}

/* k-cycles since compile/reset: one tick per stmadvance call, so the graph
   clock stops whenever the machine is not being stepped */
int32_t graph_time_tick(CSOUND *csound, GRAPH_RUNNER_QUERY *p) {
    GRAPH_RUNNER *g = p->ref.runner;
    *p->out = (MYFLT) stm_snapshot_u64(csound, g, &g->graph_tick);
    return OK;
}

/* graph time in seconds: derived from the integer count of sample frames
   advanced by stmadvance, never accumulated as floating-point seconds */
int32_t graph_time_global(CSOUND *csound, GRAPH_RUNNER_QUERY *p) {
    GRAPH_RUNNER *g = p->ref.runner;
    *p->out = (MYFLT) stm_snapshot_u64(csound, g, &g->total_sample_frames) / CS_ESR;
    return OK;
}

/* seconds since the current node became current: 0 on the node's own first
   cycle, and unaffected by a stmnext that stmadvance rejected */
int32_t graph_time_node(CSOUND *csound, GRAPH_RUNNER_QUERY *p) {
    STM_NODE_TIME_SNAPSHOT clock =
            stm_node_time_snapshot(csound, p->ref.runner);
    *p->out = (MYFLT) (clock.total_frames - clock.node_enter) / CS_ESR;
    return OK;
}

static int32_t graph_transition_init(CSOUND *csound, GRAPH_TRANSITION *p) {
    if (stm_runner_ref_init(csound, &p->h, *p->handle, STM_RUNNER_READER, "[stm] stmevent: invalid runner", &p->ref) != OK) {
        return NOTOK;
    }

    p->next_event_seq = stm_snapshot_u64(csound, p->ref.runner, &p->ref.runner->event_seq) + 1U;
    return OK;
}

static int32_t graph_transition_deinit(CSOUND *csound, GRAPH_TRANSITION *p) {
    return stm_runner_ref_deinit(csound, &(p->h), &p->ref);
}

int32_t graph_transition(CSOUND *csound, GRAPH_TRANSITION *p) {
    STM_TRANSITION_SNAPSHOT snapshot = stm_transition_snapshot(csound, p->ref.runner, p->next_event_seq);

    *p->available = FL(0.0);
    *p->overflow = FL(0.0);

    if (snapshot.overflow) {
        *p->overflow = FL(1.0);
        p->next_event_seq = snapshot.oldest_sequence;
    }

    if (!snapshot.available) return OK;

    *p->available = FL(1.0);
    *p->status = (MYFLT) snapshot.event.status;
    *p->sequence = (MYFLT) snapshot.event.sequence;
    *p->from = (MYFLT) snapshot.event.from;
    *p->to = (MYFLT) snapshot.event.to;
    p->next_event_seq = snapshot.event.sequence + 1U;
    return OK;
}

static int32_t graph_checkpoint_init(CSOUND *csound, GRAPH_CHECKPOINT *p) {
    p->last_name[0] = '\0';
    *p->check = FL(0.0);
    return stm_runner_ref_init(csound, &p->h, *p->runner_handle, STM_RUNNER_WRITER, "[stm] stmcall: invalid runner", &p->ref);
}

static int32_t graph_checkpoint_resume_init(CSOUND *csound, GRAPH_CHECKPOINT *p) {
    p->last_name[0] = '\0';
    *p->check = FL(0.0);
    return stm_runner_ref_init(csound, &p->h, *p->runner_handle, STM_RUNNER_WRITER, "[stm] stmrecall: invalid runner", &p->ref);
}

static int32_t graph_checkpoint_deinit(CSOUND *csound, GRAPH_CHECKPOINT *p) {
    return stm_runner_ref_deinit(csound, &p->h, &p->ref);
}

int32_t graph_checkpoint(CSOUND *csound, GRAPH_CHECKPOINT *p) {
    *p->check = FL(0.0);

    if (*p->trig != FL(1.0)) return OK;
    if (strcmp(p->checkpoint_name->data, p->last_name) == 0) {
        *p->check = FL(1.0); // already captured by this opcode instance
        return OK;
    }

    GRAPH_RUNNER *runner = p->ref.runner;
    if (runner == NULL) {
        return csound->PerfError(csound, &p->h,"[stm] stmcall: runner is not initialized");
    }

    stm_runner_update_begin(csound, runner);
    int32_t result = record_gcheckpoint(runner, p->checkpoint_name->data);
    stm_runner_update_end(csound, runner);

    if (result != OK) {
        return csound->PerfError(csound, &p->h,"[stm] stmcall: could not record checkpoint '%s'",p->checkpoint_name->data);
    }

    snprintf(p->last_name, sizeof(p->last_name), "%s", p->checkpoint_name->data);
    *p->check = FL(1.0);
    return OK;
}

int32_t graph_checkpoint_resume(CSOUND *csound, GRAPH_CHECKPOINT *p) {
    GRAPH_RUNNER *runner = p->ref.runner;
    if (runner == NULL) {
        return csound->PerfError(csound, &p->h,"[stm] stmrecall: runner is not initialized");
    }

    *p->check = FL(0.0);

    if (*p->trig != FL(1.0)) {
        /* Re-arm the same checkpoint name for the next trigger pulse. */
        p->last_name[0] = '\0';
        return OK;
    }

    if (STM_LOAD(&runner->run_state) == STM_RUNNER_PAUSED) {
        return OK;
    }

    if (strcmp(p->checkpoint_name->data, p->last_name) == 0) {
        *p->check = FL(1.0); // already resumed for the current high trigger
        return OK;
    }

    stm_runner_update_begin(csound, runner);
    int32_t result = resume_gcheckpoint(runner, p->checkpoint_name->data);
    stm_runner_update_end(csound, runner);

    if (result != OK) {
        return csound->PerfError(csound, &p->h,"[stm] stmrecall: could not resume checkpoint '%s'",p->checkpoint_name->data);
    }

    snprintf(p->last_name, sizeof(p->last_name), "%s", p->checkpoint_name->data);
    *p->check = FL(1.0);
    return OK;
}

static int32_t graph_pause_init(CSOUND *csound, GRAPH_PAUSE *p) {
    return stm_runner_ref_init(csound, &p->h, *p->runner_handle, STM_RUNNER_WRITER, "[stm] stmpause/resume: invalid runner", &p->ref);
}

static int32_t graph_pause_deinit(CSOUND *csound, GRAPH_PAUSE *p) {
    return stm_runner_ref_deinit(csound, &p->h, &p->ref);
}

int32_t graph_pause(CSOUND *csound, GRAPH_PAUSE *p) {
    if (*p->trig != FL(1.0)) return OK;

    GRAPH_RUNNER *runner = p->ref.runner;
    if (runner == NULL) {
        return csound->PerfError(csound, &p->h,"[stm] stmpause: runner is not initialized");
    }

    stm_runner_update_begin(csound, runner);

    if (STM_LOAD(&runner->run_state) == STM_RUNNER_PAUSED) {
        stm_runner_update_end(csound, runner);
        return OK;
    }

    uint32_t current_node = STM_LOAD(&runner->current_node);
    STM_STORE(&runner->requested_node, STM_NO_NODE);
    STM_STORE(&runner->request_conflict, STM_REQUEST_OK);
    STM_STORE(&runner->run_state, STM_RUNNER_PAUSED);
    record_gevent(runner, current_node, current_node, STM_EVENT_PAUSED, 0);
    stm_runner_update_end(csound, runner);

    return OK;
}

int32_t graph_resume(CSOUND *csound, GRAPH_PAUSE *p) {
    if (*p->trig != FL(1.0)) return OK;

    GRAPH_RUNNER *runner = p->ref.runner;
    if (runner == NULL) {
        return csound->PerfError(csound, &p->h,"[stm] stmresume: runner is not initialized");
    }

    stm_runner_update_begin(csound, runner);

    if (STM_LOAD(&runner->run_state) == STM_RUNNER_RUNNING) {
        stm_runner_update_end(csound, runner);
        return OK;
    }

    uint32_t current_node = STM_LOAD(&runner->current_node);
    STM_STORE(&runner->requested_node, STM_NO_NODE);
    STM_STORE(&runner->request_conflict, STM_REQUEST_OK);
    STM_STORE(&runner->run_state, STM_RUNNER_RUNNING);
    record_gevent(runner, current_node, current_node, STM_EVENT_RESUME, 0);
    stm_runner_update_end(csound, runner);

    return OK;
}



// CSOUND OP-INTER

#define S(x) sizeof(x)

static OENTRY stm[] = {
    { "stmcreate",      S(GRAPH_CREATE),        0, "i",      "",      (SUBR) graph_create,                 NULL,                           (SUBR) graph_create_deinit     },
    { "stmaddnode",     S(GRAPH_ADD_NODE),      0, "",       "iS",    (SUBR) graph_add_node,               NULL,                           NULL                           },
    { "stmaddedge",     S(GRAPH_ADD_EDGE),      0, "",       "iSS",   (SUBR) graph_add_edge,               NULL,                           NULL                           },
    { "stmaddcondedge", S(GRAPH_ADD_COND_EDGE), 0, "",       "iSS[]", (SUBR) graph_add_cond_edge,          NULL,                           NULL                           },
    { "stmcompile",     S(GRAPH_COMPILE),       0, "i",      "i",     (SUBR) graph_compile,                NULL,                           (SUBR) graph_compile_deinit    },
    { "stminstance",    S(GRAPH_INSTANCE),      0, "i",      "i",     (SUBR) graph_instance,               NULL,                           (SUBR) graph_instance_deinit   },
    { "stmcurrent",     S(GRAPH_CURRENT),       0, "S",      "i",     (SUBR) graph_current_init,           (SUBR) graph_current,           (SUBR) graph_current_deinit    },
    { "stmcurrentid",   S(GRAPH_RUNNER_QUERY),  0, "k",      "i",     (SUBR) graph_current_id_init,        (SUBR) graph_current_id,        (SUBR) graph_current_id_deinit },
    { "stmadvance",     S(GRAPH_ADVANCE),       0, "kkk",    "i",     (SUBR) graph_advance_init,           (SUBR) graph_advance,           (SUBR) graph_advance_deinit    },
    { "stmnext",        S(GRAPH_NEXT),          0, "",       "iS",    (SUBR) graph_next_init,              (SUBR) graph_next,              (SUBR) graph_next_deinit       },
    { "stmnext.id",     S(GRAPH_NEXT_ID),       0, "",       "ik",    (SUBR) graph_next_id_init,           (SUBR) graph_next_id,           (SUBR) graph_next_id_deinit    },
    { "stmonenter",     S(GRAPH_ON_EE),         0, "k",      "iS",    (SUBR) graph_on_ee_init,             (SUBR) graph_on_enter,          (SUBR) graph_on_ee_deinit      },
    { "stmonexit",      S(GRAPH_ON_EE),         0, "k",      "iS",    (SUBR) graph_on_ee_init,             (SUBR) graph_on_exit,           (SUBR) graph_on_ee_deinit      },
    { "stmnodename",    S(GRAPH_NODE_NAME),     0, "S",      "ik",    (SUBR) graph_node_name_init,         (SUBR) graph_node_name,         (SUBR) graph_node_name_deinit  },
    { "stmnodeid",      S(GRAPH_NODE_ID),       0, "k",      "iS",    (SUBR) graph_node_id_init,           (SUBR) graph_node_id,           (SUBR) graph_node_id_deinit    },
    { "stmnodecount",   S(GRAPH_RUNNER_QUERY),  0, "k",      "i",     (SUBR) graph_node_count_init,        (SUBR) graph_node_count,        (SUBR) graph_node_count_deinit },
    { "stmedgecount",   S(GRAPH_RUNNER_QUERY),  0, "k",      "i",     (SUBR) graph_edge_count_init,        (SUBR) graph_edge_count,        (SUBR) graph_edge_count_deinit },
    { "stmreset",       S(GRAPH_RESET),         0, "",       "i",     (SUBR) graph_reset_init,             (SUBR) graph_reset,             (SUBR) graph_reset_deinit      },
    { "stmentry",       S(GRAPH_ENTRY),         0, "",       "iS",    (SUBR) graph_entry,                  NULL,                           NULL                           },
    { "stmtick",        S(GRAPH_RUNNER_QUERY),  0, "k",      "i",     (SUBR) graph_time_init,              (SUBR) graph_time_tick,         (SUBR) graph_time_deinit       },
    { "stmnodetime",    S(GRAPH_RUNNER_QUERY),  0, "k",      "i",     (SUBR) graph_time_init,              (SUBR) graph_time_node,         (SUBR) graph_time_deinit       },
    { "stmtime",        S(GRAPH_RUNNER_QUERY),  0, "k",      "i",     (SUBR) graph_time_init,              (SUBR) graph_time_global,       (SUBR) graph_time_deinit       },
    { "stmevent",       S(GRAPH_TRANSITION),    0, "kkkkkk", "i",     (SUBR) graph_transition_init,        (SUBR) graph_transition,        (SUBR) graph_transition_deinit },
    { "stmcall",        S(GRAPH_CHECKPOINT),    0, "k",      "iSk",   (SUBR) graph_checkpoint_init,        (SUBR) graph_checkpoint,        (SUBR) graph_checkpoint_deinit },
    { "stmrecall",      S(GRAPH_CHECKPOINT),    0, "k",      "iSk",   (SUBR) graph_checkpoint_resume_init, (SUBR) graph_checkpoint_resume, (SUBR) graph_checkpoint_deinit },
    { "stmpause",       S(GRAPH_PAUSE),         0, "",       "ik",    (SUBR) graph_pause_init,             (SUBR) graph_pause,             (SUBR) graph_pause_deinit      },
    { "stmresume",      S(GRAPH_PAUSE),         0, "",       "ik",    (SUBR) graph_pause_init,             (SUBR) graph_resume,            (SUBR) graph_pause_deinit      },
};

int32_t stm_init_(CSOUND *csound) {
    return csound->AppendOpcodes(csound, &(stm[0]), (int32_t) (sizeof(stm) / sizeof(OENTRY)));
}
