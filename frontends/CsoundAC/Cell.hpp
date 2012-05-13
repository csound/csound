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
#ifndef CELL_H
#define CELL_H

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include "ScoreNode.hpp"
#include <eigen3/Eigen/Dense>
%}
#else
#include "ScoreNode.hpp"
#include <eigen3/Eigen/Dense>
#endif

namespace csound
{
/**
 * Score node that simplifies building up repetitive
 * and overlapping motivic cells, such as used in Minimalism.
 */
class SILENCE_PUBLIC Cell :
    public ScoreNode
{
public:
    /**
     * The number of times to repeat the notes produced by the child nodes
     * of this.
     */
    int repeatCount;
    /**
     * Indicates whether the durationSeconds of this cell is added
     * to the duration of the notes produced by the child nodes of this
     * (true) at each repetition, or is simply the duration of the
     * cell (false), in which case the notes of the Nth repetition may
     * (or may not) overlap the notes of the N + 1th repetition.   .
     */
    bool relativeDuration;
    /**
     * If relativeDuraton is true, then this time is added to the
     * duration of the Nth repetition in order to obtain the
     * starting time of the N + 1th repetition; if relativeDuration is
     * false, then this time is added to the starting time of the Nth
     * repetition in order to obtain the starting time of the N + 1th
     * repetition.
     */
    double durationSeconds;
    Cell();
    virtual ~Cell();
    virtual void produceOrTransform(Score &collectingScore,
            size_t beginAt,
            size_t endAt,
            const Eigen::MatrixXd &coordinates);
};

/**
 * The notes produced by each child node are intercut to produce
 * the notes produced by this; e.g. if there are 3 child nodes, then
 * the notes produced by this are node 0 note 0, node 1 note 0, node
 * 2 note 0; node 0 note 1, node 1 note 1, node node 2 note 1; node 0
 * note 2, node 1 note 2, node 2 note 2, and so on. If the child nodes
 * do not each produce the same number of notes, then production stops
 * with the last note of the longest child.
 */
class SILENCE_PUBLIC Intercut :
    public ScoreNode
{
public:
    Intercut();
    virtual ~Intercut();
    virtual Eigen::MatrixXd traverse(const Eigen::MatrixXd &globalCoordinates,
            Score &collectingScore);
    virtual void produceOrTransform(Score &collectingScore,
            size_t beginAt,
            size_t endAt,
            const Eigen::MatrixXd &coordinates);
};

/**
 * The notes produced by each (not all) child node,
 * are rescaled to all start at the same time, and last for the
 * same duration; that of the 0th child, or a specified
 * duration.
 */
class SILENCE_PUBLIC Stack :
    public ScoreNode
{
public:
    /**
     * If non-zero, then each the notes of each child node in turn are
     * rescaled to fit within this duration; if zero, then the notes
     * of each child node are rescaled to fit within the duration of the
     * first (0th) node.
     */
    double duration;
    Stack();
    virtual ~Stack();
    virtual Eigen::MatrixXd traverse(const Eigen::MatrixXd &globalCoordinates,
            Score &collectingScore);
    virtual void produceOrTransform(Score &collectingScore,
            size_t beginAt,
            size_t endAt,
            const Eigen::MatrixXd &coordinates);
};

/**
 * All notes produced by child[N - 1] are rescaled and stacked on top
 * of each note produced by child[N - 2], and so on.
 */
class SILENCE_PUBLIC Koch :
    public ScoreNode
{
public:
    Koch();
    virtual ~Koch();
    virtual Eigen::MatrixXd traverse(const Eigen::MatrixXd &globalCoordinates,
            Score &collectingScore);
    virtual void produceOrTransform(Score &collectingScore,
            size_t beginAt,
            size_t endAt,
            const Eigen::MatrixXd &coordinates);
};
}
#endif

