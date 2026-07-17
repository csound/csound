/*
  insert.c:

  Copyright (C) 1991, 1997, 1999, 2002, 2005, 2013, 2024
  Barry Vercoe, Istvan Varga, John ffitch,
  Gabriel Maldonado, matt ingalls,
  Victor Lazzarini, Steven Yi

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

#include "csoundCore.h" /*  INSERT.C */
#include "oload.h"
#include "udo.h"
#include "aops.h"       /* for cond's */
#include "midiops.h"
#include "namedins.h"
#include "pstream.h"
#include "interlocks.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "csound_orc_semantics.h"
#include "csound_orc_compile.h"
#include <inttypes.h>

static void show_allocs(CSOUND *);
static void deact(CSOUND *, INSDS *);
static void xturnoff_now_internal(CSOUND *, INSDS *, int32_t);
static void free_unlinked_instance(CSOUND *, INSDS *);
static void sched_off_time(CSOUND *, INSDS *);
static int32_t insert_midi(CSOUND *csound, int32_t insno, MCHNBLK *chn,
                           MEVENT *mep);
static int32_t insert(CSOUND *csound, int32_t insno, EVTBLK *newevtp);
static void maxalloc_turnoff(CSOUND *csound, int32_t insno);
static INSDS *instantiate(CSOUND *csound, int32_t insno, int32_t link);

int32_t instance_has_async_refs(const INSDS *ip)
{
  return ip != NULL && ATOMIC_GET(ip->async_ref_count) != 0;
}

int32_t instance_is_reclaimable(const INSDS *ip)
{
  return ip != NULL && !ATOMIC_GET8(ip->actflg) &&
         !instance_has_async_refs(ip) && !ATOMIC_GET(ip->init_running) &&
         !ATOMIC_GET(ip->turnoff_pending);
}

static void inactive_instance_lock(CSOUND *csound)
{
  if (csound->realtime_locks_initialized)
    csoundSpinLock(&csound->instance_spinlock);
}

static void inactive_instance_unlock(CSOUND *csound)
{
  if (csound->realtime_locks_initialized)
    csoundSpinUnLock(&csound->instance_spinlock);
}

void async_instance_lock(CSOUND *csound)
{
  if (csound->realtime_locks_initialized)
    csoundSpinLock(&csound->async_ref_spinlock);
}

void async_instance_unlock(CSOUND *csound)
{
  if (csound->realtime_locks_initialized)
    csoundSpinUnLock(&csound->async_ref_spinlock);
}

static void enqueue_init_turnoff_locked(CSOUND *csound, INSDS *ip)
{
  ip->init_turnoff_next = csound->init_turnoff_pending;
  csound->init_turnoff_pending = ip;
}

int32_t instance_init_begin(CSOUND *csound, INSDS *ip)
{
  int32_t depth;
  int32_t state;
  int32_t result = CSOUND_SUCCESS;

  async_instance_lock(csound);
  depth = ATOMIC_GET(ip->init_running);
  state = ATOMIC_GET(ip->turnoff_pending);
  /* FINALIZING belongs to the performance-thread handoff. Do not resurrect an
     instance after its terminal turnoff has been published. */
  if (state == INSTANCE_TURNOFF_FINALIZING ||
      state == INSTANCE_TURNOFF_RECLAIM ||
      (state == INSTANCE_TURNOFF_REQUESTED && depth == 0)) {
    result = CSOUND_ERROR;
  }
  else
    ATOMIC_SET(ip->init_running, depth + 1);
  async_instance_unlock(csound);
  return result;
}

INSTANCE_INIT_RESULT instance_init_finish(CSOUND *csound, INSDS *ip)
{
  int32_t depth;
  INSTANCE_INIT_RESULT result = INSTANCE_INIT_DEFERRED;

  async_instance_lock(csound);
  depth = ATOMIC_GET(ip->init_running);
  if (depth <= 0) {
    async_instance_unlock(csound);
    return result;
  }
  ATOMIC_SET(ip->init_running, --depth);
  if (depth == 0) {
    if (ATOMIC_GET(ip->turnoff_pending) == INSTANCE_TURNOFF_REQUESTED) {
      ATOMIC_SET(ip->init_done, 0);
      ATOMIC_SET(ip->turnoff_pending, INSTANCE_TURNOFF_FINALIZING);
      enqueue_init_turnoff_locked(csound, ip);
      result = INSTANCE_INIT_TURNOFF;
    }
    else if (ATOMIC_GET(ip->turnoff_pending) == INSTANCE_TURNOFF_NONE)
      result = INSTANCE_INIT_COMPLETE;
  }
  async_instance_unlock(csound);
  return result;
}

void instance_init_request_turnoff(CSOUND *csound, INSDS *ip)
{
  int32_t state;

  async_instance_lock(csound);
  state = ATOMIC_GET(ip->turnoff_pending);
  ATOMIC_SET(ip->init_done, 0);
  if (state != INSTANCE_TURNOFF_FINALIZING &&
      state != INSTANCE_TURNOFF_RECLAIM) {
    if (ATOMIC_GET(ip->init_running) > 0) {
      ATOMIC_SET(ip->turnoff_pending, INSTANCE_TURNOFF_REQUESTED);
    }
    else {
      ATOMIC_SET(ip->turnoff_pending, INSTANCE_TURNOFF_FINALIZING);
      enqueue_init_turnoff_locked(csound, ip);
    }
  }
  async_instance_unlock(csound);
}

void instance_process_pending_turnoffs(CSOUND *csound)
{
  INSDS *pending;
  INSDS *reclaim = NULL;
  int32_t allocLocked = 0;

#if CSOUND_SPINLOCK_AVAILABLE
  if (csound->realtime_locks_initialized) {
    if (csoundSpinTryLock(&csound->alloc_spinlock) != CSOUND_SUCCESS)
      return;
    allocLocked = 1;
  }
#endif

  async_instance_lock(csound);
  pending = csound->init_turnoff_pending;
  csound->init_turnoff_pending = NULL;
  async_instance_unlock(csound);

  while (pending != NULL) {
    INSDS *ip = pending;
    INSDS *next = ip->init_turnoff_next;
    int32_t finalize = 0;
    int32_t freeNow = 0;
    int32_t waitForReaders = 0;
    int32_t state;

    ip->init_turnoff_next = NULL;
    async_instance_lock(csound);
    state = ATOMIC_GET(ip->turnoff_pending);
    finalize = state == INSTANCE_TURNOFF_FINALIZING &&
               ATOMIC_GET(ip->init_running) == 0;
    async_instance_unlock(csound);

    if (finalize) {
      if (ATOMIC_GET8(ip->actflg) == 0)
        ATOMIC_SET8(ip->actflg, 1);
      xturnoff_now_internal(csound, ip, 1);
    }

    async_instance_lock(csound);
    state = ATOMIC_GET(ip->turnoff_pending);
    if (state == INSTANCE_TURNOFF_FINALIZING ||
        state == INSTANCE_TURNOFF_RECLAIM) {
      waitForReaders = ATOMIC_GET(ip->free_pending) && !ip->linked &&
                       instance_has_async_refs(ip);
      if (waitForReaders) {
        ATOMIC_SET(ip->turnoff_pending, INSTANCE_TURNOFF_RECLAIM);
        enqueue_init_turnoff_locked(csound, ip);
      }
      else {
        freeNow = ATOMIC_GET(ip->free_pending) && !ip->linked;
        ATOMIC_SET(ip->free_pending, 0);
        ATOMIC_SET(ip->turnoff_pending, INSTANCE_TURNOFF_NONE);
      }
    }
    async_instance_unlock(csound);

    if (freeNow) {
      ip->init_turnoff_next = reclaim;
      reclaim = ip;
    }
    pending = next;
  }

  if (allocLocked)
    csoundSpinUnLock(&csound->alloc_spinlock);

  /* Unlinked destruction may release files and aggregate values. Keep that
     work outside the short allocation-lock section used for active chains. */
  while (reclaim != NULL) {
    INSDS *ip = reclaim;
    reclaim = ip->init_turnoff_next;
    ip->init_turnoff_next = NULL;
    free_unlinked_instance(csound, ip);
  }
}

static INSDS *take_inactive_instance(CSOUND *csound, INSTRTXT *tp)
{
  INSDS *current;
  INSDS *previous = NULL;

  inactive_instance_lock(csound);
  current = tp->act_instance;
  while (current != NULL && !instance_is_reclaimable(current)) {
    previous = current;
    current = current->nxtact;
  }
  if (current == NULL) {
    inactive_instance_unlock(csound);
    return NULL;
  }
  if (previous == NULL)
    tp->act_instance = current->nxtact;
  else
    previous->nxtact = current->nxtact;
  current->nxtact = NULL;
  inactive_instance_unlock(csound);
  /* A previous deactivation may have deferred this close until its final
     asynchronous reader released the instance. No reader remains here. */
  if (current->fdchp != NULL)
    fdchclose(csound, current);
  return current;
}

INSDS *allocate_or_take_instance(CSOUND *csound, INSTRTXT *tp,
                                 int32_t insno)
{
  INSDS *ip;

  ip = take_inactive_instance(csound, tp);
  if (ip != NULL)
    return ip;

  /* Building an instance may allocate and initialize a large opcode graph.
     Do that work before taking the instance-list lock, then publish the
     completed instance in one short critical section. It is returned directly,
     so it never enters the free list. */
  ip = instantiate(csound, insno, 0);
  if (UNLIKELY(ip == NULL))
    return NULL;

  tp = ip->instr;
  inactive_instance_lock(csound);
  ip->prvinstance = tp->lst_instance;
  if (tp->lst_instance != NULL)
    tp->lst_instance->nxtinstance = ip;
  else
    tp->instance = ip;
  tp->lst_instance = ip;
  ip->insno = (int16_t) insno;
  ip->linked = 1;
  inactive_instance_unlock(csound);
  return ip;
}

void recycle_inactive_instance(CSOUND *csound, INSDS *ip)
{
  INSTRTXT *tp = ip->instr;

  /* The caller owns an inactive instance detached from this reuse list. */
  inactive_instance_lock(csound);
  ip->nxtact = tp->act_instance;
  tp->act_instance = ip;
  inactive_instance_unlock(csound);
}

static void alloc_queue_lock(CSOUND *csound)
{
  csoundSpinLock(&csound->alloc_queue_spinlock);
}

static void alloc_queue_unlock(CSOUND *csound)
{
  csoundSpinUnLock(&csound->alloc_queue_spinlock);
}

/* These locks are reachable from the performance thread. A platform without
   a real spin primitive must reject asynchronous realtime mode rather than
   silently substitute a blocking mutex or Csound's no-op fallback. */
int32_t realtime_spin_lock_init(spin_lock_t *spinlock)
{
#if CSOUND_SPINLOCK_AVAILABLE
  return csoundSpinLockInit(spinlock);
#else
  IGN(spinlock);
  return CSOUND_ERROR;
#endif
}

void realtime_spin_lock_destroy(spin_lock_t *spinlock)
{
#if defined(__GNUC__) && defined(HAVE_PTHREAD_SPIN_LOCK)
  pthread_spin_destroy(spinlock);
#else
  IGN(spinlock);
#endif
}

int32_t alloc_queue_lock_init(CSOUND *csound)
{
  csound->alloc_queue_items = 0;
  csound->alloc_queue_wp = 0;
  return realtime_spin_lock_init(&csound->alloc_queue_spinlock);
}

void alloc_queue_lock_destroy(CSOUND *csound)
{
  realtime_spin_lock_destroy(&csound->alloc_queue_spinlock);
}

/* Reserve, fill, and publish a realtime allocation request. */
int32_t alloc_queue_enqueue(CSOUND *csound, const ALLOC_DATA *data)
{
  unsigned long wp;
  int32_t result = CSOUND_SUCCESS;

  if (UNLIKELY(csound->alloc_queue == NULL))
    return CSOUND_ERROR;

  alloc_queue_lock(csound);
  if (UNLIKELY(csound->alloc_queue_items >= MAX_ALLOC_QUEUE)) {
    result = CSOUND_ERROR;
  }
  else {
    wp = csound->alloc_queue_wp;
    csound->alloc_queue[wp] = *data;
    csound->alloc_queue_wp = wp + 1 < MAX_ALLOC_QUEUE ? wp + 1 : 0;
    csound->alloc_queue_items++;
  }
  alloc_queue_unlock(csound);
  return result;
}

static int32_t alloc_queue_dequeue(CSOUND *csound, ALLOC_DATA *data,
                                   unsigned long *readPosition)
{
  int32_t result = 0;

  alloc_queue_lock(csound);
  if (csound->alloc_queue_items > 0) {
    *data = csound->alloc_queue[*readPosition];
    *readPosition = *readPosition + 1 < MAX_ALLOC_QUEUE ?
      *readPosition + 1 : 0;
    csound->alloc_queue_items--;
    result = 1;
  }
  alloc_queue_unlock(csound);
  return result;
}

static int32_t alloc_queue_has_items(CSOUND *csound)
{
  int32_t result;

  alloc_queue_lock(csound);
  result = csound->alloc_queue_items > 0;
  alloc_queue_unlock(csound);
  return result;
}

static size_t evtblk_strarg_size(const EVTBLK *src)
{
  char *end = src->strarg;
  int32_t n = src->scnt;

  if (end == NULL)
    return 0;

  if (n <= 0)
    return strlen(end) + 1;

  while (n--)
    end += strlen(end) + 1;
  return (size_t)(end - src->strarg);
}

static void free_queued_evtblk_pfields(CSOUND *csound, EVTBLK *evt)
{
  if (evt->p != NULL) {
    csound->Free(csound, evt->p);
    evt->p = NULL;
  }
}

static void free_queued_evtblk(CSOUND *csound, EVTBLK *evt)
{
  free_queued_evtblk_pfields(csound, evt);
  if (evt->strarg != NULL) {
    csound->Free(csound, evt->strarg);
    evt->strarg = NULL;
  }
}

