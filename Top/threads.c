/*
  threads.c:

  Copyright (C) 2007 The Csound #project

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

#if defined(__linux) || defined(__linux__)
/* for pthread_mutex_timedlock() */
#define _XOPEN_SOURCE 600
#endif

#ifndef HAVE_GETTIMEOFDAY
#if defined(LINUX)    || defined(__unix)   || defined(__unix__) || \
    defined(__MACH__) || defined(__HAIKU__)
#define HAVE_GETTIMEOFDAY 1
#endif
#endif

#include "csoundCore.h"
#include <errno.h>

#if 0
static CS_NOINLINE void notImplementedWarning_(const char *name)
{
#ifndef __EMSCRIPTEN__
  fprintf(stderr, Str("%s() is not implemented on this platform.\n"), name);
#endif
}
#endif

#if defined(HAVE_PTHREAD)

#if defined(WIN32)
#include <windows.h>
#include <process.h>

void gettimeofday_(struct timeval* p, void* tz /* IGNORED */)
   {
          union {
             long long ns100; /*time since 1 Jan 1601 in 100ns units */
                 FILETIME ft;
          } now;

      GetSystemTimeAsFileTime( &(now.ft) );
      p->tv_usec=(long)((now.ns100 / 10LL) % 1000000LL );
      p->tv_sec= (long)((now.ns100-(116444736000000000LL))/10000000LL);
}

/**
 * Runs an external command with the arguments specified in 'argv'.
 * argv[0] is the name of the program to execute (if not a full path
 * file name, it is searched in the directories defined by the PATH
 * environment variable). The list of arguments should be terminated
 * by a NULL pointer.
 * If 'noWait' is zero, the function waits until the external program
 * finishes, otherwise it returns immediately. In the first case, a
 * non-negative return value is the exit status of the command (0 to
 * 255), otherwise it is the PID of the newly created process.
 * On error, a negative value is returned.
 */

 long csoundRunCommand(const char * const *argv, int32_t noWait)
{
    long    retval;

    if (argv == NULL || argv[0] == NULL)
      return -1L;
    retval = (long) _spawnvp((noWait ? (int32_t) _P_NOWAIT : (int32_t) _P_WAIT),
                             argv[0], argv);
    if (!noWait && retval >= 0L)
      retval &= 255L;
    return retval;
}

 void csoundSleep(size_t milliseconds)
{
    Sleep((DWORD) milliseconds);
}

#else

#include <sys/wait.h>
/**
 * Runs an external command with the arguments specified in 'argv'.
 * argv[0] is the name of the program to execute (if not a full path
 * file name, it is searched in the directories defined by the PATH
 * environment variable). The list of arguments should be terminated
 * by a NULL pointer.
 * If 'noWait' is zero, the function waits until the external program
 * finishes, otherwise it returns immediately. In the first case, a
 * non-negative return value is the exit status of the command (0 to
 * 255), otherwise it is the PID of the newly created process.
 * On error, a negative value is returned.
 */

 long csoundRunCommand(const char * const *argv, int32_t noWait)
{
    long    retval;

    if (argv == NULL || argv[0] == NULL)
      return -1L;
    retval = (long) fork();
    if (retval == 0L) {
      /* child process */
      execvp(argv[0], (char**) argv);
      /* Do not run the host's exit handlers or flush its copied streams. */
      _exit(255);
    }
    else if (retval > 0L && noWait == 0) {
      int status;
      for (;;) {
        if (waitpid((pid_t) retval, &status, 0) < 0) {
          if (errno == EINTR)
            continue;
          return -1L;
        }
        if (WIFEXITED(status) != 0) {
          retval = (long) (WEXITSTATUS(status)) & 255L;
          return retval;
        }
        if (WIFSIGNALED(status) != 0) {
          retval = 255L;
          return retval;
        }
      }
    }
    return retval;
}

 void csoundSleep(size_t milliseconds)
{
    struct timespec ts;
    register size_t n, s;

    s = milliseconds / (size_t) 1000;
    n = milliseconds - (s * (size_t) 1000);
    n = (size_t) ((int32_t) n * 1000000);
    ts.tv_sec = (time_t) s;
    ts.tv_nsec = (long) n;
    while (nanosleep(&ts, &ts) != 0)
      ;
}

#endif

#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#define BARRIER_SERIAL_THREAD (-1)

#if !defined(HAVE_PTHREAD_BARRIER_INIT)
#if !defined( __MACH__)&&!defined(__HAIKU__)&&!defined(ANDROID)&& \
    !defined(__CYGWIN__)

