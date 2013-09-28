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

#ifndef _INCLUDES_H
#define _INCLUDES_H

#include "max_headers.h"

#define PHOENIX_LIMIT 6 // Have to set > 5 for boost 1.44.0.beta1.
#ifdef MACOSX
	#undef check
#endif
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include <assert.h>
#include <math.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "eksepshun.h"

#ifdef _WINDOWS
	#pragma warning(disable : 4996) // disable deprecated functions
	#pragma warning(disable : 4142) // disable benign redefinition warnings
	#include <direct.h>
	#include <malloc.h>
#endif

#endif // _INCLUDES_H