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

#include "sequencer.h"

#ifdef _USE_BOOST_SERIALIZATION
	#include "Parser.h" // Include here instead of in sequencer.h. That way, other files that include
						// sequencer.h won't take so damn long to compile.
    #include <boost/property_tree/xml_parser.hpp>
	using boost::property_tree::ptree;
#endif

using boost::ptr_multiset;
using namespace dvx;

ControlEvent::ControlEvent(int time, const string &name, MYFLT value) :
	Event(EVENT_TYPE_CONTROL, time), m_name(name), m_value(value)
{}

#ifdef _USE_BOOST_SERIALIZATION
ControlEvent::ControlEvent(ptree &pt) :
		Event(EVENT_TYPE_CONTROL, pt.get<int>("time")), m_name(pt.get<string>("name")), m_value(pt.get<MYFLT>("value"))
	{}

void ControlEvent::save(ptree &pt) const
{
	ptree& t = pt.add_child("Event", ptree()); // Add empty child to pt and store reference in t.
	t.put("type", m_type);
	t.put("time", m_time);
	t.put("name", m_name);
	t.put("value", m_value);

}
#endif

CsoundEvent::CsoundEvent(int time, const char *evt) :
	Event(EVENT_TYPE_CSOUND, time), m_event(evt)
{}

#ifdef _USE_BOOST_SERIALIZATION
CsoundEvent::CsoundEvent(ptree &pt) :
	Event(EVENT_TYPE_CSOUND, pt.get<int>("time")), m_event(pt.get<string>("e"))
{}

void CsoundEvent::save(ptree &pt) const
{
	ptree& t = pt.add_child("Event", ptree()); // Add empty child to pt and store reference in t.
	t.put("type", m_type);
	t.put("time", m_time);
	t.put("e", m_event);
}
#endif

MidiEvent::MidiEvent(int time, const byte *buf, int bytes) :
	Event(EVENT_TYPE_MIDI, time), m_size(bytes)
{
	assert(m_size<=MAX_MIDI_MESSAGE_SIZE);
	memcpy(m_buffer, buf, m_size);
}

#ifdef _USE_BOOST_SERIALIZATION
MidiEvent::MidiEvent(ptree &pt) :
	Event(EVENT_TYPE_MIDI, pt.get<int>("time")), m_size(pt.get<int>("size"))
{
	string byte_str;
	std::vector<int> v(0); // parse_integers appends values, so vector must be empty.
	byte_str = pt.get<string>("bytes");
	Parser::parse_integers(byte_str.begin(), byte_str.end(), v);
	for(int i=0; i<m_size; i++)
		m_buffer[i] = v[i];
}

void MidiEvent::save(ptree &pt) const
{
	ptree& t = pt.add_child("Event", ptree()); // Add empty child to pt and store reference in t.
	t.put("type", m_type);
	t.put("time", m_time);
	t.put("size", m_size);

	string byte_str;
	std::vector<int> v(MAX_MIDI_MESSAGE_SIZE);
	for(int i=0; i<m_size; i++) v[i] = m_buffer[i];
	back_insert_iterator<string> sink(byte_str);
	Parser::generate_integers(sink, v);
	t.put("bytes", byte_str);
}
#endif

StringEvent::StringEvent(int time, const string &name, const char *value) :
	Event(EVENT_TYPE_STRING, time), m_name(name), m_string(value)
{}

#ifdef _USE_BOOST_SERIALIZATION
StringEvent::StringEvent(ptree &pt) :
	Event(EVENT_TYPE_STRING, pt.get<int>("time")), m_name(pt.get<string>("name")), m_string(pt.get<string>("string"))
{}

void StringEvent::save(ptree &pt) const
{
	ptree& t = pt.add_child("Event", ptree()); // Add empty child to pt and store reference in t.
	t.put("type", m_type);
	t.put("time", m_time);
	t.put("string", m_string);
}
#endif

// --------------------------------------------------------------------------------------

