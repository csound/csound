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
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "CsoundVST.hpp"

//static AudioEffect *effect = 0;
bool oome = false;

#if MAC
#pragma export on
#endif

// prototype of the export function main
#if defined(__GNUC__) && defined(WIN32)
#define main main_plugin
extern "C" __declspec(dllexport) AEffect *main_plugin (audioMasterCallback audioMaster)
#elif defined(LINUX)
AEffect *main_plugin (audioMasterCallback audioMaster)
#else
AEffect *main(audioMasterCallback audioMaster)
#endif
{
	// get vst version
	if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
	 return 0;  // old version

	CsoundVST *effect = new CsoundVST (audioMaster);
	if (!effect)
		return 0;
	if (oome)
	{
		delete effect;
		return 0;
	}
	return effect->getAeffect ();
}

#if MAC
#pragma export off
#endif


#if WIN32
#include <windows.h>
void* hInstance;
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
	hInstance = hInst;
	return 1;
}
#endif
