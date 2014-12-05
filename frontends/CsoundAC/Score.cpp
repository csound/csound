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
#include "CppSound.hpp"
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

#include "allegro.h"

#if defined(HAVE_MUSICXML2)
#if defined(EXP)
#undef EXP
#endif
#include "elements.h"
#include "factory.h"
#include "xml.h"
#include "xmlfile.h"
#include "xml_tree_browser.h"
#include "xmlreader.h"
#include "midicontextvisitor.h"

using namespace MusicXML2;
#endif

namespace csound
{

void SILENCE_PUBLIC printChord(std::ostream &stream, std::string label, const std::vector<double> &chord)
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

void SILENCE_PUBLIC printChord(std::string label, const std::vector<double> &chord)
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

#if defined(HAVE_MUSICXML2)

class ScoreMidiWriter : public midiwriter
{
public:
    long tpq;
    double tempo;
    ScoreMidiWriter(Score *score_) : tpq(1000), tempo(1.0), score(*score_)  {
        score.clear();
        score.removeArrangement();
    }
    virtual void startPart (int instrCount) {
    }
    virtual void newInstrument (std::string instrName, int chan=-1) {
    }
    virtual void endPart (long date) {
    }
    virtual void newNote (long start_, int channel_, float key_, int velocity_, int duration_) {
        double start = double(start_) / double(tpq) * tempo;
        double duration = double(duration_) / double(tpq) * tempo;
        double status = 144.0;
        double channel = double(channel_);
        double key = key_;
        double velocity = velocity_;
        score.append(start, duration, status, channel, key, velocity);
    }
    virtual void tempoChange (long date, int bpm) {
        tempo = 60.0 / double(bpm);
    }
    virtual void pedalChange (long date, pedalType t, int value) {
    }
    virtual void volChange (long date, int chan, int vol) {
    }
    virtual void bankChange (long date, int chan, int bank) {
    }
    virtual void progChange (long date, int chan, int prog) {
    }
protected:
    Score &score;
};

#endif

void Score::load(std::string filename)
{
    System::inform("BEGAN Score::load(%s)...\n", filename.c_str());
    if (filename.find(".mid") != std::string::npos ||
            filename.find(".MID") != std::string::npos) {
        std::ifstream stream;
        stream.open(filename.c_str(), std::ifstream::binary);
        load(stream);
        stream.close();
    }
#if defined(HAVE_MUSICXML2)
    else if (filename.find(".xml") != std::string::npos ||
            filename.find(".XML") != std::string::npos) {
        xmlreader xmlReader;
        Sxmlelement sxmlElement;
        // Try to read an SXMLFile out of the MusicXML file.
        SXMLFile sxmlFile = xmlReader.read(filename.c_str());
        if (sxmlFile) {
            // Get the document tree of XML elements from the SXMLFile.
            sxmlElement = sxmlFile->elements();
        }
        if (sxmlElement) {
            // Create a ScoreMidiWriter that is attached to this Score.
            ScoreMidiWriter scoreMidiWriter(this);
            // Create a midicontextvisitor, which calls into an abstract midiwriter interface,
            // which is attached to our ScoreMidiWriter, which implements that midiwriter interface.
            midicontextvisitor midicontextvisitor_(scoreMidiWriter.tpq, &scoreMidiWriter);
            // Create an xml_tree_browser that is attached to our midicontextvisitor.
            xml_tree_browser xmlTreeBrowser(&midicontextvisitor_);
            // The xml_tree_browser will carry the midicontextvisitor to all the elements
            // of the document tree, in the proper order, calling newNote as appropriate.
            xmlTreeBrowser.browse(*sxmlElement);
        }
    }
#endif
    else {
        System::error("Unknown file format in Score::load().\n");
    }
    System::inform("ENDED Score::load().\n");
}

void Score::load(std::istream &stream)
{
    Alg_seq seq(stream, true);
    seq.convert_to_seconds();
    Alg_iterator iterator(&seq, false);
    iterator.begin();
    for(;;) {
        Alg_event *event = iterator.next();
        if (!event) {
            break;
        }
        append(event->get_start_time(),
               event->get_duration(),
               double(144),
               double(event->chan),
               event->get_pitch(),
               event->get_loud());
    }
    // MidiFile midiFile;
    // midiFile.read(stream);
    // load(midiFile);
}

#if defined(HAVE_MUSICXML2)
static Sxmlattribute newAttribute(const string& name, const string& value)
{
    Sxmlattribute attribute = xmlattribute::create();
    attribute->setName(name);
    attribute->setValue(value);
    return attribute;
}

static Sxmlattribute newAttributeI(const string& name, int value)
{
    Sxmlattribute attribute = xmlattribute::create();
    attribute->setName(name);
    attribute->setValue(value);
    return attribute;
}

static Sxmlelement newElement(int type, const string& value)
{
    Sxmlelement elt = factory::instance().create(type);
    elt->setValue (value);
    return elt;
}

static Sxmlelement newElementI(int type, int value)
{
    Sxmlelement elt = factory::instance().create(type);
    elt->setValue (value);
    return elt;
}

static Sxmlelement makeAttributes()
{
    Sxmlelement attributes = factory::instance().create(k_attributes);
    attributes->push (newElementI(k_divisions, 1000));
    Sxmlelement time_ = factory::instance().create(k_time);
    time_->push (newElement(k_beats, "4"));
    time_->push (newElement(k_beat_type, "4"));
    attributes->push (time_);
    Sxmlelement clef = factory::instance().create(k_clef);
    clef->push (newElement(k_sign, "G"));
    clef->push (newElement(k_line, "2"));
    attributes->push (clef);
    return attributes;
}

static std::string makePartId(int partid)
{
    return "";
}

static Sxmlelement makePart(int instrument, const std::vector<const Event *> &part_)
{
    Sxmlelement part = factory::instance().create(k_part);
    char buffer[0x100];
    std::sprintf(buffer, "Instrument %d", instrument);
    part->add(newAttribute("id", buffer));
    size_t measure = 0;
    size_t time_ = 0;
    size_t divisionsPerMeasure = 4 * 32;
    // We have to quantize time in divisions of a quarter note.
    // We assume that 32nd notes are good enough.
    // We have to keep track of what measure we are in, and create new measures as required.
    // We have to translate pitches to diatonic names, and get the accidental.
    // And we have to back up to make chords.
    return part;
}

static Sxmlelement makePartList(std::map<int, std::vector<const Event *> > parts)
{
    Sxmlelement partlist = factory::instance().create(k_part_list);
    for (std::map<int, std::vector<const Event *> >::iterator it = parts.begin(); it != parts.end(); ++it) {
        Sxmlelement scorepart = factory::instance().create(k_score_part);
        char partid[0x100];
        std::sprintf(partid, "Instrument %d", it->first);
        scorepart->add(newAttribute("id", partid));
        scorepart->push(newElement(k_part_name, partid));
        Sxmlelement scoreinstrument = factory::instance().create(k_score_instrument);
        scoreinstrument->add(newAttribute("id", partid));
        scoreinstrument->push(newElement(k_instrument_name, partid));
        scorepart->push(scoreinstrument);
        partlist->push(scorepart);
    }
    return partlist;
}

static Sxmlelement makeIdentification(const Score &score)
{
    Sxmlelement id = factory::instance().create(k_identification);
    Sxmlelement encoding = factory::instance().create(k_encoding);
    encoding->push (newElement(k_software, "MusicXML Library v2"));
    id->push (encoding);
    return id;
}

static Sxmlelement createScore(const Score &score_, std::string filename)
{
    Sxmlelement score = factory::instance().create(k_score_partwise);
    score->push (newElement(k_movement_title, filename.c_str()));
    score->push (makeIdentification(score_));
    // Break up this Score into parts, each for one whole instrument number.
    std::map<int, std::vector<const Event *> > parts;
    for (size_t i = 0, n = score_.size(); i < n; ++i) {
        const Event &event = score_[i];
        int instrument = event.getChannel();
        parts[instrument].push_back(&event);
    }
    // First we have to make our part list.
    score->push(makePartList(parts));
    // Now add each Score part to the corresponding MusicXML part.
    for (std::map<int, std::vector<const Event *> >::iterator it = parts.begin(); it != parts.end(); ++it) {
        score->push(makePart(it->first, it->second));
    }
    return score;
}
#endif

void Score::save(std::string filename)
{
    System::inform("BEGAN Score::save(%s)...\n", filename.c_str());
    std::fstream stream;
    stream.open(filename.c_str(), std::ios_base::out | std::ios_base::binary);
    if (filename.find(".mid") != std::string::npos ||
            filename.find(".MID") != std::string::npos) {
        save(stream);
        System::inform("ENDED Score::save().\n");
    }
#if defined(HAVE_MUSICXML2)
    else if (filename.find(".xml") != std::string::npos ||
            filename.find(".XML") != std::string::npos) {
        // This Score has to be sorted first.
        sort();
        // Create an XMLFile to write to.
        SXMLFile xmlFile = TXMLFile::create();
        // Add an XML declaration to the XMLFile.
        TXMLDecl *xmlDeclaration = new TXMLDecl("1.0", "", TXMLDecl::kNo);
        xmlFile->set(xmlDeclaration);
        // Add a document type declaration.
        TDocType *documentTypeDeclaration = new TDocType("score-partwise");
        xmlFile->set(documentTypeDeclaration);
        // Create a MusicXML2 document in which first one part is written,
        // then the next part, and so on.
        xmlFile->set(createScore(*this, filename));
        // Print the XML document to the output stream.
        xmlFile->print(stream);
    }
#endif
    else {
        System::error("Unknown file format in Score::save().\n");
    }
    stream.close();
}

void Score::save(std::ostream &stream)
{
    Alg_seq seq;
    for (size_t i = 0, n = size(); i < n; ++i) {
        const Event &event = at(i);
        if (event.isNoteOn()) {
            int channel = event.getChannel();
            double time = event.getTime();
            double duration = event.getDuration();
            float pitch = event.getKey();
            float loudness = event.getVelocity();
            Alg_note *note = seq.create_note(time,
                    channel,
                    pitch,
                    pitch,
                    loudness,
                    duration);
            // Does nothing if the track already exists.
            seq.add_track(channel);
            seq.add_event(note, channel);
        }
    }
    // Write with time in seconds.
    seq.write(std::cout, true);
    seq.smf_write(stream);
    // save(midifile);
    // midifile.write(stream);
}

static double max(double a, double b)
{
    if(a > b) {
        return a;
    }
    return b;
}

static double min(double a, double b)
{
    if(a < b) {
        return a;
    }
    return b;
}

void Score::getScale(std::vector<Event> &score, int dimension, size_t beginAt, size_t endAt, double &minimum, double &range)
{
    if(beginAt == endAt) {
        return;
    }
    const Event &beginEvent = score[beginAt];
    double maximum = beginEvent[dimension];
    const Event &endEvent = score[endAt - 1];
    minimum = endEvent[dimension];
    if(dimension == Event::TIME) {
        const Event &e = score[beginAt];
        maximum = max(e.getTime(), e.getTime() + e.getDuration());
        minimum = min(e.getTime(), e.getTime() + e.getDuration());
        double beginning;
        double ending;
        for( ; beginAt != endAt; ++beginAt) {
            const Event &event = score[beginAt];
            beginning = min(event.getTime(), event.getTime() + event.getDuration());
            ending = max(event.getTime(), event.getTime() + event.getDuration());
            if(ending > maximum) {
                maximum = ending;
            } else if(beginning < minimum) {
                minimum = beginning;
            }
        }
    } else {
        for( ; beginAt != endAt; ++beginAt) {
            const Event &event = score[beginAt];
            if(event[dimension] > maximum) {
                maximum = event[dimension];
            }
            if(event[dimension] < minimum) {
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
    if(!(rescaleMinimum || rescaleRange)) {
        return;
    }
    if(beginAt == endAt) {
        return;
    }
    double actualMinimum;
    double actualRange;
    getScale(score, dimension, beginAt, endAt, actualMinimum, actualRange);
    double scale;
    if(actualRange == 0.0) {
        scale = 1.0;
    } else {
        scale = targetRange / actualRange;
    }
    for( ; beginAt != endAt; ++beginAt) {
        Event &event = score[beginAt];
        event[dimension] = event[dimension] - actualMinimum;
        if(rescaleRange) {
            event[dimension] = event[dimension] * scale;
        }
        if(rescaleMinimum) {
            event[dimension] = event[dimension] + targetMinimum;
        } else {
            event[dimension] = event[dimension] + actualMinimum;
        }
    }
}

void Score::findScale(void)
{
    for(int dimension = 0; dimension < Event::ELEMENT_COUNT; ++dimension) {
        getScale(*this, dimension, 0, size(), scaleActualMinima[dimension], scaleActualRanges[dimension]);
    }
}

void Score::rescale(void)
{
    for(int dimension = 0; dimension < Event::ELEMENT_COUNT; ++dimension) {
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
    for(int dimension = 0; dimension < Event::HOMOGENEITY; dimension++) {
        event[dimension] = event[dimension] - scaleActualMinima[dimension];
        double scale;
        if(scaleActualRanges[dimension] == 0.0) {
            scale = 1.0;
        } else {
            scale = scaleTargetRanges[dimension] / scaleActualRanges[dimension];
        }
        if(rescaleRanges[dimension]) {
            event[dimension] = event[dimension] * scale;
        }
        if(rescaleMinima[dimension]) {
            event[dimension] = event[dimension] + scaleTargetMinima[dimension];
        } else {
            event[dimension] = event[dimension] + scaleActualMinima[dimension];
        }
    }
}

void Score::dump(std::ostream &stream)
{
    stream << "silence::Score = " << int(size()) << " events:" << std::endl;
    for(Score::iterator i = begin(); i != end(); ++i) {
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

void Score::append(double time_, double duration, double status, double instrument, double key, double velocity, double phase, double pan, double depth, double height, double pitches)
{
    Event event;
    event.setTime(time_);
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
    std::sort(begin(), end());
}

double Score::getDuration()
{
    double start = 0.0;
    double end = 0.0;
    for (int i = 0, n = size(); i < n; ++i) {
        const Event &event = at(i);
        if (i == 0) {
            start = event.getTime();
            end = event.getOffTime();
        } else {
            if (event.getTime() < start) {
                start = event.getTime();
            }
            if (event.getOffTime() > end) {
                end = event.getOffTime();
            }
        }
    }
    return end - start;
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
            double outputDb = inputDb + gain;
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
    if (end_ > size()) {
        end_ = size();
    }
    for (size_t i = begin_; i < end_; i++) {
        Event &event = (*this)[i];
        double pitch = double(event.getKeyNumber());
        event.setKey(Voicelead::closestPitch(pitch, pitches));
    }
}

void Score::setPitchClassSet(size_t begin_, size_t end_, const std::vector<double> &pcs, size_t divisionsPerOctave_)
{
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
    if (endSource > size()) {
        endSource = size();
    }
    if (beginSource == endSource) {
        return;
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
    if (endSource > size()) {
        endSource = size();
    }
    if (beginSource == endSource) {
        return;
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

void Score::setK(size_t priorBegin, size_t begin, size_t end, double base, double range)
{
    std::vector<double> pitches = getPitches(priorBegin, begin);
    std::vector<double> pcs = Voicelead::uniquePcs(pitches);
    printChord("  before K:            ", pcs);
    std::vector<double> kpcs = Voicelead::K(pcs);
    printChord("  after K:             ", kpcs);
    setPitchClassSet(begin, end, kpcs);
}

void Score::setKV(size_t priorBegin, size_t begin, size_t end, double V, double base, double range)
{
    std::vector<double> pitches = getPitches(priorBegin, begin);
    std::vector<double> pcs = Voicelead::uniquePcs(pitches);
    std::vector<double> kpcs = Voicelead::K(pcs);
    std::vector<double> pt = Voicelead::pitchClassSetToPandT(kpcs);
    setPTV(begin, end, pt[0], pt[1], V, base, range);
}

void Score::setKL(size_t priorBegin, size_t begin, size_t end, double base, double range, bool avoidParallels)
{
    std::vector<double> pitches = getPitches(priorBegin, begin);
    std::vector<double> pcs = Voicelead::uniquePcs(pitches);
    std::vector<double> kpcs = Voicelead::K(pcs);
    voicelead(priorBegin,
            begin,
            begin,
            end,
            kpcs,
            base,
            range,
            avoidParallels);
}
static std::vector<double> matchContextSize(const std::vector<double> context, const std::vector<double> pcs)
{
    std::vector<double> localPcs = pcs;
    localPcs.resize(context.size());
    for (size_t i = pcs.size(), j = 0; i < context.size(); ++i, ++j) {
        localPcs[i] = pcs[j % pcs.size()];
    }
    return localPcs;
}
void Score::setQ(size_t priorBegin, size_t begin, size_t end, double Q, const std::vector<double> &context, double base, double range)
{
    System::inform("BEGAN Score::setQ(%f)...\n", Q);
    std::vector<double> pitches = getPitches(priorBegin, begin);
    std::vector<double> pcs = Voicelead::uniquePcs(pitches);
    printChord("  prior pcs:     ", pcs);
    printChord("  context:       ", context);
    std::vector<double> localPcs = matchContextSize(context, pcs);
    printChord("  localPcs:  ", localPcs);
    std::vector<double> qpcs = Voicelead::Q(localPcs, Q, context);
    printChord("  effect of Q:   ", qpcs);
    setPitchClassSet(begin, end, qpcs);
    pitches = getPitches(begin, end);
    pcs = Voicelead::uniquePcs(pitches);
    printChord("  posterior pcs: ", pcs);
    System::inform("ENDED Score::setQ.\n");
}

void Score::setQV(size_t priorBegin, size_t begin, size_t end, double Q, const std::vector<double> &context, double V, double base, double range)
{
    std::vector<double> pitches = getPitches(priorBegin, begin);
    std::vector<double> pcs = Voicelead::uniquePcs(pitches);
    printChord("  prior pcs:     ", pcs);
    printChord("  context:       ", context);
    std::vector<double> localPcs = matchContextSize(context, pcs);
    printChord("  localPcs:  ", localPcs);
    std::vector<double> qpcs = Voicelead::Q(localPcs, Q, context);
    printChord("  effect of Q:   ", qpcs);
    std::vector<double> pt = Voicelead::pitchClassSetToPandT(qpcs);
    setPTV(begin, end, pt[0], pt[1], V, base, range);
}

void Score::setQL(size_t priorBegin, size_t begin, size_t end, double Q, const std::vector<double> &context, double base, double range, bool avoidParallels)
{
    std::vector<double> pitches = getPitches(priorBegin, begin);
    std::vector<double> pcs = Voicelead::uniquePcs(pitches);
    printChord("  prior pcs:     ", pcs);
    printChord("  context:       ", context);
    std::vector<double> localPcs = matchContextSize(context, pcs);
    printChord("  localPcs:  ", localPcs);
    std::vector<double> qpcs = Voicelead::Q(localPcs, Q, context);
    printChord("  effect of Q:   ", qpcs);
    voicelead(priorBegin,
            begin,
            begin,
            end,
            qpcs,
            base,
            range,
            avoidParallels);
}

struct TimeAtComparator {
    double time;
    TimeAtComparator(double time_) : time(time_) {
    }
    bool operator()(const Event &event) {
        if (event.getTime() >= time) {
            return true;
        } else {
            return false;
        }
    }
};

int Score::indexAtTime(double time_)
{
    int index = size();
    std::vector<Event>::iterator it = std::find_if(begin(), end(), TimeAtComparator(time_));
    if (it != end()) {
        index = (it - begin());
    }
    return index;
}

struct TimeAfterComparator {
    double time;
    TimeAfterComparator(double time_) : time(time_) {
    }
    bool operator()(const Event &event) {
        if (event.getTime() > time) {
            return true;
        } else {
            return false;
        }
    }
};

int Score::indexAfterTime(double time_)
{
    int index = size();
    std::vector<Event>::iterator it = std::find_if(begin(), end(), TimeAfterComparator(time_));
    if (it != end()) {
        index = (it - begin());
    }
    return index;
}

double Score::indexToTime(size_t index)
{
    double time_ = DBL_MAX;
    if (index >= 0 && index < size()) {
        time_ = (*this)[index].getTime();
    }
    return time_;
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
        double time_ = event.getTime();
        double duration = event.getDuration();
        event.setTime(time_ * factor);
        event.setDuration(duration * factor);
    }
}

void Score::remove(size_t index)
{
    erase(begin() + index);
}
void Score::tieOverlappingNotes(bool considerInstrumentNumber)
{
    sort();
    for (int laterI = size() - 1; laterI > 1; --laterI) {
        Event &laterEvent = (*this)[laterI];
        if (!laterEvent.isNote()) {
            continue;
        }
        for (int earlierI = laterI - 1; earlierI > 0; --earlierI) {
            Event &earlierEvent = (*this)[earlierI];
            if (!earlierEvent.isNote()) {
                continue;
            }
            if (earlierEvent.getKeyNumber() != laterEvent.getKeyNumber()) {
                continue;
            }
            if (earlierEvent.getVelocity() <= 0.0 || laterEvent.getVelocity() <= 0.0) {
                continue;
            }
            if (earlierEvent.getOffTime() < laterEvent.getTime()) {
                continue;
            }
            if (considerInstrumentNumber && (earlierEvent.getChannel() != laterEvent.getChannel())) {
                continue;
            }
            // Ok, must be tied.
            earlierEvent.setOffTime(laterEvent.getOffTime());
            erase(begin() + laterI);
            break;
        }
    }
}

void Score::temper(double tonesPerOctave)
{
    for (size_t i = 0, n = size(); i < n; ++i) {
        (*this)[i].temper(tonesPerOctave);
    }
}
}
