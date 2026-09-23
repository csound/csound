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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/ 

#ifdef  HAVE_SOCKETS 
/* Haiku 'int32' etc definitions in net headers conflict with sysdep.h */
#define __HAIKU_CONFLICT


#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
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
#define STRING_RECV_BUFFER_SIZE (MTU + 1)

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
#if defined(WIN32) && !defined(__CYGWIN__)
  SOCKET sock, conn;
#else
  int32_t sock, conn;
#endif
  int32_t init_done;
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
  int32_t wsa_started;
  volatile int32_t threadon;
  int32_t buffsize;
  int32_t channels;
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
  int32_t wsa_started;
  volatile int32_t threadon;
  int32_t buffsize;
  int32_t outsamps, rcvsamps;
  CSOUND  *cs;
  void    *thrid;
  void  *cb;
  struct sockaddr_in server_addr;
} SOCKRECVSTR;

static void deinit_udp_receiver(CSOUND *csound, volatile int32_t *threadon,
                                void **thrid, int32_t *sock, void **cb,
                                int32_t *wsa_started)
{
    *threadon = 0;
    if (*sock != SOCKET_ERROR) {
#if defined(WIN32) && !defined(__CYGWIN__)
      closesocket(*sock);
#else
      close(*sock);
#endif
      *sock = SOCKET_ERROR;
    }
    if (*thrid != NULL) {
      csound->JoinThread(*thrid);
      *thrid = NULL;
    }
    if (*cb != NULL) {
      csound->DestroyCircularBuffer(csound, *cb);
      *cb = NULL;
    }
#if defined(WIN32) && !defined(__CYGWIN__)
    if (*wsa_started)
      WSACleanup();
#endif
    *wsa_started = 0;
}

static int32_t deinit_udpRecv(CSOUND *csound, void *pdata)
{
    SOCKRECV *p = (SOCKRECV *) pdata;

    deinit_udp_receiver(csound, &p->threadon, &p->thrid, &p->sock, &p->cb,
                        &p->wsa_started);
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
      if ((bytes = (int32_t) recvfrom(p->sock, (void *)tmp, MTU, 0, &from, &clilen)) > 0) {
        csound->WriteCircularBuffer(csound, p->cb, tmp,
                                    bytes/(sizeof(MYFLT)*p->channels));
      }
    }
    return (uintptr_t) 0;
}

static int32_t deinit_udpRecv_S(CSOUND *csound, void *pdata)
{
    SOCKRECVSTR *p = (SOCKRECVSTR *) pdata;

    deinit_udp_receiver(csound, &p->threadon, &p->thrid, &p->sock, &p->cb,
                        &p->wsa_started);
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
      if ((bytes = (int32_t) recvfrom(p->sock, (void *)tmp, MTU, 0, &from, &clilen)) > 0) {
        if (tmp[bytes - 1] != '\0')
          tmp[bytes++] = '\0';
        csound->WriteCircularBuffer(csound, p->cb, tmp, bytes);
      }
    }
    return (uintptr_t) 0;
}


/* UDP version one channel */
static int32_t init_recv(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *buf;
    p->sock = SOCKET_ERROR;
    p->wsa_started = 0;
    p->thrid = NULL;
    p->cb = NULL;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
    p->wsa_started = 1;
#endif
    p->cs = csound;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0))
      return csound->InitError(csound, "%s", Str("Cannot set nonblock"));
#else
    {
      u_long argp = 1;
      err = ioctlsocket(p->sock, FIONBIO, &argp);
      if (UNLIKELY(err != NO_ERROR))
        return csound->InitError(csound, "%s", Str("Cannot set nonblock"));
    }
#endif
    if (UNLIKELY(p->sock == SOCKET_ERROR)) {
      return csound->InitError(csound, "%s", Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int32_t) *p->ptr2);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) == SOCKET_ERROR))
      return csound->InitError(csound, "%s", Str("bind failed"));

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
    p->buffsize = (int32_t)(p->buffer.size/sizeof(MYFLT));
    p->channels = 1;
    p->cb = csound->CreateCircularBuffer(csound,  *p->ptr3, sizeof(MYFLT));
    /* create thread */
    p->threadon = 1;
    p->thrid = csound->CreateThread(udpRecv, (void *) p);
    p->buf = p->buffer.auxp;
    p->outsamps = p->rcvsamps = 0;
    return OK;
}

