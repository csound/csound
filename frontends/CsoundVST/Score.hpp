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
#ifndef SCORE_H
#define SCORE_H
#ifdef SWIG
%module CsoundVST
%{
#include "Event.hpp"
#include "Midifile.hpp"
#include <iostream>
#include <vector>
  %}
%include "std_string.i"
%include "std_vector.i"
#else
#include "Event.hpp"
#include "Midifile.hpp"
#include <iostream>
#include <vector>
#endif

namespace csound 
{
  /**
   * Base class for collections of events in music space.
   * Can order events by time.
   * <p>
   * The implementation is a std::vector of Events.
   * The elements of the vector are value objects, not references.
   */
  class Score : 
    public std::vector<csound::Event>
  {
  protected:
    void createMusicModel();
  public:
    Event scaleTargetMinima;
    std::vector<bool> rescaleMinima;
    Event scaleTargetRanges;
    std::vector<bool> rescaleRanges;
    Event scaleActualMinima;
    Event scaleActualRanges;
    MidiFile midifile;
    Score();
    virtual ~Score();
    virtual void initialize();
    virtual void append(Event event);
    virtual void append(double time, double duration, double status, double channel, double key, double velocity, double phase=0, double pan=0, double depth=0, double height=0, double pitches=4095);
    virtual void load(std::string filename);
    virtual void load(std::istream &stream);
    virtual void load(MidiFile &midiFile);
    virtual void save(std::string filename);
    virtual void save(std::ostream &stream);
    virtual void save(MidiFile &midiFile);
    static void getScale(std::vector<Event> &score, int dimension, size_t beginAt, size_t endAt, double &minimum, double &range);
    static void setScale(std::vector<Event> &score, int dimension, bool rescaleMinimum, bool rescaleRange, size_t beginAt, size_t endAt, double targetMinimum, double targetRange);
    virtual void findScale();
    virtual void rescale();
    virtual void rescale(Event &event);
    /**
     * Sort all events in the score by time, instrument number, pitch, duration, loudness, 
     * and other dimensions as given by Event::SORT_ORDER.
     */
    virtual void sort();
    virtual void dump(std::ostream &stream);
    virtual std::string toString();
    virtual double getDuration();
    virtual void rescale(int dimension, bool rescaleMinimum, double minimum, bool rescaleRange = false, double range = 0.0);
    /**
     * Translate the Silence events in this to a Csound score, that is, to a list of i statements.
     * The Silence events are rounded off to the nearest equally tempered pitch by the 
     * specified number of tones per octave; if this argument is zero, the pitch is not tempered.
     * The Silence events are conformed to the nearest pitch-class set in the pitch-class set
     * dimension of the event, if the conform pitches argument is true; otherwise, the pitches
     * are not conformed.
     */
    virtual std::string getCsoundScore(double tonesPerOctave = 12.0, bool conformPitches = false);
  };
}	
#endif
