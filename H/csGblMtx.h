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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef CSOUND_CSGBLMTX_H
#define CSOUND_CSGBLMTX_H

#if defined(__linux) || defined(__linux__) || defined(__unix) ||    \
    defined(__unix__) || defined(__MACOSX__) || defined(__APPLE__)

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

static  pthread_mutex_t     csound_global_lock_ = PTHREAD_MUTEX_INITIALIZER;

static
#ifdef __GNUC__
#  ifndef __STRICT_ANSI__
  inline
#  else
  __inline__
#  endif
#endif
    void csound_global_mutex_lock(void)
{
    pthread_mutex_lock(&csound_global_lock_);
}

static
#ifdef __GNUC__
#  ifndef __STRICT_ANSI__
  inline
#  else
  __inline__
#  endif
#endif
    void csound_global_mutex_unlock(void)
{
    pthread_mutex_unlock(&csound_global_lock_);
}

#ifdef __cplusplus
}       /* extern "C" */
#endif

#elif defined(_WIN32) || defined(__WIN32__)

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

static  CRITICAL_SECTION    csound_global_lock_;

#ifdef __GNUC__

static __attribute__ ((__constructor__)) void csound_global_mutex_init_(void)
{
    InitializeCriticalSection(&csound_global_lock_);
}

static __attribute__ ((__destructor__)) void csound_global_mutex_destroy_(void)
{
    DeleteCriticalSection(&csound_global_lock_);
}

#else

static  int     csound_global_lock_init_done_ = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

#endif

static
#ifdef __GNUC__
#  ifndef __STRICT_ANSI__
  inline
#  else
  __inline__
#  endif
#elif defined(_MSC_VER)
  __inline
#endif
    void csound_global_mutex_lock(void)
{
#ifndef __GNUC__
    if (csound_global_lock_init_done_)
#endif
      EnterCriticalSection(&csound_global_lock_);
}

static
#ifdef __GNUC__
#  ifndef __STRICT_ANSI__
  inline
#  else
  __inline__
#  endif
#elif defined(_MSC_VER)
  __inline
#endif
    void csound_global_mutex_unlock(void)
{
#ifndef __GNUC__
    if (csound_global_lock_init_done_)
#endif
      LeaveCriticalSection(&csound_global_lock_);
}

#ifdef __cplusplus
}       /* extern "C" */
#endif

#else

#ifdef __GNUC__
#  warning "global thread locks not supported on this platform"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static
#ifdef __GNUC__
#  ifndef __STRICT_ANSI__
  inline
#  else
  __inline__
#  endif
#elif defined(_MSC_VER)
  __inline
#endif
    void csound_global_mutex_lock(void)
{
}

static
#ifdef __GNUC__
#  ifndef __STRICT_ANSI__
  inline
#  else
  __inline__
#  endif
#elif defined(_MSC_VER)
  __inline
#endif
    void csound_global_mutex_unlock(void)
{
}

#ifdef __cplusplus
}       /* extern "C" */
#endif

#endif

#endif      /* CSOUND_CSGBLMTX_H */

