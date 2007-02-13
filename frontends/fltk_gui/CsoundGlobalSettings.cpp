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
#if defined(LINUX)
    textEditorProgram = "cseditor";
    soundEditorProgram = "audacity";
    soundPlayerProgram = "aplay";
    helpBrowserProgram = "firefox";
#elif defined(WIN32)
    textEditorProgram = "cseditor";
    soundEditorProgram = "audacity";
    soundPlayerProgram = "sndrec32";
    helpBrowserProgram = "explorer";
#else
    textEditorProgram = "cseditor";
    soundEditorProgram = "audacity";
    soundPlayerProgram = "quicktime";
    helpBrowserProgram = "safari";
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
    useBuiltInEditor = true;
    guiPosX = -1;
    guiPosY = -1;
    orcEditorPosX = -1;
    orcEditorPosY = -1;
    orcEditorWidth = 660;
    orcEditorHeight = 400;
    scoEditorPosX = -1;
    scoEditorPosY = -1;
    scoEditorWidth = 660;
    scoEditorHeight = 400;
    consolePosX = -1;
    consolePosY = -1;
    consoleWidth = 480;
    consoleHeight = 220;
}

CsoundGlobalSettings::~CsoundGlobalSettings()
{
}

