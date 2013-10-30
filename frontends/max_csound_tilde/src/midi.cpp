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

#include "midi.h"

using namespace dvx;

MidiBuffer::MidiBuffer(t_object * o) : m_buffer(BUFFER_SIZE), m_sysex(false), m_lock((char*)"MidiBuffer"), m_obj(o)
{
	memset(m_activeNoteMatrix, 0, MIDI_MATRIX_SIZE);
}

void MidiBuffer::Clear()
{
	ScopedLock lock(m_lock);
	m_buffer.clear();
	m_sysex = false;
	memset(m_activeNoteMatrix, 0, MIDI_MATRIX_SIZE);
}

byte MidiBuffer::Dequeue()
{
	byte b = m_buffer.front();
	m_buffer.pop_front();
	return b;
}

void MidiBuffer::DequeueBuffer(byte* b, int n)
{		
	for(int i=0; i<n; i++)
	{
		b[i] = m_buffer.front();
		m_buffer.pop_front();
	}
}

int MidiBuffer::DequeueCompleteMessage(byte* buf, int n)
{
	byte b, status;
	ScopedLock lock(m_lock);

	while(!m_buffer.empty())
	{
		b = m_buffer.front(); // Peek into buffer to get next status byte (don't dequeue).
		status = b & 0xf0;
		switch(status)
		{
		case 0x80: // Note-off (3 bytes)
			if(n >= 3 && m_buffer.size() >= 3)
			{
				DequeueBuffer(buf,3);
				m_activeNoteMatrix[buf[0] & 0x0f][buf[1]] = 0;
				return 3;
			}
			else return 0;
		case 0x90: // Note-on (3 bytes)
			if(n >= 3 && m_buffer.size() >= 3)
			{
				DequeueBuffer(buf,3);
				++m_activeNoteMatrix[buf[0] & 0x0f][buf[1]];
				return 3;
			}
			else return 0;
		case 0xA0: // Aftertouch (3 bytes)
		case 0xE0: // Pitch Bend (3 bytes)
		case 0xB0: // Control Change (3 bytes)
			if(n >= 3 && m_buffer.size() >= 3)
			{
				DequeueBuffer(buf,3);
				return 3;
			}
			else return 0;
		case 0xC0: // Program Change (2 bytes)
		case 0xD0: // Channel Aftertouch (2 bytes)
			if(n >= 2 && m_buffer.size() >= 2)
			{
				DequeueBuffer(buf,2);
				return 2;
			}
			else return 0;
		default: // Status unrecognized.
			Dequeue(); // Discard the current byte.
			//break;
		}	
	}
	return 0;
}

void MidiBuffer::Enqueue(byte b, bool lock)
{
	ScopedLock k(m_lock,lock);
	if(!m_sysex && b != 0xf0 && !m_buffer.full()) { m_buffer.push_back(b); return; }
	if(b == 0xf0)
		m_sysex = true;
	else if(b == 0xf7)
		m_sysex = false;

	if(m_buffer.full()) 
		object_warn(m_obj, "MidiBuffer is Full!");
}

void MidiBuffer::EnqueueBuffer(byte* b, int n, bool lock)
{
	byte status = 0;
		
	status = b[0] & 0xf0;
	switch(status)
	{
	case 0x80: // Note-off (3 bytes)
	case 0x90: // Note-on (3 bytes)
	case 0xA0: // Aftertouch (3 bytes)
	case 0xB0: // Control Change (3 bytes)
	case 0xE0: // Pitch Bend (3 bytes)
		if(3 != n) return;
		break;
	case 0xC0: // Program Change (2 bytes)
	case 0xD0: // Channel Aftertouch (2 bytes)
		if(2 != n) return;
		break;
	default:
		return; // Don't add disallowed types of MIDI messages.
	}	
	
	ScopedLock k(m_lock,lock);
	
	// If there isn't enough space for all bytes stored in b, then don't enqueue anything.
	if(size_t(n) <= (m_buffer.capacity() - m_buffer.size()))
		for(int i=0; i<n; i++)
			m_buffer.push_back(b[i]);
	else
		object_warn(m_obj, "MidiBuffer is Full!");
}

void MidiBuffer::Flush()
{
	byte buf[3];
	buf[2] = 64;

	ScopedLock lock(m_lock);

	for(byte c=0; c<16; c++)
		for(byte p=0; p<128; p++)
			if(m_activeNoteMatrix[c][p])
			{
				buf[0] = 0x80 | c;
				buf[1] = p;
				if(!Full())
				{
					EnqueueBuffer(buf,3,false);
					m_activeNoteMatrix[c][p] = 0;
				}
			}
}
