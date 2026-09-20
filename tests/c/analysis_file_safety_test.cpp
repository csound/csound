#include "gtest/gtest.h"

#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "convolve.h"
#include "lpc.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {

uint64_t currentProcessId()
{
#if defined(_WIN32)
  return static_cast<uint64_t>(_getpid());
#else
  return static_cast<uint64_t>(getpid());
#endif
}

class AnalysisFileSafetyTests : public ::testing::Test {
 protected:
  void SetUp() override
  {
    const auto root = std::filesystem::temp_directory_path();
    const std::string prefix =
      "csound-analysis-file-tests-" + std::to_string(currentProcessId()) +
      "-" + std::to_string(reinterpret_cast<uintptr_t>(this)) + "-";

    for (uint32_t attempt = 0; attempt < 100; ++attempt) {
      const auto candidate = root / (prefix + std::to_string(attempt));
      std::error_code error;
      if (std::filesystem::create_directory(candidate, error)) {
        directory = candidate;
        return;
      }
      if (error && error != std::errc::file_exists)
        FAIL() << "could not create " << candidate << ": " << error.message();
    }
    FAIL() << "could not create a unique temporary directory in " << root;
  }

  void TearDown() override
  {
    if (directory.empty())
      return;
    std::error_code error;
    std::filesystem::remove_all(directory, error);
  }

  void writeFile(const std::filesystem::path &path,
                 const std::vector<uint8_t> &data)
  {
    std::ofstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open()) << "could not open " << path;
    stream.write(reinterpret_cast<const char *>(data.data()),
                 static_cast<std::streamsize>(data.size()));
    stream.close();
    ASSERT_TRUE(stream.good()) << "could not write " << path;
  }

  int32_t runFileOpcode(const std::string &statement, MYFLT *firstSample = nullptr)
  {
    std::string csd =
      "<CsoundSynthesizer>\n"
      "<CsOptions>\n-n -d -m0\n</CsOptions>\n"
      "<CsInstruments>\n"
      "sr = 44100\nksmps = 32\nnchnls = 1\n0dbfs = 1\n"
      "instr 1\n" + statement + "\nendin\n"
      "</CsInstruments>\n"
      "<CsScore>\ni 1 0 0.01\n</CsScore>\n"
      "</CsoundSynthesizer>\n";
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    int32_t result = csoundCompileCSD(csound, csd.c_str(), 1, 0);
    if (result == CSOUND_SUCCESS)
      result = csoundStart(csound);
    if (result == CSOUND_SUCCESS)
      result = csoundPerformKsmps(csound);
    if (result == CSOUND_SUCCESS && firstSample != nullptr)
      *firstSample = csoundGetSpout(csound)[0];
    csoundDestroy(csound);
    return result;
  }

  int32_t loadPvx(const std::filesystem::path &path)
  {
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    PVOCEX_MEMFILE file;
    int32_t result =
      csound->PVOCEX_LoadFile(csound, path.generic_string().c_str(), &file);
    csoundDestroy(csound);
    return result;
  }

  std::filesystem::path directory;
};

template <typename T>
void appendNative(std::vector<uint8_t> &data, T value)
{
  size_t offset = data.size();
  data.resize(offset + sizeof(T));
  std::memcpy(data.data() + offset, &value, sizeof(T));
}

void appendU16(std::vector<uint8_t> &data, uint16_t value)
{
  data.push_back(static_cast<uint8_t>(value));
  data.push_back(static_cast<uint8_t>(value >> 8));
}

void appendU32(std::vector<uint8_t> &data, uint32_t value)
{
  data.push_back(static_cast<uint8_t>(value));
  data.push_back(static_cast<uint8_t>(value >> 8));
  data.push_back(static_cast<uint8_t>(value >> 16));
  data.push_back(static_cast<uint8_t>(value >> 24));
}

void appendFloat(std::vector<uint8_t> &data, float value)
{
  uint32_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  appendU32(data, bits);
}

void appendTag(std::vector<uint8_t> &data, const char *tag)
{
  data.insert(data.end(), tag, tag + 4);
}

void putU32(std::vector<uint8_t> &data, size_t offset, uint32_t value)
{
  ASSERT_LE(offset + 4, data.size());
  data[offset] = static_cast<uint8_t>(value);
  data[offset + 1] = static_cast<uint8_t>(value >> 8);
  data[offset + 2] = static_cast<uint8_t>(value >> 16);
  data[offset + 3] = static_cast<uint8_t>(value >> 24);
}

