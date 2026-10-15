#if defined(CSOUND_TEST_PTHREADS) && !defined(__EMSCRIPTEN__) && !defined(__wasi__)
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "midifile.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {
class RealtimeQueueTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;
  std::mutex gateMutex;
  std::condition_variable gateChanged;
  bool entered = false, released = false;
  std::atomic<int> midiInits{0}, markers{0};
  std::vector<unsigned char> pending;
  std::filesystem::path midiPath;

  static RealtimeQueueTests &host(CSOUND *cs)
  { return *static_cast<RealtimeQueueTests *>(csoundGetHostData(cs)); }
  static int32_t holdInit(CSOUND *cs, void *)
  {
    auto &self = host(cs);
    std::unique_lock<std::mutex> lock(self.gateMutex);
    self.entered = true;
    self.gateChanged.notify_all();
    return self.gateChanged.wait_for(lock, std::chrono::seconds(10),
                                    [&self] { return self.released; }) ? 0 : -1;
  }
  static int32_t midiInit(CSOUND *cs, void *) { ++host(cs).midiInits; return 0; }
  static int32_t mark(CSOUND *cs, void *) { ++host(cs).markers; return 0; }
  static int32_t open(CSOUND *cs, void **data, const char *)
  { *data = csoundGetHostData(cs); return 0; }
  static int32_t read(CSOUND *, void *data, unsigned char *buffer, int32_t size)
  {
    auto &bytes = static_cast<RealtimeQueueTests *>(data)->pending;
    size_t count = std::min(bytes.size(), static_cast<size_t>(size));
    std::copy_n(bytes.begin(), count, buffer);
    bytes.erase(bytes.begin(), bytes.begin() + count);
    return static_cast<int32_t>(count);
  }
  static int32_t close(CSOUND *, void *) { return 0; }
  static void message(CSOUND *, int32_t, const char *) {}
  void SetUp() override
  {
#if !CSOUND_SPINLOCK_AVAILABLE
    GTEST_SKIP() << "Realtime mode requires a native spinlock backend";
#endif
    csound = csoundCreate(this, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundSetMessageStringCallback(csound, message);
    csoundSetHostMIDIIO(csound);
    csoundSetExternalMidiInOpenCallback(csound, open);
    csoundSetExternalMidiReadCallback(csound, read);
    csoundSetExternalMidiInCloseCallback(csound, close);
    ASSERT_EQ(csoundSetOption(csound, "-n -d --daemon --realtime -M0"), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_queue_wait", sizeof(OPDS), 0,
                                "", "", holdInit, nullptr, nullptr), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_queue_midi", sizeof(OPDS), 0,
                                "", "", midiInit, nullptr, nullptr), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_queue_mark", sizeof(OPDS), 0,
                                "", "", mark, nullptr, nullptr), 0);
    ASSERT_EQ(csoundCompileOrc(csound, R"(
      sr = 48000
      ksmps = 16
      nchnls = 1
      0dbfs = 1
      massign 0, 1
      instr 1
        test_queue_midi
        asig init 0.25
        out asig
      endin
      instr 8
        test_queue_mark
      endin
      instr 9
        test_queue_wait
      endin
    )", 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    csoundEventString(csound, "i9 0 0", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    std::unique_lock<std::mutex> lock(gateMutex);
    ASSERT_TRUE(gateChanged.wait_for(lock, std::chrono::seconds(5),
                                    [this] { return entered; }));
  }
  void release()
  {
    std::lock_guard<std::mutex> lock(gateMutex);
    released = true;
    gateChanged.notify_all();
  }
  void TearDown() override
  {
    release();
    if (csound != nullptr) csoundDestroy(csound);
    if (!midiPath.empty()) {
      std::error_code error;
      std::filesystem::remove(midiPath, error);
    }
  }
  bool drain()
  {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (alloc_queue_has_pending(csound)) {
      if (std::chrono::steady_clock::now() > deadline) return false;
      csoundSleep(1);
    }
    return true;
  }
  void expectNoteOff()
  {
    for (int i = 0; i < 100 && midiInits == 0; ++i) {
      ASSERT_EQ(csoundPerformKsmps(csound), 0);
      csoundSleep(1);
    }
    ASSERT_EQ(midiInits, 1);
    ASSERT_TRUE(drain());
    for (int i = 0; i < 4; ++i) ASSERT_EQ(csoundPerformKsmps(csound), 0);
    for (uint32_t i = 0; i < csoundGetKsmps(csound); ++i)
      EXPECT_EQ(csoundGetSpout(csound)[i], FL(0.0));
  }
};

