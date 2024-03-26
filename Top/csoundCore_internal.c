// kperf_debug
// csoundIsInitThread

#include "csoundCore_internal.h"

#include <string.h>           // for NULL, memset

#include "cs_new_dispatch.h"  // for dag_build, dag_reinit, dag_end_task
#include "csdebug.h"          // for csdebug_data_t, bkpt_node_t, CSDEBUG_ST...
#include "csdebug_internal.h"
#include "prototyp.h"         // for csoundYield
#include "threadsafe.h"       // for message_dequeue

/* return 1 if the current op thread is init-time,
   zero if not.
   return value may be incorrect in realtime mode
*/
int csoundIsInitThread(CSOUND *csound) { return csound->ids ? 1 : 0; }

inline static void mix_out(MYFLT *out, MYFLT *in, uint32_t smps) {
  int i;
  for (i = 0; i < smps; i++)
    out[i] += in[i];
}

#ifdef PARCS
inline int nodePerf(CSOUND *csound, int index, int numThreads) {
  INSDS *insds = NULL;
  OPDS *opstart = NULL;
  int played_count = 0;
  int which_task;
  INSDS **task_map = (INSDS **)csound->dag_task_map;
  double time_end;
#define INVALID (-1)
#define WAIT (-2)
  int next_task = INVALID;
  IGN(index);

  while (1) {
    int done;
    which_task = dag_get_task(csound, index, numThreads, next_task);
    // printf("******** Select task %d %d\n", which_task, index);
    if (which_task == WAIT)
      continue;
    if (which_task == INVALID)
      return played_count;
    /* VL: the validity of icurTime needs to be checked */
    time_end = (csound->ksmps + csound->icurTime) / csound->esr;
    insds = task_map[which_task];
    if (insds->offtim > 0 && time_end > insds->offtim) {
      /* this is the last cycle of performance */
      insds->ksmps_no_end = insds->no_end;
    }
#if defined(MSVC)
    done = InterlockedExchangeAdd(&insds->init_done, 0);
#elif defined(HAVE_ATOMIC_BUILTIN)
    done = __atomic_load_n((int *)&insds->init_done, __ATOMIC_SEQ_CST);
#else
    done = insds->init_done;
#endif

    if (done) {
      opstart = (OPDS *)task_map[which_task];
      if (insds->ksmps == csound->ksmps) {
        insds->spin = csound->spin;
        insds->spout = csound->spout_tmp + index * csound->nspout;
        insds->kcounter = csound->kcounter;
        csound->mode = 2;
        while ((opstart = opstart->nxtp) != NULL) {
          /* In case of jumping need this repeat of opstart */
          opstart->insdshead->pds = opstart;
          csound->op = opstart->optext->t.opcod;
          (*opstart->opadr)(csound, opstart); /* run each opcode */
          opstart = opstart->insdshead->pds;
        }
        csound->mode = 0;
      } else {
        int i, n = csound->nspout, start = 0;
        int lksmps = insds->ksmps;
        int incr = csound->nchnls * lksmps;
        int offset = insds->ksmps_offset;
        int early = insds->ksmps_no_end;
        OPDS *opstart;
        insds->spin = csound->spin;
        insds->spout = csound->spout_tmp + index * csound->nspout;
        insds->kcounter = csound->kcounter * csound->ksmps;

        /* we have to deal with sample-accurate code
           whole CS_KSMPS blocks are offset here, the
           remainder is left to each opcode to deal with.
        */
        while (offset >= lksmps) {
          offset -= lksmps;
          start += csound->nchnls;
        }
        insds->ksmps_offset = offset;
        if (UNLIKELY(early)) {
          n -= (early * csound->nchnls);
          insds->ksmps_no_end = early % lksmps;
        }
        for (i = start; i < n;
             i += incr, insds->spin += incr, insds->spout += incr) {
          opstart = (OPDS *)insds;
          csound->mode = 2;
          while ((opstart = opstart->nxtp) != NULL) {
            opstart->insdshead->pds = opstart;
            csound->op = opstart->optext->t.opcod;
            (*opstart->opadr)(csound, opstart); /* run each opcode */
            opstart = opstart->insdshead->pds;
          }
          csound->mode = 0;
          insds->kcounter++;
        }
      }
      insds->ksmps_offset = 0; /* reset sample-accuracy offset */
      insds->ksmps_no_end = 0; /* reset end of loop samples */
      played_count++;
    }
    // printf("******** finished task %d\n", which_task);
    next_task = dag_end_task(csound, which_task);
  }
  return played_count;
}
#endif // PARCS

