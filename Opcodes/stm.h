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

   The graph owns the state-machine structure: named nodes and the edges that
   define valid transitions between them. Node computation remains ordinary
   Csound code, typically written as user-defined opcodes operating on a shared
   struct passed by reference.

   At k-rate, the orchestra asks the graph for the current node with stmcurrent,
   dispatches explicitly to the matching node implementation, then calls
   stmadvance. During its execution, a node may request a transition with
   stmnext. stmadvance validates that request against the graph edges and applies
   it only if it is legal.

   The practical runtime loop is:

   stmcurrent -> dispatch current node -> node updates shared state calls stmnext -> stmadvance

   This keeps the state-machine control flow explicit and deterministic while
   leaving DSP, analysis, and musical behavior in the orchestra.

   ENTER/EXIT EVENTS

   stmonenter and stmonexit report graph-level transition events, not an
   observer-local rising/falling edge. Each opcode instance records the graph
   event sequence that was current when it was initialized; it only triggers
   for a later event where the named node was entered or exited. Therefore an
   observer scheduled after node B is already current does not receive a fake
   stmonenter(B) on its first pass.

   These opcodes observe the latest graph event seen by that opcode call. If a
   graph is advanced through multiple transitions between two passes of a slow
   observer, intermediate events may be missed. For process lifecycle binding,
   the deterministic pattern is to run the enter/exit checks in the same
   supervisor loop that advances the graph, and bind the initial state
   explicitly if it needs an initial process.

   GRAPH CLOCK

   Every graph carries its own clock, read with three k-rate opcodes:

     stmtick     - k-cycles elapsed since stmcompile or stmreset
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

     1. call stmadvance exactly once per graph per k-cycle. Two instrument
        instances sharing one graph handle would tick it twice per cycle.
     2. read the time opcodes BEFORE stmadvance. After it they already report
        the values of the next cycle.

   Times are derived from integer sample-frame counters rather than accumulated
   floating-point seconds, so they cannot drift over long runs. Each stmadvance
   adds the advancing instrument's current CS_KSMPS to the graph's total sample
   count, which means successive drivers with different local setksmps values
   contribute their real control-period lengths without retroactively rescaling
   earlier steps. stmtick is counted internally in a uint64 and returned as
   MYFLT, hence exact up to 2^53 in a double build and 2^24 in a float one.

   Node time is measured from the tick at which the current node was entered:

     - it reads 0 on the node's own first cycle
     - a transition rejected by stmadvance (no such edge) leaves it untouched,
       since the current node never changed
     - stmreset restarts tick, graph time and node time at 0, exactly the way
       stmcompile does

   REGISTRY HANDLES

   Graph handles are numeric MYFLT values because orchestra code cannot safely
   carry raw C pointers. The registry stores graphs in reusable slots and
   encodes both the slot index and a generation counter into the handle. This
   lets STM reuse a freed slot without making an old handle point to the new
   graph now living there.

   The handle encoding is intentionally kept within 2^24, so all handle
   integers are exact even in single-precision MYFLT builds. With the current
   constants this allows 4096 live registry slots per CSOUND instance and 4096
   generations per slot, for up to 16,777,216 graph lifetimes before every slot
   would be exhausted.

   CONCURRENCY MODEL

   A graph handle may be shared across instruments, including under multicore
   performance. STM protects the registry, graph lookup, graph mutation and
   graph deinit with a per-CSOUND STM registry mutex. This prevents ordinary
   data races and resolve/free races when different instruments touch the same
   graph.

   Synchronization is per opcode call, not a transaction over the whole
   stmcurrent -> node dispatch -> stmnext -> stmadvance orchestra pattern.
   If multiple instruments mutate one graph, the scheduler's execution order is
   still the semantic order. A single supervisor remains the most deterministic
   pattern, but it is not enforced.
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
#define STM_HANDLE_SLOT_BASE 4096U
#define STM_HANDLE_MAX_EXACT 16777216U
#define STM_HANDLE_GENERATION_MAX (STM_HANDLE_MAX_EXACT / STM_HANDLE_SLOT_BASE)
#define NO_NODE UINT32_MAX
#define STM_REGISTRY_NAME "::stm_registry::"

typedef struct {
    char *name;
    uint32_t id;
    uint32_t *edges; // node id in graph
    uint32_t edge_count;
    uint32_t edge_capacity;
} GRAPH_NODE;

typedef struct {
    GRAPH_NODE *nodes;
    // node section
    uint32_t node_count;
    uint32_t node_capacity;
    uint32_t current_node;
    uint32_t previous_node;
    uint32_t requested_node;
    uint32_t start_node;
    // time section: the clock is driven by stmadvance, see the note above
    uint64_t graph_tick; // stmadvance calls since compile/reset
    uint64_t total_sample_frames; // samples advanced since compile/reset
    uint64_t node_sample_on_enter; // total_sample_frames at node entry
    int32_t reset_pending; // stmreset ran this cycle: next stmadvance does not tick
    // transition event section: graph-level enter/exit events
    uint64_t event_seq;
    uint32_t event_entered_node;
    uint32_t event_exited_node;
    // compile and freeze
    int32_t compiled;
} GRAPH;

