#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(name)
#include "../../../Opcodes/pvlock.c"

void csound_test_pvlock(int32_t n, int32_t mode, cs_float beta,
                        cs_float *back, cs_float *front, cs_float *previous,
                        void *scratch)
{
    AUXCH data = {0};
    data.auxp = scratch;
    pv_lock_ld(&data, n, mode, beta, back, front, previous);
}
