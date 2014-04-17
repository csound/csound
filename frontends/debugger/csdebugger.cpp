/* Simple Debugging Application

Simple command line debugger using Andres new csdebugger branch

Usage:
csddebug [filename.csd] [instrument number] [ksmps offset]

filename.csd = csd file to run with debugger
instrument number = instrument to debug
ksmps offset] = number of k-cycle before breakpoint is hit

In order to compile this example you will need to link to the Csound6 library. The following command line can be used to build on Linux:
g++ csddebuggerExample.cpp -o csddebug -I/usr/local/include/csound -L/usr/local/lib -lcsound64 -lcsnd6
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

    INSDS *insds = csoundDebugGetInstrument(csound);
    // Copy variable list
    CS_VARIABLE *vp = insds->instr->varPool->head;
    while (vp)
    {
        cout << " \n";
        if (vp->varName[0] != '#')
        {
            cout << "VarName:"<< vp->varName << "\t";;
            if (strcmp(vp->varType->varTypeName, "i") == 0
                || strcmp(vp->varType->varTypeName, "k") == 0)
            {
                if (vp->memBlock) {
                    cout << "---" << *((MYFLT *)vp->memBlock);
                } else
                {
                    MYFLT *varmem = insds->lclbas + vp->memBlockIndex;
                    cout << "value = " << *varmem << "\t";;
                }
            }
            else if(strcmp(vp->varType->varTypeName, "S") == 0)
            {
                STRINGDAT *varmem;
                if (vp->memBlock) {
                    varmem = (STRINGDAT *)vp->memBlock;
                } else {
                    varmem = (STRINGDAT *) (insds->lclbas + vp->memBlockIndex);
                }
                cout << "value = " << std::string(varmem->data) << "\t\t";;//, varmem->size));
            }
            else if (strcmp(vp->varType->varTypeName, "a") == 0)
            {
                if (vp->memBlock)
                {
                    cout << " =======" << *((MYFLT *)vp->memBlock) << *((MYFLT *)vp->memBlock + 1)
                         << *((MYFLT *)vp->memBlock + 2)<< *((MYFLT *)vp->memBlock + 3);
                }
                else
                {
                    MYFLT *varmem = insds->lclbas + vp->memBlockIndex;
                    cout << "value = "<< *varmem << "\t";
                }
            } else {

            }
            cout << " varType[" << vp->varType->varTypeName << "]";
        }
        vp = vp->next;
    }

    //Copy active instrument list
    INSDS *in = insds;
    while (in->prvact) {
        in = in->prvact;
    }
}
