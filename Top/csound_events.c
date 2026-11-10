/*
    csound_events.c: engine events

    Copyright (C) 2025 The Csound Developers

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
#include "csoundCore.h"
#include "corfile.h"
#include "sread.h"

void score_event_async(CSOUND *csound, char type, const cs_float *pfields,
                           long numFields);
void read_score_async(CSOUND *csound, const char *message);
void async_refused(CSOUND *csound);
void csoundInputMessage(CSOUND *csound, const char *message);

void rewind_score(CSOUND *csound);   /* musmon.c */
void midifile_rewind_score(CSOUND *csound); /* midifile.c */


int32_t csound_score_event(CSOUND *csound, char type,
                                 const cs_float *pfields, long numFields) {
  if ((csound->engineStatus & CS_STATE_COMP) == 0) {
      csound->Message(csound,
                    Str("Csound has not started yet, no events scheduled.\n"));
    return CSOUND_ERROR;
  }
  EVTBLK evt;
  int32_t ret;
  memset(&evt, 0, sizeof(EVTBLK));
  evt.strarg = NULL;
  evt.scnt = 0;
  evt.opcod = type;
  evt.pcnt = (int16)numFields;
  ret = insert_event_at_sample(csound, &evt, pfields, csound->icurTimeSamples);
  return ret;
}

int32_t csound_score_event_absolute(CSOUND *csound, char type,
                                         const cs_float *pfields, long numFields,
                                         cs_double time_ofs) {
  if ((csound->engineStatus & CS_STATE_COMP) == 0) {
      csound->Message(csound,
                    Str("Csound has not started yet, no events scheduled.\n"));
    return CSOUND_ERROR;
  }
  EVTBLK evt;
  int32_t ret;
  memset(&evt, 0, sizeof(EVTBLK));
  evt.strarg = NULL;
  evt.scnt = 0;
  evt.opcod = type;
  evt.pcnt = (int16)numFields;
  ret = insert_event_at_sample(csound, &evt, pfields, time_ofs*csound->esr);
  return ret;
}



#if defined(_MSC_VER)
#define SCORE_READ_LOCAL __declspec(thread)
#elif defined(BARE_METAL)
#define SCORE_READ_LOCAL
#else
#define SCORE_READ_LOCAL _Thread_local
#endif

/* Where a fatal error in the score reader lands during csoundReadScore().
   exitjmp cannot serve: it is the frame of a call that has returned when a host
   calls from outside a performance, it belongs to the performance thread when
   a host calls beside one, and a jump to it ends that performance. So each
   thread has its own target, and another thread's fatal error never finds it. */
typedef struct score_read_s {
  CSOUND *csound;
  jmp_buf target;
  struct score_read_s *outer;
} SCORE_READ;

static SCORE_READ_LOCAL SCORE_READ *score_read;

/* Does not return if this thread is reading a score for this instance. */
void score_read_unwind(CSOUND *csound) {
  if (score_read != NULL && score_read->csound == csound)
    longjmp(score_read->target, 1);
}

int32_t csoundReadScore(CSOUND *csound, const char *str) {
  SCORE_READ guard;
  int32_t first;
  char *sc;

  /* protect resource */
  if (csound->scorestr != NULL && csound->scorestr->body != NULL)
    corfile_rewind(csound->scorestr);
  csound->scorestr = corfile_create_w(csound);
  corfile_puts(csound, (char *)str, csound->scorestr);
  // #ifdef SCORE_PARSER
  if (csound->engineStatus & CS_STATE_COMP)
    corfile_puts(csound, "\n#exit\n", csound->scorestr);
  else
    corfile_puts(csound, "\ne\n#exit\n", csound->scorestr);
  // #endif
  corfile_flush(csound, csound->scorestr);
  first = csound->scstr == NULL && (csound->engineStatus & CS_STATE_COMP) == 0;
  guard.csound = csound;
  guard.outer = score_read;
  if (setjmp(guard.target)) {
    score_read = guard.outer;
    sread_abandon(csound);
    if (first)
      corfile_rm(csound, &csound->scstr);
    csound->ErrorMsg(csound, Str("score refused: none of it was scheduled\n"));
    return CSOUND_ERROR;
  }
  score_read = &guard;
  sc = scsortstr(csound, csound->scorestr);
  score_read = guard.outer;
  /* copy sorted score name */
  if (first) {
    csound->playscore = csound->scstr;
    // corfile_rm(csound, &(csound->scorestr));
    // printf("%s\n", O->playscore->body);
  } else {
    csoundInputMessage(csound, (const char *)sc);
    csound->Free(csound, sc);
    corfile_rm(csound, &(csound->scorestr));
  }
  return CSOUND_SUCCESS;
}

 void csoundEventString(CSOUND *csound, const char *message,
                              int32_t async) {
  if (async) {
    read_score_async(csound, message);
  } else
    csoundReadScore(csound, message);
}

 void csoundEvent(CSOUND *csound, int32_t type, const cs_float *params,
                        int32_t nparams, int32_t async) {
  char c;
  if (type == CS_INSTR_EVENT)
    c = 'i';
  else if (type == CS_TABLE_EVENT)
    c = 'f';
  else if (type == CS_END_EVENT)
    c = 'e';
  else if (type == CS_ADV_EVENT)
    c = 'a';
  else {
    if (async)
      async_refused(csound);
    return;
  }

  if (async)
    score_event_async(csound, c, params, nparams);
  else
    csound_score_event(csound, c, params, nparams);
}

/*
 * SCORE HANDLING
 */

 int32_t csoundIsScorePending(CSOUND *csound) {
  return csound->csoundIsScorePending_;
}

 void csoundSetScorePending(CSOUND *csound, int32_t pending) {
  csound->csoundIsScorePending_ = pending;
}

 void csoundSetScoreOffsetSeconds(CSOUND *csound, cs_float offset) {
  cs_double aTime;
  cs_float prv = (cs_float)csound->csoundScoreOffsetSeconds_;

  csound->csoundScoreOffsetSeconds_ = offset;
  if (offset < FL(0.0))
    return;
  /* if csoundCompile() was not called yet, just store the offset */
  if (!(csound->engineStatus & CS_STATE_COMP))
    return;
  /* otherwise seek to the requested time now */
  aTime = (cs_double)offset - (csound->icurTimeSamples / csound->esr);
  if (aTime < 0.0 || offset < prv) {
    csoundRewindScore(csound); /* will call csoundSetScoreOffsetSeconds */
    return;
  }
  if (aTime > 0.0) {
    EVTBLK evt;
    memset(&evt, 0, sizeof(EVTBLK));
    cs_float pfields[3];
    evt.strarg = NULL;
    evt.scnt = 0;
    evt.opcod = 'a';
    evt.pcnt = 3;
    pfields[1] =  pfields[0] = FL(0.0);
    pfields[2] = (cs_float)aTime;
    insert_event_at_sample(csound, &evt, pfields, csound->icurTimeSamples);
  }
}

 cs_float csoundGetScoreOffsetSeconds(CSOUND *csound) {
  return csound->csoundScoreOffsetSeconds_;
}

 void csoundRewindScore(CSOUND *csound) {
  rewind_score(csound);
  if (csound->oparms->FMidiname != NULL)
    midifile_rewind_score(csound);
}
