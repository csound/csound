#include "csound.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <sndfile.h>
#include <fstream>
#include <iterator>
#include <string>
#ifdef _WIN32
#include <process.h>
#else
#include <sys/resource.h>
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

  // Performs a CSD that holds `licence` into a WAV file, and returns the
  // copyright the file was given.
  std::string writtenCopyright(const std::string &licence)
  {
    const auto wav = path("licence.wav");
    const auto text = csd("instr 1\nendin\n", licence +
                          "<CsOptions>\n-W -o \"" + wav + "\"\n</CsOptions>\n");
    if (csoundCompileCSD(csound, text.c_str(), 1, 0) != 0 ||
        csoundStart(csound) != 0)
      return "not performed: " + messages();
    while (csoundPerformKsmps(csound) == 0)
      ;
    csoundReset(csound);
    SF_INFO info = {};
    SNDFILE *file = sf_open(wav.c_str(), SFM_READ, &info);
    if (file == nullptr)
      return "no file";
    const char *copyright = sf_get_string(file, SF_STR_COPYRIGHT);
    std::string result = copyright != nullptr ? copyright : "";
    sf_close(file);
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

// On a started engine csoundCompileCSD skips <CsOptions>, by the rules that read it.
TEST_F(CsdCompileTests, TagsInACommentDoNotEndCsOptionsOnAStartedEngine)
{
  ASSERT_EQ(0, csoundStart(csound));
  const auto text = csd("instr 1\nendin\n",
                        "<CsOptions>\n--ksmps=16 /* a comment\n</CsOptions>\n"
                        "</CsoundSynthesizer>\n*/\n</CsOptions>\n");
  EXPECT_EQ(0, csoundCompileCSD(csound, text.c_str(), 1, 0)) << messages();
}

TEST_F(CsdCompileTests, UnterminatedCommentFailsOnAStartedEngine)
{
  ASSERT_EQ(0, csoundStart(csound));
  const auto text = csd("instr 1\nendin\n", "<CsOptions>\n/* never closed\n</CsOptions>\n");
  EXPECT_NE(0, csoundCompileCSD(csound, text.c_str(), 1, 0));
  const auto output = messages();
  EXPECT_NE(std::string::npos, output.find("Missing end tag </CsOptions>")) << output;
}

TEST_F(CsdCompileTests, CommentClosedAcrossAChunkEndsOnAStartedEngine)
{
  ASSERT_EQ(0, csoundStart(csound));
  const auto text = csd("instr 1\nendin\n",
                        "<CsOptions>\n/* a comment\n</CsOptions>\n"
                        "</CsoundSynthesizer>\n" + std::string(4094, 'x') +
                        "*/\n</CsOptions>\n");
  EXPECT_EQ(0, csoundCompileCSD(csound, text.c_str(), 1, 0)) << messages();
}

TEST_F(CsdCompileTests, CommentAfterALongCommentOpensOnAStartedEngine)
{
  ASSERT_EQ(0, csoundStart(csound));
  const auto text = csd("instr 1\nendin\n",
                        "<CsOptions>\n/* " + std::string(4200, 'x') + "\n*/\n"
                        "/* the second\n</CsOptions>\n</CsoundSynthesizer>\n*/\n"
                        "</CsOptions>\n");
  EXPECT_EQ(0, csoundCompileCSD(csound, text.c_str(), 1, 0)) << messages();
}

#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
// A second <CsScore bin=...> frees the first's file name with Csound's
// allocator: free() aborted. In a child, with an alarm in case it hangs.
TEST_F(CsdCompileTests, SecondExternalScoreDoesNotAbort)
{
  const std::string score = "<CsScore bin=\"cp\">\ni 1 0 0\n</CsScore>\n";
  const auto text = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\nendin\n"
    "</CsInstruments>\n" + score + score + "</CsoundSynthesizer>\n";
  ASSERT_EXIT(([&] {
      alarm(10);
      exit(csoundCompileCSD(csound, text.c_str(), 1, 0) == 0 ? 0 : 1);
  }()), ::testing::ExitedWithCode(0), "");
}

// When a <CsScore bin=...> on a started engine cannot make its temporary
// name, the name it freed is not freed again by the next one.
TEST_F(CsdCompileTests, ExternalScoreAfterAFailedOneDoesNotAbort)
{
  const char *text = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\nendin\n"
    "</CsInstruments>\n<CsScore bin=\"cp\">\ni 1 0 0\n</CsScore>\n"
    "</CsoundSynthesizer>\n";
  const auto missing = path("missing");
  const auto present = path("");
  ASSERT_EXIT(([&] {
      alarm(10);
      if (csoundStart(csound) != 0) exit(2);
      if (csoundCompileCSD(csound, text, 1, 0) != 0) exit(3);
      if (setenv("TMPDIR", missing.c_str(), 1) != 0) exit(4);
      if (csoundCompileCSD(csound, text, 1, 0) == 0) exit(5);
      if (setenv("TMPDIR", present.c_str(), 1) != 0) exit(6);
      exit(csoundCompileCSD(csound, text, 1, 0) == 0 ? 0 : 1);
  }()), ::testing::ExitedWithCode(0), "");
}
#endif

// A started engine has its MIDI input: a later CSD skips <CsMidifileB>.
TEST_F(CsdCompileTests, MidifileBIsSkippedOnAStartedEngine)
{
  ASSERT_EQ(0, csoundStart(csound));
  const auto text = csd("instr 1\nendin\n",
                        "<CsMidifileB>\n"
                        "TVRoZAAAAAYAAAABAGBNVHJrAAAADACQPGRggDwAAP8vAA==\n"
                        "</CsMidifileB>\n");
  EXPECT_EQ(0, csoundCompileCSD(csound, text.c_str(), 1, 0));
  const auto output = messages();
  EXPECT_NE(std::string::npos, output.find("Skipping <CsMidifileB>")) << output;
}

#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
// <CsFile> writes its lines once, and ends only at a line that starts with
// its end tag. It used to write its first line for ever:
// in a child with its file size capped, that fails instead of filling a disk.
TEST_F(CsdCompileTests, CsFileWritesItsLines)
{
  const auto file = path("plain.txt");
  // Its second line is read in two chunks, the second starting with the tag.
  const auto body = "hello\n" + std::string(1023, 'x') +
    "</CsFile> stays text\nworld\n";
  const auto text = "<CsoundSynthesizer>\n<CsFile filename=\"" + file +
    "\">\n" + body + "</CsFile>\n<CsInstruments>\ninstr 1\nendin\n"
    "</CsInstruments>\n</CsoundSynthesizer>\n";
  ASSERT_EXIT(([&] {
      struct rlimit limit;
      limit.rlim_cur = limit.rlim_max = 1 << 20;
      if (setrlimit(RLIMIT_FSIZE, &limit) != 0) exit(2);
      if (csoundCompileCSD(csound, text.c_str(), 1, 0) != 0) exit(3);
      std::ifstream in(file, std::ios::binary);
      const std::string written((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
      exit(written == body ? 0 : 1);
  }()), ::testing::ExitedWithCode(0), "");
}
#endif

TEST_F(CsdCompileTests, ShortLicenseSetsTheCopyright)
{
  const auto copyright =
    writtenCopyright("<CsShortLicense>\n6\n</CsShortLicense>\n");
  EXPECT_NE(std::string::npos,
            copyright.find("Creative Commons Attribution\nCC BY\n"))
    << copyright;
}

TEST_F(CsdCompileTests, IgnoreCsoptsSkipsCsOptionsInFile)
{
  ASSERT_NO_FATAL_FAILURE(write("options.csd", withKsmps16));
  ASSERT_EQ(0, csoundSetOption(csound, "-+ignore_csopts=1"));
  ASSERT_EQ(0, csoundCompileCSD(csound, path("options.csd").c_str(), 0, 0))
    << messages();
  EXPECT_EQ(32u, csoundGetKsmps(csound));
}

#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
// A CSD whose last byte is a bare CR, with no </CsoundSynthesizer>, is read
// to its end. It used to be read for ever: in a child with an alarm, that
// fails instead of hanging the tests.
TEST_F(CsdCompileTests, FinalCarriageReturnEndsTheRead)
{
  const std::string text = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\n"
    "endin\n</CsInstruments>\n<CsScore>\ne\n</CsScore>\r";
  ASSERT_EXIT(([&] {
      alarm(10);
      exit(csoundCompileCSD(csound, text.c_str(), 1, 0) == 0 ? 0 : 1);
  }()), ::testing::ExitedWithCode(0), "");
}
#endif
}
