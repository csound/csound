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
#ifndef NODE_HPP
#define NODE_HPP

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include "Score.hpp"
#include <vector>
#include <eigen3/Eigen/Dense>
%}
%include "std_string.i"
%include "std_vector.i"
%template(NodeVector) std::vector<csound::Node*>;
#else
#include "Score.hpp"
#include <vector>
#include <eigen3/Eigen/Dense>
#endif

namespace csound
{
  /**
   * Base class for all music graph nodes in the Silence system.
   * Nodes can transform silence::Events produced by child nodes.
   * Nodes can produce silence::Events.
   */
  class SILENCE_PUBLIC Node
  {
  protected:
    Eigen::MatrixXd localCoordinates;
  public:
    /**
     * Child Nodes, if any.
     */
    std::vector<Node *> children;
    Node();
    virtual ~Node();
    /**
     * Returns the local transformation of coordinate system.
     */
    virtual Eigen::MatrixXd  getLocalCoordinates() const;
    /**
     * The default implementation postconcatenates its own local coordinate system
     * with the global coordinates, then passes the score and the product of coordinate systems
     * to each child, thus performing a depth-first traversal of the music graph.
     */
    virtual Eigen::MatrixXd traverse(const Eigen::MatrixXd &globalCoordinates,
                                           Score &score);
    /**
     * This function is called by the traverse() function.
     * The default implementation does nothing.
     * If a derived node produces new Events, then it must transform them by
     * the composite coordinates, then append them to the collecting score.
     * If a derived node transforms Events produced by child Nodes, then it
     * must transform only Events in the collecting score starting at the
     * startAt index and continuing up to, but not including, the endAt index.
     * These bookmarks, in turn, must be set in the Traverse function by
     * all Nodes that produce events.
     */
    virtual void produceOrTransform(Score &collectingScore,
                                    size_t beginAt,
                                    size_t endAt,
                                    const Eigen::MatrixXd &compositeCordinates);
    virtual Eigen::MatrixXd createTransform();
    virtual void clear();
    /**
     * Returns a reference to the indicated element of the local transformation of coordinate system.
     */
    virtual double &element(size_t row, size_t column);
    /**
     * Sets the indicated element of the local transformation of coordinate system.
     */
    virtual void setElement(size_t row, size_t column, double value);
    virtual void addChild(Node *node);
  };
  typedef Node* NodePtr;

  /**
   * Removes all duplicate events produced by the child nodes of this.
   */
  class SILENCE_PUBLIC RemoveDuplicates : public Node
  {
    public:
    virtual void produceOrTransform(Score &score,
                                    size_t beginAt,
                                    size_t endAt,
                                    const Eigen::MatrixXd &compositeCordinates);

  };
}
#endif