typedef struct barrier {
    pthread_mutex_t mut;
    pthread_cond_t cond;
    uint32_t count, max, iteration;
} barrier_t;
#endif
#endif


 void *csoundCreateThread2(uintptr_t (*threadRoutine)(void *),
                                 uint32_t stack,
                                 void *userdata)
{
    pthread_attr_t attr;
    pthread_t *thread;
    int status;

    if (pthread_attr_init(&attr) != 0)
      return NULL;
    if (stack != 0 && pthread_attr_setstacksize(&attr, stack) != 0) {
      pthread_attr_destroy(&attr);
      return NULL;
    }
    thread = (pthread_t *)malloc(sizeof(pthread_t));
    if (thread == NULL) {
      pthread_attr_destroy(&attr);
      return NULL;
    }
    status = pthread_create(thread, &attr,
                            (void *(*)(void *))(void*)threadRoutine, userdata);
    pthread_attr_destroy(&attr);
    if (status == 0)
      return thread;
    free(thread);
    return NULL;

}


 void *csoundCreateThread(uintptr_t (*threadRoutine)(void *),
                                void *userdata)
{
    pthread_t *pthread = (pthread_t *) malloc(sizeof(pthread_t));
    if (pthread == NULL)
      return NULL;
    if (!pthread_create(pthread, (pthread_attr_t*) NULL,
                        (void *(*)(void *))(void*)threadRoutine, userdata)) {
      return (void*) pthread;
    }
    free(pthread);
    return NULL;

}

 void *csoundGetCurrentThreadId(void)
{
    pthread_t *ppthread = (pthread_t *)malloc(sizeof(pthread_t));
    if (ppthread == NULL)
      return NULL;
    *ppthread = pthread_self(); /* This version wastes space but works */
    return ppthread;
}

 uintptr_t csoundJoinThread(void *thread)
{
    void *threadRoutineReturnValue = NULL;
    int32_t pthreadReturnValue;
    pthread_t *pthread = (pthread_t *)thread;
    if(thread == NULL) return 0;
    pthreadReturnValue = pthread_join(*pthread,
                                      &threadRoutineReturnValue);
    if (pthreadReturnValue) {
        return (uintptr_t) ((intptr_t) pthreadReturnValue);
    } else {
        free(pthread);
        return (uintptr_t) threadRoutineReturnValue;
    }
}

typedef struct CsoundThreadLock_s {
  pthread_mutex_t m;
  pthread_cond_t  c;
  unsigned char   s;
} CsoundThreadLock_t;

 void *csoundCreateThreadLock(void)
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

 int32_t csoundWaitThreadLock(void *threadLock, size_t milliseconds)
{
    CsoundThreadLock_t  *p;
    int32_t                 retval = 0;
    p = (CsoundThreadLock_t*) threadLock;
    pthread_mutex_lock(&(p->m));
    if (!p->s) {
      if (milliseconds) {
        struct timeval  tv;
        struct timespec ts;
        register size_t n, s;
#ifndef HAVE_GETTIMEOFDAY
      gettimeofday_(&tv, NULL);
#else
      gettimeofday(&tv, NULL);
#endif
        s = milliseconds / (size_t) 1000;
        n = milliseconds - (s * (size_t) 1000);
        s += (size_t) tv.tv_sec;
        n = (size_t) (((int32_t) n * 1000 + (int32_t) tv.tv_usec) * 1000);
        ts.tv_nsec = (long) (n < (size_t) 1000000000 ? n : n - 1000000000);
        ts.tv_sec = (time_t) (n < (size_t) 1000000000 ? s : s + 1);
        do {
          retval = pthread_cond_timedwait(&(p->c), &(p->m), &ts);
        } while (!p->s && !retval);
      }
      else
        retval = ETIMEDOUT;
    }
    /* A notification may arrive as the timed wait expires. */
    if (p->s) retval = 0;
    p->s = (unsigned char) 0;
    pthread_mutex_unlock(&(p->m));

    return retval;
}

 void csoundWaitThreadLockNoTimeout(void *threadLock)
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

 void csoundNotifyThreadLock(void *threadLock)
{
    CsoundThreadLock_t  *p;

    p = (CsoundThreadLock_t*) threadLock;
    pthread_mutex_lock(&(p->m));
    p->s = (unsigned char) 1;
    pthread_cond_signal(&(p->c));
    pthread_mutex_unlock(&(p->m));
}

 void csoundDestroyThreadLock(void *threadLock)
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


 void *csoundCreateBarrier(uint32_t max)
{
#if !defined(HAVE_PTHREAD_BARRIER_INIT)
  /* iteration needed to distinguish between separate sets of max threads */
  /* where a thread enters the barrier before others have had a chance to leave */
  /* this limits us to 2^32 barrier synchronisations, but only if one thread */
  /* gets stuck and doesn't leave for 2^32 other synchronisations */
  barrier_t *b;
  if (max == 0) return (void*)EINVAL;
  b = (barrier_t *)malloc(sizeof(barrier_t));
  pthread_mutex_init(&b->mut, NULL);
  pthread_cond_init(&b->cond, NULL);
  b->count = 0;
  b->iteration = 0;
  b->max = max;
  return b;
#else
  pthread_barrier_t *barrier =
    (pthread_barrier_t *) malloc(sizeof(pthread_barrier_t));
  int32_t status = pthread_barrier_init(barrier, 0, max);
  fprintf(stderr, "Create barrier %d => %p (%d)\n", max, barrier, status);
  if (status) return 0;
  return barrier;
#endif
}

 int32_t csoundDestroyBarrier(void *barrier)
{
#if !defined(HAVE_PTHREAD_BARRIER_INIT)
  barrier_t *b = (barrier_t *)barrier;
  if (b->count > 0) return EBUSY;
  pthread_cond_destroy(&b->cond);
  pthread_mutex_destroy(&b->mut);
#else
  pthread_barrier_destroy(barrier);
#endif
  free(barrier);
  return 0;
}