std::vector<uint8_t> makeAtsFile()
{
  std::vector<uint8_t> data;
  const std::array<double, 13> values = {
    123.0, 44100.0, 512.0, 1024.0, 1.0, 1.0, 1.0,
    1000.0, 0.1, 1.0, 0.0, 0.5, 440.0
  };
  for (double value : values)
    appendNative(data, value);
  return data;
}

TEST_F(AnalysisFileSafetyTests, ConvolveRejectsIncompleteFiles)
{
  CVSTRUCT header = {};
  header.magic = CVMAGIC;
  header.headBsize = sizeof(header);
  header.dataBsize = 18 * sizeof(MYFLT);
  header.dataFormat = CVMYFLT;
  header.samplingRate = 44100;
  header.src_chnls = header.channel = 1;
  header.Hlen = 8;
  header.Format = CVRECT;
  std::vector<uint8_t> data;
  appendNative(data, header);
  data.resize(sizeof(header) + header.dataBsize, 0);
  auto path = directory / "complete.cv";
  ASSERT_NO_FATAL_FAILURE(writeFile(path, data));
  EXPECT_EQ(CSOUND_SUCCESS, runFileOpcode(
    "aInput init 0\naOut convolve aInput, \"" + path.generic_string() + "\""));
  for (size_t length : {sizeof(header) - 1, data.size() - sizeof(MYFLT)}) {
    auto shortPath = directory / ("short-" + std::to_string(length) + ".cv");
    ASSERT_NO_FATAL_FAILURE(writeFile(
      shortPath, std::vector<uint8_t>(data.begin(), data.begin() + length)));
    EXPECT_NE(CSOUND_SUCCESS, runFileOpcode(
      "aInput init 0\naOut convolve aInput, \"" + shortPath.generic_string() + "\""));
  }
}

TEST_F(AnalysisFileSafetyTests, ConvolveTextKeepsFinalValueWithoutNewline)
{
  /* The text format stores the writer's byte counts, but the loader uses
     the current MYFLT size. Keep that cross-build use working. */
  std::string text = "CVANAL\n44 72 36 44100 1 1 8 1\n";
  for (int i = 0; i < 17; ++i)
    text += "0\n";
  text += "1.25";
  auto path = directory / "no-final-newline.cv";
  ASSERT_NO_FATAL_FAILURE(writeFile(
    path, std::vector<uint8_t>(text.begin(), text.end())));
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  MEMFIL *file = csound->LoadMemoryFile(
    csound, path.generic_string().c_str(), CSFTYPE_CVANAL, nullptr);
  EXPECT_NE(nullptr, file);
  if (file != nullptr) {
    EXPECT_EQ(sizeof(CVSTRUCT) + 18 * sizeof(MYFLT), size_t(file->length));
    MYFLT last = 0;
    std::memcpy(&last, file->endp - sizeof(MYFLT), sizeof(MYFLT));
    EXPECT_EQ(MYFLT(1.25), last);
  }
  csoundDestroy(csound);
  EXPECT_EQ(CSOUND_SUCCESS, runFileOpcode(
    "aInput init 0\naOut convolve aInput, \"" + path.generic_string() + "\""));
}

/* Two frames with 42 partials, including frequencies above the highest
   noise band. Partial amplitudes are distinct so selection is observable. */
std::vector<uint8_t> makeSinnoiFile(int type, bool swapped)
{
  std::vector<uint8_t> data;
  for (double value : {123.0, 48000.0, 512.0, 1024.0, 42.0, 2.0,
                       84.0, 21000.0, 1.0, static_cast<double>(type)})
    appendNative(data, value);
  for (int frame = 0; frame < 2; ++frame) {
    appendNative(data, frame * 0.5);
    for (int partial = 0; partial < 42; ++partial) {
      appendNative(data, double((frame + 1) * (partial + 1)));
      appendNative(data, 21000.0);
      if (type == 2 || type == 4)
        appendNative(data, 0.0);
    }
    if (type >= 3)
      for (int band = 0; band < 25; ++band)
        appendNative(data, 1.0);
  }
  if (swapped)
    for (size_t offset = 0; offset < data.size(); offset += sizeof(double))
      std::reverse(data.begin() + offset, data.begin() + offset + sizeof(double));
  return data;
}

