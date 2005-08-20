/*
    midisend.c:

    Copyright (C) 1997 Dave Philips
              (C) 2005 Istvan Varga

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"                                 /*    MIDISEND.C    */
#include "midioops.h"

static const unsigned char midiMsgBytes[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 0, 1
};

void send_midi_message(CSOUND *csound, int status, int data1, int data2)
{
    unsigned char buf[4];
    unsigned char nbytes;

    buf[0] = (unsigned char) status;
    nbytes = midiMsgBytes[(unsigned char) status >> 3];
    buf[1] = (unsigned char) data1;
    buf[2] = (unsigned char) data2;
    if (!nbytes)
      return;
    csound->midiGlobals->MidiWriteCallback(csound,
                                           csound->midiGlobals->midiOutUserData,
                                           &(buf[0]), (int) nbytes);
}

void note_on(CSOUND *csound, int chan, int num, int vel)
{
    send_midi_message(csound, (chan & 0x0F) | MD_NOTEON, num, vel);
}

void note_off(CSOUND *csound, int chan, int num, int vel)
{
    send_midi_message(csound, (chan & 0x0F) | MD_NOTEOFF, num, vel);
}

void control_change(CSOUND *csound, int chan, int num, int value)
{
    send_midi_message(csound, (chan & 0x0F) | MD_CNTRLCHG, num, value);
}

void after_touch(CSOUND *csound, int chan, int value)
{
    send_midi_message(csound, (chan & 0x0F) | MD_CHANPRESS, value, 0);
}

void program_change(CSOUND *csound, int chan, int num)
{
    send_midi_message(csound, (chan & 0x0F) | MD_PGMCHG, num, 0);
}

void pitch_bend(CSOUND *csound, int chan, int lsb, int msb)
{
    send_midi_message(csound, (chan & 0x0F) | MD_PTCHBENDCHG, lsb, msb);
}

void poly_after_touch(CSOUND *csound, int chan, int note_num, int value)
{
    send_midi_message(csound, (chan & 0x0F) | MD_POLYAFTER, note_num, value);
}

void openMIDIout(CSOUND *csound)
{
    MGLOBAL *p = csound->midiGlobals;
    int     retval;

    if (p->MIDIoutDONE)
      return;
    if (p->MidiOutOpenCallback == NULL)
      csoundDie(csound, Str(" *** no callback for opening MIDI output"));
    if (p->MidiWriteCallback == NULL)
      csoundDie(csound, Str(" *** no callback for writing MIDI data"));

    p->MIDIoutDONE = 1;
    retval = p->MidiOutOpenCallback(csound, &(p->midiOutUserData),
                                            csound->oparms->Midioutname);
    if (retval != 0) {
      csoundDie(csound, Str(" *** error opening MIDI out device: %d (%s)"),
                        retval, csoundExternalMidiErrorString(csound, retval));
    }
}

