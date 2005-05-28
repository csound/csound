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
#if defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning (disable:4786)
#endif
#include "Event.hpp"
#include "Midifile.hpp"

namespace csound
{
        int     Event::SORT_ORDER[] = {
                Event::TIME,
                Event::INSTRUMENT,
                Event::KEY,
                Event::DURATION,
                Event::VELOCITY,
                Event::PAN,
                Event::DEPTH,
                Event::HEIGHT,
                Event::PHASE,
                Event::PITCHES,
                Event::STATUS,
                Event::HOMOGENEITY
        };

        const char *Event::labels[] = {
                "Time",
                "Duration",
                "Status",
                "Instrument",
                "Key",
                "Velocity",
                "Pan",
                "Depth",
                "Height",
                "Phase",
                "PitchClassSet",
                "Homogeneity"
        };

        bool operator < (const Event &a, const Event &b)
        {
                for(int i = 0; i < Event::ELEMENT_COUNT; i++)
                {
                        if(a[Event::SORT_ORDER[i]] < b[Event::SORT_ORDER[i]])
                        {
                                return true;
                        }
                        else if(a[Event::SORT_ORDER[i]] > b[Event::SORT_ORDER[i]])
                        {
                                return false;
                        }
                }
                return false;
        }

        Event::Event()
        {
                initialize();
        }

        Event::Event(const Event &a)
        {
                resize(a.size());
                *this = a;
        }

        Event::Event(std::string text)
        {
                initialize();
                std::istringstream stream(text);
                clear();
                std::vector<double> buffer;
                double f;
                while(!stream.eof())
                {
                        stream >> f;
                        buffer.push_back(f);
                }
                resize(buffer.size());
                std::copy(buffer.begin(), buffer.end(), begin());
        }

        Event::Event(const ublas::vector<double> &a)
        {
                resize(a.size());
                std::copy(a.begin(), a.end(), begin());
        }

        Event::Event(const std::vector<double> &a)
        {
                resize(a.size());
                std::copy(a.begin(), a.end(), begin());
        }

        Event::Event(double time, double duration, double status, double instrument, double key, double velocity, double phase, double pan, double depth, double height, double pitches)
        {
                initialize();
                set(time, duration, status, instrument, key, velocity, phase, pan, depth, height, pitches);
        }

        Event::~Event()
        {
        }

        void Event::initialize()
        {
                resize(ELEMENT_COUNT);
                operator *= (0);
                (*this)[HOMOGENEITY] = 1.0;
        }

        bool Event::isNoteOn() const
        {
                return (Conversions::round(getStatusNumber()) == MidiFile::CHANNEL_NOTE_ON) &&
                        (getVelocity() > 0.0);
        }

        bool Event::isNoteOff() const
        {
                return (Conversions::round(getStatusNumber()) == MidiFile::CHANNEL_NOTE_OFF) ||
                        ((Conversions::round(getStatusNumber()) == MidiFile::CHANNEL_NOTE_ON) &&
                        (getVelocity() <= 0.0));
        }

        bool Event::isNote() const
        {
                return isNoteOn() || isNoteOff();
        }

        bool Event::isMatchingNoteOff(const Event& event) const
        {
                if(!isNoteOn())
                {
                        return false;
                }
                if(!event.isNoteOff())
                {
                        return false;
                }
                if(Conversions::round((*this)[INSTRUMENT]) != Conversions::round(event[INSTRUMENT]))
                {
                        return false;
                }
                if(Conversions::round((*this)[KEY]) != Conversions::round(event[KEY]))
                {
                        return false;
                }
                return true;
        }

        bool Event::isMatchingEvent(const Event& event) const
        {
                if(Conversions::round((*this)[INSTRUMENT]) != Conversions::round(event[INSTRUMENT]))
                {
                        return false;
                }
                return true;
        }

        void Event::set(double time, double duration, double status, double instrument, double key, double velocity, double phase, double pan, double depth, double height, double pitches)
        {
                (*this)[TIME] = time;
                (*this)[DURATION] = duration;
                (*this)[STATUS] = status;
                (*this)[INSTRUMENT] = instrument;
                (*this)[KEY] = key;
                (*this)[VELOCITY] = velocity;
                (*this)[PHASE] = phase;
                (*this)[PAN] = pan;
                (*this)[DEPTH] = depth;
                (*this)[HEIGHT] = height;
                (*this)[PITCHES] = pitches;
        }

