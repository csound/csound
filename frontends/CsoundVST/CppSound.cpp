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
#include "CsoundFile.hpp"
#include "CppSound.hpp"
#include "System.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

extern "C" 
{
    extern MYFLT timtot;
    extern void csoundDefaultMidiOpen(void *csound);
};

CppSound::CppSound() : isCompiled(false), 
	isPerforming(false), 
 	csound(0), 
  	spoutSize(0),
  	go(false)
{
	csound = (ENVIRON *)csoundCreate(this);
}

CppSound::~CppSound()
{
	if(csound)
	{
		csoundDestroy(csound);
		csound = 0;
	}
}

int CppSound::perform()
{
	int argc = 0;
	char **argv = 0;
	scatterArgs(getCommand(), &argc, &argv);
	int returnValue = perform(argc, argv);
	deleteArgs(argc, argv);
	return returnValue;
}

int CppSound::perform(int argc, char **argv)
{
	double beganAt = double(clock()) / double(CLOCKS_PER_SEC);
	isCompiled = false;
	isPerforming = false;
	go = false;
	message("BEGAN CppSound::perform(%d, %x)...\n", argc, argv);
	if(argc <= 0)
	{
		message("ENDED CppSound::perform without compiling or performing.\n");
		return 0;
	}
	int returnValue = compile(argc, argv);
	if(returnValue == -1)
	{
		return returnValue;
	}
	for(isPerforming = true; isPerforming && go; )
	{
        isPerforming = !performKsmps();
    }
	csoundCleanup(csound);
	double endedAt = double(clock()) / double(CLOCKS_PER_SEC);
	double elapsed = endedAt - beganAt;
	message("Elapsed time = %f seconds.\n", elapsed);
	message("ENDED CppSound::perform.\n");
	isCompiled = false;
	isPerforming = false;
	return 1;
}

void CppSound::stop()
{
	message("RECEIVED CppSound::stop...\n");
	isCompiled = false;
	isPerforming = false;
	go = false;
}

void CppSound::reset()
{
	csoundReset(csound);
}

int CppSound::compile(int argc, char **argv)
{
	message("BEGAN CppSound::compile(%d, %x)...\n", argc, argv);
	int returnValue = csoundCompile(csound, argc, argv);
	spoutSize = ksmps * nchnls * sizeof(MYFLT);
	if(returnValue)
	{
	    isCompiled = false;
	    go = false;
	}
	else
	{
	    isCompiled = true;
	    go = true;
	}
	message("ENDED CppSound::compile.\n");
	return returnValue;
}

int CppSound::compile()
{
	message("BEGAN CppSound::compile()...\n");
	int argc = 0;
	char **argv = 0;
	if(getCommand().length() <= 0)
	{
	   message("No Csound command.\n");
	   return -1;
	}
	scatterArgs(getCommand(), &argc, &argv);
	int returnValue = compile(argc, argv);
	deleteArgs(argc, argv);
	message("ENDED CppSound::compile.\n");
	return returnValue;
}

int CppSound::performKsmps()
{
	return csoundPerformKsmps(csound);
}

void CppSound::cleanup()
{
	csoundCleanup(csound);
}

MYFLT *CppSound::getSpin()
{
	return csoundGetSpin(csound);
}

MYFLT *CppSound::getSpout()
{
	return csoundGetSpout(csound);
}

void CppSound::setMessageCallback(void (*messageCallback)(void *hostData, const char *format, va_list args))
{
	csoundSetMessageCallback(csound, messageCallback);
}

int CppSound::getKsmps()
{
	return csoundGetKsmps(csound);
}

int CppSound::getNchnls()
{
	return csoundGetNchnls(csound);
}

int CppSound::getMessageLevel()
{
	return csoundGetMessageLevel(csound);
}

void CppSound::setMessageLevel(int messageLevel)
{
	csoundSetMessageLevel(csound, messageLevel);
}

void CppSound::throwMessage(const char *format,...)
{
	va_list args;
	va_start(args, format);
	csoundThrowMessageV(csound, format, args);
	va_end(args);
}

void CppSound::throwMessageV(const char *format, va_list args)
{
	csoundThrowMessageV(csound, format, args);
}

void CppSound::setThrowMessageCallback(void (*throwCallback)(void *csound_, const char *format, va_list args))
{
	csoundSetThrowMessageCallback(csound, throwCallback);
}

int CppSound::isExternalMidiEnabled()
{
	return csoundIsExternalMidiEnabled(csound);
}

void CppSound::setExternalMidiEnabled(int enabled)
{
	csoundSetExternalMidiEnabled(csound, enabled);
}

void CppSound::setExternalMidiOpenCallback(void (*midiOpen)(void *csound))
{
	csoundSetExternalMidiOpenCallback(csound, midiOpen);
}

void CppSound::setExternalMidiReadCallback(int (*midiReadCallback)(void *ownerData, unsigned char *midiData, int size))
{
	csoundSetExternalMidiReadCallback(csound, midiReadCallback);
}

void CppSound::setExternalMidiCloseCallback(void (*midiClose)(void *csound))
{
	csoundSetExternalMidiCloseCallback(csound, midiClose);
}

void CppSound::defaultMidiOpen()
{
  csoundDefaultMidiOpen(csound);
}

int CppSound::isScorePending()
{
	return csoundIsScorePending(csound);
}

void CppSound::setScorePending(int pending)
{
	csoundSetScorePending(csound, pending);
}

void CppSound::setScoreOffsetSeconds(MYFLT offset)
{
	csoundSetScoreOffsetSeconds(csound, offset);
}

MYFLT CppSound::getScoreOffsetSeconds()
{
	return csoundGetScoreOffsetSeconds(csound);
}

void CppSound::rewindScore()
{
	csoundRewindScore(csound);
}

MYFLT CppSound::getSr()
{
	return csoundGetSr(csound);
}

MYFLT CppSound::getKr()
{
	return csoundGetKr(csound);
}

int CppSound::appendOpcode(char *opname, int dsblksiz, int thread, char *outypes, char *intypes, SUBR iopadr, SUBR kopadr, SUBR aopadr, SUBR dopadr)       
{
	return csoundAppendOpcode(csound, opname, dsblksiz, thread, outypes, intypes, iopadr, kopadr, aopadr, dopadr);
}

void CppSound::message(const char *format,...)
{
	va_list args;
	va_start(args, format);
	csoundMessageV(csound, format, args);
	va_end(args);
}

void CppSound::messageV(const char *format, va_list args)
{
	csoundMessageV(csound, format, args);
}

int CppSound::loadExternals()
{
	return csoundLoadExternals(csound);
}

size_t CppSound::getSpoutSize() const
{
	return spoutSize;
}

void CppSound::inputMessage(std::string istatement)
{
	csound->InputMessage(csound, istatement.c_str());
}

void *CppSound::getCsound()
{
    return csound;
}

void CppSound::write(const char *text)
{
    csoundMessage(getCsound(), text);
}

long CppSound::getThis()
{
    return (long) this;
}	

bool CppSound::getIsCompiled() const
{
    return isCompiled;
}

void CppSound::setIsPerforming(bool isPerforming)
{
    this->isPerforming = isPerforming;
}

bool CppSound::getIsPerforming() const
{
    return isPerforming;
}

bool CppSound::getIsGo() const
{
    return go;
}

/**
* Glue for incomplete Csound API.
*/
extern "C"
{

#ifdef WIN32
	int XOpenDisplay(char *)
	{
		return 1;
	}
#endif
};
