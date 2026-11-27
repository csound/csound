/*
**  cs_new_dispatch.c
**
**    Copyright (C)  Martin Brain (mjb@cs.bath.ac.uk) 04/08/12
**    Realisation in code for Csound John ffitch Feb 2013
**
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


** Fast system for managing task dependencies and dispatching to threads.
**
** Has a DAG of tasks and has to assign them to worker threads while respecting
** dependency order.
**
** OPT marks code relevant to particular optimisations (listed below the code).
** INV marks invariants
** NOTE marks notes
*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include "csoundCore.h"
#include "cs_par_base.h"
#include "interlocks.h"
#include "entry.h"
#include "opcode.h"
#include "udo.h"
#include <stdbool.h>

#if defined(_MSC_VER)
/* For InterlockedCompareExchange */
#include <windows.h>
#endif

/* Used as an error value */
//typedef int32_t taskID;
#define INVALID (-1)
#define WAIT    (-2)

/* Each task has a status */
//enum state { WAITING = 3,          /* Dependencies have not been finished */
//           AVAILABLE = 2,        /* Dependencies met, ready to be run */
//           INPROGRESS = 1,       /* Has been started */
//           DONE = 0 };           /* Has been completed */

/* Sets of prerequiste tasks for each task */
//typedef struct _watchList {
//  taskID id;
//  struct _watchList *next;
//} watchList;

/* Array of states of each task -- need to move to CSOUND structure */
//static enum state *task_status = NULL;          /* OPT : Structure lay out */
//static watchList **task_watch = NULL;
//static INSDS **task_map = NULL;

/* INV : Read by multiple threads, updated by only one */
/* Thus use atomic read and write */

//static char ** task_dep;                        /* OPT : Structure lay out */
//static watchList * wlmm;

#define INIT_SIZE (100)
//static int32_t task_max_size;

static void dag_print_state(CSOUND *csound)
{
    int32_t i;
    watchList *w;
    printf("*** %d tasks\n", csound->dag_num_active);
    for (i=0; i<csound->dag_num_active; i++) {
      printf("%d(%d): ", i, csound->dag_task_map[i]->insno);
      switch (csound->dag_task_status[i].s) {
      case DONE:
        printf("status=DONE (watchList ");
        w = csound->dag_task_watch[i];
        while (w) { printf("%d ", w->id); w=w->next; }
        printf(")\n");
        break;
      case INPROGRESS:
        printf("status=INPROGRESS (watchList ");
        w = csound->dag_task_watch[i];
        while (w) { printf("%d ", w->id); w=w->next; }
        printf(")\n");
        break;
      case AVAILABLE:
        printf("status=AVAILABLE (watchList ");
        w = csound->dag_task_watch[i];
        while (w) { printf("%d ", w->id); w=w->next; }
        printf(")\n");
        break;
      case WAITING:
        {
          char *tt = csound->dag_task_dep[i];
          int32_t j;
          printf("status=WAITING for tasks [");
          if (tt != NULL) {
            for (j=0; j<i; j++) if (tt[j]) printf("%d ", j);
          }
          printf("]\n");
        }
        break;
      default:
        printf("status=???\n"); break;
      }
    }
}

/* For now allocate a fixed maximum number of tasks; FIXME */
static void create_dag(CSOUND *csound, int32_t max)
{
    /* Allocate the main task status and watchlists */
    csound->dag_task_status = csound->Calloc(csound, sizeof(stateWithPadding)*max);
    csound->dag_task_watch  = csound->Calloc(csound, sizeof(watchList*)*max);
    csound->dag_task_map    = csound->Calloc(csound, sizeof(INSDS*)*max);
    csound->dag_task_dep    = (char **)csound->Calloc(csound, sizeof(char*)*max);
    csound->dag_wlmm = (watchList *)csound->Calloc(csound, sizeof(watchList)*max);
    csound->dag_task_max_size = max;
}

static void recreate_dag(CSOUND *csound, int32_t old_max, int32_t max)
{
    /* Allocate the main task status and watchlists */
    csound->dag_task_status =
      csound->ReAlloc(csound, (void *)csound->dag_task_status,
               sizeof(stateWithPadding)*max);
    csound->dag_task_watch  =
      csound->ReAlloc(csound, (void *)csound->dag_task_watch,
               sizeof(watchList*)*max);
    csound->dag_task_map    =
      csound->ReAlloc(csound, (void *)csound->dag_task_map, sizeof(INSDS*)*max);
    csound->dag_task_dep    =
      (char **)csound->ReAlloc(csound, csound->dag_task_dep, sizeof(char*)*max);
    csound->dag_wlmm        =
      (watchList *)csound->ReAlloc(csound, csound->dag_wlmm, sizeof(watchList)*max);
    if (max > old_max) {
      size_t added = (size_t)(max - old_max);
      memset((void *)(csound->dag_task_status + old_max), 0,
             sizeof(stateWithPadding) * added);
      memset((void *)(csound->dag_task_watch + old_max), 0,
             sizeof(watchList *) * added);
      memset((void *)(csound->dag_task_map + old_max), 0,
             sizeof(INSDS *) * added);
      memset((void *)(csound->dag_task_dep + old_max), 0,
             sizeof(char *) * added);
      memset((void *)(csound->dag_wlmm + old_max), 0,
             sizeof(watchList) * added);
    }
    csound->dag_task_max_size = max;
}

