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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
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

PUBLIC long csoundRunCommand(const char * const *argv, int noWait)
{
    long    retval;

    if (argv == NULL || argv[0] == NULL)
      return -1L;
    retval = (long) _spawnvp((noWait ? (int) _P_NOWAIT : (int) _P_WAIT),
                             argv[0], argv);
    if (!noWait && retval >= 0L)
      retval &= 255L;
    return retval;
}

PUBLIC void csoundSleep(size_t milliseconds)
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

PUBLIC long csoundRunCommand(const char * const *argv, int noWait)
{
    long    retval;

    if (argv == NULL || argv[0] == NULL)
      return -1L;
    retval = (long) fork();
    if (retval == 0L) {
      /* child process */
      if (execvp(argv[0], (char**) argv) != 0)
        exit(-1);
      /* this is not actually reached */
      exit(0);
    }
    else if (retval > 0L && noWait == 0) {
      int   status = 0;
      while (waitpid((pid_t) retval, &status, 0) != (pid_t) ECHILD) {
        if (WIFEXITED(status) != 0) {
          retval = (long) (WEXITSTATUS(status)) & 255L;
          return retval;
        }
        if (WIFSIGNALED(status) != 0) {
          retval = 255L;
          return retval;
        }
      }
      retval = 255L;
    }
    return retval;
}

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

#endif

#ifndef __wasi__
#include <errno.h>
#endif

#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#define BARRIER_SERIAL_THREAD (-1)

#if !defined(HAVE_PTHREAD_BARRIER_INIT)
#if !defined( __MACH__)&&!defined(__HAIKU__)&&!defined(ANDROID)&& \
    !defined(NACL)&&!defined(__CYGWIN__)

typedef struct barrier {
    pthread_mutex_t mut;
    pthread_cond_t cond;
    unsigned int count, max, iteration;
} barrier_t;
#endif
#endif


PUBLIC void *csoundCreateThread2(uintptr_t (*threadRoutine)(void *),
                                 unsigned int stack,
                                 void *userdata)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack);
    pthread_t *pthread = (pthread_t *) malloc(sizeof(pthread_t));
    if (!pthread_create(pthread, (pthread_attr_t*) NULL,
                        (void *(*)(void *))(void*)threadRoutine, userdata)) {
      return (void*) pthread;
    }
    free(pthread);
    return NULL;

}


PUBLIC void *csoundCreateThread(uintptr_t (*threadRoutine)(void *),
                                void *userdata)
{
    pthread_t *pthread = (pthread_t *) malloc(sizeof(pthread_t));
    if (!pthread_create(pthread, (pthread_attr_t*) NULL,
                        (void *(*)(void *))(void*)threadRoutine, userdata)) {
      return (void*) pthread;
    }
    free(pthread);
    return NULL;

}

PUBLIC void *csoundGetCurrentThreadId(void)
{
    pthread_t *ppthread = (pthread_t *)malloc(sizeof(pthread_t));
    *ppthread = pthread_self(); /* This version wastes space but works */
    return ppthread;
}

PUBLIC uintptr_t csoundJoinThread(void *thread)
{
    void *threadRoutineReturnValue = NULL;
    int pthreadReturnValue;
    pthread_t *pthread = (pthread_t *)thread;
    if(thread == NULL) return 0;
    pthreadReturnValue = pthread_join(*pthread,
                                      &threadRoutineReturnValue);
    free(pthread);
    if (pthreadReturnValue) {
        return (uintptr_t) ((intptr_t) pthreadReturnValue);
    } else {
        return (uintptr_t) threadRoutineReturnValue;
    }
}

#if !defined(ANDROID) && (/*defined(LINUX) ||*/ defined(__HAIKU__) || defined(WIN32))

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

#ifndef HAVE_GETTIMEOFDAY
      gettimeofday_(&tv, NULL);
#else
      gettimeofday(&tv, NULL);
