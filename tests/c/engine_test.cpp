#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csound_graph_display.h"
#include "fftlib.h"
extern "C" {
#include "aops.h"
#include "complex_ops.h"
}
#include <algorithm>
#include <atomic>
#include <clocale>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdio.h>
#include <string>
#include <thread>
#include <vector>
#include "gtest/gtest.h"
#include "time.h"

namespace {

std::vector<MYFLT> directComplexDft(const std::vector<MYFLT>& input,
                                    bool inverse)
{
    const int32_t size = static_cast<int32_t>(input.size() / 2);
    const double direction = inverse ? 1.0 : -1.0;
    const double scale = inverse ? 1.0 / size : 1.0;
    const double twoPi = 2.0 * std::acos(-1.0);
    std::vector<MYFLT> output(input.size(), FL(0.0));

    for (int32_t bin = 0; bin < size; ++bin) {
        double real = 0.0;
        double imag = 0.0;
        for (int32_t sample = 0; sample < size; ++sample) {
            const double angle = direction * twoPi *
              static_cast<double>(bin) * sample / size;
            const double cosine = std::cos(angle);
            const double sine = std::sin(angle);
            const double inputReal = input[sample * 2];
            const double inputImag = input[sample * 2 + 1];
            real += inputReal * cosine - inputImag * sine;
            imag += inputReal * sine + inputImag * cosine;
        }
        output[bin * 2] = static_cast<MYFLT>(real * scale);
        output[bin * 2 + 1] = static_cast<MYFLT>(imag * scale);
    }
    return output;
}

std::string drainMessageBuffer(CSOUND *csound)
{
    std::string messages;
    while (csoundGetMessageCnt(csound) > 0) {
        const char *message = csoundGetFirstMessage(csound);
        if (message != nullptr)
            messages += message;
        csoundPopFirstMessage(csound);
    }
    return messages;
}

class NumericLocaleGuard {
public:
    NumericLocaleGuard()
    {
        const char *locale = std::setlocale(LC_NUMERIC, nullptr);
        if (locale != nullptr)
            savedLocale = locale;
    }

    ~NumericLocaleGuard()
    {
        if (!savedLocale.empty())
            std::setlocale(LC_NUMERIC, savedLocale.c_str());
    }

private:
    std::string savedLocale;
};

bool setCommaDecimalLocale()
{
    static const char *const candidates[] = {
      "de_DE.UTF-8", "de_DE.utf8", "de_DE",
      "fr_FR.UTF-8", "fr_FR.utf8", "fr_FR",
      "German_Germany.1252", "German_Germany.65001"
    };

    for (const char *candidate : candidates) {
        if (std::setlocale(LC_NUMERIC, candidate) != nullptr &&
            std::strcmp(std::localeconv()->decimal_point, ",") == 0)
            return true;
    }
    return false;
}

}

class EngineTests : public ::testing::Test {
public:
    EngineTests ()
    {
    }

    virtual ~EngineTests ()
    {
    }

