/**
* C S O U N D   V S T 
*
* A VST plugin version of Csound, with Python scripting.
*
* L I C E N S E
*
* This software is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this software; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <CsoundVST.hpp>
#include <CsoundVstFltk.hpp>
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv)
{
        std::fprintf(stderr, "Starting CsoundVST...\n" );
#if defined(WIN32)
        HINSTANCE lib = LoadLibrary("_CsoundVST.dll");
        if(!lib) {
            DWORD lastError = GetLastError();
            std::fprintf(stderr, "DLL load error: %d.\n", lastError);
        }
        std::fprintf(stderr, "lib = 0x%x\n", lib);
        AEffect* (*plugin_main)(audioMasterCallback audioMaster) = (AEffect* (*)(audioMasterCallback audioMaster)) GetProcAddress(lib, "main");
        std::fprintf(stderr, "plugin_main = 0x%x\n", plugin_main);
        int (*init_CsoundVST)() = (int (*)()) GetProcAddress(lib, "init_CsoundVST");
        std::fprintf(stderr, "init_CsoundVST = 0x%x\n", init_CsoundVST);
        CsoundVST *(*CreateCsoundVST_)() = (CsoundVST *(*)()) GetProcAddress(lib, "CreateCsoundVST");
        std::fprintf(stderr, "CreateCsoundVST_ = 0x%x\n", CreateCsoundVST_);
#endif
		CsoundVST *csoundVST = CreateCsoundVST();
        std::fprintf(stderr, "csoundVST = 0x%x\n", csoundVST);
		AEffEditor *editor = csoundVST->getEditor();
		editor->open(0);
		if(argc == 2) {
		  csoundVST->openFile(argv[1]);
		}
		return csoundVST->run();
}