Sequencer::Sequencer(t_object *o, CSOUND *c, ChannelGroup *g, MidiBuffer *m) :
	m_obj(o),
	m_events(),
	m_cur_event(),
	m_csound(c),
	m_inChannelGroup(g),
	m_midiInputBuffer(m),
	m_lock((char*)"Sequencer"),
	m_time(0), m_timerRes(1), m_nticks(0), m_fticks(0), m_nSamples(0), m_sr(DEFAULT_SAMPLE_RATE),
	m_playing(false),
	m_recording(false),
	m_max_clock(NULL),
	m_read_write_thread_exists(false)
{
	m_max_clock = clock_new(this, (method)Sequencer_TimerCallback);
	SetBPM(DEFAULT_BPM);
	for(int i=0; i<16; i++)
		for(int j=0; j<128; j++)
			m_ctrlMatrix[i][j] = (byte) 128;
	memset(m_activeNoteMatrix, 0, MIDI_MATRIX_SIZE);
}

Sequencer::~Sequencer()
{
	clock_unset(m_max_clock);
	freeobject((t_object *) m_max_clock);
	StopRecording();
	StopPlaying();
	if(m_read_write_thread_exists)
	{
		object_post(m_obj, "Waiting for sequencer read/write thread to finish.");
		if(0 != csoundJoinThread(m_read_write_thread))
			object_error(m_obj, "Sequencer::~Sequencer() : csoundJoinThread() failed.");
	}
}

float Sequencer::CalcMsPerTick(float bpm)
{
	// 1 / ((beats/ms) * (ticks/beat))
	return (float)( 1.0 / (((double)bpm / 60000.0) * (double)TICK_RESOLUTION) );
}

void Sequencer::SetBPM(float bpm)
{
	if(bpm >= MIN_BPM && bpm <= MAX_BPM)
	{
		m_bpm = bpm;
		m_ms_per_tick = CalcMsPerTick(bpm);
		m_microSecPerBeat = (long) (60.0f * 1000000.0f / bpm);
	}
}

int Sequencer::AdvanceTicks()
{
	m_nticks = (int)((m_fticks + m_timerRes * 1000 * TICK_RESOLUTION) / m_microSecPerBeat);
	m_fticks += (int)((m_timerRes * 1000 * TICK_RESOLUTION) - (m_nticks * m_microSecPerBeat));
	return m_nticks;
}

/* Have to define C function inside namespace if it's a friend
   to a C++ class that's inside a namespace. */
namespace dvx
{
	void Sequencer_TimerCallback(Sequencer *s)
	{
		if(s->m_playing) s->ProcessEvents(); // If playing, process some events.
		s->m_time += s->AdvanceTicks();      // Advance the tick count and update the time.

		if(s->m_time >= MAX_TIME)
		{
			object_error(s->m_obj, "Maximum sequencer time reached. Stopping playing/recording.");
			if(s->m_playing) s->StopPlaying();
			if(s->m_recording) s->StopRecording();
		}

		if(s->m_recording || s->m_playing) clock_fdelay(s->m_max_clock, s->m_timerRes);
	}

	uintptr_t Sequencer_ReadThreadFunc(void *data)
  {
    Sequencer::ParamObject *spo = (Sequencer::ParamObject*)data;
		object_post(spo->m_seq->m_obj, "Reading sequence from %s...", spo->m_file.c_str());
		spo->m_seq->Read(spo->m_file);
		spo->m_seq->m_read_write_thread_exists = false;
		delete spo;
    return 0;
  }

	uintptr_t Sequencer_WriteThreadFunc(void* data)
	{
    Sequencer::ParamObject *spo = (Sequencer::ParamObject*)data;
		object_post(spo->m_seq->m_obj, "Writing sequence to %s...", spo->m_file.c_str());
		spo->m_seq->Write(spo->m_file);
		spo->m_seq->m_read_write_thread_exists = false;
		delete spo;
    return 0;
  }

} // namespace dvx

void Sequencer::SampleBasedTimerCallback()
{
	if(m_playing) ProcessEvents();
	m_time += AdvanceTicks();
	if(m_time >= MAX_TIME)
	{
		object_error(m_obj, "Stopping sequencer. Maximum time reached.");
		if(m_playing) StopPlaying();
		if(m_recording) StopRecording();
	}
}

void Sequencer::AdvanceSampleCount(int n)
{
	m_nSamples += n * 1000;
	while(m_nSamples >= m_sr)
	{
		m_nSamples -= m_sr;
		SampleBasedTimerCallback();
	}
}