typedef struct {
    GRAPH *graph;
    uint32_t generation;
} STM_REGISTRY_SLOT;

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
    // private: owner state used by deinit, independent from the exposed handle
    GRAPH *graph;
    uint32_t slot;
    uint32_t generation;
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
    // inputs
    MYFLT *handle;
} GRAPH_COMPILE;

typedef struct {
    OPDS h;
    // outputs
    STRINGDAT *cur;
    // inputs
    MYFLT *handle;
} GRAPH_CURRENT; // k-rate

typedef struct {
    OPDS h;
    // outputs
    MYFLT *cur;
    // inputs
    MYFLT *handle;
} GRAPH_CURRENT_ID; // k-rate

typedef struct {
    OPDS h;
    // outputs
    MYFLT *changed;
    // inputs
    MYFLT *handle;
} GRAPH_ADVANCE; // k-rate

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    STRINGDAT *next_node;
} GRAPH_NEXT;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    MYFLT *next_node;
} GRAPH_NEXT_ID;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *trig;
    // inputs
    MYFLT *handle;
    STRINGDAT *node;
    // private: the node id is stable (the graph is immutable once compiled), but
    // the GRAPH itself must never be cached, it dies with the instrument that
    // created it. Resolve the handle on every perf pass instead.
    int32_t node_id;
    uint64_t last_seen_event_seq;
} GRAPH_ON_EE;

// INTROSPECTION

typedef struct {
    OPDS h;
    // outputs
    MYFLT *node_id;
    // inputs
    MYFLT *handle;
    STRINGDAT *node_name;
} GRAPH_NODE_ID;

typedef struct {
    OPDS h;
    // outputs
    STRINGDAT *node_name;
    // inputs
    MYFLT *handle;
    MYFLT *node_id;
} GRAPH_NODE_NAME;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *node_count;
    // inputs
    MYFLT *handle;
} GRAPH_NODE_COUNT;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *edge_count;
    // inputs
    MYFLT *handle;
} GRAPH_EDGE_COUNT;


// CONTROL FLOW

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
} GRAPH_RESET;

typedef struct {
    OPDS h;
    // inputs
    MYFLT *handle;
    STRINGDAT *entry_node;
} GRAPH_ENTRY;

// TIME SECTION

typedef struct {
    OPDS h;
    // outputs
    MYFLT *t;
    // inputs
    MYFLT *handle;
} GRAPH_TIME;


// INTERFACE

// graph
int32_t graph_create(CSOUND *csound, GRAPH_CREATE *p); // i-time
int32_t graph_create_deinit(CSOUND *csound, GRAPH_CREATE *p);
int32_t graph_compile(CSOUND *csound, GRAPH_COMPILE *p); // i-time
int32_t graph_reset(CSOUND *csound, GRAPH_RESET *p); // k-time
// node
int32_t graph_add_node(CSOUND *csound, GRAPH_ADD_NODE *p); // i-time
int32_t graph_node_id(CSOUND *csound, GRAPH_NODE_ID *p); // k-time
int32_t graph_node_name(CSOUND *csound, GRAPH_NODE_NAME *p); // k-time
int32_t graph_node_count(CSOUND *csound, GRAPH_NODE_COUNT *p); // k-time
int32_t graph_entry(CSOUND *csound, GRAPH_ENTRY *p); // i-time
// edge
int32_t graph_add_edge(CSOUND *csound, GRAPH_ADD_EDGE *p); // i-time
int32_t graph_add_cond_edge(CSOUND *csound, GRAPH_ADD_COND_EDGE *p); // i-time
int32_t graph_edge_count(CSOUND *csound, GRAPH_EDGE_COUNT *p); // k-time
// flow and trig
int32_t graph_current(CSOUND *csound, GRAPH_CURRENT *p); // k-time
int32_t graph_current_id(CSOUND *csound, GRAPH_CURRENT_ID *p); // k-time
int32_t graph_advance(CSOUND *csound, GRAPH_ADVANCE *p); // k-time
int32_t graph_next(CSOUND *csound, GRAPH_NEXT *p); // k-time
int32_t graph_next_id(CSOUND *csound, GRAPH_NEXT_ID *p); // k-time
int32_t graph_on_ee_init(CSOUND *csound, GRAPH_ON_EE *p); // i-time
int32_t graph_on_enter(CSOUND *csound, GRAPH_ON_EE *p); // k-time
int32_t graph_on_exit(CSOUND *csound, GRAPH_ON_EE *p); // k-time
// time
int32_t graph_time_tick(CSOUND *csound, GRAPH_TIME *p); // k-time
int32_t graph_time_global(CSOUND *csound, GRAPH_TIME *p); // k-time
int32_t graph_time_node(CSOUND *csound, GRAPH_TIME *p); // k-time



#endif
