//  vst4cs: VST HOST OPCODES FOR CSOUND
//
//  Uses code by Hermann Seib from his Vst Host program
//  and from the vst~ object by Thomas Grill,
//  which in turn borrows from the Psycle tracker.
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
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
//  02111-1307 USA

#ifndef VST4CS_H
#define VST4CS_H
#include "csdl.h"

typedef struct VSTINIT_ {
        OPDS h;
        // Inputs.
        MYFLT *iVSThandle;
        MYFLT *iplugin;
        MYFLT *iverbose;
} VSTINIT ;

typedef struct VSTINFO_ {
        OPDS h;
        // Inputs.
        MYFLT *iVSThandle;
} VSTINFO ;

typedef struct VSTAUDIO_ {
        OPDS h;
        // Outputs.
        MYFLT *aout1;
        MYFLT *aout2;
        // Inputs.
        MYFLT *iVSThandle;
        MYFLT *ain1;
        MYFLT *ain2;
        // State.
        size_t framesPerBlock;
        size_t channels;
} VSTAUDIO ;

typedef struct VSTNOTE_ {
        OPDS h;
        // Inputs.
        MYFLT *iVSThandle;
        MYFLT *kchan;
        MYFLT *knote;
        MYFLT *kveloc;
        MYFLT *kdur;
        // State.
        MYFLT framesRemaining;
} VSTNOTE ;

typedef struct VSTMIDIOUT_ {
        OPDS h;
        // Inputs.
        MYFLT *iVSThandle;
        MYFLT *kstatus;
        MYFLT *kchan;
        MYFLT *kdata1;
        MYFLT *kdata2;
        // State.
        MYFLT oldkstatus;
        MYFLT oldkchan;
        MYFLT oldkvalue;
} VSTMIDIOUT;

typedef struct VSTPARAMGET_ {
        OPDS h;
        // Outputs.
        MYFLT *kvalue;
        // Intputs.
        MYFLT *iVSThandle;
        MYFLT *kparam;
} VSTPARAMGET ;

typedef struct VSTPARAMSET_ {
        OPDS h;
        // Inputs.
        MYFLT *iVSThandle;
        MYFLT *kparam;
        MYFLT *kvalue;
        // State.
        MYFLT oldkparam;
        MYFLT oldkvalue;
} VSTPARAMSET ;

typedef struct VSTBANKLOAD_ {
        OPDS h;
        // Inputs.
        MYFLT *iVSThandle;
        MYFLT *ibank;
} VSTBANKLOAD;

typedef struct VSTPROGSET_ {
        OPDS h;
        // Inputs.
        MYFLT *iVSThandle;
        MYFLT *iprogram;
} VSTPROGSET;

typedef struct VSTEDIT_ {
        OPDS h;
        // Inputs.
        MYFLT *iVSThandle;
} VSTEDIT;

#endif

