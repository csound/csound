#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <algorithm>

#define oentries doppler_test_entries
#define csoundModuleInit_doppler testModuleInit_doppler
#include "../../Opcodes/doppler.cpp"
#undef csoundModuleInit_doppler
#undef oentries

namespace {
int initialized, destroyed, cleaned;
int32_t ignoreInitError(CSOUND *, const char *, ...) { return NOTOK; }

class TrackedInterpolator : public LinearInterpolator {
public:
    ~TrackedInterpolator() override { ++destroyed; }
};

int32_t observeInit(CSOUND *csound, void *data)
{
    int32_t result = doppler_test_entries[0].init(csound, data);
    if (result == OK) {
        auto *opcode = static_cast<Doppler *>(data);
        delete opcode->audioInterpolator;
        opcode->audioInterpolator = new TrackedInterpolator;
        ++initialized;
    }
    return result;
}

int32_t observeCleanup(CSOUND *csound, void *data)
{
    int32_t result = doppler_test_entries[0].deinit(csound, data);
    auto *opcode = static_cast<Doppler *>(data);
    EXPECT_EQ(opcode->audioBufferQueue, nullptr);
    EXPECT_EQ(opcode->sourcePositionQueue, nullptr);
    EXPECT_EQ(opcode->audioInterpolator, nullptr);
    EXPECT_EQ(opcode->smoothingFilter, nullptr);
    ++cleaned;
    return result;
}

class DopplerTests : public ::testing::Test {
protected:
    CSOUND *csound;
    void SetUp() override {
        initialized = destroyed = cleaned = 0;
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "-m0");
        const auto &entry = doppler_test_entries[0];
        ASSERT_EQ(csoundAppendOpcode(csound, "test_doppler", entry.dsblksiz,
            entry.flags, entry.outypes, entry.intypes, observeInit, entry.perf,
            entry.deinit ? observeCleanup : nullptr), OK);
    }
    void TearDown() override { csoundDestroy(csound); }
};

TEST_F(DopplerTests, ReinitializationAndNoteEndReleaseState)
{
    ASSERT_EQ(csoundCompileCSD(csound,
        "<CsoundSynthesizer>\n<CsInstruments>\n"
        "sr = 1024\nksmps = 8\nnchnls = 1\n0dbfs = 1\n"
        "instr 1\n"
        "kcycle timeinstk\n"
        "if kcycle == 3 then\n reinit AGAIN\n endif\n"
        "AGAIN:\n"
        "asource init .25\n"
        "aout test_doppler asource, 20, 0\n"
        "rireturn\n"
        "out aout\nendin\n</CsInstruments>\n<CsScore>\n"
        "i 1 0 .125\ni 1 .25 .125\ne\n"
        "</CsScore>\n</CsoundSynthesizer>\n", 1, 0), OK);
    ASSERT_EQ(csoundStart(csound), OK);
    while (csoundPerformKsmps(csound) == 0) {}
    EXPECT_EQ(initialized, 4);
    EXPECT_EQ(cleaned, 2);
    EXPECT_EQ(destroyed, initialized);
}

TEST_F(DopplerTests, ResetReleasesAnActiveNote)
{
    ASSERT_EQ(csoundCompileCSD(csound,
        "<CsoundSynthesizer>\n<CsInstruments>\n"
        "sr = 1024\nksmps = 8\nnchnls = 1\n"
        "instr 1\nasource init .25\n"
        "aout test_doppler asource, 20, 0\nendin\n"
        "</CsInstruments>\n<CsScore>\ni 1 0 10\ne\n"
        "</CsScore>\n</CsoundSynthesizer>\n", 1, 0), OK);
    ASSERT_EQ(csoundStart(csound), OK);
    for (int i = 0; i < 8; ++i) ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(initialized, 1);
    EXPECT_EQ(cleaned, 0);
    csoundReset(csound);
    EXPECT_EQ(cleaned, 1);
    EXPECT_EQ(destroyed, initialized);
}

TEST_F(DopplerTests, PartialBlocksPreserveDelayAndClearInactiveSamples)
{
    for (int delay : {0, 4, 13}) {
        Doppler opcode = {};
        INSDS instrument = {};
        instrument.esr = 1024;
        instrument.ekr = 128;
        instrument.ksmps = 8;
        opcode.opds.insdshead = &instrument;
        cs_float input[8], output[8];
        cs_float source = delay, mic = 0, speed = 1024, cutoff = 6;
        opcode.audioInput = input;
        opcode.audioOutput = output;
        opcode.kSourcePosition = &source;
        opcode.kMicPosition = &mic;
        opcode.jSpeedOfSound = &speed;
        opcode.jUpdateFilterCutoff = &cutoff;
        ASSERT_EQ(opcode.init(csound), OK);
        int sample = 0;
        const struct { uint32_t start, end; } blocks[] = {
            {3, 8}, {0, 8}, {0, 8}, {0, 8}, {0, 5}, {0, 0}
        };
        for (const auto &block : blocks) {
            instrument.ksmps_offset = block.start;
            instrument.ksmps_no_end = 8 - block.end;
            std::fill(input, input + 8, cs_float(-999));
            std::fill(output, output + 8, cs_float(-999));
            for (uint32_t i = block.start; i < block.end; ++i)
                input[i] = sample + i - block.start + 1;
            ASSERT_EQ(opcode.kontrol(csound), OK);
            for (uint32_t i = 0; i < 8; ++i) {
                if (i >= block.start && i < block.end) {
                    // The existing interpolator adds one sample of latency.
                    EXPECT_EQ(output[i], sample > delay ? sample - delay : 0);
                    ++sample;
                } else {
                    EXPECT_EQ(output[i], 0);
                }
            }
        }
        EXPECT_EQ(opcode.noteoff(csound), OK);
        EXPECT_EQ(opcode.noteoff(csound), OK);
    }
}

TEST_F(DopplerTests, RejectInvalidPropagationParameters)
{
    csound->InitError = ignoreInitError;
    Doppler opcode = {};
    INSDS instrument = {};
    instrument.esr = 1024;
    instrument.ekr = 128;
    instrument.ksmps = 8;
    opcode.opds.insdshead = &instrument;
    cs_float speed = 0, cutoff = 6;
    opcode.jSpeedOfSound = &speed;
    opcode.jUpdateFilterCutoff = &cutoff;
    EXPECT_EQ(opcode.init(csound), NOTOK);
    speed = -2;
    EXPECT_EQ(opcode.init(csound), NOTOK);
    speed = 340;
    cutoff = -2;
    EXPECT_EQ(opcode.init(csound), NOTOK);
    EXPECT_EQ(opcode.noteoff(csound), OK);
}
} // namespace
