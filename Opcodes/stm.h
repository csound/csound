/*
    stm.h:

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

#ifndef STM_H
#define STM_H

#include "stdopcod.h"
#include "csound.h"
#include "sysdep.h"
#include <stdint.h>


#define INITIAL_NODE_CAPACITY 10
#define INITIAL_EDGE_CAPACITY 10
#define INITIAL_GRAPH_CAPACITY 8
#define TRANSITION_BUFFER_CAPACITY 10
#define CHECKPOINT_BUFFER_CAPACITY 100
#define STM_HANDLE_SLOT_BASE 4096U
#define STM_HANDLE_MAX_EXACT 16777216U
#define STM_HANDLE_GENERATION_MAX (STM_HANDLE_MAX_EXACT / STM_HANDLE_SLOT_BASE)
#define STM_NO_NODE UINT32_MAX
#define STM_REGISTRY_NAME "::stm_registry::"
#define STM_REQUEST_OK 0
#define STM_REQUEST_CONFLICT 1

#define STM_GET_BUILDER_INIT(csound, opds, handle, msg, reg, out) \
    stm_get_object_init_locked((csound), (opds), (handle), STM_OBJECT_BUILDER, (msg), (reg), (void **) (out))

#define STM_GET_DEFINITION_INIT(csound, opds, handle, msg, reg, out) \
    stm_get_object_init_locked((csound), (opds), (handle), STM_OBJECT_DEFINITION, (msg), (reg), (void **) (out))

#define STM_GET_RUNNER_INIT(csound, opds, handle, msg, reg, out) \
    stm_get_object_init_locked((csound), (opds), (handle), STM_OBJECT_RUNNER, (msg), (reg), (void **) (out))


typedef enum {
    STM_RUNNER_READER = 0,
    STM_RUNNER_WRITER
} STM_RUNNER_ACCESS;

typedef enum {
    STM_EVENT_CHANGED = 1,
    STM_EVENT_SELF_TRANSITION,
    STM_EVENT_RESET,
    STM_EVENT_RESUME
} GRAPH_EVENT_STATUS;

typedef struct {
    uint64_t sequence;
    uint32_t from;
    uint32_t to;
    int32_t status;
} GRAPH_TRANSITION_EVENT;

typedef struct {
    uint64_t sequence;
    uint32_t entered_node;
    uint32_t exited_node;
} STM_LATEST_EVENT_SNAPSHOT;

typedef struct {
    uint64_t total_frames;
    uint64_t node_enter;
} STM_NODE_TIME_SNAPSHOT;

typedef struct {
    int32_t available;
    int32_t overflow;
    uint64_t oldest_sequence;
    GRAPH_TRANSITION_EVENT event;
} STM_TRANSITION_SNAPSHOT;

typedef struct {
    char *name;
    uint32_t id;
    uint32_t *edges; // node id in graph
    uint32_t edge_count;
    uint32_t edge_capacity;
} GRAPH_NODE;

typedef struct {
    GRAPH_NODE *nodes;
    uint32_t node_count;
    uint32_t node_capacity;
    uint32_t start_node;
    int32_t compiled;
} GRAPH_BUILDER;

typedef struct {
    GRAPH_NODE *nodes;
    uint32_t node_count;
    uint32_t start_node;
    uint32_t refcount;
} GRAPH_DEFINITION;

typedef struct {
    char name[64];
    int32_t is_valid;
    uint32_t current_node;
    uint32_t previous_node;
    uint32_t requested_node;
    int32_t request_conflict;
    uint64_t graph_tick;
    uint64_t total_sample_frames;
    uint64_t node_sample_on_enter;
} STM_CHECKPOINT;

typedef struct {
    GRAPH_DEFINITION *definition;
    uint32_t refcount;
    uint32_t state_version;
#if !defined(HAVE_ATOMIC_BUILTIN)
    void *state_mutex;
#endif
    INSDS *writer_owner;
    uint32_t writer_claims;
    // mutable (runner)
    uint32_t current_node;
    uint32_t previous_node;
    uint32_t requested_node;
    int32_t request_conflict;
    // time section: the clock is driven by stmadvance, see the note above
    uint64_t graph_tick; // stmadvance calls since compile/reset
    uint64_t total_sample_frames; // samples advanced since compile/reset
    uint64_t node_sample_on_enter; // total_sample_frames at node entry
    // transition event section: runner-level enter/exit events
    uint64_t event_seq;
    uint32_t event_entered_node;
    uint32_t event_exited_node;
    // cycle (graph reset)
    uint64_t reset_pcycle;
    int32_t has_reset_cycle;
    // transitions buffer
    GRAPH_TRANSITION_EVENT *transitions;
    uint32_t tndx_write;
    uint32_t transition_count;
    // checkpoints
    STM_CHECKPOINT *checkpoints;
    uint32_t cndx_write;
    uint32_t ckp_count;
} GRAPH_RUNNER;

typedef struct {
    GRAPH_RUNNER *runner;
    INSDS *writer_owner;
    int32_t writer_claimed;
} STM_RUNNER_REF;

typedef enum {
    STM_OBJECT_NONE = 0,
    STM_OBJECT_BUILDER,
    STM_OBJECT_DEFINITION,
    STM_OBJECT_RUNNER
} STM_OBJECT_TYPE;

typedef union {
    void *ptr;
    GRAPH_BUILDER *builder;
    GRAPH_DEFINITION *definition;
    GRAPH_RUNNER *runner;
} STM_OBJECT_POINTER;

typedef struct {
    STM_OBJECT_POINTER object;
    uint32_t generation;
    STM_OBJECT_TYPE type;
} STM_REGISTRY_SLOT;

typedef struct {
    uint32_t slot;
    uint32_t generation;
    STM_OBJECT_TYPE type;
} STM_OWNER_TOKEN;

/* Per-csound registry: handles encode a slot and generation into an integer
   stored as MYFLT (kept within the exact integer range of float builds). */
