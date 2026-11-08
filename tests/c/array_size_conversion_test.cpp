#define __BUILDING_LIBCSOUND
#include <cmath>
#include <limits>
#include "csoundCore.h"
#include "gtest/gtest.h"

#include "arrays.h"

TEST(ArraySizeConversion, RejectsInvalidSizesWithoutChangingOutput)
{
    const cs_float invalid[] = {
      std::numeric_limits<cs_float>::quiet_NaN(),
      std::numeric_limits<cs_float>::infinity(),
      -std::numeric_limits<cs_float>::infinity(),
      FL(-1.0), FL(2147483648.0)
    };
    for (cs_float input : invalid) {
      int32_t size = 17;
      EXPECT_EQ(NOTOK, csound_array_size_to_int32(input, &size));
      EXPECT_EQ(17, size);
    }
    EXPECT_EQ(NOTOK, csound_array_size_to_int32(FL(1.0), nullptr));
}

TEST(ArraySizeConversion, AcceptsAndTruncatesSizesWithinRange)
{
    const cs_float valid[] = {FL(0.0), FL(-0.0), FL(0.75), FL(17.75)};
    const int32_t expected[] = {0, 0, 0, 17};
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i) {
      int32_t size = -1;
      EXPECT_EQ(OK, csound_array_size_to_int32(valid[i], &size));
      EXPECT_EQ(expected[i], size);
    }
    cs_float upper = std::nextafter(FL(2147483648.0), FL(0.0));
    int32_t size = -1;
    EXPECT_EQ(OK, csound_array_size_to_int32(upper, &size));
    EXPECT_EQ(static_cast<int32_t>(upper), size);
}