    virtual void SetUp ()
    {
      csound = csoundCreate (NULL,NULL);
      csoundCreateMessageBuffer (csound, 0);
      csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

TEST_F (EngineTests, testComplexFftMatchesDirectDft)
{
    const std::vector<std::vector<MYFLT>> inputs = {
      {FL(1.0), FL(0.5), FL(-2.0), FL(1.25), FL(0.75), FL(-0.5),
       FL(3.0), FL(-1.5), FL(-0.25), FL(2.0), FL(1.5), FL(-0.75)},
      {FL(1.0), FL(0.5), FL(-2.0), FL(1.25), FL(0.75), FL(-0.5),
       FL(3.0), FL(-1.5), FL(-0.25), FL(2.0), FL(1.5), FL(-0.75),
       FL(0.5), FL(1.75), FL(-1.0), FL(-0.25)}
    };
    const double tolerance =
      sizeof(MYFLT) == sizeof(float) ? 0.0001 : 1.0e-10;

    for (const auto& input : inputs) {
        const int32_t size = static_cast<int32_t>(input.size() / 2);
        const auto expectedForward = directComplexDft(input, false);
        auto actualForward = input;
        if (size == 6)
            csoundComplexFFTnp2(csound, actualForward.data(), size);
        else
            csound->ComplexFFT(csound, actualForward.data(), size);

        for (size_t i = 0; i < actualForward.size(); ++i)
            EXPECT_NEAR(actualForward[i], expectedForward[i], tolerance)
              << "FFT size " << size << ", component " << i;

        const auto expectedInverse = directComplexDft(actualForward, true);
        auto actualInverse = actualForward;
        if (size == 6)
            csoundInverseComplexFFTnp2(csound, actualInverse.data(), size);
        else
            csound->InverseComplexFFT(csound, actualInverse.data(), size);

        for (size_t i = 0; i < actualInverse.size(); ++i) {
            EXPECT_NEAR(actualInverse[i], expectedInverse[i], tolerance)
              << "inverse FFT size " << size << ", component " << i;
            EXPECT_NEAR(actualInverse[i], input[i], tolerance)
              << "round trip size " << size << ", component " << i;
        }
    }
}

TEST_F (EngineTests, testSscanfUsesCLocale)
{
    NumericLocaleGuard localeGuard;
    if (!setCommaDecimalLocale())
        GTEST_SKIP() << "No comma-decimal locale is installed";

    char input[] = "3.5";
    double value = 0.0;

    EXPECT_EQ(csound->Sscanf(input, "%lf", &value), 1);
    EXPECT_DOUBLE_EQ(value, 3.5);
}

TEST_F (EngineTests, testComplexFftReportsUnsupportedSizes)
{
    MYFLT buffer[] = {
      FL(1.0), FL(1.0), FL(2.0), FL(-1.0), FL(0.5), FL(2.0)
    };
    drainMessageBuffer(csound);

    csoundComplexFFTnp2(csound, buffer, 3);
    csoundInverseComplexFFTnp2(csound, buffer, 3);

    const std::string messages = drainMessageBuffer(csound);
    EXPECT_NE(messages.find("csoundComplexFFTnp2(): invalid FFT size, 3"),
              std::string::npos);
    EXPECT_NE(messages.find("csoundInverseComplexFFTnp2(): invalid FFT size, 3"),
              std::string::npos);
}

TEST_F (EngineTests, testTypedComplexFftRejectsOddSize)
{
    csoundSetOption(csound, "-n");
    ASSERT_EQ(csoundCompileOrc(csound, R"(
      instr 1
        kInput:Complex[] = [complex(1, 1), complex(2, -1), complex(0.5, 2)]
        kSpectrum:Complex[] = fft(kInput)
        turnoff
      endin
    )", 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 0.01", 0);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    EXPECT_NE(csoundPerformKsmps(csound), CSOUND_SUCCESS);

    const std::string messages = drainMessageBuffer(csound);
    EXPECT_NE(messages.find(
                "fft: input array size (3) must be 1 or even"),
              std::string::npos);
}

static int32_t deinitCallCount = 0;

static int32_t failingDeinit(CSOUND *, void *)
{
    deinitCallCount++;
    return NOTOK;
}

static int32_t succeedingDeinit(CSOUND *, void *)
{
    deinitCallCount++;
    return OK;
}

TEST_F (EngineTests, testDeinitContinuesAfterError)
{
    INSDS owner {};
    OPDS first {};
    OPDS second {};
    OPTXT firstText {};
    OPTXT secondText {};
    OENTRY firstEntry {};
    OENTRY secondEntry {};

    firstEntry.opname = const_cast<char *>("failing-deinit");
    secondEntry.opname = const_cast<char *>("succeeding-deinit");
    firstText.t.oentry = &firstEntry;
    secondText.t.oentry = &secondEntry;
    first.deinit = failingDeinit;
    first.optext = &firstText;
    first.nxtd = &second;
    second.deinit = succeedingDeinit;
    second.optext = &secondText;
    owner.nxtd = &first;
    deinitCallCount = 0;

    deinit_pass(csound, &owner);

    ASSERT_EQ(deinitCallCount, 2);
}

TEST_F (EngineTests, testNestedInitKeepsTurnoffPending)
{
    INSDS instance {};

    // This local instance is never shared, so direct field access is safe and
    // avoids MSVC's long-only Interlocked overloads in this C++ test.
    EXPECT_EQ(instance_init_begin(csound, &instance), CSOUND_SUCCESS);
    EXPECT_EQ(instance_init_begin(csound, &instance), CSOUND_SUCCESS);
    EXPECT_EQ(instance.init_running, 2);
    EXPECT_EQ(instance_init_finish(csound, &instance),
              INSTANCE_INIT_DEFERRED);

    instance.init_done = 1;
    instance_init_request_turnoff(csound, &instance);
    EXPECT_EQ(instance.init_done, 0);
    EXPECT_EQ(instance.turnoff_pending,
              INSTANCE_TURNOFF_REQUESTED);
    EXPECT_EQ(csound->init_turnoff_pending, nullptr);
    EXPECT_EQ(instance_init_begin(csound, &instance), CSOUND_SUCCESS);
    EXPECT_EQ(instance.init_running, 2);
    EXPECT_EQ(instance.turnoff_pending,
              INSTANCE_TURNOFF_REQUESTED);
    EXPECT_EQ(instance_init_finish(csound, &instance),
              INSTANCE_INIT_DEFERRED);
    EXPECT_EQ(instance_init_finish(csound, &instance),
              INSTANCE_INIT_TURNOFF);
    EXPECT_EQ(instance.init_running, 0);
    EXPECT_EQ(instance.turnoff_pending,
              INSTANCE_TURNOFF_FINALIZING);
    EXPECT_EQ(csound->init_turnoff_pending, &instance);
    instance_init_request_turnoff(csound, &instance);
    EXPECT_EQ(csound->init_turnoff_pending, &instance);
    EXPECT_EQ(instance.init_turnoff_next, nullptr);
    EXPECT_EQ(instance_init_begin(csound, &instance), CSOUND_ERROR);
    EXPECT_EQ(instance.init_running, 0);
    EXPECT_EQ(instance_init_finish(csound, &instance),
              INSTANCE_INIT_DEFERRED);
    EXPECT_EQ(instance.turnoff_pending,
              INSTANCE_TURNOFF_FINALIZING);
    EXPECT_EQ(csound->init_turnoff_pending, &instance);

    instance.turnoff_pending = INSTANCE_TURNOFF_RECLAIM;
    EXPECT_EQ(instance_init_begin(csound, &instance), CSOUND_ERROR);
    EXPECT_EQ(instance.init_running, 0);

    instance.turnoff_pending = INSTANCE_TURNOFF_REQUESTED;
    EXPECT_EQ(instance_init_begin(csound, &instance), CSOUND_ERROR);
    EXPECT_EQ(instance.init_running, 0);

    csound->init_turnoff_pending = nullptr;
    instance.init_turnoff_next = nullptr;
    instance.turnoff_pending = INSTANCE_TURNOFF_NONE;
}

TEST_F (EngineTests, testRecycleDetachedInactiveInstance)
{
    INSTRTXT instrument {};
    INSDS available {};
    INSDS detached {};

    available.instr = &instrument;
    detached.instr = &instrument;
    instrument.act_instance = &available;

    recycle_inactive_instance(csound, &detached);

    EXPECT_EQ(instrument.act_instance, &detached);
    EXPECT_EQ(detached.nxtact, &available);
}

TEST_F (EngineTests, testRealtimeLongJmpPreservesInitThreadContext)
{
    INSDS instance {};
    OPDS opcode {};
    int32_t initialPerfErrors = csound->perferrcnt;

    // A non-null handle means the event thread may own this init context.
    csound->event_insert_thread = &opcode;
    csound->curip = &instance;
    csound->ids = &opcode;
    csound->reinitflag = 1;
    csound->tieflag = 1;
    csound->inerrcnt = 2;

    int32_t jumpResult = setjmp(csound->exitjmp);
    if (jumpResult == 0)
      csoundLongJmp(csound, 1);

    EXPECT_NE(jumpResult, 0);
    EXPECT_EQ(csound->curip, &instance);
    EXPECT_EQ(csound->ids, &opcode);
    EXPECT_EQ(csound->reinitflag, 1);
    EXPECT_EQ(csound->tieflag, 1);
    EXPECT_EQ(csound->inerrcnt, 2);
    EXPECT_EQ(csound->perferrcnt, initialPerfErrors);

    // Do not leave a fake thread handle or stack pointers for TearDown().
    csound->event_insert_thread = nullptr;
    csound->curip = nullptr;
    csound->ids = nullptr;
    csound->reinitflag = 0;
    csound->tieflag = 0;
    csound->inerrcnt = 0;
    csound->engineStatus &= ~CS_STATE_JMP;
}
TEST_F (EngineTests, testRealComplexSubtraction)
{
    struct SubtractionCase {
        COMPLEXDAT input;
        bool scalarFirst;
        MYFLT expectedReal;
        MYFLT expectedImag;
    };

    const MYFLT scalar = FL(10.0);
    const MYFLT angle = std::atan2(FL(4.0), FL(3.0));
    const SubtractionCase cases[] = {
        {{FL(3.0), FL(4.0), 0}, true,  FL(7.0),  FL(-4.0)},
        {{FL(3.0), FL(4.0), 0}, false, FL(-7.0), FL(4.0)},
        {{FL(5.0), angle, 1},    true,  FL(7.0),  FL(-4.0)},
        {{FL(5.0), angle, 1},    false, FL(-7.0), FL(4.0)},
    };
    const MYFLT tolerance = std::numeric_limits<MYFLT>::epsilon() * FL(256.0);

    for (const auto& testCase : cases) {
        COMPLEXDAT input = testCase.input;
        COMPLEXDAT result = {};
        MYFLT scalarArg = scalar;
        AOP opcode = {};
        opcode.r = reinterpret_cast<MYFLT *>(&result);

        if (testCase.scalarFirst) {
            opcode.a = &scalarArg;
            opcode.b = reinterpret_cast<MYFLT *>(&input);
            ASSERT_EQ(real_sub_complex(csound, &opcode), OK);
        }
        else {
            opcode.a = reinterpret_cast<MYFLT *>(&input);
            opcode.b = &scalarArg;
            ASSERT_EQ(complex_sub_real(csound, &opcode), OK);
        }

        EXPECT_EQ(result.isPolar, input.isPolar);
        const MYFLT resultReal = result.isPolar
          ? result.real * std::cos(result.imag) : result.real;
        const MYFLT resultImag = result.isPolar
          ? result.real * std::sin(result.imag) : result.imag;
        EXPECT_NEAR(resultReal, testCase.expectedReal, tolerance);
        EXPECT_NEAR(resultImag, testCase.expectedImag, tolerance);
    }
}

struct InvalidRealFFTSizeCase {
    const char *name;
    const char *statement;
    const char *error;
};

class InvalidRealFFTSizeTests
    : public ::testing::TestWithParam<InvalidRealFFTSizeCase> {
protected:
    void SetUp() override
    {
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    }

    void TearDown() override
    {
        csoundDestroy(csound);
    }

    CSOUND *csound {nullptr};
};

static int32_t realFFTSetupCalls;
static void *(*realFFTSetup)(CSOUND *, int32_t, int32_t);

static void *countRealFFTSetup(CSOUND *csound, int32_t size, int32_t direction)
{
    ++realFFTSetupCalls;
    return realFFTSetup(csound, size, direction);
}

TEST_P(InvalidRealFFTSizeTests, RejectsBeforeBackendSetup)
{
    const InvalidRealFFTSizeCase &test = GetParam();
    std::string orchestra = "instr 1\n";
    orchestra += test.statement;
    orchestra += "\nendin\n";
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 0", 0);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);

    realFFTSetupCalls = 0;
    realFFTSetup = csound->RealFFTSetup;
    csound->RealFFTSetup = countRealFFTSetup;
    EXPECT_NE(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_EQ(realFFTSetupCalls, 0);

    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        const char *message = csoundGetFirstMessage(csound);
        if (message != nullptr)
            messages += message;
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(messages.find(test.error), std::string::npos)
        << "Expected error text not found in:\n" << messages;
}

INSTANTIATE_TEST_SUITE_P(
    ArrayOpcodes, InvalidRealFFTSizeTests,
    ::testing::Values(
        InvalidRealFFTSizeCase{
            "TypedForwardEmpty",
            "kInput[] init 0\nkSpectrum:Complex[] rfft kInput",
            "rfft: transform size must be even and at least 2"},
        InvalidRealFFTSizeCase{
            "TypedForwardOne",
            "kInput[] init 1\nkSpectrum:Complex[] rfft kInput",
            "rfft: transform size must be even and at least 2"},
        InvalidRealFFTSizeCase{
            "TypedForwardOdd",
            "kInput[] init 5\nkSpectrum:Complex[] rfft kInput",
            "rfft: transform size must be even and at least 2"},
        InvalidRealFFTSizeCase{
            "TypedInverseEmpty",
            "kSpectrum:Complex[] init 0\nkOutput[] rifft kSpectrum",
            "rifft: input spectrum must contain at least 2 bins"},
        InvalidRealFFTSizeCase{
            "TypedInverseOne",
            "kSpectrum:Complex[] init 1\nkOutput[] rifft kSpectrum",
            "rifft: input spectrum must contain at least 2 bins"},
        InvalidRealFFTSizeCase{
            "PackedForwardEmpty",
            "kInput[] init 0\nkOutput[] rfft kInput",
            "rfft: transform size must be even and at least 2"},
        InvalidRealFFTSizeCase{
            "PackedForwardOne",
            "kInput[] init 1\nkOutput[] rfft kInput",
            "rfft: transform size must be even and at least 2"},
        InvalidRealFFTSizeCase{
            "PackedForwardOdd",
            "kInput[] init 5\nkOutput[] rfft kInput",
            "rfft: transform size must be even and at least 2"},
        InvalidRealFFTSizeCase{
            "PackedInitForwardOdd",
            "iInput[] init 5\niOutput[] rfft iInput",
            "rfft: transform size must be even and at least 2"},
        InvalidRealFFTSizeCase{
            "PackedInverseEmpty",
            "kInput[] init 0\nkOutput[] rifft kInput",
            "rifft: transform size must be even and at least 2"},
        InvalidRealFFTSizeCase{
            "PackedInverseOne",
            "kInput[] init 1\nkOutput[] rifft kInput",
            "rifft: transform size must be even and at least 2"},
        InvalidRealFFTSizeCase{
            "PackedInverseOdd",
            "kInput[] init 5\nkOutput[] rifft kInput",
            "rifft: transform size must be even and at least 2"},
        InvalidRealFFTSizeCase{
            "PackedInitInverseOdd",
            "iInput[] init 5\niOutput[] rifft iInput",
            "rifft: transform size must be even and at least 2"}),
    [](const ::testing::TestParamInfo<InvalidRealFFTSizeCase> &info) {
        return info.param.name;
    });

TEST_F (EngineTests, testUdpServer)
{
    csoundSetIsGraphable(csound, 1);
    csoundSetOption(csound,"--port=12345");
    csoundStart(csound);
    csoundSleep(1000);
}

TEST_F (EngineTests, testScoreOffset)
{
    csoundSetOption(csound,"-n");
    csoundEventString(csound, R"(
    i 1 0 1
    )", 0);
    csoundCompileOrc(csound, R"(
      instr 1
      endin
    )", 0);
                      
    csoundStart(csound);
    csoundSetScoreOffsetSeconds(csound, 1);
    ASSERT_TRUE(csoundPerformKsmps(csound) != 0); 
}

TEST_F (EngineTests, testScoreRewind)
{
    csoundSetOption(csound,"-n");
    csoundEventString(csound, R"(
    i 1 0 1
    )", 0);
    csoundCompileOrc(csound, R"(
      instr 1
      prints "Top...\n";
      endin
    )", 0);
                      
    csoundStart(csound);
    while(csoundPerformKsmps(csound) == 0)
      ;
    csoundRewindScore(csound);
    ASSERT_TRUE(csoundPerformKsmps(csound) == 0); 
}

TEST_F (EngineTests, testCommandLineArgumentsApi)
{
    char firstArgument[] = "concert.orc";
    const char *arguments[] = {
      firstArgument,
      "first violin",
      "--quiet",
      ""
    };

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 4, arguments), CSOUND_SUCCESS);
    firstArgument[0] = 'X';

    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 4);
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 0), "concert.orc");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 1), "first violin");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 2), "--quiet");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 3), "");
    ASSERT_EQ(csoundGetCommandLineArg(csound, -1), nullptr);
    ASSERT_EQ(csoundGetCommandLineArg(csound, 4), nullptr);
    ASSERT_EQ(csoundEvalCode(csound, R"(
      Sarguments:S[] argv
      return lenarray(Sarguments)
    )"), 4);

    const char *invalidArguments[] = {nullptr};
    ASSERT_EQ(csoundSetCommandLineArgs(csound, 1, invalidArguments),
              CSOUND_ERROR);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 4);

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 0, nullptr), CSOUND_SUCCESS);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 0);
    ASSERT_EQ(csoundEvalCode(csound, R"(
      Sarguments:S[] argv
      return lenarray(Sarguments)
    )"), 0);

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 4, arguments), CSOUND_SUCCESS);
    csoundReset(csound);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 0);
}

