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
#ifndef EVENT_H
#define EVENT_H
#ifdef SWIG
%module CsoundVST
%{
#include "Conversions.hpp"
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
  %}
%include "std_string.i"
%include "std_map.i"
%template(StringMap) std::map<std::string,std::string>;
#ifdef SWIGPYTHON
%template(StringStringPair) std::pair<std::string,std::string>;
#endif
%template(EventVector) std::vector<csound::Event>;
#else
#include "Conversions.hpp"
#include "Midifile.hpp"
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <boost/numeric/ublas/vector.hpp>
using namespace boost::numeric;
#endif

namespace csound 
{
  /**
   * Represents an event in music space, such as a note of definite duration,
   * a MIDI-like "note on" or "note off" event, or a MIDI-like control event.
   * Fields have the same semantics as MIDI with some differences.
   * All fields are floats; status is stored separately from channel; 
   * channel can have any positive value; spatial location in X, Y, and Z are stored;
   * phase in radians is stored; and pitch-class set is stored.
   * <p>
   * Events can be multiplied (matrix dot product) with the local coordinate system
   * of a Node or transform to translate, scale, or rotate them in any or all dimensions
   * of music space.
   * <p>
   * Events usually are value objects, not references.
   * <p>
   * Silence Events translate to Csound score statements ("i" statements),
   * but they are always real-time score statements at time 0, suitable
   * for use with Csound's -L or line event option.
   */
  class Event : 
    public ublas::vector<double> 
  {
  public:
    typedef enum 
      {
	TIME = 0,
	DURATION,
	STATUS,
	INSTRUMENT,
	KEY,
	VELOCITY,
	PHASE,
	PAN,
	DEPTH,
	HEIGHT,
	PITCHES,
	HOMOGENEITY,
	ELEMENT_COUNT,
      } Dimensions;
    enum
      {
	INDEFINITE = 16384,
      };
    static int SORT_ORDER[];
    static const char *labels[];
    std::map<std::string,std::string> properties;
    Event();
    Event(const Event &a);
    Event(std::string text);
    Event(const ublas::vector<double, ublas::unbounded_array<double> > &a);
    Event(double time, double duration, double status, double instrument, double key, double velocity, double phase, double pan, double depth, double height, double pitches);
    Event(const std::vector<double> &v);
    virtual ~Event();
    void initialize();
    bool isMidiEvent() const;
    bool isNoteOn() const;
    bool isNoteOff() const;
    bool isNote() const;
    bool isMatchingNoteOff(const Event& event) const;
    bool isMatchingEvent(const Event& event) const;
    void set(double time, double duration, double status, double instrument, double key, double velocity, double phase=0, double pan=0, double depth=0, double height=0, double pitches=4095);
    void setMidi(double time, char status, char key, char velocity);
    int getMidiStatus() const;
    int getStatusNumber() const;
    double getStatus() const;
    void setStatus(double status);
    int getChannel() const;
    double getInstrument() const;
    void setInstrument(double instrument);
    double getTime() const;
    void setTime(double time);
    double getDuration() const;
    void setDuration(double duration);
    double getOffTime() const;
    int getKeyNumber() const;
    double getKey() const;
    double getKey(double tonesPerOctave) const;
    void setKey(double key);
    double getFrequency() const;
    void setFrequency(double frequency);
    int getVelocityNumber() const;
    double getVelocity() const;
    void setVelocity(double velocity);
    double getGain() const;
    double getPan() const;
    void setPan(double pan);
    double getDepth() const;
    void setDepth(double depth);
    double getHeight() const;
    void setHeight(double height);
    double getPitches() const;
    void setPitches(double pitches);
    double getAmplitude() const;
    void setAmplitude(double amplitude);
    double getPhase() const;
    void setPhase(double phase);
    double getLeftGain() const;
    double getRightGain() const;
    virtual void dump(std::ostream &stream);
    virtual std::string toString() const;
    virtual std::string toCsoundIStatement(double tempering = 12.0) const;
    virtual std::string toCsoundIStatementHeld(int tag, double tempering = 12.0) const;
    virtual std::string toCsoundIStatementRelease(int tag, double tempering = 12.0) const;
    virtual void conformToPitchClassSet();
    virtual void temper(double divisionsPerOctave);
    virtual std::string getProperty(std::string name);
    virtual void setProperty(std::string name, std::string value);
    virtual void removeProperty(std::string nameO);
    virtual void clearProperties();
    virtual void createNoteOffEvent(Event &event) const;
    Event &operator = (const Event &a);
    Event &operator = (const ublas::vector<double> &a);
  };

  bool operator < (const Event& a, const Event &b);
}
#endif