int kperf_nodebug(CSOUND *csound) {
  INSDS *ip;
  int lksmps = csound->ksmps;
  /* update orchestra time */
  csound->kcounter = ++(csound->global_kcounter);
  csound->icurTime += csound->ksmps;
  csound->curBeat += csound->curBeat_inc;

  /* call message_dequeue to run API calls */
  message_dequeue(csound);

  /* if skipping time on request by 'a' score statement: */
  if (UNLIKELY(UNLIKELY(csound->advanceCnt))) {
    csound->advanceCnt--;
    return 1;
  }
  /* if i-time only, return now */
  if (UNLIKELY(csound->initonly))
    return 1;
  /* PC GUI needs attention, but avoid excessively frequent */
  /* calls of csoundYield() */
  if (UNLIKELY(--(csound->evt_poll_cnt) < 0)) {
    csound->evt_poll_cnt = csound->evt_poll_maxcnt;
    if (UNLIKELY(!csoundYield(csound)))
      csound->LongJmp(csound, 1);
  }

  /* for one kcnt: */
  if (csound->oparms_.sfread) /*   if audio_infile open  */
    csound->spinrecv(csound); /*      fill the spin buf  */
  /* clear spout */
  memset(csound->spout, 0, csound->nspout * sizeof(MYFLT));
  memset(csound->spout_tmp, 0,
         sizeof(MYFLT) * csound->nspout * csound->oparms->numThreads);
  ip = csound->actanchor.nxtact;

  if (ip != NULL) {
    /* There are 2 partitions of work: 1st by inso,
       2nd by inso count / thread count. */
    if (csound->multiThreadedThreadInfo != NULL) {
#ifdef PARCS
      if (csound->dag_changed)
        dag_build(csound, ip);
      else
        dag_reinit(csound); /* set to initial state */

      /* process this partition */
      csound->WaitBarrier(csound->barrier1);

      (void)nodePerf(csound, 0, 1);

      /* wait until partition is complete */
      csound->WaitBarrier(csound->barrier2);

      // do the mixing of thread buffers
      {
        int k;
        for (k = 1; k < csound->oparms->numThreads; k++)
          mix_out(csound->spout_tmp, csound->spout_tmp + k * csound->nspout,
                  csound->nspout);
      }
#endif
      csound->multiThreadedDag = NULL;
    } else {
      int done;
      double time_end = (csound->ksmps + csound->icurTime) / csound->esr;

      while (ip != NULL) { /* for each instr active:  */
        INSDS *nxt = ip->nxtact;
        if (UNLIKELY(csound->oparms->sampleAccurate && ip->offtim > 0 &&
                     time_end > ip->offtim)) {
          /* this is the last cycle of performance */
          //   csound->Message(csound, "last cycle %d: %f %f %d\n",
          //       ip->insno, csound->icurTime/csound->esr,
          //          ip->offtim, ip->no_end);
          ip->ksmps_no_end = ip->no_end;
        }
        done = ATOMIC_GET(ip->init_done);
        if (done == 1) { /* if init-pass has been done */
          int error = 0;
          OPDS *opstart = (OPDS *)ip;
          ip->spin = csound->spin;
          ip->spout = csound->spout_tmp;
          ip->kcounter = csound->kcounter;
          if (ip->ksmps == csound->ksmps) {
            csound->mode = 2;
            while (error == 0 && opstart != NULL &&
                   (opstart = opstart->nxtp) != NULL && ip->actflg) {
              opstart->insdshead->pds = opstart;
              csound->op = opstart->optext->t.opcod;
              error = (*opstart->opadr)(csound, opstart); /* run each opcode */
              opstart = opstart->insdshead->pds;
            }
            csound->mode = 0;
          } else {
            int error = 0;
            int i, n = csound->nspout, start = 0;
            lksmps = ip->ksmps;
            int incr = csound->nchnls * lksmps;
            int offset = ip->ksmps_offset;
            int early = ip->ksmps_no_end;
            OPDS *opstart;
            ip->kcounter = (csound->kcounter - 1) * csound->ksmps / lksmps;

            /* we have to deal with sample-accurate code
               whole CS_KSMPS blocks are offset here, the
               remainder is left to each opcode to deal with.
            */
            while (offset >= lksmps) {
              offset -= lksmps;
              start += csound->nchnls;
            }
            ip->ksmps_offset = offset;
            if (UNLIKELY(early)) {
              n -= (early * csound->nchnls);
              ip->ksmps_no_end = early % lksmps;
            }
            for (i = start; i < n;
                 i += incr, ip->spin += incr, ip->spout += incr) {
              ip->kcounter++;
              opstart = (OPDS *)ip;
              csound->mode = 2;
              while (error == 0 && (opstart = opstart->nxtp) != NULL &&
                     ip->actflg) {
                opstart->insdshead->pds = opstart;
                csound->op = opstart->optext->t.opcod;
                // csound->ids->optext->t.oentry->opname;
                error =
                    (*opstart->opadr)(csound, opstart); /* run each opcode */
                opstart = opstart->insdshead->pds;
              }
              csound->mode = 0;
            }
          }
        }
        /*else csound->Message(csound, "time %f\n",
                               csound->kcounter/csound->ekr);*/
        ip->ksmps_offset = 0; /* reset sample-accuracy offset */
        ip->ksmps_no_end = 0; /* reset end of loop samples */
        if (nxt == NULL)
          ip = ip->nxtact;
        /* VL 13.04.21 this allows for deletions to operate
                            correctly on the active
                            list at perf time
                            this allows for turnoff2 to work correctly
                            */
        else
          ip = nxt; /* now check again if there is nothing nxt
                                     in the chain making sure turnoff also
                                     works */
      }
    }
  }
  csound->spoutran(csound); /* send to audio_out */
  return 0;
}

