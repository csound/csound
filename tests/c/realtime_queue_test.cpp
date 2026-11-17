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
#if !CSOUND_SPINLOCK_AVAILABLE || \
    (!defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN))
    GTEST_SKIP() << "Realtime mode requires native spinlocks and atomics";
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
      instr 7
        kGo chnget "go"
        kDone init 0
        kCount init 0
        kCount += 1
        chnset kCount, "seven"
        if kGo == 1 && kDone == 0 then
          kDone = 1
          reinit again
        endif
      again:
        test_queue_mark
        rireturn
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
  // As -F would: at 100 ms (96 of 480 PPQN), a note on, then volume 50 and
  // balance 60 on channel 1. The note waits for init behind i9, the volume
  // waits for the note, and the balance stays in the parser's buffer.
  void playFileAt100ms()
  {
    midiPath = std::filesystem::temp_directory_path() /
      ("csound-midi-rewind-" + std::to_string(reinterpret_cast<uintptr_t>(csound)) + ".mid");
    const unsigned char midi[] = {
      'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 1, 224,
      'M', 'T', 'r', 'k', 0, 0, 0, 16,
      96, 0x90, 60, 100, 0, 0xB0, 7, 50, 0, 0xB0, 8, 60,
      0, 0xFF, 0x2F, 0
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

TEST_F(RealtimeQueueTests, RewindDropsFileMidiNotYetDispatched)
{
  playFileAt100ms();
  for (int cycle = 0; cycle < 400; ++cycle)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  const cs_float *controls = csound->m_chnbp[0]->ctl_val;
  ASSERT_NE(controls[7], FL(50.0));
  csoundRewindScore(csound);
  ASSERT_EQ(controls[7], FL(127.0));
  ASSERT_EQ(controls[8], FL(64.0));
  release();
  ASSERT_TRUE(drain());
  // The file's events come again at 100 ms, 300 cycles from the rewind.
  for (int cycle = 0; cycle < 10; ++cycle)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(controls[7], FL(127.0));
  EXPECT_EQ(controls[8], FL(64.0));
}

TEST_F(RealtimeQueueTests, RewindKeepsDeviceMidiNotYetDispatched)
{
  // A device is not the score: what it sent before the rewind still comes.
  playFileAt100ms();
  pending = {0x90, 60, 100, 0xB0, 7, 50, 0xB0, 8, 60};
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  const cs_float *controls = csound->m_chnbp[0]->ctl_val;
  ASSERT_NE(controls[7], FL(50.0));
  csoundRewindScore(csound);
  release();
  ASSERT_TRUE(drain());
  for (int cycle = 0; cycle < 4; ++cycle)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(controls[7], FL(50.0));
  EXPECT_EQ(controls[8], FL(60.0));
}

TEST_F(RealtimeQueueTests, RewindDropsANoteQueuedForInit)
{
  // The note is due, and waits for its init behind i9's, when the host
  // rewinds the score: it is from before the rewind, and does not start
  // after it. One sent after the rewind does.
  csoundEventString(csound, "i8 0 -1", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  csoundRewindScore(csound);
  release();
  ASSERT_TRUE(drain());
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(markers, 0);
  csoundEventString(csound, "i8 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  ASSERT_TRUE(drain());
  EXPECT_EQ(markers, 1);
}

TEST_F(RealtimeQueueTests, RewindDropsAMidiNoteQueuedForInit)
{
  // As it would have been turned off had its init run before the rewind.
  pending = {0x90, 60, 100};
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  csoundRewindScore(csound);
  release();
  ASSERT_TRUE(drain());
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(midiInits, 0);
  pending = {0x90, 62, 100};
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  ASSERT_TRUE(drain());
  EXPECT_EQ(midiInits, 1);
}

TEST_F(RealtimeQueueTests, RewindTurnsOffANoteQueuedForReinit)
{
  // i7 performs; its reinit waits behind i9's held init when the host
  // rewinds the score. The rewind turns i7 off: its reinit does not run,
  // and it performs no more.
  release();
  ASSERT_TRUE(drain());
  csoundEventString(csound, "i7 0 -1", 0);
  for (int i = 0; i < 100 && markers == 0; ++i) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  ASSERT_EQ(markers, 1);
  {
    std::lock_guard<std::mutex> lock(gateMutex);
    entered = released = false;
  }
  csoundEventString(csound, "i9 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  {
    std::unique_lock<std::mutex> lock(gateMutex);
    ASSERT_TRUE(gateChanged.wait_for(lock, std::chrono::seconds(5),
                                     [this] { return entered; }));
  }
  csoundSetControlChannel(csound, "go", 1);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  csoundRewindScore(csound);
  release();
  ASSERT_TRUE(drain());
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  const cs_float count = csoundGetControlChannel(csound, "seven", nullptr);
  for (int i = 0; i < 4; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(markers, 1);
  EXPECT_EQ(csoundGetControlChannel(csound, "seven", nullptr), count);
  // As after a reinit, in Csound and in the note, which a later one reuses.
  EXPECT_EQ(csound->GetReinitFlag(csound), 0);
  int notes = 0;
  for (INSDS *ip = csound->engineState.instrtxtp[7]->act_instance;
       ip != nullptr; ip = ip->nxtinactive, ++notes)
    EXPECT_EQ(ip->reinitflag, 0);
  EXPECT_GT(notes, 0);
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
// A note performs while another's init runs rewindscore. The note's opcode
// holds its performance once that init has begun, for at most 200 ms, and
// sees whether the note was deinitialised in the meantime.
class RealtimeRewindOpcodeTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;
  std::mutex mutex;
  std::condition_variable changed;
  std::atomic<int> performed{0}, hold{0}, deinits{0}, after{0};
  std::atomic<int> deinitsInInit{-1}, done{0}, waiting{0}, scheduled{0};
  std::atomic<int> holding{0}, deinitWaits{0};
  std::vector<int> order;             // the order notes 19 and 20 init in
  bool before = false;
  int releases = 0;                   // holds let go, under mutex
  cs_float scheduledAt = -1;          // the scheduled note's p2, under mutex
  cs_double scheduledPlays = -1;      // the score time it first performs at
  int deinitsInPerformance = -1;

  static RealtimeRewindOpcodeTests &host(CSOUND *cs)
  { return *static_cast<RealtimeRewindOpcodeTests *>(csoundGetHostData(cs)); }
  static int32_t noteInit(CSOUND *, void *) { return 0; }
  static int32_t notePerf(CSOUND *cs, void *)
  {
    auto &self = host(cs);
    ++self.performed;
    if (self.hold.exchange(0) == 0) return 0;
    std::unique_lock<std::mutex> lock(self.mutex);
    if (self.changed.wait_for(lock, std::chrono::seconds(5),
                              [&self] { return self.before; }))
      self.changed.wait_for(lock, std::chrono::milliseconds(200),
                            [&self] { return self.deinits > 0; });
    self.deinitsInPerformance = self.deinits;
    return 0;
  }
  static int32_t noteDeinit(CSOUND *cs, void *)
  {
    auto &self = host(cs);
    std::unique_lock<std::mutex> lock(self.mutex);
    ++self.deinits;
    self.changed.notify_all();
    // in the middle of the rewind, until an init is done
    if (self.deinitWaits)
      self.changed.wait_for(lock, std::chrono::seconds(5),
                            [&self] { return self.done > 0; });
    return 0;
  }
  static int32_t markBefore(CSOUND *cs, void *)
  {
    auto &self = host(cs);
    std::lock_guard<std::mutex> lock(self.mutex);
    self.before = true;
    self.changed.notify_all();
    return 0;
  }
  // The performance goes on for three periods while this init is not done.
  static int32_t markAfter(CSOUND *cs, void *)
  {
    auto &self = host(cs);
    const int from = self.performed;
    const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (self.performed < from + 3 &&
           std::chrono::steady_clock::now() < deadline)
      csoundSleep(1);
    self.deinitsInInit = self.deinits.load();
    ++self.after;
    return 0;
  }
  // An init that waits for a rewind to turn i1 off: in the middle of it.
  static int32_t waitForTheRewind(CSOUND *cs, void *)
  {
    auto &self = host(cs);
    self.waiting = 1;
    std::unique_lock<std::mutex> lock(self.mutex);
    self.changed.wait_for(lock, std::chrono::seconds(5),
                          [&self] { return self.deinits > 0; });
    return 0;
  }
  static int32_t markDone(CSOUND *cs, void *)
  {
    auto &self = host(cs);
    std::lock_guard<std::mutex> lock(self.mutex);
    ++self.done;
    self.changed.notify_all();
    return 0;
  }
  // An init held until the test lets it go: the nth hold, at its nth
  // release.
  static int32_t holdInit(CSOUND *cs, void *)
  {
    auto &self = host(cs);
    std::unique_lock<std::mutex> lock(self.mutex);
    const int n = ++self.holding;
    self.changed.wait_for(lock, std::chrono::seconds(5),
                          [&self, n] { return self.releases >= n; });
    return 0;
  }
  void release(int all = 0)
  {
    std::lock_guard<std::mutex> lock(mutex);
    releases = all ? 1 << 20 : releases + 1;
    changed.notify_all();
  }
  struct At {
    OPDS h;
    cs_float *time;
    int32_t played;
  };
  static int32_t markScheduled(CSOUND *cs, void *p)
  {
    auto &self = host(cs);
    std::lock_guard<std::mutex> lock(self.mutex);
    self.scheduledAt = *static_cast<At *>(p)->time;
    static_cast<At *>(p)->played = 0;
    ++self.scheduled;
    return 0;
  }
  static int32_t markOrder(CSOUND *cs, void *p)
  {
    auto &self = host(cs);
    std::lock_guard<std::mutex> lock(self.mutex);
    self.order.push_back(static_cast<int>(*static_cast<At *>(p)->time));
    return 0;
  }
  static int32_t markScheduledPlays(CSOUND *cs, void *p)
  {
    auto &self = host(cs);
    if (static_cast<At *>(p)->played++ == 0) {
      std::lock_guard<std::mutex> lock(self.mutex);
      self.scheduledPlays = csoundGetScoreTime(cs);
    }
    return 0;
  }
  static void message(CSOUND *, int32_t, const char *) {}
  void SetUp() override
  {
#if !CSOUND_SPINLOCK_AVAILABLE || \
    (!defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN))
    GTEST_SKIP() << "Realtime mode requires native spinlocks and atomics";
#endif
    csound = csoundCreate(this, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundSetMessageStringCallback(csound, message);
    ASSERT_EQ(csoundSetOption(csound, "-n -d --daemon --realtime"), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_rewind_wait", sizeof(OPDS), 0,
                                "", "", waitForTheRewind, nullptr, nullptr), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_rewind_done", sizeof(OPDS), 0,
                                "", "", markDone, nullptr, nullptr), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_rewind_hold", sizeof(OPDS), 0,
                                "", "", holdInit, nullptr, nullptr), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_rewind_order", sizeof(At), 0,
                                "", "i", markOrder, nullptr, nullptr), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_rewind_scheduled", sizeof(At),
                                0, "", "i", markScheduled, markScheduledPlays,
                                nullptr), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_rewind_note", sizeof(OPDS), 0,
                                "", "", noteInit, notePerf, noteDeinit), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_rewind_before", sizeof(OPDS),
                                0, "", "", markBefore, nullptr, nullptr), 0);
    ASSERT_EQ(csoundAppendOpcode(csound, "test_rewind_after", sizeof(OPDS),
                                0, "", "", markAfter, nullptr, nullptr), 0);
    ASSERT_EQ(csoundCompileOrc(csound, R"(
      sr = 48000
      ksmps = 16
      nchnls = 1
      0dbfs = 1
      instr 1
        test_rewind_note
      endin
      instr 2
        test_rewind_before
        rewindscore
        test_rewind_after
      endin
      instr 4
        test_rewind_before
        iResult compilestr "rewindscore\n"
        test_rewind_after
      endin
      instr 5
        rewindscore
      endin
      instr 6
        test_rewind_wait
        rewindscore
        event_i "i", 8, 0, 0.01
        test_rewind_done
      endin
      instr 7
        rewindscore
        event_i "i", 8, 0.05, 0.01
      endin
      instr 8
        test_rewind_scheduled p2
      endin
      instr 9
        setscorepos 0
        event_i "i", 8, 0.05, 0.01
      endin
      instr 10
        rewindscore
        scoreline_i "i 8 0.05 0.01"
        test_rewind_hold
      endin
      instr 11
        rewindscore
        event_i "i", 8, 0.05, 0.01
        test_rewind_wait
      endin
      instr 12
        rewindscore
        event_i "i", 8, 0.05, 0.01
        rewindscore
        event_i "i", 8, 0.07, 0.01
      endin
      instr 13
        rewindscore
        setscorepos 5
      endin
      instr 14
        rewindscore
        test_rewind_before
        event_i "i", 8, 0
        test_rewind_done
      endin
      instr 15
        rewindscore
        event_i "i", 8, 0.15, 0.01
        setscorepos 0.1
      endin
      instr 16
        rewindscore
        scoreline_i "i 8 0.05 0.01"
        rewindscore
      endin
      instr 17
        scoreline_i "i 8 0.05 0.01"
        rewindscore
        test_rewind_hold
      endin
      instr 18
        rewindscore
        event_i "i", 20, 0, 0.01
        test_rewind_hold
        event_i "i", 19, 0, 0.01
        test_rewind_hold
      endin
      instr 19, 20
        test_rewind_order p1
      endin
      instr 21
        rewindscore
        event_i "i", 8, 0.05, 0.01
        test_rewind_hold
        rewindscore
        event_i "i", 8, 0.07, 0.01
      endin
      instr 22
        rewindscore
        event_i "i", 8, 0.05, 0.01
        test_rewind_hold
      endin
      instr 23
        rewindscore
        event_i "i", 8, 0.05, 0.01
        test_rewind_wait
        event_i "i", 8, 0.06, 0.01
        test_rewind_done
      endin
      instr 25
        rewindscore
        test_rewind_hold
        setscorepos 5
      endin
      instr 26
        rewindscore
        setscorepos 0.1
        event_i "i", 8, 0.15, 0.01
        setscorepos 0.05
      endin
    )", 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
  }
  void TearDown() override
  {
    release(1);
    if (csound != nullptr) csoundDestroy(csound);
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
  void rewindThenSchedule(const char *note, cs_float at = 0.05);
  void askAndWait(const char *note, int periods);
  // i1 performs; the init of `rewinder` runs while i1's performance is held.
  void rewindWhileANotePerforms(const char *rewinder)
  {
    csoundEventString(csound, "i1 0 -1", 0);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (performed == 0 && std::chrono::steady_clock::now() < deadline) {
      ASSERT_EQ(csoundPerformKsmps(csound), 0);
      csoundSleep(1);
    }
    ASSERT_GT(performed, 0);
    hold = 1;
    csoundEventString(csound, rewinder, 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    // The rewind did not turn i1 off under its performance, nor before the
    // init that asked for it was done: the start of a later control period
    // did.
    EXPECT_EQ(deinitsInPerformance, 0);
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (deinits == 0 && std::chrono::steady_clock::now() < deadline) {
      ASSERT_EQ(csoundPerformKsmps(csound), 0);
      csoundSleep(1);
    }
    EXPECT_EQ(after, 1);
    EXPECT_EQ(deinitsInInit, 0);
    EXPECT_EQ(deinits, 1);
    // One rewind: the score's time goes on from it.
    const int64_t time = csoundGetCurrentTimeSamples(csound);
    for (int i = 0; i < 10; ++i)
      ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(csoundGetCurrentTimeSamples(csound), time + 10 * 16);
  }
};

TEST_F(RealtimeRewindOpcodeTests, ThePerformanceMakesTheRewind)
{
  rewindWhileANotePerforms("i2 0 0");
}

TEST_F(RealtimeRewindOpcodeTests, ThePerformanceMakesARewindFromCompilestr)
{
  rewindWhileANotePerforms("i4 0 0");
}

// 0.2 s performed, then `note` is sent; one period queues its init, which
// is done while the host does not perform; then 0.2 s more.
void RealtimeRewindOpcodeTests::rewindThenSchedule(const char *note,
                                                   cs_float at)
{
  for (int i = 0; i < 600; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  csoundEventString(csound, note, 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (alloc_queue_has_pending(csound) &&
         std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_FALSE(alloc_queue_has_pending(csound));
  for (int i = 0; i < 600; ++i) {     // each period once the init thread
    ASSERT_EQ(csoundPerformKsmps(csound), 0);   // is done: no lag
    ASSERT_TRUE(drain());
  }
  ASSERT_TRUE(drain());
  // The note the init scheduled after the rewind plays, `at` after it.
  EXPECT_EQ(scheduled, 1);
  std::lock_guard<std::mutex> lock(mutex);
  EXPECT_DOUBLE_EQ(scheduledAt, at);
  EXPECT_GE(scheduledPlays, at);
  EXPECT_LT(scheduledPlays, at + 0.1);
}

TEST_F(RealtimeRewindOpcodeTests, ANoteScheduledAfterRewindscorePlays)
{
  rewindThenSchedule("i7 0 0");
}

TEST_F(RealtimeRewindOpcodeTests, ANoteScheduledAfterSetscoreposPlays)
{
  rewindThenSchedule("i9 0 0");
}

TEST_F(RealtimeRewindOpcodeTests, ANoteScheduledAfterTheSecondOfTwoAsksPlays)
{
  // Two rewinds ask in one init, a note after each: the second rewind drops
  // the first note, and the second plays, 0.07 s after it.
  rewindThenSchedule("i12 0 0", 0.07);
}

TEST_F(RealtimeRewindOpcodeTests, ARewindDuringTheAskingInitServesIt)
{
  // i5 asks for a rewind; i11 asks too, schedules a note and waits: i5's
  // rewind comes while i11 waits, and serves its ask. The note goes in from
  // that rewind's time origin, and there is no second rewind.
  csoundEventString(csound, "i1 0 -1", 0);
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (performed == 0 && std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  ASSERT_GT(performed, 0);
  csoundEventString(csound, "i5 0 0\ni11 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (waiting == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(waiting, 1);
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (deinits == 0 && std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  ASSERT_EQ(deinits, 1);
  int periods = 1;                    // the rewind's own, which turned i1 off
  ASSERT_TRUE(drain());               // i11 done, no period after the rewind
  for (int i = 0; i < 600; ++i, ++periods) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_TRUE(drain());
  }
  EXPECT_EQ(csoundGetCurrentTimeSamples(csound), periods * 16);
  EXPECT_EQ(scheduled, 1);
  std::lock_guard<std::mutex> lock(mutex);
  EXPECT_DOUBLE_EQ(scheduledAt, 0.05);
  EXPECT_GE(scheduledPlays, 0.05);
  EXPECT_LT(scheduledPlays, 0.15);
}

// 0.2 s performed, then `note` is sent; one period queues its init, which
// is done while the host does not perform; then `periods` more.
void RealtimeRewindOpcodeTests::askAndWait(const char *note, int periods)
{
  for (int i = 0; i < 600; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  csoundEventString(csound, note, 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  ASSERT_TRUE(drain());
  for (int i = 0; i < periods; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  ASSERT_TRUE(drain());
}

TEST_F(RealtimeRewindOpcodeTests, SetscoreposAfterRewindscoreMovesTheScoreOnce)
{
  // The rewind takes the score to 5 s, and the advance setscorepos would
  // have made from the time before it is not made as well.
  askAndWait("i13 0 0", 10);
  EXPECT_GE(csoundGetScoreTime(csound), 5.0);
  EXPECT_LT(csoundGetScoreTime(csound), 5.01);
}

TEST_F(RealtimeRewindOpcodeTests, SetscoreposBackAfterRewindscoreDropsWhatCameBetween)
{
  // After rewindscore, setscorepos 0.1, a note at 0.15, then setscorepos
  // 0.05: the seek back rewinds again, as it would without --realtime, and
  // drops the note. The score goes on from 0.05.
  askAndWait("i26 0 0", 1);
  for (int i = 0; i < 600; ++i) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_TRUE(drain());
  }
  EXPECT_EQ(scheduled, 0);
  EXPECT_GE(csoundGetScoreTime(csound), 0.05 + 600 * 16 / 48000.0);
  EXPECT_LT(csoundGetScoreTime(csound), 0.06 + 600 * 16 / 48000.0);
}

TEST_F(RealtimeRewindOpcodeTests, ABadEventAfterRewindscoreFailsTheInit)
{
  // An event that cannot be scheduled (too few p-fields) is refused at
  // once, as without a rewind: the init stops there.
  askAndWait("i14 0 0", 10);
  std::lock_guard<std::mutex> lock(mutex);
  EXPECT_TRUE(before);
  EXPECT_EQ(done, 0);
}

TEST_F(RealtimeRewindOpcodeTests, SetscoreposForwardKeepsWhatWasScheduled)
{
  // setscorepos 0.1 after the rewind is a seek forward, from the rewind's
  // time 0, though the time before the rewind was 0.2 s: it asks for no
  // second rewind, and the note scheduled before it, at 0.15, plays.
  askAndWait("i15 0 0", 600);
  EXPECT_EQ(scheduled, 1);
  std::lock_guard<std::mutex> lock(mutex);
  EXPECT_DOUBLE_EQ(scheduledAt, 0.15);
}

TEST_F(RealtimeRewindOpcodeTests, ALineSentBetweenTwoAsksPlays)
{
  // A rewind does not empty the line buffer: a line sent between two asks
  // is read after both.
  rewindThenSchedule("i16 0 0");
}

TEST_F(RealtimeRewindOpcodeTests, ALineSentBeforeTheAskPlays)
{
  // The line is sent before rewindscore, and the init goes on for five
  // periods: it is read after the rewind, as without --realtime.
  for (int i = 0; i < 600; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  csoundEventString(csound, "i17 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (holding == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(holding, 1);
  for (int i = 0; i < 5; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  release();
  ASSERT_TRUE(drain());
  for (int i = 0; i < 600; ++i) {     // each period once the init thread
    ASSERT_EQ(csoundPerformKsmps(csound), 0);   // is done: no lag
    ASSERT_TRUE(drain());
  }
  ASSERT_TRUE(drain());
  EXPECT_EQ(scheduled, 1);
  std::lock_guard<std::mutex> lock(mutex);
  EXPECT_DOUBLE_EQ(scheduledAt, 0.05);
  EXPECT_GE(scheduledPlays, 0.05);
  EXPECT_LT(scheduledPlays, 0.15);
}

TEST_F(RealtimeRewindOpcodeTests, WhatAServedAskHeldGoesInFirst)
{
  // i5's ask is counted; i18 asks, schedules i20 and is held while the
  // performance makes i5's rewind, which serves i18's ask; then i18
  // schedules i19 and is held again for five periods: i20, held for the
  // rewind, comes first, not when i18 is done.
  csoundEventString(csound, "i5 0 0\ni18 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (holding == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(holding, 1);
  for (int i = 0; i < 5; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  release();
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (holding < 2 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(holding, 2);
  for (int i = 0; i < 5; ++i) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  release();
  ASSERT_TRUE(drain());
  for (int i = 0; i < 5; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  ASSERT_TRUE(drain());
  std::lock_guard<std::mutex> lock(mutex);
  EXPECT_EQ(order, (std::vector<int>{20, 19}));
}

TEST_F(RealtimeRewindOpcodeTests, AnAskAfterAServedOneIsMade)
{
  // i21 asks, schedules a note and is held while the performance makes
  // i5's rewind, which serves its ask; then it asks again and schedules
  // another. The second rewind drops the first note and is made, and the
  // second note plays, 0.07 s after it.
  csoundEventString(csound, "i5 0 0\ni21 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (holding == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(holding, 1);
  for (int i = 0; i < 5; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  release();
  for (int i = 0; i < 600; ++i) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_TRUE(drain());
  }
  ASSERT_TRUE(drain());
  EXPECT_EQ(scheduled, 1);
  std::lock_guard<std::mutex> lock(mutex);
  EXPECT_DOUBLE_EQ(scheduledAt, 0.07);
}

TEST_F(RealtimeRewindOpcodeTests, WhatAnAskingInitSchedulesDuringTheRewindPlays)
{
  // The rewind i5 asked for serves i23's ask. i1's deinit, in the middle of
  // that rewind, lets i23 go on, and waits until i23 is done: i23 holds its
  // second note too, as the rewind is under way, and puts in both once the
  // rewind has dropped what was pending and reset the time. Both notes
  // play, from the rewind.
  csoundEventString(csound, "i1 0 -1", 0);
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (performed == 0 && std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  ASSERT_GT(performed, 0);
  deinitWaits = 1;
  csoundEventString(csound, "i5 0 0\ni23 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (waiting == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(waiting, 1);
  for (int i = 0; i < 600; ++i) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_TRUE(drain());
  }
  ASSERT_TRUE(drain());
  EXPECT_EQ(done, 1);
  EXPECT_EQ(scheduled, 2);
}

TEST_F(RealtimeRewindOpcodeTests, SetscoreposAfterAServedAskSeeksFromTheRewind)
{
  // i5's ask is counted; i25 asks and is held while the performance makes
  // i5's rewind, which serves i25's ask; then i25 seeks forward to 5 s,
  // from the rewound time.
  csoundEventString(csound, "i5 0 0\ni25 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (holding == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(holding, 1);
  for (int i = 0; i < 5; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  release();
  ASSERT_TRUE(drain());
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  EXPECT_GE(csoundGetScoreTime(csound), 5.0);
  EXPECT_LT(csoundGetScoreTime(csound), 5.01);
}

TEST_F(RealtimeRewindOpcodeTests, TwoRewindsDuringTheAskingInitDropWhatItHeld)
{
  // i22 asks and schedules a note; the host rewinds twice while i22's init
  // goes on. The first rewind serves the ask, and the second drops what it
  // would have put in.
  for (int i = 0; i < 600; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  csoundEventString(csound, "i22 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (holding == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(holding, 1);
  csoundRewindScore(csound);
  csoundRewindScore(csound);
  release();
  ASSERT_TRUE(drain());
  for (int i = 0; i < 600; ++i) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_TRUE(drain());
  }
  ASSERT_TRUE(drain());
  EXPECT_EQ(scheduled, 0);
}

TEST_F(RealtimeRewindOpcodeTests, ALineScheduledAfterRewindscorePlays)
{
  // scoreline_i's line goes to the line buffer, which the performance reads
  // each period. The asking init goes on for five periods after it: the
  // line is read after the rewind all the same, and its note plays 0.05 s
  // after it.
  for (int i = 0; i < 600; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  csoundEventString(csound, "i10 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (holding == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(holding, 1);
  for (int i = 0; i < 5; ++i)
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
  release();
  ASSERT_TRUE(drain());
  for (int i = 0; i < 600; ++i) {     // each period once the init thread
    ASSERT_EQ(csoundPerformKsmps(csound), 0);   // is done: no lag
    ASSERT_TRUE(drain());
  }
  ASSERT_TRUE(drain());
  EXPECT_EQ(scheduled, 1);
  std::lock_guard<std::mutex> lock(mutex);
  EXPECT_DOUBLE_EQ(scheduledAt, 0.05);
  EXPECT_GE(scheduledPlays, 0.05);
  EXPECT_LT(scheduledPlays, 0.15);
}

TEST_F(RealtimeRewindOpcodeTests, ARewindServesGlobalCodeThatAskedBeforeIt)
{
  // i5 asks for a rewind. Global code compiled after it asks too, and waits
  // for that rewind, which serves both: there is no second one.
  csoundEventString(csound, "i1 0 -1", 0);
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (performed == 0 && std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  ASSERT_GT(performed, 0);
  csoundEventString(csound, "i5 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  ASSERT_EQ(csoundCompileOrc(csound, "rewindscore\ntest_rewind_wait\n", 1), 0);
  // No period until the global code waits: i5's rewind comes in the next.
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (waiting == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(waiting, 1);
  int periods = 0;
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while ((deinits == 0 || alloc_queue_has_pending(csound)) &&
         std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ++periods;
    csoundSleep(1);
  }
  ASSERT_EQ(deinits, 1);
  ASSERT_FALSE(alloc_queue_has_pending(csound));
  for (int i = 0; i < 10; ++i, ++periods) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  EXPECT_EQ(csoundGetCurrentTimeSamples(csound), periods * 16);
}

TEST_F(RealtimeRewindOpcodeTests, GlobalCodeAskingAfterARewindGetsItsOwn)
{
  // i5 asks for a rewind. Global code compiled after it waits for that
  // rewind, then asks: global code is not a note the rewind turns off, and
  // it gets a rewind of its own.
  csoundEventString(csound, "i1 0 -1", 0);
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (performed == 0 && std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  ASSERT_GT(performed, 0);
  csoundEventString(csound, "i5 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  ASSERT_EQ(csoundCompileOrc(csound, "test_rewind_wait\nrewindscore\n", 1), 0);
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (waiting == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(waiting, 1);
  int periods = 0;
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while ((deinits == 0 || alloc_queue_has_pending(csound)) &&
         std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ++periods;
    csoundSleep(1);
  }
  ASSERT_EQ(deinits, 1);
  ASSERT_FALSE(alloc_queue_has_pending(csound));
  for (int i = 0; i < 10; ++i, ++periods) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  // The first rewind came at the start of the first period: the second one,
  // later, set the score's time back again.
  EXPECT_LT(csoundGetCurrentTimeSamples(csound), periods * 16);
  EXPECT_GE(csoundGetCurrentTimeSamples(csound), 10 * 16);
}

TEST_F(RealtimeRewindOpcodeTests, AnInitFromBeforeARewindAsksForNone)
{
  // i5 and i6 are queued together. i5 asks for a rewind, which comes while
  // i6's init waits for it and turns i6 off: i6's rewindscore, after the
  // rewind, asks for none.
  csoundEventString(csound, "i1 0 -1", 0);
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (performed == 0 && std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  ASSERT_GT(performed, 0);
  csoundEventString(csound, "i5 0 0\ni6 0 0", 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  // No period until i6's init has begun: i5's rewind comes in the next.
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (waiting == 0 && std::chrono::steady_clock::now() < deadline)
    csoundSleep(1);
  ASSERT_EQ(waiting, 1);
  int periods = 0;
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (done == 0 && std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ++periods;
    csoundSleep(1);
  }
  ASSERT_EQ(done, 1);
  EXPECT_EQ(deinits, 1);
  // i6's item is done, and would have asked by now.
  while (alloc_queue_has_pending(csound) &&
         std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ++periods;
    csoundSleep(1);
  }
  ASSERT_FALSE(alloc_queue_has_pending(csound));
  for (int i = 0; i < 10; ++i, ++periods) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    csoundSleep(1);
  }
  // One rewind, at the start of the first of these periods: the score's
  // time counts them all.
  EXPECT_EQ(csoundGetCurrentTimeSamples(csound), periods * 16);
  // And i6's note, scheduled after its rewindscore, goes with i6.
  ASSERT_TRUE(drain());
  EXPECT_EQ(scheduled, 0);
}

} // namespace

#endif
