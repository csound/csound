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
  class Cell :
    public ScoreNode
  {
  public:
    int repeatCount;
    bool relativeDuration;
    double durationSeconds;
    Cell();
    virtual ~Cell();
    virtual void produceOrTransform(Score &score, size_t beginAt, size_t endAt, const Eigen::MatrixXd &coordinates);
  };
}
#endif

