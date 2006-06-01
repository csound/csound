/*
    sockrecv.c:

    Copyright (C) 2006 by John ffitch

    This file is not yet part of Csound.

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

#if defined(__linux) || defined(__linux__)
/* for usleep() */
#  define _XOPEN_SOURCE 600
#endif

#include "csdl.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#define MAXBUFS 32
#define MTU (1456)

extern int inet_aton(const char *cp, struct in_addr *inp);

static uintptr_t udpRecv(void *data);
static int deinit_udpRecv(CSOUND *csound, void *pdata);

typedef struct {
    OPDS    h;
    MYFLT   *asig, *ipaddress, *port;
    AUXCH   aux, tmp;
    int     sock;
    struct sockaddr_in server_addr;
} SOCKRECVT;

typedef struct {
    OPDS    h;
    MYFLT   *asigl, *ipaddress, *port, *ibuffnos;
    AUXCH   buffer, tmp;
    MYFLT   *buf;
    int     sock;
    int     wp, rp, wbufferuse, rbufferuse;
    int     threadon;
    int     usedbuf[MAXBUFS];
    int     bufnos, bufsamps[MAXBUFS];
    CSOUND  *cs;
    void    *thrid;
    struct sockaddr_in server_addr;
} SOCKRECV;

typedef struct {
    OPDS    h;
    MYFLT   *asigl, *asigr, *ipaddress, *port, *ibuffnos;
    AUXCH   buffer, tmp;
    MYFLT   *buf;
    int     sock;
    int     wp, rp, wbufferuse, rbufferuse;
    int     threadon;
    int     usedbuf[MAXBUFS];
    int     bufnos, bufsamps[MAXBUFS];
    CSOUND  *cs;
    void    *thrid;
    struct sockaddr_in server_addr;
} SOCKRECVS;

static int deinit_udpRecv(CSOUND *csound, void *pdata)
{
    SOCKRECVS *p = (SOCKRECVS *) pdata;

    p->threadon = 0;
    csound->JoinThread(p->thrid);
    /* csound->Message(csound, "+++ udp receive thread finished.\n"); */
    return OK;
}

static uintptr_t udpRecv(void *pdata)
{
    struct sockaddr from;
    socklen_t clilen = sizeof(from);
    SOCKRECVS *p = (SOCKRECVS *) pdata;
    int     *threadon = &p->threadon;
    MYFLT   *tmp = (MYFLT *) p->tmp.auxp;
    MYFLT   *buf;
 /* CSOUND  *cs = p->cs; */
    int     i, bytes, n, bufnos = p->bufnos;

    while (*threadon) {
      /* get the data from the socket and store it in a tmp buffer */
      if ((bytes = recvfrom(p->sock, tmp, MTU, 0, &from, &clilen))) {
        while (p->usedbuf[p->wbufferuse] == 1) {
          usleep(1);
        }
        p->wbufferuse++;
        p->wbufferuse = (p->wbufferuse == bufnos ? 0 : p->wbufferuse);
        buf = (MYFLT*) ((char*) p->buffer.auxp + (p->wbufferuse * MTU));
        p->usedbuf[p->wbufferuse] = 1;
        p->bufsamps[p->wbufferuse] = n = bytes / sizeof(MYFLT);
        for (i = 0; i < n; i++)
          buf[i] = tmp[i];
      }
    }
    /* cs->Message(cs, "+++ udp receive thread released.\n"); */
    return (uintptr_t) 0;
}

static int deinit_udpRecvS(CSOUND *csound, void *pdata)
{
    SOCKRECV *p = (SOCKRECV *) pdata;

    p->threadon = 0;
    csound->JoinThread(p->thrid);
    /* csound->Message(csound, "+++ udp receive thread finished.\n"); */
    return OK;
}

