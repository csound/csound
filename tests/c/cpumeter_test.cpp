#ifndef WIN32
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <float.h>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>
#if defined(__MACH__)
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#endif

#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(name)

namespace {
int32_t initError(CSOUND *, const char *, ...) { return NOTOK; }
int32_t perfError(CSOUND *, OPDS *, const char *, ...) { return NOTOK; }

template <typename T> struct Meter {
    T opcode = {};
    INSDS instrument = {};
    OPTXT text = {};
    MYFLT interval = FL(0.015625);
    std::array<MYFLT, 3> cells[32];
    explicit Meter(int outputs) {
        instrument.esr = 1024;
        instrument.ksmps = 8;
        text.t.outArgCount = outputs;
        opcode.h.insdshead = &instrument;
        opcode.h.optext = &text;
        opcode.itrig = &interval;
        for (int i = 0; i < 32; ++i) {
            cells[i] = {123, -1, 456};
            opcode.kk[i] = i < outputs ? &cells[i][1] : nullptr;
        }
    }
    void checkGuards() {
        for (int i = 0; i < 32; ++i) {
            EXPECT_EQ(cells[i][0], 123);
            EXPECT_EQ(cells[i][2], 456);
            if (i >= text.t.outArgCount) EXPECT_EQ(cells[i][1], -1);
        }
    }
};

class CpuMeterTests : public ::testing::Test {
protected:
    CSOUND *csound;
    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csound->InitError = initError;
        csound->PerfError = perfError;
    }
    void TearDown() override { csoundDestroy(csound); }
};
} // namespace

