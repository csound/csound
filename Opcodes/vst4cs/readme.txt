vst4cs
------

VST host opcodes for Csound 5.
By: Andres Cabrera
Using code by: Hermann Seib and the vst~ object
VST is a trademark of Steinberg Media Technologies GmbH
VST Plug-In Technology by Steinberg

Released under the same license as Csound5. Please check that.

Current version: 0.1alpha

Installing: Copy vst4cs.dll into your plugin opcodes directory. Copy documentation and source where it suits you.

See html docuentation for new opcodes.


Changes by Michael Gogins
-------------------------

Add to Csound 5 CVS and SConstruct.

Use MYFLT for compiling with either single or double precision.

Change many parameters and non-in/out fields from MYFLT* to MYFLT or other type.

Add virtual destructor to VSTHost class.

To do: Make static members of VSTPlugin non-static.

Remove Windows-specific header files and data types 
to enable building on Linux and OS X.

Use cross-platform Csound API functions for loading shared libraries 
and getting function addresses.

To do: Use std collections throughout.

Use Csound message printing functions throughout.

Remove all redundant comments.

Simplify turnoff code in vstnote.

Enable fractional pitches.

To do: Cleanup plugin memory allocated on the heap.

Write audio only on last instance of vstplug.

To do: Add the ability to load programs and program banks, and to set programs.



