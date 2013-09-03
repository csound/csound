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
#include "Soundfile.hpp"
#include "Conversions.hpp"

namespace csound
{
  Soundfile::Soundfile() : sampleCount(0), startTimeSeconds(0.0)
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

  void Soundfile::jonesParksGrain(double centerTimeSeconds,
                                  double durationSeconds,
                                  double beginningFrequencyHz,
                                  double centerFrequencyHz,
                                  double centerAmplitude,
                                  double centerPhaseOffsetRadians,
                                  double pan,
                                  bool synchronousPhase,
                                  bool buffer)
  {
    if (synchronousPhase) {
      double wavelengthSeconds = 1.0 / centerFrequencyHz;
      double wavelengths = centerTimeSeconds / wavelengthSeconds;
      double wholecycles = 0;
      double fractionalCycle = std::modf(wavelengths, &wholecycles);
      centerPhaseOffsetRadians = Conversions::get2PI() * fractionalCycle;
    }
    double leftGain = Conversions::leftPan(pan);
    double rightGain = Conversions::rightPan(pan);
    double centerTime = - (durationSeconds / 2.0);
    int samplingRate = getFramesPerSecond();
    double samplingInterval = 1.0 / double(samplingRate);
    size_t frameCount = size_t(2.0 * durationSeconds / samplingInterval);
    double gaussianWidth = std::exp(1.0) / std::pow(durationSeconds / 4.0, 2.0);
    double endingFrequencyHz = centerFrequencyHz + (centerFrequencyHz - beginningFrequencyHz);
    double chirpRate = (endingFrequencyHz - beginningFrequencyHz) / durationSeconds;
    double omega = Conversions::get2PI() * centerFrequencyHz;
    std::complex<double> c0(log(centerAmplitude) - (gaussianWidth * std::pow(centerTime, 2.0)),
                            (centerPhaseOffsetRadians - (chirpRate / 2.0) * centerTime) - (omega * centerTime));
    std::complex<double> c1(-2.0 * gaussianWidth * samplingInterval * centerTime,
                            - (samplingInterval * (chirpRate * centerTime + omega)));
    std::complex<double> c2 = (-std::complex<double>(gaussianWidth, chirpRate / 2.0)) * std::pow(samplingInterval, 2.0);
    std::complex<double> exp_2_c2 = std::exp(2.0 * c2);
    std::complex<double> h0 = std::exp(c1 + c2);
    std::complex<double> h1(0.0, 0.0);
    std::complex<double> f0 = std::exp(c0);
    std::complex<double> f1(0.0, 0.0);
    size_t channelCount = getChannelsPerFrame();
    grainOutput.resize(frameCount, channelCount);
    grainBuffer.resize(frameCount, channelCount);
    for(size_t frameI = 0; frameI < frameCount; frameI++) {
      double sample = f0.real();
      //std::cout << sample << std::endl;
      if (channelCount == 2) {
        grainOutput(frameI, 0) += (leftGain * sample);
        grainOutput(frameI, 1) += (rightGain * sample);
      } else if (channelCount == 1) {
        grainOutput(frameI, 0) += sample;
      } else {
        for(size_t channelI = 0; channelI < channelCount; channelI++) {
          grainOutput(frameI, channelI) += sample;
        }
      }
      h1 = h0 * exp_2_c2;
      h0 = h1;
      f1 = h1 * f0;
      f0 = f1;
    }
    sampleCount = frameCount * channelCount;
    startTimeSeconds = centerTimeSeconds - (durationSeconds / 2.0);
    if (!buffer) {
      mixGrain();
    }
  }

  void Soundfile::cosineGrain(double centerTimeSeconds,
                              double durationSeconds,
                              double frequencyHz,
                              double amplitude,
                              double phaseOffsetRadians,
                              double pan,
                              bool synchronousPhase,
                              bool buffer)
  {
    if (synchronousPhase) {
      double wavelengthSeconds = 1.0 / frequencyHz;
      double wavelengths = centerTimeSeconds / wavelengthSeconds;
      double wholecycles = 0;
      double fractionalCycle = std::modf(wavelengths, &wholecycles);
      phaseOffsetRadians = Conversions::get2PI() * fractionalCycle;
    }
    size_t frameN = size_t(Conversions::round(durationSeconds * getFramesPerSecond()));
    size_t channelN = getChannelsPerFrame();
    grainOutput.resize(frameN, channelN);
    grainBuffer.resize(frameN, channelN);
    double leftGain = Conversions::leftPan(pan);
    double rightGain = Conversions::rightPan(pan);
    // The signal is a cosine sinusoid.
    double framesPerSecond = double(getFramesPerSecond());
    double sinusoidRadiansPerFrame = Conversions::get2PI() * frequencyHz / framesPerSecond;
    double sinusoidCoefficient = 2.0 * std::cos(sinusoidRadiansPerFrame);
    // The initial frame.
    double sinusoid1 = std::cos(phaseOffsetRadians);
    // What would have been the previous frame.
    double sinusoid2 = std::cos(-sinusoidRadiansPerFrame + phaseOffsetRadians);
    // The envelope is exactly 1 cycle of a cosine sinusoid, offset by -1.
    double envelopeFrequencyHz = 1.0 / durationSeconds;
    double envelopeRadiansPerFrame =  Conversions::get2PI() * envelopeFrequencyHz / framesPerSecond;
    double envelopeCoefficient = 2.0 * std::cos(envelopeRadiansPerFrame);
    // The initial frame.
    double envelope1 = std::cos(0.0);
    // What would have been the previous frame.
    double envelope2 = std::cos(-envelopeRadiansPerFrame);
     // Precompute grain into buffer.
    double signal = 0.0;
    double temporary = 0.0;
    for(size_t frameI = 0; frameI < frameN; frameI++)
      {
        signal = (sinusoid1 * (envelope1 - 1.0)) * amplitude;
        if (channelN == 2) {
          grainOutput(frameI, 0) += (leftGain * signal);
          grainOutput(frameI, 1) += (rightGain * signal);
        } else if (channelN == 1) {
          grainOutput(frameI, 0) += signal;
        } else {
          for(size_t channelI = 0; channelI < channelN; channelI++) {
            grainOutput(frameI, channelI) += signal;
          }
        }
        temporary = sinusoid1;
        sinusoid1 = sinusoidCoefficient * sinusoid1 - sinusoid2;
        sinusoid2 = temporary;
        temporary = envelope1;
        envelope1 = envelopeCoefficient * envelope1 - envelope2;
        envelope2 = temporary;
      }
    sampleCount = frameN * channelN;
    startTimeSeconds = centerTimeSeconds - (durationSeconds / 2.0);
    if (!buffer) {
      mixGrain();
    }
  }

  void Soundfile::mixGrain()
  {
    seekSeconds(startTimeSeconds);
    mixFrames(&grainOutput(0, 0), sampleCount, &grainBuffer(0, 0));
    grainOutput *= 0.0;
  }
}

