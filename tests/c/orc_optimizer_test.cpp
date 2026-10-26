#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
using Engine = std::unique_ptr<CSOUND, decltype(&csoundDestroy)>;
Engine engine(int passes, bool dump = false) {
    Engine cs(csoundCreate(nullptr, nullptr), csoundDestroy);
    csoundCreateMessageBuffer(cs.get(), 0);
    EXPECT_EQ(0, csoundSetOption(cs.get(), "-n -d -m4"));
    std::string option = "-+orc_opt_passes=" + std::to_string(passes);
    EXPECT_EQ(0, csoundSetOption(cs.get(), option.c_str()));
    if (dump) EXPECT_EQ(0, csoundSetOption(cs.get(), "--verbose=0x200"));
    return cs;
}
std::string messages(CSOUND *cs) {
    std::string result;
    while (csoundGetMessageCnt(cs)) {
        result += csoundGetFirstMessage(cs);
        csoundPopFirstMessage(cs);
    }
    return result;
}
std::string optimizerDumps(const std::string &log) {
    std::string result;
    size_t position = 0;
    while ((position = log.find("ORC-OPT ", position)) != std::string::npos) {
        size_t end = log.find("ORC-OPT end\n", position);
        if (end == std::string::npos) {
            ADD_FAILURE() << "Incomplete optimizer dump";
            break;
        }
        end += std::strlen("ORC-OPT end\n");
        result.append(log, position, end - position);
        position = end;
    }
    return result;
}
std::string header(int block) {
    return "sr=48000\nksmps=" + std::to_string(block) +
           "\nnchnls=1\n0dbfs=1\nseed 12345\n";
}
enum class RenderStage { Compilation, Startup, Performance };
struct RenderStep {
    RenderStage stage;
    int32_t status;
    std::string log;
};
struct Render {
    std::vector<cs_float> samples;
    std::vector<RenderStep> steps;
};
Render render(int passes, int block, const std::string &body) {
    auto cs = engine(passes);
    Render result;
    auto record = [&](RenderStage stage, int32_t status) {
        result.steps.push_back({stage, status, messages(cs.get())});
        return status;
    };
    std::string source = header(block) + body;
    if (record(RenderStage::Compilation,
               csoundCompileOrc(cs.get(), source.c_str(), 0)) != 0) return result;
    if (record(RenderStage::Startup, csoundStart(cs.get())) != 0) return result;
    csoundEventString(cs.get(), "i 1 0 1", 0);
    for (int i = 0; i < 8; ++i) {
        if (record(RenderStage::Performance, csoundPerformKsmps(cs.get())) != 0)
            break;
        const cs_float *out = csoundGetSpout(cs.get());
        result.samples.insert(result.samples.end(), out, out + block);
    }
    return result;
}
void identical(const Render &a, const Render &b, int32_t expectedExit = 0) {
    ASSERT_GE(a.steps.size(), 3u);
    ASSERT_EQ(a.steps.size(), b.steps.size());
    EXPECT_EQ(RenderStage::Compilation, a.steps[0].stage);
    EXPECT_EQ(RenderStage::Startup, a.steps[1].stage);
    for (size_t i = 0; i < a.steps.size(); ++i) {
        EXPECT_EQ(a.steps[i].stage, b.steps[i].stage);
        EXPECT_EQ(a.steps[i].status, b.steps[i].status);
        EXPECT_EQ(i + 1 == a.steps.size() ? expectedExit : 0, a.steps[i].status);
        EXPECT_EQ(a.steps[i].log, b.steps[i].log);
        if (i >= 2) EXPECT_EQ(RenderStage::Performance, a.steps[i].stage);
    }
    if (expectedExit == 0) EXPECT_EQ(10u, a.steps.size());
    ASSERT_EQ(a.samples.size(), b.samples.size());
    // Bit comparison intentionally catches signed zero and rounding changes.
    if (!a.samples.empty())
        EXPECT_EQ(0, std::memcmp(a.samples.data(), b.samples.data(),
                                a.samples.size() * sizeof(cs_float)));
}

