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

  main_tclsh.c: Tcl/Tk csound-aware interpreter
*/

#include "tclcsound.h"

/* initialize Tcl Tk Interpreter */
int Tcl_ApInit(Tcl_Interp *interp)
{
    int     status;

    status = Tcl_Init(interp);
    if (status != TCL_OK) {
      return TCL_ERROR;
    }
    printf("cstclsh: TclCsound tcl interpreter\n"
           "(c) Victor Lazzarini\n"
           "Music Technology Lab\n"
           "NUI Maynooth, 2005\n");

    return tclcsound_initialise(interp);
}

int main(int argc, char *argv[])
{
    Tcl_Main(argc, argv, Tcl_ApInit);
    return 0;
}

