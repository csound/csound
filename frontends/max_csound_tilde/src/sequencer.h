/*
    csound~ : A MaxMSP external interface for the Csound API.
    
    Created by Davis Pyon on 2/4/06.
    Copyright 2006-2010 Davis Pyon. All rights reserved.
    
    LICENSE AGREEMENT
    
    This software is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    
    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with this software; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _SEQUENCER_H
#define _SEQUENCER_H

#include "includes.h"
#include "definitions.h"
#include "csound.h"
#include "channel.h" 
#include "midi.h"
#include "util.h"
#undef Str
#ifdef _USE_BOOST_SERIALIZATION
	#include <boost/property_tree/ptree.hpp>
	#include <boost/property_tree/xml_parser.hpp>
	#include <boost/property_tree/json_parser.hpp>
	using boost::property_tree::ptree;
#endif

using namespace std;
using boost::ptr_multiset;

namespace dvx {

// Don't change the order!  If adding new event types, add to end.
enum 
{ 
	EVENT_TYPE_NONE = -1, EVENT_TYPE_CSOUND = 0, EVENT_TYPE_CONTROL, EVENT_TYPE_MIDI, EVENT_TYPE_STRING
};

#define TICK_RESOLUTION 480      // ticks per beat
#define MIN_BPM 20               // minimum tempo
#define MAX_BPM 500              // maximum tempo
#define DEFAULT_BPM 120.0        // default tempo
#define MAX_TIME 3456000         // TICK_RESOLUTION * 120 bpm * 60 minutes
#define FILE_MAGIC_NUMBER 0x1357 // Each recorded sequence file starts with this integer.
#define DEFAULT_SAVE_LOAD_BUFFER_SIZE 262144

class Event
{
	friend class Sequencer;

public:
	Event() : m_type(EVENT_TYPE_NONE), m_time(0) {}
	Event(int type, int time) : m_type(type), m_time(time) {}
	virtual ~Event() {}

	bool operator<(const Event &e) const { return m_time < e.m_time; }
	int time() { return m_time; }
	virtual int type() const = 0;

	#ifdef _USE_BOOST_SERIALIZATION
	virtual void save(ptree &pt) const = 0;
	#endif

protected:
	int m_type; // EventType
	int m_time; // Time in ticks.
};

class ControlEvent : public Event
{
public:
	ControlEvent(int time, const string &name, MYFLT value);
	~ControlEvent() {}

	int type() const { return m_type; }

	#ifdef _USE_BOOST_SERIALIZATION
	ControlEvent(ptree &pt);
	void save(ptree &pt) const;
	#endif

	string m_name;
	MYFLT m_value;
};

class CsoundEvent : public Event
{
public:
	CsoundEvent(int time, const char *evt);
	~CsoundEvent() {}

	int type() const { return m_type; }

	#ifdef _USE_BOOST_SERIALIZATION
	CsoundEvent(ptree &pt);
	void save(ptree &pt) const;
	#endif

	string m_event;
};

class MidiEvent : public Event
{
public:
	MidiEvent(int time, const byte *buf, int bytes);
	~MidiEvent() {}

	int type() const { return m_type; }

	#ifdef _USE_BOOST_SERIALIZATION
	MidiEvent(ptree &pt);
	void save(ptree &pt) const;
	#endif

	byte m_buffer[MAX_MIDI_MESSAGE_SIZE];
	int m_size;
};

class StringEvent : public Event
{
public:
	StringEvent(int time, const string &name, const char *value);	
	~StringEvent() {}

	int type() const { return m_type; }

	#ifdef _USE_BOOST_SERIALIZATION
	StringEvent(ptree &pt);
	void save(ptree &pt) const;
	#endif

	string m_name;
	string m_string;
};

class Sequencer
{
	enum { FILE_BINARY = 0, FILE_JSON, FILE_XML };

public:
	class ParamObject
	{
	public:
		ParamObject(Sequencer *s, const string & file) : m_seq(s), m_file(file) {}

		Sequencer *m_seq;
		string m_file;
	};

public:
	Sequencer(t_object *o, CSOUND *c, ChannelGroup *g, MidiBuffer *m);
	~Sequencer();

	bool AddCsoundEvent(char *buf, bool lock); // Returns true on success.
	bool AddControlEvent(const string &name, MYFLT value, bool lock); // Returns true on success.
	bool AddStringEvent(const string &name, const char *str, bool lock); // Returns true on success.
	bool AddMIDIEvent(byte *buf, int nBytes, bool lock); // Returns true on success.
	void ProcessEvents(); // Play stored events.

	void AdvanceSampleCount(int n);
	void SampleBasedTimerCallback();
	void SetBPM(float bpm);
	inline void SetSR(int sr) { m_sr = sr; }
	void StartRecording();
	void StopRecording();
	void StartPlaying();
	void StopPlaying();
	inline bool Playing() { return m_playing; }
	inline bool Recording() { return m_recording; }

	/* Careful; does not check bounds. */
	inline void UpdateCtrlMatrix(byte chan, byte ctrl, byte val) { m_ctrlMatrix[chan][ctrl] = val; }

	// Write sequence to text file in json/xml format. Does not throw std::exception's.
	// Set current directory before calling if file is relative path.
	void Write(const string & file);

	// Read sequence from text file in json/xml format. Does not throw std::exception's.
	// Set current directory before calling if file is relative path.
	void Read(const string & file);

	// Reads binary csound~ sequence files.
	// Set current directory before calling if file is relative path.
	void ReadBinary(const string & file);

	// Writes binary csound~ sequence files.
	// Set current directory before calling if file is relative path.
	void WriteBinary(const string & file);

	friend void Sequencer_TimerCallback(Sequencer *s);
	friend uintptr_t Sequencer_ReadThreadFunc(void *spo);
	friend uintptr_t Sequencer_WriteThreadFunc(void *spo);

	void* m_read_write_thread;
	volatile bool m_read_write_thread_exists;

