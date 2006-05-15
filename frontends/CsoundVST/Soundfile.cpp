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
#include "Conversions.hpp"
#include <complex>
#include <boost/numeric/ublas/matrix.hpp>

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

  void Soundfile::jonesParksGrain(double centerTime_,
                                  double duration,
                                  double beginningFrequency,
                                  double centerFrequency,
                                  double centerAmplitude,
                                  double centerPhase,
                                  double pan)
  {
    double leftGain = Conversions::leftPan(pan);
    double rightGain = Conversions::rightPan(pan);
    double centerTime = - (duration / 2.0);
    int samplingRate = getFramesPerSecond();
    double samplingInterval = 1.0 / double(samplingRate);
    size_t frameCount = size_t(2.0 * duration / samplingInterval);
    double gaussianWidth = std::exp(1.0) / std::pow(duration / 4.0, 2.0);
    double endingFrequency = centerFrequency + (centerFrequency - beginningFrequency);
    double chirpRate = (endingFrequency - beginningFrequency) / duration;
    double omega = Conversions::TWO_PI_ * centerFrequency;
    std::complex<double> c0(log(centerAmplitude) - (gaussianWidth * std::pow(centerTime, 2.0)),
                            (centerPhase - (chirpRate / 2.0) * centerTime) - (omega * centerTime));
    std::complex<double> c1(-2.0 * gaussianWidth * samplingInterval * centerTime,
                            - (samplingInterval * (chirpRate * centerTime + omega)));
    std::complex<double> c2 = (-std::complex<double>(gaussianWidth, chirpRate / 2.0)) * std::pow(samplingInterval, 2.0);
    std::complex<double> exp_2_c2 = std::exp(2.0 * c2);
    std::complex<double> h0 = std::exp(c1 + c2);
    std::complex<double> h1(0.0, 0.0);
    std::complex<double> f0 = std::exp(c0);
    std::complex<double> f1(0.0, 0.0);
    size_t channelCount = getChannelsPerFrame();
    boost::numeric::ublas::matrix<double> grainOutput(frameCount, channelCount);
    boost::numeric::ublas::matrix<double> grainBuffer(frameCount, channelCount);
    for(size_t frameI = 0; frameI < frameCount; frameI++)
      {
        double sample = f0.real();
        //std::cout << sample << std::endl;
        if (channelCount == 2) {
          grainOutput(frameI, 0) = leftGain * sample;
          grainOutput(frameI, 1) = rightGain * sample;
        } else if (channelCount == 1) {
          grainOutput(frameI, 0) = sample;
        } else {
          for(size_t channelI = 0; channelI < channelCount; channelI++) {
            grainOutput(frameI, channelI) = sample;
          }
        }
        h1 = h0 * exp_2_c2;
        h0 = h1;
        f1 = h1 * f0;
        f0 = f1;
      }
    seekSeconds(centerTime_ - (duration / 2.0));
    int sampleCount = frameCount * channelCount;
    mixFrames(&grainOutput(0, 0), sampleCount, &grainBuffer(0, 0));
  }

  void Soundfile::cosineGrain(double centerTime, double duration, double sineFrequency, double gain, double sinePhase, double pan)
  {
    int framesPerSecond = getFramesPerSecond();
    double leftGain = Conversions::leftPan(pan);
    double rightGain = Conversions::rightPan(pan);
    double sineAngularFrequency = Conversions::get2PI() * sineFrequency / double(framesPerSecond);
    double sineCoefficient = 2.0 * std::cos (sineAngularFrequency);
    double sineSignal2 = std::sin (sinePhase + sineAngularFrequency);
    double sineSignal1 = std::sin (sinePhase + 2.0 * sineAngularFrequency);
    double cosineFrequency = 1.0 / duration;
    size_t frameCount = (size_t) Conversions::round (duration * getFramesPerSecond());
    double cosineAngularFrequency =  Conversions::get2PI() * cosineFrequency / double(framesPerSecond);
    double cosineCoefficient = 2.0 * std::cos (cosineAngularFrequency);
    double cosineSignal2 = std::cos (cosineAngularFrequency);
    double cosineSignal1 = std::cos (2.0 * cosineAngularFrequency);
    double signal = 0.0;
    double sineSignal = 0.0;
    double cosineSignal = 0.0;
    size_t channelCount = getChannelsPerFrame();
    boost::numeric::ublas::matrix<double> grainOutput(frameCount, channelCount);
    boost::numeric::ublas::matrix<double> grainBuffer(frameCount, channelCount);
    for(size_t frameI = 0; frameI < frameCount; frameI++)
      {
        if (channelCount == 2) {
          grainOutput(frameI, 0) = leftGain * signal;
          grainOutput(frameI, 1) = rightGain * signal;
        } else if (channelCount == 1) {
          grainOutput(frameI, 0) = signal;
        } else {
          for(size_t channelI = 0; channelI < channelCount; channelI++) {
            grainOutput(frameI, channelI) = signal;
          }
        }
        sineSignal = sineCoefficient * sineSignal1 - sineSignal2;
        sineSignal2 = sineSignal1;
        sineSignal1 = sineSignal;
        cosineSignal = cosineCoefficient * cosineSignal1 - cosineSignal2;
        cosineSignal2 = cosineSignal1;
        cosineSignal1 = cosineSignal;
        signal = (sineSignal * (cosineSignal - 1.0)) * gain;
      }
    seekSeconds(centerTime - (duration / 2.0));
    int sampleCount = frameCount * channelCount;
    mixFrames(&grainOutput(0, 0), sampleCount, &grainBuffer(0, 0));
  }

}
