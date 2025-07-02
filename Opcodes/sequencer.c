
/*
  sequencer.c:

  Copyright (C) 2021    John ffitch

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
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include <math.h>
typedef struct {
  OPDS        h;
  MYFLT       *res;           /*  state */
  ARRAYDAT    *riff;          /* initial note row */
  ARRAYDAT    *instr;         /* renderers for each note */
  ARRAYDAT    *data;          /* extra data for pitch info */
  MYFLT       *kbpm;          /* speed of sequence */
  MYFLT       *klen;          /* Length of sequece to use */
  MYFLT       *mode;          /* Mode; -1 backward,
                                 0 loop frward;
                                 +ve mutate
                                 -1 backward
                                 -2 back & forth
                                 -3 random
                                 -4 frward 1-shot
                                 -5 backward 1-shot
                                 -6 shuffle
                                 -7 reset
                              */
  MYFLT       *step;          /* Step mode in force */
  MYFLT       *reset;         /* Reset key */
  MYFLT       *verbos;
  MYFLT       *id;            /* so can find it amonst others */
  // Internals
  int32_t     max_length;
  int32_t         cnt;            /* Count loops for mutator */
  int32_t         next;           /* next step nuber */
  int32_t         time;           /* time in samples to next step */
  int32_t         direction;      /* direction of steps */
  int32_t         seq[128];
} SEQ;


typedef struct {
  OPDS        h;
  MYFLT       *res;           /*  state */
  ARRAYDAT    *riff;          /* copy intervnal array */
  MYFLT       *id;
  SEQ         *q;
} SEQSTATE;


static int32_t sequencer_init(CSOUND *csound, SEQ *p)
{
  int32_t i;
  p->max_length = p->riff->sizes[0];
  if (p->max_length != p->instr->sizes[0] ||
      (p->data->dimensions == 2 && p->max_length != p->data->sizes[1]) ||
      (p->data->dimensions == 1 && p->max_length != p->data->sizes[0]) ||
      p->max_length >= 128) {
    return csound->InitError(csound, "%s", Str("sequ: arrays have differing sizes"));
  }
  p->time = 0;
  p->next = 0;
  p->cnt = 1;
  p->direction = 1;            /* forwards */
  for (i = 0; i<p->riff->sizes[0]; i++)
    p->seq[i] = i;
  for (i=0; i<p->riff->sizes[0]; i++)
    printf("%d: %d %f\n", i, (int)(p->instr->data[i]), p->riff->data[i]);
  SEQ **q;

  if ((int)(*p->id)<0 || (int)(*p->id)>9)
    return csound->InitError(csound, "%s", Str("sequ: id out of range"));
    
  q = (SEQ**)csound->QueryGlobalVariable(csound, "sequGlobals");
  if (q == NULL) {
    csound->CreateGlobalVariable(csound, "sequGlobals", 10*sizeof(SEQ*));
    q = (SEQ**)csound->QueryGlobalVariable(csound, "sequGlobals");
  }
  q[(int)*p->id] = p;
  return OK;
}


