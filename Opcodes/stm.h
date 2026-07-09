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

   GRAPH CLOCK

   Every graph carries its own clock, read with three k-rate opcodes:

     stmtick     - k-cycles elapsed since stmcompile or stmreset
     stmtime     - graph time in seconds, that is tick * kperiod
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

   Both times are derived from the tick count rather than accumulated, so they
   cannot drift over long runs, single precision builds included. kperiod is
   captured from the instrument that calls stmadvance, so a driver with a local
   setksmps is measured on its own control rate. stmtick is counted internally
   in a uint64 and returned as MYFLT, hence exact up to 2^53 in a double build
   and 2^24 in a float one.

   Node time is measured from the tick at which the current node was entered:

     - it reads 0 on the node's own first cycle
     - a transition rejected by stmadvance (no such edge) leaves it untouched,
       since the current node never changed
     - stmreset restarts tick, graph time and node time at 0, exactly the way
       stmcompile does
*/

#ifndef STM_H
#define STM_H

#include "stdopcod.h"
#include "csound.h"
#include "sysdep.h"
#include <csdl.h>
#include <stdint.h>


#define INITIAL_NODE_CAPACITY 10
#define INITIAL_EDGE_CAPACITY 10
#define INITIAL_GRAPH_CAPACITY 8
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
    uint64_t node_tick_on_enter; // graph_tick at which current_node was entered
    MYFLT kperiod; // seconds per k-cycle of the advancing instrument
    int32_t reset_pending; // stmreset ran this cycle: next stmadvance does not tick
    // compile and freeze
    int32_t compiled;
} GRAPH;

/* Per-csound registry: handles are 1-based indices into this table,
   stored as MYFLT (exact for small ints, no pointer truncation). */
typedef struct {
    GRAPH **graphs;
    uint32_t count;
    uint32_t capacity;
} STM_REGISTRY;

typedef struct {
    OPDS h;
    // outputs
    MYFLT *handle;
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
    // private
    int32_t node_id;
    int32_t was_current;
    GRAPH *g;
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