void dag_cleanup(CSOUND *csound)
{
    int32_t i;
    if (csound->dag_task_dep != NULL) {
      for (i = 0; i < csound->dag_task_max_size; i++) {
        if (csound->dag_task_dep[i] != NULL) {
          csound->Free(csound, csound->dag_task_dep[i]);
          csound->dag_task_dep[i] = NULL;
        }
      }
      csound->Free(csound, csound->dag_task_dep);
      csound->dag_task_dep = NULL;
    }
    if (csound->dag_task_status != NULL) {
      csound->Free(csound, (void *)csound->dag_task_status);
      csound->dag_task_status = NULL;
    }
    if (csound->dag_task_watch != NULL) {
      csound->Free(csound, (void *)csound->dag_task_watch);
      csound->dag_task_watch = NULL;
    }
    if (csound->dag_task_map != NULL) {
      csound->Free(csound, (void *)csound->dag_task_map);
      csound->dag_task_map = NULL;
    }
    if (csound->dag_wlmm != NULL) {
      csound->Free(csound, csound->dag_wlmm);
      csound->dag_wlmm = NULL;
    }
    csound->dag_task_max_size = 0;
    csound->dag_num_active = 0;
}

#define DAG_RES_ZAK (1u << 0)
#define DAG_RES_TAB (1u << 1)
#define DAG_RES_CHN (1u << 2)
#define DAG_RES_INT (1u << 3)
#define DAG_RES_STK (1u << 4)

typedef struct dag_ptr_pools {
    void **reads;
    int32_t read_used, read_cap, read_overflow;
    void **writes;
    int32_t write_used, write_cap, write_overflow;
} DAG_PTR_POOLS;

typedef struct dag_instr_deps {
    uint32_t read_res;
    uint32_t write_res;
    int32_t requires_order;
    int32_t has_udo_call;
    int32_t read_off, nreads;
    int32_t write_off, nwrites;
} DAG_INSTR_DEPS;

static inline const DAG_ACTIVE_SNAPSHOT *dag_load_active_snapshot_raw(
    const CSOUND *csound)
{
#if defined(_MSC_VER)
    return (const DAG_ACTIVE_SNAPSHOT *)InterlockedCompareExchangePointer(
        (PVOID volatile *)&csound->dag_active_snapshot, NULL, NULL);
#elif defined(HAVE_ATOMIC_BUILTIN)
    return __atomic_load_n(&csound->dag_active_snapshot, __ATOMIC_SEQ_CST);
#else
    return csound->dag_active_snapshot;
#endif
}

static inline void dag_store_active_snapshot(CSOUND *csound,
                                             DAG_ACTIVE_SNAPSHOT *snap)
{
#if defined(_MSC_VER)
    (void)InterlockedExchangePointer(
        (PVOID volatile *)&csound->dag_active_snapshot, (PVOID)snap);
#elif defined(HAVE_ATOMIC_BUILTIN)
    __atomic_store_n(&csound->dag_active_snapshot, snap, __ATOMIC_SEQ_CST);
#else
    csound->dag_active_snapshot = snap;
#endif
}

const DAG_ACTIVE_SNAPSHOT *dag_load_active_snapshot(CSOUND *csound)
{
    if (csound == NULL)
      return NULL;
    return dag_load_active_snapshot_raw(csound);
}

static void dag_pin_snapshot_inactive_nodes(const DAG_ACTIVE_SNAPSHOT *snap,
                                            long epoch)
{
    int32_t i;
    if (snap == NULL || (epoch & 1L) == 0)
      return;
    for (i = 0; i < snap->count; i++) {
      INSDS *item = snap->items[i];
      if (item != NULL && !ATOMIC_GET8(item->actflg)) {
        ATOMIC_SET(item->retired_epoch, epoch);
      }
    }
}

static void dag_reclaim_active_snapshots_locked(CSOUND *csound)
{
    long current_epoch = ATOMIC_GET(csound->performance_epoch);
    DAG_ACTIVE_SNAPSHOT **link = &csound->dag_active_retired;
    while (*link != NULL) {
      DAG_ACTIVE_SNAPSHOT *cur = *link;
      if ((cur->retired_epoch & 1L) == 0 ||
          cur->retired_epoch != current_epoch) {
        *link = cur->next_retired;
        free(cur);
      } else {
        link = &cur->next_retired;
      }
    }
}

