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


Changes 
-------

Add to Csound 5 CVS and SConstruct (mkg).

Use MYFLT for compiling with either single or double precision (mkg).

Change many parameters and non-in/out fields from MYFLT* to MYFLT or other type (mkg).

Add virtual destructor to VSTHost class (mkg).

Remove Windows-specific header files and data types 
to enable building on Linux and OS X (mkg).

Use cross-platform Csound API functions for loading shared libraries 
and getting function addresses (mkg).

Use std collections throughout -- no explicit heap memory management at all (mkg).

Use Csound message printing functions throughout (mkg).

Remove all redundant comments (mkg).

Simplify turnoff code in vstnote (mkg).

Enable fractional pitches (mkg).

Cleanup plugin memory allocated on the heap (by using std::vector) (mkg).

Write audio only on last instance of vstplug (mkg).

Move all VSTPlugin function implementations to vsthost.cpp 
except for some functions kept inline for speed (mkg).

Make all static members of VSTPlugin non-static (mkg).

Add the ability to load programs and program banks (ac), and to set programs (mkg).

Implement plugin edit windows (mkg).

Complete and refine message levels (log and debug) (mkg).

Add vstplugg opcode to collect audio from multiple vstnote or vstout instruments (ac and mkg).


To Do
-----


Change editor to krate so orcs can open/close/reopen editors.


