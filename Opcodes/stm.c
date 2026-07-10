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

/* NOTE: Ownership/lifetime notes:
   - the STM_REGISTRY struct itself is stored as a Csound global variable, so
     its natural owner is the CSOUND instance;
   - each GRAPH is owned by the stmcreate opcode instance that created it, and
     graph_create_deinit frees it, then clears the registry slot;
   - reusing NULL registry slots keeps the table compact. Handles encode both
     the slot index and the slot generation, so a stale numeric handle cannot
     become valid again just because the same slot was reused. */

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

static int32_t stm_myflt_to_uint32(MYFLT value, uint32_t max_exclusive,
        uint32_t *out) {
    /* Check range before casting: out-of-range float-to-int conversion is
       undefined, and the negated >= rejects NaN. */
    if (!(value >= FL(0.0)) || value >= (MYFLT) max_exclusive) return 0;

    uint32_t converted = (uint32_t) value;
    if ((MYFLT) converted != value) return 0;

    *out = converted;
    return 1;
}

/* Register a graph, returns an encoded handle (0 on failure).
   The first generation preserves the old handles: slot 0 -> handle 1. */
static uint32_t stm_register_graph(CSOUND *csound, GRAPH *g, uint32_t *slot_out, uint32_t *generation_out) {
    STM_REGISTRY *reg = stm_registry_query(csound);
    if (reg == NULL) {
        if (csound->CreateGlobalVariable(csound, STM_REGISTRY_NAME, sizeof(STM_REGISTRY)) != 0) {
            return 0;
        }
        reg = stm_registry_query(csound);
        if (reg == NULL) { return 0; }
        reg->capacity = INITIAL_GRAPH_CAPACITY;
        reg->count = 0;
        reg->slots = csound->Calloc(csound, sizeof(STM_REGISTRY_SLOT) * reg->capacity);
        if (reg->slots == NULL) { return 0; }
        reg->mutex = csound->Create_Mutex(0);
        if (reg->mutex == NULL) { return 0; }
    }

    stm_registry_lock(csound, reg);

    /* reuse free slot; generation 0 means the slot exhausted its handle space */
    for (uint32_t i = 0; i < reg->count; i++) {
        STM_REGISTRY_SLOT *slot = &reg->slots[i];
        if (slot->graph == NULL && slot->generation > 0 && slot->generation <= STM_HANDLE_GENERATION_MAX) {
            slot->graph = g;
            *slot_out = i;
            *generation_out = slot->generation;
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
    reg->slots[slot].graph = g;
    reg->slots[slot].generation = 1;
    *slot_out = slot;
    *generation_out = 1;
    uint32_t handle = stm_make_handle(slot, 1);
    stm_registry_unlock(csound, reg);
    return handle;
}

static GRAPH *stm_handle_to_graph_locked(STM_REGISTRY *reg, MYFLT handle) {
    uint32_t slot;
    uint32_t generation;
    if (!stm_decode_handle(handle, &slot, &generation)) return NULL;
    if (slot >= reg->count) return NULL;
    if (reg->slots[slot].generation != generation) return NULL;
    return reg->slots[slot].graph;
}

static int32_t stm_get_graph_init_locked(CSOUND *csound, OPDS *opds, MYFLT handle, const char *msg, STM_REGISTRY **reg_out, GRAPH **g_out) {
    STM_REGISTRY *reg = stm_registry_query(csound);
    if (reg == NULL) {
        return csound->InitError(csound, "%s", msg);
    }

    stm_registry_lock(csound, reg);
    GRAPH *g = stm_handle_to_graph_locked(reg, handle);
    if (g == NULL) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "%s", msg);
    }

    (void) opds;
    *reg_out = reg;
    *g_out = g;
    return OK;
}