#endif

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
    if (0==pthread_mutex_destroy((pthread_mutex_t*) lock))
      free(lock);
    else
      perror("csoundDestroyThreadLock: ");
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
#ifndef HAVE_GETTIMEOFDAY
      gettimeofday_(&tv, NULL);
#else
      gettimeofday(&tv, NULL);
#endif
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


PUBLIC void *csoundCreateBarrier(unsigned int max)
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
  int status = pthread_barrier_init(barrier, 0, max);
  fprintf(stderr, "Create barrier %d => %p (%d)\n", max, barrier, status);
  if (status) return 0;
  return barrier;
#endif
}

PUBLIC int csoundDestroyBarrier(void *barrier)
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
PUBLIC int csoundWaitBarrier(void *barrier)
{
#if !defined(HAVE_PTHREAD_BARRIER_INIT)
  int ret;
  unsigned int it;
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

PUBLIC void *csoundCreateMutex(int isRecursive)
{
    pthread_mutex_t     *mutex_ = (pthread_mutex_t*) NULL;
    pthread_mutexattr_t attr;

    if (pthread_mutexattr_init(&attr) == 0) {
      if (pthread_mutexattr_settype(&attr, (isRecursive ?
                                            (int) PTHREAD_MUTEX_RECURSIVE
                                            : (int) PTHREAD_MUTEX_DEFAULT))
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

PUBLIC void csoundLockMutex(void *mutex_)
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

PUBLIC int csoundLockMutexNoWait(void *mutex_)
{
    return pthread_mutex_trylock((pthread_mutex_t*) mutex_);
}

/**
 * Releases the indicated mutex object, which should be owned by
 * the current thread, otherwise the operation of this function is
 * undefined. A recursive mutex needs to be unlocked as many times
 * as it was locked previously.
 */

PUBLIC void csoundUnlockMutex(void *mutex_)
{
    pthread_mutex_unlock((pthread_mutex_t*) mutex_);
}

/**
 * Destroys the indicated mutex object. Destroying a mutex that
 * is currently owned by a thread results in undefined behavior.
 */

PUBLIC void csoundDestroyMutex(void *mutex_)
{
    if (mutex_ != NULL) {
      pthread_mutex_destroy((pthread_mutex_t*) mutex_);
      free(mutex_);
    }
}

/* ------------------------------------------------------------------------ */


PUBLIC void* csoundCreateCondVar()
{
  pthread_cond_t* condVar = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));

  if (condVar != NULL)
    pthread_cond_init(condVar, NULL);
  return (void*) condVar;
}

PUBLIC void csoundCondWait(void* condVar, void* mutex) {
        pthread_cond_wait(condVar, mutex);
}

PUBLIC void csoundCondSignal(void* condVar) {
        pthread_cond_signal(condVar);
}

PUBLIC void csoundDestroyCondVar(void* condVar) {
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

PUBLIC void *csoundCreateThread2(uintptr_t (*threadRoutine)(void *), unsigned int stack, void *userdata) {
  threadParams  p;
  void          *h;
  unsigned int  threadID;

  p.func = threadRoutine;
  p.userdata = userdata;
  p.threadLock = CreateEvent(0, 0, 0, 0);
  if (p.threadLock == (HANDLE) 0)
    return NULL;
  h = (void*) _beginthreadex(NULL, stack, threadRoutineWrapper,
      (void*) &p, (unsigned) 0, &threadID);
  if (h != NULL)
    WaitForSingleObject(p.threadLock, INFINITE);
  CloseHandle(p.threadLock);
  return h;

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

PUBLIC void *csoundGetCurrentThreadId(void)
{
    DWORD* d = malloc(sizeof(DWORD));
    *d = GetCurrentThreadId();
    return (void*) d;
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

PUBLIC void *csoundCreateMutex(int isRecursive)
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

PUBLIC void csoundLockMutex(void *mutex_)
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

PUBLIC int csoundLockMutexNoWait(void *mutex_)
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

PUBLIC void csoundUnlockMutex(void *mutex_)
{
  LeaveCriticalSection((LPCRITICAL_SECTION) mutex_);
}

/**
 * Destroys the indicated mutex object. Destroying a mutex that
 * is currently owned by a thread results in undefined behavior.
 */

PUBLIC void csoundDestroyMutex(void *mutex_)
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

PUBLIC long csoundRunCommand(const char * const *argv, int noWait)
{
  long    retval;

  if (argv == NULL || argv[0] == NULL)
    return -1L;
  retval = (long) _spawnvp((noWait ? (int) _P_NOWAIT : (int) _P_WAIT),
      argv[0], argv);
  if (!noWait && retval >= 0L)
    retval &= 255L;
  return retval;
}

PUBLIC void* csoundCreateCondVar()
{
    CONDITION_VARIABLE* condVar =
      (CONDITION_VARIABLE*)malloc(sizeof(CONDITION_VARIABLE));

    if (condVar != NULL)
      InitializeConditionVariable(condVar);
    return (void*) condVar;
}

PUBLIC void csoundCondWait(void* condVar, void* mutex) {
    CONDITION_VARIABLE* cv = (CONDITION_VARIABLE*)condVar;
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)mutex;
    SleepConditionVariableCS(cv, cs, INFINITE);
}

PUBLIC void csoundCondSignal(void* condVar) {
    CONDITION_VARIABLE* cv = (CONDITION_VARIABLE*)condVar;
    WakeConditionVariable(cv);
}

PUBLIC void csoundDestroyCondVar(void* condVar) {
    memset(condVar, '\0', sizeof(CONDITION_VARIABLE));
    free(condVar);
}

// REMOVE FOLLOWING BARRIER DEFINITION WINDOWS SUPPORT LIMITED to WIN 8.1+
typedef struct barrier {
    CRITICAL_SECTION* mut;
    CONDITION_VARIABLE* cond;
    unsigned int count, max, iteration;
} win_barrier_t;

PUBLIC void *csoundCreateBarrier(unsigned int max)
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

PUBLIC int csoundDestroyBarrier(void *barrier)
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

PUBLIC int csoundWaitBarrier(void *barrier)
{
    int ret;
    unsigned int it;
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

#else

PUBLIC void *csoundCreateThread2(uintptr_t (*threadRoutine)(void *), unsigned int stack,
                                void *userdata)
{
    //notImplementedWarning_("csoundCreateThread");
    return NULL;
}


PUBLIC void *csoundCreateThread(uintptr_t (*threadRoutine)(void *),
                                void *userdata)
{
    //notImplementedWarning_("csoundCreateThread");
    return NULL;
}

PUBLIC void *csoundGetCurrentThreadId(void)
{
    //notImplementedWarning_("csoundGetCurrentThreadId");
    return NULL;
}

PUBLIC uintptr_t csoundJoinThread(void *thread)
{
    //notImplementedWarning_("csoundJoinThread");
    return (uintptr_t) 0;
}

PUBLIC void *csoundCreateThreadLock(void)
{
    //notImplementedWarning_("csoundCreateThreadLock");
    return NULL;
}

PUBLIC int csoundWaitThreadLock(void *lock, size_t milliseconds)
{
    //notImplementedWarning_("csoundWaitThreadLock");
    return 0;
}

PUBLIC void csoundWaitThreadLockNoTimeout(void *lock)
{
    //notImplementedWarning_("csoundWaitThreadLockNoTimeout");
}

PUBLIC void csoundNotifyThreadLock(void *lock)
{
    //notImplementedWarning_("csoundNotifyThreadLock");
}

PUBLIC void csoundDestroyThreadLock(void *lock)
{
    //notImplementedWarning_("csoundDestroyThreadLock");
}

PUBLIC void *csoundCreateMutex(int isRecursive)
{
    //notImplementedWarning_("csoundCreateMutex");
    return NULL;
}

PUBLIC void csoundLockMutex(void *mutex_)
{
    //notImplementedWarning_("csoundLockMutex");
}

PUBLIC int csoundLockMutexNoWait(void *mutex_)
{
    //notImplementedWarning_("csoundLockMutexNoWait");
    return 0;
}

PUBLIC void csoundUnlockMutex(void *mutex_)
{
    //notImplementedWarning_("csoundUnlockMutex");
}

PUBLIC void csoundDestroyMutex(void *mutex_)
{
    //notImplementedWarning_("csoundDestroyMutex");
}

PUBLIC void *csoundCreateBarrier(unsigned int max)
{
    //notImplementedWarning_("csoundDestroyBarrier");
    return NULL;
}

PUBLIC int csoundDestroyBarrier(void *barrier)
{
    //notImplementedWarning_("csoundDestroyBarrier");
    return 0;
}

PUBLIC int csoundWaitBarrier(void *barrier)
{
    //notImplementedWarning_("csoundWaitBarrier");
    return 0;
}


PUBLIC void* csoundCreateCondVar()
{
    //notImplementedWarning_("csoundCreateCondVar");
    return NULL;
}

PUBLIC void csoundCondWait(void* condVar, void* mutex) {
    //notImplementedWarning_("csoundCreateCondWait");
}

PUBLIC void csoundCondSignal(void* condVar) {
    // notImplementedWarning_("csoundCreateCondSignal");
}

PUBLIC void csoundDestroyCondVar(void* condVar) {
    // notImplementedWarning_("csoundDestroyCondVar");
}

PUBLIC long csoundRunCommand(const char * const *argv, int noWait) {
    //notImplementedWarning_("csoundRunCommand");
    return 0;
}

PUBLIC void csoundSleep(size_t milliseconds) {
    //notImplementedWarning_("csoundSleep");
}


#endif

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

int csoundSpinTryLock(spin_lock_t *spinlock) {
    return _InterlockedExchange(spinlock, 1) == 0 ? CSOUND_SUCCESS : CSOUND_ERROR;
}

int csoundSpinLockInit(spin_lock_t *spinlock) {
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

int csoundSpinTryLock(spin_lock_t *spinlock) {
    return os_unfair_lock_trylock(spinlock) == true ? CSOUND_SUCCESS : CSOUND_ERROR;
}

int csoundSpinLockInit(spin_lock_t *spinlock) {
    IGN(spinlock);
    return 0;
}

#else // Old spinlock interface

void csoundSpinLock(spin_lock_t *spinlock) {
    OSSpinLockLock((volatile OSSpinLock *) spinlock);
}

void csoundSpinUnLock(spin_lock_t *spinlock) {
    OSSpinLockUnlock((volatile OSSpinLock *) spinlock);
}

int csoundSpinTryLock(spin_lock_t *spinlock) {
    return OSSpinLockTry((volatile OSSpinLock *) spinlock) == true ?
      CSOUND_SUCCESS : CSOUND_ERROR;
}

int csoundSpinLockInit(spin_lock_t *spinlock) {
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

int csoundSpinTryLock(spin_lock_t *spinlock) {
    return pthread_spin_trylock(spinlock);
}

int csoundSpinLockInit(spin_lock_t *spinlock) {
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

int csoundSpinTryLock(spin_lock_t *spinlock) {
    spin_lock_t unset = 0;
    spin_lock_t set = 1;
    return __atomic_compare_exchange_n(spinlock, &unset, set, false,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ?
      CSOUND_SUCCESS : CSOUND_ERROR;
}

int csoundSpinLockInit(spin_lock_t *spinlock) {
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

int csoundSpinTryLock(spin_lock_t *spinlock) {
    IGN(spinlock);
    return 1;
}

int csoundSpinLockInit(spin_lock_t *spinlock) {
    IGN(spinlock);
    return 0;
}

#endif
