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
    CsoundUtilitySettings();
    ~CsoundUtilitySettings();
};

CsoundUtility *CreateUtility_ListOpcodes(CsoundGUIConsole *,
                                         CsoundUtilitySettings&);
CsoundUtility *CreateUtility_Cvanal(CsoundGUIConsole *,
                                    CsoundUtilitySettings&);
CsoundUtility *CreateUtility_Pvanal(CsoundGUIConsole *,
                                    CsoundUtilitySettings&);

#endif  // CSOUNDUTILITY_HPP