typedef struct {
    STM_REGISTRY_SLOT *slots;
    uint32_t count;
    uint32_t capacity;
    void *mutex;
} STM_REGISTRY;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *handle;
    STM_OWNER_TOKEN owner;
} GRAPH_CREATE;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    STRINGDAT *node_name;
} GRAPH_ADD_NODE;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    STRINGDAT *from;
    STRINGDAT *to;
} GRAPH_ADD_EDGE;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    STRINGDAT *from;
    ARRAYDAT *targets;
} GRAPH_ADD_COND_EDGE;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *def_handle; // definition handle
    // inputs
    MYFLT *bld_handle; // builder handle
    STM_OWNER_TOKEN owner;
} GRAPH_COMPILE;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *runner_handle;
    // inputs
    MYFLT *def_handle;
    STM_OWNER_TOKEN owner;
} GRAPH_INSTANCE;

typedef struct {
    OPDS h;
    // outputs
    STRINGDAT *cur;
    // inputs
    MYFLT *handle;
    STM_RUNNER_REF ref;
} GRAPH_CURRENT; // k-rate

/* Shared by every k-rate runner query with a single MYFLT output:
   stmcurrentid, stmnodecount, stmedgecount, stmtick, stmtime, stmnodetime. */
typedef struct {
    OPDS h;
    // outputs
    MYFLT *out;
    // inputs
    MYFLT *handle;
    STM_RUNNER_REF ref;
} GRAPH_RUNNER_QUERY; // k-rate

typedef enum {
    STM_NO_REQUEST = 0,
    STM_CHANGED,
    STM_ILLEGAL_EDGE,
    STM_SELF_TRANSITION,
    STM_CONFLICT
} STM_ADVANCE_STATUS;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *status;
    MYFLT *id_from;
    MYFLT *id_to;
    // inputs
    MYFLT *handle;
    STM_RUNNER_REF ref;
} GRAPH_ADVANCE; // k-rate

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    STRINGDAT *next_node;
    STM_RUNNER_REF ref;
} GRAPH_NEXT;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    MYFLT *next_node;
    STM_RUNNER_REF ref;
} GRAPH_NEXT_ID;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *trig;
    // inputs
    MYFLT *handle;
    STRINGDAT *node;
    // The node id is stable in the immutable definition. The retained runner
    // reference keeps both the runtime state and its definition alive.
    int32_t node_id;
    uint64_t last_seen_event_seq;
    STM_RUNNER_REF ref;
} GRAPH_ON_EE;

// INTROSPECTION

