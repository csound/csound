/*
    main.cpp:
    Copyright (C) 2006 Istvan Varga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include "CsoundGUI.hpp"
#include <FL/fl_ask.H>

int main(int argc, char **argv)
{
    CsoundGUIMain   *mainWin;

    if (csoundInitialize(&argc, &argv, 0) < 0) {
      fprintf(stderr, "Couldn't Initialize Csound! Try removing command line flags.\nQuitting...\n");
      fl_alert("Couldn't Initialize Csound!\nTry removing command line flags.");
      return -1;
    }
    Fl::lock();
    mainWin = new CsoundGUIMain;
    mainWin->run(argc, argv);
    delete mainWin;
    Fl::wait(0.0);

    return 0;
}

