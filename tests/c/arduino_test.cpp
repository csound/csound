#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <cstring>

#if !defined(WIN32) && !defined(__wasm__) && !defined(NO_SERIAL_OPCODES)
#include <fcntl.h>
#include <unistd.h>

namespace {
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(x)
#include "../../Opcodes/serial.c"

int32_t ignoreInitError(CSOUND *, const char *, ...) { return NOTOK; }
int32_t ignorePerfError(CSOUND *, OPDS *, const char *, ...) { return NOTOK; }
void *failThread(uintptr_t (*)(void *), void *) { return nullptr; }

class ArduinoTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    int master = -1;
    STRINGDAT name = {};
    cs_float port = -1, baud = 9600;
    ARD_START starter = {};

    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csound->InitError = ignoreInitError;
        csound->PerfError = ignorePerfError;
        master = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
        ASSERT_GE(master, 0);
        ASSERT_EQ(grantpt(master), 0);
        ASSERT_EQ(unlockpt(master), 0);
        name.data = ptsname(master);
        ASSERT_NE(name.data, nullptr);
        name.size = strlen(name.data) + 1;
        starter.returnedPort = &port;
        starter.portName = &name;
        starter.baudRate = &baud;
    }
    void TearDown() override {
        csoundDestroy(csound);
        if (master >= 0) close(master);
    }
    void send(std::initializer_list<unsigned char> bytes) {
        ASSERT_EQ(write(master, bytes.begin(), bytes.size()), bytes.size());
    }
    bool waitForValue(int index, int value) {
        for (int i = 0; i < 200; ++i) {
            csound->LockMutex(starter.q->lock);
            int actual = starter.q->values[index];
            csound->UnlockMutex(starter.q->lock);
            if (actual == value) return true;
            csound->Sleep(1);
        }
        return false;
    }
};

TEST_F(ArduinoTests, StopWhileSilentUnsyncedOrMidValue) {
    for (int state = 0; state < 3; ++state) {
        ASSERT_EQ(arduinoStart(csound, &starter), OK);
        int descriptor = static_cast<int>(port);
        if (state == 1) send({1, 2, 3});
        if (state == 2) send({0xf8, 4});
        csound->Sleep(5);
        auto before = std::chrono::steady_clock::now();
        EXPECT_EQ(arduinoStop(csound, &starter), OK);
        EXPECT_LT(std::chrono::steady_clock::now() - before,
                  std::chrono::seconds(1));
        EXPECT_EQ(fcntl(descriptor, F_GETFD), -1);
        EXPECT_EQ(arduinoStop(csound, &starter), OK);
        EXPECT_EQ(arduino_deinit(csound, &starter), OK);
    }
}

TEST_F(ArduinoTests, RestartKeepsOldReadersAndCleanupSeparate) {
    ASSERT_EQ(arduinoStart(csound, &starter), OK);
    cs_float value = 0, index = 0, halfTime = 0;
    ARD_READ reader = {};
    reader.val = &value;
    reader.port = &port;
    reader.index = &index;
    reader.ihtim = &halfTime;
    ASSERT_EQ(arduinoReadSetup(csound, &reader), OK);
    send({0xf8, 123, 0});
    ASSERT_TRUE(waitForValue(0, 123));
    EXPECT_EQ(arduinoRead(csound, &reader), OK);
    EXPECT_EQ(value, 123);
    ARD_START oldOwner = starter;
    EXPECT_EQ(arduinoStop(csound, &starter), OK);
    EXPECT_EQ(arduinoRead(csound, &reader), NOTOK);
    ASSERT_EQ(arduinoStart(csound, &starter), OK);
    EXPECT_EQ(arduinoRead(csound, &reader), NOTOK);
    EXPECT_EQ(arduino_deinit(csound, &oldOwner), OK);
    EXPECT_NE(starter.q->thread, nullptr);
    EXPECT_EQ(arduinoReadSetup(csound, &reader), OK);
    send({0xf8, 42, 0});
    ASSERT_TRUE(waitForValue(0, 42));
    EXPECT_EQ(arduinoRead(csound, &reader), OK);
    EXPECT_EQ(value, 42);
    EXPECT_EQ(arduino_deinit(csound, &starter), OK);
    EXPECT_EQ(arduinoRead(csound, &reader), NOTOK);
}

TEST_F(ArduinoTests, FailedStartAndDuplicateStartLeaveNoExtraSession) {
    auto createThread = csound->CreateThread;
    csound->CreateThread = failThread;
    EXPECT_EQ(arduinoStart(csound, &starter), NOTOK);
    auto *state = static_cast<ARDUINO_GLOBALS *>(
        csound->QueryGlobalVariable(csound, "arduinoGlobals_"));
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->thread, nullptr);
    EXPECT_EQ(fcntl(state->port, F_GETFD), -1);
    EXPECT_EQ(arduino_deinit(csound, &starter), OK);
    csound->CreateThread = createThread;
    ASSERT_EQ(arduinoStart(csound, &starter), OK);
    ARD_START duplicate = {};
    cs_float duplicatePort = -1;
    duplicate.returnedPort = &duplicatePort;
    duplicate.portName = &name;
    duplicate.baudRate = &baud;
    EXPECT_EQ(arduinoStart(csound, &duplicate), NOTOK);
    EXPECT_EQ(duplicatePort, -1);
    EXPECT_EQ(arduino_deinit(csound, &duplicate), OK);
    EXPECT_NE(starter.q->thread, nullptr);
}

TEST_F(ArduinoTests, SensorBoundsFloatBitsAndReset) {
    ASSERT_EQ(arduinoStart(csound, &starter), OK);
    // Ignore unsupported sensor slots, then decode a valid pair.
    send({0xf8, 1, 240, 2, 249, 127, 239});
    ASSERT_TRUE(waitForValue(29, 1023));
    cs_float value, index = 30, halfTime = 0;
    ARD_READ reader = {};
    reader.val = &value;
    reader.port = &port;
    reader.index = &index;
    reader.ihtim = &halfTime;
    ASSERT_EQ(arduinoReadSetup(csound, &reader), OK);
    EXPECT_EQ(arduinoRead(csound, &reader), NOTOK);
    index = 29;
    EXPECT_EQ(arduinoRead(csound, &reader), OK);
    EXPECT_EQ(value, 1023);
    cs_float first = 0, second = 1, third = 2;
    ARD_READF floatReader = {};
    floatReader.val = &value;
    floatReader.port = &port;
    floatReader.index1 = &first;
    floatReader.index2 = &second;
    floatReader.index3 = &third;
    ASSERT_EQ(arduinoReadFSetup(csound, &floatReader), OK);
    // Three ten-bit pieces of -1.0f (0xbf800000), low two bits omitted.
    send({0xf8, 0, 0, 0, 8, 126, 21});
    ASSERT_TRUE(waitForValue(2, 766));
    EXPECT_EQ(arduinoReadF(csound, &floatReader), OK);
    EXPECT_EQ(value, -1);
    third = 30;
    EXPECT_EQ(arduinoReadF(csound, &floatReader), NOTOK);
    int descriptor = static_cast<int>(port);
    csoundReset(csound);
    EXPECT_EQ(fcntl(descriptor, F_GETFD), -1);
}
} // namespace
#endif
