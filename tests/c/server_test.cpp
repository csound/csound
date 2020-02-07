#include "csound.hpp"
#include "csPerfThread.hpp"
#include <stdio.h>
#include <CUnit/Basic.h>
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

int init_suite1(void)
{
    return 0;
}

int clean_suite1(void)
{
    return 0;
}

#if defined(__WINNT__)
    #include <Windows.h>
#else
    #include "unistd.h"
#endif

void udp_send(const char* msg) {
    struct sockaddr_in server_addr;
    int sock;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
    return;
#endif
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(sock < 0)) {
      return;
    }
#ifndef WIN32
    if (UNLIKELY(fcntl(sock, F_SETFL, O_NONBLOCK)<0)) {
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
  server_addr.sin_port = htons((int) 44100);    
  sendto(sock, (void*) msg, strlen(msg)+1, 0,
       (const struct sockaddr *) &server_addr,
	 sizeof(server_addr));
}


void test_server(void)
{
    const char  *instrument =
            "instr 1 \n"
            "k1 expon p4, p3, p4*0.001 \n"
            "a1 randi  k1, p5   \n"
            "out  a1   \n"
            "endin \n";

    Csound csound;
    csound.SetOption((char*)"-odac");
    csound.SetOption((char*)"--port=44100");
    csound.Start();
    CsoundPerformanceThread performanceThread(csound.GetCsound());
    performanceThread.Play();
    udp_send(instrument);
    udp_send("$i1 0 2 1000 1000");
    csoundSleep(3000);
    udp_send("##close##");
    performanceThread.Join();
    csound.Cleanup();
    csound.Reset();
}


int main()
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("perfthread tests", init_suite1, clean_suite1);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "Test server", test_server))
        )
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

