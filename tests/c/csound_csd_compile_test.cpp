#include "csound.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <sndfile.h>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>
#include <utility>
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

// <CsFile>'s end tag after a line's leading blanks, which fill a chunk of
// 1023 bytes or leave its end to cut the tag. The blanks are written, as a
// chunk of them always was. In a child with its file size capped, as above.
TEST_F(CsdCompileTests, CsFileEndsAtATagAfterLongBlanks)
{
  const std::pair<std::string, size_t> files[] = {
    {path("filled.txt"), 1023}, {path("cut.txt"), 1020}};
  std::string text = "<CsoundSynthesizer>\n";
  for (const auto &[file, blanks] : files)
    text += "<CsFile filename=\"" + file + "\">\nhello\n" +
      std::string(blanks, ' ') + "</CsFile>\n";
  text += "<CsInstruments>\ninstr 1\nendin\n</CsInstruments>\n"
    "</CsoundSynthesizer>\n";
  ASSERT_EXIT(([&] {
      struct rlimit limit;
      limit.rlim_cur = limit.rlim_max = 1 << 20;
      if (setrlimit(RLIMIT_FSIZE, &limit) != 0) exit(2);
      if (csoundCompileCSD(csound, text.c_str(), 1, 0) != 0) exit(3);
      for (const auto &[file, blanks] : files) {
        std::ifstream in(file, std::ios::binary);
        const std::string written((std::istreambuf_iterator<char>(in)),
                                  std::istreambuf_iterator<char>());
        if (written != "hello\n" + std::string(blanks, ' ')) exit(1);
      }
      exit(0);
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

TEST_F(CsdCompileTests, ShortLicenceOneEndsAtItsName)
{
  const auto copyright =
    writtenCopyright("<CsShortLicence>\n1\n</CsShortLicence>\n");
  const std::string name =
    "Creative Commons Attribution-NonCommercial-NoDerivatives\nCC BY-NC-ND\n";
  ASSERT_GE(copyright.size(), name.size()) << copyright;
  EXPECT_EQ(name, copyright.substr(copyright.size() - name.size()));
}

// A line longer than 4095 bytes is one line: its rest is no licence number.
TEST_F(CsdCompileTests, ShortLicenceOnALongLineIsItsNumber)
{
  const auto copyright = writtenCopyright(
    "<CsShortLicence>\n6" + std::string(4094, ' ') + "\n</CsShortLicence>\n");
  EXPECT_NE(std::string::npos,
            copyright.find("Creative Commons Attribution\nCC BY\n"))
    << copyright;
}

TEST_F(CsdCompileTests, NoInstrumentsIsSaid)
{
  const char *text = "<CsoundSynthesizer>\n<CsScore>\ne\n</CsScore>\n"
    "</CsoundSynthesizer>\n";
  EXPECT_NE(0, csoundCompileCSD(csound, text, 1, 0));
  const auto output = messages();
  EXPECT_NE(std::string::npos,
            output.find("CSD file *string* has no <CsInstruments>")) << output;
}

TEST_F(CsdCompileTests, IgnoreCsoptsSkipsCsOptionsInFile)
{
  ASSERT_NO_FATAL_FAILURE(write("options.csd", withKsmps16));
  ASSERT_EQ(0, csoundSetOption(csound, "-+ignore_csopts=1"));
  ASSERT_EQ(0, csoundCompileCSD(csound, path("options.csd").c_str(), 0, 0))
    << messages();
  EXPECT_EQ(32u, csoundGetKsmps(csound));
}

// The end tag of a skipped <CsMidifileB> can follow its data on one line, as
// it can when the section is read.
TEST_F(CsdCompileTests, MidifileBEndTagAfterItsDataIsSkippedOnAStartedEngine)
{
  ASSERT_EQ(0, csoundStart(csound));
  const auto text = csd("instr 1\nendin\n",
                        "<CsMidifileB>\nAAAA</CsMidifileB>\n");
  EXPECT_EQ(0, csoundCompileCSD(csound, text.c_str(), 1, 0)) << messages();
}

// So can a tag that the end of a chunk of a long line cuts: here the first
// chunk's 4095 bytes end with "</C".
TEST_F(CsdCompileTests, MidifileBEndTagCutByAChunkIsSkippedOnAStartedEngine)
{
  ASSERT_EQ(0, csoundStart(csound));
  const auto text = csd("instr 1\nendin\n", "<CsMidifileB>\n" +
                        std::string(4092, 'A') + "</CsMidifileB>\n");
  EXPECT_EQ(0, csoundCompileCSD(csound, text.c_str(), 1, 0)) << messages();
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

// On a started engine, each CSD counts its lines from its own start, not on
// from the CSDs compiled before it.
TEST_F(CsdCompileTests, EachCsdCountsItsOwnLinesOnAStartedEngine)
{
  ASSERT_EQ(0, csoundStart(csound));
  const auto lineOfError = [&]() {
    const char *text = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\n"
      "prints(\"bad\" \"syntax\")\nendin\n</CsInstruments>\n"
      "</CsoundSynthesizer>\n";
    EXPECT_NE(0, csoundCompileCSD(csound, text, 1, 0));
    const auto output = messages();
    std::smatch line;
    return std::regex_search(output, line, std::regex("line [0-9]+")) ?
      line.str() : "no line in: " + output;
  };
  const auto first = lineOfError();
  const auto second = lineOfError();
  EXPECT_EQ(first, second);
}

// A base64 section before <CsInstruments> leaves the orchestra's line numbers
// as blank lines in its place do: registering its file reset the CSD's count.
TEST_F(CsdCompileTests, Base64SectionKeepsLineNumbers)
{
  const auto lineOfError = [&](const std::string &before) {
    const auto text = "<CsoundSynthesizer>\n" + before + "<CsInstruments>\n"
      "instr 1\nprints(\"bad\" \"syntax\")\nendin\n</CsInstruments>\n"
      "</CsoundSynthesizer>\n";
    EXPECT_NE(0, csoundCompileCSD(csound, text.c_str(), 1, 0));
    const auto output = messages();
    std::smatch line;
    return std::regex_search(output, line, std::regex("line [0-9]+")) ?
      line.str() : "no line in: " + output;
  };
  const auto plain = lineOfError("\n\n\n");
  const auto withFile = lineOfError("<CsFileB filename=\"" + path("b64.bin") +
                                    "\">\naGk=\n</CsFileB>\n");
  EXPECT_EQ(plain, withFile);
}

#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
// The temporary files of a CSD's sections go at a reset, mkstemp's empty
// file beside each of them too. In a child, for its TMPDIR.
TEST_F(CsdCompileTests, TemporaryFilesLeaveNothing)
{
  const auto tmpdir = path("tmp");
  ASSERT_TRUE(std::filesystem::create_directory(tmpdir));
  const char *text = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\nendin\n"
    "</CsInstruments>\n<CsScore bin=\"cp\">\ni 1 0 0\n</CsScore>\n"
    "<CsMidifileB>\nAAAA\n</CsMidifileB>\n</CsoundSynthesizer>\n";
  ASSERT_EXIT(([&] {
      if (setenv("TMPDIR", tmpdir.c_str(), 1) != 0) exit(2);
      if (csoundCompileCSD(csound, text, 1, 0) != 0) exit(3);
      csoundReset(csound);
      exit(std::filesystem::is_empty(tmpdir) ? 0 : 1);
  }()), ::testing::ExitedWithCode(0), "");
}
#endif

#if !defined(_WIN32)
// An indented </CsScore> ends an external score, as it ends a score.
TEST_F(CsdCompileTests, IndentedEndTagEndsAnExternalScore)
{
  const char *text = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\nendin\n"
    "</CsInstruments>\n<CsScore bin=\"cp\">\ni 1 0 0\n  </CsScore>\n"
    "</CsoundSynthesizer>\n";
  EXPECT_EQ(0, csoundCompileCSD(csound, text, 1, 0)) << messages();
}

// So does one after a line's leading blanks, which fill a chunk or leave
// its end to cut the tag. One external score per instance: where a second
// one aborts (its first's name freed with free()), the test still fails
// cleanly.
TEST_F(CsdCompileTests, EndTagAfterLongBlanksEndsAnExternalScore)
{
  for (const size_t blanks : {4095, 4090}) {
    const auto text = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\n"
      "endin\n</CsInstruments>\n<CsScore bin=\"cp\">\ni 1 0 0\n" +
      std::string(blanks, ' ') + "</CsScore>\n</CsoundSynthesizer>\n";
    CSOUND *instance = csoundCreate(nullptr, nullptr);
    ASSERT_NE(nullptr, instance);
    csoundSetOption(instance, "-n -d -m0");
    EXPECT_EQ(0, csoundCompileCSD(instance, text.c_str(), 1, 0))
      << blanks << " blanks";
    csoundDestroy(instance);
  }
}
#endif

#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
// An external score with no end tag fails and leaves no file of its own. In
// a child, for its TMPDIR.
TEST_F(CsdCompileTests, UnendedExternalScoreLeavesNoFile)
{
  const auto tmpdir = path("tmp");
  ASSERT_TRUE(std::filesystem::create_directory(tmpdir));
  const char *text = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\nendin\n"
    "</CsInstruments>\n<CsScore bin=\"cp\">\ni 1 0 0\n";
  ASSERT_EXIT(([&] {
      if (setenv("TMPDIR", tmpdir.c_str(), 1) != 0) exit(2);
      if (csoundCompileCSD(csound, text, 1, 0) == 0) exit(3);
      csoundReset(csound);
      exit(std::filesystem::is_empty(tmpdir) ? 0 : 1);
  }()), ::testing::ExitedWithCode(0), "");
}
#endif

#if !defined(_WIN32)
// An external score before <CsInstruments> leaves the orchestra's line
// numbers as blank lines in its place do: reading its generated file back
// counted that file's lines as the CSD's.
TEST_F(CsdCompileTests, ExternalScoreKeepsLineNumbers)
{
  const auto lineOfError = [&](const std::string &before) {
    const auto text = "<CsoundSynthesizer>\n" + before + "<CsInstruments>\n"
      "instr 1\nprints(\"bad\" \"syntax\")\nendin\n</CsInstruments>\n"
      "</CsoundSynthesizer>\n";
    EXPECT_NE(0, csoundCompileCSD(csound, text.c_str(), 1, 0));
    const auto output = messages();
    std::smatch line;
    return std::regex_search(output, line, std::regex("line [0-9]+")) ?
      line.str() : "no line in: " + output;
  };
  const auto plain = lineOfError("\n\n\n\n");
  const auto withScore = lineOfError("<CsScore bin=\"cp\">\ni 1 0 1\ni 1 1 1\n"
                                     "</CsScore>\n");
  EXPECT_EQ(plain, withScore);
}
#endif

// A score compiled on a started engine gets no closing e, which would end
// the performance: with no </CsoundSynthesizer>, or in several sections.
TEST_F(CsdCompileTests, ScoreOnAStartedEngineDoesNotEndIt)
{
  ASSERT_EQ(0, csoundStart(csound));
  const std::string orchestra = "<CsoundSynthesizer>\n<CsInstruments>\n"
    "instr 1\nendin\n</CsInstruments>\n";
  const std::string score = "<CsScore>\ni 1 0 1\n</CsScore>\n";
  const std::string texts[] = {
    orchestra + score,
    orchestra + score + score + "</CsoundSynthesizer>\n"};
  for (const auto &text : texts) {
    ASSERT_EQ(0, csoundCompileCSD(csound, text.c_str(), 1, 0)) << messages();
    int ended = 0;
    for (int i = 0; i < 20 && !ended; i++)
      ended = csoundPerformKsmps(csound) != 0;
    EXPECT_EQ(0, ended) << text;
  }
}

// Before the start, a second CSD's score is queued, as a second
// csoundEventString's is, and plays at its times: it was sorted and
// dropped. Queued async, its notes follow its orchestra into the engine.
TEST_F(CsdCompileTests, SecondScoreBeforeTheStartPlays)
{
  const char *first = "<CsoundSynthesizer>\n<CsInstruments>\n"
    "sr=48000\nksmps=32\nnchnls=1\n0dbfs=1\n"
    "instr 1\nchnset chnget:i(\"played\") + 1, \"played\"\nendin\n"
    "</CsInstruments>\n<CsScore>\ni 1 0 0.1\n</CsScore>\n"
    "</CsoundSynthesizer>\n";
  const char *second = "<CsoundSynthesizer>\n<CsInstruments>\n"
    "instr 2\nchnset chnget:i(\"played\") + 10, \"played\"\nendin\n"
    "</CsInstruments>\n<CsScore>\ni 1 0.01 0\ni 2 0.02 0\ne\n</CsScore>\n"
    "</CsoundSynthesizer>\n";
  for (const int async : {0, 1}) {
    CSOUND *instance = csoundCreate(nullptr, nullptr);
    ASSERT_NE(nullptr, instance);
    csoundSetOption(instance, "-n -d -m0");
    ASSERT_EQ(0, csoundCompileCSD(instance, first, 1, 0));
    ASSERT_EQ(0, csoundCompileCSD(instance, second, 1, async));
    ASSERT_EQ(0, csoundStart(instance));
    // A cycle is 32 samples at 48 kHz, so the notes at 0.01 s and 0.02 s
    // play at cycles 15 and 30: look before, between and after them.
    int cycles = 0;
    double before = -1, between = -1, after = -1;
    while (cycles < 1000 && csoundPerformKsmps(instance) == 0) {
      cycles++;
      const double played =
        csoundGetControlChannel(instance, "played", nullptr);
      if (cycles == 10) before = played;
      if (cycles == 20) between = played;
      if (cycles == 40) after = played;
    }
    EXPECT_EQ(1, before) << "async " << async;
    EXPECT_EQ(2, between) << "async " << async;
    EXPECT_EQ(12, after) << "async " << async;
    EXPECT_EQ(12, csoundGetControlChannel(instance, "played", nullptr))
      << "async " << async;
    EXPECT_EQ(150, cycles) << "async " << async; /* the first score's 0.1 s */
    csoundDestroy(instance);
  }
}

// A CSD with no score, compiled on a started engine, queues nothing: it
// queued "#exit", which the realtime reader refused as a line.
TEST_F(CsdCompileTests, NoScoreOnAStartedEngineQueuesNothing)
{
  ASSERT_EQ(0, csoundStart(csound));
  const char *text = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\nendin\n"
    "</CsInstruments>\n</CsoundSynthesizer>\n";
  for (const int async : {0, 1}) {
    ASSERT_EQ(0, csoundCompileCSD(csound, text, 1, async)) << messages();
    for (int i = 0; i < 2; i++)
      ASSERT_EQ(0, csoundPerformKsmps(csound));
    const auto output = messages();
    EXPECT_EQ(std::string::npos, output.find("illegal RT scoreline"))
      << "async " << async << ": " << output;
  }
}

// A byte 0xFF is a character of a CSD, not its end: a line starting with one,
// here in a Latin-1 comment, ended the orchestra.
TEST_F(CsdCompileTests, ByteFFIsNoEndOfTheCsd)
{
  const auto text = csd("/*\n\xff in a Latin-1 comment\n*/\ninstr 1\nendin\n");
  EXPECT_EQ(0, csoundCompileCSD(csound, text.c_str(), 1, 0)) << messages();
}

// A byte 0xFF in a score's string is kept, in a CSD's score and in a score
// it includes: the CSD's reader dropped it, and the score's reader took it
// for the end of the score, losing the next statement.
TEST_F(CsdCompileTests, ByteFFInAScoreStringIsKept)
{
  ASSERT_NO_FATAL_FAILURE(
    write("ff.sco", "i 1 0 0 \"caf\xff" "e\"\ni 1 0 0 \"abcd\"\n"));
  const auto text = "<CsoundSynthesizer>\n<CsInstruments>\ninstr 1\n"
    "S1 = p4\nprints \"len %d\\n\", strlen(S1)\nendin\n</CsInstruments>\n"
    "<CsScore>\ni 1 0 0 \"caf\xff" "e\"\n#include \"" + path("ff.sco") +
    "\"\n</CsScore>\n</CsoundSynthesizer>\n";
  ASSERT_EQ(0, csoundCompileCSD(csound, text.c_str(), 1, 0)) << messages();
  ASSERT_EQ(0, csoundStart(csound)) << messages();
  while (csoundPerformKsmps(csound) == 0)
    ;
  const auto output = messages();
  std::string lengths;
  const std::regex length("len [0-9]+");
  for (std::sregex_iterator i(output.begin(), output.end(), length), end;
       i != end; ++i)
    lengths += i->str() + "\n";
  EXPECT_EQ("len 5\nlen 5\nlen 4\n", lengths) << output;
}

// A byte 0xFF in base64 data is no base64, as any other byte that is not:
// the data ended there, and the file was written short without a word.
TEST_F(CsdCompileTests, ByteFFInBase64DataIsAnError)
{
  const auto text = csd("instr 1\nendin\n", "<CsFileB filename=\"" +
                        path("ff.bin") + "\">\naGVs\xff" "bG8=\n</CsFileB>\n");
  EXPECT_NE(0, csoundCompileCSD(csound, text.c_str(), 1, 0));
  const auto output = messages();
  EXPECT_NE(std::string::npos, output.find("Non base64 character")) << output;
}

// csoundScoreSort and csoundScoreExtract copy what they read whole: a byte
// 0xFF in a string ended the score's reading, and their copy of the result.
TEST_F(CsdCompileTests, ScoreSortAndExtractKeepAByteFF)
{
  ASSERT_NO_FATAL_FAILURE(
    write("in.sco", "i 1 0 1 \"caf\xff" "e\"\ni 1 1 1 \"abcd\"\ne\n"));
  ASSERT_NO_FATAL_FAILURE(write("all.xtr", ""));
  for (const bool sort : {true, false}) {
    FILE *in = fopen(path("in.sco").c_str(), "rb");
    ASSERT_NE(nullptr, in);
    FILE *out = fopen(path("out.sco").c_str(), "wb");
    ASSERT_NE(nullptr, out);
    FILE *extract = fopen(path("all.xtr").c_str(), "rb");
    ASSERT_NE(nullptr, extract);
    EXPECT_EQ(0, sort ? csoundScoreSort(csound, in, out) :
              csoundScoreExtract(csound, in, out, extract)) << messages();
    fclose(extract);
    fclose(out);
    fclose(in);
    std::ifstream copy(path("out.sco"), std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(copy)),
                           std::istreambuf_iterator<char>());
    EXPECT_NE(std::string::npos, text.find("\"caf\xff" "e\""))
      << (sort ? "sort: " : "extract: ") << text;
    EXPECT_NE(std::string::npos, text.find("\"abcd\""))
      << (sort ? "sort: " : "extract: ") << text;
    csoundReset(csound);
  }
}
}
