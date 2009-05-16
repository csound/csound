#!/bin/sh

scons usePortMIDI=0 custom=custom-linux-mkg.py useJack=1 buildCsoundVST=0 buildvst4cs=0 buildInterfaces=1 buildCsoundAC=1 buildJavaWrapper=0 useOSC=1 buildPythonOpcodes=1 buildLoris=0 buildStkOpcodes=0 buildWinsound=1 noFLTKThreads=0 useLrint=1 usePortAudio=0 buildPDClass=1 buildVirtual=1 buildCsound5GUI=1 buildTclcsound=0 buildLua=1 useDouble=1 dynamicCsoundLibrary=1 buildCSEditor=1  buildRelease=0 buildDSSI=1 noDebug=0 gcc4opt=atom $1 $2 $3 $4

# doxygen
