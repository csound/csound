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
#include "memory.h"
#include "sndfile.h"
#include "util.h"

#ifndef _CSOUNDTABLE_H
#define _CSOUNDTABLE_H

namespace dvx {

class CsoundTable
{
public:
	CsoundTable(t_object *o, CSOUND *c) : m_obj(o), m_csound(c), m_data(NULL), m_num(-1) {}
	~CsoundTable() {}

	t_object *m_obj;
	CSOUND *m_csound; // Do not delete/free! Data pointed to is allocated by Csound.
	MYFLT* m_data;    // Do not delete/free! Data pointed to is allocated by Csound.
	int m_num;
	int m_channels;
	int m_samples;
	int m_frames;
	
	// Return true if num exists, false otherwise.
	static bool TableExists(CSOUND *csound, int num);

	// Return true if num exists, false otherwise.
	bool TableExists(int num);

	// Return true if num exists, false otherwise.  Fills m_data with audio data if true.
	bool GetTable(int num);

	/*
	   If num doesn't exist, it will be created.  
	   If num exists, resize == true, and new size is different power of 2, then it will be replaced.
	   If sizeTime == 0, then audio data will be read from offsetTime until end of file or until Csound table is full.
   
	   Return values:
		   0 on success.
		  -1 if file cannot be opened/found.
		   1 if offset is greater than length of file.
		   2 on failure to read audio data from file.
	*/   
	int LoadAudioFile(char *filename, int num, int channel, int resize, float offsetTime, float sizeTime);

	/*
	   Similar to CsoundTable_LoadAudioFile() in functionality, except this function reads audio data from MSP buffer~'s.
	   A negative offsetTime or sizeTime means that time is in frames.
	   A positive offsetTime or sizeTime means that time is in seconds.

	   Since there is no way to tell how many channels a Csound table has:
	   -> Assume every Csound table has only 1 channel.
	   -> sourceChannel range is {1, # of channels in buffer~}
	   -> If sourceChannel == 0, then assume Csound table has same # of channels as buffer~, then copy all channels.
   
	   Return values:
			0 on success.
		   -1 if no [buffer~] with name bufferSymbol exists.
			1 if offset is greater than length of [buffer~].
			2 if could not create csound table.
	*/ 
	int ReadBufferTilde(t_symbol *bufferSymbol, int num, int sourceChannel, int resize, float offsetTime, float sizeTime);

	/*
	   Copies audio data from an MSP buffer~ to a Csound table.
	   A negative offsetTime or sizeTime means that time is in frames.
	   A positive offsetTime or sizeTime means that time is in seconds.
  
	   Since there is no way to tell how many channels a Csound table has:
	   -> Assume every Csound table has only 1 channel.
	   -> targetChannel range is {1, # of channels in buffer~}
	   -> If sourceChannel == 0, then assume Csound table has same # of channels as buffer~, then copy all channels.
	*/ 
	int WriteBufferTilde(t_symbol *bufferSymbol, int num, int targetChannel, float offsetTime, float sizeTime);

	// Returns 0 on success, 1 if table doesn't exist, 2 if index out of bounds.
	int Get(int num, int index, MYFLT *value);

	// Returns 0 on success, 1 if table doesn't exist, 2 if index out of bounds.
	int Set(int num, int index, MYFLT value);
};

} // namespace ct

#endif