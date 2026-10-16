/*
 * Browser realtime control requests.
 * Copyright (C) 2026 The Csound Developers
 * Licensed under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "csoundCore.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define WASI_API_SLOTS 4U
#define WASI_API_TEXT_CAPACITY (1024U * 1024U)
#define WASI_API_CANCELLED (-2)

enum {
  WASI_API_COMPILE = 1,
  WASI_API_EVAL,
  WASI_API_GET_STRING,
  WASI_API_SET_STRING,
  WASI_API_GET_CONTROL,
  WASI_API_SET_CONTROL
};

__attribute__((used))
uint32_t csoundRealtimeApiCapacityWasi(void)
{
  return WASI_API_TEXT_CAPACITY;
}

#ifdef CSOUND_WASI_REALTIME

typedef struct {
  int32_t operation;
  uint32_t id;
  uint32_t generation;
  cs_float number;
  char text[WASI_API_TEXT_CAPACITY];
} WASI_API_REQUEST;

typedef struct {
  CSOUND *csound;
  void *thread;
  uint32_t write;
  uint32_t read;
  uint32_t stopping;
  int32_t wakeup;
  WASI_API_REQUEST requests[WASI_API_SLOTS];
} WASI_API_QUEUE;

/* Copies the string before returning; the worker retains all native storage.
   A dedicated worker posts this directly to the UI, so completion does not
   depend on the render worker's JavaScript event loop. */
void csoundWasiApiComplete(uint32_t id, uint32_t generation, int32_t status,
                          cs_float number, const char *text)
    __attribute__((__import_module__("env"),
                   __import_name__("csoundWasiApiComplete")));

char *csoundGetStringChannelWasi(CSOUND *csound, const char *name);

static int32_t execute_request(CSOUND *csound, WASI_API_REQUEST *request,
                               cs_float *number, char **text)
{
  int32_t result = CSOUND_SUCCESS;
  void *value;
  /* Compiler state and channel creation belong to init. This mutex is used
     only by background threads; existing voices keep rendering. Long global
     init code can still delay the initialisation of newly scheduled notes. */
  csoundLockMutex(csound->init_pass_threadlock);
  switch (request->operation) {
  case WASI_API_COMPILE:
    *number = csoundCompileOrc(csound, request->text, 0);
    break;
  case WASI_API_EVAL:
    /* Copy retval before the next compile can replace instrument zero. */
    *number = csoundEvalCode(csound, request->text);
    break;
  case WASI_API_GET_STRING:
    if (csoundGetChannelPtr(csound, &value, request->text,
                           CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL)) {
      result = CSOUND_ERROR;
      break;
    }
    *text = csoundGetStringChannelWasi(csound, request->text);
    if (*text == NULL)
      result = CSOUND_MEMORY;
    break;
  case WASI_API_SET_STRING:
    if (csoundGetChannelPtr(csound, &value, request->text,
                           CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL)) {
      result = CSOUND_ERROR;
      break;
    }
    csoundSetStringChannel(csound, request->text,
                          request->text + strlen(request->text) + 1);
    break;
  case WASI_API_GET_CONTROL:
    *number = csoundGetControlChannel(csound, request->text, &result);
    break;
  case WASI_API_SET_CONTROL:
    if (csoundGetChannelPtr(csound, &value, request->text,
                           CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL)) {
      result = CSOUND_ERROR;
      break;
    }
    csoundSetControlChannel(csound, request->text, request->number);
    break;
  default:
    result = CSOUND_ERROR;
    break;
  }
  csoundUnlockMutex(csound->init_pass_threadlock);
  return result;
}

static uintptr_t control_thread(void *userdata)
{
  WASI_API_QUEUE *queue = userdata;
  uint32_t read = 0;
  for (;;) {
    int32_t wakeup = __atomic_load_n(&queue->wakeup, __ATOMIC_ACQUIRE);
    uint32_t stopping = __atomic_load_n(&queue->stopping, __ATOMIC_ACQUIRE);
    uint32_t write = __atomic_load_n(&queue->write, __ATOMIC_ACQUIRE);
    if (read == write) {
      /* Read write after the stop flag: seeing stop must also see every
         request published before it, so none lose their cancellation reply. */
      if (stopping)
        break;
      /* Taking the wake counter before checking the queue prevents a submit
         or stop between the check and wait from becoming a lost wakeup. */
      __builtin_wasm_memory_atomic_wait32(&queue->wakeup, wakeup, -1);
      continue;
    }
    WASI_API_REQUEST *request = &queue->requests[read % WASI_API_SLOTS];
    cs_float number = 0;
    char *text = NULL;
    int32_t status = WASI_API_CANCELLED;
    if (!__atomic_load_n(&queue->stopping, __ATOMIC_ACQUIRE))
      status = execute_request(queue->csound, request, &number, &text);
    csoundWasiApiComplete(request->id, request->generation, status, number, text);
    free(text);
    /* Release only after the callback has copied the result and identifiers. */
    __atomic_store_n(&queue->read, ++read, __ATOMIC_RELEASE);
  }
  return 0;
}