static int32_t copy_evtblk_for_queue(CSOUND *csound, EVTBLK *dst,
                                     const EVTBLK *src)
{
  *dst = *src;
  dst->p = NULL;
  dst->strarg = NULL;

  if (src->p != NULL && src->pcnt >= 0) {
    size_t bytes = sizeof(MYFLT) * ((size_t)src->pcnt + 1U);
    dst->p = (MYFLT *) csound->Malloc(csound, bytes);
    if (UNLIKELY(dst->p == NULL))
      return CSOUND_MEMORY;
    memcpy(dst->p, src->p, bytes);
  }

  if (src->strarg != NULL) {
    size_t bytes = evtblk_strarg_size(src);
    dst->strarg = (char *) csound->Malloc(csound, bytes);
    if (UNLIKELY(dst->strarg == NULL)) {
      free_queued_evtblk(csound, dst);
      return CSOUND_MEMORY;
    }
    memcpy(dst->strarg, src->strarg, bytes);
  }

  return CSOUND_SUCCESS;
}

/* Helper function to get type string from argument without unsafe casting */
static char* get_arg_type_from_arg(ARG *arg, CS_VARIABLE **var) {
    if (arg->type == ARG_CONSTANT) {
      *var = NULL;
      return "c";
    } else if (arg->type == ARG_STRING) {
      *var = NULL;
      return "S";
    } else if (arg->type == ARG_PFIELD) {
      *var = NULL;
      return "p";
    } else if (arg->type == ARG_LABEL) {
      *var = NULL;
      return "l";
    } else {
      *var = (CS_VARIABLE *) arg->argPtr;
      return (*var)->varType->varTypeName;
    }
}

static void print_messages(CSOUND *csound, int32_t attr, const char *str){
#if defined(WIN32)
  switch (attr & CSOUNDMSG_TYPE_MASK) {
  case CSOUNDMSG_ERROR:
  case CSOUNDMSG_WARNING:
  case CSOUNDMSG_REALTIME:
    fprintf(stderr, str);
    break;
  default:
    fprintf(stdout, str);
  }
#else
  FILE *fp = stderr;
  if ((attr & CSOUNDMSG_TYPE_MASK) == CSOUNDMSG_STDOUT)
    fp = stdout;
  if (!attr || !csound->enableMsgAttr) {
    fprintf(fp, "%s", str);
    return;
  }
  if ((attr & CSOUNDMSG_TYPE_MASK) == CSOUNDMSG_ORCH)
    if (attr & CSOUNDMSG_BG_COLOR_MASK)
      fprintf(fp, "\033[4%cm", ((attr & 0x70) >> 4) + '0');
  if (attr & CSOUNDMSG_FG_ATTR_MASK) {
    if (attr & CSOUNDMSG_FG_BOLD)
      fprintf(fp, "\033[1m");
    if (attr & CSOUNDMSG_FG_UNDERLINE)
      fprintf(fp, "\033[4m");
  }
  if (attr & CSOUNDMSG_FG_COLOR_MASK)
    fprintf(fp, "\033[3%cm", (attr & 7) + '0');
  fprintf(fp, "%s", str);
  fprintf(fp, "\033[m");
#endif
}

#define QUEUESIZ 64

static void message_string_enqueue(CSOUND *csound, int32_t attr,
                                   const char *str) {
  unsigned long wp = csound->message_string_queue_wp;
  csound->message_string_queue[wp].attr = attr;
  strNcpy(csound->message_string_queue[wp].str, str, MAX_MESSAGE_STR);
  //csound->message_string_queue[wp].str[MAX_MESSAGE_STR-1] = '\0';
  csound->message_string_queue_wp = wp + 1 < QUEUESIZ ? wp + 1 : 0;
  ATOMIC_INCR(csound->message_string_queue_items);
}

static void no_op(CSOUND *csound, int32_t attr,
                  const char *format, va_list args) {
  IGN(csound);
  IGN(attr);
  IGN(format);
  IGN(args);
};

/* do init pass for this instr */
static int32_t init_pass(CSOUND *csound, INSDS *ip) {
  int32_t error = 0;
  OPDS *ids = csound->ids;
  INSDS *curip = csound->curip;
  if(csound->oparms->realtime)
    csoundLockMutex(csound->init_pass_threadlock);
  csound->curip = ip;
  csound->ids = (OPDS *)ip;
  while (error == 0 && (csound->ids = csound->ids->nxti) != NULL) {
    csound->mode = 1;
    csound->op = csound->ids->optext->t.oentry->opname;
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
      csound->Message(csound, "init %s (%p):\n", csound->op, csound->ids);
     }
    error = (*csound->ids->init)(csound, csound->ids);
    csound->mode = 0;
  }
  csound->ids = ids;
  csound->curip = curip;

  if (error == 0) {
    recycle_init_only_udo_instances(csound, ip);
  }

  if(csound->oparms->realtime)
    csoundUnlockMutex(csound->init_pass_threadlock);

  return error;
}

int32_t rireturn(CSOUND *csound, void *p);

/* do reinit pass */
static int32_t reinit_pass(CSOUND *csound, INSDS *ip, OPDS *ids) {
  int32_t error = 0;
  if(csound->oparms->realtime) {
    csoundLockMutex(csound->init_pass_threadlock);
  }
  /* Each queued pass owns a complete reinit interval. An earlier queued pass
     may already have cleared these flags before this one starts. */
  csound->reinitflag = ip->reinitflag = 1;
  csound->curip = ip;
  csound->ids = ids;
  csound->mode = 1;
  while (error == 0 && (csound->ids = csound->ids->nxti) != NULL &&
         (csound->ids->init != (SUBR) rireturn)){
    csound->op = csound->ids->optext->t.oentry->opname;
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
      csound->Message(csound, "reinit %s:\n", csound->op);
    error = (*csound->ids->init)(csound, csound->ids);
  }
  csound->mode = 0;

  csound->reinitflag = ip->reinitflag = 0;
  if(csound->oparms->realtime)
    csoundUnlockMutex(csound->init_pass_threadlock);
  return error;
}

static int32_t realtime_init_pass(CSOUND *csound, INSDS *ip)
{
  int32_t error;

  /* Init opcodes may acquire alloc_spinlock; the init mutex remains held. */
  csoundSpinUnLock(&csound->alloc_spinlock);
  error = init_pass(csound, ip);
  csoundSpinLock(&csound->alloc_spinlock);
  return error;
}

static int32_t realtime_reinit_pass(CSOUND *csound, INSDS *ip, OPDS *ids)
{
  int32_t error;

  /* Reinit has the same lock requirements as the initial pass. */
  csoundSpinUnLock(&csound->alloc_spinlock);
  error = reinit_pass(csound, ip, ids);
  csoundSpinLock(&csound->alloc_spinlock);
  return error;
}

/*
 * creates a thread to process instance allocations
 */
uintptr_t event_insert_thread(void *p) {
  CSOUND *csound = (CSOUND *) p;
  float wakeup = (1000*csound->ksmps/csound->esr);
  unsigned long rp = 0, items, rpm = 0;
  message_string_queue_t *mess = NULL;
  void (*csoundMessageStringCallback)(CSOUND *csound,
                                      int32_t attr,
                                      const char *str) = NULL;
  void (*csoundMessageCallback)(CSOUND *csound,
                                int32_t attr,
                                const char *format,
                                va_list args)
    = csound->csoundMessageCallback_;
  if(csound->oparms_.msglevel){
    if(csound->message_string_queue == NULL)
      csound->message_string_queue = (message_string_queue_t *)
        csound->Calloc(csound, QUEUESIZ*sizeof(message_string_queue_t));
    mess = csound->message_string_queue;
    if(csound->csoundMessageStringCallback)
      csoundMessageStringCallback = csound->csoundMessageStringCallback;
    else csoundMessageStringCallback = print_messages;
    csoundSetMessageStringCallback(csound, message_string_enqueue);
  } else {
    csoundSetMessageCallback(csound, no_op);
  }

  for (;;) {
    ALLOC_DATA data;
    uint32_t processed = 0;

    while (processed < MAX_ALLOC_QUEUE &&
           alloc_queue_dequeue(csound, &data, &rp)) {
      /* Keep the common lock order: init mutex, then allocation lock. */
      if (csound->init_pass_threadlock)
        csoundLockMutex(csound->init_pass_threadlock);
      switch (data.type) {
        case ALLOC_DATA_MERGE_STATE: {
          ENGINE_STATE *engine_state = data.engine_state;
          TYPE_TABLE *type_table = data.type_table;
          OPDS *ids = data.ids;
          merge_state_realtime(csound, engine_state, type_table, ids);
          break;
        }

        case ALLOC_DATA_REINIT_PASS: {
          INSDS *ip = data.ip;
          OPDS *ids = data.ids;
          int32_t error;
          INSTANCE_INIT_RESULT initResult;
          csoundSpinLock(&csound->alloc_spinlock);
          error = realtime_reinit_pass(csound, ip, ids);
          initResult = instance_init_finish(csound, ip);
          if (error == 0 && initResult != INSTANCE_INIT_TURNOFF) {
            ATOMIC_SET8(ip->actflg, 1);
            if (initResult == INSTANCE_INIT_COMPLETE)
              ATOMIC_SET(ip->init_done, 1);
          }
          else
            ATOMIC_SET(ip->init_done, 0);
          if (error != 0)
            instance_init_request_turnoff(csound, ip);
          csoundSpinUnLock(&csound->alloc_spinlock);
          break;
        }

        case ALLOC_DATA_INIT_PASS: {
          INSDS *ip = data.ip;
          int32_t error;
          INSTANCE_INIT_RESULT initResult;
          ATOMIC_SET(ip->init_done, 0);
          if (UNLIKELY(instance_init_begin(csound, ip) != CSOUND_SUCCESS))
            break;
          csoundSpinLock(&csound->alloc_spinlock);
          error = realtime_init_pass(csound, ip);
          initResult = instance_init_finish(csound, ip);
          if (initResult == INSTANCE_INIT_COMPLETE && error == 0)
            ATOMIC_SET(ip->init_done, 1);
          if (error != 0)
            instance_init_request_turnoff(csound, ip);
          csoundSpinUnLock(&csound->alloc_spinlock);
          break;
        }

        case ALLOC_DATA_MIDI_EVENT:
          csoundSpinLock(&csound->alloc_spinlock);
          insert_midi(csound, data.insno, data.chn, &data.mep);
          csoundSpinUnLock(&csound->alloc_spinlock);
          break;

        case ALLOC_DATA_SCORE_EVENT:
        {
          int32_t result;
          csoundSpinLock(&csound->alloc_spinlock);
          result = insert(csound, data.insno, &data.blk);
          csoundSpinUnLock(&csound->alloc_spinlock);
          if (result == 0) {
            /* insert() copies p-fields, but keeps strarg for INSDS::strarg. */
            free_queued_evtblk_pfields(csound, &data.blk);
            data.blk.strarg = NULL;
          }
          else {
            free_queued_evtblk(csound, &data.blk);
          }
          break;
        }
      }
      if (csound->init_pass_threadlock)
        csoundUnlockMutex(csound->init_pass_threadlock);
      processed++;
    }
    diskin2_async_drain_deferred(csound);
    if(processed == 0)
      csoundSleep((int32_t) ((int32_t) wakeup > 0 ? wakeup : 1));
    items = ATOMIC_GET(csound->message_string_queue_items);
    while(items) {
      if(mess != NULL)
        csoundMessageStringCallback(csound, mess[rpm].attr,  mess[rpm].str);
      ATOMIC_DECR(csound->message_string_queue_items);
      items--;
      rpm = rpm + 1 < QUEUESIZ ? rpm + 1 : 0;
    }
    /* Stop requests reject new work at the engine boundary. Drain all
       published work so init depths and copied score data are not stranded. */
    if (!csound->event_insert_loop && !alloc_queue_has_items(csound))
      break;
  }

  csoundSetMessageCallback(csound, csoundMessageCallback);
  return (uintptr_t) NULL;
}

int32_t init0(CSOUND *csound)
{
  INSTRTXT  *tp = csound->engineState.instrtxtp[0];
  INSDS     *ip;

  csound->curip = ip = allocate_or_take_instance(csound, tp, 0);
  if (UNLIKELY(ip == NULL))
    return csound->InitError(csound, "%s",
                             Str("could not allocate instrument 0"));
  csound->ids = (OPDS*) ip;
  tp->active++;
  ip->actflg++;
  ip->esr = csound->esr;
  ip->pidsr = csound->pidsr;
  ip->sicvt = csound->sicvt;
  ip->onedsr = csound->onedsr;
  ip->ksmps = csound->ksmps;
  ip->ekr = csound->ekr;
  ip->kcounter = csound->kcounter;
  ip->onedksmps = csound->onedksmps;
  ip->onedkr = csound->onedkr;
  ip->kicvt = csound->kicvt;
  csound->inerrcnt = 0;
  csound->mode = 1;

  while ((csound->ids = csound->ids->nxti) != NULL) {
    csound->op = csound->ids->optext->t.oentry->opname;
    (*csound->ids->init)(csound, csound->ids);  /*   run all i-code     */
  }
  csound->mode = 0;
  return csound->inerrcnt;                        /*   return errcnt      */
}

