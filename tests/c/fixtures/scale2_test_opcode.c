#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(name)
#include "../../../Opcodes/ugakbari.c"

void csound_test_scale2_step(cs_double coefficient, cs_double pole,
                             cs_float target, cs_double *state,
                             cs_double *correction)
{
    SCALE2 p = {0};
    cs_float zero = FL(0.0), one = FL(1.0), output;
    p.kinval = p.imin = &zero;
    p.imax = &one;
    p.kmin = p.kmax = &target;
    p.koutval = &output;
    p.c1 = coefficient;
    p.c2 = pole;
    p.yt1 = *state;
    p.correction = *correction;
    scale2_process(NULL, &p);
    *state = p.yt1;
    *correction = p.correction;
}
