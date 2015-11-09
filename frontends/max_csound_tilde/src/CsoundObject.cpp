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

#include "CsoundObject.h"
#include "csound.h"
#include "csound~.h"
#include "memory.h" 
#include "util.h"

using namespace dvx;

CsoundObject::CsoundObject(t_csound *partner) :
	m_csound((CSOUND*) csoundCreate(partner)),
	m_x(partner),
	m_obj((t_object*)partner),
	m_lock((char*)"CsoundObject"),
	m_iChanGroup(m_obj, m_csound, IN_),
	m_oChanGroup(m_obj, m_csound, OUT_),
	m_midiBuffer(m_obj),
	m_scripter(m_obj, m_iChanGroup),
	m_sequencer(m_obj, m_csound, &m_iChanGroup, &m_midiBuffer),
	m_msg_buf(m_obj),
	m_sr(sys_getsr()),
	m_chans(2), m_inChans(2), m_outChans(2),
	m_ksmps(32),          // Set to non-zero to avoid modulo by zero (could happen if
	                      // csound_dsp() is called before compilation of a Csound orchestra).
	m_in_index(0),
	m_out_index(1000000), // Must init m_out_index to some impossibly large value.
	m_scale(FL(1.0)),
	m_oneDivScale(FL(1.0)),
	m_compiled(false),
	m_renderingToFile(false),
	m_performanceFinished(true),
	m_defaultPathID(path_getdefault()),
	m_defaultPath(),
	m_path(),
	m_args(m_obj),
	m_textBufferLock(),
	m_textBuffer(new char[MAX_STRING_LENGTH]),
	m_renderThreadExists(false)
{
	m_textBuffer.get()[0] = '\0'; // Must init to empty string for messageCallback().
//	csoundSetInputValueCallback(m_csound, inputValueCallback);
//	csoundSetOutputValueCallback(m_csound, outputValueCallback);
   	csoundSetInputChannelCallback(m_csound, inputValueCallback);
	csoundSetOutputChannelCallback(m_csound, outputValueCallback);
	csoundSetMessageCallback(m_csound, messageCallback);
}

CsoundObject::~CsoundObject()
{
	csoundDestroy(m_csound);
	m_msg_buf.post();
}				

void CsoundObject::Compile()
{
	int result = CSOUND_ERROR;

	// Stop the render thread if it exists.
	if(m_renderThreadExists) 
	{
		m_performanceFinished = false;
    
		if(0 != csoundJoinThread(m_renderThread))
			m_msg_buf.add(message::_ERROR_MSG, "csoundJoinThread() with render thread failed.");
		m_renderThreadExists = false;
	}

	/* Stop() clears Csound channel pointers for all ChannelObjects, and then it
	   calls csoundReset(), which kills off Csound channels. If we don't clear
	   ChannelObject Csound pointers, then when csound_outputClockCallback() calls
	   ProcessDirtyChannels, or user sends a "control" message, access to freed
	   memory will occur.*/
	Stop(); 

	result = Compile(true);

	if(result == COMPILATION_SUCCESS_MISMATCHED_SR && m_x->matchMaxSR && m_x->sr > 0)
	{
		Stop(); // We compiled above and got a new set of Csound channel pointers. Clear them.
		if(m_x->messageOutputEnabled)
			m_msg_buf.addv(message::_NORMAL_MSG, "Recompiling with sr = %d and ksmps = %d.", m_x->sr, m_ksmps);

		ScopedLock k(m_lock);
		m_args.RemoveSRandKR();
		m_args.AddSRandKR(m_x->sr,m_ksmps); // Add "-r" and "-k" flags to override Csound sr and kr.
		result = Compile(false);
	}
	
	if(result != COMPILATION_FAILURE) 
		outlet_bang(m_x->compiled_bang_outlet); // Send success bang outside any locked zones.
}

