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

/**
 * Contains classes for algorithmic composition, and supporting classes
 * for rendering audio from algorithmically generated scores, especially
 * using Csound. All of these classes are designed for wrapping in 
 * dynamic languges such as Python or Lua.
 */
namespace csound
{
  /**
   * Base class for user-defined musical compositions.
   * Contains a Score object for collecting generated Events
   * such as notes and control messages.
   */
  class Composition
  {
  protected:
    Score score;
    double tonesPerOctave;
    bool conformPitches;
  public:
    Composition();
    virtual ~Composition();
    /**
     * Generate performance events and store them in the score.
     * Must be overidden in derived classes.
     */
    virtual void generate();
    /**
     * Convenience function that erases the existing score,
     * invokes generate(), and invokes perform().
     */
    virtual void render();
    /**
     * Performs the current score.
     * The default implementation does nothing.
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
  };
}
#endif

