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
