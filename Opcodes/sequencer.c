
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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include <math.h>
#include "arrays.h"
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
  uint32_t        cnt;            /* Count loops for mutator */
  int32_t         next;           /* next step nuber */
  int32_t         time;           /* time in samples to next step */
  int32_t         direction;      /* direction of steps */
  int32_t         seq[128];
} SEQ;

typedef struct {
  OPDS        h;
  MYFLT       *res;           /*  state */
  MYFLT       *kstart;        /* kstart */
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
  uint32_t        cnt;            /* Count loops for mutator */
  int32_t         next;           /* next step nuber */
  int32_t         time;           /* time in samples to next step */
  int32_t         direction;      /* direction of steps */
  int32_t         seq[128];
  int32_t         init_flag;
} SEQ2;


/* Both sequ forms publish the same view for sequstate. */
typedef struct {
  void *owner;
  int32_t max_length;
  MYFLT *klen;
  int32_t *seq;
  uint32_t *cnt;
} SEQREF;

#define SEQU_LENGTH(value, maximum) \
  ((value) >= FL(1.0) ? ((value) < (maximum) ? (int32_t)(value) : (maximum)) : 1)

typedef struct {
  OPDS        h;
  MYFLT       *res;           /*  state */
  ARRAYDAT    *riff;          /* copy intervnal array */
  MYFLT       *id;
  SEQREF      *q;
} SEQSTATE;


static int32_t sequ_register(CSOUND *csound, void *owner, MYFLT id,
                             int32_t length, MYFLT *klen, int32_t *seq,
                             uint32_t *cnt)
{
  SEQREF *q;
  int32_t index;
  if (UNLIKELY(!(id >= FL(0.0) && id < FL(10.0))))
    return csound->InitError(csound, Str("sequ: id out of range"));
  index = (int32_t)id;
  q = (SEQREF*)csound->QueryGlobalVariable(csound, "sequGlobals");
  if (q == NULL) {
    csound->CreateGlobalVariable(csound, "sequGlobals", 10*sizeof(SEQREF));
    q = (SEQREF*)csound->QueryGlobalVariable(csound, "sequGlobals");
  }
  q[index].owner = owner;
  q[index].max_length = length;
  q[index].klen = klen;
  q[index].seq = seq;
  q[index].cnt = cnt;
  return OK;
}

static int32_t sequ_deinit(CSOUND *csound, void *owner)
{
  int32_t i;
  SEQREF *q = (SEQREF*)csound->QueryGlobalVariable(csound, "sequGlobals");
  if (q != NULL)
    for (i = 0; i < 10; i++)
      if (q[i].owner == owner)
        q[i].owner = NULL;
  return OK;
}

static int32_t sequencer_init(CSOUND *csound, SEQ *p)
{
  int32_t i;
  if (UNLIKELY(p->riff->dimensions != 1 || p->instr->dimensions != 1 ||
               (p->data->dimensions != 1 && p->data->dimensions != 2)))
    return csound->InitError(csound, Str("sequ: invalid array dimensions"));
  p->max_length = p->riff->sizes[0];
  if (p->max_length < 1 || p->max_length != p->instr->sizes[0] ||
      (p->data->dimensions == 2 &&
       (p->data->sizes[0] < 1 || p->max_length != p->data->sizes[1])) ||
      (p->data->dimensions == 1 && p->max_length != p->data->sizes[0]) ||
      p->max_length > 128) {
    return csound->InitError(csound, "%s", Str("sequ: arrays have differing sizes"));
  }
  p->time = 0;
  p->next = 0;
  p->cnt = 1;
  p->direction = 1;            /* forwards */
  for (i = 0; i<p->riff->sizes[0]; i++)
    p->seq[i] = i;
  if (*p->verbos)
    for (i = 0; i < p->max_length; i++)
      printf("%d: %g %g\n", i, p->instr->data[i], p->riff->data[i]);
  return sequ_register(csound, p, *p->id, p->max_length,
                       p->klen, p->seq, &p->cnt);
}

static int32_t sequencer2_init(CSOUND *csound, SEQ2 *p)
{
  int32_t i;
  if (UNLIKELY(p->riff->dimensions != 1 || p->instr->dimensions != 1 ||
               (p->data->dimensions != 1 && p->data->dimensions != 2)))
    return csound->InitError(csound, Str("sequ: invalid array dimensions"));
  p->max_length = p->riff->sizes[0];
  if (p->max_length < 1 || p->max_length != p->instr->sizes[0] ||
      (p->data->dimensions == 2 &&
       (p->data->sizes[0] < 1 || p->max_length != p->data->sizes[1])) ||
      (p->data->dimensions == 1 && p->max_length != p->data->sizes[0]) ||
      p->max_length > 128) {
    return csound->InitError(csound, "%s", Str("sequ: arrays have differing sizes"));
  }
  p->time = 0;
  p->next = 0;
  p->init_flag = 1;
  p->cnt = 1;
  p->direction = 1;            /* forwards */
  for (i = 0; i<p->riff->sizes[0]; i++)
    p->seq[i] = i;
  if (*p->verbos)
    for (i = 0; i < p->max_length; i++)
      printf("%d: %g %g\n", i, p->instr->data[i], p->riff->data[i]);
  return sequ_register(csound, p, *p->id, p->max_length,
                       p->klen, p->seq, &p->cnt);
}

