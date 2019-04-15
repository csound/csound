/*
  haiku_midi.h:

  Haiku MIDI Interface
  -- Caution -- things may change...

  Copyright (C) 2012 Peter J. Goodeve

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


#ifndef _HAIKU_MIDI
#define _HAIKU_MIDI


class MidiIn {
 public:
        MidiIn(const char *name);
        ~MidiIn();
        uint32 GetEvent();

        const char *nodename;
        class BMidiProducer *extSource;
private:
        class MidiInHandler *handler;
};

struct MidiEvent {
        uint8 nbytes, status, data1, data2;
        MidiEvent(uint8 n, uint8 s=0, uint8 d1=0, uint8 d2=0) :
                nbytes(n), status(s), data1(d1), data2(d2) {}
        MidiEvent(uint32 seq) {*(uint32 *)&nbytes = seq;}
        int Size() {return nbytes;}
        uint8 * Bytes() {return (uint8 *)&status;}
        operator uint32() {return *(uint32 *)&nbytes;}
        operator uint8 *() {return (uint8 *)&nbytes;}
        MidiEvent& operator = (uint32 seq)
                        {*(uint32 *)&nbytes = seq; return *this;}
};


#endif
