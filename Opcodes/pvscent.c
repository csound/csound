/* pvscent.c:
   Calculation of spectral centroid as Beauchamp

   (c) John ffitch, 2005

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

#include "csdl.h"
#include "pstream.h"

typedef struct {
        OPDS    h;
        MYFLT   *ans;
        PVSDAT  *fin;
        unsigned long   lastframe;
} PVSCENT;

int pvscentset(ENVIRON *csound, PVSCENT *p)
{
     p->lastframe = 0;
     *p->ans = FL(0.0);
     if (!(p->fin->format==PVS_AMP_FREQ) || (p->fin->format==PVS_AMP_PHASE))
       return
         csound->InitError(csound, Str("pvscent: format must be amp-phase or amp-freq.\n"));
     return OK;
}

int pvscent(ENVIRON *csound, PVSCENT *p)
{
    long i,N = p->fin->N;
    MYFLT c = FL(0.0);
    MYFLT d = FL(0.0);
    MYFLT j, binsize = FL(0.5)*esr/(MYFLT)N;
    float *fin = (float *) p->fin->frame.auxp;
    if (p->lastframe < p->fin->framecount) {
      for (i=0,j=FL(0.5)*binsize; i<N+2; i+=2, j += binsize) {
        c += fin[i]*j;         /* This ignores phase */
        d += fin[i];
      }
      *p->ans = (d==FL(0.0) ? FL(0.0) : c/d);
      p->lastframe = p->fin->framecount;
    }
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
   {"pvscent", S(PVSCENT), 3, "k", "f", (SUBR)pvscentset, (SUBR)pvscent, NULL}
};

LINKAGE

