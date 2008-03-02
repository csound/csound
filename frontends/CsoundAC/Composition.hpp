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
#ifndef COMPOSITION_H
#define COMPOSITION_H

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include "CppSound.hpp"
#include "Score.hpp"
  %}
#else
#include "CppSound.hpp"
#include "Score.hpp"
#endif

namespace csound
{
  /**
   * Base class for user-derived musical compositions.
   * Contains a Score object for collecting generated Events 
   * such as notes and control messages,
   * and an Orchestra object for rendering the generated scores.
   */
  class Composition
  {
  protected:
    Score score;
    double tonesPerOctave;
    bool conformPitches;
    CppSound cppSound_;
    CppSound *cppSound;
    /**
     * Prepended to generated score.
     */
    std::string csoundScoreHeader;
  public:
    Composition();
    virtual ~Composition();
    /**
     * Generate performance events and store them in the score.
     * Must be overidden in derived classes.
     */
    virtual void generate();
    /**
     * Translate the generated score to a Csound score 
     * and export it for performance.
     * The time given by extendSeconds is used for a concluding e statement.
     */
    virtual void createCsoundScore(std::string addToScore = "", 
				   double extendSeconds = 5.0);
    /**
     * Convenience function that erases the existing score,
     * appends optional text to it,
     * invokes generate(), invokes createCsoundScore(), and invokes perform().
     */
    virtual void render();
    /**
     * Uses csound to perform the current score.
     */
    virtual void perform();
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
     * Sets the self-contained Orchestra.
     */
    virtual void setCppSound(CppSound *orchestra);
    /**
     * Return the self-contained Orchestra.
     */
    virtual CppSound *getCppSound();
    /**
     * Write as if to stdout or stderr.
     */
    virtual void write(const char *text);
    virtual void setTonesPerOctave(double tonesPerOctave);
    virtual double getTonesPerOctave() const;
    virtual void setConformPitches(bool conformPitches);
    virtual bool getConformPitches() const;
    /**
     * Set the Csound orchestra
     * (convenience wrapper for CppSound::setOrchestra()).
     */
    virtual void setCsoundOrchestra(std::string orchestra);
    /**
     * Return the Csound orchestra
     * (convenience wrapper for CppSound::getOrchestra()).
      */
    virtual std::string getCsoundOrchestra() const;
    /**
     * Set a Csound score fragment to be prepended
     * to the generated score (createCsoundScore is called with it).
     */
    virtual void setCsoundScoreHeader(std::string header);
    /**
     * Return the Csound score header that is prepended
     * to generated scores.
     */
    virtual std::string getCsoundScoreHeader() const;
    /**
     * Re-assign instrument numbers for export to Csound score
     * (convenience wrapper for Score::arrange()).
     */
    virtual void arrange(int oldInstrumentNumber, int newInstrumentNumber);
    /**
     * Re-assign instrument numbers and adjust gains 
     * for export to Csound score
     * (convenience wrapper for Score::arrange()).
     */
    virtual void arrange(int oldInstrumentNumber, 
			 int newInstrumentNumber, 
			 double gain);
    /**
     * Re-assign instrument numbers, adjust gains, 
     * and change pans for export to Csound score
     * (convenience wrapper for Score::arrange()).
     */
    virtual void arrange(int oldInstrumentNumber, 
			 int newInstrumentNumber, 
			 double gain, 
			 double pan);
    /**
     * Remove instrument number, gain, and pan assignments
     * (convenience wrapper for Score::removeArrangement()).
     */
    virtual void removeArrangement();
    /**
     * Set Csound command line
     * (convenience wrapper for CppSound::setCommand()).
     */
    virtual void setCsoundCommand(std::string command);
    /**
     * Return Csound command line
     * (convenience wrapper for CppSound::getCommand()).
     */
    virtual std::string getCsoundCommand() const;
  };
}
#endif