static int32_t print_opcall(CSOUND *csound, TEXT *tp)
{
  int32_t n, nn;
  char *name;
  ARG *arg;

  if(!strcmp(tp->opcod, "endin") ||
     !strcmp(tp->opcod, "endop")) {
    csound->Message(csound, "%sn", tp->opcod);
    return 0;
  }

  if (tp->outlist && (n = tp->outlist->count) != 0) {
    nn = 0;
    arg = tp->outArgs;
    CS_VARIABLE *var = NULL;
    char *type = get_arg_type_from_arg(arg, &var);
    char  arrtype[64];

    while (n-- > 1) {
      if(*type == '[') {
        if (var != NULL && var->subType != NULL) {
          snprintf(arrtype, 64, "%s[]", var->subType->varTypeName);
        } else {
          snprintf(arrtype, 64, "unknown[]");
        }
        type = arrtype;
      }
      csound->Message(csound, "%s:%s,", tp->outlist->arg[nn++], type);
      arg = arg->next;
      type = get_arg_type_from_arg(arg, &var);
    }
    if(*type == '[') {
      if (var != NULL && var->subType != NULL) {
        snprintf(arrtype, 64, "%s[]", var->subType->varTypeName);
      } else {
        snprintf(arrtype, 64, "unknown[]");
      }
      type = arrtype;
    }
    csound->Message(csound, "%s:%s ", tp->outlist->arg[nn++], type);
  }
  name = strip_extension(csound, tp->opcod);
  csound->Message(csound, "%s ", name);
  if (tp->inlist  && (n = tp->inlist->count) != 0) {
    nn = 0;
    arg = tp->inArgs;
    CS_VARIABLE *var = NULL;
    char *type = get_arg_type_from_arg(arg, &var);
    char  arrtype[64];

    while (n-- > 1) {
      if(*type == '[') {
        if (var != NULL && var->subType != NULL) {
          snprintf(arrtype, 64, "%s[]", var->subType->varTypeName);
        } else {
          snprintf(arrtype, 64, "unknown[]");
        }
        type = arrtype;
      }
      csound->Message(csound, "%s:%s,", tp->inlist->arg[nn++], type);
      arg = arg->next;
      type = get_arg_type_from_arg(arg, &var);
    }
    if(*type == '[') {
      if (var != NULL && var->subType != NULL) {
        snprintf(arrtype, 64, "%s[]", var->subType->varTypeName);
      } else {
        snprintf(arrtype, 64, "unknown[]");
      }
      type = arrtype;
    }
    csound->Message(csound, "%s:%s", tp->inlist->arg[nn++], type);
  }
  csound->Message(csound,"\n");
  return 1;
}

static void set_xtratim(CSOUND *csound, INSDS *ip)
{
  if (UNLIKELY(ip->relesing))
    return;
  ip->offtim = (csound->icurTimeSamples +
                ip->ksmps * (double) ip->xtratim)/csound->esr;
  ip->offbet = csound->curBeat + (csound->curBeat_inc * (double) ip->xtratim);
  ip->relesing = 1;
  csound->engineState.instrtxtp[ip->insno]->pending_release++;
}

static void release_cpu_power(CSOUND *csound, INSTRTXT *tp)
{
  if (tp->cpuload > FL(0.0))
    csound->cpu_power_busy -= tp->cpuload;
}

/* insert an instr copy into active list */
/*      then run an init pass            */
int32_t insert_event(CSOUND *csound, int32_t insno, EVTBLK *newevtp) {

  if(csound->oparms->realtime) {
    ALLOC_DATA data = { 0 };
    int32_t result;
    data.insno = insno;
    data.type = ALLOC_DATA_SCORE_EVENT;
    result = copy_evtblk_for_queue(csound, &data.blk, newevtp);
    if (UNLIKELY(result != CSOUND_SUCCESS))
      return result;
    result = alloc_queue_enqueue(csound, &data);
    if (UNLIKELY(result != CSOUND_SUCCESS))
      free_queued_evtblk(csound, &data.blk);
    return result;
  }
  else return insert(csound, insno, newevtp);
}

void maxalloc_turnoff(CSOUND *csound, int32_t insno) {
  INSTRTXT  *tp = csound->engineState.instrtxtp[insno];

  //turnoff mode: 0 do not turn off, 1 turnoff oldest, 2 turnoff newest
  if (tp->turnoff_mode > 0) {
    INSDS *ip, *ip2, *nip;
    ip = &(csound->actanchor);
    ip2 = NULL;
    while ((ip = ip->nxtact) != NULL && (int32_t) ip->insno != insno);

    if (ip != NULL) {
      do {
        nip = ip->nxtact;
        ip2 = ip;
        if (tp->turnoff_mode == 1) //turnoff oldest
          break;
        ip = nip;
      } while (ip != NULL && (int32_t) ip->insno == insno);
    }
    if (ip2 != NULL) {
      xturnoff_now(csound, ip2);
      if (!ip2->actflg) {  /* if current note was deactivated: */
        while (ip2->pds != NULL && ip2->pds->nxtp != NULL)
          ip2->pds = ip2->pds->nxtp;            /* loop to last opds */
      }
    }
  }
}


/* insert new event with different instance orderings
   order = 0 - standard order
   order = 1 - add to the end of chain
*/
static int32_t insert_new(CSOUND *csound, int32_t insno,
                   EVTBLK *newevtp, int32_t order)
{
  INSTRTXT  *tp;
  INSDS     *ip;
  OPARMS    *O = csound->oparms;
  CS_VAR_MEM *pfields = NULL;        /* *** was uninitialised *** */
  int32_t   tie = 0, i;
  int32_t  n, error = 0;
  INSTANCE_INIT_RESULT initResult;
  MYFLT  *flp, *fep;
  MYFLT newp1 = 0;

  if (UNLIKELY(csound->advanceCnt))
    return 0;
  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
    char *name = csound->engineState.instrtxtp[insno]->insname;
    if (UNLIKELY(name))
      csound->Message(csound, Str("activating instr %s at %"PRIi64"\n"),
                      name, csound->icurTimeSamples);
    else
      csound->Message(csound, Str("activating instr %d at %"PRIi64"\n"),
                      insno, csound->icurTimeSamples);
  }
  csound->inerrcnt = 0;

  tp = csound->engineState.instrtxtp[insno];
  if (UNLIKELY(tp->muted == 0)) {
    char *name = csound->engineState.instrtxtp[insno]->insname;
    if (UNLIKELY(name))
      csound->Warning(csound, Str("Instrument %s muted\n"), name);
    else
      csound->Warning(csound, Str("Instrument %d muted\n"), insno);
    return 0;
  }
  if (tp->cpuload > FL(0.0)) {
    csound->cpu_power_busy += tp->cpuload;
    /* if there is no more cpu processing time*/
    if (UNLIKELY(csound->cpu_power_busy > FL(100.0))) {
      csound->cpu_power_busy -= tp->cpuload;
      csoundWarning(csound, Str("cannot allocate last note because "
                                "it exceeds 100%% of cpu time"));
      return(0);
    }
  }
  if (UNLIKELY(tp->maxalloc > 0 && tp->active >= tp->maxalloc)) {
    maxalloc_turnoff(csound, insno);
    if (tp->active >= tp->maxalloc) {
      release_cpu_power(csound, tp);
      csoundWarning(csound, Str("cannot allocate last note because it exceeds "
                                "instr maxalloc"));
      return(0);
    }
  }
  /* If named ensure we have the fraction */
  if (csound->engineState.instrtxtp[insno]->insname && newevtp->strarg)
    newp1 = named_instr_find(csound, newevtp->strarg);

  newevtp->p[1] = newp1 != 0 ? newp1 : newevtp->p[1];
  /* if find this insno, active, with indef (tie) & matching p1
     and tie was not suppressed */
  for (ip = tp->instance; ip != NULL; ip = ip->nxtinstance) {
    if (ip->actflg && ip->offtim < 0.0
        && ip->p1.value == newevtp->p[1]) {
      csound->tieflag++;
      ip->tieflag = 1;
      tie = 1;
      break;
    }
  }
  if (tie) {
    ATOMIC_SET(ip->init_done, 0);
    if (UNLIKELY(instance_init_begin(csound, ip) != CSOUND_SUCCESS)) {
      ip->tieflag = 0;
      if (csound->tieflag > 0)
        csound->tieflag--;
      release_cpu_power(csound, tp);
      return csound->InitError(csound,
                               Str("cannot reinitialize instrument %d while "
                                   "it is being turned off"), insno);
    }
  }

  if(!tie) {
    /* alloc new dspace if needed */
    ip = tp->isNew ? NULL : take_inactive_instance(csound, tp);
    if (ip == NULL) {
      csound->instance_count++;
      if (UNLIKELY(O->msglevel & CS_RNGEMSG)) {
        char *name = csound->engineState.instrtxtp[insno]->insname;
        if (UNLIKELY(name))
          csound->ErrorMsg(csound, Str("new alloc for instr %s:\n"), name);
        else
          csound->ErrorMsg(csound, Str("new alloc for instr %d:\n"), insno);
      }
      instance(csound, insno);
      tp->isNew=0;
      ip = take_inactive_instance(csound, tp);
    }
    if (UNLIKELY(ip == NULL)) {
      release_cpu_power(csound, tp);
      return csound->InitError(csound,
                               Str("could not allocate instrument %d"), insno);
    }
    if(csoundGetDebug(csound) & DEBUG_RUNTIME)
      csoundMessage(csound, "insert(): instance = %p\n", ip);
    ATOMIC_SET(ip->init_done, 0);
    ip->insno = (int16) insno;
    ip->esr = csound->esr;
    ip->pidsr = csound->pidsr;
    ip->sicvt = csound->sicvt;
    ip->onedsr = csound->onedsr;
    ip->ksmps = csound->ksmps;
    ip->ekr = csound->ekr;
    ip->kcounter = csound->kcounter;
    ip->onedksmps = csound->onedksmps;
    ip->onedkr = csound->onedkr;
    ip->kicvt = csound->kicvt;
    ip->pds = NULL;
    if (UNLIKELY(instance_init_begin(csound, ip) != CSOUND_SUCCESS)) {
      release_cpu_power(csound, tp);
      return csound->InitError(csound,
                               Str("cannot initialize instrument %d while "
                                   "it is being turned off"), insno);
    }
    /* Add an active instrument */
    tp->active++;
    tp->instcnt++;
    csound->dag_changed++;      /* Need to remake DAG */
    if(order == 1) { // MODE 1 = add to end
      INSDS *prvp, *nxtp;
      nxtp = &(csound->actanchor);
      // splice at end of chain
      while ((prvp = nxtp) &&
             (nxtp = prvp->nxtact) != NULL)
        ;
      ip->nxtact = nxtp;
      ip->prvact = prvp;
      prvp->nxtact = ip;
    }
    if(order == 2) { // MODE 2 = add to start
      INSDS *nxtp;
      nxtp = &(csound->actanchor);
      // splice at the top of chaing
      ip->nxtact = nxtp->nxtact;
      nxtp->nxtact->prvact = ip;
      if (nxtp->nxtact) nxtp->nxtact->prvact = ip;
      ip->prvact = nxtp;
    }
    else {  // default order
      INSDS *prvp, *nxtp;
      nxtp = &(csound->actanchor);
      // standard splice: instrument number and p1 ascending order
      while ((prvp = nxtp) && (nxtp = prvp->nxtact) != NULL) {
        if (nxtp->insno > insno ||
            (nxtp->insno == insno && nxtp->p1.value > newevtp->p[1])) {
          nxtp->prvact = ip;
          break;
        }
      }
      ip->nxtact = nxtp;
      ip->prvact = prvp;
      prvp->nxtact = ip;
    }

    ip->tieflag = 0;
    ip->actflg++;                   /*    and mark the instr active */
    if(ip->instance_id == 0)
      ip->instance_id = csound->instance_count;
  }


  /* init: */
  pfields = (CS_VAR_MEM*)&ip->p0;
  if (tp->psetdata) {
    int32_t i;
    CS_VAR_MEM* pfields = (CS_VAR_MEM*) &ip->p0;
    MYFLT *pdat = tp->psetdata + 2;
    int32 nn = tp->pmax - 2;             /*   put cur vals in pflds */

    for (i = 0; i < nn; i++) {
      CS_VAR_MEM* pfield = (pfields + i + 3);
      pfield->value = *(pdat + i);
    }
  }
  n = tp->pmax;
  if (UNLIKELY((tp->nocheckpcnt == 0) &&
               n != newevtp->pcnt &&
               !tp->psetdata)) {
    char *name = csound->engineState.instrtxtp[insno]->insname;
    if (UNLIKELY(name))
      csoundWarning(csound, Str("instr %s uses %d p-fields but is given %d"),
                    name, n, newevtp->pcnt);
    else
      csoundWarning(csound, Str("instr %d uses %d p-fields but is given %d"),
                    insno, n, newevtp->pcnt);
  }
  if (newevtp->p3orig >= FL(0.0))
    ip->offbet = csound->beatOffs
      + (double) newevtp->p2orig + (double) newevtp->p3orig;
  else
    ip->offbet = -1.0;
  flp = &ip->p1.value;
  fep = &newevtp->p[0];

  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
    csound->Message(csound, "psave beg at %p\n", (void*) flp);
  if (n > newevtp->pcnt) n = newevtp->pcnt; /* IV - Oct 20 2002 */
  for (i = 1; i < n + 1; i++) {
    CS_VAR_MEM* pfield = pfields + i;
    pfield->varType = (CS_TYPE*)&CS_VAR_TYPE_P;
    pfield->value = fep[i];
  }
  if (n < tp->pmax && tp->psetdata==NULL) {
    for (i = 0; i < tp->pmax - n; i++) {
      CS_VAR_MEM* pfield = pfields + i + n + 1;
      pfield->varType = (CS_TYPE*)&CS_VAR_TYPE_P;
      pfield->value = 0;
    }
  }
  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
    csound->Message(csound, "   ending at %p\n", (void*) flp);

  if (O->Beatmode)
    ip->p2.value     = (MYFLT) (csound->icurTimeSamples/csound->esr - csound->timeOffs);
  ip->offtim       = (double) ip->p3.value;         /* & duplicate p3 for now */
  ip->m_chnbp      = (MCHNBLK*) NULL;
  ip->xtratim      = 0;
  ip->relesing     = 0;
  ip->m_sust       = 0;
  ip->nxtolap      = NULL;
  ip->opcod_iobufs = NULL;
  ip->strarg       = newevtp->strarg;  /* copy strarg so it does not get lost */

  /* new code for sample-accurate timing, not for tied notes */
  /* VL 18 Dec 24 - needs to be set before init pass to propagate to UDOS */
  if (O->sampleAccurate && !tie) {
    int64_t start_time_samps, start_time_kcycles;
    double duration_samps;
    start_time_samps = (int64_t) (ip->p2.value * csound->esr);
    duration_samps =  ip->p3.value * csound->esr;
    start_time_kcycles = start_time_samps/csound->ksmps;
    ip->ksmps_offset = (uint32_t) (start_time_samps - start_time_kcycles*csound->ksmps);
    /* with no p3 or xtratim values, can't set the sample accur duration */
    if (ip->p3.value > 0 && ip->xtratim == 0 ){
      int32_t tmp = ((int)duration_samps+ip->ksmps_offset)%csound->ksmps;
      if (tmp != 0)ip->no_end = csound->ksmps - tmp; else ip->no_end = 0;
    }
    else ip->no_end = 0;
    ip->ksmps_no_end = 0;
  }
  else {
    /* ksmps_offset = */
    ip->ksmps_offset = 0;
    ip->ksmps_no_end = 0;
    ip->no_end = 0;
  }

  // current event needs to be reset here
  csound->init_event = newevtp;
  error = csound->oparms->realtime ? realtime_init_pass(csound, ip) :
    init_pass(csound, ip);
  initResult = instance_init_finish(csound, ip);
  if (initResult == INSTANCE_INIT_COMPLETE && error == 0) {
    ATOMIC_SET(ip->init_done, 1);
  }
  else {
    ATOMIC_SET(ip->init_done, 0);
  }
  if (initResult == INSTANCE_INIT_TURNOFF) {
    return error != 0 ? error : csound->inerrcnt;
  }
  if (UNLIKELY(csound->inerrcnt || ip->p3.value == FL(0.0))) {
    if (csound->oparms->realtime)
      instance_init_request_turnoff(csound, ip);
    else
      xturnoff_now(csound, ip);
    return csound->inerrcnt;
  }