/* when barrier is passed, all threads except one return 0 */
 int32_t csoundWaitBarrier(void *barrier)
{
#if !defined(HAVE_PTHREAD_BARRIER_INIT)
  int32_t ret;
  uint32_t it;
    barrier_t *b = (barrier_t *)barrier;
    pthread_mutex_lock(&b->mut);
    b->count++;
    it = b->iteration;
    if (b->count >= b->max) {
      b->count = 0;
      b->iteration++;
      pthread_cond_broadcast(&b->cond);
      ret = BARRIER_SERIAL_THREAD;
    } else {
      while (it == b->iteration) pthread_cond_wait(&b->cond, &b->mut);
      ret = 0;
    }
    pthread_mutex_unlock(&b->mut);
    return ret;
#else
    return pthread_barrier_wait((pthread_barrier_t *)barrier);
#endif
}

/**
 * Creates and returns a mutex object, or NULL if not successful.
 * Mutexes can be faster than the more general purpose monitor objects
 * returned by csoundCreateThreadLock() on some platforms, and can also
 * be recursive, but the result of unlocking a mutex that is owned by
 * another thread or is not locked is undefined.
 * If 'isRecursive' is non-zero, the mutex can be re-locked multiple
 * times by the same thread, requiring an equal number of unlock calls;
 * otherwise, attempting to re-lock the mutex results in undefined
 * behavior.
 * Note: the handles returned by csoundCreateThreadLock() and
 * csoundCreateMutex() are not compatible.
 */

 void *csoundCreateMutex(int32_t isRecursive)
{
    pthread_mutex_t     *mutex_ = (pthread_mutex_t*) NULL;
    pthread_mutexattr_t attr;

    if (pthread_mutexattr_init(&attr) == 0) {
      if (pthread_mutexattr_settype(&attr, (isRecursive ?
                                            (int32_t) PTHREAD_MUTEX_RECURSIVE
                                            : (int32_t) PTHREAD_MUTEX_DEFAULT))
          == 0) {
        mutex_ = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
        if (mutex_ != NULL) {
          if (pthread_mutex_init(mutex_, &attr) != 0) {
            free((void*) mutex_);
            mutex_ = (pthread_mutex_t*) NULL;
          }
        }
      }
      pthread_mutexattr_destroy(&attr);
    }
    return (void*) mutex_;
}

/**
 * Acquires the indicated mutex object; if it is already in use by
 * another thread, the function waits until the mutex is released by
 * the other thread.
 */

 void csoundLockMutex(void *mutex_)
{
    pthread_mutex_lock((pthread_mutex_t*) mutex_);
}

/**
 * Acquires the indicated mutex object and returns zero, unless it is
 * already in use by another thread, in which case a non-zero value is
 * returned immediately, rather than waiting until the mutex becomes
 * available.
 * Note: this function may be unimplemented on Windows.
 */

 int32_t csoundLockMutexNoWait(void *mutex_)
{
    return pthread_mutex_trylock((pthread_mutex_t*) mutex_);
}

/**
 * Releases the indicated mutex object, which should be owned by
 * the current thread, otherwise the operation of this function is
 * undefined. A recursive mutex needs to be unlocked as many times
 * as it was locked previously.
 */

 void csoundUnlockMutex(void *mutex_)
{
    pthread_mutex_unlock((pthread_mutex_t*) mutex_);
}

/**
 * Destroys the indicated mutex object. Destroying a mutex that
 * is currently owned by a thread results in undefined behavior.
 */

 void csoundDestroyMutex(void *mutex_)
{
    if (mutex_ != NULL) {
      pthread_mutex_destroy((pthread_mutex_t*) mutex_);
      free(mutex_);
    }
}

/* ------------------------------------------------------------------------ */


 void* csoundCreateCondVar()
{
  pthread_cond_t* condVar = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));

  if (condVar != NULL)
    pthread_cond_init(condVar, NULL);
  return (void*) condVar;
}

 void csoundCondWait(void* condVar, void* mutex) {
        pthread_cond_wait(condVar, mutex);
}

 void csoundCondSignal(void* condVar) {
        pthread_cond_signal(condVar);
}

 void csoundDestroyCondVar(void* condVar) {
        pthread_cond_destroy((pthread_cond_t*)condVar);
        free(condVar);
}

/* ------------------------------------------------------------------------ */

#elif defined(WIN32)
#include <windows.h>
#if !defined(_USING_V110_SDK71_)
#include <synchapi.h>
#endif
#include <process.h>

/* #undef NO_WIN9X_COMPATIBILITY */

typedef struct {
  HANDLE      handle; /* Keep first: the engine reads the native thread handle. */
  uintptr_t   (*func)(void *);
  void        *userdata;
  uintptr_t   result;
} threadParams;

