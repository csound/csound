/*
    OS-sock.c:

    Copyright (C) 2001, 2003 Nicola Bernadino, Stefan Kersten, John ffitch

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
 * File:        OSC-sock.c
 * vim:         set sw=4 set cin
 * Author:      Stefan Kersten <steve@k-hornz.de>
 * Contents:    Csound/OSC support
 * ==============================================================
 * $Id$
 * ==============================================================
 */
                                /* As not ANSI */
#ifndef gcc
#define inline
#endif

#include "csdl.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
/* UDP */
#include <fcntl.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netdb.h>

#if defined(TRUE)
#	undef TRUE		/* otherwise we will lose some typedefing */
#endif  /* defined(TRUE) */
#if defined(FALSE)
#	undef FALSE		/* same as above */
#endif /* defined(FALSE) */

#include "OSC-Kit/OSC-common.h"
#include "OSC-Kit/OSC-timetag.h"
#include "OSC-Kit/OSC-address-space.h"
#include "OSC-Kit/OSC-receive.h"
#include "OSC-Kit/NetworkUDP.h"

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


/* ===================================================================
 * OSCsend
 */

#include <netinet/in.h>
#include "OSC-Kit/OSC-client.h"

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

/* ===================================================================
 * Common
 */

static void oscWarning(char *who, char *what, ...);
static void __DEBUG(char *s, ...);

/* ===================================================================
 * OSCrecv
 */

/*
 * Constants
 */

/* Name used for port lookups in /etc/services */
#define SERVICE_UDP "csound-OSCrecv"
/* Fallback port if lookup fails for some reason */
#define PORT_UDP 2222

/* These determine the maximum number of instruments using OSCsock,
 * the number of service names that can be used in a single
 * instrument and the number of groups of one instrument (instances)
 * you wish to control separately (those values can be overridden
 * with the OSCinit opcode).
 */
#define MAX_INS 99
#define MAX_SERVICE 20
#define MAX_GROUP 20

/* When these sizes are exceeded, packets will have to be dropped */
#define MAX_RECV_BUF_SIZE 2048  /* Max size of incoming UDP buffers;
                                   does UDP allow sizes > 4096? */
#define MAX_NUM_RECV_BUF 50     /* Max number of incoming buffers */
#define MAX_NUM_QUEUE 50        /* Max number of objects in the OSC queue */
#define MAX_NUM_CB 100          /* Max number of active callbacks in the scheduler */

/*
 * Types
 */

typedef struct
{
  OSCBoolean state;
  MYFLT value, min, max;
} oscGroupStruct;

typedef struct
{
  OSCBoolean state;
  OSCcontainer cont;
  oscGroupStruct *group;
} oscServiceStruct;

typedef struct
{
  OSCBoolean state;
  OSCcontainer cont;
  oscServiceStruct *service;
} oscInsStruct;

typedef struct
{
  int maxIns, maxService, maxGroup;
  oscInsStruct *ins;
} oscStateStruct;


/*
 * Variables
 */

/* Global states and values */
static oscStateStruct *oscState;
static OSCBoolean oscIsUp = FALSE;

static int oscSockFD = 0;

/* OSC containers */
static OSCcontainer oscTopLevelCont;
static OSCcontainer oscVarCont;


/*
 * Declarations
 */
static void oscReceivePacket(int sockFD);

static void oscGroupCB(void *context, int arglen, const void *vargs,
                       OSCTimeTag when, NetworkReturnAddressPtr ra);

inline static char *oscIntToString(int i);
inline static int oscFractToInt(MYFLT fract);

inline static void oscContextMake(oscContext *, int, int, int);
inline static OSCBoolean oscContextInsNoCheck(const oscContext *c);
inline static OSCBoolean oscContextServiceNoCheck(const oscContext *c);
inline static OSCBoolean oscContextGroupNoCheck(const oscContext *c);

