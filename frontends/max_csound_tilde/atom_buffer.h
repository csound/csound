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

#ifndef _ATOM_BUFFER_H
#define _ATOM_BUFFER_H

#include "includes.h"
#include "definitions.h"

namespace dvx {

class atom_buffer
{
	enum { DEFAULT_CAPACITY = 512 };
public:
	atom_buffer(int tuple_size = 1);
	~atom_buffer() {}

	bool add(int n);
	bool add(double d);
	bool add(float f);
	bool add(t_symbol *s);
	bool add(const char *s);
	void send_tuples(void * outlet);

	inline int full() { return m_size == m_capacity; }
	inline int tuple_size() { return m_tuple_size; }
	
private:
	int m_tuple_size;
	int m_capacity;
	int m_size;
	t_atom m_buffer[DEFAULT_CAPACITY];

#ifdef _DEBUG
	int m_largest_size; // Check this at end of debugging session to see if DEFAULT_CAPACITY is reasonable.
#endif
};

} // namespace dvx

#endif // _ATOM_BUFFER_H