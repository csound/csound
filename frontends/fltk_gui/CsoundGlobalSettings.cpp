/*
    CsoundGlobalSettings.cpp:
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

CsoundGlobalSettings::CsoundGlobalSettings()
{
#ifndef WIN32
    textEditorProgram = "xterm -e vim";
    soundEditorProgram = "audacity";
    helpBrowserProgram =
        "firefox /usr/local/share/doc/csound/manual/index.html";
#else
    textEditorProgram = "notepad";
    soundEditorProgram = "sndrec32";
    helpBrowserProgram =
        "explorer \"C:\\Program Files\\Csound\\doc\\manual\\index.html\"";
#endif
    performanceSettings1_Name = "";
    performanceSettings2_Name = "";
    performanceSettings3_Name = "";
    performanceSettings4_Name = "";
    performanceSettings5_Name = "";
    performanceSettings6_Name = "";
    performanceSettings7_Name = "";
    performanceSettings8_Name = "";
    performanceSettings9_Name = "";
    performanceSettings10_Name = "";
    forcePerformanceSettings = false;
    editSoundFileAfterPerformance = false;
}

CsoundGlobalSettings::~CsoundGlobalSettings()
{
}