namespace linux_meter {
std::string contents;
std::set<FILE *> files;
bool failOpen;
void replace(FILE *file, const std::string &text) {
    fflush(file);
    EXPECT_EQ(ftruncate(fileno(file), 0), 0);
    rewind(file);
    EXPECT_GE(fputs(text.c_str(), file), 0);
    fflush(file);
    rewind(file);
}
FILE *openProc(const char *path, const char *) {
    EXPECT_STREQ(path, "/proc/stat");
    if (failOpen) { errno = ENOENT; return nullptr; }
    FILE *file = tmpfile();
    if (file) { files.insert(file); replace(file, contents); }
    return file;
}
int closeProc(FILE *file) {
    EXPECT_EQ(files.erase(file), 1u);
    return fclose(file);
}
#ifndef LINUX
#define LINUX
#define UNDEFINE_LINUX
#endif
#define fopen openProc
#define fclose closeProc
#include "../../Opcodes/cpumeter.c"
#undef fclose
#undef fopen
#ifdef UNDEFINE_LINUX
#undef LINUX
#undef UNDEFINE_LINUX
#endif

const char *baseline =
    "cpu 100 0 100 800 0 0 0 0\n"
    "cpu0 50 0 50 400 0 0 0 0\n"
    "cpu1 50 0 50 400 0 0 0 0\nintr 0\n";
const char *updated =
    "cpu 140 0 100 860 0 0 0 0\n"
    "cpu0 60 0 50 430 0 0 0 0\n"
    "cpu1 80 0 50 430 0 0 0 0\nintr 0\n";

TEST_F(CpuMeterTests, LinuxParsesCoreLabelsAndMeasuresInterval) {
    contents = baseline;
    Meter<CPUMETER> meter(4);
    ASSERT_EQ(cpupercent_init(csound, &meter.opcode), OK);
    EXPECT_EQ(meter.cells[0][1], 0);
    replace(meter.opcode.fp, updated);
    ASSERT_EQ(cpupercent(csound, &meter.opcode), OK);
    EXPECT_EQ(meter.cells[0][1], 0); // The refresh interval is two blocks.
    ASSERT_EQ(cpupercent(csound, &meter.opcode), OK);
    EXPECT_EQ(meter.cells[0][1], 40);
    EXPECT_EQ(meter.cells[1][1], 25);
    EXPECT_EQ(meter.cells[2][1], 50);
    EXPECT_EQ(meter.cells[3][1], 0);
    ASSERT_EQ(cpupercent_renew(csound, &meter.opcode), OK);
    EXPECT_EQ(meter.cells[0][1], 0); // No elapsed ticks.
    meter.checkGuards();
    contents = updated;
    ASSERT_EQ(cpupercent_init(csound, &meter.opcode), OK);
    EXPECT_EQ(files.size(), 1u);
    EXPECT_EQ(meter.cells[0][1], 0);
    EXPECT_EQ(deinit_cpupercent(csound, &meter.opcode), OK);
    EXPECT_EQ(deinit_cpupercent(csound, &meter.opcode), OK);
    EXPECT_TRUE(files.empty());
}

TEST_F(CpuMeterTests, LinuxMissingCoresAndCounterDecreases) {
    contents = baseline;
    Meter<CPUMETER> meter(4);
    ASSERT_EQ(cpupercent_init(csound, &meter.opcode), OK);
    replace(meter.opcode.fp,
        "cpu 120 0 100 780 0 0 0 0\n"
        "cpu0 60 0 50 390 0 0 0 0\n"
        "cpu2 900 0 0 100 0 0 0 0\nintr 0\n");
    ASSERT_EQ(cpupercent_renew(csound, &meter.opcode), OK);
    EXPECT_EQ(meter.cells[0][1], 100);
    EXPECT_EQ(meter.cells[1][1], 100);
    EXPECT_EQ(meter.cells[2][1], 0);
    EXPECT_EQ(meter.cells[3][1], 0);
    meter.checkGuards();
    EXPECT_EQ(deinit_cpupercent(csound, &meter.opcode), OK);
}

TEST_F(CpuMeterTests, LinuxFailuresReleaseFilesAndValidateInterval) {
    Meter<CPUMETER> meter(1);
    for (MYFLT interval : {MYFLT(-1), std::numeric_limits<MYFLT>::infinity(),
                          std::numeric_limits<MYFLT>::quiet_NaN()}) {
        meter.interval = interval;
        EXPECT_EQ(cpupercent_init(csound, &meter.opcode), NOTOK);
        EXPECT_TRUE(files.empty());
    }
    meter.interval = 0;
    failOpen = true;
    EXPECT_EQ(cpupercent_init(csound, &meter.opcode), NOTOK);
    failOpen = false;
    contents = "cpu broken\n";
    EXPECT_EQ(cpupercent_init(csound, &meter.opcode), NOTOK);
    EXPECT_TRUE(files.empty());
    contents = baseline;
    ASSERT_EQ(cpupercent_init(csound, &meter.opcode), OK);
    replace(meter.opcode.fp, "intr 0\n");
    EXPECT_EQ(cpupercent(csound, &meter.opcode), NOTOK);
    EXPECT_EQ(deinit_cpupercent(csound, &meter.opcode), OK);
    EXPECT_TRUE(files.empty());
    meter.checkGuards();
}

TEST_F(CpuMeterTests, EngineClosesProcFileAtNoteEnd) {
    contents = baseline;
    const auto &entry = cpumeter_localops[0];
    ASSERT_EQ(csoundAppendOpcode(csound, "test_cpumeter", entry.dsblksiz,
        entry.flags, entry.outypes, entry.intypes,
        entry.init, entry.perf, entry.deinit), OK);
    csoundSetOption(csound, "-n");
    ASSERT_EQ(csoundCompileCSD(csound,
        "<CsoundSynthesizer>\n<CsInstruments>\n"
        "sr=1024\nksmps=8\nnchnls=1\n"
        "instr 1\nkusage test_cpumeter .01\nendin\n"
        "</CsInstruments>\n<CsScore>\ni 1 0 .125\ne\n"
        "</CsScore>\n</CsoundSynthesizer>\n", 1, 0), OK);
    ASSERT_EQ(csoundStart(csound), OK);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(files.size(), 1u);
    while (csoundPerformKsmps(csound) == 0) {}
    EXPECT_TRUE(files.empty());
}
} // namespace linux_meter