static uint32_t __stdcall threadRoutineWrapper(void *arg)
{
  threadParams *p = (threadParams *)arg;
  p->result = p->func(p->userdata);
  return 0;
}

 void *csoundCreateThread2(uintptr_t (*threadRoutine)(void *), uint32_t stack,
                          void *userdata)
{
  threadParams *p = (threadParams *)malloc(sizeof(threadParams));
  uint32_t threadID;
  if (p == NULL)
    return NULL;
  p->func = threadRoutine;
  p->userdata = userdata;
  p->result = 0;
  p->handle = (HANDLE)_beginthreadex(NULL, stack, threadRoutineWrapper,
                                    p, 0, &threadID);
  if (p->handle != NULL)
    return p;
  free(p);
  return NULL;
}

 void *csoundCreateThread(uintptr_t (*threadRoutine)(void *), void *userdata)
{
  return csoundCreateThread2(threadRoutine, 0, userdata);
}

 void *csoundGetCurrentThreadId(void)
{
    DWORD* d = malloc(sizeof(DWORD));
    if (d == NULL)
      return NULL;
    *d = GetCurrentThreadId();
    return (void*) d;
}

 uintptr_t csoundJoinThread(void *thread)
{
  threadParams *p = (threadParams *)thread;
  uintptr_t result;
  if (p == NULL)
    return 0;
  if (WaitForSingleObject(p->handle, INFINITE) != WAIT_OBJECT_0)
    return (uintptr_t)GetLastError();
  result = p->result;
  CloseHandle(p->handle);
  free(p);
  return result;
}

 void *csoundCreateThreadLock(void)
{
  HANDLE threadLock = CreateEvent(0, 0, TRUE, 0);
  return (void*) threadLock;
}

 int32_t csoundWaitThreadLock(void *lock, size_t milliseconds)
{
  return (int32_t) WaitForSingleObject((HANDLE) lock, milliseconds);
}

 void csoundWaitThreadLockNoTimeout(void *lock)
{
  WaitForSingleObject((HANDLE) lock, INFINITE);
}

 void csoundNotifyThreadLock(void *lock)
{
  SetEvent((HANDLE) lock);
}

 void csoundDestroyThreadLock(void *lock)
{
  CloseHandle((HANDLE) lock);
}

 void csoundSleep(size_t milliseconds)
{
  Sleep((DWORD) milliseconds);
}

/**
 * Creates and returns a mutex object, or NULL if not successful.
 * Mutexes can be faster than the more general purpose monitor objects
 * returned by csoundCreateThreadLock() on some platforms, and can also
 * be recursive, but the result of unlocking a mutex that is owned by
 * another thread or is not locked is undefined.
 * If 'isRecursive' is non-zero, the mutex can be re-locked multiple
 * times by the same thread, requiring an equal number of unlock calls;
 * otherwise, attempting to re-lock the mutex results in undefined
 * behavior.
 * Note: the handles returned by csoundCreateThreadLock() and
 * csoundCreateMutex() are not compatible.
 */

 void *csoundCreateMutex(int32_t isRecursive)
{
  CRITICAL_SECTION  *cs;

  (void) isRecursive;
  cs = (CRITICAL_SECTION*) malloc(sizeof(CRITICAL_SECTION));
  if (cs != NULL)
    InitializeCriticalSection((LPCRITICAL_SECTION) cs);
  return (void*) cs;
}

/**
 * Acquires the indicated mutex object; if it is already in use by
 * another thread, the function waits until the mutex is released by
 * the other thread.
 */

 void csoundLockMutex(void *mutex_)
{
  EnterCriticalSection((LPCRITICAL_SECTION) mutex_);
}

/**
 * Acquires the indicated mutex object and returns zero, unless it is
 * already in use by another thread, in which case a non-zero value is
 * returned immediately, rather than waiting until the mutex becomes
 * available.
 * Note: this function may be unimplemented on Windows.
 */

 int32_t csoundLockMutexNoWait(void *mutex_)
{
#ifdef NO_WIN9X_COMPATIBILITY
  BOOL    retval;
  /* FIXME: may need to define _WIN32_WINNT before including windows.h */
  retval = TryEnterCriticalSection((LPCRITICAL_SECTION) mutex_);
  return (retval == FALSE ? 1 : 0);
#else
  /* stub for compatibility with Windows 9x */
  EnterCriticalSection((LPCRITICAL_SECTION) mutex_);
  return 0;
#endif
}

/**
 * Releases the indicated mutex object, which should be owned by
 * the current thread, otherwise the operation of this function is
 * undefined. A recursive mutex needs to be unlocked as many times
 * as it was locked previously.
 */

 void csoundUnlockMutex(void *mutex_)
{
  LeaveCriticalSection((LPCRITICAL_SECTION) mutex_);
}

/**
 * Destroys the indicated mutex object. Destroying a mutex that
 * is currently owned by a thread results in undefined behavior.
 */

 void csoundDestroyMutex(void *mutex_)
{
  if (mutex_ != NULL) {
    DeleteCriticalSection((LPCRITICAL_SECTION) mutex_);
    free(mutex_);
  }
}

