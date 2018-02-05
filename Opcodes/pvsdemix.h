/* pvsdemix.h:

  (c) Victor Lazzarini, 2005

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

PVSDEMIX:
De-mixing of stereo sources.

fsig   pvsdemix fleft,fright,kpos,kwidth,ipoints

INITIALIZATION:

ipoints - total number of discrete points, which will divide
each pan side of the stereo image. This ultimately affects
the resolution of the process.

PERFORMANCE

fleft - fsig containing the PVS analysis signal of the left channel
fright - fsig containing the PVS analysis signal of the right channel
kpos - the azimuth target centre position, which will be de-mixed, from
left to right (-1 <= kpos <= 1). This is the reverse pan-pot control.
kwidth - the azimuth subspace width, which will determine the number
of points around kpos which will be used in the de-mixing process.
(1 <= kwidth <= ipoints).

*/

#ifndef _PVSDEMIX_H
#define _PVSDEMIX_H

#include "pstream.h"

typedef struct _pvsdemix {
        OPDS h;
        PVSDAT  *fout;
        PVSDAT  *finleft;
        PVSDAT  *finright;
        MYFLT   *pos;
        MYFLT   *width;
        MYFLT   *slices;
        AUXCH   left;
        AUXCH   right;
        AUXCH   maxl;
        AUXCH   maxr;
        AUXCH   minl;
        AUXCH   minr;
        int32_t     beta;
        uint32  lastframe;
}
PVSDEMIX;

static int32_t pvsdemix_init(CSOUND *csound, PVSDEMIX *p);
static int32_t
pvsdemix_process(CSOUND *csound, PVSDEMIX *p);

#endif