TEST_F(AnalysisFileSafetyTests, SinnoiPartialSelectionAndNoiseBounds)
{
  for (int type = 1; type <= 4; ++type) {
    for (bool swapped : {false, true}) {
      auto path = directory / ("sinnoi-" + std::to_string(type) +
                                (swapped ? "-swapped.ats" : ".ats"));
      ASSERT_NO_FATAL_FAILURE(writeFile(path, makeSinnoiFile(type, swapped)));
      /* Zero frequency scaling keeps every cosine at one. */
      const std::string file = ", 1, 0, 0, \"" + path.generic_string() + "\", ";
      MYFLT sample = 0;
      ASSERT_EQ(CSOUND_SUCCESS, runFileOpcode(
        "aOut ATSsinnoi 1" + file + "42\nout aOut", &sample));
      EXPECT_EQ(MYFLT(1806), sample); // all 42 partials in the last frame
      ASSERT_EQ(CSOUND_SUCCESS, runFileOpcode(
        "aOut ATSsinnoi 0.25" + file + "3, 37, 2\nout aOut", &sample));
      EXPECT_EQ(MYFLT(180), sample); // partials 38, 40, 42 halfway between frames
    }
  }
}

TEST_F(AnalysisFileSafetyTests, SinnoiRejectsInvalidSelection)
{
  auto path = directory / "sinnoi.ats";
  ASSERT_NO_FATAL_FAILURE(writeFile(path, makeSinnoiFile(3, false)));
  const std::string opcode =
    "aOut ATSsinnoi 0, 1, 0, 1, \"" + path.generic_string() + "\", ";
  for (const char *selection : {"-1", "43", "1, -1", "1, 42",
                                "3, 38, 2", "2, 0, 0", "2, 0, -1"})
    EXPECT_NE(CSOUND_SUCCESS, runFileOpcode(opcode + selection)) << selection;
}

std::vector<uint8_t> makeLpcFile(int frames)
{
  LPHEADER header = {};
  header.headersize = sizeof(header) + 4; // legacy four-byte padding
  header.lpmagic = LP_MAGIC;
  header.npoles = 2;
  header.nvals = 6;
  header.framrate = 100;
  header.srate = 44100;
  header.duration = MYFLT(frames) / 100;
  std::vector<uint8_t> data;
  appendNative(data, header);
  data.resize(header.headersize, 0);
  for (int frame = 0; frame < frames; ++frame)
    for (MYFLT value : {MYFLT(frame + 1), MYFLT(1), MYFLT(0),
                        MYFLT(100), MYFLT(0), MYFLT(0)})
      appendNative(data, value);
  return data;
}

TEST_F(AnalysisFileSafetyTests, LpreadSingleAndLongAnalysisFrames)
{
  for (int frames : {1, 40000}) {
    auto path = directory / ("frames-" + std::to_string(frames) + ".lpc");
    ASSERT_NO_FATAL_FAILURE(writeFile(path, makeLpcFile(frames)));
    for (const char *time : {"0", "0.005", "350.005", "10000000000"}) {
      MYFLT sample = 0;
      ASSERT_EQ(CSOUND_SUCCESS, runFileOpcode(
        "kRms, kOriginal, kError, kPitch lpread " + std::string(time) +
        ", \"" + path.generic_string() + "\"\naOut = kRms\nout aOut", &sample));
      double expected = (std::min)(double(frames), std::stod(time) * 100 + 1);
      EXPECT_NEAR(expected, sample, 0.01) << frames << " frames at " << time;
    }
  }
  /* A headerless single frame and a text file without a final newline. */
  auto raw = makeLpcFile(1);
  raw.erase(raw.begin(), raw.begin() + sizeof(LPHEADER) + 4);
  auto rawPath = directory / "headerless.lpc";
  ASSERT_NO_FATAL_FAILURE(writeFile(rawPath, raw));
  EXPECT_EQ(CSOUND_SUCCESS, runFileOpcode(
    "k1,k2,k3,k4 lpread 0, \"" + rawPath.generic_string() + "\", 2, 100"));
  auto textPath = directory / "text.lpc";
  std::string text = "LPANAL\n0 999 2 6\n100 44100 0.01\n1\n1\n0\n100\n0\n0";
  ASSERT_NO_FATAL_FAILURE(writeFile(
    textPath, std::vector<uint8_t>(text.begin(), text.end())));
  EXPECT_EQ(CSOUND_SUCCESS, runFileOpcode(
    "k1,k2,k3,k4 lpread 0, \"" + textPath.generic_string() + "\""));

  /* Publish an interpolated pole analysis for a downstream filter. */
  auto poles = makeLpcFile(1);
  LPHEADER header;
  std::memcpy(&header, poles.data(), sizeof(header));
  header.lpmagic = LP_MAGIC2;
  header.nvals = 8;
  std::memcpy(poles.data(), &header, sizeof(header));
  poles.resize(header.headersize);
  for (MYFLT value : {MYFLT(1), MYFLT(1), MYFLT(0), MYFLT(100),
                      MYFLT(.5), MYFLT(-.5), MYFLT(.5), MYFLT(.5)})
    appendNative(poles, value);
  auto polePath = directory / "poles.lpc";
  ASSERT_NO_FATAL_FAILURE(writeFile(polePath, poles));
  std::string reader = "k1,k2,k3,k4 lpread 0, \"" + polePath.generic_string() + "\"\n";
  EXPECT_EQ(CSOUND_SUCCESS, runFileOpcode(
    "lpslot 0\n" + reader + "lpslot 1\n" + reader +
    "lpslot 2\nlpinterp 0,1,0.5\naIn init 1\naOut lpreson aIn\nout aOut"));
}

