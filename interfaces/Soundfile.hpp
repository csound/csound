/*
 * C S O U N D
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
#ifndef CSOUND_SOUNDFILE_H
#define CSOUND_SOUNDFILE_H

#ifdef SWIG
%module csnd6
%{
#include <sndfile.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
  %}
%include "std_string.i"
#ifdef SWIGPYTHON
%typemap(in) double *outputFrame {
  static double buffer[16];
  $1 = &buffer[0];
  for (int i = 0, n = PySequence_Size($input); i < n; i++) {
    PyObject *o = PyFloat_FromDouble($1[i]);
    PySequence_SetItem($input, i, o);
  }
}
%typemap(in) double *inputFrame {
  static double buffer[16];
  $1 = &buffer[0];
  for (int i = 0, n = PySequence_Size($input); i < n; i++) {
    PyObject *o = PySequence_ITEM($input, i);
    $1[i] = PyFloat_AS_DOUBLE(o);
  }
}
%typemap(in) (double *outputFrames, int samples) {
  $1 = (double *) PyString_AsString($input);
  $2 = PyString_Size($input) / sizeof(double);
}
%typemap(in) (double *inputFrames, int samples) {
  $1 = (double *) PyString_AsString($input);
  $2 = PyString_Size($input) / sizeof(double);
}
%typemap(in) double *mixedFrames {
  $1 = (double *) PyString_AsString($input);
}

#endif
#else
#include <sndfile.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#endif

#if defined(WIN32)
#define SILENCE_PUBLIC __declspec(dllexport)
#elif defined(__GNUC__)
#define SILENCE_PUBLIC __attribute__ ( (visibility("default")) )
#else
#define SILENCE_PUBLIC
#endif

namespace csound
{
  /**
   * Simple, basic read/write access, in sample frames, to PCM soundfiles.
   * Reads and writes any format, but write defaults to WAV float format.
   * This class is designed for Python wrapping with SWIG.
   * See http://www.mega-nerd.com/libsndfile for more information
   * on the underlying libsndfile library.
   */
#ifdef SWIG
  class Soundfile
#else
  class SILENCE_PUBLIC Soundfile
#endif
  {
    SNDFILE *sndfile;
    SF_INFO sf_info;
  protected:
    virtual void initialize() ;
  public:
    Soundfile();
    virtual ~Soundfile() ;
    virtual int getFramesPerSecond() const;
    virtual void setFramesPerSecond(int framesPerSecond);
    virtual int getChannelsPerFrame() const;
    virtual void setChannelsPerFrame(int channelsPerFrame);
    /**
     * See sndfile.h for a descriptive list of format numbers.
     */
    virtual int getFormat() const;
    /**
     * See sndfile.h for a descriptive list of format numbers.
     */
    virtual void setFormat(int format);
    /**
     * Return the number of sample frames in a just opened file,
     * or just after calling updateHeader.
     */
    virtual int getFrames() const;
    /**
     * Open an existing soundfile for reading and/or writing.
     */
    virtual int open(std::string filename);
    /**
     * Create a new soundfile for writing and/or reading.
     * The default soundfile format is WAV PCM float samples at 44100 frames per second, stereo.
     */
    virtual int create(std::string filename, int framesPerSecond = 44100, int channelsPerFrame = 2, int format = SF_FORMAT_WAV | SF_FORMAT_FLOAT);
    /**
     * Position the soundfile read/write pointer at the indicated sample frame.
     * Set whence to 0 for SEEK_SET, 1 for SEEK_CUR, 2 for SEEK_END.
     * Calling with whence = SEEK_CUR and frames = 0 returns the current read/write pointer.
     */
    virtual int seek(int frames, int whence = 0);
    virtual double seekSeconds(double seconds, int whence = 0);
    /**
     * Read one sample frame, and return it in a double array (in C++) or a sequence (in Python).
     * The array or the sequence must already contain as many elements as there are channels.
     * For efficiency, there is no bounds checking.
     */
    virtual int readFrame(double *outputFrame);
    /**
     * Write one sample frame, from a double array (in C++) or a sequence (in Python).
     * The array or the sequence must contain as many elements as there are channels.
     * For efficiency, there is no checking of bounds or type in Python; the string must contain Floats.
     * In Python this function is not thread-safe, as a static buffer is used internally.
     */
    virtual int writeFrame(double *inputFrame);
    /**
     * Read one or more samples, and return them in a double array (in C++) or a binary string (in Python).
     * The array or the string must already contain as many elements as there are samples (channels times frames).
     * Channels are interleaved within frames.
     * For efficiency, there is no bounds checking; on return the string will contain binary Float64.
     * In Python this function is not thread-safe, as a static buffer is used internally.
     */
    virtual int readFrames(double *outputFrames, int samples);
    /**
     * Write one or more samples, from a double array (in C++) or a binary string (in Python).
     * The array or the string must contain as many elements as there are samples (channels times frames)
     * Channels are interleaved within frames.
     * For efficiency, there is no checking of bounds or type in Python; the string must contain binary Float64.
     */
    virtual int writeFrames(double *inputFrames, int samples);
    /**
     * Mix one or more samples, from a double array (in C++) or a binary string (in Python),
     * into the existing signal in the soundfile.
     * The arrays or the strings must contain as many elements as there are samples (channels times frames)
     * Channels are interleaved within frames.
     * For efficiency, there is no checking of bounds or type in Python; the string must contain binary Float64.
     */
    virtual int mixFrames(double *inputFrames, int samples, double *mixedFrames);
    /**
     * Update the soundfile header with the current file size,
     * RIFF chunks, and so on.
     */
    virtual void updateHeader();
    /**
     * Close the soundfile. Should be called once for every opened or created soundfile,
     * although the class destructor will automatically close an open soundfile.
     */
    virtual int close() ;
    /**
     * Print to stderr any current error status message.
     */
    virtual void error() const;
    /**
     * Make the soundfile be so many seconds of silence.
     */
    virtual void blank(double duration);
  };
}
#endif