int32_t traceInit(CSOUND *cs, void *) {
    static_cast<std::string *>(csoundGetHostData(cs))->append("i");
    return OK;
}
int32_t tracePerf(CSOUND *cs, void *) {
    static_cast<std::string *>(csoundGetHostData(cs))->append("p");
    return OK;
}
int32_t traceDeinit(CSOUND *cs, void *) {
    static_cast<std::string *>(csoundGetHostData(cs))->append("d");
    return OK;
}

TEST(OrcOptimizer, UdoPreservesObservableLifecycle) {
    for (int pass : {0, 1}) {
        std::string trace;
        auto cs = engine(pass);
        csoundSetHostData(cs.get(), &trace);
        ASSERT_EQ(0, csoundAppendOpcode(cs.get(), "optimizer_trace", sizeof(OPDS),
            0, "", "", traceInit, tracePerf, traceDeinit));
        std::string source = header(16) +
            "opcode Trace,0,0\noptimizer_trace\nendop\ninstr 1\nTrace\nendin\n";
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
        ASSERT_EQ(0, csoundStart(cs.get()));
        csoundEventString(cs.get(), "i 1 0 1", 0);
        for (int i = 0; i < 3; i++) ASSERT_EQ(0, csoundPerformKsmps(cs.get()));
        cs.reset();
        EXPECT_EQ("ipppd", trace);
    }
}

TEST(OrcOptimizer, ArithmeticHasIndependentExpectedResult) {
    for (int block : {1, 16, 64, 256}) {
        const char *source = "instr 1\nkx = (2+3)*4\naout = kx/2\nout aout\nendin\n";
        auto off = render(0, block, source);
        auto on = render(1, block, source);
        identical(off, on);
        for (auto sample : off.samples) EXPECT_EQ(FL(10), sample);
    }
}

TEST(OrcOptimizer, StatefulAndControlFlowNegativeControls) {
    const char *programs[] = {
        "instr 1\na1 random -0.2,0.2\na2 random -0.2,0.2\nout a1+a2\nendin\n",
        "instr 1\na1 oscili 0.2,440\na2 delay a1,0.001\nout a1+a2\nendin\n",
        "instr 1\nkx init 0\nkx += 1\nif kx > 3 then\nkx = 1\nendif\n"
        "aout = kx\nout aout\nendin\n",
        "opcode Identity,a,a\nsetksmps 1\na1 xin\nxout a1\nendop\n"
        "instr 1\na1 oscili 0.2,440\na2 Identity a1\nout a2\nendin\n",
        "instr 1\nkx init 0\nkx += 1\nif kx == 2 then\nreinit again\nendif\n"
        "again:\nprints \"optimizer-init\\n\"\nrireturn\naout = kx\nout aout\nendin\n",
        "instr 1\nkx[] fillarray 1,2,3\nkindex init 0\nkindex = (kindex+1)%3\n"
        "aout = kx[kindex]\nout aout\nendin\n"
    };
    for (const char *source : programs) for (int block : {1, 64}) {
        SCOPED_TRACE(source);
        identical(render(0, block, source), render(1, block, source));
    }
}

TEST(OrcOptimizer, UnusedDivisionStillWarns) {
    const char *source = "instr 1\nkzero init 0\nkunused = 1/kzero\naout = 0\nout aout\nendin\n";
    auto off = render(0, 16, source);
    identical(off, render(1, 16, source));
    ASSERT_GE(off.steps.size(), 3u);
    EXPECT_NE(std::string::npos, off.steps[2].log.find("Division by zero"));
}

TEST(OrcOptimizer, PreservesStartupMessagesAndTheirOrder) {
    const char *source =
        "prints \"global-first\\n\"\nprints \"global-second\\n\"\n"
        "instr 1\nprints \"note-init-message\\n\"\naout = 0\nout aout\nendin\n";
    auto off = render(0, 16, source);
    identical(off, render(1, 16, source));
    ASSERT_GE(off.steps.size(), 3u);
    const auto &startup = off.steps[1].log;
    const auto first = startup.find("global-first\n");
    const auto second = startup.find("global-second\n");
    ASSERT_NE(std::string::npos, first);
    ASSERT_NE(std::string::npos, second);
    EXPECT_LT(first, second);
    EXPECT_NE(std::string::npos, off.steps[2].log.find("note-init-message\n"));
}

