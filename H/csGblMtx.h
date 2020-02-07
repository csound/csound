/*
    csGblMtx.h:

    Copyright (C) 2005 Istvan Varga

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
#ifndef CSOUND_CSGBLMTX_H


#ifdef HAVE_PTHREAD
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

static pthread_mutex_t csound_global_lock_ = PTHREAD_MUTEX_INITIALIZER;

void csoundLock() {
  pthread_mutex_lock(&csound_global_lock_);
}

void csoundUnLock() {
  pthread_mutex_unlock(&csound_global_lock_);
}


#ifdef __cplusplus
}
#endif

#elif defined(_WIN32) || defined (__WIN32__)
#define _WIN32_WINNT 0x0600
#include <windows.h>

#ifdef __cplusplus
extern "C" {
  #endif

static INIT_ONCE g_InitOnce = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION* csound_global_lock;

static BOOL CALLBACK InitHandleFunction ( PINIT_ONCE InitOnce, PVOID Parameter,
    PVOID *lpContext) {

    CRITICAL_SECTION* cs = (CRITICAL_SECTION*) malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(cs);
    *lpContext = cs;
    return 1;
}



void csoundLock() {
    BOOL status;
    CRITICAL_SECTION* cs;

    status = InitOnceExecuteOnce(&g_InitOnce, InitHandleFunction, NULL, &cs);
    if (status) {
      EnterCriticalSection(cs);
    }
}

void csoundUnLock() {

    BOOL status;
    CRITICAL_SECTION* cs;

    status = InitOnceExecuteOnce(&g_InitOnce, InitHandleFunction, NULL, &cs);
    if (status) {
      LeaveCriticalSection(cs);
    }
}


#ifdef __cplusplus
}
#endif

#else /* END WIN32 */
#ifdef __cplusplus
extern "C" {
#endif

void csoundLock() {
}

void csoundUnLock() {
}

#ifdef __cplusplus
}
#endif

#endif


#endif      /* CSOUND_CSGBLMTX_H */
