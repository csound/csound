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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA


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
#include "cs_par_orc_semantics.h"
#include <stdbool.h>

#if defined(_MSC_VER)
/* For InterlockedCompareExchange */
#include <windows.h>
#endif

/* Used as an error value */
//typedef int taskID;
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
//static int task_max_size;

static void dag_print_state(CSOUND *csound)
{
    int i;
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
          int j;
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
    int max = csound->dag_task_max_size;
    csound->dag_task_status = csound->Calloc(csound, sizeof(stateWithPadding)*max);
    csound->dag_task_watch  = csound->Calloc(csound, sizeof(watchList*)*max);
    csound->dag_task_map    = csound->Calloc(csound, sizeof(INSDS*)*max);
    csound->dag_task_dep    = (char **)csound->Calloc(csound, sizeof(char*)*max);
    csound->dag_wlmm = (watchList *)csound->Calloc(csound, sizeof(watchList)*max);
}

static void recreate_dag(CSOUND *csound)
{
    /* Allocate the main task status and watchlists */
    int max = csound->dag_task_max_size;
    csound->dag_task_status =
      csound->ReAlloc(csound, (stateWithPadding *)csound->dag_task_status,
               sizeof(stateWithPadding)*max);
    csound->dag_task_watch  =
      csound->ReAlloc(csound, (struct watchList *)csound->dag_task_watch,
               sizeof(watchList*)*max);
    csound->dag_task_map    =
      csound->ReAlloc(csound, (INSDS *)csound->dag_task_map, sizeof(INSDS*)*max);
    csound->dag_task_dep    =
      (char **)csound->ReAlloc(csound, csound->dag_task_dep, sizeof(char*)*max);
    csound->dag_wlmm        =
      (watchList *)csound->ReAlloc(csound, csound->dag_wlmm, sizeof(watchList)*max);
}

static INSTR_SEMANTICS *dag_get_info(CSOUND* csound, int insno)
{
    INSTR_SEMANTICS *current_instr =
      csp_orc_sa_instr_get_by_num(csound, insno);
    if (current_instr == NULL) {
      current_instr =
        csp_orc_sa_instr_get_by_name(csound,
           csound->engineState.instrtxtp[insno]->insname);
      if (UNLIKELY(current_instr == NULL))
        csound->Die(csound,
                    Str("Failed to find semantic information"
                        " for instrument '%i'"),
                    insno);
    }
    return current_instr;
}

static int dag_intersect(CSOUND *csound, struct set_t *current,
                         struct set_t *later, int cnt)
{
    IGN(cnt);
    struct set_t *ans;
    int res = 0;
    struct set_element_t *ele;
    ans = csp_set_intersection(csound, current, later);
    res = ans->count;
    ele = ans->head;
    while (ele != NULL) {
      struct set_element_t *next = ele->next;
      csound->Free(csound, ele);
      ele = next; res++;
    }
    csound->Free(csound, ans);
    return res;
}