/* UDP version for strings */
static int32_t init_recv_S(CSOUND *csound, SOCKRECVSTR *p)
{
    char    *buf;
    int64_t circular_buffer_size;
    p->sock = SOCKET_ERROR;
    p->wsa_started = 0;
    p->thrid = NULL;
    p->cb = NULL;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
    p->wsa_started = 1;
#endif

    p->cs = csound;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0))
      return csound->InitError(csound, "%s", Str("Cannot set nonblock"));
#else
    {
      u_long nonblocking = 1;
      if (UNLIKELY(ioctlsocket(p->sock, FIONBIO, &nonblocking)
                   == SOCKET_ERROR))
        return csound->InitError(csound, "%s", Str("Cannot set nonblock"));
    }
#endif
    if (UNLIKELY(p->sock == SOCKET_ERROR)) {
      return csound->InitError(csound, "%s", Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int32_t) *p->ptr2);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) == SOCKET_ERROR))
      return csound->InitError(csound, "%s", Str("bind failed"));

    if (p->buffer.auxp == NULL ||
        (uint64_t) STRING_RECV_BUFFER_SIZE > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, STRING_RECV_BUFFER_SIZE, &p->buffer);
    else {
      buf = (char *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, STRING_RECV_BUFFER_SIZE);
    }
    /* create a buffer to store the received string data */
    if (p->tmp.auxp == NULL ||
        (int64_t) p->tmp.size < STRING_RECV_BUFFER_SIZE)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, STRING_RECV_BUFFER_SIZE, &p->tmp);
    else {
      buf = (char *) p->tmp.auxp;      /* make sure buffer is empty */
      memset(buf, 0, STRING_RECV_BUFFER_SIZE);
    }
    p->buffsize = (int32_t) p->buffer.size;
    /* validate before casting: an out-of-range or NaN MYFLT to int64
       conversion is undefined behaviour */
    if (UNLIKELY(!(*p->ptr3 >= FL(1.0)) ||
                 (double) *p->ptr3 >
                 (double) (INT32_MAX / (int32_t) sizeof(MYFLT))))
      return csound->InitError(csound, "%s",
                               Str("invalid sockrecv buffer length"));
    circular_buffer_size = (int64_t) *p->ptr3 * (int64_t) sizeof(MYFLT);
    p->cb = csound->CreateCircularBuffer(csound,
                                         (int32_t) circular_buffer_size,
                                         sizeof(char));
    /* create thread */
    p->threadon = 1;
    p->thrid = csound->CreateThread(udpRecv_S, (void *) p);
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
    if (p->outsamps < p->rcvsamps)
      *ksig = p->buf[p->outsamps++];
    return OK;
}

static int32_t send_recv_S(CSOUND *csound, SOCKRECVSTR *p)
{
    STRINGDAT *str = p->ptr1;
    char *start, *end;
    size_t available, len;
    if (p->outsamps >= p->rcvsamps) {
      p->outsamps =  0;
      p->rcvsamps =
        csound->ReadCircularBuffer(csound, p->cb, p->buf, p->buffsize);
    }
    if (p->rcvsamps == 0) {
      str->data[0] = '\0';
      return OK;
    }
    start = &p->buf[p->outsamps];
    available = (size_t) (p->rcvsamps - p->outsamps);
    end = (char *) memchr(start, '\0', available);
    len = end == NULL ? available : (size_t) (end - start);
    if (len + 1 > str->size) {  /* ensure enough space for result */
      str->data = csound->ReAlloc(csound, str->data, len+1);
      str->size  = len + 1;
    }
    memcpy(str->data, start, len);
    str->data[len] = '\0';
    p->outsamps += (int32_t) len;
    if (end != NULL)
      p->outsamps++;
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
        if (rcvsamps == 0) break;
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
    p->sock = SOCKET_ERROR;
    p->wsa_started = 0;
    p->thrid = NULL;
    p->cb = NULL;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
    p->wsa_started = 1;
#endif

    p->cs = csound;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0))
      return csound->InitError(csound, "%s", Str("Cannot set nonblock"));