TEST(OrcOptimizer, RecordsExpectedNonzeroExitAndStopsRendering) {
    const char *source =
        "instr 1\nprintf \"performance-exit-message\\n\", 1\nexitnowk 7\nendin\n";
    auto off = render(0, 16, source);
    identical(off, render(1, 16, source), -7);
    ASSERT_EQ(3u, off.steps.size());
    EXPECT_EQ(-7, off.steps.back().status);
    EXPECT_TRUE(off.samples.empty());
    EXPECT_NE(std::string::npos, off.steps.back().log.find("performance-exit-message\n"));
}

TEST(OrcOptimizer, StableDumpsAndLiteralOperationsRemain) {
    std::string reference;
    for (int pass : {0, 1}) {
        auto cs = engine(pass, true);
        std::string source = header(16) +
            "instr 1\nkx = (2+3)*4\nif kx > 1 then\nkx += 1\nendif\nendin\n";
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
        auto log = optimizerDumps(messages(cs.get()));
        ASSERT_NE(std::string::npos, log.find("ORC-OPT before"));
        EXPECT_NE(std::string::npos, log.find("ORC-OPT templates phase=resolved"));
        EXPECT_NE(std::string::npos, log.find("op ##add"));
        EXPECT_NE(std::string::npos, log.find("op ##mul"));
        EXPECT_EQ(std::string::npos, log.find("__synthetic_"));
        EXPECT_EQ(std::string::npos, log.find("#k"));
        if (reference.empty()) reference = log;
        else EXPECT_EQ(reference, log);
    }
}

TEST(OrcOptimizer, DumpsResolvedOperandTypesAlongsideWildcardSignatures) {
    const char *body = R"(
opcode CopyValue,k,k
  inputValue:k xin
  xout inputValue
endop
instr 1
  initial:i = 1
  control:k init 2
  signal:a init 0
  text:S = "value"
  values:k[] init 2
  control = initial
  control = CopyValue(control)
endin
)";
    std::string reference;
    for (int pass : {0, 1}) {
        auto cs = engine(pass, true);
        std::string source = header(16) + body;
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
        const auto log = optimizerDumps(messages(cs.get()));
        EXPECT_NE(std::string::npos, log.find("op xin out=*"));
        EXPECT_NE(std::string::npos, log.find("op xout out= in=*"));
        size_t position = 0;
        for (int stage = 0; stage < 3; ++stage) {
            const auto end = log.find("ORC-OPT end\n", position);
            ASSERT_NE(std::string::npos, end);
            const auto dump = log.substr(position, end - position);
            for (const char *operand : {"inputValue:k", "initial:i", "control:k",
                                        "signal:a", "text:S", "values:k[]", "1:c"}) {
                EXPECT_NE(std::string::npos, dump.find(operand)) << operand;
            }
            EXPECT_NE(std::string::npos, dump.find("[control:k] <- [initial:i]"));
            position = end + std::strlen("ORC-OPT end\n");
        }
        if (reference.empty()) reference = log;
        else EXPECT_EQ(reference, log);
    }
}

TEST(OrcOptimizer, DumpsStructuredOperandBindingsInEveryStage) {
    const char *body = R"(
struct Field value:i
struct Record field:Field, value:i
globalField@global:Field init 3
shared@global:Record init globalField, 4
instr 1
  field:Field init 11
  thing:Record init field, 22
  first:i = thing.value
  nested:i = thing.field.value
  thing.value = 33
  globalRead:i = shared.value
endin
)";
    std::string reference;
    for (int pass : {0, 1}) {
        auto cs = engine(pass, true);
        std::string source = header(16) + body;
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
        const auto log = optimizerDumps(messages(cs.get()));
        size_t position = 0;
        for (int stage = 0; stage < 3; ++stage) {
            const auto end = log.find("ORC-OPT end\n", position);
            ASSERT_NE(std::string::npos, end);
            const auto dump = log.substr(position, end - position);
            for (const char *binding : {"[first:i] <- [thing.value:i]",
                                        "[nested:i] <- [thing.field.value:i]",
                                        "[thing.value:i] <- [33:c]",
                                        "[globalRead:i] <- [shared.value:i]"}) {
                EXPECT_NE(std::string::npos, dump.find(binding)) << binding;
            }
            position = end + std::strlen("ORC-OPT end\n");
        }
        if (reference.empty()) reference = log;
        else EXPECT_EQ(reference, log);
    }
}

