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
#include "Score.hpp"
#include "System.hpp"
#include <set>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <sstream>

namespace csound
{
	Score::Score(void) : 
		rescaleMinima(Event::ELEMENT_COUNT), 
		rescaleRanges(Event::ELEMENT_COUNT)
	{
		initialize();
	}

	Score::~Score(void)
	{
	}

	void Score::load(std::string filename)
	{
		System::inform("BEGAN Score::load(%s)...\n", filename.c_str());
		std::ifstream stream;
		//stream.open(filename.c_str(), std::ios_base::binary);
		stream.open(filename.c_str(), std::ifstream::binary);
		load(stream);
		stream.close();
		System::inform("ENDED Score::load().\n");
	}

	void Score::load(std::istream &stream)
	{
		MidiFile midiFile;
		midiFile.read(stream);
		load(midiFile);
	}

	void Score::save(std::string filename)
	{
		System::inform("BEGAN Score::save(%s)...\n", filename.c_str());
		std::ofstream stream;
		//stream.open(filename.c_str(), std::ios_base::binary);
		stream.open(filename.c_str(), std::ifstream::binary);
		save(stream);
		stream.close();
		System::inform("ENDED Score::save().\n");
	}

	void Score::save(std::ostream &stream)
	{
		save(midifile);
		midifile.write(stream);
	}

	static double max(double a, double b)
	{
		if(a > b)
		{
			return a;
		}
		return b;
	}

	static double min(double a, double b)
	{
		if(a < b)
		{
			return a;
		}
		return b;
	}

	void Score::getScale(std::vector<Event> &score, int dimension, size_t beginAt, size_t endAt, double &minimum, double &range)
	{
		if(beginAt == endAt)
		{
			return;
		}
		const Event &beginEvent = score[beginAt];
		double maximum = beginEvent[dimension];
		const Event &endEvent = score[endAt - 1];
		minimum = endEvent[dimension];
		if(dimension == Event::TIME)
		{
			const Event &e = score[beginAt];
			maximum = max(e.getTime(), e.getTime() + e.getDuration());
			minimum = min(e.getTime(), e.getTime() + e.getDuration());
			double beginning;
			double ending;
			for( ; beginAt != endAt; ++beginAt)
			{
				const Event &event = score[beginAt];
				beginning = min(event.getTime(), event.getTime() + event.getDuration());
				ending = max(event.getTime(), event.getTime() + event.getDuration());
				if(ending > maximum)
				{
					maximum = ending;
				}
				else if(beginning < minimum)
				{
					minimum = beginning;
				}
			}
		}
		else
		{
			for( ; beginAt != endAt; ++beginAt)
			{
				const Event &event = score[beginAt];
				if(event[dimension] > maximum)
				{
					maximum = event[dimension];
				}
				if(event[dimension] < minimum)
				{
					minimum = event[dimension];
				}
			}
		}
		range = maximum - minimum;
	}

	void Score::setScale(std::vector<Event> &score, int dimension, bool rescaleMinimum, bool rescaleRange, size_t beginAt, size_t endAt, double targetMinimum, double targetRange)
	{
		if(!(rescaleMinimum || rescaleRange))
		{
			return;
		}
		if(beginAt == endAt)
		{
			return;
		}
		double actualMinimum;
		double actualRange;
		getScale(score, dimension, beginAt, endAt, actualMinimum, actualRange);
		double scale;
		if(actualRange == 0.0)
		{
			scale = 1.0;
		}
		else
		{
			scale = targetRange / actualRange;
		}
		for( ; beginAt != endAt; ++beginAt)
		{
			Event &event = score[beginAt];
			event[dimension] = event[dimension] - actualMinimum;
			if(rescaleRange)
			{
				event[dimension] = event[dimension] * scale;
			}
			if(rescaleMinimum)
			{
				event[dimension] = event[dimension] + targetMinimum;
			}
			else
			{
				event[dimension] = event[dimension] + actualMinimum;
			}
		}
	}

