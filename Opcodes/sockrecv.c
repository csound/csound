/*
  sockrecv.c:

  Copyright (C) 2006 by John ffitch

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

/* Haiku 'int32' etc definitions in net headers conflict with sysdep.h */
#define __HAIKU_CONFLICT

#include "csoundCore.h"
#include <stdlib.h>
#include <sys/types.h>
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define SOCKET_ERROR (-1)
#endif
#include <string.h>
#include <errno.h>

#define MAXBUFS 32
#define MTU (1456)

#ifndef WIN32
extern  int32_t     inet_aton(const char *cp, struct in_addr *inp);
#endif

static  uintptr_t udpRecv(void *data);
static  int32_t     deinit_udpRecv(CSOUND *csound, void *pdata);

typedef struct {
  OPDS    h;
  MYFLT   *asig;
  MYFLT   *res;
  STRINGDAT *ipaddress;
  MYFLT *port;
  AUXCH   aux, tmp;
  int32_t     sock, conn;
  struct sockaddr_in server_addr;
} SOCKRECVT;

typedef struct {
  OPDS    h;
  STRINGDAT *kstr;
  STRINGDAT *ipaddress;
  MYFLT *port;
  AUXCH   aux, tmp;
  int32_t     sock, conn;
  struct sockaddr_in server_addr;
} SOCKRECVS;

typedef struct {
  OPDS    h;
  /* 1 channel: ptr1=asig, ptr2=port, ptr3=buffnos */
  /* 2 channel: ptr1=asigl, ptr2=asigr, ptr3=port, ptr4=buffnos */
  MYFLT   *ptr1, *ptr2, *ptr3, *ptr4;
  AUXCH   buffer, tmp;
  MYFLT   *buf;
  int32_t     sock;
  volatile int32_t threadon;
  int32_t buffsize;
  int32_t outsamps, rcvsamps;
  CSOUND  *cs;
  void    *thrid;
  void  *cb;
  struct sockaddr_in server_addr;
} SOCKRECV;

typedef struct {
  OPDS    h;
  /* 1 channel: ptr1=asig, ptr2=port, ptr3=buffnos */
  /* 2 channel: ptr1=asigl, ptr2=asigr, ptr3=port, ptr4=buffnos */
  STRINGDAT *ptr1;
  MYFLT   *ptr2, *ptr3, *ptr4;
  AUXCH   buffer, tmp;
  char    *buf;
  int32_t     sock;
  volatile int32_t threadon;
  int32_t buffsize;
  int32_t outsamps, rcvsamps;
  CSOUND  *cs;
  void    *thrid;
  void  *cb;
  struct sockaddr_in server_addr;
} SOCKRECVSTR;

static int32_t deinit_udpRecv(CSOUND *csound, void *pdata)
{
    SOCKRECV *p = (SOCKRECV *) pdata;

    p->threadon = 0;
    csound->JoinThread(p->thrid);
    return OK;
}

static uintptr_t udpRecv(void *pdata)
{
    struct sockaddr from;
    socklen_t clilen = sizeof(from);
    SOCKRECV *p = (SOCKRECV *) pdata;
    MYFLT   *tmp = (MYFLT *) p->tmp.auxp;
    int32_t     bytes;
    CSOUND *csound = p->cs;

    while (p->threadon) {
      /* get the data from the socket and store it in a tmp buffer */
      if ((bytes = recvfrom(p->sock, (void *)tmp, MTU, 0, &from, &clilen)) > 0) {
        csound->WriteCircularBuffer(csound, p->cb, tmp, bytes/sizeof(MYFLT));
      }
    }
    return (uintptr_t) 0;
}

static int32_t deinit_udpRecv_S(CSOUND *csound, void *pdata)
{
    SOCKRECV *p = (SOCKRECV *) pdata;

    p->threadon = 0;
    csound->JoinThread(p->thrid);

#ifndef WIN32
    close(p->sock);
    csound->Message(csound, Str("OSCraw: Closing socket\n"));
#else
    closesocket(p->sock);
    csound->Message(csound, Str("OSCraw: Closing socket\n"));
#endif
    return OK;
}

