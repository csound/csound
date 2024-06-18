/*
    csound_new.h: NEW API - Experimenetal

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

//Instantiation
PUBLIC int 	csoundInitialize (int flags)
PUBLIC CSOUND*  csoundCreate (void *hostData) 
PUBLIC void 	csoundDestroy (CSOUND *) 

//Performance
PUBLIC int 	    csoundCompile (CSOUND *, int argc, const char **argv)
PUBLIC int 	    csoundPerformKsmps (CSOUND *)

//Realtime Audio
PUBLIC MYFLT * 	csoundGetSpin (CSOUND *)
PUBLIC MYFLT * 	csoundGetSpout (CSOUND *csound)

//Realtime MIDI
PUBLIC void 	csoundSetExternalMidiInOpenCallback (CSOUND *, int(*func)(CSOUND *, void **userData, const char *devName))
PUBLIC void 	csoundSetExternalMidiReadCallback (CSOUND *, int(*func)(CSOUND *, void *userData, unsigned char *buf, int nBytes)) 
PUBLIC void 	csoundSetExternalMidiInCloseCallback (CSOUND *, int(*func)(CSOUND *, void *userData))
PUBLIC void 	csoundSetExternalMidiOutOpenCallback (CSOUND *, int(*func)(CSOUND *, void **userData, const char *devName)) 
PUBLIC void 	csoundSetExternalMidiWriteCallback (CSOUND *, int(*func)(CSOUND *, void *userData, const unsigned char *buf, int nBytes)) 

//Messages
PUBLIC void 	csoundCreateMessageBuffer (CSOUND *csound, int toStdOut)
PUBLIC const char * 	csoundGetFirstMessage (CSOUND *csound)
PUBLIC void 	csoundPopFirstMessage (CSOUND *csound)
PUBLIC int 	    csoundGetMessageCnt (CSOUND *csound)
void PUBLIC 	csoundDestroyMessageBuffer (CSOUND *csound)

//Channels Controls / Events
PUBLIC void 	csoundGetAudioChannel (CSOUND *csound, const char *name, MYFLT *samples)
PUBLIC void 	csoundSetAudioChannel (CSOUND *csound, const char *name, MYFLT *samples)
PUBLIC void 	csoundGetStringChannel (CSOUND *csound, const char *name, char *string)
PUBLIC void 	csoundSetStringChannel (CSOUND *csound, const char *name, char *string)

PUBLIC void 	csoundInputMessage (CSOUND *, const char *message)

//Tables
PUBLIC int 	    csoundTableLength (CSOUND *, int table)
PUBLIC void 	csoundTableCopyIn (CSOUND *csound, int table, MYFLT *src)
PUBLIC void 	csoundTableCopyOut (CSOUND *csound, int table, MYFLT *dest)

//Opcodes
PUBLIC int 	csoundAppendOpcode (CSOUND *, const char *opname, int dsblksiz, int flags, int thread, const char *outypes, const char *intypes, int(*iopadr)(CSOUND *, void *), int(*kopadr)(CSOUND *, void *), int(*aopadr)(CSOUND *, void *))

//Threading and concurrency

#endif  /* CSOUND_H */