static inline void opcode_perf_debug(CSOUND *csound, csdebug_data_t *data,
                                     INSDS *ip) {
  OPDS *opstart = (OPDS *)ip;
  while ((opstart = opstart->nxtp) != NULL) {
    /* check if we have arrived at a line breakpoint */
    bkpt_node_t *bp_node = data->bkpt_anchor->next;
    if (data->debug_opcode_ptr) {
      opstart = data->debug_opcode_ptr;
      data->debug_opcode_ptr = NULL;
    }
    int linenum = opstart->optext->t.linenum;
    while (bp_node) {
      if (bp_node->instr == ip->p1.value || (bp_node->instr == 0)) {
        if ((bp_node->line) == linenum) { /* line matches */
          if (bp_node->count < 2) { /* skip of 0 or 1 has the same effect */
            if (data->debug_opcode_ptr != opstart) { /* did we just stop here */
              data->debug_instr_ptr = ip;
              data->debug_opcode_ptr = opstart;
              data->status = CSDEBUG_STATUS_STOPPED;
              data->cur_bkpt = bp_node;
              csoundDebuggerBreakpointReached(csound);
              bp_node->count = bp_node->skip;
              return;
            } else {
              data->debug_opcode_ptr = NULL; /* if just stopped here-continue */
            }
          } else {
            bp_node->count--;
          }
        }
      }
      bp_node = bp_node->next;
    }
    opstart->insdshead->pds = opstart;
    csound->mode = 2;
    (*opstart->opadr)(csound, opstart); /* run each opcode */
    opstart = opstart->insdshead->pds;
    csound->mode = 0;
  }
  mix_out(csound->spout, ip->spout, ip->ksmps * csound->nchnls);
}

static inline void process_debug_buffers(CSOUND *csound, csdebug_data_t *data) {
  bkpt_node_t *bkpt_node;
  while (csoundReadCircularBuffer(csound, data->bkpt_buffer, &bkpt_node, 1) ==
         1) {
    if (bkpt_node->mode == CSDEBUG_BKPT_CLEAR_ALL) {
      bkpt_node_t *n;
      while (data->bkpt_anchor->next) {
        n = data->bkpt_anchor->next;
        data->bkpt_anchor->next = n->next;
        csound->Free(csound, n); /* TODO this should be moved from kperf to a
                    non-realtime context */
      }
      csound->Free(csound, bkpt_node);
    } else if (bkpt_node->mode == CSDEBUG_BKPT_DELETE) {
      bkpt_node_t *n = data->bkpt_anchor->next;
      bkpt_node_t *prev = data->bkpt_anchor;
      while (n) {
        if (n->line == bkpt_node->line && n->instr == bkpt_node->instr) {
          prev->next = n->next;
          if (data->cur_bkpt == n)
            data->cur_bkpt = n->next;
          csound->Free(csound, n); /* TODO this should be moved from kperf to a
                      non-realtime context */
          n = prev->next;
          continue;
        }
        prev = n;
        n = n->next;
      }
      //        csound->Free(csound, bkpt_node); /* TODO move to non rt context
      //        */
    } else {
      // FIXME sort list to optimize
      bkpt_node->next = data->bkpt_anchor->next;
      data->bkpt_anchor->next = bkpt_node;
    }
  }
}

