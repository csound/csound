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
#if defined(__MACH__)
#include <CoreServices/CoreServices.h>
#endif

#include "CsoundTable.h"
#include "buffer.h"	// Leave this out of CsoundTable.h; it causes conflicts with boost/max header combo.

using namespace dvx;

bool CsoundTable::GetTable(int num)
{
	MYFLT *table = NULL;
	
	m_samples = csoundGetTable(m_csound, &table, num);
	if(m_samples == -1) 
	{	
		m_data = NULL;
		return false; // Table with # == num does not exist.
	}

	m_data = table;       // Store the pointer to Csound table audio.
	m_num = num;
	m_channels = 1;       // Cannot get channel info yet; assume mono.
	m_frames = m_samples; // Cannot get channel info yet; assume # of frames == # of samples.
	return true;
}

bool CsoundTable::TableExists(CSOUND *csound, int num)
{
	MYFLT *table = NULL;
	return -1 != csoundGetTable(csound, &table, num);
}

bool CsoundTable::TableExists(int num)
{
	MYFLT *table = NULL;
	return -1 != csoundGetTable(m_csound, &table, num);
}

int CsoundTable::LoadAudioFile(char *filename, int num, int channel, int resize, float offsetTime, float sizeTime)
{
	char eventStr[MAX_EVENT_MESSAGE_SIZE];
	SF_INFO sf;
	SNDFILE *sf_ptr = NULL;
	int offsetFrames, sizeFrames, sizeSamples;
	int i, tmp, framesToRead, framesRead=0, result=0;
    
	if(!GetTable(num))
	{   // num doesn't exist.
		
		if((sf_ptr = sf_open(filename, SFM_READ, &sf)) == NULL)
		{
			object_error(m_obj, "Unable to open %s.", filename);
			return -1;
		}
		
		if(channel > 0 && channel > sf.channels) channel = sf.channels;
		offsetFrames = (int)(offsetTime * (float) sf.samplerate);
		if(offsetFrames >= sf.frames) 
		{
			object_error(m_obj, "Offset time is greater than size of audio file.");
			sf_close(sf_ptr);
			return 1;
		}
		
		if(sizeTime > 0.0f) 
		{
			// User has specified the amount of frames to read in seconds.
			// If necessary, change sizeFrames so that (offsetFrames + sizeFrames) <= sf.frames.
			sizeFrames = (int)(sizeTime * (float) sf.samplerate);
			if((offsetFrames + sizeFrames) > sf.frames) sizeFrames = sf.frames - offsetFrames;
		}
		else sizeFrames = sf.frames - offsetFrames; // User didn't specify amount of frames to read.  Read from offset to end of file.
		
		// Get next power of two greater than the total # of frames to read.
		if(channel > 0) m_samples = sizeFrames;
		else            m_samples = sizeFrames * sf.channels;
		m_samples = nextPowOf2(m_samples) + 1; // Add guard point for interpolating opcodes.
		
		snprintf(eventStr, MAX_EVENT_MESSAGE_SIZE-1, "f %d 0 %d 1 \"%s\" %f 0 %d", num, m_samples, filename, offsetTime, channel);
		csoundInputMessage(m_csound, eventStr);
		csoundPerformKsmps(m_csound); // Process one k-cycle to force table replacement now.
	}
	else // num exists
	{   
		if((sf_ptr = sf_open(filename, SFM_READ, &sf)) == NULL)
		{
			object_error(m_obj, "Unable to open %s.", filename);
			return -1;
		}
		
		if(channel > 0 && channel > sf.channels) channel = sf.channels;
		
		offsetFrames = (int)(offsetTime * (float) sf.samplerate);
		if(offsetFrames >= sf.frames) 
		{
			object_error(m_obj, "Offset time is greater than size of audio file.");
			sf_close(sf_ptr);
			return 1;
		}
		
		if(sizeTime > 0.0f) 
		{
			// User has specified the amount of frames to read in seconds.
			// If necessary, change sizeFrames so that (offsetFrames + sizeFrames) <= sf.frames.
			sizeFrames = (int)(sizeTime * (float) sf.samplerate);
			if((offsetFrames + sizeFrames) > sf.frames) sizeFrames = sf.frames - offsetFrames;
		}
		else sizeFrames = sf.frames - offsetFrames; // User didn't specify amount of frames to read.  Read from offset to end of file.
		
		if(channel == 0) sizeSamples = sizeFrames * sf.channels;
		else             sizeSamples = sizeFrames;
		
		if(resize)
		{
			// Replace existing table.
			sizeSamples = nextPowOf2(sizeSamples) + 1; 
			snprintf(eventStr, MAX_EVENT_MESSAGE_SIZE-1, "f %d 0 %d 1 \"%s\" %f 0 %d", num, sizeSamples, filename, offsetTime, channel);
			csoundInputMessage(m_csound, eventStr);
			csoundPerformKsmps(m_csound); // Process one k-cycle to force table replacement now.
		}
		else
		{	
			try 
			{	// Just read in as many samples as possible into existing table.

				boost::scoped_ptr<MYFLT> scoped_ptr_buffer;
				MYFLT *buffer = NULL;
			
				if(channel > 0 && sizeFrames > m_samples)        framesToRead = m_samples;
				else if(channel == 0 && sizeSamples > m_samples) framesToRead = m_samples / sf.channels;
				else                                             framesToRead = sizeFrames;
		
				// Create a temporary buffer for audio data.
				scoped_ptr_buffer.reset(new MYFLT[framesToRead * sf.channels]); 
				buffer = scoped_ptr_buffer.get();
		
				// Seek into the file by offsetFrames.
				sf_seek(sf_ptr, offsetFrames, SEEK_SET);
			
				#ifdef USE_DOUBLE
					framesRead = sf_readf_double(sf_ptr, (double*)buffer, framesToRead);
				#else
					framesRead = sf_readf_float(sf_ptr, buffer, framesToRead);
				#endif		
				if(framesRead != framesToRead) 
				{
					object_error(m_obj, "Reading audio data from %s failed.", filename);
					result = 2;
				}
				else
				{
					memset(m_data, 0, sizeof(MYFLT) * m_samples); // Zero out the Csound table.
			
					if(channel == 0) 
						// Copy all channels as interleaved data.
						memcpy(m_data, buffer, sizeof(MYFLT) * framesRead * sf.channels);
					else
					{
						tmp = channel-1;
						for(i=0; i<framesRead; i++)
							// Copy only the specified channel's data.
							m_data[i] = buffer[i*sf.channels + tmp];
					}
				}	
			}
			catch(std::bad_alloc & ex)
			{
				object_error(m_obj, "CsoundTable::LoadAudioFile() : %s", ex.what());
			}
		}
	}
	sf_close(sf_ptr);
	return result;
}

