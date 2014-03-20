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

#ifndef HAVE_GETTIMEOFDAY
#if defined(LINUX)    || defined(__unix)   || defined(__unix__) || \
    defined(__MACH__) || defined(__HAIKU__)
#define HAVE_GETTIMEOFDAY 1
#endif
#endif

#include "csoundCore.h"
#include "csGblMtx.h"


#if defined(WIN32)
#include <windows.h>
#include <process.h>

#ifndef MSVC
//void __stdcall GetSystemTimeAsFileTime(FILETIME*);
#endif

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



#if defined(LINUX) || defined(__MACH__) || defined(__HAIKU__) || \
    defined(ANDROID) || defined(WIN32)

#include <errno.h>
#include <pthread.h>
#include <time.h>
#ifdef MSVC
#include <time.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#define BARRIER_SERIAL_THREAD (-1)

#if !defined(HAVE_PTHREAD_BARRIER_INIT)
#if !defined(__MACH__) && !defined(__HAIKU__) && !defined(ANDROID) && !defined(NACL)

typedef struct barrier {
    pthread_mutex_t mut;
    pthread_cond_t cond;
    unsigned int count, max, iteration;
} barrier_t;
#endif
#endif

PUBLIC void *csoundCreateThread(uintptr_t (*threadRoutine)(void *),
                                void *userdata)
{
    pthread_t *pthread = (pthread_t *) malloc(sizeof(pthread_t));
    if (!pthread_create(pthread, (pthread_attr_t*) NULL,
                        (void *(*)(void *)) threadRoutine, userdata)) {
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
    pthreadReturnValue = pthread_join(*pthread,
                                      &threadRoutineReturnValue);
    if (pthreadReturnValue) {
        return (uintptr_t) ((intptr_t) pthreadReturnValue);
    } else {
        return (uintptr_t) threadRoutineReturnValue;
    }
}

#if !defined(ANDROID) && (defined(LINUX) || defined(__HAIKU__) || defined(WIN32))

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
  int status = pthread_barrier_init(barrier, 0, max-1);
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
    int ret, it;
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

PUBLIC void *csoundGetCurrentThreadId(void)
{
    notImplementedWarning_("csoundGetCurrentThreadId");
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

PUBLIC void *csoundCreateMutex(int isRecursive)
{
    notImplementedWarning_("csoundCreateMutex");
    return NULL;
}

PUBLIC void csoundLockMutex(void *mutex_)
{
    notImplementedWarning_("csoundLockMutex");
}

PUBLIC int csoundLockMutexNoWait(void *mutex_)
{
    notImplementedWarning_("csoundLockMutexNoWait");
    return 0;
}

PUBLIC void csoundUnlockMutex(void *mutex_)
{
    notImplementedWarning_("csoundUnlockMutex");
}

PUBLIC void csoundDestroyMutex(void *mutex_)
{
    notImplementedWarning_("csoundDestroyMutex");
}

PUBLIC void *csoundCreateBarrier(unsigned int max)
{
    notImplementedWarning_("csoundDestroyBarrier");
    return NULL;
}

PUBLIC int csoundDestroyBarrier(void *barrier)
{
    notImplementedWarning_("csoundDestroyBarrier");
    return 0;
}

PUBLIC int csoundWaitBarrier(void *barrier)
{
    notImplementedWarning_("csoundWaitBarrier");
    return 0;
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

#ifdef MSVC

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    (void) hinstDLL;
    (void) lpvReserved;
    switch ((int) fdwReason) {
    case (int) DLL_PROCESS_ATTACH:
      InitializeCriticalSection(&csound_global_lock_);
      csound_global_lock_init_done_ = 1;
      break;
    case (int) DLL_PROCESS_DETACH:
      csound_global_lock_init_done_ = 0;
      DeleteCriticalSection(&csound_global_lock_);
      break;
    }
    return TRUE;
}

#endif
