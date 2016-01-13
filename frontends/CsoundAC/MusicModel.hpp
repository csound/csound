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
#ifndef MUSICMODEL_H
#define MUSICMODEL_H

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include "ScoreModel.hpp"
#include "CppSound.hpp"
#include "Node.hpp"
#include "Score.hpp"
%}
#else
#include "ScoreModel.hpp"
#include "CppSound.hpp"
#include "Node.hpp"
#include "Score.hpp"
#endif

namespace csound
{
  /**
   * A ScoreModel that uses Csound to render generated scores,
   * via the CppSound class.
   */
  class SILENCE_PUBLIC MusicModel :
    public ScoreModel
  {
  public:
    int threadCount;
    MusicModel();
    virtual ~MusicModel();
    virtual void initialize();
    virtual int generate();
#ifndef LINUX
    virtual intptr_t
#else
    virtual long
#endif
      getThis();
    virtual Node *getThisNode();
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
    virtual int render();
    virtual void stop();
    /**
     * Uses csound to perform the current score.
     */
    virtual int perform();
    /**
     * Clear all contents of this.
     */
    virtual void clear();
    /**
     * Sets the self-contained Orchestra.
     */
    virtual void setCppSound(CppSound *orchestra);
    /**
     * Return the self-contained Orchestra.
     */
    virtual CppSound *getCppSound();
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
     * Re-assign instrument number for export to Csound score
     * (convenience wrapper for Score::arrange()).
     */
    virtual void arrange(int oldInstrumentNumber, int newInstrumentNumber);
    /**
     * Re-assign instrument number and adjust gain
     * for export to Csound score
     * (convenience wrapper for Score::arrange()).
     */
    virtual void arrange(int oldInstrumentNumber,
                         int newInstrumentNumber,
                         double gain);
    /**
     * Re-assign instrument number, adjust gain,
     * and change pan for export to Csound score
     * (convenience wrapper for Score::arrange()).
     */
    virtual void arrange(int oldInstrumentNumber,
                         int newInstrumentNumber,
                         double gain,
                         double pan);
    /**
     * Re-assign instrument by name for export to Csound score.
     */
    virtual void arrange(int silenceInstrumentNumber,
                         std::string csoundInstrumentName);
    /**
     * Re-assign instrument by name and adjust gains for export to Csound score.
     */
    virtual void arrange(int silenceInstrumentNumber,
                         std::string csoundInstrumentName,
                         double gain);
    /**
     * Re-assign instrument by name, adjust gain, and change pan for export to Csound score.
     */
    virtual void arrange(int silenceInstrumentNumber,
                         std::string csoundInstrumentName,
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
    /**
     * Pass the invoking program's command-line arguments to processArgs()
     * and it will perform the following commands:
     *
     * --csound        Render generated score using set Csound orchestra.
     * --midi          Render generated score as MIDI file and play it (default).
     * --pianoteq      Play generated MIDI sequence file with Pianoteq.
     * --pianoteq-wav  Render score to soundfile using Pianoteq,
     *                 post-process it, and play it.
     * --playmidi      Play generated MIDI filev
     *                 post-process it, and play it.
     * --playwav       Play rendered or normalized output soundfile.
     * --post          Post-process Csound output soundfile:
     *                 normalize, CD, MP3, tag, and play it.
     */
    virtual int processArgs(const std::vector<std::string> &args);
  protected:
    /**
     * Self-contained Csound object.
     */
    CppSound cppSound_;
    /**
     * Pointer to a Csound object that is
     * used to render scores. Defaults to
     * the internal Csound object, but
     * can be re-set to an external Csound object.
     */
    CppSound *cppSound;
    /**
     * Prepended to generated score.
     */
    std::string csoundScoreHeader;
  };
}

#endif
