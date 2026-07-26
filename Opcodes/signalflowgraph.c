/*
  C compatibility implementation of the signal-flow graph opcodes.

  Copyright (C) 2026 The Csound Developers

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include "pstream.h"
#include <stdio.h>
#include <string.h>

#define SFG_GLOBAL_NAME "sfg_globals"
#define SFG_MAX_ID 256

#ifdef __wasi__
#define SFG_LOCK_READY(lock) 1
#else
#define SFG_LOCK_READY(lock) ((lock) != NULL)
#endif

typedef struct SFG_OUTLET_NODE_ {
  void *instance;
  struct SFG_OUTLET_NODE_ *next;
} SFG_OUTLET_NODE;

typedef struct SFG_CONNECTION_ {
  char source_id[SFG_MAX_ID];
  char sink_id[SFG_MAX_ID];
  struct SFG_CONNECTION_ *next;
} SFG_CONNECTION;

typedef struct SFG_FTABLE_ {
  int32_t pcnt;
  MYFLT *pfields;
  unsigned long string_hash;
  int32_t fno;
  struct SFG_FTABLE_ *next;
} SFG_FTABLE;

typedef struct {
  CSOUND *csound;
  void *ports_lock;
  void *ftables_lock;
  SFG_CONNECTION *connections;
  SFG_OUTLET_NODE *aoutlets;
  SFG_OUTLET_NODE *koutlets;
  SFG_OUTLET_NODE *foutlets;
  SFG_OUTLET_NODE *voutlets;
  SFG_OUTLET_NODE *kidoutlets;
  SFG_FTABLE *ftables;
} SFG_STATE;

typedef struct {
  OPDS h;
  STRINGDAT *name;
  MYFLT *signal;
  char source_id[SFG_MAX_ID];
  SFG_STATE *state;
  SFG_OUTLET_NODE *node;
} SFG_OUTLETA;

typedef struct {
  OPDS h;
  MYFLT *signal;
  STRINGDAT *name;
  char sink_id[SFG_MAX_ID];
  SFG_STATE *state;
  int32_t sample_count;
} SFG_INLETA;

typedef struct {
  OPDS h;
  STRINGDAT *name;
  MYFLT *signal;
  char source_id[SFG_MAX_ID];
  SFG_STATE *state;
  SFG_OUTLET_NODE *node;
} SFG_OUTLETK;

typedef struct {
  OPDS h;
  MYFLT *signal;
  STRINGDAT *name;
  char sink_id[SFG_MAX_ID];
  SFG_STATE *state;
} SFG_INLETK;

typedef struct {
  OPDS h;
  STRINGDAT *name;
  PVSDAT *signal;
  char source_id[SFG_MAX_ID];
  SFG_STATE *state;
  SFG_OUTLET_NODE *node;
} SFG_OUTLETF;

typedef struct {
  OPDS h;
  PVSDAT *signal;
  STRINGDAT *name;
  char sink_id[SFG_MAX_ID];
  SFG_STATE *state;
  int32_t initialized;
} SFG_INLETF;

typedef struct {
  OPDS h;
  STRINGDAT *name;
  ARRAYDAT *signal;
  char source_id[SFG_MAX_ID];
  SFG_STATE *state;
  SFG_OUTLET_NODE *node;
} SFG_OUTLETV;

typedef struct {
  OPDS h;
  ARRAYDAT *signal;
  STRINGDAT *name;
  char sink_id[SFG_MAX_ID];
  SFG_STATE *state;
  size_t value_count;
} SFG_INLETV;

typedef struct {
  OPDS h;
  STRINGDAT *name;
  STRINGDAT *instance_name;
  MYFLT *signal;
  char source_id[SFG_MAX_ID];
  char instance_id[SFG_MAX_ID];
  SFG_STATE *state;
  SFG_OUTLET_NODE *node;
} SFG_OUTLETKID;

typedef struct {
  OPDS h;
  MYFLT *signal;
  STRINGDAT *name;
  STRINGDAT *instance_name;
  char sink_id[SFG_MAX_ID];
  char instance_id[SFG_MAX_ID];
  SFG_STATE *state;
} SFG_INLETKID;

typedef struct {
  OPDS h;
  MYFLT *source;
  STRINGDAT *outlet;
  MYFLT *sink;
  STRINGDAT *inlet;
  MYFLT *gain;
} SFG_CONNECT;

typedef struct {
  OPDS h;
  MYFLT *source;
  STRINGDAT *outlet;
  STRINGDAT *sink;
  STRINGDAT *inlet;
  MYFLT *gain;
} SFG_CONNECT_I;

typedef struct {
  OPDS h;
  STRINGDAT *source;
  STRINGDAT *outlet;
  MYFLT *sink;
  STRINGDAT *inlet;
  MYFLT *gain;
} SFG_CONNECT_II;

typedef struct {
  OPDS h;
  STRINGDAT *source;
  STRINGDAT *outlet;
  STRINGDAT *sink;
  STRINGDAT *inlet;
  MYFLT *gain;
} SFG_CONNECT_S;

typedef struct {
  OPDS h;
  MYFLT *instrument;
  MYFLT *args[VARGMAX];
} SFG_ALWAYSON;

typedef struct {
  OPDS h;
  STRINGDAT *instrument;
  MYFLT *args[VARGMAX];
} SFG_ALWAYSON_S;

typedef struct {
  OPDS h;
  MYFLT *ifno;
  MYFLT *p1;
  MYFLT *p2;
  MYFLT *p3;
  MYFLT *p4;
  MYFLT *p5;
  MYFLT *args[VARGMAX - 5];
} SFG_FTGEN;

typedef struct SFG_NAMEDGEN_ {
  char *name;
  int32_t genum;
  struct SFG_NAMEDGEN_ *next;
} SFG_NAMEDGEN;

static SFG_STATE *sfg_get_state(CSOUND *csound)
{
  SFG_STATE **slot =
      (SFG_STATE **)csound->QueryGlobalVariable(csound, SFG_GLOBAL_NAME);
  return slot != NULL ? *slot : NULL;
}

static int32_t sfg_reset(CSOUND *csound, void *user_data)
{
  SFG_STATE **slot;
  SFG_STATE *state;
  SFG_CONNECTION *connection;
  SFG_OUTLET_NODE *outlet;
  SFG_FTABLE *ftable;
  int32_t i;
  SFG_OUTLET_NODE **lists[5];

  IGN(user_data);
  slot = (SFG_STATE **)csound->QueryGlobalVariable(csound, SFG_GLOBAL_NAME);
  if (slot == NULL || *slot == NULL)
    return OK;
  state = *slot;

  if (state->ports_lock != NULL)
    csound->LockMutex(state->ports_lock);
  while (state->connections != NULL) {
    connection = state->connections;
    state->connections = connection->next;
    csound->Free(csound, connection);
  }
  lists[0] = &state->aoutlets;
  lists[1] = &state->koutlets;
  lists[2] = &state->foutlets;
  lists[3] = &state->voutlets;
  lists[4] = &state->kidoutlets;
  for (i = 0; i < 5; ++i) {
    while (*lists[i] != NULL) {
      outlet = *lists[i];
      *lists[i] = outlet->next;
      csound->Free(csound, outlet);
    }
  }
  if (state->ports_lock != NULL) {
    csound->UnlockMutex(state->ports_lock);
    csound->DestroyMutex(state->ports_lock);
  }

  if (state->ftables_lock != NULL)
    csound->LockMutex(state->ftables_lock);
  while (state->ftables != NULL) {
    ftable = state->ftables;
    state->ftables = ftable->next;
    csound->Free(csound, ftable->pfields);
    csound->Free(csound, ftable);
  }
  if (state->ftables_lock != NULL) {
    csound->UnlockMutex(state->ftables_lock);
    csound->DestroyMutex(state->ftables_lock);
  }

  csound->Free(csound, state);
  *slot = NULL;
  csound->DestroyGlobalVariable(csound, SFG_GLOBAL_NAME);
  return OK;
}

static int32_t sfg_create_state(CSOUND *csound)
{
  SFG_STATE **slot;
  SFG_STATE *state;

  slot = (SFG_STATE **)csound->QueryGlobalVariable(csound, SFG_GLOBAL_NAME);
  if (slot != NULL && *slot != NULL)
    return OK;
  if (slot == NULL &&
      csound->CreateGlobalVariable(csound, SFG_GLOBAL_NAME,
                                   sizeof(SFG_STATE *)) != 0)
    return NOTOK;
  slot = (SFG_STATE **)csound->QueryGlobalVariable(csound, SFG_GLOBAL_NAME);
  if (slot == NULL)
    return NOTOK;
  state = (SFG_STATE *)csound->Calloc(csound, sizeof(SFG_STATE));
  if (state == NULL)
    return NOTOK;
  state->csound = csound;
  state->ports_lock = csound->Create_Mutex(0);
  state->ftables_lock = csound->Create_Mutex(0);
  *slot = state;
  return csound->RegisterResetCallback(csound, state, sfg_reset);
}

static void sfg_make_port_id(CSOUND *csound, OPDS *h, STRINGDAT *name,
                             char *result)
{
  INSTRTXT **instruments = csound->GetInstrumentList(csound);
  const char *instrument_name = NULL;
  int32_t instrument_number = h->insdshead->insno;

  if (instruments != NULL && instruments[instrument_number] != NULL)
    instrument_name = instruments[instrument_number]->insname;
  if (instrument_name != NULL)
    snprintf(result, SFG_MAX_ID, "%s:%s", instrument_name, name->data);
  else
    snprintf(result, SFG_MAX_ID, "%d:%s", instrument_number, name->data);
}

static int32_t sfg_register_outlet(CSOUND *csound, SFG_STATE *state,
                                   SFG_OUTLET_NODE **list, void *instance,
                                   SFG_OUTLET_NODE **node_slot)
{
  SFG_OUTLET_NODE *node;

  if (state == NULL || !SFG_LOCK_READY(state->ports_lock))
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  csound->LockMutex(state->ports_lock);
  node = (SFG_OUTLET_NODE *)csound->Calloc(csound, sizeof(SFG_OUTLET_NODE));
  if (node == NULL) {
    csound->UnlockMutex(state->ports_lock);
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph allocation failed"));
  }
  node->instance = instance;
  node->next = *list;
  *list = node;
  *node_slot = node;
  csound->UnlockMutex(state->ports_lock);
  return OK;
}

static int32_t sfg_remove_outlet(CSOUND *csound, SFG_STATE *state,
                                 SFG_OUTLET_NODE **list, void *instance,
                                 SFG_OUTLET_NODE **node_slot)
{
  SFG_OUTLET_NODE **link;
  SFG_OUTLET_NODE *node;

  if (state == NULL || !SFG_LOCK_READY(state->ports_lock))
    return OK;
  csound->LockMutex(state->ports_lock);
  link = list;
  while (*link != NULL && (*link)->instance != instance)
    link = &(*link)->next;
  if (*link != NULL) {
    node = *link;
    *link = node->next;
    csound->Free(csound, node);
  }
  *node_slot = NULL;
  csound->UnlockMutex(state->ports_lock);
  return OK;
}

static int32_t sfg_is_connected(const SFG_STATE *state, const char *source_id,
                                const char *sink_id)
{
  const SFG_CONNECTION *connection = state->connections;
  while (connection != NULL) {
    if (strcmp(connection->source_id, source_id) == 0 &&
        strcmp(connection->sink_id, sink_id) == 0)
      return 1;
    connection = connection->next;
  }
  return 0;
}

static int32_t sfg_outleta_init(CSOUND *csound, SFG_OUTLETA *p)
{
  p->state = sfg_get_state(csound);
  if (p->state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  sfg_make_port_id(csound, &p->h, p->name, p->source_id);
  return sfg_register_outlet(csound, p->state, &p->state->aoutlets, p,
                             &p->node);
}

static int32_t sfg_outleta_deinit(CSOUND *csound, SFG_OUTLETA *p)
{
  if (p->state == NULL)
    return OK;
  return sfg_remove_outlet(csound, p->state, &p->state->aoutlets, p,
                           &p->node);
}

static int32_t sfg_inleta_init(CSOUND *csound, SFG_INLETA *p)
{
  p->state = sfg_get_state(csound);
  if (p->state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  p->sample_count = p->h.insdshead->ksmps;
  sfg_make_port_id(csound, &p->h, p->name, p->sink_id);
  return OK;
}

static int32_t sfg_inleta_perf(CSOUND *csound, SFG_INLETA *p)
{
  SFG_OUTLET_NODE *node;
  int32_t i;

  memset(p->signal, 0, (size_t)p->sample_count * sizeof(MYFLT));
  csound->LockMutex(p->state->ports_lock);
  for (node = p->state->aoutlets; node != NULL; node = node->next) {
    SFG_OUTLETA *source = (SFG_OUTLETA *)node->instance;
    if (source->h.insdshead->actflg &&
        sfg_is_connected(p->state, source->source_id, p->sink_id)) {
      for (i = 0; i < p->sample_count; ++i)
        p->signal[i] += source->signal[i];
    }
  }
  csound->UnlockMutex(p->state->ports_lock);
  return OK;
}

static int32_t sfg_outletk_init(CSOUND *csound, SFG_OUTLETK *p)
{
  p->state = sfg_get_state(csound);
  if (p->state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  sfg_make_port_id(csound, &p->h, p->name, p->source_id);
  return sfg_register_outlet(csound, p->state, &p->state->koutlets, p,
                             &p->node);
}

static int32_t sfg_outletk_deinit(CSOUND *csound, SFG_OUTLETK *p)
{
  if (p->state == NULL)
    return OK;
  return sfg_remove_outlet(csound, p->state, &p->state->koutlets, p,
                           &p->node);
}

static int32_t sfg_inletk_init(CSOUND *csound, SFG_INLETK *p)
{
  p->state = sfg_get_state(csound);
  if (p->state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  sfg_make_port_id(csound, &p->h, p->name, p->sink_id);
  return OK;
}

static int32_t sfg_inletk_perf(CSOUND *csound, SFG_INLETK *p)
{
  SFG_OUTLET_NODE *node;

  *p->signal = FL(0.0);
  csound->LockMutex(p->state->ports_lock);
  for (node = p->state->koutlets; node != NULL; node = node->next) {
    SFG_OUTLETK *source = (SFG_OUTLETK *)node->instance;
    if (source->h.insdshead->actflg &&
        sfg_is_connected(p->state, source->source_id, p->sink_id))
      *p->signal += *source->signal;
  }
  csound->UnlockMutex(p->state->ports_lock);
  return OK;
}

static int32_t sfg_outletf_init(CSOUND *csound, SFG_OUTLETF *p)
{
  p->state = sfg_get_state(csound);
  if (p->state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  sfg_make_port_id(csound, &p->h, p->name, p->source_id);
  return sfg_register_outlet(csound, p->state, &p->state->foutlets, p,
                             &p->node);
}

static int32_t sfg_outletf_deinit(CSOUND *csound, SFG_OUTLETF *p)
{
  if (p->state == NULL)
    return OK;
  return sfg_remove_outlet(csound, p->state, &p->state->foutlets, p,
                           &p->node);
}

static int32_t sfg_prepare_fsignal(CSOUND *csound, SFG_INLETF *p,
                                   const PVSDAT *source);

static int32_t sfg_inletf_init(CSOUND *csound, SFG_INLETF *p)
{
  SFG_OUTLET_NODE *node;

  p->state = sfg_get_state(csound);
  if (p->state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  p->initialized = 0;
  sfg_make_port_id(csound, &p->h, p->name, p->sink_id);
  csound->LockMutex(p->state->ports_lock);
  for (node = p->state->foutlets; node != NULL; node = node->next) {
    SFG_OUTLETF *source = (SFG_OUTLETF *)node->instance;
    if (source->signal->N > 0 &&
        sfg_is_connected(p->state, source->source_id, p->sink_id)) {
      sfg_prepare_fsignal(csound, p, source->signal);
      break;
    }
  }
  csound->UnlockMutex(p->state->ports_lock);
  return OK;
}

static int32_t sfg_prepare_fsignal(CSOUND *csound, SFG_INLETF *p,
                                   const PVSDAT *source)
{
  size_t byte_count;

  p->signal->N = source->N;
  p->signal->NB = source->NB;
  p->signal->overlap = source->overlap;
  p->signal->winsize = source->winsize;
  p->signal->wintype = source->wintype;
  p->signal->format = source->format;
  p->signal->sliding = source->sliding;
  p->signal->framecount = 0;
  byte_count = source->sliding
                   ? sizeof(MYFLT) * (size_t)p->h.insdshead->ksmps *
                         (size_t)(source->N + 2)
                   : sizeof(float) * (size_t)(source->N + 2);
  if (p->signal->frame.auxp == NULL || p->signal->frame.size < byte_count)
    csound->AuxAlloc(csound, byte_count, &p->signal->frame);
  p->initialized = 1;
  return OK;
}

static int32_t sfg_inletf_perf(CSOUND *csound, SFG_INLETF *p)
{
  SFG_OUTLET_NODE *node;
  int32_t have_frame = 0;

  csound->LockMutex(p->state->ports_lock);
  for (node = p->state->foutlets; node != NULL; node = node->next) {
    SFG_OUTLETF *source = (SFG_OUTLETF *)node->instance;
    PVSDAT *input = source->signal;
    size_t i;
    size_t value_count;
    if (!source->h.insdshead->actflg ||
        !sfg_is_connected(p->state, source->source_id, p->sink_id))
      continue;
    if (!p->initialized)
      sfg_prepare_fsignal(csound, p, input);
    if (p->signal->N != input->N || p->signal->format != input->format ||
        p->signal->sliding != input->sliding)
      continue;
    value_count = input->sliding
                      ? (size_t)p->h.insdshead->ksmps *
                            (size_t)(input->N + 2)
                      : (size_t)(input->N + 2);
    if (!have_frame) {
      memcpy(p->signal->frame.auxp, input->frame.auxp,
             value_count *
                 (input->sliding ? sizeof(MYFLT) : sizeof(float)));
      p->signal->framecount = input->framecount;
      have_frame = 1;
    }
    else if (input->sliding) {
      CMPLX *output_frame = (CMPLX *)p->signal->frame.auxp;
      const CMPLX *input_frame = (const CMPLX *)input->frame.auxp;
      for (i = 0; i < value_count / 2; ++i) {
        if (input_frame[i].re > output_frame[i].re)
          output_frame[i] = input_frame[i];
      }
    }
    else {
      float *output_frame = (float *)p->signal->frame.auxp;
      const float *input_frame = (const float *)input->frame.auxp;
      for (i = 0; i < value_count; i += 2) {
        if (input_frame[i] > output_frame[i]) {
          output_frame[i] = input_frame[i];
          output_frame[i + 1] = input_frame[i + 1];
        }
      }
      if (input->framecount > p->signal->framecount)
        p->signal->framecount = input->framecount;
    }
  }
  csound->UnlockMutex(p->state->ports_lock);
  return OK;
}

static size_t sfg_array_value_count(const ARRAYDAT *array)
{
  size_t count = (size_t)array->arrayMemberSize / sizeof(MYFLT);
  int32_t i;
  for (i = 0; i < array->dimensions; ++i)
    count *= (size_t)array->sizes[i];
  return count;
}

static int32_t sfg_outletv_init(CSOUND *csound, SFG_OUTLETV *p)
{
  p->state = sfg_get_state(csound);
  if (p->state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  sfg_make_port_id(csound, &p->h, p->name, p->source_id);
  return sfg_register_outlet(csound, p->state, &p->state->voutlets, p,
                             &p->node);
}

static int32_t sfg_outletv_deinit(CSOUND *csound, SFG_OUTLETV *p)
{
  if (p->state == NULL)
    return OK;
  return sfg_remove_outlet(csound, p->state, &p->state->voutlets, p,
                           &p->node);
}

static int32_t sfg_inletv_init(CSOUND *csound, SFG_INLETV *p)
{
  p->state = sfg_get_state(csound);
  if (p->state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  p->value_count = sfg_array_value_count(p->signal);
  sfg_make_port_id(csound, &p->h, p->name, p->sink_id);
  return OK;
}

static int32_t sfg_inletv_perf(CSOUND *csound, SFG_INLETV *p)
{
  SFG_OUTLET_NODE *node;
  size_t i;

  memset(p->signal->data, 0, p->value_count * sizeof(MYFLT));
  csound->LockMutex(p->state->ports_lock);
  for (node = p->state->voutlets; node != NULL; node = node->next) {
    SFG_OUTLETV *source = (SFG_OUTLETV *)node->instance;
    if (source->h.insdshead->actflg &&
        sfg_is_connected(p->state, source->source_id, p->sink_id) &&
        sfg_array_value_count(source->signal) == p->value_count) {
      for (i = 0; i < p->value_count; ++i)
        p->signal->data[i] += source->signal->data[i];
    }
  }
  csound->UnlockMutex(p->state->ports_lock);
  return OK;
}

static int32_t sfg_outletkid_init(CSOUND *csound, SFG_OUTLETKID *p)
{
  p->state = sfg_get_state(csound);
  if (p->state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  sfg_make_port_id(csound, &p->h, p->name, p->source_id);
  csound->StringArg2Name(csound, p->instance_id, p->instance_name->data,
                         "", 1);
  return sfg_register_outlet(csound, p->state, &p->state->kidoutlets, p,
                             &p->node);
}

static int32_t sfg_outletkid_deinit(CSOUND *csound, SFG_OUTLETKID *p)
{
  if (p->state == NULL)
    return OK;
  return sfg_remove_outlet(csound, p->state, &p->state->kidoutlets, p,
                           &p->node);
}

static int32_t sfg_inletkid_init(CSOUND *csound, SFG_INLETKID *p)
{
  p->state = sfg_get_state(csound);
  if (p->state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  sfg_make_port_id(csound, &p->h, p->name, p->sink_id);
  csound->StringArg2Name(csound, p->instance_id, p->instance_name->data,
                         "", 1);
  return OK;
}

static int32_t sfg_inletkid_perf(CSOUND *csound, SFG_INLETKID *p)
{
  SFG_OUTLET_NODE *node;

  *p->signal = FL(0.0);
  csound->LockMutex(p->state->ports_lock);
  for (node = p->state->kidoutlets; node != NULL; node = node->next) {
    SFG_OUTLETKID *source = (SFG_OUTLETKID *)node->instance;
    if (source->h.insdshead->actflg &&
        strcmp(source->instance_id, p->instance_id) == 0 &&
        sfg_is_connected(p->state, source->source_id, p->sink_id))
      *p->signal += *source->signal;
  }
  csound->UnlockMutex(p->state->ports_lock);
  return OK;
}

static const char *sfg_numeric_instrument_name(CSOUND *csound, MYFLT *value,
                                               char *buffer)
{
  if (IsStringCode(*value))
    return csound->StringArg2Name(csound, buffer,
                                  csound->GetArgString(csound, *value), "", 1);
  return csound->StringArg2Name(csound, buffer, value, "", 0);
}

static const char *sfg_string_instrument_name(CSOUND *csound,
                                              STRINGDAT *value, char *buffer)
{
  return csound->StringArg2Name(csound, buffer, value->data, "", 1);
}

static int32_t sfg_connect_ids(CSOUND *csound, const char *source_name,
                               STRINGDAT *outlet, const char *sink_name,
                               STRINGDAT *inlet)
{
  SFG_STATE *state = sfg_get_state(csound);
  SFG_CONNECTION *connection;
  char source_id[SFG_MAX_ID];
  char sink_id[SFG_MAX_ID];

  if (state == NULL)
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph is not initialized"));
  snprintf(source_id, sizeof(source_id), "%s:%s", source_name, outlet->data);
  snprintf(sink_id, sizeof(sink_id), "%s:%s", sink_name, inlet->data);
  csound->LockMutex(state->ports_lock);
  for (connection = state->connections; connection != NULL;
       connection = connection->next) {
    if (strcmp(connection->source_id, source_id) == 0 &&
        strcmp(connection->sink_id, sink_id) == 0) {
      csound->UnlockMutex(state->ports_lock);
      return OK;
    }
  }
  connection =
      (SFG_CONNECTION *)csound->Calloc(csound, sizeof(SFG_CONNECTION));
  if (connection == NULL) {
    csound->UnlockMutex(state->ports_lock);
    return csound->InitError(csound, "%s",
                             Str("signal-flow graph allocation failed"));
  }
  snprintf(connection->source_id, sizeof(connection->source_id), "%s",
           source_id);
  snprintf(connection->sink_id, sizeof(connection->sink_id), "%s", sink_id);
  connection->next = state->connections;
  state->connections = connection;
  csound->UnlockMutex(state->ports_lock);
  return OK;
}

static int32_t sfg_connect(CSOUND *csound, SFG_CONNECT *p)
{
  char source[SFG_MAX_ID];
  char sink[SFG_MAX_ID];
  return sfg_connect_ids(
      csound, sfg_numeric_instrument_name(csound, p->source, source),
      p->outlet, sfg_numeric_instrument_name(csound, p->sink, sink),
      p->inlet);
}

static int32_t sfg_connect_i(CSOUND *csound, SFG_CONNECT_I *p)
{
  char source[SFG_MAX_ID];
  char sink[SFG_MAX_ID];
  return sfg_connect_ids(
      csound, sfg_numeric_instrument_name(csound, p->source, source),
      p->outlet, sfg_string_instrument_name(csound, p->sink, sink), p->inlet);
}

static int32_t sfg_connect_ii(CSOUND *csound, SFG_CONNECT_II *p)
{
  char source[SFG_MAX_ID];
  char sink[SFG_MAX_ID];
  return sfg_connect_ids(
      csound, sfg_string_instrument_name(csound, p->source, source),
      p->outlet, sfg_numeric_instrument_name(csound, p->sink, sink), p->inlet);
}

static int32_t sfg_connect_s(CSOUND *csound, SFG_CONNECT_S *p)
{
  char source[SFG_MAX_ID];
  char sink[SFG_MAX_ID];
  return sfg_connect_ids(
      csound, sfg_string_instrument_name(csound, p->source, source),
      p->outlet, sfg_string_instrument_name(csound, p->sink, sink), p->inlet);
}

static int32_t sfg_schedule_alwayson(CSOUND *csound, OPDS *h,
                                     MYFLT instrument,
                                     MYFLT *const *optional_args)
{
  MYFLT event[VARGMAX] = {FL(0.0)};
  int32_t input_count = GetInputArgCnt(h);
  int32_t i;

  if (input_count < 1 || input_count + 2 > VARGMAX)
    return csound->InitError(csound, "%s", Str("alwayson: invalid arguments"));
  event[0] = instrument;
  event[1] = csound->GetScoreOffsetSeconds(csound);
  event[2] = FL(-1.0);
  for (i = 1; i < input_count; ++i) {
    if (optional_args[i - 1] == NULL)
      return csound->InitError(csound, "%s",
                               Str("alwayson: invalid arguments"));
    event[i + 2] = *optional_args[i - 1];
  }
  csound->Event(csound, 0, event, input_count + 2);
  return OK;
}

static int32_t sfg_alwayson(CSOUND *csound, SFG_ALWAYSON *p)
{
  return sfg_schedule_alwayson(csound, &p->h, *p->instrument, p->args);
}

static int32_t sfg_alwayson_s(CSOUND *csound, SFG_ALWAYSON_S *p)
{
  MYFLT instrument =
      (MYFLT)csound->StringArg2Insno(csound, p->instrument->data, 1);
  return sfg_schedule_alwayson(csound, &p->h, instrument, p->args);
}

static unsigned long sfg_hash_string(const unsigned char *text)
{
  unsigned long hash = 5381;
  int32_t c;
  while ((c = *text++) != 0)
    hash = ((hash << 5) + hash) + (unsigned long)c;
  return hash;
}

static int32_t sfg_ftable_matches(const SFG_FTABLE *cached, const EVTBLK *event,
                                  unsigned long string_hash)
{
  int32_t i;
  if (cached->pcnt != event->pcnt ||
      cached->string_hash != string_hash)
    return 0;
  for (i = 0; i <= event->pcnt; ++i) {
    if (cached->pfields[i] != event->p[i])
      return 0;
  }
  return 1;
}

static int32_t sfg_ftgenonce(CSOUND *csound, SFG_FTGEN *p,
                             int32_t named_generator,
                             int32_t string_parameter)
{
  SFG_STATE *state = sfg_get_state(csound);
  SFG_NAMEDGEN *named;
  SFG_FTABLE *cached;
  SFG_FTABLE *entry;
  EVTBLK event;
  FUNC *function = NULL;
  MYFLT *pfields;
  int32_t input_count;
  int32_t i;
  int32_t generator;
  int32_t status;
  unsigned long string_hash = 0;

  if (state == NULL || !SFG_LOCK_READY(state->ftables_lock))
    return csound->InitError(csound, "%s",
                             Str("ftgenonce: state is not initialized"));
  if (p == NULL || p->ifno == NULL || p->p1 == NULL || p->p2 == NULL ||
      p->p3 == NULL || p->p4 == NULL || p->p5 == NULL)
    return csound->InitError(csound, "%s",
                             Str("ftgenonce: missing required argument"));
  input_count = GetInputArgCnt(&p->h);
  if (input_count < 5)
    return csound->InitError(csound, "%s",
                             Str("ftgenonce: expected at least five arguments"));
  pfields =
      (MYFLT *)csound->Calloc(csound,
                              ((size_t)input_count + 1) * sizeof(MYFLT));
  if (pfields == NULL)
    return csound->InitError(csound, "%s",
                             Str("ftgenonce: allocation failed"));
  memset(&event, 0, sizeof(EVTBLK));
  event.opcod = 'f';
  event.pcnt = input_count;
  event.p = pfields;
  event.p[1] = *p->p1;
  event.p[2] = event.p2orig = FL(0.0);
  event.p[3] = event.p3orig = *p->p3;

  if (named_generator) {
    named = (SFG_NAMEDGEN *)csound->GetNamedGens(csound);
    while (named != NULL &&
           strcmp(named->name, ((STRINGDAT *)p->p4)->data) != 0)
      named = named->next;
    if (named == NULL) {
      csound->Free(csound, pfields);
      return csound->InitError(csound, Str("Named gen \"%s\" not defined"),
                               ((STRINGDAT *)p->p4)->data);
    }
    event.p[4] = (MYFLT)named->genum;
  }
  else
    event.p[4] = *p->p4;

  if (string_parameter) {
    generator = (int32_t)event.p[4];
    if (generator < 0)
      generator = -generator;
    if (generator != 1 && generator != 23 && generator != 28 &&
        generator != 43) {
      csound->Free(csound, pfields);
      return csound->InitError(csound, "%s",
                               Str("ftgen string arg not allowed"));
    }
    event.p[5] = SSTRCOD;
    event.strarg = ((STRINGDAT *)p->p5)->data;
    string_hash = sfg_hash_string((const unsigned char *)event.strarg);
  }
  else
    event.p[5] = *p->p5;

  for (i = 5; i < input_count; ++i) {
    if (p->args[i - 5] == NULL) {
      csound->Free(csound, pfields);
      return csound->InitError(csound, "%s",
                               Str("ftgenonce: invalid arguments"));
    }
    event.p[i + 1] = *p->args[i - 5];
  }

  *p->ifno = FL(0.0);
  csound->LockMutex(state->ftables_lock);
  for (cached = state->ftables; cached != NULL; cached = cached->next) {
    if (sfg_ftable_matches(cached, &event, string_hash)) {
      *p->ifno = (MYFLT)cached->fno;
      csound->UnlockMutex(state->ftables_lock);
      csound->Free(csound, pfields);
      return OK;
    }
  }

  status = csound->FTCreate(csound, &function, &event, 1);
  if (status != 0 || function == NULL) {
    csound->UnlockMutex(state->ftables_lock);
    csound->Free(csound, pfields);
    return csound->InitError(csound, "%s", Str("ftgenonce error"));
  }
  entry = (SFG_FTABLE *)csound->Calloc(csound, sizeof(SFG_FTABLE));
  if (entry == NULL) {
    csound->UnlockMutex(state->ftables_lock);
    csound->Free(csound, pfields);
    return csound->InitError(csound, "%s",
                             Str("ftgenonce: allocation failed"));
  }
  entry->pcnt = event.pcnt;
  entry->pfields = pfields;
  entry->string_hash = string_hash;
  entry->fno = function->fno;
  entry->next = state->ftables;
  state->ftables = entry;
  *p->ifno = (MYFLT)function->fno;
  csound->UnlockMutex(state->ftables_lock);
  return OK;
}

static int32_t sfg_ftgenonce_i(CSOUND *csound, SFG_FTGEN *p)
{
  return sfg_ftgenonce(csound, p, 0, 0);
}

static int32_t sfg_ftgenonce_s(CSOUND *csound, SFG_FTGEN *p)
{
  return sfg_ftgenonce(csound, p, 1, 0);
}

static int32_t sfg_ftgenonce_is(CSOUND *csound, SFG_FTGEN *p)
{
  return sfg_ftgenonce(csound, p, 0, 1);
}

static int32_t sfg_ftgenonce_ss(CSOUND *csound, SFG_FTGEN *p)
{
  return sfg_ftgenonce(csound, p, 1, 1);
}

static int32_t sfg_noop(CSOUND *csound, void *p)
{
  IGN(csound);
  IGN(p);
  return OK;
}

static OENTRY sfg_opcodes[] = {
    {"outleta", sizeof(SFG_OUTLETA), _CW, "", "Sa", (SUBR)sfg_outleta_init,
     (SUBR)sfg_noop, (SUBR)sfg_outleta_deinit},
    {"inleta", sizeof(SFG_INLETA), _CR, "a", "S", (SUBR)sfg_inleta_init,
     (SUBR)sfg_inleta_perf, NULL},
    {"outletk", sizeof(SFG_OUTLETK), _CW, "", "Sk", (SUBR)sfg_outletk_init,
     (SUBR)sfg_noop, (SUBR)sfg_outletk_deinit},
    {"inletk", sizeof(SFG_INLETK), _CR, "k", "S", (SUBR)sfg_inletk_init,
     (SUBR)sfg_inletk_perf, NULL},
    {"outletkid", sizeof(SFG_OUTLETKID), _CW, "", "SSk",
     (SUBR)sfg_outletkid_init, (SUBR)sfg_noop,
     (SUBR)sfg_outletkid_deinit},
    {"inletkid", sizeof(SFG_INLETKID), _CR, "k", "SS",
     (SUBR)sfg_inletkid_init, (SUBR)sfg_inletkid_perf, NULL},
    {"outletf", sizeof(SFG_OUTLETF), _CW, "", "Sf", (SUBR)sfg_outletf_init,
     (SUBR)sfg_noop, (SUBR)sfg_outletf_deinit},
    {"inletf", sizeof(SFG_INLETF), _CR, "f", "S", (SUBR)sfg_inletf_init,
     (SUBR)sfg_inletf_perf, NULL},
    {"outletv", sizeof(SFG_OUTLETV), _CW, "", "Sa[]", (SUBR)sfg_outletv_init,
     (SUBR)sfg_noop, (SUBR)sfg_outletv_deinit},
    {"inletv", sizeof(SFG_INLETV), _CR, "a[]", "S", (SUBR)sfg_inletv_init,
     (SUBR)sfg_inletv_perf, NULL},
    {"connect", sizeof(SFG_CONNECT), 0, "", "iSiSp", (SUBR)sfg_connect, NULL,
     NULL},
    {"connect.i", sizeof(SFG_CONNECT_I), 0, "", "iSSSp", (SUBR)sfg_connect_i,
     NULL, NULL},
    {"connect.ii", sizeof(SFG_CONNECT_II), 0, "", "SSiSp",
     (SUBR)sfg_connect_ii, NULL, NULL},
    {"connect.S", sizeof(SFG_CONNECT_S), 0, "", "SSSSp",
     (SUBR)sfg_connect_s, NULL, NULL},
    {"alwayson", sizeof(SFG_ALWAYSON), 0, "", "im", (SUBR)sfg_alwayson, NULL,
     NULL},
    {"alwayson.S", sizeof(SFG_ALWAYSON_S), 0, "", "Sm",
     (SUBR)sfg_alwayson_s, NULL, NULL},
    {"ftgenonce", sizeof(SFG_FTGEN), TW, "i", "iiiiim",
     (SUBR)sfg_ftgenonce_i, NULL, NULL},
    {"ftgenonce.S", sizeof(SFG_FTGEN), TW, "i", "iiiSim",
     (SUBR)sfg_ftgenonce_s, NULL, NULL},
    {"ftgenonce.iS", sizeof(SFG_FTGEN), TW, "i", "iiiiSm",
     (SUBR)sfg_ftgenonce_is, NULL, NULL},
    {"ftgenonce.SS", sizeof(SFG_FTGEN), TW, "i", "iiiSSm",
     (SUBR)sfg_ftgenonce_ss, NULL, NULL},
};

PUBLIC int32_t csoundModuleInit_signalflowgraph(CSOUND *csound)
{
  int32_t status = sfg_create_state(csound);
  if (status != OK)
    return status;
  return csound->AppendOpcodes(
      csound, sfg_opcodes,
      (int32_t)(sizeof(sfg_opcodes) / sizeof(sfg_opcodes[0])));
}

#ifdef BUILD_PLUGINS
PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  return sfg_create_state(csound);
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
  return csoundModuleInit_signalflowgraph(csound);
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound)
{
  return sfg_reset(csound, NULL);
}
#endif
