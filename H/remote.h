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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef CSOUND_REMOTE_H
#define CSOUND_REMOTE_H

#ifdef WIN32
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>


extern  int     inet_aton(const char *cp, struct in_addr *inp);
void m_chanmsg(CSOUND *csound, MEVENT *mep);   /* called from midirecv & musmon */
char remoteID(CSOUND *csound);

#define REMOT_PORT 40002

#define SCOR_EVT 1
#define MIDI_EVT 2
#define MIDI_MSG 3
#define MAXSEND (sizeof(EVTBLK) + 2*sizeof(int))
#define GLOBAL_REMOT -99

typedef struct {
    char *adr;
    int   rfd;
} SOCK;

typedef struct {
  SOCK *socksout; /* = NULL; */
  int *socksin; /* = NULL; */
  int *insrfd_list; /* = NULL; */
  int *chnrfd_list; /* = NULL; */
  int insrfd_count; /* = 0; */
  int chnrfd_count; /* = 0; */
  int  *insrfd; /* = NULL; */
  int  *chnrfd; /* = NULL; */
  char *ipadrs; /* = NULL; */
  struct sockaddr_in to_addr;
  struct sockaddr_in local_addr;
} REMOTE_GLOBALS;

typedef struct {                        /* Remote Communication buffer          */
    int         len;                    /* lentot = len + type + data used      */
    int         type;
    char        data[MAXSEND];
} REMOT_BUF;

typedef struct {                        /* structs for INSTR 0 opcodes */
    OPDS    h;
    MYFLT   *str1, *str2, *insno[64];
} INSREMOT;

typedef struct {                                /* structs for INSTR 0 opcodes */
    OPDS    h;
    MYFLT   *str1, *insno[64];
} INSGLOBAL;

typedef struct {
    OPDS    h;
    MYFLT   *str1, *str2, *chnum[16];
} MIDREMOT;

typedef struct {                                /* structs for INSTR 0 opcodes */
    OPDS    h;
    MYFLT   *str1, *chnum[16];
} MIDGLOBAL;

extern int *insrfd;
extern int *chnrfd;
extern int insrfd_count;
extern int chnrfd_count;

int CLsend(CSOUND *csound, int conn, void *data, int length);
int SVrecv(CSOUND *csound, int conn, void *data, int length);

/* musmon:      divert a score insno event to a remote machine */
int insSendevt(CSOUND *p, EVTBLK *evt, int rfd);

/* musmon:      send an event (funcs, reverbs) to all active remote machines */
int insGlobevt(CSOUND *p, EVTBLK *evt);

/* musmon:      divert a MIDI channel event to a remote machine */
int MIDIsendevt(CSOUND *p, MEVENT *evt, int rfd);

/* musmon:      send a MIDI channel event (ctrlrs, reverbs) to all
   active remote machines */
int MIDIGlobevt(CSOUND *p, MEVENT *evt);

/* midirecv:    divert a MIDI channel message to a remote machine */
int MIDIsend_msg(CSOUND *p, MEVENT *evt, int rfd);

/* midirecv:    send a MIDI channel message (ctrlrs, reverbs) to all
   active remote machines */
int MIDIGlob_msg(CSOUND *p, MEVENT *evt);

#endif      /* CSOUND_REMOTE_H */
