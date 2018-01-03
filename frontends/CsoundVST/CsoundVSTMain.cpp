//  COUNDVST
//
//  A VST plugin version of Csound.
//
//  VST is a trademark of Steinberg Media Technologies GmbH.
//  VST Plug-In Technology by Steinberg.
//
//  Copyright (C) 2004 Michael Gogins
//
//  The CsoundVST library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  The CsoundVST library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with the CsoundVST library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
//  02110-1301 USA
//
//  Linking CsoundVST statically or dynamically with other modules is making a
//  combined work based on CsoundVST. Thus, the terms and conditions of the GNU
//  Lesser General Public License cover the whole combination.
//
//  In addition, as a special exception, the copyright holders of CsoundVST
//  give you permission to combine CsoundVST with free software programs
//  or libraries that are released under the GNU LGPL and with code included
//  in the standard release of the VST SDK version 2 under the terms of the
//  license stated in the VST SDK version 2 files. You may copy and distribute
//  such a system following the terms of the GNU LGPL for CsoundVST and the
//  licenses of the other code concerned. The source code for the VST SDK
//  version 2 is available in the VST SDK hosted at
//  https://github.com/steinbergmedia/vst3sdk.
//
//  Note that people who make modified versions of CsoundVST are not obligated to
//  grant this special exception for their modified versions; it is their
//  choice whether to do so. The GNU Lesser General Public License gives
//  permission to release a modified version without this exception; this
//  exception also makes it possible to release a modified version which
//  carries forward this exception.

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "CsoundVST.hpp"

#ifdef VSTPluginMain
#undef VSTPluginMain
#endif

extern "C"
{
    PUBLIC AEffect* VSTPluginMain(audioMasterCallback audioMaster)
    {
        if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0)) {
            return 0;
        }
        AudioEffect* effect = new CsoundVST(audioMaster);
        if (!effect) {
            return 0;
        }
        return effect->getAeffect();
    }
};

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
 return new CsoundVST (audioMaster);
}
