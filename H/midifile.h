/*
    midifile.h:

    Copyright (C) 2005 Istvan Varga, (C) 2025 Victor Lazzarini

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA
*/

#ifndef CSOUND_MIDIFILE_H
#define CSOUND_MIDIFILE_H

#ifdef __cplusplus
extern "C" {
#endif

/* open MIDI file, read all tracks, and create event list */

int32_t csoundMIDIFileOpen(CSOUND *csound, const char *name);

/* read MIDI file event data at performace time */

int32_t csoundMIDIFileRead(CSOUND *csound, unsigned char *buf, int32_t nBytes);

/* destroy MIDI file event list */

int32_t csoundMIDIFileClose(CSOUND *csound);

 /* ------------------------------------------------------------------------ */

typedef struct {
    OPDS    h;
    MYFLT   *kResult;
    MYFLT   *num;
} MIDITEMPO;

typedef struct {
    OPDS    h;
    MYFLT *res;
    STRINGDAT *mfile;
    MYFLT *port;
} MFILE;


typedef struct {
    OPDS    h;
    MYFLT   *kstat;
    MYFLT   *kchn;
    MYFLT   *kdat1;
    MYFLT   *kdat2;
    MYFLT   *ktime;
    MYFLT   *kevt;
    MYFLT   *num;
} MIDIFEVT;  
  
/* miditempo opcode: returns the current tempo of MIDI file */

  int32_t midiTempoOpcode(CSOUND *csound, MIDITEMPO *p);
  int32_t midiFileStatus(CSOUND *csound, MIDITEMPO *p);
  int32_t midi_file_opcode(CSOUND *csound, void *p);
  int32_t midi_file_mute(CSOUND *csound, void *p);
  int32_t midi_file_pause(CSOUND *csound, void *p);
  int32_t midi_file_play(CSOUND *csound, void *p);
  int32_t midi_file_rewind(CSOUND *csound, void *p);
  int32_t midi_file_len(CSOUND *csound, void *p);
  int32_t midi_set_tempo(CSOUND *csound, void *p);
  int32_t midi_set_pos(CSOUND *csound, void *pp);
  int32_t midi_get_pos(CSOUND *csound, void *pp);
  int32_t midi_file_get_number_events(CSOUND *csound, void *p);
  int32_t midi_file_get_event(CSOUND *csound, void *p);
#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_MIDIFILE_H */

