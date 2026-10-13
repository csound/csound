/* Check the portable conversion helpers used when lrint and SSE are absent. */
#undef USE_LRINT
#include "csdl.h"

int main(void)
{
    /* Values above the 32-bit range must still fit in the 64-bit result. */
    if (CS_FLOAT2LONG64((cs_double)4294967296.0) != INT64_C(4294967296) ||
        CS_FLOAT2LONG64((cs_double)-4294967296.0) != -INT64_C(4294967296))
        return 1;
    if (CS_FLOAT2LONG64((cs_double)2.75) != 2 ||
        CS_FLOAT2LONG64((cs_double)-2.75) != -2)
        return 1;
    if (CS_FLOAT2LRND64((cs_double)4294967296.0) != INT64_C(4294967296) ||
        CS_FLOAT2LRND64((cs_double)-4294967296.0) != -INT64_C(4294967296))
        return 1;
    if (CS_FLOAT2LRND64((cs_double)2.5) != 3 ||
        CS_FLOAT2LRND64((cs_double)-2.5) != -3 ||
        CS_FLOAT2LRND64((cs_double)2.25) != 2 ||
        CS_FLOAT2LRND64((cs_double)-2.25) != -2)
        return 1;
    return 0;
}
