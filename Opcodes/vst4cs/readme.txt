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

Building: Once you have built csound5, add the following lines to Sconstruct at line 639 (may change) after the line:
zipDependencies.append(pluginEnvironment.SharedLibrary('wave-terrain', 
    ['Opcodes/wave-terrain.c']))

and before # Plug ins with External Dependencies

Add:
vst4Environment = commonEnvironment.Copy()
vst4Environment.Append(LIBS = ['kernel32'])
vst4Environment.Append(LIBS = ['gdi32'])
vst4Environment.Append(LIBS = ['wsock32'])
vst4Environment.Append(LIBS = ['ole32'])
vst4Environment.Append(LIBS = ['uuid'])
vst4Environment.Append(CPPPATH = ['h:/vstsdk2.3/source/common'])
zipDependencies.append(vst4Environment.SharedLibrary('vst4cs', 
     Split('''Opcodes/vst4cs/vst4cs.cpp
              Opcodes/vst4cs/vsthost.cpp''')))

then run scons and vst4cs should build. Remember to adjust paths as necessary.

See html docuentation for new opcodes.



vst4cs
------

Host VST para Csound5

Version actual:0.1alpha

Instalacion: Copie el archivo vst4cs.dll a la carpeta que contiene los plugins 
de csound5. Copie la documentacion y el codigo fuente donde le parezca.

Vea la documentacion html para conocer los nuevos opcodes.


Changes by Michael Gogins
-------------------------

Add to Csound 5 CVS and SConstruct.

Use MYFLT for compiling with either single or double precision.

Change many parameters and non-in/out fields from MYFLT* to MYFLT or other type.

Add virtual destructor to VSTHost class.

Make static members of VSTPlugin non-static.

Remove Windows-specific header files and data types 
to enable building on Linux and OS X.

Use cross-platform Csound API functions for loading shared libraries 
and getting function addresses.

Use std collections throughout.

Use Csound message printing functions throughout.

Remove all redundant comments.

Simplify turnoff code in vstnote.

Enable fractional pitches.

Cleanup plugin memory allocated on the heap.

Write audio only on last instance of vstplug.