int CsoundObject::Compile(bool lock)
{
  int result = COMPILATION_FAILURE;
//	char *opcodedir = NULL;
//	string default_opcodedir;
	MYFLT *csIn = NULL;

	if(!m_args.ArgListValid())
	{
		m_msg_buf.add(message::_ERROR_MSG, "Can't compile. Please provide a valid csound message first.");
		return COMPILATION_FAILURE;
	}
	
	try
	{
		m_midiBuffer.Clear();
	
        // FIXME - look into setting this again, at least for Windows
//		#ifdef MACOSX
//			if(sizeof(MYFLT) == sizeof(float))
//				default_opcodedir = "/Library/Frameworks/CsoundLib.framework/Resources/Opcodes";
//			else
//				default_opcodedir = "/Library/Frameworks/CsoundLib64.framework/Versions/6.0/Resources/Opcodes64";
//		#elif _WINDOWS
//			if(sizeof(MYFLT) == sizeof(float))
//				default_opcodedir = "C:\\Program Files\\Csound\\plugins";
//			else
//				default_opcodedir = "C:\\Program Files\\Csound\\plugins64";
//		#endif
	
		{	
			// Begin locked section.
			ScopedLock k(m_lock,lock);

			m_compiled = false;

//			if(sizeof(MYFLT) == sizeof(float))
//			{
//				opcodedir = (char*) csoundGetEnv(NULL, "OPCODE6DIR");
//				if(!opcodedir && 0 != csoundSetGlobalEnv("OPCODE6DIR", default_opcodedir.c_str()))
//					m_msg_buf.addv(message::_ERROR_MSG, "csoundSetGlobalEnv(OPCODE6DIR,%s) failed.", default_opcodedir.c_str());
//			}
//			else
//			{
//				opcodedir = (char*) csoundGetEnv(NULL, "OPCODE6DIR64");
//				if(!opcodedir && 0 != csoundSetGlobalEnv("OPCODE6DIR64", default_opcodedir.c_str()))
//					m_msg_buf.addv(message::_ERROR_MSG, "csoundSetGlobalEnv(OPCODE6DIR64,%s) failed.", default_opcodedir.c_str());
//			}
		
			// Let the user know what arguments are being passed to csoundCompile().
			if(m_x->messageOutputEnabled) 
				m_msg_buf.add(message::_NORMAL_MSG, m_args.GetArgumentsAsString());
			
            #ifdef _WINDOWS
                Sleep(10); // Multiple compiles in quick successsion may crash without sleeping for a bit.
            #else
                usleep(10000);
            #endif

            // Cannot render to file if csoundSetHostImplementedAudioIO() is called.
            if(!m_renderingToFile) {
                csoundSetHostImplementedAudioIO(m_csound, 1, 0);
                csoundSetHostImplementedMIDIIO(m_csound, 1);
            }

            // These functions cannot be called until after csoundPreCompile()
            // has executed.  csoundPreCompile() allocates memory for an MGLOBAL
            // struct which contains the pointers to these functions.
            csoundSetExternalMidiInOpenCallback(m_csound, midiInOpenCallback);
            csoundSetExternalMidiInCloseCallback(m_csound, midiInCloseCallback);
            csoundSetExternalMidiReadCallback(m_csound, midiReadCallback);			
            csoundSetExternalMidiOutOpenCallback(m_csound, midiOutOpenCallback);
            csoundSetExternalMidiOutCloseCallback(m_csound, midiOutCloseCallback);
            csoundSetExternalMidiWriteCallback(m_csound, midiWriteCallback);

            // Set the current directory again (in case it was changed).
            SetCurDir();

            if(CSOUND_SUCCESS != csoundCompile(m_csound, m_args.NumArgs(), m_args.GetArray())) {
                m_msg_buf.add(message::_ERROR_MSG, "csoundCompile() failed.");
            } else {
                result = COMPILATION_SUCCESS;
                csoundSetHostData(m_csound, m_x);
                m_chans = csoundGetNchnls(m_csound);
                m_ksmps = csoundGetKsmps(m_csound);
                m_x->evenlyDivisible = (m_x->vectorSize % m_ksmps == 0);
    //            MaxChannelStringLength = csoundGetStrVarMaxLen(m_csound);
                MaxChannelStringLength = 256; //FIXME - not sure this is correct...
                m_compiled = true;
                m_in_index = 0;
                m_out_index = 1000000; // Must init to something larger than the largest possible Max vector size.
                m_performanceFinished = false;
                m_sr = csoundGetSr(m_csound);
                m_sequencer.SetSR(m_sr);
              
                if(m_x->sr != m_sr)
                {
                    result = COMPILATION_SUCCESS_MISMATCHED_SR;
                    if(!m_x->matchMaxSR && m_x->messageOutputEnabled)
                        m_msg_buf.addv(message::_WARNING_MSG, "Max sr (%d) != Csound sr (%d)", m_x->sr, m_sr);
                }

                m_scale = csoundGet0dBFS(m_csound);
                m_oneDivScale = 1.0 / m_scale;
            
                if(m_chans != m_x->numInSignals)
                    m_msg_buf.addv(message::_WARNING_MSG, "# of Csound audio channels (%d) != # of signal inlets (%d).", m_chans, m_x->numInSignals);
            
                if(m_chans != m_x->numOutSignals)
                    m_msg_buf.addv(message::_WARNING_MSG, "# of Csound audio channels (%d) != # of signal outlets (%d).", m_chans, m_x->numOutSignals);

                // If DSP is not active and bypass == 0, perform one k-cycle of the newly compiled
                // Csound instance to force table loading.
                if(!sys_getdspstate() && !m_x->bypass)
                {
                    m_iChanGroup.GetPtrs();                             // Sync channel values before running a k-cycle.
                    csIn = csoundGetSpin(m_csound);
                    memset(csIn, 0, sizeof(MYFLT) * m_ksmps * m_chans); // Fill the input buffers with zeros.
                    m_performanceFinished = csoundPerformKsmps(m_csound);
                }

                m_iChanGroup.GetPtrs();
                m_oChanGroup.GetPtrs();

                // Set inChans to the lesser of the two (x->chans and x->numInSignals).  Same for outChans.
                m_inChans = (m_chans < m_x->numInSignals ? m_chans : m_x->numInSignals);
                m_outChans = (m_chans < m_x->numOutSignals ? m_chans : m_x->numOutSignals);
            
                if(m_renderingToFile)
                {
                  
                    void *threadCreateResult = csoundCreateThread(CsoundObject_RenderThreadFunc, (void*)this);
                    if(threadCreateResult == NULL) m_msg_buf.add(message::_ERROR_MSG, "Could not create Csound render thread.");
                    else m_renderThreadExists = true;
                }
            }
		} // End locked section.
	}
	catch(std::exception & ex)
	{
		m_msg_buf.addv(message::_ERROR_MSG, "CsoundObject::Compile() : %s", ex.what());
		result = COMPILATION_FAILURE;
	}
	return result;
}

