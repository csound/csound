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
#ifndef SCOREMODEL_H
#define SCOREMODEL_H

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include "Composition.hpp"
#include "Node.hpp"
#include "Score.hpp"
  %}
#else
#include "Composition.hpp"
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
  class ScoreModel :
    public Composition,
    public Node
  {
  public:
    ScoreModel();
    virtual ~ScoreModel();
    virtual void initialize();
    /**
     * Generates a score based on a music graph defined
     * by the child nodes of this.
     */
    virtual void generate();
    /**
     * Clears the score.
     */
    virtual void clear();
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
     * Returns a MIDI filename based on the filename
     * of this, by appending ".mid" to the filename.
     */
    virtual std::string getMidiFilename();
    /**
     * Returns a soundfile name based on the filename
     * of this, by appending ".wav" to the filename.
     */
    virtual std::string getOutputSoundfileName();
    /**
     * Returns the address of this as a long integer.
     */
    virtual long getThis();
    /**
     * Returns the address of this as a Node pointer.
     */
    virtual Node *getThisNode();
  protected:
    std::string filename;
  };
}
#endif
