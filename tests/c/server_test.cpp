#include <stdio.h>
#include <array>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#if defined(WIN32) && !defined(__CYGWIN__)
# include <winsock2.h>
# include <ws2tcpip.h>
#else
# include <fcntl.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif
#if defined (WIN32) && !defined(__MINGW32__)
# include <Windows.h>
#else
# include "unistd.h"
#endif
#if defined(CSOUND_TEST_OSC_PLUGIN) && defined(__MINGW32__)
# include <windows.h>
#endif
#if defined(CSOUND_TEST_OSC_PLUGIN) && !(defined(WIN32) && !defined(__CYGWIN__))
# include <dlfcn.h>
#endif

#include "csound.hpp"
#include "csPerfThread.hpp"

void udp_send(const char* msg) {
    struct sockaddr_in server_addr;
    int32_t sock;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = { 0 };
    int32_t err;
    if (UNLIKELY((err = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0))
        return;
#endif
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(sock < 0)) {
        return;
    }
#ifndef WIN32
    if (UNLIKELY(fcntl(sock, F_SETFL, O_NONBLOCK) < 0)) {
        close(sock);
        return;
    }
#else
    {
        u_long argp = 1;
        err = ioctlsocket(sock, FIONBIO, &argp);
        if (UNLIKELY(err != NO_ERROR)) {
            closesocket(sock);
            return;
        }
    }
#endif
    server_addr.sin_family = AF_INET;
#if defined(WIN32) && !defined(__CYGWIN__)
    server_addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
    inet_aton("127.0.0.1", &server_addr.sin_addr);
#endif
    server_addr.sin_port = htons((int)44100);
    sendto(sock, msg, strlen(msg) + 1, 0,
        (const struct sockaddr*)&server_addr,
        sizeof(server_addr));
}

#ifdef CSOUND_TEST_OSC_PLUGIN_DIR
namespace {

int32_t findFreeUdpPort()
{
    struct sockaddr_in address = {};
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = { 0 };
    SOCKET sock;
    int32_t addressSize = sizeof(address);
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
      return -1;
#else
    int32_t sock;
    socklen_t addressSize = sizeof(address);
#endif

    sock = socket(AF_INET, SOCK_DGRAM, 0);
#if defined(WIN32) && !defined(__CYGWIN__)
    if (sock == INVALID_SOCKET) {
      WSACleanup();
      return -1;
    }
#else
    if (sock < 0)
      return -1;
#endif
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    if (bind(sock, (const struct sockaddr*)&address, sizeof(address)) < 0 ||
        getsockname(sock, (struct sockaddr*)&address, &addressSize) < 0) {
#if defined(WIN32) && !defined(__CYGWIN__)
      closesocket(sock);
      WSACleanup();
#else
      close(sock);
#endif
      return -1;
    }
#if defined(WIN32) && !defined(__CYGWIN__)
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return ntohs(address.sin_port);
}

bool performBlocks(CSOUND *csound, int32_t count)
{
    for (int32_t i = 0; i < count; ++i) {
      if (csoundPerformKsmps(csound) != CSOUND_SUCCESS)
        return false;
      csoundSleep(1);
    }
    return true;
}

bool waitForChannel(CSOUND *csound, const char *name, MYFLT expected)
{
    for (int32_t i = 0; i < 1000; ++i) {
      if (csoundGetControlChannel(csound, name, NULL) == expected)
        return true;
      if (!performBlocks(csound, 1))
        return false;
    }
    return false;
}

/* Each csound instance dlopens and dlcloses the OSC plugin. Under
   AddressSanitizer, unloading an instrumented dylib leaves stale shadow
   behind, and the next dlopen of the same image can crash inside dyld's
   load notification. Pin the plugin once so it stays mapped for the whole
   test process. */
bool pinOscPlugin()
{
#if defined(WIN32) && !defined(__CYGWIN__)
    return LoadLibraryA(CSOUND_TEST_OSC_PLUGIN) != NULL;
#else
    return dlopen(CSOUND_TEST_OSC_PLUGIN, RTLD_NOW | RTLD_NODELETE) != NULL;
#endif
}

std::string readMessages(CSOUND *csound)
{
    std::string messages;
    while (csoundGetMessageCnt(csound) > 0) {
      const char *message = csoundGetFirstMessage(csound);
      if (message != NULL)
        messages += message;
      csoundPopFirstMessage(csound);
    }
    return messages;
}

}
#endif