void Sequencer::StartRecording()
{
	ChannelGroup *c = m_inChannelGroup;
	ChannelObject *co = NULL;
	byte ch, ctrl;
	byte buf[3];

	if(m_read_write_thread_exists)
	{
		object_warn(m_obj, "Can't start recording. Please wait until reading/writing has finished...");
		return;
	}

	if(m_playing) StopPlaying();

	if(!m_recording)
	{
		m_events.clear(); // Delete all events.
		m_recording = true;
		m_time = m_nticks = m_fticks = 0;
		SetBPM(DEFAULT_BPM);

		// Add one Control event for each ChannelObject in the table.
		{
			ScopedLock k(c->m_lock);
			boost::ptr_set<ChannelObject>::iterator it;

			for(it = c->m_channels.begin(); it != c->m_channels.end(); it++)
			{
				co = &*it;
				if(co->IsControlChannel())     AddControlEvent(co->m_name, co->m_value, true);
				else if(co->IsStringChannel()) AddStringEvent(co->m_name, co->m_str_value.c_str(), true);
			}
		}

		// Add one MIDI ctrl event for every entry in the m_ctrlMatrix < 128.
		for(ch=0; ch<16; ch++)
			for(ctrl=0; ctrl<128; ctrl++)
			{
				if(m_ctrlMatrix[ch][ctrl] < 128)
				{
					buf[0] = 0xB0 | ch;
					buf[1] = ctrl;
					buf[2] = m_ctrlMatrix[ch][ctrl];
					AddMIDIEvent(buf, 3, true);
				}
			}

		// Set the time to 1 so that we can distinguish between
		// recorded events and initializing events.
		m_time = 1;

		// Set the clock so that Sequencer_TimerCallback() is called in m_timerRes ms.
		clock_fdelay(m_max_clock, m_timerRes);
	}
}

void Sequencer::StopRecording()
{
	Event *e = NULL;
	MidiEvent *me = NULL;
	bool firstEventFound = false;
	int firstEventTime = 1;
	byte activeNoteMatrix[16][128];
	byte status = 0, chan, pitch, vel, b[3];

	m_recording = false;
	memset(activeNoteMatrix, 0, MIDI_MATRIX_SIZE);

	/* Find the first event with time >= 1 and subtract object's time - 1 from that event and all
	   subsequent events.  In other words, move all recorded events back in time so that when the
	   user hits play, playback of recorded events will begin immediately. Events with time == 0
	   are initializing events; that's why we're looking for first event with time >= 1.

	   While moving events in time, keep track of MIDI note-on's and note-off's. After going
	   through all events, take care of hanging MIDI notes. */

	ScopedLock k(m_lock);
	boost::ptr_multiset<Event>::iterator it;

	for(it = m_events.begin(); it != m_events.end(); it++)
	{
		e = &*it;
		if(e->m_time > 0)
		{
			if(!firstEventFound)
			{
				firstEventFound = true;
				firstEventTime = e->m_time;
			}
			// multiset is still consistent because we're subtracting all event times
			// by the same amount, so we don't have to remove/insert elements individually.
			e->m_time -= firstEventTime - 1;
		}

		if(e->type() == EVENT_TYPE_MIDI)
		{
			me = static_cast<MidiEvent*>(e);
			status = me->m_buffer[0] & 0xf0;
			switch(status)
			{
			case 0x80: // Note-off
				chan = me->m_buffer[0] & 0x0f;
				pitch = me->m_buffer[1];
				if(activeNoteMatrix[chan][pitch] > 0) activeNoteMatrix[chan][pitch] -= 1;
				break;
			case 0x90: // Note-on
				chan = me->m_buffer[0] & 0x0f;
				pitch = me->m_buffer[1];
				vel = me->m_buffer[2];
				if(vel > 0)	activeNoteMatrix[chan][pitch] += 1;
				else if(activeNoteMatrix[chan][pitch] > 0) activeNoteMatrix[chan][pitch] -= 1;
				break;
			}
		}
	}

	// If any entries in active_note_count[][] are > 0, add a note-off for that chan+pitch.
	for(int i=0; i<16; i++)
	{
		for(int j=0; j<128; j++)
		{
			if(activeNoteMatrix[i][j] > 0)
			{
				b[0] = (byte)i | 0x80;
				b[1] = (byte)j;
				b[2] = 0;
				AddMIDIEvent(b, 3, false); // Don't lock; using scoped lock above.
			}
		}
	}
}

