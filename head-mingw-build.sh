#!/bin/sh

scons custom=custom-mingw.py buildLuaWrapper=1 buildPythonWrapper=1 usePortMIDI=1 useJack=0 buildCsoundVST=1 buildvst4cs=1 buildInterfaces=1 buildCsoundAC=1 buildJavaWrapper=1 useOSC=1 buildPythonOpcodes=1 buildLoris=0 buildStkOpcodes=1 buildWinsound=1 noFLTKThreads=0 buildPDClass=1 buildVirtual=1 buildTclcsound=1 tclversion=85 buildLua=1 useDouble=1 dynamicCsoundLibrary=1 buildCSEditor=1 gcc4opt=core2 buildRelease=1 bufferoverflowu=0 noDebug=0 useOpenMP=1 install=1 $1 $2 $3 $4

# doxygen
