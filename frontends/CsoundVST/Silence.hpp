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
#ifndef SILENCE_H
#define SILENCE_H

/**
* These are the declarations for the Silence framework,
* in other words all the classes defined in the Silence library itself.
* There must be one header file and one source file for each framework class.
* <p>
* SWIG is run on this file to generate wrappers for all Silence framework classes
* in other languages, especially scripting languages such as Python, 
* and that musicians will then compose in those languages.
* Therefore, all framework headers must be included in this header,
* and all framework headers must use #ifdef SWIG to declare
* the module and make other SWIG declarations (see Node.h for an extensive example).
* The order of declaration is important to SWIG!
* <p>
* It is also expected that doxygen will be used to generate documentation
* from comments in the framework header files.
*/

#ifdef SWIG
%module CsoundVST
%{
    #include <string>
    #include <vector>
    #include <map>
    #include <boost/numeric/ublas/vector.hpp>
    #include <boost/numeric/ublas/matrix.hpp>
%}
#else
#include <string>
#include <vector>
#include <map>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#endif

#include "Conversions.hpp"
#include "System.hpp"
#include "CsoundFile.hpp"
#include "CppSound.hpp"
#include "Event.hpp"
#include "Midifile.hpp"
#include "Score.hpp"
#include "Composition.hpp"
#include "Node.hpp"
#include "ScoreNode.hpp"
#include "Cell.hpp"
#include "Hocket.hpp"
#include "Rescale.hpp"
#include "MusicModel.hpp"
#include "Random.hpp"
#include "ImageToScore.hpp"
#include "StrangeAttractor.hpp"
#include "Lindenmayer.hpp"
#include "MCRM.hpp"

#endif