static uintptr_t udpRecvS(void *pdata)
{
    struct sockaddr from;
    socklen_t clilen = sizeof(from);
    SOCKRECV *p = (SOCKRECV *) pdata;
    int     *threadon = &p->threadon;
    MYFLT   *tmp = (MYFLT *) p->tmp.auxp;
    MYFLT   *buf;
 /* CSOUND  *cs = p->cs; */
    int     i, bytes, n, bufnos = p->bufnos;

    while (*threadon) {
      /* get the data from the socket and store it in a tmp buffer */
      if ((bytes = recvfrom(p->sock, tmp, MTU, 0, &from, &clilen))) {
        while (p->usedbuf[p->wbufferuse] == 1) {
          usleep(1);
        }
        p->wbufferuse++;
        p->wbufferuse = (p->wbufferuse == bufnos ? 0 : p->wbufferuse);
        buf = (MYFLT*) ((char*) p->buffer.auxp + (p->wbufferuse * MTU));
        p->usedbuf[p->wbufferuse] = 1;
        p->bufsamps[p->wbufferuse] = n = bytes / sizeof(MYFLT);
        for (i = 0; i < n; i++)
          buf[i] = tmp[i];
      }
    }
    /* cs->Message(cs, "+++ udp receive thread released.\n"); */
    return (uintptr_t) 0;
}

/* UDP version one channel */
static int init_recv(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *buf;
    int     bufnos;

    p->wp = 0;
    p->rp = 0;
    p->cs = csound;
    p->bufnos = *p->ibuffnos;
    if (p->bufnos > MAXBUFS)
      p->bufnos = MAXBUFS;
    bufnos = p->bufnos;

    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (p->sock < 0) {
      csound->InitError(csound, "creating socket");
      return NOTOK;
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;  /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int) *p->port);  /* the port */
    /* associate the socket with the address and port */
    if (bind(p->sock, (struct sockaddr *) &p->server_addr,
             sizeof(p->server_addr)) < 0)
      return csound->InitError(csound, "bind failed");

    if (p->buffer.auxp == NULL || (long) (MTU * bufnos) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU * bufnos, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;  /* make sure buffer is empty */
      memset(buf, 0, MTU * bufnos);
    }
    /* create a buffer to store the received interleaved audio data */
    if (p->tmp.auxp == NULL || (long) p->tmp.size < MTU)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->tmp);
    else {
      buf = (MYFLT *) p->tmp.auxp;  /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }

    /* create thread */
    p->thrid = csound->CreateThread(udpRecvS, (void *) p);
    csound->RegisterDeinitCallback(csound, (void *) p, deinit_udpRecvS);
    p->threadon = 1;
    memset(p->usedbuf, 0, bufnos * sizeof(int));
    memset(p->bufsamps, 0, bufnos * sizeof(int));
    p->buf = p->buffer.auxp;
    p->rbufferuse = p->wbufferuse = 0;

    return OK;
}

static int send_recv(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *asigl = p->asigl;
    MYFLT   *buf = p->buf;
    int     i, n, ksmps = csound->ksmps;
    int     *bufsamps = p->bufsamps;
    int     bufnos = p->bufnos;

    for (i = 0, n = p->rp; i < ksmps; i++, n++) {
      if (n == bufsamps[p->rbufferuse]) {
        p->usedbuf[p->rbufferuse] = 0;
        p->rbufferuse++;
        p->rbufferuse = (p->rbufferuse == bufnos ? 0 : p->rbufferuse);
        buf = (MYFLT*) ((char*) p->buffer.auxp + (p->rbufferuse * MTU));
        while (p->usedbuf[p->rbufferuse] == 0) {
          usleep(1);
        }
        n = 0;
      }
      asigl[i] = buf[n];
    }
    p->rp = n;
    p->buf = buf;

    return OK;
}

