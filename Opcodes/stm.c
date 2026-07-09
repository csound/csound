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



#include "stm.h"
#include "coreDefs.h"
#include "csound.h"
#include "sysdep.h"
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

/* Register a graph, returns a 1-based handle (0 on failure). */
static uint32_t stm_register_graph(CSOUND *csound, GRAPH *g) {
    STM_REGISTRY *reg = stm_registry_query(csound);
    if (reg == NULL) {
        if (csound->CreateGlobalVariable(csound, STM_REGISTRY_NAME, sizeof(STM_REGISTRY)) != 0)
            return 0;
        reg = stm_registry_query(csound);
        if (reg == NULL) return 0;
        reg->capacity = INITIAL_GRAPH_CAPACITY;
        reg->count = 0;
        reg->graphs = csound->Calloc(csound, sizeof(GRAPH *) * reg->capacity);
        if (reg->graphs == NULL) return 0;
    }

    if (reg->count == reg->capacity) {
        uint32_t newcap = reg->capacity * 2;
        GRAPH **grown = csound->ReAlloc(csound, reg->graphs, sizeof(GRAPH *) * newcap);
        if (grown == NULL) return 0;
        reg->graphs = grown;
        reg->capacity = newcap;
    }

    reg->graphs[reg->count] = g;
    return ++reg->count; // 1-based handle
}

static GRAPH *stm_handle_to_graph(CSOUND *csound, MYFLT handle) {
    STM_REGISTRY *reg = stm_registry_query(csound);
    if (reg == NULL) return NULL;
    uint32_t idx = (uint32_t) handle;
    if (idx == 0 || idx > reg->count) return NULL;
    return reg->graphs[idx - 1];
}

/* ------------------------------------------------------------------ */
/* graph helpers                                                      */
/* ------------------------------------------------------------------ */

static int32_t graph_find_node(GRAPH *g, const char *name) {
    for (uint32_t i = 0; i < g->node_count; i++) {
        if (strcmp(g->nodes[i].name, name) == 0)
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

static int32_t add_edge_helper(CSOUND *csound, GRAPH *g, const char *from_node_name, const char *to_node_name) {
    int32_t from = graph_find_node(g, from_node_name);
    int32_t to = graph_find_node(g, to_node_name);

    if (from < 0 || to < 0) { return NOTOK; }

    GRAPH_NODE *from_node = &g->nodes[from];

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

/* ------------------------------------------------------------------ */
/* opcodes                                                            */
/* ------------------------------------------------------------------ */

int32_t graph_create_deinit(CSOUND *csound, GRAPH_CREATE *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g != NULL) {
        if (g->nodes != NULL) {
            for (uint32_t i = 0; i < g->node_count; i++) {
                csound->Free(csound, g->nodes[i].edges);
                csound->Free(csound, g->nodes[i].name);
            }
            csound->Free(csound, g->nodes);
        }
        // null the registry slot so the handle can no longer resolve
        STM_REGISTRY *reg = stm_registry_query(csound);
        uint32_t idx = (uint32_t) *p->handle;
        if (reg != NULL && idx > 0 && idx <= reg->count)
            reg->graphs[idx - 1] = NULL;
        csound->Free(csound, g);
        *p->handle = 0;
    }
    return OK;
}

int32_t graph_create(CSOUND *csound, GRAPH_CREATE *p) {
    GRAPH *g = (GRAPH *) csound->Calloc(csound, sizeof(GRAPH));
    if (g == NULL)
        return csound->InitError(csound, "[stm] stmcreate: graph memory error");

    g->node_capacity = INITIAL_NODE_CAPACITY;
    g->node_count = 0;
    g->nodes = csound->Calloc(csound, sizeof(GRAPH_NODE) * g->node_capacity);
    if (g->nodes == NULL) {
        csound->Free(csound, g);
        return csound->InitError(csound, "[stm] stmcreate: graph memory error");
    }

    g->compiled = 0;

    uint32_t handle = stm_register_graph(csound, g);
    if (handle == 0) {
        csound->Free(csound, g->nodes);
        csound->Free(csound, g);
        return csound->InitError(csound, "[stm] stmcreate: registry error");
    }

    g->start_node = NO_NODE;
    *p->handle = (MYFLT) handle;
    return OK;
}

int32_t graph_add_node(CSOUND *csound, GRAPH_ADD_NODE *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL) {
        return csound->InitError(csound, "[stm] stmaddnode: invalid graph");
    }
    if (g->compiled) {
        return csound->InitError(csound, "[stm] stmaddnode: graph already compiled (immutable)");
    }
    if (graph_find_node(g, p->node_name->data) >= 0) {
        return csound->InitError(csound, "[stm] stmaddnode: duplicate node name '%s'", p->node_name->data);
    }

    if (g->node_count == g->node_capacity) {
        uint32_t newcap = g->node_capacity * 2;
        GRAPH_NODE *grown = csound->ReAlloc(csound, g->nodes, sizeof(GRAPH_NODE) * newcap);
        if (grown == NULL) {
            return csound->InitError(csound, "[stm] stmaddnode: memory error");
        }
        g->nodes = grown;
        g->node_capacity = newcap;
    }
    uint32_t id = g->node_count;

    GRAPH_NODE *node = &g->nodes[id];
    memset(node, 0, sizeof(GRAPH_NODE));

    node->id = id;
    size_t namelen = strlen(p->node_name->data);
    node->name = csound->Calloc(csound, namelen + 1);
    if (node->name == NULL)
        return csound->InitError(csound, "[stm] stmaddnode: memory error");
    memcpy(node->name, p->node_name->data, namelen);

    node->edge_count = 0;
    node->edge_capacity = INITIAL_EDGE_CAPACITY;
    node->edges = csound->Calloc(csound, sizeof(uint32_t) * node->edge_capacity);
    if (node->edges == NULL) {
        csound->Free(csound, node->name);
        return csound->InitError(csound, "[stm] stmaddnode: memory error");
    }

    g->node_count++;
    return OK;
}

int32_t graph_add_edge(CSOUND *csound, GRAPH_ADD_EDGE *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL) {
        return csound->InitError(csound, "[stm] stmaddedge: invalid graph");
    }

    if (g->compiled) {
        return csound->InitError(csound, "[stm] stmaddedge: graph already compiled (immutable)");
    }

    if (add_edge_helper(csound, g, p->from->data, p->to->data) == NOTOK) {
        return csound->InitError(csound, "[stm] stmaddedge, something went wrong");
    }

    return OK;
}

