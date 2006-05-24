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

extern  int     inet_aton(const char *cp, struct in_addr *inp);

typedef struct {
    OPDS   h;
    MYFLT  *asig, *ipaddress, *port, *buffersize, *threshold;
    AUXCH  aux, tmp;
    int sock; 
    int wp, rp, state_full, state_empty, samples_wrote;
    struct sockaddr_in server_addr;
} SOCKRECV;

typedef struct {
    OPDS    h;
    MYFLT   *asigl, *asigr, *ipaddress, *port;
    AUXCH   aux;
    int     sock;
    struct sockaddr_in server_addr;
} SOCKRECVS;

#define MTU (1456)
#define MAX_SAMPLES_PER_PACKET (346)

/* UDP version */
int init_recv(CSOUND *csound, SOCKRECV *p)
{
    // initialise the buffer
    int buffersize = *p->buffersize;
    int samp_per_packet = MAX_SAMPLES_PER_PACKET;
    p->state_full=0; 
    p->state_empty=0;
    p->wp=0;
    p->rp=0;

    if( (sizeof(MYFLT)*csound->ksmps) > MTU)
    {
	csound->InitError(csound, "The ksmps must be smaller than 346 samples to fit in a udp-packet.");
	return NOTOK;
    }
    // create the socket
    p->sock = socket(PF_INET, SOCK_DGRAM, 0);
    MYFLT* buf;
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
 
    /* create the jitterbuffer */
    if(*p->buffersize>MAX_SAMPLES_PER_PACKET)
	*p->buffersize=MAX_SAMPLES_PER_PACKET;
    if (p->aux.auxp == NULL || (long) (buffersize * sizeof(MYFLT)) > p->aux.size)
        /* allocate space for the buffer */
        csound->AuxAlloc(csound, buffersize * sizeof(MYFLT), &p->aux);
    else {
        buf = (MYFLT *)p->aux.auxp;  /* make sure buffer is empty */
        do {
	    *buf++ = FL(0.0);
        } while (--buffersize);
    }

    /* create a tmp buffer for the arriving packet */
    if (p->tmp.auxp == NULL || (long) (MTU) > p->tmp.size)
        /* allocate space for the buffer */
        csound->AuxAlloc(csound, MTU, &p->tmp);
    else {
        buf = (MYFLT *)p->tmp.auxp;  /* make sure buffer is empty */
        do {
	    *buf++ = FL(0.0);
        } while (--samp_per_packet);
    }
    return OK;
}

int send_recv(CSOUND *csound, SOCKRECV* p)
{
    struct sockaddr from;
    int i;
// n = sizeof(MYFLT)*csound->ksmps;
    socklen_t clilen;
    ssize_t numbytes;
    clilen = sizeof(from);
    MYFLT ksmps = csound->ksmps;
    MYFLT *buf = (MYFLT *) p->aux.auxp;
    MYFLT *tmp = (MYFLT *) p->tmp.auxp;
    MYFLT *asig = p->asig;
    int wp = p->wp;
    int rp = p->rp;
    int buffersize = (*p->buffersize);
    int threshold = (*p->threshold);

    // get the data from the socket and store it in a tmp buffer
    numbytes = recvfrom(p->sock, tmp, MTU, 0, &from, &clilen);
    if (numbytes < 0) {
	csound->PerfError(csound, "sendto failed");
	return NOTOK;
    }

    // read from the jitterbuffer if it is not empty
    if(p->state_empty)
    {
        for(i=0; i<ksmps; i++)
        {
            // indicate that the buffer is empty
            if(wp==rp){
                p->state_empty=0;
		break;
	    }
            else
            {
		asig[i]=buf[rp];
		rp=(rp!= buffersize-1 ? rp+1 : 0);
            }
	}
	p->rp=rp;
    }

    // write the data in the jitterbuffer
    for(i=0; i<(numbytes/sizeof(MYFLT)); i++)
    {
        buf[wp]=tmp[i];
        wp = (wp!=buffersize-1 ? wp+1 : 0);
        // indicate that there is an bufferoverflow
        if(wp==rp)
            p->state_full=1;
    }
    // if there was a bufferoverflow.
    if(p->state_full)
    {
        p->wp=wp;
        // set the read-pointer upper_treshhold elements behind the write-pointer
        rp=wp;
        for(i=0; i<threshold; i++)
            rp = (rp!=0 ? rp-1 : buffersize-1);
	p->rp=rp;
        p->state_full=0;
        p->state_empty=1;
        return NOTOK;
    }
    // fill the buffer up to the threshold first
    if(!p->state_empty)
    {
	// Enable for Debug purpose:
	// csound->Message(csound, Str("Init-Jitterbuffer: Fill up to the threshold.\n"));
        p->samples_wrote += numbytes/sizeof(MYFLT);
        if(p->samples_wrote >= threshold)
        {
            p->state_empty=1;
            p->samples_wrote=0;
        }
    }
    p->wp=wp;
    return OK;
}


/* UDP version 2 channel */
static int init_recvS(CSOUND *csound, SOCKRECVS *p)
{
    int     n = MTU;
    MYFLT   *buf;

    if((sizeof(MYFLT)*csound->ksmps) > MTU)
    {
	csound->InitError(csound, "The ksmps must be smaller than 346 samples to fit in a udp-packet.");
	return NOTOK;
    }
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

    /* create a buffer to store the received interleaved audio data */
    if (p->aux.auxp == NULL || (long) (n * sizeof(MYFLT)) > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux);
    else {
      buf = (MYFLT *) p->aux.auxp;  /* make sure buffer is empty */
      do {
        *buf++ = FL(0.0);
      } while (--n);
    }

    return OK;
}

static int send_recvS(CSOUND *csound, SOCKRECVS *p)
{
    struct sockaddr from;
    MYFLT   *asigl = p->asigl;
    MYFLT   *asigr = p->asigr;
    MYFLT   *buf = (MYFLT *) p->aux.auxp;
    int     i, j;
    int     n = ((sizeof(MYFLT) * csound->ksmps) * 2);
    int     m = (csound->ksmps * 2);
    socklen_t clilen = sizeof(from);

    if (recvfrom(p->sock, buf, n, 0, &from, &clilen) < 0) {
      csound->PerfError(csound, "sendto failed");
      return NOTOK;
    }

    for (i = 0, j = 0; i < m; i += 2, j++) {
      asigl[j] = buf[i];
      asigr[j] = buf[i + 1];
    }

    return OK;
}

/* TCP version */
static int init_srecv(CSOUND *csound, SOCKRECV *p)
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

static int send_srecv(CSOUND *csound, SOCKRECV *p)
{
    int     n = sizeof(MYFLT) * csound->ksmps;

    if (n != read(p->sock, p->asig, n)) {
      csound->Message(csound, "Expected %d got %d\n",
                      sizeof(MYFLT) * csound->ksmps, n);
      csound->PerfError(csound, "read from socket failed");
      return NOTOK;
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "sockrecv", S(SOCKRECV), 5, "a", "Siii", (SUBR) init_recv, NULL,
    (SUBR) send_recv },
  { "sockrecvs", S(SOCKRECVS), 5, "aa", "Si", (SUBR) init_recvS, NULL,
    (SUBR) send_recvS },
  { "strecv", S(SOCKRECV), 5, "a", "Si", (SUBR) init_srecv, NULL,
    (SUBR) send_srecv }
};

LINKAGE