void Sequencer::StartPlaying()
{
	if(m_read_write_thread_exists)
	{
		object_warn(m_obj, "Can't start playing. Please wait until reading/writing has finished...");
		return;
	}
	if(m_playing) StopPlaying();
	if(m_recording) StopRecording();
	ScopedLock k(m_lock);

	m_playing = true;
	m_cur_event = m_events.begin();
	m_time = m_nticks = m_fticks = 0;
	memset(m_activeNoteMatrix, 0, MIDI_MATRIX_SIZE);
	clock_fdelay(m_max_clock, m_timerRes);
}

void Sequencer::StopPlaying()
{
	int i, j;
	byte b[3];
	ScopedLock k(m_lock);

	m_cur_event = m_events.begin();
	m_playing = false;
	m_time = m_nticks = m_fticks = 0;
	m_nSamples = 0;

	// Get rid of hanging notes.
	for(i=0; i<16; i++)
		for(j=0; j<128; j++)
		{
			if(m_activeNoteMatrix[i][j] > 0)
			{
				b[0] = (byte)i | 0x80;
				b[1] = (byte)j;
				b[2] = 0;
				m_midiInputBuffer->EnqueueBuffer(b, 3);
			}
		}
}

bool Sequencer::AddCsoundEvent(char *buf, bool lock)
{
	int len = strlen(buf);

	if(len >= MAX_EVENT_MESSAGE_SIZE)
	{
		object_error(m_obj, "Event string size %d too large.  Max size is %d.", len, MAX_EVENT_MESSAGE_SIZE - 1);
		return false;
	}

	ScopedLock k(m_lock,lock);

	try	{ m_events.insert(new CsoundEvent(m_time, buf)); }
	catch(std::exception &e)
	{
		object_post(m_obj, "CsoundEvent allocation/insertion failed: %s", e.what());
		return false;
	}

	return true;
}

bool Sequencer::AddControlEvent(const string &name, MYFLT value, bool lock)
{
	ScopedLock k(m_lock,lock);

	try { m_events.insert(new ControlEvent(m_time, name, value)); }
	catch(std::exception &e)
	{
		object_post(m_obj, "ControlEvent allocation/insertion failed: %s", e.what());
		return false;
	}
	return true;
}

bool Sequencer::AddStringEvent(const string &name, const char *str, bool lock)
{
	ScopedLock k(m_lock,lock);

	try { m_events.insert(new StringEvent(m_time, name, str)); }
	catch(std::exception &e)
	{
		object_post(m_obj, "StringEvent allocation/insertion failed: %s", e.what());
		return false;
	}
	return true;
}

bool Sequencer::AddMIDIEvent(byte *buf, int nBytes, bool lock)
{
	ScopedLock k(m_lock,lock);

	try	{ m_events.insert(new MidiEvent(m_time, buf, nBytes)); }
	catch(std::exception &e)
	{
		object_post(m_obj, "MidiEvent allocation/insertion failed: %s", e.what());
		return false;
	}
	return true;
}