static int32_t stm_get_graph_perf_locked(CSOUND *csound, OPDS *opds, MYFLT handle, const char *msg, STM_REGISTRY **reg_out, GRAPH **g_out) {
    STM_REGISTRY *reg = stm_registry_query(csound);
    if (reg == NULL) {
        return csound->PerfError(csound, opds, "%s", msg);
    }

    stm_registry_lock(csound, reg);
    GRAPH *g = stm_handle_to_graph_locked(reg, handle);
    if (g == NULL) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, opds, "%s", msg);
    }

    *reg_out = reg;
    *g_out = g;
    return OK;
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
    GRAPH *g = p->graph;
    if (g != NULL) {
        STM_REGISTRY *reg = stm_registry_query(csound);
        stm_registry_lock(csound, reg);

        if (g->nodes != NULL) {
            for (uint32_t i = 0; i < g->node_count; i++) {
                csound->Free(csound, g->nodes[i].edges);
                csound->Free(csound, g->nodes[i].name);
            }
            csound->Free(csound, g->nodes);
        }
        // null the registry slot so the handle can no longer resolve
        if (reg != NULL && p->slot < reg->count) {
            STM_REGISTRY_SLOT *slot = &reg->slots[p->slot];
            if (slot->graph == g && slot->generation == p->generation) {
                slot->graph = NULL;
                if (slot->generation < STM_HANDLE_GENERATION_MAX) {
                    slot->generation++;
                } else {
                    slot->generation = 0;
                }
            }
        }
        csound->Free(csound, g);
        stm_registry_unlock(csound, reg);
        p->graph = NULL;
        p->slot = 0;
        p->generation = 0;
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

    uint32_t slot;
    uint32_t generation;
    uint32_t handle = stm_register_graph(csound, g, &slot, &generation);
    if (handle == 0) {
        csound->Free(csound, g->nodes);
        csound->Free(csound, g);
        return csound->InitError(csound, "[stm] stmcreate: registry error");
    }

    g->start_node = NO_NODE;
    p->graph = g;
    p->slot = slot;
    p->generation = generation;
    *p->handle = (MYFLT) handle;
    return OK;
}

int32_t graph_add_node(CSOUND *csound, GRAPH_ADD_NODE *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_init_locked(csound, &(p->h), *p->handle,
            "[stm] stmaddnode: invalid graph", &reg, &g) != OK) return NOTOK;
    if (g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddnode: graph already compiled (immutable)");
    }
    if (graph_find_node(g, p->node_name->data) >= 0) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddnode: duplicate node name '%s'", p->node_name->data);
    }

    if (g->node_count == g->node_capacity) {
        uint32_t newcap = g->node_capacity * 2;
        GRAPH_NODE *grown = csound->ReAlloc(csound, g->nodes, sizeof(GRAPH_NODE) * newcap);
        if (grown == NULL) {
            stm_registry_unlock(csound, reg);
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

    g->node_count++;
    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_add_edge(CSOUND *csound, GRAPH_ADD_EDGE *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_init_locked(csound, &(p->h), *p->handle, "[stm] stmaddedge: invalid graph", &reg, &g) != OK) {
        return NOTOK;
    }

    if (g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddedge: graph already compiled (immutable)");
    }

    if (add_edge_helper(csound, g, p->from->data, p->to->data) == NOTOK) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddedge, something went wrong");
    }

    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_compile(CSOUND *csound, GRAPH_COMPILE *p) {
    STM_REGISTRY *reg;
    GRAPH *g;

    if (stm_get_graph_init_locked(csound, &(p->h), *p->handle, "[stm] stmcompile: invalid graph", &reg, &g) != OK) {
        return NOTOK;
    }

    if (g->node_count == 0) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmcompile: graph has no nodes");
    }

    for (uint32_t i = 0; i < g->node_count; i++) {
        GRAPH_NODE *n = &g->nodes[i];
        for (uint32_t e = 0; e < n->edge_count; e++) {
            if (n->edges[e] >= g->node_count) {
                stm_registry_unlock(csound, reg);
                return csound->InitError(csound, "[stm] stmcompile: invalid edge");
            }
        }
    }

    g->compiled = 1;
    if (g->start_node == NO_NODE) g->start_node = 0;
    g->current_node = g->start_node; // entry node
    g->previous_node = NO_NODE;
    g->requested_node = NO_NODE;
    g->graph_tick = 0;
    g->total_sample_frames = 0;
    g->node_sample_on_enter = 0;
    g->reset_pending = 0;
    g->event_seq = 1;
    g->event_entered_node = g->current_node;
    g->event_exited_node = NO_NODE;

    stm_registry_unlock(csound, reg);
    return OK;
}

