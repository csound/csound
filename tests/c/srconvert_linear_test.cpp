#define __BUILDING_LIBCSOUND

#include <array>

extern "C" {
#include "srconvert.h"
}

#include "gtest/gtest.h"

namespace {

TEST(SrcLinearProcess, OneSampleBlockKeepsHistoryInsideInput) {
  const cs_float sentinel = -12345.f;
  const cs_float input = 1.f;
  std::array<cs_float, 2> in = {sentinel, input};
  std::array<cs_float, 6> staged{};
  std::array<cs_float, 1> out{};
  cs_float history = 0.f;

  CVTDAT dat{};
  dat.bufferin = reinterpret_cast<float *>(staged.data());
  dat.data = &history;

  CS_VARIABLE var{};
  var.varType = &CS_VAR_TYPE_K;

  SR_CONVERTER converter{};
  converter.ratio = 6.f;
  converter.insize = 1;
  converter.outsize = 1;
  converter.blocks = 6;
  converter.mode = 4;
  converter.dat = &dat;
  converter.ncvt = 1;
  converter.var = &var;

  ASSERT_EQ(src_convert(nullptr, &converter, in.data() + 1, out.data()), 0);
  EXPECT_EQ(history, input);
}

}  // namespace