static uintptr_t udpRecv_S(void *pdata)
{
    struct sockaddr from;
    socklen_t clilen = sizeof(from);
    SOCKRECVSTR *p = (SOCKRECVSTR*) pdata;
    char *tmp = (char *) p->tmp.auxp;
    int32_t     bytes;
    CSOUND *csound = p->cs;

    while (p->threadon) {
      /* get the data from the socket and store it in a tmp buffer */
      if ((bytes = recvfrom(p->sock, (void *)tmp, MTU, 0, &from, &clilen)) > 0) {
        bytes = (bytes+sizeof(MYFLT)-1)/sizeof(MYFLT);
        csound->WriteCircularBuffer(csound, p->cb, tmp, bytes);
      }
    }
    return (uintptr_t) 0;
}


/* UDP version one channel */
static int32_t init_recv(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *buf;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
    p->cs = csound;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0))
      return csound->InitError(csound, Str("Cannot set nonblock"));
#else
    {
      u_long argp = 1;
      err = ioctlsocket(p->sock, FIONBIO, &argp);
      if (UNLIKELY(err != NO_ERROR))
        return csound->InitError(csound, Str("Cannot set nonblock"));
    }
#endif
    if (UNLIKELY(p->sock == SOCKET_ERROR)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int32_t) *p->ptr2);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) == SOCKET_ERROR))
      return csound->InitError(csound, Str("bind failed"));

    if (p->buffer.auxp == NULL || (uint64_t) (MTU) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }
    /* create a buffer to store the received interleaved audio data */
    if (p->tmp.auxp == NULL || (int64_t) p->tmp.size < MTU)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->tmp);
    else {
      buf = (MYFLT *) p->tmp.auxp;      /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }
    p->buffsize = p->buffer.size/sizeof(MYFLT);
    p->cb = csound->CreateCircularBuffer(csound,  *p->ptr3, sizeof(MYFLT));
    /* create thread */
    p->threadon = 1;
    p->thrid = csound->CreateThread(udpRecv, (void *) p);
    csound->RegisterDeinitCallback(csound, (void *) p, deinit_udpRecv);
    p->buf = p->buffer.auxp;
    p->outsamps = p->rcvsamps = 0;
    return OK;
}

/* UDP version for strings */
static int32_t init_recv_S(CSOUND *csound, SOCKRECVSTR *p)
{
    MYFLT   *buf;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif

    p->cs = csound;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0))
      return csound->InitError(csound, Str("Cannot set nonblock"));
#endif
    if (UNLIKELY(p->sock == SOCKET_ERROR)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int32_t) *p->ptr2);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) == SOCKET_ERROR))
      return csound->InitError(csound, Str("bind failed"));

    if (p->buffer.auxp == NULL || (uint64_t) (MTU) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }
    /* create a buffer to store the received string data */
    if (p->tmp.auxp == NULL || (int64_t) p->tmp.size < MTU)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->tmp);
    else {
      buf = (MYFLT *) p->tmp.auxp;      /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }
    p->buffsize = p->buffer.size/sizeof(MYFLT);
    p->cb = csound->CreateCircularBuffer(csound,  *p->ptr3, sizeof(MYFLT));
    /* create thread */
    p->threadon = 1;
    p->thrid = csound->CreateThread(udpRecv_S, (void *) p);
    csound->RegisterDeinitCallback(csound, (void *) p, deinit_udpRecv_S);
    p->buf = p->buffer.auxp;
    p->outsamps = p->rcvsamps = 0;
    return OK;
}

static int32_t send_recv_k(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *ksig = p->ptr1;
    *ksig = FL(0.0);
    if (p->outsamps >= p->rcvsamps){
      p->outsamps =  0;
      p->rcvsamps =
        csound->ReadCircularBuffer(csound, p->cb, p->buf, p->buffsize);
    }
    *ksig = p->buf[p->outsamps++];
    return OK;
}

static int32_t send_recv_S(CSOUND *csound, SOCKRECVSTR *p)
{
    STRINGDAT *str = p->ptr1;
    size_t len;
    if (p->outsamps >= p->rcvsamps) {
      p->outsamps =  0;
      p->rcvsamps =
        csound->ReadCircularBuffer(csound, p->cb, p->buf, p->buffsize);
    }
    len = strlen(&p->buf[p->outsamps]);
    //printf("len %d ans %s\n", len, &p->buf[p->outsamps]);
    if (len>str->size) {        /* ensure enough space for result */
      str->data = csound->ReAlloc(csound, str->data, len+1);
      str->size  = len;
    }
    strNcpy(str->data, &p->buf[p->outsamps], len+1);
    p->outsamps += len+1;       /* Move bffer on */
    return OK;
}


