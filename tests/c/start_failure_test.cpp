#include "gtest/gtest.h"
#include "csound.hpp"
#include "csPerfThread.hpp"

/* A csoundStart that fails leaves a half-made engine, and every performance
 * call fails, until csoundReset. They used to perform it, and the first
 * k-cycle called an audio output that had never been set, or wrote through
 * buffers that had never been allocated.
 */

// Android's OpenSL ES backend performs through this.
extern "C" int32_t perform_buffer(CSOUND *csound);

namespace {

const char *const orchestra =
    "sr = 48000\nksmps = 16\nnchnls = 1\n0dbfs = 1\n"
    "instr 1\nendin\n";

int32_t refuseMidiIn(CSOUND *, void **, const char *) { return -1; }

int32_t readMidiIn(CSOUND *, void *, unsigned char *, int32_t) { return 0; }

class StartFailureTests : public ::testing::Test {
 protected:
  /* No rc file of the user's, at the start and at each reset. */
  void SetUp() override
  {
    csoundSetGlobalEnv("CSOUND7RC", "");
    csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
  }

  void TearDown() override
  {
    csoundDestroy(csound);
    csoundSetGlobalEnv("CSOUND7RC", nullptr);
  }

  /* MIDI input that does not open: csoundStart fails in the engine's start,
   * after it has said the engine is compiled.
   */
  void failToOpenMidiIn()
  {
    csoundSetHostAudioIO(csound);
    csoundSetHostMIDIIO(csound);
    csoundSetExternalMidiInOpenCallback(csound, refuseMidiIn);
    csoundSetExternalMidiReadCallback(csound, readMidiIn);
    ASSERT_EQ(csoundSetOption(csound, "-M0"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra, 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_ERROR);
  }

  /* The same instance, reset, starts and performs. */
  void expectAResetStartsAgain()
  {
    csoundReset(csound);
    csoundSetHostAudioIO(csound);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra, 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    EXPECT_EQ(csoundPerformKsmps(csound), 0);
  }

  CSOUND *csound = nullptr;
};

TEST_F(StartFailureTests, PerformKsmpsFailsUntilAReset)
{
  failToOpenMidiIn();
  EXPECT_EQ(csoundPerformKsmps(csound), CSOUND_ERROR);
  EXPECT_EQ(csoundPerformKsmps(csound), CSOUND_ERROR);
  EXPECT_EQ(csoundStart(csound), CSOUND_ERROR);
  EXPECT_EQ(csoundPerformKsmps(csound), CSOUND_ERROR);
  expectAResetStartsAgain();
}

/* Global code that fails at init fails the start the same way. */
TEST_F(StartFailureTests, AfterAnInitErrorInGlobalCode)
{
  csoundSetHostAudioIO(csound);
  ASSERT_EQ(csoundCompileOrc(csound, "ksmps = 16\ni1 = ftlen(99)\n", 0),
            CSOUND_SUCCESS);
  ASSERT_EQ(csoundStart(csound), CSOUND_ERROR);
  EXPECT_EQ(csoundPerformKsmps(csound), CSOUND_ERROR);
  expectAResetStartsAgain();
}

/* So does a sound file to read that does not open, once the audio output
 * routine is set.
 */
TEST_F(StartFailureTests, AfterAnInputFileThatDoesNotOpen)
{
  ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundSetOption(csound, "-ino-such-input.wav"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundCompileOrc(csound, orchestra, 0), CSOUND_SUCCESS);
  ASSERT_EQ(csoundStart(csound), CSOUND_ERROR);
  EXPECT_EQ(csoundPerformKsmps(csound), CSOUND_ERROR);
  expectAResetStartsAgain();
}

TEST_F(StartFailureTests, PerformanceThreadFails)
{
  failToOpenMidiIn();
  CsoundPerformanceThread thread(csound);
  thread.Play();
  EXPECT_LT(thread.Join(), 0);
  expectAResetStartsAgain();
}

TEST_F(StartFailureTests, PerformBufferFails)
{
  failToOpenMidiIn();
  EXPECT_EQ(perform_buffer(csound), CSOUND_ERROR);
  expectAResetStartsAgain();
}

/* A fresh instance, and one reset after a performance, still perform. */
TEST_F(StartFailureTests, AStartAfterAResetPerforms)
{
  csoundSetHostAudioIO(csound);
  ASSERT_EQ(csoundCompileOrc(csound, orchestra, 0), CSOUND_SUCCESS);
  ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
  EXPECT_EQ(csoundPerformKsmps(csound), 0);
  expectAResetStartsAgain();
}

}  // namespace
