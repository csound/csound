/*
    CsoundUtility.hpp:
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

#ifndef CSOUNDUTILITY_HPP
#define CSOUNDUTILITY_HPP

#include "csound.hpp"

#include <iostream>
#include <string>
#include <vector>

class CsoundGUIConsole;

class CsoundUtility {
 protected:
    CSOUND  *csound;
    void    *utilityThread;
    CsoundGUIConsole  *consoleWindow;
    int     status;     // 0: running, > 0: completed, < 0: failed
    std::vector<std::string>  args;
 public:
    CsoundUtility(CsoundGUIConsole *, std::vector<std::string>&);
    virtual ~CsoundUtility();
    int Stop();
    int GetStatus()
    {
      return status;
    }
 protected:
    virtual int runUtility_(int, char **);
 private:
    static int yieldCallback(CSOUND *);
    static uintptr_t threadFunc(void *);
};

class CsoundListOpcodesUtility : public CsoundUtility {
 public:
    CsoundListOpcodesUtility(CsoundGUIConsole *, std::vector<std::string>&);
    ~CsoundListOpcodesUtility();
 protected:
    int runUtility_(int, char **);
};

// ----------------------------------------------------------------------------

class CsoundUtilitySettings {
 public:
    // -----------------------------------------------------------------
    // list opcodes
    bool        listOpcodes_printDetails;
    // -----------------------------------------------------------------
    // cvanal
    std::string cvanal_inputFile;
    std::string cvanal_outputFile;
    int         cvanal_channel;
    double      cvanal_beginTime;
    double      cvanal_duration;
    // -----------------------------------------------------------------
    // pvanal
    std::string pvanal_inputFile;
    std::string pvanal_outputFile;
    int         pvanal_channel;
    double      pvanal_beginTime;
    double      pvanal_duration;
    int         pvanal_frameSize;
    int         pvanal_overlap;
    int         pvanal_hopSize;
    int         pvanal_windowType;      // 0: Hamming, 1: von Hann, 2: Kaiser
    // -----------------------------------------------------------------
    // hetro
    std::string hetro_inputFile;
    std::string hetro_outputFile;
    int         hetro_channel;
    double      hetro_beginTime;
    double      hetro_duration;
    double      hetro_startFreq;
    int         hetro_partials;
    double      hetro_maxAmp;
    double      hetro_minAmp;
    int         hetro_breakPoints;
    double      hetro_cutoffFreq;
    // -----------------------------------------------------------------
    // lpanal
    std::string lpanal_inputFile;
    std::string lpanal_outputFile;
    int         lpanal_channel;
    double      lpanal_beginTime;
    double      lpanal_duration;
    bool        lpanal_altMode;
    int         lpanal_poles;
    int         lpanal_hopSize;
    std::string lpanal_comment;
    double      lpanal_minFreq;
    double      lpanal_maxFreq;
    int         lpanal_verbosity;   // 0: normal, 1: verbose, 2: debug
    // -----------------------------------------------------------------
    // sndinfo
    std::string sndinfo_inputFile;
    // -----------------------------------------------------------------
    // srconv
    std::string srconv_inputFile;
    std::string srconv_outputFile;
    double      srconv_pitchRatio;
    double      srconv_sampleRate;
    int         srconv_quality;         // 1 to 8
    int         srconv_fileType;        // 0: RAW, 1: WAV, 2: AIFF, 3: IRCAM
    int         srconv_sampleFormat;    // 0: U8, 1: S16, 2: S32, 3: F32
    bool        srconv_peakChunks;
    bool        srconv_rewriteHeader;
    int         srconv_heartBeat;
    // -----------------------------------------------------------------
    // dnoise
    std::string dnoise_inputFile;
    std::string dnoise_outputFile;
    std::string dnoise_noiseFile;
    double      dnoise_beginTime;
    double      dnoise_endTime;
    int         dnoise_fftSize;         // log2(FFT size) - 6
    // dnoise_overlap controls the analysis window length:
    //   0: 4 * FFT size
    //   1: 2 * FFT size
    //   2: 1 * FFT size
    //   3: FFT size / 2
    int         dnoise_overlap;
    int         dnoise_synLen;          // log2(synthesis window length) - 5
    int         dnoise_decFact;         // log2(decimation factor) - 2
    double      dnoise_threshold;
    int         dnoise_sharpness;       // 1 to 5
    int         dnoise_numFFT;          // number of FFT frames to average
    double      dnoise_minGain;
    int         dnoise_fileType;        // 0: RAW, 1: WAV, 2: AIFF, 3: IRCAM
    int         dnoise_sampleFormat;    // 0: U8, 1: S16, 2: S32, 3: F32
    int         dnoise_heartBeat;
    bool        dnoise_rewriteHeader;
    bool        dnoise_verbose;
    // -----------------------------------------------------------------
    CsoundUtilitySettings();
    ~CsoundUtilitySettings();
};

CsoundUtility *CreateUtility_ListOpcodes(CsoundGUIConsole *,
                                         CsoundUtilitySettings&);
CsoundUtility *CreateUtility_Cvanal(CsoundGUIConsole *,
                                    CsoundUtilitySettings&);
CsoundUtility *CreateUtility_Pvanal(CsoundGUIConsole *,
                                    CsoundUtilitySettings&);
CsoundUtility *CreateUtility_Hetro(CsoundGUIConsole *,
                                   CsoundUtilitySettings&);
CsoundUtility *CreateUtility_Lpanal(CsoundGUIConsole *,
                                    CsoundUtilitySettings&);
CsoundUtility *CreateUtility_Sndinfo(CsoundGUIConsole *,
                                     CsoundUtilitySettings&);
CsoundUtility *CreateUtility_Srconv(CsoundGUIConsole *,
                                    CsoundUtilitySettings&);
CsoundUtility *CreateUtility_Dnoise(CsoundGUIConsole *,
                                    CsoundUtilitySettings&);

#endif  // CSOUNDUTILITY_HPP