private:
	int AdvanceTicks();
	float CalcMsPerTick(float bpm);
	
	t_object *m_obj;                           // Used for object_post() calls. Don't delete/free!
	ptr_multiset<Event> m_events;              // Where events are stored.
	ptr_multiset<Event>::iterator m_cur_event; // Used to keep track of current play event during playback.
	CSOUND *m_csound;				 // Ptr to parent t_csound's CSOUND instance. Don't delete/free!
	ChannelGroup *m_inChannelGroup;  // Ptr to parent t_csound's input ChannelGroup. Don't delete/free!
	MidiBuffer *m_midiInputBuffer;   // Ptr to parent t_csound's MidiBuffer. Don't delete/free!
	DEFAULT_LOCK_TYPE m_lock;                     // Used to make Sequencer thread safe.

	int m_time;             // current time in ticks
	int m_timerRes;         // Max clock timer resolution (in milliseconds).
	int m_nticks;           // holds the # of ticks generated in a timer interrupt
	int m_fticks;           // fractional # of ticks; acts as accumulator for nticks
	int m_nSamples;			// Used to count the # of samples processed during non-realtime rendering. For every k-cycle,
	                        // nSamples is increased by 1000 * ksmps.  Whenever the # of samples >= 1ms, 
							// events are processed and nSamples is reduced by 1ms worth of samples.
	int m_sr;				// Should be equal to csound sr.
	float m_bpm;			// Need an arbitrary bpm in order to time stamp events with tick count.
	float m_ms_per_tick;	// The current milliseconds / tick (dependant on bpm).	
	long m_microSecPerBeat;	// Dependant on bpm.
	volatile bool m_playing;
	volatile bool m_recording;       // If == true, sequencer is in record mode. If false, sequencer is in play mode (default).
	void *m_max_clock;		// Millisecond counter for recording events.

	byte m_ctrlMatrix[16][128];
	byte m_activeNoteMatrix[16][128];
};

void Sequencer_TimerCallback(Sequencer *s);
uintptr_t Sequencer_ReadThreadFunc(void *spo);
uintptr_t Sequencer_WriteThreadFunc(void *spo);

} // namespace dvx

#endif