static int32_t sequencer(CSOUND *csound, SEQ *p)
{
  int32_t len = SEQU_LENGTH(*p->klen, p->max_length);
  int32_t i = p->next;
  int32_t mode;
  if (UNLIKELY(!(*p->mode >= FL(-8.0) && (double)*p->mode <= INT32_MAX)))
    return csound->PerfError(csound, &p->h, Str("sequ: invalid mode"));
  mode = (int32_t)*p->mode;

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
    p->direction = 1;
    if (i < 0 || i >= len) { // End of cycle
      i = p->next = 0; p->direction = 1;
    }
  }
  else {
    switch (mode) {
    case -1:
      p->direction = -1;
      if (p->cnt==1 || i < 0 || i >= len) { /* backward and end of loop */
        p->next = i = len-1;
        p->direction = -1;
      }
      break;
    case -2:
      if (i < 0) {
        p->direction = 1;
        i = 0;
      }
      else if (i >= len) {
        p->direction = -1;
        i = len - 1;
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
      if (i < 0) i = 0;
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
      if (i >= len) i = len - 1;
      break;
    case -6:
      p->direction = 1;
      if (i < 0 || i >= len) {
        int32_t j, k = 0;
        for (j =0; j<len; j++) {
          k = rand() % (j + 1);
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
      p->direction = 1;
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
    double duration, samples;
    if (UNLIKELY(!(*p->kbpm > FL(0.0))))
      return csound->PerfError(csound, &p->h, Str("sequ: tempo must be positive"));
    duration = 60.0 / *p->kbpm * p->riff->data[p->seq[i]];
    samples = duration * CS_ESR;
    if (UNLIKELY(!(samples >= 0.0 && samples <= INT32_MAX)))
      return csound->PerfError(csound, &p->h, Str("sequ: invalid step duration"));
    if (inst != 0) {
      char buff[100];
      if (p->data->dimensions==2) {
        int32_t j;
        snprintf(buff, 99, "i %0.2f 0 %g ",
                 inst, duration);
        for (j=0; j< p->data->sizes[0]; j++)
          snprintf(buff+strlen(buff), 99-strlen(buff), "%g ",
                   p->data->data[(j*p->max_length)+p->seq[i]]);
        snprintf(buff+strlen(buff), 99-strlen(buff), "\n");
      }
      else
        snprintf(buff, 100, "i %0.2f 0 %f %f\n",
                inst, duration,
                p->data->data[p->seq[i]]);
      //printf("***Score;ine:%s", buff);
      csound->ReadScore(csound, buff); /* schedule instr for event */
    }
    p->time = (int32_t)samples;
    /* printf("Step %d riff %d instr %0.4f len %f\n", */
    /*    i,p->seq[i], p->instr->data[p->seq[i]], p->riff->data[p->seq[i]]); */
    // Mutate every mode events
    if (mode > 0 && len > 1 && p->cnt%mode == 0) {
      int32_t r1, r2;
      do {
        r1 = rand() % len;
        r2 = rand() % len;
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
    p->next = i + p->direction;
    //if (*p->mode >=0) p->next++;
    //else if (mode == -1) p->next--;
    if (mode != -8) p->cnt++;
  }
  if (*p->verbos)
    printf("Next Step %d time = %d samples\n", p->next, p->time);
  return OK;
}

static int32_t sequencer2(CSOUND *csound, SEQ2 *p)
{
  int32_t len = SEQU_LENGTH(*p->klen, p->max_length);
  int32_t start = *p->kstart > FL(0.0) ?
    (*p->kstart < len ? (int32_t)*p->kstart : len - 1) : 0;
  
  int32_t i = p->next;
  int32_t mode;
  if (UNLIKELY(!(*p->mode >= FL(-8.0) && (double)*p->mode <= INT32_MAX)))
    return csound->PerfError(csound, &p->h, Str("sequ: invalid mode"));
  mode = (int32_t)*p->mode;


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
  if (p->init_flag) {
    i = p->next = start;
    p->init_flag = 0;
  }
  if (mode >= 0) {
    p->direction = 1;
    if (i < start || i >= len) { // End of cycle
      i = p->next = start; p->direction = 1;
    }
  }
  else {
    switch (mode) {
    case -1:
      p->direction = -1;
      if (p->cnt==1 || i < start || i >= len) { /* backward and end of loop */
        p->next = i = len-1;
        p->direction = -1;
      }
      break;
    case -2:
      if (i < start) {
        p->direction = 1;
        i = start;
      }
      else if (i >= len) {
        p->direction = -1;
        i = len - 1;
      }
      break;
    case -3:
      i = start + rand()%(len-start); /* random selection */
      break;
    case -4:
      p->direction = 1;
      if (i>=len) {
        *p->res = -1;
        return OK;
      }
      if (i < start) i = start;
      break;
    case -5:
      p->direction = -1;
      if (p->cnt==1) {
        i = p->next = len-1;
      }
      else if (i<start) {
        *p->res = -1;
        return OK;
      }
      if (i >= len) i = len - 1;
      break;
    case -6:
      p->direction = 1;
      if (i < start || i >= len) {
        int32_t j, k = 0;
        for (j = start; j<len; j++) {
          k = start + rand() % (j - start + 1);
          if (k != j) p->seq[j] = p->seq[k];
          p->seq[k] =  j;
        }
        p->next = i = start;
        p->direction = 1;
      }
      break;
    case -7:
      minus7:
      p->init_flag = 0;
      p->time = 0;
      p->cnt = 1;
      p->direction = 1;
      for (i = 0; i<p->riff->sizes[0]; i++)
        p->seq[i] = i;
      i = p->next = start;
      break;
    case -8:
      *p->res = -1;
      return OK;
      break;
    }
  }
  {
    MYFLT inst = p->instr->data[p->seq[i]];
    double duration, samples;
    if (UNLIKELY(!(*p->kbpm > FL(0.0))))
      return csound->PerfError(csound, &p->h, Str("sequ: tempo must be positive"));
    duration = 60.0 / *p->kbpm * p->riff->data[p->seq[i]];
    samples = duration * CS_ESR;
    if (UNLIKELY(!(samples >= 0.0 && samples <= INT32_MAX)))
      return csound->PerfError(csound, &p->h, Str("sequ: invalid step duration"));
    if (inst != 0) {
      char buff[100];
      if (p->data->dimensions==2) {
        int32_t j;
        snprintf(buff, 99, "i %0.2f 0 %g ",
                 inst, duration);
        for (j=0; j< p->data->sizes[0]; j++)
          snprintf(buff+strlen(buff), 99-strlen(buff), "%g ",
                   p->data->data[(j*p->max_length)+p->seq[i]]);
        snprintf(buff+strlen(buff), 99-strlen(buff), "\n");
      }
      else
        snprintf(buff, 100, "i %0.2f 0 %f %f\n",
                inst, duration,
                p->data->data[p->seq[i]]);
      //printf("***Score;ine:%s", buff);
      csound->ReadScore(csound, buff); /* schedule instr for event */
    }
    p->time = (int32_t)samples;
    /* printf("Step %d riff %d instr %0.4f len %f\n", */
    /*    i,p->seq[i], p->instr->data[p->seq[i]], p->riff->data[p->seq[i]]); */
    // Mutate every mode events
    if (mode > 0 && len - start > 1 && p->cnt%mode == 0) {
      int32_t r1, r2;
      do {
        r1 = start + rand() % (len - start);
        r2 = start + rand() % (len - start);
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
    p->next = i + p->direction;
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
  int32_t id;
  SEQREF *r;
  if (UNLIKELY(!(*p->id >= FL(0.0) && *p->id < FL(10.0))))
    return csound->InitError(csound, Str("sequstate: id out of range"));
  id = (int32_t)*p->id;
  r = (SEQREF*)csound->QueryGlobalVariable(csound, "sequGlobals");
  if (UNLIKELY(r == NULL || r[id].owner == NULL))
    return csound->InitError(csound, Str("sequstate: no active sequence"));
  p->q = &r[id];
  if (UNLIKELY(tabinit(csound, p->riff, p->q->max_length,
                       p->h.insdshead) != OK))
    return csound->InitError(csound, Str("sequstate: cannot allocate output array"));
  return sequState(csound, p);
}

static int32_t sequState(CSOUND *csound, SEQSTATE* p)
{
  SEQREF *q = p->q;
  int32_t i, len;
  if (UNLIKELY(q->owner == NULL))
    return csound->PerfError(csound, &p->h, Str("sequstate: sequence has ended"));
  len = SEQU_LENGTH(*q->klen, q->max_length);
  if (UNLIKELY(p->riff->dimensions != 1 || p->riff->sizes[0] < len))
    return csound->PerfError(csound, &p->h, Str("sequstate: output array is too small"));
  for (i = 0; i < len; i++)
    p->riff->data[i] = q->seq[i];
  *p->res = (MYFLT)*q->cnt;
  return OK;
}

static OENTRY sequencer_localops[] =
  {
   { "sequ", sizeof(SEQ), 0, "k",
     "i[]i[]i[]kkOOOoo",
     (SUBR) sequencer_init, (SUBR) sequencer, (SUBR) sequ_deinit },
      { "sequ", sizeof(SEQ2), 0, "k",
     "ki[]i[]i[]kkOOOoo",
     (SUBR) sequencer2_init, (SUBR) sequencer2, (SUBR) sequ_deinit },

   
   { "sequstate.i", sizeof(SEQSTATE), 0,  "ii[]", "o",
     (SUBR) sequStateInit },
   { "sequstate.k", sizeof(SEQSTATE), 0,  "kk[]", "o",
   (SUBR) sequStateInit, (SUBR) sequState
  }
};


LINKAGE_BUILTIN(sequencer_localops)
