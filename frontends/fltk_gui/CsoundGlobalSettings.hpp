/*
    CsoundGlobalSettings.hpp:
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

#ifndef CSOUNDGLOBALSETTINGS_HPP
#define CSOUNDGLOBALSETTINGS_HPP

#include <iostream>
#include <string>

class CsoundGlobalSettings {
 public:
    std::string textEditorProgram;
    std::string soundEditorProgram;
    std::string soundPlayerProgram;
    std::string helpBrowserProgram;
    std::string performanceSettings1_Name;
    std::string performanceSettings2_Name;
    std::string performanceSettings3_Name;
    std::string performanceSettings4_Name;
    std::string performanceSettings5_Name;
    std::string performanceSettings6_Name;
    std::string performanceSettings7_Name;
    std::string performanceSettings8_Name;
    std::string performanceSettings9_Name;
    std::string performanceSettings10_Name;
    bool    forcePerformanceSettings;
    bool    editSoundFileAfterPerformance;
    bool    useBuiltInEditor;
    int guiPosX;
    int guiPosY;
    int orcEditorPosX;
    int orcEditorPosY;
    int orcEditorWidth;
    int orcEditorHeight;
    int scoEditorPosX;
    int scoEditorPosY;
    int scoEditorWidth;
    int scoEditorHeight;
    int consolePosX;
    int consolePosY;
    int consoleWidth;
    int consoleHeight;
    // -----------------------------------------------------------------
    CsoundGlobalSettings();
    ~CsoundGlobalSettings();
};

#endif  // CSOUNDGLOBALSETTINGS_HPP

