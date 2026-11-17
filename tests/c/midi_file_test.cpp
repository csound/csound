#include "gtest/gtest.h"

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "../../H/midifile.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

class MidiFileReadTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;
  std::filesystem::path path, loop;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
#if defined(_WIN32)
    const auto pid = _getpid();
#else
    const auto pid = getpid();
#endif
    path = std::filesystem::temp_directory_path() /
      ("csound-midi-read-" + std::to_string(pid) + "-" +
       std::to_string(reinterpret_cast<uintptr_t>(csound)) + ".mid");
    // Program change and note-on at tick zero, followed by end of track.
    const unsigned char midi[] = {
      'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 1, 224,
      'M', 'T', 'r', 'k', 0, 0, 0, 11,
      0, 0xC0, 5, 0, 0x90, 60, 100, 0, 0xFF, 0x2F, 0
    };
    std::ofstream file(path, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file.write(reinterpret_cast<const char *>(midi), sizeof(midi));
    file.close();
    ASSERT_TRUE(file.good());

    const std::string orc =
      "sr=1000\nksmps=1\nnchnls=1\n"
      "iFirst midifileopen {{" + path.generic_string() + "}}, 1\n"
      "iSecond midifileopen {{" + path.generic_string() + "}}, 2\n"
      "midifileplay iFirst\nmidifileplay iSecond\n";
    ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
    ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
  }

  void TearDown() override
  {
    if (csound != nullptr)
      csoundDestroy(csound);
    std::error_code error;
    if (!path.empty())
      std::filesystem::remove(path, error);
    if (!loop.empty())
      std::filesystem::remove(loop, error);
  }

  void checkRead(int capacity, std::initializer_list<unsigned char> expected)
  {
    std::array<unsigned char, 16> buffer;
    buffer.fill(0x55);
    ASSERT_EQ(csoundMIDIFileRead(csound, buffer.data(), capacity),
              static_cast<int>(expected.size()));
    size_t index = 0;
    for (unsigned char byte : expected)
      EXPECT_EQ(buffer[index++], byte);
    for (; index < buffer.size(); ++index)
      EXPECT_EQ(buffer[index], 0x55) << "byte " << index;
  }
};

TEST_F(MidiFileReadTests, CountsPortBytesAndKeepsEventsThatDoNotFit)
{
  checkRead(0, {});
  checkRead(2, {});
  checkRead(3, {0xC0, 0x81, 5});
  checkRead(3, {0xC0, 0x82, 5});
  checkRead(4, {0x90, 0x81, 60, 100});
  checkRead(4, {0x90, 0x82, 60, 100});
  checkRead(4, {});
}

TEST_F(MidiFileReadTests, ALoopedFileIsReadAgainOnlyInALaterCycle)
{
  // Every event of the files, and their end, are at tick zero. A looped one
  // is read again once file time has passed, not in the same cycle, where
  // the reader would never run dry.
  ASSERT_EQ(csoundCompileOrc(csound, "midifileloop 1, 1\n", 0), 0);
  checkRead(16, {0xC0, 0x81, 5, 0x90, 0x81, 60, 100,
                 0xC0, 0x82, 5, 0x90, 0x82, 60, 100});
  checkRead(16, {});
  ASSERT_EQ(csoundPerformKsmps(csound), 0);
  checkRead(16, {0xC0, 0x81, 5, 0x90, 0x81, 60, 100});
  checkRead(16, {});
}

TEST_F(MidiFileReadTests, ALoopedFileKeepsALengthBetweenKPeriods)
{
  // 400 ticks a beat at 120 bpm, and 1000 k-periods a second, make a tick
  // 1.25 k-periods. Looped, a file whose event is at tick 0 and its end at
  // tick 1 gives its event 4 times in 5 cycles, not once a cycle.
  loop = path.parent_path() / ("loop-" + path.filename().string());
  const unsigned char midi[] = {
    'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 1, 0x90,
    'M', 'T', 'r', 'k', 0, 0, 0, 8,
    0, 0xB0, 7, 100, 1, 0xFF, 0x2F, 0
  };
  {
    std::ofstream file(loop, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file.write(reinterpret_cast<const char *>(midi), sizeof(midi));
    ASSERT_TRUE(file.good());
  }
  const std::string orc = "iLoop midifileopen {{" + loop.generic_string() +
    "}}, 3\nmidifileplay iLoop\nmidifileloop 1, iLoop\n";
  ASSERT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0);
  std::array<unsigned char, 64> buffer;
  ASSERT_GT(csoundMIDIFileRead(csound, buffer.data(), 64), 0);
  int passes = 0;
  for (int cycle = 1; cycle <= 20; ++cycle) {
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    passes += csoundMIDIFileRead(csound, buffer.data(), 64) / 4;
  }
  EXPECT_EQ(passes, 16);
}

