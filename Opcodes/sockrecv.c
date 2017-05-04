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
  MYFLT   *asig;
  STRINGDAT *ipaddress;
  MYFLT *port;
  AUXCH   aux, tmp;
  int     sock, conn;
  struct sockaddr_in server_addr;
} SOCKRECVT;

typedef struct {
  OPDS    h;
  STRINGDAT *kstr;
  STRINGDAT *ipaddress;
  MYFLT *port;
  AUXCH   aux, tmp;
  int     sock, conn;
  struct sockaddr_in server_addr;
} SOCKRECVS;

typedef struct {
  OPDS    h;
  /* 1 channel: ptr1=asig, ptr2=port, ptr3=buffnos */
  /* 2 channel: ptr1=asigl, ptr2=asigr, ptr3=port, ptr4=buffnos */
  MYFLT   *ptr1, *ptr2, *ptr3, *ptr4;
  AUXCH   buffer, tmp;
  MYFLT   *buf;
  int     sock;
  volatile int threadon;
  int buffsize;
  int outsamps, rcvsamps;
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
  int     sock;
  volatile int threadon;
  int buffsize;
  int outsamps, rcvsamps;
  CSOUND  *cs;
  void    *thrid;
  void  *cb;
  struct sockaddr_in server_addr;
} SOCKRECVSTR;

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
    int     bytes;
    CSOUND *csound = p->cs;

    while (p->threadon) {
      /* get the data from the socket and store it in a tmp buffer */
      if ((bytes = recvfrom(p->sock, (void *)tmp, MTU, 0, &from, &clilen)) > 0) {
        csound->WriteCircularBuffer(csound, p->cb, tmp, bytes/sizeof(MYFLT));
      }
    }
    return (uintptr_t) 0;
}

static int deinit_udpRecv_S(CSOUND *csound, void *pdata)
{
    SOCKRECV *p = (SOCKRECV *) pdata;

    p->threadon = 0;
    csound->JoinThread(p->thrid);
    return OK;
}

static uintptr_t udpRecv_S(void *pdata)
{
    struct sockaddr from;
    socklen_t clilen = sizeof(from);
    SOCKRECVSTR *p = (SOCKRECVSTR*) pdata;
    char *tmp = (char *) p->tmp.auxp;
    int     bytes;
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
static int init_recv(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *buf;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif

    p->cs = csound;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0))
      return csound->InitError(csound, Str("Cannot set nonblock"));
#endif
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

    if (p->buffer.auxp == NULL || (unsigned long) (MTU) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }
    /* create a buffer to store the received interleaved audio data */
    if (p->tmp.auxp == NULL || (long) p->tmp.size < MTU)
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
static int init_recv_S(CSOUND *csound, SOCKRECVSTR *p)
{
    MYFLT   *buf;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif

    p->cs = csound;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0))
      return csound->InitError(csound, Str("Cannot set nonblock"));
#endif
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

    if (p->buffer.auxp == NULL || (unsigned long) (MTU) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }
    /* create a buffer to store the received string data */
    if (p->tmp.auxp == NULL || (long) p->tmp.size < MTU)
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

static int send_recv_k(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *ksig = p->ptr1;
    *ksig = FL(0.0);
    if(p->outsamps >= p->rcvsamps){
      p->outsamps =  0;
      p->rcvsamps =
        csound->ReadCircularBuffer(csound, p->cb, p->buf, p->buffsize);
    }
    *ksig = p->buf[p->outsamps++];
    return OK;
}

static int send_recv_S(CSOUND *csound, SOCKRECVSTR *p)
{
    STRINGDAT *str = p->ptr1;
    int len;
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
    strncpy(str->data, &p->buf[p->outsamps], len+1);
    p->outsamps += len+1;       /* Move bffer on */
    return OK;
}