int32_t graph_compile(CSOUND *csound, GRAPH_COMPILE *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);

    if (g == NULL)
        return csound->InitError(csound, "[stm] stmcompile: invalid graph");
    if (g->node_count == 0)
        return csound->InitError(csound, "[stm] stmcompile: graph has no nodes");

    for (uint32_t i = 0; i < g->node_count; i++) {
        GRAPH_NODE *n = &g->nodes[i];
        for (uint32_t e = 0; e < n->edge_count; e++) {
            if (n->edges[e] >= g->node_count)
                return csound->InitError(csound, "[stm] stmcompile: invalid edge");
        }
    }

    g->compiled = 1;
    if (g->start_node == NO_NODE) g->start_node = 0;
    g->current_node = g->start_node; // entry node
    g->previous_node = NO_NODE;
    g->requested_node = NO_NODE;
    g->graph_tick = 0;
    g->node_tick_on_enter = 0;
    g->kperiod = FL(0.0);
    g->reset_pending = 0;

    return OK;
}

/* return the current node name (for orchestra-side dispatch) */
int32_t graph_current(CSOUND *csound, GRAPH_CURRENT *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);

    if (g == NULL || !g->compiled) {
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrent: graph not compiled");
    }
    if (g->current_node >= g->node_count) {
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrent: invalid current node");
    }

    const char *name = g->nodes[g->current_node].name;
    size_t len = strlen(name);
    if (len >= p->cur->size) {
        void *newp = csound->ReAlloc(csound, p->cur->data, len + 1);
        if (newp == NULL)
            return csound->PerfError(csound, &(p->h), "[stm] stmcurrent: memory error");
        p->cur->data = newp;
        p->cur->size = len + 1;
    }
    memcpy(p->cur->data, name, len + 1);
    return OK;
}

/* return the current node id (for orchestra-side dispatch) */
int32_t graph_current_id(CSOUND *csound, GRAPH_CURRENT_ID *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);

    if (g == NULL || !g->compiled) {
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrentid: graph not compiled");
    }
    if (g->current_node >= g->node_count) {
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrentid: invalid current node");
    }

    *p->cur = g->current_node;
    return OK;
}

