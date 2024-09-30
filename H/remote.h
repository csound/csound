/*
    remote.h:

    Copyright (C) 2006 by Barry Vercoe

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef CSOUND_REMOTE_H
#define CSOUND_REMOTE_H

#ifdef HAVE_SOCKETS
  #if defined(WIN32) && !defined(__CYGWIN__)
    #include <winsock2.h>
    #ifndef SHUT_RDWR
      #define SHUT_RD   0x00
      #define SHUT_WR   0x01
      #define SHUT_RDWR 0x02
    #endif
  #else
    #include <sys/ioctl.h>
    #ifdef __HAIKU__
      #include <sys/sockio.h>
    #endif
    #include <sys/socket.h>
    #include <netinet/in.h>
    #ifdef MACOSX
      #include <net/if.h>
    #endif
    #ifdef LINUX
      #include <linux/if.h>
    #endif
    #include <arpa/inet.h>
    #ifdef HAVE_UNISTD_H
    #  include <unistd.h>
    #endif
  #endif
#endif /* HAVE_SOCKETS */

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#include <string.h>
#include <errno.h>


void m_chanmsg(CSOUND *csound, MEVENT *mep);   /* called from midirecv & musmon */
char remoteID(CSOUND *csound);

#define REMOT_PORT 40002

#define SCOR_EVT 1
#define MIDI_EVT 2
#define MIDI_MSG 3
#define MAXSEND (sizeof(EVTBLK) + 2*sizeof(int32_t))
#define GLOBAL_REMOT -99

typedef struct {                        /* Remote Communication buffer          */
    int32_t         len;                    /* lentot = len + type + data used      */
    int32_t         type;
    char        data[MAXSEND];
} REMOT_BUF;

#ifdef HAVE_SOCKETS


typedef struct {
    char *adr;
    int32_t   rfd;
} SOCK;

typedef struct {
  SOCK *socksout; /* = NULL; */
  int32_t *socksin; /* = NULL; */
  int32_t *insrfd_list; /* = NULL; */
  int32_t *chnrfd_list; /* = NULL; */
  int32_t insrfd_count; /* = 0; */
  int32_t chnrfd_count; /* = 0; */
  int32_t  *insrfd; /* = NULL; */
  int32_t  *chnrfd; /* = NULL; */
  char *ipadrs; /* = NULL; */
  struct sockaddr_in to_addr;
  struct sockaddr_in local_addr;
  REMOT_BUF CLsendbuf;          /* rt evt output Communications buffer */
  int32_t   remote_port;            /* = 40002 default */
} REMOTE_GLOBALS;

#endif /* HAVE_SOCKETS */

typedef struct {                        /* structs for INSTR 0 opcodes */
    OPDS    h;
    MYFLT   *port;
} REMOTEPORT;

typedef struct {                        /* structs for INSTR 0 opcodes */
    OPDS    h;
   STRINGDAT  *str1, *str2;
   MYFLT *insno[64];
} INSREMOT;

typedef struct {                                /* structs for INSTR 0 opcodes */
    OPDS    h;
    STRINGDAT *str1;
    MYFLT  *insno[64];
} INSGLOBAL;

typedef struct {
    OPDS    h;
  STRINGDAT   *str1, *str2;
  MYFLT  *chnum[16];
} MIDREMOT;

typedef struct {                                /* structs for INSTR 0 opcodes */
    OPDS    h;
  STRINGDAT   *str1;
  MYFLT  *chnum[16];
} MIDGLOBAL;

int32_t CLsend(CSOUND *csound, int32_t conn, void *data, int32_t length);
int32_t SVrecv(CSOUND *csound, int32_t conn, void *data, int32_t length);

/* musmon:      divert a score insno event to a remote machine */
int32_t insSendevt(CSOUND *p, EVTBLK *evt, int32_t rfd);

/* musmon:      send an event (funcs, reverbs) to all active remote machines */
int32_t insGlobevt(CSOUND *p, EVTBLK *evt);

/* musmon:      divert a MIDI channel event to a remote machine */
int32_t MIDIsendevt(CSOUND *p, MEVENT *evt, int32_t rfd);

/* musmon:      send a MIDI channel event (ctrlrs, reverbs) to all
   active remote machines */
int32_t MIDIGlobevt(CSOUND *p, MEVENT *evt);

/* midirecv:    divert a MIDI channel message to a remote machine */
int32_t MIDIsend_msg(CSOUND *p, MEVENT *evt, int32_t rfd);

/* midirecv:    send a MIDI channel message (ctrlrs, reverbs) to all
   active remote machines */
int32_t MIDIGlob_msg(CSOUND *p, MEVENT *evt);

/* musmon: returns the active input sockets # */
int32_t* getRemoteSocksIn(CSOUND *csound);

/* musmon: determine whether an instrument accepts remove events */
int32_t getRemoteInsRfd(CSOUND *csound, int32_t insno);

/* musmon: determine how many instruments accept remove events */
int32_t getRemoteInsRfdCount(CSOUND *csound);

/* musmon: determine whether MIDI channel accepts remove events */
int32_t getRemoteChnRfd(CSOUND *csound, int32_t chan);

#endif      /* CSOUND_REMOTE_H */
