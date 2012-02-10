#!/bin/sh

scons usePortMIDI=0 custom=custom-linux-mkg.py buildLuaWrapper=1 buildLuaOpcodes=1 buildPythonWrapper=1 useJack=1 buildCsoundVST=1 buildvst4cs=0 buildInterfaces=1 buildCsoundAC=1 buildJavaWrapper=0 useOSC=1 buildPythonOpcodes=1 buildLoris=0 buildStkOpcodes=1 buildWinsound=1 noFLTKThreads=0 useLrint=1 usePortAudio=0 buildPDClass=1 buildVirtual=1 buildTclcsound=0 buildLua=1 useDouble=1 dynamicCsoundLibrary=1 buildRelease=0 buildDSSI=1 noDebug=0 gcc4opt=atom $1 $2 $3 $4

# doxygen
