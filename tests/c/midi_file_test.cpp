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

TEST_F(MidiFileReadTests, SharesCapacityAcrossFiles)
{
  checkRead(10, {0xC0, 0x81, 5, 0x90, 0x81, 60, 100, 0xC0, 0x82, 5});
  checkRead(4, {0x90, 0x82, 60, 100});
  checkRead(10, {});
}
