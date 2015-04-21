/*
    csound~ : A MaxMSP external interface for the Csound API.

    Created by Davis Pyon on 2/4/06.
    Copyright 2006-2010 Davis Pyon. All rights reserved.

    LICENSE AGREEMENT

    This software is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this software; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H

#define DEFAULT_NUM_SIGNALS 2
#define MAX_ARGS 64
#define MAX_STRING_LENGTH 1024
#define MAX_CHAN_NAME_LENGTH 64
#define MAX_EVENT_MESSAGE_SIZE 1024
#define DEFAULT_CLOCK_INTERVAL 10
#define DEFAULT_MESSAGE_CLOCK_INTERVAL 200
#define RECORD_CLOCK_INTERVAL 1
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_STOPPAGE_TIME 8 // seconds
#define MAX_ATOM_LIST_SIZE 32
#define MIDI_MATRIX_SIZE 2048   // 16 * 128

// INCORRECT
// Should be platform AND compiler.
#ifdef _WINDOWS
	#define _USE_BOOST_SERIALIZATION
	typedef unsigned char byte;
#endif

#ifdef MACOSX
	#ifndef PPC
		// Sequencer uses boost.serialization which requires 1-byte bool (darwin ppc arch uses 4-byte bool).
		#define _USE_BOOST_SERIALIZATION
	#endif
	typedef unsigned char byte;
	typedef unsigned int DWORD;
#endif

#ifndef FL
	#define FL(x) x##f
#endif

#endif // _DEFINITIONS_H