static int32_t send_recv(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *asig = p->ptr1;
    MYFLT   *buf = p->buf;
    int32_t     i, nsmps = CS_KSMPS;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t outsamps = p->outsamps, rcvsamps = p->rcvsamps;
    memset(asig, 0, sizeof(MYFLT)*nsmps);
    if (UNLIKELY(early)) nsmps -= early;

    for(i=offset; i < nsmps ; i++){
      if (outsamps >= rcvsamps){
        outsamps =  0;
        rcvsamps = csound->ReadCircularBuffer(csound, p->cb, buf, p->buffsize);
      }
      asig[i] = buf[outsamps];
      outsamps++;
    }
    p->rcvsamps = rcvsamps;
    p->outsamps = outsamps;
    return OK;
}


/* UDP version two channel */
static int32_t init_recvS(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *buf;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif

    p->cs = csound;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0))
      return csound->InitError(csound, Str("Cannot set nonblock"));
#endif
    if (UNLIKELY(p->sock == SOCKET_ERROR)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int32_t) *p->ptr3);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) == SOCKET_ERROR))
      return csound->InitError(csound, Str("bind failed"));

    if (p->buffer.auxp == NULL || (uint64_t) (MTU) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }
    /* create a buffer to store the received interleaved audio data */
    if (p->tmp.auxp == NULL || (int64_t) p->tmp.size < MTU)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->tmp);
    else {
      buf = (MYFLT *) p->tmp.auxp;      /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }
    p->cb = csound->CreateCircularBuffer(csound,  *p->ptr4, sizeof(MYFLT));
    /* create thread */
    p->threadon = 1;
    p->thrid = csound->CreateThread(udpRecv, (void *) p);
    csound->RegisterDeinitCallback(csound, (void *) p, deinit_udpRecv);
    p->buf = p->buffer.auxp;
    p->outsamps = p->rcvsamps = 0;
    p->buffsize = p->buffer.size/sizeof(MYFLT);
    return OK;
}

static int32_t send_recvS(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *asigl = p->ptr1;
    MYFLT   *asigr = p->ptr2;
    MYFLT   *buf = p->buf;
    int32_t     i, nsmps = CS_KSMPS;
    int32_t outsamps = p->outsamps, rcvsamps = p->rcvsamps;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    memset(asigl, 0, sizeof(MYFLT)*nsmps);
    memset(asigr, 0, sizeof(MYFLT)*nsmps);

    if (UNLIKELY(early)) nsmps -= early;
    for(i=offset; i < nsmps ; i++){
      if (outsamps >= rcvsamps){
        outsamps =  0;
        rcvsamps = csound->ReadCircularBuffer(csound, p->cb, buf, p->buffsize);
      }
      asigl[i] = buf[outsamps++];
      asigr[i] = buf[outsamps++];
    }
    p->rcvsamps = rcvsamps;
    p->outsamps = outsamps;

    return OK;
}

/* TCP version */
static int32_t init_srecv(CSOUND *csound, SOCKRECVT *p)
{
    socklen_t clilen;
    int32_t err;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
    /* create a STREAM (TCP) socket in the INET (IP) protocol */
    p->sock = socket(PF_INET, SOCK_STREAM, 0);

#if defined(WIN32) && !defined(__CYGWIN__)
    if (p->sock == SOCKET_ERROR) {
      err = WSAGetLastError();
      csound->InitError(csound, Str("socket failed with error: %ld\n"), err);
    }
#else
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }
#endif

    /* create server address: where we want to connect to */

    /* clear it out */
    memset(&(p->server_addr), 0, sizeof(p->server_addr));

    /* it is an INET address */
    p->server_addr.sin_family = AF_INET;

    /* the server IP address, in network byte order */
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data, &(p->server_addr.sin_addr));
#endif
    /* the port we are going to listen on, in network byte order */
    p->server_addr.sin_port = htons((int32_t) *p->port);

    /* associate the socket with the address and port */
    err = bind(p->sock, (struct sockaddr *) &p->server_addr,
               sizeof(p->server_addr));
