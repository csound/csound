/*
* C S O U N D   V S T 
*
* A VST plugin version of Csound, with Python scripting.
*
* L I C E N S E
*
* This software is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this software; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef MIDIFILE_H
#define MIDIFILE_H
#ifdef SWIG
%module CsoundVST
%{
#include <algorithm>
#include <utility>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
%}
%include "std_string.i"
%template(EventVector) std::vector<csound::Event>;
%template(MidiByteVector) std::vector<unsigned char>;
#else
#include <algorithm>
#include <utility>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#endif

namespace csound 
{
    typedef unsigned char csound_u_char;
	class MidiFile;

	class Chunk
	{
	public:
		int id;
		int chunkSize;
		int chunkSizePosition;
		int chunkStart;
		int chunkEnd;
		Chunk(const char *_id);
		virtual ~Chunk(void);
		virtual void read(std::istream &stream);
		virtual void write(std::ostream &stream);
		virtual void markChunkSize(std::ostream &stream);
		virtual void markChunkStart(std::ostream &stream);
		virtual void markChunkEnd(std::ostream &stream);
	};

	class MidiHeader : public Chunk
	{
	public:
		short type;
		short trackCount;
		short timeFormat;
		MidiHeader(void);
		virtual ~MidiHeader(void);
		virtual void clear(void);
		virtual void read(std::istream &stream);
		virtual void write(std::ostream &stream);
	};
	
	/**
	* This class is used to store ALL Midi messages.
	*/
	class MidiEvent : public std::vector<csound_u_char>
	{
	public:
		int ticks;
		double time;
		MidiEvent(void);
		virtual ~MidiEvent(void);
		virtual void read(std::istream &stream, MidiFile &midiFile);
		virtual void write(std::ostream &stream, MidiFile &midiFile, int lastTick);
		virtual int getStatus(void);
		virtual int getStatusNybble(void);
		virtual int getChannelNybble(void);
		virtual int getKey(void);
		virtual int getVelocity(void);
		virtual int getMetaType(void);
		virtual unsigned char getMetaData(int i);
		virtual size_t getMetaSize(void);
		virtual unsigned char read(std::istream &stream);
		virtual bool isChannelVoiceMessage();
		virtual bool isNoteOn(void);
		virtual bool isNoteOff(void);
		virtual bool isMatchingNoteOff(MidiEvent &offEvent);
		friend bool operator < (const MidiEvent &a, MidiEvent &b);
	};
	
	class MidiTrack : public Chunk, public std::vector<MidiEvent>
	{
	public:
		MidiTrack(void);
		virtual ~MidiTrack(void);
		virtual void read(std::istream &stream, MidiFile &midiFile);
		virtual void write(std::ostream &stream, MidiFile &midiFile);
		virtual void sort(void);
	};
	
	class TempoMap : public std::map<int, double>
	{
	public:
		double getCurrentSecondsPerTick(int tick);
	};

	/**
	* Reads and writes format 0 and format 1 standard MIDI files.
	*/
	class MidiFile
	{
	public:
		typedef enum {
			CHANNEL_NOTE_OFF = 0x80,
			CHANNEL_NOTE_ON = 0x90,
			CHANNEL_KEY_PRESSURE = 0xa0,
			CHANNEL_CONTROL_CHANGE = 0xb0,
			CHANNEL_PROGRAM_CHANGE = 0xc0,
			CHANNEL_AFTER_TOUCH = 0xd0,
			CHANNEL_PITCH_BEND = 0xe0,
			SYSTEM_EXCLUSIVE = 0xf0,
			SYSTEM_MIDI_TIME_CODE = 0xf1,
			SYSTEM_SONG_POSITION_POINTER = 0xf2,
			SYSTEM_SONG_SELECT = 0xf3,
			SYSTEM_TUNE_REQUEST = 0xf6,
			SYSTEM_END_OF_EXCLUSIVE = 0xf7,
			SYSTEM_TIMING_CLOCK = 0xf8,
			SYSTEM_START = 0xfa,
			SYSTEM_CONTINUE = 0xfb,
			SYSTEM_STOP = 0xfc,
			SYSTEM_ACTIVE_SENSING = 0xfe,
			META_EVENT = 0xff,
		} MidiEventTypes;
		typedef enum {
			META_SEQUENCE_NUMBER = 0x00,
			META_TEXT_EVENT = 0x01,
			META_COPYRIGHT_NOTICE = 0x02,
			META_SEQUENCE_NAME = 0x03,
			META_INSTRUMENT_NAME = 0x04,
			META_LYRIC = 0x05,
			META_MARKER = 0x06,
			META_CUE_POINT = 0x07,
			META_CHANNEL_PREFIX = 0x20,
			META_END_OF_TRACK = 0x2f,
			META_SET_TEMPO = 0x51,
			META_SMPTE_OFFSET = 0x54,
			META_TIME_SIGNATURE = 0x58,
			META_KEY_SIGNATURE = 0x59,
			META_SEQUENCER_SPECIFIC = 0x74,
		} MetaEventTypes;
		typedef enum {
			CONTROLLER_MOD_WHEEL = 1,
			CONTROLLER_BREATH = 2,
			CONTROLLER_FOOT = 4,
			CONTROLLER_BALANCE = 8,
			CONTROLLER_PAN = 10,
			CONTROLLER_EXPRESSION = 11,
			/* 7 bit controllers */
			CONTROLLER_DAMPER_PEDAL = 0x40,
			CONTROLLER_PORTAMENTO = 0x41, 
			CONTROLLER_SOSTENUTO = 0x42,
			CONTROLLER_SOFT_PEDAL = 0x43,
			CONTROLLER_GENERAL_4 = 0x44,
			CONTROLLER_HOLD_2 = 0x45,
			CONTROLLER_7GENERAL_5 = 0x50,
			CONTROLLER_GENERAL_6 = 0x51,
			CONTROLLER_GENERAL_7 = 0x52,
			CONTROLLER_GENERAL_8 = 0x53,
			CONTROLLER_TREMOLO_DEPTH = 0x5c,
			CONTROLLER_CHORUS_DEPTH = 0x5d,
			CONTROLLER_DETUNE = 0x5e,
			CONTROLLER_PHASER_DEPTH = 0x5f,
			/* parameter values */
			CONTROLLER_DATA_INC = 0x60,
			CONTROLLER_DATA_DEC = 0x61,
			/* parameter selection */
			CONTROLLER_NON_REG_LSB = 0x62,
			CONTROLLER_NON_REG_MSB = 0x63,
			CONTROLLER_REG_LSB = 0x64,
			CONTROLLER_REG_MSG = 0x65,
			CONTROLLER_CONTINUOUS_AFTERTOUCH = 128,
		} MidiControllers;
		static int readVariableLength(std::istream &stream);
		static void writeVariableLength(std::ostream &stream, int value);
		static int toInt(int c1, int c2, int c3, int c4);
		static short toShort(int c1, int c2);
		static int readInt(std::istream &stream);
		static void writeInt(std::ostream &stream, int value);
		static short readShort(std::istream &stream);
		static void writeShort(std::ostream &stream, short value);
		static int chunkName(int a, int b, int c, int d);
		void computeTimes(void);
		int currentTick;
		double currentTime;
		double currentSecondsPerTick;
		double microsecondsPerQuarterNote;
		unsigned char lastStatus;
		MidiHeader midiHeader;
		TempoMap tempoMap;
		std::vector<MidiTrack> midiTracks;
		MidiFile(void);
		virtual ~MidiFile(void);
		virtual void clear(void);
		virtual void read(std::istream &stream);
		virtual void write(std::ostream &stream);
		virtual void load(std::string filename);
		virtual void save(std::string filename);
		virtual void dump(std::ostream &stream);
		virtual void sort(void);
	};

	bool operator < (const MidiEvent &a, MidiEvent &b);	
}
#endif
