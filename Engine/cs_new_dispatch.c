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
#include <limits.h>
#include "csoundCore.h"
#include "cs_par_base.h"
#include "interlocks.h"
#include "entry.h"
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
          for (j=0; j<i; j++) if (tt[j]) printf("%d ", j);
          printf("]\n");
        }
        break;
      default:
        printf("status=???\n"); break;
      }
    }
}

/* For now allocate a fixed maximum number of tasks; FIXME */
static void create_dag(CSOUND *csound)
{
    /* Allocate the main task status and watchlists */
    int32_t max = csound->dag_task_max_size;
    csound->dag_task_status = csound->Calloc(csound, sizeof(stateWithPadding)*max);
    csound->dag_task_watch  = csound->Calloc(csound, sizeof(watchList*)*max);
    csound->dag_task_map    = csound->Calloc(csound, sizeof(INSDS*)*max);
    csound->dag_task_dep    = (char **)csound->Calloc(csound, sizeof(char*)*max);
    csound->dag_wlmm = (watchList *)csound->Calloc(csound, sizeof(watchList)*max);
}

static void recreate_dag(CSOUND *csound, int32_t old_max)
{
    /* Allocate the main task status and watchlists */
    int32_t max = csound->dag_task_max_size;
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

typedef struct dag_instr_deps {
    uint32_t read_res;
    uint32_t write_res;
    int32_t requires_order;
    void **reads;
    int32_t nreads, cap_reads;
    void **writes;
    int32_t nwrites, cap_writes;
} DAG_INSTR_DEPS;

static void dag_deps_add_ptr(CSOUND *csound, void ***arr,
                             int32_t *n, int32_t *cap, void *ptr)
{
    int32_t i;
    for (i = 0; i < *n; i++) {
      if ((*arr)[i] == ptr)
        return;
    }
    if (*n >= *cap) {
      int32_t newcap = (*cap == 0) ? 8 : (*cap * 2);
      *arr = (void **)csound->ReAlloc(csound, *arr, sizeof(void *) * newcap);
      *cap = newcap;
    }
    (*arr)[(*n)++] = ptr;
}

static void dag_collect_instr_deps(CSOUND *csound, INSTRTXT *instr,
                                   DAG_INSTR_DEPS *deps, int32_t depth)
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
          dag_collect_instr_deps(csound, udo->ip, deps, depth + 1);
        } else if (ep->perf == (SUBR)subinstr ||
                   ep->init == (SUBR)subinstrset ||
                   ep->init == (SUBR)subinstrset_S) {
          /* The subinstrument target is chosen at init, possibly from a p-field. */
          deps->requires_order = 1;
        }
      }
      for (arg = op->t.outArgs; arg != NULL; arg = arg->next) {
        if (arg->type == ARG_GLOBAL && arg->argPtr != NULL)
          dag_deps_add_ptr(csound, &deps->writes, &deps->nwrites,
                           &deps->cap_writes, arg->argPtr);
      }
      for (arg = op->t.inArgs; arg != NULL; arg = arg->next) {
        if (arg->type == ARG_GLOBAL && arg->argPtr != NULL) {
          dag_deps_add_ptr(csound, &deps->reads, &deps->nreads,
                           &deps->cap_reads, arg->argPtr);
          if (ep != NULL && (ep->flags & WI) != 0)
            dag_deps_add_ptr(csound, &deps->writes, &deps->nwrites,
                             &deps->cap_writes, arg->argPtr);
        }
      }
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
                                 const DAG_INSTR_DEPS *b)
{
    if (a->requires_order || b->requires_order)
      return 1;
    if ((a->write_res & (b->read_res | b->write_res)) != 0 ||
        (a->read_res & b->write_res) != 0)
      return 1;
    if (dag_ptr_lists_overlap(a->writes, a->nwrites, b->reads, b->nreads) ||
        dag_ptr_lists_overlap(a->writes, a->nwrites, b->writes, b->nwrites) ||
        dag_ptr_lists_overlap(a->reads, a->nreads, b->writes, b->nwrites))
      return 1;
    return 0;
}

