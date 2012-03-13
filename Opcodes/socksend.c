/*
    socksend.c:

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
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern  int     inet_aton(const char *cp, struct in_addr *inp);

typedef struct {
    OPDS    h;
    MYFLT   *asig, *ipaddress, *port, *buffersize;
    MYFLT   *format;
    AUXCH   aux;
    int     sock;
    int     bsize, wp;
    int     ff, bwidth;
    struct sockaddr_in server_addr;
} SOCKSEND;

typedef struct {
    OPDS    h;
    MYFLT   *asigl, *asigr, *ipaddress, *port, *buffersize;
    MYFLT   *format;
    AUXCH   aux;
    int     sock;
    int     bsize, wp;
    int     ff, bwidth;
    struct sockaddr_in server_addr;
} SOCKSENDS;

#define MTU (1456)

/* UDP version one channel */
static int init_send(CSOUND *csound, SOCKSEND *p)
{
    int     bsize;
    int     bwidth = sizeof(MYFLT);
#ifdef WIN32
    WSADATA wsaData = {0};
    int err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
    p->ff = (int)(*p->format);
    p->bsize = bsize = (int) *p->buffersize;
    /* if (UNLIKELY((sizeof(MYFLT) * bsize) > MTU)) { */
    /*   return csound->InitError(csound, Str("The buffersize must be <= %d samples " */
    /*                                        "to fit in a udp-packet."), */
    /*                            (int) (MTU / sizeof(MYFLT))); */
    /* } */
    p->wp = 0;

    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
#ifdef WIN32
    p->server_addr.sin_addr.S_un.S_addr = inet_addr((const char *) p->ipaddress);
#else
    inet_aton((const char *) p->ipaddress,
              &p->server_addr.sin_addr);    /* the server IP address */
#endif
    p->server_addr.sin_port = htons((int) *p->port);    /* the port */

    if (p->ff) bwidth = sizeof(int16);
    /* create a buffer to write the interleaved audio to  */
    if (p->aux.auxp == NULL || (long) (bsize * bwidth) > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, (bsize * bwidth), &p->aux);
    else {
      memset(p->aux.auxp, 0, bwidth * bsize);
    }
    p->bwidth = bwidth;
    return OK;
}

static int send_send(CSOUND *csound, SOCKSEND *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);
    int     i, wp, ksmps = csound->ksmps;
    int     buffersize = p->bsize;
    MYFLT   *asig = p->asig;
    MYFLT   *out = (MYFLT *) p->aux.auxp;
    int16   *outs = (int16 *) p->aux.auxp;
    int     ff = p->ff;

    for (i = 0, wp = p->wp; i < ksmps; i++, wp++) {
      if (wp == buffersize) {
        /* send the package when we have a full buffer */
        if (UNLIKELY(sendto(p->sock, (void*)out, buffersize  * p->bwidth, 0, to,
                            sizeof(p->server_addr)) < 0)) {
          return csound->PerfError(csound, Str("sendto failed"));
        }
        wp = 0;
      }
      if (ff) { // Scale for 0dbfs and make LE
        int16 val = (int16)((32768.0*asig[i])/csound->e0dbfs);
        union cheat {
          char  benchar[2];
          int16 bensht;
        } ch;
        ch.benchar[0] = 0xFF & val;
        ch.benchar[1] = 0xFF & (val >> 8);
        outs[wp] = ch.bensht;
      }
      else
       out[wp] = asig[i];
    }
    p->wp = wp;

    return OK;
}

/* UDP version 2 channels */
static int init_sendS(CSOUND *csound, SOCKSENDS *p)
{
    int     bsize;
    int     bwidth = sizeof(MYFLT);
#ifdef WIN32
    WSADATA wsaData = {0};
    int err;
    if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
      csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif

    p->ff = (int)(*p->format);
    p->bsize = bsize = (int) *p->buffersize;
    /* if (UNLIKELY((sizeof(MYFLT) * bsize) > MTU)) { */
    /*   return csound->InitError(csound, Str("The buffersize must be <= %d samples " */
    /*                                        "to fit in a udp-packet."), */
    /*                            (int) (MTU / sizeof(MYFLT))); */
    /* } */
    p->wp = 0;

    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
#ifdef WIN32
    p->server_addr.sin_addr.S_un.S_addr = inet_addr((const char *) p->ipaddress);
#else
    inet_aton((const char *) p->ipaddress,
              &p->server_addr.sin_addr);    /* the server IP address */
#endif
    p->server_addr.sin_port = htons((int) *p->port);    /* the port */

    if (p->ff) bwidth = sizeof(int16);
    /* create a buffer to write the interleaved audio to */
    if (p->aux.auxp == NULL || (long) (bsize * bwidth) > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, (bsize * bwidth), &p->aux);
    else {
      memset(p->aux.auxp, 0, bwidth * bsize);
    }
    p->bwidth = bwidth;
    return OK;
}

static int send_sendS(CSOUND *csound, SOCKSENDS *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);
    MYFLT   *asigl = p->asigl;
    MYFLT   *asigr = p->asigr;
    MYFLT   *out = (MYFLT *) p->aux.auxp;
    int16   *outs = (int16 *) p->aux.auxp;
    int     i;
    int     buffersize = p->bsize;
    int     wp, ksmps = csound->ksmps;
    int     ff = p->ff;

    /* store the samples of the channels interleaved in the packet */
    /* (left, right) */
    for (i = 0, wp = p->wp; i < ksmps; i++, wp += 2) {
      if (wp == buffersize) {
        /* send the package when we have a full buffer */
        if (UNLIKELY(sendto(p->sock, (void*)out, buffersize * p->bwidth, 0, to,
                            sizeof(p->server_addr)) < 0)) {
          return csound->PerfError(csound, Str("sendto failed"));
        }
        wp = 0;
      }
      if (ff) { // Scale for 0dbfs and make LE
        int16 val = 0x8000*(asigl[i]/csound->e0dbfs);
        union {
          char  benchar[2];
          int16 bensht;
        } ch;

        ch.benchar[0] = 0xFF & val;
        ch.benchar[1] = 0xFF & (val >> 8);
        outs[wp] = ch.bensht;
        val = 0x8000*(asigl[i+1]/csound->e0dbfs);
        ch.benchar[0] = 0xFF & val;
        ch.benchar[1] = 0xFF & (val >> 8);
        outs[wp + 1] = ch.bensht;
      }
      else {
        out[wp] = asigl[i];
        out[wp + 1] = asigr[i];
      }
    }
    p->wp = wp;

    return OK;
}

/* TCP version */
static int init_ssend(CSOUND *csound, SOCKSEND *p)
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

static int send_ssend(CSOUND *csound, SOCKSEND *p)
{
    int     n = sizeof(MYFLT) * csound->ksmps;

    if (n != write(p->sock, p->asig, n)) {
      csound->Message(csound, "Expected %d got %d\n",
                      (int) (sizeof(MYFLT) * csound->ksmps), n);
      return csound->PerfError(csound, Str("write to socket failed"));
    }

    return OK;
}

#define S(x)    sizeof(x)

static OENTRY socksend_localops[] = {
  { "socksend", S(SOCKSEND), 5, "", "aSiio", (SUBR) init_send, NULL,
    (SUBR) send_send },
  { "socksends", S(SOCKSENDS), 5, "", "aaSiio", (SUBR) init_sendS, NULL,
    (SUBR) send_sendS },
  { "stsend", S(SOCKSEND), 5, "", "aSi", (SUBR) init_ssend, NULL,
    (SUBR) send_ssend }
};

LINKAGE1(socksend_localops)