#endif
    if (UNLIKELY(p->sock == SOCKET_ERROR)) {
      return csound->InitError(csound, "%s", Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int32_t) *p->ptr3);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) == SOCKET_ERROR))
      return csound->InitError(csound, "%s", Str("bind failed"));

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
    /* Queue whole stereo frames so a full queue cannot split a pair. */
    p->channels = 2;
    p->cb = csound->CreateCircularBuffer(csound, (int32_t)*p->ptr4 / 2,
                                        2 * sizeof(MYFLT));
    if (UNLIKELY(p->cb == NULL))
      return csound->InitError(csound, "%s", Str("sockrecvs: invalid buffer size"));
    /* create thread */
    p->threadon = 1;
    p->thrid = csound->CreateThread(udpRecv, (void *) p);
    p->buf = p->buffer.auxp;
    p->outsamps = p->rcvsamps = 0;
    p->buffsize = (int32_t)(p->buffer.size/sizeof(MYFLT));
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
        rcvsamps = 2 * csound->ReadCircularBuffer(csound, p->cb, buf,
                                                p->buffsize / 2);
        if (rcvsamps == 0) break;
      }
      asigl[i] = buf[outsamps++];
      asigr[i] = buf[outsamps++];
    }
    p->rcvsamps = rcvsamps;
    p->outsamps = outsamps;

    return OK;
}

/* TCP version */
static int32_t deinit_srecv(CSOUND *csound, SOCKRECVT *p)
{
    IGN(csound);
    if (!p->init_done)
      return OK;
#if defined(WIN32) && !defined(__CYGWIN__)
    if (p->conn != INVALID_SOCKET) closesocket(p->conn);
    if (p->sock != INVALID_SOCKET) closesocket(p->sock);
    WSACleanup();
#else
    if (p->conn != SOCKET_ERROR) close(p->conn);
    if (p->sock != SOCKET_ERROR) close(p->sock);
#endif
    p->conn = p->sock = SOCKET_ERROR;
    p->init_done = 0;
    return OK;
}

static int32_t init_srecv(CSOUND *csound, SOCKRECVT *p)
{
    socklen_t clilen;
    int32_t err;
    const char *message;
    deinit_srecv(csound, p);
    p->conn = p->sock = SOCKET_ERROR;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
    p->init_done = 1;
    /* create a STREAM (TCP) socket in the INET (IP) protocol */
    p->sock = socket(PF_INET, SOCK_STREAM, 0);
    if (UNLIKELY(p->sock == SOCKET_ERROR)) {
      message = Str("creating socket");
      goto error;
    }

    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data, &p->server_addr.sin_addr);
#endif
    p->server_addr.sin_port = htons((int32_t) *p->port);

    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) == SOCKET_ERROR)) {
      message = Str("bind failed");
      goto error;
    }
    if (UNLIKELY(listen(p->sock, 5) == SOCKET_ERROR)) {
      message = Str("listen failed");
      goto error;
    }
    clilen = sizeof(p->server_addr);
    p->conn = accept(p->sock, (struct sockaddr *) &p->server_addr, &clilen);
    if (UNLIKELY(p->conn == SOCKET_ERROR)) {
      message = Str("accept failed");
      goto error;
    }
    return OK;

 error:
#if defined(WIN32) && !defined(__CYGWIN__)
    err = WSAGetLastError();
#else
    err = errno;
#endif
    deinit_srecv(csound, p);
    return csound->InitError(csound, "%s (%d)", message, err);
}

