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
 graphs inspired by LangGraph. The graph owns the state machine: nodes
 (identified by name) and edges (valid transitions). Node computation is
 written as ordinary user-defined opcodes that operate on a shared struct
 passed by reference, and dispatched from the orchestra according to the
 current node id (stmcurrent). A node requests a transition with stmnext;
 stmadvance validates it against the edges and advances the current node,
 keeping control logic separate from audio DSP.
*/



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
    uint32_t node_count;
    uint32_t node_capacity;

    uint32_t current_node;
    uint32_t previous_node;
    uint32_t requested_node;

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



int32_t graph_create(CSOUND *csound, GRAPH_CREATE *p); // i-time
int32_t graph_create_deinit(CSOUND *csound, GRAPH_CREATE *p);
int32_t graph_add_node(CSOUND *csound, GRAPH_ADD_NODE *p); // i-time
int32_t graph_add_edge(CSOUND *csound, GRAPH_ADD_EDGE *p); // i-time
int32_t graph_add_cond_edge(CSOUND *csound, GRAPH_ADD_COND_EDGE *p); // i-time
int32_t graph_compile(CSOUND *csound, GRAPH_COMPILE *p); // i-time
int32_t graph_current(CSOUND *csound, GRAPH_CURRENT *p); // k-time
int32_t graph_advance(CSOUND *csound, GRAPH_ADVANCE *p); // k-time
int32_t graph_next(CSOUND *csound, GRAPH_NEXT *p); // k-time
int32_t graph_on_ee_init(CSOUND *csound, GRAPH_ON_EE *p); // i-time
int32_t graph_on_enter(CSOUND *csound, GRAPH_ON_EE *p); // k-time
int32_t graph_on_exit(CSOUND *csound, GRAPH_ON_EE *p); // k-time
