vst4cs
------

VST HOST OPCODES FOR CSOUND

Uses code by Hermann Seib from the vst~ object, 
which in turn borrows from the Psycle tracker.
VST is a trademark of Steinberg Media Technologies GmbH.
VST Plug-In Technology by Steinberg.

Copyright (C) 2004 Andres Cabrera, Michael Gogins

The vst4cs library is free software; you can redistribute it
and/or modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

The vst4cs library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with The vst4cs library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA

Current version: 0.1alpha

Installing: Copy vst4cs.dll into your plugin opcodes directory. 
Copy documentation and source where it suits you.

See the HTML documentation for new opcodes.


Changes by Michael Gogins
-------------------------

Add to Csound 5 CVS and SConstruct.

Use MYFLT for compiling with either single or double precision.

Change many parameters and non-in/out fields from MYFLT* to MYFLT or other type.

Add virtual destructor to VSTHost class.

Remove Windows-specific header files and data types 
to enable building on Linux and OS X.

Use cross-platform Csound API functions for loading shared libraries 
and getting function addresses.

Use std collections throughout -- no explicit heap memory management at all.

Use Csound message printing functions throughout.

Remove all redundant comments.

Simplify turnoff code in vstnote.

Enable fractional pitches.

Cleanup plugin memory allocated on the heap (by using std::vector).

Write audio only on last instance of vstplug.

Moved all VSTPlugin function implementations to vsthost.cpp 
except for some functions kept inline for speed.


To Do
-----

Make static members of VSTPlugin non-static.

Add the ability to load programs and program banks, and to set programs.

Implement plugin edit windows.

To do: Message levels.