__attribute__((used))
int32_t csoundRealtimeApiStartWasi(CSOUND *csound)
{
  if (csound == NULL || !csound->oparms->realtime ||
      !(csound->engineStatus & CS_STATE_COMP) ||
      csound->init_pass_threadlock == NULL)
    return CSOUND_ERROR;
  if (csound->wasi_realtime_api != NULL)
    return CSOUND_SUCCESS;
  /* Allocate before rendering. Submission copies into these fixed slots and
     never enters malloc or a mutex on the render worker. */
  WASI_API_QUEUE *queue = calloc(1, sizeof(*queue));
  if (queue == NULL)
    return CSOUND_MEMORY;
  queue->csound = csound;
  queue->thread = csoundCreateThread(control_thread, queue);
  if (queue->thread == NULL) {
    free(queue);
    return CSOUND_ERROR;
  }
  csound->wasi_realtime_api = queue;
  return CSOUND_SUCCESS;
}

static WASI_API_REQUEST *write_slot(WASI_API_QUEUE *queue)
{
  if (queue == NULL || __atomic_load_n(&queue->stopping, __ATOMIC_ACQUIRE))
    return NULL;
  uint32_t write = __atomic_load_n(&queue->write, __ATOMIC_RELAXED);
  uint32_t read = __atomic_load_n(&queue->read, __ATOMIC_ACQUIRE);
  if (write - read >= WASI_API_SLOTS)
    return NULL;
  return &queue->requests[write % WASI_API_SLOTS];
}

__attribute__((used))
char *csoundRealtimeApiBufferWasi(CSOUND *csound)
{
  WASI_API_REQUEST *request = csound == NULL ? NULL :
      write_slot(csound->wasi_realtime_api);
  /* A peek, not a reservation. The single JS producer fills and submits in
     one synchronous call. Invalid requests do not consume a queue slot. */
  return request == NULL ? NULL : request->text;
}

__attribute__((used))
int32_t csoundRealtimeApiSubmitWasi(CSOUND *csound, int32_t operation,
                                   uint32_t id, uint32_t generation,
                                   uint32_t textBytes, cs_float number)
{
  WASI_API_QUEUE *queue = csound == NULL ? NULL : csound->wasi_realtime_api;
  WASI_API_REQUEST *request = write_slot(queue);
  if (request == NULL || operation < WASI_API_COMPILE ||
      operation > WASI_API_SET_CONTROL || textBytes == 0 ||
      textBytes > WASI_API_TEXT_CAPACITY ||
      request->text[textBytes - 1] != '\0')
    return CSOUND_ERROR;
  const char *end = memchr(request->text, '\0', textBytes);
  if (end == NULL)
    return CSOUND_ERROR;
  if (operation >= WASI_API_GET_STRING && end == request->text)
    return CSOUND_ERROR;
  if (operation == WASI_API_SET_STRING) {
    size_t nameBytes = (size_t) (end - request->text) + 1;
    if (nameBytes >= textBytes ||
        memchr(request->text + nameBytes, '\0', textBytes - nameBytes) !=
          request->text + textBytes - 1)
      return CSOUND_ERROR;
  } else if (end != request->text + textBytes - 1) {
    return CSOUND_ERROR;
  }
  if (operation == WASI_API_SET_CONTROL && !isfinite(number))
    return CSOUND_ERROR;
  request->operation = operation;
  request->id = id;
  request->generation = generation;
  request->number = number;
  uint32_t write = __atomic_load_n(&queue->write, __ATOMIC_RELAXED);
  __atomic_store_n(&queue->write, write + 1, __ATOMIC_RELEASE);
  __atomic_fetch_add(&queue->wakeup, 1, __ATOMIC_RELEASE);
  __builtin_wasm_memory_atomic_notify(&queue->wakeup, 1);
  return CSOUND_SUCCESS;
}

__attribute__((used))
void csoundRealtimeApiStopWasi(CSOUND *csound)
{
  WASI_API_QUEUE *queue = csound == NULL ? NULL : csound->wasi_realtime_api;
  if (queue == NULL)
    return;
  /* Stop/reset share the single producer thread. The running request owns
     its slot until completion; undispatched requests receive cancellation. */
  __atomic_store_n(&queue->stopping, 1, __ATOMIC_RELEASE);
  __atomic_fetch_add(&queue->wakeup, 1, __ATOMIC_RELEASE);
  __builtin_wasm_memory_atomic_notify(&queue->wakeup, 1);
  csoundJoinThread(queue->thread);
  csound->wasi_realtime_api = NULL;
  free(queue);
}

#else

int32_t csoundRealtimeApiStartWasi(CSOUND *csound)
{
  (void) csound;
  return CSOUND_ERROR;
}

char *csoundRealtimeApiBufferWasi(CSOUND *csound)
{
  (void) csound;
  return NULL;
}

int32_t csoundRealtimeApiSubmitWasi(CSOUND *csound, int32_t operation,
                                   uint32_t id, uint32_t generation,
                                   uint32_t textBytes, cs_float number)
{
  (void) csound;
  (void) operation;
  (void) id;
  (void) generation;
  (void) textBytes;
  (void) number;
  return CSOUND_ERROR;
}

void csoundRealtimeApiStopWasi(CSOUND *csound)
{
  (void) csound;
}

#endif
