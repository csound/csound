/*****************************************************************************
 *  CsoundLib.h
 *      matt ingalls - may 2002
 *
 *      Macintosh implementation of the Csound API.
 *
 *      External software accesses csound functions through
 *      function pointers stored in a 'CsoundLib' struct.
 *      This struct is filled when you call the main entry point.
 ****************************************************************************/

#ifndef CSOUNDLIB
#define CSOUNDLIB

#include "csound.h"

/* All Csound API functions are stored in this struct
 * See csound.h file for documentation about the functions
 */
typedef struct CsoundLib
{
        int (*csoundPerform)(int argc, char **argv, void *ownerData);
        int (*csoundCompile)(int argc, char **argv, void *ownerData);
        int (*csoundPerformKsmps)(void);
        int (*csoundPerformBuffer)(void);
        void (*csoundCleanup)(void);
        void (*csoundReset)(void);
        MYFLT (*csoundGetSr)(void);
        MYFLT (*csoundGetKr)(void);
        int (*csoundGetKsmps)(void);
        int (*csoundGetNchnls)(void);
        int (*csoundGetSampleFormat)(void);
        int (*csoundGetSampleSize)(void);
        long (*csoundGetInputBufferSize)(void);
        long (*csoundGetOutputBufferSize)(void);
        void * (*csoundGetInputBuffer)(void);
        void * (*csoundGetOutputBuffer)(void);
        MYFLT * (*csoundGetSpin)(void);
        MYFLT * (*csoundGetSpout)(void);
        int (*csoundIsScorePending)(void);
        void (*csoundSetScorePending)(int pending);
        MYFLT (*csoundGetScoreOffsetSeconds)(void);
        void (*csoundSetScoreOffsetSeconds)(MYFLT offset);
        void (*csoundRewindScore)(void);
        void (*csoundSetMessageCallback)(void (*CsoundMessageCallback)(void *ownerData, const char *message));
        void (*csoundSetThrowMessageCallback)(void (*throwCallback)(void *ownerData, const char *message));
        int (*csoundGetMessageLevel)(void);
        void (*csoundSetMessageLevel)(int messageLevel);
        void (*csoundInputMessage)(const char *message);
        void (*csoundKeyPress)(char c);
        void (*csoundSetInputValueCallback)(void (*inputValueCalback)(void *ownerData, char *channelName, MYFLT *value));
        void (*csoundSetOutputValueCallback)(void (*outputValueCalback)(void *ownerData, char *channelName, MYFLT value));
        void (*csoundScoreEvent)(char type, MYFLT *pFields, long numFields);
        void (*csoundSetExternalMidiOpenCallback)(void (*midiOpenCallback)(void *ownerData));
        void (*csoundSetExternalMidiReadCallback)(int (*readMidiCallback)(void *ownerData, unsigned char *midiData, int size));
        void (*csoundSetExternalMidiWriteCallback)(int (*writeMidiCallback)(void *ownerData, unsigned char *midiData));
        void (*csoundSetExternalMidiCloseCallback)(void (*closeMidiCallback)(void *ownerData));
        void (*csoundSetIsGraphable)(int isGraphable);
        void (*csoundSetMakeGraphCallback)(void (*makeGraphCallback)(void *ownerData, WINDAT *p, char *name));
        void (*csoundSetDrawGraphCallback)(void (*drawGraphCallback)(void *ownerData, WINDAT *p));
        void (*csoundSetKillGraphCallback)(void (*killGraphCallback)(void *ownerData, WINDAT *p));
        void (*csoundSetExitGraphCallback)(int (*exitGraphCallback)(void *ownerData));
        OENTRY* (*csoundNewOpcodeList)(void);
        void (*csoundDisposeOpcodeList)(OENTRY *list);
        int (*csoundAppendOpcode)(OENTRY *opcodeEntry);
        long (*csoundLoadOpcodes)(const char *libraryPath);
        long (*csoundLoadAllOpcodes)(void);
        void (*csoundOpcodeDeinitialize)(INSDS *instrumentInstance);
        int (*csoundLoadLibrary)(const char *libraryPath);
        int (*csoundFindLibraryProcedure)(int library, const char *procedureName);
        void (*csoundSetYieldCallback)(int (*yieldCallback)(void *ownerData));
        void (*csoundSetEnv)(const char *envi, const char *path);
} CsoundLib;

#endif