int kperf_debug(CSOUND *csound) {
  INSDS *ip;
  csdebug_data_t *data = (csdebug_data_t *)csound->csdebug_data;
  int lksmps = csound->ksmps;
  /* call message_dequeue to run API calls */
  message_dequeue(csound);

  if (!data || data->status != CSDEBUG_STATUS_STOPPED) {
    /* update orchestra time */
    csound->kcounter = ++(csound->global_kcounter);
    csound->icurTime += csound->ksmps;
    csound->curBeat += csound->curBeat_inc;
  }

  /* if skipping time on request by 'a' score statement: */
  if (UNLIKELY(csound->advanceCnt)) {
    csound->advanceCnt--;
    return 1;
  }
  /* if i-time only, return now */
  if (UNLIKELY(csound->initonly))
    return 1;
  /* PC GUI needs attention, but avoid excessively frequent */
  /* calls of csoundYield() */
  if (UNLIKELY(--(csound->evt_poll_cnt) < 0)) {
    csound->evt_poll_cnt = csound->evt_poll_maxcnt;
    if (UNLIKELY(!csoundYield(csound)))
      csound->LongJmp(csound, 1);
  }

  if (data) { /* process debug commands*/
    process_debug_buffers(csound, data);
  }

  if (!data || data->status == CSDEBUG_STATUS_RUNNING) {
    /* for one kcnt: */
    if (csound->oparms_.sfread) /*   if audio_infile open  */
      csound->spinrecv(csound); /*      fill the spin buf  */
    /* clear spout */
    memset(csound->spout, 0, csound->nspout * sizeof(MYFLT));
    memset(csound->spout_tmp, 0, csound->nspout * sizeof(MYFLT));
  }

  ip = csound->actanchor.nxtact;
  /* Process debugger commands */
  debug_command_t command = CSDEBUG_CMD_NONE;
  if (data) {
    csoundReadCircularBuffer(csound, data->cmd_buffer, &command, 1);
    if (command == CSDEBUG_CMD_STOP && data->status != CSDEBUG_STATUS_STOPPED) {
      data->debug_instr_ptr = ip;
      data->status = CSDEBUG_STATUS_STOPPED;
      csoundDebuggerBreakpointReached(csound);
    }
    if (command == CSDEBUG_CMD_CONTINUE &&
        data->status == CSDEBUG_STATUS_STOPPED) {
      if (data->cur_bkpt && data->cur_bkpt->skip <= 2)
        data->cur_bkpt->count = 2;
      data->status = CSDEBUG_STATUS_RUNNING;
      if (data->debug_instr_ptr) {
        /* if not NULL, resume from last active */
        ip = data->debug_instr_ptr;
        data->debug_instr_ptr = NULL;
      }
    }
    if (command == CSDEBUG_CMD_NEXT && data->status == CSDEBUG_STATUS_STOPPED) {
      data->status = CSDEBUG_STATUS_NEXT;
    }
  }
  if (ip != NULL && data != NULL && (data->status != CSDEBUG_STATUS_STOPPED)) {
    /* There are 2 partitions of work: 1st by inso,
       2nd by inso count / thread count. */
    if (csound->multiThreadedThreadInfo != NULL) {
#ifdef PARCS
      if (csound->dag_changed)
        dag_build(csound, ip);
      else
        dag_reinit(csound); /* set to initial state */

      /* process this partition */
      csound->WaitBarrier(csound->barrier1);

      (void)nodePerf(csound, 0, 1);

      /* wait until partition is complete */
      csound->WaitBarrier(csound->barrier2);
      // do the mixing of thread buffers
      {
        int k;
        for (k = 1; k < csound->oparms->numThreads; k++)
          mix_out(csound->spout_tmp, csound->spout_tmp + k * csound->nspout,
                  csound->nspout);
      }
#endif
      csound->multiThreadedDag = NULL;
    } else {
      int done;
      double time_end = (csound->ksmps + csound->icurTime) / csound->esr;

      while (ip != NULL) { /* for each instr active:  */
        if (UNLIKELY(csound->oparms->sampleAccurate && ip->offtim > 0 &&
                     time_end > ip->offtim)) {
          /* this is the last cycle of performance */
          //   csound->Message(csound, "last cycle %d: %f %f %d\n",
          //       ip->insno, csound->icurTime/csound->esr,
          //          ip->offtim, ip->no_end);
          ip->ksmps_no_end = ip->no_end;
        }
        done = ATOMIC_GET(ip->init_done);
        if (done == 1) { /* if init-pass has been done */
          /* check if next command pending and we are on the
             first instrument in the chain */
          /* coverity says data already dereferenced by here */
          if (/*data &&*/ data->status == CSDEBUG_STATUS_NEXT) {
            if (data->debug_instr_ptr == NULL) {
              data->debug_instr_ptr = ip;
              data->debug_opcode_ptr = NULL;
              data->status = CSDEBUG_STATUS_STOPPED;
              csoundDebuggerBreakpointReached(csound);
              return 0;
            } else {
              ip = data->debug_instr_ptr;
              data->debug_instr_ptr = NULL;
            }
          }
          /* check if we have arrived at an instrument breakpoint */
          bkpt_node_t *bp_node = data->bkpt_anchor->next;
          while (bp_node && data->status != CSDEBUG_STATUS_NEXT) {
            if (bp_node->instr == ip->p1.value && (bp_node->line == -1)) {
              if (bp_node->count < 2) {
                /* skip of 0 or 1 has the same effect */
                data->debug_instr_ptr = ip;
                data->debug_opcode_ptr = NULL;
                data->cur_bkpt = bp_node;
                data->status = CSDEBUG_STATUS_STOPPED;
                csoundDebuggerBreakpointReached(csound);
                bp_node->count = bp_node->skip;
                return 0;
              } else {
                bp_node->count--;
              }
            }
            bp_node = bp_node->next;
          }
          ip->spin = csound->spin;
          ip->spout = csound->spout_tmp;
          ip->kcounter = csound->kcounter;
          if (ip->ksmps == csound->ksmps) {
            opcode_perf_debug(csound, data, ip);
          } else { /* when instrument has local ksmps */
            int i, n = csound->nspout, start = 0;
            lksmps = ip->ksmps;
            int incr = csound->nchnls * lksmps;
            int offset = ip->ksmps_offset;
            int early = ip->ksmps_no_end;
            ip->spin = csound->spin;
            ip->kcounter = csound->kcounter * csound->ksmps / lksmps;

            /* we have to deal with sample-accurate code
                 whole CS_KSMPS blocks are offset here, the
                 remainder is left to each opcode to deal with.
              */
            while (offset >= lksmps) {
              offset -= lksmps;
              start += csound->nchnls;
            }
            ip->ksmps_offset = offset;
            if (UNLIKELY(early)) {
              n -= (early * csound->nchnls);
              ip->ksmps_no_end = early % lksmps;
            }

            for (i = start; i < n;
                 i += incr, ip->spin += incr, ip->spout += incr) {
              opcode_perf_debug(csound, data, ip);
              ip->kcounter++;
            }
          }
        }
        ip->ksmps_offset = 0; /* reset sample-accuracy offset */
        ip->ksmps_no_end = 0; /* reset end of loop samples */
        ip = ip->nxtact;      /* but this does not allow for all deletions */
        if (/*data &&*/ data->status == CSDEBUG_STATUS_NEXT) {
          data->debug_instr_ptr = ip; /* we have reached the next
                                         instrument. Break */
          data->debug_opcode_ptr = NULL;
          if (ip != NULL) { /* must defer break until next kperf */
            data->status = CSDEBUG_STATUS_STOPPED;
            csoundDebuggerBreakpointReached(csound);
            return 0;
          }
        }
      }
    }
  }

  if (!data || data->status != CSDEBUG_STATUS_STOPPED)
    csound->spoutran(csound); /*      send to audio_out  */

  return 0;
}