inline static oscStateStruct *oscStateMake(int, int, int);
#define oscMaxInsGet() oscState->maxIns
#define oscMaxServiceGet() oscState->maxService
#define oscMaxGroupGet() oscState->maxGroup

inline static OSCBoolean oscInsIsUp(const oscContext *c);
inline static void oscInsUp(const oscContext *c);

inline static OSCBoolean oscServiceIsUp(const oscContext *c);
inline static void oscServiceUp(const oscContext *c);

inline static OSCBoolean oscGroupIsUp(const oscContext *c);
inline static void oscGroupUp(const oscContext *c);
inline static MYFLT oscGroupGetValue(const oscContext *c);
inline static void oscGroupSetValue(const oscContext *, MYFLT, MYFLT, MYFLT max);

inline static void oscInsAddCont(const oscContext *c, OSCcontainer parent);
inline static void oscServiceAddCont(const oscContext *c, char *name);
inline static void oscGroupAddMethod(const oscContext *c);

static void oscRecvWarning(char *what, ...);

int oscInitUDP(const char*, int);
int oscInitOSC(void);

/* ===================================================================
 * Interface
 */

/*
 * Global OSC initialization; should be called before any use of
 * OSCrecv
 */
int osc_init(OSCINIT *p)
{
    int port = *(p->iport);
    int maxIns = MAX_INS;
    int maxService = *(p->imaxservice);
    int maxGroup = *(p->imaxgroup);

    if (oscIsUp)
      return initerror("you should only call OSCinit once, globally");

    if (maxService == 0)
      maxService = MAX_SERVICE;

    if (maxGroup == 0)
      maxGroup = MAX_GROUP;

#ifdef OSC_DEBUG
    __DEBUG("osc_init: allocating %d instruments, %d services, %d groups",
            maxIns, maxService, maxGroup);
#endif

    if ((oscState = oscStateMake(maxIns, maxService, maxGroup)) == NULL) {
      return initerror("memory allocation failed");
    }
    oscSockFD = oscInitUDP(SERVICE_UDP, port);

    if (oscSockFD < 0)
      return initerror("network initialization failed");

    if (oscInitOSC() == -1)
      return initerror("OSC library initialization failed (oscInitOSC)");

    oscIsUp = TRUE;
    return OK;
}

/*
 * Opcode initialization: set up OSC address space and register
 * callbacks
 */