TEST_F(AnalysisFileSafetyTests, LpreadRejectsIncompleteDataAndEmptySlots)
{
  auto data = makeLpcFile(1);
  for (size_t length : {size_t(4), data.size() - sizeof(MYFLT)}) {
    auto path = directory / ("short-" + std::to_string(length) + ".lpc");
    ASSERT_NO_FATAL_FAILURE(writeFile(
      path, std::vector<uint8_t>(data.begin(), data.begin() + length)));
    EXPECT_NE(CSOUND_SUCCESS, runFileOpcode(
      "k1,k2,k3,k4 lpread 0, \"" + path.generic_string() + "\""));
  }
  for (const char *consumer : {"aOut lpreson aIn",
                               "aOut lpfreson aIn, 1",
                               "kFreq, kBw lpformant 1",
                               "lpinterp 0, 1, 0.5"})
    EXPECT_NE(CSOUND_SUCCESS, runFileOpcode(
      std::string("lpslot 20\naIn init 0\n") + consumer)) << consumer;
}

std::vector<uint8_t> makeHetroFile()
{
  std::vector<uint8_t> data;
  const std::array<int16_t, 9> values = {
    1, -1, 0, 0, 32767, -2, 0, 440, 32767
  };
  for (int16_t value : values)
    appendNative(data, value);
  return data;
}

std::vector<uint8_t> makePvxFile()
{
  std::vector<uint8_t> data;
  appendTag(data, "RIFF");
  appendU32(data, 180);
  appendTag(data, "WAVE");
  appendTag(data, "fmt ");
  appendU32(data, 80);
  appendU16(data, 0xFFFE);
  appendU16(data, 1);
  appendU32(data, 44100);
  appendU32(data, 88200);
  appendU16(data, 2);
  appendU16(data, 16);
  appendU16(data, 62);
  appendU16(data, 16);
  appendU32(data, 0);
  appendU32(data, 0x8312B9C2);
  appendU16(data, 0x2E6E);
  appendU16(data, 0x11D4);
  const std::array<uint8_t, 8> guid_tail =
    {0xA8, 0x24, 0xDE, 0x5B, 0x96, 0xC3, 0xAB, 0x21};
  data.insert(data.end(), guid_tail.begin(), guid_tail.end());
  appendU32(data, 1);
  appendU32(data, 32);
  appendU16(data, 0);
  appendU16(data, 0);
  appendU16(data, 3);
  appendU16(data, 0);
  appendU32(data, 2);
  appendU32(data, 2);
  appendU32(data, 1);
  appendU32(data, 16);
  appendFloat(data, 44100.0F);
  appendFloat(data, 0.0F);
  appendTag(data, "data");
  appendU32(data, 80);
  data.resize(data.size() + 80, 0);
  return data;
}