void Sequencer::ProcessEvents()
{
	byte status, chan, pitch, vel;
	ScopedLock k(m_lock);

	if(m_cur_event == m_events.end())
	{
		m_playing = false;
		return;
	}

	while(m_cur_event != m_events.end() && m_cur_event->m_time <= m_time)
	{
		switch(m_cur_event->type())
		{
		case EVENT_TYPE_CSOUND:
			{
				CsoundEvent &e = static_cast<CsoundEvent&>(*m_cur_event);
				csoundInputMessage(m_csound, e.m_event.c_str());
			}
			break;
		case EVENT_TYPE_CONTROL:
			{
				ControlEvent &e = static_cast<ControlEvent&>(*m_cur_event);
				m_inChannelGroup->SetVal(e.m_name.c_str(), CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL, e.m_value);
			}
			break;
		case EVENT_TYPE_STRING:
			{
				StringEvent &e = static_cast<StringEvent&>(*m_cur_event);
				m_inChannelGroup->SetVal(e.m_name.c_str(), CSOUND_INPUT_CHANNEL | CSOUND_STRING_CHANNEL, e.m_string.c_str());
			}
			break;
		case EVENT_TYPE_MIDI:
			{
				MidiEvent &e = static_cast<MidiEvent&>(*m_cur_event);
				m_midiInputBuffer->EnqueueBuffer(e.m_buffer, e.m_size);

				// Keep track of note-on/off's.
				status = e.m_buffer[0] & 0xf0;
				switch(status)
				{
				case 0x80: // Note-off
					chan = e.m_buffer[0] & 0x0f;
					pitch = e.m_buffer[1];
					if(m_activeNoteMatrix[chan][pitch] > 0)
						m_activeNoteMatrix[chan][pitch] -= 1;
					break;
				case 0x90: // Note-on
					chan = e.m_buffer[0] & 0x0f;
					pitch = e.m_buffer[1];
					vel = e.m_buffer[2];
					if(vel > 0)
						m_activeNoteMatrix[chan][pitch] += 1;
					else if(m_activeNoteMatrix[chan][pitch] > 0)
						m_activeNoteMatrix[chan][pitch] -= 1;
					break;
				}
			}
			break;
		default: break;
		}
		++m_cur_event;
	}
}


void Sequencer::Write(const string & file)
{
	int file_type = FILE_BINARY;
	string file_lowercase = file;

	to_lower(file_lowercase);

	if(m_playing) StopPlaying();
	if(m_recording) StopRecording();
	if(file_lowercase.find(".json") != string::npos)
		file_type = FILE_JSON;
	else if(file_lowercase.find(".xml") != string::npos)
		file_type = FILE_XML;

	if(FILE_BINARY == file_type)
	{
		WriteBinary(file);
		return;
	}

	#ifdef _USE_BOOST_SERIALIZATION
	try
	{
		ptree top_pt;
		ptree & pt = top_pt.add_child("Events", ptree()); // Create Events ptree and save reference in pt.
		ScopedLock k(m_lock);

		ptr_multiset<Event>::iterator it;
		for(it = m_events.begin(); it != m_events.end(); it++) it->save(pt);

		if(FILE_JSON == file_type)
		{
			write_json(file, top_pt, std::locale());
		}
		else if(FILE_XML == file_type)
		{
      boost::property_tree::xml_writer_settings<std::string> w(' ', 4); // Set indentation character and amount.
      write_xml(file, top_pt, std::locale(), w);
			//write_xml(file, top_pt, std::locale(), boost::property_tree::xml_parser::xml_writer_make_settings(' ', 4u));
		}
		object_post(m_obj, "Finished writing %s.", file.c_str());
	}
	catch(std::exception & ex)
	{
		object_error(m_obj, "Sequencer::WriteToFile() exception: %s", ex.what());
	}
	#endif
}

void Sequencer::Read(const string & file)
{
	int file_type = FILE_BINARY;
	string file_lowercase = file;

	to_lower(file_lowercase);

	if(m_playing) StopPlaying();
	if(m_recording) StopRecording();
	if(file_lowercase.find(".json") != string::npos)
		file_type = FILE_JSON;
	else if(file_lowercase.find(".xml") != string::npos)
		file_type = FILE_XML;

	if(FILE_BINARY == file_type)
	{
		ReadBinary(file);
		return;
	}

	#ifdef _USE_BOOST_SERIALIZATION
	try
	{
		int type = EVENT_TYPE_NONE;
		ptree top_pt;
		ptree::iterator it;

		if(FILE_JSON == file_type)
			read_json(file, top_pt);
		else if(FILE_XML == file_type)
			read_xml(file, top_pt);

		ScopedLock k(m_lock);
		m_events.clear();
		ptree & t = top_pt.get_child("Events");

		for(it = t.begin(); it != t.end(); it++)
		{
			type = it->second.get<int>("type");
			switch(type)
			{
			case EVENT_TYPE_CSOUND:  m_events.insert(new CsoundEvent(it->second)); break;
			case EVENT_TYPE_CONTROL: m_events.insert(new ControlEvent(it->second));	break;
			case EVENT_TYPE_MIDI:    m_events.insert(new MidiEvent(it->second)); break;
			case EVENT_TYPE_STRING:  m_events.insert(new StringEvent(it->second)); break;
			default:
				object_error(m_obj, "Unknown event type found in %s.", file.c_str());
				break;
			}
		}
		object_post(m_obj, "Finished reading %s.", file.c_str());
	}
	catch(std::exception & ex)
	{
		object_error(m_obj, "Sequencer::Read() exception: %s", ex.what());
	}
	#endif
}