TEST_F(RealtimeQueueTests, NoteOffInSameHostReadWaitsForDelayedNoteOn)
{
  pending = {0x90, 60, 100, 0x80, 60, 0};
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_TRUE(pending.empty()); // Both messages came from one host read.
  EXPECT_EQ(midiInits, 0);
  release();
  expectNoteOff();
}

TEST_F(RealtimeQueueTests, ChordNoteOnsQueueBeforeAnyInitCompletes)
{
  pending = {0x90, 60, 100, 0x90, 64, 100, 0x90, 67, 100};
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(midiInits, 0);
  release();
  ASSERT_TRUE(drain());
  // All three onsets must have reached the worker in the same control cycle.
  EXPECT_EQ(midiInits, 3);
  pending = {0x80, 60, 0, 0x80, 64, 0, 0x80, 67, 0};
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
}

TEST_F(RealtimeQueueTests, FileClockKeepsAdvancingWhileControlWaitsForInit)
{
  midiPath = std::filesystem::temp_directory_path() /
    ("csound-midi-clock-" + std::to_string(reinterpret_cast<uintptr_t>(csound)) + ".mid");
  // On + controller at tick zero; a second onset at 100 ms (96 of 480 PPQN).
  const unsigned char midi[] = {
    'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 1, 224,
    'M', 'T', 'r', 'k', 0, 0, 0, 16,
    0, 0x90, 60, 100, 0, 0xB0, 1, 100,
    96, 0x90, 64, 100, 0, 0xFF, 0x2F, 0
  };
  {
    std::ofstream file(midiPath, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file.write(reinterpret_cast<const char *>(midi), sizeof(midi));
    ASSERT_TRUE(file.good());
  }
  csound->oparms->FMidiname = csoundStrdup(csound, midiPath.string().c_str());
  ASSERT_EQ(csoundMIDIFileOpen(csound, csound->oparms->FMidiname), 0);
  csound->oparms->FMidiin = 1;
  for (int cycle = 0; cycle < 400; ++cycle)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(midiInits, 0);
  release();
  ASSERT_TRUE(drain());
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  ASSERT_TRUE(drain());
  EXPECT_EQ(midiInits, 2);
}

TEST_F(RealtimeQueueTests, SustainMessagesKeepTheirOrderAroundDelayedInit)
{
  pending = {0x90, 60, 100, 0xB0, 64, 127, 0x80, 60, 0};
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  release();
  ASSERT_TRUE(drain());
  for (int i = 0; i < 4; ++i) ASSERT_EQ(csoundPerformKsmps(csound), 0);
  ASSERT_EQ(midiInits, 1);
  EXPECT_EQ(csoundGetSpout(csound)[0], FL(0.25));
  pending = {0xB0, 64, 0};
  expectNoteOff();
}

TEST_F(RealtimeQueueTests, FullAllocationQueueKeepsHostMidiForLater)
{
  for (int i = 0; i < MAX_ALLOC_QUEUE; ++i)
    csoundEventString(csound, "i8 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  pending = {0x90, 60, 100, 0x80, 60, 0};
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(pending.size(), 6u);
  release();
  ASSERT_TRUE(drain());
  EXPECT_EQ(markers, MAX_ALLOC_QUEUE);
  expectNoteOff();
}

TEST_F(RealtimeQueueTests, StopClosesAdmissionBeforeJoiningWorker)
{
  std::thread stop([this] { csoundReset(csound); });
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (ATOMIC_GET(csound->event_insert_loop) &&
         std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  EXPECT_EQ(ATOMIC_GET(csound->event_insert_loop), 0);
  ALLOC_DATA data = {};
  data.type = -1; // No operation if an old worker incorrectly accepts it.
  EXPECT_EQ(alloc_queue_enqueue(csound, &data), CSOUND_ERROR);
  release();
  stop.join();
}

TEST_F(RealtimeQueueTests, StopDrainsAcceptedMidiAndFullAllocationQueue)
{
  pending = {0x90, 60, 100};
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  for (int i = 0; i < MAX_ALLOC_QUEUE - 1; ++i)
    csoundEventString(csound, "i8 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  release();
  csoundReset(csound);
  EXPECT_EQ(midiInits, 1);
  EXPECT_EQ(markers, MAX_ALLOC_QUEUE - 1);
}
TEST_F(RealtimeQueueTests, AsyncCompileRejectsStoppedWorker)
{
  release();
  ASSERT_TRUE(drain());
  alloc_queue_stop(csound);
  EXPECT_EQ(csoundCompileOrc(csound, "instr 10\nendin\n", 1), CSOUND_ERROR);
}
} // namespace

#endif