typedef struct {
    OPDS h;
    // outputs
    MYFLT *node_id;
    // inputs
    MYFLT *handle;
    STRINGDAT *node_name;
    STM_RUNNER_REF ref;
} GRAPH_NODE_ID;

typedef struct {
    OPDS h;
    // outputs
    STRINGDAT *node_name;
    // inputs
    MYFLT *handle;
    MYFLT *node_id;
    STM_RUNNER_REF ref;
} GRAPH_NODE_NAME;

// CONTROL FLOW

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    STM_RUNNER_REF ref;
} GRAPH_RESET;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    STRINGDAT *entry_node;
} GRAPH_ENTRY;

// TRANSITION EVENT RING

typedef struct {
    OPDS h;
    // outputs
    MYFLT *status;
    MYFLT *sequence;
    MYFLT *overflow;
    MYFLT *available;
    MYFLT *from;
    MYFLT *to;
    // inputs
    MYFLT *handle;
    // private
    uint64_t next_event_seq;
    STM_RUNNER_REF ref;
} GRAPH_TRANSITION;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *check;
    // inputs
    MYFLT *runner_handle;
    STRINGDAT *checkpoint_name;
    MYFLT *trig;
    //private
    STM_RUNNER_REF ref;
    char last_name[64];
} GRAPH_CHECKPOINT;


// INTERFACE

// graph
int32_t graph_create(CSOUND *csound, GRAPH_CREATE *p); // i-time
int32_t graph_create_deinit(CSOUND *csound, GRAPH_CREATE *p);
int32_t graph_compile(CSOUND *csound, GRAPH_COMPILE *p); // i-time
int32_t graph_compile_deinit(CSOUND *csound, GRAPH_COMPILE *p);
int32_t graph_instance(CSOUND *csound, GRAPH_INSTANCE *p); // i-time
int32_t graph_instance_deinit(CSOUND *csound, GRAPH_INSTANCE *p);
int32_t graph_reset(CSOUND *csound, GRAPH_RESET *p); // k-time
// node
int32_t graph_add_node(CSOUND *csound, GRAPH_ADD_NODE *p); // i-time
int32_t graph_node_id(CSOUND *csound, GRAPH_NODE_ID *p); // k-time
int32_t graph_node_name(CSOUND *csound, GRAPH_NODE_NAME *p); // k-time
int32_t graph_node_count(CSOUND *csound, GRAPH_RUNNER_QUERY *p); // k-time
int32_t graph_entry(CSOUND *csound, GRAPH_ENTRY *p); // i-time
// edge
int32_t graph_add_edge(CSOUND *csound, GRAPH_ADD_EDGE *p); // i-time
int32_t graph_add_cond_edge(CSOUND *csound, GRAPH_ADD_COND_EDGE *p); // i-time
int32_t graph_edge_count(CSOUND *csound, GRAPH_RUNNER_QUERY *p); // k-time
// flow and trig
int32_t graph_current(CSOUND *csound, GRAPH_CURRENT *p); // k-time
int32_t graph_current_id(CSOUND *csound, GRAPH_RUNNER_QUERY *p); // k-time
int32_t graph_advance(CSOUND *csound, GRAPH_ADVANCE *p); // k-time
int32_t graph_next(CSOUND *csound, GRAPH_NEXT *p); // k-time
int32_t graph_next_id(CSOUND *csound, GRAPH_NEXT_ID *p); // k-time
int32_t graph_on_ee_init(CSOUND *csound, GRAPH_ON_EE *p); // i-time
int32_t graph_on_enter(CSOUND *csound, GRAPH_ON_EE *p); // k-time
int32_t graph_on_exit(CSOUND *csound, GRAPH_ON_EE *p); // k-time
int32_t graph_transition(CSOUND *csound, GRAPH_TRANSITION *p); // k-time
// time
int32_t graph_time_tick(CSOUND *csound, GRAPH_RUNNER_QUERY *p); // k-time
int32_t graph_time_global(CSOUND *csound, GRAPH_RUNNER_QUERY *p); // k-time
int32_t graph_time_node(CSOUND *csound, GRAPH_RUNNER_QUERY *p); // k-time
// points
int32_t graph_checkpoint(CSOUND *csound, GRAPH_CHECKPOINT *p); // k-time
int32_t graph_checkpoint_resume(CSOUND *csound, GRAPH_CHECKPOINT *p); // k-time


#endif
