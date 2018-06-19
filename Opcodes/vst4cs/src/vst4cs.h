//  vst4cs: VST HOST OPCODES FOR CSOUND
//
//  Uses code by Hermann Seib from his VSTHost program and from the vst~
//  object by Thomas Grill (no license), which in turn borrows from the Psycle
//  tracker (also based on VSTHost).
//
//  VST is a trademark of Steinberg Media Technologies GmbH.
//  VST Plug-In Technology by Steinberg.
//
//  Copyright (C) 2004 Andres Cabrera, Michael Gogins
//
//  The vst4cs library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  The vst4cs library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with The vst4cs library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
//  02110-1301 USA
//
//  Linking vst4cs statically or dynamically with other modules is making a
//  combined work based on vst4cs. Thus, the terms and conditions of the GNU
//  Lesser General Public License cover the whole combination.
//
//  In addition, as a special exception, the copyright holders of vst4cs,
//  including the Csound developers and Hermann Seib, the original author of
//  VSTHost, give you permission to combine vst4cs with free software programs
//  or libraries that are released under the GNU LGPL and with code included
//  in the standard release of the VST SDK version 2 under the terms of the
//  license stated in the VST SDK version 2 files. You may copy and distribute
//  such a system following the terms of the GNU LGPL for vst4cs and the
//  licenses of the other code concerned. The source code for the VST SDK
//  version 2 is available in the VST SDK hosted at
//  https://github.com/steinbergmedia/vst3sdk.
//
//  Note that people who make modified versions of vst4cs are not obligated to
//  grant this special exception for their modified versions; it is their
//  choice whether to do so. The GNU Lesser General Public License gives
//  permission to release a modified version without this exception; this
//  exception also makes it possible to release a modified version which
//  carries forward this exception.

#ifndef VST4CS_H
#define VST4CS_H

#include "csdl.h"

class VSTPlugin;

// In all of these, sizeof(MYFLT) is not necessarily equal to or greater than
// sizeof(VSTPlugin *), hence type casting can not be used and a table of
// handles must be used.

typedef struct VSTINIT_ {
    OPDS    h;
    // Inputs.
    MYFLT   *iVSThandle;
    MYFLT   *iplugin;
    MYFLT   *iverbose;
    // State.
    VSTPlugin *vstplugin;
} VSTINIT;

typedef struct VSTINFO_ {
    OPDS    h;
    // Inputs.
    MYFLT   *iVSThandle;
    // State.
    VSTPlugin *vstplugin;
} VSTINFO;

typedef struct VSTAUDIO_ {
    OPDS    h;
    // Outputs.
    MYFLT   *aouts[32];
    // Inputs.
    MYFLT   *iVSThandle;
    MYFLT   *ains[32];
    // State.
    size_t  framesPerBlock;
    size_t  pluginInChannels;
    size_t  pluginOutChannels;
    size_t  opcodeInChannels;
    size_t  opcodeOutChannels;
    VSTPlugin *vstplugin;
} VSTAUDIO;

typedef struct VSTNOTE_ {
    OPDS    h;
    // Inputs.
    MYFLT   *iVSThandle;
    MYFLT   *kchan;
    MYFLT   *knote;
    MYFLT   *kveloc;
    MYFLT   *kdur;
    // State.
    size_t  vstHandle;
    int     chn;
    int     note;
    size_t  framesRemaining;
    VSTPlugin *vstplugin;
} VSTNOTE;

typedef struct VSTMIDIOUT_ {
    OPDS    h;
    // Inputs.
    MYFLT   *iVSThandle;
    MYFLT   *kstatus;
    MYFLT   *kchan;
    MYFLT   *kdata1;
    MYFLT   *kdata2;
    // State.
    size_t  vstHandle;
    int     prvMidiData;
    VSTPlugin *vstplugin;
} VSTMIDIOUT;

typedef struct VSTPARAMGET_ {
    OPDS    h;
    // Outputs.
    MYFLT   *kvalue;
    // Intputs.
    MYFLT   *iVSThandle;
    MYFLT   *kparam;
    // State.
    VSTPlugin *vstplugin;
} VSTPARAMGET;

typedef struct VSTPARAMSET_ {
    OPDS    h;
    // Inputs.
    MYFLT   *iVSThandle;
    MYFLT   *kparam;
    MYFLT   *kvalue;
    // State.
    MYFLT   oldkparam;
    MYFLT   oldkvalue;
    VSTPlugin *vstplugin;
} VSTPARAMSET;

typedef struct VSTBANKLOAD_ {
    OPDS    h;
    // Inputs.
    MYFLT   *iVSThandle;
    MYFLT   *ibank;
    // State.
    VSTPlugin *vstplugin;
} VSTBANKLOAD;

typedef struct VSTPROGSET_ {
    OPDS    h;
    // Inputs.
    MYFLT   *iVSThandle;
    MYFLT   *iprogram;
    // State.
    VSTPlugin *vstplugin;
} VSTPROGSET;

typedef struct VSTEDIT_ {
    OPDS    h;
    // Inputs.
    MYFLT   *iVSThandle;
    // State.
    VSTPlugin *vstplugin;
} VSTEDIT;

typedef struct VSTTEMPO_ { //gab
    OPDS h;
    // Inputs.
    MYFLT *tempo;
    MYFLT *iVSThandle;
    // State.
    VSTPlugin *vstplugin;
} VSTTEMPO;

#endif