void dag_build(CSOUND *csound, INSDS *chain)
{
    INSDS *save = chain;
    INSDS **task_map;
    int i;

    //printf("DAG BUILD***************************************\n");
    csound->dag_num_active = 0;
    while (chain != NULL) {
      csound->dag_num_active++;
      chain = chain->nxtact;
    }
    if (csound->dag_num_active>csound->dag_task_max_size) {
      //printf("**************need to extend task vector\n");
      csound->dag_task_max_size = csound->dag_num_active+INIT_SIZE;
      recreate_dag(csound);
    }
    if (csound->dag_task_status == NULL)
      create_dag(csound); /* Should move elsewhere */
    else {
      memset((void*)csound->dag_task_watch, '\0',
             sizeof(watchList*)*csound->dag_task_max_size);
      for (i=0; i<csound->dag_task_max_size; i++) {
        if (csound->dag_task_dep[i]) {
          csound->dag_task_dep[i]= NULL;
        }
        csound->dag_wlmm[i].id = INVALID;
      }
    }
    task_map = csound->dag_task_map;
    for (i=0; i<csound->dag_num_active; i++) {
      csound->dag_task_status[i].s = AVAILABLE;
      csound->dag_wlmm[i].id=i;
    }
    csound->dag_changed = 0;
    if (UNLIKELY(csound->oparms->odebug))
      printf("dag_num_active = %d\n", csound->dag_num_active);
    i = 0; chain = save;
    while (chain != NULL) {     /* for each instance check against later */
      int j = i+1;              /* count of instance */
      if (UNLIKELY(csound->oparms->odebug))
        printf("\nWho depends on %d (instr %d)?\n", i, chain->insno);
      INSDS *next = chain->nxtact;
      INSTR_SEMANTICS *current_instr = dag_get_info(csound, chain->insno);
      //csp_set_print(csound, current_instr->read);
      //csp_set_print(csound, current_instr->write);
      while (next) {
        INSTR_SEMANTICS *later_instr = dag_get_info(csound, next->insno);
        int cnt = 0;
        if (UNLIKELY(csound->oparms->odebug)) printf("%d ", j);
        //csp_set_print(csound, later_instr->read);
        //csp_set_print(csound, later_instr->write);
        //csp_set_print(csound, later_instr->read_write);
        if (dag_intersect(csound, current_instr->write,
                          later_instr->read, cnt++)       ||
            dag_intersect(csound, current_instr->read_write,
                          later_instr->read, cnt++)       ||
            dag_intersect(csound, current_instr->read,
                          later_instr->write, cnt++)      ||
            dag_intersect(csound, current_instr->write,
                          later_instr->write, cnt++)      ||
            dag_intersect(csound, current_instr->read_write,
                          later_instr->write, cnt++)      ||
            dag_intersect(csound, current_instr->read,
                          later_instr->read_write, cnt++) ||
            dag_intersect(csound, current_instr->write,
                          later_instr->read_write, cnt++)) {
          char *tt = csound->dag_task_dep[j];
          if (tt==NULL) {
            /* get dep vector if missing and set watch first time */
            tt = csound->dag_task_dep[j] =
              (char*)csound->Calloc(csound, sizeof(char)*(j+1));
            csound->dag_task_status[j].s = WAITING;
            csound->dag_wlmm[j].next = csound->dag_task_watch[i];
            csound->dag_wlmm[j].id = j;
            csound->dag_task_watch[i] = &(csound->dag_wlmm[j]);
            //printf("set watch %d to %d\n", j, i);
          }
          tt[i] = 1;
          //printf("-yes ");
        }
        j++; next = next->nxtact;
      }
      task_map[i] = chain;
      i++; chain = chain->nxtact;
    }
    if (UNLIKELY(csound->oparms->odebug)) dag_print_state(csound);
}

