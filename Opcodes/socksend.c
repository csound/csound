/*
    socksend.c:

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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

int inet_aton(const char *cp, struct in_addr *inp);

typedef struct {
    OPDS        h;
    MYFLT       *asig, *ipaddress, *port;
    int sock, conn, frag;
    struct sockaddr_in server_addr;
} SOCKSEND;

#define MTU (1456)

/* UDP version */
int init_send(CSOUND *csound, SOCKSEND *p)
{
    p->sock = socket(PF_INET, SOCK_DGRAM, 0);

    if (p->sock < 0) {
      csound->InitError(csound, "creating socket");
      return NOTOK;
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    inet_aton((const char*)p->ipaddress,
              &p->server_addr.sin_addr);    /* the server IP address */
    p->server_addr.sin_port = htons((int)*p->port);      /* the port */
    p->frag = (sizeof(MYFLT)*csound->ksmps>MTU); /* fragment packets? */
    return OK;
}

int send_send(CSOUND *csound, SOCKSEND* p)
{
    const struct sockaddr *to = (const struct sockaddr *)(&p->server_addr);
    int n = sizeof(MYFLT)*csound->ksmps;
    if (p->frag) {
      while (n>MTU) {
        if (sendto(p->sock, p->asig, MTU, 0, to, sizeof(p->server_addr)) < 0) {
          csound->PerfError(csound, "sendto failed");
          return NOTOK;
        }
        n -= MTU;
      }
    }
    if (sendto(p->sock, p->asig, n, 0, to, sizeof(p->server_addr)) < 0) {
      csound->PerfError(csound, "sendto failed");
      return NOTOK;
    }
    return OK;
}

/* TCP version */
int init_ssend(CSOUND *csound, SOCKSEND* p)
{
    int clilen;
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

    /* associate the socket with the address and port */
    if (bind(p->sock,(struct sockaddr *)&p->server_addr,sizeof(p->server_addr))
        < 0) {
      csound->InitError(csound,"bind failed");
      return NOTOK;
    }

    /* start the socket listening for new connections -- may wait */
    if (listen(p->sock, 5) < 0) {
      csound->InitError(csound,"listen failed");
      return NOTOK;
    }
    clilen = sizeof(p->server_addr);
    p->conn = accept(p->sock, (struct sockaddr *)&p->server_addr, &clilen);

    if (p->conn < 0) {
      csound->InitError(csound,"accept failed");
      return NOTOK;
    }
    return OK;
}

int send_ssend(CSOUND *csound, SOCKSEND* p)
{
    int n = sizeof(MYFLT)*csound->ksmps;
    if (n!=write(p->conn, p->asig, sizeof(MYFLT)*csound->ksmps)) {
      csound->PerfError(csound, "write to socket failed");
      return NOTOK;
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "socksend", S(SOCKSEND), 5, "", "aSi", (SUBR)init_send, NULL, (SUBR)send_send},
  { "stsend", S(SOCKSEND), 5, "", "aSi", (SUBR)init_ssend, NULL, (SUBR)send_ssend}
};

LINKAGE

