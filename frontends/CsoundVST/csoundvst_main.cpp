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
#include <iostream>

int main(int argc, char **argv)
{
#if defined(WIN32)
        HINSTANCE lib = LoadLibrary("_CsoundVST.dll");
        AEffect* (*plugin_main)(audioMasterCallback audioMaster) = (AEffect* (*)(audioMasterCallback audioMaster)) GetProcAddress(lib, "main");
        std::cout << "lib = " << lib << " main = " << (void *)plugin_main << std::endl;
        int (*init_CsoundVST)() = (int (*)()) GetProcAddress(lib, "init_CsoundVST");
        std::cout << "lib = " << lib << " init_CsoundVST = " << (void *)init_CsoundVST << std::endl;
#endif
		CsoundVST *csoundVST = CreateCsoundVST();
		AEffEditor *editor = csoundVST->getEditor();
		editor->open(0);
		return 0;
}

