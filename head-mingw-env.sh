#!/bin/sh
echo Setting up environment to build and run Csound head branch from build area...
echo Order of paths can be vitally important!
export PATH=/c/utah/opt/Mega-Nerd/libsndfile:${PATH}
export PATH=/c/utah/opt/portaudio:${PATH}
export PATH=/c/utah/opt/portmidi:${PATH}
export PATH=/c/utah/opt/Tcl/bin:${PATH}
export PATH=/c/utah/opt/clisp-2.49:${PATH}
export PATH=/c/utah/opt/swigwin-2.0.4:${PATH}
export PATH=/c/utah/opt/pd/bin:${PATH}
export PATH=/c/utah/opt/liblo:${PATH}
export PATH=/c/utah/opt/Java/jdk1.7.0/bin:${PATH}
export PATH=/c/utah/opt/fluidsynth/trunk/fluidsynth/src:${PATH}
export PATH=/c/utah/opt/luajit/src:${PATH}
export PATH=/c/utah/opt/Graphviz_2.28/bin:${PATH}
export PATH=/d/utah/home/mkg/csound/csound6:${PATH}
export PATH=${PATH}:/c/utah/opt/doxygen/bin
export PATH=${PATH}:/c/utah/opt/ImageMagick-6.6.3-Q16
export PATH=${PATH}:/c/utah/opt/Python27
export PATH=${PATH}:/c/utah/opt/Python27/Scripts
export PATH=${PATH}:/c/utah/opt/fltk-1.1.10/src
export PATH=${PATH}:/c/utah/opt/fltk-1.1.10/lib
export PATH=${PATH}:/c/utah/opt/fltk-1.1.10/bin
export PATH=${PATH}:/c/utah/opt/fltk-1.1.10/fluid
export PATH=${PATH}:/c/utah/opt/musicxml-v2/win32/codeblocks
export PATH=${PATH}:/c/utah/opt/gtk/bin
export PATH=${PATH}:/c/utah/opt/MinGW/msys/1.0/bin
export PATH=${PATH}:/c/utah/opt/scite
export PATH=${PATH}:/c/utah/opt/emacs-23.2/bin
export PATH=${PATH}:/c/utah/opt/Qt/QtCreator/bin
export PATH=${PATH}:/c/utah/opt/qcs-build-desktop/bin
export PATH=${PATH}:/c/utah/opt/CMake-2.8/bin
export PATH=/c/utah/opt/PythonQt-build-desktop/lib:${PATH}
export PATH=${PATH}:/c/utah/opt/MiKTeX-2.9/miktex/bin
export RAWWAVE_PATH=D:/utah/home/mkg/csound/csound5/Opcodes/stk/rawwaves
export OPCODEDIR64=D:/utah/home/mkg/csound/csound6
export PYTHONPATH=D:/utah/home/mkg/csound/csound6
export PATH=/c/utah/opt/MinGW/bin:${PATH}
export LUA_PATH=${LUA_PATH}";D:/utah/home/mkg/csound/csound6/?.lua"
export LUA_CPATH=${LUA_CPATH}";D:/utah/home/mkg/csound/csound6/?.dll;D:/utah/home/mkg/csound/csound6/lua?.dll"
echo $PATH

