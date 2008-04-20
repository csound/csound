#!/bin/sh

scons custom=custom-mingw.py usePortMIDI=1 useBufferOverflowLibrary=1 useJack=0 buildCsoundVST=0 buildvst4cs=1 buildInterfaces=0 buildCsoundAC=0 buildJavaWrapper=0 useOSC=1 buildPythonOpcodes=0 buildLoris=0 buildStkOpcodes=1 buildWinsound=0 noFLTKThreads=0 buildPDClass=0 buildVirtual=1 buildCsound5GUI=0 buildTclcsound=0 buildLua=0 useDouble=1 dynamicCsoundLibrary=1 buildCSEditor=0 buildRelease=0 bufferoverflowu=1 noDebug=0 useGprof=1 $1 $2 $3 $4

# doxygen