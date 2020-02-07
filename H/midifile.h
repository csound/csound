/*
    midifile.h:

    Copyright (C) 2005 Istvan Varga

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

#ifndef CSOUND_MIDIFILE_H
#define CSOUND_MIDIFILE_H

#ifdef __cplusplus
extern "C" {
#endif

/* open MIDI file, read all tracks, and create event list */

int csoundMIDIFileOpen(CSOUND *csound, const char *name);

/* read MIDI file event data at performace time */

int csoundMIDIFileRead(CSOUND *csound, unsigned char *buf, int nBytes);

/* destroy MIDI file event list */

int csoundMIDIFileClose(CSOUND *csound);

 /* ------------------------------------------------------------------------ */

typedef struct {
    OPDS    h;
    MYFLT   *kResult;
} MIDITEMPO;

/* miditempo opcode: returns the current tempo of MIDI file */

extern int midiTempoOpcode(CSOUND *csound, MIDITEMPO *p);
extern int midiFileStatus(CSOUND *csound, MIDITEMPO *p);

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_MIDIFILE_H */

