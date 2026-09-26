#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstring>
#include <string>

#if defined(_WIN32) || defined(CSOUND_SERIAL_WINDOWS_TEST)
#include <winsock2.h>
#else
#include <unistd.h>
#include <fcntl.h>
#ifndef __wasm__
#include <poll.h>
#endif
#endif

#ifndef NO_SERIAL_OPCODES
namespace {
#if defined(_WIN32) || defined(CSOUND_SERIAL_WINDOWS_TEST)
// Exercise the Windows backend without requiring a physical serial device.
int opened, closed;
bool setupFails, readFails, writeFails, shortWrite;
std::string received, sent;
COMMTIMEOUTS configuredTimeouts;

HANDLE WINAPI testCreateFileA(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES,
                             DWORD, DWORD, HANDLE) {
    return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(++opened));
}
BOOL WINAPI testCloseHandle(HANDLE) { ++closed; return TRUE; }
BOOL WINAPI testGetCommState(HANDLE, LPDCB state) {
    state->XonChar = 17;
    state->XoffChar = 19;
    return TRUE;
}
BOOL WINAPI testSetCommState(HANDLE, LPDCB state) {
    return !setupFails && state->fBinary && state->XonChar != state->XoffChar;
}
BOOL WINAPI testSetCommTimeouts(HANDLE, LPCOMMTIMEOUTS value) {
    configuredTimeouts = *value;
    return TRUE;
}
BOOL WINAPI testReadFile(HANDLE, LPVOID output, DWORD length, LPDWORD count,
                         LPOVERLAPPED) {
    if (readFails) return FALSE;
    *count = static_cast<DWORD>(std::min<size_t>(length, received.size()));
    memcpy(output, received.data(), *count);
    received.erase(0, *count);
    return TRUE;
}
BOOL WINAPI testWriteFile(HANDLE, LPCVOID input, DWORD length, LPDWORD count,
                          LPOVERLAPPED) {
    if (writeFails) return FALSE;
    *count = length - (shortWrite && length > 0 ? 1 : 0);
    sent.append(static_cast<const char *>(input), *count);
    return TRUE;
}
BOOL WINAPI testPurgeComm(HANDLE, DWORD flags) {
    if (flags != PURGE_RXCLEAR) return FALSE;
    received.clear();
    return TRUE;
}

#ifndef WIN32
#define WIN32
#endif
#define CreateFileA testCreateFileA
#define CloseHandle testCloseHandle
#define GetCommState testGetCommState
#define SetCommState testSetCommState
#define SetCommTimeouts testSetCommTimeouts
#define ReadFile testReadFile
#define WriteFile testWriteFile
#define PurgeComm testPurgeComm
#endif

// Keep this copy private; the library supplies the registered opcodes.
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(x)
#include "../../Opcodes/serial.c"

int32_t ignoreInitError(CSOUND *, const char *, ...) { return NOTOK; }
int32_t ignorePerfError(CSOUND *, OPDS *, const char *, ...) { return NOTOK; }

class SerialTests : public ::testing::Test {
protected:
    CSOUND *csound;
    void SetUp() override {
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csound->InitError = ignoreInitError;
        csound->PerfError = ignorePerfError;
#ifdef WIN32
        opened = closed = 0;
        setupFails = readFails = writeFails = shortWrite = false;
        received.clear();
        sent.clear();
#endif
    }
    void TearDown() override { csoundDestroy(csound); }
};

#ifdef WIN32
TEST_F(SerialTests, CloseReuseAndFailedSetup) {
    SERIALEND end = {};
    cs_float port = 0;
    end.port = &port;
    EXPECT_EQ(serialEnd(csound, &end), NOTOK);
    for (int i = 0; i < 10; ++i)
        ASSERT_EQ(serialport_init(csound, "COM1", 9600), i);
    EXPECT_EQ(configuredTimeouts.ReadIntervalTimeout, MAXDWORD);
    EXPECT_EQ(configuredTimeouts.ReadTotalTimeoutConstant, 0u);
    EXPECT_EQ(configuredTimeouts.ReadTotalTimeoutMultiplier, 0u);
    EXPECT_LT(serialport_init(csound, "COM1", 9600), 0);
    EXPECT_EQ(closed, 1);
    port = 3;
    EXPECT_EQ(serialEnd(csound, &end), OK);
    EXPECT_EQ(serialEnd(csound, &end), NOTOK);
    EXPECT_EQ(serialport_init(csound, "COM1", 9600), 3);
    EXPECT_EQ(closed, 2);
    port = 10;
    EXPECT_EQ(serialEnd(csound, &end), NOTOK);
    port = -1;
    EXPECT_EQ(serialEnd(csound, &end), NOTOK);
    port = 3;
    ASSERT_EQ(serialEnd(csound, &end), OK);
    setupFails = true;
    EXPECT_LT(serialport_init(csound, "COM1", 9600), 0);
    EXPECT_EQ(closed, 4);
    setupFails = false;
    EXPECT_EQ(serialport_init(csound, "COM1", 9600), 3);
}