/* UDP version two channel */
static int init_recvS(CSOUND *csound, SOCKRECVS *p)
{
    MYFLT   *buf;
    int     bufnos;

    p->wp = 0;
    p->rp = 0;
    p->cs = csound;
    p->bufnos = *p->ibuffnos;
    if (p->bufnos > MAXBUFS)
      p->bufnos = MAXBUFS;
    bufnos = p->bufnos;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (p->sock < 0) {
      csound->InitError(csound, "creating socket");
      return NOTOK;
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;  /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int) *p->port);  /* the port */
    /* associate the socket with the address and port */
    if (bind(p->sock, (struct sockaddr *) &p->server_addr,
             sizeof(p->server_addr)) < 0)
      return csound->InitError(csound, "bind failed");

    if (p->buffer.auxp == NULL || (long) (MTU * bufnos) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU * bufnos, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;  /* make sure buffer is empty */
      memset(buf, 0, MTU * bufnos);
    }
    /* create a buffer to store the received interleaved audio data */
    if (p->tmp.auxp == NULL || (long) p->tmp.size < MTU)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->tmp);
    else {
      buf = (MYFLT *) p->tmp.auxp;  /* make sure buffer is empty */
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

    return OK;
}

static int send_recvS(CSOUND *csound, SOCKRECVS *p)
{
    MYFLT   *asigl = p->asigl;
    MYFLT   *asigr = p->asigr;
    MYFLT   *buf = p->buf;
    int     i, n, ksmps = csound->ksmps;
    int     *bufsamps = p->bufsamps;
    int     bufnos = p->bufnos;

    for (i = 0, n = p->rp; i < ksmps; i++, n += 2) {
      if (n == bufsamps[p->rbufferuse]) {
        p->usedbuf[p->rbufferuse] = 0;
        p->rbufferuse++;
        p->rbufferuse = (p->rbufferuse == bufnos ? 0 : p->rbufferuse);
        buf = (MYFLT*) ((char*) p->buffer.auxp + (p->rbufferuse * MTU));
        while (p->usedbuf[p->rbufferuse] == 0) {
          usleep(1);
        }
        n = 0;
      }
      asigl[i] = buf[n];
      asigr[i] = buf[n + 1];
    }
    p->rp = n;
    p->buf = buf;

    return OK;
}

/* TCP version */
static int init_srecv(CSOUND *csound, SOCKRECVT *p)
{
    /* create a STREAM (TCP) socket in the INET (IP) protocol */
    p->sock = socket(PF_INET, SOCK_STREAM, 0);

    if (p->sock < 0) {
      csound->InitError(csound, "creating socket");
      return NOTOK;
    }

    /* create server address: where we want to connect to */

    /* clear it out */
    memset(&(p->server_addr), 0, sizeof(p->server_addr));

    /* it is an INET address */
    p->server_addr.sin_family = AF_INET;

    /* the server IP address, in network byte order */
    inet_aton((const char *) p->ipaddress, &(p->server_addr.sin_addr));

    /* the port we are going to listen on, in network byte order */
    p->server_addr.sin_port = htons((int) *p->port);

again:
    if (connect(p->sock, (struct sockaddr *) &p->server_addr,
                sizeof(p->server_addr)) < 0) {
      if (errno == ECONNREFUSED)
        goto again;
      return csound->InitError(csound, "connect failed");
    }

    return OK;
}

static int send_srecv(CSOUND *csound, SOCKRECVT *p)
{
    int     n = sizeof(MYFLT) * csound->ksmps;

    if (n != read(p->sock, p->asig, n)) {
      csound->Message(csound, "Expected %d got %d\n",
                      (int) (sizeof(MYFLT) * csound->ksmps), n);
      csound->PerfError(csound, "read from socket failed");
      return NOTOK;
    }

    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "sockrecv", S(SOCKRECV), 5, "a", "Sii", (SUBR) init_recv, NULL,
    (SUBR) send_recv },
  { "sockrecvs", S(SOCKRECVS), 5, "aa", "Sii", (SUBR) init_recvS, NULL,
    (SUBR) send_recvS },
  { "strecv", S(SOCKRECVT), 5, "a", "Si", (SUBR) init_srecv, NULL,
    (SUBR) send_srecv }
};

LINKAGE

