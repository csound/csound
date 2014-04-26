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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA

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

void brkpt_cb(CSOUND *csound, int line, double instr, void *userdata);

int main(int argc, char *argv[])
{
    if (argc == 1) {
        cout << "Usage:" << endl;
        cout << argv[0] << " [filename.csd] [instrument number] [ksmps offset]" << endl;
        return -1;
    }
    int break_count = 0;
    Csound* csound = new Csound();
    csound->Compile(2,argv);
    csound->Start();
    csoundDebuggerInit(csound->GetCsound());
    int ksmpsOffset = atoi(argv[3]);
    csoundSetBreakpointCallback(csound->GetCsound(), brkpt_cb, (void *) &ksmpsOffset);
    int ksmpsCount = 0;

    cout << "Csound filename: " << argv[1] << '\n';
    cout << "Instr number:" << argv[2] << '\n';



    while (csound->PerformKsmps()==0)
    {
        if(ksmpsCount==ksmpsOffset){
            csoundSetInstrumentBreakpoint(csound->GetCsound(), 1.1, 0);
        }
        ksmpsCount++;
    }

    csoundDebuggerClean(csound->GetCsound());
    delete csound;
}

void brkpt_cb(CSOUND *csound, int line, double instr, void *userdata)
{
    int *ksmpOffset = (int *) userdata;
    cout << "\nBreakpoint at instr " << instr << "\nNumber of k-cycles into performance: " << *ksmpOffset << "\n------------------------------------------------------";;
    *ksmpOffset = *ksmpOffset + 1;

    debug_instr_t *debug_instr = csoundDebugGetCurrentInstrInstance(csound);
    // Copy variable list
    debug_variable_t *vp = csoundDebugGetVariables(csound, debug_instr);
    while (vp)
    {
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