TEST_F (EngineTests, testCommandLineArgumentSeparator)
{
    const char *staleArguments[] = {"stale"};
    const char *arguments[] = {
      "csound",
      "-n",
      "--suppress-version",
      "--code=instr 1\nendin",
      "--",
      "concert.orc",
      "first violin",
      "--logfile=ignored",
      ""
    };

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 1, staleArguments),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompile(csound, 9, arguments), CSOUND_SUCCESS);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 4);
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 0), "concert.orc");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 1), "first violin");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 2), "--logfile=ignored");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 3), "");
}

TEST_F (EngineTests, testExplicitCommandLineArgumentsSurviveCompile)
{
    const char *applicationArguments[] = {"concert.orc", "first violin"};
    const char *compileArguments[] = {
      "csound",
      "-n",
      "--suppress-version",
      "--code=instr 1\nendin"
    };

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 2, applicationArguments),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompile(csound, 4, compileArguments), CSOUND_SUCCESS);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 2);
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 0), "concert.orc");
    ASSERT_STREQ(csoundGetCommandLineArg(csound, 1), "first violin");
}

TEST_F (EngineTests, testEmptyCommandLineArgumentSeparator)
{
    const char *staleArguments[] = {"stale"};
    const char *compileArguments[] = {
      "csound",
      "-n",
      "--suppress-version",
      "--code=instr 1\nendin",
      "--"
    };

    ASSERT_EQ(csoundSetCommandLineArgs(csound, 1, staleArguments),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompile(csound, 5, compileArguments), CSOUND_SUCCESS);
    ASSERT_EQ(csoundGetCommandLineArgCount(csound), 0);
}

