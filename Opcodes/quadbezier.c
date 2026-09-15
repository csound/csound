/*
    quadbezier.c:

    Copyright (C) 2016 Guillermo Senna.

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include <math.h>

static MYFLT FindTforX(MYFLT x1, MYFLT x2, MYFLT x3, int32_t x);

/*
   This Gen routine fills a table with the values produced by applying the
   quadratic B??zier function. It is aimed at frontend developers and will
   draw the correct paths as long as it is treated like a regular function.
   This means that, for example, you won't be able to create a circle and
   store it inside an f-table.

   References:
   https://pomax.github.io/bezierinfo/ -> A very concise summary on the matter.
   http://stackoverflow.com/questions/5634460/quadratic-bezier-curve-calculate-point
   http://stackoverflow.com/questions/27791915/quadratic-bezier-curve-calculate-t-given-x
   https://github.com/vikman90/bezier
   https://en.wikipedia.org/wiki/B??zier_curve

   Implementation:
   It is assumed that x1 equals 0 and that x[n] can't never be equal or
   greater than x[n+1]. On the other hand, cx[n] can be equal to x[n] or
   x[n+1].

   For the coding part, I've recycled code from some of the numbered GEN
   routines and specially from the "fareygen" and other named routines.
   Credit for that goes to the respective authors of those routines.

*/

static int32_t quadbeziertable (FGDATA *ff, FUNC *ftp)
{
    int32_t nvals, nargs, n, j = 0;
    MYFLT   *fp = ftp->ftable;
    CSOUND *csound = ff->csound;

    nvals = ff->flen;
    nargs = ff->e.pcnt - 4;
    if (UNLIKELY(nargs < 5)) {
      return csound->FtError(ff, "%s", Str("insufficient arguments"));
    }
    if (UNLIKELY((nargs - 1) % 4 != 0))
      return csound->FtError(ff, "%s", Str("incomplete quadbezier segment"));
    ff->e.p[4] *= -1;

    for (n = 4; n < nargs; n += 4)
    {
      MYFLT x1 = (n < 8) ? FL(0.0) : ff->e.p[n];
      MYFLT cx = ff->e.p[n+2], x2 = ff->e.p[n+4];
      if (UNLIKELY(!isfinite(x2) || !(x2 > x1)))
        return csound->FtError(ff, "%s",
                              Str("quadbezier endpoints must be finite and increasing"));
      if (UNLIKELY(!(cx >= x1 && cx <= x2)))
        return csound->FtError(ff, "%s",
                              Str("quadbezier control point must lie within its segment"));

      /* Keep the sample index separate from the curve's fractional coordinates. */
      while (j <= nvals && j <= x2) {
        MYFLT t;
        t = FindTforX(x1, cx, x2, j);
        fp[j++] = (FL(1.0) - t) * (FL(1.0) - t) * ff->e.p[n+1] +
          FL(2.0) * (FL(1.0) - t) * t * ff->e.p[n+3] + t * t * ff->e.p[n+5];
      }
    }
    return OK;
}

/* utility functions */

static MYFLT FindTforX(MYFLT x1, MYFLT x2, MYFLT x3, int32_t
                       x)
{
    MYFLT a =  (x1 - FL(2.0)*x2 + x3), b = FL(2.0)* (-x1 + x2), c = x1 - x;
    if (x == x1)
      return FL(0.0);
    if (x == x3)
      return FL(1.0);
    /* Rationalize the root to avoid cancellation for nearly linear segments. */
    if (a)
      return -FL(2.0)*c / (b + SQRT(b*b - FL(4.0)*a*c));
    else
      return (x-x1)/b;
}


static NGFENS quadbezier_fgens[] = {
  { "quadbezier", quadbeziertable },
  { NULL, NULL }
};

FLINKAGE_BUILTIN(quadbezier_fgens)

