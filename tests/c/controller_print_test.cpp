#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "cs_internal.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {
class ControllerPrintTests : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;
  std::filesystem::path path;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
#if defined(_WIN32)
    const auto pid = _getpid();
#else
    const auto pid = getpid();
#endif
    path = std::filesystem::temp_directory_path() /
      ("csound-controllers-" + std::to_string(pid) + "-" +
       std::to_string(reinterpret_cast<uintptr_t>(csound)) + ".txt");
    std::filesystem::remove(path);
  }

  void TearDown() override
  {
    csoundDestroy(csound);
    std::error_code error;
    std::filesystem::remove(path, error);
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

  void reset()
  {
    csoundReset(csound);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
  }

  void start(const std::string &body,
             const std::string &score = "i 1 0 .02\nf 0 .03\n")
  {
    const std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\n"
      "sr=1000\nksmps=1\nnchnls=1\ninstr 1\n" + body +
      "\nendin\n</CsInstruments>\n<CsScore>\n" + score +
      "</CsScore>\n</CsoundSynthesizer>";
    ASSERT_EQ(0, csoundCompileCSD(csound, csd.c_str(), 1, 0)) << messages();
    ASSERT_EQ(0, csoundStart(csound)) << messages();
  }

  void finish()
  {
    int result = 0;
    for (int i = 0; i < 100 && result == 0; ++i)
      result = csoundPerformKsmps(csound);
    ASSERT_NE(0, result) << "performance did not finish";
  }

  int openFiles()
  {
    int count = 0;
    for (auto *file = static_cast<CSFILE *>(csound->open_files);
         file != nullptr; file = file->nxt)
      ++count;
    return count;
  }

  std::string contents()
  {
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), {});
  }
};

TEST_F(ControllerPrintTests, SavesAndPrintsControllerValues)
{
  start("ctrlinit 1, 0, 0, 127, 127\n"
        "kValues[] ctrlsave 1, 0, 127\n"
        "ctrlprint kValues, {{" + path.generic_string() + "}}\n");
  finish();
  EXPECT_EQ(0, csound->inerrcnt + csound->perferrcnt) << messages();
  EXPECT_EQ(0, openFiles());
  EXPECT_NE(std::string::npos, contents().find("ctrlinit\t1, 0,0, 127,127"));
}

TEST_F(ControllerPrintTests, KeepsExtendedMidiChannels)
{
  start("ctrlinit 1024, 7, 42\n"
        "kValues[] ctrlsave 1024, 7\n"
        "ctrlprint kValues, {{" + path.generic_string() + "}}\n");
  finish();
  EXPECT_EQ(0, csound->inerrcnt + csound->perferrcnt) << messages();
  EXPECT_NE(std::string::npos, contents().find("ctrlinit\t1024, 7,42"));
}

TEST_F(ControllerPrintTests, RejectsInvalidArraysBeforeWriting)
{
  const char *arrays[] = {
    "kValues[] init 1",                       // missing header
    "kValues[][] init 2, 2",                 // not a vector
    "kValues[] fillarray 2, 1, 7, 42",        // count exceeds capacity
    "kValues[] fillarray -1, 1",             // negative count
    "kValues[] fillarray 1e30, 1",           // count cannot fit an integer
    "kValues[] fillarray 1, 0, 7, 42",        // invalid channel
    "kValues[] fillarray 1, 1025, 7, 42",
    "kValues[] fillarray 1, 1, -1, 42",       // invalid controller
    "kValues[] fillarray 1, 1, 128, 42",
    "kValues[] fillarray 1, 1, 7, 1e30"       // invalid value
  };
  for (const char *array : arrays) {
    SCOPED_TRACE(array);
    reset();
    std::filesystem::remove(path);
    start(std::string(array) + "\nctrlprint kValues, {{" +
          path.generic_string() + "}}\n");
    finish();
    EXPECT_NE(std::string::npos, messages().find("ctrlprint:"));
    EXPECT_TRUE(contents().empty());
    EXPECT_EQ(0, openFiles());
  }
}

TEST_F(ControllerPrintTests, RejectsInvalidSaveInputs)
{
  for (const char *args : {"-1, 7", "1025, 7", "1e30, 7",
                           "1, -1", "1, 128", "1, 1e30"}) {
    SCOPED_TRACE(args);
    reset();
    start(std::string("kValues[] ctrlsave ") + args + "\n");
    finish();
    EXPECT_GT(csound->inerrcnt + csound->perferrcnt, 0) << messages();
  }
}

TEST_F(ControllerPrintTests, ClosesFilesOnReinitNoteEndAndReuse)
{
  for (bool presets : {false, true}) {
    SCOPED_TRACE(presets);
    reset();
    const std::string printer = presets ? "ctrlprintpresets " : "ctrlprint kValues, ";
    start("kValues[] ctrlsave 1, 7\nkPreset ctrlpreset 1, 1, 7, 42\n"
          "kCycle init 0\nkCycle += 1\n"
          "if kCycle == 3 then\nreinit PRINT\nendif\n"
          "PRINT:\n" + printer + "{{" + path.generic_string() + "}}\nrireturn\n",
          "i 1 0 .02\ni 1 .04 .02\nf 0 .08\n");
    for (int i = 0; i < 70; ++i) {
      csoundPerformKsmps(csound);
      EXPECT_LE(openFiles(), 1) << "cycle " << i;
      if (i == 5 || i == 45) {
        EXPECT_EQ(1, openFiles());
      }
      if (i == 30 || i == 69) {
        EXPECT_EQ(0, openFiles());
      }
    }
    EXPECT_EQ(0, csound->inerrcnt + csound->perferrcnt) << messages();
    EXPECT_NE(std::string::npos, contents().find(presets ? "ctrlpreset" : "ctrlinit"));
  }
}

TEST_F(ControllerPrintTests, ReportsFileOpenFailure)
{
  start("kValues[] ctrlsave 1, 7\nctrlprint kValues, {{" +
        (path / "missing.txt").generic_string() + "}}\n");
  finish();
  EXPECT_NE(std::string::npos, messages().find("Cannot open"));
  EXPECT_EQ(0, openFiles());
}
} // namespace
