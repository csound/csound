#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstring>
#include <vector>
#ifdef HAVE_SOCKETS
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

static std::vector<unsigned char> incoming;
static size_t readPosition;
static bool interruptRead, failAccept;
#if defined(WIN32) && !defined(__CYGWIN__)
using TestSocket = SOCKET;
#else
using TestSocket = int;
#endif
static TestSocket listener, connection;

static void interruptError()
{
#if defined(WIN32) && !defined(__CYGWIN__)
    WSASetLastError(WSAEINTR);
#else
    errno = EINTR;
#endif
}
extern "C" TestSocket csound_test_strecv_accept(TestSocket sock,
                                               struct sockaddr *, socklen_t *)
{
    listener = sock;
    if (failAccept) {
      interruptError();
      return (TestSocket)-1;
    }
    connection = socket(AF_INET, SOCK_STREAM, 0);
    return connection;
}
extern "C" int csound_test_strecv_recv(TestSocket, void *buffer, size_t size, int)
{
    if (interruptRead) {
      interruptRead = false;
      interruptError();
      return -1;
    }
    size_t n = (std::min)({size, incoming.size() - readPosition, size_t(3)});
    if (n) memcpy(buffer, incoming.data() + readPosition, n);
    readPosition += n;
    return (int)n;
}
extern "C" OENTRY *csound_test_strecv_opcode(void);

namespace {
int32_t ignoreInitError(CSOUND *, const char *, ...) { return NOTOK; }
// The opcode's public arguments; storage for its private state comes from OENTRY.
struct ReceiverArgs {
    OPDS h;
    MYFLT *asig, *res;
    STRINGDAT *ipaddress;
    MYFLT *port;
};
class StrecvTests : public ::testing::Test {
protected:
    CSOUND *cs;
    OENTRY *opcode;
    ReceiverArgs *receiver;
    INSDS instance = {};
    STRINGDAT address = {};
    MYFLT port = 0, state = 0;
    MYFLT output[8];
    void SetUp() override {
      cs = csoundCreate(nullptr, nullptr);
      csoundCreateMessageBuffer(cs, 0);
      cs->InitError = ignoreInitError;
      opcode = csound_test_strecv_opcode();
      receiver = (ReceiverArgs *)calloc(1, opcode->dsblksiz);
      instance.ksmps = 8;
      address.data = (char *)"127.0.0.1";
      receiver->h.insdshead = &instance;
      receiver->ipaddress = &address;
      receiver->port = &port;
      receiver->res = &state;
      receiver->asig = output;
      readPosition = 0;
      interruptRead = failAccept = false;
      incoming.clear();
    }
    void TearDown() override {
      opcode->deinit(cs, receiver);
      free(receiver);
      csoundDestroy(cs);
    }
    bool isClosed(TestSocket sock) {
      struct sockaddr address;
      socklen_t length = sizeof(address);
      return getsockname(sock, &address, &length) == -1;
    }
};

TEST_F(StrecvTests, ReassemblesByteFragmentsWithinActiveSamples)
{
    const MYFLT samples[] = {1, 2, 3, 4, 5, 6};
    const auto *bytes = reinterpret_cast<const unsigned char *>(samples);
    incoming.assign(bytes, bytes + sizeof(samples));
    instance.ksmps_offset = instance.ksmps_no_end = 1;
    ASSERT_EQ(opcode->init(cs, receiver), OK);
    interruptRead = true;
    ASSERT_EQ(opcode->perf(cs, receiver), OK);
    EXPECT_EQ(state, sizeof(samples));
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[7], 0);
    for (int i = 0; i < 6; ++i) EXPECT_EQ(output[i + 1], samples[i]);
    EXPECT_EQ(readPosition, incoming.size());
    EXPECT_EQ(opcode->deinit(cs, receiver), OK);
    EXPECT_TRUE(isClosed(listener));
    EXPECT_TRUE(isClosed(connection));
}

TEST_F(StrecvTests, ClearsOutputAfterEofAndClosesOnInitFailure)
{
    const MYFLT samples[] = {1, 2};
    const auto *bytes = reinterpret_cast<const unsigned char *>(samples);
    incoming.assign(bytes, bytes + sizeof(MYFLT) + 3);
    ASSERT_EQ(opcode->init(cs, receiver), OK);
    EXPECT_EQ(opcode->perf(cs, receiver), OK);
    EXPECT_EQ(state, -1);
    EXPECT_EQ(output[0], 1);
    for (int i = 1; i < 8; ++i) EXPECT_EQ(output[i], 0);
    EXPECT_TRUE(isClosed(listener));
    EXPECT_TRUE(isClosed(connection));
    EXPECT_EQ(opcode->perf(cs, receiver), OK);
    for (MYFLT sample : output) EXPECT_EQ(sample, 0);
    EXPECT_EQ(state, -1);
    failAccept = true;
    EXPECT_EQ(opcode->init(cs, receiver), NOTOK);
    EXPECT_TRUE(isClosed(listener));
    EXPECT_EQ(opcode->deinit(cs, receiver), OK);
}
} // namespace
#endif
