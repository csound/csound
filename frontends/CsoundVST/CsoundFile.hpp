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
#pragma warning(disable: 4786)
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
#include <io.h>
#endif
#endif

inline void gatherArgs(int argc, const char **argv, std::string &commandLine)
{
	for(int i = 0; i < argc; i++)
	{
		if(i == 0)
		{
			commandLine = argv[i];
		}
		else
		{
			commandLine += " ";
			commandLine += argv[i];
		}
	}
}

inline void scatterArgs(const std::string commandLine, int *argc, char ***argv)
{
	char *buffer = strdup(commandLine.c_str());
	char *separators = " \t\n\r";
	int argcc = 0;
	char **argvv = (char **) calloc(sizeof(char *), 100);
	char *token = strtok(buffer, separators);   
	while(token)
	{
		argvv[argcc] = strdup(token);
		argcc++;
		token = strtok(0, separators);
	}
	*argc = argcc;
	*argv = (char **) realloc(argvv, sizeof(char *) * (argcc + 1));
	free(buffer);
}

inline void deleteArgs(int argc, char **argv)
{
	if(!argv)
	{
		return;
	}
	for(int i = 0; i < argc; i++)
	{
		if(argv[i])
		{
			free(argv[i]);
		}
	}
	free(argv);
}

inline std::string &trim(std::string &value)
{
	size_t i = value.find_first_not_of(" \n\r\t");
	if(i != value.npos)
	{
		value.erase(0, i);
	}
	else
	{
		value.erase(value.begin(), value.end());
		return value;
	}
	i = value.find_last_not_of(" \n\r\t");
	if(i != value.npos)
	{
		value.erase(i + 1, value.npos);
	}
	return value;
}

inline std::string &trimQuotes(std::string &value)
{
	size_t i = value.find_first_not_of("\"");
	if(i != value.npos)
	{
		value.erase(0, i);
	}
	i = value.find_last_not_of("\"");
	if(i != value.npos)
	{
		value.erase(i + 1, value.npos);
	}
	return value;
}

/**
*	Returns true if definition is a valid Csound instrument definition block.
*	Also returns the part before the instr number, the instr number, 
*	the name (all text after the first comment on the same line as the instr number),
*	and the part after the instr number, all by reference.
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
	*	What are we storing, anyway?
	*/
	std::string filename;
	/**
	*	CsOptions
	*/
	std::string command;
	/**
	*	CsInstruments
	*/
	std::string orchestra;
	/**
	*	CsScore
	*/
	std::string score;
	/**
	*	CsMidi
	*/
	std::vector<unsigned char> midifile;
public:
	/**
	*	Patch library and arrangement.
	*/
	std::string libraryFilename;
	std::vector<std::string> arrangement;
	CsoundFile();
	virtual ~CsoundFile(void){};
	virtual std::string generateFilename();
	virtual std::string getFilename(void);
	virtual void setFilename(std::string name);
	virtual int load(std::string filename);
	virtual int load(std::istream &stream);
	virtual int save(std::string filename) const;
	virtual int save(std::ostream &stream) const;
	virtual int importFile(std::string filename);
	virtual int importFile(std::istream &stream);
	virtual int importCommand(std::istream &stream);
	virtual int exportCommand(std::ostream &stream) const;
	virtual int importOrchestra(std::istream &stream);
	virtual int exportOrchestra(std::ostream &stream) const;
	virtual int importScore(std::istream &stream);
	virtual int exportScore(std::ostream &stream) const;
	virtual int importArrangement(std::istream &stream);
	virtual int exportArrangement(std::ostream &stream) const;
	virtual int exportArrangementForPerformance(std::string filename) const;
	virtual int exportArrangementForPerformance(std::ostream &stream) const;
	virtual int importMidifile(std::istream &stream);
	virtual int exportMidifile(std::ostream &stream) const;
	virtual std::string getCommand(void) const;
	virtual void setCommand(std::string commandLine);
	virtual std::string getOrcFilename(void) const;
	virtual std::string getScoFilename(void) const;
	virtual std::string getMidiFilename(void) const;
	virtual std::string getOutputSoundfileName(void) const;
	virtual std::string getOrchestra(void) const;
	virtual void setOrchestra(std::string orchestra);
	virtual int getInstrumentCount(void) const;
	virtual std::string getOrchestraHeader(void) const;
	virtual bool getInstrument(int number, std::string &definition) const;
	//virtual bool getInstrumentNumber(int index, std::string &definition) const;
	virtual bool getInstrument(std::string name, std::string &definition) const;
	virtual std::string getScore() const;
	virtual void setScore(std::string score);
	virtual int getArrangementCount() const;
	virtual std::string getArrangement(int index) const;
	virtual void addArrangement(std::string instrument);
	virtual void setArrangement(int index, std::string instrument);
	virtual void insertArrangement(int index, std::string instrument);
	virtual void removeArrangement(int index);
	virtual void setCSD(std::string xml);
	virtual std::string getCSD(void) const;
	virtual void addScoreLine(const std::string line);
	virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10, double p11);
	virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10);
	virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9);
	virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8);
	virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7);
	virtual void addNote(double p1, double p2, double p3, double p4, double p5, double p6);
	virtual void addNote(double p1, double p2, double p3, double p4, double p5);
	virtual void addNote(double p1, double p2, double p3, double p4);
	virtual void addNote(double p1, double p2, double p3);
	virtual bool exportForPerformance(void);
	virtual void removeAll(void);
	virtual void removeCommand(void);
	virtual void removeOrchestra(void);
	virtual void removeScore(void);
	virtual void removeArrangement(void);
	virtual void removeMidifile(void);
	//virtual void getInstrumentNames(std::vector<std::string> &names) const;
	virtual bool loadOrcLibrary(const char *filename = 0);
};

#endif	 //	CSOUND_FILE_H