static uint16_t unused_udp_port()
{
    struct sockaddr_in server_addr = {};
#if defined(WIN32) && !defined(__CYGWIN__)
    SOCKET sock;
#else
    int32_t sock;
#endif
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = { 0 };
    int32_t err;
    if ((err = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
        return 0;
#endif
    sock = socket(AF_INET, SOCK_DGRAM, 0);
#if defined(WIN32) && !defined(__CYGWIN__)
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return 0;
    }
#else
    if (sock < 0)
        return 0;
#endif
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = 0;
    if (bind(sock, (const struct sockaddr*) &server_addr,
             sizeof(server_addr)) != 0) {
#if defined(WIN32) && !defined(__CYGWIN__)
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 0;
    }
#if defined(WIN32) && !defined(__CYGWIN__)
    int address_size = sizeof(server_addr);
#else
    socklen_t address_size = sizeof(server_addr);
#endif
    if (getsockname(sock, (struct sockaddr*) &server_addr,
                    &address_size) != 0) {
        server_addr.sin_port = 0;
    }
#if defined(WIN32) && !defined(__CYGWIN__)
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return ntohs(server_addr.sin_port);
}

static bool udp_send_bytes(const void *data, size_t size, uint16_t port)
{
    struct sockaddr_in server_addr = {};
#if defined(WIN32) && !defined(__CYGWIN__)
    SOCKET sock;
#else
    int32_t sock;
#endif
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = { 0 };
    int32_t err;
    if ((err = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
        return false;
#endif
    sock = socket(AF_INET, SOCK_DGRAM, 0);
#if defined(WIN32) && !defined(__CYGWIN__)
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }
#else
    if (sock < 0)
        return false;
#endif
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = htons(port);
    bool sent = sendto(sock, (const char*) data, (int32_t) size, 0,
                       (const struct sockaddr*) &server_addr,
                       sizeof(server_addr)) == (int32_t) size;
#if defined(WIN32) && !defined(__CYGWIN__)
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return sent;
}

#ifdef CSOUND_TEST_OSC_PLUGIN_DIR
static bool udp_send_osc_int(uint16_t port, const char *path, int32_t value)
{
    std::vector<char> packet;
    auto appendOscString = [&packet](const char *text) {
      packet.insert(packet.end(), text, text + strlen(text) + 1);
      while ((packet.size() & 3U) != 0U)
        packet.push_back('\0');
    };

    appendOscString(path);
    appendOscString(",i");
    const uint32_t networkValue = htonl(static_cast<uint32_t>(value));
    const char *bytes = reinterpret_cast<const char*>(&networkValue);
    packet.insert(packet.end(), bytes, bytes + sizeof(networkValue));
    return udp_send_bytes(packet.data(), packet.size(), port);
}
#endif

class ServerTests : public ::testing::Test {
public:
    ServerTests ()
    {
    }

    virtual ~ServerTests ()
    {
    }

    virtual void SetUp ()
    {
      /* keep csound's signal handler from masking crashes in tests */
      csoundInitialize(CSOUNDINIT_NO_SIGNAL_HANDLER | CSOUNDINIT_NO_ATEXIT);
#ifdef CSOUND_TEST_OSC_PLUGIN_DIR
      static const bool oscPluginPinned = pinOscPlugin();
      ASSERT_TRUE(oscPluginPinned);
      ASSERT_EQ(csoundSetGlobalEnv(CSOUND_TEST_OSC_PLUGIN_ENV,
                                  CSOUND_TEST_OSC_PLUGIN_DIR),
                CSOUND_SUCCESS);
#endif
      csound = csoundCreate (NULL,NULL);
      csoundCreateMessageBuffer (csound, 0);
      //csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }


    CSOUND* csound {nullptr};
};

TEST_F (ServerTests, testServer) {
    const char  *instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    Csound csound;
    csound.SetOption("-odac");
    csound.SetOption("--port=44100");
    csound.Start();

    CsoundPerformanceThread performanceThread(csound.GetCsound());
    performanceThread.Play();

    udp_send(instrument);
    udp_send("$i1 0 2 1000 1000");

    csoundSleep(3000);

    udp_send("##close##");

    performanceThread.Join();
    csound.Reset();
}

#ifdef CSOUND_TEST_OSC_PLUGIN_DIR
TEST_F(ServerTests, OscListenersKeepStablePortAddresses)
{
    const int32_t listenerPort = findFreeUdpPort();
    ASSERT_GT(listenerPort, 0);
    int32_t addedPort = findFreeUdpPort();
    for (int32_t i = 0; i < 10 && addedPort == listenerPort; ++i)
      addedPort = findFreeUdpPort();
    ASSERT_GT(addedPort, 0);
    ASSERT_NE(addedPort, listenerPort);

    const std::string orchestra =
      "sr = 48000\n"
      "ksmps = 32\n"
      "nchnls = 1\n"
      "0dbfs = 1\n"
      "chn_k \"received\", 3\n"
      "gihListener oscinit " + std::to_string(listenerPort) + "\n"
      "instr 1\n"
      "  kvalue init 0\n"
      "  kreceived osclisten gihListener, \"/stable\", \"f\", kvalue\n"
      "  if kreceived == 1 then\n"
      "    chnset kvalue, \"received\"\n"
      "  endif\n"
      "endin\n"
      "instr 2\n"
      "  ihandle oscinit " + std::to_string(addedPort) + "\n"
      "endin\n"
      "instr 3\n"
      "  ktrigger init 1\n"
      "  oscsendlo ktrigger, \"127.0.0.1\", " +
        std::to_string(listenerPort) + ", \"/stable\", \"f\", p4\n"
      "endin\n";

    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-d"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 60", 0);
    ASSERT_TRUE(performBlocks(csound, 4));

    csoundEventString(csound, "i 3 0 0.01 1", 0);
    ASSERT_TRUE(waitForChannel(csound, "received", static_cast<MYFLT>(1.0)));

    for (int32_t i = 0; i < 8; ++i) {
      csoundEventString(csound, "i 2 0 0.002", 0);
      ASSERT_TRUE(performBlocks(csound, 4));
    }

    csoundEventString(csound, "i 3 0 0.01 2", 0);
    EXPECT_TRUE(waitForChannel(csound, "received", static_cast<MYFLT>(2.0)));

    EXPECT_EQ(readMessages(csound).find("deinit error"), std::string::npos);
}

TEST_F(ServerTests, OscListenerTeardownWhilePacketsArrive)
{
    const int32_t listenerPort = findFreeUdpPort();
    ASSERT_GT(listenerPort, 0);
    int32_t teardownPort = findFreeUdpPort();
    for (int32_t i = 0; i < 10 && teardownPort == listenerPort; ++i)
      teardownPort = findFreeUdpPort();
    ASSERT_GT(teardownPort, 0);
    ASSERT_NE(teardownPort, listenerPort);

    const std::string orchestra =
      "sr = 48000\n"
      "ksmps = 32\n"
      "nchnls = 1\n"
      "0dbfs = 1\n"
      "chn_k \"race_received\", 3\n"
      "chn_k \"teardown_received\", 3\n"
      "gihListener oscinit " + std::to_string(listenerPort) + "\n"
      "instr 1\n"
      "  kvalue init 0\n"
      "  kreceived osclisten gihListener, \"/race\", \"i\", kvalue\n"
      "  if kreceived == 1 then\n"
      "    chnset kvalue, \"race_received\"\n"
      "  endif\n"
      "endin\n"
      "instr 2\n"
      "  kvalue init 0\n"
      "  kvalue += 1\n"
      "  oscsendlo kvalue, \"127.0.0.1\", " +
        std::to_string(listenerPort) + ", \"/race\", \"i\", kvalue\n"
      "endin\n"
      "instr 3\n"
      "  ihandle oscinit " + std::to_string(teardownPort) + "\n"
      "  kvalue init 0\n"
      "  kreceived osclisten ihandle, \"/teardown\", \"i\", kvalue\n"
      "  if kreceived == 1 then\n"
      "    chnset kvalue, \"teardown_received\"\n"
      "  endif\n"
      "endin\n"
      "instr 4\n"
      "  kvalue init 0\n"
      "  kvalue += 1\n"
      "  oscsendlo kvalue, \"127.0.0.1\", " +
        std::to_string(teardownPort) + ", \"/teardown\", \"i\", kvalue\n"
      "endin\n";

    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-d"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    csoundEventString(csound, "i 2 0 2", 0);
    csoundEventString(csound, "i 4 0 2", 0);

    for (int32_t i = 0; i < 8; ++i) {
      csoundEventString(csound, "i 1 0 0.003", 0);
      ASSERT_TRUE(performBlocks(csound, 8));
    }
    EXPECT_GT(csoundGetControlChannel(csound, "race_received", NULL),
              static_cast<MYFLT>(0.0));

    for (int32_t i = 0; i < 4; ++i) {
      csoundEventString(csound, "i 3 0 0.003", 0);
      ASSERT_TRUE(performBlocks(csound, 8));
    }

    EXPECT_GT(csoundGetControlChannel(csound, "teardown_received", NULL),
              static_cast<MYFLT>(0.0));
    EXPECT_EQ(readMessages(csound).find("deinit error"), std::string::npos);
}

TEST_F(ServerTests, OscArrayListenerKeepsStablePortAddress)
{
    const int32_t listenerPort = findFreeUdpPort();
    ASSERT_GT(listenerPort, 0);
    int32_t addedPort = findFreeUdpPort();
    for (int32_t i = 0; i < 10 && addedPort == listenerPort; ++i)
      addedPort = findFreeUdpPort();
    ASSERT_GT(addedPort, 0);
    ASSERT_NE(addedPort, listenerPort);

    const std::string orchestra =
      "sr = 48000\n"
      "ksmps = 32\n"
      "nchnls = 1\n"
      "0dbfs = 1\n"
      "chn_k \"array_int\", 3\n"
      "chn_k \"array_float\", 3\n"
      "gihListener oscinit " + std::to_string(listenerPort) + "\n"
      "instr 1\n"
      "  kreceived, kvalues[] osclisten gihListener, \"/array\", \"if\"\n"
      "  if kreceived == 1 then\n"
      "    chnset kvalues[0], \"array_int\"\n"
      "    chnset kvalues[1], \"array_float\"\n"
      "  endif\n"
      "endin\n"
      "instr 2\n"
      "  ihandle oscinit " + std::to_string(addedPort) + "\n"
      "endin\n"
      "instr 3\n"
      "  ktrigger init 1\n"
      "  oscsendlo ktrigger, \"127.0.0.1\", " +
        std::to_string(listenerPort) + ", \"/array\", \"if\", 7, p4\n"
      "endin\n";

    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-d"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 60", 0);
    ASSERT_TRUE(performBlocks(csound, 4));

    csoundEventString(csound, "i 3 0 0.01 1.25", 0);
    ASSERT_TRUE(waitForChannel(csound, "array_float",
                               static_cast<MYFLT>(1.25)));
    EXPECT_EQ(csoundGetControlChannel(csound, "array_int", NULL),
              static_cast<MYFLT>(7.0));

    for (int32_t i = 0; i < 8; ++i) {
      csoundEventString(csound, "i 2 0 0.002", 0);
      ASSERT_TRUE(performBlocks(csound, 4));
    }

    csoundEventString(csound, "i 3 0 0.01 2.5", 0);
    EXPECT_TRUE(waitForChannel(csound, "array_float",
                               static_cast<MYFLT>(2.5)));
    EXPECT_EQ(csoundGetControlChannel(csound, "array_int", NULL),
              static_cast<MYFLT>(7.0));
    EXPECT_EQ(readMessages(csound).find("deinit error"), std::string::npos);
}

TEST_F(ServerTests, OscAudioBlobRoundTripsPreserveCompleteBlock)
{
    const int32_t listenerPort = findFreeUdpPort();
    ASSERT_GT(listenerPort, 0);

    const std::string orchestra =
      "sr = 48000\n"
      "ksmps = 32\n"
      "nchnls = 1\n"
      "0dbfs = 1\n"
      "chn_k \"audio_lo_sum\", 3\n"
      "chn_k \"audio_lo_weighted_sum\", 3\n"
      "chn_k \"audio_socket_sum\", 3\n"
      "chn_k \"audio_socket_weighted_sum\", 3\n"
      "gihListener oscinit " + std::to_string(listenerPort) + "\n"
      "instr 1\n"
      "  aLo init 0\n"
      "  aSocket init 0\n"
      "  kLo osclisten gihListener, \"/audio-lo\", \"a\", aLo\n"
      "  kSocket osclisten gihListener, \"/audio-socket\", \"a\", aSocket\n"
      "  if kLo == 1 then\n"
      "    kIndex = 0\n"
      "    kSum = 0\n"
      "    kWeightedSum = 0\n"
      "    until kIndex >= ksmps do\n"
      "      kSum += aLo[kIndex]\n"
      "      kWeightedSum += aLo[kIndex] * (kIndex + 1)\n"
      "      kIndex += 1\n"
      "    od\n"
      "    chnset kSum, \"audio_lo_sum\"\n"
      "    chnset kWeightedSum, \"audio_lo_weighted_sum\"\n"
      "  endif\n"
      "  if kSocket == 1 then\n"
      "    kIndex = 0\n"
      "    kSum = 0\n"
      "    kWeightedSum = 0\n"
      "    until kIndex >= ksmps do\n"
      "      kSum += aSocket[kIndex]\n"
      "      kWeightedSum += aSocket[kIndex] * (kIndex + 1)\n"
      "      kIndex += 1\n"
      "    od\n"
      "    chnset kSum, \"audio_socket_sum\"\n"
      "    chnset kWeightedSum, \"audio_socket_weighted_sum\"\n"
      "  endif\n"
      "endin\n"
      "instr 2\n"
      "  aPayload init 0\n"
      "  kIndex = 0\n"
      "  until kIndex >= ksmps do\n"
      "    aPayload[kIndex] = kIndex + 1\n"
      "    kIndex += 1\n"
      "  od\n"
      "  kTrigger init 1\n"
      "  oscsendlo kTrigger, \"127.0.0.1\", " +
        std::to_string(listenerPort) +
        ", \"/audio-lo\", \"a\", aPayload\n"
      "endin\n"
      "instr 3\n"
      "  aPayload init 0\n"
      "  kIndex = 0\n"
      "  until kIndex >= ksmps do\n"
      "    aPayload[kIndex] = kIndex + 101\n"
      "    kIndex += 1\n"
      "  od\n"
      "  kTrigger init 1\n"
      "  OSCsend kTrigger, \"127.0.0.1\", " +
        std::to_string(listenerPort) +
        ", \"/audio-socket\", \"a\", aPayload\n"
      "endin\n";

    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-d"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 60", 0);
    ASSERT_TRUE(performBlocks(csound, 4));

    csoundEventString(csound, "i 2 0 0.01", 0);
    ASSERT_TRUE(waitForChannel(csound, "audio_lo_sum",
                               static_cast<MYFLT>(528.0)));
    EXPECT_EQ(csoundGetControlChannel(csound, "audio_lo_weighted_sum", NULL),
              static_cast<MYFLT>(11440.0));

    csoundEventString(csound, "i 3 0 0.01", 0);
    ASSERT_TRUE(waitForChannel(csound, "audio_socket_sum",
                               static_cast<MYFLT>(3728.0)));
    EXPECT_EQ(csoundGetControlChannel(csound,
                                      "audio_socket_weighted_sum", NULL),
              static_cast<MYFLT>(64240.0));

    EXPECT_EQ(readMessages(csound).find("malformed"), std::string::npos);
}

