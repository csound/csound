/*
  midi_output_test.cpp:

  What MIDI output opcodes send, read through a host's MIDI output, so no
  MIDI device is required.

  midion sends on the port its last argument names, as every other MIDI
  output opcode does. It kept its port pointer after its own state, where
  the engine never set it, so its first k-cycle read through NULL.
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

  // Plays `body` as instr 1, a note of three k-cycles, to the score's end.
  void play(const std::string &body)
  {
    const std::string orchestra =
      "sr = 1000\nksmps = 10\nnchnls = 1\n0dbfs = 1\n"
      "instr 1\n" + body + "\nendin\n";
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0);
    csoundEventString(csound, "i 1 0 0.03\n", 0);
    ASSERT_EQ(csoundStart(csound), 0);
    int cycles = 0;
    while (csoundPerformKsmps(csound) == 0)
      ASSERT_LT(++cycles, 10);
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

}  // namespace
