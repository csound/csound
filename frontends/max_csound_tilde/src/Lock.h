/*
    csound~ : A MaxMSP external interface for the Csound API.
    
    Created by Davis Pyon on 2/4/06.
    Copyright 2006-2010 Davis Pyon. All rights reserved.
    
    LICENSE AGREEMENT
    
    This software is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    
    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with this software; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "includes.h"
#include "definitions.h"
#include <pthread.h>

#ifdef _WINDOWS
#include <Windows.h>
#include <malloc.h>
#elif MACOSX
#include <libkern/OSAtomic.h>
#endif

#ifndef _LOCK_H
#define _LOCK_H

#ifdef _WINDOWS
	#define DEFAULT_LOCK_TYPE dvx::spinlock
#elif MACOSX
	#define DEFAULT_LOCK_TYPE dvx::spinlock
#endif

namespace dvx {

class abstract_lock
{
public:
	abstract_lock(char *name = NULL);
	virtual ~abstract_lock() {}

	void setName(char *name);
	virtual void lock(char *context = NULL) = 0;
//	virtual void trylock(char *context = NULL) = 0;
	virtual void unlock(char *context = NULL) = 0;

protected:
	// Hide copy ctor, and assignment operator.
	abstract_lock(const abstract_lock & other) {}
	abstract_lock & operator=(const abstract_lock & other) { return *this; }

#ifdef _DEBUG
	void log_lock(char *context);
	void log_unlock(char *context);
#endif

	enum { MAXI_STR_LEN = 128 };

	char m_name[MAXI_STR_LEN];
	bool m_locked;
	char m_context[MAXI_STR_LEN];
	DWORD m_thread_id;
};

// Uses pthread mutex.
class plock : public abstract_lock
{
public:
	plock(char *name = NULL);
	~plock();
	void lock(char *context = NULL);
	void trylock(char *context = NULL);
	void unlock(char *context = NULL);
private:
	plock(const plock & other) {}
	plock & operator=(const plock & other) { return *this; }

	pthread_mutex_t m_mutex;
};

#ifdef MACOSX
// Uses Mac OSAtomic type OSSpinLock.
class spinlock : public abstract_lock
{
public:
	spinlock(char *name = NULL);
	~spinlock();
	void lock(char *context = NULL);
	void trylock(char *context = NULL);
	void unlock(char *context = NULL);
private:
	spinlock(const spinlock & other) {}
	spinlock & operator=(const spinlock & other) { return *this; }
	
	OSSpinLock m_slock;
};
#endif

#ifdef _WINDOWS

// Uses Windows InterlockedCompareExchange() and InterlockedExchange().
class spinlock : public abstract_lock
{
public:
	spinlock(char *name = NULL);
	~spinlock();
	void lock(char *context = NULL);
//	void trylock(char *context = NULL);
	void unlock(char *context = NULL);
private:
	spinlock(const spinlock & other) {}
	spinlock & operator=(const spinlock & other) { return *this; }

	/* According to MSDN, all arguments to InterlockedCompareExchange() must occur on 32-bit boundary.
	   Otherwise, spinlock will behave unpredictably.  So, we have pointers to long integers
	   and we allocate with _aligned_malloc() and deallocate with _aligned_free().
	*/
	LONG * m_long;
	LONG * m_0;
	LONG * m_1;
};

// Uses pthread spinlocks (only available on Windows).
class pspinlock : public abstract_lock
{
public:
	pspinlock(char *name = NULL);
	~pspinlock();
	void lock(char *context = NULL);
	void unlock(char *context = NULL);
private:
	pspinlock(const pspinlock & other) {}
	pspinlock & operator=(const pspinlock & other) { return *this; }

	pthread_spinlock_t m_spinlock;
};

// Uses Windows critical section and InitializeCriticalSectionAndSpinCount() to emulate spinlock.
class wspinlock : public abstract_lock
{
public:
	wspinlock(char *name = NULL);
	~wspinlock();
	void lock(char *context = NULL);
	void unlock(char *context = NULL);
private:
	enum { MAX_SPIN_COUNT = 4000 }; // Used for WIN_CRITICAL_SECTION.

	wspinlock(const wspinlock & other) {}
	wspinlock & operator=(const wspinlock & other) { return *this; }

	CRITICAL_SECTION m_critical_section;
};

#endif // _WINDOWS	

class ScopedLock
{
public:
	ScopedLock(abstract_lock &lock, bool doIt = true) : m_lock(&lock), m_doIt(doIt)
	{ 
		if(m_doIt)
			m_lock->lock(NULL);
	}
	ScopedLock(abstract_lock &lock, char *context, bool doIt = true) : m_lock(&lock), m_doIt(doIt)
	{ 
		if(m_doIt)
			m_lock->lock(context);
	}
	~ScopedLock()
	{
		if(m_doIt)
			m_lock->unlock();
	}

protected:
	abstract_lock *m_lock;
	bool m_doIt;
};

} // namespace dvx

#endif