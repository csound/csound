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
 graphs inspired by LangGraph. Nodes are executable Opcode objects operating
 on a shared struct passed by reference, while edges define valid transitions
 between computations. The graph runtime executes the current node, applies in-place
 state updates, validates transitions, and advances deterministically at k-rate,
 cleanly separating control logic from audio DSP.
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

    *p->handle = (MYFLT) handle;
    return OK;
}

int32_t graph_add_node(CSOUND *csound, GRAPH_ADD_NODE *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);
    if (g == NULL)
        return csound->InitError(csound, "[stm] stmaddnode: invalid graph");
    if (g->compiled)
        return csound->InitError(csound, "[stm] stmaddnode: graph already compiled (immutable)");
    if (graph_find_node(g, p->node_name->data) >= 0)
        return csound->InitError(csound, "[stm] stmaddnode: duplicate node name '%s'", p->node_name->data);

    if (g->node_count == g->node_capacity) {
        uint32_t newcap = g->node_capacity * 2;
        GRAPH_NODE *grown = csound->ReAlloc(csound, g->nodes, sizeof(GRAPH_NODE) * newcap);
        if (grown == NULL)
            return csound->InitError(csound, "[stm] stmaddnode: memory error");
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
    g->current_node = 0; // entry node = first added
    g->previous_node = NO_NODE;
    g->requested_node = NO_NODE;

    return OK;
}

/* return the current node name (for orchestra-side dispatch) */
int32_t graph_current(CSOUND *csound, GRAPH_CURRENT *p) {
    GRAPH *g = stm_handle_to_graph(csound, *p->handle);

    if (g == NULL || !g->compiled)
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrent: graph not compiled");
    if (g->current_node >= g->node_count)
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrent: invalid current node");

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



#define S(x) sizeof(x)

static OENTRY stm[] = {
    { "stmcreate",      S(GRAPH_CREATE),        0, "i", "",      (SUBR) graph_create,        NULL,                  (SUBR) graph_create_deinit },
    { "stmaddnode",     S(GRAPH_ADD_NODE),      0, "",  "iS",    (SUBR) graph_add_node,      NULL,                  NULL },
    { "stmaddedge",     S(GRAPH_ADD_EDGE),      0, "",  "iSS",   (SUBR) graph_add_edge,      NULL,                  NULL },
    { "stmaddcondedge", S(GRAPH_ADD_COND_EDGE), 0, "",  "iSS[]", (SUBR) graph_add_cond_edge, NULL,                  NULL },
    { "stmcompile",     S(GRAPH_COMPILE),       0, "",  "i",     (SUBR) graph_compile,       NULL,                  NULL },
    { "stmcurrent",     S(GRAPH_CURRENT),       0, "S", "i",     NULL,                       (SUBR) graph_current,  NULL },
    { "stmadvance",     S(GRAPH_ADVANCE),       0, "k", "i",     NULL,                       (SUBR) graph_advance,  NULL },
    { "stmnext",        S(GRAPH_NEXT),          0, "",  "iS",    NULL,                       (SUBR) graph_next,     NULL },
    { "stmonenter",     S(GRAPH_ON_EE),         0, "k", "iS",    (SUBR) graph_on_ee_init,    (SUBR) graph_on_enter, NULL },
    { "stmonexit",      S(GRAPH_ON_EE),         0, "k", "iS",    (SUBR) graph_on_ee_init,    (SUBR) graph_on_exit,  NULL },
};

int32_t stm_init_(CSOUND *csound) {
    return csound->AppendOpcodes(csound, &(stm[0]), (int32_t) (sizeof(stm) / sizeof(OENTRY)));
}