#if defined(__MACH__) && !defined(LINUX)
namespace mach_meter {
std::vector<integer_t> ticks;
std::map<vm_address_t, vm_size_t> allocations;
bool failQuery;
kern_return_t getInfo(host_t, processor_flavor_t, natural_t *cpus,
                      processor_info_array_t *info, mach_msg_type_number_t *count) {
    if (failQuery) return KERN_FAILURE;
    *cpus = (natural_t)ticks.size() / CPU_STATE_MAX;
    *count = (mach_msg_type_number_t)ticks.size();
    *info = (integer_t *)malloc(ticks.size() * sizeof(integer_t));
    memcpy(*info, ticks.data(), ticks.size() * sizeof(integer_t));
    allocations[(vm_address_t)*info] = ticks.size() * sizeof(integer_t);
    return KERN_SUCCESS;
}
kern_return_t releaseInfo(vm_map_t, vm_address_t address, vm_size_t size) {
    EXPECT_EQ(allocations.at(address), size);
    allocations.erase(address);
    free((void *)address);
    return KERN_SUCCESS;
}
#define host_processor_info getInfo
#define vm_deallocate releaseInfo
#include "../../Opcodes/cpumeter.c"
#undef vm_deallocate
#undef host_processor_info

TEST_F(CpuMeterTests, MachWritesOnlyRequestedOutputsAndIncludesAllCores) {
    for (int outputs : {1, 3, 32}) {
        Meter<CPUMETER> meter(outputs);
        ticks.assign(64 * CPU_STATE_MAX, 0);
        ASSERT_EQ(cpupercent_init(csound, &meter.opcode), OK);
        for (int cpu = 0; cpu < 64; ++cpu)
            ticks[cpu * CPU_STATE_MAX + (cpu < 32 ? CPU_STATE_USER : CPU_STATE_IDLE)] = 40;
        ASSERT_EQ(cpupercent(csound, &meter.opcode), OK);
        EXPECT_EQ(meter.cells[0][1], 0);
        ASSERT_EQ(cpupercent(csound, &meter.opcode), OK);
        EXPECT_EQ(meter.cells[0][1], 50);
        for (int output = 1; output < outputs; ++output)
            EXPECT_EQ(meter.cells[output][1], 100);
        EXPECT_EQ(allocations.size(), 1u);
        meter.checkGuards();
        EXPECT_EQ(deinit_cpupercent(csound, &meter.opcode), OK);
        EXPECT_TRUE(allocations.empty());
    }
}

TEST_F(CpuMeterTests, MachWrapsCountersAndResetsBaselineOnReinit) {
    Meter<CPUMETER> meter(4);
    ticks.assign(CPU_STATE_MAX, 0);
    ticks[CPU_STATE_USER] = -6; // Unsigned counter near wraparound.
    ticks[CPU_STATE_IDLE] = 100;
    ASSERT_EQ(cpupercent_init(csound, &meter.opcode), OK);
    ticks[CPU_STATE_USER] = 4;
    ticks[CPU_STATE_IDLE] = 130;
    ASSERT_EQ(cpupercent_renew(csound, &meter.opcode), OK);
    EXPECT_EQ(meter.cells[0][1], 25);
    EXPECT_EQ(meter.cells[1][1], 25);
    EXPECT_EQ(meter.cells[2][1], 0);
    EXPECT_EQ(meter.cells[3][1], 0);
    ASSERT_EQ(cpupercent_renew(csound, &meter.opcode), OK);
    EXPECT_EQ(meter.cells[0][1], 0);
    ASSERT_EQ(cpupercent_init(csound, &meter.opcode), OK);
    EXPECT_EQ(allocations.size(), 1u);
    EXPECT_EQ(meter.cells[0][1], 0);
    ticks.resize(2 * CPU_STATE_MAX, 200);
    ASSERT_EQ(cpupercent_renew(csound, &meter.opcode), OK);
    EXPECT_EQ(meter.cells[0][1], 0);
    failQuery = true;
    EXPECT_EQ(cpupercent_renew(csound, &meter.opcode), NOTOK);
    EXPECT_EQ(allocations.size(), 1u);
    EXPECT_EQ(cpupercent_init(csound, &meter.opcode), NOTOK);
    EXPECT_TRUE(allocations.empty());
    failQuery = false;
    EXPECT_EQ(deinit_cpupercent(csound, &meter.opcode), OK);
    meter.checkGuards();
}
} // namespace mach_meter
#endif
#endif
