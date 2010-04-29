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
#ifndef CSOUNDMUSICMODEL_H
#define CSOUNDMUSICMODEL_H

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include "Composition.hpp"
#include "CppSound.hpp"
#include "MusicModel.hpp"
#include "Node.hpp"
#include "Score.hpp"
  %}
#else
#include "Composition.hpp"
#include "CppSound.hpp"
#include "MusicModel.hpp"
#include "Node.hpp"
#include "Score.hpp"
using namespace boost::numeric;
#endif

namespace csound
{
  /**
   * Base class for compositions
   * that use the principle of a music graph to generate a score.
   * A music graph is a directed acyclic graph of nodes
   * including empty nodes, nodes that contain only child nodes,
   * score nodes, event generator nodes, event transformer nodes,
   * and others. Each node is associated with a local transformation
   * of coordinate system in music space using a 12 x 12 homogeneous matrix.
   * To generate the score, the music graph is traversed depth first, and
   * each node postconcatenates its local transformation of coordinate system
   * with the coordinate system of its parent to derive a new local coordinate system,
   * which is applied to all child events.
   */
  class CsoundMusicModel :
    public Composition,
    public MusicModel
  {
    std::string filename;
  public:
    CsoundMusicModel();
    virtual ~CsoundMusicModel();
    virtual void initialize();
    virtual void generate();
    /**
     * Translate the generated score to a Csound score
     * and export it for performance.
     * The time given by extendSeconds is used for a concluding e statement.
     */
    virtual void createCsoundScore(std::string addToScore = "",
                                   double extendSeconds = 5.0);
    virtual void clear();
    virtual std::string getFilename() const;
    virtual void setFilename(std::string filename);
    static std::string generateFilename();
    virtual std::string getMidiFilename();
    virtual std::string getOutputSoundfileName();
    virtual long getThis();
    virtual Node *getThisNode();
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
  protected:
    CppSound cppSound_;
    CppSound *cppSound;
    /**
     * Prepended to generated score.
     */
    std::string csoundScoreHeader;
  };
}
#endif
