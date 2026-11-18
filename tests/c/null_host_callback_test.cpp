#include "csound.h"
#include "csound_graph_display.h"
#include "gtest/gtest.h"

#include <cstring>
#include <string>

/* A host callback set to NULL. Csound called some of them with no check,
 * and a host that cleared one crashed.
 */

namespace {

int32_t reads, writes, draws, exits, plays, records;

int32_t openMidi(CSOUND *, void **userData, const char *)
{
  *userData = nullptr;
  return 0;
}

int32_t readMidi(CSOUND *, void *, unsigned char *, int32_t)
{
  reads++;
  return 0;
}

int32_t writeMidi(CSOUND *, void *, const unsigned char *, int32_t nbytes)
{
  writes++;
  return nbytes;
}

int32_t closeMidi(CSOUND *, void *) { return 0; }

void drawGraph(CSOUND *, WINDAT *) { draws++; }

int32_t exitGraph(CSOUND *)
{
  exits++;
  return 0;
}

int32_t openAudio(CSOUND *, const csRtAudioParams *) { return 0; }

void playAudio(CSOUND *, const cs_float *, int32_t) { plays++; }

int32_t recordAudio(CSOUND *, cs_float *buffer, int32_t nbytes)
{
  std::memset(buffer, 0, (size_t) nbytes);
  records++;
  return nbytes;
}

void closeAudio(CSOUND *) {}

const char *const header = "sr = 48000\nksmps = 16\nnchnls = 1\n0dbfs = 1\n";

const std::string displaying = std::string(header) +
    "instr 1\n a1 = oscili(0.5, 440)\n display a1, 0.001\nendin\n"
    "schedule(1, 0, 1)\n";

class NullHostCallbackTests : public ::testing::Test {
 protected:
  /* No rc file of the user's, at the start and at each reset. */
  void SetUp() override
  {
    reads = writes = draws = exits = plays = records = 0;
    csoundSetGlobalEnv("CSOUND7RC", "");
    csound = csoundCreate(nullptr, nullptr);
    csoundCreateMessageBuffer(csound, 0);
  }

  void TearDown() override
  {
    csoundDestroy(csound);
    csoundSetGlobalEnv("CSOUND7RC", nullptr);
  }

  void start(const char *orchestra)
  {
    ASSERT_EQ(csoundCompileOrc(csound, orchestra, 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
  }

  void perform(int32_t cycles)
  {
    for (int32_t i = 0; i < cycles; i++)
      ASSERT_EQ(csoundPerformKsmps(csound), 0);
  }

  /* Displays on, and -g, so that csoundInitDisplay looks for no graphics
   * plugin and takes the same path wherever it runs.
   */
  void displays()
  {
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-g"), CSOUND_SUCCESS);
  }

  /* MIDI in and out through the host, as the option says. */
  void hostMidi(const char *option)
  {
    csoundSetHostMIDIIO(csound);
    csoundSetExternalMidiInOpenCallback(csound, openMidi);
    csoundSetExternalMidiReadCallback(csound, readMidi);
    csoundSetExternalMidiInCloseCallback(csound, closeMidi);
    csoundSetExternalMidiOutOpenCallback(csound, openMidi);
    csoundSetExternalMidiWriteCallback(csound, writeMidi);
    csoundSetExternalMidiOutCloseCallback(csound, closeMidi);
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, option), CSOUND_SUCCESS);
  }

