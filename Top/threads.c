#include "csound.h"
#include "cs.h"
#include "prototyp.h"

#if defined(WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#include <portaudio.h>

PUBLIC void *csoundCreateThread(void *csound,
                                int (*threadRoutine)(void *userdata),
                                void *userdata)
{
    return (void *) _beginthread((void (*)(void *)) threadRoutine,
                                 (unsigned int)0, userdata);
}

PUBLIC int csoundJoinThread(void *csound, void *thread)
{
    WaitForSingleObject(thread, INFINITE);
    return 0;
}

PUBLIC void *csoundCreateThreadLock(void *csound_)
{
    ENVIRON *csound = (ENVIRON *)csound_;
    HANDLE threadLock = CreateEvent(0, 0, 0, 0);
    if (!threadLock) {
      csound->Message(csound,
                      "csoundCreateThreadLock: Failed to create thread lock.\n");
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

void csoundLock(void)
{
}

void csoundUnLock(void)
{
}

#elif defined(LINUX) || defined(__CYGWIN__) || defined(__MACH__)

#include <pthread.h>

void *csoundCreateThread(void *csound,
                         int (*threadRoutine)(void *userdata), void *userdata)
{
    pthread_t pthread = 0;
    if (!pthread_create(&pthread, 0,
                       (void *(*) (void*)) threadRoutine, userdata)) {
      return (void *)pthread;
    } else {
      return 0;
    }
}

int csoundJoinThread(void *csound, void *thread)
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

void *csoundCreateThreadLock(void *csound)
{
    pthread_mutex_t *pthread_mutex =
      (pthread_mutex_t *)mmalloc(csound, sizeof(pthread_mutex_t));
    if (pthread_mutex_init(pthread_mutex, 0) == 0) {
      return (void *)pthread_mutex;
    } else {
      return 0;
    }
}

void csoundWaitThreadLock(void *csound, void *lock, size_t milliseconds)
{
    pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
    /*int returnValue = */pthread_mutex_lock(pthread_mutex);
}

void csoundNotifyThreadLock(void *csound, void *lock)
{
    pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
    /*int returnValue = */pthread_mutex_unlock(pthread_mutex);
}

void csoundDestroyThreadLock(void *csound, void *lock)
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
                  "csoundDestroyThreadLock is not implemented on this platform.\n");
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

