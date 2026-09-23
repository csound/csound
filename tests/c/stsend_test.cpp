#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstring>
#include <vector>
#ifdef HAVE_SOCKETS
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
using TestSocket = SOCKET;
using AddressLength = int;
#else
#include <sys/socket.h>
using TestSocket = int;
using AddressLength = socklen_t;
#endif

static std::vector<unsigned char> sentBytes;
static TestSocket openedSocket;
static int connects, sends;
static bool refuseConnection, interruptSend, zeroSend;

extern "C" int csound_test_stsend_connect(TestSocket socket,
                                           const struct sockaddr *, AddressLength)
{
    openedSocket = socket;
    ++connects;
    if (!refuseConnection) return 0;
    // Stop the old retry loop after two attempts rather than hanging the test.
#if defined(WIN32) && !defined(__CYGWIN__)
    WSASetLastError(connects == 1 ? WSAECONNREFUSED : WSAEINVAL);
#else
    errno = connects == 1 ? ECONNREFUSED : EINVAL;
#endif
    return -1;
}

#if defined(WIN32) && !defined(__CYGWIN__)
extern "C" int csound_test_stsend_send(SOCKET, const char *data, int size, int)
#else
extern "C" ssize_t csound_test_stsend_send(int, const void *data, size_t size, int)
#endif
{
    ++sends;
    if (interruptSend) {
      interruptSend = false;
#if defined(WIN32) && !defined(__CYGWIN__)
      WSASetLastError(WSAEINTR);
#else
      errno = EINTR;
#endif
      return -1;
    }
    if (zeroSend) return 0;
    size_t n = std::min<size_t>(size, 3);
    const auto *bytes = reinterpret_cast<const unsigned char *>(data);
    sentBytes.insert(sentBytes.end(), bytes, bytes + n);
    return (int)n;
}

extern "C" OENTRY *csound_test_stsend_opcode(void);

namespace {
int32_t initError(CSOUND *, const char *, ...) { return NOTOK; }
int32_t perfError(CSOUND *, OPDS *, const char *, ...) { return NOTOK; }
struct SenderArgs {
    OPDS h;
    MYFLT *input;
    STRINGDAT *host;
    MYFLT *port;
};
class StsendTests : public ::testing::Test {
protected:
    CSOUND *cs;
    OENTRY *opcode;
    SenderArgs *sender;
    INSDS instance = {};
    STRINGDAT host = {};
    MYFLT port = 9000;
    void SetUp() override {
      cs = csoundCreate(nullptr, nullptr);
      csoundCreateMessageBuffer(cs, 0);
      cs->InitError = initError;
      cs->PerfError = perfError;
      opcode = csound_test_stsend_opcode();
      ASSERT_NE(opcode, nullptr);
      sender = (SenderArgs *)calloc(1, opcode->dsblksiz);
      sender->h.insdshead = &instance;
      host.data = (char *)"127.0.0.1";
      sender->host = &host;
      sender->port = &port;
      sentBytes.clear();
      connects = sends = 0;
      refuseConnection = interruptSend = zeroSend = false;
    }
    void TearDown() override {
      opcode->deinit(cs, sender);
      free(sender);
      csoundDestroy(cs);
    }
    bool closed() {
      struct sockaddr address;
      AddressLength length = sizeof(address);
      return getsockname(openedSocket, &address, &length) == -1;
    }
};

TEST_F(StsendTests, SendsEveryByteOfActiveSamplesAfterShortAndInterruptedWrites)
{
    MYFLT input[] = {99, 1, 2, 3, 4, 99};
    instance.ksmps = 6;
    instance.ksmps_offset = instance.ksmps_no_end = 1;
    sender->input = input;
    ASSERT_EQ(opcode->init(cs, sender), OK);
    interruptSend = true;
    EXPECT_EQ(opcode->perf(cs, sender), OK);
    const auto *first = reinterpret_cast<const unsigned char *>(&input[1]);
    EXPECT_EQ(sentBytes, (std::vector<unsigned char>(first, first + 4*sizeof(MYFLT))));
    EXPECT_GT(sends, 2);
    EXPECT_EQ(opcode->deinit(cs, sender), OK);
    EXPECT_TRUE(closed());
}

TEST_F(StsendTests, RefusedConnectionsAndZeroWritesStopAndCloseTheSocket)
{
    refuseConnection = true;
    EXPECT_EQ(opcode->init(cs, sender), NOTOK);
    EXPECT_EQ(connects, 1);
    EXPECT_TRUE(closed());
    refuseConnection = false;
    ASSERT_EQ(opcode->init(cs, sender), OK);
    MYFLT input = 1;
    instance.ksmps = 1;
    sender->input = &input;
    zeroSend = true;
    EXPECT_EQ(opcode->perf(cs, sender), NOTOK);
    EXPECT_EQ(sends, 1);
    EXPECT_TRUE(closed());
    EXPECT_EQ(opcode->deinit(cs, sender), OK);
}
}
#endif