TEST_F(ServerTests, OscListenerContinuesAfterPortDeinit)
{
    const int32_t listenerPort = findFreeUdpPort();
    ASSERT_GT(listenerPort, 0);

    const std::string orchestra =
      "sr = 48000\n"
      "ksmps = 32\n"
      "nchnls = 1\n"
      "0dbfs = 1\n"
      "chn_k \"queued_received\", 3\n"
      "chn_k \"queued_cycles\", 3\n"
      "chn_k \"queued_pending\", 3\n"
      "chn_k \"queued_status\", 3\n"
      "instr 1\n"
      "  ihandle oscinit " + std::to_string(listenerPort) + "\n"
      "endin\n"
      "instr 2\n"
      "  kvalue init 0\n"
      "  kreceived osclisten 0, \"/queued\", \"i\", kvalue\n"
      "  kcount init 0\n"
      "  kcycles init 0\n"
      "  kcycles += 1\n"
      "  if kreceived == 1 then\n"
      "    kcount += 1\n"
      "    chnset kcount, \"queued_received\"\n"
      "  endif\n"
      "  kpending osccount\n"
      "  chnset kcycles, \"queued_cycles\"\n"
      "  chnset kpending, \"queued_pending\"\n"
      "  chnset kreceived, \"queued_status\"\n"
      "endin\n";

    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-d"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);

    csoundEventString(csound, "i 1 0 0.02", 0);
    ASSERT_TRUE(performBlocks(csound, 2));
    csoundEventString(csound, "i 2 0 0.1", 0);
    ASSERT_TRUE(performBlocks(csound, 2));

    for (int32_t i = 0; i < 128; ++i)
      ASSERT_TRUE(udp_send_osc_int(static_cast<uint16_t>(listenerPort),
                                   "/queued", i));
    csoundSleep(50);
    ASSERT_TRUE(performBlocks(csound, 1));
    ASSERT_GT(csoundGetControlChannel(csound, "queued_pending", NULL),
              static_cast<MYFLT>(32.0));
    ASSERT_GT(csoundGetControlChannel(csound, "queued_received", NULL),
              static_cast<MYFLT>(0.0));

    ASSERT_TRUE(performBlocks(csound, 40));
    EXPECT_GT(csoundGetControlChannel(csound, "queued_cycles", NULL),
              static_cast<MYFLT>(40.0));
    EXPECT_GT(csoundGetControlChannel(csound, "queued_pending", NULL),
              static_cast<MYFLT>(0.0));
    EXPECT_EQ(csoundGetControlChannel(csound, "queued_status", NULL),
              static_cast<MYFLT>(0.0));

    const std::string messages = readMessages(csound);
    EXPECT_EQ(messages.find("deinit error"), std::string::npos);
}