  /* Audio in through the host, and out with "-odac" or none with "-n". */
  void hostAudio(const char *output = "-odac")
  {
    csoundSetPlayopenCallback(csound, openAudio);
    csoundSetRtplayCallback(csound, playAudio);
    csoundSetRecopenCallback(csound, openAudio);
    csoundSetRtrecordCallback(csound, recordAudio);
    csoundSetRtcloseCallback(csound, closeAudio);
    // a module name that no module takes, so these callbacks stay
    ASSERT_EQ(csoundSetOption(csound, "-+rtaudio=host"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, output), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-iadc"), CSOUND_SUCCESS);
  }

  void expectTheStartFails()
  {
    std::string orchestra = std::string(header) + "instr 1\nendin\n";
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    EXPECT_EQ(csoundStart(csound), CSOUND_ERROR);
  }

  CSOUND *csound = nullptr;
};

TEST_F(NullHostCallbackTests, ANullMidiDeviceListListsNoDevice)
{
  csoundSetMIDIDeviceListCallback(csound, nullptr);
  EXPECT_EQ(csoundGetMIDIDevList(csound, nullptr, 0), 0);
  EXPECT_EQ(csoundGetMIDIDevList(csound, nullptr, 1), 0);
}

TEST_F(NullHostCallbackTests, AMidiReadClearedInAPerformanceReadsNothing)
{
  hostMidi("-M0");
  std::string orchestra = std::string(header) + "instr 1\nendin\n";
  start(orchestra.c_str());
  perform(4);
  ASSERT_GT(reads, 0);
  const int32_t before = reads;
  csoundSetExternalMidiReadCallback(csound, nullptr);
  perform(4);
  EXPECT_EQ(reads, before);
}

TEST_F(NullHostCallbackTests, AMidiWriteClearedInAPerformanceWritesNothing)
{
  hostMidi("-Q0");
  std::string orchestra = std::string(header) +
      "instr 1\n midiout 176, 1, 7, 100\nendin\nschedule(1, 0, 1)\n";
  start(orchestra.c_str());
  perform(4);
  ASSERT_GT(writes, 0);
  const int32_t before = writes;
  csoundSetExternalMidiWriteCallback(csound, nullptr);
  perform(4);
  EXPECT_EQ(writes, before);
}

TEST_F(NullHostCallbackTests, ADrawGraphClearedInAPerformanceIsNotCalled)
{
  csoundSetDrawGraphCallback(csound, drawGraph);
  displays();
  start(displaying.c_str());
  perform(16);
  ASSERT_GT(draws, 0);
  const int32_t before = draws;
  csoundSetDrawGraphCallback(csound, nullptr);
  perform(16);
  EXPECT_EQ(draws, before);
}

/* csoundInitDisplay set the exit graph callback to its own at the start. */
TEST_F(NullHostCallbackTests, AnExitGraphSetBeforeTheStartIsCalled)
{
  csoundSetExitGraphCallback(csound, exitGraph);
  displays();
  std::string orchestra = std::string(header) + "instr 1\nendin\n";
  start(orchestra.c_str());
  perform(1);
  csoundReset(csound);
  EXPECT_EQ(exits, 1);
}

/* And again for each ftable displayed with no make graph callback. */
TEST_F(NullHostCallbackTests, AnExitGraphOutlivesAnFtableDisplay)
{
  csoundSetDrawGraphCallback(csound, drawGraph);
  displays();
  std::string orchestra = std::string(header) +
      "instr 1\n i1 = ftgen(0, 0, 16, 10, 1)\nendin\n";
  start(orchestra.c_str());
  csoundSetExitGraphCallback(csound, exitGraph);
  csoundEventString(csound, "i 1 0 0", 0);
  perform(2);
  ASSERT_GT(draws, 0);
  csoundReset(csound);
  EXPECT_EQ(exits, 1);
}

/* With displays off, csoundInitDisplay put its own callbacks in place of the
 * host's, and csoundReset keeps the graph callbacks.
 */
TEST_F(NullHostCallbackTests, APerformanceWithNoDisplaysKeepsTheDrawGraph)
{
  csoundSetDrawGraphCallback(csound, drawGraph);
  ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundSetOption(csound, "-d"), CSOUND_SUCCESS);
  start(displaying.c_str());
  perform(16);
  EXPECT_EQ(draws, 0);
  csoundReset(csound);
  displays();
  start(displaying.c_str());
  perform(16);
  EXPECT_GT(draws, 0);
}

TEST_F(NullHostCallbackTests, ANullAudioDeviceListListsNoDevice)
{
  csoundSetAudioDeviceListCallback(csound, nullptr);
  EXPECT_EQ(csoundGetAudioDevList(csound, nullptr, 0), 0);
  EXPECT_EQ(csoundGetAudioDevList(csound, nullptr, 1), 0);
}

TEST_F(NullHostCallbackTests, ANullAudioOutputOpenFailsTheStart)
{
  hostAudio();
  csoundSetPlayopenCallback(csound, nullptr);
  expectTheStartFails();
}

TEST_F(NullHostCallbackTests, ANullAudioOutputFailsTheStart)
{
  hostAudio();
  csoundSetRtplayCallback(csound, nullptr);
  expectTheStartFails();
}

TEST_F(NullHostCallbackTests, ANullAudioInputOpenFailsTheStart)
{
  hostAudio();
  csoundSetRecopenCallback(csound, nullptr);
  expectTheStartFails();
}

TEST_F(NullHostCallbackTests, ANullAudioInputFailsTheStart)
{
  hostAudio();
  csoundSetRtrecordCallback(csound, nullptr);
  expectTheStartFails();
}

/* With output open, the output closes the device, and the input otherwise. */
TEST_F(NullHostCallbackTests, ANullAudioCloseIsNotCalled)
{
  hostAudio();
  csoundSetRtcloseCallback(csound, nullptr);
  std::string orchestra = std::string(header) + "instr 1\nendin\n";
  start(orchestra.c_str());
  perform(64);   // more than one buffer
  EXPECT_GT(plays, 0);
  EXPECT_GT(records, 0);
  csoundReset(csound);
}

TEST_F(NullHostCallbackTests, ANullAudioCloseIsNotCalledForInputAlone)
{
  hostAudio("-n");
  csoundSetRtcloseCallback(csound, nullptr);
  std::string orchestra = std::string(header) + "instr 1\nendin\n";
  start(orchestra.c_str());
  perform(64);
  EXPECT_GT(records, 0);
  csoundReset(csound);
}

}  // namespace
