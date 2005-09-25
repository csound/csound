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
%template(MyfltVector) std::vector<MYFLT>;
#else
#include "CsoundFile.hpp"
#include <string>
#include <vector>
#include <csound.h>
#endif

/**
 * Class interface to the Csound API.
 */
class CppSound : public CsoundFile
{
protected:
  CSOUND *csound;
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
  std::string renderedSoundfile;
public:
  /**
   *    Default creator.
   */
  CppSound();
  /**
   *    Virtual destructor.
   */
  virtual ~CppSound();
  /**
   *    Using the specified arguments,
   *    compiles and performs the orchestra and score,
   *    in one pass, just as Csound would do.
   */
  virtual int perform(int argc, char **argv);
  /**
   *    Using stored arguments,
   *    compiles and performs the orchestra and score,
   *    in one pass, just as Csound would do.
   */
  virtual int perform();
  /**
   *    Stops the performance.
   */
  virtual void stop();
  /**
   * Reset and prepare an instance of Csound for compilation.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
   * CSOUND_MEMORY if an error occured.
   */
  virtual int preCompile();
  /**
   *    Compiles the score and orchestra without performing them,
   *    in preparation for calling performKsmps.
   */
  virtual int compile(int argc, char **argv);
  /**
   *    Using stored arguments,
   *    compiles the score and orchestra without performing them,
   *    in preparation for calling performKsmps.
   */
  virtual int compile();
  /**
   *    Causes Csound to read ksmps of audio sample frames from its input buffer,
   *    compute the performance,
   *    and write the performed sample frames to its output buffer.
   *   If absolute is true, performs a block of audio whether or not the Csound
   *   score is finished.
   */
  virtual int performKsmps(bool absolute = true);
  /**
   *    Must be called after the final call to performKsmps.
   */
  virtual void cleanup();
  /**
   *    Resets all internal state.
   */
  virtual void reset();
  /**
   *    Returns the address of the Csound input buffer;
   *    external software can write to it before calling performKsmps.
   */
  virtual MYFLT *getSpin();
  /**
   *    Returns the address of the Csound output buffer;
   *    external software can read from it after calling performKsmps.
   */
  virtual MYFLT *getSpout();
  /**
   *    Returns the size of the sample frame output buffer in bytes.
   */
  virtual size_t getSpoutSize() const;
  /**
   *    Sets a function for Csound to call to print informational messages through external software.
   */
  virtual void setMessageCallback(void (*messageCallback)(CSOUND *csound, int attr, const char *format, va_list args));
  /**
   *    Print an informational message.
   */
  virtual void message(const char *format,...);
  /**
   *    Print an informational message.
   */
  virtual void messageV(const char *format, va_list args);
  /**
   *    Stops execution with an error message or exception.
   */
  virtual void throwMessage(const char *format,...);
  /**
   *    Stops execution with an error message or exception.
   */
  virtual void throwMessageV(const char *format, va_list args);
  /**
   *    Called by external software to set a funtion for Csound to stop execution
   *    with an error message or exception.
   */
  virtual void setThrowMessageCallback(void (*throwMessageCallback)(CSOUND *csound, const char *format, va_list args));
  /**
   *    Called by external software to set a function for Csound to call to open MIDI input.
   */
  virtual void setExternalMidiInOpenCallback(int (*ExternalMidiInOpen)(CSOUND *csound, void **userData,
                                                                       const char *devName));
  /**
   *    Called by external software to set a function for Csound to call to read MIDI messages.
   */
  virtual void setExternalMidiReadCallback(int (*ExternalMidiRead)(CSOUND *csound, void *userData,
                                                                   unsigned char *buf, int nbytes));
  /**
   *    Called by external software to set a function for Csound to call to close MIDI input.
   */
  virtual void setExternalMidiInCloseCallback(int (*ExternalMidiInClose)(CSOUND *csound, void *userData));
  /**
   *    Returns the number of audio sample frames per control sample.
   */
  virtual int getKsmps();
  /**
   *    Returns the number of audio output channels.
   */
  virtual int getNchnls();
  /**
   *    Sets the message level (0 to 7).
   */
  virtual void setMessageLevel(int messageLevel);
  /**
   *    Returns the Csound message level (0 to 7).
   */
  virtual int getMessageLevel();
  /**
   *    Appends an opcode to the opcode list.
   */
  int appendOpcode(char *opname, int dsblksiz, int thread, char *outypes, char *intypes, int (*iopadr)(CSOUND*, void*), int (*kopadr)(CSOUND*, void*), int (*aopadr)(CSOUND*, void*));
  /**
   *    Returns whether Csound's score is synchronized with external software.
   */
  virtual int isScorePending();
  /**
   *    Sets whether Csound's score is synchronized with external software.
   */
  virtual void setScorePending(int pending);
  /**
   *    Csound events prior to the offset are consumed and discarded prior to beginning performance.
   *    Can be used by external software to begin performance midway through a Csound score.
   */
  virtual void setScoreOffsetSeconds(MYFLT offset);
  /**
   *    Csound events prior to the offset are consumed and discarded prior to beginning performance.
   *    Can be used by external software to begin performance midway through a Csound score.
   */
  virtual MYFLT getScoreOffsetSeconds();
  /**
   *    Rewind a compiled Csound score to its beginning.
   */
  virtual void rewindScore();
  /**
   *    Returns the number of audio sample frames per second.
   */
  virtual MYFLT getSr();
  /**
   *    Returns the number of control samples per second.
   */
  virtual MYFLT getKr();
  /**
   *    Loads plugin opcodes.
   */
  virtual int loadExternals();
  /**
   *    Returns a function table.
   */
/* FIXME: FUNC */ void *(*ftfind)(MYFLT *index);
  /**
   *    Sends a line event.
   */
  virtual void inputMessage(std::string istatement);
  /**
   *   Returns the actual instance of Csound.
   */
  virtual CSOUND *getCsound();
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
  /**
   * Set up Python to print Csound messages.
   */
  virtual void setPythonMessageCallback();
  /**
   * Returns the length of a function table.
   */
  virtual int tableLength(int table);
  /**
   * Returns the value of a slot in a functon table.
   */
  virtual MYFLT tableGet(int table, int index);
  /**
   * Sets the value of a slot in a function table.
   */
  virtual void tableSet(int table, int index, MYFLT value);
  /**
   * Send a score event to Csound in real time.
   */
  virtual void scoreEvent(char opcode, std::vector<MYFLT> &pfields);
  /**
   * Return a pointer to the CppSound base (for Java wrappers).
   */
  virtual CsoundFile *getThisCsoundFile();
  /**
   * Return the name of the output soundfile from the Csound
   * parameters structure.
   */
  virtual std::string CppSound::getOutputSoundfileName() const;

#if !defined(SWIG)
  /**
   * Called by external software to set a function for Csound to
   * fetch input control values.  The 'invalue' opcodes will
   * directly call this function.
   */
  virtual void setInputValueCallback(void (*inputValueCallback)(CSOUND *csound,
                                                                const char *channelName,
                                                                MYFLT *value));

  /**
   * Called by external software to set a function for Csound to
   * send output control values.  The 'outvalue' opcodes will
   * directly call this function.
   */
  virtual void setOutputValueCallback(void (*outputValueCallback)(CSOUND *csound,
                                                                  const char *channelName,
                                                                  MYFLT value));
#endif
};

#endif

