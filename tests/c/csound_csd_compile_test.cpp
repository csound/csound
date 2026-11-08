#include "csound.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {
class CsdCompileTests : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;
  std::filesystem::path directory;

  void SetUp() override
  {
#ifdef _WIN32
    const int processId = _getpid();
#else
    const int processId = getpid();
#endif
    directory = std::filesystem::path(::testing::TempDir()) /
      ("csound-csd-source-" + std::to_string(processId) + "-" +
       ::testing::UnitTest::GetInstance()->current_test_info()->name());
    ASSERT_TRUE(std::filesystem::create_directory(directory));
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(nullptr, csound);
    csoundCreateMessageBuffer(csound, 0);
    ASSERT_EQ(0, csoundSetOption(csound, "-n -d -m0 --csd-line-nums=1"));
  }

  void TearDown() override
  {
    if (csound) csoundDestroy(csound);
    if (!directory.empty()) std::filesystem::remove_all(directory);
  }

  std::string csd(const std::string &orchestra = "instr 1\nendin\n",
                  const std::string &options = "")
  {
    return "<CsoundSynthesizer>\n" + options + "<CsInstruments>\n"
      "sr=44100\nksmps=32\nnchnls=1\n" + orchestra +
      "</CsInstruments>\n<CsScore>\ne\n</CsScore>\n</CsoundSynthesizer>\n";
  }

  void write(const std::string &name, const std::string &source)
  {
    std::ofstream file(directory / name, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file << source;
    file.close();
    ASSERT_TRUE(file.good());
  }

  std::string path(const std::string &name)
  {
    return (directory / name).generic_string();
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

  const std::string invalidOrchestra =
    "instr 1\nprints(\"bad\" \"syntax\")\nendin\n";

  // The orchestra in csd() says ksmps=32.
  const std::string withKsmps16 =
    csd("instr 1\nendin\n", "<CsOptions>\n--ksmps=16\n</CsOptions>\n");
};

TEST_F(CsdCompileTests, FileOrchestraErrorNamesCsd)
{
  ASSERT_NO_FATAL_FAILURE(write("my piece.csd", csd(invalidOrchestra)));
  EXPECT_NE(0, csoundCompileCSD(csound, path("my piece.csd").c_str(), 0, 0));
  const auto output = messages();
  EXPECT_NE(std::string::npos, output.find("syntax error")) << output;
  EXPECT_NE(std::string::npos,
            output.find("from file " + path("my piece.csd"))) << output;
  EXPECT_EQ(std::string::npos, output.find("*string*")) << output;
}

TEST_F(CsdCompileTests, IncludeErrorNamesIncludeAndCsd)
{
  ASSERT_NO_FATAL_FAILURE(write("bid.udo", invalidOrchestra));
  const auto includePath = "--env:INCDIR=\"" + directory.generic_string() + "\"";
  ASSERT_EQ(0, csoundSetOption(csound, includePath.c_str()));
  ASSERT_NO_FATAL_FAILURE(
    write("my piece.csd", csd("#include \"bid.udo\"\n")));
  EXPECT_NE(0, csoundCompileCSD(csound, path("my piece.csd").c_str(), 0, 0));
  const auto output = messages();
  EXPECT_NE(std::string::npos, output.find("from file bid.udo")) << output;
  EXPECT_NE(std::string::npos,
            output.find("from file " + path("my piece.csd"))) << output;
  EXPECT_EQ(std::string::npos, output.find("*string*")) << output;
}

TEST_F(CsdCompileTests, TextOrchestraErrorKeepsStringName)
{
  EXPECT_NE(0, csoundCompileCSD(csound, csd(invalidOrchestra).c_str(), 1, 0));
  const auto output = messages();
  EXPECT_NE(std::string::npos, output.find("from file *string*")) << output;
}

TEST_F(CsdCompileTests, FileThenTextDoesNotReuseFileName)
{
  ASSERT_NO_FATAL_FAILURE(write("first.csd", csd()));
  ASSERT_EQ(0, csoundCompileCSD(csound, path("first.csd").c_str(), 0, 0))
    << messages();
  messages();
  EXPECT_NE(0, csoundCompileCSD(csound, csd(invalidOrchestra).c_str(), 1, 0));
  const auto output = messages();
  EXPECT_NE(std::string::npos, output.find("from file *string*")) << output;
  EXPECT_EQ(std::string::npos, output.find(path("first.csd"))) << output;
}

TEST_F(CsdCompileTests, TextThenFileUsesFileName)
{
  ASSERT_EQ(0, csoundCompileCSD(csound, csd().c_str(), 1, 0)) << messages();
  messages();
  ASSERT_NO_FATAL_FAILURE(write("second.csd", csd(invalidOrchestra)));
  EXPECT_NE(0, csoundCompileCSD(csound, path("second.csd").c_str(), 0, 0));
  const auto output = messages();
  EXPECT_NE(std::string::npos,
            output.find("from file " + path("second.csd"))) << output;
  EXPECT_EQ(std::string::npos, output.find("*string*")) << output;
}

TEST_F(CsdCompileTests, FileThenFileUsesNewName)
{
  ASSERT_NO_FATAL_FAILURE(write("first.csd", csd()));
  ASSERT_EQ(0, csoundCompileCSD(csound, path("first.csd").c_str(), 0, 0))
    << messages();
  messages();
  ASSERT_NO_FATAL_FAILURE(write("second.csd", csd(invalidOrchestra)));
  EXPECT_NE(0, csoundCompileCSD(csound, path("second.csd").c_str(), 0, 0));
  const auto output = messages();
  EXPECT_NE(std::string::npos,
            output.find("from file " + path("second.csd"))) << output;
  EXPECT_EQ(std::string::npos, output.find(path("first.csd"))) << output;
  EXPECT_EQ(std::string::npos, output.find("*string*")) << output;
}

TEST_F(CsdCompileTests, EmptyOptionLeavesFileNameToTheCsd)
{
  ASSERT_NO_FATAL_FAILURE(write("piece.csd", csd()));
  ASSERT_EQ(0, csoundSetOption(csound, ""));
  const auto file = path("piece.csd");
  const char *arguments[] = {"csound", file.c_str()};
  EXPECT_EQ(CSOUND_SUCCESS, csoundCompile(csound, 2, arguments)) << messages();
}

TEST_F(CsdCompileTests, CsOptionsAreTakenByDefault)
{
  ASSERT_EQ(0, csoundCompileCSD(csound, withKsmps16.c_str(), 1, 0))
    << messages();
  EXPECT_EQ(16u, csoundGetKsmps(csound));
}

TEST_F(CsdCompileTests, IgnoreCsoptsSkipsCsOptionsInText)
{
  ASSERT_EQ(0, csoundSetOption(csound, "-+ignore_csopts=1"));
  ASSERT_EQ(0, csoundCompileCSD(csound, withKsmps16.c_str(), 1, 0))
    << messages();
  EXPECT_EQ(32u, csoundGetKsmps(csound));
}

TEST_F(CsdCompileTests, IgnoreCsoptsSkipsCsOptionsInFile)
{
  ASSERT_NO_FATAL_FAILURE(write("options.csd", withKsmps16));
  ASSERT_EQ(0, csoundSetOption(csound, "-+ignore_csopts=1"));
  ASSERT_EQ(0, csoundCompileCSD(csound, path("options.csd").c_str(), 0, 0))
    << messages();
  EXPECT_EQ(32u, csoundGetKsmps(csound));
}
}
