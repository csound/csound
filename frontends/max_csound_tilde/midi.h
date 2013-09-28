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

#include "definitions.h"
#include "memory.h"
#include "Lock.h"
#include <vector>
#include <boost/circular_buffer.hpp>

#ifndef _MIDI_H
#define _MIDI_H

#define MAX_MIDI_MESSAGE_SIZE 3

namespace dvx {

// A circular byte buffer.
class MidiBuffer
{
public:
	MidiBuffer(t_object * o);
	virtual ~MidiBuffer() {}

	enum { BUFFER_SIZE = 256 };

public:
	/* Empty the buffer and reset the active note matrix. */
	void Clear();

	/* Try to get complete message with length <= n. 
	   Returns length of message if available, or 0 on failure. 
	   Locks/Unlocks m_lock. */
	int DequeueCompleteMessage(byte* buf, int n);

	inline bool Empty() const { return m_buffer.empty(); }

	/* When m_sysex == true, any data bytes recieved through this function
	   will not be added to the buffer.  If 0xf0 (begin sysex data) is received,
       m_sysex is set to true.  If 0xf7 (end sysex data) is received, m_sysex
       is set to false. 
	   Locks/Unlocks m_lock. */
	void Enqueue(byte b, bool lock = true);

	/* Enqueue n bytes from b into m_buffer.  b should contain only 1 complete MIDI message. 
	   Locks/Unlocks m_lock. */
	void EnqueueBuffer(byte* b, int n, bool lock = true);
	
	/* Stop active notes by enqueuing note-off's. 
	   Locks/Unlocks m_lock. */
	void Flush();

	inline bool Full() const { return m_buffer.full(); }
	
private:

	/* Dequeue n bytes into b from mb->buffer. 
	   Does not lock/unlock m_lock.  Does not check m_buffer's size. */
	void DequeueBuffer(byte* b, int n); 

	/* Dequeue a single byte from the buffer. 
	   Does not lock/unlock m_lock.  Does not check m_buffer's size. */
	byte Dequeue();

	boost::circular_buffer<byte> m_buffer; // Contains the MIDI bytes.
	bool m_sysex; // Used to filter out sysex data.
	DEFAULT_LOCK_TYPE m_lock;  // Protects the buffer for thread safety.
	t_object *m_obj;
	byte m_activeNoteMatrix[16][128];
};

} // namespace dvx

#endif // _MIDI_H