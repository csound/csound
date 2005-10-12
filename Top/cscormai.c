/*
    cscormai.c:

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

#include "cscore.h"                                   /*   CSCOREMAIN.C   */
#include "csoundCore.h"
#include "text.h"

extern void cscore(CSOUND*);
void err_printf(char *, ...);

int main(int argc, char **argv) /* cscore stub to run a user prog standalone   */
{
    CSOUND*  cs;
    FILE*    insco;
    int      result;
    
    /* Standalone Cscore is now a client of the Csound API */
    result = csoundInitialize(0, NULL, 0);
    if  (result != CSOUND_SUCCESS) {
        err_printf("Could not initialize the Csound library.");
        exit(-1);
    }
    cs = csoundCreate(NULL);
    if  (cs == NULL) {
        err_printf("Could not instantiate Csound.");
        exit(-1);
    }
    
    /* open the command line scorein file */
    if (!(--argc)) {
        err_printf("Insufficient arguments: must provide an input filename.\n");
        exit(-1);
    }
    if (!(insco = fopen(*++argv, "r"))) {
        err_printf("Cannot open the input score %s\n", *argv);
        exit(-1);
    }

    csoundInitializeCscore(cs, insco, stdout);
    cscore(cs);                         /* and call the user cscore program   */
    return 0;
}

/*int lplay(CSOUND* cs, EVLIST *a)        /* for standalone cscore: no full Csound, so *\/
                                        /* field lplay calls & put events to stderr  *\/
{
    FILE *osave;

    csoundMessage(cs, 
        Str("cscore lplay:  full Csound would now play the following score\n"));
    osave = cs->oscfp;
    cs->oscfp = stderr;
    cscoreListPut(cs,a);
    cs->oscfp = osave;
    return OK;
}
*/
/* This standalone cscore stub is invoked with cscore_xxx.c as follows:    */
/*       cc -o cscore cscore_xxx.c -lcscore                                */
/* or, if no libcscore.a was created at installation:                      */
/*       cc -o cscore cscore_xxx.c $CSOUND/cscoremain.o $CSOUND/cscorefns.o \
                         $CSOUND/rdscor.o $CSOUND/memalloc.o               */
/* where CSOUND is an environment variable denoting a Csound directory     */
/* containing previously compiled modules.                                 */
/* If cannot find cscore.h use -I/usr/local/include, or copy from Csound   */
/* The resulting executable can be run with:                               */
/*       cscore scorin > scoreout                                          */

#include <stdarg.h>

void err_printf(char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
}