void CsoundObject::Perform()
{
	int i, j, chan, indexMultChans;
	int vectorSize = m_x->vectorSize;
	float **in = m_x->in, **out = m_x->out;
	MYFLT *csIn, *csOut;
	ScopedLock k(m_lock);

	if(m_compiled && !m_renderingToFile && !m_performanceFinished)
	{
		csOut = csoundGetSpout(m_csound);
		csIn = csoundGetSpin(m_csound);

		if(m_x->evenlyDivisible)
		{	/* ksmps evenly divides the current Max vector size. Keep filling csIn[].
			 * When csIn[] is full, process Csound.  Processing Csound will give us
			 * ksmps output frames stored in csOut[].  At the end of the for loop, if
			 * there are any frames in csOut[], copy them to the output buffer provided
			 * by Max.  Since vector_size % ksmps == 0, this results in latency = 0 samples. */
			j = 0;
			for(i=0; i<vectorSize; i++)
			{
				if(!m_performanceFinished)
				{
					indexMultChans = m_in_index * m_chans;
					for(chan=0; chan<m_inChans; chan++)
						csIn[indexMultChans + chan] = (MYFLT)in[chan][i] * m_scale; 
					
					if(++m_in_index == m_ksmps) 
					{	
						m_performanceFinished = csoundPerformKsmps(m_csound);
						m_in_index = 0;
						m_out_index = 0;
					}

					while(m_out_index < m_ksmps && j < vectorSize)
					{
						indexMultChans = m_out_index * m_chans;
						for(chan=0; chan<m_outChans; chan++)
							out[chan][j] = (t_float)(csOut[indexMultChans + chan] * m_oneDivScale); 
						++j;
						++m_out_index;
					}
				}
			}
		}
		else // x->evenlyDivisible == false
		{	/* ksmps does not evenly divide the current Max vector size. Here's a description of the loop: 
			 * We add a frame from the Max input vectors to csIn[], check to see if we have ksmps frames,
			 * if we have ksmps frames then process Csound, then copy a frame from csOut[] to the Max output
			 * vector (csOut[] may contain only zeros). This results in latency = ksmps. */
			for(i=0; i<vectorSize; i++)
			{
				if(!m_performanceFinished)
				{
					if(m_in_index == m_ksmps) 
					{	
						m_performanceFinished = csoundPerformKsmps(m_csound);
						m_in_index = 0;
					}
					indexMultChans = m_in_index * m_chans;
					for(chan=0; chan<m_inChans; chan++)
						csIn[indexMultChans + chan] = (MYFLT)in[chan][i] * m_scale;
					
					for(chan=0; chan<m_outChans; chan++)
						out[chan][i] = (t_float)(csOut[indexMultChans + chan] * m_oneDivScale);
					
					++m_in_index;
				}
			}
		}
		if(m_performanceFinished) 
		{
			m_iChanGroup.ClearPtrs();
			m_oChanGroup.ClearPtrs();
			defer_low(m_obj, (method)csound_sendPerfDoneBang, NULL, 0, NULL);
		}
	}
}