void Sequencer::ReadBinary(const string & file)
{
	FILE *fp = NULL;
	byte *buffer = NULL, *bytePtr = NULL;
	int fileSize = 0, numEvents = 0, result;
	int i, type, time, len, data2_size, magic;
	char buf[MAX_EVENT_MESSAGE_SIZE];
	float value;
	bool reverse = false;
	int magic_number = FILE_MAGIC_NUMBER;
	int magic_number_reverse = magic_number;

	reverseBytes((byte*)&magic_number_reverse, sizeof(int));

	if(m_playing) StopPlaying();
	if(m_recording) StopRecording();

	ScopedLock k(m_lock);

	m_events.clear();

	fp = fopen(file.c_str(), "rb");

	if(fp == NULL)
	{
		object_error(m_obj, "fopen() failed for %s", file.c_str());
		return;
	}

	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buffer = (byte*) MemoryNew(fileSize);

	if(buffer == NULL)
	{
		object_error(m_obj, "MemoryNew() failed to create buffer for reading from %s", file.c_str());
		fclose(fp);
		return;
	}

	result = fread(buffer, 1, fileSize, fp);

	if(result != fileSize)
	{
		fclose(fp);
		MemoryFree(buffer);
		object_error(m_obj, "fread() failed for %s", file.c_str());
		return;
	}

	fclose(fp);
	bytePtr = buffer;
	magic = *(int*)bytePtr;
	bytePtr += sizeof(int);

	if(magic != magic_number && magic != magic_number_reverse)
	{
		object_error(m_obj, "%s is not a csound~ recorded sequence.", file.c_str());
	}
	else
	{
		if(magic == magic_number_reverse) reverse = true;
		numEvents = *(int*)bytePtr;
		reverseNumber((byte*)&numEvents, sizeof(int), reverse);
		bytePtr += sizeof(int);

		for(i=0; i<numEvents; i++)
		{
			type = *(int*)bytePtr;
			reverseNumber((byte*)&type, sizeof(int), reverse);
			bytePtr += sizeof(int);

			time = *(int*)bytePtr;
			reverseNumber((byte*)&time, sizeof(int), reverse);
			bytePtr += sizeof(int);

			len = *(int*)bytePtr; // data1 size
			reverseNumber((byte*)&len, sizeof(int), reverse);
			bytePtr += sizeof(int);

			m_time = time;		// must set time before adding event

			switch(type)
			{
			case EVENT_TYPE_CSOUND:
				memcpy(buf, bytePtr, len);		// get data1
				bytePtr += len + sizeof(int);	// skip data1 and data2 size (== 0 in this case)
				AddCsoundEvent(buf, false);
				break;
			case EVENT_TYPE_CONTROL:
				memcpy(buf, bytePtr, len);		// get data1
				bytePtr += len + sizeof(int);	// skip data1 and data2 size (== sizeof(float) in this case)
				value = *(float*)bytePtr;		// get data2
				reverseNumber((byte*)&value, sizeof(int), reverse);
				bytePtr += sizeof(float);		// skip data2
				AddControlEvent(buf, (double)value, false);
				break;
			case EVENT_TYPE_STRING:
				memcpy(buf, bytePtr, len);		// get data1
				bytePtr += len;					// skip data1
				data2_size = *(int*)bytePtr;	// get data2 size
				bytePtr += sizeof(int);			// skip data2 size
				AddStringEvent(buf, (char*)bytePtr, false); // Add data2 string to sequencer.
				bytePtr += data2_size;			// skip data2
				break;
			case EVENT_TYPE_MIDI:
				memcpy(buf, bytePtr, len);		// data1
				bytePtr += len + sizeof(int);	// skip data1 and data2 size (== 0 in this case)
				AddMIDIEvent((byte*) buf, len, false);
				break;
			}
		}

		object_post(m_obj, "\"%s\" successfully read into csound~.", file.c_str());
	}

	MemoryFree(buffer);
}