TEST(MidiFileRewindTests, AnEmptyFileNeverPlayedStaysPaused)
{
  // A rewind of the score plays again a file that had played to its end.
  // A file with no events at all, which the orchestra never played, is at
  // its end too, and stays paused.
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  ASSERT_NE(csound, nullptr);
#if defined(_WIN32)
  const auto pid = _getpid();
#else
  const auto pid = getpid();
#endif
  const auto path = std::filesystem::temp_directory_path() /
    ("csound-midi-empty-" + std::to_string(pid) + ".mid");
  const unsigned char midi[] = {
    'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 1, 224,
    'M', 'T', 'r', 'k', 0, 0, 0, 4, 0, 0xFF, 0x2F, 0
  };
  {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(midi), sizeof(midi));
  }
  const std::string orc =
    "sr=1000\nksmps=1\nnchnls=1\n"
    "giFile midifileopen {{" + path.generic_string() + "}}\n"
    "instr 1\nchnset midifilestatus:k(giFile), \"playing\"\nendin\n";
  EXPECT_EQ(csoundSetOption(csound, "-n"), 0);
  EXPECT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0);
  EXPECT_EQ(csoundStart(csound), 0);
  EXPECT_EQ(csoundPerformKsmps(csound), 0);
  csoundRewindScore(csound);
  csoundEventString(csound, "i1 0 1", 0);
  EXPECT_EQ(csoundPerformKsmps(csound), 0);
  EXPECT_EQ(csoundGetControlChannel(csound, "playing", nullptr), 0);
  csoundDestroy(csound);
  std::error_code error;
  std::filesystem::remove(path, error);
}

namespace {
struct ReopenCounts {
  int opens = 0, notes = 0;
};

int32_t countOpen(CSOUND *csound, void *)
{
  ++static_cast<ReopenCounts *>(csoundGetHostData(csound))->opens;
  return 0;
}

int32_t countNote(CSOUND *csound, void *)
{
  ++static_cast<ReopenCounts *>(csoundGetHostData(csound))->notes;
  return 0;
}

// A note of the score opens a MIDI file and plays it, read through -F with
// no file name: a note on at 0 ms, its note off at 10 ms, a second note at
// `later` ms if given, and the file's end at `end` ms (500 ticks a beat at 120 bpm make a tick a millisecond). Gives
// the opens and note ons of each of three passes of the score, rewound after
// 100 ms each time.
// How the score's note opens the file: in its init, in code its init
// compiles, or in an instance it creates and its performance inits.
enum class Opener { Init, Compiled, Created };

std::vector<ReopenCounts> passesOfAScoreThatOpensAFile(int end,
                                                       Opener how = Opener::Init,
                                                       int later = 0)
{
  std::vector<ReopenCounts> passes;
  ReopenCounts counts;
  CSOUND *csound = csoundCreate(&counts, nullptr);
  if (csound == nullptr)
    return passes;
#if defined(_WIN32)
  const auto pid = _getpid();
#else
  const auto pid = getpid();
#endif
  const auto path = std::filesystem::temp_directory_path() /
    ("csound-midi-reopen-" + std::to_string(pid) + "-" + std::to_string(end) +
     ".mid");
  std::vector<unsigned char> midi = {
    'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 0x01, 0xF4,
    'M', 'T', 'r', 'k', 0, 0, 0, 0,
    0, 0x90, 60, 100, 10, 0x80, 60, 0
  };
  const auto delta = [&midi](int ms) {
    if (ms >= 128)
      midi.push_back(static_cast<unsigned char>(0x80 | (ms >> 7)));
    midi.push_back(static_cast<unsigned char>(ms & 0x7F));
  };
  int at = 10;
  if (later > 0) {                    // a second note, on at `later` ms
    delta(later - at);
    midi.insert(midi.end(), {0x90, 62, 100});
    delta(10);
    midi.insert(midi.end(), {0x80, 62, 0});
    at = later + 10;
  }
  delta(end - at);
  midi.insert(midi.end(), {0xFF, 0x2F, 0});
  midi[21] = static_cast<unsigned char>(midi.size() - 22);
  {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(midi.data()),
               static_cast<std::streamsize>(midi.size()));
  }
  csoundAppendOpcode(csound, "test_count_open", sizeof(OPDS), 0, "", "",
                     countOpen, nullptr, nullptr);
  csoundAppendOpcode(csound, "test_count_note", sizeof(OPDS), 0, "", "",
                     countNote, nullptr, nullptr);
  const std::string open =
    "iFile midifileopen \"" + path.generic_string() + "\"\n"
    "midifileplay iFile\n"
    "test_count_open\n";
  std::string escaped;
  for (char c : open)
    escaped += c == '"' ? std::string("\\\"") :
               c == '\n' ? std::string("\\n") : std::string(1, c);
  const std::string orc =
    "sr=1000\nksmps=1\nnchnls=1\nmassign 0, 2\n"
    "instr Opener\n" + open + "endin\n"
    "instr 1\n" +
    (how == Opener::Compiled ? "iResult compilestr \"" + escaped + "\"\n" :
     how == Opener::Created ?
       "ins:Instr = create(Opener)\nerr:k = perf(ins)\ndelete(ins)\n" :
       open) +
    "endin\n"
    "instr 2\ntest_count_note\nendin\n";
  if (csoundSetOption(csound, "-n") == 0 && csoundSetOption(csound, "-F") == 0 &&
      csoundCompileOrc(csound, orc.c_str(), 0) == 0) {
    csoundEventString(csound, "i1 0 .001\nf0 1\n", 0);
    if (csoundStart(csound) == 0) {
      for (int pass = 0; pass < 3; ++pass) {
        counts = ReopenCounts();
        for (int i = 0; i < 100; ++i)
          csoundPerformKsmps(csound);
        passes.push_back(counts);
        csoundRewindScore(csound);
      }
    }
  }
  csoundDestroy(csound);
  std::error_code error;
  std::filesystem::remove(path, error);
  return passes;
}
}  // namespace