static void dag_retire_active_snapshot_locked(CSOUND *csound,
                                              DAG_ACTIVE_SNAPSHOT *old_snap)
{
    long epoch;
    if (old_snap == NULL)
      return;
    epoch = ATOMIC_GET(csound->performance_epoch);
    if ((epoch & 1L) != 0) {
      dag_pin_snapshot_inactive_nodes(old_snap, epoch);
      old_snap->retired_epoch = epoch;
      old_snap->next_retired = csound->dag_active_retired;
      csound->dag_active_retired = old_snap;
    } else {
      free(old_snap);
    }
}

static DAG_ACTIVE_SNAPSHOT *dag_alloc_snapshot(int32_t count)
{
    size_t alloc_items = (size_t)(count > 0 ? count : 1);
    size_t sz = offsetof(DAG_ACTIVE_SNAPSHOT, items) +
                sizeof(INSDS *) * alloc_items;
    DAG_ACTIVE_SNAPSHOT *snap = (DAG_ACTIVE_SNAPSHOT *)malloc(sz);
    if (snap != NULL) {
      snap->count = count;
      snap->retired_epoch = 0;
      snap->next_retired = NULL;
      snap->items[0] = NULL;
    }
    return snap;
}

static void dag_publish_active_snapshot_from_array_locked(
    CSOUND *csound, INSDS *const *items, int32_t count)
{
    DAG_ACTIVE_SNAPSHOT *old_snap = csound->dag_active_snapshot;
    DAG_ACTIVE_SNAPSHOT *new_snap;

    dag_reclaim_active_snapshots_locked(csound);
    if (old_snap == NULL && count == 0)
      return;
    if (old_snap != NULL && old_snap->count == count &&
        (count == 0 ||
         memcmp(old_snap->items, items, sizeof(INSDS *) * (size_t)count) == 0))
      return;

    new_snap = dag_alloc_snapshot(count);
    if (new_snap == NULL) {
      dag_pin_snapshot_inactive_nodes(old_snap,
                                      ATOMIC_GET(csound->performance_epoch));
      return;
    }
    if (count > 0)
      memcpy(new_snap->items, items, sizeof(INSDS *) * (size_t)count);
    dag_store_active_snapshot(csound, new_snap);
    dag_retire_active_snapshot_locked(csound, old_snap);
}

void dag_publish_active_snapshot_locked(CSOUND *csound)
{
    DAG_ACTIVE_SNAPSHOT *old_snap;
    DAG_ACTIVE_SNAPSHOT *new_snap;
    INSDS *cur;
    int32_t count = 0;
    int32_t same;

    if (csound == NULL)
      return;
    dag_reclaim_active_snapshots_locked(csound);
    old_snap = csound->dag_active_snapshot;
    same = (old_snap != NULL);
    for (cur = csound->actanchor.nxtact; cur != NULL; cur = cur->nxtact) {
      if (same && (count >= old_snap->count || old_snap->items[count] != cur))
        same = 0;
      count++;
    }
    if (old_snap == NULL && count == 0)
      return;
    if (same && count == old_snap->count)
      return;

    new_snap = dag_alloc_snapshot(count);
    if (new_snap == NULL) {
      dag_pin_snapshot_inactive_nodes(old_snap,
                                      ATOMIC_GET(csound->performance_epoch));
      return;
    }
    count = 0;
    for (cur = csound->actanchor.nxtact; cur != NULL; cur = cur->nxtact) {
      new_snap->items[count++] = cur;
    }
    dag_store_active_snapshot(csound, new_snap);
    dag_retire_active_snapshot_locked(csound, old_snap);
}

void dag_cleanup_active_snapshots(CSOUND *csound)
{
    DAG_ACTIVE_SNAPSHOT *cur;
    DAG_ACTIVE_SNAPSHOT *retired;
    if (csound == NULL)
      return;
    cur = csound->dag_active_snapshot;
    csound->dag_active_snapshot = NULL;
    free(cur);
    retired = csound->dag_active_retired;
    csound->dag_active_retired = NULL;
    while (retired != NULL) {
      DAG_ACTIVE_SNAPSHOT *next = retired->next_retired;
      free(retired);
      retired = next;
    }
}

