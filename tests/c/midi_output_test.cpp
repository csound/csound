/*
  midi_output_test.cpp:

  What MIDI output opcodes send, read through a host's MIDI output, so no
  MIDI device is required.

  midion and mrtmsg send on the port their last argument names, as every
  other MIDI output opcode does. midion kept its port pointer after its own
  state, where the engine never set it, so its first k-cycle read through
  NULL. mrtmsg was allocated at the size of xtratim's struct, one argument
  short, so its port pointer landed in the next opcode's header.

  midion2 turns off only a note it sent. Its first trigger turned off
  note 0 on channel 1, and a midion2 never triggered did so at its release.

  moscil's first note lasts kdur and is followed by kpause, as the others
  are. It lasted one k-cycle, with no pause, and cut the second one short.
*/

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"

#include <string>
#include <vector>

namespace {

class MidiOutputTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;
  // Each message's bytes, then the port it was sent on.
  std::vector<std::vector<int>> sent;
  // The k-cycle each message was sent in.
  std::vector<uint64_t> kcycles;
  // The orchestra's header.
  std::string header = "sr = 1000\nksmps = 10\nnchnls = 1\n0dbfs = 1\n";
  // k-cycles of 1/64 s, exact in binary, for durations that end on one.
  static constexpr const char *exactHeader =
    "sr = 1024\nksmps = 16\nnchnls = 1\n0dbfs = 1\n";

  static int32_t open(CSOUND *csound, void **data, const char *)
  {
    *data = csoundGetHostData(csound);
    return 0;
  }

  static int32_t write(CSOUND *csound, void *data, const unsigned char *bytes,
                       int32_t count)
  {
    std::vector<int> message(bytes, bytes + count);
    message.push_back(csound->GetMidiOutPort(csound));
    static_cast<MidiOutputTests *>(data)->sent.push_back(message);
    static_cast<MidiOutputTests *>(data)->kcycles.push_back(
      csoundGetKcounter(csound));
    return count;
  }

  static int32_t close(CSOUND *, void *) { return 0; }

  void SetUp() override
  {
    csound = csoundCreate(this, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetHostMIDIIO(csound);
    csoundSetExternalMidiOutOpenCallback(csound, open);
    csoundSetExternalMidiWriteCallback(csound, write);
    csoundSetExternalMidiOutCloseCallback(csound, close);
    ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
    ASSERT_EQ(csoundSetOption(csound, "-Q0"), 0);
  }

  void TearDown() override { csoundDestroy(csound); }

  // Plays `body` as instr 1 to the score's end, k-cycles of 10 ms unless
  // the header says otherwise. By default the score is a note of three
  // k-cycles.
  void play(const std::string &body, const char *score = "i 1 0 0.03\n")
  {
    const std::string orchestra =
      header + "instr 1\n" + body + "\nendin\n";
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0);
    csoundEventString(csound, score, 0);
    ASSERT_EQ(csoundStart(csound), 0);
    int cycles = 0;
    while (csoundPerformKsmps(csound) == 0)
      ASSERT_LT(++cycles, 20);
  }
};

TEST_F(MidiOutputTests, MidionSendsOnItsPort)
{
  // A note, a new note number on the second k-cycle, and the release.
  play("kn = timeinstk() < 2 ? 60 : 62\n"
       "midion 2, kn, 100, 3");
  EXPECT_EQ(sent, (std::vector<std::vector<int>>{
    {0x91, 60, 100, 3},
    {0x81, 60, 100, 3}, {0x91, 62, 100, 3},
    {0x81, 62, 100, 3}}));
}

TEST_F(MidiOutputTests, MidionWithNoPortSendsOnPortZero)
{
  play("midion 2, 60, 100");
  EXPECT_EQ(sent, (std::vector<std::vector<int>>{
    {0x91, 60, 100, 0}, {0x81, 60, 100, 0}}));
}

TEST_F(MidiOutputTests, MrtmsgSendsOnItsPort)
{
  // The port pointer was the next opcode's link to the one after it.
  play("mrtmsg 1, 3\n"
       "noteon 2, 60, 100, 1\n"
       "noteoff 2, 60, 0, 1");
  EXPECT_EQ(sent, (std::vector<std::vector<int>>{
    {0xFA, 3}, {0x91, 60, 100, 1}, {0x81, 60, 0, 1}}));
}

TEST_F(MidiOutputTests, MrtmsgBeforeTheLastInitOpcode)
{
  // That link is NULL when the next opcode is the last with an init pass.
  play("mrtmsg 1, 3\n"
       "noteon 2, 60, 100, 1");
  EXPECT_EQ(sent, (std::vector<std::vector<int>>{
    {0xFA, 3}, {0x91, 60, 100, 1}}));
}

TEST_F(MidiOutputTests, Midion2TurnsOffOnlyTheNoteItSent)
{
  // Triggers on the first and third k-cycles, the second a new channel and
  // note, which turns off the first.
  play("kt = timeinstk() == 2 ? 0 : 1\n"
       "kc = timeinstk() < 3 ? 2 : 3\n"
       "midion2 kc, 59 + timeinstk(), 100, kt");
  EXPECT_EQ(sent, (std::vector<std::vector<int>>{
    {0x91, 60, 100, 0},
    {0x81, 60, 100, 0}, {0x92, 62, 100, 0},
    {0x82, 62, 100, 0}}));
}

TEST_F(MidiOutputTests, Midion2NeverTriggeredSendsNothing)
{
  play("midion2 2, 60, 100, 0");
  EXPECT_EQ(sent, (std::vector<std::vector<int>>{}));
}

TEST_F(MidiOutputTests, Midion2ReusingAnInstanceSendsNoSecondNoteOff)
{
  // The second note takes the first one's instance, released and inactive.
  play("kt = timeinstk() == 1 ? 1 : 0\n"
       "midion2 2, 60, 100, kt",
       "i 1 0 0.03\ni 1 0.06 0.03\n");
  EXPECT_EQ(sent, (std::vector<std::vector<int>>{
    {0x91, 60, 100, 0}, {0x81, 60, 100, 0},
    {0x91, 60, 100, 0}, {0x81, 60, 100, 0}}));
}

TEST_F(MidiOutputTests, Midion2TiedNoteTurnsOffTheSoundingNote)
{
  // A tie runs midion2's init again, while the held note still sounds.
  play("kt init 1\n"
       "midion2 2, p4, 100, kt\n"
       "kt = 0",
       "i 1.1 0 -1 60\ni 1.1 0.03 0.03 62\n");
  EXPECT_EQ(sent, (std::vector<std::vector<int>>{
    {0x91, 60, 100, 0},
    {0x81, 60, 100, 0}, {0x91, 62, 100, 0},
    {0x81, 62, 100, 0}}));
}

TEST_F(MidiOutputTests, MoscilPlaysKdurAndKpauseFromItsFirstNote)
{
  // Three k-cycles on and two off, from the first note, for ten k-cycles.
  header = exactHeader;
  play("moscil 2, 60, 100, 3/64, 2/64", "i 1 0 0.15625\n");
  EXPECT_EQ(kcycles, (std::vector<uint64_t>{1, 4, 6, 9}));
  EXPECT_EQ(sent, (std::vector<std::vector<int>>{
    {0x91, 60, 100, 0}, {0x81, 60, 100, 0},
    {0x91, 60, 100, 0}, {0x81, 60, 100, 0}}));
}

}  // namespace
