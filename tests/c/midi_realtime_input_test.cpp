#include "csound.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

class MidiRealtimeInputTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;
  std::vector<unsigned char> pending;
  int32_t capacity = 0;  // of the buffer, as the last read was told
  int32_t excess = 0;    // counted past what a read really wrote
  std::string extraOrchestra;

  static int32_t open(CSOUND *csound, void **data, const char *)
  {
    *data = csoundGetHostData(csound);
    return 0;
  }

  static int32_t read(CSOUND *, void *data, unsigned char *buffer,
                      int32_t capacity)
  {
    auto *test = static_cast<MidiRealtimeInputTests *>(data);
    auto &bytes = test->pending;
    const auto count = std::min(bytes.size(), static_cast<size_t>(capacity));
    test->capacity = capacity;
    std::copy_n(bytes.begin(), count, buffer);
    bytes.erase(bytes.begin(), bytes.begin() + count);
    return static_cast<int32_t>(count) + (count > 0 ? test->excess : 0);
  }

  static int32_t close(CSOUND *, void *) { return 0; }

  virtual void options() {}

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
    options();
    if (HasFatalFailure())
      return;
    const std::string orchestra = R"(
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
    )" + extraOrchestra;
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0);
    csoundEventString(csound, "i1 0 1\ni2 0 1\ni3 0 1\n", 0);
    ASSERT_EQ(csoundStart(csound), 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  }

  void TearDown() override { csoundDestroy(csound); }

  std::string messages()
  {
    std::string text;
    while (csoundGetMessageCnt(csound) > 0) {
      text += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    return text;
  }

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

TEST_F(MidiRealtimeInputTests, ACountPastTheBufferIsAnErrorAndReadsNothing)
{
  for (int32_t bytes : {1, 1000000000}) {
    SCOPED_TRACE(bytes);
    excess = bytes;
    pending.assign(capacity, 0xFB);
    messages();
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_EQ(csoundGetControlChannel(csound, "continue", nullptr), 0);
    ASSERT_NE(messages().find("error reading MIDI device"), std::string::npos);
  }
  excess = 0;
  cycle({0xFB}, 1);
  cycle({}, 0);
}

TEST_F(MidiRealtimeInputTests, ACountThatFillsTheBufferIsRead)
{
  pending.assign(capacity, 0xFB);
  messages();
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(csoundGetControlChannel(csound, "continue", nullptr), 1);
  EXPECT_EQ(messages().find("error reading MIDI device"), std::string::npos);
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

TEST_F(MidiRealtimeInputTests, RunningStatusRepeatsTheLastMessage)
{
  message({{0x90, 60, 100, 61, 101}, {}}, 0x90, 1, 61, 101);
  message({{62, 102}}, 0x90, 1, 62, 102);
  message({{0x90, 0x81, 63, 100, 64, 101}, {}}, 0x90, 17, 64, 101);
  message({{65, 0xF8, 102}}, 0x90, 17, 65, 102);
  EXPECT_EQ(csoundGetControlChannel(csound, "clock", nullptr), 1);
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

// A MIDI file alongside the device: the control change 0xB4 7 100 on the
// second cycle after SetUp's, and a note on channel 1, held, on the fifth. The
// fixture's sr and ksmps give 3000 k-cycles a second, and at 120 beats a minute
// 1500 ticks a beat make a tick a k-cycle.
class MidiDeviceAndFileInputTests : public MidiRealtimeInputTests {
 protected:
  std::string file;

  void options() override
  {
#if defined(_WIN32)
    const auto pid = _getpid();
#else
    const auto pid = getpid();
#endif
    file = std::string("midi_device_and_file_") +
      ::testing::UnitTest::GetInstance()->current_test_info()->name() + "_" +
      std::to_string(pid) + ".mid";
    const unsigned char midi[] = {
      'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 0x05, 0xDC,
      'M', 'T', 'r', 'k', 0, 0, 0, 12,
      2, 0xB4, 7, 100, 3, 0x90, 60, 100, 0, 0xFF, 0x2F, 0
    };
    std::ofstream out(file, std::ios::binary);
    ASSERT_TRUE(out.is_open());
    out.write(reinterpret_cast<const char *>(midi), sizeof(midi));
    out.close();
    ASSERT_TRUE(out.good());
    ASSERT_EQ(csoundSetOption(csound, ("-F" + file).c_str()), 0);
    extraOrchestra = R"(
      massign 1, 4
      instr 4
        chnset 1, "held"
        chnset release(), "released"
      endin
    )";
  }

  void TearDown() override
  {
    MidiRealtimeInputTests::TearDown();
    std::remove(file.c_str());
  }
};

TEST_F(MidiDeviceAndFileInputTests, FileEventIsNoPortOfADeviceStatus)
{
  // The read that holds the status ends there, and the file's event follows
  // it in the same refill. Its status is not the device's port byte.
  message({{}, {0x90}}, 0xB0, 5, 7, 100);
  message({{60, 100}}, 0x90, 1, 60, 100);
}

TEST_F(MidiDeviceAndFileInputTests, FileEventBetweenPartsOfADeviceMessage)
{
  // A read callback should not return an incomplete message, as the
  // MidiReadCallback contract says. A host passing bytes on as they come
  // does, and a device's status keeps its data past the file's event.
  message({{0x90}, {}}, 0xB0, 5, 7, 100);
  message({{60, 100}}, 0x90, 1, 60, 100);
}

TEST_F(MidiDeviceAndFileInputTests, FileEventKeepsADeviceRunningStatus)
{
  // A message in running status is complete: this is within the contract.
  message({{0x90, 60, 100}, {}}, 0xB0, 5, 7, 100);
  message({{61, 100}}, 0x90, 1, 61, 100);
}

TEST_F(MidiDeviceAndFileInputTests, FileEventDoesNotEndADeviceSystemExclusive)
{
  // One long message in two reads, the file's event landing between them.
  // The device has a status of its own, so the rest of the system exclusive
  // would be data of it, or of the file's, if the file's status had ended it.
  message({{0x90, 60, 100, 0xF0, 1, 2}, {}}, 0xB0, 5, 7, 100);
  message({{3, 4, 0xF7}}, 0xB0, 5, 7, 100);
  message({{0x90, 61, 100}}, 0x90, 1, 61, 100);
}

TEST_F(MidiDeviceAndFileInputTests, FileEventKeepsADevicePortAndRealTime)
{
  // The device's port outlasts the file's event, in running status, and a
  // real time byte still goes through.
  message({{0x90, 0x81, 60, 100}}, 0x90, 17, 60, 100);
  message({{}}, 0xB0, 5, 7, 100);
  message({{0xFB, 61, 101}}, 0x90, 17, 61, 101);
  EXPECT_EQ(csoundGetControlChannel(csound, "continue", nullptr), 1);
}

TEST_F(MidiDeviceAndFileInputTests, DeviceBytesComeBeforeTheirRefillsFileEvent)
{
  message({{}, {0x90, 60, 100}}, 0x90, 1, 60, 100);
  message({{}}, 0xB0, 5, 7, 100);
}

TEST_F(MidiDeviceAndFileInputTests, DeviceDataBeforeAnyStatusIsNoMessage)
{
  // The device has sent no status yet, so its byte is part of no message: not
  // a note off for the note the file holds on channel 1.
  message({{}, {}, {}, {}, {}}, 0x90, 1, 60, 100);
  ASSERT_EQ(csoundGetControlChannel(csound, "held", nullptr), 1);
  message({{60}}, 0x90, 1, 60, 100);
  EXPECT_EQ(csoundGetControlChannel(csound, "released", nullptr), 0);
}