static void dag_deps_add_ptr(DAG_PTR_POOLS *pools, int32_t is_write,
                             int32_t off, int32_t *n, void *ptr)
{
    void **arr = is_write ? pools->writes : pools->reads;
    int32_t *used = is_write ? &pools->write_used : &pools->read_used;
    int32_t cap = is_write ? pools->write_cap : pools->read_cap;
    int32_t *overflow = is_write ? &pools->write_overflow
                                 : &pools->read_overflow;
    int32_t i;

    if (*overflow) {
      (*used)++;
      return;
    }
    for (i = 0; i < *n; i++) {
      if (arr[off + i] == ptr)
        return;
    }
    if (*used >= cap) {
      *overflow = 1;
      (*used)++;
      return;
    }
    arr[(*used)++] = ptr;
    (*n)++;
}

static int32_t is_inplace_global_write_op(const OENTRY *ep)
{
    if (ep == NULL) {
      return 0;
    }
    if ((ep->flags & WI) != 0 || ep->useropinfo != NULL) {
      return 1;
    }
    if (ep->opname == NULL) {
      return 0;
    }
    return strncmp(ep->opname, "##array_set", 11) == 0 ||
           strcmp(ep->opname, "##array_init") == 0 ||
           strncmp(ep->opname, "##array_member_set", 18) == 0 ||
           strcmp(ep->opname, "##member_set") == 0;
}

static inline OPCODINFO *load_latest_udo(const OPCODINFO *udo)
{
#ifdef HAVE_ATOMIC_BUILTIN
    return __atomic_load_n(&udo->latest, __ATOMIC_ACQUIRE);
#else
    return udo->latest;
#endif
}

static void dag_collect_instr_deps(INSTRTXT *instr, DAG_INSTR_DEPS *deps,
                                   DAG_PTR_POOLS *pools, int32_t depth)
{
    OPTXT *op;
    if (instr == NULL || depth > 16) {
      /* A call whose effects are unknown must preserve instrument order. */
      deps->requires_order = 1;
      return;
    }
    for (op = instr->nxtop; op != NULL && !deps->requires_order; op = op->nxtop) {
      OENTRY *ep = op->t.oentry;
      ARG *arg;
      if (ep != NULL) {
        if (ep->flags & ZR) deps->read_res |= DAG_RES_ZAK;
        if (ep->flags & ZW) deps->write_res |= DAG_RES_ZAK;
        if (ep->flags & TR) deps->read_res |= DAG_RES_TAB;
        if (ep->flags & TW) deps->write_res |= DAG_RES_TAB;
        if (ep->flags & _CR) deps->read_res |= DAG_RES_CHN;
        if (ep->flags & _CW) deps->write_res |= DAG_RES_CHN;
        if (ep->flags & IR) deps->read_res |= DAG_RES_INT;
        if (ep->flags & IW) {
          deps->write_res |= DAG_RES_INT;
          deps->requires_order = 1;
        }
        if (ep->flags & SK) {
          deps->read_res |= DAG_RES_STK;
          deps->write_res |= DAG_RES_STK;
        }
        if (ep->useropinfo != NULL) {
          OPCODINFO *udo = (OPCODINFO *)ep->useropinfo;
          OPCODINFO *latest = load_latest_udo(udo);
          deps->has_udo_call = 1;
          dag_collect_instr_deps(udo->ip, deps, pools, depth + 1);
          if (latest != NULL && latest != udo) {
            dag_collect_instr_deps(latest->ip, deps, pools, depth + 1);
          }
        } else if (ep->perf == (SUBR)subinstr ||
                   ep->init == (SUBR)subinstrset ||
                   ep->init == (SUBR)subinstrset_S ||
                   ep->perf == (SUBR)opcode_run_perf ||
                   ep->perf == (SUBR)opcode_array_perf ||
                   ep->perf == (SUBR)opcode_object_perf ||
                   ep->init == (SUBR)opcode_object_init ||
                   ep->init == (SUBR)opcode_array_init) {
          /* Indirect targets (subinstruments and Opcode objects) are resolved
             dynamically, so preserve instrument order. */
          deps->requires_order = 1;
        }
      }
      for (arg = op->t.outArgs; arg != NULL; arg = arg->next) {
        if (arg->type == ARG_GLOBAL && arg->argPtr != NULL)
          dag_deps_add_ptr(pools, 1, deps->write_off,
                           &deps->nwrites, arg->argPtr);
      }
      for (arg = op->t.inArgs; arg != NULL; arg = arg->next) {
        if (arg->type == ARG_GLOBAL && arg->argPtr != NULL) {
          dag_deps_add_ptr(pools, 0, deps->read_off,
                           &deps->nreads, arg->argPtr);
          if (is_inplace_global_write_op(ep))
            dag_deps_add_ptr(pools, 1, deps->write_off,
                             &deps->nwrites, arg->argPtr);
        }
      }
    }
}

