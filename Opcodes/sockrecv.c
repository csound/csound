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

#include "csdl.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

int inet_aton(const char *cp, struct in_addr *inp);

typedef struct {
  OPDS        h;
  MYFLT       *asig, *ipaddress, *port;
  int sock, frag;
  struct sockaddr_in server_addr;
} SOCKRECV;

#define MTU (1456)

/* UDP version */
int init_recv(CSOUND *csound, SOCKRECV *p)
{
    p->sock = socket(PF_INET, SOCK_DGRAM, 0);

    if (p->sock < 0) {
      csound->InitError(csound, "creating socket");
      return NOTOK;
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    p->server_addr.sin_port = htons((int)*p->port);      /* the port */
    /* associate the socket with the address and port */
    if (bind(p->sock, (struct sockaddr *)&p->server_addr,
             sizeof(p->server_addr)) < 0)
      return csound->InitError(csound, "bind failed");
    p->frag = (sizeof(MYFLT)*csound->ksmps>MTU); /* fragment packets? */
    return OK;
}

int send_recv(CSOUND *csound, SOCKRECV* p)
{
    const struct sockaddr from;
    char *a = (void *)p->asig;
    int n = sizeof(MYFLT)*csound->ksmps;
    int clilen;
    if (p->frag) {
      while (n>MTU) {
        clilen = sizeof(from);
        if (recvfrom(p->sock, a, MTU, 0, &from, &clilen) < 0) {
          csound->PerfError(csound, "sendto failed");
          return NOTOK;
        }
        n -= MTU;
        a += MTU;
      }
    }
    clilen = sizeof(from);
    if (recvfrom(p->sock, a, n, 0, &from, &clilen) < 0) {
      csound->PerfError(csound, "sendto failed");
      return NOTOK;
    }
    return OK;
}

/* TCP version */
int init_srecv(CSOUND *csound, SOCKRECV* p)
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
    inet_aton((const char*)p->ipaddress, &(p->server_addr.sin_addr));

    /* the port we are going to listen on, in network byte order */
    p->server_addr.sin_port = htons((int)*p->port);

 again:
    if (connect(p->sock, (struct sockaddr *)&p->server_addr, 
                sizeof(p->server_addr)) < 0) {
      if (errno == ECONNREFUSED) goto again;
      return csound->InitError(csound,"connect failed");
    }
    return OK;
}

int send_srecv(CSOUND *csound, SOCKRECV* p)
{
    int n = sizeof(MYFLT)*csound->ksmps;
    if (n!=read(p->sock, p->asig, n)) {
      csound->Message(csound, "Expected %d got %d\n",
                      sizeof(MYFLT)*csound->ksmps, n);
      csound->PerfError(csound, "read from socket failed");
      return NOTOK;
    }
    return OK;
}



#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "sockrecv", S(SOCKRECV), 5, "a", "Si", (SUBR)init_recv, NULL, (SUBR)send_recv},
  { "strecv", S(SOCKRECV), 5, "a", "Si", (SUBR)init_srecv, NULL, (SUBR)send_srecv}
};

LINKAGE