void CsoundObject::Perform64()
{
	int i, j, chan, indexMultChans;
	int vectorSize = m_x->vectorSize;
	double **in = m_x->in64, **out = m_x->out64;
	MYFLT *csIn, *csOut;
	ScopedLock k(m_lock);

	if(m_compiled && !m_renderingToFile && !m_performanceFinished)
	{
		csOut = csoundGetSpout(m_csound);
		csIn = csoundGetSpin(m_csound);

		if(m_x->evenlyDivisible)
		{	/* ksmps evenly divides the current Max vector size. Keep filling csIn[].
			 * When csIn[] is full, process Csound.  Processing Csound will give us
			 * ksmps output frames stored in csOut[].  At the end of the for loop, if
			 * there are any frames in csOut[], copy them to the output buffer provided
			 * by Max.  Since vector_size % ksmps == 0, this results in latency = 0 samples. */
			j = 0;
			for(i=0; i<vectorSize; i++)
			{
				if(!m_performanceFinished)
				{
					indexMultChans = m_in_index * m_chans;
					for(chan=0; chan<m_inChans; chan++)
						csIn[indexMultChans + chan] = (MYFLT)in[chan][i] * m_scale; 
					
					if(++m_in_index == m_ksmps) 
					{	
						m_performanceFinished = csoundPerformKsmps(m_csound);
						m_in_index = 0;
						m_out_index = 0;
					}

					while(m_out_index < m_ksmps && j < vectorSize)
					{
						indexMultChans = m_out_index * m_chans;
						for(chan=0; chan<m_outChans; chan++)
							out[chan][j] = (t_double)(csOut[indexMultChans + chan] * m_oneDivScale);
						++j;
						++m_out_index;
					}
				}
			}
		}
		else // x->evenlyDivisible == false
		{	/* ksmps does not evenly divide the current Max vector size. Here's a description of the loop: 
			 * We add a frame from the Max input vectors to csIn[], check to see if we have ksmps frames,
			 * if we have ksmps frames then process Csound, then copy a frame from csOut[] to the Max output
			 * vector (csOut[] may contain only zeros). This results in latency = ksmps. */
			for(i=0; i<vectorSize; i++)
			{
				if(!m_performanceFinished)
				{
					if(m_in_index == m_ksmps) 
					{	
						m_performanceFinished = csoundPerformKsmps(m_csound);
						m_in_index = 0;
					}
					indexMultChans = m_in_index * m_chans;
					for(chan=0; chan<m_inChans; chan++)
						csIn[indexMultChans + chan] = (MYFLT)in[chan][i] * m_scale;
					
					for(chan=0; chan<m_outChans; chan++)
						out[chan][i] = (t_double)(csOut[indexMultChans + chan] * m_oneDivScale);
					
					++m_in_index;
				}
			}
		}
		if(m_performanceFinished) 
		{
			m_iChanGroup.ClearPtrs();
			m_oChanGroup.ClearPtrs();
			defer_low(m_obj, (method)csound_sendPerfDoneBang, NULL, 0, NULL);
		}
	}
}