#if defined(WIN32) && !defined(__CYGWIN__)
    if (UNLIKELY(err == SOCKET_ERROR)) {
      err = WSAGetLastError();
#else
    if (UNLIKELY(err<0)) {
      err= errno;
#endif
      return csound->InitError(csound, Str("bind failed (%d)"), err);
    }

    /* start the socket listening for new connections -- may wait */
    err = listen(p->sock, 5);
#if defined(WIN32) && !defined(__CYGWIN__)
    if (UNLIKELY(err == SOCKET_ERROR)) {
      err = WSAGetLastError();
#else
    if (UNLIKELY(err<0)) {
      err= errno;
#endif
      return csound->InitError(csound, Str("listen failed (%d)"), err);
    }
    clilen = sizeof(p->server_addr);
    p->conn = accept(p->sock, (struct sockaddr *) &p->server_addr, &clilen);
#if defined(WIN32) && !defined(__CYGWIN__)
    if (UNLIKELY(err == SOCKET_ERROR)) {
      err = WSAGetLastError();
#else
    if (UNLIKELY(err<0)) {
      err= errno;
#endif
      return csound->InitError(csound, Str("accept failed (%d)"), err);
    }
    return OK;
}

static int32_t send_srecv(CSOUND *csound, SOCKRECVT *p)
{
    int32_t     n, k = sizeof(MYFLT) * CS_KSMPS;
    MYFLT       *q = p->asig;
    if (p->sock<0) {
      if (p->res) *p->res = -1;
      return OK;
    }
    memset(q, '\0', k);
 again:
    errno = 0;
    n = recv(p->conn, q, k, 0);
    if (n==0) {      /* Connection broken */
      if (p->res) *p->res = -1;
      close(p->sock);
      p->sock = -1;
      return OK;
    }
    if (UNLIKELY(n<0||errno!=0))
      return csound->PerfError(csound, &(p->h),
                               Str("read from socket failed"));
    if (k==n) {
      if (p->res) *p->res = sizeof(MYFLT) * CS_KSMPS;
      return OK;
    }
    /* Only partialread so loop for the rest */
    //printf("k=%d n=%d\n", k, n);
    k = k-n;
    q+= n;
    goto again;
}

typedef struct _rawosc {
  OPDS h;
  ARRAYDAT *sout;
  MYFLT *kflag;
  MYFLT  *port;
  AUXCH   buffer;
  int32_t     sock;
  /*
    AUXCH tmp;
    volatile int32_t threadon;
    CSOUND  *cs;
    void    *thrid;
    void  *cb;
  */
  struct sockaddr_in server_addr;
} RAWOSC;

#include "arrays.h"

static int32_t destroy_raw_osc(CSOUND *csound, void *pp) {
    RAWOSC *p = (RAWOSC *) pp;
#ifndef WIN32
    close(p->sock);
    csound->Message(csound, Str("OSCraw: Closing socket\n"));
#else
    closesocket(p->sock);
    csound->Message(csound, Str("OSCraw: Closing socket\n"));
#endif
    return OK;
}



static int32_t init_raw_osc(CSOUND *csound, RAWOSC *p)
{
    MYFLT   *buf;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0))
      return csound->InitError(csound, Str("Cannot set nonblock"));
#else
    u_long nMode = 1; // 1: NON-BLOCKING
    if (ioctlsocket (p->sock, FIONBIO, &nMode) == SOCKET_ERROR)
       return csound->InitError(csound, Str("Cannot set nonblock"));
#endif
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int32_t) *p->port);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) == SOCKET_ERROR))
      return csound->InitError(csound, Str("bind failed"));

    if (p->buffer.auxp == NULL || (uint64_t) (MTU) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }

    csound->RegisterDeinitCallback(csound, (void *) p, destroy_raw_osc);
    if(p->sout->data == NULL)
      tabinit(csound, p->sout, 2);

  return OK;
}

static inline char le_test(){
    union _le {
      char c[2];
      short s;
    } le = {{0x0001}};
    return le.c[0];
}

static inline char *byteswap(char *p, int32_t N){
    if (le_test()) {
      char tmp;
      int32_t j ;
      for(j = 0; j < N/2; j++) {
        tmp = p[j];
        p[j] = p[N - j - 1];
        p[N - j - 1] = tmp;
      }
    }
    return p;
}

