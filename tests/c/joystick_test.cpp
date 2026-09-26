#if defined(__linux__) || defined(CSOUND_TEST_JOYSTICK)
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include "../../Opcodes/linuxjoystick.h"
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <map>
#include <limits>
#include <set>
#include <sys/ioctl.h>
#include <vector>

namespace {
struct ReadResult { js_event event; ssize_t size; int error; };
std::map<int, std::deque<ReadResult>> pending;
std::set<int> descriptors;
std::vector<int> closed;
int nextFd, openCalls, readCalls;
bool failOpen, failQuery;
uint8_t axes, buttons;
FUNC tables[3];
std::vector<cs_float> data[3];

int mockOpen(const char *, int flags, ...) {
    ++openCalls;
    EXPECT_NE(flags & O_NONBLOCK, 0);
    if (failOpen) { errno = ENOENT; return -1; }
    descriptors.insert(nextFd);
    return nextFd++;
}
int mockClose(int fd) {
    EXPECT_EQ(descriptors.erase(fd), 1u);
    closed.push_back(fd);
    return 0;
}
int mockIoctl(int fd, unsigned long request, ...) {
    EXPECT_EQ(descriptors.count(fd), 1u);
    if (failQuery) { errno = EIO; return -1; }
    va_list args;
    va_start(args, request);
    auto *result = va_arg(args, uint8_t *);
    *result = request == JSIOCGAXES ? axes : buttons;
    va_end(args);
    return 0;
}
ssize_t mockRead(int fd, void *buffer, size_t size) {
    ++readCalls;
    EXPECT_EQ(descriptors.count(fd), 1u);
    if (pending[fd].empty()) { errno = EAGAIN; return -1; }
    auto result = pending[fd].front();
    pending[fd].pop_front();
    if (result.size > 0) {
        EXPECT_LE((size_t)result.size, size);
        std::memcpy(buffer, &result.event, result.size);
    }
    errno = result.error;
    return result.size;
}

#define open mockOpen
#define close mockClose
#define ioctl mockIoctl
#define read mockRead
#undef LINKAGE
#define LINKAGE
#include "../../Opcodes/linuxjoystick.c"
#undef read
#undef ioctl
#undef close
#undef open

int32_t ignorePerfError(CSOUND *, OPDS *, const char *, ...) { return NOTOK; }
FUNC *findTable(CSOUND *, cs_float *number) {
    if (*number < 1 || *number > 3) return nullptr;
    return &tables[(int)*number - 1];
}
void sizeTable(int index, size_t size) {
    data[index].assign(size, -99);
    tables[index].flen = (uint32_t)size;
    tables[index].ftable = data[index].data();
}
void event(int fd, uint8_t type, uint8_t number, int16_t value) {
    pending[fd].push_back({{0, value, type, number}, sizeof(js_event), 0});
}

struct Instance {
    LINUXJOYSTICK opcode = {};
    cs_float result = -1, device = 0, table = 1;
    CSOUND *csound;
    explicit Instance(CSOUND *engine) : csound(engine) {
        opcode.kresult = &result;
        opcode.kdev = &device;
        opcode.ktable = &table;
        linuxjoystick_init(csound, &opcode);
    }
    ~Instance() { linuxjoystick_deinit(csound, &opcode); }
    int32_t run() { return linuxjoystick(csound, &opcode); }
};

class JoystickTests : public ::testing::Test {
protected:
    CSOUND *csound;
    void SetUp() override {
        pending.clear(); descriptors.clear(); closed.clear();
        nextFd = openCalls = readCalls = 0;
        failOpen = failQuery = false;
        axes = 2; buttons = 3;
        for (int i = 0; i < 3; ++i) sizeTable(i, 8);
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csound->FTFind = findTable;
        csound->PerfError = ignorePerfError;
    }
    void TearDown() override {
        EXPECT_TRUE(descriptors.empty());
        csoundDestroy(csound);
    }
};

TEST_F(JoystickTests, RejectSmallTablesOnEveryCycleAndTableChange) {
    Instance instance(csound);
    sizeTable(0, 2);
    EXPECT_EQ(instance.run(), NOTOK);
    EXPECT_EQ(instance.run(), NOTOK);
    EXPECT_EQ(readCalls, 0);
    EXPECT_EQ(data[0][0], -99);
    sizeTable(0, 8);
    EXPECT_EQ(instance.run(), OK);
    EXPECT_EQ(data[0][0], 2);
    EXPECT_EQ(data[0][1], 3);
    instance.table = 2;
    sizeTable(1, 1);
    EXPECT_EQ(instance.run(), NOTOK);
    EXPECT_EQ(data[1][0], -99);
    sizeTable(1, 8);
    EXPECT_EQ(instance.run(), OK);
    EXPECT_EQ(data[1][0], 2);
    EXPECT_EQ(data[1][1], 3);
    sizeTable(1, 10);
    EXPECT_EQ(instance.run(), OK);
    EXPECT_EQ(data[1][0], 2);
    EXPECT_EQ(data[1][1], 3);
    sizeTable(1, 0); // Same table number can now refer to a shorter table.
    EXPECT_EQ(instance.run(), NOTOK);
    instance.table = 0;
    EXPECT_EQ(instance.run(), NOTOK);
    EXPECT_EQ(instance.result, 0);
}

TEST_F(JoystickTests, ValidateEventsAndBoundMaskShifts) {
    Instance instance(csound);
    axes = 35; buttons = 32;
    sizeTable(0, 69);
    event(0, JS_EVENT_INIT | JS_EVENT_AXIS, 30, -123);
    event(0, JS_EVENT_BUTTON, 31, 1);
    event(0, JS_EVENT_AXIS, 35, 999);
    event(0, JS_EVENT_BUTTON, 32, 999);
    event(0, JS_EVENT_BUTTON | JS_EVENT_AXIS, 0, 999);
    ASSERT_EQ(instance.run(), OK);
    EXPECT_EQ(data[0][32], -123);
    EXPECT_EQ(data[0][68], 1);
    EXPECT_EQ(data[0][37], -99);
    EXPECT_EQ(data[0][2], -99);
    EXPECT_EQ(instance.result, (cs_float)(UINT64_C(3) | (UINT64_C(1) << 32)));
    EXPECT_EQ(instance.run(), OK);
    EXPECT_EQ(instance.result, 0);
}

TEST_F(JoystickTests, CloseDeviceZeroOnSwitchReinitAndDeinit) {
    Instance instance(csound);
    ASSERT_EQ(instance.run(), OK);
    EXPECT_EQ(instance.opcode.devFD, 0);
    instance.device = 1;
    ASSERT_EQ(instance.run(), OK);
    EXPECT_EQ(closed, std::vector<int>({0}));
    EXPECT_EQ(linuxjoystick_init(csound, &instance.opcode), OK);
    EXPECT_EQ(closed, std::vector<int>({0, 1}));
    ASSERT_EQ(instance.run(), OK);
    EXPECT_EQ(linuxjoystick_deinit(csound, &instance.opcode), OK);
    EXPECT_EQ(linuxjoystick_deinit(csound, &instance.opcode), OK);
    EXPECT_EQ(closed, std::vector<int>({0, 1, 2}));
}

TEST_F(JoystickTests, ValidateDeviceNumberBeforeRounding) {
    Instance instance(csound);
    for (cs_float value : {cs_float(-1), cs_float(2147483648.0),
                       std::numeric_limits<cs_float>::infinity(),
                       std::numeric_limits<cs_float>::quiet_NaN()}) {
        instance.device = value;
        EXPECT_EQ(instance.run(), NOTOK);
        EXPECT_EQ(instance.result, 0);
    }
    EXPECT_EQ(openCalls, 0);
    instance.device = FL(0.25);
    EXPECT_EQ(instance.run(), OK);
    EXPECT_EQ(instance.run(), OK);
    EXPECT_EQ(openCalls, 1);
}

TEST_F(JoystickTests, RetryFailuresWithoutStaleMasksOrSharedReadState) {
    Instance first(csound), second(csound);
    failOpen = true;
    first.result = 7;
    ASSERT_EQ(first.run(), OK);
    EXPECT_EQ(first.result, 0);
    EXPECT_EQ(openCalls, 2);
    ASSERT_EQ(first.run(), OK);
    EXPECT_EQ(openCalls, 2);
    first.device = 1;
    failOpen = false; failQuery = true;
    ASSERT_EQ(first.run(), OK);
    EXPECT_TRUE(descriptors.empty());
    failQuery = false;
    first.device = 2;
    pending[1].push_back({{}, -1, EINTR});
    event(1, JS_EVENT_AXIS, 0, 42);
    ASSERT_EQ(first.run(), OK);
    EXPECT_EQ(data[0][2], 42);
    pending[1].push_back({{}, 2, 0});
    first.result = 15;
    ASSERT_EQ(first.run(), OK);
    EXPECT_EQ(first.result, 0);
    EXPECT_EQ(first.opcode.devFD, -1);
    second.table = 2;
    event(2, JS_EVENT_BUTTON, 0, 1);
    ASSERT_EQ(second.run(), OK);
    EXPECT_EQ(data[1][4], 1);
    EXPECT_EQ(second.result, 19);
}

TEST_F(JoystickTests, EngineClosesDeviceAtNoteEnd) {
    const auto &entry = localops[0];
    ASSERT_EQ(csoundAppendOpcode(csound, "test_joystick", entry.dsblksiz,
        entry.flags, entry.outypes, entry.intypes,
        entry.init, entry.perf, entry.deinit), OK);
    csoundSetOption(csound, "-n");
    ASSERT_EQ(csoundCompileCSD(csound,
        "<CsoundSynthesizer>\n<CsInstruments>\n"
        "sr=1024\nksmps=8\nnchnls=1\n"
        "instr 1\nkresult test_joystick 0, 1\nendin\n"
        "</CsInstruments>\n<CsScore>\n"
        "i 1 0 .125\ni 1 .25 .125\ne\n"
        "</CsScore>\n</CsoundSynthesizer>\n", 1, 0), OK);
    ASSERT_EQ(csoundStart(csound), OK);
    while (csoundPerformKsmps(csound) == 0) {}
    EXPECT_EQ(openCalls, 2);
    EXPECT_EQ(closed.size(), 2u);
    EXPECT_TRUE(descriptors.empty());
}
} // namespace
#endif