TEST_F (EngineTests, testRealtimeAsyncCompileMergesOnEventThread)
{
    csoundSetOption(csound, "-n");
    csoundSetOption(csound, "-d");
    csoundSetOption(csound, "--realtime");
    ASSERT_EQ(csoundCompileOrc(csound, R"(
      sr = 48000
      ksmps = 64
      nchnls = 1
      0dbfs = 1
      instr 1
      endin
    )", 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);

    ASSERT_EQ(csoundCompileOrc(csound, R"(
      instr AsyncMerged
      endin
    )", 1), CSOUND_SUCCESS);

    int32_t insno = 0;
    for (int32_t i = 0; i < 200 && insno <= 0; ++i) {
      csoundSleep(10);
      insno = csoundGetInstrNumber(csound, "AsyncMerged");
    }

    ASSERT_GT(insno, 0);
}

TEST_F (EngineTests, testRealtimeAllocQueueRejectsOverflow)
{
    csound->alloc_queue = static_cast<ALLOC_DATA *>(
      csound->Calloc(csound, sizeof(ALLOC_DATA) * MAX_ALLOC_QUEUE));
    ASSERT_NE(csound->alloc_queue, nullptr);
    ASSERT_EQ(alloc_queue_lock_init(csound), CSOUND_SUCCESS);
    ATOMIC_SET(csound->alloc_queue_items, 0);
    csound->alloc_queue_wp = 0;

    for (int32_t i = 0; i < MAX_ALLOC_QUEUE; ++i) {
      ALLOC_DATA data = { 0 };
      data.insno = i + 1;
      ASSERT_EQ(alloc_queue_enqueue(csound, &data), CSOUND_SUCCESS)
        << "queue item " << i;
    }

    ASSERT_EQ(ATOMIC_GET(csound->alloc_queue_items), MAX_ALLOC_QUEUE);
    ASSERT_EQ(csound->alloc_queue_wp, 0u);
    for (int32_t i = 0; i < MAX_ALLOC_QUEUE; ++i)
      ASSERT_EQ(csound->alloc_queue[i].insno, i + 1);

    ALLOC_DATA overflow = { 0 };
    overflow.insno = MAX_ALLOC_QUEUE + 1;
    ASSERT_EQ(alloc_queue_enqueue(csound, &overflow), CSOUND_ERROR);
    ASSERT_EQ(ATOMIC_GET(csound->alloc_queue_items), MAX_ALLOC_QUEUE);
    ASSERT_EQ(csound->alloc_queue_wp, 0u);
    ASSERT_EQ(csound->alloc_queue[0].insno, 1);

    int32_t realtime = csound->oparms->realtime;
    EVTBLK event = { 0 };
    MCHNBLK channel = { 0 };
    MEVENT midi = { 0 };
    csound->oparms->realtime = 1;
    ASSERT_EQ(insert_event(csound, 1, &event), CSOUND_ERROR);
    ASSERT_EQ(insert_midi_event(csound, 1, &channel, &midi), CSOUND_ERROR);
    csound->oparms->realtime = realtime;

    ATOMIC_SET(csound->alloc_queue_items, 0);
    csound->alloc_queue_wp = 0;
    alloc_queue_lock_destroy(csound);
    csound->Free(csound, csound->alloc_queue);
    csound->alloc_queue = nullptr;
}

TEST_F (EngineTests, testRealtimeInsertEventCopiesQueuedEvtblk)
{
    csound->alloc_queue = static_cast<ALLOC_DATA *>(
      csound->Calloc(csound, sizeof(ALLOC_DATA) * MAX_ALLOC_QUEUE));
    ASSERT_NE(csound->alloc_queue, nullptr);
    ASSERT_EQ(alloc_queue_lock_init(csound), CSOUND_SUCCESS);

    int32_t realtime = csound->oparms->realtime;
    MYFLT pfields[] = { FL(0.0), FL(1.0), FL(0.0), FL(0.25) };
    char strarg[] = "First\0Second";
    EVTBLK event = { 0 };
    event.opcod = 'i';
    event.pcnt = 3;
    event.p = pfields;
    event.scnt = 2;
    event.strarg = strarg;
    csound->oparms->realtime = 1;

    ASSERT_EQ(insert_event(csound, 1, &event), CSOUND_SUCCESS);
    ASSERT_EQ(ATOMIC_GET(csound->alloc_queue_items), 1);

    ALLOC_DATA *queued = &csound->alloc_queue[0];
    ASSERT_EQ(queued->type, ALLOC_DATA_SCORE_EVENT);
    ASSERT_EQ(queued->insno, 1);
    ASSERT_NE(queued->blk.p, nullptr);
    ASSERT_NE(queued->blk.strarg, nullptr);
    ASSERT_NE(queued->blk.p, event.p);
    ASSERT_NE(queued->blk.strarg, event.strarg);
    ASSERT_EQ(queued->blk.pcnt, event.pcnt);
    ASSERT_EQ(queued->blk.p[1], FL(1.0));
    ASSERT_STREQ(queued->blk.strarg, "First");
    ASSERT_STREQ(queued->blk.strarg + std::strlen("First") + 1, "Second");

    pfields[1] = FL(99.0);
    strarg[0] = 'X';
    ASSERT_EQ(queued->blk.p[1], FL(1.0));
    ASSERT_STREQ(queued->blk.strarg, "First");

    csound->Free(csound, queued->blk.p);
    csound->Free(csound, queued->blk.strarg);
    queued->blk.p = nullptr;
    queued->blk.strarg = nullptr;
    csound->oparms->realtime = realtime;
    alloc_queue_lock_destroy(csound);
    csound->Free(csound, csound->alloc_queue);
    csound->alloc_queue = nullptr;
}

TEST_F (EngineTests, testRealtimeAllocQueueMultipleProducers)
{
    constexpr int32_t producerCount = 4;
    constexpr int32_t itemsPerProducer = MAX_ALLOC_QUEUE / producerCount;
    std::atomic<int32_t> failures {0};
    std::atomic<int32_t> ready {0};
    std::atomic<bool> start {false};
    std::vector<std::thread> producers;

    csound->alloc_queue = static_cast<ALLOC_DATA *>(
      csound->Calloc(csound, sizeof(ALLOC_DATA) * MAX_ALLOC_QUEUE));
    ASSERT_NE(csound->alloc_queue, nullptr);
    ASSERT_EQ(alloc_queue_lock_init(csound), CSOUND_SUCCESS);

    for (int32_t producer = 0; producer < producerCount; ++producer) {
      producers.emplace_back([this, producer, itemsPerProducer, &failures, &ready, &start]() {
        ready++;
        while (!start.load())
          std::this_thread::yield();
        for (int32_t index = 0; index < itemsPerProducer; ++index) {
          ALLOC_DATA data = { 0 };
          data.insno = producer * itemsPerProducer + index + 1;
          if (alloc_queue_enqueue(csound, &data) != CSOUND_SUCCESS)
            failures++;
        }
      });
    }

    while (ready.load() != producerCount)
      std::this_thread::yield();
    start = true;

    for (auto &producer : producers)
      producer.join();

    ASSERT_EQ(failures.load(), 0);
    ASSERT_EQ(csound->alloc_queue_items, MAX_ALLOC_QUEUE);
    std::vector<int32_t> instrumentNumbers;
    for (int32_t index = 0; index < MAX_ALLOC_QUEUE; ++index)
      instrumentNumbers.push_back(csound->alloc_queue[index].insno);
    std::sort(instrumentNumbers.begin(), instrumentNumbers.end());
    for (int32_t index = 0; index < MAX_ALLOC_QUEUE; ++index)
      ASSERT_EQ(instrumentNumbers[index], index + 1);

    alloc_queue_lock_destroy(csound);
    csound->Free(csound, csound->alloc_queue);
    csound->alloc_queue = nullptr;
}
