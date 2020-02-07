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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "Soundfile.hpp"
#include <csound.h>
#include <complex>

namespace csound
{
#ifndef PI
#define PI      (3.14159265358979323846)
#endif
#ifndef TWOPI
#define TWOPI   (6.28318530717958647692)
#endif

  Soundfile::Soundfile()
  {
    initialize();
  }
  Soundfile::~Soundfile()
  {
    close();
  }
  void Soundfile::initialize()
  {
    sndfile = 0;
    std::memset(&sf_info, 0, sizeof(sf_info));
  }
  int Soundfile::getFramesPerSecond() const
  {
    return sf_info.samplerate;
  }
  void Soundfile::setFramesPerSecond(int framesPerSecond)
  {
    sf_info.samplerate = framesPerSecond;
  }
  int Soundfile::getChannelsPerFrame() const
  {
    return sf_info.channels;
  }
  void Soundfile::setChannelsPerFrame(int channelsPerFrame)
  {
    sf_info.channels = channelsPerFrame;
  }
  int Soundfile::getFormat() const
  {
    return sf_info.format;
  }
  void Soundfile::setFormat(int format)
  {
    sf_info.format = format;
  }
  int Soundfile::getFrames() const
  {
    return (int) sf_info.frames;
  }
  int Soundfile::open(std::string filename)
  {
    close();
    sndfile = sf_open(filename.c_str(), SFM_RDWR, &sf_info);
    if (!sndfile) {
      error();
      return -1;
    }
    return 0;
  }
  int Soundfile::create(std::string filename, int framesPerSecond, int channelsPerFrame, int format)
  {
    close();
    sf_info.samplerate = framesPerSecond;
    sf_info.channels = channelsPerFrame;
    sf_info.format = format;
    sndfile = sf_open(filename.c_str(), SFM_RDWR, &sf_info);
    if (!sndfile) {
      error();
      return -1;
    }
    return 0;
  }
  int Soundfile::seek(int frames, int whence)
  {
    int frame = sf_seek(sndfile, frames, whence);
    if (frame == -1) {
      error();
    }
    return frame;
  }
  double Soundfile::seekSeconds(double seconds, int whence)
  {
    int frame = int(seconds * double(sf_info.samplerate));
    frame = sf_seek(sndfile, frame, whence);
    if (frame == -1) {
      error();
    }
    return frame;
  }
  int Soundfile::readFrame(double *outputFrame)
  {
    return sf_readf_double(sndfile, outputFrame, 1);
  }
  int Soundfile::writeFrame(double *inputFrame)
  {
    return sf_writef_double(sndfile, inputFrame, 1);
  }
  int Soundfile::readFrames(double *outputFrames, int samples)
  {
    return sf_read_double(sndfile, outputFrames, samples);
  }
  int Soundfile::writeFrames(double *inputFrames, int samples)
  {
    return sf_write_double(sndfile, inputFrames, samples);
  }
  int Soundfile::mixFrames(double *inputFrames, int samples, double *mixedFrames)
  {
    size_t position = sf_seek(sndfile, 0, SEEK_CUR);
    sf_readf_double(sndfile, mixedFrames, samples);
    for (int i = 0; i < samples; i++) {
      mixedFrames[i] += inputFrames[i];
    }
    sf_seek(sndfile, position, SEEK_SET);
    return sf_writef_double(sndfile, mixedFrames, samples);
  }
  void Soundfile::updateHeader()
  {
    /* int status = */ sf_command(sndfile, SFC_UPDATE_HEADER_NOW, 0, 0);
  }
  int Soundfile::close()
  {
    int status = 0;
    if (sndfile) {
      status = sf_close(sndfile);
      if (status) {
        std::cerr << sf_error_number(status) << std::endl;
      }
    }
    initialize();
    return status;
  }
  void Soundfile::error() const
  {
    std::cerr << sf_strerror(sndfile) << std::endl;
  }

  void Soundfile::blank(double duration)
  {
    seekSeconds(0.0);
    std::vector<double> frame;
    frame.resize(getChannelsPerFrame());
    int framesToWrite = int(duration * getFramesPerSecond());
    for (int i = 0; i < framesToWrite; i++) {
      sf_writef_double(sndfile, &frame.front(), 1);
    }
    updateHeader();
    seekSeconds(0.0);
  }

}