static void dag_collect_instance_deps(INSDS *insds, DAG_INSTR_DEPS *deps,
                                      DAG_PTR_POOLS *pools)
{
    INSDS *cur;
    const INSTRTXT *seen[64];
    int32_t nseen = 0;
    int32_t steps = 0;

    if (insds == NULL) {
      deps->requires_order = 1;
      return;
    }
    dag_collect_instr_deps(insds->instr, deps, pools, 0);
    if (deps->has_udo_call && !ATOMIC_GET(insds->init_done)) {
      /* Async realtime init or reinit may still be unbound on the init thread
         and become runnable mid-cycle: preserve order until next DAG build. */
      deps->requires_order = 1;
      return;
    }
    if (insds->instr != NULL) {
      seen[nseen++] = insds->instr;
    }
    /* Also inspect the UDO templates bound to this instance at init time,
       so live UDO redefinitions that bind newer notes still serialize cleanly. */
    cur = insds;
    while (!deps->requires_order && cur->opcod_deact != NULL) {
      UOPCODE *udo;
      INSDS *udoIp;
      if (steps >= 64) {
        deps->requires_order = 1;
        break;
      }
      udo = (UOPCODE *)cur->opcod_deact;
      udoIp = udo->ip;
      steps++;
      if (udoIp == NULL || !ATOMIC_GET(udoIp->init_done)) {
        /* A call linked but not bound yet, or currently reinitializing on
           another thread: what it will write is unknown, so keep order. */
        deps->requires_order = 1;
        break;
      }
      if (udoIp->instr != NULL) {
        int32_t i, already = 0;
        for (i = 0; i < nseen; i++) {
          if (seen[i] == udoIp->instr) {
            already = 1;
            break;
          }
        }
        if (!already) {
          if (nseen < 64) {
            seen[nseen++] = udoIp->instr;
          }
          dag_collect_instr_deps(udoIp->instr, deps, pools, 1);
        }
      }
      cur = udoIp;
    }
}

static int32_t dag_ptr_lists_overlap(void *const *a, int32_t na,
                                     void *const *b, int32_t nb)
{
    int32_t i, j;
    for (i = 0; i < na; i++) {
      for (j = 0; j < nb; j++) {
        if (a[i] == b[j])
          return 1;
      }
    }
    return 0;
}

static int32_t dag_deps_conflict(const DAG_INSTR_DEPS *a,
                                 const DAG_INSTR_DEPS *b,
                                 void *const *read_pool,
                                 void *const *write_pool)
{
    if (a->requires_order || b->requires_order)
      return 1;
    if ((a->write_res & (b->read_res | b->write_res)) != 0 ||
        (a->read_res & b->write_res) != 0)
      return 1;
    if (dag_ptr_lists_overlap(write_pool + a->write_off, a->nwrites,
                              read_pool + b->read_off, b->nreads) ||
        dag_ptr_lists_overlap(write_pool + a->write_off, a->nwrites,
                              write_pool + b->write_off, b->nwrites) ||
        dag_ptr_lists_overlap(read_pool + a->read_off, a->nreads,
                              write_pool + b->write_off, b->nwrites))
      return 1;
    return 0;
}