/* apply the transition requested by the node (if it is a valid edge),
   return 1 if the current node changed, 0 otherwise */
int32_t graph_advance(CSOUND *csound, GRAPH_ADVANCE *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);

    if (g == NULL || !g->compiled) {
        return csound->PerfError(csound, &(p->h), "[stm] stmadvance: graph not compiled");
    }

    if (g->current_node >= g->node_count) {
        return csound->PerfError(csound, &(p->h), "[stm] stmadvance: invalid current node");
    }

    GRAPH_NODE *node = &g->nodes[g->current_node];
    int32_t changed = 0;

    if (g->requested_node != NO_NODE) {
        uint32_t target = g->requested_node;
        g->requested_node = NO_NODE;

        if (target < g->node_count && graph_has_edge(node, target)) {
            g->previous_node = g->current_node;
            g->current_node = target;
            changed = 1;
        }
    }

    g->kperiod = (MYFLT) CS_KSMPS / CS_ESR;
    if (g->reset_pending) {
        // graph restarted during this cycle: its clock starts on the next one
        g->reset_pending = 0;
    } else {
        g->graph_tick++;
        // node time is measured relative to the tick the node became current
        if (changed) g->node_tick_on_enter = g->graph_tick;
    }

    *p->changed = (MYFLT) changed;
    return OK;
}

int32_t graph_next(CSOUND *csound, GRAPH_NEXT *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL)
        return csound->PerfError(csound, &(p->h), "[stm] stmnext: invalid graph");

    int32_t requested_node = graph_find_node(g, p->next_node->data);
    if (requested_node < 0)
        return csound->PerfError(csound, &(p->h), "[stm] stmnext: node '%s' not found", p->next_node->data);

    g->requested_node = (uint32_t) requested_node;
    return OK;
}

int32_t graph_next_id(CSOUND *csound, GRAPH_NEXT_ID *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL)
        return csound->PerfError(csound, &(p->h), "[stm] stmnextid: invalid graph");

    int32_t requested_node = (int32_t) *p->next_node;
    if (requested_node < 0 || (uint32_t) requested_node >= g->node_count)
        return csound->PerfError(csound, &(p->h), "[stm] stmnextid: node '%d' not found", requested_node);

    g->requested_node = (uint32_t) requested_node;
    return OK;
}

int32_t graph_add_cond_edge(CSOUND *csound, GRAPH_ADD_COND_EDGE *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL) {
        return csound->InitError(csound, "[stm] stmaddcondedge: invalid graph");
    }

    if (g->compiled) {
        return csound->InitError(csound, "[stm] stmaddcondedge: graph already compiled (immutable)");
    }

    STRINGDAT *items = (STRINGDAT *) p->targets->data;
    int32_t ntargets = p->targets->sizes[0];
    for (int32_t i = 0; i < ntargets; i++) {
        if (add_edge_helper(csound, g, p->from->data, items[i].data) == NOTOK) {
            return csound->InitError(csound, "[stm] stmaddcondedge, something went wrong");
        }
    }

    return OK;
}

int32_t graph_on_ee_init(CSOUND *csound, GRAPH_ON_EE *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL) {
        return csound->InitError(csound, "[stm] on enter/exit: invalid graph");
    }

    int32_t n = graph_find_node(g, p->node->data);
    if (n < 0) {
        return csound->InitError(csound, "[stm] on enter/exit : node '%s' not found", p->node->data);
    }

    p->node_id = n;
    p->g = g;
    *p->trig = FL(0.0);
    p->was_current = 0;

    return OK;
}

/* rising-edge trigger: 1 only on the cycle this node becomes current */
int32_t graph_on_enter(CSOUND *csound, GRAPH_ON_EE *p) {
    GRAPH *g = p->g;
    if (g == NULL || !g->compiled) {
        return csound->PerfError(csound, &(p->h), "[stm] stmonenter: graph not compiled");
    }

    int32_t is_curr = g->current_node == (uint32_t) p->node_id;
    *p->trig = is_curr && !p->was_current ? FL(1.0) : FL(0.0);
    p->was_current = is_curr;
    return OK;
}

