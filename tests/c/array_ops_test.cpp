#define __BUILDING_LIBCSOUND

extern "C" {
#include "array_ops.h"
}
#include "gtest/gtest.h"

#include <array>

TEST(ArrayOpsTests, SumAudioArrayStaysWithinOutputBlock)
{
  constexpr int32_t ksmps = 8;
  constexpr int32_t guardSize = 8;
  constexpr MYFLT guardValue = FL(1234.0);
  const std::array<std::array<int32_t, 2>, 7> bounds{{
    {0, 0}, {3, 0}, {0, 2}, {3, 2}, {3, 4}, {7, 0}, {3, 5}
  }};

  std::array<MYFLT, 5 * ksmps> input{};
  for (int32_t channel = 0; channel < 5; ++channel) {
    for (int32_t i = 0; i < ksmps; ++i)
      input[channel * ksmps + i] = (MYFLT)(10 * channel + i);
  }

  for (int32_t channels : {1, 2, 4, 5}) {
    for (auto [offset, early] : bounds) {
      SCOPED_TRACE(::testing::Message() << "channels=" << channels
                   << " offset=" << offset << " early=" << early);
      std::array<MYFLT, ksmps + 2 * guardSize> output;
      output.fill(guardValue);

      int32_t sizes[] = {channels};
      ARRAYDAT array{};
      array.dimensions = 1;
      array.sizes = sizes;
      array.arrayMemberSize = ksmps * sizeof(MYFLT);
      array.data = input.data();

      INSDS instance{};
      instance.ksmps = ksmps;
      instance.ksmps_offset = offset;
      instance.ksmps_no_end = early;

      TABQUERY1 opcode{};
      opcode.h.insdshead = &instance;
      opcode.ans = output.data() + guardSize;
      opcode.tab = &array;

      ASSERT_EQ(tabsuma(nullptr, &opcode), OK);

      for (int32_t i = 0; i < guardSize; ++i) {
        EXPECT_EQ(output[i], guardValue);
        EXPECT_EQ(output[guardSize + ksmps + i], guardValue);
      }
      for (int32_t i = 0; i < ksmps; ++i) {
        MYFLT expected = FL(0.0);
        if (i >= offset && i < ksmps - early)
          expected = (MYFLT)(5 * channels * (channels - 1) + channels * i);
        EXPECT_EQ(opcode.ans[i], expected) << "sample=" << i;
      }
    }
  }
}