/* return the current node name (for orchestra-side dispatch) */
int32_t graph_current(CSOUND *csound, GRAPH_CURRENT *p) {
    STM_REGISTRY *reg;
    GRAPH *g;

    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmcurrent: graph not compiled", &reg, &g) != OK) {
        return NOTOK;
    }

    if (!g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrent: graph not compiled");
    }
    if (g->current_node >= g->node_count) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrent: invalid current node");
    }

    const char *name = g->nodes[g->current_node].name;
    size_t len = strlen(name);
    if (len >= p->cur->size) {
        void *newp = csound->ReAlloc(csound, p->cur->data, len + 1);
        if (newp == NULL) {
            stm_registry_unlock(csound, reg);
            return csound->PerfError(csound, &(p->h), "[stm] stmcurrent: memory error");
        }
        p->cur->data = newp;
        p->cur->size = len + 1;
    }
    memcpy(p->cur->data, name, len + 1);
    stm_registry_unlock(csound, reg);
    return OK;
}

/* return the current node id (for orchestra-side dispatch) */
int32_t graph_current_id(CSOUND *csound, GRAPH_CURRENT_ID *p) {
    STM_REGISTRY *reg;
    GRAPH *g;

    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmcurrentid: graph not compiled", &reg, &g) != OK) {
        return NOTOK;
    }

    if (!g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrentid: graph not compiled");
    }
    if (g->current_node >= g->node_count) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmcurrentid: invalid current node");
    }

    *p->cur = g->current_node;
    stm_registry_unlock(csound, reg);
    return OK;
}

/* apply the transition requested by the node (if it is a valid edge),
   return 1 if the current node changed, 0 otherwise */
int32_t graph_advance(CSOUND *csound, GRAPH_ADVANCE *p) {
    STM_REGISTRY *reg;
    GRAPH *g;

    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmadvance: graph not compiled", &reg, &g) != OK) {
        return NOTOK;
    }

    if (!g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmadvance: graph not compiled");
    }

    if (g->current_node >= g->node_count) {
        stm_registry_unlock(csound, reg);
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
            g->event_seq++;
            g->event_exited_node = g->previous_node;
            g->event_entered_node = target;
            changed = 1;
        }
    }

    if (g->reset_pending) {
        // graph restarted during this cycle: its clock starts on the next one
        g->reset_pending = 0;
    } else {
        g->graph_tick++;
        g->total_sample_frames += (uint64_t) CS_KSMPS;
        if (changed) {
            g->node_sample_on_enter = g->total_sample_frames;
        }
    }

    *p->changed = (MYFLT) changed;
    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_next(CSOUND *csound, GRAPH_NEXT *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle,
            "[stm] stmnext: invalid graph", &reg, &g) != OK) return NOTOK;

    int32_t requested_node = graph_find_node(g, p->next_node->data);
    if (requested_node < 0) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmnext: node '%s' not found", p->next_node->data);
    }

    g->requested_node = (uint32_t) requested_node;
    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_next_id(CSOUND *csound, GRAPH_NEXT_ID *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmnextid: invalid graph", &reg, &g) != OK) {
        return NOTOK;
    }

    uint32_t requested_node;
    if (!stm_myflt_to_uint32(*p->next_node, g->node_count, &requested_node)) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmnextid: invalid node id");
    }

    g->requested_node = requested_node;
    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_add_cond_edge(CSOUND *csound, GRAPH_ADD_COND_EDGE *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_init_locked(csound, &(p->h), *p->handle, "[stm] stmaddcondedge: invalid graph", &reg, &g) != OK) {
        return NOTOK;
    }

    if (g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmaddcondedge: graph already compiled (immutable)");
    }

    STRINGDAT *items = (STRINGDAT *) p->targets->data;
    int32_t ntargets = p->targets->sizes[0];
    for (int32_t i = 0; i < ntargets; i++) {
        if (add_edge_helper(csound, g, p->from->data, items[i].data) == NOTOK) {
            stm_registry_unlock(csound, reg);
            return csound->InitError(csound, "[stm] stmaddcondedge, something went wrong");
        }
    }

    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_on_ee_init(CSOUND *csound, GRAPH_ON_EE *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_init_locked(csound, &(p->h), *p->handle, "[stm] on enter/exit: invalid graph", &reg, &g) != OK) {
        return NOTOK;
    }

    int32_t n = graph_find_node(g, p->node->data);
    if (n < 0) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] on enter/exit : node '%s' not found", p->node->data);
    }

    p->node_id = n;
    *p->trig = FL(0.0);
    p->last_seen_event_seq = g->event_seq;

    stm_registry_unlock(csound, reg);
    return OK;
}

