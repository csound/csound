/*
  HaikuMidi.cpp:

  Haiku real-time MIDI handling for Csound5

  Copyright (C) 2012-2019 Peter J. Goodeve

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


#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <Locker.h>

#include <MidiRoster.h>
#include <MidiProducer.h>
#include <MidiConsumer.h>

#include "haiku_midi.h"

#define XDPRINTF(x)
#define DPRINTF(x)
//#define DPRINTF(x) printf x


//////////////////////////////////////////////////////////


class MidiInHandler : public BMidiLocalConsumer
{
        friend class MidiIn;
        MidiIn * param;
public:
        MidiInHandler(MidiIn * mp, const char * name=NULL)
         :  BMidiLocalConsumer(name), param(mp), nextevent(0) {
                midi_in_sem = create_sem(0, "midi input");
         }
        ~MidiInHandler() {
                delete_sem(midi_in_sem);
        }


        void NoteOff(uchar channel,
                     uchar note,
                     uchar velocity,
                     bigtime_t time);

        void NoteOn(uchar channel,
                    uchar note,
                    uchar velocity,
                    bigtime_t time);

        void KeyPressure(uchar channel,
                         uchar note,
                         uchar pressure,
                         bigtime_t time);

        void ControlChange(uchar channel,
                           uchar controlNumber,
                           uchar controlValue,
                           bigtime_t time);

        void ProgramChange(uchar channel,
                           uchar programNumber,
                           bigtime_t time);

        void ChannelPressure(uchar channel,
                             uchar pressure,
                             bigtime_t time);

        void PitchBend(uchar channel,
                       uchar lsb,
                       uchar msb,
                       bigtime_t time);

        void SystemExclusive(void* data,
                             size_t dataLength,
                             bigtime_t time);

        void SystemCommon(uchar statusByte,
                          uchar data1,
                          uchar data2,
                          bigtime_t time);

        void SystemRealTime(uchar statusByte, bigtime_t time);


        void midiEventIn(MidiEvent &s, bigtime_t time);
        void midiSysExIn(void *data, size_t size, bigtime_t time);

        uint32 nextevent;
        sem_id midi_in_sem;         // to be waited on by MIDI loop (w. timeout)
};


//////////////////////////////////////////////////////////

void MidiInHandler::NoteOff(uchar channel,
                            uchar note,
                            uchar velocity,
                            bigtime_t time)
{
        MidiEvent s(3, 0x80|channel, note, velocity);
        midiEventIn(s, time);
}


void MidiInHandler::NoteOn(uchar channel,
                           uchar note,
                           uchar velocity,
                           bigtime_t time)
{
        MidiEvent s(3, 0x90|channel, note, velocity);
        midiEventIn(s, time);
}


void MidiInHandler::KeyPressure(uchar channel,
                                uchar note,
                                uchar pressure,
                                bigtime_t time)
{
        MidiEvent s(3, 0xA0|channel, note, pressure);
        midiEventIn(s, time);
}


void MidiInHandler::ControlChange(uchar channel,
                                  uchar controlNumber,
                                  uchar controlValue,
                                  bigtime_t time)
{
        MidiEvent s(3, 0xB0|channel, controlNumber, controlValue);
        midiEventIn(s, time);
}


void MidiInHandler::ProgramChange(uchar channel,
                                  uchar programNumber,
                                  bigtime_t time)
{
        MidiEvent s(2, 0xC0|channel, programNumber);
        midiEventIn(s, time);
}


void MidiInHandler::ChannelPressure(uchar channel,
                                    uchar pressure,
                                    bigtime_t time)
{
        MidiEvent s(2, 0xD0|channel, pressure);
        midiEventIn(s, time);
}


void MidiInHandler::PitchBend(uchar channel,
                              uchar lsb,
                              uchar msb,
                              bigtime_t time)
{
        MidiEvent s(3, 0xE0|channel, lsb, msb);
        midiEventIn(s, time);
}


void MidiInHandler::SystemExclusive(void* data,
                                    size_t dataLength,
                                    bigtime_t time)
{
        midiSysExIn(data, dataLength, time);
}


void MidiInHandler::SystemCommon(uchar statusByte,
                                 uchar data1,
                                 uchar data2,
                                 bigtime_t time)
{
// Ignored for the moment (size varies...)
}


void MidiInHandler::SystemRealTime(uchar statusByte, bigtime_t time)
{
        MidiEvent s(3, statusByte);
        midiEventIn(s, time);
}


/////////////////////////////////////////

// Called by MidiInHandler input functions...:
void MidiInHandler::midiEventIn(MidiEvent &s, bigtime_t time)
{
        nextevent = s;
        acquire_sem_etc(midi_in_sem, 1, B_RELATIVE_TIMEOUT, 1000000);
}


void MidiInHandler::midiSysExIn(void *data, size_t size, bigtime_t time)
{
        // completely ignored (for now anyway...)
}


//////////////////////////////////////////////////////////

static BMidiProducer *FindProducer(const char *name)
{
        if (!name || !*name) return NULL;       // just being safe...
        BMidiProducer *object;
        int32 id = 0;
        DPRINTF(("looking for endpoint %s\n", name);)
        if (*name == '?') {
                fprintf(stderr,
                        "\n===========================\nAvailable MIDI Sources:\n");
                while(object = BMidiRoster::NextProducer(&id)){
                        fprintf(stderr, "    %s\n", object->Name());
                        object->Release();
                }
                fprintf(stderr, "===========================\n\n");
        }
        else while(object = BMidiRoster::NextProducer(&id)){
                DPRINTF(("checking endpoint %p %s\n", object, object->Name());)
                if(!strcmp(object->Name(),name)) {
                        return object;
                }
                object->Release();
        }
        return NULL;
}


MidiIn::MidiIn(const char *name) :
nodename(name), extSource(NULL), handler(NULL)
{
        if (!name || !*name) return;    // empty name -- don't publish
        extSource = FindProducer(name);
        if (*name == '?') return;       // was just a query
        if (extSource) {
                DPRINTF(("Connecting to external Producer %s\n", name);)
                handler = new MidiInHandler(this);      // anonymous endpoint
                if (extSource->Connect(handler) != B_OK) {
                        extSource = NULL;
                        handler->Release();
                        handler = NULL;
                        fprintf(stderr, "Failed to connect to %s!!\n", name);
                }
                else
                        fprintf(stderr, "Connected to MIDI source %s\n", name);
                extSource->Release();   // our extra hold...
        } else {
                DPRINTF(("Registering as Consumer %s\n", name);)
                handler = new MidiInHandler(this, name);
                BMidiRoster::Register(handler); // needs error check!
                DPRINTF(("Published OK -- thread %lx\n", find_thread(NULL));)
                fprintf(stderr, "Providing MIDI Input %s\n", name);
        }
}


MidiIn::~MidiIn()
{
        if (handler) {
                DPRINTF(("Unregistering  %s\n", nodename? nodename : "<NULL>");)
                handler->Release();
                DPRINTF(("handler %p released\n", handler);)
                if (!handler->param->extSource) BMidiRoster::Unregister(handler);
                handler = NULL;
                DPRINTF(("Unregistered... handler now %p\n", handler);)
        }
        DPRINTF(("Unpublished OK -- thread %lx\n", find_thread(NULL));)
}


uint32 MidiIn::GetEvent() {
        uint32 ev = handler->nextevent;
        if (!handler) return 0;
        if (ev != 0) {
                handler->nextevent = 0;
                release_sem(handler->midi_in_sem);
        }
        return ev;
}
