/*
  socksend.c:

  Copyright (C) 2006 by John ffitch
                2018 by Victor Lazzarini

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
#include "arrays.h"
#include <sys/types.h>
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET_ERROR (-1)
#endif
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

extern  int32_t     inet_aton(const char *cp, struct in_addr *inp);

typedef struct {
  OPDS    h;
  cs_float   *asig;
  STRINGDAT *ipaddress;
  cs_float *port, *buffersize;
  cs_float   *format;
  AUXCH   aux;
  int32_t     sock, init_done;
  int32_t     bsize, wp;
  int32_t     ff, bwidth;
  struct sockaddr_in server_addr;
} SOCKSEND;

typedef struct {
  OPDS    h;
  STRINGDAT *str;
  STRINGDAT *ipaddress;
  cs_float *port, *buffersize;
  cs_float   *format;
  AUXCH   aux;
  int32_t     sock, init_done;
  int32_t     bsize, wp;
  int32_t     ff, bwidth;
  struct sockaddr_in server_addr;
} SOCKSENDT;

typedef struct {
  OPDS    h;
  cs_float   *asigl, *asigr;
  STRINGDAT *ipaddress;
  cs_float *port, *buffersize;
  cs_float   *format;
  AUXCH   aux;
  int32_t     sock, init_done;
  int32_t     bsize, wp;
  int32_t     ff, bwidth;
  struct sockaddr_in server_addr;
} SOCKSENDS;

#define UDP_MAX_PAYLOAD 65507

/* Store signed PCM16 in little-endian order without per-sample calls. */
#define SOCKSEND_PCM16(DEST, SAMPLE, SCALE) do {                         \
    cs_double pcm_value = (SAMPLE) * (SCALE);                              \
    int32_t pcm_int = pcm_value >= 32767.0 ? 32767 :                    \
                      pcm_value <= -32768.0 ? -32768 : (int32_t)pcm_value; \
    unsigned char *pcm_dest = (unsigned char *)(DEST);                  \
    pcm_dest[0] = (unsigned char)pcm_int;                               \
    pcm_dest[1] = (unsigned char)((uint16_t)pcm_int >> 8);               \
  } while (0)

static int32_t close_udp_socket(int32_t sock, int32_t *init_done)
{
    if (!*init_done) return OK;
    *init_done = 0;
#if defined(WIN32) && !defined(__CYGWIN__)
    int32_t result = closesocket((SOCKET)sock);
    WSACleanup();
#else
    int32_t result = close(sock);
#endif
    return result;
}

static int32_t socksend_deinit(CSOUND *csound, SOCKSEND *p)
{
    IGN(csound);
    return close_udp_socket(p->sock, &p->init_done);
}

static int32_t socksends_deinit(CSOUND *csound, SOCKSENDS *p)
{
    IGN(csound);
    return close_udp_socket(p->sock, &p->init_done);
}

/* UDP version one channel */
static int32_t init_send_common(CSOUND *csound, SOCKSEND *p, int32_t isString)
{
    int32_t bwidth, bsize;
    p->ff = (int32_t)*p->format;
    bwidth = isString ? 1 : (p->ff ? sizeof(int16) : sizeof(cs_float));
    if (!(*p->buffersize >= FL(1.0) &&
          *p->buffersize <= UDP_MAX_PAYLOAD / bwidth))
      return csound->InitError(csound, "%s", Str("socksend: invalid buffer length"));
    bsize = (int32_t)*p->buffersize;
    p->bsize = bsize;
    p->wp = 0;
    if (!p->init_done) {
#if defined(WIN32) && !defined(__CYGWIN__)
      WSADATA wsaData = {0};
      int32_t err = WSAStartup(MAKEWORD(2,2), &wsaData);
      if (UNLIKELY(err != 0))
        return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
      p->sock = socket(AF_INET, SOCK_DGRAM, 0);
      if (UNLIKELY(p->sock == SOCKET_ERROR)) {
#if defined(WIN32) && !defined(__CYGWIN__)
        WSACleanup();
#endif
        return csound->InitError(csound, "%s", Str("creating socket"));
      }
      p->init_done = 1;
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data,
              &p->server_addr.sin_addr);    /* the server IP address */
#endif
    p->server_addr.sin_port = htons((int32_t) *p->port);    /* the port */

    /* create a buffer to write the interleaved audio to  */
    if (p->aux.auxp == NULL || (uint32_t) (bsize * bwidth) > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, (bsize * bwidth), &p->aux);
    else {
      memset(p->aux.auxp, 0, bwidth * bsize);
    }
    p->bwidth = bwidth;
    return OK;
}

static int32_t init_send(CSOUND *csound, SOCKSEND *p)
{
    return init_send_common(csound, p, 0);
}

static int32_t init_send_Str(CSOUND *csound, SOCKSENDT *p)
{
    return init_send_common(csound, (SOCKSEND *)p, 1);
}

