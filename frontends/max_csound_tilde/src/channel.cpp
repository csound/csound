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

#include "channel.h"

using namespace std;
using namespace dvx;
extern int MaxChannelStringLength;

ChannelObject::ChannelObject(CSOUND* csound, const char *name, int type) :
  m_csound(csound),
	m_name(name), 
	m_type(type), 
	m_value(FL(0.0)), 
	m_str_value(),
	m_dirty(true),
	m_csoundChanPtr(NULL), 
	m_sym(gensym((char*)m_name.c_str()))
{
}

bool ChannelObject::IsInputChannel() const { return m_type & CSOUND_INPUT_CHANNEL; }
bool ChannelObject::IsOutputChannel() const { return m_type & CSOUND_INPUT_CHANNEL; }
bool ChannelObject::IsControlChannel() const
{
	return (m_type & CSOUND_CHANNEL_TYPE_MASK) == CSOUND_CONTROL_CHANNEL;
}
bool ChannelObject::IsStringChannel() const
{
	return (m_type & CSOUND_CHANNEL_TYPE_MASK) == CSOUND_STRING_CHANNEL;
}

void ChannelObject::SetVal(MYFLT val)
{
	m_value = val;
	if(m_csoundChanPtr)
		*m_csoundChanPtr = m_value;
	m_dirty = true;
}

void ChannelObject::SetStringVal(const char *str)
{	
	assert(str);
	m_str_value = str;
	if(m_csoundChanPtr)
    csoundSetStringChannel(m_csound, m_name.c_str(), (char*)str);
	m_dirty = true;
}

void ChannelObject::GetStringVal()
{
  if(m_csoundChanPtr) {
    int strsize = csoundGetChannelDatasize(m_csound, m_name.c_str());
    char* data = (char*)malloc(strsize + 1);
    csoundGetStringChannel(m_csound, m_name.c_str(), data);
    m_str_value = data;
    free(data);
  }
}

void ChannelObject::SyncVal(Direction d)
{
	if(!m_csoundChanPtr) return;
	if(IN_ == d)
	{
		if(IsControlChannel())
			*m_csoundChanPtr = m_value;
		else if(IsStringChannel())
      SetStringVal(m_str_value.c_str());
	}
	else if(OUT_ == d)
	{
		if(IsControlChannel())
			m_value = *m_csoundChanPtr;
		else if(IsStringChannel())
      GetStringVal();
		m_dirty = true;
	}
}

