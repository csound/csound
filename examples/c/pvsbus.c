/*
    pvsbus.c: example program for pvs bus interface
    works with pvsbus.csd

    (C) V Lazzarini, 2006

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

/* Console Csound using the Csound API. */

#include "csound.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


int main(int argc, char **argv)
{
    CSOUND  *csound;
    char    *fname = NULL;
    int     k, result;
    PVSDATEXT dataout, datain;

    /* initialise PVSATEXT data */
    datain.N = 1024;
    datain.format = 0;
    datain.overlap = 256;
    datain.winsize = 1024;
    datain.frame = (float *) calloc(sizeof(float),(datain.N+2));

    dataout.N = 1024;
    dataout.format = 0;
    dataout.overlap = 256;
    dataout.winsize = 1024;
    dataout.frame = (float *) calloc(sizeof(float),(dataout.N+2));

    /*  Create Csound. */
    csound = csoundCreate(NULL);

    /*  One complete performance cycle. */
    result = csoundCompile(csound, argc, argv);
    while (!result){

      /* copy data from pvs out to pvs in bus */
      for(k=0; k < 1026; k++)
          datain.frame[k] = dataout.frame[k];
      datain.framecount = dataout.framecount;
      /* send in the pvs in bus data */
      csoundSetPvsChannel(csound, &datain, "0");
      /* one ksmps pass */
      result = csoundPerformKsmps(csound);
      /* receive the pvs out bus data */
      csoundGetPvsChannel(csound, &dataout, "0");
    }
    /* delete Csound instance */
    csoundDestroy(csound);
    free(datain.frame);
    free(dataout.frame);
    return (result >= 0 ? 0 : result);
}

