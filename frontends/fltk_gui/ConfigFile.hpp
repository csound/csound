/*
    ConfigFile.hpp:
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

#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include "CsoundGlobalSettings.hpp"
#include "CsoundPerformanceSettings.hpp"
#include "CsoundUtility.hpp"

int writeCsound5GUIConfigFile(const char *fileName,
                              CsoundGlobalSettings& cfg);

int writeCsound5GUIConfigFile(const char *fileName,
                              CsoundPerformanceSettings& cfg);

int writeCsound5GUIConfigFile(const char *fileName,
                              CsoundUtilitySettings& cfg);

int readCsound5GUIConfigFile(const char *fileName,
                             CsoundGlobalSettings& cfg);

int readCsound5GUIConfigFile(const char *fileName,
                             CsoundPerformanceSettings& cfg);

int readCsound5GUIConfigFile(const char *fileName,
                             CsoundUtilitySettings& cfg);

#endif  // CONFIGFILE_HPP

