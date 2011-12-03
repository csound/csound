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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <string.h>
#include <errno.h>

#define MAXBUFS 32
#define MTU (1456)

#ifndef WIN32
extern  int     inet_aton(const char *cp, struct in_addr *inp);
#endif

static  uintptr_t udpRecv(void *data);
static  int     deinit_udpRecv(CSOUND *csound, void *pdata);

typedef struct {
    OPDS    h;
    MYFLT   *asig, *ipaddress, *port;
    AUXCH   aux, tmp;
    int     sock;
    struct sockaddr_in server_addr;
} SOCKRECVT;

typedef struct {
    OPDS    h;
    /* 1 channel: ptr1=asig, ptr2=port, ptr3=buffnos */
    /* 2 channel: ptr1=asigl, ptr2=asigr, ptr3=port, ptr4=buffnos */
    MYFLT   *ptr1, *ptr2, *ptr3, *ptr4;
    AUXCH   buffer, tmp;
    MYFLT   *buf;
    int     sock;
    int     wp, rp, wbufferuse, rbufferuse, canread;
    volatile int threadon;
    int     usedbuf[MAXBUFS];
    int     bufnos, bufsamps[MAXBUFS];
    CSOUND  *cs;
    void    *thrid;
    struct sockaddr_in server_addr;
} SOCKRECV;

static int deinit_udpRecv(CSOUND *csound, void *pdata)
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
    MYFLT   *buf;
    int     i, bytes, n, bufnos = p->bufnos;

    while (p->threadon) {
      /* get the data from the socket and store it in a tmp buffer */
      if ((bytes = recvfrom(p->sock, (void *)tmp, MTU, 0, &from, &clilen))) {
        p->wbufferuse++;
        p->wbufferuse = (p->wbufferuse == bufnos ? 0 : p->wbufferuse);
        buf = (MYFLT *) ((char *) p->buffer.auxp + (p->wbufferuse * MTU));
        p->usedbuf[p->wbufferuse] = 1;
        p->bufsamps[p->wbufferuse] = n = bytes / sizeof(MYFLT);
        for (i = 0; i < n; i++)
          buf[i] = tmp[i];
        p->canread = 1;
      }
    }
    return (uintptr_t) 0;
}

/* UDP version one channel */
static int init_recv(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *buf;
    int     bufnos;
#ifdef WIN32
    WSADATA wsaData = {0};
    int err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif

    p->wp = 0;
    p->rp = 0;
    p->cs = csound;
    p->bufnos = *p->ptr3;
    if (p->bufnos > MAXBUFS)
      p->bufnos = MAXBUFS;
    bufnos = p->bufnos;

    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError
        (csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int) *p->ptr2);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) < 0))
      return csound->InitError(csound, Str("bind failed"));

    if (p->buffer.auxp == NULL || (long) (MTU * bufnos) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU * bufnos, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU * bufnos);
    }
    /* create a buffer to store the received interleaved audio data */
    if (p->tmp.auxp == NULL || (long) p->tmp.size < MTU)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->tmp);
    else {
      buf = (MYFLT *) p->tmp.auxp;      /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }

    /* create thread */
    p->thrid = csound->CreateThread(udpRecv, (void *) p);
    csound->RegisterDeinitCallback(csound, (void *) p, deinit_udpRecv);
    p->threadon = 1;
    memset(p->usedbuf, 0, bufnos * sizeof(int));
    memset(p->bufsamps, 0, bufnos * sizeof(int));
    p->buf = p->buffer.auxp;
    p->rbufferuse = p->wbufferuse = 0;
    p->canread = 0;

    return OK;
}

static int send_recv(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *asig = p->ptr1;
    MYFLT   *buf = p->buf;
    int     i, n, ksmps = csound->ksmps;
    int     *bufsamps = p->bufsamps;
    int     bufnos = p->bufnos;

    if (p->canread) {
      for (i = 0, n = p->rp; i < ksmps; i++, n++) {
        if (n == bufsamps[p->rbufferuse]) {
          p->usedbuf[p->rbufferuse] = 0;
          p->rbufferuse++;
          p->rbufferuse = (p->rbufferuse == bufnos ? 0 : p->rbufferuse);
          buf = (MYFLT *) ((char *) p->buffer.auxp + (p->rbufferuse * MTU));
          n = 0;
          if (p->usedbuf[p->rbufferuse] == 0) {
            p->canread = 0;
            break;
          }
        }
        asig[i] = buf[n];
      }
      p->rp = n;
      p->buf = buf;
    }
    else {
      memset(asig, 0, sizeof(MYFLT)*ksmps);
      /* for (i = 0; i < ksmps; i++) */
      /*   asig[i] = FL(0.0); */
    }
    return OK;
}

