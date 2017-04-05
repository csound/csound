/*
    init_static_modules.c:

    Copyright (C) 2016 Michael Gogins

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"
#include "csmodule.h"

/* Do not declare these in header files; just define them in the module file 
 * as extern "C", and declare them here as extern. 
 */
extern int csoundModuleCreate_signalflowgraph(CSOUND *csound);
extern int csoundModuleInit_ampmidid(CSOUND *);
extern int csoundModuleInit_doppler(CSOUND *);
extern int csoundModuleInit_fractalnoise(CSOUND *);
extern int csoundModuleInit_ftsamplebank(CSOUND *);
extern int csoundModuleInit_mixer(CSOUND *);
extern int csoundModuleInit_signalflowgraph(CSOUND *);

/**
 * Called from the beginning of csoundInitModules to initialize opcodes and 
 * other modules that normally are dynamically loaded (e.g., C++ opcodes), 
 * but that on other platforms (e.g. PNaCl) are statically linked.
 * 
 * The pattern here is to define in the module source code a 
 * "csoundModuleInit_XXX" function, where XXX is the basename of the 
 * module file, and to call that function here.
 *
 * PLEASE NOTE: csoundModuleInit MUST call csoundModuleInit_XXX;
 * csoundModuleInit_XXX MUST NOT call csoundModuleInit.
 *
 * A similar pattern must be used if it is necessary to call 
 * csoundModuleCreate, etc.
 */
int init_static_modules(CSOUND *csound) 
{
    int result = 0;
    csoundMessage(csound, "init_static_modules...\n");
    result |= csoundModuleInit_ampmidid(csound);    
    result |= csoundModuleInit_doppler(csound);    
    result |= csoundModuleInit_fractalnoise(csound);
    result |= csoundModuleInit_ftsamplebank(csound);    
    result |= csoundModuleInit_mixer(csound);
    result |= csoundModuleCreate_signalflowgraph(csound);
    result |= csoundModuleInit_signalflowgraph(csound);
    return result;
}
