/*
    Copyright (C) 2026 The Csound Developers

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
#ifndef CSOUND_MESSAGE_CALLBACKS_H
#define CSOUND_MESSAGE_CALLBACKS_H

#include "csoundCore.h"
#include <string.h>
#if defined(MSVC)
#include <intrin.h>
#endif

typedef void (*CSOUND_MESSAGE_CALLBACK)(CSOUND *, int32_t, const char *, va_list);
typedef void (*CSOUND_MESSAGE_STRING_CALLBACK)(CSOUND *, int32_t, const char *);

/* Realtime startup and cleanup replace these slots while producers may log.
   Keep publication atomic without taking a lock on the audio thread. */
static inline CSOUND_MESSAGE_CALLBACK csoundLoadMessageCallback(CSOUND *csound)
{
#if defined(MSVC)
  /* Interlocked uses data pointers; copy the function pointer representation. */
  void *value = _InterlockedCompareExchangePointer(
      (void *volatile *) &csound->csoundMessageCallback_, NULL, NULL);
  CSOUND_MESSAGE_CALLBACK callback;
  memcpy(&callback, &value, sizeof(callback));
  return callback;
#elif defined(HAVE_ATOMIC_BUILTIN)
  return __atomic_load_n(&csound->csoundMessageCallback_, __ATOMIC_SEQ_CST);
#else
  return csound->csoundMessageCallback_;
#endif
}

static inline void csoundStoreMessageCallback(CSOUND *csound,
                                              CSOUND_MESSAGE_CALLBACK callback)
{
#if defined(MSVC)
  void *value;
  memcpy(&value, &callback, sizeof(value));
  _InterlockedExchangePointer((void *volatile *) &csound->csoundMessageCallback_,
                             value);
#elif defined(HAVE_ATOMIC_BUILTIN)
  __atomic_store_n(&csound->csoundMessageCallback_, callback, __ATOMIC_SEQ_CST);
#else
  csound->csoundMessageCallback_ = callback;
#endif
}

static inline CSOUND_MESSAGE_STRING_CALLBACK
csoundLoadMessageStringCallback(CSOUND *csound)
{
#if defined(MSVC)
  void *value = _InterlockedCompareExchangePointer(
      (void *volatile *) &csound->csoundMessageStringCallback, NULL, NULL);
  CSOUND_MESSAGE_STRING_CALLBACK callback;
  memcpy(&callback, &value, sizeof(callback));
  return callback;
#elif defined(HAVE_ATOMIC_BUILTIN)
  return __atomic_load_n(&csound->csoundMessageStringCallback, __ATOMIC_SEQ_CST);
#else
  return csound->csoundMessageStringCallback;
#endif
}

static inline void csoundStoreMessageStringCallback(
    CSOUND *csound, CSOUND_MESSAGE_STRING_CALLBACK callback)
{
#if defined(MSVC)
  void *value;
  memcpy(&value, &callback, sizeof(value));
  _InterlockedExchangePointer(
      (void *volatile *) &csound->csoundMessageStringCallback, value);
#elif defined(HAVE_ATOMIC_BUILTIN)
  __atomic_store_n(&csound->csoundMessageStringCallback, callback,
                   __ATOMIC_SEQ_CST);
#else
  csound->csoundMessageStringCallback = callback;
#endif
}

#endif