int CsoundTable::WriteBufferTilde(t_symbol *bufferSymbol, int num, int targetChannel, float offsetTime, float sizeTime)
{
	int i, tmp, offsetFrames, offsetSamples, sizeFrames;
	float *buffer = NULL;
	t_buffer *b = NULL;   // Pointer to a MSP buffer~ obje
	static t_symbol *ps_buffer = NULL;
	static bool firstTime = true;
	long saveinuse;
	MYFLT csoundSR = csoundGetSr(m_csound);
	
	if(firstTime)
	{
		firstTime = false;
		ps_buffer = gensym("buffer~");
	}
	
	// Get a pointer to the buffer~ object with name bufferSymbol.
	if(!((b = (t_buffer *)(bufferSymbol->s_thing)) && ob_sym(b) == ps_buffer && b->b_valid == true))
	{
		object_error(m_obj, "No buffer~ with name \"%s\" exists.", bufferSymbol->s_name);
		return -1;
	}
	
	// Get a pointer to the Csound table corresponding to num.	
	if(!GetTable(num)) 
	{
		object_error(m_obj, "Csound table #%d does not exist.", num);
		return -1;
	}
	
	// Mark the buffer~ as "in use".
	saveinuse = b->b_inuse;
	b->b_inuse = true;
	buffer = b->b_samples;
	
	if(targetChannel < 0 || targetChannel > b->b_nchans)
	{
		object_error(m_obj, "buffer~ object %s doesn't have a channel #%d.", bufferSymbol->s_name, targetChannel);
		b->b_inuse = saveinuse;
		return -1;
	}
	
	// If offsetTime or sizeTime is positive, then they are in seconds.  Otherwise, they are in frames.
	
	if(offsetTime >= 0) offsetFrames = (float)((MYFLT)offsetTime * csoundSR);
	else                offsetFrames = (int)-offsetTime;
	
	if(sizeTime >= 0) sizeFrames = (float)((MYFLT)sizeTime * csoundSR);
	else              sizeFrames = (int)-sizeTime;	
	
	if(offsetFrames >= m_frames) 
	{
		object_error(m_obj, "Offset time is greater than size of Csound table #%d.", num);
		b->b_inuse = saveinuse;
		return 1;
	}
	
	if(targetChannel == 0)
	{	// Copying all channels. Assume that the Csound table has the same # of 
		// channels as the buffer~.
		m_channels = b->b_nchans;
		m_frames = m_samples / m_channels;
	}
	else
	{
		m_channels = 1;
		m_frames = m_samples;
	}
	
	offsetSamples = offsetFrames * m_channels;

	if(sizeFrames == 0 || ((offsetFrames + sizeFrames) > m_frames))
		// User didn't specify amount of frames to read.
		// Read from offset to end of file.
		sizeFrames = m_frames - offsetFrames; 

	// If sizeFrames is greater than the # of frames in the target buffer~,
	// then sizeFrames = target buffer~'s frame size.
	if(sizeFrames > b->b_frames) sizeFrames = b->b_frames; 
	
	// Copy the audio data.
	if(targetChannel == 0)
		for(i=0; i<(sizeFrames * b->b_nchans); i++)
			buffer[i] = m_data[i + offsetSamples];
	else
	{
		tmp = targetChannel-1;
		for(i=0; i<sizeFrames; i++)
			buffer[i*b->b_nchans + tmp] = m_data[i + offsetFrames];
	}
	
	b->b_inuse = saveinuse; // Release the buffer~ object.
	return 0;
}