void Sequencer::WriteBinary(const string & file)
{
	FILE *fp = NULL;
	byte *buffer = NULL;
	int realloc_result=0, result=0, len, byteCount = 0, bufferSize = DEFAULT_SAVE_LOAD_BUFFER_SIZE;
	int numEvents = 0;
	float fval;
	void * vptr = NULL;
	static int magic = FILE_MAGIC_NUMBER;

	if(m_playing) StopPlaying();
	if(m_recording) StopRecording();

	ScopedLock k(m_lock);

	fp = fopen(file.c_str(), "wb");

	if(fp == NULL)
	{
		object_post(m_obj, "fopen() failed for %s", file.c_str());
		return;
	}

	buffer = (byte *) MemoryNew(bufferSize);

	if(buffer == NULL)
	{
		object_post(m_obj, "MemoryNew() failed to create buffer for writing to %s", file.c_str());
		return;
	}

	// First is the magic number.
	realloc_result = BufferWrite(&buffer, &magic, sizeof(int), &byteCount, &bufferSize);

	// Then comes the # of events.
	if(!realloc_result)
	{
		numEvents = m_events.size();
		realloc_result = BufferWrite(&buffer, &numEvents, sizeof(int), &byteCount, &bufferSize);
	}

	// Now we add the events.
	boost::ptr_multiset<Event>::iterator it;
	for(it = m_events.begin(); it != m_events.end(); it++)
	{
		// Save type.
		if(!realloc_result)
			realloc_result = BufferWrite(&buffer, &it->m_type, sizeof(int), &byteCount, &bufferSize);

		// Save time.
		if(!realloc_result)
			realloc_result = BufferWrite(&buffer, &it->m_time, sizeof(int), &byteCount, &bufferSize);

		// Get data1 size.
		switch(it->m_type)
		{
		case EVENT_TYPE_CSOUND:
		case EVENT_TYPE_CONTROL:
		case EVENT_TYPE_STRING:
			len = static_cast<StringEvent&>(*it).m_name.size() + 1;
			vptr = (void*) static_cast<StringEvent&>(*it).m_name.c_str();
			break;
		case EVENT_TYPE_MIDI:
			len = static_cast<MidiEvent&>(*it).m_size;
			vptr = (void*) static_cast<MidiEvent&>(*it).m_buffer;
			break;
		}

		// Save data1 size.
		if(!realloc_result)
			realloc_result = BufferWrite(&buffer, &len, sizeof(int), &byteCount, &bufferSize);

		// Save data1.
		if(!realloc_result)
			realloc_result = BufferWrite(&buffer, vptr, len, &byteCount, &bufferSize);

		// Get data2 size.
		switch(it->m_type)
		{
		case EVENT_TYPE_CSOUND:
		case EVENT_TYPE_MIDI:
			len = 0;
			break;
		case EVENT_TYPE_CONTROL:
			len = sizeof(float);
			fval = (float) static_cast<ControlEvent&>(*it).m_value;
			vptr = &fval;
			break;
		case EVENT_TYPE_STRING:
			len = static_cast<StringEvent&>(*it).m_string.size() + 1;
			vptr = (void*) static_cast<StringEvent&>(*it).m_string.c_str();
			break;
		}

		// Save data2 size.
		if(!realloc_result)
			realloc_result = BufferWrite(&buffer, &len, sizeof(int), &byteCount, &bufferSize);

		// Save data2.
		if(!realloc_result && len > 0)
			realloc_result = BufferWrite(&buffer, vptr, len, &byteCount, &bufferSize);

		if(realloc_result) break;
	}

	if(!realloc_result)
		result = fwrite(buffer, 1, byteCount, fp);

	fclose(fp);

	if(realloc_result == 1 || result != byteCount) object_post(m_obj, "fwrite() failed to write to %s.", file.c_str());
	else post("%s successfully saved.", file.c_str());

	MemoryFree(buffer);
}