        void Event::setMidi(double time, char status, char key, char velocity)
        {
                (*this)[TIME] = time;
                (*this)[STATUS] = (status & 0xf0);
                (*this)[INSTRUMENT] = (status & 0xf);
                (*this)[DURATION] = INDEFINITE;
                (*this)[KEY] = key;
                (*this)[VELOCITY] = velocity;
        }

        int Event::getMidiStatus() const
        {
                int status = getStatusNumber();
                status |= (getChannel() % 16);
                return status;
        }

        int Event::getStatusNumber() const
        {
                return int(Conversions::round((*this)[STATUS]));
        }

        double Event::getStatus() const
        {
                return (*this)[STATUS];
        }

        int Event::getChannel() const
        {
                return int(Conversions::round((*this)[INSTRUMENT]));
        }

        double Event::getInstrument() const
        {
                return (*this)[INSTRUMENT];
        }

        double Event::getTime() const
        {
                return (*this)[TIME];
        }

        double Event::getDuration() const
        {
                return (*this)[DURATION];
        }

        double Event::getOffTime() const
        {
                if((*this)[DURATION] < 0)
                {
                        return (*this)[TIME] + INDEFINITE;
                }
                else
                {
                        return (*this)[TIME] + (*this)[DURATION];
                }
        }

        int Event::getKeyNumber() const
        {
                return int(Conversions::round((*this)[KEY]));
        }

        double Event::getKey() const
        {
                return (*this)[KEY];
        }

        double Event::getFrequency() const
        {
                return Conversions::midiToHz(getKey());
        }

        int Event::getVelocityNumber() const
        {
                return int(Conversions::round((*this)[VELOCITY]));
        }

        double Event::getVelocity() const
        {
                return (*this)[VELOCITY];
        }

        double Event::getPhase() const
        {
                return (*this)[PHASE];
        }

        double Event::getPan() const
        {
                return (*this)[PAN];
        }

        void Event::dump(std::ostream &stream)
        {
                for(size_t i = 0, n = size(); i < n; ++i)
                {
                        stream << (*this)[i];
                        stream << " ";
                }
                stream << std::endl;
        }

        Event &Event::operator = (const Event &a)
        {
                resize(a.size());
                std::copy(a.begin(), a.end(), begin());
                return *this;
        }

        Event &Event::operator = (const ublas::vector<double> &a)
        {
                resize(a.size());
                std::copy(a.begin(), a.end(), begin());
                return *this;
        }

        double Event::getLeftGain() const
        {
                return Conversions::leftPan(getPan());
        }

        double Event::getRightGain() const
        {
                return Conversions::rightPan(getPan());
        }

        double Event::getGain() const
        {
                return Conversions::midiToGain(getVelocity());
        }

        double Event::getKey(double tonesPerOctave) const
        {
                if(tonesPerOctave == 0.0)
                {
                        return getKey();
                }
                else
                {
                        double tones = Conversions::round(Conversions::midiToOctave(getKey()) * tonesPerOctave);
                        return Conversions::octaveToMidi(tones / tonesPerOctave, false);
                }
        }

        std::string Event::toCsoundIStatement(double tonesPerOctave) const
        {
                char buffer[0x100];
                sprintf(buffer, "i %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g\n",
                        getInstrument(),
                        getTime(),
                        getDuration(),
                        getKey(tonesPerOctave),
                        getVelocity(),
                        getPhase(),
                        getPan(),
                        getDepth(),
                        getHeight(),
                        getPitches(),
                        (*this)[HOMOGENEITY]);
                return buffer;
        }

        std::string Event::toCsoundIStatementHeld(int tag, double tempering) const
        {
                char buffer[0x100];
                double octave = Conversions::midiToOctave(getKey());
                if(tempering != 0.0)
                {
                        octave = Conversions::temper(octave, tempering);
                }
                sprintf(buffer, "i %d.%d %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g\n",
                        (int) Conversions::round(getInstrument()),
                        tag,
                        getTime(),
                        std::fabs(getDuration()) * -1.0,
                        Conversions::octaveToMidi(octave, false),
                        getVelocity(),
                        getPhase(),
                        getPan(),
                        getDepth(),
                        getHeight(),
                        getPitches(),
                        (*this)[HOMOGENEITY]);
                return buffer;
        }