static int send_recv(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *asig = p->ptr1;
    MYFLT   *buf = p->buf;
    int     i, nsmps = CS_KSMPS;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int outsamps = p->outsamps, rcvsamps = p->rcvsamps;
    memset(asig, 0, sizeof(MYFLT)*nsmps);
    if (UNLIKELY(early)) nsmps -= early;

    for(i=offset; i < nsmps ; i++){
      if(outsamps >= rcvsamps){
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
static int init_recvS(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *buf;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif

    p->cs = csound;
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
    if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0))
      return csound->InitError(csound, Str("Cannot set nonblock"));
#endif
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

    if (p->buffer.auxp == NULL || (unsigned long) (MTU) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }
    /* create a buffer to store the received interleaved audio data */
    if (p->tmp.auxp == NULL || (long) p->tmp.size < MTU)
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

static int send_recvS(CSOUND *csound, SOCKRECV *p)
{
    MYFLT   *asigl = p->ptr1;
    MYFLT   *asigr = p->ptr2;
    MYFLT   *buf = p->buf;
    int     i, nsmps = CS_KSMPS;
    int outsamps = p->outsamps, rcvsamps = p->rcvsamps;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    memset(asigl, 0, sizeof(MYFLT)*nsmps);
    memset(asigr, 0, sizeof(MYFLT)*nsmps);

    if (UNLIKELY(early)) nsmps -= early;
    for(i=offset; i < nsmps ; i++){
      if(outsamps >= rcvsamps){
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
static int init_srecv(CSOUND *csound, SOCKRECVT *p)
{
    socklen_t clilen;
#if defined(WIN32) && !defined(__CYGWIN__)
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
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data, &(p->server_addr.sin_addr));
#endif
    /* the port we are going to listen on, in network byte order */
    p->server_addr.sin_port = htons((int) *p->port);

    /* associate the socket with the address and port */
    if (UNLIKELY(bind
                 (p->sock, (struct sockaddr *) &p->server_addr, sizeof(p->server_addr))
                 < 0)) {
      return csound->InitError(csound, Str("bind failed"));
    }

    /* start the socket listening for new connections -- may wait */
    if (UNLIKELY(listen(p->sock, 5) < 0)) {
      return csound->InitError(csound, Str("listen failed"));
    }
    clilen = sizeof(p->server_addr);
    p->conn = accept(p->sock, (struct sockaddr *) &p->server_addr, &clilen);

    if (UNLIKELY(p->conn < 0)) {
      return csound->InitError(csound, Str("accept failed"));
    }
    return OK;
}

static int send_srecv(CSOUND *csound, SOCKRECVT *p)
{
    int     n = sizeof(MYFLT) * CS_KSMPS;

    if (UNLIKELY(n != read(p->conn, p->asig, sizeof(MYFLT) * CS_KSMPS))) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("read from socket failed"));
    }
    return OK;
}


typedef struct _rawosc {
  OPDS h;
  ARRAYDAT *sout;
  MYFLT *kflag;
  MYFLT  *port;
  AUXCH   buffer;
  int     sock;
  /*
    AUXCH tmp;
    volatile int threadon;
    CSOUND  *cs;
    void    *thrid;
    void  *cb;
  */
  struct sockaddr_in server_addr;
} RAWOSC;


static inline void tabensure(CSOUND *csound, ARRAYDAT *p, int size)
{
    if (p->data==NULL || p->dimensions == 0 ||
        (p->dimensions==1 && p->sizes[0] < size)) {
      size_t ss;
      if (p->data == NULL) {
        CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
        p->arrayMemberSize = var->memBlockSize;
      }

      ss = p->arrayMemberSize*size;
      if (p->data==NULL) p->data = (MYFLT*)csound->Calloc(csound, ss);
      else p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
      p->dimensions = 1;
      p->sizes = (int*)csound->Malloc(csound, sizeof(int));
      p->sizes[0] = size;
    }
}


static int init_raw_osc(CSOUND *csound, RAWOSC *p)
{
    MYFLT   *buf;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
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
      return csound->InitError
        (csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int) *p->port);    /* the port */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                      sizeof(p->server_addr)) < 0))
      return csound->InitError(csound, Str("bind failed"));

    if (p->buffer.auxp == NULL || (unsigned long) (MTU) > p->buffer.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, MTU, &p->buffer);
    else {
      buf = (MYFLT *) p->buffer.auxp;   /* make sure buffer is empty */
      memset(buf, 0, MTU);
    }

  tabensure(csound, p->sout,2);
  return OK;
}

static inline char le_test(){
    union _le {
      char c[2];
      short s;
    } le = {{0x0001}};
    return le.c[0];
}

static inline char *byteswap(char *p, int N){
    if(le_test()) {
      char tmp;
      int j ;
      for(j = 0; j < N/2; j++) {
        tmp = p[j];
        p[j] = p[N - j - 1];
        p[N - j - 1] = tmp;
      }
    }
    return p;
}

