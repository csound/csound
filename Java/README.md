This directory contains the SWIG files for generating Java bindings for Csound
used for Csound 5 and 6 on desktop and Android. For Csound 7, the desktop
bindings will be replaced with JNI-based bindings based on
[CsoundJNI](https://github.com/kunstmusik/CsoundJNI). We are currently
determining whether to bring those bindings into this repo or maintain
separately. 

The SWIG binding is now disabled for building with desktop Csound7. It is left
here temporarily as it is used for the Android build. We will look at moving
Android to JNI as well.  