        std::string Event::toCsoundIStatementRelease(int tag, double tempering) const
        {
                char buffer[0x100];
                double octave = Conversions::midiToOctave(getKey());
                if(tempering != 0.0)
                {
                        octave = Conversions::temper(octave, tempering);
                }
                sprintf(buffer, "i -%d.%d %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g\n",
                        (int) Conversions::round(getInstrument()),
                        tag,
                        getTime(),
                        getDuration(),
                        Conversions::octaveToMidi(octave, false),
                        getVelocity(),
                        getPhase(),
                        getPan(),
                        getDepth(),
                        getHeight(),
                        getPitches(),
                        (*this)[HOMOGENEITY]);
                return buffer;
        }

        std::string Event::toString() const
        {
                char buffer[0x100];
                sprintf(buffer, "t%8.3f d%8.3f s%3.0f i%6.2f k%6.2f v%6.2f x%5.2f pcs%8.2f",
                        getTime(),
                        getDuration(),
                        getStatus(),
                        getInstrument(),
                        getKey(),
                        getVelocity(),
                        getPan(),
                        getPitches());
                return buffer;
        }

        void Event::setStatus(double value)
        {
                (*this)[STATUS] = value;
        }

        void Event::setInstrument(double value)
        {
                (*this)[INSTRUMENT] = value;
        }

        void Event::setTime(double value)
        {
                (*this)[TIME] = value;
        }

        void Event::setDuration(double value)
        {
                (*this)[DURATION] = value;
        }

        void Event::setKey(double value)
        {
                (*this)[KEY] = value;
        }

        void Event::setFrequency(double value)
        {
                (*this)[KEY] = Conversions::hzToMidi(value, false);
        }

        void Event::setVelocity(double value)
        {
                (*this)[VELOCITY] = value;
        }

        void Event::setPan(double value)
        {
                (*this)[PAN] = value;
        }

        void Event::setPhase(double value)
        {
                (*this)[PHASE] = value;
        }

        double Event::getAmplitude() const
        {
                return Conversions::midiToAmplitude(getVelocity());
        }

        void Event::setAmplitude(double amplitude)
        {
                setVelocity(Conversions::amplitudeToMidi(amplitude));
        }

        bool Event::isMidiEvent() const
        {
                return (*this)[STATUS] >= MidiFile::CHANNEL_NOTE_OFF;
        }

        void Event::conformToPitchClassSet()
        {
                double midiKey = (*this)[KEY];
                int pitchClass = int(Conversions::midiToPitchClass(midiKey));
                double octave = Conversions::midiToRoundedOctave(midiKey);
                int pitchClassSet = int(Conversions::round((*this)[PITCHES])) % 4096;
                pitchClass = (int) Conversions::findClosestPitchClass(pitchClassSet, pitchClass);
                (*this)[KEY] = Conversions::octaveToMidi(octave, true) + Conversions::pitchClassToMidi(pitchClass);
        }

        void Event::temper(double divisionsPerOctave)
        {
                double octave = Conversions::midiToOctave(getKey());
                octave = Conversions::temper(octave, divisionsPerOctave);
                setKey(Conversions::octaveToMidi(octave, true));
        }

        double Event::getDepth() const
        {
                return (*this)[DEPTH];
        }

        void Event::setDepth(double depth)
        {
                (*this)[DEPTH] = depth;
        }

        double Event::getHeight() const
        {
                return (*this)[HEIGHT];
        }

        void Event::setHeight(double height)
        {
                (*this)[DEPTH] = height;
        }

        double Event::getPitches() const
        {
                return (*this)[PITCHES];
        }

        void Event::setPitches(double pitches)
        {
                (*this)[PITCHES] = pitches;
        }

        std::string Event::getProperty(std::string name)
        {
                if(properties.find(name) != properties.end())
                {
                        return properties[name];
                }
                else
                {
                        return "";
                }
        }

        void Event::setProperty(std::string name, std::string value)
        {
                properties[name] = value;
        }

        void Event::removeProperty(std::string name)
        {
                properties.erase(name);
        }

        void Event::clearProperties()
        {
                properties.clear();
        }

        void Event::createNoteOffEvent(Event &event) const
        {
                event = *this;
                event.setStatus(128);
                event.setTime(event.getTime() + event.getDuration());
                event.setDuration(0.0);
        }
}