/* rising-edge trigger: 1 only on the cycle this node becomes not current */
int32_t graph_on_exit(CSOUND *csound, GRAPH_ON_EE *p) {
    GRAPH *g = p->g;
    if (g == NULL || !g->compiled) {
        return csound->PerfError(csound, &(p->h), "[stm] stmonexit: graph not compiled");
    }

    int32_t is_curr = g->current_node == (uint32_t) p->node_id;
    *p->trig = !is_curr && p->was_current ? FL(1.0) : FL(0.0);
    p->was_current = is_curr;
    return OK;
}


int32_t graph_node_id(CSOUND *csound, GRAPH_NODE_ID *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL) {
        return csound->PerfError(csound, &(p->h), "[stm] stmnodeid: not valid graph");
    }

    int32_t n = graph_find_node(g, p->node_name->data);
    if (n < 0) {
        return csound->PerfError(csound, &(p->h), "[stm] stmnodeid: node not found");
    }

    *p->node_id = (MYFLT) n;
    return OK;
}

int32_t graph_node_name(CSOUND *csound, GRAPH_NODE_NAME *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL) {
        return csound->PerfError(csound, &(p->h), "[stm] stmnodename: not valid graph");
    }

    uint32_t node_id = (uint32_t) *p->node_id;
    if (node_id >= g->node_count) {
        return csound->PerfError(csound, &(p->h), "[stm] stmnodename: invalid node id");
    }

    const char *name = g->nodes[node_id].name;
    size_t len = strlen(name);
    if (len >= p->node_name->size) {
        void *newp = csound->ReAlloc(csound, p->node_name->data, len + 1);
        if (newp == NULL)
            return csound->PerfError(csound, &(p->h), "[stm] stmnodename: memory error");
        p->node_name->data = newp;
        p->node_name->size = len + 1;
    }

    memcpy(p->node_name->data, name, len + 1);
    return OK;
}

int32_t graph_node_count(CSOUND *csound, GRAPH_NODE_COUNT *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL) {
        return csound->PerfError(csound, &(p->h), "[stm] stmnodecount: not valid graph");
    }

    *p->node_count = g->node_count;
    return OK;
}

int32_t graph_edge_count(CSOUND *csound, GRAPH_EDGE_COUNT *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL) {
        return csound->PerfError(csound, &(p->h), "[stm] stmedgecount: not valid graph");
    }

    uint32_t edge_count = 0;
    for (uint32_t i = 0; i < g->node_count; i++) {
        edge_count += g->nodes[i].edge_count;
    }

    *p->edge_count = edge_count;
    return OK;
}

int32_t graph_reset(CSOUND *csound, GRAPH_RESET *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL || !g->compiled) {
        return csound->PerfError(csound, &(p->h), "[stm] stmreset: graph not compiled");
    }

    g->current_node = g->start_node;
    g->previous_node = NO_NODE;
    g->requested_node = NO_NODE;
    g->graph_tick = 0;
    g->node_tick_on_enter = 0;
    g->kperiod = FL(0.0);
    g->reset_pending = 1;

    return OK;
}

int32_t graph_entry(CSOUND *csound, GRAPH_ENTRY *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL) {
        return csound->InitError(csound, "[stm] stmentry: not valid graph");
    }

    if (g->compiled) {
        return csound->InitError(csound, "[stm] stmentry: graph already compiled. Move entry before stmcompile");
    }

    int32_t n = graph_find_node(g, p->entry_node->data);
    if (n < 0) {
        return csound->InitError(csound, "[stm] stmentry: node not found");
    }

    g->start_node = (uint32_t) n;
    return OK;
}

/* k-cycles since compile/reset: one tick per stmadvance call, so the graph
   clock stops whenever the machine is not being stepped */
int32_t graph_time_tick(CSOUND *csound, GRAPH_TIME *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL || !g->compiled) {
        return csound->PerfError(csound, &(p->h), "[stm] stmtick: graph not compiled");
    }

    *p->t = (MYFLT) g->graph_tick;
    return OK;
}

/* graph time in seconds: derived from the tick count, never accumulated, so it
   cannot drift. This is graph time, not performance time: use times() for that */