	void Score::findScale(void)
	{
		for(int dimension = 0; dimension < Event::ELEMENT_COUNT; ++dimension)
		{
			getScale(*this, dimension, 0, size(), scaleActualMinima[dimension], scaleActualRanges[dimension]);
		}
	}

	void Score::rescale(void)
	{
		for(int dimension = 0; dimension < Event::ELEMENT_COUNT; ++dimension)
		{
			setScale(*this,
				dimension, 
				rescaleMinima[dimension], 
				rescaleRanges[dimension], 
				0, 
				size(), 
				scaleTargetMinima[dimension], 
				scaleTargetRanges[dimension]);
		}
	}

	void Score::rescale(Event &event)
	{
		for(int dimension = 0; dimension < Event::HOMOGENEITY; dimension++)
		{
			event[dimension] = event[dimension] - scaleActualMinima[dimension];
			double scale;
			if(scaleActualRanges[dimension] == 0.0)
			{
				scale = 1.0;
			}
			else
			{
				scale = scaleTargetRanges[dimension] / scaleActualRanges[dimension];
			}
			if(rescaleRanges[dimension])
			{
				event[dimension] = event[dimension] * scale;
			}
			if(rescaleMinima[dimension])
			{
				event[dimension] = event[dimension] + scaleTargetMinima[dimension];
			}
			else
			{
				event[dimension] = event[dimension] + scaleActualMinima[dimension];
			}
		}
	}

	void Score::load(MidiFile &midiFile)
	{
		std::vector<Event>::clear();
		for(std::vector<MidiTrack>::iterator i = midiFile.midiTracks.begin(); i != midiFile.midiTracks.end(); ++i)
		{
			std::set<MidiEvent*> offEvents;
			for(std::vector<MidiEvent>::iterator j = (*i).begin(); j != (*i).end(); ++j)
			{
				MidiEvent &noteOnEvent = *j;
				if(noteOnEvent.isNoteOn())
				{
					for(std::vector<MidiEvent>::iterator k = j; k != (*i).end(); ++k)
					{
						MidiEvent &noteOffEvent = *k;
						if(noteOnEvent.isMatchingNoteOff(noteOffEvent))
						{
							if(offEvents.find(&noteOffEvent) == offEvents.end())
							{
								double status = noteOnEvent.getStatusNybble();
								double instrument = noteOnEvent.getChannelNybble();
								double time = noteOnEvent.time;
								double duration = noteOffEvent.time - noteOnEvent.time;
								double key = noteOnEvent.getKey();
								double velocity = noteOnEvent.getVelocity();
								push_back(Event(time, duration, status, instrument, key, velocity, 0, 0, 0, 0, 4095));
								offEvents.insert(&noteOffEvent);
								break;
							}
						}
					}						
				}
			}
		}
		findScale();
		sort();
	}

	void Score::save(MidiFile &midiFile)
	{
		findScale();
		sort();
		// Format 0 file.
		midiFile.clear();
		MidiTrack midiTrack;
		midiFile.midiTracks.push_back(midiTrack);
		for(Score::iterator it = begin(); it != end(); ++it)
		{
			Event &event = (*it);
			// event.dump(std::cout);
			if(event.isNoteOn())
			{
				MidiEvent onEvent;
				onEvent.time = event.getTime();
				onEvent.ticks = int(Conversions::round(onEvent.time / midiFile.currentSecondsPerTick));
				onEvent.push_back(event.getMidiStatus());
				onEvent.push_back(event.getKeyNumber());
				onEvent.push_back(event.getVelocityNumber());
				midiFile.midiTracks[0].push_back(onEvent);
				MidiEvent offEvent;
				offEvent.time = event.getTime() + event.getDuration();
				offEvent.ticks = int(Conversions::round(offEvent.time / midiFile.currentSecondsPerTick));
				offEvent.push_back(event.getMidiStatus());
				offEvent.push_back(event.getKeyNumber());
				offEvent.push_back(0);
				midiFile.midiTracks[0].push_back(offEvent);
			}
		}
		midiFile.midiTracks[0].sort();
		MidiEvent trackEnd;
		if(midiFile.midiTracks.size() > 0)
		{
			if(midiFile.midiTracks[0].size() > 0)
			{
				trackEnd.ticks = midiFile.midiTracks[0][midiFile.midiTracks[0].size() - 1].ticks;
			}
		}
		trackEnd.push_back(MidiFile::META_EVENT);
		trackEnd.push_back(MidiFile::META_END_OF_TRACK);
		trackEnd.push_back(0);
		midiFile.midiTracks[0].push_back(trackEnd);
	}

