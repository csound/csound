/* Measure actual INSDS allocation requests as notes accumulate.
   Build against the matching source and library. See optimizer/README.md.
   This uses private fields for measurement, not as a supported host API. */
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

struct Allocations {
    void *(*allocate)(CSOUND *, size_t);
    std::unordered_map<void *, size_t> sizes;
};

static void *recordAllocation(CSOUND *cs, size_t bytes) {
    auto *record = static_cast<Allocations *>(csoundGetHostData(cs));
    void *pointer = record->allocate(cs, bytes);
    record->sizes[pointer] = bytes;
    return pointer;
}

static void check(CSOUND *cs, int result) {
    if (!result) return;
    while (csoundGetMessageCnt(cs)) {
        std::fputs(csoundGetFirstMessage(cs), stderr);
        csoundPopFirstMessage(cs);
    }
    std::exit(1);
}

int main(int argc, char **argv) {
    if (argc != 3 || (std::strcmp(argv[2], "repeated") &&
                     std::strcmp(argv[2], "unique") &&
                     std::strcmp(argv[2], "literals"))) {
        std::fprintf(stderr, "usage: %s OPTIMIZER-NAMES repeated|unique|literals\n", argv[0]);
        return 2;
    }
    Allocations record;
    CSOUND *cs = csoundCreate(&record, nullptr);
    if (!cs) return 1;
    csoundCreateMessageBuffer(cs, 0);
    check(cs, csoundSetOption(cs, "-n -d -m0"));
    check(cs, csoundSetOption(cs, "--optimize=none"));
    check(cs, csoundSetOption(cs, (std::string("--optimize=") + argv[1]).c_str()));
    std::string source = "sr=48000\nksmps=64\nnchnls=1\n0dbfs=1\ninstr 1\n"
                         "kInput init p4\nkSum = 0\n";
    for (int i = 0; i < 512; ++i) {
        const auto index = std::to_string(i);
        const auto value = std::to_string(!std::strcmp(argv[2], "repeated") ? i % 8 : i);
        if (!std::strcmp(argv[2], "literals"))
            source += "iValue" + index + " = (2+" + value + ")*3\n";
        else
            source += "kValue" + index + " = (kInput+" + value + ")*2\n";
    }
    for (int i = 0; i < 512; ++i)
        source += std::string("kSum += ") +
                  (!std::strcmp(argv[2], "literals") ? "iValue" : "kValue") +
                  std::to_string(i) + "\n";
    source += "aOut = kSum*0.000001\nout aOut\nendin\n";
    check(cs, csoundCompileOrc(cs, source.c_str(), 0));
    check(cs, csoundStart(cs));
    record.allocate = cs->Calloc;
    uint64_t hash = UINT64_C(14695981039346656037);
    auto capture = [&]() {
        const auto *bytes = reinterpret_cast<const unsigned char *>(csoundGetSpout(cs));
        for (size_t i = 0; i < 64 * sizeof(cs_float); ++i) {
            hash ^= bytes[i];
            hash *= UINT64_C(1099511628211);
        }
    };
    for (int step = 0; step < 8; ++step) {
        for (int note = 0; note < 16; ++note)
            csoundEventString(cs, "i 1 0 100 2", 0);
        // Observe allocation only during activation. Keep it out of render timing.
        cs->Calloc = recordAllocation;
        check(cs, csoundPerformKsmps(cs));
        cs->Calloc = record.allocate;
        capture();
        auto *instrument = cs->engineState.instrtxtp[1];
        size_t total = 0, instanceBytes = 0;
        int voices = 0;
        for (auto *instance = instrument->instance; instance; instance = instance->nxtinstance) {
            auto found = record.sizes.find(instance);
            if (found == record.sizes.end()) return 3;
            instanceBytes = found->second;
            total += instanceBytes;
            ++voices;
        }
        if (voices != (step + 1) * 16) return 4;
        const auto start = std::chrono::steady_clock::now();
        for (int block = 0; block < 128; ++block)
            check(cs, csoundPerformKsmps(cs));
        const auto end = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(end - start).count();
        capture();
        std::printf("{\"step\":%d,\"seconds\":%.6f,\"voices\":%d,"
                    "\"pool_bytes\":%d,\"variables\":%d,\"instance_bytes\":%zu,"
                    "\"total_instance_bytes\":%zu,\"render_ms\":%.6f,\"audio_hash\":\"%016llx\"}\n",
                    step, (step + 1) * 129.0 * 64 / 48000, voices,
                    instrument->varPool->poolSize, instrument->varPool->varCount,
                    instanceBytes, total, ms, static_cast<unsigned long long>(hash));
    }
    csoundDestroy(cs);
    return 0;
}
