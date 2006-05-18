/*
  Copyright (C) 2005 Victor Lazzarini

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA

  tclcsound.c: Tcl/Tk command module entry points
*/

#include "tclcsound.h"

/* initialize Tcl Tk Interpreter */
PUBLIC int Tclcsound_Init(Tcl_Interp * interp)
{
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
      return TCL_ERROR;
    }
/*
    if (Tk_InitStubs(interp, "8.1", 0) == NULL) {
      return TCL_ERROR;
    }
*/
    tclcsound_initialise(interp);

    Tcl_PkgProvide(interp, "tclcsound", "1.0");
    return TCL_OK;
}

PUBLIC int Tclcsound_SafeInit(Tcl_Interp * interp)
{
    return Tclcsound_Init(interp);
}

