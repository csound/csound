#include "csound.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <initializer_list>
#include <vector>

class MidiRealtimeInputTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;
  std::vector<unsigned char> pending;

  static int32_t open(CSOUND *csound, void **data, const char *)
  {
    *data = csoundGetHostData(csound);
    return 0;
  }

  static int32_t read(CSOUND *, void *data, unsigned char *buffer,
                      int32_t capacity)
  {
    auto &bytes = static_cast<MidiRealtimeInputTests *>(data)->pending;
    const auto count = std::min(bytes.size(), static_cast<size_t>(capacity));
    std::copy_n(bytes.begin(), count, buffer);
    bytes.erase(bytes.begin(), bytes.begin() + count);
    return static_cast<int32_t>(count);
  }

  static int32_t close(CSOUND *, void *) { return 0; }

  void SetUp() override
  {
    csound = csoundCreate(this, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetHostMIDIIO(csound);
    csoundSetExternalMidiInOpenCallback(csound, open);
    csoundSetExternalMidiReadCallback(csound, read);
    csoundSetExternalMidiInCloseCallback(csound, close);
    ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
    ASSERT_EQ(csoundSetOption(csound, "-M0"), 0);
    ASSERT_EQ(csoundCompileOrc(csound, R"(
      sr = 48000
      ksmps = 16
      nchnls = 1
      massign 0, 0
      instr 1
        chnset midicontinue(), "continue"
        chnset midiclockin(), "clock"
        chnset midistart(), "start"
        chnset midistop(), "stop"
      endin
      instr 2
        chnset midicontinue(), "second"
      endin
      instr 3
        kstatus, kchan, kdata1, kdata2 midiin
        if kstatus != 0 then
          chnset kstatus, "status"
          chnset kchan, "channel"
          chnset kdata1, "data1"
          chnset kdata2, "data2"
        endif
      endin
    )", 0), 0);
    csoundEventString(csound, "i1 0 1\ni2 0 1\ni3 0 1\n", 0);
    ASSERT_EQ(csoundStart(csound), 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  }

  void TearDown() override { csoundDestroy(csound); }

  void cycle(std::initializer_list<unsigned char> bytes,
             int continued, int clock = 0, int start = 0, int stop = 0)
  {
    SCOPED_TRACE(::testing::PrintToString(bytes));
    pending = bytes;
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_TRUE(pending.empty());
    EXPECT_EQ(csoundGetControlChannel(csound, "continue", nullptr), continued);
    EXPECT_EQ(csoundGetControlChannel(csound, "second", nullptr), continued);
    EXPECT_EQ(csoundGetControlChannel(csound, "clock", nullptr), clock);
    EXPECT_EQ(csoundGetControlChannel(csound, "start", nullptr), start);
    EXPECT_EQ(csoundGetControlChannel(csound, "stop", nullptr), stop);
  }

  // Each of `reads` is what one read returns, a cycle apart, as a host passing
  // bytes on as they come does. The last message they make is the one expected.
  void message(std::initializer_list<std::vector<unsigned char>> reads,
               int status, int channel, int data1, int data2)
  {
    for (const auto &bytes : reads) {
      SCOPED_TRACE(::testing::PrintToString(bytes));
      pending = bytes;
      ASSERT_EQ(csoundPerformKsmps(csound), 0);
      ASSERT_TRUE(pending.empty());
    }
    EXPECT_EQ(csoundGetControlChannel(csound, "status", nullptr), status);
    EXPECT_EQ(csoundGetControlChannel(csound, "channel", nullptr), channel);
    EXPECT_EQ(csoundGetControlChannel(csound, "data1", nullptr), data1);
    EXPECT_EQ(csoundGetControlChannel(csound, "data2", nullptr), data2);
  }
};

TEST_F(MidiRealtimeInputTests, ContinueIsIndependentOfClock)
{
  cycle({}, 0);
  cycle({0xFB}, 1);
  cycle({}, 0);
  cycle({0xF8}, 0, 1);
  cycle({}, 0);
  cycle({0xFB, 0xF8}, 1, 1);
  cycle({}, 0);
  cycle({0xF8, 0xFB}, 1, 1);
  cycle({}, 0);
  cycle({0xFA, 0xFB, 0xFC, 0xF8}, 1, 1, 1, 1);
  cycle({}, 0);
}

TEST_F(MidiRealtimeInputTests, ContinueIsACycleFlagForAllReaders)
{
  cycle({0xFA}, 0, 0, 1);
  cycle({0xFC}, 0, 0, 0, 1);
  cycle({0xFB, 0xFB}, 1);
  cycle({0xFB}, 1);
  cycle({}, 0);
}

TEST_F(MidiRealtimeInputTests, TransportFlagsSurviveNoteMessages)
{
  cycle({0xFB, 0x90, 60, 100}, 1);
  cycle({}, 0);
  // Realtime bytes may occur between the data bytes of a channel message.
  cycle({0x90, 61, 0xFB, 100}, 1);
  cycle({}, 0);
  cycle({0x90, 62, 100, 0xFB}, 1);
  cycle({}, 0);
  cycle({0xFA, 0xFB, 0xFC, 0xF8, 0x80, 61, 0, 0x80, 62, 0},
        1, 1, 1, 1);
  cycle({0x80, 60, 0}, 0);
  cycle({}, 0);
}

TEST_F(MidiRealtimeInputTests, ReadEndingOnAStatusIgnoresAnEarlierRead)
{
  // The second read leaves the first one's 0x81 right after its status.
  message({{0xF8, 0x81, 60, 0}}, 0x80, 2, 60, 0);
  message({{0x90}, {60, 100}}, 0x90, 1, 60, 100);
  message({{0xF8, 0x81, 61, 0}}, 0x80, 2, 61, 0);
  message({{0x90, 61, 100}}, 0x90, 1, 61, 100);
}

TEST_F(MidiRealtimeInputTests, MessageSplitAcrossReadsComesOutWhole)
{
  message({{0x90}, {60, 100}}, 0x90, 1, 60, 100);
  message({{0x91, 61}, {101}}, 0x90, 2, 61, 101);
  message({{0x92}, {}, {}, {62}, {}, {102}}, 0x90, 3, 62, 102);
  message({{0xC3}, {5}}, 0xC0, 4, 5, 0);
}

TEST_F(MidiRealtimeInputTests, PortByteComesInTheReadOfItsStatus)
{
  message({{0x90, 0x81, 60, 100}}, 0x90, 17, 60, 100);
  message({{0x90, 0x82}, {62, 100}}, 0x90, 33, 62, 100);
  message({{0x90, 0x83, 63}, {100}}, 0x90, 49, 63, 100);
  message({{0x90, 0xBF}, {}, {64}, {100}}, 0x90, 1009, 64, 100);
  message({{0xC0, 0x81, 7}}, 0xC0, 17, 7, 0);
  // A port past the last one is port 0.
  message({{0x90, 0xC1, 65, 100}}, 0x90, 1, 65, 100);
}

TEST_F(MidiRealtimeInputTests, StatusAfterACutOffStatusIsAStatus)
{
  // A read ends on a status whose data never comes: the status that
  // starts the next read begins a message of its own.
  message({{0xC0}, {0x90, 60, 100}}, 0x90, 1, 60, 100);
  message({{0x90}, {0x81, 61, 100}}, 0x80, 2, 61, 100);
}

TEST_F(MidiRealtimeInputTests, RealtimeByteAfterAStatusIsNoPort)
{
  message({{0x90, 0xF8, 60, 100}}, 0x90, 1, 60, 100);
  EXPECT_EQ(csoundGetControlChannel(csound, "clock", nullptr), 1);
  message({{0x90}, {0xF8, 61, 100}}, 0x90, 1, 61, 100);
  EXPECT_EQ(csoundGetControlChannel(csound, "clock", nullptr), 1);
  message({{0x90, 0xFC, 63, 100}}, 0x90, 1, 63, 100);
  EXPECT_EQ(csoundGetControlChannel(csound, "stop", nullptr), 1);
}