static int32_t perf_raw_osc(CSOUND *csound, RAWOSC *p) {

    ARRAYDAT *sout = p->sout;
    if (sout->sizes[0] < 2 ||
       sout->dimensions > 1)
      return
        csound->PerfError(csound, &(p->h), Str("output array too small\n"));

    STRINGDAT *str = (STRINGDAT *) p->sout->data;
    char *buf = (char *) p->buffer.auxp;
    int32_t n = 0, j = 1;
    size_t len = 0;
    char c;
    memset(buf, 0, p->buffer.size);
    uint32_t size = 0;
    char *types  = NULL;
    struct sockaddr from;
    socklen_t clilen = sizeof(from);
    int32_t bytes =
      recvfrom(p->sock, (void *)buf, MTU-1, 0, &from, &clilen);
    if (bytes < 0) bytes = 0;

    // terminating string to satisfy coverity
    buf[p->buffer.size-1] = '\0';

    if (bytes) {
      if (strncmp(buf,"#bundle",7) == 0) { // bundle
        buf += 8;
        buf += 8;
        size = *((uint32_t *) buf);
        byteswap((char *)&size, 4);
        buf += 4;
      } else size = bytes;
      while(size > 0 && size < MTU)  {
        /* get address & types */
        if (n < sout->sizes[0]) {
          len = strlen(buf);
          // printf("len %d size %d incr %d\n",
          //        len, str[n].size, ((size_t) ceil((len+1)/4.)*4));
          if (len >= str[n].size) {
            str[n].data = csound->ReAlloc(csound, str[n].data, len+1);
            memset(str[n].data,0,len+1);
            str[n].size  = len+1;
          }
          strNcpy(str[n].data, buf, len+1);
          //str[n].data[len] = '\0'; // explicitly terminate it.
          n++;
          buf += ((size_t) ceil((len+1)/4.)*4);
        }
        if (n < sout->sizes[0]) {
          len = strlen(buf);
          if (len >= str[n].size) {
            str[n].data = csound->ReAlloc(csound, str[n].data, len+1);
            str[n].size  = len+1;
          }
          strNcpy(str[n].data, buf, len+1);
          //str[n].data[str[n].size-1] = '\0'; // explicitly terminate it.
          types = str[n].data;
          n++;
          buf += ((size_t) ceil((len+1)/4.)*4);
        }
        j = 1;
        // parse data
        while((c = types[j++]) != '\0' && n < sout->sizes[0]){
          if (c == 'f') {
            float f = *((float *) buf);
            byteswap((char*)&f,4);
            if (str[n].size < 32) {
              str[n].data = csound->ReAlloc(csound, str[n].data, 32);
              str[n].size  = 32;
            }
            snprintf(str[n].data, str[n].size, "%f", f);
            buf += 4;
          } else if (c == 'i') {
            int32_t d = *((int32_t *) buf);
            byteswap((char*) &d,4);
            if (str[n].size < 32) {
              str[n].data = csound->ReAlloc(csound, str[n].data, 32);
              str[n].size  = 32;
            }
            snprintf(str[n].data, str[n].size, "%d", d);
            buf += 4;
          } else if (c == 's') {
            len = strlen(buf);
            if (len+1 > str[n].size) {
              str[n].data = csound->ReAlloc(csound, str[n].data, len+1);
              str[n].size  = len+1;
            }
            strNcpy(str[n].data, buf, len+1);
            //str[n].data[len] = '\0';
            len = ceil((len+1)/4.)*4;
            buf += len;
          } else if (c == 'b') {
            len = *((uint32_t *) buf);
            byteswap((char*)&len,4);
            len = ceil((len)/4.)*4;
            if (len > str[n].size) {
              str[n].data = csound->ReAlloc(csound, str[n].data, len+1);
              str[n].size  = len+1;
            }
            strNcpy(str[n].data, buf, len+1);
            //str[n].data[len] = '\0';
            buf += len;
          }
          n++;
        }
        size = *((uint32_t *) buf);
        byteswap((char *)&size, 4);
        buf += 4;
      }
    }
    *p->kflag = n;
    return OK;
}




#define S(x)    sizeof(x)

static OENTRY sockrecv_localops[] = {
  { "sockrecv.k", S(SOCKRECV), 0, 3, "k", "ii",
    (SUBR) init_recv, (SUBR) send_recv_k, NULL },
  { "sockrecv.a", S(SOCKRECV), 0, 3, "a", "ii",
    (SUBR) init_recv, (SUBR) send_recv, NULL },
  { "sockrecv.S", S(SOCKRECVSTR), 0, 3, "S", "ii",
    (SUBR) init_recv_S,
    (SUBR) send_recv_S, NULL },
  { "sockrecvs", S(SOCKRECV), 0, 3, "aa", "ii",
    (SUBR) init_recvS,
    (SUBR) send_recvS, NULL },
  { "strecv", S(SOCKRECVT), 0, 3, "az", "Si",
    (SUBR) init_srecv,
    (SUBR) send_srecv, NULL },
  { "OSCraw", S(RAWOSC), 0, 3, "S[]k", "i",
    (SUBR) init_raw_osc, (SUBR) perf_raw_osc, NULL, NULL}
};

LINKAGE_BUILTIN(sockrecv_localops)
