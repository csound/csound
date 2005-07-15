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

#include "csoundCore.h"

#if defined(WIN32) && !defined(__CYGWIN__)
#include <windows.h>

PUBLIC void *csoundCreateThread(void *csound,
                                int (*threadRoutine)(void *userdata),
                                void *userdata)
{
    return (void *) _beginthread((void (*)(void *)) threadRoutine,
                                 (unsigned int) 0, userdata);
}

PUBLIC int csoundJoinThread(void *csound, void *thread)
{
    WaitForSingleObject(thread, INFINITE);
    return 0;
}

PUBLIC void *csoundCreateThreadLock(void *csound_)
{
    ENVIRON *csound = (ENVIRON *)csound_;
    HANDLE threadLock = CreateEvent(0, 0, TRUE, 0);
    if (!threadLock) {
      csound->Message(csound, "csoundCreateThreadLock: "
                              "Failed to create thread lock.\n");
    }
    return (void *)threadLock;
}

PUBLIC void csoundWaitThreadLock(void *csound, void *lock, size_t milliseconds)
{
    WaitForSingleObject((HANDLE) lock, milliseconds);
}

PUBLIC void csoundNotifyThreadLock(void *csound, void *lock)
{
    SetEvent((HANDLE) lock);
}

PUBLIC void csoundDestroyThreadLock(void *csound, void *lock)
{
    CloseHandle((HANDLE) lock);
}

/* internal functions for csound.c */

static  HANDLE  cs_mutex = (HANDLE) 0;

void csoundLock(void)
{
    if (cs_mutex == (HANDLE) 0)
      cs_mutex = CreateEvent(0, 0, 0, 0);
    else
      WaitForSingleObject(cs_mutex, 10000);
}

void csoundUnLock(void)
{
    if (cs_mutex != (HANDLE) 0)
      SetEvent(cs_mutex);
}

#elif defined(LINUX) || defined(__CYGWIN__) || defined(__MACH__)

#include <pthread.h>

PUBLIC void *csoundCreateThread(void *csound,
                                int (*threadRoutine)(void *userdata),
                                void *userdata)
{
    pthread_t pthread = 0;
    if (!pthread_create(&pthread, 0,
                       (void *(*) (void*)) threadRoutine, userdata)) {
      return (void *)pthread;
    }
    return NULL;
}

PUBLIC int csoundJoinThread(void *csound, void *thread)
{
    pthread_t pthread = (pthread_t)thread;
    void *threadRoutineReturnValue = NULL;
    int pthreadReturnValue = pthread_join(pthread, &threadRoutineReturnValue);
    if (pthreadReturnValue) {
      return *((int*) threadRoutineReturnValue);
    } else {
      return pthreadReturnValue;
    }
}

PUBLIC void *csoundCreateThreadLock(void *csound)
{
    pthread_mutex_t *pthread_mutex =
      (pthread_mutex_t *)mmalloc(csound, sizeof(pthread_mutex_t));
    if (pthread_mutex_init(pthread_mutex, 0) == 0) {
      return (void *)pthread_mutex;
    }
    return NULL;
}

PUBLIC void csoundWaitThreadLock(void *csound, void *lock, size_t milliseconds)
{
    pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
    /*int returnValue = */pthread_mutex_lock(pthread_mutex);
}

PUBLIC void csoundNotifyThreadLock(void *csound, void *lock)
{
    pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
    /*int returnValue = */pthread_mutex_unlock(pthread_mutex);
}

PUBLIC void csoundDestroyThreadLock(void *csound, void *lock)
{
    pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
    /*int returnValue = */pthread_mutex_destroy(pthread_mutex);
    mfree(csound, lock);
}

/* internal functions for csound.c */

static  pthread_mutex_t cs_mutex = PTHREAD_MUTEX_INITIALIZER;

void csoundLock(void)
{
    pthread_mutex_lock(&cs_mutex);
}

void csoundUnLock(void)
{
    pthread_mutex_unlock(&cs_mutex);
}

#else

PUBLIC void *csoundCreateThread(void *csound,
                                int (*threadRoutine)(void *userdata),
                                void *userdata)
{
    csoundMessage(csound,
                  "csoundCreateThread is not implemented on this platform.\n");
    return 0;
}

PUBLIC int csoundJoinThread(void *csound, void *thread)
{
    csoundMessage(csound,
                  "csoundJoinThread is not implemented on this platform.\n");
    return 0;
}

PUBLIC void *csoundCreateThreadLock(void *csound)
{
    csoundMessage(csound,
                  "csoundCreateThreadLock is not implemented on this platform.\n");
    return NULL;
}

PUBLIC void csoundWaitThreadLock(void *csound, void *lock, size_t milliseconds)
{
    csoundMessage(csound,
                  "csoundWaitThreadLock is not implemented on this platform.\n");
    return;
}

PUBLIC void csoundNotifyThreadLock(void *csound, void *lock)
{
    csoundMessage(csound,
                  "csoundNotifyThreadLock is not implemented on this platform.\n");
    return;
}

PUBLIC void csoundDestroyThreadLock(void *csound, void *lock)
{
    csoundMessage(csound,
                  "csoundDestroyThreadLock is not implemented "
                  "on this platform.\n");
    return;
}

/* internal functions for csound.c */

void csoundLock(void)
{
}

void csoundUnLock(void)
{
}

#endif