static int perf_raw_osc(CSOUND *csound, RAWOSC *p) {

    ARRAYDAT *sout = p->sout;
    if(sout->sizes[0] < 2 ||
       sout->dimensions > 1)
      return
        csound->PerfError(csound, p->h.insdshead, Str("output array too small\n"));

    STRINGDAT *str = (STRINGDAT *) p->sout->data;
    char *buf = (char *) p->buffer.auxp;
    int len = 0, n = 0, j = 1;
    char c;
    memset(buf, 0, p->buffer.size);
    uint32_t size = 0;
    char *types;
    struct sockaddr from;
    socklen_t clilen = sizeof(from);
    int bytes =
      recvfrom(p->sock, (void *)buf, MTU, 0, &from, &clilen);
    if(bytes < 0) bytes = 0;

    if(bytes) {
      if(strncmp(buf,"#bundle",7) == 0) { // bundle
        buf += 8;
        buf += 8;
        size = *((uint32_t *) buf);
        byteswap((char *)&size, 4);
        buf += 4;
      } else size = bytes;
      while(size > 0)  {
        /* get address & types */
        if(n < sout->sizes[0]) {
          len = strlen(buf);
          // printf("len %d size %d incr %d\n",
          //        len, str[n].size, ((size_t) ceil((len+1)/4.)*4));
          if(len >= str[n].size) {
            str[n].data = csound->ReAlloc(csound, str[n].data, len+1);
            memset(str[n].data,0,len+1);
            str[n].size  = len+1;
          }
          strncpy(str[n].data, buf, len);
          str[n].data[len] = '\0'; // explicitly terminate it.
          n++;
          buf += ((size_t) ceil((len+1)/4.)*4);
        }
        if (n < sout->sizes[0]) {
          len = strlen(buf);
          if(len >= str[n].size) {
            str[n].data = csound->ReAlloc(csound, str[n].data, len+1);
            str[n].size  = len+1;
          }
          strncpy(str[n].data, buf, len);
          str[n].data[str[n].size-1] = '\0'; // explicitly terminate it.
          types = str[n].data;
          n++;
          buf += ((size_t) ceil((len+1)/4.)*4);
        }
        j = 1;
        // parse data
        while((c = types[j++]) != '\0' && n < sout->sizes[0]){
          if(c == 'f') {
            float f = *((float *) buf);
            byteswap((char*)&f,4);
            if(str[n].size < 32) {
              str[n].data = csound->ReAlloc(csound, str[n].data, 32);
              str[n].size  = 32;
            }
            snprintf(str[n].data, str[n].size, "%f", f);
            buf += 4;
          } else if (c == 'i') {
            int d = *((int32_t *) buf);
            byteswap((char*) &d,4);
            if(str[n].size < 32) {
              str[n].data = csound->ReAlloc(csound, str[n].data, 32);
              str[n].size  = 32;
            }
            snprintf(str[n].data, str[n].size, "%d", d);
            buf += 4;
          } else if (c == 's') {
            len = strlen(buf);
            if(len > str[n].size) {
              str[n].data = csound->ReAlloc(csound, str[n].data, len+1);
              str[n].size  = len+1;
            }
            strncpy(str[n].data, buf, len);
            str[n].data[len] = '\0';
            len = ceil((len+1)/4.)*4;
            buf += len;
          } else if (c == 'b') {
            len = *((uint32_t *) buf);
            byteswap((char*)&len,4);
            len = ceil((len)/4.)*4;
            if(len > str[n].size) {
              str[n].data = csound->ReAlloc(csound, str[n].data, len+1);
              str[n].size  = len+1;
            }
            strncpy(str[n].data, buf, len);
            str[n].data[len] = '\0';
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
  { "sockrecv", S(SOCKRECV), 0, 7, "s", "ii", (SUBR) init_recv, (SUBR) send_recv_k,
    (SUBR) send_recv, NULL },
  { "sockrecv", S(SOCKRECVSTR), 0, 3, "S", "ii", (SUBR) init_recv_S,
    (SUBR) send_recv_S, NULL },
  { "sockrecvs", S(SOCKRECV), 0, 5, "aa", "ii", (SUBR) init_recvS, NULL,
    (SUBR) send_recvS, NULL },
  { "strecv", S(SOCKRECVT), 0, 5, "a", "Si", (SUBR) init_srecv, NULL,
    (SUBR) send_srecv, NULL },
  { "OSCraw", S(RAWOSC), 0, 3, "S[]k", "i",
    (SUBR) init_raw_osc, (SUBR) perf_raw_osc, NULL, NULL}
};

LINKAGE_BUILTIN(sockrecv_localops)