#ifdef BETA
  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
    csound->Message(csound, "In insert:  %d %lf %lf\n",
                    __LINE__, ip->p3.value, ip->offtim); /* *********** */
#endif
  if (ip->p3.value > FL(0.0) && ip->offtim > 0.0) { /* if still finite time, */
    double p2 = (double) ip->p2.value + csound->timeOffs;
    ip->offtim = p2 + (double) ip->p3.value;
    if (O->sampleAccurate && !tie  &&
        ip->p3.value > 0 &&
        ip->xtratim == 0) /* ceil for sample-accurate ending */
      ip->offtim = CEIL(ip->offtim*csound->ekr) / csound->ekr;
    else /* normal : round */
      ip->offtim = FLOOR(ip->offtim * csound->ekr +0.5)/csound->ekr;
    if (O->Beatmode) {
      p2 = ((p2*csound->esr - csound->icurTimeSamples) / csound->ibeatTime)
        + csound->curBeat;
      ip->offbet = p2 + ((double) ip->p3.value*csound->esr / csound->ibeatTime);
    }
#ifdef BETA
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
      csound->Message(csound,
                      "Calling sched_off_time line %d; offtime= %lf (%lf)\n",
                      __LINE__, ip->offtim, ip->offtim*csound->ekr);
#endif
    if(csound->oparms->realtime) // compensate for possible late starts
      {
        double p2 = (double) ip->p2.value + csound->timeOffs;
        ip->offtim += (csound->icurTimeSamples/csound->esr - p2);
      }
    sched_off_time(csound, ip);                  /*   put in turnoff list */
  }
  else {
    ip->offbet = -1.0;
    ip->offtim = -1.0;                        /*   else mark indef     */
  }

  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
    char *name = csound->engineState.instrtxtp[insno]->insname;
    if (UNLIKELY(name))
      csound->Message(csound, Str("instr %s now active:\n"), name);
    else
      csound->Message(csound, Str("instr %d now active:\n"), insno);
    show_allocs(csound);
  }
  if (newevtp->pinstance != NULL) {
    /* place instance on output var memory */
    memcpy(newevtp->pinstance, &ip, sizeof(INSDS *));
  }
  return 0;
}


int32_t insert(CSOUND *csound, int32_t insno,
                     EVTBLK *newevtp) {
  return insert_new(csound, insno, newevtp, 0);
}


/* insert a MIDI instr copy into active list */
/*  then run an init pass                    */
int32_t insert_midi_event(CSOUND *csound, int32_t insno, MCHNBLK *chn,
                          MEVENT *mep) {

  if(csound->oparms->realtime) {
    ALLOC_DATA data = { 0 };
    data.insno = insno;
    data.chn = chn;
    data.mep = *mep;
    data.type = ALLOC_DATA_MIDI_EVENT;
    return alloc_queue_enqueue(csound, &data);
  }
  else return insert_midi(csound, insno, chn, mep);

}

int32_t insert_midi(CSOUND *csound, int32_t insno, MCHNBLK *chn, MEVENT *mep)
{
  INSTRTXT  *tp;
  INSDS     *ip, **ipp, *prvp, *nxtp;
  OPARMS    *O = csound->oparms;
  CS_VAR_MEM *pfields;
  int32_t pmax = 0, error = 0;
  INSTANCE_INIT_RESULT initResult;

  if (UNLIKELY(csound->advanceCnt))
    return 0;
  if (UNLIKELY(insno < 0 || csound->engineState.instrtxtp[insno]->muted == 0))
    return 0;     /* muted */

  tp = csound->engineState.instrtxtp[insno];
  if (tp->cpuload > FL(0.0)) {
    csound->cpu_power_busy += tp->cpuload;
    if (UNLIKELY(csound->cpu_power_busy > FL(100.0))) {
      /* if there is no more cpu time */
      csound->cpu_power_busy -= tp->cpuload;
      csoundWarning(csound, Str("cannot allocate last note because "
                                "it exceeds 100%% of cpu time"));
      return(0);
    }
  }
  if (UNLIKELY(tp->maxalloc > 0 && tp->active >= tp->maxalloc)) {
    maxalloc_turnoff(csound, insno);
    if (tp->active >= tp->maxalloc) {
      release_cpu_power(csound, tp);
      csoundWarning(csound, Str("cannot allocate last note because it exceeds "
                                "instr maxalloc"));
      return(0);

    }
  }
  tp->active++;
  tp->instcnt++;
  csound->dag_changed++;      /* Need to remake DAG */
  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
    char *name = csound->engineState.instrtxtp[insno]->insname;
    if (UNLIKELY(name))
      csound->Message(csound, Str("MIDI activating instr %s\n"), name);
    else
      csound->Message(csound, Str("MIDI activating instr %d\n"), insno);
  }
  csound->inerrcnt = 0;
  ipp = &chn->kinsptr[mep->dat1];       /* key insptr ptr           */
  /* alloc new dspace if needed */
  ip = tp->isNew ? NULL : take_inactive_instance(csound, tp);
  if (ip == NULL) {
    if (UNLIKELY(O->msglevel & CS_RNGEMSG)) {
      char *name = csound->engineState.instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Message(csound, Str("new MIDI alloc for instr %s:\n"), name);
      else
        csound->Message(csound, Str("new MIDI alloc for instr %d:\n"), insno);
    }
    instance(csound, insno);
    tp->isNew = 0;
    ip = take_inactive_instance(csound, tp);
  }
  if (UNLIKELY(ip == NULL)) {
    tp->active--;
    tp->instcnt--;
    release_cpu_power(csound, tp);
    return csound->InitError(csound,
                             Str("could not allocate instrument %d"), insno);
  }
  ATOMIC_SET(ip->init_done, 0);
  ip->insno = (int16) insno;
  /* A MIDI note-off can arrive as soon as the instance is published below. */
  if (UNLIKELY(instance_init_begin(csound, ip) != CSOUND_SUCCESS)) {
    tp->active--;
    tp->instcnt--;
    release_cpu_power(csound, tp);
    return csound->InitError(csound,
                             Str("cannot initialize instrument %d while "
                                 "it is being turned off"), insno);
  }

  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
    csound->Message(csound, "Now %d active instr %d\n", tp->active, insno);
  if (UNLIKELY((prvp = *ipp) != NULL)) {          /*   if key currently activ */
    if(O->msglevel & 0x400)
    csoundWarning(csound,
                  Str("MIDI note overlaps with key %d on same channel"),
                  (int32_t) mep->dat1);
    while (prvp->nxtolap != NULL)       /*   append to overlap list */
      prvp = prvp->nxtolap;
    prvp->nxtolap = ip;
  }
  else
    *ipp = ip;
  /* of overlapping notes, the one that was turned on first will be */
  /* turned off first as well */
  ip->nxtolap = NULL;

  nxtp = &(csound->actanchor);          /* now splice into activ lst */
  while ((prvp = nxtp) && (nxtp = prvp->nxtact) != NULL) {
    if (nxtp->insno > insno) {
      nxtp->prvact = ip;
      break;
    }
  }
  ip->nxtact       = nxtp;
  ip->prvact       = prvp;
  prvp->nxtact     = ip;
  ip->actflg++;                         /* and mark the instr active */
  ip->m_chnbp      = chn;               /* rec address of chnl ctrl blk */
  ip->m_pitch      = (unsigned char) mep->dat1;    /* rec MIDI data   */
  ip->m_veloc      = (unsigned char) mep->dat2;
  ip->xtratim      = 0;
  ip->m_sust       = 0;
  ip->relesing     = 0;
  ip->offbet       = -1.0;
  ip->offtim       = -1.0;              /* set indef duration */
  ip->opcod_iobufs = NULL;              /* IV - Sep 8 2002:            */
  ip->p1.value     = (MYFLT) insno;     /* set these required p-fields */
  ip->p2.value     = (MYFLT) (csound->icurTimeSamples/csound->esr - csound->timeOffs);
  ip->p3.value     = FL(-1.0);
  ip->esr          = csound->esr;
  ip->pidsr        = csound->pidsr;
  ip->sicvt        = csound->sicvt;
  ip->onedsr       = csound->onedsr;
  ip->ksmps        = csound->ksmps;
  ip->ekr          = csound->ekr;
  ip->kcounter     = csound->kcounter;
  ip->onedksmps    = csound->onedksmps;
  ip->onedkr       = csound->onedkr;
  ip->kicvt        = csound->kicvt;
  ip->pds          = NULL;
  pfields          = (CS_VAR_MEM*)&ip->p0;

  if (tp->psetdata != NULL) {
    int32_t i;
    MYFLT *pdat = tp->psetdata + 2;
    int32 nn = tp->pmax - 2;             /*   put cur vals in pflds */

    for (i = 0; i < nn; i++) {
      CS_VAR_MEM* pfield = (pfields + i + 3);
      pfield->value = *(pdat + i);
    }
    pmax = tp->pmax;
  }


  /* MIDI channel message note on routing overrides pset: */
  if (O->midiKey) {
    int32_t pfield_index = O->midiKey;
    CS_VAR_MEM* pfield = (pfields + pfield_index);
    MYFLT value = (MYFLT) ip->m_pitch;
    pfield->value = value;

    if (UNLIKELY(O->msglevel & CS_WARNMSG)) {
      csound->Message(csound, "  midiKey:         pfield: %3d  value: %3d\n",
                      pfield_index, (int32_t) pfield->value);
    }
    if (pmax < pfield_index) pmax = pfield_index;
  }
  else if (O->midiKeyCps) {
    int32_t pfield_index = O->midiKeyCps;
    CS_VAR_MEM* pfield = (pfields + pfield_index);
    MYFLT value = (MYFLT) ip->m_pitch;
    value = value / FL(12.0) + FL(3.0);
    value = value * OCTRES;
    value = (MYFLT) CPSOCTL((int32) value);
    pfield->value = value;

    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
      csound->Message(csound, "  midiKeyCps:      pfield: %3d  value: %3d\n",
                      pfield_index, (int32_t) pfield->value);
    }
    if (pmax < pfield_index) pmax = pfield_index;
  }
  else if (O->midiKeyOct) {
    int32_t pfield_index = O->midiKeyOct;
    CS_VAR_MEM* pfield = (pfields + pfield_index);
    MYFLT value = (MYFLT) ip->m_pitch;
    value = value / FL(12.0) + FL(3.0);
    pfield->value = value;
    if (UNLIKELY(O->msglevel & CS_WARNMSG)) {
      csound->Message(csound, "  midiKeyOct:      pfield: %3d  value: %3d\n",
                      pfield_index, (int32_t) pfield->value);
    }
    if (pmax < pfield_index) pmax = pfield_index;
  }
  else if (O->midiKeyPch) {
    int32_t pfield_index = O->midiKeyPch;
    CS_VAR_MEM* pfield = (pfields + pfield_index);
    MYFLT value = (MYFLT) ip->m_pitch;
    double octave = 0;
    double fraction = 0.0;
    value = value / FL(12.0) + FL(3.0);
    fraction = modf(value, &octave);
    fraction *= 0.12;
    value = octave + fraction;
    pfield->value = value;
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
      csound->Message(csound, "  midiKeyPch:      pfield: %3d  value: %3d\n",
                      pfield_index, (int32_t) pfield->value);
    }
    if (pmax < pfield_index) pmax = pfield_index;
  }
  if (O->midiVelocity) {
    int32_t pfield_index = O->midiVelocity;
    CS_VAR_MEM* pfield = (pfields + pfield_index);
    MYFLT value = (MYFLT) ip->m_veloc;
    pfield->value = value;
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
      csound->Message(csound, "  midiVelocity:    pfield: %3d  value: %3d\n",
                      pfield_index, (int32_t) pfield->value);
    }
    if (pmax < pfield_index) pmax = pfield_index;
  }
  else if (O->midiVelocityAmp) {
    int32_t pfield_index = O->midiVelocityAmp;
    CS_VAR_MEM* pfield = (pfields + pfield_index);
    MYFLT value = (MYFLT) ip->m_veloc;
    value = value * value / FL(16239.0);
    value = value * csound->e0dbfs;
    pfield->value = value;
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
      csound->Message(csound, "  midiVelocityAmp: pfield: %3d  value: %.3f\n",
                      pfield_index, pfield->value);
    }
    if (pmax < pfield_index) pmax = pfield_index;
  }

  EVTBLK evt = {0};
  if (pmax > 0) {
    int32_t i;
    csound->init_event = &evt;
    csound->init_event->pcnt = pmax;
    csound->init_event->p = (MYFLT *) csound->Calloc(csound, sizeof(MYFLT)*(pmax+1));
    for (i =1; i < csound->init_event->pcnt+1; i++) {
      csound->init_event->p[i] = pfields[i].value;
   }
  } else csound->init_event = NULL;

  error = csound->oparms->realtime ? realtime_init_pass(csound, ip) :
    init_pass(csound, ip);
  if(evt.p) csound->Free(csound, evt.p);
  csound->init_event = NULL;
  initResult = instance_init_finish(csound, ip);
  if (initResult == INSTANCE_INIT_COMPLETE && error == 0) {
    ATOMIC_SET(ip->init_done, 1);
  }
  else {
    ATOMIC_SET(ip->init_done, 0);
  }
  if (initResult == INSTANCE_INIT_TURNOFF) {
    return error != 0 ? error : csound->inerrcnt;
  }

  if (UNLIKELY(csound->inerrcnt)) {
    if (csound->oparms->realtime)
      instance_init_request_turnoff(csound, ip);
    else
      xturnoff_now(csound, ip);
    return csound->inerrcnt;
  }
  ip->tieflag = ip->reinitflag = 0;
  csound->tieflag = csound->reinitflag = 0;

  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
    char *name = csound->engineState.instrtxtp[insno]->insname;
    if (UNLIKELY(name))
      csound->Message(csound, Str("instr %s now active:\n"), name);
    else
      csound->Message(csound, Str("instr %d now active:\n"), insno);
    show_allocs(csound);
  }
  return 0;
}