TEST(MidiFileRewindTests, ACopyANoteOpenedAndThatEndedStaysEnded)
{
  // The score opens a new copy of the file on every pass: the copies of the
  // earlier passes, which ended, do not play again beside it.
  const auto passes = passesOfAScoreThatOpensAFile(20);
  ASSERT_EQ(passes.size(), 3u);
  for (const auto &pass : passes) {
    EXPECT_EQ(pass.opens, 1);
    EXPECT_EQ(pass.notes, 1);
  }
}

TEST(MidiFileRewindTests, ACopyANoteCompiledStaysEnded)
{
  // The note compiles the code that opens the file: that code runs on every
  // pass too, and the copies of the earlier passes stay ended.
  const auto passes = passesOfAScoreThatOpensAFile(20, Opener::Compiled);
  ASSERT_EQ(passes.size(), 3u);
  for (const auto &pass : passes) {
    EXPECT_EQ(pass.opens, 1);
    EXPECT_EQ(pass.notes, 1);
  }
}

TEST(MidiFileRewindTests, ACopyAnInstanceANoteCreatedOpenedStaysEnded)
{
  // The note creates an instance, which its performance inits: the
  // instance opens the file on every pass, and the copies of the earlier
  // passes stay ended.
  const auto passes = passesOfAScoreThatOpensAFile(20, Opener::Created);
  ASSERT_EQ(passes.size(), 3u);
  for (const auto &pass : passes) {
    EXPECT_EQ(pass.opens, 1);
    EXPECT_EQ(pass.notes, 1);
  }
}

TEST(MidiFileRewindTests, ACopyANoteOpenedGoesOnThroughARewind)
{
  // Still playing at the rewind, the copy goes on from where it was: its
  // second note, at 150 ms, comes 50 ms into the next pass, beside the new
  // copy's first, and it does not start again (1, 2, 3 note ons), nor stop
  // (1, 1, 1).
  const auto passes = passesOfAScoreThatOpensAFile(500, Opener::Init, 150);
  ASSERT_EQ(passes.size(), 3u);
  const int notes[] = {1, 2, 2};
  for (size_t pass = 0; pass < passes.size(); ++pass) {
    EXPECT_EQ(passes[pass].opens, 1);
    EXPECT_EQ(passes[pass].notes, notes[pass]);
  }
}

TEST_F(MidiFileReadTests, SharesCapacityAcrossFiles)
{
  checkRead(10, {0xC0, 0x81, 5, 0x90, 0x81, 60, 100, 0xC0, 0x82, 5});
  checkRead(4, {0x90, 0x82, 60, 100});
  checkRead(10, {});
}

TEST(MidiFileLengthTests, IsNotRoundedToKPeriods)
{
  // 96 ticks a beat at 120 bpm, and the end of the track at tick 384: the
  // file lasts 2 s, which are 1378.125 k-periods at sr 44100 and ksmps 64.
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  ASSERT_NE(csound, nullptr);
#if defined(_WIN32)
  const auto pid = _getpid();
#else
  const auto pid = getpid();
#endif
  const auto path = std::filesystem::temp_directory_path() /
    ("csound-midi-length-" + std::to_string(pid) + ".mid");
  const unsigned char midi[] = {
    'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 0, 96,
    'M', 'T', 'r', 'k', 0, 0, 0, 9,
    0, 0xB0, 9, 1, 0x83, 0x00, 0xFF, 0x2F, 0
  };
  {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(midi), sizeof(midi));
  }
  const std::string orc =
    "sr=44100\nksmps=64\nnchnls=1\n"
    "iFile midifileopen {{" + path.generic_string() + "}}\n"
    "chnset midifilelen(iFile), \"length\"\n";
  EXPECT_EQ(csoundSetOption(csound, "-n"), 0);
  EXPECT_EQ(csoundCompileOrc(csound, orc.c_str(), 0), 0);
  EXPECT_EQ(csoundStart(csound), 0);
  EXPECT_DOUBLE_EQ(csoundGetControlChannel(csound, "length", nullptr), 2.0);
  csoundDestroy(csound);
  std::error_code error;
  std::filesystem::remove(path, error);
}
