#if defined(WIN32)
#include "csound.h"
#include "cs.h"
#include <windows.h>
#include <portaudio.h>

PUBLIC void *csoundCreateThread(void *csound, int (*threadRoutine)(void *userdata), void *userdata)
{
    return (void *) _beginthread((void (*)(void *)) threadRoutine, 
        (unsigned int)0, userdata);
}
        
PUBLIC int csoundJoinThread(void *csound, void *thread)
{
    WaitForSingleObject(thread, INFINITE);
}

PUBLIC void *csoundCreateThreadLock(void *csound)
{
	return (void *)CreateEvent(0, 0, 0, 0);
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

#elif defined(LINUX)

#include <pthread.h>

void *csoundCreateThread(void *csound, 
    int (*threadRoutine)(void *userdata), void *userdata)
{
	pthread_t *pthread = 0;
	if(!pthread_create(pthread, 0, 
        (void *(*) (void*)) threadRoutine, userdata)) {
		return pthread;
	} else {
		return 0;
	}
}
        
int csoundJoinThread(void *csound, void *thread)
{
    pthread_t *pthread = (pthread_t *)thread;
    int threadRoutineReturnValue = 0;
    int pthreadReturnValue = pthread_join(pthread, &threadRoutineReturnValue);
    if pthreadReturnValue {
        return returnValue;
    } else {
        return pthreadReturnValue;
    }
}

void *csoundCreateThreadLock()
{
	pthread_mutex_t *pthread_mutex;
	if(pthread_mutex_init(pthread_mutex, 0) == 0) {
		return pthread_mutex;
	} else {
		return 0;
	}
}

void csoundWaitThreadLock(void *lock, size_t milliseconds)
{
	pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
	int returnValue = pthread_mutex_lock(pthread_mutex);
}

void csoundNotifyThreadLock(void *lock)
{
	pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
	int returnValue = pthread_mutex_unlock(pthread_mutex);
}

void csoundDestroyThreadLock(void *lock)
{
	pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
	int returnValue = pthread_mutex_destroy(pthread_mutex);
}

#else

PUBLIC void *csoundCreateThread(void *csound, int (*threadRoutine)(void *userdata), void *userdata)
{
    csoundMessage(csound, "csoundCreateThread is not implemented on this platform.\n");
    return 0;
}
        
PUBLIC int csoundJoinThread(void *csound, void *thread)
{
    csoundMessage(csound, "csoundJoinThread is not implemented on this platform.\n");
    return 0;
}

#endif

int csoundStreamCallback(const void *input, void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    ENVIRON *csound = (ENVIRON *)userData;
    float *floatInput = (float *)input;
    float *floatOutput = (float *)output;
    MYFLT *spin_ = csound->GetSpin(csound);
    MYFLT *spout_ = csound->GetSpout(csound);
    int ksmps_ = csound->GetKsmps(csound);
    int frameIndex;
    int spinIndex;
    int spoutIndex;
    int keepPerforming;
    for(frameIndex = 0; frameIndex < frameCount; 
        frameIndex += ksmps_, floatInput += ksmps_, floatOutput += ksmps_) {
        for(spinIndex = 0; spinIndex < ksmps; spinIndex++) {
            spin_[spinIndex] = floatInput[spinIndex];
        }     
        keepPerforming = csound->PerformKsmps(csound);
        for(spoutIndex = 0; spoutIndex < ksmps; spoutIndex++) {
            floatOutput[spoutIndex] = spout_[spoutIndex];
        }
        csoundYield(csound);
        if(!keepPerforming) {
            return paComplete;
        }
    }
    return paContinue;
}



