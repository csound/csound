/*
    csound~ : A MaxMSP external interface for the Csound API.
    
    Created by Davis Pyon on 2/4/06.
    Copyright 2006-2010 Davis Pyon. All rights reserved.
    
    LICENSE AGREEMENT
    
    This software is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    
    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with this software; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef WIN_VERSION

#include <windows.h>
#include "ext.h"

#ifdef _DEBUG
	#include <crtdbg.h>
#endif

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved ) 
{
	int result;

	// Perform actions based on the reason for calling.    
	switch( fdwReason ) 
    {         
		case DLL_PROCESS_ATTACH:	
			// Initialize once for each new process.  Return FALSE to fail DLL load.
			// Since we do nothing in our DLL_THREAD_ATTACH and DLL_THREAD_DETACH calls below, 
			// we don't need to actually receive those calls. The below call tells the OS to 
			// optimize those out.  
			DisableThreadLibraryCalls(hinstDLL);

#ifdef _DEBUG
			/*
			int tmpDbgFlag;
			_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
			_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
			
			// Set the debug-heap flag to keep freed blocks in the
			// heap's linked list - This will allow us to catch any
			// inadvertent use of freed memory
			
			tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
			//tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;
			//tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;
			tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
			_CrtSetDbgFlag(tmpDbgFlag);
			*/
			{
				char buff[_MAX_PATH];
				OutputDebugString("DLL_PROCESS_ATTACH: ");
				GetModuleFileName(hinstDLL, buff, _MAX_PATH);
				OutputDebugString(buff);
				OutputDebugString("\n");
			}
#endif
			break;
        case DLL_THREAD_ATTACH:         
			// Do thread-specific initialization.
            break;        
		case DLL_THREAD_DETACH:
			// Do thread-specific cleanup.            
			break;
        case DLL_PROCESS_DETACH:        
			// Perform any necessary cleanup.
            break;    
	}    
	return TRUE; 
}

#endif // #ifdef WIN_VERSION

