/*
  cmidi_output_test.cpp:

  Unit test for the CoreMIDI output plugin (InOut/cmidi.c).

  It creates its own virtual CoreMIDI destination, so no physical MIDI
  hardware or external device is required, then drives Csound through the
  "coremidi" MIDI output module and checks that the bytes emitted by the
  midiout opcodes actually arrive at the destination.

  This test is only compiled on macOS with the CoreMIDI plugin available.
*/

#include "gtest/gtest.h"

#if defined(__APPLE__)

#include <CoreServices/CoreServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <CoreMIDI/CoreMIDI.h>

#include "csound.h"

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace {

/* Virtual CoreMIDI destination collecting every byte delivered to it. */
struct MidiSink {
  MIDIClientRef client {0};
  MIDIEndpointRef endpoint {0};
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<unsigned char> bytes;
};

void midiReadProc(const MIDIPacketList *pktlist, void *refCon, void *) {
  MidiSink *sink = static_cast<MidiSink *>(refCon);
  std::lock_guard<std::mutex> lock(sink->mutex);
  const MIDIPacket *packet = &pktlist->packet[0];
  for (UInt32 i = 0; i < pktlist->numPackets; i++) {
    sink->bytes.insert(sink->bytes.end(), packet->data,
                       packet->data + packet->length);
    packet = MIDIPacketNext(packet);
  }
  sink->cv.notify_all();
}

/* Index of the endpoint within the CoreMIDI destination list; this is the
   number Csound's coremidi module uses to select an output device. */
int destinationIndex(MIDIEndpointRef endpoint) {
  ItemCount n = MIDIGetNumberOfDestinations();
  for (ItemCount i = 0; i < n; i++)
    if (MIDIGetDestination(i) == endpoint)
      return static_cast<int>(i);
  return -1;
}

}  // namespace

class CoreMidiOutputTests : public ::testing::Test {
 protected:
  MidiSink sink;
  CSOUND *csound {nullptr};
  std::string priorOpcodeDir;
  std::string priorOpcodeDir64;
  bool hadOpcodeDir {false};
  bool hadOpcodeDir64 {false};

  void SetUp() override {
#if defined(CSOUND_TEST_COREMIDI_PLUGIN_DIR)
    /* Capture the prior plugin search path first, before any assertion that
       could abort SetUp, so TearDown always restores the original values
       (and never deletes variables it did not set). csoundGetEnv() also sees
       overrides set with csoundSetGlobalEnv(), unlike std::getenv(). */
    const char *value = csoundGetEnv(nullptr, "OPCODE7DIR");
    hadOpcodeDir = value != nullptr;
    if (hadOpcodeDir)
      priorOpcodeDir = value;
    value = csoundGetEnv(nullptr, "OPCODE7DIR64");
    hadOpcodeDir64 = value != nullptr;
    if (hadOpcodeDir64)
      priorOpcodeDir64 = value;
#endif
    ASSERT_EQ(MIDIClientCreate(CFSTR("csound unittests"), nullptr, nullptr,
                               &sink.client), noErr);
    ASSERT_EQ(MIDIDestinationCreate(sink.client, CFSTR("csound unit test sink"),
                                    midiReadProc, &sink, &sink.endpoint),
              noErr);

#if defined(CSOUND_TEST_COREMIDI_PLUGIN_DIR)
    /* Point the plugin search path at the freshly built coremidi plugin
       before Csound loads its default modules. */
    csoundSetGlobalEnv("OPCODE7DIR", CSOUND_TEST_COREMIDI_PLUGIN_DIR);
    csoundSetGlobalEnv("OPCODE7DIR64", CSOUND_TEST_COREMIDI_PLUGIN_DIR);
#endif
  }

  void TearDown() override {
    if (csound != nullptr)
      csoundDestroy(csound);
#if defined(CSOUND_TEST_COREMIDI_PLUGIN_DIR)
    csoundSetGlobalEnv("OPCODE7DIR",
                       hadOpcodeDir ? priorOpcodeDir.c_str() : nullptr);
    csoundSetGlobalEnv("OPCODE7DIR64",
                       hadOpcodeDir64 ? priorOpcodeDir64.c_str() : nullptr);
#endif
    if (sink.client != 0)
      MIDIClientDispose(sink.client);
  }
};

TEST_F(CoreMidiOutputTests, SendsMidiMessages) {
#if !defined(CSOUND_TEST_COREMIDI_PLUGIN_DIR)
  GTEST_SKIP() << "The coremidi plugin is not built";
#else
  const int index = destinationIndex(sink.endpoint);
  ASSERT_GE(index, 0);

  csound = csoundCreate(nullptr, nullptr);
  ASSERT_NE(csound, nullptr);
  csoundCreateMessageBuffer(csound, 0);

  bool haveCoreMidi = false;
  char *name = nullptr, *type = nullptr;
  for (int32_t n = 0; !csoundGetModule(csound, n, &name, &type); n++) {
    if (name != nullptr && type != nullptr && strcmp(name, "coremidi") == 0 &&
        strcmp(type, "midi") == 0) {
      haveCoreMidi = true;
      break;
    }
  }
  if (!haveCoreMidi)
    GTEST_SKIP() << "The coremidi module is not available";

  csoundSetMIDIModule(csound, "coremidi");
  ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
  const std::string q = "-Q" + std::to_string(index);
  ASSERT_EQ(csoundSetOption(csound, q.c_str()), 0);

  const char *orc = R"(
    sr = 48000
    ksmps = 16
    nchnls = 1
    instr 1
      midiout_i 0x10, 1, 60, 100, 0
      midiout_i 0x10, 1, 62, 101, 0
      midiout_i 0x80, 1, 60, 0, 0
    endin
  )";
  ASSERT_EQ(csoundCompileOrc(csound, orc, 0), 0);
  csoundEventString(csound, "i1 0 0.05\n", 0);
  ASSERT_EQ(csoundStart(csound), 0);
  for (int i = 0; i < 8; i++)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);

  const std::vector<unsigned char> expected = {
      0x90, 60, 100, 0x90, 62, 101, 0x80, 60, 0};
  std::unique_lock<std::mutex> lock(sink.mutex);
  sink.cv.wait_for(lock, std::chrono::seconds(5), [&] {
    return sink.bytes.size() >= expected.size();
  });
  EXPECT_EQ(sink.bytes, expected);
#endif
}

#endif  // __APPLE__