static int32_t sequencer(CSOUND *csound, SEQ *p)
{
  int32_t len = (int)*p->klen;
  int32_t i = p->next;
  int32_t mode = (int)*p->mode;

  if (len<=0) len = 1;
  if (len>=p->max_length) len= p->max_length;
  if (*p->step!=FL(0.0)) {    /* Step style so no clock */
    if (*p->step>=FL(0.0)) {  /* a user call to move on */
      p->time = 0;
    }
    else {
      p->time = CS_KSMPS;
      *p->res = -FL(1.0);
      return OK;
    }
  }
  else if (*p->reset!= FL(0.0)) {
    if (*p->verbos) printf("RESET!!\n");
    goto minus7;
  }
  else if (p->time > (int32_t) CS_KSMPS) {         /* Not yet time to act */
    //printf("**time= %d", p->time);
    p->time -= CS_KSMPS;
    *p->res = -FL(1.0);
    //printf(" -> %d\n", p->time);
    return OK;
  }
  /* Time for an event */
  if (mode >= 0) {
    if (i >= len) { // End of cycle
      i = p->next = 0; p->direction = 1;
    }
  }
  else {
    switch (mode) {
    case -1:
      if (p->cnt==1 || i < 0) { /* backward and end of loop */
        p->next = i = len-1;
        p->direction = -1;
      }
      break;
    case -2:
      if (i<0|| i>=len) {
        p->direction = -p->direction;
        p->next = i += p->direction;
      }
      break;
    case -3:
      i = rand()%len; /* random selection */
      break;
    case -4:
      p->direction = 1;
      if (i>=len) {
        *p->res = -1;
        return OK;
      }
      break;
    case -5:
      p->direction = -1;
      if (p->cnt==1) {
        i = p->next = len-1;
      }
      else if (i<0) {
        *p->res = -1;
        return OK;
      }
      break;
    case -6:
      if (i>=len) {
        int32_t j, k = 0;
        for (j =0; j<len; j++) {
          k = rand() % (j+1);
          if (k != j) p->seq[j] = p->seq[k];
          p->seq[k] =  j;
        }
        p->next = i = 0;
        p->direction = 1;
      }
      break;
    case -7:
      minus7:
      p->time = 0;
      p->cnt = 1;
      for (i = 0; i<p->riff->sizes[0]; i++)
        p->seq[i] = i;
      i = p->next = 0;
      break;
    case -8:
      *p->res = -1;
      return OK;
      break;
    }
  }
  {
    MYFLT inst = p->instr->data[p->seq[i]];
    if (inst != 0) {
      char buff[100];
      if (p->data->dimensions==2) {
        int32_t j;
        snprintf(buff, 99, "i %0.2f 0 %g ",
                 inst, 60.0/(*p->kbpm)*p->riff->data[p->seq[i]]);
        for (j=0; j< p->data->sizes[0]; j++)
          snprintf(buff+strlen(buff), 99-strlen(buff), "%g ",
                   p->data->data[(j*p->max_length)+p->seq[i]]);
        snprintf(buff+strlen(buff), 99-strlen(buff), "\n");
      }
      else
        snprintf(buff, 100, "i %0.2f 0 %f %f\n",
                inst, 60.0/(*p->kbpm)*p->riff->data[p->seq[i]],
                p->data->data[p->seq[i]]);
      //printf("***Score;ine:%s", buff);
      csound->ReadScore(csound, buff); /* schedule instr for event */
    }
    p->time = (p->riff->data[i] * CS_ESR * 60.0) / *p->kbpm;
    /* printf("Step %d riff %d instr %0.4f len %f\n", */
    /*    i,p->seq[i], p->instr->data[p->seq[i]], p->riff->data[p->seq[i]]); */
    // Mutate every mode events
    if (mode > 0 && len>1 && p->cnt%mode == 0) {
      int32_t r1, r2;
      do {
        r1 = rand()%len;
        r2 = rand()%len;
      } while (r1==r2);
      {
        int32_t tm = p->seq[r1];
        p->seq[r1] = p->seq[r2];
        p->seq[r2] = tm;
        if (*p->verbos)
          printf("swap %d and %d\n", r1, r2);
      }
    }
    *p->res = (MYFLT)i;
    p->next += p->direction;
    //if (*p->mode >=0) p->next++;
    //else if (mode == -1) p->next--;
    if (mode != -8) p->cnt++;
  }
  if (*p->verbos)
    printf("Next Step %d time = %d samples\n", p->next, p->time);
  return OK;
}

static int32_t sequState(CSOUND *csound, SEQSTATE* p);

static int32_t sequStateInit(CSOUND *csound, SEQSTATE* p)
{
  int32_t id = (int)*p->id;
  SEQ **r = (SEQ**)csound->QueryGlobalVariable(csound, "sequGlobals");
  if (r==NULL || r[id]==NULL) {
    csound->Warning(csound, "%s", Str("No sequs"));
    return OK;
  }
  p->q = r[id];
  if (p->riff->sizes[0] != p->q->max_length)
    csound->InitError(csound, "%s", Str("sequstate: Wrong sizeof output array"));
  sequState(csound, p);
  return OK;
}

static int32_t sequState(CSOUND *csound, SEQSTATE* p)
{
  SEQ* q = p->q;
  int32_t i;
  int32_t len = (int)*q->klen;
  for (i=0; i<len; i++)
    p->riff->data[i] = /* q->riff->data[*/ q->seq[i]/*]*/;
  *p->res = (MYFLT)q->cnt;
  return OK;
}

static OENTRY sequencer_localops[] =
  {
   { "sequ", sizeof(SEQ), 0, "k",
     "i[]i[]i[]kkOOOoo",
     (SUBR) sequencer_init, (SUBR) sequencer },
   { "sequstate.i", sizeof(SEQSTATE), 0,  "ii[]", "o",
     (SUBR) sequStateInit },
   { "sequstate.k", sizeof(SEQSTATE), 0,  "kk[]", "o",
   (SUBR) sequStateInit, (SUBR) sequState
  }
};


LINKAGE_BUILTIN(sequencer_localops)
