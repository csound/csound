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
#include "ScoreModel.hpp"
#include "Exception.hpp"
#include "Composition.hpp"
#include "System.hpp"

namespace csound
{
  ScoreModel::ScoreModel()
  {
  }

  ScoreModel::~ScoreModel()
  {
    //clear();
  }

  void ScoreModel::initialize()
  {
  }

  int ScoreModel::generate()
  {
    if (children.size()) {
      score.clear();
    }
    traverse(getLocalCoordinates(), score);
    System::message("Generated %d events.\n", score.size());
    return 0;
  }

  void ScoreModel::clear()
  {
    Node::clear();
    Composition::clear();
  }
#ifndef LINUX
  intptr_t
#else
  long
#endif
  ScoreModel::getThis()
  {
    return (intptr_t) this;
  }

  Node *ScoreModel::getThisNode()
  {
    return (Node *)this;
  }

}

