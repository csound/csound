/**
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
#ifdef _MSC_VER
#pragma warning (disable:4786) 
#endif
#include "Midifile.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

namespace csound 
{
	int MidiFile::readVariableLength(std::istream &stream)
	{
		char c = 0;
		stream.get(c);
		int value = c;
		if ( c & 0x80 ) 
		{
			value &= 0x7f;
			do {
				stream.get(c);
				value = (value << 7) + (c & 0x7f);
			} while (c & 0x80);
		}
		return value;
	} 

	void MidiFile::writeVariableLength(std::ostream &stream, int value)
	{
		unsigned long buffer = value & 0x7f;
		while((value >>= 7) > 0)
		{
			buffer <<= 8;
			buffer |= 0x80;
			buffer += (value & 0x7f);
		}
		for(;;)
		{
			unsigned char c = (unsigned char) (buffer & 0xff);
			stream.put(c); 
			if(buffer & 0x80)
			{
				buffer >>= 8;
			}
			else
			{
				return;
			}
		}
	} 

	int MidiFile::toInt(int c1, int c2, int c3, int c4)
	{
		int value = 0;
		value =                (c1 & 0xff);
		value = (value << 8) + (c2 & 0xff);
		value = (value << 8) + (c3 & 0xff);
		value = (value << 8) + (c4 & 0xff);
		return value;
	}

	short MidiFile::toShort(int c1, int c2)
	{
		return ((c1 & 0xff ) << 8) + (c2 & 0xff);
	}

	int MidiFile::readInt(std::istream &stream)
	{
		char c1, c2, c3, c4;
		stream.get(c1);
		stream.get(c2);
		stream.get(c3);
		stream.get(c4);
		return toInt(c1,c2,c3,c4);
	}

	void MidiFile::writeInt(std::ostream &stream, int value)
	{		
		stream.put((char)((0xff000000 & value) >> 24));
		stream.put((char)((0x00ff0000 & value) >> 16));
		stream.put((char)((0x0000ff00 & value) >>  8));
		stream.put((char)((0xf00000ff & value)      ));
	}

	short MidiFile::readShort(std::istream &stream)
	{
		char c1, c2;
		stream.get(c1);
		stream.get(c2);
		return toShort(c1,c2);
	}

	void MidiFile::writeShort(std::ostream &stream, short value)
	{
		stream.put((unsigned char)((0x0000ff00 & value) >>  8));
		stream.put((unsigned char)((0x000000ff & value)      ));
	}

	int MidiFile::chunkName(int a, int b, int c, int d)
	{
		return (a << 24) + (b << 16) + (c << 8) + d;
	}

	Chunk::Chunk(const char *_id)
	{
		id = MidiFile::chunkName(_id[0], _id[1], _id[2], _id[3]);
	}

	Chunk::~Chunk()
	{
	}

	void Chunk::read(std::istream &stream)
	{
		int _id = MidiFile::readInt(stream);
		char idString[5];
		memcpy(idString, &id, 4);
		idString[4] = 0;
		char _idString[5];
		memcpy(_idString, &_id, 4);
		_idString[4] = 0;
		if(id != _id)
		{
			std::cerr << "Unexpected chunk id: " << _idString << " (should be " << idString << ")." << std::endl;
		}
		else
		{
			std::cerr << "Read chunk: " << _idString << "." << std::endl;
		}
		chunkSize = MidiFile::readInt(stream);
	}

	void Chunk::write(std::ostream &stream)
	{
		MidiFile::writeInt(stream, id);
		markChunkSize(stream);
		MidiFile::writeInt(stream, chunkSize);
		markChunkStart(stream);
	}

	void Chunk::markChunkSize(std::ostream &stream)
	{
		chunkSizePosition = stream.tellp();
	}

	void Chunk::markChunkStart(std::ostream &stream)
	{
		chunkStart = stream.tellp();
	}

	void Chunk::markChunkEnd(std::ostream &stream)
	{
		chunkEnd = stream.tellp();
		chunkSize = chunkEnd - chunkStart;
		stream.seekp(chunkSizePosition);
		MidiFile::writeInt(stream, chunkSize);
		stream.seekp(chunkEnd);
	}

	MidiHeader::MidiHeader() : Chunk("MThd")
	{
		clear();
	}

	MidiHeader::~MidiHeader()
	{
		clear();
	}

	void MidiHeader::clear()
	{
		type = 0;
		trackCount = 1;
		timeFormat = 480;
	}

	void MidiHeader::read(std::istream &stream)
	{
		Chunk::read(stream);
		type = MidiFile::readShort(stream);
		trackCount = MidiFile::readShort(stream);
		timeFormat = MidiFile::readShort(stream);
	}

	void MidiHeader::write(std::ostream &stream)
	{
		Chunk::write(stream);
		MidiFile::writeShort(stream, type);
		MidiFile::writeShort(stream, trackCount);
		MidiFile::writeShort(stream, timeFormat);
		Chunk::markChunkEnd(stream);
	}

	MidiEvent::MidiEvent() : ticks(0), time(0)
	{
	}

	MidiEvent::~MidiEvent()
	{
	}

	void MidiEvent::write(std::ostream &stream, MidiFile &midiFile, int lastTick)
	{
		int deltaTicks = ticks - lastTick;
		MidiFile::writeVariableLength(stream, deltaTicks);
		//	Channel event.
		if(getMetaType() < 0)
		{
			for(size_t i = 0, n = size(); i < n; i++)
			{
				stream.put((*this)[i]);
			}
		}
		//	Meta event.
		else
		{
			stream.put((*this)[0]);
			stream.put((*this)[1]);
			size_t n = getMetaSize();
			MidiFile::writeVariableLength(stream, (int) n);
			size_t i;
			for(i = 3, n = size(); i < n; i++)
			{
				stream.put((*this)[i]);
			}
		}
	}

	int MidiEvent::getStatus()
	{
		return (*this)[0];
	}

	int MidiEvent::getStatusNybble()
	{
		return (*this)[0] & 0xf0;
	}

	int MidiEvent::getChannelNybble()
	{
		return (*this)[0] & 0x0f;
	}

	int MidiEvent::getKey()
	{
		return (*this)[1];
	}

	int MidiEvent::getVelocity()
	{
		return (*this)[2];
	}

	int MidiEvent::getMetaType()
	{
		if(getStatus() == MidiFile::META_EVENT)
		{
			return (*this)[1];
		}
		return -1;
	}

	unsigned char MidiEvent::getMetaData(int i)
	{
		return (*this)[i + 2];
	}

	size_t MidiEvent::getMetaSize()
	{
		return (size() - 2);
	}

	unsigned char MidiEvent::read(std::istream &stream)
	{
		char c;
		stream.get(c);
		push_back((unsigned char) c);
#ifdef _DEBUG
//		fprintf(stderr, " pos %x val %x\n", (int) stream.tellg(), (unsigned char) c);
#endif
		return (unsigned char) c;
	}

	bool MidiEvent::isChannelVoiceMessage()
	{
		if(getStatusNybble() < MidiFile::CHANNEL_NOTE_OFF)
		{
			return false;
		}
		if(getStatusNybble() > MidiFile::CHANNEL_PITCH_BEND)
		{
			return false;
		}
		return true;
	}

	bool MidiEvent::isNoteOn()
	{
		if(getStatusNybble() == MidiFile::CHANNEL_NOTE_ON && (*this)[2] > 0)
		{
			return true;
		}
		return false;
	}

	bool MidiEvent::isNoteOff()
	{
		if(getStatusNybble() == MidiFile::CHANNEL_NOTE_OFF)
		{
			return true;
		}
		if(getStatusNybble() == MidiFile::CHANNEL_NOTE_ON && (*this)[2] == 0)
		{
			return true;
		}
		return false;
	}

	bool MidiEvent::isMatchingNoteOff(MidiEvent &offEvent)
	{
		if(!(offEvent.time > time))
		{
			return false;
		}
		if(!isNoteOn())
		{
			return false;
		}
		if(!offEvent.isNoteOff())
		{
			return false;
		}
		if(getKey() != offEvent.getKey())
		{
			return false;
		}
		return true;
	}

	MidiTrack::MidiTrack() : Chunk("MTrk")
	{
	}

	MidiTrack::~MidiTrack()
	{
	}

	void MidiTrack::read(std::istream &stream, MidiFile &midiFile)
	{
		Chunk::read(stream);
		for(int eventCount = 0; ; eventCount++)
		{
			MidiEvent midiEvent;
			midiEvent.read(stream, midiFile);
			push_back(midiEvent);
			if(stream.eof())
			{
				break;
			}
			if(midiEvent.getMetaType() == MidiFile::META_END_OF_TRACK)
			{
				break;
			}
		}
	}

	void MidiEvent::read(std::istream &stream, MidiFile &midiFile)
	{
		ticks = MidiFile::readVariableLength(stream);
		midiFile.currentTick += ticks;
		double secondsPerTick = midiFile.tempoMap.getCurrentSecondsPerTick(midiFile.currentTick);
		if(secondsPerTick == -1)
		{
			secondsPerTick = midiFile.currentSecondsPerTick;
		}
		time = midiFile.currentTime = (midiFile.currentTime + (secondsPerTick * ticks));
		int peeked = stream.peek();
		if(stream.eof())
		{
			std::cerr << "MIDI file incorrectly read EOF." << std::endl;
			return;
		}
		if(peeked < 0x80)
		{
#ifdef _DEBUG
//			fprintf(stderr, "Running status: %x\n", midiFile.lastStatus);
#endif
			push_back(midiFile.lastStatus);
		}
		else
		{
			read(stream);
			midiFile.lastStatus = getStatus();
#ifdef _DEBUG
//			fprintf(stderr, "Status:         %x\n", getStatus());
#endif
		}
		switch(getStatusNybble())
		{
		case MidiFile::CHANNEL_NOTE_OFF:
		case MidiFile::CHANNEL_NOTE_ON:
		case MidiFile::CHANNEL_KEY_PRESSURE:
		case MidiFile::CHANNEL_CONTROL_CHANGE:
		case MidiFile::CHANNEL_PITCH_BEND:
			{
				read(stream);
				read(stream);
			}
			break;
		case MidiFile::CHANNEL_PROGRAM_CHANGE:
		case MidiFile::CHANNEL_AFTER_TOUCH:
			{
				read(stream);
			}
			break;
		case MidiFile::SYSTEM_EXCLUSIVE:
			{
				switch(getStatus())
				{
				case MidiFile::SYSTEM_EXCLUSIVE:
					while(read(stream) != 0xf7);
					break;
				case MidiFile::META_EVENT:
					{
						//	Type.
						read(stream);
						//	Size.
						int n = MidiFile::readVariableLength(stream);
						//	Data.
						for(int i = 0; i < n; i++)
						{
							read(stream);
						}
						std::cout << "Meta event " << getMetaType() << " (" << n << " bytes): ";
						switch(getMetaType())
						{
						case MidiFile::META_SET_TEMPO:
							{
								std::cout << "set tempo";
								midiFile.microsecondsPerQuarterNote = (getMetaData(0) << 16) + (getMetaData(1) << 8) + getMetaData(2);
								midiFile.computeTimes();
							}
							break;
						case MidiFile::META_TIME_SIGNATURE:
							{
								std::cout << "time signature";
								double numerator = getMetaData(0);
								double denominator = getMetaData(1);
								double clocksPerBeat = getMetaData(2);
								double thirtySecondNotesPerMidiQuarterNote = getMetaData(3);
							}
							break;
						case MidiFile::META_SEQUENCER_SPECIFIC:
							std::cout << "sequencer specific";
							break;
						default:
							std::cout << "not handled";
							break;
						}
						std::cout << std::endl;
					}
					break;
				}
			}
			break;
		default:
			{
				int badStatus = getStatus();
				std::cout << "Error reading midi event: status == " << badStatus << std::endl;
			}
			break;
		}
	}

	void MidiTrack::write(std::ostream &stream, MidiFile &midiFile)
	{
		Chunk::write(stream);
		int lastTick = 0;
		for(std::vector<MidiEvent>::iterator it = begin(); it != end(); ++it)
		{
			MidiEvent &event = (*it);
			event.write(stream, midiFile, lastTick);
			lastTick = event.ticks;
		}
		Chunk::markChunkEnd(stream);
	}

	double TempoMap::getCurrentSecondsPerTick(int tick)
	{
		std::map<int,double>::iterator it = lower_bound(tick);
		if(it == end())
		{
			return -1;
		}
		return (*it).second;
	}

	MidiFile::MidiFile()
	{
		clear();
	}

	MidiFile::~MidiFile()
	{
	}

	void MidiFile::clear()
	{
		currentTick = 0;
		currentTime = 0;
		currentSecondsPerTick = 0;
		lastStatus = 0;
		midiHeader.clear();
		midiTracks.erase(midiTracks.begin(), midiTracks.end());
		tempoMap.erase(tempoMap.begin(), tempoMap.end());
		microsecondsPerQuarterNote = (60.0 / 120.0) * 1000000.0;
		computeTimes();
	}

	void MidiFile::read(std::istream &stream)
	{
		clear();
		midiHeader.read(stream);
		computeTimes();
		for(int i = 0; i < midiHeader.trackCount; i++)
		{
			currentTick = 0;
			currentTime = 0;
			MidiTrack midiTrack;
			midiTrack.read(stream, *this);
			midiTracks.push_back(midiTrack);
		}
	}

	void MidiFile::write(std::ostream &stream)
	{
		midiHeader.write(stream);
		for(int i = 0; i < midiHeader.trackCount; i++)
		{
			midiTracks[i].write(stream, *this);
		}
	}

	void MidiFile::load(std::string filename)
	{
		std::ifstream stream(filename.c_str(), std::ifstream::binary | std::ifstream::in);
		read(stream);
	}

	void MidiFile::save(std::string filename)
	{
		std::ofstream stream(filename.c_str(), std::ofstream::binary | std::ofstream::out);
		write(stream);
	}

	void MidiFile::computeTimes()
	{
		if(midiHeader.timeFormat < 0)
		{
			int frameCode = (-midiHeader.timeFormat) >> 8;
			double framesPerSecond;
			switch(frameCode)
			{
			case 24: 
				framesPerSecond = 24.0;
				break;
			case 25:
				framesPerSecond = 25.0;
				break;
			case 29:
				framesPerSecond = 29.97;
				break;
			case 30:
				framesPerSecond = 30.0;
				break;
			default:
				framesPerSecond = 30.0;
			}
			int ticksPerFrame = midiHeader.timeFormat & 0xff;
			currentSecondsPerTick = (1.0 / framesPerSecond) / ticksPerFrame;
		}
		else
		{
			double ticksPerQuarterNote = double(midiHeader.timeFormat);
			double secondsPerQuarterNote = microsecondsPerQuarterNote / 1000000.0;
			currentSecondsPerTick = secondsPerQuarterNote / ticksPerQuarterNote;
		}
		tempoMap[currentTick] = currentSecondsPerTick;
	}

	void MidiFile::dump(std::ostream &stream)
	{
		stream << "CHUNK ID: " << midiHeader.id << std::endl;
		stream << "Type: " << midiHeader.type << std::endl;
		stream << "Tracks: " << midiHeader.trackCount << std::endl;
		stream << "Time format: " << midiHeader.timeFormat << std::endl;
		for(size_t i = 0; i < midiTracks.size(); i++)
		{
			stream << "TRACK: " << (unsigned int) i << std::endl;
			MidiTrack &midiTrack = midiTracks[i];
			for(size_t j = 0; j < midiTrack.size(); j++)
			{
				MidiEvent &midiEvent = midiTrack[j];
				stream << (unsigned int) j << " (" << midiEvent.ticks << ":" << midiEvent.time << ") ";
				for(size_t k = 0; k < midiEvent.size(); k++)
				{
					stream << (int) midiEvent[k] << " ";
				}
				stream << std::endl;
			}
		}
	}

	void MidiFile::sort()
	{
		for(std::vector<MidiTrack>::iterator it = midiTracks.begin(); it != midiTracks.end(); ++it)
		{
			(*it).sort();
		}
	}

	void MidiTrack::sort()
	{
		std::sort(begin(), end());
	}

	bool operator < (const MidiEvent &a, MidiEvent &b)
	{

		return (a.time < b.time);
	}
}
