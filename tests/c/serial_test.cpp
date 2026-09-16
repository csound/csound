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
    MYFLT port = 0;
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
    MYFLT port = serialport_init(csound, "COM1", 9600), value = 42;
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

TEST_F(SerialTests, StringWritesUseContentLength) {
    char storage[64];
    memset(storage, 'x', sizeof(storage));
    memcpy(storage, "abc", 4);
    STRINGDAT str = {};
    str.data = storage;
    str.size = sizeof(storage);
    SERIALWRITE writer = {};
#ifdef WIN32
    MYFLT port = serialport_init(csound, "COM1", 9600);
#else
    int descriptors[2];
    ASSERT_EQ(pipe(descriptors), 0);
    MYFLT port = descriptors[1];
#endif
    writer.port = &port;
    writer.toWrite = reinterpret_cast<MYFLT *>(&str);
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
