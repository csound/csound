/*
    threads.c:

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

#if defined(__linux) || defined(__linux__)
/* for pthread_mutex_timedlock() */
#define _XOPEN_SOURCE 600
#endif

#include "csoundCore.h"
#include "csGblMtx.h"

#if defined(WIN32)

#include <windows.h>
#include <process.h>

typedef struct {
    uintptr_t   (*func)(void *);
    void        *userdata;
    HANDLE      threadLock;
} threadParams;

static unsigned int __stdcall threadRoutineWrapper(void *p)
{
    uintptr_t (*threadRoutine)(void *);
    void      *userData;

    threadRoutine = ((threadParams*) p)->func;
    userData = ((threadParams*) p)->userdata;
    SetEvent(((threadParams*) p)->threadLock);
    return (unsigned int) threadRoutine(userData);
}

PUBLIC void *csoundCreateThread(uintptr_t (*threadRoutine)(void *),
                                void *userdata)
{
    threadParams  p;
    void          *h;
    unsigned int  threadID;

    p.func = threadRoutine;
    p.userdata = userdata;
    p.threadLock = CreateEvent(0, 0, 0, 0);
    if (p.threadLock == (HANDLE) 0)
      return NULL;
    h = (void*) _beginthreadex(NULL, (unsigned) 0, threadRoutineWrapper,
                               (void*) &p, (unsigned) 0, &threadID);
    if (h != NULL)
      WaitForSingleObject(p.threadLock, INFINITE);
    CloseHandle(p.threadLock);
    return h;
}

PUBLIC uintptr_t csoundJoinThread(void *thread)
{
    DWORD   retval = (DWORD) 0;
    WaitForSingleObject((HANDLE) thread, INFINITE);
    GetExitCodeThread((HANDLE) thread, &retval);
    CloseHandle((HANDLE) thread);
    return (uintptr_t) retval;
}

PUBLIC void *csoundCreateThreadLock(void)
{
    HANDLE threadLock = CreateEvent(0, 0, TRUE, 0);
    return (void*) threadLock;
}

PUBLIC int csoundWaitThreadLock(void *lock, size_t milliseconds)
{
    return (int) WaitForSingleObject((HANDLE) lock, milliseconds);
}

PUBLIC void csoundWaitThreadLockNoTimeout(void *lock)
{
    WaitForSingleObject((HANDLE) lock, INFINITE);
}

PUBLIC void csoundNotifyThreadLock(void *lock)
{
    SetEvent((HANDLE) lock);
}

PUBLIC void csoundDestroyThreadLock(void *lock)
{
    CloseHandle((HANDLE) lock);
}

PUBLIC void csoundSleep(size_t milliseconds)
{
    Sleep((DWORD) milliseconds);
}

#elif defined(LINUX) || defined(__MACH__)

#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

PUBLIC void *csoundCreateThread(uintptr_t (*threadRoutine)(void *),
                                void *userdata)
{
    pthread_t pthread = (pthread_t) 0;
    if (!pthread_create(&pthread, (pthread_attr_t*) NULL,
                        (void *(*)(void *)) threadRoutine, userdata)) {
      return (void*) pthread;
    }
    return NULL;
}

PUBLIC uintptr_t csoundJoinThread(void *thread)
{
    void      *threadRoutineReturnValue = NULL;
    int       pthreadReturnValue;

    pthreadReturnValue = pthread_join((pthread_t) thread,
                                      &threadRoutineReturnValue);
    if (pthreadReturnValue)
      return (uintptr_t) ((intptr_t) pthreadReturnValue);
    return (uintptr_t) threadRoutineReturnValue;
}

#ifdef LINUX

PUBLIC void *csoundCreateThreadLock(void)
{
    pthread_mutex_t *pthread_mutex;

    pthread_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (pthread_mutex == NULL)
      return NULL;
    if (pthread_mutex_init(pthread_mutex, NULL) != 0) {
      free(pthread_mutex);
      return NULL;
    }
    return (void*) pthread_mutex;
}

PUBLIC int csoundWaitThreadLock(void *lock, size_t milliseconds)
{
    {
      register int retval = pthread_mutex_trylock((pthread_mutex_t*) lock);
      if (!retval)
        return retval;
      if (!milliseconds)
        return retval;
    }
    {
      struct timeval  tv;
      struct timespec ts;
      register size_t n, s;
      gettimeofday(&tv, NULL);
      s = milliseconds / (size_t) 1000;
      n = milliseconds - (s * (size_t) 1000);
      s += (size_t) tv.tv_sec;
      n = (size_t) (((int) n * 1000 + (int) tv.tv_usec) * 1000);
      ts.tv_nsec = (long) (n < (size_t) 1000000000 ? n : n - 1000000000);
      ts.tv_sec = (time_t) (n < (size_t) 1000000000 ? s : s + 1);
      return pthread_mutex_timedlock((pthread_mutex_t*) lock, &ts);
    }
}

PUBLIC void csoundWaitThreadLockNoTimeout(void *lock)
{
    pthread_mutex_lock((pthread_mutex_t*) lock);
}

PUBLIC void csoundNotifyThreadLock(void *lock)
{
    pthread_mutex_unlock((pthread_mutex_t*) lock);
}

PUBLIC void csoundDestroyThreadLock(void *lock)
{
    pthread_mutex_destroy((pthread_mutex_t*) lock);
    free(lock);
}

