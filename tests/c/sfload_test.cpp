#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>

namespace {
class SfloadTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    std::filesystem::path directory, source;
    decltype(CSOUND::FileOpen) openFile;
    decltype(CSOUND::FileClose) closeFile;
    std::unordered_set<void *> files;

    static void *trackedOpen(CSOUND *cs, void *file, int32_t type,
                            const char *name, void *mode, const char *env,
                            int32_t format, int32_t temporary)
    {
        auto &self = *static_cast<SfloadTests *>(csoundGetHostData(cs));
        void *fd = self.openFile(cs, file, type, name, mode, env, format, temporary);
        if (fd && format == CSFTYPE_SOUNDFONT) self.files.insert(fd);
        return fd;
    }
    static int32_t trackedClose(CSOUND *cs, void *fd, uint32_t flags)
    {
        auto &self = *static_cast<SfloadTests *>(csoundGetHostData(cs));
        self.files.erase(fd);
        return self.closeFile(cs, fd, flags);
    }
    void SetUp() override
    {
        csound = csoundCreate(this, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n -d -m0");
        openFile = csound->FileOpen;
        closeFile = csound->FileClose;
        csound->FileOpen = trackedOpen;
        csound->FileClose = trackedClose;
        directory = std::filesystem::temp_directory_path() /
            ("csound-sfload-" + std::to_string(reinterpret_cast<uintptr_t>(csound)));
        ASSERT_TRUE(std::filesystem::create_directory(directory));
        source = std::filesystem::path(__FILE__).parent_path() /
                 "../../samples/sf_GMbank.sf2";
    }
    void TearDown() override
    {
        csoundDestroy(csound);
        std::filesystem::remove_all(directory);
    }
    std::string load(const std::string &variable, const std::filesystem::path &path)
    {
        return variable + " sfload \"" + path.generic_string() + "\"\n";
    }
    void start(const std::string &header, const std::string &instruments,
               const std::string &score)
    {
        std::string csd = "<CsoundSynthesizer>\n<CsInstruments>\n"
            "sr=1024\nksmps=8\nnchnls=1\n" + header + instruments +
            "</CsInstruments>\n<CsScore>\n" + score +
            "</CsScore>\n</CsoundSynthesizer>";
        ASSERT_EQ(0, csoundCompileCSD(csound, csd.c_str(), 1, 0));
        ASSERT_EQ(0, csoundStart(csound));
    }
};

TEST_F(SfloadTests, FailedLoadsDoNotConsumeBankHandles)
{
    std::ofstream(directory / "empty.sf2", std::ios::binary).close();
    {
        std::ifstream input(source, std::ios::binary);
        ASSERT_TRUE(input.good());
        char header[12];
        input.read(header, sizeof(header));
        std::ofstream output(directory / "truncated.sf2", std::ios::binary);
        output.write(header, sizeof(header));
    }
    {
        const char incomplete[] = "RIFF\x04\x00\x00\x00sfbk";
        std::ofstream output(directory / "incomplete.sf2", std::ios::binary);
        output.write(incomplete, sizeof(incomplete)-1);
    }
    {
        std::ifstream input(source, std::ios::binary);
        std::string data((std::istreambuf_iterator<char>(input)), {});
        // The sample-header table follows the sample data in this fixture.
        const auto shdr = data.rfind("shdr");
        ASSERT_NE(std::string::npos, shdr);
        ASSERT_LT(shdr + 8 + 45, data.size());
        data[shdr + 8 + 45] |= char(0x80); // first sample uses unsupported ROM
        std::ofstream output(directory / "rom.sf2", std::ios::binary);
        output.write(data.data(), data.size());
    }
    start("", "instr 1\n" + load("iBank", directory / "missing.sf2") +
          "endin\ninstr 2\n" + load("iBank", directory / "empty.sf2") +
          "endin\ninstr 3\n" + load("iBank", directory / "truncated.sf2") +
          "endin\ninstr 4\n" + load("iBank", directory / "incomplete.sf2") +
          "endin\ninstr 5\n" + load("iBank", directory / "rom.sf2") +
          "endin\ninstr 6\n" + load("iBank", source) +
          "chnset iBank+1, \"loaded\"\nendin\n",
          "i 1 0 .01\ni 2 0 .01\ni 3 0 .01\ni 4 0 .01\ni 5 0 .01\ni 6 .02 .01\n");
    while (csoundPerformKsmps(csound) == 0) {}
    int error = 0;
    EXPECT_EQ(1, csoundGetControlChannel(csound, "loaded", &error));
    EXPECT_EQ(0, error);
    EXPECT_EQ(5, csound->perferrcnt);
    EXPECT_TRUE(files.empty());
}

TEST_F(SfloadTests, DuplicateLoadsReuseHandlesAcrossBankGrowth)
{
    std::string header;
    for (int i = 0; i < 11; ++i) {
        const auto path = directory / ("bank" + std::to_string(i) + ".sf2");
        std::filesystem::copy_file(source, path);
        header += load("iBank", path);
        header += "if iBank != " + std::to_string(i) +
                  " then\nexitnow -1\nendif\n";
    }
    const std::string option = "--env:SFDIR=" + directory.generic_string();
    ASSERT_EQ(0, csoundSetOption(csound, option.c_str()));
    header += load("iDuplicate", "bank0.sf2");
    header += "chnset iDuplicate+1, \"duplicate\"\n";
    start(header, "instr 1\nendin\n", "i 1 0 .01\n");
    int error = 0;
    EXPECT_EQ(1, csoundGetControlChannel(csound, "duplicate", &error));
    EXPECT_EQ(0, error);
    EXPECT_TRUE(files.empty());
}
}
