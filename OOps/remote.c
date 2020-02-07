
/*
    remote.c:

    Copyright (C) 2006 by Barry Vercoe

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"
#include "remote.h"

/* Somewhat revised from the original.  Pete G. Nov 2012
   More correct, I think, but I could be wrong... (:-/)
*/

/* #ifdef HAVE_SOCKETS */
/*   #ifndef WIN32 */
/*     #include <sys/ioctl.h> */
/*     #ifdef LINUX */
/*       #include <linux/if.h> */
/*     #endif */
/*     #ifdef __HAIKU__ */
/*       #include <sys/sockio.h> */
/*     #endif */
/*     #include <sys/socket.h> */
/*     #include <netinet/in.h> */
/*     #include <arpa/inet.h> */
/*     extern int32_t inet_aton (const char *, struct in_addr *); */
/*     #include <net/if.h> */
/*   #else */
/*     #include <winsock2.h> */
/*   #endif /\* not WIN32 *\/ */
/* #endif /\* HAVE_SOCKETS *\/ */


#define MAXREMOTES 10

#define ST(x)   (((REMOTE_GLOBALS*) ((CSOUND*)csound)->remoteGlobals)->x)

void remote_Cleanup(CSOUND *csound);

void remoteRESET(CSOUND *csound)
{
    /* Recover space */
    if (csound->remoteGlobals) csound->Free(csound, csound->remoteGlobals);
    csound->remoteGlobals = NULL;
}

#if defined(HAVE_SOCKETS)
#if !defined(WIN32) || defined(__CYGWIN__)
#include <netdb.h>
#endif
#if 0
static int32_t foo(char *ipaddr)
{
    /* VL 12/10/06: something needs to go here */
    /* gethostbyname is the real answer; code below is unsafe */
    char hostname[1024];
    struct hostent *he;
    struct sockaddr_in sin;
    gethostname(hostname, sizeof(hostname));
    printf("hostname=%s\n", hostname);
    he = gethostbyname(hostname);

    memset(&sin, 0, sizeof (struct sockaddr_in));
    memmove(&sin.sin_addr, he->h_addr_list[0], he->h_length);
    strcpy(ipaddr, inet_ntoa (sin.sin_addr));
    printf("IP: %s\n", ipaddr);
    return 0;
}
#endif

 /* get the IPaddress of this machine */
static int32_t getIpAddress(char *ipaddr)
{
#if defined(WIN32) && !defined(__CYGWIN__)
    /* VL 12/10/06: something needs to go here */
    /* gethostbyname is the real answer; code below is unsafe */
    char hostname[1024];
    struct hostent *he;
    struct sockaddr_in sin;
    if (gethostname(hostname, sizeof(hostname))<0) return 1;
    if ((he = gethostbyname(hostname))==NULL) return 1;

    memset(&sin, 0, sizeof (struct sockaddr_in));
    memmove(&sin.sin_addr, he->h_addr_list[0], he->h_length);
    strcpy(ipaddr, inet_ntoa (sin.sin_addr));
    return 0;
#else
    int32_t ret = 1;
    struct ifreq ifr;
    int32_t fd;

    fd = socket(AF_INET,SOCK_DGRAM, 0);
    if (fd >= 0) {
      char *dev = getenv("CS_ETHER");
      if (dev)
        strNcpy(ifr.ifr_name, dev, IFNAMSIZ-1);
      else {
#ifdef MACOSX
        strNcpy(ifr.ifr_name, "en0", IFNAMSIZ-1);
#else
        strNcpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
#endif
      }
      ifr.ifr_name[IFNAMSIZ-1] = '\0';
      if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
        char *local;
        local = inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr);
        strcpy(ipaddr, local);
        printf("IP for remote: %s: %s\n", ifr.ifr_name, ipaddr);
        ret = 0;
      }
      else {
        strNcpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);
        ifr.ifr_name[IFNAMSIZ-1] = '\0';
        if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
          char *local;
          local = inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr);
          strcpy(ipaddr, local);
          printf("IP for remote: %s: %s\n", ifr.ifr_name, ipaddr);
          ret = 0;
        }
      }
    }
    if (fd>=0) close(fd);
    return ret;
#endif
}

char remoteID(CSOUND *csound)
{
    int32_t len = strlen(ST(ipadrs));
    return ST(ipadrs)[len-1];
}

