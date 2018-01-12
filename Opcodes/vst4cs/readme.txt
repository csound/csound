vst4cs
------

VST HOST OPCODES FOR CSOUND

vst4cs: VST HOST OPCODES FOR CSOUND

Uses code by Hermann Seib from his VSTHost program and from the vst~ 
object by Thomas Grill (no license), which in turn borrows from the Psycle 
tracker (also based on VSTHost).

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
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
02110-1301 USA

Linking vst4cs statically or dynamically with other modules is making a
combined work based on vst4cs. Thus, the terms and conditions of the GNU
Lesser General Public License cover the whole combination.

In addition, as a special exception, the copyright holders of vst4cs, 
including the Csound developers and Hermann Seib, the original author of 
VSTHost, give you permission to combine vst4cs with free software programs 
or libraries that are released under the GNU LGPL and with code included 
in the standard release of the VST SDK version 2 under the terms of the 
license stated in the VST SDK version 2 files. You may copy and distribute 
such a system following the terms of the GNU LGPL for vst4cs and the 
licenses of the other code concerned. The source code for the VST SDK 
version 2 is available in the VST SDK hosted at
https://github.com/steinbergmedia/vst3sdk.

Note that people who make modified versions of vst4cs are not obligated to
grant this special exception for their modified versions; it is their
choice whether to do so. The GNU Lesser General Public License gives
permission to release a modified version without this exception; this
exception also makes it possible to release a modified version which
carries forward this exception.

Current version: 1.0

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

Renamed opcodes -- vstaudio for vstplug, vstmidiout for vstout, vstparamset for vstpsend, vstparamget for vstpret (mkg).

Implemented (supposedly) sample-accurate timing for note on and note off (mkg).