int CsoundTable::ReadBufferTilde(t_symbol *bufferSymbol, int num, int sourceChannel, int resize, float offsetTime, float sizeTime)
{
	char eventStr[MAX_EVENT_MESSAGE_SIZE];
	int offsetFrames, sizeFrames;
	int i, size, tmp;
	float *buffer = NULL;
	t_buffer *b = NULL; // Pointer to a MSP buffer~ obje
	static t_symbol *ps_buffer = NULL;
	static bool firstTime = true;
	long saveinuse;
	
	if(firstTime)
	{
		firstTime = false;
		ps_buffer = gensym("buffer~");
	}
	
	// Get a pointer to the buffer~ object with name bufferSymbol.
	if(!((b = (t_buffer *)(bufferSymbol->s_thing)) && ob_sym(b) == ps_buffer && b->b_valid == true))
	{
		object_error(m_obj, "No buffer~ with name \"%s\" exists.", bufferSymbol->s_name);
		return -1;
	}
	
	// Get a pointer to the Csound table corresponding to num.	
	if(!GetTable(num))
	{   // num doesn't exist.
		saveinuse = b->b_inuse;
		b->b_inuse = true;
		buffer = b->b_samples;
		if(sourceChannel > 0 && sourceChannel > b->b_nchans) sourceChannel = b->b_nchans;
		if(sourceChannel > 0) size = b->b_frames;
		else                  size = b->b_frames * b->b_nchans;
		
		// If offsetTime or sizeTime is positive, then they are in seconds.  Otherwise, they are in frames.
		if(offsetTime >= 0)	offsetFrames = (int)(offsetTime * (float) b->b_sr);
		else                offsetFrames = (int)-offsetTime;
		if(offsetFrames >= b->b_frames) 
		{
			object_error(m_obj, "Offset time is greater than size of buffer~ %s.", bufferSymbol->s_name);
			b->b_inuse = saveinuse;
			return 1;
		}
		
		if(sizeTime == 0.0f) 
			// User didn't specify amount of frames to read.  Read from offset to end of file.
			sizeFrames = b->b_frames - offsetFrames; 
		else if(sizeTime < 0.0f)
		{
			sizeFrames = (int)-sizeTime; // sizeTime is < 0.  User specified amount of frames to read. 
			if((offsetFrames + sizeFrames) > b->b_frames)
				// Too many frames, read from offset to end of file.
				sizeFrames = b->b_frames - offsetFrames;
		}
		else if(sizeTime > 0.0f)
		{
			// User has specified the amount of frames to read in seconds.
			// If necessary, change sizeFrames so that (offsetFrames + sizeFrames) <= sf.frames.
			sizeFrames = (int)(sizeTime * (float) b->b_sr);
			if((offsetFrames + sizeFrames) > b->b_frames)
				sizeFrames = b->b_frames - offsetFrames;
		}
		
		// Get next power of two greater than the total # of frames to read.
		if(sourceChannel > 0) m_samples = sizeFrames;
		else                  m_samples = sizeFrames * b->b_nchans;
		m_samples = nextPowOf2(m_samples) + 1; // Add guard point for interpolating opcodes.
		
		snprintf(eventStr, MAX_EVENT_MESSAGE_SIZE-1, "f %d 0 %d 7 0 %d 0", num, m_samples, m_samples);
		csoundInputMessage(m_csound, eventStr);
		csoundPerformKsmps(m_csound); // We need to process one k-cycle to force table creation now.
		
		// num should exist now; let's get a pointer to it.
		if(GetTable(num))
		{
			// Let's copy the audio data into the Csound table.
			if(sourceChannel == 0) 
			{
				// Copy all channels as interleaved data.
				tmp = offsetFrames * b->b_nchans;
				for(i=0; i<(sizeFrames * b->b_nchans); i++) m_data[i] = buffer[i+tmp]; 
			}
			else
			{
				// Copy only the specified channel's data.
				tmp = sourceChannel-1+(offsetFrames * b->b_nchans);
				for(i=0; i<sizeFrames; i++) m_data[i] = buffer[i*b->b_nchans + tmp]; 
			}
		}
		b->b_inuse = saveinuse;
	}
	else // num exists
	{   
		saveinuse = b->b_inuse;
		b->b_inuse = true;
		buffer = b->b_samples;
		if(sourceChannel > 0 && sourceChannel > b->b_nchans) sourceChannel = b->b_nchans;
		if(sourceChannel > 0) size = b->b_frames;
		else                  size = b->b_frames * b->b_nchans;
		
		if(offsetTime >= 0)	offsetFrames = (int)(offsetTime * (float) b->b_sr);
		else                offsetFrames = (int)-offsetTime;
		if(offsetFrames >= b->b_frames) 
		{
			object_error(m_obj, "Offset time is greater than size of buffer~ %s.", bufferSymbol->s_name);
			b->b_inuse = saveinuse;
			return 1;
		}
		
		if(sizeTime == 0.0f) 
			// User didn't specify amount of frames to read.  Read from offset to end of file.
			sizeFrames = b->b_frames - offsetFrames;
		else if(sizeTime < 0.0f)
		{
			sizeFrames = (int)-sizeTime;
			if((offsetFrames + sizeFrames) > b->b_frames)
				sizeFrames = b->b_frames - offsetFrames;
		}
		else if(sizeTime > 0.0f) 
		{
			// User has specified the amount of frames to read in seconds.
			// If necessary, change sizeFrames so that (offsetFrames + sizeFrames) <= sf.frames.
			sizeFrames = (int)(sizeTime * (float) b->b_sr);
			if((offsetFrames + sizeFrames) > b->b_frames)
				sizeFrames = b->b_frames - offsetFrames;
		}
		
		if(sourceChannel > 0) size = sizeFrames;
		else                  size = sizeFrames * b->b_nchans;
		
		if(resize)
		{
			// Replace the table with one that is the same size as the amount of data we want to copy.
			size = nextPowOf2(size) + 1; 
			snprintf(eventStr, MAX_EVENT_MESSAGE_SIZE-1, "f %d 0 %d 7 0 %d 0", num, size, size);
			csoundInputMessage(m_csound, eventStr);
			csoundPerformKsmps(m_csound); // We need to process one k-cycle to force table replacement now.
			
			// num has been replaced; let's get a pointer to the new table.
			if(!GetTable(num)) { b->b_inuse = saveinuse; return 2; }
		}
		else if(size > m_samples)
		{
			size = m_samples;
			if(sourceChannel > 0) sizeFrames = m_samples;
			else                  sizeFrames = m_samples / b->b_nchans;
		}

		// Let's copy the audio data into the Csound table.
		if(sourceChannel == 0) 
		{
			// Copy all channels as interleaved data.
			tmp = offsetFrames * b->b_nchans;
			for(i=0; i<(sizeFrames * b->b_nchans); i++) m_data[i] = buffer[i+tmp]; 
		}
		else
		{
			// Copy only the specified channel's data.
			tmp = sourceChannel-1+(offsetFrames * b->b_nchans);
			for(i=0; i<sizeFrames; i++) m_data[i] = buffer[i*b->b_nchans + tmp]; 
		}
		b->b_inuse = saveinuse;
	}
	return 0;
}

int CsoundTable::Get(int num, int index, MYFLT *value)
{
	if(!GetTable(num)) return 1;
	if(index < m_samples) 
		*value = m_data[index];
	else
		return 2;
	return 0;
}

int CsoundTable::Set(int num, int index, MYFLT value)
{	
	if(!GetTable(num)) return 1;
	if(index < m_samples) 
		m_data[index] = value;
	else
		return 2;
	return 0;
}
