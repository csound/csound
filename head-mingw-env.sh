#!/bin/sh
echo Setting up environment to run Csound head branch from build area...
echo Order of paths can be vitally important!
export PATH=/c/utah/opt/Mega-Nerd/libsndfile:${PATH}
export PATH=/c/utah/opt/portaudio:${PATH}
export PATH=/c/utah/opt/portmidi:${PATH}
export PATH=/c/utah/opt/Tcl/bin:${PATH}
export PATH=/c/utah/opt/clisp-2.49:${PATH}
export PATH=/c/utah/opt/swigwin-2.0.4:${PATH}
export PATH=/c/utah/opt/pd/bin:${PATH}
export PATH=/c/utah/opt/liblo:${PATH}
export PATH=/c/utah/opt/Java/jdk1.6.0_23/bin:${PATH}
export PATH=/c/utah/opt/fluidsynth-1.1.3/src:${PATH}
export PATH=/c/utah/opt/luajit/src:${PATH}
export PATH=/c/utah/opt/Graphviz2.26.3/bin:${PATH}
export PATH=/d/utah/home/mkg/csound/csound5:${PATH}
export PATH=${PATH}:/c/utah/opt/doxygen/bin
export PATH=${PATH}:/c/utah/opt/ImageMagick-6.6.3-Q16
export PATH=${PATH}:/c/utah/opt/Python27
export PATH=${PATH}:/c/utah/opt/Python27/Scripts
export PATH=${PATH}:/c/utah/opt/fltk-1.1.10/src
export PATH=${PATH}:/c/utah/opt/fltk-1.1.10/lib
export PATH=${PATH}:/c/utah/opt/fltk-1.1.10/bin
export PATH=${PATH}:/c/utah/opt/fltk-1.1.10/fluid
export PATH=${PATH}:/c/utah/opt/qutecsound/bin
export PATH=${PATH}:/c/utah/opt/musicxml-v2/win32/codeblocks
export PATH=${PATH}:/c/utah/opt/gtk/bin
export PATH=${PATH}:/c/utah/opt/MinGW/msys/1.0/bin
export PATH=/c/utah/opt/Qt/2010.05/bin:${PATH}
export PATH=/c/utah/opt/Qt/2010.05/qt/bin:${PATH}
export PATH=/c/utah/opt/PythonQt-build-desktop/lib:${PATH}
export PATH=${PATH}:/c/utah/opt/MiKTeX-2.9/miktex/bin
export PATH=${PATH}:/c/utah/opt/MiKTeX-2.9/miktex/bin
export RAWWAVE_PATH=D:/utah/home/mkg/csound/csound5/Opcodes/stk/rawwaves
export OPCODEDIR64=D:/utah/home/mkg/csound/csound5
export PYTHONPATH=D:/utah/home/mkg/csound/csound5
echo $PATH