void CsoundObject::Rewind()
{
	ScopedLock k(m_lock);

	if(!m_compiled) return;
	csoundSetScoreOffsetSeconds(m_csound, (MYFLT) 0);
	csoundRewindScore(m_csound);
	csoundSetScorePending(m_csound, 1);
	m_performanceFinished = false;
}

void CsoundObject::SetCsoundArguments(short argc, const t_atom *argv)
{
	ScopedLock k(m_lock);
	m_args.SetCsoundArguments(argc,argv,m_path,m_defaultPath);
}

void CsoundObject::SetCurDir()
{
	// TODO: Is this try/catch really necessary? 
	try
	{
		if(m_path.size()) change_directory(m_obj, m_path.c_str());
		else if(m_defaultPath.size()) change_directory(m_obj, m_defaultPath.c_str());
	}
	catch(std::exception & ex)
	{
		object_error(m_obj, "CsoundObject::SetCurDir() : %s", ex.what());
	}
}

void CsoundObject::Stop()
{
	bool send_bang = false;
	m_iChanGroup.ClearPtrs();
	m_oChanGroup.ClearPtrs();
	{
		ScopedLock k(m_lock);
//		if(m_compiled)
//		{
   		csoundStop(m_csound);
			csoundCleanup(m_csound);
			csoundReset(m_csound);
			m_compiled = false;
			m_performanceFinished = true;
			send_bang = true;
//		}
	}
	if(send_bang) 
		csound_sendPerfDoneBang(m_x,NULL,0,NULL); // Always call outlets outside locked zones.
}

uintptr_t CsoundObject_RenderThreadFunc(void* data)
{
  CsoundObject *cso = (CsoundObject*)data;
	Sequencer &seq = cso->m_sequencer;
	bool firstPass = true;
	bool inStoppageTime = false;
	int stoppageCounter = 0, stoppageTime = DEFAULT_STOPPAGE_TIME * cso->m_sr;

	seq.StopRecording();
	seq.StopPlaying(); // Resets time to 0.
	
	while(1)
	{
		ScopedLock k(cso->m_lock);
		if(!cso->m_performanceFinished)
		{
			if(firstPass)
			{
				firstPass = false;
				seq.SampleBasedTimerCallback(); // Send time 0 events. Eventually calls seq.ProcessEvents().
			}
			cso->m_performanceFinished = csoundPerformKsmps(cso->m_csound);
			seq.AdvanceSampleCount(cso->m_ksmps); // Eventually calls seq.ProcessEvents().
			if(!inStoppageTime && !seq.Playing())
			{
				inStoppageTime = true;
				stoppageCounter = 0;
			}
			if(inStoppageTime)
			{
				stoppageCounter += cso->m_ksmps;
				if(stoppageCounter >= stoppageTime) cso->m_performanceFinished = true;
			}
		}
		if(cso->m_performanceFinished) break;
	}
	cso->Stop();
	cso->m_renderThreadExists = false;
  return 0;
}


void inputValueCallback(CSOUND *csound, const char *name, void *channelValuePtr, const void *channelType)
{
//	CsoundObject *cso = (CsoundObject *) csoundGetHostData(csound);
    
   	t_csound *x = (t_csound *) csoundGetHostData(csound);
	if(x->input) {
        x->cso->m_iChanGroup.GetVal(name, (MYFLT*)channelValuePtr );
    }
}

