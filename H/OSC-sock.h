/*
    OSC-sock.h:

    Copyright (C) 2001, 2003 Nicola Bernadino, Stefan Kersten

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

/* ==============================================================
 * File:        OSC-sock.h
 * vim:         set sw=4 set cin
 * Author:      Stefan Kersten <steve@k-hornz.de>
 * Contents:    Csound/OSC support
 * ==============================================================
 * $Id$
 * ==============================================================
 */

#if !defined(_OSC_sock_h_)
#       define _OSC_sock_h_

#include "csdl.h"

#ifndef HAVE_BZERO
#define bzero(x,y) memcpy(x,0,y)
#endif

/* ===================================================================
 * OSCrecv
 */
typedef struct {
    OPDS h;                 /* default header */
    /* Arguments */
    MYFLT *iport;           /* receive network port */
    MYFLT *imaxservice;     /* highest number of services
                               in an instrument */
    MYFLT *imaxgroup;       /* highest number of controllable
                               groups of one instrument (instances) */
} OSCINIT;

typedef struct
{
    int insNo, serviceNo, groupNo;
} oscContext;

typedef struct {
    OPDS h;          /* default header */
    MYFLT *result;   /* krate result */
    /* Arguments */
    MYFLT *iservice; /* service number */
    MYFLT *iinit;    /* initial krate value (opt) */
    MYFLT *imin;     /* min value for result (opt) */
    MYFLT *imax;     /* max value for result (opt) */
    /* Private */
    oscContext c;    /* instrument information */
} OSCRECV;

void  osc_init(void*), osc_recv_set(void*), osc_recv(void*);

/* ===================================================================
 * OSCsend
 */

#include <netinet/in.h>
#include <OSC/OSC-client.h>

typedef struct
{
    OPDS h;             /* default header */
    /* Arguments */
    MYFLT *arg;         /* krate argument */
    MYFLT *iaddr;       /* OSC address */
    MYFLT *ismps;       /* buffer size in kcycles */
    MYFLT *iport;       /* UDP port */
    MYFLT *ihost;       /* hostname (opt) */
    /* UDP */
    struct sockaddr_in servAddr;
    int sockFD;
    /* OSC */
    char *oscAddr;
    OSCbuf *oscBufArr;
    int bufPos;         /* consumed cycles */
    char **bufArr;
} OSCSEND;

void  osc_send_set(void*), osc_send_k(void*);

#endif /* !defined(_OSC_sock_h_) */