#else   /* LINUX */

typedef struct CsoundThreadLock_s {
    pthread_mutex_t m;
    pthread_cond_t  c;
    unsigned char   s;
} CsoundThreadLock_t;

PUBLIC void *csoundCreateThreadLock(void)
{
    CsoundThreadLock_t  *p;

    p = (CsoundThreadLock_t*) malloc(sizeof(CsoundThreadLock_t));
    if (p == NULL)
      return NULL;
    memset(p, 0, sizeof(CsoundThreadLock_t));
    if (pthread_mutex_init(&(p->m), (pthread_mutexattr_t*) NULL) != 0) {
      free((void*) p);
      return NULL;
    }
    if (pthread_cond_init(&(p->c), (pthread_condattr_t*) NULL) != 0) {
      pthread_mutex_destroy(&(p->m));
      free((void*) p);
      return NULL;
    }
    p->s = (unsigned char) 1;

    return (void*) p;
}

PUBLIC int csoundWaitThreadLock(void *threadLock, size_t milliseconds)
{
    CsoundThreadLock_t  *p;
    int                 retval = 0;

    p = (CsoundThreadLock_t*) threadLock;
    pthread_mutex_lock(&(p->m));
    if (!p->s) {
      if (milliseconds) {
        struct timeval  tv;
        struct timespec ts;
        register size_t n, s;
        gettimeofday(&tv, NULL);
        s = milliseconds / (size_t) 1000;
        n = milliseconds - (s * (size_t) 1000);
        s += (size_t) tv.tv_sec;
        n = (size_t) (((int) n * 1000 + (int) tv.tv_usec) * 1000);
        ts.tv_nsec = (long) (n < (size_t) 1000000000 ? n : n - 1000000000);
        ts.tv_sec = (time_t) (n < (size_t) 1000000000 ? s : s + 1);
        do {
          retval = pthread_cond_timedwait(&(p->c), &(p->m), &ts);
        } while (!p->s && !retval);
      }
      else
        retval = ETIMEDOUT;
    }
    p->s = (unsigned char) 0;
    pthread_mutex_unlock(&(p->m));

    return retval;
}

PUBLIC void csoundWaitThreadLockNoTimeout(void *threadLock)
{
    CsoundThreadLock_t  *p;

    p = (CsoundThreadLock_t*) threadLock;
    pthread_mutex_lock(&(p->m));
    while (!p->s) {
      pthread_cond_wait(&(p->c), &(p->m));
    }
    p->s = (unsigned char) 0;
    pthread_mutex_unlock(&(p->m));
}

PUBLIC void csoundNotifyThreadLock(void *threadLock)
{
    CsoundThreadLock_t  *p;

    p = (CsoundThreadLock_t*) threadLock;
    pthread_mutex_lock(&(p->m));
    p->s = (unsigned char) 1;
    pthread_cond_signal(&(p->c));
    pthread_mutex_unlock(&(p->m));
}

PUBLIC void csoundDestroyThreadLock(void *threadLock)
{
    CsoundThreadLock_t  *p;

    if (threadLock == NULL)
      return;
    csoundNotifyThreadLock(threadLock);
    p = (CsoundThreadLock_t*) threadLock;
    pthread_cond_destroy(&(p->c));
    pthread_mutex_destroy(&(p->m));
    free(threadLock);
}

#endif  /* !LINUX */

PUBLIC void csoundSleep(size_t milliseconds)
{
    struct timespec ts;
    register size_t n, s;

    s = milliseconds / (size_t) 1000;
    n = milliseconds - (s * (size_t) 1000);
    n = (size_t) ((int) n * 1000000);
    ts.tv_sec = (time_t) s;
    ts.tv_nsec = (long) n;
    while (nanosleep(&ts, &ts) != 0)
      ;
}

#else

static CS_NOINLINE void notImplementedWarning_(const char *name)
{
    fprintf(stderr, Str("%s() is not implemented on this platform.\n"), name);
}

PUBLIC void *csoundCreateThread(uintptr_t (*threadRoutine)(void *),
                                void *userdata)
{
    notImplementedWarning_("csoundCreateThread");
    return NULL;
}

PUBLIC uintptr_t csoundJoinThread(void *thread)
{
    notImplementedWarning_("csoundJoinThread");
    return (uintptr_t) 0;
}

PUBLIC void *csoundCreateThreadLock(void)
{
    notImplementedWarning_("csoundCreateThreadLock");
    return NULL;
}

PUBLIC int csoundWaitThreadLock(void *lock, size_t milliseconds)
{
    notImplementedWarning_("csoundWaitThreadLock");
    return 0;
}

PUBLIC void csoundWaitThreadLockNoTimeout(void *lock)
{
    notImplementedWarning_("csoundWaitThreadLockNoTimeout");
}

PUBLIC void csoundNotifyThreadLock(void *lock)
{
    notImplementedWarning_("csoundNotifyThreadLock");
}

PUBLIC void csoundDestroyThreadLock(void *lock)
{
    notImplementedWarning_("csoundDestroyThreadLock");
}

PUBLIC void csoundSleep(size_t milliseconds)
{
    notImplementedWarning_("csoundSleep");
}

#endif

/* internal functions for csound.c */

void csoundLock(void)
{
    csound_global_mutex_lock();
}

void csoundUnLock(void)
{
    csound_global_mutex_unlock();
}

