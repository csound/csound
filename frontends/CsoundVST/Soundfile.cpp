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
#include "Soundfile.hpp"

namespace csound
{
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
                        framebuffer.resize(0);
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
                        }
                        framebuffer.resize(getChannelsPerFrame());
                        return (int) sndfile;
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
                        }
                        framebuffer.resize(getChannelsPerFrame());
                        return (int) sndfile;
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
                        int frame = seconds * double(sf_info.samplerate);
                        frame = sf_seek(sndfile, frame, whence);
                        if (frame == -1) {
                                error();
                        }
                        return frame;
                }
        int Soundfile::read(double *samples, int frames)
                {
                        return sf_readf_double(sndfile, samples, frames);
                }
        int Soundfile::write(double *samples, int frames)
                {
                        return sf_writef_double(sndfile, samples, frames);
                }

        void Soundfile::updateHeader()
                {
                        int status = sf_command(sndfile, SFC_UPDATE_HEADER_NOW, 0, 0);
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
}
