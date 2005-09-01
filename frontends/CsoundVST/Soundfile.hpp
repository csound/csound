/*
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
#ifndef CSOUNDVST_SOUNDFILE_H
#define CSOUNDVST_SOUNDFILE_H
#ifdef SWIG
%module CsoundVST
%{
#include <sndfile.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
%}
%include "std_string.i"
%include "carrays.i"
%array_class(double, doubleArray)
#else
#include <sndfile.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#endif

namespace csound
{
	/**
	* Simple, basic read/write access, in sample frames, to PCM soundfiles.
	* Reads and writes any format, but write defaults to WAV float format.
	* This class is designed for Python wrapping with SWIG.
	*/
	class Soundfile
	{
		SNDFILE *sndfile;
		SF_INFO sf_info;
		std::vector<double> framebuffer;
		public:
			Soundfile();
			virtual ~Soundfile() ;
			virtual void initialize() ;
			virtual int getFramesPerSecond() const;
			virtual void setFramesPerSecond(int framesPerSecond);
			virtual int getChannelsPerFrame() const;
			virtual void setChannelsPerFrame(int channelsPerFrame);
			virtual int getFormat() const;
			virtual void setFormat(int format);
			virtual int getFrames() const;
			virtual int open(std::string filename);
			virtual int create(std::string filename, int framesPerSecond = 44100, int channelsPerFrame = 2, int format = SF_FORMAT_WAV | SF_FORMAT_FLOAT);
			/**
			* Set whence to 0 for SEEK_SET, 1 for SEEK_CUR, 2 for SEEK_END.
			* Calling with whence = SEEK_CUR and frames = 0 returns the current frame pointer.
			*/
			virtual int seek(int frames, int whence = 0);
			virtual double seekSeconds(double seconds, int whence = 0);
			virtual int read(double *samples, int frames);
			virtual int write(double *frames, int samples);
			virtual void updateHeader();
			virtual int close() ;
			virtual void error() const;
	};
}
#endif
