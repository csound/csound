/*
* C S O U N D A C
*
* Copyright (c) 2001-2003 by Michael Gogins. All rights reserved.
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
#if defined(SWIGLUA)

/* This function already gets exported in other form */
/* Don't export this to avoid build failures on amd64 */
%ignore Counterpoint::message(const char*, va_list);
%ignore csound::print(const char*, va_list);

%module(directors="1") luaCsoundAC
%{
        #include <algorithm>
        #include <vector>
        #include "Silence.hpp"
%}
%apply int { size_t };


%typemap(freearg) char ** {
  free((char *) $1);
}

%feature("director") Node;
%include "Silence.hpp"

#endif
