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

void score_event_async(CSOUND *csound, char type, const MYFLT *pfields,
                           long numFields);
void read_score_async(CSOUND *csound, const char *message);
void csoundInputMessage(CSOUND *csound, const char *message);

void rewind_score(CSOUND *csound);   /* musmon.c */
void midifile_rewind_score(CSOUND *csound); /* midifile.c */


int32_t csound_score_event(CSOUND *csound, char type,
                                 const MYFLT *pfields, long numFields) {
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
  ret = csoundEvent__at_sample(csound, &evt, pfields, csound->icurTimeSamples);
  return ret;
}

int32_t csound_score_event_absolute(CSOUND *csound, char type,
                                         const MYFLT *pfields, long numFields,
                                         double time_ofs) {
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
  ret = csoundEvent__at_sample(csound, &evt, pfields, time_ofs*csound->esr);
  return ret;
}



int32_t csoundReadScore(CSOUND *csound, const char *str) {
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
  /* copy sorted score name */
  if (csound->scstr == NULL && (csound->engineStatus & CS_STATE_COMP) == 0) {
    scsortstr(csound, csound->scorestr);
    csound->playscore = csound->scstr;
    // corfile_rm(csound, &(csound->scorestr));
    // printf("%s\n", O->playscore->body);
  } else {

    char *sc = scsortstr(csound, csound->scorestr);
    csoundInputMessage(csound, (const char *)sc);
    csound->Free(csound, sc);
    corfile_rm(csound, &(csound->scorestr));
  }
  return CSOUND_SUCCESS;
}

PUBLIC void csoundEventString(CSOUND *csound, const char *message,
                              int32_t async) {
  if (async) {
    read_score_async(csound, message);
  } else
    csoundReadScore(csound, message);
}

PUBLIC void csoundEvent(CSOUND *csound, int32_t type, const MYFLT *params,
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
  else return;

  if (async)
    score_event_async(csound, c, params, nparams);
  else
    csound_score_event(csound, c, params, nparams);
}

/*
 * SCORE HANDLING
 */

PUBLIC int32_t csoundIsScorePending(CSOUND *csound) {
  return csound->csoundIsScorePending_;
}

PUBLIC void csoundSetScorePending(CSOUND *csound, int32_t pending) {
  csound->csoundIsScorePending_ = pending;
}

PUBLIC void csoundSetScoreOffsetSeconds(CSOUND *csound, MYFLT offset) {
  double aTime;
  MYFLT prv = (MYFLT)csound->csoundScoreOffsetSeconds_;

  csound->csoundScoreOffsetSeconds_ = offset;
  if (offset < FL(0.0))
    return;
  /* if csoundCompile() was not called yet, just store the offset */
  if (!(csound->engineStatus & CS_STATE_COMP))
    return;
  /* otherwise seek to the requested time now */
  aTime = (double)offset - (csound->icurTimeSamples / csound->esr);
  if (aTime < 0.0 || offset < prv) {
    csoundRewindScore(csound); /* will call csoundSetScoreOffsetSeconds */
    return;
  }
  if (aTime > 0.0) {
    EVTBLK evt;
    memset(&evt, 0, sizeof(EVTBLK));
    MYFLT pfields[3];
    evt.strarg = NULL;
    evt.scnt = 0;
    evt.opcod = 'a';
    evt.pcnt = 3;
    evt.p[1] = evt.p[0] = FL(0.0);
    evt.p[2] = (MYFLT)aTime;
    csoundEvent__at_sample(csound, &evt, pfields, csound->icurTimeSamples);
  }
}

PUBLIC MYFLT csoundGetScoreOffsetSeconds(CSOUND *csound) {
  return csound->csoundScoreOffsetSeconds_;
}

PUBLIC void csoundRewindScore(CSOUND *csound) {
  rewind_score(csound);
  if (csound->oparms->FMidiname != NULL)
    midifile_rewind_score(csound);
}