int32_t dag_build(CSOUND *csound, INSDS *chain)
{
    INSDS **task_map;
    DAG_INSTR_DEPS *task_deps;
    DAG_PTR_POOLS pools;
    int32_t i;
    int32_t cap;
    int32_t active_count = 0;
    int32_t built = 0;
    int32_t attempt;

    /* Allocate DAG buffers outside alloc_spinlock so a memdie()/LongJmp
       on allocation failure can never unwind out while holding alloc_spinlock.
       Leave existing dag_num_active and dag_task_dep intact until a locked
       snapshot succeeds, so contended calls do not disturb prior state. */
    if (csound->dag_task_status == NULL) {
      int32_t new_max = csound->dag_task_max_size;
      if (new_max < INIT_SIZE)
        new_max = INIT_SIZE;
      create_dag(csound, new_max);
    }

    cap = csound->dag_task_max_size;
    task_deps = (DAG_INSTR_DEPS *)csound->Calloc(
        csound, sizeof(DAG_INSTR_DEPS) * (size_t)cap);
    pools.read_cap = 64;
    pools.write_cap = 64;
    pools.reads = (void **)csound->Malloc(
        csound, sizeof(void *) * (size_t)pools.read_cap);
    pools.writes = (void **)csound->Malloc(
        csound, sizeof(void *) * (size_t)pools.write_cap);

    for (attempt = 0; attempt < 4; attempt++) {
      INSDS *cur;
      int32_t count = 0;
      int32_t count_overflow = 0;
      int32_t locked = 0;

#if CSOUND_SPINLOCK_AVAILABLE
      if (csound->realtime_locks_initialized) {
        if (csoundSpinTryLock(&csound->alloc_spinlock) != CSOUND_SUCCESS) {
          /* Keep dag_changed nonzero so the next k-cycle rebuilds once
             actanchor/engineState publication completes; let the caller run
             from the published active snapshot instead of walking live links. */
          break;
        }
        locked = 1;
        cur = csound->actanchor.nxtact;
      } else
#endif
      {
        cur = (chain != NULL) ? chain : csound->actanchor.nxtact;
      }

      pools.read_used = 0;
      pools.write_used = 0;
      pools.read_overflow = 0;
      pools.write_overflow = 0;
      task_map = csound->dag_task_map;

      while (cur != NULL) {
        if (count < cap) {
          csound->dag_task_status[count].s = AVAILABLE;
          csound->dag_wlmm[count].id = count;
          task_map[count] = cur;
          memset(&task_deps[count], 0, sizeof(DAG_INSTR_DEPS));
          task_deps[count].read_off = pools.read_used;
          task_deps[count].write_off = pools.write_used;
          dag_collect_instance_deps(cur, &task_deps[count], &pools);
        } else {
          count_overflow = 1;
        }
        count++;
        cur = cur->nxtact;
      }

      if (!count_overflow && !pools.read_overflow && !pools.write_overflow) {
        ATOMIC_SET(csound->dag_changed, 0);
        dag_publish_active_snapshot_from_array_locked(
            csound, csound->dag_task_map, count);
#if CSOUND_SPINLOCK_AVAILABLE
        if (locked)
          csoundSpinUnLock(&csound->alloc_spinlock);
#endif
        active_count = count;
        built = 1;
        break;
      }

#if CSOUND_SPINLOCK_AVAILABLE
      if (locked)
        csoundSpinUnLock(&csound->alloc_spinlock);
#endif
      if (attempt == 3)
        break;

      if (count_overflow) {
        int32_t old_max = csound->dag_task_max_size;
        int32_t grown_max = count + INIT_SIZE;
        recreate_dag(csound, old_max, grown_max);
        cap = csound->dag_task_max_size;
        task_deps = (DAG_INSTR_DEPS *)csound->ReAlloc(
            csound, task_deps, sizeof(DAG_INSTR_DEPS) * (size_t)cap);
      }
      if (pools.read_overflow) {
        int32_t new_cap = pools.read_used + 64;
        pools.reads = (void **)csound->ReAlloc(
            csound, pools.reads, sizeof(void *) * (size_t)new_cap);
        pools.read_cap = new_cap;
      }
      if (pools.write_overflow) {
        int32_t new_cap = pools.write_used + 64;
        pools.writes = (void **)csound->ReAlloc(
            csound, pools.writes, sizeof(void *) * (size_t)new_cap);
        pools.write_cap = new_cap;
      }
    }

    if (!built) {
      csound->Free(csound, pools.writes);
      csound->Free(csound, pools.reads);
      csound->Free(csound, task_deps);
      return 0;
    }

    csound->dag_num_active = 0;
    for (i = 0; i < csound->dag_task_max_size; i++) {
      if (csound->dag_task_dep[i]) {
        csound->Free(csound, csound->dag_task_dep[i]);
        csound->dag_task_dep[i] = NULL;
      }
      if (i >= active_count)
        csound->dag_wlmm[i].id = INVALID;
    }
    memset((void *)csound->dag_task_watch, 0,
           sizeof(watchList *) * (size_t)csound->dag_task_max_size);

    if (active_count == 0) {
      csound->Free(csound, pools.writes);
      csound->Free(csound, pools.reads);
      csound->Free(csound, task_deps);
      return 1;
    }

    task_map = csound->dag_task_map;
    if (UNLIKELY(csound->oparms->odebug))
      printf("dag_num_active = %d\n", active_count);
    for (i = 0; i < active_count; i++) {
      int32_t j;
      if (UNLIKELY(csound->oparms->odebug))
        printf("\nWho depends on %d (instr %d)?\n", i, task_map[i]->insno);
      for (j = i + 1; j < active_count; j++) {
        if (UNLIKELY(csound->oparms->odebug))
          printf("%d ", j);
        if (dag_deps_conflict(&task_deps[i], &task_deps[j],
                              pools.reads, pools.writes)) {
          char *tt = csound->dag_task_dep[j];
          if (tt == NULL) {
            tt = csound->dag_task_dep[j] =
              (char *)csound->Calloc(csound, sizeof(char) * (size_t)(j + 1));
            csound->dag_task_status[j].s = WAITING;
            csound->dag_wlmm[j].next = csound->dag_task_watch[i];
            csound->dag_wlmm[j].id = j;
            csound->dag_task_watch[i] = &(csound->dag_wlmm[j]);
          }
          tt[i] = 1;
        }
      }
    }
    csound->dag_num_active = active_count;
    csound->Free(csound, pools.writes);
    csound->Free(csound, pools.reads);
    csound->Free(csound, task_deps);
    if (UNLIKELY(csound->oparms->odebug)) dag_print_state(csound);
    return 1;
}

