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
#ifndef CSOUNDFILE_H
#define CSOUNDFILE_H
#if defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning(disable: 4786)
#endif
#ifdef SWIG
%module CsoundVST
%include "std_string.i"
%include "std_vector.i"
%{
#include <string>
#include <vector>
  %}
#else
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <stdlib.h>

#if defined(WIN32)
#define PUBLIC __declspec(dllexport)
#include <io.h>
#else
#define PUBLIC
#endif
#endif

void gatherArgs(int argc, const char **argv, std::string &commandLine);

void scatterArgs(const std::string commandLine, int *argc, char ***argv);

void deleteArgs(int argc, char **argv);

std::string &trim(std::string &value);

std::string &trimQuotes(std::string &value);

/**
 *       Returns true if definition is a valid Csound instrument definition block.
 *       Also returns the part before the instr number, the instr number,
 *       the name (all text after the first comment on the same line as the instr number),
 *       and the part after the instr number, all by reference.
 */
bool parseInstrument(const std::string &definition, std::string &preNumber, std::string &id, std::string &name, std::string &postNumber);

/**
 * Manages a Csound Structured Data (CSD) file with facilities
 * for creating an arrangement of selected instruments in the orchestra,
 * and for programmatically building score files.
 */
class CsoundFile
{
protected:
  /**
   *       What are we storing, anyway?
   */
  std::string filename;
  /**
   *       CsOptions
   */
  std::string command;
  /**
   *       CsInstruments
   */
  std::string orchestra;
  /**
   *       CsScore
   */
  std::string score;
  /**
   *       CsMidi
   */
  std::vector<unsigned char> midifile;
public:
  /**
   *       Patch library and arrangement.
   */
  std::string libraryFilename;
  std::vector<std::string> arrangement;
  PUBLIC CsoundFile();
  PUBLIC virtual ~CsoundFile(void){};
  PUBLIC virtual std::string generateFilename();
  PUBLIC virtual std::string getFilename(void);
  PUBLIC virtual void setFilename(std::string name);
  PUBLIC virtual int load(std::string filename);
  PUBLIC virtual int load(std::istream &stream);
  PUBLIC virtual int save(std::string filename) const;
  PUBLIC virtual int save(std::ostream &stream) const;
  PUBLIC virtual int importFile(std::string filename);
  PUBLIC virtual int importFile(std::istream &stream);
  PUBLIC virtual int importCommand(std::istream &stream);
  PUBLIC virtual int exportCommand(std::ostream &stream) const;
  PUBLIC virtual int importOrchestra(std::istream &stream);
  PUBLIC virtual int exportOrchestra(std::ostream &stream) const;
  PUBLIC virtual int importScore(std::istream &stream);
  PUBLIC virtual int exportScore(std::ostream &stream) const;
  PUBLIC virtual int importArrangement(std::istream &stream);
  PUBLIC virtual int exportArrangement(std::ostream &stream) const;
  PUBLIC virtual int exportArrangementForPerformance(std::string filename) const;
  PUBLIC virtual int exportArrangementForPerformance(std::ostream &stream) const;
  PUBLIC virtual int importMidifile(std::istream &stream);
  PUBLIC virtual int exportMidifile(std::ostream &stream) const;
  PUBLIC virtual std::string getCommand(void) const;
  PUBLIC virtual void setCommand(std::string commandLine);
  PUBLIC virtual std::string getOrcFilename(void) const;
  PUBLIC virtual std::string getScoFilename(void) const;
  PUBLIC virtual std::string getMidiFilename(void) const;
  PUBLIC virtual std::string getOutputSoundfileName(void) const;
  PUBLIC virtual std::string getOrchestra(void) const;
  PUBLIC virtual void setOrchestra(std::string orchestra);
  PUBLIC virtual int getInstrumentCount(void) const;
  PUBLIC virtual std::string getOrchestraHeader(void) const;
  PUBLIC virtual bool getInstrument(int number, std::string &definition) const;
  //PUBLIC virtual bool getInstrumentNumber(int index, std::string &definition) const;
  PUBLIC virtual bool getInstrument(std::string name, std::string &definition) const;
  PUBLIC virtual std::string getScore() const;
  PUBLIC virtual void setScore(std::string score);
  PUBLIC virtual int getArrangementCount() const;
  PUBLIC virtual std::string getArrangement(int index) const;
  PUBLIC virtual void addArrangement(std::string instrument);
  PUBLIC virtual void setArrangement(int index, std::string instrument);
  PUBLIC virtual void insertArrangement(int index, std::string instrument);
  PUBLIC virtual void removeArrangement(int index);
  PUBLIC virtual void setCSD(std::string xml);
  PUBLIC virtual std::string getCSD(void) const;
  PUBLIC virtual void addScoreLine(const std::string line);
  PUBLIC virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10, double p11);
  PUBLIC virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10);
  PUBLIC virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9);
  PUBLIC virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8);
  PUBLIC virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7);
  PUBLIC virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6);
  PUBLIC virtual void addNote(double p1, double p2, double p3, double p4, double p5);
  PUBLIC virtual void addNote(double p1, double p2, double p3, double p4);
  PUBLIC virtual void addNote(double p1, double p2, double p3);
  PUBLIC virtual bool exportForPerformance(void);
  PUBLIC virtual void removeAll(void);
  PUBLIC virtual void removeCommand(void);
  PUBLIC virtual void removeOrchestra(void);
  PUBLIC virtual void removeScore(void);
  PUBLIC virtual void removeArrangement(void);
  PUBLIC virtual void removeMidifile(void);
  //PUBLIC virtual void getInstrumentNames(std::vector<std::string> &names) const;
  PUBLIC virtual bool loadOrcLibrary(const char *filename = 0);
};

#endif   //     CSOUND_FILE_H

