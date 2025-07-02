#include <stdio.h>
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
