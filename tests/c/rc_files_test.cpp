#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "csound.h"

/* CSOUND7RC set but empty: an instance reads no rc file. Each file here sets a
 * variable of its own, so csoundGetEnv() says what an instance took. HOME is
 * pointed inside the test's own directory, the test runs from there, and the
 * real CSOUND7RC is put aside: no file of the user running it is read.
 */

namespace fs = std::filesystem;

namespace {

const char *const orchestra = "ksmps = 64\nnchnls = 1\n0dbfs = 1\n";

/* An orchestra's own instance says 1 once it runs, and 2 if it took the file's
 * macro: 0 is one that never ran.
 */
const char *const parent =
    "instr 1\n"
    "  child:Csound = create()\n"
    "  err:i = setoption(child, \"-n\")\n"
    "  err = compilestr(child, {{\n"
    "    chnset(1, \"took\")\n"
    "#ifdef RC_HERE\n"
    "    chnset(2, \"took\")\n"
    "#end\n"
    "  }})\n"
    "  err = start(child)\n"
    "  took:i = chnget(child, \"took\")\n"
    "  chnset(took, \"took\")\n"
    "  delete(child)\n"
    "endin\n"
    "schedule(1, 0, 0)\n";

const char *const csd =
    "<CsoundSynthesizer>\n"
    "<CsInstruments>\n"
    "ksmps = 64\n"
    "</CsInstruments>\n"
    "<CsScore>\n"
    "e\n"
    "</CsScore>\n"
    "</CsoundSynthesizer>\n";

void setProcessEnv(const char *name, const char *value)
{
#ifdef _WIN32
  _putenv_s(name, value != nullptr ? value : "");
#else
  if (value != nullptr)
    setenv(name, value, 1);
  else
    unsetenv(name);
#endif
}

class RcFileTests : public ::testing::Test {
 protected:
  void SetUp() override
  {
    started_in = fs::current_path();
    if (const char *value = std::getenv("CSOUND7RC"))
      users_rc = value;
    setProcessEnv("CSOUND7RC", nullptr);
    root = fs::temp_directory_path() /
           ("csound-rc-files-" + std::to_string(std::random_device{}()));
    fs::create_directories(root / "home");
    fs::create_directories(root / "here");
    std::ofstream(root / "home" / ".csound7rc") << "--env:RC_HOME=1\n";
    std::ofstream(root / "here" / ".csound7rc")
        << "--env:RC_HERE=1 --omacro:RC_HERE=1\n";
    std::ofstream(root / "named") << "--env:RC_NAMED=1\n";
    std::ofstream(root / "here" / "empty.csd") << csd;
    fs::current_path(root / "here");
    ASSERT_EQ(csoundSetGlobalEnv("HOME", (root / "home").string().c_str()), 0);
  }

  void TearDown() override
  {
    for (CSOUND *csound : instances)
      csoundDestroy(csound);
    fs::current_path(started_in);
    csoundSetGlobalEnv("HOME", nullptr);
    csoundSetGlobalEnv("CSOUND7RC", nullptr);
    setProcessEnv("CSOUND7RC", users_rc ? users_rc->c_str() : nullptr);
    std::error_code error;
    fs::remove_all(root, error);
  }

  CSOUND *instance()
  {
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    instances.push_back(csound);
    return csound;
  }

  /* What a host says, for every instance from here on. */
  static void readNoRcFile()
  {
    ASSERT_EQ(csoundSetGlobalEnv("CSOUND7RC", ""), 0);
  }

  static void option(CSOUND *csound)
  {
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
  }

  /* With no option given: csoundStart() is then what reads the files. */
  static void start(CSOUND *csound)
  {
    csoundSetHostAudioIO(csound);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra, 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
  }

  static void compile(CSOUND *csound)
  {
    const char *argv[] = {"csound", "-n", "empty.csd"};
    ASSERT_EQ(csoundCompile(csound, 3, argv), CSOUND_SUCCESS);
  }

  static double childTook(CSOUND *csound)
  {
    EXPECT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    EXPECT_EQ(csoundCompileOrc(csound, parent, 0), CSOUND_SUCCESS);
    EXPECT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    EXPECT_EQ(csoundPerformKsmps(csound), 0);
    return csoundGetControlChannel(csound, "took", nullptr);
  }

  static bool took(CSOUND *csound, const char *name)
  {
    return csoundGetEnv(csound, name) != nullptr;
  }

  /* An instance keeps the environment it was created with, so one made before
   * the word and one made after can stand side by side.
   */
  void expectOnlyTheOneBeforeReads(void (*reach)(CSOUND *), const char *name)
  {
    CSOUND *plain = instance();
    readNoRcFile();
    CSOUND *told = instance();
    reach(plain);
    reach(told);
    EXPECT_TRUE(took(plain, name));
    EXPECT_FALSE(took(told, name));
  }

  fs::path root, started_in;
  std::optional<std::string> users_rc;
  std::vector<CSOUND *> instances;
};

TEST_F(RcFileTests, NotReadAtTheFirstOption)
{
  expectOnlyTheOneBeforeReads(option, "RC_HERE");
}

TEST_F(RcFileTests, NotReadByCsoundCompile)
{
  expectOnlyTheOneBeforeReads(compile, "RC_HERE");
}

TEST_F(RcFileTests, NotReadByCsoundStart)
{
  expectOnlyTheOneBeforeReads(start, "RC_HERE");
}

TEST_F(RcFileTests, TheFileInHomeIsNotReadEither)
{
  expectOnlyTheOneBeforeReads(option, "RC_HOME");
}

TEST_F(RcFileTests, NotReadThroughAReset)
{
  CSOUND *plain = instance();
  for (int32_t run = 0; run < 2; run++) {
    option(plain);
    EXPECT_TRUE(took(plain, "RC_HERE"));
    csoundReset(plain);
  }
  readNoRcFile();
  CSOUND *told = instance();
  for (int32_t run = 0; run < 2; run++) {
    option(told);
    EXPECT_FALSE(took(told, "RC_HERE"));
    csoundReset(told);
  }
}

TEST_F(RcFileTests, NotReadByAnInstanceAnOrchestraCreates)
{
  EXPECT_EQ(childTook(instance()), 2);
  readNoRcFile();
  EXPECT_EQ(childTook(instance()), 1);
}

TEST_F(RcFileTests, TheCommandLineSaysItWithEnv)
{
  CSOUND *csound = instance();
  const char *argv[] = {"csound", "--env:CSOUND7RC=", "-n", "empty.csd"};
  ASSERT_EQ(csoundCompile(csound, 4, argv), CSOUND_SUCCESS);
  EXPECT_FALSE(took(csound, "RC_HERE"));
}

TEST_F(RcFileTests, AFileItNamesIsReadInPlaceOfHomeAsBefore)
{
  ASSERT_EQ(csoundSetGlobalEnv("CSOUND7RC", (root / "named").string().c_str()),
            0);
  CSOUND *csound = instance();
  option(csound);
  EXPECT_TRUE(took(csound, "RC_NAMED"));
  EXPECT_FALSE(took(csound, "RC_HOME"));
  EXPECT_TRUE(took(csound, "RC_HERE"));
}

}  // namespace
