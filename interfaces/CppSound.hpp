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
#ifndef CSND_CPPSOUND_H
#define CSND_CPPSOUND_H
#ifdef SWIG
%module csnd
%include "std_string.i"
%include "std_vector.i"
%{
#include "Csound.hpp"
#include "CsoundFile.hpp"
#include <string>
#include <vector>
  %}
%template(MyfltVector) std::vector<MYFLT>;
#else
#include "Csound.hpp"
#include "CsoundFile.hpp"
#include <string>
#include <vector>
#endif

#if defined(WIN32)
#define PUBLIC __declspec(dllexport)
#else
#define PUBLIC
#endif

class PUBLIC CppSound : public Csound, public CsoundFile
{
  bool go;
  bool isCompiled;
  bool isPerforming;
  size_t spoutSize;
  std::string renderedSoundfile;
public:
  PUBLIC CppSound();
  PUBLIC virtual ~CppSound();  
  PUBLIC virtual CSOUND *getCsound();
  PUBLIC virtual long getThis();
  PUBLIC virtual int compile(int argc, char **argv);
  PUBLIC virtual int compile();  
  PUBLIC virtual size_t getSpoutSize() const;
  PUBLIC virtual int perform(int argc, char **argv);
  PUBLIC virtual int perform();
  PUBLIC virtual int performKsmps(bool absolute);
  PUBLIC virtual void cleanup();
  PUBLIC virtual void inputMessage(std::string istatement);
  PUBLIC virtual void write(const char *text);
  PUBLIC virtual bool getIsCompiled() const;
  PUBLIC virtual void setIsPerforming(bool isPerforming);
  PUBLIC virtual bool getIsPerforming() const;
  PUBLIC virtual bool getIsGo();
  PUBLIC virtual void stop();
  PUBLIC virtual void setPythonMessageCallback();
};

#endif

