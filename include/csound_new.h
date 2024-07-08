/*
    csound_new.h: NEW API - Experimental

    Copyright (C) 2024

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef CSOUND_H
#define CSOUND_H

// Instantiation
PUBLIC int 	csoundInitialize (int flags);
PUBLIC CSOUND*  csoundCreate (void *hostData);
PUBLIC void 	csoundDestroy (CSOUND *);

// Attributes
PUBLIC int  csoundGetVersion (void);
PUBLIC int  csoundGetAPIVersion (void);

// Performance
// One compilation function that does everything
// For example, we could potentially use "argc" as a code
// > 0 - argc/argv as main() command-line parameters
// 0 - start csound as a daemon with no code, ignore argv
// -1 - argv[0] is an orchestra string (csoundCompileStr)
// -2 - argv[0] is a complete csound   (csoundCompileCsdText)
// First call to csoundCompile() also starts Csound so we remove csoundStart()
// async defines async operation
// NB - interface with double pointers may not be ideal for non C/C++
PUBLIC int  csoundCompile (CSOUND *, int argc, const char **argv, int async);
PUBLIC int  csoundPerformKsmps (CSOUND *);

// Realtime Audio
// this is now effectively -n since there is no access to buffer, only to spin.
// we need a better name I think
PUBLIC void csoundSetHostImplementedAudioIO(CSOUND *, int state);
// access to spin/spout (no need to give individual sample access)
PUBLIC MYFLT *csoundGetSpin (CSOUND *);
PUBLIC MYFLT *csoundGetSpout (CSOUND *csound);

// no audio IO callbacks - that's only available in the module API now.
// MIDI callbacks can be set with a single new function
typedef struct {
  int(*open)(CSOUND *, void **userData, const char *devName, int mode); // MIDI open callback (mode=CS_MIDI_IN, CS_MIDI_OUT)
  int(*read)(CSOUND *, void *userData, unsigned char *buf, int nBytes);
  int(*write)(CSOUND *, void *userData, unsigned char *buf, int nBytes);
  int(*close)(CSOUND *, void *userData);
} CS_MIDI_CALLBACKS;
PUBLIC void csoundSetMIDICallbacks(CSOUND *, CS_MIDI_CALLBACKS *);
 
// Messages
PUBLIC void 	   csoundCreateMessageBuffer (CSOUND *csound, int toStdOut)
PUBLIC const char *csoundGetFirstMessage (CSOUND *csound)
PUBLIC void 	   csoundPopFirstMessage (CSOUND *csound)
PUBLIC int 	   csoundGetMessageCnt (CSOUND *csound)
void PUBLIC 	   csoundDestroyMessageBuffer (CSOUND *csound)

// Channels
PUBLIC int csoundListChannels(CSOUND *, controlChannelInfo_t **lst);
PUBLIC void csoundDeleteChannelList(CSOUND *, controlChannelInfo_t *lst);
// basic types
PUBLIC MYFLT csoundGetControlChannel(CSOUND *csound, const char *name,
                                       int *err);
PUBLIC void csoundSetControlChannel(CSOUND *csound,
                                      const char *name, MYFLT val);
PUBLIC void csoundGetAudioChannel(CSOUND *csound,
                                    const char *name, MYFLT *samples);
PUBLIC void csoundSetAudioChannel(CSOUND *csound,
                                    const char *name, MYFLT *samples);
PUBLIC void csoundGetStringChannel(CSOUND *csound,
                                     const char *name, char *string);
PUBLIC  void csoundSetStringChannel(CSOUND *csound,
                                      const char *name, char *string);
// generic data access (e.g. for other types, structs, etc)
PUBLIC int csoundGetChannelPtr(CSOUND *,
                                 MYFLT **p, const char *name, int type);
PUBLIC int csoundGetChannelDatasize(CSOUND *csound, const char *name);

// events:
// async selects asynchronous operation
// new function - replaces csoundInputMessage
PUBLIC void  csoundEventString (CSOUND *, const char *message, int async);
// new function - replaces csoundScoreEvent
// type 0 - instrument instance     CS_INSTR_EVENT
// type 1 - functiob table instance CS_TABLE_EVENT
PUBLIC void  csoundEvent (CSOUND *, int type, MYFLT *params, int async);

//Tables
PUBLIC int 	csoundTableLength (CSOUND *, int table);
PUBLIC void 	csoundTableCopyIn (CSOUND *csound, int table, MYFLT *src);
PUBLIC void 	csoundTableCopyOut (CSOUND *csound, int table, MYFLT *dest);

//Opcodes - adapted for new internals
PUBLIC int 	csoundAppendOpcode (CSOUND *, const char *opname, int dsblksiz, int flags,
                                    const char *outypes, const char *intypes, int(*init)(CSOUND *, void *),
                                    int(*perf)(CSOUND *, void *), int(*deinit)(CSOUND *, void *));

//Threading and concurrency

#endif  /* CSOUND_H */
