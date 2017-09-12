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
#ifndef SILENCE_H
#define SILENCE_H

/** \namespace csound
 * The csound namespace contains classes for doing algorithmic composition,
 * and for rendering audio from algorithmically generated scores,
 * especially using Csound.
 *
 * There should be one header file for each class declared in CsoundAC,
 * and that header file should be #included in Silence.hpp.
 *
 * SWIG is run on Silence.hpp to generate wrappers for all CsoundAC classes
 * in other languages, especially scripting languages such as Python,
 * Therefore, all framework headers must be included in this header,
 * and all framework headers must use #ifdef SWIG to declare
 * the module and make other SWIG declarations (see Node.h for an extensive example).
 * The order of declaration is important to SWIG!
 *
 * It is also expected that doxygen will be used to generate documentation
 * from comments in the framework header files.
 */
#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%begin %{
#include <cmath>
%}
%feature("autodoc", "1");
%include "typemaps.i"
%include "std_vector.i"
%{
#include <csound.h>
#include <string>
#include <vector>
#include <map>
#include <eigen3/Eigen/Dense>
%}
#else
#include <string>
#include <vector>
#include <map>
#include <eigen3/Eigen/Dense>
#endif

#include "Conversions.hpp"
#include "System.hpp"
#include "Event.hpp"
#include "Midifile.hpp"
#include "Score.hpp"
#include "Composition.hpp"
#include "Node.hpp"
#include "Counterpoint.hpp"
#include "CounterpointNode.hpp"
#include "ScoreNode.hpp"
#include "Cell.hpp"
#include "Hocket.hpp"
#include "Rescale.hpp"
#include "ScoreModel.hpp"
#include "MusicModel.hpp"
#include "Sequence.hpp"
#include "Random.hpp"
#include "ImageToScore.hpp"
#include "StrangeAttractor.hpp"
#include "Lindenmayer.hpp"
#include "MCRM.hpp"
#include "Soundfile.hpp"
#include "Voicelead.hpp"
#include "VoiceleadingNode.hpp"
#include "ChordLindenmayer.hpp"
#include "ChordSpace.hpp"

#endif