TEST_F(AnalysisFileSafetyTests, AtsMutationsFailCleanly)
{
  const std::vector<uint8_t> valid = makeAtsFile();
  const std::array<size_t, 3> lengths = {1, 79, valid.size() - 1};
  for (size_t length : lengths) {
    std::vector<uint8_t> truncated(valid.begin(), valid.begin() + length);
    auto path =
      directory / ("truncated-" + std::to_string(length) + ".ats");
    ASSERT_NO_FATAL_FAILURE(writeFile(path, truncated));
    std::string statement =
      "iValue ATSinfo \"" + path.generic_string() + "\", 0";
    EXPECT_NE(CSOUND_SUCCESS, runFileOpcode(statement));
  }

  std::vector<uint8_t> corrupt = valid;
  double count = static_cast<double>(INT32_MAX);
  std::memcpy(corrupt.data() + 5 * sizeof(double), &count, sizeof(count));
  auto path = directory / "corrupt-count.ats";
  ASSERT_NO_FATAL_FAILURE(writeFile(path, corrupt));
  std::string statement =
    "iValue ATSinfo \"" + path.generic_string() + "\", 0";
  EXPECT_NE(CSOUND_SUCCESS, runFileOpcode(statement));
}

TEST_F(AnalysisFileSafetyTests, HetroMutationsFailCleanly)
{
  const std::vector<uint8_t> valid = makeHetroFile();
  const std::array<size_t, 4> lengths = {1, 2, 4, valid.size() - 1};
  for (size_t length : lengths) {
    std::vector<uint8_t> truncated(valid.begin(), valid.begin() + length);
    auto path =
      directory / ("truncated-" + std::to_string(length) + ".het");
    ASSERT_NO_FATAL_FAILURE(writeFile(path, truncated));
    std::string statement =
      "aSignal adsyn 1, 1, 1, \"" + path.generic_string() + "\"";
    EXPECT_NE(CSOUND_SUCCESS, runFileOpcode(statement));
  }

  std::vector<uint8_t> corrupt = valid;
  int16_t count = INT16_MAX;
  std::memcpy(corrupt.data(), &count, sizeof(count));
  auto path = directory / "corrupt-count.het";
  ASSERT_NO_FATAL_FAILURE(writeFile(path, corrupt));
  std::string statement =
    "aSignal adsyn 1, 1, 1, \"" + path.generic_string() + "\"";
  EXPECT_NE(CSOUND_SUCCESS, runFileOpcode(statement));
}

TEST_F(AnalysisFileSafetyTests, PvxMutationsFailCleanly)
{
  const std::vector<uint8_t> valid = makePvxFile();
  auto valid_path = directory / "valid.pvx";
  ASSERT_NO_FATAL_FAILURE(writeFile(valid_path, valid));
  ASSERT_EQ(0, loadPvx(valid_path));

  std::vector<uint8_t> trailing = valid;
  trailing.push_back(0xAA);
  trailing.push_back(0x55);
  auto trailing_path = directory / "valid-trailing.pvx";
  ASSERT_NO_FATAL_FAILURE(writeFile(trailing_path, trailing));
  ASSERT_EQ(0, loadPvx(trailing_path));

  const std::array<size_t, 4> lengths = {1, 12, 99, valid.size() - 1};
  for (size_t length : lengths) {
    std::vector<uint8_t> truncated(valid.begin(), valid.begin() + length);
    auto path =
      directory / ("truncated-" + std::to_string(length) + ".pvx");
    ASSERT_NO_FATAL_FAILURE(writeFile(path, truncated));
    EXPECT_NE(0, loadPvx(path));
  }

  std::vector<uint8_t> corrupt_riff = valid;
  putU32(corrupt_riff, 4, UINT32_MAX);
  auto corrupt_riff_path = directory / "corrupt-riff-size.pvx";
  ASSERT_NO_FATAL_FAILURE(writeFile(corrupt_riff_path, corrupt_riff));
  EXPECT_NE(0, loadPvx(corrupt_riff_path));

  std::vector<uint8_t> corrupt_bins = valid;
  putU32(corrupt_bins, 76, 0x7FFFFFFF);
  auto corrupt_bins_path = directory / "corrupt-bin-count.pvx";
  ASSERT_NO_FATAL_FAILURE(writeFile(corrupt_bins_path, corrupt_bins));
  EXPECT_NE(0, loadPvx(corrupt_bins_path));

  std::vector<uint8_t> corrupt_data = valid;
  putU32(corrupt_data, 104, UINT32_MAX);
  auto corrupt_data_path = directory / "corrupt-data-size.pvx";
  ASSERT_NO_FATAL_FAILURE(writeFile(corrupt_data_path, corrupt_data));
  EXPECT_NE(0, loadPvx(corrupt_data_path));
}

} // namespace