TEST_F(SerialTests, ReadPrintFlushAndWriteErrors) {
    cs_float port = serialport_init(csound, "COM1", 9600), value = 42;
    ASSERT_EQ(port, 0);
    SERIALREAD reader = {};
    reader.port = &port;
    reader.rChar = &value;
    EXPECT_EQ(serialRead(csound, &reader), OK);
    EXPECT_EQ(value, -1);
    received = std::string(1, '\xff');
    EXPECT_EQ(serialRead(csound, &reader), OK);
    EXPECT_EQ(value, 255);
    readFails = true;
    EXPECT_EQ(serialRead(csound, &reader), NOTOK);
    EXPECT_EQ(value, -1);
    SERIALPRINT printer = {};
    printer.port = &port;
    EXPECT_EQ(serialPrint(csound, &printer), NOTOK);
    readFails = false;
    received = std::string(32768, 'x');
    EXPECT_EQ(serialPrint(csound, &printer), OK);
    EXPECT_TRUE(received.empty());
    SERIALFLUSH flush = {};
    flush.port = &port;
    received = "pending";
    EXPECT_EQ(serialFlush(csound, &flush), OK);
    EXPECT_TRUE(received.empty());
    SERIALWRITE writer = {};
    writer.port = &port;
    writer.toWrite = &value;
    value = 255;
    EXPECT_EQ(serialWrite(csound, &writer), OK);
    EXPECT_EQ(sent, std::string(1, '\xff'));
    shortWrite = true;
    EXPECT_EQ(serialWrite(csound, &writer), NOTOK);
    writeFails = true;
    EXPECT_EQ(serialWrite(csound, &writer), NOTOK);
}
#endif

#if !defined(WIN32) && !defined(__wasm__)
TEST_F(SerialTests, RawInputPreservesBytes) {
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    ASSERT_GE(master, 0);
    ASSERT_EQ(grantpt(master), 0);
    ASSERT_EQ(unlockpt(master), 0);
    const char *path = ptsname(master);
    ASSERT_NE(path, nullptr);
    int slave = open(path, O_RDWR | O_NOCTTY);
    ASSERT_GE(slave, 0);
    struct termios state;
    ASSERT_EQ(tcgetattr(slave, &state), 0);
    state.c_iflag |= ICRNL | ISTRIP | PARMRK | IXON | IXOFF;
    state.c_lflag |= ECHONL | IEXTEN;
    ASSERT_EQ(tcsetattr(slave, TCSANOW, &state), 0);

    int fd = serialport_init(csound, path, 9600);
    ASSERT_GE(fd, 0);
    ASSERT_EQ(tcgetattr(fd, &state), 0);
    EXPECT_EQ(state.c_iflag & (IGNBRK | BRKINT | PARMRK | INPCK | ISTRIP |
                              INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY), 0u);
    EXPECT_EQ(state.c_lflag & (ICANON | ECHO | ECHONL | ISIG | IEXTEN), 0u);

    const unsigned char bytes[] = {0, 13, 10, 17, 19, 127, 128, 255};
    ASSERT_EQ(write(master, bytes, sizeof(bytes)), sizeof(bytes));
    struct pollfd ready = {fd, POLLIN, 0};
    ASSERT_EQ(poll(&ready, 1, 1000), 1);
    cs_float port = fd, value;
    SERIALREAD reader = {};
    reader.port = &port;
    reader.rChar = &value;
    for (unsigned char byte : bytes) {
        EXPECT_EQ(serialRead(csound, &reader), OK);
        EXPECT_EQ(value, byte);
    }
    EXPECT_EQ(serialRead(csound, &reader), OK);
    EXPECT_EQ(value, -1);
    close(fd);
    close(slave);
    close(master);
}
#endif

TEST_F(SerialTests, StringWritesUseContentLength) {
    char storage[64];
    memset(storage, 'x', sizeof(storage));
    memcpy(storage, "abc", 4);
    STRINGDAT str = {};
    str.data = storage;
    str.size = sizeof(storage);
    SERIALWRITE writer = {};
#ifdef WIN32
    cs_float port = serialport_init(csound, "COM1", 9600);
#else
    int descriptors[2];
    ASSERT_EQ(pipe(descriptors), 0);
    cs_float port = descriptors[1];
#endif
    writer.port = &port;
    writer.toWrite = reinterpret_cast<cs_float *>(&str);
    EXPECT_EQ(serialWrite_S(csound, &writer), OK);
#ifdef WIN32
    EXPECT_EQ(sent, "abc");
    shortWrite = true;
    EXPECT_EQ(serialWrite_S(csound, &writer), NOTOK);
    writeFails = true;
    EXPECT_EQ(serialWrite_S(csound, &writer), NOTOK);
#else
    char output[64];
    EXPECT_EQ(read(descriptors[0], output, sizeof(output)), 3);
    EXPECT_EQ(std::string(output, 3), "abc");
    close(descriptors[0]);
    close(descriptors[1]);
#endif
}
} // namespace

#endif // NO_SERIAL_OPCODES