/* UDP version two channel */
static int init_recvS(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *buf;
    int     bufnos;
#ifdef WIN32
    WSADATA wsaData = {0};
    int err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif

    p->wp = 0;
    p->rp = 0;
    p->cs = csound;
    p->bufnos = *p->ptr4;
    if (p->bufnos > MAXBUFS)
      p->bufnos = MAXBUFS;
    bufnos = p->bufnos;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int) *p->ptr3);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) < 0))
      return csound->InitError(csound, Str("bind failed"));

    if (p->buffer.auxp == NULL || (long) (MTU * bufnos) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU * bufnos, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU * bufnos);
    }
    /* create a buffer to store the received interleaved audio data */
    if (p->tmp.auxp == NULL || (long) p->tmp.size < MTU)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->tmp);
    else {
      buf = (MYFLT *) p->tmp.auxp;      /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }

    /* create thread */
    p->thrid = csound->CreateThread(udpRecv, (void *) p);
    csound->RegisterDeinitCallback(csound, (void *) p, deinit_udpRecv);
    p->threadon = 1;
    memset(p->usedbuf, 0, bufnos * sizeof(int));
    memset(p->bufsamps, 0, bufnos * sizeof(int));
    p->buf = p->buffer.auxp;
    p->rbufferuse = p->wbufferuse = 0;
    p->canread = 0;

    return OK;
}

static int send_recvS(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *asigl = p->ptr1;
    MYFLT   *asigr = p->ptr2;
    MYFLT   *buf = p->buf;
    int     i, n, ksmps = csound->ksmps;
    int     *bufsamps = p->bufsamps;
    int     bufnos = p->bufnos;

    if (p->canread) {
      for (i = 0, n = p->rp; i < ksmps; i++, n += 2) {
        if (n == bufsamps[p->rbufferuse]) {
          p->usedbuf[p->rbufferuse] = 0;
          p->rbufferuse++;
          p->rbufferuse = (p->rbufferuse == bufnos ? 0 : p->rbufferuse);
          buf = (MYFLT *) ((char *) p->buffer.auxp + (p->rbufferuse * MTU));
          n = 0;
          if (p->usedbuf[p->rbufferuse] == 0) {
            p->canread = 0;
            break;
          }
        }
        asigl[i] = buf[n];
        asigr[i] = buf[n + 1];
      }
      p->rp = n;
      p->buf = buf;
    }
    else {
      memset(asigl, 0, sizeof(MYFLT)*ksmps);
      memset(asigr, 0, sizeof(MYFLT)*ksmps);
      /* for (i = 0; i < ksmps; i++) { */
      /*   asigl[i] = FL(0.0); */
      /*   asigr[i] = FL(0.0); */
    /* } */
    }
    return OK;
}

/* TCP version */
static int init_srecv(CSOUND *csound, SOCKRECVT *p)
{
#ifdef WIN32
    WSADATA wsaData = {0};
    int err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
    /* create a STREAM (TCP) socket in the INET (IP) protocol */
    p->sock = socket(PF_INET, SOCK_STREAM, 0);

    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }

    /* create server address: where we want to connect to */

    /* clear it out */
    memset(&(p->server_addr), 0, sizeof(p->server_addr));

    /* it is an INET address */
    p->server_addr.sin_family = AF_INET;

    /* the server IP address, in network byte order */
#ifdef WIN32
    p->server_addr.sin_addr.S_un.S_addr = inet_addr((const char *) p->ipaddress);
#else
    inet_aton((const char *) p->ipaddress, &(p->server_addr.sin_addr));
#endif
    /* the port we are going to listen on, in network byte order */
    p->server_addr.sin_port = htons((int) *p->port);

again:
    if (connect(p->sock, (struct sockaddr *) &p->server_addr,
                sizeof(p->server_addr)) < 0) {
#ifdef ECONNREFUSED
      if (errno == ECONNREFUSED)
        goto again;
#endif
      return csound->InitError(csound, Str("connect failed (%d)"), errno);
    }

    return OK;
}

static int send_srecv(CSOUND *csound, SOCKRECVT *p)
{
    int     n = sizeof(MYFLT) * csound->ksmps;

    if (n != read(p->sock, p->asig, n)) {
      csound->Message(csound, "Expected %d got %d\n",
                      (int) (sizeof(MYFLT) * csound->ksmps), n);
      return csound->PerfError(csound, Str("read from socket failed"));
    }

    return OK;
}

#define S(x)    sizeof(x)

static OENTRY sockrecv_localops[] = {
  { "sockrecv", S(SOCKRECV), 5, "a", "ii", (SUBR) init_recv, NULL,
    (SUBR) send_recv },
  { "sockrecvs", S(SOCKRECV), 5, "aa", "ii", (SUBR) init_recvS, NULL,
    (SUBR) send_recvS },
  { "strecv", S(SOCKRECVT), 5, "a", "Si", (SUBR) init_srecv, NULL,
    (SUBR) send_srecv }
};

LINKAGE1(sockrecv_localops)