static int32_t send_send(CSOUND *csound, SOCKSEND *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t     wp;
    int32_t     buffersize = p->bsize;
    cs_float   *asig = p->asig;
    cs_float   *out = (cs_float *) p->aux.auxp;
    int16   *outs = (int16 *) p->aux.auxp;
    int32_t     ff = p->ff;

    cs_double scale = ff ? 32768.0 / csound->Get0dBFS(csound) : 0;
    if (UNLIKELY(early)) nsmps -= early;
    for (i = offset, wp = p->wp; i < nsmps; i++) {
      if (ff)
        SOCKSEND_PCM16(&outs[wp], asig[i], scale);
      else
        out[wp] = asig[i];
      if (++wp == buffersize) {
        if (UNLIKELY(sendto(p->sock, (void*)out, buffersize * p->bwidth, 0, to,
                            sizeof(p->server_addr)) == SOCKET_ERROR))
          return csound->PerfError(csound, &(p->h), "%s", Str("sendto failed"));
        wp = 0;
      }
    }
    p->wp = wp;

    return OK;
}

static int32_t send_send_k(CSOUND *csound, SOCKSEND *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);

    int32_t     buffersize = p->bsize;
    cs_float   *ksig = p->asig;
    cs_float   *out = (cs_float *) p->aux.auxp;
    int16   *outs = (int16 *) p->aux.auxp;
    int32_t     ff = p->ff;


    if (ff) {
      cs_double scale = 32768.0 / csound->Get0dBFS(csound);
      SOCKSEND_PCM16(&outs[p->wp], *ksig, scale);
    }
    else
      out[p->wp] = *ksig;
    if (++p->wp == buffersize) {
      if (UNLIKELY(sendto(p->sock, (void*)out, buffersize * p->bwidth, 0, to,
                          sizeof(p->server_addr)) == SOCKET_ERROR))
        return csound->PerfError(csound, &(p->h), "%s", Str("sendto failed"));
      p->wp = 0;
    }

    return OK;
}

static int32_t send_send_Str(CSOUND *csound, SOCKSENDT *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);

    int32_t     buffersize = p->bsize;
    char    *out = (char *) p->aux.auxp;
    char    *q = p->str->data;
    size_t     len = q != NULL ? strlen(q) : 0;

    if (UNLIKELY(len >= (size_t) buffersize)) {
      csound->Warning(csound, "%s", Str("string truncated in socksend"));
      len = buffersize-1;
    }
    if (len != 0) memcpy(out, q, len);
    memset(out+len, 0, buffersize-len);
    /* send the package with the string each time */
    if (UNLIKELY(sendto(p->sock, (void*)out, buffersize, 0, to,
                        sizeof(p->server_addr)) ==SOCKET_ERROR)) {
      return csound->PerfError(csound, &(p->h), "%s", Str("sendto failed"));
    }
    return OK;
}



/* UDP version 2 channels */
static int32_t init_sendS(CSOUND *csound, SOCKSENDS *p)
{
    int32_t bwidth, bsize;
    p->ff = (int32_t)*p->format;
    bwidth = p->ff ? sizeof(int16) : sizeof(cs_float);
    if (!(*p->buffersize >= FL(1.0) &&
          *p->buffersize <= UDP_MAX_PAYLOAD / bwidth))
      return csound->InitError(csound, "%s", Str("socksend: invalid buffer length"));
    bsize = (int32_t)*p->buffersize;
    if (bsize < 2 || (bsize & 1))
      return csound->InitError(csound, "%s",
                               Str("socksends: buffer length must be even"));
    p->bsize = bsize;
    p->wp = 0;
    if (!p->init_done) {
#if defined(WIN32) && !defined(__CYGWIN__)
      WSADATA wsaData = {0};
      int32_t err = WSAStartup(MAKEWORD(2,2), &wsaData);
      if (UNLIKELY(err != 0))
        return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
      p->sock = socket(AF_INET, SOCK_DGRAM, 0);
      if (UNLIKELY(p->sock == SOCKET_ERROR)) {
#if defined(WIN32) && !defined(__CYGWIN__)
        WSACleanup();
#endif
        return csound->InitError(csound, "%s", Str("creating socket"));
      }
      p->init_done = 1;
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data,
              &p->server_addr.sin_addr);    /* the server IP address */
#endif
    p->server_addr.sin_port = htons((int32_t) *p->port);    /* the port */

    /* create a buffer to write the interleaved audio to */
    if (p->aux.auxp == NULL || (uint32_t) (bsize * bwidth) > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, (bsize * bwidth), &p->aux);
    else {
      memset(p->aux.auxp, 0, bwidth * bsize);
    }
    p->bwidth = bwidth;
    return OK;
}