/**
 * Runs an external command with the arguments specified in 'argv'.
 * argv[0] is the name of the program to execute (if not a full path
 * file name, it is searched in the directories defined by the PATH
 * environment variable). The list of arguments should be terminated
 * by a NULL pointer.
 * If 'noWait' is zero, the function waits until the external program
 * finishes, otherwise it returns immediately. In the first case, a
 * non-negative return value is the exit status of the command (0 to
 * 255), otherwise it is the PID of the newly created process.
 * On error, a negative value is returned.
 */

 long csoundRunCommand(const char * const *argv, int32_t noWait)
{
  long    retval;

  if (argv == NULL || argv[0] == NULL)
    return -1L;
  retval = (long) _spawnvp((noWait ? (int32_t) _P_NOWAIT : (int32_t) _P_WAIT),
      argv[0], argv);
  if (!noWait && retval >= 0L)
    retval &= 255L;
  return retval;
}

 void* csoundCreateCondVar()
{
    CONDITION_VARIABLE* condVar =
      (CONDITION_VARIABLE*)malloc(sizeof(CONDITION_VARIABLE));

    if (condVar != NULL)
      InitializeConditionVariable(condVar);
    return (void*) condVar;
}

 void csoundCondWait(void* condVar, void* mutex) {
    CONDITION_VARIABLE* cv = (CONDITION_VARIABLE*)condVar;
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)mutex;
    SleepConditionVariableCS(cv, cs, INFINITE);
}

 void csoundCondSignal(void* condVar) {
    CONDITION_VARIABLE* cv = (CONDITION_VARIABLE*)condVar;
    WakeConditionVariable(cv);
}

 void csoundDestroyCondVar(void* condVar) {
    memset(condVar, '\0', sizeof(CONDITION_VARIABLE));
    free(condVar);
}

// REMOVE FOLLOWING BARRIER DEFINITION WINDOWS SUPPORT LIMITED to WIN 8.1+
typedef struct barrier {
    CRITICAL_SECTION* mut;
    CONDITION_VARIABLE* cond;
    uint32_t count, max, iteration;
} win_barrier_t;

 void *csoundCreateBarrier(uint32_t max)
{
  win_barrier_t *barrier =
    (win_barrier_t*)malloc(sizeof(win_barrier_t));

  barrier->cond = (CONDITION_VARIABLE*)csoundCreateCondVar();
  barrier->mut = (CRITICAL_SECTION*)csoundCreateMutex(0);
  barrier->count = 0;
  barrier->iteration = 0;
  barrier->max = max;

  return (void*) barrier;

  // REPLACE ABOVE WITH FOLLOWING ONCE WINDOWS SUPPORT LIMITED to WIN 8.1+
  //SYNCHRONIZATION_BARRIER *barrier =
  //  (SYNCHRONIZATION_BARRIER*)malloc(sizeof(SYNCHRONIZATION_BARRIER));

  //if (barrier != NULL)
  //  InitializeSynchronizationBarrier(barrier, max, -1);
  //return (void*) barrier;
}

 int32_t csoundDestroyBarrier(void *barrier)
{
    win_barrier_t *winb = (win_barrier_t*)barrier;
    free(winb->cond);
    csoundDestroyMutex(winb->mut);
    free(winb);
    return 0;
    // REPLACE ABOVE WITH FOLLOWING ONCE WINDOWS SUPPORT LIMITED to WIN 8.1+
    //DeleteSynchronizationBarrier(barrier);
    //return 0;
}

 int32_t csoundWaitBarrier(void *barrier)
{
    int32_t ret;
    uint32_t it;
  win_barrier_t *winb = (win_barrier_t*)barrier;
  csoundLockMutex(winb->mut);
  winb->count++;
  it = winb->iteration;
  if (winb->count >= winb->max) {
      winb->count = 0;
      winb->iteration++;
      WakeAllConditionVariable(winb->cond);
      ret = 1;
  }
  else {
      while(it == winb->iteration) {
        csoundCondWait(winb->cond, winb->mut);
      }
      ret = 0;
  }
  csoundUnlockMutex(winb->mut);
  return ret;
    // REPLACE ABOVE WITH FOLLOWING ONCE WINDOWS SUPPORT LIMITED to WIN 8.1+
    //EnterSynchronizationBarrier(barrier, 0);
    //return 0;
}


/* ------------------------------------------------------------------------ */

