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
#ifndef SCORENODE_H
#define SCORENODE_H

#ifdef SWIG
%module CsoundVST
%{
#include "Node.hpp"
#include "Score.hpp"
%}
#else
#include "Node.hpp"
#include "Score.hpp"
using namespace boost::numeric;
#endif

namespace csound
{
        /**
        * Node class that produces events from the contained score,
        * which can be built up programmatically or imported from a standard MIDI file.
        */
        class ScoreNode :
                public Node
        {
        protected:
                Score score;
        public:
                std::string importFilename;
                ScoreNode();
                virtual ~ScoreNode();
                virtual void produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &coordinates);
                virtual Score &getScore();
        };
}
#endif

