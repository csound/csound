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
#include "CppSound.hpp"
#include "Midifile.hpp"
#include "Score.hpp"
#include "System.hpp"
#include "Conversions.hpp"
#include "Voicelead.hpp"
#include <algorithm>
#include <cfloat>
#include <set>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <sstream>

namespace csound
{

  void printChord(std::ostream &stream, std::string label, const std::vector<double> &chord) 
  {
    if (!( (System::getMessageLevel() & System::INFORMATION_LEVEL) == System::INFORMATION_LEVEL) ) {
      return;
    }
    stream << label.c_str() << "[";
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      if (i > 0) {
	stream << ", ";
      }
      stream << chord[i];
    }
    stream << "]" << std::endl;
  }

  void printChord(std::string label, const std::vector<double> &chord) 
  {
    if (!( (System::getMessageLevel() & System::INFORMATION_LEVEL) == System::INFORMATION_LEVEL) ) {
      return;
    }
    std::stringstream stream;
    printChord(stream, label, chord);
    System::inform(stream.str().c_str());
  }

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

  void Score::setScale(std::vector<Event> &score, 
		       int dimension, bool rescaleMinimum, 
		       bool rescaleRange, 
		       size_t beginAt, 
		       size_t endAt, 
		       double targetMinimum, 
		       double targetRange)
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
    for(std::vector<MidiTrack>::iterator trackI = midiFile.midiTracks.begin(); trackI != midiFile.midiTracks.end(); ++trackI)
      {
        std::set<MidiEvent*> offEvents;
        for(std::vector<MidiEvent>::iterator onEventI = (*trackI).begin(); onEventI != (*trackI).end(); ++onEventI)
          {
            MidiEvent &noteOnEvent = *onEventI;
            if(noteOnEvent.isNoteOn())
              {
                for(std::vector<MidiEvent>::iterator offEventI = onEventI; offEventI != (*trackI).end(); ++offEventI)
                  {
                    MidiEvent &noteOffEvent = *offEventI;
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
                            append(time, duration, status, instrument, key, velocity);
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

  std::string Score::getCsoundScore(double tonesPerOctave, bool conformPitches)
  {
    std::string csoundScore;
    sort();
    for( Score::iterator it = begin(); it != end(); ++it ) {
      int oldInstrument = int( std::floor( it->getInstrument() ) );
      if( gains.find( oldInstrument ) != gains.end() ) {
        double inputDb = it->getVelocity();
        double gain = gains[oldInstrument];
        double outputDb = Conversions::gainToDb( inputDb, gain );
        it->setVelocity( outputDb );
      }
      if( pans.find( oldInstrument ) != pans.end() ) {
        double pan = pans[oldInstrument];
        it->setPan( pan );
      }
      if( reassignments.find( oldInstrument ) != reassignments.end() ) {
        it->setInstrument( reassignments[oldInstrument] );
      }
      if( conformPitches ) {
        it->conformToPitchClassSet();
      }
      csoundScore.append( it->toCsoundIStatement( tonesPerOctave ) );
    }
    return csoundScore;
  }

  void Score::arrange(int oldInstrumentNumber, int newInstrumentNumber)
  {
    reassignments[oldInstrumentNumber] = newInstrumentNumber;
  }

  void Score::arrange(int oldInstrumentNumber, int newInstrumentNumber, double gain)
  {
    reassignments[oldInstrumentNumber] = newInstrumentNumber;
    gains[oldInstrumentNumber] = gain;
  }

  void Score::arrange(int oldInstrumentNumber, int newInstrumentNumber, double gain, double pan)
  {
    reassignments[oldInstrumentNumber] = newInstrumentNumber;
    gains[oldInstrumentNumber] = gain;
    pans[oldInstrumentNumber] = pan;
  }

  void Score::removeArrangement()
  {
    reassignments.clear();
    gains.clear();
    pans.clear();
  }

  std::vector<double> Score::getPitches(size_t begin_, size_t end_, size_t divisionsPerOctave_) const
  {
    System::inform("BEGAN Score::getPitches(%d, %d, %d)\n", begin_, end_, divisionsPerOctave_);
    if (begin_ < 0) {
      begin_ = 0;
    }
    if (end_ > size()) {
      end_ = size();
    }
    std::set<double> pitches;
    std::vector<double> chord;
    for (size_t i = begin_; i < end_; i++) {
      const Event &event = (*this)[i];
      double pitch = event.getKey(divisionsPerOctave_);
      if (pitches.find(pitch) == pitches.end()) {
        pitches.insert(pitch);
        chord.push_back(pitch);
	//System::inform("  i: %d  pitch: %f\n", i, pitch);
      }
    }
    std::sort(chord.begin(), chord.end());
    printChord("  pitches:             ", chord);
    System::inform("ENDED Score::getPitches.\n");
    return chord;
  }

  void Score::setPitches(size_t begin_, size_t end_, const std::vector<double> &pitches)
  {
    if (begin_ < 0) {
      begin_ = 0;
    }
    if (end_ > size()) {
      end_ = size();
    }
    for (size_t i = begin_; i < end_; i++) {
      Event &event = (*this)[i];
      event.setKey(Voicelead::closestPitch(event.getKey(), pitches));
    }
  }

  void Score::setPitchClassSet(size_t begin_, size_t end_, const std::vector<double> &pcs, size_t divisionsPerOctave_)
  {
    if (begin_ < 0) {
      begin_ = 0;
    }
    if (end_ > size()) {
      end_ = size();
    }
    if (begin_ == end_) {
      return;
    }
    for (size_t i = begin_; i < end_; i++) {
      Event &event = (*this)[i];
      event.setKey(Voicelead::conformToPitchClassSet(event.getKey(), pcs, divisionsPerOctave_));
    }
  }

  std::vector<double> Score::getPTV(size_t begin_, 
				    size_t end_, 
				    double lowest, 
				    double range, 
				    size_t divisionsPerOctave_) const
  {
    if (begin_ < 0) {
      begin_ = 0;
    }
    if (end_ > size()) {
      end_ = size();
    }
    std::vector<double> ptv(3);
    std::vector<double> chord = getPitches(begin_, end_, divisionsPerOctave_);
    if (chord.size() == 0) {
      return ptv;
    }
    ptv = Voicelead::chordToPTV(chord, lowest, lowest + range, divisionsPerOctave_);
    return ptv;
  }

  void Score::setPTV(size_t begin_, 
		     size_t end_, 
		     double P, 
		     double T, 
		     double V, 
		     double lowest, 
		     double range, 
		     size_t divisionsPerOctave_)
  {
    if (begin_ < 0) {
      begin_ = 0;
    }
    if (end_ > size()) {
      end_ = size();
    }
    if (begin_ == end_) {
      return;
    }
    System::inform("BEGAN Score::setPTV(%d, %d, %f, %f, %f, %f, %f, %d)...\n", begin_, end_, P, T, V, lowest, range, divisionsPerOctave_);
    std::vector<double> voicing = Voicelead::ptvToChord(P, T, V, lowest, lowest + range, divisionsPerOctave_);
    setPitches(begin_, end_, voicing);
    std::vector<double> pcs = Voicelead::uniquePcs(voicing, divisionsPerOctave_);
    printChord("pcs of voicing: ", pcs);
    System::inform("ENDED Score::setPTV.\n");
  }

  std::vector<double> Score::getPT(size_t begin_, 
				   size_t end_, 
				   double lowest, 
				   double range, 
				   size_t divisionsPerOctave_) const
  {
    if (begin_ < 0) {
      begin_ = 0;
    }
    if (end_ > size()) {
      end_ = size();
    }
    std::vector<double> pt(2);
    std::vector<double> chord = getPitches(begin_, end_, divisionsPerOctave_);
    if (chord.size() == 0) {
      return pt;
    }
    std::vector<double> pitchClassSet = Voicelead::uniquePcs(chord, divisionsPerOctave_);
    pt = Voicelead::pitchClassSetToPandT(pitchClassSet, divisionsPerOctave_);
    return pt;
  }

  void Score::setPT(size_t begin_, 
		    size_t end_, 
		    double P, 
		    double T, 
		    double lowest, 
		    double range, 
		    size_t divisionsPerOctave_)
  {
    if (begin_ < 0) {
      begin_ = 0;
    }
    if (end_ > size()) {
      end_ = size();
    }
    if (begin_ == end_) {
      return;
    }
    System::inform("BEGAN Score::setPT(%d, %d, %f, %f, %f, %f, %d)...\n", begin_, end_, P, T, lowest, range, divisionsPerOctave_);
    std::vector<double> pitchClassSet = Voicelead::pAndTtoPitchClassSet(P, T, divisionsPerOctave_);
    printChord("  pitch-class set:     ", pitchClassSet);
    setPitchClassSet(begin_, end_, pitchClassSet, divisionsPerOctave_);
    std::vector<double> result = getPitches(begin_, end_, divisionsPerOctave_);
    printChord("  result:              ", result);
    std::vector<double> resultTones = Voicelead::uniquePcs(result, divisionsPerOctave_);
    printChord("  as pitch-class set:  ", resultTones);  
    System::inform("ENDED Score::setPT.\n");
  }

  std::vector<double> Score::getVoicing(size_t begin_, 
					size_t end_, 
					size_t divisionsPerOctave_) const
  {
    System::inform("BEGAN Score::getVoicing(%d, %d, %d)...\n", begin_, end_, divisionsPerOctave_);
    std::vector<double> pitches = getPitches(begin_, end_, divisionsPerOctave_);
    std::set<double> pcs;
    std::vector<double> voicing;
    for (size_t i = 0, n = pitches.size(); i < n; i++) {
      double pitch = pitches[i];
      double pc = Voicelead::pc(pitch, divisionsPerOctave_);
      if (pcs.find(pc) == pcs.end()) {
	pcs.insert(pc);
	voicing.push_back(pitch);
      }
    }
    std::sort(voicing.begin(), voicing.end());
    printChord("  voicing:             ", voicing);
    std::vector<double> resultTones = Voicelead::uniquePcs(voicing, divisionsPerOctave_);
    printChord("  as pitch-class set:  ", resultTones);  
    System::inform("ENDED Score::getVoicing.\n");
    return voicing;
  }

  void Score::setVoicing(size_t begin_, 
			 size_t end_, 
			 const std::vector<double> &voicing, 
			 double range, 
			 size_t divisionsPerOctave_)
  {
    if (begin_ < 0) {
      begin_ = 0;
    }
    if (end_ > size()) {
      end_ = size();
    }
    if (begin_ == end_) {
      return;
    }
    std::map<double, double> pitchesForPitchClassSets;
    for (size_t i = 0, n = voicing.size(); i < n; i++) {
      double pitch = voicing[i];
      double pc = Voicelead::pc(pitch, divisionsPerOctave_);
      pitchesForPitchClassSets[pc] = pitch;
    }
    std::vector<double> pcs = Voicelead::pcs(voicing, divisionsPerOctave_);
    for (size_t i = begin_; i < end_; i++) {
      Event &event = (*this)[i];
      double pitch = Voicelead::conformToPitchClassSet(event.getKey(), pcs, divisionsPerOctave_);
      double pc = Voicelead::pc(pitch);
      double voicedPitch = pitchesForPitchClassSets[pc];
      if (pitch < voicedPitch) {
	pitch += double(divisionsPerOctave_);
      }
      event.setKey(pitch);
    }
   }

  void Score::voicelead(size_t beginSource,
			size_t endSource,
			size_t beginTarget,
			size_t endTarget,
			double lowest,
			double range,
			bool avoidParallelFifths,
			size_t divisionsPerOctave_)
  {
    if ( (System::getMessageLevel() & System::INFORMATION_LEVEL) == System::INFORMATION_LEVEL) {
      std::stringstream stream;
      stream << "BEGAN Score::voicelead:..." << std::endl;
      stream << "  beginSource:         " << beginSource << std::endl;
      stream << "  endSource:           " << endSource << std::endl;
      stream << "  beginTarget:         " << beginTarget << std::endl;
      stream << "  endTarget:           " << endTarget << std::endl;
      stream << "  lowest:              " << lowest << std::endl;
      stream << "  range:               " << range << std::endl;
      stream << "  avoidParallelFifths: " << avoidParallelFifths << std::endl;
      stream << "  divisionsPerOctave:  " << divisionsPerOctave_ << std::endl;
      stream << std::endl;
      stream.flush();
      System::inform(stream.str().c_str());
    }
    if (beginSource < 0) {
      beginSource = 0;
    }
    if (endSource > size()) {
      endSource = size();
    }
    if (beginSource == endSource) {
      return;
    }
    if (beginTarget < 0) {
      beginTarget = 0;
    }
    if (endTarget > size()) {
      endTarget = size();
    }
    if (beginTarget == endTarget) {
      return;
    }
    if ((beginSource == beginTarget) && (endSource == endTarget)) {
      System::inform("First segment, returning without doing anything.\n");
      return;
    }
    std::vector<double> source = getVoicing(beginSource, endSource, divisionsPerOctave_);
    printChord("  source voicing:      ", source);
    if (source.size() == 0) {
      return;
    }
    std::vector<double> target = getVoicing(beginTarget, endTarget, divisionsPerOctave_);
    if (target.size() == 0) {
      return;
    }
    printChord("  target voicing:      ", target);
    std::vector<double> tones = Voicelead::pcs(target, divisionsPerOctave_);
    printChord("  target voicing tones:", tones);
    // Double voices in the source if necessary.
    if (tones.size() > source.size()) {
      size_t k = source.size();
      size_t n = tones.size() - k;
      for (size_t i = 0, j = 0; i < n; i++, j++) {
        if (j >= k) {
	  j = 0;
	}
        source.push_back(source[j]);
      }
    }
    printChord("  source doubled:      ", source);
    // Double voices in the target if necessary.
    if (source.size() > tones.size()) {
      size_t k = tones.size();
      size_t n = source.size() - k;
      for (size_t i = 0, j = 0; i < n; i++, j++) {
        if (j >= k) {
	  j = 0;
	}
        tones.push_back(tones[j]);
      }
    }
    printChord("  tones doubled:       ", tones);
    //std::vector<double> voicing = Voicelead::recursiveVoicelead(source, tones, lowest, range, avoidParallelFifths, divisionsPerOctave_);
    std::vector< std::vector<double> > result3 = Voicelead::nonBijectiveVoicelead(source, tones, divisionsPerOctave_);
    const std::vector<double> voicing = result3[2];
    printChord("  final target voicing:", voicing);
    //setVoicing(beginTarget, endTarget, voicing, range, divisionsPerOctave_);
    setPitches(beginTarget, endTarget, voicing);
    std::vector<double> result = getPitches(beginTarget, endTarget, divisionsPerOctave_);
    printChord("  result:              ", result);
    std::vector<double> resultTones = Voicelead::uniquePcs(result, divisionsPerOctave_);
    printChord("  as pitch-class set:  ", resultTones);  
    System::inform("ENDED Score::voicelead.\n");
  }

  void Score::voicelead(size_t beginSource,
			size_t endSource,
			size_t beginTarget,
			size_t endTarget,
			const std::vector<double> &target,
			double lowest,
			double range,
			bool avoidParallelFifths,
			size_t divisionsPerOctave_)
  {
    if ( (System::getMessageLevel() & System::INFORMATION_LEVEL) == System::INFORMATION_LEVEL ) {
      std::stringstream stream;
      stream << "BEGAN Score::voicelead:..." << std::endl;
      stream << "  beginSource:         " << beginSource << std::endl;
      stream << "  endSource:           " << endSource << std::endl;
      stream << "  beginTarget:         " << beginTarget << std::endl;
      stream << "  endTarget:           " << endTarget << std::endl;
      printChord(stream, "  target:              ", target);
      stream << "  lowest:              " << lowest << std::endl;
      stream << "  range:               " << range << std::endl;
      stream << "  avoidParallelFifths: " << avoidParallelFifths << std::endl;
      stream << "  divisionsPerOctave:  " << divisionsPerOctave_ << std::endl;
      stream << std::endl;
      stream.flush();
      System::inform(stream.str().c_str());
    }
    if (beginSource < 0) {
      beginSource = 0;
    }
    if (endSource > size()) {
      endSource = size();
    }
    if (beginSource == endSource) {
      return;
    }
    if (beginTarget < 0) {
      beginTarget = 0;
    }
    if (endTarget > size()) {
      endTarget = size();
    }
    if (beginTarget == endTarget) {
      return;
    }
    if ((beginSource == beginTarget) && (endSource == endTarget)) {
      setPitchClassSet(beginTarget, endTarget, target, divisionsPerOctave_);
      std::vector<double> result = getPitches(beginTarget, endTarget, divisionsPerOctave_);
      printChord("  result:              ", result);
      std::vector<double> resultTones = Voicelead::uniquePcs(result, divisionsPerOctave_);
      printChord("  as pitch-class set:  ", resultTones);  
      return;
    }
    std::vector<double> source = getVoicing(beginSource, endSource, divisionsPerOctave_);
    printChord("  source voicing:      ", source);
     if (source.size() == 0) {
      return;
    }
    if (target.size() == 0) {
      return;
    }
    std::vector<double> tones = Voicelead::pcs(target, divisionsPerOctave_);
    printChord("  target tones:        ", target);
    // Double voices in the source if necessary.
    if (tones.size() > source.size()) {
      size_t k = source.size();
      size_t n = tones.size() - k;
      for (size_t i = 0, j = 0; i < n; i++, j++) {
        if (j >= k) {
	  j = 0;
	}
        source.push_back(source[j]);
      }
      printChord("  doubled source:      ", source);
    }
    // Double voices in the target if necessary.
    if (source.size() > tones.size()) {
      size_t k = tones.size();
      size_t n = source.size() - k;
      for (size_t i = 0, j = 0; i < n; i++, j++) {
        if (j >= k) {
	  j = 0;
	}
        tones.push_back(tones[j]);
      }
      std::sort(tones.begin(), tones.end());
      printChord("  doubled tones:       ", tones);
    }
    //std::vector<double> voicing = Voicelead::recursiveVoicelead(source, tones, lowest, range, avoidParallelFifths, divisionsPerOctave_);
    std::vector< std::vector<double> > result3 = Voicelead::nonBijectiveVoicelead(source, tones, divisionsPerOctave_);
    const std::vector<double> voicing = result3[2];
    printChord("  target voicing:      ", voicing);
    //setVoicing(beginTarget, endTarget, voicing, range, divisionsPerOctave_);
    setPitches(beginTarget, endTarget, voicing);
    std::vector<double> result = getPitches(beginTarget, endTarget, divisionsPerOctave_);
    printChord("  result:              ", result);
    std::vector<double> resultTones = Voicelead::uniquePcs(result, divisionsPerOctave_);
    printChord("  as pitch-class set:  ", resultTones);  
    System::inform("ENDED Score::voicelead.\n");
  }

  struct TimeAtComparator
  {
    double time;
    TimeAtComparator(double time_) : time(time_)
    {
    }
    bool operator()(const Event &event)
    {
      if (event.getTime() >= time) {
        return true;
      } else {
        return false;
      }
    }
  };

  int Score::indexAtTime(double time)
  {
    int index = size();
    std::vector<Event>::iterator it = std::find_if(begin(), end(), TimeAtComparator(time));
    if (it != end()) {
      index = (it - begin());
    }
    return index;
  }

  struct TimeAfterComparator
  {
    double time;
    TimeAfterComparator(double time_) : time(time_)
    {
    }
    bool operator()(const Event &event)
    {
      if (event.getTime() > time) {
        return true;
      } else {
        return false;
      }
    }
  };

  int Score::indexAfterTime(double time)
  {
    int index = size();
    std::vector<Event>::iterator it = std::find_if(begin(), end(), TimeAfterComparator(time));
    if (it != end()) {
      index = (it - begin());
    }
    return index;
  }

  double Score::indexToTime(size_t index)
  {
    double time = DBL_MAX;
    if (index >= 0 && index < size()) {
      time = (*this)[index].getTime();
    }
    return time;
  }

  void Score::setDuration(double targetDuration)
  {
    double currentDuration = getDuration();
    if (currentDuration == 0.0) {
      return;
    }
    double factor = targetDuration / currentDuration;
    for (size_t i = 0, n = size(); i < n; i++) {
      Event &event = (*this)[i];
      double time = event.getTime();
      double duration = event.getDuration();
      event.setTime(time * factor);
      event.setDuration(duration * factor);
    }
  }
}
