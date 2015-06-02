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

#ifndef _MESSAGE_BUFFER_H
#define _MESSAGE_BUFFER_H

#include "includes.h"
#include "definitions.h"
#include "Lock.h"
#include <boost/ptr_container/ptr_deque.hpp>

namespace dvx {

class message
{
public:
	enum { _NORMAL_MSG = 0, _WARNING_MSG, _ERROR_MSG };

	message(int type, const std::string & s) : m_type(type), m_string(s) {}
	message(int type, const char * s) : m_type(type), m_string(s) {}
	~message() {}

	int m_type;
	std::string m_string;
};

class message_buffer
{
	enum { _LIMIT = 100000 };
	
public:
	message_buffer(t_object *o);
	~message_buffer() {}

	void add(int type, const string & s);
	void add(int type, const char * s);
	void addv(int type, const char * s, ... );
	void post();

private:
	t_object *m_obj;
	boost::ptr_deque<message> m_q;
	DEFAULT_LOCK_TYPE m_lock;

#ifdef _DEBUG
	size_t m_largest_size; // Check this at end of debugging session to see if DEFAULT_CAPACITY is reasonable.
#endif
};

} // namespace dvx

#endif //_MESSAGE_BUFFER_H