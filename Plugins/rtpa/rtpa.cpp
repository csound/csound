/*
    rtpa.cpp:

    Copyright (C) 2004, 2005 John ffitch, Istvan Varga,
                             Michael Gogins, Victor Lazzarini

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

/*                                              RTPA.C for PortAudio    */

/* The rtpa module links with PortAudio which, in turn, contains C++ code.
 * In order for Csound built with MinGW to handle exceptions thrown from 
 * this C++ code in the PortAudio library, it is necessary to compile and link
 * rtpa itself as C++ code.
 */

#ifdef __cplusplus
extern "C" {
#endif
    
#include "rtpa.c"

#ifdef __cplusplus
};
#endif
