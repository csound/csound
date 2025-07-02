/*
 *  csdebugger.cpp:

    Copyright (C) 2014 Andres Cabrera, Rory Walsh

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA

Simple Debugging Application
----------------------------

Simple command line debugger using Andres new csdebugger branch

Usage:
csdebugger [filename.csd] [instrument number] [ksmps offset]

filename.csd = csd file to run with debugger
instrument number = instrument to debug
ksmps offset] = number of k-cycle before breakpoint is hit
*/

#include <stdio.h>
#include "csound.hpp"
#include "csdebug.h"
#include <iostream>
#include <iomanip>

using namespace std;

void brkpt_cb(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata);

int main(int argc, char **argv)
{
    if (argc == 1) {
        cout << "Usage:" << endl;
        cout << argv[0] << " [filename.csd] [instrument number] [ksmps offset]"
             << endl;
        return -1;
    }

    int ksmpsOffset = atoi(argv[3]);

    Csound* csound = new Csound();
    csound->Compile(2, (const char **)argv);
    csound->Start();
    csoundDebuggerInit(csound->GetCsound());
    csoundSetBreakpointCallback(csound->GetCsound(), brkpt_cb, NULL);

    cout << "Csound filename: " << argv[1] << '\n';
    cout << "Instr number:" << argv[2] << '\n';

    double instr = atof(argv[2]);
    if (instr >= 1.0) {
        csoundSetInstrumentBreakpoint(csound->GetCsound(), instr, ksmpsOffset);
    } else {
        cout << "Invalid instrument breakpoint: " << instr;
    }
    while(csound->PerformKsmps() == 0);

    csoundDebuggerClean(csound->GetCsound());
    delete csound;
}

void brkpt_cb(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *userdata)
{
  (void) (csound); (void) (userdata);
    cout << "\nBreakpoint at instr " << bkpt_info->breakpointInstr->p1
         << "\nNumber of k-cycles into performance: "
         << bkpt_info->breakpointInstr->kcounter
         << "\n------------------------------------------------------";

//    debug_instr_t *debug_instr = bkpt_info.breakpointInstr;
    debug_variable_t *vp = bkpt_info->instrVarList;
    while (vp) {
        cout << " \n";
        if (vp->name[0] != '#')
        {
            cout << "VarName:"<< vp->name << "\t";;
            if (strcmp(vp->typeName, "i") == 0
                    || strcmp(vp->typeName, "k") == 0) {
                cout << "value = " << *((MYFLT *) vp->data) << "\t";;
            } else if(strcmp(vp->typeName, "S") == 0) {
                cout << "value = " << (char *) vp->data << "\t\t";
            } else if (strcmp(vp->typeName, "a") == 0) {
                MYFLT *data = (MYFLT *) vp->data;
                cout << "value[0] = "<< data[0] << "\t";
            } else {
                cout << "Unknown type\t";
            }
            cout << " varType[" << vp->typeName << "]";
        }
        vp = vp->next;
    }
}
