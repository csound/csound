#include "csdl.h"
#include <math.h>

static MYFLT SolveQuadratic(MYFLT a, MYFLT b, MYFLT c);
static MYFLT FindTforX(MYFLT x1, MYFLT x2, MYFLT x3, int x);


static CS_NOINLINE int fterror(const FGDATA *ff, const char *s, ...)
{
    CSOUND  *csound = ff->csound;
    char    buf[64];
    va_list args;

    snprintf(buf, 64, Str("ftable %d: "), ff->fno);
    va_start(args, s);
    csound->ErrMsgV(csound, buf, s, args);
    va_end(args);
    csound->Message(csound, "f%3.0f %8.2f %8.2f ",
                            ff->e.p[1], ff->e.p2orig, ff->e.p3orig);
    if (csound->ISSTRCOD(ff->e.p[4]))
      csound->Message(csound, ff->e.strarg);
    else
      csound->Message(csound, "%8.2f", ff->e.p[4]);
    if (csound->ISSTRCOD(ff->e.p[5]))
      csound->Message(csound, "  \"%s\" ...\n", ff->e.strarg);
    else
      csound->Message(csound, "%8.2f ...\n", ff->e.p[5]);

    return -1;
}

static int quadbeziertable (FGDATA *ff, FUNC *ftp)
{
    int nvals, nargs, n;
    MYFLT   *fp = ftp->ftable;

    nvals = ff->flen;
    nargs = ff->e.pcnt - 4;
    if (UNLIKELY(nargs < 5)) {
      return fterror(ff, Str("insufficient arguments"));
    }
    ff->e.p[4] *= -1;

    for (n = 4; n < nargs; n += 4)
    {
      int j, x1;
      j = (n < 8 ? j = 0 : ff->e.p[n]);
      x1 = j;
      while (j <= ff->e.p[n+4]) {
        MYFLT t;
        t = FindTforX(x1, ff->e.p[n+2], ff->e.p[n+4], j);
        if (j <= nvals)
          fp[j++] = (FL(1.0) - t) * (FL(1.0) - t) * ff->e.p[n+1] +
            FL(2.0) * (FL(1.0) - t) * t * ff->e.p[n+3] + t * t * ff->e.p[n+5];
      }
    }
    return OK;
}

/* utility functions */

inline static MYFLT SolveQuadratic(MYFLT a, MYFLT b, MYFLT c)
{
    MYFLT determinant;
    determinant = b*b - 4*a*c;
    if (determinant >= 0)
      return (-b + SQRT(determinant)) / (FL(2.0)*a);
    else
      return 0;
}

static MYFLT FindTforX(MYFLT x1, MYFLT x2, MYFLT x3, int x)
{
    MYFLT a =  (x1 - FL(2.0)*x2 + x3), b = FL(2.0)* (-x1 + x2), c = x1 - x;
    if (a)
      return SolveQuadratic(a, b, c);
    else
      return (x-x1)/b;
}


static NGFENS quadbezier_fgens[] = {
  { "quadbezier", quadbeziertable },
  { NULL, NULL }
};

FLINKAGE_BUILTIN(quadbezier_fgens)

