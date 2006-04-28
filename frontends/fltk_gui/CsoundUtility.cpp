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
    if (csoundPreCompile(p->csound) == 0) {
      if (csoundCreateGlobalVariable(p->csound, "_csound5gui_utility",
                                                sizeof(CsoundUtility*)) == 0) {
        std::vector<char *>   argv_;
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
                             std::vector<std::string>& args)
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
                                                   std::vector<std::string>&
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
}

CsoundUtilitySettings::~CsoundUtilitySettings()
{
}

// ----------------------------------------------------------------------------

static void cmdLine_addIntegerOpt(std::vector<std::string>& cmdLine,
                                  const char *optName, int value)
{
    char    buf[64];

    std::sprintf(&(buf[0]), "%s%d", optName, value);
    cmdLine.push_back(&(buf[0]));
}

static void cmdLine_addDoubleOpt(std::vector<std::string>& cmdLine,
                                 const char *optName, double value)
{
    char    buf[64];

    if (!(value > -10000000.0 && value < 10000000.0))
      return;
    std::sprintf(&(buf[0]), "%s%g", optName, value);
    cmdLine.push_back(&(buf[0]));
}

CsoundUtility *CreateUtility_ListOpcodes(CsoundGUIConsole *consoleWindow,
                                         CsoundUtilitySettings& parm)
{
    std::vector<std::string>  args;

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
    std::vector<std::string>  args;
    std::string               arg;

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
    std::vector<std::string>  args;
    std::string               arg;

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