static int32_t send_sendS(CSOUND *csound, SOCKSENDS *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);
    cs_float   *asigl = p->asigl;
    cs_float   *asigr = p->asigr;
    cs_float   *out = (cs_float *) p->aux.auxp;
    int16   *outs = (int16 *) p->aux.auxp;
    int32_t     wp;
    int32_t     buffersize = p->bsize;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t     ff = p->ff;

    cs_double scale = ff ? 32768.0 / csound->Get0dBFS(csound) : 0;
    if (UNLIKELY(early)) nsmps -= early;
    for (i = offset, wp = p->wp; i < nsmps; i++) {
      if (ff) {
        SOCKSEND_PCM16(&outs[wp], asigl[i], scale);
        SOCKSEND_PCM16(&outs[wp + 1], asigr[i], scale);
      }
      else {
        out[wp] = asigl[i];
        out[wp + 1] = asigr[i];
      }
      wp += 2;
      if (wp == buffersize) {
        if (UNLIKELY(sendto(p->sock, (void*)out, buffersize * p->bwidth, 0, to,
                            sizeof(p->server_addr)) == SOCKET_ERROR))
          return csound->PerfError(csound, &(p->h), "%s", Str("sendto failed"));
        wp = 0;
      }
    }
    p->wp = wp;

    return OK;
}

/* TCP version */

typedef struct {
  OPDS h;
  cs_float *asig;
  STRINGDAT *ipaddress;
  cs_float *port;
#if defined(WIN32) && !defined(__CYGWIN__)
  SOCKET sock;
#else
  int32_t sock;
#endif
  int32_t init_done;
} STSEND;

static int32_t stsend_deinit(CSOUND *csound, STSEND *p)
{
    IGN(csound);
    if (!p->init_done)
      return OK;
#if defined(WIN32) && !defined(__CYGWIN__)
    if (p->sock != INVALID_SOCKET) closesocket(p->sock);
    WSACleanup();
#else
    if (p->sock != SOCKET_ERROR) close(p->sock);
#endif
    p->sock = SOCKET_ERROR;
    p->init_done = 0;
    return OK;
}

static int32_t init_ssend(CSOUND *csound, STSEND *p)
{
    int32_t err;
    const char *message;
    struct sockaddr_in address;
    stsend_deinit(csound, p);
    p->sock = SOCKET_ERROR;
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
    p->init_done = 1;
    /* create a STREAM (TCP) socket in the INET (IP) protocol */
    p->sock = socket(PF_INET, SOCK_STREAM, 0);

    if (UNLIKELY(p->sock == SOCKET_ERROR)) {
      message = Str("creating socket");
      goto error;
    }
#ifdef SO_NOSIGPIPE
    {
      int enabled = 1;
      if (setsockopt(p->sock, SOL_SOCKET, SO_NOSIGPIPE,
                     &enabled, sizeof(enabled)) == SOCKET_ERROR) {
        message = Str("setting socket option");
        goto error;
      }
    }
#endif
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
#if defined(WIN32) && !defined(__CYGWIN__)
    address.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data, &address.sin_addr);
#endif
    address.sin_port = htons((int32_t) *p->port);
    if (UNLIKELY(connect(p->sock, (struct sockaddr *) &address,
                          sizeof(address)) == SOCKET_ERROR)) {
      message = Str("connect failed");
      goto error;
    }
    return OK;

 error:
#if defined(WIN32) && !defined(__CYGWIN__)
    err = WSAGetLastError();
#else
    err = errno;
#endif
    stsend_deinit(csound, p);
    return csound->InitError(csound, "%s (%d)", message, err);
}

static int32_t send_ssend(CSOUND *csound, STSEND *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t remaining = sizeof(cs_float) * (CS_KSMPS-offset-early);
    const char *data = (const char *) &p->asig[offset];
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    while (remaining > 0) {
      int32_t sent = (int32_t) send(p->sock, data, remaining, flags);
      if (UNLIKELY(sent <= 0)) {
        if (sent < 0) {
#if defined(WIN32) && !defined(__CYGWIN__)
          if (WSAGetLastError() == WSAEINTR) continue;
#else
          if (errno == EINTR) continue;
#endif
        }
        stsend_deinit(csound, p);
        return csound->PerfError(csound, &p->h,
                                 "%s", Str("write to socket failed"));
      }
      data += sent;
      remaining -= sent;
    }
    return OK;
}


typedef struct {
  OPDS h;
  cs_float *kwhen;
  STRINGDAT *ipaddress;
  cs_float *port;        /* UDP port */
  STRINGDAT *dest;
  STRINGDAT *type;
  cs_float *arg[32];     /* only 26 can be used, but add a few more for safety */
  AUXCH   aux;
  AUXCH   types;
#if defined(WIN32) && !defined(__CYGWIN__)
  SOCKET sock;
#else
  int32_t sock;
#endif
  int32_t ntypes;
  cs_float   last;
  struct sockaddr_in server_addr;
  int32_t err_state;
  int32_t init_done;
  int32_t fstime;
} OSCSEND2;

