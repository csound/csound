#include "csound.h"
#include "csound_misc.h"
#include "gtest/gtest.h"
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace {
class FtprintCallbacks : public ::testing::TestWithParam<bool> {
protected:
  CSOUND *csound = nullptr;
  std::vector<std::vector<std::string>> tableMessages;

  void record(int32_t attr, const char *message)
  {
    if ((attr & CSOUNDMSG_TYPE_MASK) != CSOUNDMSG_ORCH) return;
    if (std::string(message).rfind("ftable ", 0) == 0)
      tableMessages.emplace_back();
    else if (!tableMessages.empty())
      tableMessages.back().emplace_back(message);
  }

  static void stringMessage(CSOUND *csound, int32_t attr, const char *message)
  {
    static_cast<FtprintCallbacks *>(csoundGetHostData(csound))->record(attr, message);
  }

  static void formattedMessage(CSOUND *csound, int32_t attr,
                               const char *format, va_list args)
  {
    va_list copy;
    va_copy(copy, args);
    const int length = vsnprintf(nullptr, 0, format, copy);
    va_end(copy);
    ASSERT_GE(length, 0);
    std::vector<char> text(static_cast<size_t>(length) + 1);
    vsnprintf(text.data(), text.size(), format, args);
    stringMessage(csound, attr, text.data());
  }

  void SetUp() override
  {
    csound = csoundCreate(this, nullptr);
    if (GetParam()) csoundSetMessageStringCallback(csound, stringMessage);
    else csoundSetMessageCallback(csound, formattedMessage);
  }

  void TearDown() override { csoundDestroy(csound); }

  void run(const char *fixture)
  {
    const auto path = std::filesystem::path(__FILE__).parent_path().parent_path()
      / "commandline" / fixture;
    ASSERT_EQ(0, csoundCompileCSD(csound, path.string().c_str(), 0, 0));
    ASSERT_EQ(0, csoundStart(csound));
    int status = 0;
    for (int block = 0; block < 100 && status == 0; ++block)
      status = csoundPerformKsmps(csound);
    ASSERT_GT(status, 0);
  }
};

TEST_P(FtprintCallbacks, BuffersRowsWithoutLosingLongOutput)
{
  // The CSD covers long rows, single-value rows, a slice and a k-rate trigger.
  ASSERT_NO_FATAL_FAILURE(run("test_ftprint_output.csd"));
  ASSERT_EQ(4u, tableMessages.size());
  std::string expected = "   0: ";
  for (int i = 0; i < 1024; ++i)
    expected += i == 1023 ? "0.0000\n" : "0.0000 ";
  std::string actual;
  for (const auto &chunk : tableMessages[0]) {
    EXPECT_LE(chunk.size(), 1023u); // Fits the string callback's message buffer.
    actual += chunk;
  }
  EXPECT_EQ(expected, actual);
  EXPECT_LE(tableMessages[0].size(), 10u); // A few chunks, not thousands of callbacks.
  EXPECT_EQ((std::vector<std::string>{"   2: 3.0000\n", "   3: 4.0000\n"}), tableMessages[1]);
  EXPECT_EQ((std::vector<std::string>{"   1: 2.0000 \n"}), tableMessages[2]);
  EXPECT_EQ((std::vector<std::string>{"   0: 1.0000 2.0000 3.0000\n",
                                     "   3: 4.0000 5.0000 \n"}), tableMessages[3]);
}

INSTANTIATE_TEST_SUITE_P(HostMessages, FtprintCallbacks, ::testing::Bool(),
  [](const ::testing::TestParamInfo<bool> &info) {
    return info.param ? "StringCallback" : "FormattedCallback";
  });
}