void dag_reinit(CSOUND *csound)
{
    int32_t i;
    int32_t max = csound->dag_task_max_size;
    volatile stateWithPadding *task_status = csound->dag_task_status;
    watchList * volatile *task_watch = csound->dag_task_watch;
    watchList *wlmm = csound->dag_wlmm;
    if (csound->dag_num_active <= 0 || task_status == NULL)
      return;
    if (UNLIKELY(csound->oparms->odebug))
      printf("DAG REINIT************************\n");
    for (i=csound->dag_num_active; i<max; i++)
      task_status[i].s = DONE;
    task_status[0].s = AVAILABLE;
    task_watch[0] = NULL;
    for (i=1; i<csound->dag_num_active; i++) {
      int32_t j;
      task_status[i].s = AVAILABLE;
      task_watch[i] = NULL;
      if (csound->dag_task_dep[i]==NULL) continue;
      for (j=0; j<i; j++)
        if (csound->dag_task_dep[i][j]) {
          task_status[i].s = WAITING;
          wlmm[i].id = i;
          wlmm[i].next = task_watch[j];
          task_watch[j] = &wlmm[i];
          break;
        }
    }
    //dag_print_state(csound);
}

#if defined(_MSC_VER)
#define ATOMIC_READ(x) \
  ((enum state)InterlockedCompareExchange((volatile LONG *)&(x), 0, 0))
#define ATOMIC_WRITE(x,v) \
  ((void)InterlockedExchange((volatile LONG *)&(x), (LONG)(v)))
#define ATOMIC_READ_PTR(x) \
  ((watchList *)InterlockedCompareExchangePointer((PVOID volatile *)&(x), \
                                                  NULL, NULL))
#else
#define ATOMIC_READ(x) __atomic_load_n(&(x), __ATOMIC_SEQ_CST)
#define ATOMIC_WRITE(x,v) __atomic_store_n(&(x), (v), __ATOMIC_SEQ_CST)
#define ATOMIC_READ_PTR(x) __atomic_load_n(&(x), __ATOMIC_SEQ_CST)
#endif
#if defined(_MSC_VER)
#define ATOMIC_CAS(x,current,new) \
  (current == InterlockedCompareExchange(x, new, current))
#else
#define ATOMIC_CAS(x,current,new)  \
  __atomic_compare_exchange_n(x,&(current),new, true, __ATOMIC_SEQ_CST, \
                              __ATOMIC_SEQ_CST)
#endif

#if defined(_MSC_VER)
#define ATOMIC_CAS_PTR(x,current,new) \
  (current == InterlockedCompareExchangePointer(x, new, current))
#else
#define ATOMIC_CAS_PTR(x,current,new)  \
  __atomic_compare_exchange_n(x,&(current),new, true, __ATOMIC_SEQ_CST,\
                              __ATOMIC_SEQ_CST)
#endif