static int32_t send_srecv(CSOUND *csound, SOCKRECVT *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    int32_t remaining = sizeof(MYFLT) * (CS_KSMPS - offset - early);
    int32_t received = 0;
    char *q = (char *) &p->asig[offset];

    memset(p->asig, 0, sizeof(MYFLT) * CS_KSMPS);
    if (!p->init_done) {
      if (p->res) *p->res = -1;
      return OK;
    }
    while (remaining > 0) {
      int32_t n = (int32_t) recv(p->conn, q, remaining, 0);
      if (n == 0) {
        /* Discard an incomplete sample at end of stream. */
        memset(q - received % sizeof(MYFLT), 0,
               received % sizeof(MYFLT));
        deinit_srecv(csound, p);
        if (p->res) *p->res = -1;
        return OK;
      }
      if (UNLIKELY(n < 0)) {
#if defined(WIN32) && !defined(__CYGWIN__)
        if (WSAGetLastError() == WSAEINTR) continue;
#else
        if (errno == EINTR) continue;
#endif
        deinit_srecv(csound, p);
        if (p->res) *p->res = -1;
        return csound->PerfError(csound, &p->h,
                                 "%s", Str("read from socket failed"));
      }
      q += n;
      remaining -= n;
      received += n;
    }
    if (p->res) *p->res = received;
    return OK;
}

typedef struct _rawosc {
  OPDS h;
  ARRAYDAT *sout;
  MYFLT *kflag;
  MYFLT  *port;
  AUXCH   buffer;
#if defined(WIN32) && !defined(__CYGWIN__)
  SOCKET sock;
#else
  int32_t     sock;
#endif
  int32_t wsa_started;
  int32_t init_done;
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
    IGN(csound);
    if (!p->init_done)
      return OK;
#if defined(WIN32) && !defined(__CYGWIN__)
    if (p->sock != INVALID_SOCKET)
      closesocket(p->sock);
    if (p->wsa_started)
      WSACleanup();
#else
    if (p->sock != SOCKET_ERROR)
      close(p->sock);
#endif
    p->sock = SOCKET_ERROR;
    p->wsa_started = 0;
    p->init_done = 0;
    return OK;
}



static int32_t init_raw_osc(CSOUND *csound, RAWOSC *p)
{
    MYFLT   *buf;
    const char *message;
    destroy_raw_osc(csound, p);
    p->sock = SOCKET_ERROR;
    p->wsa_started = 0;
    if (UNLIKELY(!(*p->port >= FL(0.0) && *p->port <= FL(65535.0))))
      return csound->InitError(csound, "%s", Str("invalid port number"));
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
    p->wsa_started = 1;
#endif
    p->init_done = 1;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock == SOCKET_ERROR)) {
      message = Str("creating socket");
      goto error;
    }
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0)) {
      message = Str("Cannot set nonblock");
      goto error;
    }
#else
    u_long nMode = 1; // 1: NON-BLOCKING
    if (ioctlsocket (p->sock, FIONBIO, &nMode) == SOCKET_ERROR) {
      message = Str("Cannot set nonblock");
      goto error;
    }
#endif
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int32_t) *p->port);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) == SOCKET_ERROR)) {
      message = Str("bind failed");
      goto error;
    }

    if (p->buffer.auxp == NULL || (uint64_t) (MTU) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }
    if(p->sout->data == NULL)
      if (UNLIKELY(tabinit(csound, p->sout, 2,
                           p->h.insdshead) != OK)) {
        destroy_raw_osc(csound, p);
        return csound_array_init_resize_error(csound);
      }

  return OK;

 error:
  destroy_raw_osc(csound, p);
  return csound->InitError(csound, "%s", message);
}



static int32_t oscraw_reserve(CSOUND *csound, STRINGDAT *str, size_t len)
{
    if (len + 1 > str->size) {
      char *newData = (char *) csound->ReAlloc(csound, str->data, len + 1);
      if (newData == NULL)
        return NOTOK;
      str->data = newData;
      str->size = len + 1;
    }
    return OK;
}

static int32_t oscraw_store(CSOUND *csound, STRINGDAT *str,
                            const char *data, size_t len)
{
    if (oscraw_reserve(csound, str, len) != OK)
      return NOTOK;
    if (len != 0)
      memcpy(str->data, data, len);
    str->data[len] = '\0';
    return OK;
}

