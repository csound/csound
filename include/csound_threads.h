/*
  cs_threads.h:cross-platform threads interface

  Copyright (C) 2024

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

#ifndef CS_THREADS_H
#define CS_THREADS_H

#ifdef __cplusplus
extern "C" {
#endif


  /** @defgroup THREADING Threading and concurrency
   *
   *  @{ */

  /**
   * Called by external software to set a function for checking system
   * events, yielding cpu time for coopertative multitasking, etc.
   * This function is optional. It is often used as a way to 'turn off'
   * Csound, allowing it to exit gracefully. In addition, some operations
   * like utility analysis routines are not reentrant and you should use
   * this function to do any kind of updating during the operation.
   * Returns an 'OK to continue' boolean.
   */
  PUBLIC void csoundSetYieldCallback(CSOUND *, int32_t (*yieldCallback_)(CSOUND *));

  /**
   * Creates and starts a new thread of execution.
   * Returns an opaque pointer that represents the thread on success,
   * or NULL for failure.
   * The userdata pointer is passed to the thread routine.
   */
  PUBLIC void *csoundCreateThread(uintptr_t (*threadRoutine)(void *),
                                  void *userdata);

  /**
   * Creates and starts a new thread of execution
   * with a user-defined stack size.
   * Returns an opaque pointer that represents the thread on success,
   * or NULL for failure.
   * The userdata pointer is passed to the thread routine.
   */
  PUBLIC void *csoundCreateThread2(uintptr_t (*threadRoutine)(void *),
                                   uint32_t stack,
                                   void *userdata);

  /**
   * Returns the ID of the currently executing thread,
   * or NULL for failure.
   *
   * NOTE: The return value can be used as a pointer
   * to a thread object, but it should not be compared
   * as a pointer. The pointed to values should be compared,
   * and the user must free the pointer after use.
   */
  PUBLIC void *csoundGetCurrentThreadId(void);

  /**
   * Waits until the indicated thread's routine has finished.
   * Returns the value returned by the thread routine.
   */
  PUBLIC uintptr_t csoundJoinThread(void *thread);
  /**
   * Creates and returns a monitor object, or NULL if not successful.
   * The object is initially in signaled (notified) state.
   */
  PUBLIC void *csoundCreateThreadLock(void);

  /**
   * Waits on the indicated monitor object for the indicated period.
   * The function returns either when the monitor object is notified,
   * or when the period has elapsed, whichever is sooner; in the first case,
   * zero is returned.
   * If 'milliseconds' is zero and the object is not notified, the function
   * will return immediately with a non-zero status.
   */
  PUBLIC int32_t csoundWaitThreadLock(void *lock, size_t milliseconds);

  /**
   * Waits on the indicated monitor object until it is notified.
   * This function is similar to csoundWaitThreadLock() with an infinite
   * wait time, but may be more efficient.
   */
  PUBLIC void csoundWaitThreadLockNoTimeout(void *lock);

  /**
   * Notifies the indicated monitor object.
   */
  PUBLIC void csoundNotifyThreadLock(void *lock);

  /**
   * Destroys the indicated monitor object.
   */
  PUBLIC void csoundDestroyThreadLock(void *lock);

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
  PUBLIC void *csoundCreateMutex(int32_t isRecursive);

  /**
   * Acquires the indicated mutex object; if it is already in use by
   * another thread, the function waits until the mutex is released by
   * the other thread.
   */
  PUBLIC void csoundLockMutex(void *mutex_);

  /**
   * Acquires the indicated mutex object and returns zero, unless it is
   * already in use by another thread, in which case a non-zero value is
   * returned immediately, rather than waiting until the mutex becomes
   * available.
   * Note: this function may be unimplemented on Windows.
   */
  PUBLIC int32_t csoundLockMutexNoWait(void *mutex_);

  /**
   * Releases the indicated mutex object, which should be owned by
   * the current thread, otherwise the operation of this function is
   * undefined. A recursive mutex needs to be unlocked as many times
   * as it was locked previously.
   */
  PUBLIC void csoundUnlockMutex(void *mutex_);

  /**
   * Destroys the indicated mutex object. Destroying a mutex that
   * is currently owned by a thread results in undefined behavior.
   */
  PUBLIC void csoundDestroyMutex(void *mutex_);


  /**
   * Create a Thread Barrier. Max value parameter should be equal to
   * number of child threads using the barrier plus one for the
   * master thread */

  PUBLIC void *csoundCreateBarrier(uint32_t max);

  /**
   * Destroy a Thread Barrier.
   */
  PUBLIC int32_t csoundDestroyBarrier(void *barrier);

  /**
   * Wait on the thread barrier.
   */
  PUBLIC int32_t csoundWaitBarrier(void *barrier);


  /** Creates a conditional variable */
  PUBLIC void* csoundCreateCondVar();

  /** Waits up on a conditional variable and mutex */
  PUBLIC void csoundCondWait(void* condVar, void* mutex);

  /** Signals a conditional variable */
  PUBLIC void csoundCondSignal(void* condVar);

  /** Destroys a conditional variable */
  PUBLIC void csoundDestroyCondVar(void* condVar);

  /**
   * If the spinlock is not locked, lock it and return;
   * if is is locked, wait until it is unlocked, then lock it and return.
   * Uses atomic compare and swap operations that are safe across processors
   * and safe for out of order operations,
   * and which are more efficient than operating system locks.
   * Use spinlocks to protect access to shared data, especially in functions
   * that do little more than read or write such data, for example:
   *
   * @code
   * static spin_lock_t lock = SPINLOCK_INIT;
   * csoundSpinLockInit(&lock);
   * void write(size_t frames, int32_t* signal)
   * {
   *   csoundSpinLock(&lock);
   *   for (size_t frame = 0; i < frames; frame++) {
   *     global_buffer[frame] += signal[frame];
   *   }
   *   csoundSpinUnlock(&lock);
   * }
   * @endcode
   */
  PUBLIC int32_t csoundSpinLockInit(spin_lock_t *spinlock);

  
  /**
   * Locks the spinlock
   */
  PUBLIC void csoundSpinLock(spin_lock_t *spinlock);

  /**
   * Tries the lock, returns CSOUND_SUCCESS if lock could be acquired,
      CSOUND_ERROR, otherwise.
   */
  PUBLIC int32_t csoundSpinTryLock(spin_lock_t *spinlock);

  /**
   * Unlocks the spinlock
   */
  PUBLIC void csoundSpinUnLock(spin_lock_t *spinlock);

 /** @}*/

#ifdef __cplusplus
}
#endif

#endif
