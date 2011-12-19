/*
    cscoremain.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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
*/

#include "csound.h"                                   /*   CSCOREMAIN.C   */
#include <stdarg.h>

extern void cscore(CSOUND*);

/* Cscore stub to run a user program standalone */

/* To create a standalone cscore program, compile this file with your own
   file (e.g. mycscore.c) containing a cscore() control function.  An
   example is given in the file cscore.c in this directory.  More examples
   may be found in the examples/cscore/ directory of the csound5 source tree.

   Use this command (or your system's equivalent) to compile:

     gcc mycscore.c cscoremain.c -o cscore -lcsound -L/usr/local/lib \
         -I/usr/local/include/csound

   On Mac OS X, use this command to compile:

     gcc mycscore.c cscoremain.c -o cscore -framework CsoundLib \
         -I/Library/Frameworks/CsoundLib.framework/Headers

   The resulting executable can be run with:

     ./cscore scorein > scoreout
                OR
     ./cscore scorein scoreout

 */

int main(int argc, char **argv)
{
    CSOUND  *cs;
    FILE    *insco, *outsco;
    int     result;

    /* Standalone Cscore is now a client of the Csound API */
    result = csoundInitialize(NULL, NULL, 0);
    if (result < 0) {
      fprintf(stderr, "Could not initialize the Csound library.\n");
      return -1;
    }
    cs = csoundCreate(NULL);
    if (cs == NULL) {
      fprintf(stderr, "Could not instantiate Csound.\n");
      return -1;
    }

    /* open the command line scorein file */
    if (argc < 2) {
      fprintf(stderr,
              "Insufficient arguments: must provide an input filename.\n");
      return -1;
    }
    if (!(insco = fopen(argv[1], "r"))) {
      fprintf(stderr, "Cannot open the input score '%s'\n", argv[1]);
      return -1;
    }

    /* open the command line scoreout file (stdout if none provided) */
    if (argc < 3) outsco = stdout;
    else if (!(outsco = fopen(argv[2], "w"))) {
      fprintf(stderr, "Cannot open the output score '%s'\n", argv[2]);
      return -1;
    }

    csoundInitializeCscore(cs, insco, outsco);
    cscore(cs);                         /* and call the user cscore program   */
    return 0;
}