/* rising-edge trigger: 1 only on the cycle this node becomes current */
int32_t graph_on_enter(CSOUND *csound, GRAPH_ON_EE *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmonenter: invalid graph (freed?)", &reg, &g) != OK) {
        return NOTOK;
    }

    if (!g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmonenter: graph not compiled");
    }

    int32_t entered = g->event_seq > p->last_seen_event_seq && g->event_entered_node == (uint32_t) p->node_id;
    *p->trig = entered ? FL(1.0) : FL(0.0);
    p->last_seen_event_seq = g->event_seq;
    stm_registry_unlock(csound, reg);
    return OK;
}

/* rising-edge trigger: 1 only on the cycle this node becomes not current */
int32_t graph_on_exit(CSOUND *csound, GRAPH_ON_EE *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmonexit: invalid graph (freed?)", &reg, &g) != OK) {
        return NOTOK;
    }

    if (!g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmonexit: graph not compiled");
    }

    int32_t exited = g->event_seq > p->last_seen_event_seq && g->event_exited_node == (uint32_t) p->node_id;
    *p->trig = exited ? FL(1.0) : FL(0.0);
    p->last_seen_event_seq = g->event_seq;
    stm_registry_unlock(csound, reg);
    return OK;
}


int32_t graph_node_id(CSOUND *csound, GRAPH_NODE_ID *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmnodeid: not valid graph", &reg, &g) != OK) {
        return NOTOK;
    }

    int32_t n = graph_find_node(g, p->node_name->data);
    if (n < 0) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmnodeid: node not found");
    }

    *p->node_id = (MYFLT) n;
    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_node_name(CSOUND *csound, GRAPH_NODE_NAME *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmnodename: not valid graph", &reg, &g) != OK) {
        return NOTOK;
    }

    uint32_t node_id;
    if (!stm_myflt_to_uint32(*p->node_id, g->node_count, &node_id)) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmnodename: invalid node id");
    }

    const char *name = g->nodes[node_id].name;
    size_t len = strlen(name);
    if (len >= p->node_name->size) {
        void *newp = csound->ReAlloc(csound, p->node_name->data, len + 1);
        if (newp == NULL) {
            stm_registry_unlock(csound, reg);
            return csound->PerfError(csound, &(p->h), "[stm] stmnodename: memory error");
        }
        p->node_name->data = newp;
        p->node_name->size = len + 1;
    }

    memcpy(p->node_name->data, name, len + 1);
    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_node_count(CSOUND *csound, GRAPH_NODE_COUNT *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmnodecount: not valid graph", &reg, &g) != OK) {
        return NOTOK;
    }

    *p->node_count = g->node_count;
    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_edge_count(CSOUND *csound, GRAPH_EDGE_COUNT *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmedgecount: not valid graph", &reg, &g) != OK) {
        return NOTOK;
    }

    uint32_t edge_count = 0;
    for (uint32_t i = 0; i < g->node_count; i++) {
        edge_count += g->nodes[i].edge_count;
    }

    *p->edge_count = edge_count;
    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_reset(CSOUND *csound, GRAPH_RESET *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmreset: graph not compiled", &reg, &g) != OK) {
        return NOTOK;
    }

    if (!g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmreset: graph not compiled");
    }

    uint32_t old_node = g->current_node;
    g->current_node = g->start_node;
    g->previous_node = NO_NODE;
    g->requested_node = NO_NODE;
    g->graph_tick = 0;
    g->total_sample_frames = 0;
    g->node_sample_on_enter = 0;
    g->reset_pending = 1;
    g->event_seq++;
    g->event_exited_node = old_node != g->start_node ? old_node : NO_NODE;
    g->event_entered_node = g->start_node;

    stm_registry_unlock(csound, reg);
    return OK;
}