TEST(OrcOptimizer, ControlsArePerEngineAndResetToLegacyDefault) {
    auto off = engine(0), on = engine(1, true);
    auto controls = [](CSOUND *cs) {
        return static_cast<int32_t *>(csoundQueryGlobalVariable(cs, "__orc_optimizer"));
    };
    ASSERT_NE(nullptr, controls(off.get()));
    EXPECT_EQ(0, controls(off.get())[0]);
    EXPECT_EQ(1, controls(on.get())[0]);
    EXPECT_EQ(0, csoundGetDebug(off.get()) & DEBUG_OPTIMIZER);
    EXPECT_NE(0, csoundGetDebug(on.get()) & DEBUG_OPTIMIZER);
    EXPECT_NE(0, off->ParseConfigurationVariable(off.get(), "orc_opt_passes", "2"));
    csoundSetDebug(off.get(), DEBUG_OPTIMIZER);
    csoundReset(off.get());
    ASSERT_NE(nullptr, controls(off.get()));
    EXPECT_EQ(1, controls(off.get())[0]);
    EXPECT_EQ(0, csoundGetDebug(off.get()) & DEBUG_OPTIMIZER);
}

TEST(OrcOptimizer, VerboseMaskSelectsOptimizerDumps) {
    const std::pair<const char *, int32_t> cases[] = {
        {"--verbose=0", 0}, {"--verbose=0x10", DEBUG_PARSER},
        {"--verbose=0x200", DEBUG_OPTIMIZER},
        {"--verbose=0x210", DEBUG_OPTIMIZER | DEBUG_PARSER},
        {"--verbose=0x7fffffff", DEBUG_FULL},
        {"-v512", DEBUG_OPTIMIZER},
        {"--verbose", DEBUG_FULL & ~DEBUG_OPTIMIZER},
        {"-v", 0xFFFFFFF & ~DEBUG_OPTIMIZER}
    };
    for (const auto &testCase : cases) {
        const char *option = testCase.first;
        SCOPED_TRACE(option);
        auto cs = engine(1);
        ASSERT_EQ(0, csoundSetOption(cs.get(), option));
        EXPECT_EQ(testCase.second, csoundGetDebug(cs.get()));
        const bool enabled = (testCase.second & DEBUG_OPTIMIZER) != 0;
        std::string source = header(16) + "instr 1\nkx = (2+3)*4\nendin\n";
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
        const auto log = messages(cs.get());
        for (const char *stage : {"ORC-OPT before", "ORC-OPT after",
                                  "ORC-OPT templates phase=resolved"}) {
            EXPECT_EQ(enabled, log.find(stage) != std::string::npos);
        }
    }
}

TEST(OrcOptimizer, ParseCompileDeleteAndInvalidInput) {
    for (int pass : {0, 1}) {
        auto cs = engine(pass);
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), header(16).c_str(), 0));
        for (int i = 0; i < 20; ++i) {
            TREE *tree = csoundParseOrc(cs.get(), i % 2 ? "" :
                "instr 1\nkx = (2+3)*4\nendin\n");
            ASSERT_NE(nullptr, tree);
            EXPECT_EQ(0, csoundCompileTree(cs.get(), tree, 0));
            csoundDeleteTree(cs.get(), tree);
        }
        EXPECT_EQ(nullptr, csoundParseOrc(cs.get(),
            "instr 2\nkx = no_such_optimizer_function(1)\nendin\n"));
        EXPECT_FALSE(messages(cs.get()).empty());
    }
}
} // namespace
