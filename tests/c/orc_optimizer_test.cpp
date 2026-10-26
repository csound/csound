#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "prototyp.h"
#include "csdebug.h"
#include "csound_orc.h"
#include "csound_orc_optimize.h"
extern "C" TREE *csound_orc_optimize(CSOUND *, TREE *, TYPE_TABLE *);
extern "C" {
#include "new_orc_parser.h"
}
#include "gtest/gtest.h"
#include <cstring>
#include <cmath>
#include <clocale>
#include <limits>
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
    EXPECT_EQ(0, passes & ~(CSOUND_OPT_LEGACY | CSOUND_OPT_CONSTANT_FOLDING |
                          CSOUND_OPT_COPY_REMOVAL | CSOUND_OPT_CSE |
                          CSOUND_OPT_AUDIO_MUL_ADD));
    EXPECT_EQ(0, csoundSetOption(cs.get(), "--optimize=none"));
    if (passes & CSOUND_OPT_LEGACY)
        EXPECT_EQ(0, csoundSetOption(cs.get(), "--optimize-legacy"));
    if (passes & CSOUND_OPT_CONSTANT_FOLDING)
        EXPECT_EQ(0, csoundSetOption(cs.get(), "--optimize-constant-folding"));
    if (passes & CSOUND_OPT_COPY_REMOVAL)
        EXPECT_EQ(0, csoundSetOption(cs.get(), "--optimize-copy-removal"));
    if (passes & CSOUND_OPT_CSE)
        EXPECT_EQ(0, csoundSetOption(cs.get(), "--optimize-common-subexpression-elimination"));
    if (passes & CSOUND_OPT_AUDIO_MUL_ADD)
        EXPECT_EQ(0, csoundSetOption(cs.get(), "--optimize-audio-mul-add"));
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
    for (int pass : {0, 1, 9}) {
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
    for (int pass : {0, 1, 9}) {
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
    for (int pass : {0, 1, 9}) {
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
    for (int pass : {0, 1, 9}) {
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
    EXPECT_EQ(nullptr, off->QueryConfigurationVariable(off.get(), "orc_opt_passes"));
    csoundSetDebug(off.get(), DEBUG_OPTIMIZER);
    csoundReset(off.get());
    ASSERT_NE(nullptr, controls(off.get()));
    EXPECT_EQ(1, controls(off.get())[0]);
    EXPECT_EQ(0, csoundGetDebug(off.get()) & DEBUG_OPTIMIZER);
}

TEST(OrcOptimizer, NamedOptionsComposeAndSupportAllAndNone) {
    auto cs = engine(1);
    auto selection = static_cast<int32_t *>(
        csoundQueryGlobalVariable(cs.get(), "__orc_optimizer"));
    ASSERT_NE(nullptr, selection);
    // Run these in order to check accumulation, clearing and duplicate names.
    const std::pair<const char *, int> cases[] = {
        {"--optimize-copy-removal", 5},
        {"--optimize=none", 0},
        {"--optimize=copy-removal", 4},
        {"--optimize-legacy", 5},
        {"--optimize=none", 0},
        {"--optimize=legacy,copy-removal", 5},
        {"--optimize=none", 0},
        {"--optimize=copy-removal,legacy,copy-removal", 5},
        {"--optimize=none", 0},
        {"--optimize=constant-folding", 2},
        {"--optimize-copy-removal", 6},
        {"--optimize-legacy", 7},
        {"--optimize=none", 0},
        {"--optimize=legacy,constant-folding", 3},
        {"--optimize=none", 0},
        {"--optimize=constant-folding,legacy,constant-folding", 3},
        {"--optimize=none", 0},
        {"--optimize-common-subexpression-elimination", 8},
        {"--optimize-constant-folding", 10},
        {"--optimize-legacy", 11},
        {"--optimize=none", 0},
        {"--optimize=common-subexpression-elimination", 8},
        {"--optimize=constant-folding,legacy,common-subexpression-elimination", 11},
        {"--optimize=none", 0},
        {"--optimize=legacy,common-subexpression-elimination", 9},
        {"--optimize-audio-mul-add", 41},
        {"--optimize=all", 47},
        {"--optimize-constant-folding --optimize-legacy", 47},
        {"--optimize=all --optimize=none --optimize-legacy", 1}
    };
    for (const auto &test : cases) {
        SCOPED_TRACE(test.first);
        ASSERT_EQ(0, csoundSetOption(cs.get(), test.first));
        EXPECT_EQ(test.second, *selection);
    }
}

TEST(OrcOptimizer, InvalidNamesLeaveTheSelectionUnchanged) {
    auto cs = engine(1);
    auto selection = static_cast<int32_t *>(
        csoundQueryGlobalVariable(cs.get(), "__orc_optimizer"));
    ASSERT_NE(nullptr, selection);
    messages(cs.get());
    for (const char *option : {"--optimize", "--optimize=", "--optimize-",
                              "--optimize=unknown", "--optimize-unknown",
                              "--optimize=copy-removal,unknown", "--optimize=legacy,",
                              "--optimize=,legacy", "--optimize=legacy,,copy-removal",
                              "--optimize=all,legacy", "--optimize=none,legacy",
                              "--optimize-legacy,copy-removal", "--optimize=3",
                              "--optimize-copy-removal=0",
                              "--optimize=constant-folding,unknown",
                              "--optimize=legacy,,constant-folding",
                              "--optimize-legacy,constant-folding",
                              "--optimize-constant-folding=0",
                              "--optimize=common-subexpression-elimination,unknown",
                              "--optimize=legacy,,common-subexpression-elimination",
                              "--optimize-legacy,common-subexpression-elimination",
                              "--optimize-common-subexpression-elimination=0"}) {
        SCOPED_TRACE(option);
        EXPECT_NE(0, csoundSetOption(cs.get(), option));
        EXPECT_EQ(1, *selection);
        const auto log = messages(cs.get());
        EXPECT_NE(std::string::npos, log.find("Csound Command ERROR"));
        EXPECT_NE(std::string::npos, log.find(
            !std::strcmp(option, "--optimize") ? "--optimize requires"
                                               : "optimization name"));
    }
    EXPECT_EQ(0, csoundSetOption(cs.get(), "--optimize=all"));
    EXPECT_EQ(47, *selection);
}

TEST(OrcOptimizer, MissingSettingsReportAnOptionError) {
    auto cs = engine(1);
    ASSERT_EQ(0, csoundDestroyGlobalVariable(cs.get(), "__orc_optimizer"));
    messages(cs.get());
    EXPECT_NE(0, csoundSetOption(cs.get(), "--optimize-copy-removal"));
    EXPECT_NE(std::string::npos,
              messages(cs.get()).find("optimizer settings are unavailable"));
}

TEST(OrcOptimizer, NamedPassesAreAdditiveAndRejectBadListsAtomically) {
    auto cs = engine(0);
    auto selected = [&] {
        return *static_cast<int32_t *>(csoundQueryGlobalVariable(cs.get(), "__orc_optimizer"));
    };
    ASSERT_EQ(0, csoundSetOption(cs.get(), "--optimize=audio-mul-add"));
    EXPECT_EQ(32, selected());
    ASSERT_EQ(0, csoundSetOption(cs.get(), "--optimize-legacy"));
    EXPECT_EQ(33, selected());
    ASSERT_EQ(0, csoundSetOption(cs.get(), "--optimize=none"));
    EXPECT_EQ(0, selected());
    ASSERT_EQ(0, csoundSetOption(cs.get(), "--optimize=legacy,audio-mul-add"));
    EXPECT_EQ(33, selected());
    ASSERT_EQ(0, csoundSetOption(cs.get(), "--optimize=none"));
    for (const char *option : {"--optimize=legacy,unknown", "--optimize=legacy,",
                              "--optimize=", "--optimize", "--optimize-unknown"}) {
        SCOPED_TRACE(option);
        EXPECT_NE(0, csoundSetOption(cs.get(), option));
        EXPECT_EQ(0, selected());
    }
    ASSERT_EQ(0, csoundSetOption(cs.get(), "--optimize=all"));
    EXPECT_EQ(47, selected());
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
    for (int pass = 0; pass < 16; ++pass) {
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

std::string copyDump(int passes, const char *body) {
    auto cs = engine(passes, true);
    const std::string source = header(16) + body;
    EXPECT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
    const auto log = optimizerDumps(messages(cs.get()));
    const auto begin = log.find("ORC-OPT after");
    EXPECT_NE(std::string::npos, begin);
    return begin == std::string::npos ? "" :
        log.substr(begin, log.find("ORC-OPT end", begin) - begin);
}

TEST(OrcOptimizer, ScalarCopyWritesTheLocalDestination) {
    const char *body = "instr 1\niValue=p4+1\naOut=iValue\nout aOut\nendin\n";
    const auto before = copyDump(1, body);
    EXPECT_NE(std::string::npos, before.find("[iValue:i] <- [t1:i]"));
    // Copy removal must work both alone and with the default legacy pass.
    for (int passes : {4, 5}) {
        SCOPED_TRACE(passes);
        const auto after = copyDump(passes, body);
        EXPECT_NE(std::string::npos, after.find("[iValue:i] <- [p4:p,1:c]"));
        EXPECT_EQ(std::string::npos, after.find("[iValue:i] <- [t1:i]"));
        for (int block : {1, 16, 64})
            identical(render(1, block, body), render(passes, block, body));
    }
}

TEST(OrcOptimizer, ScalarCopiesKeepAliasedInputsAndExpressionOrder) {
    const char *bodies[] = {
        "instr 1\niValue init 7\niValue=iValue+1\niValue=20-iValue\n"
        "iValue=iValue*2\naOut=iValue\nout aOut\nendin\n",
        "instr 1\niValue=(p4+1)*(p4-2)\naOut=iValue\nout aOut\nendin\n",
        "instr 1\nvalue:i = p4+1\naOut=value\nout aOut\nendin\n",
        "instr 1\niValue=(16777216+1)-16777216\naOut=iValue\nout aOut\nendin\n",
        "instr 1\niValue init 0\nif p4==0 then\niValue=p4+2\n"
        "else\niValue=p4-3\nendif\naOut=iValue\nout aOut\nendin\n",
        "instr 1\nkCount init 0\nkCount+=1\nif kCount==2 then\nreinit again\n"
        "endif\nagain:\niValue=p4+2\nprints \"value=%g\\n\",iValue\n"
        "rireturn\naOut=iValue\nout aOut\nendin\n"
    };
    for (const char *body : bodies) {
        SCOPED_TRACE(body);
        identical(render(1, 16, body), render(5, 16, body));
    }
}

TEST(OrcOptimizer, ScalarCopiesLeaveOtherRatesAndStorageAlone) {
    const char *bodies[] = {
        "instr 1\nkValue=p4+1\nendin\n",
        "instr 1\naValue=p4+1\nendin\n",
        "giValue init 0\ninstr 1\ngiValue=p4+1\nendin\n",
        "giSource init 2\ninstr 1\niValue=giSource+1\nendin\n",
        "iGlobal=2+3\ninstr 1\nendin\n",
        "opcode Value,i,i\niIn xin\niOut=iIn+1\nxout iOut\nendop\n"
        "instr 1\niValue=Value(2)\nendin\n",
        "instr 1\niValue=1/0\nendin\n",
        "instr 1\niArray[] init 2\niArray[0]=p4+1\nendin\n",
        "struct Record value:i\ninstr 1\nrecord:Record init 0\n"
        "record.value=p4+1\niValue=record.value+1\nendin\n",
        "value@global:i init 0\ninstr 1\nvalue=p4+1\nendin\n",
        "instr 1\nSValue=strcat(\"a\",\"b\")\nendin\n",
        // The ternary's shared temporary has multiple definitions and a label.
        "instr 1\niValue=(p4>0 ? p4+1 : p4-1)\nendin\n"
    };
    for (const char *body : bodies) {
        SCOPED_TRACE(body);
        EXPECT_EQ(copyDump(1, body), copyDump(5, body));
    }
}

struct CopyOverload { OPDS h; cs_float *out, *left, *right; };
int32_t copyOverload(CSOUND *, void *data) {
    static_cast<CopyOverload *>(data)->out[0] = FL(42);
    return OK;
}

TEST(OrcOptimizer, ScalarCopiesKeepAnOverloadedProducer) {
    for (int pass : {1, 5}) {
        auto cs = engine(pass, true);
        OENTRY op = {};
        op.opname = const_cast<char *>("##add.ii");
        op.dsblksiz = sizeof(CopyOverload);
        op.outypes = const_cast<char *>("i");
        op.intypes = const_cast<char *>("ii");
        op.init = copyOverload;
        ASSERT_EQ(0, csoundPrependOpcodes(cs.get(), &op, 1));
        const auto source = header(16) +
            "instr 1\niValue=p4+1\nchnset iValue,\"value\"\nendin\n";
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
        const auto log = optimizerDumps(messages(cs.get()));
        const auto after = log.find("ORC-OPT after");
        ASSERT_NE(std::string::npos, after);
        EXPECT_NE(std::string::npos, log.substr(after).find(
            "[iValue:i] <- [t1:i]"));
        ASSERT_EQ(0, csoundStart(cs.get()));
        csoundEventString(cs.get(), "i 1 0 1 2", 0);
        ASSERT_EQ(0, csoundPerformKsmps(cs.get()));
        int32_t error = 0;
        EXPECT_EQ(FL(42), csoundGetControlChannel(cs.get(), "value", &error));
        EXPECT_EQ(0, error);
    }
}

TEST(OrcOptimizer, ScalarCopiesKeepATemporaryWithAnotherRead) {
    auto cs = engine(1);
    const auto source = header(16) +
        "instr 1\niValue=p4+1\niOther=iValue\nendin\n";
    TREE *tree = csoundParseOrc(cs.get(), source.c_str());
    ASSERT_NE(nullptr, tree);
    TREE *instrument = tree->next;
    while (instrument && instrument->type != INSTR_TOKEN) instrument = instrument->next;
    ASSERT_NE(nullptr, instrument);
    TREE *producer = instrument->right;
    ASSERT_NE(nullptr, producer);
    ASSERT_NE(nullptr, producer->left);
    const std::string temporary = producer->left->value->lexeme;
    ASSERT_EQ(0, temporary.find("#i"));
    TREE *copy = producer->next;
    ASSERT_NE(nullptr, copy);
    TREE *otherRead = copy->next;
    ASSERT_NE(nullptr, otherRead);
    ASSERT_NE(nullptr, otherRead->right);

    // Generated names cannot be written in orchestra source. Point the second
    // assignment at the same temporary to test the single-read guard directly.
    cs->Free(cs.get(), otherRead->right->value->lexeme);
    otherRead->right->value->lexeme = csoundStrdup(cs.get(), temporary.c_str());
    ASSERT_EQ(0, csoundSetOption(cs.get(), "--optimize-copy-removal"));
    csound_orc_optimize(cs.get(), tree->next, static_cast<TYPE_TABLE *>(tree->markup));
    EXPECT_EQ(copy, producer->next);
    EXPECT_EQ(temporary, producer->left->value->lexeme);
    csoundDeleteTree(cs.get(), tree);
}

void copyBreakpoint(CSOUND *, debug_bkpt_info_t *info, void *data) {
    ++*static_cast<int *>(data);
    EXPECT_EQ(9, info->currentOpcode->line);
    bool found = false;
    for (auto *var = info->instrVarList; var; var = var->next) {
        if (!strcmp(var->name, "iValue")) {
            EXPECT_EQ(FL(3), *static_cast<cs_float *>(var->data));
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST(OrcOptimizer, ScalarCopiesKeepDebuggerLinesAndNamedValues) {
    for (int pass : {1, 5}) {
        auto cs = engine(pass);
        const auto source = header(16) +
            "instr 1\niValue=p4+1\nkCount init 0\nkCount=kCount+1\nendin\n";
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
        ASSERT_EQ(0, csoundStart(cs.get()));
        csoundEventString(cs.get(), "i 1 0 1 2", 0);
        int hits = 0;
        ASSERT_EQ(0, csoundDebuggerInit(cs.get()));
        csoundSetBreakpointCallback(cs.get(), copyBreakpoint, &hits);
        csoundSetBreakpoint(cs.get(), 9, 1, 0);
        csoundPerformKsmps(cs.get());
        EXPECT_EQ(1, hits);
        csoundDebuggerClean(cs.get());
    }
}

std::string foldedDump(int passes, const std::string &body) {
    auto cs = engine(passes, true);
    const std::string source = header(16) + body;
    EXPECT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
    const auto log = optimizerDumps(messages(cs.get()));
    const auto begin = log.find("ORC-OPT after");
    EXPECT_NE(std::string::npos, begin);
    return begin == std::string::npos ? "" :
        log.substr(begin, log.find("ORC-OPT end", begin) - begin);
}

TEST(OrcOptimizer, LiteralFoldingRemovesOnlyTheArithmetic) {
    const char *body = "instr 1\niValue = (2+3)*(4-1)\n"
                       "prints \"value=%g\\n\", iValue\nendin\n";
    const auto before = foldedDump(1, body);
    const auto after = foldedDump(3, body);
    EXPECT_NE(std::string::npos, before.find("op ##add.ii"));
    for (const char *op : {"op ##add.ii", "op ##sub.ii", "op ##mul.ii"})
        EXPECT_EQ(std::string::npos, after.find(op));
    EXPECT_NE(std::string::npos, after.find("[iValue:i] <- [15:c]"));
    identical(render(1, 16, body), render(3, 16, body));
}

TEST(OrcOptimizer, LiteralFoldingKeepsCommaDecimalHostLocale) {
    // Restore the host locale even when an assertion fails or the test skips.
    struct LocaleGuard {
        std::string saved = std::setlocale(LC_NUMERIC, nullptr);
        ~LocaleGuard() { std::setlocale(LC_NUMERIC, saved.c_str()); }
    } guard;
    auto cs = engine(3, true);
    bool found = false;
    for (const char *name : {"de_DE.UTF-8", "de_DE.utf8", "de_DE",
                             "fr_FR.UTF-8", "fr_FR.utf8", "fr_FR",
                             "German_Germany.1252", "German_Germany.65001"}) {
        if (std::setlocale(LC_NUMERIC, name) &&
            !std::strcmp(std::localeconv()->decimal_point, ",")) {
            found = true;
            break;
        }
    }
    if (!found) GTEST_SKIP() << "No comma-decimal locale is installed";

    // Some targets only have the host's strtod. This checks folding on builds
    // where the constant-pool reader already supports a different host locale.
    char input[] = "1.5", *end;
    if (csoundStrtod(input, &end) != FL(1.5) || *end != '\0')
        GTEST_SKIP() << "No locale-independent constant-pool reader in this build";
    const std::string hostLocale = std::setlocale(LC_NUMERIC, nullptr);
    const auto source = header(16) +
        "iValue = (1.2 + 3.4) * 2\nreturn iValue\n";
    const cs_float expected = (FL(1.2) + FL(3.4)) * FL(2);
    const cs_float result = csoundEvalCode(cs.get(), source.c_str());
    EXPECT_EQ(0, std::memcmp(&expected, &result, sizeof(result)));
    const auto log = optimizerDumps(messages(cs.get()));
    const auto begin = log.find("ORC-OPT after");
    ASSERT_NE(std::string::npos, begin);
    const auto after = log.substr(begin, log.find("ORC-OPT end", begin) - begin);
    EXPECT_EQ(std::string::npos, after.find("op ##add.ii"));
    EXPECT_EQ(std::string::npos, after.find("op ##mul.ii"));

    // The generated token's cached value must also use the folded result,
    // rather than reading the dot-decimal spelling through the host's atof.
    TREE *tree = csoundParseOrc(cs.get(), source.c_str());
    ASSERT_NE(nullptr, tree);
    TREE *assignment = tree->next;
    while (assignment && (!assignment->left || !assignment->left->value ||
           std::strcmp(assignment->left->value->lexeme, "iValue")))
        assignment = assignment->next;
    ASSERT_NE(nullptr, assignment);
    ASSERT_NE(nullptr, assignment->right);
    EXPECT_EQ(NUMBER_TOKEN, assignment->right->type);
    EXPECT_EQ(expected, assignment->right->value->fvalue);
    csoundDeleteTree(cs.get(), tree);
    EXPECT_EQ(hostLocale, std::setlocale(LC_NUMERIC, nullptr));
    EXPECT_STREQ(",", std::localeconv()->decimal_point);
}

TEST(OrcOptimizer, LiteralFoldingPreservesSamplePrecisionAndSignedZero) {
    struct Case { const char *expression; cs_float expected; };
    const Case cases[] = {
        {"2+3", FL(5)}, {"(2+3)*(4-1)", FL(15)},
        {"(16777216+1)-16777216", sizeof(cs_float) == 4 ? FL(0) : FL(1)},
        {"(9007199254740992+1)-9007199254740992", FL(0)},
        {"(-0.0)*2", -FL(0.0)}, {"(-0.0)+(-0.0)", -FL(0.0)},
        {"0.1+0.2", FL(0.1)+FL(0.2)}, {"1e-20*2", FL(1e-20)*FL(2)},
        // Stay finite near the upper bound of the configured sample type.
        {sizeof(cs_float) == 4 ? "1e38*2" : "1e308*1.5",
         sizeof(cs_float) == 4 ? (cs_float)2e38 : (cs_float)1.5e308}
    };
    for (const auto &test : cases) {
        SCOPED_TRACE(test.expression);
        cs_float values[2];
        for (int mode = 0; mode < 2; ++mode) {
            auto cs = engine(mode ? 3 : 1);
            const std::string source = header(16) + "iResult = " +
                                       test.expression + "\nreturn iResult\n";
            values[mode] = csoundEvalCode(cs.get(), source.c_str());
            EXPECT_EQ(0, std::memcmp(&values[mode], &test.expected, sizeof(cs_float)));
        }
        EXPECT_EQ(0, std::memcmp(&values[0], &values[1], sizeof(cs_float)));
    }
}

TEST(OrcOptimizer, LiteralFoldingLeavesUncertainOperationsAlone) {
    const char *bodies[] = {
        "instr 1\niValue = 1/0\nendin\n",
        "instr 1\niValue = 1e300*1e300\nendin\n",
        "instr 1\niValue = 1e-300*1e-300\nendin\n",
        // Finite inputs can still produce overflow or a subnormal result.
        sizeof(cs_float) == 4 ? "instr 1\niValue = 3e38*2\nendin\n"
                              : "instr 1\niValue = 1e308*2\nendin\n",
        sizeof(cs_float) == 4 ? "instr 1\niValue = 2e-38*0.5\nendin\n"
                              : "instr 1\niValue = 3e-308*0.5\nendin\n",
        "instr 1\niValue = p4+3\nendin\n",
        "instr 1\niSource init 2\niValue = iSource+3\nendin\n",
        // Preserve the temporary when its consumer runs at a different phase.
        "instr 1\nkValue = 2+3\nendin\n",
        // Unknown opcodes may retain or write their input storage.
        "instr 1\nprints \"value=%g\\n\", 2+3\nendin\n"
    };
    for (const auto *body : bodies) {
        SCOPED_TRACE(body);
        EXPECT_EQ(foldedDump(1, body), foldedDump(3, body));
    }
}

TEST(OrcOptimizer, LiteralFoldingPreservesControlFlowAndUdoInitialization) {
    const char *bodies[] = {
        "instr 1\niValue init 0\nif p4 == 0 then\niValue=(2+3)*4\n"
        "else\niValue=(7-2)*3\nendif\naOut=iValue\nout aOut\nendin\n",
        "opcode Value,i,0\niValue=(2+3)*4\nxout iValue\nendop\n"
        "instr 1\niValue Value\naOut=iValue\nout aOut\nendin\n",
        "instr 1\nkCount init 0\nkCount+=1\nif kCount==2 then\nreinit again\n"
        "endif\nagain:\niValue=(2+3)*4\nprints \"init=%g\\n\",iValue\n"
        "rireturn\naOut=iValue\nout aOut\nendin\n"
    };
    for (const auto *body : bodies)
        for (int block : {1, 64}) identical(render(1, block, body), render(3, block, body));
}

struct LiteralOverload { OPDS h; cs_float *out, *left, *right; };
int32_t literalOverload(CSOUND *cs, void *data) {
    ++*static_cast<int *>(csoundGetHostData(cs));
    static_cast<LiteralOverload *>(data)->out[0] = FL(42);
    return OK;
}

TEST(OrcOptimizer, LiteralFoldingDoesNotCallOrReplaceOverloads) {
    for (int pass : {1, 3}) {
        int calls = 0;
        auto cs = engine(pass);
        csoundSetHostData(cs.get(), &calls);
        OENTRY overload = {};
        overload.opname = const_cast<char *>("##add.ii");
        overload.dsblksiz = sizeof(LiteralOverload);
        overload.outypes = const_cast<char *>("i");
        overload.intypes = const_cast<char *>("ii");
        overload.init = literalOverload;
        ASSERT_EQ(0, csoundPrependOpcodes(cs.get(), &overload, 1));
        const std::string source = header(16) +
            "iValue=2+3\nchnset iValue,\"result\"\n";
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
        EXPECT_EQ(0, calls);
        ASSERT_EQ(0, csoundStart(cs.get()));
        EXPECT_EQ(1, calls);
        int32_t error = 0;
        EXPECT_EQ(FL(42), csoundGetControlChannel(cs.get(), "result", &error));
        EXPECT_EQ(0, error);
    }
}
std::string cseDump(int passes, const std::string &body) {
    auto cs = engine(passes, true);
    EXPECT_EQ(0, csoundCompileOrc(cs.get(), (header(16) + body).c_str(), 0));
    const auto log = optimizerDumps(messages(cs.get()));
    const auto begin = log.find("ORC-OPT after");
    EXPECT_NE(std::string::npos, begin);
    return begin == std::string::npos ? "" :
        log.substr(begin, log.find("ORC-OPT end", begin) - begin);
}
size_t occurrences(const std::string &text, const std::string &part) {
    size_t count = 0, at = 0;
    while ((at = text.find(part, at)) != std::string::npos) {
        ++count;
        at += part.size();
    }
    return count;
}

TEST(OrcOptimizer, ReusesResolvedScalarExpressionsAtEachRate) {
    for (const char rate : {'i', 'k'}) {
        const std::string input(1, rate);
        const auto body = "instr 1\n" + input + "Input init 2\n" +
            input + "First=(" + input + "Input+3)*4\n" +
            input + "Second=(" + input + "Input+3)*4\n" +
            input + "Third=(" + input + "Input+3)*4\n" +
            "aOut=" + input + "Second\nout aOut\nendin\n";
        const auto before = cseDump(1, body), after = cseDump(9, body);
        for (const char *operation : {"op ##add", "op ##mul"}) {
            EXPECT_EQ(3u, occurrences(before, operation));
            EXPECT_EQ(1u, occurrences(after, operation));
        }
        for (int block : {1, 16, 64, 256}) {
            auto result = render(9, block, body);
            identical(render(1, block, body), result);
            for (auto value : result.samples) EXPECT_EQ(FL(20), value);
        }
    }
    // Explicit type annotations need not follow the old i/k name prefixes.
    const char *body = "instr 1\nvalue:k init 2\nfirst:k=value+3\n"
                       "second:k=value+3\nendin\n";
    EXPECT_EQ(1u, occurrences(cseDump(9, body), "op ##add"));
}

TEST(OrcOptimizer, ScalarCseInvalidatesWritesAndKeepsOperandOrder) {
    const char *body = R"(
instr 1
  kInput = 2
  kFirst = kInput - 1
  kInput = 5
  kSecond = kInput - 1
  kReversed = 1 - kInput
  aOut = kFirst + kSecond + kReversed
  out aOut
endin
)";
    EXPECT_EQ(3u, occurrences(cseDump(9, body), "op ##sub"));
    auto result = render(9, 16, body);
    identical(render(1, 16, body), result);
    for (auto value : result.samples) EXPECT_EQ(FL(1), value);
}

TEST(OrcOptimizer, ConstantFoldingAndScalarCseWorkTogether) {
    const char *body = R"(
instr 1
  iInput = 2
  iFirst = (iInput + 3) * 4
  iSecond = (iInput + 3) * 4
  iConstant = (2 + 3) * (4 - 1)
  aOut = iFirst + iSecond + iConstant
  out aOut
endin
)";
    // Folding and CSE must run with or without legacy and copy removal.
    for (int pass : {10, 11, 14, 15}) {
        const auto dump = cseDump(pass, body);
        EXPECT_NE(std::string::npos, dump.find("[iConstant:i] <- [15:c]"));
        EXPECT_EQ(1u, occurrences(dump, "op ##mul"));
        auto result = render(pass, 16, body);
        identical(render(1, 16, body), result);
        for (auto value : result.samples) EXPECT_EQ(FL(55), value);
    }
}

TEST(OrcOptimizer, ScalarCseStopsAtCallsLabelsAndPhaseChanges) {
    const char *bodies[] = {
        "instr 1\nkValue init 2\nkFirst=kValue+1\n"
        "printks \"barrier\\n\",0\nkSecond=kValue+1\nendin\n",
        "instr 1\nkValue init 2\nkFirst=kValue+1\n"
        "boundary:\nkSecond=kValue+1\nendin\n",
        "instr 1\niValue init 2\niFirst=iValue+1\nkValue=3\n"
        "iSecond=iValue+1\nendin\n",
        "giValue init 2\ninstr 1\niFirst=giValue+1\niSecond=giValue+1\nendin\n",
        "instr 1\naValue init 2\naFirst=aValue+1\naSecond=aValue+1\nendin\n",
        "instr 1\nkValues[] fillarray 1,2\nkFirst=kValues[0]+1\n"
        "kSecond=kValues[0]+1\nendin\n",
        "struct Item value:k\ninstr 1\nitem:Item init 2\n"
        "kFirst=item.value+1\nkSecond=item.value+1\nendin\n",
        "opcode Pair,k,k\nkInput xin\nkFirst=kInput+1\n"
        "kSecond=kInput+1\nxout kSecond\nendop\ninstr 1\nkResult=Pair(2)\nendin\n",
        "instr 1\nkZero init 0\nkFirst=1/kZero\nkSecond=1/kZero\nendin\n"
    };
    for (const char *body : bodies) {
        SCOPED_TRACE(body);
        EXPECT_EQ(cseDump(1, body), cseDump(9, body));
        identical(render(1, 16, body), render(9, 16, body));
    }
}

TEST(OrcOptimizer, ScalarCseKeepsLoopReinitAndExceptionalValues) {
    const char *bodies[] = {
        "instr 1\nkCount init 0\nkCount+=1\nkSum=0\nkIndex=0\n"
        "while kIndex<3 do\nkFirst=kIndex+1\nkSecond=kIndex+1\n"
        "kSum+=kFirst+kSecond\nkIndex+=1\nod\naOut=kSum+kCount\nout aOut\nendin\n",
        "instr 1\nkCount init 0\nkCount+=1\nif kCount==2 then\nreinit again\n"
        "endif\nagain:\niValue init 2\niFirst=iValue+1\niSecond=iValue+1\n"
        "prints \"value=%g\\n\",iSecond\nrireturn\naOut=iSecond\nout aOut\nendin\n",
        "instr 1\nkValue init -0.0\nkFirst=kValue*2\nkSecond=kValue*2\n"
        "aOut=kSecond\nout aOut\nendin\n",
        "instr 1\nkValue init 1e300\nkFirst=kValue*kValue\nkSecond=kValue*kValue\n"
        "kNan=kFirst-kSecond\nkOne=kNan+1\nkTwo=kNan+1\n"
        "aOut=kTwo\nout aOut\nendin\n"
    };
    for (const char *body : bodies) {
        SCOPED_TRACE(body);
        identical(render(1, 16, body), render(9, 16, body));
    }
}

struct CseOverload { OPDS h; cs_float *out, *left, *right; };
int32_t cseOverload(CSOUND *cs, void *data) {
    auto *counter = static_cast<int *>(csoundGetHostData(cs));
    *static_cast<CseOverload *>(data)->out = ++*counter;
    return OK;
}
TEST(OrcOptimizer, ScalarCseDoesNotTrustAnOverloadedName) {
    for (int pass : {1, 9}) {
        int calls = 0;
        auto cs = engine(pass);
        csoundSetHostData(cs.get(), &calls);
        OENTRY op = {};
        op.opname = const_cast<char *>("##add.kk");
        op.dsblksiz = sizeof(CseOverload);
        op.outypes = const_cast<char *>("k");
        op.intypes = const_cast<char *>("kk");
        op.perf = cseOverload;
        ASSERT_EQ(0, csoundPrependOpcodes(cs.get(), &op, 1));
        const auto source = header(16) +
            "instr 1\nkValue init 2\nkFirst=kValue+1\nkSecond=kValue+1\nendin\n";
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
        ASSERT_EQ(0, csoundStart(cs.get()));
        csoundEventString(cs.get(), "i 1 0 1", 0);
        ASSERT_EQ(0, csoundPerformKsmps(cs.get()));
        EXPECT_EQ(2, calls);
    }
}

TEST(OrcOptimizer, ScalarCseKeepsATemporaryWithALaterRead) {
    auto cs = engine(1);
    const auto source = header(16) +
        "instr 1\niInput=2\niFirst=iInput+1\niSecond=iInput+1\niLater=iSecond\nendin\n";
    TREE *tree = csoundParseOrc(cs.get(), source.c_str());
    ASSERT_NE(nullptr, tree);
    TREE *scope = tree->next;
    while (scope && scope->type != INSTR_TOKEN) scope = scope->next;
    ASSERT_NE(nullptr, scope);
    TREE *duplicate = scope->right->next->next->next;
    ASSERT_NE(nullptr, duplicate);
    const std::string temp = duplicate->left->value->lexeme;
    ASSERT_EQ(0, temp.find("#i"));
    TREE *later = duplicate->next->next;
    ASSERT_NE(nullptr, later);
    // Generated names cannot be written in a score. Add a second use to the
    // lowered tree to check that the pass does not leave that use uninitialized.
    cs->Free(cs.get(), later->right->value->lexeme);
    later->right->value->lexeme = csoundStrdup(cs.get(), temp.c_str());
    ASSERT_EQ(0, csoundSetOption(cs.get(), "--optimize-common-subexpression-elimination"));
    csound_orc_optimize(cs.get(), tree->next, static_cast<TYPE_TABLE *>(tree->markup));
    EXPECT_EQ(duplicate, scope->right->next->next->next);
    EXPECT_EQ(temp, duplicate->left->value->lexeme);
    csoundDeleteTree(cs.get(), tree);
}

TEST(OrcOptimizer, ScalarCseReplacesBothReadsInOneConsumer) {
    for (int pass : {1, 8, 9, 10, 11}) {
        auto cs = engine(1);
        const auto source = header(16) + R"(
instr 1
  iInput = 2
  iFirst = iInput + 1
  iSecond = (iInput + 1) * 2
  aOut = iSecond
  out aOut
endin
)";
        TREE *tree = csoundParseOrc(cs.get(), source.c_str());
        ASSERT_NE(nullptr, tree);
        TREE *scope = tree->next;
        while (scope && scope->type != INSTR_TOKEN) scope = scope->next;
        ASSERT_NE(nullptr, scope);
        TREE *first = scope->right->next;
        TREE *duplicate = first->next->next;
        ASSERT_NE(nullptr, duplicate);
        ASSERT_EQ(first->markup, duplicate->markup);
        TREE *consumer = duplicate->next;
        ASSERT_NE(nullptr, consumer);
        ASSERT_NE(nullptr, consumer->right->next);

        // Generated names cannot appear in source. Make the multiplication
        // read the second addition's temporary twice, as temp * temp.
        const std::string temp = duplicate->left->value->lexeme;
        ASSERT_EQ(0, temp.find("#i"));
        TREE *secondRead = consumer->right->next;
        cs->Free(cs.get(), secondRead->value->lexeme);
        secondRead->value->lexeme = csoundStrdup(cs.get(), temp.c_str());
        secondRead->type = consumer->right->type;
        secondRead->markup = consumer->right->markup;
        *static_cast<int32_t *>(csoundQueryGlobalVariable(cs.get(), "__orc_optimizer")) = pass;
        csound_orc_optimize(cs.get(), tree->next, static_cast<TYPE_TABLE *>(tree->markup));
        if (pass & 8) {
            EXPECT_EQ(consumer, first->next->next);
            EXPECT_STREQ(first->left->value->lexeme, consumer->right->value->lexeme);
            EXPECT_STREQ(first->left->value->lexeme, secondRead->value->lexeme);
        }
        ASSERT_EQ(0, csoundCompileTree(cs.get(), tree, 0));
        csoundDeleteTree(cs.get(), tree);
        ASSERT_EQ(0, csoundStart(cs.get()));
        csoundEventString(cs.get(), "i 1 0 1", 0);
        ASSERT_EQ(0, csoundPerformKsmps(cs.get()));
        for (int sample = 0; sample < 16; ++sample)
            EXPECT_EQ(FL(9), csoundGetSpout(cs.get())[sample]);
    }
}

void cseBreakpoint(CSOUND *, debug_bkpt_info_t *info, void *data) {
    ++*static_cast<int *>(data);
    EXPECT_EQ(11, info->currentOpcode->line);
    int found = 0;
    for (auto *var = info->instrVarList; var; var = var->next) {
        if (!strcmp(var->name, "iFirst") || !strcmp(var->name, "iSecond")) {
            EXPECT_EQ(FL(3), *static_cast<cs_float *>(var->data));
            ++found;
        }
    }
    EXPECT_EQ(2, found);
}
TEST(OrcOptimizer, ScalarCseKeepsDebuggerLinesAndNamedValues) {
    for (int pass : {1, 9}) {
        auto cs = engine(pass);
        const auto source = header(16) +
            "instr 1\niInput=2\niFirst=iInput+1\niSecond=iInput+1\n"
            "kCount init 0\nkCount=kCount+1\nendin\n";
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
        ASSERT_EQ(0, csoundStart(cs.get()));
        csoundEventString(cs.get(), "i 1 0 1", 0);
        int hits = 0;
        ASSERT_EQ(0, csoundDebuggerInit(cs.get()));
        csoundSetBreakpointCallback(cs.get(), cseBreakpoint, &hits);
        csoundSetBreakpoint(cs.get(), 11, 1, 0);
        csoundPerformKsmps(cs.get());
        EXPECT_EQ(1, hits);
        csoundDebuggerClean(cs.get());
    }
}

TEST(OrcOptimizer, FusesOnlyAudioControlAudioExpression) {
    const std::pair<const char *, bool> cases[] = {
        {"aResult = aInput*kGain+aOther", true},
        {"aResult = aOther+aInput*kGain", false},
        {"aResult = kGain*aInput+aOther", false},
        {"aResult = aInput*0.5+aOther", false},
        {"aResult = aInput*aOther+aOther", false},
        {"aResult = aInput*kGain-aOther", false},
        {"aProduct = aInput*kGain\naResult = aProduct+aOther", false},
    };
    for (const auto &test : cases) {
        SCOPED_TRACE(test.first);
        const auto source = header(64) + "instr 1\naInput init 0.5\n"
            "aOther init 0.25\nkGain init 0.5\n" + test.first + "\nout aResult\nendin\n";
        for (int passes : {1, 33}) {
            auto cs = engine(passes, true);
            ASSERT_EQ(0, csoundCompileOrc(cs.get(), source.c_str(), 0));
            const auto log = messages(cs.get());
            EXPECT_EQ(test.second && passes == 33,
                      log.find("op ##muladd.aka") != std::string::npos);
        }
    }
}

TEST(OrcOptimizer, FusedAudioPreservesRoundingZerosAndNonfiniteValues) {
    const cs_float inf = std::numeric_limits<cs_float>::infinity();
    const cs_float nan = std::numeric_limits<cs_float>::quiet_NaN();
    const cs_float epsilon = std::numeric_limits<cs_float>::epsilon();
    const cs_float tiny = std::numeric_limits<cs_float>::denorm_min();
    const std::vector<cs_float> input = {FL(1)+epsilon, -FL(0), FL(0), inf,
        -inf, nan, tiny, -tiny, std::numeric_limits<cs_float>::max(), FL(0.25)};
    const std::vector<cs_float> other = {-FL(1), -FL(0), FL(0), -inf,
        inf, FL(1), FL(0), -FL(0), -inf, nan};
    for (int block : {1, 16, 64, 257}) {
        auto before = engine(1), after = engine(33);
        for (auto *cs : {before.get(), after.get()}) {
            const auto source = header(block) + "instr 1\n"
                "aInput chnget \"input\"\nkGain chnget \"gain\"\n"
                "aOther chnget \"other\"\naResult = aInput*kGain+aOther\n"
                "chnset aResult, \"result\"\nendin\n";
            ASSERT_EQ(0, csoundCompileOrc(cs, source.c_str(), 0));
            ASSERT_EQ(0, csoundStart(cs));
            csoundEventString(cs, "i 1 0 1", 0);
        }
        std::vector<cs_float> a(block), b(block), expected(block), actual(block);
        for (cs_float gain : {FL(1)-epsilon, FL(0), -FL(0), FL(1), FL(-1), inf, nan}) {
            for (size_t shift = 0; shift < input.size(); ++shift) {
                for (int i = 0; i < block; ++i) {
                    a[i] = input[(i + shift) % input.size()];
                    b[i] = other[(i + shift) % other.size()];
                }
                for (auto *cs : {before.get(), after.get()}) {
                    csoundSetAudioChannel(cs, "input", a.data());
                    csoundSetAudioChannel(cs, "other", b.data());
                    csoundSetControlChannel(cs, "gain", gain);
                    ASSERT_EQ(0, csoundPerformKsmps(cs));
                }
                csoundGetAudioChannel(before.get(), "result", expected.data());
                csoundGetAudioChannel(after.get(), "result", actual.data());
                for (int i = 0; i < block; ++i) {
                    SCOPED_TRACE(i);
                    // NaN payload choice is not specified, but all other bits,
                    // including the sign of zero, must match the two callbacks.
                    if (std::isnan(expected[i])) EXPECT_TRUE(std::isnan(actual[i]));
                    else EXPECT_EQ(0, std::memcmp(&expected[i], &actual[i], sizeof(cs_float)));
                    if (a[i] == FL(1)+epsilon && gain == FL(1)-epsilon && b[i] == -FL(1))
                        EXPECT_EQ(FL(0), actual[i]); // an FMA would return -epsilon^2
                }
            }
        }
    }
}

TEST(OrcOptimizer, FusedAudioKeepsLocalBlocksAndSampleAccurateBounds) {
    for (int local : {0, 1, 8}) {
        auto before = engine(1), after = engine(33);
        for (auto *cs : {before.get(), after.get()}) {
            ASSERT_EQ(0, csoundSetOption(cs, "--sample-accurate"));
            const auto source = header(64) + "instr 1\n" +
                (local ? "setksmps " + std::to_string(local) + "\n" : "") +
                "aInput init 0.5\naOther init 0.25\nkGain init 0.5\n"
                "aResult = aInput*kGain+aOther\nout aResult\nendin\n";
            ASSERT_EQ(0, csoundCompileOrc(cs, source.c_str(), 0));
            ASSERT_EQ(0, csoundStart(cs));
            // Start at sample 3 and stop at sample 54 of the first block.
            csoundEventString(cs, "i 1 0.0000625 0.0010625", 0);
        }
        for (int n = 0; n < 3; ++n) {
            ASSERT_EQ(0, csoundPerformKsmps(before.get()));
            ASSERT_EQ(0, csoundPerformKsmps(after.get()));
            EXPECT_EQ(0, std::memcmp(csoundGetSpout(before.get()), csoundGetSpout(after.get()),
                                    64 * sizeof(cs_float)));
            if (n == 0 && local == 0) {
                const cs_float *out = csoundGetSpout(after.get());
                for (int i = 0; i < 64; ++i)
                    EXPECT_EQ(i >= 3 && i < 54 ? FL(0.5) : FL(0), out[i]);
            }
        }
    }
}

TEST(OrcOptimizer, FusionRejectsSharedResultsAliasesAndUnknownMetadata) {
    for (int guard = 0; guard < 6; ++guard) {
        auto cs = engine(1, true);
        TREE *tree = csoundParseOrc(cs.get(), (header(64) + "instr 1\n"
            "aInput init 0.5\naOther init 0.25\nkGain init 0.5\n"
            "aResult = aInput*kGain+aOther\nout aResult\nendin\n").c_str());
        ASSERT_NE(nullptr, tree);
        TREE *scope = tree->next;
        while (scope && scope->type != INSTR_TOKEN) scope = scope->next;
        ASSERT_NE(nullptr, scope);
        TREE *mul = scope->right;
        while (mul && (!mul->value || strcmp(mul->value->lexeme, "##mul"))) mul = mul->next;
        ASSERT_NE(nullptr, mul);
        TREE *add = mul->next, *last = add;
        while (last->next) last = last->next;
        auto *pool = static_cast<CS_VAR_POOL *>(scope->markup);
        OENTRY impostor = *static_cast<OENTRY *>(mul->markup);
        if (guard == 0) {
            // Another read, even after the add, makes the intermediate shared.
            cs->Free(cs.get(), last->right->value->lexeme);
            last->right->value->lexeme = csoundStrdup(cs.get(), mul->left->value->lexeme);
        } else if (guard == 1) {
            // Distinct names in the tree must not hide an in-place output.
            cs->Free(cs.get(), add->left->value->lexeme);
            add->left->value->lexeme = csoundStrdup(cs.get(), mul->right->value->lexeme);
        } else if (guard == 2) {
            last->right->markup = pool; // opaque metadata may retain a binding
        } else if (guard == 3) {
            // Same spelling/signature, but a different implementation.
            impostor.perf = tracePerf;
            mul->markup = &impostor;
        } else if (guard == 4) {
            auto *var = cs_hash_table_get(cs.get(), pool->table, mul->left->value->lexeme);
            cs_hash_table_put(cs.get(), pool->table, const_cast<char *>("alias"), var);
        } else {
            // A source boundary must not disappear when stepping by line.
            ++add->line;
        }
        messages(cs.get());
        ASSERT_EQ(0, csoundSetOption(cs.get(), "--optimize-audio-mul-add"));
        csound_orc_optimize(cs.get(), tree->next, static_cast<TYPE_TABLE *>(tree->markup));
        EXPECT_EQ(std::string::npos, messages(cs.get()).find("op ##muladd.aka"));
        if (guard == 2) last->right->markup = nullptr;
        if (guard == 4) {
            char *key = cs_hash_table_get_key(cs.get(), pool->table, const_cast<char *>("alias"));
            cs_hash_table_remove(cs.get(), pool->table, const_cast<char *>("alias"));
            cs->Free(cs.get(), key);
        }
        csoundDeleteTree(cs.get(), tree);
    }
}

TEST(OrcOptimizer, FusionKeepsUdoAndGlobalAudioUnchanged) {
    const char *bodies[] = {
        "opcode Mix,a,aka\naInput,kGain,aOther xin\naResult=aInput*kGain+aOther\n"
        "xout aResult\nendop\ninstr 1\naInput init 0.5\naOther init 0.25\n"
        "kGain init 0.5\naResult Mix aInput,kGain,aOther\nout aResult\nendin\n",
        "gaInput init 0.5\ninstr 1\naOther init 0.25\nkGain init 0.5\n"
        "aResult=gaInput*kGain+aOther\nout aResult\nendin\n"
    };
    for (const char *body : bodies) {
        auto cs = engine(33, true);
        ASSERT_EQ(0, csoundCompileOrc(cs.get(), (header(16) + body).c_str(), 0));
        EXPECT_EQ(std::string::npos, messages(cs.get()).find("op ##muladd.aka"));
        identical(render(1, 16, body), render(33, 16, body));
    }
}

} // namespace