void dag_reinit(CSOUND *csound)
{
    int i;
    int max = csound->dag_task_max_size;
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
      int j;
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

//#define ATOMIC_READ(x) __atomic_load(&(x), __ATOMIC_SEQ_CST)
//#define ATOMIC_WRITE(x,v) __atomic_(&(x), v, __ATOMIC_SEQ_CST)
#define ATOMIC_READ(x) x
#define ATOMIC_WRITE(x,v) x = v;
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

taskID dag_get_task(CSOUND *csound, int index, int numThreads, taskID next_task)
{
    int i;
    int count_waiting = 0;
    int active = csound->dag_num_active;
    int start = (index * active) / numThreads;
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
        if (ATOMIC_CAS(&(task_status[i].s), current_task_status, INPROGRESS)) {
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

inline static int moveWatch(CSOUND *csound, watchList * volatile *w,
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
    int canQueue;
    int j, k;
    watchList * volatile *task_watch = csound->dag_task_watch;
    enum state current_task_status;
    int wait_on_current_tasks;
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
          if (moveWatch(csound, &task_watch[k], to_notify)) {
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
            if (moveWatch(csound, &task_watch[k], to_notify)) {
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


/* INV : Acyclic */
/* INV : Each entry is read by a single thread,
 *       no writes (but see OPT : Watch ordering) */
/* Thus no protection needed */

/* INV : Watches for different tasks are disjoint */
/* INV : Multiple threads can add to a watch list but only one will remove
 *       These are the only interactions */
/* Thus the use of CAS / atomic operations */

/* Used to mark lists that should not be added to, see NOTE : Race condition */
#if 0
watchList nullList;
watchList *doNotAdd = &nullList;
watchList endwatch = { NULL, NULL };

/* Lists of tasks that depend on the given task */
watchList ** watch;         /* OPT : Structure lay out */
watchListMemoryManagement *wlmm; /* OPT : Structure lay out */

/* INV : wlmm[X].s.id == X; */  /* OPT : Data structure redundancy */
/* INV : status[X] == WAITING => wlmm[X].used */
/* INV : wlmm[X].s is in watch[Y] => wlmm[X].used */


/* Watch list helper functions */

void initialiseWatch (watchList **w, taskID id) {
  wlmm[id].used = TRUE;
  wlmm[id].s.id = id;
  wlmm[id].s.tail = *w;
  *w = &(wlmm[id].s);
}

inline watchList * getWatches(taskID id) {

    return __atomic_test_and_set (&(watch[id]), doNotAdd);
}

int moveWatch (watchList **w, watchList *t) {
  watchList *local;

  t->tail = NULL;

  do {
    local = atomicRead(*w);

    if (local == doNotAdd) {
      return 0;
    } else {
      t->tail = local;
    }
  } while (!atomicCAS(*w,local,t));   /* OPT : Delay loop */

  return 1;
}

void appendToWL (taskID id, watchList *l) {
  watchList *w;

  do {
    w = watch[id];
    l->tail = w;
    w = __atomic_compare_exchange_n(&(watch[id]),w,l, false, \
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  } while (!(w == l));

}

inline void deleteWatch (watchList *t) {
  wlmm[t->id].used = FALSE;
}




typedef struct monitor {
  pthread_mutex_t l = PTHREAD_MUTEX_INITIALIZER;
  unsigned int threadsWaiting = 0;    /* Shadows the length of
                                         workAvailable wait queue */
  queue<taskID> q;                    /* OPT : Dispatch order */
  pthread_cond_t workAvailable = PTHREAD_COND_INITIALIZER;
  pthread_cond_t done = PTHREAD_COND_INITIALIZER;
} monitor;                                    /* OPT : Lock-free */

/* INV : q.size() + dispatched <= ID */
/* INV : foreach(id,q.contents()) { status[id] = AVAILABLE; } */
/* INV : threadsWaiting <= THREADS */

monitor dispatch;


void addWork(monitor *dispatch, taskID id) {
  pthread_mutex_lock(&dispatch->l);

  status[id] = AVAILABLE;
  dispatch->q.push(id);
  if (threadsWaiting >= 1) {
    pthread_cond_signal(&dispatch->c);
  }

  pthread_mutex_unlock(&dispatch->l);
  return;
}

taskID getWork(monitor *dispatch) {
  taskID returnValue;

  pthread_mutex_lock(&dispatch->l);

  while (q.empty()) {
    ++dispatch->threadsWaiting;

    if (dispatch->threadsWaiting == THREADS) {
      /* Will the last person out please turn off the lights! */
      pthread_cond_signal(&dispatch->done);
    }

    pthread_cond_wait(&dispatch->l,&dispatch->workAvailable);
    --dispatch->threadsWaiting;

    /* NOTE : A while loop is needed as waking from this requires
     * reacquiring the mutex and in the mean time someone
     * might have got it first and removed the work. */
  }

  returnValue = q.pop();

  pthread_mutex_unlock(&dispatch->l);
  return returnValue;

}

void waitForWorkToBeCompleted (monitor *dispatch) {
  /* Requires
   * INV : threadsWaiting == THREADS <=> \forall id \in ID . status[id] == DONE
   */

  pthread_mutex_lock(&dispatch->l);

  if (dispatch->threadsWaiting < THREADS) {
    pthread_cond_wait(&dispatch->l,&dispatch->done);
  }

  /* This assertion is more difficult to prove than it might first appear */
  assert(dispatch->threadsWaiting == THREADS);

  pthread_mutex_unlock(&dispatch->l);
  return;
}














void mainThread (State *s) {

  /* Set up the DAG */
  if (s->firstRun || s->updateNeeded) {
    dep = buildDAG(s);        /* OPT : Dispatch order */
    /* Other : Update anything that is indexed by task
     * (i.e. all arrays given length ID) */
  }

  /* Reset the data structure */
  foreach (id in ID) {
    watch[id] = NULL;
  }

  /* Initialise the dispatch queue */
  foreach (id in ID) {       /* OPT : Dispatch order */
    if (dep[id] == EMPTYSET) {
      atomicWrite(status[id] = AVAILABLE);
      addWork(*dispatch,id);

    } else {
      atomicWrite(status[id] = WAITING);
      initialiseWatch(&watch[choose(dep[id])], id);  /* OPT : Watch ordering */

    }
  }

/* INV : Data structure access invariants start here */
/* INV : Status only decrease from now */
/* INV : Watch list for id contains a subset of the things that depend on id */
/* INV : Each id appears in at most one watch list */
/* INV : doNotAdd only appears at the head of a watch list */
/* INV : if (watch[id] == doNotAdd) then { status[id] == DONE; } */

  waitForWorkToBeCompleted(*dispatch);

  return;
}

void workerThread (State *s) {
  taskID work;
  watchList *tasksToNotify, next;
  bool canQueue;

  do {

    task = getWork(dispatch);

    /* Do stuff */
    atomicWrite(status[work] = INPROGRESS);
    doStuff(work);
    atomicWrite(status[work] = DONE);    /* NOTE : Race condition */


    tasksToNotify = getWatches(work);

    while (tasksToNotify != NULL) {
      next = tasksToNotify->tail;

      canQueue = TRUE;
      foreach (dep in dep[tasksToNotify->id]) {  /* OPT : Watch ordering */
        if (atomicRead(status[dep]) != DONE) {
          /* NOTE : Race condition */
          if (moveWatch(watch[dep],tasksToNotify)) {
            canQueue = FALSE;
            break;
          } else {
            /* Have hit the race condition, try the next option */
            assert(atomicRead(status[dep]) == DONE);
          }
        }
      }

      if (canQueue) {                    /* OPT : Save one work item */
        addWork(*dispatch,tasksToNotify->id);
        deleteWatch(tasksToNotify);
      }

      tasksToNotify = next;
    }

  } while (1);  /* NOTE : some kind of control for thread exit needed */

  return;
}




/* OPT : Structure lay out
 *
 * All data structures that are 1. modified by one or more thread and
 * 2. accessed by multiple threads, should be aligned to cache lines and
 * padded so that there is only one instance per cache line.  This will reduce
 * false memory contention between objects that just happen to share a cache
 * line.  Blocking to 64 bytes will probably be sufficient and if people really
 * care about performance that much they can tune to their particular
 * architecture.
 */

/* OPT : Watch ordering
 *
 * Moving a watch is relatively cheap (in the uncontended case) but
 * it would be best to avoid moving watches where possible.  The ideal
 * situation would be for every task to watch the last pre-requisite.
 * There are two places in the code that affect the watch ordering;
 * the initial assignment and the movement when a watch is triggered.
 * Prefering WAITING tasks (in the later) and lower priority tasks
 * (if combined with the dispatch order optimisation below) are probably
 * good choices.  One mechanism would be to reorder the set (or array) of
 * dependencies to store this information.  When looking for a (new) watch,
 * tasks are sorted with increasing status first and then the first one picked.
 * Keeping the list sorted (or at least split between WAITING and others) with
 * each update should (if the dispatch order is fixed / slowly adapting) result
 * in the best things to watch moving to the front and thus adaptively give
 * the best possible tasks to watch.  The interaction with a disaptch order
 * heuristic would need to be considered.  Note that only one thread will
 * look at any given element of dep[] so they can be re-ordered without
 * needing locking.
 */

/* OPT : Structure lay out
 *
 * Some of the fields are not strictly needed and are just there to make
 * the algorithm cleaner and more intelligible.  The id fields of the watch
 * lists are not really needed as there is one per task and their position
 * within the watchListMemoryManager array allows the task to be infered.
 * Likewise the used flag in the memory manager is primarily for book-keeping
 * and checking / assertions and could be omitted.
 */

/* OPT : Delay loop
 *
 * In theory it is probably polite to put a slowly increasing delay in
 * after a failed compare and swap to reduce pressure on the memory
 * subsystem in the highly contended case.  As multiple threads adding
 * to a task's watch list simultaneously is probably a rare event, the
 * delay loop is probably unnecessary.
 */

/* OPT : Dispatch order
 *
 * The order in which tasks are dispatched affects the amount of
 * parallelisation possible.  Picking the exact scheduling order, even
 * if the duration of the tasks is known is probably NP-Hard (see
 * bin-packing*) so heuristics are probably the way to go.  The proporition
 * of tasks which depend on a given task is probably a reasonable primary
 * score, with tie-breaks going to longer tasks.  This can either be
 * applied to just the initial tasks (either in ordering the nodes in the DAG)
 * or in the order in which they are traversed.  Alternatively by
 * sorting the queue / using a heap / other priority queuing structure
 * it might be possible to do this dynamically.  The best solution would
 * probably be adaptive with a task having its priority incremented
 * each time another worker thread blocks on a shortage of work, with these
 * increments also propagated 'upwards' in the DAG.
 *
 * *. Which means that a solver could be used to give the best possible
 *    schedule / the maximum parallelisation.  This could be useful for
 *    optimisation.
 */

/* OPT : Lock-free
 *
 * A lock free dispatch mechanism is probably possible as threads can
 * scan the status array for things listed as AVAILABLE and then atomicCAS
 * to INPROGRESS to claim them.  But this starts to involve busy-waits or
 * direct access to futexes and is probably not worth it.
 */

/* OPT : Save one work item
 *
 * Rather than adding all watching tasks who have their dependencies met to
 * the dispatch queue, saving one (perhaps the best, see OPT : Dispatch order)
 * means the thread does not have to wait.  In the case of a purely linear DAG
 * this should be roughly as fast as the single threaded version.
 */


/* NOTE : Race condition
 *
 * There is a subtle race condition:
 *
 *   Thread 1                             Thread 2
 *   --------                             --------
 *                                        atomicRead(status[dep]) != DONE
 *   atomicWrite(status[work] = DONE);
 *   tasksToNotify = getWatches(work);
 *                                        moveWatch(watch[dep],tasksToNotify);
 *
 * The key cause is that the status and the watch list cannot be updated
 * simultaneously.  However as getWatches removes all watches and moves or
 * removes them, it is sufficient to have a doNotAdd watchList node to detect
 * this race condition and resolve it by having moveWatch() fail.
 */

void newdag_alloc(CSOUND *csound, int numtasks)
{
    doNotAdd = &endwatch;
??
    watch = (watchList **)csound->Calloc(csound, sizeof(watchList *)*numtasks);
    wlmm = (watchListMemoryManagement *)
      csound->Calloc(csound, sizeof(watchListMemoryManagement)*numtasks);

}

#endif