#elif defined(__STDC_NO_THREADS__) || defined(BARE_METAL) || defined(__wasi__)

 void *csoundCreateThread2(uintptr_t (*threadRoutine)(void *), uint32_t stack,
                                void *userdata)
{
    //notImplementedWarning_("csoundCreateThread");
    return NULL;
}


 void *csoundCreateThread(uintptr_t (*threadRoutine)(void *),
                                void *userdata)
{
    //notImplementedWarning_("csoundCreateThread");
    return NULL;
}

 void *csoundGetCurrentThreadId(void)
{
    //notImplementedWarning_("csoundGetCurrentThreadId");
    return NULL;
}

 uintptr_t csoundJoinThread(void *thread)
{
    //notImplementedWarning_("csoundJoinThread");
    return (uintptr_t) 0;
}

 void *csoundCreateThreadLock(void)
{
    //notImplementedWarning_("csoundCreateThreadLock");
    return NULL;
}

 int32_t csoundWaitThreadLock(void *lock, size_t milliseconds)
{
    //notImplementedWarning_("csoundWaitThreadLock");
    return 0;
}

 void csoundWaitThreadLockNoTimeout(void *lock)
{
    //notImplementedWarning_("csoundWaitThreadLockNoTimeout");
}

 void csoundNotifyThreadLock(void *lock)
{
    //notImplementedWarning_("csoundNotifyThreadLock");
}

 void csoundDestroyThreadLock(void *lock)
{
    //notImplementedWarning_("csoundDestroyThreadLock");
}

 void *csoundCreateMutex(int32_t isRecursive)
{
    //notImplementedWarning_("csoundCreateMutex");
    return NULL;
}

 void csoundLockMutex(void *mutex_)
{
    //notImplementedWarning_("csoundLockMutex");
}

 int32_t csoundLockMutexNoWait(void *mutex_)
{
    //notImplementedWarning_("csoundLockMutexNoWait");
    return 0;
}

 void csoundUnlockMutex(void *mutex_)
{
    //notImplementedWarning_("csoundUnlockMutex");
}

 void csoundDestroyMutex(void *mutex_)
{
    //notImplementedWarning_("csoundDestroyMutex");
}

 void *csoundCreateBarrier(uint32_t max)
{
    //notImplementedWarning_("csoundDestroyBarrier");
    return NULL;
}

 int32_t csoundDestroyBarrier(void *barrier)
{
    //notImplementedWarning_("csoundDestroyBarrier");
    return 0;
}

 int32_t csoundWaitBarrier(void *barrier)
{
    //notImplementedWarning_("csoundWaitBarrier");
    return 0;
}


 void* csoundCreateCondVar()
{
    //notImplementedWarning_("csoundCreateCondVar");
    return NULL;
}

 void csoundCondWait(void* condVar, void* mutex) {
    //notImplementedWarning_("csoundCreateCondWait");
}

 void csoundCondSignal(void* condVar) {
    // notImplementedWarning_("csoundCreateCondSignal");
}

 void csoundDestroyCondVar(void* condVar) {
    // notImplementedWarning_("csoundDestroyCondVar");
}

 long csoundRunCommand(const char * const *argv, int32_t noWait) {
    IGN(argv);
    IGN(noWait);
    return -1L;
}

 void csoundSleep(size_t milliseconds) {
    //notImplementedWarning_("csoundSleep");
}

#else // C THREADS
#include <threads.h>

typedef struct {
  thrd_t thread; /* Keep the native thread ID first, as in the pthread backend. */
  uintptr_t (*func)(void *);
  void *userdata;
  uintptr_t result;
} threadParams;

static int threadRoutineWrapper(void *arg)
{
  threadParams *p = (threadParams *)arg;
  p->result = p->func(p->userdata);
  return 0;
}

 void *csoundCreateThread(uintptr_t (*threadRoutine)(void *), void *userdata)
{
  threadParams *p = (threadParams *)malloc(sizeof(threadParams));
  if (p == NULL)
    return NULL;
  p->func = threadRoutine;
  p->userdata = userdata;
  p->result = 0;
  if (thrd_create(&p->thread, threadRoutineWrapper, p) == thrd_success)
    return p;
  free(p);
  return NULL;
}

 void *csoundCreateThread2(uintptr_t (*threadRoutine)(void *), uint32_t stack,
                          void *userdata)
{
  IGN(stack); /* C11 threads have no stack-size attribute. */
  return csoundCreateThread(threadRoutine, userdata);
}

 void *csoundGetCurrentThreadId(void)
{
  thrd_t *thread = (thrd_t *)malloc(sizeof(thrd_t));
  if (thread == NULL)
    return NULL;
  *thread = thrd_current();
  return thread;
}

 uintptr_t csoundJoinThread(void *thread)
{
  threadParams *p = (threadParams *)thread;
  uintptr_t result;
  int status;
  if (p == NULL)
    return 0;
  status = thrd_join(p->thread, NULL);
  if (status != thrd_success)
    return (uintptr_t)(intptr_t)status;
  result = p->result;
  free(p);
  return result;
}

