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
#include <boost/tokenizer.hpp>

CppSound::CppSound() : 	csound(0),
			isCompiled(false),
			isPerforming(false),
			go(false),
			spoutSize(0)
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
  int returnValue = 0;
  int argc = 0;
  char **argv = 0;
  std::string command = getCommand();
  if(command.find("-") == 0)
    {
      const char *args[] = {"csound", getFilename().c_str(), 0};
      returnValue = perform(2, (char **)args);
    }
  else
    {
      scatterArgs(getCommand(), &argc, &argv);
      returnValue = perform(argc, argv);
    }
  deleteArgs(argc, argv);
  return returnValue;
}

int CppSound::perform(int argc, char **argv)
{
  double beganAt = double(clock()) / double(CLOCKS_PER_SEC);
  isCompiled = false;
  go = false;
  message("BEGAN CppSound::perform(%d, %x)...\n", argc, argv);
  if(argc <= 0)
    {
      message("ENDED CppSound::perform without compiling or performing.\n");
      return 0;
    }
  int result = compile(argc, argv);
  if(result == -1)
    {
      return result;
    }
  for(result = 0; (result == 0) && go; )
    {
      result = performKsmps();
    }
  cleanup();
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

int CppSound::preCompile()
{
  return csoundPreCompile(csound);
}

int CppSound::compile(int argc, char **argv)
{
  message("BEGAN CppSound::compile(%d, %x)...\n", argc, argv);
  go = false;
  int returnValue = csoundCompile(csound, argc, argv);
  spoutSize = csoundGetKsmps(csound) * csoundGetNchnls(csound) * sizeof(MYFLT);
  if(returnValue)
    {
      isCompiled = false;
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

int CppSound::performKsmps(bool absolute)
{
  if(absolute){
    return csoundPerformKsmpsAbsolute(csound);
  }
  else {
    return csoundPerformKsmps(csound);
  }
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

void CppSound::setMessageCallback(void (*messageCallback)(void *hostData, int attr, const char *format, va_list args))
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
  csoundSetThrowMessageCallback(this->csound, throwCallback);
}

void CppSound::setExternalMidiInOpenCallback(int (*ExternalMidiInOpen)(void *csound, void **userData,
								       const char *devName))
{
  csoundSetExternalMidiInOpenCallback(this->csound, ExternalMidiInOpen);
}

void CppSound::setExternalMidiReadCallback(int (*ExternalMidiRead)(void *csound, void *userData,
								   unsigned char *buf, int nbytes))
{
  csoundSetExternalMidiReadCallback(this->csound, ExternalMidiRead);
}

void CppSound::setExternalMidiInCloseCallback(int (*ExternalMidiInClose)(void *csound, void *userData))
{
  csoundSetExternalMidiInCloseCallback(this->csound, ExternalMidiInClose);
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
  csoundMessageV(csound, 0, format, args);
  va_end(args);
}

void CppSound::messageV(const char *format, va_list args)
{
  csoundMessageV(csound, 0, format, args);
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
  std::vector<MYFLT> pfields;
  typedef boost::char_separator<char> charsep;
  boost::tokenizer<charsep> tokens(istatement, charsep(" "));
  boost::tokenizer<charsep>::iterator it = tokens.begin();
  std::string opcode = *it;
  for( ++it; it != tokens.end(); ++it) {
    pfields.push_back(atof(it->c_str()));
  }
  csoundScoreEvent(csound, opcode[0], &pfields.front(), pfields.size());
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
  if(csound){
    if(csoundGetSpin(csound) && csoundGetSpout(csound)){
      return go;
    }
  }
  return false;
}

extern "C" 
{
  void csoundSetMessageCallback(void *csound, void (*csoundMessageCallback)(void *csound, int attr, const char *format, va_list valist));
  int PyRun_SimpleString(const char *string);
}

static void pythonMessageCallback(void *csound, int attr, const char *format, va_list valist)
{
  static char buffer[0x1000];
  static char buffer1[0x1000];
  vsprintf(buffer, format, valist);
  static std::string actualBuffer;
  static std::string lineBuffer;
  actualBuffer.append(buffer);
  size_t position = 0;
  while((position = actualBuffer.find("\n")) != std::string::npos)
    {
      lineBuffer = actualBuffer.substr(0, position);
      actualBuffer.erase(0, position + 1); 
      actualBuffer.clear();
      sprintf(buffer1, "print '''%s'''", lineBuffer.c_str());
      PyRun_SimpleString(buffer1);
    }
}

void CppSound::setPythonMessageCallback()
{
  csoundSetMessageCallback(csound, pythonMessageCallback);
}

int CppSound::tableLength(int table)
{
  return csoundTableLength(csound, table);
}

MYFLT CppSound::tableGet(int table, int index)
{
  return csoundTableGet(csound, table, index);
}

void CppSound::tableSet(int table, int index, MYFLT value)
{
  csoundTableSet(csound, table, index, value);
}

void CppSound::scoreEvent(char opcode, std::vector<MYFLT> &pfields)
{
  csoundScoreEvent(csound, opcode, &pfields.front(), pfields.size());
}

CsoundFile *CppSound::getThisCsoundFile()
{
  return (CsoundFile *)this;
}

void CppSound::setFLTKThreadLocking(bool isLocking)
{
  csoundSetFLTKThreadLocking(csound, isLocking);
}

bool CppSound::getFLTKThreadLocking()
{
  return csoundGetFLTKThreadLocking(csound);
}

std::string CppSound::getOutputSoundfileName() const
{
  if(csound->oparms->outfilename)
    {
      return csound->oparms->outfilename;
    }
  else
    {
      return "";
    }
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