TEST_F(ServerTests, OscMulticastListenerStartsAndStops)
{
    const int32_t listenerPort = findFreeUdpPort();
    ASSERT_GT(listenerPort, 0);

    const std::string orchestra =
      "sr = 48000\n"
      "ksmps = 32\n"
      "nchnls = 1\n"
      "0dbfs = 1\n"
      "instr 1\n"
      "  ihandle oscinitm \"239.255.0.1\", " +
        std::to_string(listenerPort) + "\n"
      "endin\n";

    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-d"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 0.005", 0);
    ASSERT_TRUE(performBlocks(csound, 12));

    const std::string messages = readMessages(csound);
    EXPECT_EQ(messages.find("deinit error"), std::string::npos);
}
#endif

TEST_F (ServerTests, SockrecvBoundsUnterminatedUdpString) {
    constexpr size_t mtu = 1456;
    const uint16_t port = unused_udp_port();
    ASSERT_NE(port, 0);

    const std::string orchestra =
        "sr = 48000\n"
        "ksmps = 1\n"
        "nchnls = 1\n"
        "0dbfs = 1\n"
        "instr 1\n"
        "  Smessage sockrecv " + std::to_string(port) + ", 512\n"
        "  chnset Smessage, \"sockrecv_result\"\n"
        "endin\n";
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-d"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 60", 0);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);

    std::array<char, mtu> payload;
    payload.fill('A');
    ASSERT_TRUE(udp_send_bytes(payload.data(), payload.size(), port));

    std::string received;
    for (int32_t attempt = 0; attempt < 1000 && received.empty(); attempt++) {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        int32_t size = csoundGetChannelDatasize(csound, "sockrecv_result");
        if (size > 0) {
            std::vector<char> data((size_t) size);
            csoundGetStringChannel(csound, "sockrecv_result", data.data());
            received.assign(data.data());
        }
        if (received.empty())
            csoundSleep(1);
    }

    ASSERT_EQ(received.size(), payload.size());
    EXPECT_EQ(received, std::string(payload.data(), payload.size()));
}

TEST_F (ServerTests, SockrecvKRebindsPortAfterDeinit) {
    const uint16_t port = unused_udp_port();
    ASSERT_NE(port, 0);

    const std::string orchestra =
        "sr = 48000\n"
        "ksmps = 1\n"
        "nchnls = 1\n"
        "0dbfs = 1\n"
        "instr 1\n"
        "  kmessage sockrecv " + std::to_string(port) + ", 512\n"
        "endin\n";
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-d"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);

    csoundEventString(csound, "i 1 0 0.0001", 0);
    for (int32_t cycle = 0; cycle < 32; cycle++)
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);

    csoundEventString(csound, "i 1 0 0.0001", 0);
    for (int32_t cycle = 0; cycle < 32; cycle++)
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);

    bool bind_failed = false;
    while (csoundGetMessageCnt(csound) > 0) {
        const char *message = csoundGetFirstMessage(csound);
        if (message != nullptr && strstr(message, "bind failed") != nullptr)
            bind_failed = true;
        csoundPopFirstMessage(csound);
    }
    EXPECT_FALSE(bind_failed);
}