taskID dag_get_task(CSOUND *csound, int32_t index, int32_t numThreads, taskID next_task)
{
    int32_t i;
    int32_t count_waiting = 0;
    int32_t active = csound->dag_num_active;
    volatile stateWithPadding *task_status = csound->dag_task_status;
    int32_t start;
    enum state current_task_status;

    if (active <= 0 || task_status == NULL)
      return (taskID)INVALID;
    start = (index * active) / numThreads;

    if (next_task != INVALID) {
      // Have forwarded one task from the previous one
      // assert(ATOMIC_READ(task_status[next_task].s) == WAITING);
      ATOMIC_WRITE(task_status[next_task].s,INPROGRESS);
      return next_task;
    }

    //printf("**GetTask from %d\n", csound->dag_num_active);
    i = start;
    do {
      current_task_status = ATOMIC_READ(task_status[i].s);

      switch (current_task_status) {
      case AVAILABLE :
        // Need to CAS as the value may have changed
#ifndef WIN32        
        if (ATOMIC_CAS(&(task_status[i].s), current_task_status, INPROGRESS)) {
#else
          if (ATOMIC_CAS((long *) &(task_status[i].s), current_task_status, INPROGRESS)) {
#endif
          return (taskID)i;
        }
        break;

      case WAITING :
        //  printf("**%d waiting\n", i);
        ++count_waiting;
        break;

      case INPROGRESS :
        //  print(f"**%d active\n", i);
        break;

      case DONE :
        //printf("**%d done\n", i);
        break;

      default :
        // Enum corrupted!
        //assert(0);
        break;
      }

      // Increment modulo active
      i = (i+1 == active) ? 0 : i + 1;

    } while (i != start);
    //dag_print_state(csound);
    if (count_waiting == 0) return (taskID)INVALID;
    //printf("taskstodo=%d)\n", morework);
    return (taskID)WAIT;
}

/* This static is OK as not written */
static const watchList DoNotRead = { INVALID, NULL};

inline static int32_t move_watch(CSOUND *csound, watchList * volatile *w,
                            watchList *t)
{
     IGN(csound);
    watchList *local;
    t->next = NULL;
    //printf("moveWatch\n");
    do {
      //dag_print_state(csound);
      local = ATOMIC_READ_PTR(*w);
      if (local==&DoNotRead) {
        //printf("local is DoNotRead\n");
        return 0;//was no & earlier
      }
      else t->next = local;
    } while (!ATOMIC_CAS_PTR(w,local,t));
    //dag_print_state(csound);
    //printf("moveWatch done\n");
    return 1;
}

taskID dag_end_task(CSOUND *csound, taskID i)
{
    watchList *to_notify, *next;
    int32_t canQueue;
    int32_t j, k;
    watchList * volatile *task_watch = csound->dag_task_watch;
    enum state current_task_status;
    int32_t wait_on_current_tasks;
    taskID next_task = INVALID;
    ATOMIC_WRITE(csound->dag_task_status[i].s, DONE); /* as DONE is zero */
    // A write barrier /might/ be useful here to avoid the case
    // of the list being DoNotRead but the status being something
    // other than done.  At the time of writing this wouldn't give
    // a correctness issue, plus the semantics of GCC's CAS apparently
    // imply a write barrier, so it should be OK.
    {                                      /* ATOMIC_SWAP */
      do {
        to_notify = ATOMIC_READ_PTR(task_watch[i]);
      } while (!ATOMIC_CAS_PTR(&task_watch[i],to_notify,(watchList *) &DoNotRead));
    } //to_notify = ATOMIC_SWAP(task_watch[i], &DoNotRead);
    //printf("Ending task %d\n", i);
    next = to_notify;
    while (to_notify) {         /* walk the list of watchers */
      next = to_notify->next;
      j = to_notify->id;
      //printf("%d notifying task %d it finished\n", i, j);
      canQueue = 1;
      wait_on_current_tasks = 0;
      char *dep = csound->dag_task_dep[j];

      if (dep != NULL) {
        for (k=0; k<j; k++) {     /* seek next watch */
          if (dep[k]==0) continue;
          current_task_status = ATOMIC_READ(csound->dag_task_status[k].s);
          //printf("investigating task %d (%d)\n", k, current_task_status);

          if (current_task_status == WAITING) {   // Prefer watching blocked tasks
            //printf("found task %d to watch %d status %d\n",
            //       k, j, csound->dag_task_status[k].s);
            if (move_watch(csound, &task_watch[k], to_notify)) {
              //printf("task %d now watches %d\n", j, k);
              canQueue = 0;
              wait_on_current_tasks = 0;
              break;
            }
            else {
              /* assert csound->dag_task_status[j].s == DONE and we are in race */
              //printf("Racing status %d %d %d %d\n",
              //       csound->dag_task_status[j].s, i, j, k);
            }

          }
          else if (current_task_status == AVAILABLE ||
                   current_task_status == INPROGRESS) {
            wait_on_current_tasks = 1;
          }
          //else { printf("not %d\n", k); }
        }
      }

      // Try the same thing again but this time waiting on active or available task
      if (wait_on_current_tasks == 1 && dep != NULL) {
        for (k=0; k<j; k++) {     /* seek next watch */
          if (dep[k]==0) continue;
          current_task_status = ATOMIC_READ(csound->dag_task_status[k].s);
          //printf("investigating task %d (%d)\n", k, current_task_status);

          if (current_task_status != DONE) {   // Prefer watching blocked tasks
            //printf("found task %d to watch %d status %d\n",
            //       k, j, csound->dag_task_status[k].s);
            if (move_watch(csound, &task_watch[k], to_notify)) {
              //printf("task %d now watches %d\n", j, k);
              canQueue = 0;
              break;
            }
            else {
              /* assert csound->dag_task_status[j].s == DONE and we are in race */
              //printf("Racing status %d %d %d %d\n",
              //       csound->dag_task_status[j].s, i, j, k);
            }

          }
          //else { printf("not %d\n", k); }
        }
      }

      if (canQueue) {           /*  could use monitor here */
        if (next_task == INVALID) {
          next_task = j; // Forward directly to the thread to save re-dispatch
        } else {
          ATOMIC_WRITE(csound->dag_task_status[j].s, AVAILABLE);
        }
      }
      to_notify = next;
    }
    //dag_print_state(csound);
    return next_task;
}

