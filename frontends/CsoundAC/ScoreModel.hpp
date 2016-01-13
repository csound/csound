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
#endif

namespace csound {
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
class SILENCE_PUBLIC ScoreModel :
                        public Composition,
                        public Node {
public:
        ScoreModel();
        virtual ~ScoreModel();
        virtual void initialize();
        /**
         * Generates a score based on a music graph defined
         * by the child nodes of this.
         */
        virtual int generate();
        /**
         * Clears the score.
         */
        virtual void clear();
        /**
         * Returns the address of this as a long integer.
         */
        virtual intptr_t getThis();
        /**
         * Returns the address of this as a Node pointer.
         */
        virtual Node *getThisNode();
};
}
#endif