int32_t graph_time_global(CSOUND *csound, GRAPH_TIME *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL || !g->compiled) {
        return csound->PerfError(csound, &(p->h), "[stm] stmtime: graph not compiled");
    }

    *p->t = (MYFLT) g->graph_tick * g->kperiod;
    return OK;
}

/* seconds since the current node became current: 0 on the node's own first
   cycle, and unaffected by a stmnext that stmadvance rejected */
int32_t graph_time_node(CSOUND *csound, GRAPH_TIME *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL || !g->compiled) {
        return csound->PerfError(csound, &(p->h), "[stm] stmnodetime: graph not compiled");
    }

    *p->t = (MYFLT) (g->graph_tick - g->node_tick_on_enter) * g->kperiod;
    return OK;
}


// CSOUND OP-INTER

#define S(x) sizeof(x)

static OENTRY stm[] = {
    { "stmcreate",      S(GRAPH_CREATE),        0, "i", "",      (SUBR) graph_create,        NULL,                     (SUBR) graph_create_deinit },
    { "stmaddnode",     S(GRAPH_ADD_NODE),      0, "",  "iS",    (SUBR) graph_add_node,      NULL,                     NULL },
    { "stmaddedge",     S(GRAPH_ADD_EDGE),      0, "",  "iSS",   (SUBR) graph_add_edge,      NULL,                     NULL },
    { "stmaddcondedge", S(GRAPH_ADD_COND_EDGE), 0, "",  "iSS[]", (SUBR) graph_add_cond_edge, NULL,                     NULL },
    { "stmcompile",     S(GRAPH_COMPILE),       0, "",  "i",     (SUBR) graph_compile,       NULL,                     NULL },
    { "stmcurrent",     S(GRAPH_CURRENT),       0, "S", "i",     NULL,                       (SUBR) graph_current,     NULL },
    { "stmcurrentid",   S(GRAPH_CURRENT_ID),    0, "k", "i",     NULL,                       (SUBR) graph_current_id,  NULL },
    { "stmadvance",     S(GRAPH_ADVANCE),       0, "k", "i",     NULL,                       (SUBR) graph_advance,     NULL },
    { "stmnext",        S(GRAPH_NEXT),          0, "",  "iS",    NULL,                       (SUBR) graph_next,        NULL },
    { "stmnext.id",     S(GRAPH_NEXT_ID),       0, "",  "ik",    NULL,                       (SUBR) graph_next_id,     NULL },
    { "stmonenter",     S(GRAPH_ON_EE),         0, "k", "iS",    (SUBR) graph_on_ee_init,    (SUBR) graph_on_enter,    NULL },
    { "stmonexit",      S(GRAPH_ON_EE),         0, "k", "iS",    (SUBR) graph_on_ee_init,    (SUBR) graph_on_exit,     NULL },
    { "stmnodename",    S(GRAPH_NODE_NAME),     0, "S", "ik",    NULL,                       (SUBR) graph_node_name,   NULL },
    { "stmnodeid",      S(GRAPH_NODE_ID),       0, "k", "iS",    NULL,                       (SUBR) graph_node_id,     NULL },
    { "stmnodecount",   S(GRAPH_NODE_COUNT),    0, "k", "i",     NULL,                       (SUBR) graph_node_count,  NULL },
    { "stmedgecount",   S(GRAPH_EDGE_COUNT),    0, "k", "i",     NULL,                       (SUBR) graph_edge_count,  NULL },
    { "stmreset",       S(GRAPH_RESET),         0, "",  "i",     NULL,                       (SUBR) graph_reset,       NULL },
    { "stmentry",       S(GRAPH_ENTRY),         0, "",  "iS",    (SUBR) graph_entry,         NULL,                     NULL },
    { "stmtick",        S(GRAPH_TIME),          0, "k", "i",     NULL,                       (SUBR) graph_time_tick,   NULL },
    { "stmnodetime",    S(GRAPH_TIME),          0, "k", "i",     NULL,                       (SUBR) graph_time_node,   NULL },
    { "stmtime",        S(GRAPH_TIME),          0, "k", "i",     NULL,                       (SUBR) graph_time_global, NULL },
};

int32_t stm_init_(CSOUND *csound) {
    return csound->AppendOpcodes(csound, &(stm[0]), (int32_t) (sizeof(stm) / sizeof(OENTRY)));
}