	void Score::dump(std::ostream &stream) 
	{
		stream << "silence::Score = " << int(size()) << " events:" << std::endl;
		for(Score::iterator i = begin(); i != end(); ++i)
		{
			(*i).dump(stream);
		}
	}

	std::string Score::toString()
	{
		std::ostringstream stream;
		dump(stream);
		return stream.str();
	}

	void Score::initialize(void)
	{
		std::vector<Event>::clear();
		scaleTargetMinima[Event::STATUS] = 0;
		scaleTargetMinima[Event::INSTRUMENT] = 0;
		scaleTargetMinima[Event::TIME] = 0;
		scaleTargetMinima[Event::DURATION] = 0.125;
		scaleTargetMinima[Event::KEY] = 36.0;
		scaleTargetMinima[Event::VELOCITY] = 60.0;
		scaleTargetMinima[Event::PAN] = 0;
		scaleTargetMinima[Event::DEPTH] = 0;
		scaleTargetMinima[Event::HEIGHT] = 0;
		scaleTargetMinima[Event::PHASE] = 0;
		scaleTargetMinima[Event::PITCHES] = 0;
		scaleTargetMinima[Event::HOMOGENEITY] = 1.0;
		rescaleMinima[Event::STATUS] = false;
		rescaleMinima[Event::HOMOGENEITY] = false;
		scaleTargetRanges[Event::STATUS] = 255;
		scaleTargetRanges[Event::INSTRUMENT] = 4;
		scaleTargetRanges[Event::TIME] = 240.0;
		scaleTargetRanges[Event::DURATION] = 4.0;
		scaleTargetRanges[Event::KEY] = 60.0;
		scaleTargetRanges[Event::VELOCITY] = 20.0;
		scaleTargetRanges[Event::PAN] = 0;
		scaleTargetRanges[Event::DEPTH] = 0;
		scaleTargetRanges[Event::HEIGHT] = 0;
		scaleTargetRanges[Event::PHASE] = 0;
		scaleTargetRanges[Event::PITCHES] = 4095;
		scaleTargetRanges[Event::HOMOGENEITY] = 0;
		rescaleRanges[Event::STATUS] = false;
		rescaleRanges[Event::HOMOGENEITY] = false;
	}

	void Score::append(double time, double duration, double status, double instrument, double key, double velocity, double phase, double pan, double depth, double height, double pitches)
	{
		Event event;
		event.setTime(time);
		event.setDuration(duration);
		event.setStatus(status);
		event.setInstrument(instrument);
		event.setKey(key);
		event.setVelocity(velocity);
		event.setPhase(phase);
		event.setPan(pan);
		event.setDepth(depth);
		event.setHeight(height);
		event.setPitches(pitches);
		push_back(event);
	}

	void Score::append(Event event)
	{
		push_back(event);
	}

	void Score::sort()
	{
		std::sort(begin(), end(), std::less<Event>());
	}

	double Score::getDuration() 
	{
		findScale();
		double duration = 0.0;
		double duration_ = 0.0;
		for(Score::iterator it = begin(); it != end(); ++it)
		{
			duration_ = it->getTime() + it->getDuration();
			if(duration_ > duration)
			{
				duration = duration_;
			}
		}
		return (duration - scaleActualMinima.getTime());
	}

	void Score::rescale(int dimension, bool rescaleMinimum, double minimum, bool rescaleRange, double range)
	{
		setScale(*this, dimension, rescaleMinimum, rescaleRange, 0, size(), minimum, range);
	}
}
