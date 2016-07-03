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
#include "csound.h"
#include "Lock.h"
#include "memory.h"
#include "atom_buffer.h"

#ifndef _CHANNEL_H
#define _CHANNEL_H

using namespace std;
extern int MaxChannelStringLength;
typedef enum { IN_ = 0, OUT_ = 1 } Direction;

namespace dvx {

class Sequencer;

class ChannelObject
{
	friend class ChannelGroup;
	friend class Sequencer;

public:
	~ChannelObject() {}

	bool operator<(const ChannelObject& e) const { return m_name < e.m_name; }
	bool operator>(const ChannelObject& e) const { return m_name > e.m_name; }
	bool operator<=(const ChannelObject& e) const { return m_name <= e.m_name; }
	bool operator>=(const ChannelObject& e) const { return m_name >= e.m_name; }
	bool operator==(const ChannelObject& e) const { return m_name == e.m_name; }

	bool IsInputChannel() const;
	bool IsOutputChannel() const;
	bool IsControlChannel() const;            
	bool IsStringChannel() const;

	// Set m_value (and csoundChanPtr if it's not NULL) to val.
	void SetVal(MYFLT val);

	// Get m_value.
	inline MYFLT GetVal() const { return m_value; }                    
	
	// Set the string stored at m_csoundChanPtr = str.
	void SetStringVal(const char *str);

	// Set m_str = string stored at m_csoundChanPtr.
	void GetStringVal();

	// Make m_value and m_csoundChanPtr (or m_str_value and m_csoundChanPtr) consistent.
	void SyncVal(Direction d);

private:
	// Hide ctors, and assignment to prevent messing up ChannelGroup state.
	ChannelObject() {}
	ChannelObject(CSOUND* csound, const char *name, int type);
	ChannelObject(const ChannelObject & other) {}
	ChannelObject & operator=(const ChannelObject & other) { return *this; }

	// Change the name of this ChannelObject.
	void SetName(const char *str);
  CSOUND *m_csound;
  string m_name;           // The name of this channel.
  int m_type;		   // Bitwise or of Csound channel types.
  MYFLT m_value;          // If type & CSOUND_CONTROL_CHANNEL, then values is stored here.
  string m_str_value;      // If type & CSOUND_STRING_CHANNEL, then string will be stored here.
  bool m_dirty;          // Determines whether or not *csoundChanPtr should be set to m_value.
  MYFLT *m_csoundChanPtr; // A pointer to the actual Csound channel.
  t_symbol *m_sym;           // Since gensym() is expensive, store symbol version of m_name here.

}; // class ChannelObject


class ChannelGroup
{
	friend class Sequencer;

public:
	ChannelGroup(t_object *o, CSOUND *c, Direction d);
	~ChannelGroup();

	/* Clear each ChannelObject's Csound channel ptr.
	   m_lock is locked during this function. */
	void ClearPtrs();

	/* Find a ChannelObject and set dest to ChannelObject's value. 
	   If ChannelObject doesn't exist, set dest = 0.0 and return false.
	   m_lock is locked during this function. */
	bool GetVal(const char *name, MYFLT *dest);

	/* Create ChannelObjects for every channel returned from csoundListChannels().
	   If a channel is declared inside an instr (but not with chn_k), then instr must be 
	   active for 1 k-cycle in order to be in the list returned by csoundListChannels().
	   For each ChannelObject co in m_channels, retrieve its corresponding Csound channel ptr
	   and store it in co->m_csoundChanPtr.  Caller is responsible for checking that m_csound
	   is compiled and the performance has not finished.
	   m_lock is locked during this function. */
	void GetPtrs();

	/* Finds or create a ChannelObject with name and sets it's value.
	   Returns true on success.
       m_lock is locked during this function. */
	bool SetVal(const char *name, int type, MYFLT val);

	/* Finds or create a ChannelObject with name and sets it's string.
	   Returns true on success.
       m_lock is locked during this function. */
	bool SetVal(const char *name, int type, const char *str);

	/* Finds or create a ChannelObject with name and sets to val.
	   Will try to obtain a Csound channel ptr for ChannelObject.
	   If Csound channel ptr is obtained, Csound channel is set to val as well.
	   Returns true on success (even if Csound channel was not found/created).
	   Caller must take care to call this only if Csound instance is compiled
	   and performance is not finished.
	   m_lock is locked during this function. */
	bool SetValAndSync(const char *name, int type, MYFLT val);

	/* Finds or creates a ChannelObject with name and sets to str.
	   Will try to obtain a Csound channel ptr for ChannelObject.
	   If Csound channel ptr is obtained, Csound channel is set to str as well.
	   Returns true on success (even if Csound channel was not found/created).
	   Caller must take care to call this only if Csound instance is compiled
	   and performance is not finished.
	   m_lock is locked during this function. */
	bool SetValAndSync(const char *name, int type, const char *str);

	/* These enums are used by ProcessDirtyChannels() to determine which
	   atom_buffer to write to creating atoms for dirty channels.  Each
	   thread that can call ProcessDirtyChannels() gets its own atom_buffer.
	   If we used one atom_buffer, we'd have to protect it with locks. */
	enum { AUDIO_THREAD, CLOCK_THREAD };

	/* Create atom tuples for every dirty ChannelObject and add the tuples to atom_buffer.
	   thread should be set to either AUDIO_THREAD or CLOCK_THREAD.
	   Call SendDirtyChannels() afterwards.
	   m_lock is locked during this function. */
	void ProcessDirtyChannels(int thread);

	void SendDirtyChannels(void * outlet, int thread);
	
	void PrintChannels(void * outlet);

private:
	ChannelGroup() {} // Hide default ctor.
	ChannelGroup(const ChannelGroup & other) {} // Hide copy.
	ChannelGroup & operator=(const ChannelGroup & other) { return *this; } // Hide assignment.

	/* Find and return ChannelObject if it exists.  If not, create and return it.
	   Not protected. */
	ChannelObject* FindCreateChannel(CSOUND *csound, const char *name, int type);

	/* Find and return ChannelObject if it exists.  If not, return NULL.
	   Not thread safe. Keep it that way, because this function is used by other
	   Not protected. */
	ChannelObject* FindChannel(const char *name);

	boost::ptr_set<ChannelObject> m_channels;
	Direction m_direction;    // Determines whether this is an input or output ChannelGroup.
	t_object *m_obj;          // Used when calling object_post() or object_error().
	CSOUND *m_csound;         // Pointer to parent CsoundObject's CSOUND instance.
	DEFAULT_LOCK_TYPE m_lock; // All public member functions lock/unlock this lock.
	ChannelObject m_cmp_co;   // Used for comparison purposes only (allocated once for efficiency);
	atom_buffer m_audio_thread_atom_buffer; // Used when sending dirty output control channel messages in audio thread.
	atom_buffer m_clock_thread_atom_buffer; // Used when sending dirty output control channel messages in clock thread.

}; // class ChannelGroup

} // namespace dvx

#endif