int osc_recv_set(OSCRECV *p)
{
    static short __lineNo;
    static oscContext Sc;

    char *serviceName;
    /*    long serviceNo; */
    short lineNo = p->h.optext->t.linenum + 1;  /* prevent collision with the
                                                   default initialization of
                                                   __lineNo */
    oscContext c;

    if (!oscIsUp)
      return initerror("OSCrecv: OSC has not been initialised");

  /* Handle service string argument */
  /* Look at soundin.c if you don't understand
   * what's going on (I don't). */
    if ((serviceName = malloc(64)) == NULL)
      return initerror("OSCrecv: no space left for service string (osc_sock_set)");
    if (*p->iservice == SSTRCOD) {
      if (p->STRARG == NULL)
        strcpy(serviceName, unquote(currevent->strarg));
      else
        strcpy(serviceName, unquote(p->STRARG));
    }
    /*
     * Do we need that, too?
     else if ((serviceNo = (long)*p->iservice) < strsmax && strsets != NULL &&
     strsets[serviceNo])
     strcpy(soundiname, strsets[filno]);
    */
    else {
      /* service is a number */
      serviceName = oscIntToString((long)*p->iservice);
    }

    /* Handle service number */
    c.insNo = p->h.insdshead->insno;
    c.groupNo = oscFractToInt(p->h.insdshead->p1);

    if (c.insNo != Sc.insNo     ||
        c.groupNo != Sc.groupNo ||
        lineNo <= __lineNo) {
      /* We're in a new instrument or back
       * at the beginning of an old one */
      Sc.insNo = c.insNo;
      Sc.serviceNo = c.serviceNo = 0;
      Sc.groupNo = c.groupNo;
    }
    else {
      /* Same instrument, next opcode instance */
      c.serviceNo = ++Sc.serviceNo;
    }

    __lineNo = lineNo;  /* update static line number */

    /* Range checks */
    if (!oscContextInsNoCheck(&c)) {
      oscRecvWarning("instrument number %d not within valid range [0;%d]",
                     c.insNo, oscMaxInsGet());
      return initerror("");
    }
    if (!oscContextServiceNoCheck(&c)) {
      oscRecvWarning("service number %d not within valid range [0;%d]",
                     c.serviceNo, oscMaxServiceGet());
      return initerror("");
    }
    if (!oscContextGroupNoCheck(&c)) {
      oscRecvWarning("group number %d not within valid range [0;%d]",
                     c.groupNo, oscMaxGroupGet());
      return initerror("");
    }

    p->c = c;      /* store context for performance time use */

    /* Instrument container */
    if (!oscInsIsUp(&c)) {
      oscInsAddCont(&c, oscVarCont);
      oscInsUp(&c);
    }
    /* Service container */
    if (!oscServiceIsUp(&c)) {
      oscServiceAddCont(&c, serviceName);
      oscServiceUp(&c);
    }
    /* Group method */
    if (!oscGroupIsUp(&c)) {
      oscGroupAddMethod(&c);

      /* Set initial value of result and min/max values */
      oscGroupSetValue(&c, *(p->iinit), *(p->imin), *(p->imax));
      oscGroupUp(&c);
    }
    return OK;
}

/* Check for incoming OSC packets, invoke OSC messages and update
 * output signal
 */
int osc_recv(OSCRECV *p)
{
    fd_set readFDs;
    struct timeval wait;

  /* Process OSC messages that have to be scheduled now */
    OSCInvokeAllMessagesThatAreReady(OSCTT_CurrentTime());

    FD_ZERO(&readFDs);            /* clear read_fds */
    FD_SET(oscSockFD, &readFDs);  /* set up select */

    /* Return from select immediately */
    wait.tv_sec = 0;
    wait.tv_usec = 0;
    if (select(oscSockFD+1, &readFDs, (fd_set *)0, (fd_set * )0, &wait) < 0) {
      oscRecvWarning("system call select on socket %d failed (osc_sock)",
                     oscSockFD);
      return perferror("");
    }

    if (FD_ISSET(oscSockFD, &readFDs))
      oscReceivePacket(oscSockFD);         /* check for incoming packets */

    *p->result = oscGroupGetValue(&p->c);    /* now update result pointer */
    return OK;
}


/* ===================================================================
 * OSC Methods
 */

