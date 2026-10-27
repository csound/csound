#define __BUILDING_LIBCSOUND

extern "C" {
#include "array_ops.h"
#include "arrays_internal.h"
#include "csound_standard_types.h"
}
#include "gtest/gtest.h"

#include <cstdarg>
#include <string>

namespace {
class SliceArrayTests : public ::testing::Test {
protected:
  static int32_t perfError(CSOUND *cs, OPDS *, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    cs->MessageV(cs, 0, format, args);
    va_end(args);
    return NOTOK;
  }

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-m4");
    // Direct opcode calls have no running note for PerfError to stop.
    csound->PerfError = perfError;
    input.arrayType = output.arrayType = &CS_VAR_TYPE_K;
    ASSERT_EQ(OK, tabinit(csound, &input, 4, nullptr));
    for (int i = 0; i < 4; ++i) input.data[i] = (cs_float)(10 * (i + 1));
    arguments.count = 1;
    arguments.arg[0] = outputName;
    text.t.outlist = &arguments;
    text.t.outArgCount = 1;
    opcode.h.optext = &text;
    opcode.tab = &output;
    opcode.tabin = &input;
    opcode.start = &start;
    opcode.end = &end;
    opcode.inc = &stride;
  }

  void TearDown() override
  {
    csound_free_array_storage(csound, &input);
    csound_free_array_storage(csound, &output);
    csoundDestroy(csound);
  }

  std::string messages()
  {
    std::string result;
    while (csoundGetMessageCnt(csound)) {
      result += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    return result;
  }

  void expectValues(int length)
  {
    ASSERT_EQ(length, output.sizes[0]);
    for (int i = 0; i < length; ++i)
      EXPECT_EQ(input.data[i], output.data[i]) << "element " << i;
  }

  CSOUND *csound = nullptr;
  ARRAYDAT input{}, output{};
  TABSLICE opcode{};
  OPTXT text{};
  ARGLST arguments{};
  char outputName[7] = "kSlice";
  cs_float start = 0, end = 0, stride = 1;
};

TEST_F(SliceArrayTests, GrowsOnlyToRequestedSizeAndWarnsOnce)
{
  ASSERT_EQ(OK, tabslice(csound, &opcode));
  EXPECT_EQ(sizeof(cs_float), output.allocated);
  EXPECT_EQ(std::string::npos, messages().find("realloc"));

  // Another note can change the global i-rate bound after initialization.
  end = 2;
  ASSERT_EQ(OK, tabslice_perf(csound, &opcode));
  expectValues(3);
  EXPECT_EQ(3 * sizeof(cs_float), output.allocated);
  const std::string warning = messages();
  EXPECT_NE(std::string::npos, warning.find("warning: realloc: slicearray"));
  EXPECT_NE(std::string::npos, warning.find("kSlice[] init 3"));
  EXPECT_NE(std::string::npos, warning.find("audio dropouts"));

  // Shrinking keeps capacity; neither regrowth nor later allocation spams.
  for (int last : {1, 2, 3}) {
    end = (cs_float)last;
    ASSERT_EQ(OK, tabslice_perf(csound, &opcode));
    expectValues(last + 1);
    EXPECT_EQ(std::string::npos, messages().find("realloc"));
  }
}

TEST_F(SliceArrayTests, PreallocatedOutputNeverReallocatesOrWarns)
{
  ASSERT_EQ(OK, tabinit(csound, &output, 4, nullptr));
  ASSERT_EQ(OK, tabslice(csound, &opcode));
  cs_float *const data = output.data;
  // Any attempted buffer reallocation would fail this test.
  auto realloc = csound->ReAlloc;
  csound->ReAlloc = [](CSOUND *, void *, size_t) -> void * { return nullptr; };
  for (int last : {2, 1, 3, 0}) {
    end = (cs_float)last;
    EXPECT_EQ(OK, tabslice_perf(csound, &opcode));
    expectValues(last + 1);
    EXPECT_EQ(data, output.data);
    EXPECT_EQ(4 * sizeof(cs_float), output.allocated);
  }
  csound->ReAlloc = realloc;
  EXPECT_EQ(std::string::npos, messages().find("realloc"));
}

TEST_F(SliceArrayTests, FailedGrowthPreservesOutputAndReportsAnError)
{
  ASSERT_EQ(OK, tabslice(csound, &opcode));
  cs_float *const data = output.data;
  end = 2;
  auto realloc = csound->ReAlloc;
  csound->ReAlloc = [](CSOUND *, void *, size_t) -> void * { return nullptr; };
  EXPECT_EQ(NOTOK, tabslice_perf(csound, &opcode));
  csound->ReAlloc = realloc;
  EXPECT_EQ(data, output.data);
  EXPECT_EQ(sizeof(cs_float), output.allocated);
  expectValues(1);
  const std::string error = messages();
  EXPECT_NE(std::string::npos, error.find("Could not resize array"));
  EXPECT_EQ(std::string::npos, error.find("warning: realloc"));

  ASSERT_EQ(OK, tabslice_perf(csound, &opcode));
  expectValues(3);
  EXPECT_NE(std::string::npos, messages().find("warning: realloc"));
}

TEST_F(SliceArrayTests, ReinitializationAllowsANewGrowthWarning)
{
  ASSERT_EQ(OK, tabslice(csound, &opcode));
  end = 1;
  ASSERT_EQ(OK, tabslice_perf(csound, &opcode));
  EXPECT_NE(std::string::npos, messages().find("warning: realloc"));
  ASSERT_EQ(OK, tabslice(csound, &opcode));
  end = 3;
  ASSERT_EQ(OK, tabslice_perf(csound, &opcode));
  expectValues(4);
  EXPECT_NE(std::string::npos, messages().find("warning: realloc"));
}

TEST_F(SliceArrayTests, GrowthDoesNotBypassTheSharedArrayWriteGuard)
{
  ASSERT_EQ(OK, tabslice(csound, &opcode));
  end = 2;
  auto prepare = csound->ArrayPrepareWrite;
  csound->ArrayPrepareWrite = [](CSOUND *, ARRAYDAT *, INSDS *, int32_t) {
    return NOTOK;
  };
  EXPECT_EQ(NOTOK, tabslice_perf(csound, &opcode));
  csound->ArrayPrepareWrite = prepare;
  EXPECT_EQ(sizeof(cs_float), output.allocated);
  expectValues(1);
  EXPECT_NE(std::string::npos,
            messages().find("Cannot write shared array during performance pass"));
}

TEST_F(SliceArrayTests, GrowthHonorsTheWarningMessageLevel)
{
  csoundSetOption(csound, "-m0");
  ASSERT_EQ(OK, tabslice(csound, &opcode));
  end = 2;
  ASSERT_EQ(OK, tabslice_perf(csound, &opcode));
  expectValues(3);
  EXPECT_EQ(std::string::npos, messages().find("warning: realloc"));
}
} // namespace