static int32_t callox(CSOUND *csound)
{
    if (csound->remoteGlobals == NULL) {
      csound->remoteGlobals = csound->Calloc(csound, sizeof(REMOTE_GLOBALS));
      if (UNLIKELY(csound->remoteGlobals == NULL)) {
        csound->Message(csound, Str("insufficient memory to initialise remote"
                                    " globals."));
        goto error;
      }
      ST(remote_port) = REMOT_PORT;
    }

    ST(socksout) = (SOCK*)csound->Calloc(csound,(size_t)MAXREMOTES * sizeof(SOCK));
    if (UNLIKELY(ST(socksout) == NULL)) {
      csound->Message(csound, Str("insufficient memory to initialise outgoing "
                                  "socket table."));
      goto error;
    }

    ST(socksin) = (int32_t*) csound->Calloc(csound,
                                            (size_t)MAXREMOTES * sizeof(int32_t));
    if (UNLIKELY(ST(socksin) == NULL)) {
      csound->Message(csound, Str("insufficient memory to initialise incoming "
                                  "socket table."));
      goto error;
    }

    ST(insrfd_list) =
      (int32_t*) csound->Calloc(csound,(size_t)MAXREMOTES * sizeof(int32_t));
    if (UNLIKELY(ST(insrfd_list) == NULL)) {
      csound->Message(csound, Str("insufficient memory to initialise "
                                  "insrfd_list."));
      goto error;
    }

    ST(chnrfd_list) =
      (int32_t*) csound->Calloc(csound,(size_t)MAXREMOTES * sizeof(int32_t));
    if (UNLIKELY(ST(chnrfd_list) == NULL)) {
      csound->Message(csound, Str("insufficient memory to initialise "
                                  "chnrfd_list."));
      goto error;
    }

    ST(insrfd) = (int32_t*) csound->Calloc(csound,(size_t)129 * sizeof(int32_t));
    if (UNLIKELY(ST(insrfd) == NULL)) {
      csound->Message(csound, Str("insufficient memory to initialise "
                                  "insrfd table."));
      goto error;
    }

    ST(chnrfd) = (int32_t*) csound->Calloc(csound,(size_t)17 * sizeof(int32_t));
    if (UNLIKELY(ST(chnrfd) == NULL)) {
      csound->Message(csound, Str("insufficient memory to initialise "
                                  "chnrfd table."));
      goto error;
    }

    ST(ipadrs) = (char*) csound->Calloc(csound,(size_t)15 * sizeof(char));
    if (UNLIKELY(ST(ipadrs) == NULL)) {
      csound->Message(csound, Str("insufficient memory to initialise "
                                  "local ip address."));
      goto error;
    }

    /* get IP adrs of this machine */
    /* FIXME: don't hardcode eth0 */
    if (UNLIKELY(getIpAddress(ST(ipadrs)) != 0)) {
      csound->Message(csound, Str("unable to get local ip address."));
      goto error;
    }

    return 0;

error:
    /* Clean up anything we may have allocated before running out of memory */
    remote_Cleanup(csound);
    return -1;
}

/* Cleanup the above; called from musmon csoundCleanup */
void remote_Cleanup(CSOUND *csound)
{
    int32_t fd;
    if (csound->remoteGlobals == NULL) return;
    if (ST(socksout) != NULL) {
      SOCK *sop = ST(socksout), *sop_end = sop + MAXREMOTES;
      for ( ; sop < sop_end; sop++)
        if ((fd = sop->rfd) > 0)
          close(fd);
      csound->Free(csound,(char *)ST(socksout));
      ST(socksout) = NULL;
    }
    if (ST(socksin) != NULL) {
      int32_t *sop = ST(socksin), *sop_end = sop + MAXREMOTES;
      for ( ; sop < sop_end; sop++)
        if ((fd = *sop) > 0)
          close(fd);
      csound->Free(csound,(char *)ST(socksin));
      ST(socksin) = NULL;
    }
    if (ST(insrfd_list) != NULL) {
      csound->Free(csound, ST(insrfd_list));   ST(insrfd_list) = NULL;
    }
    if (ST(chnrfd_list) != NULL) {
      csound->Free(csound, ST(chnrfd_list));   ST(chnrfd_list) = NULL; }
    if (ST(insrfd) != NULL) {
      csound->Free(csound, ST(insrfd)); ST(insrfd) = NULL; }
    if (ST(chnrfd) != NULL) {
      csound->Free(csound, ST(chnrfd)); ST(chnrfd) = NULL; }
    if (ST(ipadrs) != NULL) {
      csound->Free(csound, ST(ipadrs)); ST(ipadrs) = NULL; }
    ST(insrfd_count) = ST(chnrfd_count) = 0;
    csound->Free(csound, csound->remoteGlobals);
    csound->remoteGlobals = NULL;
}