static int32_t oscraw_read_string(const unsigned char **cursor,
                                  const unsigned char *end,
                                  const char **value, size_t *len)
{
    const unsigned char *nul;
    size_t padded;
    if (*cursor >= end ||
        (nul = memchr(*cursor, '\0', (size_t) (end - *cursor))) == NULL)
      return NOTOK;
    *len = (size_t) (nul - *cursor);
    padded = (*len + 4) & ~(size_t) 3;
    if (padded > (size_t) (end - *cursor))
      return NOTOK;
    *value = (const char *) *cursor;
    *cursor += padded;
    return OK;
}

static int32_t oscraw_read_u32(const unsigned char **cursor,
                               const unsigned char *end, uint32_t *value)
{
    if ((size_t) (end - *cursor) < sizeof(*value))
      return NOTOK;
    memcpy(value, *cursor, sizeof(*value));
    byteswap((char *) value, sizeof(*value));
    *cursor += sizeof(*value);
    return OK;
}

static int32_t oscraw_store_array(CSOUND *csound, STRINGDAT *str,
                                  const unsigned char *data, size_t len)
{
    int32_t dimensions;
    size_t count = 1, shapeBytes, valueBytes, capacity, used = 0;
    const unsigned char *sizes;
    const unsigned char *values;
    char *output;
    int32_t i;
    if (len < sizeof(dimensions))
      return NOTOK;
    memcpy(&dimensions, data, sizeof(dimensions));
    if (dimensions < 1 || (size_t) dimensions >
        (len - sizeof(dimensions)) / sizeof(int32_t))
      return NOTOK;
    shapeBytes = (size_t) dimensions * sizeof(int32_t);
    sizes = data + sizeof(dimensions);
    for (i = 0; i < dimensions; i++) {
      int32_t size;
      memcpy(&size, sizes + (size_t) i * sizeof(size), sizeof(size));
      if (size < 0 || (size != 0 && count > SIZE_MAX / (size_t) size))
        return NOTOK;
      count *= (size_t) size;
    }
    if (count > SIZE_MAX / sizeof(MYFLT))
      return NOTOK;
    valueBytes = count * sizeof(MYFLT);
    if (sizeof(dimensions) + shapeBytes > len ||
        valueBytes > len - sizeof(dimensions) - shapeBytes)
      return NOTOK;
    capacity = 64 + (size_t) dimensions * 16 + count * 32;
    if (capacity > str->size) {
      output = (char *) csound->ReAlloc(csound, str->data, capacity);
      if (output == NULL)
        return NOTOK;
      str->data = output;
      str->size = capacity;
    }
    output = str->data;
#define OSCRAW_APPEND(...) do {                                              \
      int32_t written = snprintf(output + used, capacity - used, __VA_ARGS__); \
      if (written < 0 || (size_t) written >= capacity - used) return NOTOK;  \
      used += (size_t) written;                                               \
    } while (0)
    OSCRAW_APPEND("%d:[", dimensions);
    for (i = 0; i < dimensions; i++) {
      int32_t size;
      memcpy(&size, sizes + (size_t) i * sizeof(size), sizeof(size));
      OSCRAW_APPEND(i == 0 ? "%d" : ",%d", size);
    }
    OSCRAW_APPEND("]:[");
    values = data + sizeof(dimensions) + shapeBytes;
    for (size_t j = 0; j < count; j++) {
      MYFLT value;
      memcpy(&value, values + j * sizeof(value), sizeof(value));
      OSCRAW_APPEND(j == 0 ? "%.9g" : ",%.9g", (double) value);
    }
    OSCRAW_APPEND("]");
#undef OSCRAW_APPEND
    return OK;
}

