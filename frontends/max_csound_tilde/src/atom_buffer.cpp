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

#include "atom_buffer.h"

using namespace dvx;

atom_buffer::atom_buffer(int tuple_size) : 
	m_tuple_size(tuple_size), m_capacity(DEFAULT_CAPACITY), m_size(0)
{
	memset(m_buffer, 0, sizeof(t_atom) * m_capacity);
#ifdef _DEBUG
	m_largest_size = 0;
#endif
}

bool atom_buffer::add(int n)
{	
	if(full()) return false;
	atom_setlong(&m_buffer[m_size++], n);
	return true;
}

bool atom_buffer::add(double d)
{	
	if(full()) return false;
	atom_setfloat(&m_buffer[m_size++], (float)d);
	return true;
}

bool atom_buffer::add(float f)
{	
	if(full()) return false;
	atom_setfloat(&m_buffer[m_size++], f);
	return true;
}

bool atom_buffer::add(t_symbol *s)
{
	if(full()) return false;
	atom_setsym(&m_buffer[m_size++], s);
	return true;
}

bool atom_buffer::add(const char *s)
{
	if(full()) return false;
	atom_setsym(&m_buffer[m_size++], gensym((char*)s)); // Must force cast to char*.
	return true;
}

void atom_buffer::send_tuples(void * outlet)
{
	for(int i=0; i<=(m_size-m_tuple_size); i += m_tuple_size)
		outlet_list(outlet, 0L, m_tuple_size, &m_buffer[i]);

#ifdef _DEBUG
	m_largest_size = (m_largest_size < m_size ? m_size : m_largest_size);
#endif

	m_size = 0; // Discard any unfinished tuples.
}