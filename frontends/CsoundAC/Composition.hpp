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
#ifndef COMPOSITION_H
#define COMPOSITION_H

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include "Score.hpp"
%}
#else
#include "Score.hpp"
#endif

namespace csound
{
/**
 * Base class for user-defined musical compositions.
 * Contains a Score object for collecting generated Events
 * such as notes and control messages.
 */
class Composition
{
public:
    Composition();
    virtual ~Composition();
    /**
     * Returns the filename of this, which is used as a base
     * for derived filenames (soundfile, MIDI file, etc.).
     */
    virtual std::string getFilename() const;
    /**
     * Sets the filename of this -- basically, the
     * title of the composition.
     */
    virtual void setFilename(std::string filename);
    /**
     * Generates a versioned filename.
     */
    static std::string generateFilename();
    /**
     * Returns the current locale time as a string.
     */
    static std::string makeTimestamp();
    /**
     * Returns the time the score was generated.
     */
    virtual std::string getTimestamp() const;
    /**
     * Returns a soundfile name based on the filename
     * of this, by appending ".wav" to the filename.
     */
    virtual std::string getOutputSoundfileName();
    /**
     * Returns a soundfile name based on the filename
     * of this, by appending ".norm.wav" to the filename.
     */
    virtual std::string getNormalizedSoundfileName();
    /**
     * Returns a soundfile name for a CD audio track based on the filename
     * of this, by appending ".cd.wav" to the filename.
     */
    virtual std::string getCdSoundfileName();
    /**
     * Returns a soundfile name for an MP3 file based on the filename
     * of this, by appending ".mp3" to the filename.
     */
    virtual std::string getMp3SoundfileName();
    /**
     * Returns a MIDI filename based on the filename
     * of this, by appending ".mid" to the filename.
     */
    virtual std::string getMidiFilename();
    /**
     * Returns a MusicXML filename based on the filename
     * of this, by appending ".xml" to the filename.
     */
    virtual std::string getMusicXmlFilename();
    /**
     * Returns a MusicXML filename based on the filename
     * of this, by appending ".fms" to the filename.
     */
    virtual std::string getFomusFilename();
    /**
     * Returns a MusicXML filename based on the filename
     * of this, by appending ".ly" to the filename.
     */
    virtual std::string getLilypondFilename();
    /**
     * Generate performance events and store them in the score.
     * Must be overidden in derived classes.
     */
    virtual void generate();
    /**
     * Convenience function that calls clear(), generate(), perform().
     */
    virtual void render();
    /**
     * Convenience function that calls clear(), generate(), performAll().
     */
    virtual void renderAll();
    /**
     * Performs the current score to create an output soundfile,
     * which should be tagged with author, timestamp, copyright,
     * title, and optionally album.
     * The default implementation does nothing.
     * Must be overridden in derived classes.
     */
    virtual void perform();
    /**
     * Convenience function that calls saveMidi(), saveMusicXML(), and perform().
     */
    virtual void performMaster();
    /**
     * Convenience function that calls rescaleOutputSoundfile(), 
     * translateToCdAudio(), and translateToMp3().
     */
    virtual void translateMaster();
    /**
     * Convenience function that calls performMaster(),
     * and translateMaster().
     */
    virtual void performAll();
    /**
     * Assuming the score has been rendered,
     * uses sox to translate the output soundfile to a normalized soundfile.
     */
    virtual void normalizeOutputSoundfile(double levelDb = -3.0);
    /**
     * Assuming the score has been rendered,
     * uses sox to translate the output soundfile to normalized CD-audio format.
     */
    virtual void translateToCdAudio(double levelDb = -3.0);
    /**
     * Assuming the score has been rendered,
     * uses sox and LAME to translate the output soundfile to normalized MP3 format.
     */
    virtual void translateToMp3(double bitrate = 256.01, double levelDb = -3.0);
    /**
     * Clear all contents of this. Probably should be overridden
     * in derived classes.
     */
    virtual void clear();
    /**
     * Return the self-contained Score.
     */
    virtual Score &getScore();
    /**
     * Write as if to stderr.
     */
    virtual void write(const char *text);
    /**
     * Sets the number of equally tempered intervals
     * per octave (the default is 12, 0 means
     * non-equally tempered).
     */
    virtual void setTonesPerOctave(double tonesPerOctave);
    /**
     * Returns the number of equally tempered intervals
     * per octave (the default is 12, 0 means
     * non-equally tempered).
     */
    virtual double getTonesPerOctave() const;
    /**
     * Sets whether or not the pitches in generated
     * scores will be conformed to the nearest equally
     * tempered pitch.
     */
    virtual void setConformPitches(bool conformPitches);
    /**
     * Returns whether or not the pitches in generated
     * scores will be conformed to the nearest equally
     * tempered pitch.
     */
    virtual bool getConformPitches() const;
    /**
     * Saves the generated score in Fomus format
     * and uses Fomus and Lilypond to translate that 
     * to a PDF of music notation. A meter of 4/4
     * and a tempo of MM 120 is assumed. 
     * A vector of part names may be supplied.
     */
    virtual void translateToNotation(const std::vector<std::string> partNames = std::vector<std::string>(), std::string header = "");
    virtual std::string getArtist() const;
    virtual void setArtist(std::string value);
    virtual std::string getTitle() const;
    virtual void setTitle(std::string value);
    virtual std::string getCopyright() const;
    virtual void setCopyright(std::string value);
    virtual std::string getAlbum() const;
    virtual void setAlbum(std::string value);
    virtual std::string getLicense() const;
    virtual void setLicense(std::string value);
    protected:
    Score score;
    double tonesPerOctave;
    bool conformPitches;
    std::string filename;
    std::string artist;
    std::string title;
    std::string timestamp;
    std::string copyright;
    std::string album;
    std::string license;
};
}
#endif