static int32_t CLopen(CSOUND *csound, char *ipadrs)     /* Client -- open to send */
{
    int32_t rfd, i;
    SOCK *sop = ST(socksout), *sop_end = sop + MAXREMOTES;
    do {
      if (ipadrs == sop->adr)                      /* if socket already exists */
        return sop->rfd;                           /*   return with that   */
    } while (++sop < sop_end);

    /* create a STREAM (TCP) socket in the INET (IP) protocol */
    if (UNLIKELY(( rfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)) {
      return csound->InitError(csound, Str("could not open remote port"));
    }
    memset(&(ST(to_addr)), 0, sizeof(ST(to_addr)));    /* clear sock mem */
    ST(to_addr).sin_family = AF_INET;                  /* set as INET address */
    /* server IP adr, netwk byt order */
#if defined(WIN32) && !defined(__CYGWIN__)
    ST(to_addr).sin_addr.S_un.S_addr = inet_addr((const char *)ipadrs);
#else
    inet_aton((const char *)ipadrs, &(ST(to_addr).sin_addr));
#endif
    ST(to_addr).sin_port =
      htons((int32_t) ST(remote_port)); /* port we will listen on,
                                                            network byte order */
    for (i=0; i<10; i++){
      if (UNLIKELY(connect(rfd, (struct sockaddr *) &ST(to_addr),
                           sizeof(ST(to_addr))) < 0))
        csound->Message(csound, Str("---> Could not connect\n"));
      else goto conok;
    }
    close(rfd);
    return csound->InitError(csound,
                             Str("---> Failed all attempts to connect.\n"));

 conok:
    csound->Message(csound, Str("--->  Connected.\n"));
    for (sop = ST(socksout); sop < sop_end; sop++)
      if (sop->adr == NULL) {
        sop->adr = ipadrs;                         /* record the new socket */
        sop->rfd = rfd;
        break;
      }
    return rfd;
}

int32_t CLsend(CSOUND *csound, int32_t conn, void *data, int32_t length)
{
    int32_t nbytes;
    if (UNLIKELY((nbytes = write(conn, data, length)) <= 0)) {
      csound->ErrorMsg(csound, Str("write to socket failed"));
      return NOTOK;
    }
    /*    csound->Message(csound, "nbytes sent: %d\n", nbytes); */
    return OK;
}

static int32_t SVopen(CSOUND *csound)
           /* Server -- open to receive */
{
    int32_t conn, socklisten,opt;
    char ipadrs[15];
    int32_t *sop = ST(socksin), *sop_end = sop + MAXREMOTES;
#if defined(WIN32) && !defined(__CYGWIN__)
    int32_t clilen;
#else
    socklen_t clilen;
#endif
    opt = 1;

    if (UNLIKELY((socklisten = socket(PF_INET, SOCK_STREAM, 0)) < 0)) {
      return csound->InitError(csound, Str("creating socket\n"));
    }
    else csound->Message(csound, Str("created socket\n"));
    /* set the addresse to be reusable */
#if defined(WIN32) && !defined(__CYGWIN__)
    if (UNLIKELY( setsockopt(socklisten, SOL_SOCKET, SO_REUSEADDR,
                             (const char*)&opt, sizeof(opt)) < 0 ))
#else
    if (UNLIKELY( setsockopt(socklisten, SOL_SOCKET, SO_REUSEADDR,
                             &opt, sizeof(opt)) < 0 ))
#endif
      return
        csound->InitError(csound,
                          Str("setting socket option to reuse the address\n"));

    memset(&(ST(to_addr)), 0, sizeof(ST(to_addr)));    /* clear sock mem */
    ST(local_addr).sin_family = AF_INET;               /* set as INET address */
    /* our adrs, netwrk byt order */
#if defined(WIN32) && !defined(__CYGWIN__)
    ST(to_addr).sin_addr.S_un.S_addr = inet_addr((const char *)ipadrs);
#else
    inet_aton((const char *)ipadrs, &(ST(local_addr).sin_addr));
#endif
/*     ST(local_addr).sin_port = htons((int32_t)REMOT_PORT); */
    ST(local_addr).sin_port =
      htons((int32_t)ST(remote_port)); /* port we listen on, netwrk byte order */
    /* associate the socket with the address and port */
    if (UNLIKELY(bind (socklisten,
              (struct sockaddr *) &ST(local_addr),
                       sizeof(ST(local_addr))) < 0)) {
      shutdown(socklisten, SHUT_RD);
      return csound->InitError(csound, Str("bind failed"));
    }
    if (UNLIKELY(listen(socklisten, 5) < 0)) {    /* start the socket listening
                                                for new connections -- may wait */
      shutdown(socklisten, SHUT_RD);
      return csound->InitError(csound, Str("listen failed"));
    }
    clilen = sizeof(ST(local_addr));  /* FIX THIS FOR MULTIPLE CLIENTS !!!!!!!*/
    conn = accept(socklisten, (struct sockaddr *) &ST(local_addr), &clilen);
    if (UNLIKELY(conn < 0)) {
      shutdown(socklisten, SHUT_RD);
      return csound->InitError(csound, Str("accept failed"));
    }
    else {
      csound->Message(csound, Str("accepted, conn=%d\n"), conn);
      for (sop = ST(socksin); sop < sop_end; sop++)
        if (*sop == 0) {
          *sop = conn;                       /* record the new connection */
          break;
        }
    }
    shutdown(socklisten, SHUT_RD);
    return OK;
}

int32_t SVrecv(CSOUND *csound, int32_t conn, void *data, int32_t length)
{
    struct sockaddr from;
    /* VL, 12/10/06: I'm guessing here. If someone knows better, fix it */
#if defined(WIN32) && !defined(__CYGWIN__)
#define MSG_DONTWAIT  0
    int32_t clilen = sizeof(from);
#else
    socklen_t clilen = sizeof(from);
#endif
    size_t n;
    IGN(csound);
    n = recvfrom(conn, data, length, MSG_DONTWAIT, &from, &clilen);
    /*  if (n>0) csound->Message(csound, "nbytes received: %d\n", (int32_t)n); */
    return (int32_t)n;
}

/* /////////////  INSTR 0 opcodes ///////////////////// */

int32_t remoteport(CSOUND *csound, REMOTEPORT *p)
{
    if (csound->remoteGlobals==NULL) {
      if (UNLIKELY(callox(csound) < 0)) {
        return
          csound->InitError(csound, Str("failed to initialise remote globals."));
      }
    }
    if (ST(socksin) == NULL) {
      if (*p->port <= FL(0.0))
        ST(remote_port) = REMOT_PORT;
      else
        ST(remote_port) = (int32_t)(*p->port+FL(0.5));
      return OK;
    }
    return NOTOK;
}

int32_t insremot(CSOUND *csound, INSREMOT *p)
/* declare certain instrs for remote Csounds */
{   /*      INSTR 0 opcode  */
    int16 nargs = p->INOCOUNT;

    if (csound->remoteGlobals==NULL || ST(socksin) == NULL) {
      if (UNLIKELY(callox(csound) < 0)) {
        return
          csound->InitError(csound, Str("failed to initialise remote globals."));
      }
    }
    if (UNLIKELY(nargs < 3)) {
      return csound->InitError(csound, Str("missing instr nos"));
    }
/*     csound->Message(csound, Str("*** str1: %s own:%s\n"), */
/*                     (char *)p->str1 , ST(ipadrs)); */
    if (strcmp(ST(ipadrs), (char *)p->str1->data) == 0) {
      /* if client is this adrs */
      MYFLT   **argp = p->insno;
      int32_t rfd = 0;
      if ((rfd = CLopen(csound, (char *)p->str2->data)) < 0)
        /* open port to remote */
        return NOTOK;
      for (nargs -= 2; nargs--; ) {
        int16 insno = (int16)**argp++;     /* & for each insno */
        if (UNLIKELY(insno <= 0)) {
          close(rfd);
          return csound->InitError(csound, Str("illegal instr no"));
        }
        if (UNLIKELY(ST(insrfd)[insno])) {
          close(rfd);
          return csound->InitError(csound, Str("insno already remote"));
        }
        ST(insrfd)[insno] = rfd;   /*  record file descriptor   */
      }
      ST(insrfd_list)[ST(insrfd_count)++] = rfd;   /*  and make a list    */
    }
    else if (!strcmp(ST(ipadrs),(char *)p->str2->data)) {
      /* if server is this adrs*/
/*       csound->Message(csound, Str("*** str2: %s own:%s\n"), */
/*                       (char *)p->str2 , ST(ipadrs)); */
      /* open port to listen */
      if (UNLIKELY(SVopen(csound) == NOTOK)){
        return csound->InitError(csound, Str("Failed to open port to listen"));
      }
    }
    return OK;
}

int32_t insglobal(CSOUND *csound, INSGLOBAL *p)
/* declare certain instrs global remote Csounds */
{   /*      INSTR 0 opcode  */
    int16 nargs = p->INOCOUNT;

    if (csound->remoteGlobals==NULL || ST(socksin) == NULL) {
      if (UNLIKELY(callox(csound) < 0)) {
        return
          csound->InitError(csound, Str("failed to initialise remote globals."));
      }
    }
    if (UNLIKELY(nargs < 2)) {
      return csound->InitError(csound, Str("missing instr nos"));
    }
    csound->Message(csound, Str("*** str1: %s own:%s\n"),
                    (char *)p->str1->data , ST(ipadrs));
    if (strcmp(ST(ipadrs), (char *)p->str1->data) == 0) {
      /* if client is this adrs */
      MYFLT   **argp = p->insno;
      for (nargs -= 1; nargs--; ) {
        int16 insno = (int16)**argp++;             /* for each insno */
        if (UNLIKELY(insno <= 0 || insno > 128)) {
          return csound->InitError(csound, Str("illegal instr no"));
        }
        if (UNLIKELY(ST(insrfd)[insno])) {
          return csound->InitError(csound, Str("insno already specific remote"));
        }
        ST(insrfd)[insno] = GLOBAL_REMOT;             /*  mark as GLOBAL   */
      }
    }
    return OK;
}

int32_t midremot(CSOUND *csound, MIDREMOT *p)    /* declare certain channels for
                                                remote Csounds */
{                                            /* INSTR 0 opcode  */
    int16 nargs = p->INOCOUNT;

    if (csound->remoteGlobals==NULL || ST(socksin) == NULL) {
      if (UNLIKELY(callox(csound) < 0)) {
        return
          csound->InitError(csound, Str("failed to initialise remote globals."));
      }
    }
    if (UNLIKELY(nargs < 3)) {
      return csound->InitError(csound, Str("missing channel nos"));
    }
    if (strcmp(ST(ipadrs), (char *)p->str1->data) == 0) {
      /* if client is this adrs */
      MYFLT   **argp = p->chnum;
      int32_t  rfd;
        /* open port to remote */
      if (UNLIKELY((rfd = CLopen(csound, (char *)p->str2->data)) < 0))
        return NOTOK;
      for (nargs -= 2; nargs--; ) {
        int16 chnum = (int16)**argp++;               /* & for each channel   */
        if (UNLIKELY(chnum <= 0 || chnum > 16)) {    /* THESE ARE MIDCHANS+1 */
          close(rfd);
          return csound->InitError(csound, Str("illegal channel no"));
        }
        if (UNLIKELY(ST(chnrfd)[chnum])) {
          close(rfd);
          return csound->InitError(csound, Str("channel already remote"));
        }
        ST(chnrfd)[chnum] = rfd;                      /* record file descriptor */
      }
      ST(chnrfd_list)[ST(chnrfd_count)++] = rfd;   /* and make a list */
    }
    else if (!strcmp(ST(ipadrs), (char *)p->str2->data)) {
      /* if server is this adrs */
      /* open port to listen */
      if (UNLIKELY(SVopen(csound) == NOTOK)){
        return csound->InitError(csound, Str("Failed to open port to listen"));
      }
      csound->oparms->RMidiin = 1;            /* & enable rtevents in */
    }
    return OK;
}

int32_t midglobal(CSOUND *csound, MIDGLOBAL *p)
/* declare certain chnls global remote Csounds */
{                                         /*       INSTR 0 opcode  */
    int16 nargs = p->INOCOUNT;

    if (csound->remoteGlobals==NULL || ST(socksin) == NULL) {
      if (UNLIKELY(callox(csound) < 0)) {
        return
          csound->InitError(csound, Str("failed to initialise remote globals."));
      }
    }
    if (UNLIKELY(nargs < 2)) {
      return csound->InitError(csound, Str("missing channel nos"));
    }
/*     csound->Message(csound, Str("*** str1: %s own:%s\n"), */
/*                     (char *)p->str1 , ST(ipadrs)); */
    if (strcmp(ST(ipadrs), (char *)p->str1->data) == 0) {
      /* if client is this adrs */
      MYFLT   **argp = p->chnum;
      for (nargs -= 1; nargs--; ) {
        int16 chnum = (int16)**argp++;             /* for each channel */
        if (UNLIKELY(chnum <= 0 || chnum > 16)) {
          return csound->InitError(csound, Str("illegal channel no"));
        }
        if (UNLIKELY(ST(chnrfd)[chnum])) {
          return csound->InitError(csound, Str("channel already specific remote"));
        }
        ST(chnrfd)[chnum] = GLOBAL_REMOT;              /*  mark as GLOBAL   */
      }
    }
    return OK;
}

/* ////////////////       MUSMON SERVICES //////////////// */

int32_t insSendevt(CSOUND *csound, EVTBLK *evt, int32_t rfd)
{
    REMOT_BUF *bp = &ST(CLsendbuf);
    EVTBLK *cpp = (EVTBLK *)bp->data;       /* align an EVTBLK struct */
    int32_t nn;
    MYFLT *f, *g;
    cpp->pinstance = NULL;
    cpp->strarg = NULL;                     /* copy the initial header */
    cpp->scnt = 0;
    cpp->opcod = evt->opcod;
    cpp->pcnt = evt->pcnt;
    f = &evt->p2orig;
    g = &cpp->p2orig;
    for (nn = evt->pcnt + 3; nn--; )        /* copy the remaining data */
      *g++ = *f++;
    bp->type = SCOR_EVT;                    /* insert type and len */
    bp->len = (char *)g - (char *)bp;
    if (UNLIKELY(CLsend(csound, rfd, (void *)bp, (int32_t)bp->len) < 0)) {
      csound->ErrorMsg(csound, Str("CLsend failed"));
      return NOTOK;
    }
    else return OK;
}

int32_t insGlobevt(CSOUND *csound, EVTBLK *evt)
{  /* send an event to all remote fd's */
    int32_t nn;
    for (nn = 0; nn < ST(insrfd_count); nn++) {
      if (UNLIKELY(insSendevt(csound, evt, ST(insrfd_list)[nn]) == NOTOK))
        return NOTOK;
    }
    return OK;
}

int32_t MIDIsendevt(CSOUND *csound, MEVENT *evt, int32_t rfd)
{
    REMOT_BUF *bp = &ST(CLsendbuf);
    MEVENT *mep = (MEVENT *)bp->data;       /* align an MEVENT struct */
    *mep = *evt;                            /*      & copy the data   */
    bp->type = MIDI_EVT;                    /* insert type and len    */
    bp->len = sizeof(int32_t) * 2 + sizeof(MEVENT);

    if (UNLIKELY(CLsend(csound, rfd, (void *)bp, (size_t)bp->len) < 0)) {
      csound->ErrorMsg(csound, Str("CLsend failed"));
      return NOTOK;
    }
    else return OK;
}

int32_t MIDIGlobevt(CSOUND *csound, MEVENT *evt)
{  /* send an Mevent to all remote fd's */
    int32_t nn;
    for (nn = 0; nn < ST(chnrfd_count); nn++) {
      if (UNLIKELY(MIDIsendevt(csound, evt, ST(chnrfd_list)[nn]) == NOTOK))
        return NOTOK;
    }
    return OK;
}

int32_t MIDIsend_msg(CSOUND *csound, MEVENT *evt, int32_t rfd)
{
    REMOT_BUF *bp = &ST(CLsendbuf);
    MEVENT *mep = (MEVENT *)bp->data;       /* align an MEVENT struct */
    *mep = *evt;                            /*      & copy the data   */
    bp->type = MIDI_MSG;                    /* insert type and len    */
    bp->len = sizeof(int32_t) * 2 + sizeof(MEVENT);

    if (UNLIKELY(CLsend(csound, rfd, (void *)bp, (size_t)bp->len) < 0)) {
      csound->ErrorMsg(csound, Str("CLsend failed"));
      return NOTOK;
    }
    else return OK;
}

/* send an M chnmsg to all remote fd's */
int32_t MIDIGlob_msg(CSOUND *csound, MEVENT *evt)
{
    int32_t nn;
    for (nn = 0; nn < ST(chnrfd_count); nn++) {
      if (UNLIKELY(MIDIsend_msg(csound, evt, ST(chnrfd_list)[nn]) == NOTOK))
        return NOTOK;
    }
    return OK;
}

int32_t getRemoteInsRfd(CSOUND *csound, int32_t insno)
{
    if (csound->remoteGlobals && ST(insrfd))
        return ST(insrfd)[insno];
    else return 0;
}

int32_t getRemoteInsRfdCount(CSOUND *csound)
{
    if (csound->remoteGlobals)  return ST(insrfd_count);
    else return 0;
}

int32_t getRemoteChnRfd(CSOUND *csound, int32_t chan)
{
    if (csound->remoteGlobals && ST(chnrfd))
        return ST(chnrfd)[chan];
    else return 0;
}

int32_t* getRemoteSocksIn(CSOUND *csound)
{
    if (csound->remoteGlobals)
       return ST(socksin);
    else return 0;
}

#else /* HAVE_SOCKETS not defined */

char remoteID(CSOUND *csound)
{
    return '\0';
}

/* Cleanup the above; called from musmon csoundCleanup */
void remote_Cleanup(CSOUND *csound)
{
    csound->remoteGlobals = NULL;
    return;
}

int32_t SVrecv(CSOUND *csound, int32_t conn, void *data, int32_t length)
{
    return 0;
}

/*  INSTR 0 opcodes  */

int32_t remoteport(CSOUND *csound, REMOTEPORT *p)
{
    csound->Warning(csound, Str("*** This version of Csound was not "
            "compiled with remote event support ***\n"));
    return OK;
}

int32_t insremot(CSOUND *csound, INSREMOT *p)
/* declare certain instrs for remote Csounds */
{
    csound->Warning(csound, Str("*** This version of Csound was not "
            "compiled with remote event support ***\n"));
    return OK;
}

int32_t insglobal(CSOUND *csound, INSGLOBAL *p)
/* declare certain instrs global remote Csounds */
{
    csound->Warning(csound, Str("*** This version of Csound was not "
            "compiled with remote event support ***\n"));
    return OK;
}

int32_t midremot(CSOUND *csound, MIDREMOT *p)
/* declare certain channels for remote Csounds */
{
    csound->Warning(csound, Str("*** This version of Csound was not "
            "compiled with remote event support ***\n"));
    return OK;
}

int32_t midglobal(CSOUND *csound, MIDGLOBAL *p)
/* declare certain chnls global remote Csounds */
{
    csound->Warning(csound, Str("*** This version of Csound was not "
            "compiled with remote event support ***\n"));
    return OK;
}

/*  MUSMON SERVICES  */

int32_t insSendevt(CSOUND *csound, EVTBLK *evt, int32_t rfd)
{
    return OK;
}

int32_t insGlobevt(CSOUND *csound, EVTBLK *evt)
/* send an event to all remote fd's */
{
    return OK;
}

int32_t MIDIsendevt(CSOUND *csound, MEVENT *evt, int32_t rfd)
{
    return OK;
}

int32_t MIDIGlobevt(CSOUND *csound, MEVENT *evt)
/* send an Mevent to all remote fd's */
{
    return OK;
}

int32_t getRemoteInsRfd(CSOUND *csound, int32_t insno)
{
    return 0;
}

int32_t getRemoteInsRfdCount(CSOUND *csound)
{
    return 0;
}

int32_t getRemoteChnRfd(CSOUND *csound, int32_t chan)
{
    return 0;
}

int32_t* getRemoteSocksIn(CSOUND *csound)
{
    return NULL;
}

#endif /* HAVE_SOCKETS */