static int32_t osc_array_blob_sizes(const ARRAYDAT *array, int32_t shaped,
                                    size_t *valueBytes, size_t *blobBytes)
{
    size_t elements;
    size_t headerBytes = 0;

    if (valueBytes == NULL || blobBytes == NULL) {
      return NOTOK;
    }
    *valueBytes = 0;
    *blobBytes = 0;
    if (array == NULL || array->dimensions <= 0 ||
        csound_array_member_count(array, &elements) != OK ||
        elements > SIZE_MAX / sizeof(cs_float) ||
        (elements != 0 && array->data == NULL)) {
      return NOTOK;
    }
    *valueBytes = elements * sizeof(cs_float);
    if (shaped) {
      if ((size_t)array->dimensions >
          (SIZE_MAX - sizeof(int32_t)) / sizeof(int32_t)) {
        return NOTOK;
      }
      headerBytes = sizeof(int32_t) +
        (size_t)array->dimensions * sizeof(int32_t);
    }
    if (*valueBytes > SIZE_MAX - headerBytes) {
      return NOTOK;
    }
    *blobBytes = headerBytes + *valueBytes;
    return *blobBytes <= INT32_MAX ? OK : NOTOK;
}

static int32_t oscsend_deinit(CSOUND *csound, OSCSEND2 *p)
{
    if (!p->init_done) return OK;
    p->init_done = 0;
#if defined(WIN32) && !defined(__CYGWIN__)
    closesocket(p->sock);
    WSACleanup();
#else
    close(p->sock);
#endif
    return OK;
}


static int32_t osc_send2_init(CSOUND *csound, OSCSEND2 *p)
{
    size_t     bsize;

    if (p->init_done) {
      csound->Warning(csound, "already initialised");
      return OK;
    }

    if(p->INOCOUNT > 4) {
      if(!IS_STR_ARG(p->type))
               return csound->InitError(csound,
                             "%s", Str("Message type is not given as a string\n"));
    }


    p->ntypes = 0;
    if (p->INOCOUNT > 4) {
      size_t ntypes = strlen(p->type->data), nargs = 0;
      for (size_t i = 0; i < ntypes; i++)
        nargs += p->type->data[i] == 't' ? 2 : 1;
      if (UNLIKELY(nargs > p->INOCOUNT - 5))
        return csound->InitError(csound, "%s",
                                Str("insufficient number of arguments for "
                                    "OSC message types\n"));
      p->ntypes = (int32_t)ntypes;
    }

    if(p->INOCOUNT > 4) {
      int32_t i, iarg;
    STRINGDAT *s;
    ARRAYDAT *ar;
    FUNC *ft;
    if (p->ntypes > 0) {
      if (p->types.auxp == NULL || (size_t)p->ntypes > p->types.size)
        csound->AuxAlloc(csound, p->ntypes, &p->types);
      memcpy(p->types.auxp, p->type->data, p->ntypes);
    }

    bsize = 0;
    for(i=0,iarg=0; i < p->ntypes; i++) {
      switch(p->type->data[i]){
      case 't':
        bsize += 8;
        iarg+=2;
        break;
      case 'f':
      case 'i':
      case 'c':
      case 'm':
        bsize += 4;
        iarg++;
        break;
      case 's':
        if (UNLIKELY(!IS_STR_ARG(p->arg[iarg])))
          return csound->InitError(csound, "%s", Str("expecting a string argument\n"));
        s = (STRINGDAT *)p->arg[iarg];
        bsize += strlen(s->data) + 64;
        iarg++;
        break;
      case 'l':
      case 'h': /* OSC type tag for a 64-bit integer */
        ((char *)p->types.auxp)[i] = 'h';
        /* fall through */
      case 'd':
        bsize += 8;
        iarg++;
        break;
      case 'b': case 'T': case 'F': case 'I': case 'N':
        iarg++;
        break;
      case 'a':
        bsize += (sizeof(cs_float)*CS_KSMPS);
        iarg++;
        break;
      case 'G':
        ft = csound->FTFind(csound, p->arg[iarg]);
        if (UNLIKELY(ft == NULL))
          return csound->InitError(csound, "%s",
                                   Str("ftable not found for OSC message\n"));
        bsize += (sizeof(cs_float)*ft->flen);
        iarg++;
        break;
      case 'A':
      case 'D': {
        size_t valueBytes;
        size_t blobBytes;
        ar = (ARRAYDAT *) p->arg[iarg];
        if (UNLIKELY(osc_array_blob_sizes(
                       ar, p->type->data[i] == 'A',
                       &valueBytes, &blobBytes) != OK ||
                     bsize > SIZE_MAX - sizeof(int32_t) ||
                     blobBytes >
                       SIZE_MAX - bsize - sizeof(int32_t))) {
          return csound->InitError(
            csound, "%s", Str("OSC array payload is invalid or too large\n"));
        }
        bsize += sizeof(int32_t) + blobBytes;
        iarg++;
        break;
      }
      default:
        return csound->InitError(csound, Str("%c: data type not supported\n"),
                                 p->type->data[i]);
      }
    }

    bsize += (strlen(p->dest->data) + strlen(p->type->data) + 11);
    bsize *= 2;
    if (p->aux.auxp == NULL || bsize > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, bsize, &p->aux);
    else {
      memset(p->aux.auxp, 0, bsize);
    }
    } else {
      bsize = strlen(p->dest->data)+1;
      bsize = ceil(bsize/4.)*4;
      bsize += 8;
    if (p->aux.auxp == NULL || bsize > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, bsize, &p->aux);
    else {
      memset(p->aux.auxp, 0, bsize);
    }
    }

    /* Open the socket only after all argument checks have passed. */
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock == SOCKET_ERROR)) {
#if defined(WIN32) && !defined(__CYGWIN__)
      WSACleanup();
#endif
      return csound->InitError(csound, "%s", Str("creating socket"));
    }
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.s_addr = inet_addr(
      strcmp(p->ipaddress->data, "localhost") ? p->ipaddress->data : "127.0.0.1");
