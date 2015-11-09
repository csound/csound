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

#include "Lock.h"

using namespace dvx;

abstract_lock::abstract_lock(char *name)
{ 
	if(name != NULL)
		#ifdef _WINDOWS
			//strncpy_s(m_name, name, MAXI_STR_LEN-1);
			strncpy(m_name, name, MAXI_STR_LEN-1);
		#else
			strncpy(m_name, name, MAXI_STR_LEN-1);
		#endif
	else
		m_name[0] = '\0';

	m_locked = false; 
	m_context[0] = '\0';
}

void abstract_lock::setName(char *name)
{
	strncpy(m_name, name, MAXI_STR_LEN-1);
}

#ifdef _DEBUG

	void abstract_lock::log_lock(char *context)
	{	
	#ifdef _WINDOWS	
		m_locked = true;
		m_thread_id = GetCurrentThreadId();
		if(context != NULL) 
			strncpy(m_context, context, MAXI_STR_LEN-1);
		else
			sprintf(m_context, "no context");
	#else
		m_locked = true;
		if(context != NULL) 
			strncpy(m_context, context, MAXI_STR_LEN-1);
		else
			sprintf(m_context, "no context");
	#endif
	}

	void abstract_lock::log_unlock(char *context)
	{	
		if(m_locked == false)
		{
			if(context != NULL)
				error("Attempt to unlock an unlocked lock. Context = %s", context);
			else
				error("Attempt to unlock an unlocked lock. Context = none");
		}
		else
		{
			m_locked = false;
			m_thread_id = 0;
			m_context[0] = '\0';
		}
	}

#endif // _DEBUG

/*****************************************************************************************/
/*****************************************************************************************/

#ifdef MACOSX

spinlock::spinlock(char *name) : abstract_lock(name)
{
	m_slock = OS_SPINLOCK_INIT;
}

spinlock::~spinlock()
{
}

void spinlock::lock(char *context)
{
	OSSpinLockLock(&m_slock);
	#ifdef _DEBUG
		log_lock(context);
	#endif
}

void spinlock::unlock(char *context)
{
	#ifdef _DEBUG
		log_unlock(context);
	#endif
	OSSpinLockUnlock(&m_slock);
}

#endif

#ifdef _WINDOWS

spinlock::spinlock(char *name) : abstract_lock(name)
{
	// Must align arguments to InterlockedCompareExchange() on 32-bit boundary.
	m_long = (LONG*) _aligned_malloc(sizeof(LONG),32);
	m_0 = (LONG*) _aligned_malloc(sizeof(LONG),32);
	m_1 = (LONG*) _aligned_malloc(sizeof(LONG),32);
	*m_long = 0L;
	*m_0 = 0L;
	*m_1 = 1L;
}

spinlock::~spinlock()
{
	_aligned_free( (void*) m_long );
	_aligned_free( (void*) m_0 );
	_aligned_free( (void*) m_1 );
}

void spinlock::lock(char *context)
{
	while(InterlockedCompareExchange(m_long, *m_1, *m_0) != *m_0); // test and test and set
	#ifdef _DEBUG
		log_lock(context);
	#endif
}

void spinlock::unlock(char *context)
{
	#ifdef _DEBUG
		log_unlock(context);
	#endif
	InterlockedExchange(m_long, *m_0);
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

wspinlock::wspinlock(char *name) : abstract_lock(name)
{
	InitializeCriticalSectionAndSpinCount(&m_critical_section, MAX_SPIN_COUNT);
}

wspinlock::~wspinlock()
{
	DeleteCriticalSection(&m_critical_section);
}

void wspinlock::lock(char *context)
{
	EnterCriticalSection(&m_critical_section);
	#ifdef _DEBUG
		log_lock(context);
	#endif
}

void wspinlock::unlock(char *context)
{
	#ifdef _DEBUG
		log_unlock(context);
	#endif
	LeaveCriticalSection(&m_critical_section);
}

#endif // _WINDOWS