static void show_allocs(CSOUND *csound)      /* debugging aid */
{
  INSTRTXT *txtp;
  INSDS   *p;

  csound->Message(csound, "insno\tinstanc\tnxtinst\tprvinst\tnxtact\t"
                  "prvact\tnxtoff\tactflg\tofftim\n");
  for (txtp = &(csound->engineState.instxtanchor);
       txtp != NULL;
       txtp = txtp->nxtinstxt)

    if ((p = txtp->instance) != NULL) {
      /*
       * On Alpha, we print pointers as pointers.  heh 981101
       * and now on all platforms (JPff)
       */
      do {
        csound->Message(csound, "%d\t%p\t%p\t%p\t%p\t%p\t%p\t%d\t%3.1f\n",
                        (int32_t) p->insno, (void*) p,
                        (void*) p->nxtinstance, (void*) p->prvinstance,
                        (void*) p->nxtact, (void*) p->prvact,
                        (void*) p->nxtoff, p->actflg, p->offtim);
      } while ((p = p->nxtinstance) != NULL);
    }
}

static void sched_off_time(CSOUND *csound, INSDS *ip)
{                               /* put an active instr into offtime list  */
  INSDS *prvp, *nxtp;         /* called by insert() & midioff + xtratim */

  if ((nxtp = csound->frstoff) == NULL ||
      nxtp->offtim > ip->offtim) {            /*   set into       */
    csound->frstoff = ip;                     /*   firstoff chain */
    ip->nxtoff = nxtp;
    /* IV - Feb 24 2006: check if this note already needs to be turned off */
    /* the following comparisons must match those in sense_events() */
#ifdef BETA
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
      csound->Message(csound,"sched_off_time: %lf %lf %f\n",
                      ip->offtim, csound->icurTimeSamples/csound->esr,
                      csound->curTime_inc);

#endif
    if (csound->oparms_.Beatmode) {
      double  tval = csound->curBeat + (0.505 * csound->curBeat_inc);
      if (ip->offbet <= tval) beat_expire(csound, tval);
    }
    else {
      double  tval = (csound->icurTimeSamples + (0.505 * csound->ksmps))/csound->esr;
      if (ip->offtim <= tval) time_expire(csound, tval);
    }
#ifdef BETA
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
      csound->Message(csound,"sched_off_time: %lf %lf %lf\n", ip->offtim,
                      (csound->icurTimeSamples + (0.505 * csound->ksmps))/csound->esr,
                      csound->ekr*((csound->icurTimeSamples +
                                    (0.505 * csound->ksmps))/csound->esr));
#endif
  }
  else {
    while ((prvp = nxtp)
           && (nxtp = nxtp->nxtoff) != NULL
           && ip->offtim >= nxtp->offtim);
    prvp->nxtoff = ip;
    ip->nxtoff = nxtp;
  }
}

void deinit_pass(CSOUND *csound, INSDS *ip) {
  OPDS *dds = (OPDS *) ip;
  const char* op;
  while ((dds = dds->nxtd) != NULL) {
    int32_t error;

    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
      op = dds->optext->t.oentry->opname;
      csound->Message(csound, "deinit %s:\n", op);
    }
    error = (*dds->deinit)(csound, dds);
    if(error) {
      op = dds->optext->t.oentry->opname;
      csound->ErrorMsg(csound, "%s deinit error\n", op);
    }
  }
}

static int32_t instance_turnoff_claim(CSOUND *csound, INSDS *ip)
{
  int32_t claimed = 0;
  int32_t state;

  async_instance_lock(csound);
  state = ATOMIC_GET(ip->turnoff_pending);
  if (ATOMIC_GET(ip->init_running) > 0) {
    if (state == INSTANCE_TURNOFF_NONE)
      ATOMIC_SET(ip->turnoff_pending, INSTANCE_TURNOFF_REQUESTED);
  }
  else if (state == INSTANCE_TURNOFF_NONE) {
    /* FINALIZING quarantines the instance until this thread has completed
       every active-chain and free-list update. */
    ATOMIC_SET(ip->turnoff_pending, INSTANCE_TURNOFF_FINALIZING);
    claimed = 1;
  }
  async_instance_unlock(csound);
  return claimed;
}

static void instance_turnoff_complete(CSOUND *csound, INSDS *ip,
                                      int32_t deactivated)
{
  async_instance_lock(csound);
  if (ATOMIC_GET(ip->turnoff_pending) == INSTANCE_TURNOFF_FINALIZING) {
    if (ATOMIC_GET(ip->free_pending) && !ip->linked) {
      ATOMIC_SET(ip->turnoff_pending, deactivated ?
                 INSTANCE_TURNOFF_RECLAIM : INSTANCE_TURNOFF_FINALIZING);
      enqueue_init_turnoff_locked(csound, ip);
    }
    else
      ATOMIC_SET(ip->turnoff_pending, INSTANCE_TURNOFF_NONE);
  }
  async_instance_unlock(csound);
}

#define DEACT_LOCAL_STACK_SIZE 64

typedef enum {
  DEACT_ENTER,
  DEACT_AFTER_UDO,
  DEACT_AFTER_SUBINSTR
} DEACT_STAGE;

typedef struct {
  INSDS *ip;
  DEACT_STAGE stage;
  int32_t completeTurnoff;
} DEACT_FRAME;

static DEACT_FRAME *deact_grow_stack(CSOUND *csound, DEACT_FRAME *stack,
                                     DEACT_FRAME *localStack,
                                     size_t frameCount, size_t *capacity)
{
  DEACT_FRAME *grown;
  size_t newCapacity;

  if (UNLIKELY(*capacity > SIZE_MAX / 2 ||
               *capacity * 2 > SIZE_MAX / sizeof(DEACT_FRAME))) {
    csound->Die(csound, "%s", Str("deact: traversal depth overflow"));
    return stack;
  }

  newCapacity = *capacity * 2;
  if (stack == localStack) {
    grown = (DEACT_FRAME *) csound->Malloc(
      csound, newCapacity * sizeof(DEACT_FRAME));
    if (LIKELY(grown != NULL))
      memcpy(grown, localStack, frameCount * sizeof(DEACT_FRAME));
  }
  else {
    grown = (DEACT_FRAME *) csound->ReAlloc(
      csound, stack, newCapacity * sizeof(DEACT_FRAME));
  }

  if (UNLIKELY(grown == NULL)) {
    csound->Die(csound, "%s", Str("deact: could not grow traversal stack"));
    return stack;
  }
  *capacity = newCapacity;
  return grown;
}

/* unlink single instr from activ chain */
/*      and mark it inactive            */
static void deact_internal(CSOUND *csound, INSDS *ip)
{
  DEACT_FRAME localStack[DEACT_LOCAL_STACK_SIZE];
  DEACT_FRAME *stack = localStack;
  size_t capacity = DEACT_LOCAL_STACK_SIZE;
  size_t frameCount = 1;

  localStack[0].ip = ip;
  localStack[0].stage = DEACT_ENTER;
  localStack[0].completeTurnoff = 0;

  /* Store the recursive continuations explicitly while preserving their
     original cleanup order. Ordinary chains remain in localStack. */
  while (frameCount > 0) {
    DEACT_FRAME *frame = &stack[frameCount - 1];
    INSDS *current = frame->ip;

    switch (frame->stage) {
    case DEACT_ENTER: {
      UOPCODE *udo;

      deinit_pass(csound, current);
      csound->engineState.instrtxtp[current->insno]->active--;
      if (current->xtratim > 0)
        csound->engineState.instrtxtp[current->insno]->pending_release--;
      csound->cpu_power_busy -=
        csound->engineState.instrtxtp[current->insno]->cpuload;

      frame->stage = DEACT_AFTER_UDO;
      if (current->opcod_deact == NULL)
        continue;

      udo = (UOPCODE *) current->opcod_deact;
      for (int32_t k = 0; k < OPCODENUMOUTS_MAX; k++) {
        if (udo->cvt_in[k] != NULL) {
          src_deinit(csound, udo->cvt_in[k]);
          udo->cvt_in[k] = NULL;
        }
      }
      for (int32_t k = 0; k < OPCODENUMOUTS_MAX; k++) {
        if (udo->cvt_out[k] != NULL) {
          src_deinit(csound, udo->cvt_out[k]);
          udo->cvt_out[k] = NULL;
        }
      }

      if (udo->ip == NULL || ATOMIC_GET8(udo->ip->actflg) == 0 ||
          !instance_turnoff_claim(csound, udo->ip))
        continue;
      if (UNLIKELY(frameCount == capacity))
        stack = deact_grow_stack(csound, stack, localStack, frameCount,
                                 &capacity);
      stack[frameCount].ip = udo->ip;
      stack[frameCount].stage = DEACT_ENTER;
      stack[frameCount].completeTurnoff = 1;
      frameCount++;
      break;
    }
    case DEACT_AFTER_UDO: {
      SUBINST *subinstr;

      if (current->opcod_deact != NULL) {
        UOPCODE *udo = (UOPCODE *) current->opcod_deact;
        udo->ip = NULL;
        udo->h.perf = (SUBR) useropcd;
        current->opcod_deact = NULL;
      }

      frame->stage = DEACT_AFTER_SUBINSTR;
      if (current->subins_deact == NULL)
        continue;

      subinstr = (SUBINST *) current->subins_deact;
      if (subinstr->ip == NULL || ATOMIC_GET8(subinstr->ip->actflg) == 0 ||
          !instance_turnoff_claim(csound, subinstr->ip))
        continue;
      if (UNLIKELY(frameCount == capacity))
        stack = deact_grow_stack(csound, stack, localStack, frameCount,
                                 &capacity);
      stack[frameCount].ip = subinstr->ip;
      stack[frameCount].stage = DEACT_ENTER;
      stack[frameCount].completeTurnoff = 1;
      frameCount++;
      break;
    }

    case DEACT_AFTER_SUBINSTR: {
      int32_t closeFiles = 0;
      INSDS *nxtp;

      if (current->subins_deact != NULL) {
        SUBINST *subinstr = (SUBINST *) current->subins_deact;
        subinstr->ip = NULL;
        current->subins_deact = NULL;
      }

      if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
        char *name =
          csound->engineState.instrtxtp[current->insno]->insname;
        if (UNLIKELY(name))
          csound->Message(csound, Str("removed instance of instr %s\n"),
                          name);
        else
          csound->Message(csound, Str("removed instance of instr %d\n"),
                          current->insno);
      }

      if (ATOMIC_GET8(current->actflg) != 0) {
        if (current->prvact &&
            (nxtp = current->prvact->nxtact = current->nxtact) != NULL) {
          nxtp->prvact = current->prvact;
        }
        current->prvact = NULL;
        current->nxtact = NULL;
        /* Prevent a loop in kperf() if an inactive instance is passed in. */
        async_instance_lock(csound);
        ATOMIC_SET(current->init_done, 0);
        ATOMIC_SET8(current->actflg, 0);
        closeFiles = current->fdchp != NULL &&
          !instance_has_async_refs(current);
        async_instance_unlock(csound);
        /* Finish synchronous file cleanup before publishing this instance to
           the free list. The realtime init thread may reuse it at once. */
        if (closeFiles)
          fdchclose(csound, current);
        if (current->linked &&
            csound->engineState.instrtxtp[current->insno] ==
              current->instr) {
          inactive_instance_lock(csound);
          current->nxtact =
            csound->engineState.instrtxtp[current->insno]->act_instance;
          csound->engineState.instrtxtp[current->insno]->act_instance =
            current;
          inactive_instance_unlock(csound);
        }
      }

      /* An already inactive, unlinked instance is not published above. */
      if (!current->linked && current->fdchp != NULL &&
          !instance_has_async_refs(current))
        fdchclose(csound, current);
      csound->dag_changed++;
      if (frame->completeTurnoff)
        instance_turnoff_complete(csound, current, 1);
      frameCount--;
      break;
    }
    }
  }

  if (stack != localStack)
    csound->Free(csound, stack);
}