void outputValueCallback(CSOUND *csound, const char *name, void *channelValuePtr, const void *channelType)
{
	t_csound *x = (t_csound *) csoundGetHostData(csound);
	int type = CSOUND_OUTPUT_CHANNEL | CSOUND_CONTROL_CHANNEL;
	
	if(!x->output) return;
	
	// Find/Create the channel, set it's value.
	x->cso->m_oChanGroup.SetVal(name, type, *((MYFLT*)channelValuePtr));
}

void messageCallback(CSOUND *csound, int attr, const char *format, va_list valist)
{	
	t_csound *x = (t_csound *) csoundGetHostData(csound);
	char text[MAX_STRING_LENGTH];
	char *newLine = NULL, *ptr = NULL;
	static char tab = (char) 9;
	
	if(!x->messageOutputEnabled) return;
	
	// Convert symbols to numbers and store output in text.
	vsprintf(text, format, valist);	
	
	// x->cso->m_textBuffer is protected by a lock because multiple calls to messageCallback()
	// may occur at the same time, so let's lock it.
	ScopedLock k(x->cso->m_textBufferLock);

	char *buf = x->cso->m_textBuffer.get();

	// Concatenate text to buf.
	strncat(buf, text, MAX_STRING_LENGTH - strlen(buf) - 1);

	// If buf contains a newline, it's time to print it.
	if((newLine = strrchr(buf, '\n')) != NULL)
	{
		// Erase the newline; object_post() adds one for us.
		*newLine = ' ';

		// Look for tabs (9), if found replace with space (32).
		while((ptr = strrchr(buf, tab)) != NULL) *ptr = ' ';

		// Don't post inside a Csound callback (can cause hangs).
		// Instead add the message to a buffer and it will be output
		// the next time csound_msgClockCallback() is called.
		x->cso->m_msg_buf.add(message::_NORMAL_MSG, buf);
		
		// Clear the contents of buf in preparation for the next line of text.
		buf[0] = '\0'; 
	}
}

int midiInOpenCallback(CSOUND *csound, void **userData, const char *buf) { return 0; }

int midiInCloseCallback(CSOUND *csound, void *userData) { return 0; }

// Read at most 'nbytes' bytes from our MidiBuffer and store in 'buf'. Returns the 
// actual number of bytes read. Incomplete messages (such as a note on status without 
// the data bytes) should not be returned.
int midiReadCallback(CSOUND *csound, void *userData, unsigned char *buf, int nbytes)
{
	t_csound *x = (t_csound *) csoundGetHostData(csound);
	MidiBuffer *mb = &x->cso->m_midiBuffer;
	Sequencer *seq = &x->cso->m_sequencer;
	int bytesLeft = nbytes, bytesRead = 0, msg_size = 0;
	
	while(bytesLeft)
	{
		// Add a complete midi message to Csound's midi buffer.
		msg_size = mb->DequeueCompleteMessage(&buf[bytesRead], bytesLeft);
		if(0 == msg_size) break;
		else
		{
			if(seq->Recording()) seq->AddMIDIEvent(&buf[bytesRead], msg_size, true);
			if(0xB0 == (buf[bytesRead] & 0xf0))
			{
				// Keep track of CC values in the sequencer.
				byte b = 0, chan, ctrl, val;
				chan = b & 0x0f;
				ctrl = buf[bytesRead + 1];
				val = buf[bytesRead + 2];
				seq->UpdateCtrlMatrix(chan, ctrl, val);
			}
			bytesRead += msg_size;
			bytesLeft -= msg_size;
		}
	}
	return bytesRead;
}

int midiOutOpenCallback(CSOUND *csound, void **userData, const char *buf) { return 0; }

int midiOutCloseCallback(CSOUND *csound, void *userData) { return 0; }

int midiWriteCallback(CSOUND *csound, void *userData, const unsigned char *buf, int nbytes)
{
	t_csound *x = (t_csound *) csoundGetHostData(csound);
	int bytesWritten = 0;
	
	while(bytesWritten < nbytes) outlet_int(x->midi_outlet, (int)buf[bytesWritten++]);
	
	return bytesWritten;
}	