typedef struct {
  mtx_t mutex;
  cnd_t condition;
  int signaled;
} CsoundThreadLock_t;

 void *csoundCreateThreadLock(void)
{
  CsoundThreadLock_t *p = malloc(sizeof(CsoundThreadLock_t));
  if (p == NULL)
    return NULL;
  if (mtx_init(&p->mutex, mtx_plain) != thrd_success) {
    free(p);
    return NULL;
  }
  if (cnd_init(&p->condition) != thrd_success) {
    mtx_destroy(&p->mutex);
    free(p);
    return NULL;
  }
  p->signaled = 1;
  return p;
}

 int32_t csoundWaitThreadLock(void *lock, size_t milliseconds)
{
  CsoundThreadLock_t *p = lock;
  int status = thrd_success;
  mtx_lock(&p->mutex);
  if (!p->signaled) {
    if (milliseconds) {
      struct timespec deadline;
      if (timespec_get(&deadline, TIME_UTC) != TIME_UTC) {
        mtx_unlock(&p->mutex);
        return NOTOK;
      }
      deadline.tv_sec += milliseconds / 1000;
      deadline.tv_nsec += (long)(milliseconds % 1000) * 1000000L;
      if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_nsec -= 1000000000L;
        deadline.tv_sec++;
      }
      do {
        status = cnd_timedwait(&p->condition, &p->mutex, &deadline);
      } while (!p->signaled && status == thrd_success);
    }
    else
      status = thrd_timedout;
  }
  /* Consume a pending notification even if the timed wait just expired. */
  if (p->signaled) status = thrd_success;
  p->signaled = 0;
  mtx_unlock(&p->mutex);
  return status == thrd_success ? OK : NOTOK;
}

 void csoundWaitThreadLockNoTimeout(void *lock)
{
  CsoundThreadLock_t *p = lock;
  mtx_lock(&p->mutex);
  while (!p->signaled)
    cnd_wait(&p->condition, &p->mutex);
  p->signaled = 0;
  mtx_unlock(&p->mutex);
}

 void csoundNotifyThreadLock(void *lock)
{
  CsoundThreadLock_t *p = lock;
  mtx_lock(&p->mutex);
  p->signaled = 1;
  cnd_signal(&p->condition);
  mtx_unlock(&p->mutex);
}

 void csoundDestroyThreadLock(void *lock)
{
  CsoundThreadLock_t *p = lock;
  if (p != NULL) {
    cnd_destroy(&p->condition);
    mtx_destroy(&p->mutex);
    free(p);
  }
}

 void *csoundCreateMutex(int32_t isRecursive)
{
    mtx_t *thread_mutex;
    thread_mutex = (mtx_t*) malloc(sizeof(mtx_t));
    if (thread_mutex == NULL)
      return NULL;
    if (mtx_init(thread_mutex, isRecursive ? mtx_plain | mtx_recursive : mtx_plain)
        != thrd_success) {
      free(thread_mutex);
      return NULL;
    }
    return (void*) thread_mutex;
}

 void csoundLockMutex(void *mutex_)
{
    mtx_lock((mtx_t *) mutex_);
}

 int32_t csoundLockMutexNoWait(void *mutex_)
{
    return mtx_trylock((mtx_t *) mutex_) == thrd_success ? OK : NOTOK;
}

 void csoundUnlockMutex(void *mutex_)
{
    mtx_unlock((mtx_t *) mutex_);
}

 void csoundDestroyMutex(void *mutex_)
{
    if(mutex_ != NULL) {
      mtx_destroy((mtx_t *) mutex_);
      free(mutex_);
    }
}

#define BARRIER_SERIAL_THREAD (-1)

typedef struct barrier {
    mtx_t mut;
    cnd_t cond;
    uint32_t count, max, iteration;
} c11_barrier_t;

 void *csoundCreateBarrier(uint32_t max)
{
  c11_barrier_t *b;
  if (max == 0) return (void*) EINVAL;
  b = (c11_barrier_t *)malloc(sizeof(c11_barrier_t));
  mtx_init(&b->mut, mtx_plain);
  cnd_init(&b->cond);
  b->count = 0;
  b->iteration = 0;
  b->max = max;
  return b;
}

 int32_t csoundDestroyBarrier(void *barrier)
{
  c11_barrier_t *b = (c11_barrier_t *)barrier;
  if (b->count > 0) return EBUSY;
  cnd_destroy(&b->cond);
  mtx_destroy(&b->mut);
  return 0;
}

 int32_t csoundWaitBarrier(void *barrier)
{
  int32_t ret;
  uint32_t it;
    c11_barrier_t *b = (c11_barrier_t *)barrier;
    mtx_lock(&b->mut);
    b->count++;
    it = b->iteration;
    if (b->count >= b->max) {
      b->count = 0;
      b->iteration++;
      cnd_broadcast(&b->cond);
      ret = BARRIER_SERIAL_THREAD;
    } else {
      while (it == b->iteration) cnd_wait(&b->cond, &b->mut);
      ret = 0;
    }
    mtx_unlock(&b->mut);
    return ret;
}

 void* csoundCreateCondVar()
{
  cnd_t* condVar = (cnd_t*)malloc(sizeof(cnd_t));
  if (condVar != NULL)
    cnd_init(condVar);
  return (void*) condVar;
}

 void csoundCondWait(void* condVar, void* mutex) {
  cnd_wait((cnd_t *) condVar, (mtx_t *) mutex); 
}

 void csoundCondSignal(void* condVar) {
  cnd_signal((cnd_t *) condVar);
}

 void csoundDestroyCondVar(void* condVar) {
  if(condVar != NULL) {
    cnd_destroy((cnd_t *) condVar);
    free(condVar);
  }
}

 long csoundRunCommand(const char * const *argv, int32_t noWait) {
  /* C11 threads provide no process API for executing an argument vector. */
  IGN(argv);
  IGN(noWait);
  return -1L;
}

 void csoundSleep(size_t milliseconds) {
    struct timespec ts;
    register size_t n, s;

    s = milliseconds / (size_t) 1000;
    n = milliseconds - (s * (size_t) 1000);
    n = (size_t) ((int32_t) n * 1000000);
    ts.tv_sec = (time_t) s;
    ts.tv_nsec = (long) n;
    while (nanosleep(&ts, &ts) != 0)
      ;
}