static void deact(CSOUND *csound, INSDS *ip)
{
  if (ATOMIC_GET8(ip->actflg) == 0)
    return;
  if (!instance_turnoff_claim(csound, ip))
    return;
  deact_internal(csound, ip);
  instance_turnoff_complete(csound, ip, 1);
}

/* Turn off a particular insalloc, also remove from list of active */
/* MIDI notes. Allows for releasing if ip->xtratim > 0. */
static void xturnoff_internal(CSOUND *csound, INSDS *ip, int32_t force)
{
  MCHNBLK *chn;

  if (!force && ATOMIC_GET8(ip->actflg) == 0)
    return;
  if (!force && !instance_turnoff_claim(csound, ip))
    return;
  if (UNLIKELY(ip->relesing)) {
    if (!force)
      instance_turnoff_complete(csound, ip, 0);
    return;                             /* already releasing: nothing to do */
  }

  chn = ip->m_chnbp;
  if (chn != NULL) {                    /* if this was a MIDI note */
    INSDS *prvip;
    prvip = chn->kinsptr[ip->m_pitch];  /*    remov from activ lst */
    if (ip->m_sust && chn->ksuscnt)
      chn->ksuscnt--;
    ip->m_sust = 0;                     /* force turnoff even if sustaining */
    if (prvip != NULL) {
      if (prvip == ip)
        chn->kinsptr[ip->m_pitch] = ip->nxtolap;
      else {
        while (prvip != NULL && prvip->nxtolap != ip)
          prvip = prvip->nxtolap;
        if (prvip != NULL)
          prvip->nxtolap = ip->nxtolap;
      }
    }
  }
  /* remove from schedoff chain first if finite duration */
  if (csound->frstoff != NULL && ip->offtim >= 0.0) {
    INSDS *prvip;
    prvip = csound->frstoff;
    if (prvip == ip)
      csound->frstoff = ip->nxtoff;
    else {
      while (prvip != NULL && prvip->nxtoff != ip)
        prvip = prvip->nxtoff;
      if (prvip != NULL)
        prvip->nxtoff = ip->nxtoff;
    }
  }
  /* if extra time needed: schedoff at new time */
  if (ip->xtratim > 0) {
    set_xtratim(csound, ip);
    /* sched_off_time can expire this note synchronously. Release ownership so
       the nested deactivation can claim and finish the turnoff. */
    if (!force)
      instance_turnoff_complete(csound, ip, 0);
#ifdef BETA
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
      csound->Message(csound, "Calling sched_off_time line %d\n", __LINE__);
#endif
    sched_off_time(csound, ip);
  }
  else {
    /* no extra time needed: deactivate immediately */
    deact_internal(csound, ip);
    csound->dag_changed++;      /* Need to remake DAG */
    if (!force)
      instance_turnoff_complete(csound, ip, 1);
  }
}

void xturnoff(CSOUND *csound, INSDS *ip)  /* turnoff a particular insalloc  */
{                                         /* called by inexclus on ctrl 111 */
  xturnoff_internal(csound, ip, 0);
}

/* Turn off instrument instance immediately, without releasing. */
/* Removes alloc from list of active MIDI notes. */
static void xturnoff_now_internal(CSOUND *csound, INSDS *ip, int32_t force)
{
  if (!force && ATOMIC_GET8(ip->actflg) == 0)
    return;
  if (!force && !instance_turnoff_claim(csound, ip))
    return;
  if (ip->xtratim > 0 && ip->relesing)
    csound->engineState.instrtxtp[ip->insno]->pending_release--;
  ip->xtratim = 0;
  ip->relesing = 0;
  xturnoff_internal(csound, ip, 1);
  if (!force)
    instance_turnoff_complete(csound, ip, 1);
}

void xturnoff_now(CSOUND *csound, INSDS *ip)
{
  xturnoff_now_internal(csound, ip, 0);
}

void xturnoff_instance(CSOUND *csound, MYFLT instr, int32_t insno, INSDS *ip,
                  int32_t mode, int32_t allow_release) {
  INSDS *ip2 = NULL, *nip;
  do {
    /* This loop does not terminate in mode=0 */
    nip = ip->nxtact;
    if (((mode & 8) && ip->offtim >= 0.0) ||
        ((mode & 4) && ip->p1.value != instr) ||
        (allow_release && ip->relesing)) {
      ip = nip;
      continue;
    }
    if (!(mode & 3)) {
      if (allow_release) {
        xturnoff(csound, ip);
      }
      else {
        nip = ip->nxtact;
        xturnoff_now(csound, ip);
      }
    }
    else {
      ip2 = ip;
      if ((mode & 3) == 1)
        break;
    }
    ip = nip;
  } while (ip != NULL && (int32_t) ip->insno == insno);

  if (ip2 != NULL) {
    if (allow_release) {
      xturnoff(csound, ip2);
    }
    else {
      xturnoff_now(csound, ip2);
    }
  }
}

void free_instr_var_memory(CSOUND* csound, INSDS* ip) {
  INSTRTXT* instrDef = ip->instr;
  CS_VAR_POOL* pool = instrDef->varPool;
  CS_VARIABLE* current = pool->head;

  if (ip->lclbas == NULL) {
    // This seems to be the case when freeing instr 0...
    return;
  }

  while (current != NULL) {
    const CS_TYPE* varType = current->varType;
    if (varType->freeVariableMemory != NULL) {
      varType->freeVariableMemory(csound,
                                  ip->lclbas + current->memBlockIndex);
    }
    current = current->next;
  }
}

/* free all inactive instr spaces */
void free_inactive_instances(CSOUND *csound)
{
  INSTRTXT  *txtp;
  INSDS     *ip, *nxtip, *prvip, **prvnxtloc, *pending;
  INSDS     *reclaim = NULL, *reclaim_tail = NULL;
  int32_t       cnt = 0;

  /* Detach reclaimable instances while holding the instance-list lock, then
     release files and memory after that lock is released. A caller may still
     serialize the surrounding section transition with alloc_spinlock. */
  inactive_instance_lock(csound);
  for (txtp = &(csound->engineState.instxtanchor);
       txtp != NULL;  txtp = txtp->nxtinstxt) {
    pending = NULL;
    if ((ip = txtp->instance) != NULL) {        /* if instance exists */

      prvip = NULL;
      prvnxtloc = &txtp->instance;
      do {
        if (instance_is_reclaimable(ip)) {
          cnt++;
          if ((nxtip = ip->nxtinstance) != NULL)
            nxtip->prvinstance = prvip;
          *prvnxtloc = nxtip;
          /* Preserve the original INSTRTXT traversal order. An owning
             instrument's AUXCH chain can contain descriptors embedded in a
             nested UDO instance, so reversing this list would free the
             descriptor storage before auxchfree() follows the chain. */
          ip->nxtinstance = NULL;
          if (reclaim_tail != NULL)
            reclaim_tail->nxtinstance = ip;
          else
            reclaim = ip;
          reclaim_tail = ip;
        }
        else {
          if (!ip->actflg && ip->linked) {
            ip->nxtact = pending;
            pending = ip;
          }
          prvip = ip;
          prvnxtloc = &ip->nxtinstance;
        }
      }
      while ((ip = *prvnxtloc) != NULL);
    }

    /* IV - Oct 31 2002 */
    if (!txtp->instance)
      txtp->lst_instance = NULL;              /* find last alloc */
    else {
      ip = txtp->instance;
      while (ip->nxtinstance) ip = ip->nxtinstance;
      txtp->lst_instance = ip;
    }

    txtp->act_instance = pending;
  }
  inactive_instance_unlock(csound);

  while (reclaim != NULL) {
    ip = reclaim;
    reclaim = reclaim->nxtinstance;
    if (ip->opcod_iobufs && ip->insno > csound->engineState.maxinsno)
      csound->Free(csound, ip->opcod_iobufs);
    if (ip->fdchp != NULL)
      fdchclose(csound, ip);
    if (ip->auxchp != NULL)
      auxchfree(csound, ip);
    free_instr_var_memory(csound, ip);
    csound->Free(csound, (char *) ip);
  }
  /* check current items in deadpool to see if they need deleting */
  {
    int32_t i;
    for (i=0; i < csound->dead_instr_no; i++) {
      if (csound->dead_instr_pool[i] != NULL) {
        INSDS *active = csound->dead_instr_pool[i]->instance;
        while (active != NULL) {
          if (!instance_is_reclaimable(active)) {
            // add_to_deadpool(csound,csound->dead_instr_pool[i]);
            break;
          }
          active = active->nxtinstance;
        }
        /* no active instances */
        if (active == NULL) {
          free_instrtxt(csound, csound->dead_instr_pool[i]);
          csound->dead_instr_pool[i] = NULL;
        }
      }
    }
  }
  if (UNLIKELY(cnt)) {
    if(csound->oparms->msglevel ||csoundGetDebug(csound) & DEBUG_RUNTIME)
      csound->Message(csound, Str("inactive allocs returned to freespace\n"));
  }
}

int32_t csoundInitError(CSOUND *csound, const char *s, ...)
{
  va_list args;
  INSDS   *ip;
  char    buf[512];

  /* RWD: need this! */
  if (UNLIKELY(csound->ids == NULL)) {
    va_start(args, s);
    csoundErrMsgV(csound, Str("\nINIT ERROR: "), s, args);
    va_end(args);
    csound->LongJmp(csound, 1);
  }
  if (csound->mode != 1)
    csoundErrorMsg(csound, Str("InitError in wrong mode %d\n"), csound->mode);
  /* IV - Oct 16 2002: check for subinstr and user opcode */
  ip = csound->ids->insdshead;
  if (ip->opcod_iobufs) {
    OPCODINFO *op = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->opcode_info;
    /* find top level instrument instance */
    do {
      ip = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->parent_ip;
    } while (ip->opcod_iobufs);
    if (op)
      snprintf(buf, 512, Str("\nINIT ERROR in instr %d (opcode %s) line %d: "),
               ip->insno, op->name, csound->ids->optext->t.linenum);
    else
      snprintf(buf, 512, Str("\nINIT ERROR in instr %d (subinstr %d) line %d: "),
               ip->insno, csound->ids->insdshead->insno,
               csound->ids->optext->t.linenum);
  }
  else
    snprintf(buf, 512, Str("\nINIT ERROR in instr %d (opcode %s) line %d: "),
             ip->insno, csound->op, csound->ids->optext->t.linenum);
  va_start(args, s);
  csoundErrMsgV(csound, buf, s, args);
  va_end(args);
  do_baktrace(csound, csound->ids->optext->t.locn);
  print_opcall(csound, &(csound->ids->optext->t));
  return ++(csound->inerrcnt);
}

int32_t csoundPerfError(CSOUND *csound, OPDS *h, const char *s, ...)
{
  va_list args;
  char    buf[512];
  INSDS *ip = h->insdshead;
  TEXT t = h->optext->t;
  if (csound->mode != 2)
    csoundErrorMsg(csound, Str("PerfError in wrong mode %d\n"), csound->mode);
  if (ip->opcod_iobufs) {
    OPCODINFO *op = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->opcode_info;

    /* find top level instrument instance */
    do {
      ip = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->parent_ip;
    } while (ip->opcod_iobufs);
    if (op) {
      snprintf(buf, 512, Str("PERF ERROR in instr %d (opcode %s) line %d: "),
               ip->insno, op->name, t.linenum);
    }
    else
      snprintf(buf, 512, Str("PERF ERROR in instr %d (subinstr %d) line %d: "),
               ip->insno, ip->insno, t.linenum);
  }
  else{
    char *name = strip_extension(csound, csound->op);
    snprintf(buf, 512, Str("PERF ERROR in instr %d (opcode %s) line %d: "),
             ip->insno, name, t.linenum);
   csound->Free(csound, name);
  }
  va_start(args, s);
  csoundErrMsgV(csound, buf, s, args);
  va_end(args);
  if (ip->pds) {
    print_opcall(csound, &(ip->pds->optext->t));
  }
  csoundErrorMsg(csound, "%s",  Str("...event aborted\n"));
  csound->perferrcnt++;
  xturnoff_now((CSOUND*) csound, ip);       /* rm ins fr actlist */
  return csound->perferrcnt;                /* contin from there */
}

/* unlink expired notes from activ chain */
/*      and mark them inactive           */
/*    close any files in each fdchain    */
void beat_expire(CSOUND *csound, double beat)
{
  INSDS  *ip;
 strt:
  if ((ip = csound->frstoff) != NULL && ip->offbet <= beat) {
    do {
      if (!ip->relesing && ip->xtratim) {
        /* IV - Nov 30 2002: */
        /*   allow extra time for finite length (p3 > 0) score notes */
        set_xtratim(csound, ip);      /* enter release stage */
        csound->frstoff = ip->nxtoff; /* update turnoff list */
#ifdef BETA
        if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
          csound->Message(csound, "Calling sched_off_time line %d\n", __LINE__);
#endif
        sched_off_time(csound, ip);
        goto strt;                    /* and start again */
      }
      else
        deact(csound, ip);    /* IV - Sep 5 2002: use deact() as it also */
    }                         /* deactivates subinstrument instances */
    while ((ip = ip->nxtoff) != NULL && ip->offbet <= beat);
    csound->frstoff = ip;
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
      csound->Message(csound, "deactivated all notes to beat %7.3f\n", beat);
      csound->Message(csound, "frstoff = %p\n", (void*) csound->frstoff);
    }
  }
}

