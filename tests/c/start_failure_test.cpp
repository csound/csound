#define __BUILDING_LIBCSOUND
#include <chrono>
#include <thread>
#include "gtest/gtest.h"
#include "csoundCore.h"
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

/* The state of the engine while csoundStart opens the audio output. */
int32_t stateWhileOpening = -1;

int32_t openOutputAndLook(CSOUND *csound, const csRtAudioParams *)
{
  stateWhileOpening = csound->engineStatus;
  return 0;
}

void playNothing(CSOUND *, const cs_float *, int32_t) {}

void closeNothing(CSOUND *) {}

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

  /* A --realtime run, on a performance thread, whose last note's init fails.
   * The thread's cleanup ends it, and errors is the count it leaves.
   */
  void realtimeRunWithAnInitError(int32_t &errors)
  {
    errors = -1;
    csoundSetHostAudioIO(csound);
    ASSERT_EQ(csoundSetOption(csound, "--realtime"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound,
                               "ksmps = 16\n"
                               "instr 1\n"
                               "  chnset(1, \"reached\")\n"
                               "  iMissing = ftlen(99)\n"
                               "endin\n", 0),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 1", 0);
    CsoundPerformanceThread thread(csound);
    thread.Play();
    /* The init runs on the event thread. Once it has begun, it finishes before
     * the cleanup, which joins that thread. */
    for (int32_t i = 0; i < 20000 &&
         csoundGetControlChannel(csound, "reached", nullptr) != 1; i++)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_EQ(csoundGetControlChannel(csound, "reached", nullptr), 1);
    thread.Stop();
    thread.Join();
    errors = csoundErrCnt(csound);
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

/* csoundReset cleared CS_STATE_COMP with `|=`, which set every other state,
 * CS_STATE_JMP among them. The cleanup that ends a --realtime performance then
 * counted the last note's init error as a performance error after a reset,
 * and not on a fresh instance.
 */
TEST_F(StartFailureTests, ARunAfterAResetCountsErrorsAsAFreshOneDoes)
{
  int32_t errors;
  realtimeRunWithAnInitError(errors);
  EXPECT_EQ(errors, 0);
  csoundReset(csound);
  realtimeRunWithAnInitError(errors);
  EXPECT_EQ(errors, 0);
}

/* csoundLongJmp set CS_STATE_JMP for every jump, also those with no event
 * thread to leave a reset to, and the cleanup of a later --realtime
 * performance read it as one still owed: an option refused before the run
 * made its count take the last note's init error.
 */
TEST_F(StartFailureTests, ARefusedOptionLeavesTheErrorCountAlone)
{
  EXPECT_NE(csoundSetOption(csound, "-o"), CSOUND_SUCCESS);
  int32_t errors;
  realtimeRunWithAnInitError(errors);
  EXPECT_EQ(errors, 0);
}

/* Android's OpenSL ES output opens inside csoundStart, and its first buffer
 * can come back and ask for a performance before csoundStart has returned.
 * Only a start that has failed may be refused, never one under way.
 */
TEST_F(StartFailureTests, AStartUnderWayRefusesNoPerformance)
{
  stateWhileOpening = -1;
  csoundSetPlayopenCallback(csound, openOutputAndLook);
  csoundSetRtplayCallback(csound, playNothing);
  csoundSetRtcloseCallback(csound, closeNothing);
  // a module name that no module takes, so these callbacks stay
  ASSERT_EQ(csoundSetOption(csound, "-+rtaudio=host"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundSetOption(csound, "-odac"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundCompileOrc(csound, orchestra, 0), CSOUND_SUCCESS);
  ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
  ASSERT_NE(stateWhileOpening, -1);
  EXPECT_TRUE(stateWhileOpening & CS_STATE_COMP);
  EXPECT_FALSE(stateWhileOpening & CS_STATE_START_FAILED);
  EXPECT_EQ(csoundPerformKsmps(csound), 0);
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