void ChannelObject::SetName(const char *str)
{
	assert(str);
	m_name = str;
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------


ChannelGroup::ChannelGroup(t_object *o, CSOUND *c, Direction d) : 
	m_channels(), m_direction(d), m_obj(o), m_csound(c), m_lock((char*)"ChannelGroup"), m_cmp_co(c, (char*)"", 0),
	m_audio_thread_atom_buffer(2),
	m_clock_thread_atom_buffer(2)
{
}

ChannelGroup::~ChannelGroup()
{
	ScopedLock lock(m_lock);
	m_channels.clear();		
}

bool ChannelGroup::SetVal(const char *name, int type, MYFLT val)
{
	ChannelObject *co = NULL;
	ScopedLock lock(m_lock);
	co = FindCreateChannel(m_csound, name, type);
	if(co)
	{ 
		co->SetVal(val); return true;
	}
	return false;
}

bool ChannelGroup::SetVal(const char *name, int type, const char *str)
{
	ChannelObject *co = NULL;
	ScopedLock lock(m_lock);
	co = FindCreateChannel(m_csound, name, type);
	if(co)
	{
    csoundSetStringChannel(m_csound, name, (char*)str);
    return true;
	}
	return false;
}

bool ChannelGroup::SetValAndSync(const char *name, int type, MYFLT val)
{
	ChannelObject *co = NULL;
	ScopedLock lock(m_lock);
	co = FindCreateChannel(m_csound, name, type);
	if(co)
	{ 
		if(NULL == co->m_csoundChanPtr)
			csoundGetChannelPtr(m_csound, &co->m_csoundChanPtr, co->m_name.c_str(), co->m_type);
			
		co->SetVal(val); return true;
	}
	return false;
}

bool ChannelGroup::SetValAndSync(const char *name, int type, const char *str)
{
	ChannelObject *co = NULL;
	ScopedLock lock(m_lock);
  co = FindCreateChannel(m_csound, name, type);
	if(co)
	{
    csoundSetStringChannel(m_csound, name, (char*)str);
    return true;
  }
	return false;
}

bool ChannelGroup::GetVal(const char *name, MYFLT *dest)
{
	ChannelObject *co = NULL;
	ScopedLock lock(m_lock);

	co = FindChannel(name);
	if(co && co->IsControlChannel()) 
	{
		*dest = co->GetVal();
		return true;
	}
	else *dest = FL(0.0);

	return false;
}

void ChannelGroup::ProcessDirtyChannels(int thread)
{
	boost::ptr_set<ChannelObject>::iterator it;
	ChannelObject *co = NULL;
	atom_buffer *b = NULL;

	switch(thread)
	{
	case AUDIO_THREAD: b = &m_audio_thread_atom_buffer; break;
	case CLOCK_THREAD: b = &m_clock_thread_atom_buffer; break;
	default: return;
	}

	{
		ScopedLock lock(m_lock);

		for(it = m_channels.begin(); it != m_channels.end(); it++)
		{
			co = &*it;
			if(co->IsControlChannel())
			{
				/* If this ChannelObject has a legitimate pointer to a Csound channel, and the value
				   of that channel != m_value, then mark it as dirty and set m_value = to that val. */
				if(co->m_csoundChanPtr && *co->m_csoundChanPtr != co->m_value)
				{
					co->m_dirty = true;
					co->m_value = *co->m_csoundChanPtr;
				}
				// Only output channel's value if ChannelObject is dirty.
				if(co->m_dirty)
				{
					co->m_dirty = false;
					b->add(co->m_sym);
					b->add(co->m_value);
				}
			}
			else if(co->IsStringChannel())
			{
				/* If this ChannelObject has a legitimate pointer to a Csound channel, and the value
				   of that channel != m_value, then mark it as dirty and set m_value = to that val. */
				if(co->m_csoundChanPtr && strncmp((char*)co->m_csoundChanPtr, co->m_str_value.c_str(), MaxChannelStringLength-1) != 0)
				{
					co->m_dirty = true;
					co->GetStringVal();
				}	
				// Only output channel's value if ChannelObject is dirty.
				if(co->m_dirty)
				{
					co->m_dirty = false; 
					b->add(co->m_sym);
					b->add(co->m_str_value.c_str());
				}
			}
		}
	}
}

void ChannelGroup::SendDirtyChannels(void * outlet, int thread)
{
	atom_buffer *b = NULL;

	switch(thread)
	{
	case AUDIO_THREAD: b = &m_audio_thread_atom_buffer; break;
	case CLOCK_THREAD: b = &m_clock_thread_atom_buffer; break;
	default: return;
	}

	b->send_tuples(outlet);
}

void ChannelGroup::GetPtrs()
{
	int i, result, numCsoundChannels;
	ChannelObject *co = NULL;
	controlChannelInfo_t *csoundChanList = NULL;
	ScopedLock lock(m_lock);
	bool channel_object_exists = false;

	/* Get pointers using the list returned by csoundListChannels().
	   A ChannelObject will be created for every Csound channel 
	   without a corresponding ChannelObject. */

	numCsoundChannels = csoundListChannels(m_csound, &csoundChanList);
	if(CSOUND_MEMORY == numCsoundChannels) 
	{
		object_error(m_obj, "csoundListChannels() failed: Not enough memory.");
		return;
	}

	for(i=0; i<numCsoundChannels; i++)
	{
		if((csoundChanList[i].type & CSOUND_CONTROL_CHANNEL) == CSOUND_CONTROL_CHANNEL)
		{
			channel_object_exists = false;
			co = FindChannel(csoundChanList[i].name);
			if(co) 
				channel_object_exists = true;
			else
				co = FindCreateChannel(m_csound, csoundChanList[i].name, csoundChanList[i].type);
			if(co)
			{
				result = csoundGetChannelPtr(m_csound, &co->m_csoundChanPtr, co->m_name.c_str(), co->m_type);
				if(0 != result)
					object_error(m_obj, "Unable to get Csound control channel: \"%s\"", co->m_name.c_str());				
				
				// Don't sync output channels; let ProcessDirtyChannels do that.
				if(IN_ == m_direction)
				{
					if(channel_object_exists)
					{
						co->SyncVal(IN_); // Use ChannelObject as source.
					}
					else
					{
						co->SyncVal(OUT_); // Use Csound channel as source.
					}
				}
			}
		}
	}
		
	/* Create new Csound channels for all ChannelObjects that didn't have
	   a corresponding Csound channel in the list returned by csoundListChannels(). */

	boost::ptr_set<ChannelObject>::iterator it;
	
	for(it = m_channels.begin(); it != m_channels.end(); it++)
	{
		co = &*it;
		if(co->m_csoundChanPtr == NULL)
		{
			result = csoundGetChannelPtr(m_csound, &co->m_csoundChanPtr, co->m_name.c_str(), co->m_type);
						
			if(0 == result)
				co->SyncVal(m_direction);
			else
				object_error(m_obj, "Unable to create a Csound control channel: \"%s\"", co->m_name.c_str());
		}
	}

	csoundDeleteChannelList(m_csound, csoundChanList);
}

void ChannelGroup::ClearPtrs()
{
	boost::ptr_set<ChannelObject>::iterator it;
	ScopedLock lock(m_lock);

	for(it = m_channels.begin(); it != m_channels.end(); it++)
		it->m_csoundChanPtr = NULL;
}

// Find and return ChannelObject if it exists.  If not, create and return it.
ChannelObject* ChannelGroup::FindCreateChannel(CSOUND* csound, const char *name, int type)
{
	boost::ptr_set<ChannelObject>::iterator it;
	ChannelObject *co = NULL;
		
	m_cmp_co.SetName(name);
	it = m_channels.find(m_cmp_co);

	try
	{
		if(it == m_channels.end())
		{
			co = new ChannelObject(csound, name, type);
			m_channels.insert(co);
		}
		else co = &*it;
	}
	catch(std::bad_alloc & ex)
	{
		object_error(m_obj, "%s", ex.what());
	}
	
	return co;
}

// Find and return ChannelObject if it exists.  If not, return NULL.
ChannelObject* ChannelGroup::FindChannel(const char *name)
{
	boost::ptr_set<ChannelObject>::iterator it;
	ChannelObject *co = NULL;
		
	m_cmp_co.SetName(name);
	it = m_channels.find(m_cmp_co);
	if(it != m_channels.end()) co = &*it;
	return co;
}


void ChannelGroup::PrintChannels(void * outlet)
{
	boost::ptr_set<ChannelObject>::iterator it;
	ScopedLock lock(m_lock);
	t_atom b[2];

	if(m_direction == IN_)
	{
		atom_setsym(&b[0], gensym("IN"));
	}
	else
	{
		atom_setsym(&b[0], gensym("OUT"));
	}
	outlet_list(outlet, 0L, 1, b);

	for(it = m_channels.begin(); it != m_channels.end(); it++)
	{
		if(it->IsControlChannel())
		{
			atom_setsym(&b[0], it->m_sym);
			atom_setfloat(&b[1], it->m_value);
		}
		else if(it->IsStringChannel())
		{
			atom_setsym(&b[0], it->m_sym);
			atom_setsym(&b[1], gensym((char*)it->m_str_value.c_str()));
		}
		outlet_list(outlet, 0L, 2, b);
	}
}