/* unlink expired notes from activ chain */
/*      and mark them inactive           */
/*    close any files in each fdchain    */
void time_expire(CSOUND *csound, double time)
{
  INSDS  *ip;

 strt:
  if ((ip = csound->frstoff) != NULL && ip->offtim <= time) {
    do {
      if (!ip->relesing && ip->xtratim) {
        /* IV - Nov 30 2002: */
        /*   allow extra time for finite length (p3 > 0) score notes */
        set_xtratim(csound, ip);      /* enter release stage */
        csound->frstoff = ip->nxtoff; /* update turnoff list */
#ifdef BETA
        if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME))
          csound->Message(csound, "Calling sched_off_time line %d\n", __LINE__);
#endif
        sched_off_time(csound, ip);

        goto strt;                    /* and start again */
      }
      else {
        deact(csound, ip);    /* IV - Sep 5 2002: use deact() as it also */
      }
    }                         /* deactivates subinstrument instances */
    while ((ip = ip->nxtoff) != NULL && ip->offtim <= time);
    csound->frstoff = ip;
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_RUNTIME)) {
      csound->Message(csound, "deactivated all notes to time %7.3f\n", time);
      csound->Message(csound, "frstoff = %p\n", (void*) csound->frstoff);
    }
  }
}

static int32_t find_label_mem_offset(CSOUND* csound, INSTRTXT* ip, char* labelName) {
  IGN(csound);
  OPTXT* optxt = (OPTXT*) ip;
  int32_t offset = 0;

  while ((optxt = optxt->nxtop) != NULL) {
    TEXT* t = &optxt->t;
    if (strcmp(t->oentry->opname, "$label") == 0 &&
        strcmp(t->opcod, labelName) == 0) {
      break;
    }
    offset += t->oentry->dsblksiz;
  }

  return offset;
}

/* create instance of an instr template */
/* allocates and sets up all pntrs      */
/**
 * Set up argument pointers (argpp) for a single opcode.
 * This can be called during initial instantiation or to reinitialize
 * argument pointers for reused instrument instances.
 */
static void setup_opcode_argpp(
  CSOUND *csound, OPDS *opds, TEXT *ttp,
  const OENTRY *ep, INSDS *ip, INSTRTXT *tp,
  MYFLT *lclbas, CS_VAR_MEM *lcloffbas,
  char *opMemStart
) {
    MYFLT **argpp;
    ARG *arg;
    int n;
    int argStringCount;

    if (ep->useropinfo == NULL)
      argpp = (MYFLT **) ((char *) opds + sizeof(OPDS));
    else          /* user defined opcodes are a special case */
      argpp = &(((UOPCODE *) ((char *) opds))->ar[0]);

    /* Set up output arguments */
    arg = ttp->outArgs;
    for (n = 0; arg != NULL; n++) {
      MYFLT *fltp;
      CS_VARIABLE* var = (CS_VARIABLE*)arg->argPtr;
      if (arg->type == ARG_GLOBAL || arg->type == ARG_LOCAL) {
        if (arg->type == ARG_LOCAL) {
          if (UNLIKELY(var == NULL)) {
            csound->Die(csound,
                        Str("setup_opcode_argpp:"
                            " NULL local variable pointer for out-arg of %s"),
                        ep->opname ? ep->opname : "(null)");
          } else {
            fltp = lclbas + var->memBlockIndex;
          }
        } else fltp = &(var->memBlock->value);

        if (arg->structPath != NULL) {
          char* path = csoundStrdup(csound, arg->structPath);
          char *next, *th;

          next = cs_strtok_r(path, ".", &th);
          while (next != NULL) {
            CS_TYPE* type = csoundGetTypeForArg(fltp);
            CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)fltp;
            CONS_CELL* members = type->members;
            int32_t i = 0;
            int32_t found = 0;
            while(members != NULL) {
              CS_VARIABLE* member = (CS_VARIABLE*)members->value;
              if (!strcmp(member->varName, next)) {
                fltp = &(structVar->members[i]->value);
                found = 1;
                break;
              }
              i++;
              members = members->next;
            }
            if (!found) {
              csound->Die(csound,
                Str("setup_opcode_argpp: struct member '%s' not found in structPath '%s' for %s"),
                next, arg->structPath, ep->opname ? ep->opname : "(null)");
              csound->Free(csound, path);
            }
            next = cs_strtok_r(NULL, ".", &th);
          }
          csound->Free(csound, path);
        }
      }
      else if (arg->type == ARG_PFIELD) {
        CS_VAR_MEM* pfield = lcloffbas + arg->index;
        fltp = &(pfield->value);
      }
      else {
        csound->Die(csound,
          Str("setup_opcode_argpp: Unhandled argument type (%d) for out-arg of %s"),
          arg->type,
          ep->opname ? ep->opname : "(null)");
      }
      argpp[n] = fltp;
      arg = arg->next;
    }

    for (argStringCount = args_required(ep->outypes); n < argStringCount; n++) {
      argpp[n] = NULL;
    }

    /* Set up input arguments */
    arg = ttp->inArgs;
    ip->lclbas = lclbas;
    int providedIn = 0;
    for (; arg != NULL; n++, arg = arg->next, providedIn++) {
      if (arg->type == ARG_CONSTANT) {
        CS_VAR_MEM *varMem = (CS_VAR_MEM*)arg->argPtr;
        argpp[n] = &varMem->value;
      }
      else if (arg->type == ARG_STRING) {
        argpp[n] = (MYFLT*)(arg->argPtr);
      }
      else if (arg->type == ARG_PFIELD) {
        CS_VAR_MEM* pfield = lcloffbas + arg->index;
        argpp[n] = &(pfield->value);
      }
      else if (arg->type == ARG_LOCAL || arg->type == ARG_GLOBAL){
        CS_VARIABLE* var = (CS_VARIABLE*)(arg->argPtr);
        argpp[n] = arg->type == ARG_LOCAL ?
          lclbas + var->memBlockIndex :
          &(var->memBlock->value);

        if (arg->structPath != NULL) {
          char* path = csoundStrdup(csound, arg->structPath);
          char *next, *th;
          MYFLT* fltp = argpp[n];
          next = cs_strtok_r(path, ".", &th);
          while (next != NULL) {
            CS_TYPE* type = csoundGetTypeForArg(fltp);
            CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)fltp;
            if (type == NULL || structVar == NULL || structVar->members == NULL)
              break;
            CONS_CELL* members = type->members;
            int32_t i = 0;
            while(members != NULL) {
              CS_VARIABLE* member = (CS_VARIABLE*)members->value;
              if (!strcmp(member->varName, next)) {
                fltp = &(structVar->members[i]->value);
                break;
              }
              i++;
              members = members->next;
            }
            next = cs_strtok_r(NULL, ".", &th);
          }
          argpp[n] = fltp;
          csound->Free(csound, path);
        }
      }
      else if (arg->type == ARG_LABEL) {
        argpp[n] = (MYFLT*)(opMemStart +
                            find_label_mem_offset(csound, tp, (char*)arg->argPtr));
      }
      else {
        argpp[n] = (MYFLT*)(opMemStart +
                    ((TEXT*)arg->argPtr)->inArgCount * sizeof(MYFLT *));
      }
    }
}


/**
 * Reinitialize all argument pointers for opcodes in an instrument instance.
 * This should be called when reusing a UDO instance to ensure argpp pointers
 * are fresh and not stale from previous usage.
 */
void csoundReinitInstrumentArgpp(CSOUND *csound, INSDS *ip)
{
    INSTRTXT *tp = ip->instr;
    OPTXT *optxt = (OPTXT*)tp;
    OPDS *opds;
    char *nxtopds;
    MYFLT *lclbas = ip->lclbas;
    CS_VAR_MEM *lcloffbas = (CS_VAR_MEM*)&ip->p0;

    /* Calculate opcode memory start */
    char *opMemStart = (char*) lclbas + tp->varPool->poolSize +
        (tp->varPool->varCount * CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET));

    nxtopds = opMemStart;

    /* Iterate through all opcodes and reinit their argpp */
    while ((optxt = optxt->nxtop) != NULL) {
        TEXT *ttp = &optxt->t;
        OENTRY *ep = ttp->oentry;

        if (UNLIKELY(ep == NULL || ep->opname == NULL)) {
            continue;
        }

        opds = (OPDS*) nxtopds;
        nxtopds += ep->dsblksiz;

        if (UNLIKELY(strcmp(ep->opname, "endin") == 0 || strcmp(ep->opname, "endop") == 0))
            break;

        if (UNLIKELY(strcmp(ep->opname, "pset") == 0 || strcmp(ep->opname, "$label") == 0))
            continue;

        /* Reinit argpp for this opcode */
        setup_opcode_argpp(csound, opds, ttp, ep, ip, tp, lclbas, lcloffbas, opMemStart);
    }
}

static CS_VAR_MEM *instance_local_memory(CS_VARIABLE *var, MYFLT *lclbas)
{
  char *value = (char *)(lclbas + var->memBlockIndex);

  var->memBlock = (CS_VAR_MEM *)(value - CS_VAR_TYPE_OFFSET);
  return var->memBlock;
}

static void initialize_instance_reserved_variables(CSOUND *csound, INSDS *ip,
                                                    MYFLT *lclbas)
{
  static const char *const numericNames[] = { "ksmps", "kr", "sr" };
  MYFLT numericValues[] = { csound->ksmps, csound->ekr, csound->esr };
  CS_VAR_POOL *pool;
  CS_VARIABLE *var;
  size_t index;

  if (UNLIKELY(ip == NULL || ip->instr == NULL || lclbas == NULL)) {
    return;
  }
  pool = ip->instr->varPool;
  if (UNLIKELY(pool == NULL)) {
    return;
  }

  for (index = 0; index < sizeof(numericNames) / sizeof(numericNames[0]);
       index++) {
    var = csoundFindVariableWithName(csound, pool, numericNames[index]);
    if (var != NULL) {
      instance_local_memory(var, lclbas)->value = numericValues[index];
    }
  }

  var = csoundFindVariableWithName(csound, pool, "this_instr");
  if (var != NULL) {
    INSTREF source = { ip->instr, 0 };
    INSTREF *destination =
      (INSTREF *)&instance_local_memory(var, lclbas)->value;

    var->varType->copyValue(csound, var->varType, destination, &source, NULL);
    destination->readonly = 1;
  }

  var = csoundFindVariableWithName(csound, pool, "this");
  if (var != NULL) {
    INSTANCEREF source = { ip, 0 };
    INSTANCEREF *destination =
      (INSTANCEREF *)&instance_local_memory(var, lclbas)->value;

    var->varType->copyValue(csound, var->varType, destination, &source, NULL);
    destination->readonly = 1;
  }
}

void recycle_udo_instance(CSOUND *csound, INSDS *ip)
{
  /* Reset local values and their cached argument pointers before deact()
     publishes the frame on the reusable-instance list. Callers must first
     exclude deinit callbacks and resources that still depend on local data. */
  free_instr_var_memory(csound, ip);
  if (ip->lclbas != NULL) {
    csoundInitializeVarPool(csound, ip->lclbas, ip->instr->varPool);
    initialize_instance_reserved_variables(csound, ip, ip->lclbas);
    csoundReinitInstrumentArgpp(csound, ip);
  }
  deact(csound, ip);
}

