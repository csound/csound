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

#ifndef _MEMORY_H
#define _MEMORY_H

inline void* MemoryNew(int size)
{
	return malloc(size);
}

inline void* MemoryNewClear(int size)
{
	return calloc(1, size);
}

inline void MemoryFree(void *ptr)
{ 
	assert(ptr);
	free(ptr);
}

// Write len bytes from src to buffer at count offset.  If needed, grow buffer and
// update bufferSize variable.
int BufferWrite(byte **buffer, const void *src, int len, int *count, int *bufferSize);

#endif // _MEMORY_H