int32_t graph_entry(CSOUND *csound, GRAPH_ENTRY *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_init_locked(csound, &(p->h), *p->handle, "[stm] stmentry: not valid graph", &reg, &g) != OK) {
        return NOTOK;
    }

    if (g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmentry: graph already compiled. Move entry before stmcompile");
    }

    int32_t n = graph_find_node(g, p->entry_node->data);
    if (n < 0) {
        stm_registry_unlock(csound, reg);
        return csound->InitError(csound, "[stm] stmentry: node not found");
    }

    g->start_node = (uint32_t) n;
    stm_registry_unlock(csound, reg);
    return OK;
}

/* k-cycles since compile/reset: one tick per stmadvance call, so the graph
   clock stops whenever the machine is not being stepped */
int32_t graph_time_tick(CSOUND *csound, GRAPH_TIME *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmtick: graph not compiled", &reg, &g) != OK) {
        return NOTOK;
    }

    if (!g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmtick: graph not compiled");
    }

    *p->t = (MYFLT) g->graph_tick;
    stm_registry_unlock(csound, reg);
    return OK;
}

/* graph time in seconds: derived from the integer count of sample frames
   advanced by stmadvance, never accumulated as floating-point seconds */
int32_t graph_time_global(CSOUND *csound, GRAPH_TIME *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmtime: graph not compiled", &reg, &g) != OK) {
        return NOTOK;
    }

    if (!g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmtime: graph not compiled");
    }

    *p->t = (MYFLT) g->total_sample_frames / CS_ESR;
    stm_registry_unlock(csound, reg);
    return OK;
}

/* seconds since the current node became current: 0 on the node's own first
   cycle, and unaffected by a stmnext that stmadvance rejected */
int32_t graph_time_node(CSOUND *csound, GRAPH_TIME *p) {
    STM_REGISTRY *reg;
    GRAPH *g;
    if (stm_get_graph_perf_locked(csound, &(p->h), *p->handle, "[stm] stmnodetime: graph not compiled", &reg, &g) != OK) {
        return NOTOK;
    }

    if (!g->compiled) {
        stm_registry_unlock(csound, reg);
        return csound->PerfError(csound, &(p->h), "[stm] stmnodetime: graph not compiled");
    }

    *p->t = (MYFLT) (g->total_sample_frames - g->node_sample_on_enter) / CS_ESR;
    stm_registry_unlock(csound, reg);
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
    { "stmtime",        S(GRAPH_TIME),          0, "k", "i",     NULL,                       (SUBR) graph_time_global, NULL }
};

int32_t stm_init_(CSOUND *csound) {
    return csound->AppendOpcodes(csound, &(stm[0]), (int32_t) (sizeof(stm) / sizeof(OENTRY)));
}