static INSDS *instantiate(CSOUND *csound, int32_t insno, int32_t link)
{
  INSTRTXT  *tp;
  INSDS     *ip;
  OPTXT     *optxt;
  OPDS      *opds, *prvids, *prvpds, *prvpdd;
  const OENTRY  *ep;
  int32_t       i, n, pextent, pextra, pextrab;
  char      *nxtopds, *opdslim;
  MYFLT     *lclbas;
  CS_VAR_MEM *lcloffbas; // start of pfields
  char*     opMemStart;

  OPARMS    *O = csound->oparms;
  int32_t   odebug = csoundGetDebug(csound) & DEBUG_RUNTIME;
  CS_VARIABLE* current;

  tp = csound->engineState.instrtxtp[insno];
  n = 3;
  if (O->midiKey>n) n = O->midiKey;
  if (O->midiKeyCps>n) n = O->midiKeyCps;
  if (O->midiKeyOct>n) n = O->midiKeyOct;
  if (O->midiKeyPch>n) n = O->midiKeyPch;
  if (O->midiVelocity>n) n = O->midiVelocity;
  if (O->midiVelocityAmp>n) n = O->midiVelocityAmp;
  pextra = n-3;
  pextrab = ((i = tp->pmax - 3L) > 0 ? (int32_t) (i * sizeof(CS_VAR_MEM)) : 0);
  /* alloc new space,  */
  pextent = sizeof(INSDS) + pextrab + pextra*sizeof(CS_VAR_MEM);

  // Check for null or corrupted varPool to prevent segfault
  size_t varPoolSize = 0;
  size_t varPoolCount = 0;
  if (tp->varPool != NULL && (uintptr_t)tp->varPool >= 0x1000) {
    varPoolSize = tp->varPool->poolSize;
    varPoolCount = tp->varPool->varCount;
  } else {
    // Treat this as a fatal initialization error
    csound->InitError(csound,
                      "Fatal initialization error in instantiate: tp->varPool is null or corrupted (tp=%p). "
                      "This indicates a serious problem with instrument initialization.",
                      (void*)tp);
    return NULL;  // Abort the instantiation path
  }

  ip =
    (INSDS*) csound->Calloc(csound,
                            (size_t) pextent + varPoolSize +
                            (varPoolCount *
                             CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET)) +
                            (varPoolCount * sizeof(CS_VARIABLE*)) +
                            tp->opdstot);
  if (UNLIKELY(ip == NULL))
    return NULL;
  ip->csound = csound;
  ip->m_chnbp = (MCHNBLK*) NULL;
  ip->instr = tp;
  if (link)
    ip->insno = insno;

  if (insno > csound->engineState.maxinsno) {
    OPCODINFO* info = tp->opcode_info;
    size_t pcnt = sizeof(OPCOD_IOBUFS) +
      sizeof(MYFLT*) * (info->inchns + info->outchns);
    ip->opcod_iobufs = (void*) csound->Malloc(csound, pcnt);
  }

  /* gbloffbas = csound->globalVarPool; */
  lcloffbas = (CS_VAR_MEM*)&ip->p0;
  lclbas = (MYFLT*) ((char*) ip + pextent);   /* split local space */
  csoundInitializeVarPool((void *)csound, lclbas, tp->varPool);

  opMemStart = nxtopds = (char*) lclbas + tp->varPool->poolSize +
    (tp->varPool->varCount * CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET));
  opdslim = nxtopds + tp->opdstot;
  if (UNLIKELY(odebug))
    csound->Message(csound,
                    Str("instr %d allocated at %p\n\tlclbas %p, opds %p\n"),
                    insno, ip, lclbas, nxtopds);
  optxt = (OPTXT*) tp;
  prvids = prvpds = prvpdd = (OPDS*) ip;
  //    prvids->insdshead = ip;

  /* initialize vars for CS_TYPE */
  for (current = tp->varPool->head; current != NULL; current = current->next) {
    char* ptr = (char*)(lclbas + current->memBlockIndex);
    const CS_TYPE** typePtr = (const CS_TYPE**)(ptr - CS_VAR_TYPE_OFFSET);
    *typePtr = current->varType;
  }

  while ((optxt = optxt->nxtop) != NULL) {    /* for each op in instr */
    TEXT *ttp = &optxt->t;
    ep = ttp->oentry;
    /* Robustness: skip unresolved nodes (no opcode entry) */
    if (UNLIKELY(ep == NULL || ep->opname == NULL)) {
      if (UNLIKELY(odebug))
        csound->Message(csound, "instantiate: skipping node with NULL oentry (line=%d)\n", ttp->linenum);
      continue;
    }
    opds = (OPDS*) nxtopds;                   /*   take reqd opds */
    nxtopds += ep->dsblksiz;
    if (UNLIKELY(strcmp(ep->opname, "endin") == 0         /*  (until ENDIN)  */
                 || strcmp(ep->opname, "endop") == 0))    /*  (or ENDOP)     */
      break;

    if (UNLIKELY(strcmp(ep->opname, "pset") == 0)) {
      ip->p1.value = (MYFLT) insno;
      continue;
    }

    if (UNLIKELY(odebug))
      csound->Message(csound, Str("op (%s) allocated at %p for instr %d nxt %p\n"),
                      ep->opname, opds, insno, nxtopds);
    /* Initialize OPDS linkage and function pointers to safe defaults */
    opds->nxti = NULL;
    opds->nxtp = NULL;
    opds->nxtd = NULL;
    opds->init = NULL;
    opds->perf = NULL;
    opds->deinit = NULL;

    opds->optext = optxt;                     /* set common headata */
    opds->insdshead = ip;
    if (strcmp(ep->opname, "$label") == 0) {     /* LABEL:       */
      LBLBLK  *lblbp = (LBLBLK *) opds;
      lblbp->prvi = prvids;                   /*    save i/p links */
      lblbp->prvp = prvpds;
      lblbp->prvd = prvpdd;
      continue;                               /*    for later refs */
    }

    if (ep->init != NULL) {  /* init */
      prvids = prvids->nxti = opds; /* link into ichain */
      opds->init = ep->init; /*   & set exec adr */
      if (UNLIKELY(odebug))
        csound->Message(csound, "%s init = %p\n",
                        ep->opname,(void*) opds->init);
    }
    if (ep->perf != NULL) {  /* perf */
      prvpds = prvpds->nxtp = opds; /* link into pchain */
      opds->perf = ep->perf;  /*     perf   */
      if (UNLIKELY(odebug))
        csound->Message(csound, "%s perf = %p\n",
                        ep->opname,(void*) opds->perf);
    }
    if(ep->deinit != NULL) {  /* deinit */
      prvpdd = prvpdd->nxtd = opds; /* link into dchain */
      opds->deinit = ep->deinit;  /*   deinit   */
      if (UNLIKELY(odebug))
        csound->Message(csound, "%s deinit = %p\n",
                        ep->opname,(void*) opds->deinit);
    }

    /* Set up argument pointers for this opcode */
    setup_opcode_argpp(csound, opds, ttp, ep, ip, tp, lclbas, lcloffbas, opMemStart);

  }
  /* display instantiated instrument */
  if(csoundGetDebug(csound) & DEBUG_RUNTIME ||
     csoundGetDebug(csound) & DEBUG_INSTR) {
    csoundMessage(csound, "instantiated instr %d\n", ip->insno);
    optxt = (OPTXT*) tp;
    while ((optxt = optxt->nxtop) != NULL) {
      if(strcmp(optxt->t.opcod, "endin") &&
         strcmp(optxt->t.opcod, "endop")) {
         csound->Message(csound, " ");
         print_opcall(csound, &(optxt->t));
      }
    }

    csoundMessage(csound, "endin (instr %d)\n", ip->insno);
  }

  initialize_instance_reserved_variables(csound, ip, lclbas);


  if (UNLIKELY(nxtopds > opdslim))
    csoundDie(csound, Str("inconsistent opds total"));

  if (link) {
    /* Publish only after the instance and its opcode graph are complete. */
    inactive_instance_lock(csound);
    ip->prvinstance = tp->lst_instance;
    if (tp->lst_instance != NULL)
      tp->lst_instance->nxtinstance = ip;
    else
      tp->instance = ip;
    tp->lst_instance = ip;
    ip->nxtact = tp->act_instance;
    tp->act_instance = ip;
    ip->linked = 1;
    inactive_instance_unlock(csound);
    if (csoundGetDebug(csound) & DEBUG_RUNTIME)
      csoundMessage(csound, "instance(): tp->act_instance = %p\n",
                    tp->act_instance);
  }

  return ip;
}

INSDS *instance(CSOUND *csound, int32_t insno) {
  return instantiate(csound, insno, 1);
}

/**
 * check for mismatching context
 */
int32_t instr_context_check(CSOUND *csound, INSDS *ip, INSDS *insdshead) {
  // different SR always fails check
  if(ip->esr != insdshead->esr) return NOTOK;
  // otherwise there is no context incompatibility
  return OK;
}

/** create instance
    - allocates a new instance
    - does not add instance to activ chain or instr act_instance

    Returns the instance pointer, uninitialised
    NB: should only be called at i-time
*/
INSDS *create_instance(CSOUND *csound, int32_t insno)
{
  INSDS     *ip;

  // create instance but don't link into act_instance chain
  ip = instantiate(csound, insno, 0);
  if(ip != NULL) {
    ip->init_done = 0;
    ip->insno = (int16_t) insno;
    ip->esr = csound->esr;
    ip->pidsr = csound->pidsr;
    ip->sicvt = csound->sicvt;
    ip->onedsr = csound->onedsr;
    ip->ksmps = csound->ksmps;
    ip->ekr = csound->ekr;
    ip->kcounter = csound->kcounter;
    ip->onedksmps = csound->onedksmps;
    ip->onedkr = csound->onedkr;
    ip->kicvt = csound->kicvt;
    ip->pds = NULL;
    ip->tieflag = 0;
    ip->actflg = 0;
    ip->offbet = -1.0;
    ip->offtim = -1.0;
    ip->m_chnbp = (MCHNBLK*) NULL;
    ip->xtratim = 0;
    ip->relesing = 0;
    ip->m_sust = 0;
    ip->nxtolap = NULL;
    ip->opcod_iobufs = NULL;
    ip->ksmps_offset = 0;
    ip->ksmps_no_end = 0;
    ip->no_end = 0;
    ip->linked = 0;
    ip->nxtoff = ip->nxtact = ip->prvact = NULL; /* NOT in act chain */


    csound->instance_count++;
    ip->instance_id = csound->instance_count;
    const OPARMS* O = csound->GetOParms(csound);
    if (UNLIKELY(O->msglevel & CS_RNGEMSG)) {
      char *name = csound->engineState.instrtxtp[ip->insno]->insname;
      if (UNLIKELY(name))
        csound->ErrorMsg(csound,
                         Str("new free alloc for instr %s:\n"), name);
      else
        csound->ErrorMsg(csound,
                         Str("new free alloc for instr %d:\n"), ip->insno);
    }
  }
  return ip;
}

/** Free instance memory
    - Instances linked to the instr act_instance chain
    are not freed, since these instances are freed by
    free_inactive_instances()
    - Unlinked instances are freed.
    All active instances are turned off.
*/
static void free_unlinked_instance(CSOUND *csound, INSDS *ip)
{
  /* The pending-turnoff handoff polls readers before reaching this function.
     Never wait here: delete can run while its caller owns an engine lock. */
  if (UNLIKELY(instance_has_async_refs(ip)))
    return;

  // deactivate any opcodes
  // NB: memory for these is freed elsewhere (free_inactive_instances)
  // as opcodes exist in the instr act_instance chain
  if (ip->opcod_deact) {
    int32_t k;
    UOPCODE *p = (UOPCODE*) ip->opcod_deact;
    // free converter if it has already been created (maybe we could reuse?)
    for(k=0; k<OPCODENUMOUTS_MAX; k++)
      if(p->cvt_in[k] != NULL) {
        src_deinit(csound, p->cvt_in[k]);
        p->cvt_in[k] = NULL; // clear pointer
      }

    for(k=0; k<OPCODENUMOUTS_MAX; k++)
      if(p->cvt_out[k] != NULL) {
        src_deinit(csound, p->cvt_out[k]);
        p->cvt_out[k] = NULL; // clear pointer
      }

    deact(csound, p->ip);     /* deactivate */
    p->ip = NULL;
    p->h.perf = (SUBR) useropcd;
    ip->opcod_deact = NULL;
  }
  // same for any subinstrs - these behave like UDOS
  if (ip->subins_deact) {
    deact(csound, ((SUBINST*) ip->subins_deact)->ip);
    ((SUBINST*) ip->subins_deact)->ip = NULL;
    ip->subins_deact = NULL;
  }
  // now we deal with memory created for this ip
  if (ip->fdchp != NULL)
    fdchclose(csound, ip);
  if (ip->auxchp != NULL)
    auxchfree(csound, ip);
  free_instr_var_memory(csound, ip);
  const OPARMS* O = csound->GetOParms(csound);
  if (UNLIKELY(O->msglevel & CS_RNGEMSG)) {
    char *name = csound->engineState.instrtxtp[ip->insno]->insname;
    if (UNLIKELY(name))
      csound->ErrorMsg(csound, Str("instance %llu (instr %s) deleted\n"),
                       ip->instance_id, name);
    else
      csound->ErrorMsg(csound, Str("instance %llu (instr %d) deleted\n"),
                       ip->instance_id, ip->insno);
  }
  csound->Free(csound, ip);
}

void free_instance(CSOUND *csound, INSDS *ip) {
  // don't touch any instances that are in the act_instance chain
  if(ip->linked) {
    /* Every active-chain node has a non-null predecessor; the head points to
       actanchor. Free-list nodes clear prvact, so do not reactivate those. */
    if (ATOMIC_GET8(ip->actflg) == 0 && ip->prvact != NULL)
      ATOMIC_SET8(ip->actflg, 1);
    xturnoff_now(csound, ip);
    return;
  }

  /* Unlinked deletion always uses the non-blocking handoff. This covers both
     queued initialization and a live asynchronous reader without sleeping on
     the performance thread or while an engine lock is held. */
  async_instance_lock(csound);
  ATOMIC_SET(ip->free_pending, 1);
  async_instance_unlock(csound);
  instance_init_request_turnoff(csound, ip);
}

/** Initialise an instance
    - copy data from evt pfields
    - run init pass
    returns CSOUND_SUCCESS or an error code
*/
int32_t init_instance(CSOUND *csound, INSDS *ip,
                      EVTBLK *newevtp){
  EVTBLK *initevt = csound->init_event;
  INSTRTXT *tp = csound->engineState.instrtxtp[ip->insno];
  CS_VAR_MEM *pfields = NULL;
  int32_t   i, n, error = CSOUND_SUCCESS;
  INSTANCE_INIT_RESULT initResult;
  MYFLT  *fep;
  pfields = (CS_VAR_MEM*) &ip->p0;
  /* init: */
  if (tp->psetdata) {
    MYFLT *pdat = tp->psetdata + 2;
    int32 nn = tp->pmax - 2; /*   put cur vals in pflds */
    for (i = 0; i < nn; i++) {
      CS_VAR_MEM* pfield = (pfields + i + 3);
      pfield->value = *(pdat + i);
    }
  }

  fep = &newevtp->p[0];
  n = newevtp->pcnt;
  for (i = 1; i < n + 1; i++) {
    CS_VAR_MEM* pfield = pfields + i;
    pfield->varType = (CS_TYPE*)&CS_VAR_TYPE_P;
    pfield->value = fep[i];
  }
  if (n < tp->pmax && tp->psetdata==NULL) {
    for (i = 0; i < tp->pmax - n; i++) {
      CS_VAR_MEM* pfield = pfields + i + n + 1;
      pfield->varType = (CS_TYPE*)&CS_VAR_TYPE_P;
      pfield->value = 0;
    }
  }

  csound->inerrcnt = 0;
  ip->strarg = newevtp->strarg;
  csound->init_event = newevtp;
  if (UNLIKELY(instance_init_begin(csound, ip) != CSOUND_SUCCESS)) {
    csound->init_event = initevt;
    return csound->InitError(csound,
                             Str("cannot initialize instrument %d while "
                                 "it is being turned off"), ip->insno);
  }
  error = init_pass(csound, ip);
  initResult = instance_init_finish(csound, ip);
  if (initResult == INSTANCE_INIT_COMPLETE && error == 0) {
    ATOMIC_SET(ip->init_done, 1);
    ip->actflg = 1;  // set as active
  }
  else {
    ATOMIC_SET(ip->init_done, 0);
  }
  if (initResult == INSTANCE_INIT_COMPLETE && error != 0)
    ip->actflg = 0;  // set as inactive
  csound->init_event = initevt;
  return error;
}
