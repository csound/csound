/*
 * C S O U N D
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
#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include "Conversions.hpp"
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <utility>
#include <boost/numeric/ublas/vector.hpp>
  %}
%include "std_string.i"
%include "std_vector.i"
%template(EventVector) std::vector<csound::Event>;
#else
#include "Conversions.hpp"
#include "Midifile.hpp"
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <utility>
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
  class SILENCE_PUBLIC Event :
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
        ELEMENT_COUNT
      } Dimensions;
    enum
      {
        INDEFINITE = 16384
      };
    std::map<std::string,std::string> properties;
    Event();
    Event(const Event &a);
    Event(std::string text);
    Event(const ublas::vector<double, ublas::unbounded_array<double> > &a);
    Event(double time, double duration, double status, double instrument, double key, double velocity, double phase, double pan, double depth, double height, double pitches);
    Event(const std::vector<double> &v);
    virtual ~Event();
    virtual void initialize();
    virtual bool isMidiEvent() const;
    virtual bool isNoteOn() const;
    virtual bool isNoteOff() const;
    virtual bool isNote() const;
    virtual bool isMatchingNoteOff(const Event& event) const;
    virtual bool isMatchingEvent(const Event& event) const;
    virtual void set(double time, double duration, double status, double instrument, double key, double velocity, double phase=0, double pan=0, double depth=0, double height=0, double pitches=4095);
    virtual void setMidi(double time, char status, char key, char velocity);
    virtual int getMidiStatus() const;
    virtual int getStatusNumber() const;
    virtual double getStatus() const;
    virtual void setStatus(double status);
    virtual int getChannel() const;
    virtual double getInstrument() const;
    virtual void setInstrument(double instrument);
    virtual double getTime() const;
    virtual void setTime(double time);
    virtual double getDuration() const;
    virtual void setDuration(double duration);
    virtual double getOffTime() const;
    virtual void setOffTime(double offTime);
    virtual int getKeyNumber() const;
    virtual double getKey() const;
    virtual double getKey(double tonesPerOctave) const;
    virtual void setKey(double key);
    virtual double getFrequency() const;
    virtual void setFrequency(double frequency);
    virtual int getVelocityNumber() const;
    virtual double getVelocity() const;
    virtual void setVelocity(double velocity);
    virtual double getGain() const;
    virtual double getPan() const;
    virtual void setPan(double pan);
    virtual double getDepth() const;
    virtual void setDepth(double depth);
    virtual double getHeight() const;
    virtual void setHeight(double height);
    virtual double getPitches() const;
    virtual void setPitches(double pitches);
    virtual double getAmplitude() const;
    virtual void setAmplitude(double amplitude);
    virtual double getPhase() const;
    virtual void setPhase(double phase);
    virtual double getLeftGain() const;
    virtual double getRightGain() const;
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
    virtual Event &operator = (const Event &a);
    virtual Event &operator = (const ublas::vector<double> &a);
#ifndef SWIG
    static int SORT_ORDER[];
    static const char *labels[];
#endif
  };

  bool SILENCE_PUBLIC operator < (const Event& a, const Event &b);
}
#endif