#endif // HAVE_PTHREADS

#if defined(MSVC)
// Spinlocks use MSVC atomics
/* This pragma must come before all public function declarations */
# pragma intrinsic(_InterlockedExchange)
void csoundSpinLock(spin_lock_t *spinlock) {
    while (_InterlockedExchange(spinlock, 1) == 1){};
}

void csoundSpinUnLock(spin_lock_t *spinlock){
    _InterlockedExchange(spinlock, 0);
}

int32_t csoundSpinTryLock(spin_lock_t *spinlock) {
    return _InterlockedExchange(spinlock, 1) == 0 ? CSOUND_SUCCESS : CSOUND_ERROR;
}

int32_t csoundSpinLockInit(spin_lock_t *spinlock) {
    *spinlock = SPINLOCK_INIT;
    return 0;
}

#elif defined(MACOSX) // MacOS native locks

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
// New spinlock interface

void csoundSpinLock(spin_lock_t *spinlock){
    os_unfair_lock_lock(spinlock);
}

void csoundSpinUnLock(spin_lock_t *spinlock){
    os_unfair_lock_unlock(spinlock);
}

int32_t csoundSpinTryLock(spin_lock_t *spinlock) {
    return os_unfair_lock_trylock(spinlock) == true ? CSOUND_SUCCESS : CSOUND_ERROR;
}

int32_t csoundSpinLockInit(spin_lock_t *spinlock) {
    *spinlock = OS_UNFAIR_LOCK_INIT;
    return 0;
}

#else // Old spinlock interface

void csoundSpinLock(spin_lock_t *spinlock) {
    OSSpinLockLock((volatile OSSpinLock *) spinlock);
}

void csoundSpinUnLock(spin_lock_t *spinlock) {
    OSSpinLockUnlock((volatile OSSpinLock *) spinlock);
}

int32_t csoundSpinTryLock(spin_lock_t *spinlock) {
    return OSSpinLockTry((volatile OSSpinLock *) spinlock) == true ?
      CSOUND_SUCCESS : CSOUND_ERROR;
}

int32_t csoundSpinLockInit(spin_lock_t *spinlock) {
    *spinlock = SPINLOCK_INIT;
    return 0;
}

#endif // MAC_OS_X_VERSION_MIN_REQUIRED

#elif defined(__GNUC__) && defined(HAVE_PTHREAD_SPIN_LOCK)
// POSIX spin locks

void csoundSpinLock(spin_lock_t *spinlock) {
    pthread_spin_lock(spinlock);
}

void csoundSpinUnLock(spin_lock_t *spinlock){
    pthread_spin_unlock(spinlock);
}

int32_t csoundSpinTryLock(spin_lock_t *spinlock) {
    return pthread_spin_trylock(spinlock);
}

int32_t csoundSpinLockInit(spin_lock_t *spinlock) {
    return pthread_spin_init(spinlock, PTHREAD_PROCESS_PRIVATE);
}


#elif defined(__GNUC__) && defined(HAVE_ATOMIC_BUILTIN)
// No POSIX spinlocks but GCC intrinsics
#include <stdbool.h>

void csoundSpinLock(spin_lock_t *spinlock){
    spin_lock_t unset = 0;
    spin_lock_t set = 1;
    while (!__atomic_compare_exchange_n(spinlock, &unset, set, false,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) { };
}

void csoundSpinUnLock(spin_lock_t *spinlock){
    __atomic_clear(spinlock, __ATOMIC_SEQ_CST);
}

int32_t csoundSpinTryLock(spin_lock_t *spinlock) {
    spin_lock_t unset = 0;
    spin_lock_t set = 1;
    return __atomic_compare_exchange_n(spinlock, &unset, set, false,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ?
      CSOUND_SUCCESS : CSOUND_ERROR;
}

int32_t csoundSpinLockInit(spin_lock_t *spinlock) {
    *spinlock = SPINLOCK_INIT;
    return 0;
}

#else // No spinlocks
void csoundSpinLock(spin_lock_t *spinlock) {
    IGN(spinlock);
}
void csoundSpinUnLock(spin_lock_t *spinlock) {
    IGN(spinlock);
}

int32_t csoundSpinTryLock(spin_lock_t *spinlock) {
    IGN(spinlock);
    return 1;
}

int32_t csoundSpinLockInit(spin_lock_t *spinlock) {
    IGN(spinlock);
    return 0;
}

#endif