static int32_t oscraw_store_values(CSOUND *csound, STRINGDAT *str,
                                   const unsigned char *data, size_t len,
                                   int32_t hasCount)
{
    size_t count, capacity, used = 0;
    char *output;
    if (len % sizeof(MYFLT) != 0)
      return NOTOK;
    count = len / sizeof(MYFLT);
    if (hasCount) {
      MYFLT declared;
      if (count == 0)
        return NOTOK;
      memcpy(&declared, data, sizeof(declared));
      if (!(declared >= FL(0.0) && declared <= (MYFLT) (count - 1)))
        return NOTOK;
      count = (size_t) declared;
      if (declared != (MYFLT) count)
        return NOTOK;
      data += sizeof(MYFLT);
    }
    capacity = 4 + count * 32;
    if (oscraw_reserve(csound, str, capacity - 1) != OK)
      return NOTOK;
    output = str->data;
    output[used++] = '[';
    for (size_t i = 0; i < count; i++) {
      MYFLT value;
      int32_t written;
      memcpy(&value, data + i * sizeof(value), sizeof(value));
      written = snprintf(output + used, capacity - used,
                         i == 0 ? "%.9g" : ",%.9g", (double) value);
      if (written < 0 || (size_t) written >= capacity - used)
        return NOTOK;
      used += (size_t) written;
    }
    output[used++] = ']';
    output[used] = '\0';
    return OK;
}

static int32_t oscraw_parse_message(CSOUND *csound, RAWOSC *p,
                                    const unsigned char *cursor,
                                    const unsigned char *end, int32_t *count)
{
    ARRAYDAT *out = p->sout;
    const char *address, *types;
    size_t addressLen, typesLen;
    if (oscraw_read_string(&cursor, end, &address, &addressLen) != OK ||
        oscraw_read_string(&cursor, end, &types, &typesLen) != OK ||
        typesLen == 0 || types[0] != ',')
      return NOTOK;
    if (*count < out->sizes[0] &&
        oscraw_store(csound, csound_string_array_element(out, (*count)++),
                     address, addressLen) != OK)
      return NOTOK;
    if (*count < out->sizes[0] &&
        oscraw_store(csound, csound_string_array_element(out, (*count)++),
                     types, typesLen) != OK)
      return NOTOK;
    for (size_t i = 1; i < typesLen; i++) {
      STRINGDAT *str;
      uint32_t raw;
      if (*count >= out->sizes[0])
        return OK;
      str = csound_string_array_element(out, *count);
      switch (types[i]) {
      case 'f': {
        float value;
        if (oscraw_read_u32(&cursor, end, &raw) != OK) return NOTOK;
        memcpy(&value, &raw, sizeof(value));
        if (oscraw_reserve(csound, str, 31) != OK)
          return NOTOK;
        snprintf(str->data, str->size, "%g", (double) value);
        break;
      }
      case 'i': {
        int32_t value;
        if (oscraw_read_u32(&cursor, end, &raw) != OK) return NOTOK;
        memcpy(&value, &raw, sizeof(value));
        if (oscraw_reserve(csound, str, 31) != OK)
          return NOTOK;
        snprintf(str->data, str->size, "%d", value);
        break;
      }
      case 's': {
        const char *value;
        size_t len;
        if (oscraw_read_string(&cursor, end, &value, &len) != OK ||
            oscraw_store(csound, str, value, len) != OK)
          return NOTOK;
        break;
      }
      case 'b':
      case 'A':
      case 'a':
      case 'G': {
        const unsigned char *blob;
        size_t padded;
        uint64_t padded64;
        if (oscraw_read_u32(&cursor, end, &raw) != OK)
          return NOTOK;
        padded64 = ((uint64_t) raw + 3) & ~(uint64_t) 3;
        if (padded64 > (size_t) (end - cursor))
          return NOTOK;
        padded = (size_t) padded64;
        blob = cursor;
        cursor += padded;
        if (types[i] == 'A') {
          if (oscraw_store_array(csound, str, blob, raw) != OK) return NOTOK;
        }
        else if (types[i] == 'a' || types[i] == 'G') {
          if (oscraw_store_values(csound, str, blob, raw,
                                  types[i] == 'a') != OK)
            return NOTOK;
        }
        else {
          size_t outputLen = 2 + (size_t) raw * 2;
          if (oscraw_reserve(csound, str, outputLen) != OK) return NOTOK;
          str->data[0] = '0'; str->data[1] = 'x';
          for (size_t j = 0; j < raw; j++)
            snprintf(str->data + 2 + j * 2, 3, "%02x", blob[j]);
        }
        break;
      }
      case 'T':
        if (oscraw_store(csound, str, "true", 4) != OK) return NOTOK;
        break;
      case 'F':
        if (oscraw_store(csound, str, "false", 5) != OK) return NOTOK;
        break;
      case 'I':
        if (oscraw_store(csound, str, "inf", 3) != OK) return NOTOK;
        break;
      case 'N':
        if (oscraw_store(csound, str, "nil", 3) != OK) return NOTOK;
        break;
      default:
        return NOTOK;
      }
      (*count)++;
    }
    return OK;
}

