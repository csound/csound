/*
    CsoundUtility.cpp:
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

#include "CsoundGUI.hpp"
using namespace std;

int CsoundUtility::yieldCallback(CSOUND *csound)
{
    CsoundUtility   *p;

    p = *((CsoundUtility**) csoundQueryGlobalVariable(csound,
                                                      "_csound5gui_utility"));
    if (!p)
      return 0;
    if (p->status != 0)
      return 0;
    return 1;
}

uintptr_t CsoundUtility::threadFunc(void *userData)
{
    CsoundUtility   *p;

    p = (CsoundUtility*) userData;
    p->csound = csoundCreate((void*) p->consoleWindow);
    if (p->csound == (CSOUND*) 0) {
      p->args.clear();
      p->status = -1;
      return (uintptr_t) ((unsigned int) p->status);
    }
    csoundSetMessageCallback(p->csound,
                             &CsoundGUIConsole::messageCallback_Thread);
    if (csoundCreateGlobalVariable(p->csound, "_csound5gui_utility",
                                   sizeof(CsoundUtility*)) == 0) {
      vector<char *>   argv_;
      int                   argc_;
      *((CsoundUtility**)
            csoundQueryGlobalVariable(p->csound, "_csound5gui_utility")) = p;
      for (argc_ = 0; argc_ < (int) p->args.size(); argc_++)
        argv_.push_back(const_cast<char *>(p->args[argc_].c_str()));
      argv_.push_back((char*) 0);
      csoundSetIsGraphable(p->csound, 1);     // disable FLTK graphs
      csoundSetYieldCallback(p->csound, &CsoundUtility::yieldCallback);
      if (p->runUtility_(argc_, &(argv_.front())) < 0)
        p->status = -1;
      else
        p->status = 1;
    }
    else
      p->status = -1;

    csoundDestroy(p->csound);
    p->csound = (CSOUND*) 0;
    p->args.clear();

    return (uintptr_t) ((unsigned int) p->status);
}

int CsoundUtility::runUtility_(int argc, char **argv)
{
    return csoundRunUtility(csound, argv[0], argc, argv);
}

int CsoundListOpcodesUtility::runUtility_(int argc, char **argv)
{
    return csoundCompile(csound, argc, argv);
}

int CsoundUtility::Stop()
{
    if (status == 0)
      status = 1;
    if (utilityThread) {
      status = (int) ((unsigned int) csoundJoinThread(utilityThread));
      utilityThread = (void*) 0;
    }
    return status;
}

CsoundUtility::CsoundUtility(CsoundGUIConsole *consoleWindow,
                             vector<string>& args)
{
    csound = (CSOUND*) 0;
    utilityThread = (void*) 0;
    this->consoleWindow = consoleWindow;
    status = 0;
    this->args = args;
    utilityThread = csoundCreateThread(&CsoundUtility::threadFunc,
                                       (void*) this);
    if (!utilityThread)
      status = -1;
}

CsoundUtility::~CsoundUtility()
{
    this->Stop();
    utilityThread = (void*) 0;
    args.clear();
}

CsoundListOpcodesUtility::CsoundListOpcodesUtility(CsoundGUIConsole
                                                       *consoleWindow,
                                                   vector<string>&
                                                       args)
    : CsoundUtility(consoleWindow, args)
{
}

CsoundListOpcodesUtility::~CsoundListOpcodesUtility()
{
}

// ----------------------------------------------------------------------------

CsoundUtilitySettings::CsoundUtilitySettings()
{
    listOpcodes_printDetails = true;
    // -----------------------------------------------------------------
    cvanal_inputFile = "";
    cvanal_outputFile = "";
    cvanal_channel = 0;
    cvanal_beginTime = 0.0;
    cvanal_duration = -1.0;
    // -----------------------------------------------------------------
    pvanal_inputFile = "";
    pvanal_outputFile = "";
    pvanal_channel = 0;
    pvanal_beginTime = 0.0;
    pvanal_duration = -1.0;
    pvanal_frameSize = 2048;
    pvanal_overlap = 0;
    pvanal_hopSize = 128;
    pvanal_windowType = 1;
    // -----------------------------------------------------------------
    hetro_inputFile = "";
    hetro_outputFile = "";
    hetro_channel = 1;
    hetro_beginTime = 0.0;
    hetro_duration = -1.0;
    hetro_startFreq = 100.0;
    hetro_partials = 10;
    hetro_maxAmp = 32767.0;
    hetro_minAmp = 64.0;
    hetro_breakPoints = 256;
    hetro_cutoffFreq = -1.0;
    // -----------------------------------------------------------------
    lpanal_inputFile = "";
    lpanal_outputFile = "";
    lpanal_channel = 1;
    lpanal_beginTime = 0.0;
    lpanal_duration = -1.0;
    lpanal_altMode = false;
    lpanal_poles = 34;
    lpanal_hopSize = 200;
    lpanal_comment = "";
    lpanal_minFreq = 70.0;
    lpanal_maxFreq = 200.0;
    lpanal_verbosity = 0;
    // -----------------------------------------------------------------
    sndinfo_inputFile = "";
    // -----------------------------------------------------------------
    srconv_inputFile = "";
    srconv_outputFile = "";
    srconv_pitchRatio = 1.0;
    srconv_sampleRate = 44100.0;
    srconv_quality = 4;
    srconv_fileType = 1;
    srconv_sampleFormat = 1;
    srconv_peakChunks = true;
    srconv_rewriteHeader = false;
    srconv_heartBeat = 0;
    // -----------------------------------------------------------------
    dnoise_inputFile = "";
    dnoise_outputFile = "";
    dnoise_noiseFile = "";
    dnoise_beginTime = 0.0;
    dnoise_endTime = -1.0;
    dnoise_fftSize = 4;         // for an FFT size of 1024
    dnoise_overlap = 2;
    dnoise_synLen = 0;
    dnoise_decFact = 0;
    dnoise_threshold = 30.0;
    dnoise_sharpness = 1;
    dnoise_numFFT = 5;
    dnoise_minGain = -40.0;
    dnoise_fileType = 1;
    dnoise_sampleFormat = 1;
    dnoise_heartBeat = 0;
    dnoise_rewriteHeader = false;
    dnoise_verbose = false;
}

CsoundUtilitySettings::~CsoundUtilitySettings()
{
}

// ----------------------------------------------------------------------------

static void cmdLine_addIntegerOpt(vector<string>& cmdLine,
                                  const char *optName, int value)
{
    char    buf[64];

    sprintf(&(buf[0]), "%s%d", optName, value);
    cmdLine.push_back(&(buf[0]));
}

static void cmdLine_addDoubleOpt(vector<string>& cmdLine,
                                 const char *optName, double value)
{
    char    buf[64];

    if (!(value > -10000000.0 && value < 10000000.0))
      return;
    sprintf(&(buf[0]), "%s%g", optName, value);
    cmdLine.push_back(&(buf[0]));
}

CsoundUtility *CreateUtility_ListOpcodes(CsoundGUIConsole *consoleWindow,
                                         CsoundUtilitySettings& parm)
{
    vector<string>  args;

    args.push_back("csound");
    if (parm.listOpcodes_printDetails)
      args.push_back("-z1");
    else
      args.push_back("-z");
    return (CsoundUtility*) (new CsoundListOpcodesUtility(consoleWindow,
                                                          args));
}

CsoundUtility *CreateUtility_Cvanal(CsoundGUIConsole *consoleWindow,
                                    CsoundUtilitySettings& parm)
{
    vector<string>  args;
    string               arg;

    if (CsoundGUIMain::isEmptyString(parm.cvanal_inputFile) ||
        CsoundGUIMain::isEmptyString(parm.cvanal_outputFile))
      return (CsoundUtility*) 0;
    args.push_back("cvanal");
    if (parm.cvanal_channel > 0)
      cmdLine_addIntegerOpt(args, "-c", parm.cvanal_channel);
    if (parm.cvanal_beginTime > 0.0)
      cmdLine_addDoubleOpt(args, "-b", parm.cvanal_beginTime);
    if (parm.cvanal_duration > 0.0)
      cmdLine_addDoubleOpt(args, "-d", parm.cvanal_duration);
    CsoundGUIMain::stripString(arg, parm.cvanal_inputFile.c_str());
    args.push_back(arg);
    CsoundGUIMain::stripString(arg, parm.cvanal_outputFile.c_str());
    args.push_back(arg);

    return (new CsoundUtility(consoleWindow, args));
}

CsoundUtility *CreateUtility_Pvanal(CsoundGUIConsole *consoleWindow,
                                    CsoundUtilitySettings& parm)
{
    vector<string>  args;
    string               arg;

    if (CsoundGUIMain::isEmptyString(parm.pvanal_inputFile) ||
        CsoundGUIMain::isEmptyString(parm.pvanal_outputFile))
      return (CsoundUtility*) 0;
    args.push_back("pvanal");
    if (parm.pvanal_channel > 0)
      cmdLine_addIntegerOpt(args, "-c", parm.pvanal_channel);
    if (parm.pvanal_beginTime > 0.0)
      cmdLine_addDoubleOpt(args, "-b", parm.pvanal_beginTime);
    if (parm.pvanal_duration > 0.0)
      cmdLine_addDoubleOpt(args, "-d", parm.pvanal_duration);
    cmdLine_addIntegerOpt(args, "-n", parm.pvanal_frameSize);
    if (parm.pvanal_overlap > 1)
      cmdLine_addIntegerOpt(args, "-w", parm.pvanal_overlap);
    else if (parm.pvanal_hopSize > 8)
      cmdLine_addIntegerOpt(args, "-h", parm.pvanal_hopSize);
    switch (parm.pvanal_windowType) {
    case 0:
      args.push_back("-H");
      break;
    case 2:
      args.push_back("-K");
      break;
    }
    CsoundGUIMain::stripString(arg, parm.pvanal_inputFile.c_str());
    args.push_back(arg);
    CsoundGUIMain::stripString(arg, parm.pvanal_outputFile.c_str());
    args.push_back(arg);

    return (new CsoundUtility(consoleWindow, args));
}

CsoundUtility *CreateUtility_Hetro(CsoundGUIConsole *consoleWindow,
                                   CsoundUtilitySettings& parm)
{
    vector<string>  args;
    string               arg;

    if (CsoundGUIMain::isEmptyString(parm.hetro_inputFile) ||
        CsoundGUIMain::isEmptyString(parm.hetro_outputFile))
      return (CsoundUtility*) 0;
    args.push_back("hetro");
    if (parm.hetro_channel > 0)
      cmdLine_addIntegerOpt(args, "-c", parm.hetro_channel);
    if (parm.hetro_beginTime > 0.0)
      cmdLine_addDoubleOpt(args, "-b", parm.hetro_beginTime);
    if (parm.hetro_duration > 0.0)
      cmdLine_addDoubleOpt(args, "-d", parm.hetro_duration);
    if (parm.hetro_startFreq > 0.0)
      cmdLine_addDoubleOpt(args, "-f", parm.hetro_startFreq);
    if (parm.hetro_partials > 0)
      cmdLine_addIntegerOpt(args, "-h", parm.hetro_partials);
    if (parm.hetro_maxAmp > 0.0)
      cmdLine_addDoubleOpt(args, "-M", parm.hetro_maxAmp);
    if (parm.hetro_minAmp > 0.0)
      cmdLine_addDoubleOpt(args, "-m", parm.hetro_minAmp);
    if (parm.hetro_breakPoints > 0)
      cmdLine_addIntegerOpt(args, "-n", parm.hetro_breakPoints);
    if (parm.hetro_cutoffFreq > 0.0)
      cmdLine_addDoubleOpt(args, "-l", parm.hetro_cutoffFreq);
    CsoundGUIMain::stripString(arg, parm.hetro_inputFile.c_str());
    args.push_back(arg);
    CsoundGUIMain::stripString(arg, parm.hetro_outputFile.c_str());
    args.push_back(arg);

    return (new CsoundUtility(consoleWindow, args));
}

CsoundUtility *CreateUtility_Lpanal(CsoundGUIConsole *consoleWindow,
                                    CsoundUtilitySettings& parm)
{
    vector<string>  args;
    string               arg;

    if (CsoundGUIMain::isEmptyString(parm.lpanal_inputFile) ||
        CsoundGUIMain::isEmptyString(parm.lpanal_outputFile))
      return (CsoundUtility*) 0;
    args.push_back("lpanal");
    if (parm.lpanal_channel > 0)
      cmdLine_addIntegerOpt(args, "-c", parm.lpanal_channel);
    if (parm.lpanal_beginTime > 0.0)
      cmdLine_addDoubleOpt(args, "-b", parm.lpanal_beginTime);
    if (parm.lpanal_duration > 0.0)
      cmdLine_addDoubleOpt(args, "-d", parm.lpanal_duration);
    if (parm.lpanal_altMode)
      args.push_back("-a");
    if (parm.lpanal_poles > 0)
      cmdLine_addIntegerOpt(args, "-p", parm.lpanal_poles);
    if (parm.lpanal_hopSize > 0)
      cmdLine_addIntegerOpt(args, "-h", parm.lpanal_hopSize);
    if (!CsoundGUIMain::isEmptyString(parm.lpanal_comment)) {
      arg = "-C";
      arg += parm.lpanal_comment;
      args.push_back(arg);
    }
    if (parm.lpanal_minFreq >= 0.0)
      cmdLine_addDoubleOpt(args, "-P", parm.lpanal_minFreq);
    if (parm.lpanal_maxFreq > 0.0)
      cmdLine_addDoubleOpt(args, "-Q", parm.lpanal_maxFreq);
    if (parm.lpanal_verbosity > 0)
      cmdLine_addIntegerOpt(args, "-v", parm.lpanal_verbosity);
    CsoundGUIMain::stripString(arg, parm.lpanal_inputFile.c_str());
    args.push_back(arg);
    CsoundGUIMain::stripString(arg, parm.lpanal_outputFile.c_str());
    args.push_back(arg);

    return (new CsoundUtility(consoleWindow, args));
}

CsoundUtility *CreateUtility_Sndinfo(CsoundGUIConsole *consoleWindow,
                                     CsoundUtilitySettings& parm)
{
    vector<string>  args;
    string               arg;

    if (CsoundGUIMain::isEmptyString(parm.sndinfo_inputFile))
      return (CsoundUtility*) 0;
    args.push_back("sndinfo");
    CsoundGUIMain::stripString(arg, parm.sndinfo_inputFile.c_str());
    args.push_back(arg);

    return (new CsoundUtility(consoleWindow, args));
}

CsoundUtility *CreateUtility_Srconv(CsoundGUIConsole *consoleWindow,
                                    CsoundUtilitySettings& parm)
{
    vector<string>  args;
    string               arg;

    if (CsoundGUIMain::isEmptyString(parm.srconv_inputFile) ||
        CsoundGUIMain::isEmptyString(parm.srconv_outputFile))
      return (CsoundUtility*) 0;
    args.push_back("srconv");
    if (parm.srconv_pitchRatio > 0.0)
      cmdLine_addDoubleOpt(args, "-P", parm.srconv_pitchRatio);
    else if (parm.srconv_sampleRate > 0.0)
      cmdLine_addDoubleOpt(args, "-r", parm.srconv_sampleRate);
    else
      args.push_back("-P1.0");
    if (parm.srconv_quality > 0)
      cmdLine_addIntegerOpt(args, "-Q", parm.srconv_quality);
    switch (parm.srconv_fileType) {
    case 0:
      args.push_back("-h");
      break;
    case 2:
      args.push_back("-A");
      break;
    case 3:
      args.push_back("-J");
      break;
    default:
      args.push_back("-W");
      break;
    }
    switch (parm.srconv_sampleFormat) {
    case 0:
      args.push_back("-8");
      break;
    case 2:
      args.push_back("-l");
      break;
    case 3:
      args.push_back("-f");
      break;
    default:
      args.push_back("-s");
      break;
    }
    if (!parm.srconv_peakChunks)
      args.push_back("-K");
    if (parm.srconv_rewriteHeader)
      args.push_back("-R");
    cmdLine_addIntegerOpt(args, "-H", parm.srconv_heartBeat);
    args.push_back("-o");
    CsoundGUIMain::stripString(arg, parm.srconv_outputFile.c_str());
    args.push_back(arg);
    CsoundGUIMain::stripString(arg, parm.srconv_inputFile.c_str());
    args.push_back(arg);

    return (new CsoundUtility(consoleWindow, args));
}

CsoundUtility *CreateUtility_Dnoise(CsoundGUIConsole *consoleWindow,
                                    CsoundUtilitySettings& parm)
{
    vector<string>  args;
    string               arg;

    if (CsoundGUIMain::isEmptyString(parm.dnoise_inputFile) ||
        CsoundGUIMain::isEmptyString(parm.dnoise_outputFile) ||
        CsoundGUIMain::isEmptyString(parm.dnoise_noiseFile))
      return (CsoundUtility*) 0;
    args.push_back("dnoise");
    if (parm.dnoise_beginTime > 0.0)
      cmdLine_addDoubleOpt(args, "-b", parm.dnoise_beginTime);
    if (parm.dnoise_endTime > 0.0)
      cmdLine_addDoubleOpt(args, "-e", parm.dnoise_endTime);
    if (parm.dnoise_fftSize >= 0)
      cmdLine_addIntegerOpt(args, "-N", (1 << (parm.dnoise_fftSize + 6)));
    if (parm.dnoise_overlap >= 0)
      cmdLine_addIntegerOpt(args, "-M", (1 << ((parm.dnoise_fftSize
                                                - parm.dnoise_overlap) + 8)));
    if (parm.dnoise_synLen > 0)
      cmdLine_addIntegerOpt(args, "-L", (1 << (parm.dnoise_synLen + 5)));
    if (parm.dnoise_decFact > 0)
      cmdLine_addIntegerOpt(args, "-D", (1 << (parm.dnoise_decFact + 2)));
    cmdLine_addDoubleOpt(args, "-t", parm.dnoise_threshold);
    if (parm.dnoise_sharpness > 0)
      cmdLine_addIntegerOpt(args, "-S", parm.dnoise_sharpness);
    if (parm.dnoise_numFFT > 0)
      cmdLine_addIntegerOpt(args, "-n", parm.dnoise_numFFT);
    cmdLine_addDoubleOpt(args, "-m", parm.dnoise_minGain);
    switch (parm.dnoise_fileType) {
    case 0:
      args.push_back("-h");
      break;
    case 2:
      args.push_back("-A");
      break;
    case 3:
      args.push_back("-J");
      break;
    default:
      args.push_back("-W");
      break;
    }
    switch (parm.dnoise_sampleFormat) {
    case 0:
      args.push_back("-8");
      break;
    case 2:
      args.push_back("-l");
      break;
    case 3:
      args.push_back("-f");
      break;
    default:
      args.push_back("-s");
      break;
    }
    if (parm.dnoise_heartBeat >= 0)
      cmdLine_addIntegerOpt(args, "-H", parm.dnoise_heartBeat);
    if (parm.dnoise_rewriteHeader)
      args.push_back("-R");
    if (parm.dnoise_verbose)
      args.push_back("-V");
    args.push_back("-i");
    CsoundGUIMain::stripString(arg, parm.dnoise_noiseFile.c_str());
    args.push_back(arg);
    args.push_back("-o");
    CsoundGUIMain::stripString(arg, parm.dnoise_outputFile.c_str());
    args.push_back(arg);
    CsoundGUIMain::stripString(arg, parm.dnoise_inputFile.c_str());
    args.push_back(arg);

    return (new CsoundUtility(consoleWindow, args));
}

