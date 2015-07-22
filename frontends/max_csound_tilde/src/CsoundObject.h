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
#include "Args.h"
#include "csound.h"
#include "channel.h"
#include "CsoundTable.h"
#include "message_buffer.h"
#include "midi.h"
#include "PatchScripter.h"
#include "sequencer.h"
#include <boost/scoped_ptr.hpp>

typedef struct _csound t_csound;
using boost::scoped_ptr;

namespace dvx {

/* Think of CsoundObject as an extension of t_csound. Almost everything in CsoundObject is
   public in order to minimize barriers between t_csound and CsoundObject. */

class CsoundObject
{
public:
	enum { COMPILATION_FAILURE = 0, COMPILATION_SUCCESS = 1, COMPILATION_SUCCESS_MISMATCHED_SR = 2 };

public:
	CsoundObject(t_csound *partner);
	~CsoundObject();

	void Compile();         // Compile the Csound orchestra. Do not post directly in here. Use m_msg_buf.
	void Perform();         // Perform Csound (must be run in audio thread).
   	void Perform64();         // Perform Csound (must be run in audio thread).
	void Rewind();
	void SetCsoundArguments(short argc, const t_atom *argv);
	void SetCurDir();       // Set cur dir to m_path. If m_path empty, use m_defaultPath.
	void Stop();            // Stop the Csound performance.

private:
	int Compile(bool lock); // The function that actually compiles the Csound orchestra.
	                        // Do not post directly in here. Use m_msg_buf.
	                        // Returns COMPILATION_SUCCESS on success.
	                        // Returns COMPILATION_SUCCESS_MISMATCHED_SR on success (but with mismatched sr's).
	                        // Returns COMPILATION_FAILURE on failure.

public:
	CSOUND *m_csound;		// Pointer to an instance of Csound.
	t_csound *m_x;          // Pointer to parent t_csound object.
	t_object *m_obj;        // Pointer to parent cast as t_object pointer.

	DEFAULT_LOCK_TYPE m_lock;  // Protects this CsoundObject.
	ChannelGroup m_iChanGroup; // Used to store input channel/value pairs.
	ChannelGroup m_oChanGroup; // Used to store output channel/value pairs.
	MidiBuffer m_midiBuffer;   // Contains a circular buffer for MIDI data bound for Csound.
	Patcher::Scripter m_scripter; // Provides csd parsing and patcher scripting functionality.
	Sequencer m_sequencer;     // Records/Plays sequences of Csound events, control messages, MIDI.
	message_buffer m_msg_buf;  // Where Csound textual output is stored via messageCallback(). 

	int m_sr;			       // Csound sample rate.
	int m_chans;			   // # of Csound audio input/output channels.
	int m_inChans;			   // Equal to lesser of m_x->numInSignals and m_chans.
	int m_outChans;			   // Equal to lesser of m_x->numOutSignals and m_chans.
	int m_ksmps;			   // Csound samples per k-cycle.
	int m_in_index;			   // Sample index into Csound's input buffer (reset to zero when index == ksmps).
	int m_out_index;		   // Sample index into Csound's output buffer.
	MYFLT m_scale;			   // The scaling factor for MSP audio data that is going to Csound.
	MYFLT m_oneDivScale;	   // The scaling factor for Csound audio data that is going to MSP.
	volatile bool m_compiled;  // True if csoundCompile() returned true.
	bool m_renderingToFile;    // True if we are rendering to a file (no real-time audio output).
	volatile bool m_performanceFinished; // If true, Csound performance has finished.
	
	short m_defaultPathID;     // A Max path ID corresponding to m_defaultPath.
	string m_defaultPath;      // Absolute path of the parent patch.
	string m_path;             // User supplied current directory for relative paths.

	Args m_args;               // Maintains the csound argument list.
	
	DEFAULT_LOCK_TYPE m_textBufferLock; // Protects textBuffer.
	scoped_ptr<char> m_textBuffer;      // Stores Csound text output.

	void* m_renderThread;	        // Thread function used when Csound is rendering output to an audio file.
	volatile bool m_renderThreadExists;	// true if renderThread references an existing thread.
};

} // namespace dvx

// Used when the csound command contains an audio output file.
uintptr_t CsoundObject_RenderThreadFunc(void *cso);

// Csound callback functions.
void inputValueCallback(CSOUND *csound, const char *name, void *val, const void*);
void outputValueCallback(CSOUND *csound, const char *name, void *val, const void*);
void messageCallback(CSOUND *csound, int attr, const char *format, va_list valist);

// Csound MIDI callback functions.
int midiInOpenCallback(CSOUND *csound, void **userData, const char *buf); 
int midiInCloseCallback(CSOUND *csound, void *userData);
int midiReadCallback(CSOUND *csound, void *userData, unsigned char *buf, int nbytes);
int midiOutOpenCallback(CSOUND *csound, void **userData, const char *buf);
int midiOutCloseCallback(CSOUND *csound, void *userData);
int midiWriteCallback(CSOUND *csound, void *userData, const unsigned char *buf, int nbytes);