static int32_t perf_raw_osc(CSOUND *csound, RAWOSC *p)
{
    ARRAYDAT *out = p->sout;
    unsigned char *buffer = (unsigned char *) p->buffer.auxp;
    const unsigned char *cursor, *end;
    struct sockaddr from;
    socklen_t fromLen = sizeof(from);
    int32_t bytes, count = 0;

    *p->kflag = 0;
    if (out->dimensions != 1 || out->sizes == NULL || out->sizes[0] < 2)
      return csound->PerfError(csound, &p->h, "%s",
                               Str("output array too small\n"));
    bytes = (int32_t) recvfrom(p->sock, buffer, MTU, 0, &from, &fromLen);
    if (bytes <= 0)
      return OK;
    cursor = buffer;
    end = buffer + bytes;
    if (bytes >= 16 && memcmp(cursor, "#bundle\0", 8) == 0) {
      cursor += 16;  /* bundle marker and time tag */
      while (cursor < end) {
        uint32_t messageSize;
        if (oscraw_read_u32(&cursor, end, &messageSize) != OK ||
            messageSize == 0 || (messageSize & 3) != 0 ||
            messageSize > (size_t) (end - cursor) ||
            oscraw_parse_message(csound, p, cursor, cursor + messageSize,
                                 &count) != OK)
          return OK;
        cursor += messageSize;
      }
    }
    else if (oscraw_parse_message(csound, p, cursor, end, &count) != OK) {
      return OK;
    }
    *p->kflag = count;
    return OK;
}




#define S(x)    sizeof(x)

static OENTRY sockrecv_localops[] = {
  { "sockrecv.k", S(SOCKRECV), 0, "k", "ii",
    (SUBR) init_recv, (SUBR) send_recv_k, (SUBR) deinit_udpRecv },
  { "sockrecv.a", S(SOCKRECV), 0, "a", "ii",
    (SUBR) init_recv, (SUBR) send_recv, (SUBR) deinit_udpRecv },
  { "sockrecv.S", S(SOCKRECVSTR), 0, "S", "ii",
    (SUBR) init_recv_S,
    (SUBR) send_recv_S, (SUBR) deinit_udpRecv_S },
  { "sockrecvs", S(SOCKRECV), 0, "aa", "ii",
    (SUBR) init_recvS,
    (SUBR) send_recvS,  (SUBR) deinit_udpRecv },
  { "strecv", S(SOCKRECVT), 0, "az", "Si",
    (SUBR) init_srecv,
    (SUBR) send_srecv, (SUBR) deinit_srecv },
  CSOUND_DEPRECATED_OPCODE("OSCraw", "oscraw", ALIAS, "Renamed alias; maintain the shared implementation through its supported name.")
  { "OSCraw", S(RAWOSC), 0, "S[]k", "i",
    (SUBR) init_raw_osc, (SUBR) perf_raw_osc,
    (SUBR) destroy_raw_osc, NULL, 2},
    { "oscraw", S(RAWOSC), 0, "S[]k", "i",
    (SUBR) init_raw_osc, (SUBR) perf_raw_osc,
    (SUBR) destroy_raw_osc, NULL}
};

LINKAGE_BUILTIN(sockrecv_localops)
#endif