/* Initialize UDP */
int oscInitUDP(const char* service, int port)
{
    struct servent *servPort;
    unsigned short thePort;
    struct sockaddr_in servAddr;
    int sockFD;

    if((sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
      return sockFD;

    if (port == 0) {
      /* Look for port in /etc/services */
      if ((servPort = getservbyname(service, "udp")) == NULL) {
        /* Use hardcoded port */
        oscRecvWarning("using hardcoded port %d", PORT_UDP);
        thePort = htons(PORT_UDP);
      }
      else
        thePort = servPort->s_port;
    }
    else
      thePort = htons(port);

    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = thePort;

    if (bind(sockFD, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
      close(sockFD);
      return -1;
    }

#if OSC_DEBUG
    __DEBUG("oscInitUDP: set up service \"%s/udp\" on port %d (socket %d)",
            service,  ntohs(thePort), sockFD);
#endif

    /* Non-blocking mode */
    fcntl(sockFD, F_SETFL, FNDELAY);

    return sockFD;
}

/* Will be called once at OSC initialization */
void *oscInitTimeMalloc(int numBytes)
{
    void *result = malloc(numBytes);
    return result;
}

void *oscRealTimeMalloc(int numBytes)
{
    return NULL;
}

/* Set up OSC address space
 * the rest will be added dynamically
 */
void oscInitAddrSpace(void)
{
    struct OSCAddressSpaceMemoryTuner t;
    struct OSCContainerQueryResponseInfoStruct cqInfo;

  /* We have to allocate enough space, so that at least all
   * group methods fit into the containers */
    t.initNumContainers = oscMaxInsGet() * oscMaxServiceGet() + 20;
    t.initNumMethods = t.initNumContainers * oscMaxGroupGet() + 20;
    t.InitTimeMemoryAllocator = oscInitTimeMalloc;
    t.RealTimeMemoryAllocator = oscRealTimeMalloc;

    /* Mother of invention */
    oscTopLevelCont = OSCInitAddressSpace(&t);

    /* Container for csound variable access */
    OSCInitContainerQueryResponseInfo(&cqInfo);
    cqInfo.comment = "Csound variables";
    oscVarCont = OSCNewContainer("var",  oscTopLevelCont, &cqInfo);
}

/* Register memory allocation routines and set buffer sizes */
int oscInitReceive(void)
{
    struct OSCReceiveMemoryTuner rt;
    int result;

    rt.InitTimeMemoryAllocator = oscInitTimeMalloc;
    rt.RealTimeMemoryAllocator = oscRealTimeMalloc;
    rt.receiveBufferSize = MAX_RECV_BUF_SIZE;
    rt.numReceiveBuffers = MAX_NUM_RECV_BUF;
    rt.numQueuedObjects = MAX_NUM_QUEUE;
    rt.numCallbackListNodes = MAX_NUM_CB;

    result = OSCInitReceive(&rt);

    if (!result)
      return -1;

    return result;
}

/* OSC lib initialization routine */
int oscInitOSC(void)
{
    oscInitAddrSpace();
    if (oscInitReceive() == -1)
      return -1;
    return 1;
}

/* Receive OSC packets from sockFD */
void oscReceivePacket(int sockFD)
{
    OSCPacketBuffer pb;
    struct NetworkReturnAddressStruct *ra;
    int maxclilen = sizeof(struct sockaddr_in);
    int n;
    int capacity = OSCGetReceiveBufferSize();
    int morePackets = 1;
    char *buf;

    while(morePackets) {
      pb = OSCAllocPacketBuffer();
      if (!pb) {
        OSCWarning("Out of memory for packet buffers - had to drop a packet!");
        return;
      }

      buf = OSCPacketBufferGetBuffer(pb);

      /* Matt Wright writes:
       * The type NetworkReturnAddressPtr is const in two ways, so that
       * callback procedures that are passed the pointer can neither change
       * the pointer itself or the data the pointer points to.  But here's
       * where we fill in the return address, so we do a cast to a non-const
       * type.
       */
      ra = (struct NetworkReturnAddressStruct *) OSCPacketBufferGetClientAddr(pb);
      ra->clilen = maxclilen;
      ra->sockfd = sockFD;

#ifdef OSC_DEBUG
      __DEBUG("oscReceivePacket: oscSockFD %d, buf %p, capacity %d\n",
              oscSockFD, buf, capacity);
#endif

      n = recvfrom(sockFD, buf, capacity, 0,
                   (struct sockaddr*)(&(ra->cl_addr)),
                   &(ra->clilen));

      if (n > 0) {
        int *sizep = OSCPacketBufferGetSize(pb);
        *sizep = n;
        OSCAcceptPacket(pb);
      }
      else {
        OSCFreePacket(pb);
        morePackets = 0;
      }
    }
}

/* OSC-Callback: operates on a single group value */
void oscGroupCB(void *context, int arglen, const void *vargs,
                OSCTimeTag when, NetworkReturnAddressPtr ra)
{
    oscGroupStruct *i = (oscGroupStruct *)context;
    MYFLT arg = *((MYFLT *)vargs);
    MYFLT res;

    int argc = arglen/4;          /* arglen contains number of OSC "words" */

    if (argc != 1) {
      oscRecvWarning("expecting exactly one floating point argument");
      oscRecvWarning("event ignored");
    }
    else {
      /* Handle min and max values */
      if (i->min != i->max) {
        if (arg <= i->min) {
#ifdef OSC_DEBUG
          __DEBUG("oscGroupCB: using min value %d", i[c->groupNo].min);
#endif
          res = i->min;
        }
        else if (arg > i->max) {
#ifdef OSC_DEBUG
          __DEBUG("oscGroupCB: using max value %d", i[c->groupNo].max);
#endif
          res = i->max;
        }
        else
          res = arg;
      }
      else
        res = arg;

      /* TODO: Add conversion from network byteorder !
       *       e.g.: OSCGetFloatFromArgs(vargs);
       */
      i->value = res;

#ifdef OSC_DEBUG
      __DEBUG("event at %u.%u: argument \"%f\"\n",
              when.seconds, when.fraction, arg);
#endif

    }
}

/* ===================================================================
 * Utility
 */

/* FIXME: <eeek>, this is nasty! */
int
oscFractToInt(MYFLT fract)
{
    char buf[30];
    char *q;
    int j;

    sprintf(buf, "%f", fract);

    /* Look for first char after '.' */
    q = strchr(buf, '.')+1;
    /* Now find last char != '0' */
    j = strlen(buf) - 1;
    while (1) {
      if (buf[j] != '0') break;
      buf[j] = '\0';
      j--;
    }
    return atoi(q);
}

char *
oscIntToString(int i)
{
    char *buf = malloc(((sizeof(int) * CHAR_BIT + 2) / 3 + 1));

    if (buf != NULL)
      sprintf(buf, "%d", i);

    return buf;
}

void oscContextMake(oscContext *c, int insNo, int serviceNo, int groupNo)
{
    c->insNo = insNo;
    c->serviceNo = serviceNo;
    c->groupNo = groupNo;

#ifdef OSC_DEBUG
    __DEBUG("oscContextMake: insNo=%d, serviceNo=%d, groupNo=%d",
            c->insNo, c->serviceNo, c->groupNo);
#endif
}

OSCBoolean oscContextInsNoCheck(const oscContext *c)
{
    return !(c->insNo < 0 || c->insNo > oscState->maxIns);
}

OSCBoolean oscContextServiceNoCheck(const oscContext *c)
{
    return !(c->serviceNo < 0 || c->serviceNo > oscState->maxService);
}

OSCBoolean oscContextGroupNoCheck(const oscContext *c)
{
    return !(c->groupNo < 0 || c->groupNo > oscState->maxGroup);
}

/* Allocate a new oscState structure from the sizes given and
 * initialize fields
 */
oscStateStruct *oscStateMake(int maxIns, int maxService, int maxGroup)
{
    int i, j, k;
    int insNo = maxIns + 1;
    int serviceNo = maxService + 1;
    int groupNo = maxGroup + 1;

    oscStateStruct *oscState = malloc(sizeof(oscStateStruct));
    if (oscState == NULL)
      return oscState;

    oscState->maxIns = maxIns;
    oscState->maxService = maxService;
    oscState->maxGroup = maxGroup;

    oscState->ins = (oscInsStruct *)malloc(sizeof(oscInsStruct)*insNo);
    if (oscState->ins == NULL)
      return oscState;

    for (i = 0; i < insNo; i++) {
      oscState->ins[i].state = FALSE;

      oscState->ins[i].service =
        (oscServiceStruct *)malloc(sizeof(oscServiceStruct)*serviceNo);
      if (oscState->ins[i].service == NULL)
        return oscState;

      for (j = 0; j < serviceNo; j++) {
        oscState->ins[i].service[j].state = FALSE;

        oscState->ins[i].service[j].group =
          (oscGroupStruct *)malloc(sizeof(oscGroupStruct)*groupNo);
        if (oscState->ins[i].service[j].group == NULL)
          return oscState;

        for (k = 0; k < groupNo; k++) {
          oscState->ins[i].service[j].group[k].state = FALSE;
          oscState->ins[i].service[j].group[k].value = 0.0;
          oscState->ins[i].service[j].group[k].min = 0.0;
          oscState->ins[i].service[j].group[k].max = 0.0;
        }
      }
    }

    return oscState;
}

OSCBoolean oscInsIsUp(const oscContext *c)
{
    return oscState->ins[c->insNo].state;
}

void oscInsUp(const oscContext *c)
{
    oscState->ins[c->insNo].state = TRUE;
}

OSCcontainer oscInsGetCont(const oscContext *c)
{
    return oscState->ins[c->insNo].cont;
}

OSCBoolean oscServiceIsUp(const oscContext *c)
{
    return oscState->ins[c->insNo].service[c->serviceNo].state;
}

void oscServiceUp(const oscContext *c)
{
    oscState->ins[c->insNo].service[c->serviceNo].state = TRUE;
}

OSCBoolean oscGroupIsUp(const oscContext *c)
{
    return oscState->ins[c->insNo].service[c->serviceNo].group[c->groupNo].state;
}

void oscGroupUp(const oscContext *c)
{
    oscState->ins[c->insNo].service[c->serviceNo].group[c->groupNo].state = TRUE;
}

MYFLT oscGroupGetValue(const oscContext *c)
{
    return oscState->ins[c->insNo].service[c->serviceNo].group[c->groupNo].value;
}

void oscGroupSetValue(const oscContext *c, MYFLT value, MYFLT min, MYFLT max)
{
    oscGroupStruct *i = oscState->ins[c->insNo].service[c->serviceNo].group;
    i[c->groupNo].value = value;
    i[c->groupNo].min = min;
    i[c->groupNo].max = max;
}

/*
 *  Container/Method management
 */
void oscInsAddCont(const oscContext *c, OSCcontainer parent)
{
    char *insNoString;
    struct OSCContainerQueryResponseInfoStruct cqInfo;

    insNoString = oscIntToString(c->insNo);

#ifdef OSC_DEBUG
    {
      char ScontString[20];
      OSCGetAddressString(ScontString, 20, parent);
      __DEBUG("oscInsAddCont: insNoString \"%s\"", insNoString);
      __DEBUG("oscInsAddCont: parent \"%s\"", ScontString);
      __DEBUG("oscInsAddCont: insNo %d", c->insNo);
    }
#endif

    oscState->ins[c->insNo].cont = OSCNewContainer(insNoString, parent, &cqInfo);

#ifdef OSC_DEBUG
    __DEBUG("oscInsAddCont: child address %p", oscState->ins[c->insNo].cont);
#endif
}

void oscServiceAddCont(const oscContext *c, char *name)
{
    struct OSCContainerQueryResponseInfoStruct cqInfo;

    OSCcontainer parent = oscState->ins[c->insNo].cont;

#ifdef OSC_DEBUG
    {
      char ScontString[20];
      OSCGetAddressString(ScontString, 20, parent);
      __DEBUG("oscServiceAddCont: parent \"%s\"", ScontString);
      __DEBUG("oscServiceAddCont: parent address %p", parent);
      __DEBUG("oscServiceAddCont: serviceNo %d", c->serviceNo);
    }
#endif

    oscState->ins[c->insNo].service[c->serviceNo].cont =
      OSCNewContainer(name, parent, &cqInfo);

#ifdef OSC_DEBUG
    {
      char ScontString[20];
      OSCGetAddressString(ScontString, 20,
                          oscState->ins[c->insNo].service[c->serviceNo].cont);
      __DEBUG("oscServiceAddCont: child address %p",
              oscState->ins[c->insNo].service[c->serviceNo].cont);
      __DEBUG("oscServiceAddCont: child \"%s\"", ScontString);
    }
#endif
}

void oscGroupAddMethod(const oscContext *c)
{
    char *groupNoString;
    struct OSCMethodQueryResponseInfoStruct mqInfo;

    OSCcontainer parent = oscState->ins[c->insNo].service[c->serviceNo].cont;

    oscGroupStruct *groupP =
      &oscState->ins[c->insNo].service[c->serviceNo].group[c->groupNo];

    groupNoString = oscIntToString(c->groupNo);

#ifdef OSC_DEBUG
    {
      char ScontString[20];
      OSCGetAddressString(ScontString, 20, parent);
      __DEBUG("oscGroupAddMethod: groupNoString \"%s\"", groupNoString);
      __DEBUG("oscGroupAddMethod: parent \"%s\"", ScontString);
      __DEBUG("oscGroupAddMethod: parent address %p", parent);
    }
#endif

    OSCNewMethod(groupNoString, parent, oscGroupCB, groupP, &mqInfo);
}

/* void oscInitWarning(char *what, ...) */
/* { */
/*     va_list ap; */
/*     va_start(ap, what); */
/*     oscWarning("OSCinit", what, ap); */
/* } */

void oscRecvWarning(char *what, ...)
{
    va_list ap;
    va_start(ap, what);
    oscWarning("OSCrecv", what, ap);
}

/* ===================================================================
 * OSCsend
 */

/*
 * Constants
 */

#define LOCALHOST_NAME "localhost"
#define MAX_BUF_SIZE 64
#define MAX_ADDR_LEN 64
#define MAX_HOST_LEN 64

static void oscSendUDP(OSCSEND *p);
static void oscSendWarning(char *what, ...);

/**************************************************************************/
/*
 * Csound interface routines
 */
/**************************************************************************/

int osc_send_set(OSCSEND *p)
{
    int i, bufSize;
    int smps = *(p->ismps);

    char *hostName;
    struct hostent *hostInfo;
    short hostPort = (short)*(p->iport);

    p->bufPos = 0;

    if ((p->oscAddr = malloc(MAX_ADDR_LEN)) == NULL) {
      return initerror("OSCsend: no space left for address string");
    }
    if (*p->iaddr == SSTRCOD) {
      if (p->STRARG == NULL)
        strcpy(p->oscAddr, unquote(currevent->strarg));
      else
        strcpy(p->oscAddr, unquote(p->STRARG));
    }
    else {
      return initerror("OSCsend: need an address string, not a number (osc_send_set)");
    }

    if (*p->ihost) {
      long hostNo;

      if (*p->ihost == SSTRCOD) {
        if ((hostName = malloc(MAX_HOST_LEN)) == NULL)
          return initerror("OSCsend: no space left for host string");
        if (p->STRARG2 == NULL)
          strcpy(hostName, unquote(currevent->strarg));
        else
          strcpy(hostName, unquote(p->STRARG2));
      }
      else if ((hostNo = (long)*p->ihost) < -strsmax &&
               strsets != NULL && strsets[hostNo]) {
        if ((hostName = malloc(MAX_HOST_LEN)) == NULL) {
          return initerror("OSCsend: no space left for host string");
        }
        strcpy(hostName, strsets[hostNo]);
      }
      else {
        hostName = LOCALHOST_NAME; /* Is this correct?? */
      }
    }
    else {
      hostName = LOCALHOST_NAME;
    }

    __DEBUG("host %s on port %d", hostName, hostPort);

    /*
     * Memory allocation
     */

    if ((p->bufArr = (char **)malloc(smps * sizeof(char))) == NULL) {
      return initerror("OSC buffer allocation failed ");
    }

    if ((p->oscBufArr = (OSCbuf *)malloc(smps * sizeof(OSCbuf))) == NULL) {
      oscSendWarning("OSC buffer allocation failed (osc_send_set)");
      return initerror("");
    }

    /* OSC buffer size plus some space for the bundle/timetag */
    bufSize = OSC_effectiveStringLength(p->oscAddr) + sizeof(float) + 100;

    __DEBUG("bufSize = %d", bufSize);

    for (i = 0; i < smps; i++) {
      if ((p->bufArr[i] = (char *)malloc(bufSize)) == NULL) {
        oscSendWarning("OSC buffer allocation failed (osc_send_set)");
        return initerror("");
      }
      OSC_initBuffer(&p->oscBufArr[i], bufSize, p->bufArr[i]);
    }

    /*
     * UDP initialization
     */

    bzero((char *)&p->servAddr, sizeof(p->servAddr));
    p->servAddr.sin_family = AF_INET;

    hostInfo = gethostbyname(hostName);
    if (hostInfo == NULL) {
      oscSendWarning("host name lookup error: %s (osc_send_set)", hostName);
      return initerror("");
    }

    p->servAddr.sin_addr = *(struct in_addr *) hostInfo->h_addr;
    p->servAddr.sin_port = htons(hostPort);

    if ((p->sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      oscSendWarning("initialization of socket %d failed (osc_send_set)",
                     p->sockFD);
    }
    return OK;
}

int osc_send_k(OSCSEND *p)
{
    int smps = *(p->ismps);
    OSCbuf *buf;

    /* Our array is full and we are ready to send the collected
     * messages */
    if (p->bufPos >= smps) {
      oscSendUDP(p);
      p->bufPos = 0;
    }

    /* Clear the buffer and write the address plus the current
     * value of the krate argument */
    buf = &p->oscBufArr[p->bufPos];
    OSC_resetBuffer(buf);
    OSC_openBundle(buf, OSCTT_CurrentTime());
    OSC_writeAddress(buf, p->oscAddr);
    OSC_writeFloatArg(buf, *(p->arg));
    OSC_closeBundle(buf);
    p->bufPos++;
    return OK;
}

/* ===================================================================
 * OSC Methods
 */

void oscSendUDP(OSCSEND *p)
{
    int i;
    OSCbuf *buf;
    int smps = *(p->ismps);
    int sockFD = p->sockFD;
    /* Is this only required for Linux/libc ? */
    struct sockaddr *sp = (struct sockaddr *)(&p->servAddr);

    /* Send each collected buffer */
    for (i = 0; i < smps; i++) {
      buf = &p->oscBufArr[i];
      sendto(sockFD,
             OSC_getPacket(buf),
             OSC_packetSize(buf),
             0, sp, sizeof(p->servAddr));
    }
}

/*
 * Utility
 */

void oscSendWarning(char *what, ...)
{
    va_list ap;
    va_start(ap, what);
    oscWarning("OSCsend", what, ap);
}

/* ===================================================================
 * Common
 */

void oscWarning(char *who, char *what, ...)
{
    va_list ap;
    fprintf(stderr, "\n%s: ", who);
    va_start(ap, what);
    vfprintf(stderr, what, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
}

/* ===================================================================
 * Debug
 */
void __DEBUG(char *s, ...)
{
    va_list ap;
    fprintf(stderr, "\nOSC_DEBUG: ");
    va_start(ap, s);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
}

/* EOF */

#define S(x) sizeof(x)

static OENTRY localops[] = {
  { "OSCinit", S(OSCINIT),  1, "",  "ioo",   (SUBR)osc_init,     NULL,      NULL },
  { "OSCrecv", S(OSCRECV),  3, "k", "Sooo",  (SUBR)osc_recv_set, (SUBR)osc_recv },
  { "OSCsend", S(OSCSEND),  3, "",  "kSiio", (SUBR)osc_send_set, (SUBR)osc_send_k }
};

LINKAGE
