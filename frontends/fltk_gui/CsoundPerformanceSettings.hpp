/*
  CsoundPerformanceSettings.hpp:
  Copyright (C) 2006 Istvan Varga

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#ifndef CSOUNDPERFORMANCESETTINGS_HPP
#define CSOUNDPERFORMANCESETTINGS_HPP

#include <iostream>
#include <string>
#include <vector>

typedef struct deviceInfo {
  std::string description;
  int cardNum;
  int portNum;   //Only on ALSA
} deviceInfo_;


class CsoundPerformanceSettings {
public:
  std::string orcName;                // orchestra or CSD name
  std::string scoName;                // score name
  std::string soundFileType;          // "wav", "aiff", etc.
  std::string soundSampleFormat;      // "short", "long", "float", etc.
  bool        enablePeakChunks;       // true by default
  int         displayMode;            // 0: none, 1: full, 2: ASCII, 3: PS
  bool        deferGEN1;              // defer GEN01 loads (default: false)
  int         bufFrames_SW;           // software buffer size (-b)
  int         nBuffers;               // number of buffers (-B / -b)
  std::string midiInFileName;         // MIDI input file (-F)
  std::string midiOutFileName;        // MIDI output file (--midioutfile)
  bool        terminateOnMidi;        // terminate on end of MIDI file (-T)
  int         heartBeatMode;          // default: 0
  bool        rewriteHeader;          // update sndfile header (default: no)
  std::string scriptFileName;         // Python script filename
  std::string inputFileName;          // input sound file (-i)
  std::string outputFileName;         // output sound file (-o)
  bool        enableSoundOutput;      // default: true
  double      beatModeTempo;          // <= 0.0: disabled
  bool        iTimeOnly;              // i-time only orch run (default: no)
  double      sampleRateOverride;     // <= 0.0: disabled
  double      controlRateOverride;    // <= 0.0: disabled
  std::string lineInput;              // device name for line input (-L)
  int         messageLevel;           // console message level (-m)
  bool        enableExpressionOpt;    // optimize expressions (default: no)
  std::string sadirPath;              // SADIR (default: none)
  std::string ssdirPath;              // SSDIR (default: none)
  std::string sfdirPath;              // SFDIR (default: none)
  std::string incdirPath;             // INCDIR (default: none)
  std::string csdocdirPath;             // INCDIR (default: none)
  std::string strsets[10];            // --strset1 to --strset10
  bool        verbose;                // verbose orchestra compilation (-v)
  bool        enableDither;           // dither sound output
  std::string pluginLibs;             // additional plugins to load
  std::string sndidArtist;            // artist tag in output soundfile
  std::string sndidComment;           // comment tag in output soundfile
  std::string sndidCopyright;         // copyright tag in output soundfile
  std::string sndidDate;              // date tag in output soundfile
  std::string sndidSoftware;          // software tag in output soundfile
  std::string sndidTitle;             // title tag in output soundfile
  bool        ignoreCSDOptions;       // default: false
  std::string jackClientName;         // JACK client name
  std::string jackInPortName;         // JACK input port name prefix
  std::string jackOutPortName;        // JACK output port name prefix
  int         maxStrLen;              // max. length of string variables
  std::string midiFileMuteTracks;     // pattern of 0 and 1 characters
  int         midiKeyMidi;             // p-field to route MIDI key value
  int         midiKeyCps;              // p-field to route MIDI key value in Hz
  int         midiKeyOct;              // p-field to route MIDI key value in octave notation
  int         midiKeyPch;              // p-field to route MIDI key value in pitch class
  int         midiVelMidi;             // p-field to route MIDI velocity
  int         midiVelAmp;              // p-field to route MIDI velocity in fullscale amp
  bool        rawControllerMode;      // disable GM compatible MIDI behavior
  std::string rtAudioModule;          // "portaudio", "jack", etc.
  std::string rtAudioOutputDevice;    // Audio output device name or number for csound
  std::string rtAudioInputDevice;     // Audio input device name or number for csound
  std::string rtMidiModule;           // "portmidi", "mme", etc.
  std::string midiInDevName;          // MIDI input device (-M)
  std::string midiOutDevName;         // MIDI output device (-Q)
  double      scoreOffsetSeconds;     // number of seconds to skip
  bool        useThreads;             // use a separate audio thread
  bool        runRealtime;            // use dac instead of filename
  std::string additionalFlags;        // Addtional flags to pass directly to the command line
  bool        useAdditionalFlags;     // use additional flags
  bool        disableDiskOutput;      // disable writing output to disk
  // -----------------------------------------------------------------
  CsoundPerformanceSettings();
  ~CsoundPerformanceSettings();

  static int fileTypeToIndex(const char *);
  static const char *indexToFileType(int);
  static int sampleFormatToIndex(const char *);
  static const char *indexToSampleFormat(int);
  void buildCommandLine(std::vector<std::string>&, bool);
};

#endif  // CSOUNDPERFORMANCESETTINGS_HPP

