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
#ifndef HOCKET_H
#define HOCKET_H

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include "ScoreNode.hpp"
%}
#else
#include "ScoreNode.hpp"
#endif

namespace csound
{
  /**
   * Simplifies constructing complex hocketted scores.
   * Where N is the moulus, this node produces every Nth note
   * of the notes produced by the child nodes of this, starting
   * with note at the starting index.
   */
  class SILENCE_PUBLIC Hocket :
    public ScoreNode
  {
  public:
    /**
     * The size of the hocket; 1 is every note, 2 is every other note, 
     * 3 is every third note, and so on.
     */
    int modulus;
    /** 
     * The index from which to begin hocketting; 0 to begin with the 
     * first note, 1 to begin with the second note, and so on.
     */
    int startingIndex;
    Hocket();
    virtual ~Hocket();
    virtual Eigen::MatrixXd traverse(const Eigen::MatrixXd &globalCoordinates, 
        Score &collectingScore);
  };
}
#endif

