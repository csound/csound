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

#ifdef SWIG
%module CsoundVST
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
   * Contains a Score object for collecting generated Events such as notes and control messages,
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
  public:
    Composition();
    virtual ~Composition();
    /**
     * Generate performance events and store them in the score. 
     * Must be overidden in derived classes.
     */
    virtual void generate();
    /**
     * Translate the generated score to a Csound score and export it for performance.
     */
    virtual void createCsoundScore();
    /**
     * Convenience function that erases the existing score, 
     * invokes generate(), and invokes perform().
     */
    virtual void render();
    /**
     * Uses csound to perform the current score.
     */
    virtual void perform();
    /**
     * Clear all contents of this. Probably should be overridden in derived classes.
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
  };
}
#endif

