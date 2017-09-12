/*
* C S O U N D A C
*
* Copyright (c) 2001-2003 by Michael Gogins. All rights reserved.
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
#if defined(SWIGJAVA)

%module CsoundAC
%{
        #include "Silence.hpp"
        #include "CsoundAC.hpp"
%}
%apply int { size_t };
%include "Silence.hpp"

// Enable the JNI class to load the required native library.
%pragma(java) jniclasscode=%{
  static {
    try {
        java.lang.System.loadLibrary("_CsoundAC");
    } catch (UnsatisfiedLinkError e) {
        java.lang.System.err.println("_CsoundAC native code library failed to load.\n" + e);
        java.lang.System.exit(1);
    }
  }
%}

// Currently, there is a problem with running Python or accessing Python
// variables in the Java virtual machine. Therefore it is not possible
// to run CsoundAC from Java at this time. CppSound however works fine.

// Need to rename the CsoundAC class to prevent a name conflict
// with the CsoundAC module.
//%inline %{
//class CsoundAC;
//%}
//%rename(JCsoundAC) CsoundAC;
//%include "Shell.hpp"
//%include "CsoundAC.hpp"

#endif
