/**
* C S O U N D   V S T 
*
* A VST plugin version of Csound, with Python scripting.
*
* L I C E N S E
*
* This software is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this software; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef CPPSOUND_H
#define CPPSOUND_H
#ifdef SWIG
%module CsoundVST
%include "std_string.i"
%include "std_vector.i"
%{
#include "CsoundFile.hpp"
#include <string>
#include <vector>
%}
#else
#include "CsoundFile.hpp"
#include <string>
#include <vector>
#include <csound.h>
#include <cs.h>
#endif

/**
* Class interface to the Csound API.
*/
class CppSound : 
	public CsoundFile
{
protected:
	ENVIRON *csound;
	bool isCompiled;
	/**
	* Csound controls this one.
	*/
	bool isPerforming;
	/**
	* The user controls this one.
	*/
	bool go;
	size_t spoutSize;
public:
	/**
	*	Default creator.
	*/
	CppSound();
	/**
	*	Virtual destructor.
	*/
	virtual ~CppSound();
	/**
	*	Using the specified arguments, 
	*	compiles and performs the orchestra and score, 
	*	in one pass, just as Csound would do.
	*/
	virtual int perform(int argc, char **argv);
	/**
	*	Using stored arguments, 
	*	compiles and performs the orchestra and score,
	*	in one pass, just as Csound would do.
	*/
	virtual int perform();
	/**
	*	Stops the performance.
	*/
	virtual void stop();
	/**
	*	Compiles the score and orchestra without performing them,
	*	in preparation for calling performKsmps.
	*/
	virtual int compile(int argc, char **argv);
	/**
	*	Using stored arguments, 
	*	compiles the score and orchestra without performing them,
	*	in preparation for calling performKsmps.
	*/
	virtual int compile();
	/**
	*	Causes Csound to read ksmps of audio sample frames from its input buffer,
	*	compute the performance, 
	*	and write the performed sample frames to its output buffer.
	*/
	virtual int performKsmps();
	/**
	*	Must be called after the final call to performKsmps.
	*/
	virtual void cleanup();
	/**
	*	Resets all internal state.
	*/
	virtual void reset();
	/**
	*	Returns the address of the Csound input buffer;
	*	external software can write to it before calling performKsmps.
	*/
	virtual MYFLT *getSpin();
	/**
	*	Returns the address of the Csound output buffer;
	*	external software can read from it after calling performKsmps.
	*/
	virtual MYFLT *getSpout();
	/**
	*	Returns the size of the sample frame output buffer in bytes.
	*/
	virtual size_t getSpoutSize() const;
	/**
	*	Sets a function for Csound to call to print informational messages through external software.
	*/
	virtual void setMessageCallback(void (*messageCallback)(void *hostData, const char *format, va_list args));
	/**
	*	Print an informational message.
	*/
	virtual void message(const char *format,...);
	/**
	*	Print an informational message.
	*/
	virtual void messageV(const char *format, va_list args);
	/**
	*	Stops execution with an error message or exception.
	*/
	virtual void throwMessage(const char *format,...);
	/**
	*	Stops execution with an error message or exception.
	*/
	virtual void throwMessageV(const char *format, va_list args);
	/**
	*	Called by external software to set a funtion for Csound to stop execution
	*	with an error message or exception.
	*/
	virtual void setThrowMessageCallback(void (*throwMessageCallback)(void *csound, const char *format, va_list args));
	/**
	*	Returns 1 if MIDI input from external software is enabled, or 0 if not.
	*/
	virtual int isExternalMidiEnabled();
	/**
	*	Sets whether MIDI input from external software is enabled.
	*/
	virtual void setExternalMidiEnabled(int enabled);
	/**
	*	Called by external software to set a function for Csound to call to open MIDI input.
	*/
	virtual void setExternalMidiOpenCallback(void (*midiOpen)(void *csound));
	/**
	*	Called by external software to set a function for Csound to call to read MIDI messages.
	*/
	virtual void setExternalMidiReadCallback(int (*midiReadCallback)(void *ownerData, unsigned char *midiData, int size));
	/**
	*	Called by external software to set a function for Csound to call to close MIDI input.
	*/
	virtual void setExternalMidiCloseCallback(void (*closeMidi)(void *csound));	
	/**
	*	Sets up internal MIDI read for VST plugins, etc.
	*/
	virtual void defaultMidiOpen();	
	/**
	*	Returns the number of audio sample frames per control sample.
	*/
	virtual int getKsmps();
	/**
	*	Returns the number of audio output channels.
	*/
	virtual int getNchnls();
	/**
	*	Sets the message level (0 to 7).
	*/
	virtual void setMessageLevel(int messageLevel);
	/**
	*	Returns the Csound message level (0 to 7).
	*/
	virtual int getMessageLevel();
	/**
	*	Appends an opcode to the opcode list.
	*/
	int appendOpcode(char *opname, int dsblksiz, int thread, char *outypes, char *intypes, SUBR iopadr, SUBR kopadr, SUBR aopadr, SUBR dopadr);
	/**
	*	Returns whether Csound's score is synchronized with external software.
	*/
	virtual int isScorePending();
	/**
	*	Sets whether Csound's score is synchronized with external software.
	*/
	virtual void setScorePending(int pending);
	/**
	*	Csound events prior to the offset are consumed and discarded prior to beginning performance.
	*	Can be used by external software to begin performance midway through a Csound score.
	*/
	virtual void setScoreOffsetSeconds(MYFLT offset);
	/**
	*	Csound events prior to the offset are consumed and discarded prior to beginning performance.	
	*	Can be used by external software to begin performance midway through a Csound score.
	*/
	virtual MYFLT getScoreOffsetSeconds();
	/**
	*	Rewind a compiled Csound score to its beginning.
	*/
	virtual void rewindScore();
	/**
	*	Returns the number of audio sample frames per second.
	*/
	virtual MYFLT getSr();
	/**
	*	Returns the number of control samples per second.
	*/
	virtual MYFLT getKr();
	/**
	*	Loads plugin opcodes.
	*/
	virtual int loadExternals();
	/**
	*	Returns a function table.
	*/
	FUNC *(*ftfind)(MYFLT *index);
	/**
	*	Sends a line event.
	*/
	virtual void inputMessage(std::string istatement);
	/**
	*   Returns the actual instance of Csound.
	*/
	virtual void *getCsound();
	/**
	*   For Python.
	*/
	virtual void write(const char *text);
	/**
	*   Shortcut for CsoundVST to get an instance pointer to a CppSound instance 
	*   created in Python.
	*/
	virtual long getThis();
	/**
	*  Indicates whether orc and sco have been compiled.
	*/
	virtual bool getIsCompiled() const;
	/**
	*  Sets whether orc and sco have been compiled, 
	*  and performance has now begun.
	*/
	virtual void setIsPerforming(bool isPerforming);
	/**
	*  Indicates whether orc and sco have been compiled, 
	*  and performance has now begun.
	*/
	virtual bool getIsPerforming() const;
	/**
	*  Indicates whether orc and sco have been compiled, 
	*  and performance should continue.
	*/
	virtual bool getIsGo() const;
};

#endif