#else
    inet_aton(strcmp(p->ipaddress->data, "localhost") ?
              p->ipaddress->data : "127.0.0.1", &p->server_addr.sin_addr);
#endif
    p->server_addr.sin_port = htons((int32_t)*p->port);

    p->last = FL(0.0);
    p->err_state = 0;
    p->init_done = 1;
    p->fstime = 1;
    return OK;
}

static inline size_t aux_realloc(CSOUND *csound, size_t size, AUXCH *aux) {
    char *p = (char *)aux->auxp;
    aux->auxp = csound->ReAlloc(csound, p, size);
    aux->size = size;
    aux->endp = (char*)aux->auxp + size;
    return size;
}

static int32_t osc_send2(CSOUND *csound, OSCSEND2 *p)
{
    if(*p->kwhen != p->last || p->fstime) {
      const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);

      int32_t buffersize = 0, i, iarg, size;
      const char *types = (const char *)p->types.auxp;
      size_t bsize = p->aux.size;
      char *out = (char *) p->aux.auxp;
      size_t needed;
      p->fstime = 0;

      /* the destination string may have grown since init, so make sure
         the buffer holds it and the type block before copying */
      size = (int32_t) strlen(p->dest->data)+1;
      needed = (size_t) (ceil(size/4.)*4) + p->types.size + 16;
      if (needed > bsize) {
        aux_realloc(csound, needed + 128, &p->aux);
        out = (char *) p->aux.auxp;
        bsize = p->aux.size;
      }
      memset(out,0,bsize);
      /* package destination in 4-byte zero-padded block */
      memcpy(out,p->dest->data,size);
      size = ceil(size/4.)*4;
      buffersize += size;
      if(p->INOCOUNT > 4) {
      /* package type in a 4-byte zero-padded block;
         add a comma to the beginning of the type string.
      */
      out[buffersize] = ',';
      /* Keep the init-time types separate from the outgoing OSC tags. */
      size = p->ntypes + 1;
      if (p->ntypes > 0)
        memcpy(out+buffersize+1, types, p->ntypes);
      /* OSC booleans and blobs use different tags from the input types. */
      for(i = 0, iarg = 0; i < p->ntypes; i++) {
        if(types[i] == 'b') {
          out[buffersize+1+i] = *p->arg[iarg] == FL(0.0) ? 'F' : 'T';
        }
        else if(types[i] == 'D' || types[i] == 'A' ||
                types[i] == 'G' || types[i] == 'a')
          out[buffersize+1+i] = 'b';
        iarg += types[i] == 't' ? 2 : 1;
      }
      size = ceil((size+1)/4.)*4;
      buffersize += size;
      /* add data to message */
      float fdata;
      double ddata; /* OSC type d is always 64 bits. */
      cs_float mdata;
      int32_t data;
      int64_t ldata;
      uint64_t udata;
      STRINGDAT *s;
      ARRAYDAT *ar;
      FUNC *ft;
      for(i = 0, iarg = 0; i < p->ntypes; i++, iarg++) {
        switch(types[i]){
        case 'f':
          /* realloc if necessary */
          if((size_t) buffersize + 4 > bsize) {
            aux_realloc(csound, buffersize + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          fdata = (float) *p->arg[iarg];
          byteswap((char *) &fdata, 4);
          memcpy(out+buffersize,&fdata, 4);
          buffersize += 4;
          break;
        case 'd':
          /* realloc if necessary */
          if((size_t) buffersize + 8 > bsize) {
            aux_realloc(csound, buffersize + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          ddata = *p->arg[iarg];
          byteswap((char *) &ddata, 8);
          memcpy(out+buffersize,&ddata, 8);
          buffersize += 8;
          break;
        case 't':
          /* realloc if necessary */
          if((size_t) buffersize + 8 > bsize) {
            aux_realloc(csound, buffersize + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          /* Each half is unsigned 32-bit, so round through a wider integer. */
          udata = (uint64_t)(uint32_t)llrint((cs_double)*p->arg[iarg]);
          iarg++;
          udata <<= 32;
          udata |= (uint32_t)llrint((cs_double)*p->arg[iarg]);
          byteswap((char *) &udata, 8);
          memcpy(out+buffersize,&udata, 8);
          buffersize += 8;
          break;
        case 'i':
        case 'm':
        case 'c':
          /* realloc if necessary */
          if((size_t) buffersize + 4 > bsize) {
            aux_realloc(csound, buffersize + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          data = CS_FLOAT2LRND(*p->arg[iarg]);
          byteswap((char *) &data, 4);
          memcpy(out+buffersize,&data, 4);
          buffersize += 4;
          break;
        case 'h':
          /* realloc if necessary */
          if((size_t) buffersize + 8 > bsize) {
            aux_realloc(csound, buffersize + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          ldata = (int64_t)llrint((cs_double)*p->arg[iarg]);
          byteswap((char *) &ldata, 8);
          memcpy(out+buffersize,&ldata, 8);
          buffersize += 8;
          break;
        case 's':
          s = (STRINGDAT *)p->arg[iarg];
          size = (int32_t) strlen(s->data)+1;
          size = ceil(size/4.)*4;
          /* realloc if necessary */
          if((size_t) buffersize + size > bsize) {
            aux_realloc(csound, buffersize + size + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          memcpy(out+buffersize, s->data, strlen(s->data)+1);
          buffersize += size;
          break;
        case 'G':
          ft = csound->FTFind(csound, p->arg[iarg]);
          if (UNLIKELY(ft == NULL))
            return csound->PerfError(csound, &(p->h), "%s",
                                     Str("ftable not found for OSC message\n"));
          size = (int32_t)(sizeof(cs_float)*ft->flen);
          if((size_t) buffersize + size + 4 > bsize) {
            aux_realloc(csound, buffersize + size + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          data = size;
          byteswap((char *)&data,4);
          memcpy(out+buffersize,&data,4);
          buffersize += 4;
          memcpy(out+buffersize,ft->ftable,size);
          buffersize += size;
          break;
        case 'A': {
          size_t valueBytes;
          size_t blobBytes;
          size_t requiredBytes;
          size_t offset;
          size_t shapeBytes;
          ar = (ARRAYDAT *) p->arg[iarg];
          if (UNLIKELY(osc_array_blob_sizes(
                         ar, 1, &valueBytes, &blobBytes) != OK ||
                       (size_t)buffersize >
                         SIZE_MAX - sizeof(int32_t) - blobBytes)) {
            return csound->PerfError(
              csound, &(p->h), "%s",
              Str("OSC array payload is invalid or too large\n"));
          }
          requiredBytes =
            (size_t)buffersize + sizeof(int32_t) + blobBytes;
          if (UNLIKELY(requiredBytes > INT32_MAX)) {
            return csound->PerfError(
              csound, &(p->h), "%s", Str("OSC message is too large\n"));
          }
          if (requiredBytes > bsize) {
            aux_realloc(csound, requiredBytes + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          /* blob length: dimension count field, the sizes, then the values */
          data = (int32_t)blobBytes;
          byteswap((char *)&data,4);
          offset = (size_t)buffersize;
          memcpy(out + offset, &data, sizeof(data));
          offset += sizeof(data);
          memcpy(out + offset, &(ar->dimensions), sizeof(ar->dimensions));
          offset += sizeof(ar->dimensions);
          shapeBytes = (size_t)ar->dimensions * sizeof(int32_t);
          memcpy(out + offset, ar->sizes, shapeBytes);
          offset += shapeBytes;
          if (valueBytes != 0)
            memcpy(out + offset, ar->data, valueBytes);
          buffersize = (int32_t)requiredBytes;
          break;
        }
        case 'D': {
          size_t valueBytes;
          size_t blobBytes;
          size_t requiredBytes;
          size_t offset;
          ar = (ARRAYDAT *) p->arg[iarg];
          if (UNLIKELY(osc_array_blob_sizes(
                         ar, 0, &valueBytes, &blobBytes) != OK ||
                       (size_t)buffersize >
                         SIZE_MAX - sizeof(int32_t) - blobBytes)) {
            return csound->PerfError(
              csound, &(p->h), "%s",
              Str("OSC array payload is invalid or too large\n"));
          }
          requiredBytes =
            (size_t)buffersize + sizeof(int32_t) + blobBytes;
          if (UNLIKELY(requiredBytes > INT32_MAX)) {
            return csound->PerfError(
              csound, &(p->h), "%s", Str("OSC message is too large\n"));
          }
          if (requiredBytes > bsize) {
            aux_realloc(csound, requiredBytes + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          data = (int32_t)blobBytes;
          byteswap((char *)&data,4);
          offset = (size_t)buffersize;
          memcpy(out + offset, &data, sizeof(data));
          offset += sizeof(data);
          if (valueBytes != 0)
            memcpy(out + offset, ar->data, valueBytes);
          buffersize = (int32_t)requiredBytes;
          break;
        }
        case 'a':
          size = (int32_t) (CS_KSMPS+1)*sizeof(cs_float);
          if((size_t) buffersize + size + 4 > bsize) {
            aux_realloc(csound, buffersize + size + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          data = size;
          byteswap((char *)&data,4);
          memcpy(out+buffersize,&data,4);
          buffersize += 4;
          mdata = CS_KSMPS;
          memcpy(out+buffersize,&mdata,sizeof(cs_float));
          memcpy(out+buffersize+sizeof(cs_float),p->arg[iarg],CS_KSMPS*sizeof(cs_float));
          buffersize += size;
          break;
        case 'T':
        case 'F':
        case 'I':
        case 'N':
        default:
          break;
        }
      }
      } else {
        out[buffersize] = ',';
        memset(out+buffersize+1, 0, 3);
        buffersize += 4;
      }
      if (UNLIKELY(sendto(p->sock, (void*)out, buffersize, 0, to,
                          sizeof(p->server_addr)) < 0)) {
        if(p->err_state == 0) {
          csound->Warning(csound, Str("OSCsend failed to send "
                                      "message with destination %s to %s:%d\n"),
                                      p->dest->data, p->ipaddress->data,
                          (int32_t) *p->port);
        }
        p->err_state = 1;
        return OK;
      } 
      p->last = *p->kwhen;
    }
    p->err_state = 0;
    return OK;
}

#define MAX_PACKET_SIZE 65536

typedef struct {
  OPDS h;
  cs_float *kwhen;
  STRINGDAT *ipaddress;
  cs_float *port;        /* UDP port */
  ARRAYDAT *dest;
  ARRAYDAT *type;
  ARRAYDAT *arg;
  cs_float *imtu;
  int32_t mtu;
  AUXCH   aux;    /* MTU bytes */
  int32_t sock, init_done;
  cs_float   last;
  struct sockaddr_in server_addr;
  int32_t first;
} OSCBUNDLE;


static int32_t oscbundle_arrays_valid(OSCBUNDLE *p)
{
    if (p->type->dimensions != 1 || p->dest->dimensions != 1 ||
        p->arg->dimensions != 2 || p->type->sizes == NULL ||
        p->dest->sizes == NULL || p->arg->sizes == NULL)
      return 0;
    int32_t rows = p->type->sizes[0], cols = p->arg->sizes[1];
    return rows >= 0 && cols >= 0 && p->dest->sizes[0] == rows &&
      p->arg->sizes[0] == rows &&
      (rows == 0 || (p->type->data != NULL && p->dest->data != NULL &&
                    (cols == 0 || p->arg->data != NULL)));
}

static int32_t oscbundle_init(CSOUND *csound, OSCBUNDLE *p) {
    if (!oscbundle_arrays_valid(p))
      return csound->InitError(csound, "%s",
        Str("oscbundle: expected matching destination, type and argument rows"));
    if (*p->imtu != FL(0.0) &&
        !(*p->imtu >= FL(16.0) && *p->imtu <= MAX_PACKET_SIZE))
      return csound->InitError(csound, "%s",
        Str("oscbundle: packet size must be between 16 and 65536 bytes"));
    p->mtu = *p->imtu == FL(0.0) ? MAX_PACKET_SIZE : (int32_t)*p->imtu;
    if (!p->init_done) {
#if defined(WIN32) && !defined(__CYGWIN__)
      WSADATA wsaData = {0};
      int32_t err;
      if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
        return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
      p->sock = socket(AF_INET, SOCK_DGRAM, 0);
      if (UNLIKELY(p->sock == SOCKET_ERROR)) {
#if defined(WIN32) && !defined(__CYGWIN__)
        WSACleanup();
#endif
        return csound->InitError(csound, "%s", Str("creating socket"));
      }
      p->init_done = 1;
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data,
              &p->server_addr.sin_addr);    /* the server IP address */
#endif
    p->server_addr.sin_port = htons((int32_t) *p->port);    /* the port */

    if (p->aux.auxp == NULL || p->aux.size < (size_t)p->mtu)
      csound->AuxAlloc(csound, p->mtu, &p->aux);
    p->first = 1;
    p->last = FL(0.0);
    return OK;
}

static int32_t oscbundle_perf(CSOUND *csound, OSCBUNDLE *p){
    if (p->first || *p->kwhen != p->last) {
      if (!oscbundle_arrays_valid(p))
        return csound->PerfError(csound, &p->h, "%s",
          Str("oscbundle: expected matching destination, type and argument rows"));
      char *buffer = (char *)p->aux.auxp;
      size_t used = 16;
      int32_t rows = p->type->sizes[0], cols = p->arg->sizes[1];
      memset(buffer, 0, p->mtu);
      memcpy(buffer, "#bundle", 7);
      buffer[15] = 1;  /* OSC's immediate timetag. */
      for (int32_t i = 0; i < rows; ++i) {
        const char *types = csound_string_array_element(p->type, i)->data;
        const char *dest = csound_string_array_element(p->dest, i)->data;
        if (types == NULL) types = "";
        if (dest == NULL) dest = "";
        size_t nargs = strlen(types), length = strlen(dest);
        if (length > (size_t)p->mtu || nargs > (size_t)p->mtu / 4)
          goto too_large;
        size_t destsize = (length + 4) & ~(size_t)3;
        size_t typesize = (nargs + 5) & ~(size_t)3;
        size_t size = destsize + typesize + nargs * 4;
        if (size + 4 > (size_t)p->mtu - used)
          goto too_large;
        uint32_t encoded = htonl((uint32_t)size);
        memcpy(buffer + used, &encoded, 4);
        used += 4;
        memcpy(buffer + used, dest, length);
        used += destsize;
        buffer[used] = ',';
        memcpy(buffer + used + 1, types, nargs);
        used += typesize;
        for (size_t n = 0; n < nargs; ++n) {
          cs_float value = n < (size_t)cols ? p->arg->data[(size_t)i * cols + n] : 0;
          if (types[n] == 'f') {
            float fdata = (float)value;
            memcpy(&encoded, &fdata, 4);
          }
          else if (types[n] == 'i')
            encoded = (uint32_t)(int32_t)value;
          else
            return csound->PerfError(csound, &p->h, "%s",
              Str("oscbundle: only i and f types are supported"));
          encoded = htonl(encoded);
          memcpy(buffer + used, &encoded, 4);
          used += 4;
        }
      }
      if (UNLIKELY(sendto(p->sock, buffer, (int32_t)used, 0,
                         (const struct sockaddr *)&p->server_addr,
                         sizeof(p->server_addr)) < 0))
        return csound->PerfError(csound, &p->h, "%s", Str("OSCbundle failed"));
      p->first = 0;
      p->last = *p->kwhen;
    }
    return OK;
 too_large:
    csound->Warning(csound, "%s",
                   Str("Bundle msg exceeded max packet size, not sent\n"));
    return OK;
}

static int32_t oscbundle_deinit(CSOUND *csound, OSCBUNDLE *p)
{
    if (!p->init_done) return OK;
    p->init_done = 0;
#if defined(WIN32)
    closesocket((SOCKET)p->sock);
    WSACleanup();
#else
    close(p->sock);
#endif
    return OK;
}



#define S(x)    sizeof(x)

static OENTRY socksend_localops[] =
  {
   { "socksend.a", S(SOCKSEND), 0, "", "aSiio", (SUBR) init_send,
     (SUBR) send_send, (SUBR) socksend_deinit },
   { "socksend.k", S(SOCKSEND), 0, "", "kSiio", (SUBR) init_send,
     (SUBR) send_send_k, (SUBR) socksend_deinit },
   { "socksend.S", S(SOCKSENDT), 0, "", "SSiio", (SUBR) init_send_Str,
     (SUBR) send_send_Str, (SUBR) socksend_deinit },
   { "socksends", S(SOCKSENDS), 0, "", "aaSiio", (SUBR) init_sendS,
     (SUBR) send_sendS, (SUBR) socksends_deinit },
   { "stsend", S(STSEND), 0, "", "aSi", (SUBR) init_ssend,
     (SUBR) send_ssend, (SUBR) stsend_deinit },
  CSOUND_DEPRECATED_OPCODE("OSCsend", "oscsend", ALIAS, "Renamed alias; maintain the shared implementation through its supported name.")
  { "OSCsend", S(OSCSEND2), 0, "", "kSkSN", (SUBR)osc_send2_init,
    (SUBR)osc_send2, (SUBR) oscsend_deinit, NULL, 2 },
  CSOUND_DEPRECATED_OPCODE("OSCbundle", "oscbundle", ALIAS, "Renamed alias; maintain the shared implementation through its supported name.")
  { "OSCbundle", S(OSCBUNDLE), 0, "", "kSkS[]S[]k[][]o", (SUBR)oscbundle_init,
    (SUBR)oscbundle_perf, (SUBR) oscbundle_deinit, NULL, 2 },
  /* aliases */
   { "oscsend", S(OSCSEND2), 0, "", "kSkSN", (SUBR)osc_send2_init,
     (SUBR)osc_send2, (SUBR) oscsend_deinit },
   { "oscbundle", S(OSCBUNDLE), 0, "", "kSkS[]S[]k[][]o", (SUBR)oscbundle_init,
     (SUBR)oscbundle_perf, (SUBR) oscbundle_deinit},
};

LINKAGE_BUILTIN(socksend_localops)
#endif
