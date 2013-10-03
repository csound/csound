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

#include "memory.h"

int BufferWrite(byte **buffer, const void *src, int len, int *count, int *bufferSize)
{
	void *dst = NULL;
	
	// Grow the buffer if buffer is over half full.
	if(*count + len > (*bufferSize / 2)) 
	{
		*bufferSize *= 2;
		*buffer = (byte*) realloc(*buffer, *bufferSize);
		if(*buffer == NULL) 
		{
			post("BufferWrite():  realloc() failed.");
			return 1;
		}
	}
	
	// Figure out where in buffer to write to.
	dst = *buffer + *count;
	
	// Copy src to buffer.
	memcpy(dst, src, len);
	
	// Update the byte count (i.e. buffer offset).
	*count += len;
		
	return 0;
}