/*
    ipmidi.c:

    Copyright (C) 2012 Henri Manson

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

/* Realtime MIDI using ipmidi library */

/* Haiku 'int32' etc definitions in net headers conflict with sysdep.h */
#define __HAIKU_CONFLICT

#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <errno.h>

#include "csdl.h"                               /*      IPMIDI.C         */
#include "midiops.h"
#include "oload.h"

static int OpenMidiInDevice_(CSOUND *csound, void **userData, const char *dev)
{
    static int sock;
    int status;
    struct sockaddr_in saddr;
    struct ip_mreq mreq;

#ifdef WIN32
    WSADATA wsaData;
    if (WSAStartup (MAKEWORD(2, 2), &wsaData) != 0) {
      fprintf(stderr, "%s", Str("WSAStartup failed!\n"));
      return -1;
    }
#endif
    printf("OpenMidiInDevice_: %s\n", dev);
    // set content of struct saddr and imreq to zero
    memset(&saddr, 0, sizeof(struct sockaddr_in));

    // open a UDP socket
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if ( sock < 0 ) {
      perror(Str("Error creating socket"));
      return -1;
    }

    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(21928);

    status = bind(sock, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in));
    if ( status < 0 ) {
#ifdef WIN32
      char *buff = strerror(errno);
      printf("WSAGetLastError() = %d\n", WSAGetLastError());
#else
      char buff[128];
      ignore_value(strerror_r(errno, buff, 128));
#endif

        csound->ErrorMsg(csound, Str("Error binding socket to interface: %s"),
                          buff);
      //perror("Error binding socket to interface");
      return -1;
    }

    mreq.imr_multiaddr.s_addr = inet_addr("225.0.0.37");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    status = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                        (const char *)&mreq, sizeof(mreq));

    if ( status < 0 ) {
#ifdef WIN32
        char *buff = strerror(errno);
        csound->ErrorMsg(csound, "WSAGetLastError() = %d\n", WSAGetLastError());
        return -1;
#else
      char buff[128];
      ignore_value(strerror_r(errno, buff, 128));

      csound->ErrorMsg(csound, Str("Error adding membership to interface: %s"),
                       buff);
      return NOTOK;
      //perror("Error binding socket to interface");
#endif
    }

    *userData = (void*) &sock;
    /* report success */
    return 0;
}

static int ReadMidiData_(CSOUND *csound, void *userData,
                         unsigned char *mbuf, int nbytes)
{
     IGN(csound);
    int             n;
    int             sock = *((int *) userData);
    fd_set          rset;
    struct timeval  timeout;
    int             rc;

    n = 0;
    FD_ZERO(&rset);
    FD_SET(sock, &rset);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    rc = select(sock + 1, &rset, NULL, NULL, &timeout);
    if (rc > 0) {
#ifdef WIN32
      n = recv(sock, mbuf, nbytes, 0);
#else
      n = read(sock, mbuf, nbytes);
#endif
      printf("ReadMidiData__ n = %d\n", n);
    }

    /* return the number of bytes read */
    return n;
}

static int CloseMidiInDevice_(CSOUND *csound, void *userData)
{
     IGN(csound);
    int             sock = *((int *) userData);
    //printf("CloseMidiInDevice_\n");
    close(sock);
#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
     OPARMS oparms;
     csound->GetOParms(csound, &oparms);
    /* nothing to do, report success */
    if (oparms.msglevel & 0x400)
      csound->Message(csound, "%s",
                      Str("ipMIDI real time MIDI plugin for Csound\n"));
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *drv;
    OPARMS oparms;
    csound->GetOParms(csound, &oparms);

    drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));
    if (drv == NULL)
      return 0;
    if (strcmp(drv, "ipmidi") != 0)
      return 0;
    if (oparms.msglevel & 0x400)
      csound->Message(csound, "%s", Str("ipmidi: ipMIDI module enabled\n"));
    csound->SetExternalMidiInOpenCallback(csound, OpenMidiInDevice_);
    csound->SetExternalMidiReadCallback(csound, ReadMidiData_);
    csound->SetExternalMidiInCloseCallback(csound, CloseMidiInDevice_);
    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    /* does not depend on MYFLT type */
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8));
}