void dag_build(CSOUND *csound, INSDS *chain)
{
    INSDS *save = chain;
    INSDS **task_map;
    DAG_INSTR_DEPS *task_deps;
    int32_t i;

    csound->dag_num_active = 0;
    while (chain != NULL) {
      csound->dag_num_active++;
      chain = chain->nxtact;
    }
    if (csound->dag_task_status == NULL) {
      if (csound->dag_num_active > csound->dag_task_max_size)
        csound->dag_task_max_size = csound->dag_num_active + INIT_SIZE;
      create_dag(csound);
    } else {
      int32_t old_max = csound->dag_task_max_size;
      for (i = 0; i < old_max; i++) {
        if (csound->dag_task_dep[i]) {
          csound->Free(csound, csound->dag_task_dep[i]);
          csound->dag_task_dep[i] = NULL;
        }
        csound->dag_wlmm[i].id = INVALID;
      }
      if (csound->dag_num_active > csound->dag_task_max_size) {
        csound->dag_task_max_size = csound->dag_num_active + INIT_SIZE;
        recreate_dag(csound, old_max);
      }
      memset((void *)csound->dag_task_watch, 0,
             sizeof(watchList *) * csound->dag_task_max_size);
    }
    csound->dag_changed = 0;
    if (csound->dag_num_active == 0)
      return;

    task_map = csound->dag_task_map;
    task_deps = (DAG_INSTR_DEPS *)csound->Calloc(
      csound, sizeof(DAG_INSTR_DEPS) * csound->dag_num_active);
    i = 0;
    chain = save;
    while (chain != NULL) {
      csound->dag_task_status[i].s = AVAILABLE;
      csound->dag_wlmm[i].id = i;
      task_map[i] = chain;
      dag_collect_instr_deps(csound, chain->instr, &task_deps[i], 0);
      i++;
      chain = chain->nxtact;
    }
    if (UNLIKELY(csound->oparms->odebug))
      printf("dag_num_active = %d\n", csound->dag_num_active);
    for (i = 0; i < csound->dag_num_active; i++) {
      int32_t j;
      if (UNLIKELY(csound->oparms->odebug))
        printf("\nWho depends on %d (instr %d)?\n", i, task_map[i]->insno);
      for (j = i + 1; j < csound->dag_num_active; j++) {
        if (UNLIKELY(csound->oparms->odebug))
          printf("%d ", j);
        if (dag_deps_conflict(&task_deps[i], &task_deps[j])) {
          char *tt = csound->dag_task_dep[j];
          if (tt == NULL) {
            tt = csound->dag_task_dep[j] =
              (char *)csound->Calloc(csound, sizeof(char) * (j + 1));
            csound->dag_task_status[j].s = WAITING;
            csound->dag_wlmm[j].next = csound->dag_task_watch[i];
            csound->dag_wlmm[j].id = j;
            csound->dag_task_watch[i] = &(csound->dag_wlmm[j]);
          }
          tt[i] = 1;
        }
      }
    }
    for (i = 0; i < csound->dag_num_active; i++) {
      if (task_deps[i].reads != NULL)
        csound->Free(csound, task_deps[i].reads);
      if (task_deps[i].writes != NULL)
        csound->Free(csound, task_deps[i].writes);
    }
    csound->Free(csound, task_deps);
    if (UNLIKELY(csound->oparms->odebug)) dag_print_state(csound);
}

void dag_reinit(CSOUND *csound)
{
    int32_t i;
    int32_t max = csound->dag_task_max_size;
    volatile stateWithPadding *task_status = csound->dag_task_status;
    watchList * volatile *task_watch = csound->dag_task_watch;
    watchList *wlmm = csound->dag_wlmm;
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
#define ATOMIC_READ(x) (x)
#define ATOMIC_WRITE(x,v) do { (x) = (v); MemoryBarrier(); } while (0)
#else
#define ATOMIC_READ(x) __atomic_load_n(&(x), __ATOMIC_SEQ_CST)
#define ATOMIC_WRITE(x,v) __atomic_store_n(&(x), (v), __ATOMIC_SEQ_CST)
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
    int32_t start = (index * active) / numThreads;
    volatile stateWithPadding *task_status = csound->dag_task_status;
    enum state current_task_status;

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
    watchList *local=*w;
    t->next = NULL;
    //printf("moveWatch\n");
    do {
      //dag_print_state(csound);
      local = ATOMIC_READ(*w);
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
        to_notify = ATOMIC_READ(task_watch[i]);
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

      for (k=0; k<j; k++) {     /* seek next watch */
        if (csound->dag_task_dep[j][k]==0) continue;
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

      // Try the same thing again but this time waiting on active or available task
      if (wait_on_current_tasks == 1) {
        for (k=0; k<j; k++) {     /* seek next watch */
          if (csound->dag_task_dep[j][k]==0) continue;
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

