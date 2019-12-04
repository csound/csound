/*
  server.c:

  Copyright (C) 2013 V Lazzarini, John ffitch

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
#ifdef NACL
typedef unsigned int u_int32_t;
#endif

/* Haiku 'int32' etc definitions in net headers conflict with sysdep.h */
#define __HAIKU_CONFLICT

#include "csoundCore.h"
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

typedef struct {
  int port;
  int     sock;
  CSOUND  *cs;
  void    *thrid;
  void  *cb;
  struct sockaddr_in server_addr;
  unsigned char status;
} UDPCOM;

#define MAXSTR 1048576 /* 1MB */

static void udp_socksend(CSOUND *csound, int *sock, const char *addr,
                         int port, const char *msg) {
  struct sockaddr_in server_addr;
  if(*sock <= 0) {
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      csound->Warning(csound, Str("UDP: Winsock2 failed to start: %d"), err);
    return;
#endif
    *sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(*sock < 0)) {
      csound->Warning(csound, Str("UDP: error creating socket"));
      return;
    }
#ifndef WIN32
    if (UNLIKELY(fcntl(*sock, F_SETFL, O_NONBLOCK)<0)) {
      csound->Warning(csound, Str("UDP Server: Cannot set nonblock"));
      if (*sock>=0) close(*sock);
      return;
    }
#else
    {
      u_long argp = 1;
      err = ioctlsocket(*sock, FIONBIO, &argp);
      if (UNLIKELY(err != NO_ERROR)) {
        csound->Warning(csound, Str("UDP Server: Cannot set nonblock"));
        closesocket(*sock);
        return;
      }
    }
#endif

  }
  server_addr.sin_family = AF_INET;    /* it is an INET address */
#if defined(WIN32) && !defined(__CYGWIN__)
  server_addr.sin_addr.S_un.S_addr = inet_addr(addr);
#else
  inet_aton(addr, &server_addr.sin_addr);    /* the server IP address */
#endif
  server_addr.sin_port = htons((int) port);    /* the port */

  if (UNLIKELY(sendto(*sock, (void*) msg, strlen(msg)+1, 0,
                      (const struct sockaddr *) &server_addr,
                      sizeof(server_addr)) < 0)) {
    csound->Warning(csound,  Str("UDP: sock end failed"));
  }
}


static uintptr_t udp_recv(void *pdata){
  struct sockaddr from;
  socklen_t clilen = sizeof(from);
  UDPCOM *p = (UDPCOM *) pdata;
  CSOUND *csound = p->cs;
  int port = p->port;
  char *orchestra = csound->Calloc(csound, MAXSTR);
  int sock = 0;
  int received, cont = 0;
  char *start = orchestra;
  size_t timout = (size_t) lround(1000/csound->GetKr(csound));

  csound->Message(csound, Str("UDP server started on port %d\n"),port);
  while (p->status) {
    if ((received =
         recvfrom(p->sock, (void *)orchestra, MAXSTR, 0, &from, &clilen)) <= 0) {
      csoundSleep(timout ? timout : 1);
      continue;
    }
    else {
      orchestra[received] = '\0'; // terminate string
      if(strlen(orchestra) < 2) continue;
      if (csound->oparms->echo)
        csound->Message(csound, "%s", orchestra);
      if (strncmp("!!close!!",orchestra,9)==0 ||
          strncmp("##close##",orchestra,9)==0) {
        csoundInputMessageAsync(csound, "e 0 0");
        break;
      }
      if(*orchestra == '&') {
        csoundInputMessageAsync(csound, orchestra+1);
      }
      else if(*orchestra == '$') {
        csoundReadScoreAsync(csound, orchestra+1);
      }
      else if(*orchestra == '@') {
        char chn[128];
        MYFLT val;
        sscanf(orchestra+1, "%s", chn);
        val = atof(orchestra+1+strlen(chn));
        csoundSetControlChannel(csound, chn, val);
      }
      else if(*orchestra == '%') {
        char chn[128];
        char *str;
        sscanf(orchestra+1, "%s", chn);
        str = cs_strdup(csound, orchestra+1+strlen(chn));
        csoundSetStringChannel(csound, chn, str);
        csound->Free(csound, str);
      }
      else if(*orchestra == ':') {
        char addr[128], chn[128], *msg;
        int sport, err = 0;
        MYFLT val;
        sscanf(orchestra+2, "%s", chn);
        sscanf(orchestra+2+strlen(chn), "%s", addr);
        sport = atoi(orchestra+3+strlen(addr)+strlen(chn));
        if(*(orchestra+1) == '@') {
          val = csoundGetControlChannel(csound, chn, &err);
          msg = (char *) csound->Calloc(csound, strlen(chn) + 32);
          sprintf(msg, "%s::%f", chn, val);
        }
        else if (*(orchestra+1) == '%') {
          MYFLT  *pstring;
          if (csoundGetChannelPtr(csound, &pstring, chn,
                                  CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL)
              == CSOUND_SUCCESS) {
            STRINGDAT* stringdat = (STRINGDAT*) pstring;
            int size = stringdat->size;
            spin_lock_t *lock =
              (spin_lock_t *) csoundGetChannelLock(csound, (char*) chn);
            msg = (char *) csound->Calloc(csound, strlen(chn) + size);
            if (lock != NULL)
              csoundSpinLock(lock);
            sprintf(msg, "%s::%s", chn, stringdat->data);
            if (lock != NULL)
              csoundSpinUnLock(lock);
          } else err = -1;
        }
        else err = -1;
        if(!err) {
          udp_socksend(csound, &sock, addr, sport,msg);
          csound->Free(csound, msg);
        }
        else
          csound->Warning(csound, Str("could not retrieve channel %s"), chn);
      }
      else if(*orchestra == '{' || cont) {
        char *cp;
        if((cp = strrchr(orchestra, '}')) != NULL) {
          if(*(cp-1) != '}') {
            *cp = '\0';
            cont = 0;
          }  else {
            orchestra += received;
            cont = 1;
          }
        }
        else {
          orchestra += received;
          cont = 1;
        }
        if(!cont) {
          orchestra = start;
          //csound->Message(csound, "%s\n", orchestra+1);
          csoundCompileOrcAsync(csound, orchestra+1);
        }
      }
      else {
        //csound->Message(csound, "%s\n", orchestra);
        csoundCompileOrcAsync(csound, orchestra);
      }
    }
  }
  csound->Message(csound, Str("UDP server on port %d stopped\n"),port);
  csound->Free(csound, start);
  // csound->Message(csound, "orchestra dealloc\n");
  if(sock > 0)
#ifndef WIN32
    close(sock);
#else
  closesocket(sock);
#endif
  return (uintptr_t) 0;

}

static int udp_start(CSOUND *csound, UDPCOM *p)
{
#if defined(WIN32) && !defined(__CYGWIN__)
  WSADATA wsaData = {0};
  int err;
  if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)){
    csound->Warning(csound, Str("Winsock2 failed to start: %d"), err);
    return CSOUND_ERROR;
  }
#endif
  p->cs = csound;
  p->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef WIN32
  if (UNLIKELY(fcntl(p->sock, F_SETFL, O_NONBLOCK)<0)) {
    csound->Warning(csound, Str("UDP Server: Cannot set nonblock"));
    if (p->sock>=0) close(p->sock);
    return CSOUND_ERROR;
  }
#else
  {
    u_long argp = 1;
    err = ioctlsocket(p->sock, FIONBIO, &argp);
    if (UNLIKELY(err != NO_ERROR)) {
      csound->Warning(csound, Str("UDP Server: Cannot set nonblock"));
      closesocket(p->sock);
      return CSOUND_ERROR;
    }
  }
#endif
  if (UNLIKELY(p->sock < 0)) {
    csound->Warning(csound, Str("error creating socket"));
    return CSOUND_ERROR;
  }
  /* create server address: where we want to send to and clear it out */
  memset(&p->server_addr, 0, sizeof(p->server_addr));
  p->server_addr.sin_family = AF_INET;    /* it is an INET address */
  p->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  p->server_addr.sin_port = htons((int) p->port);    /* the port */
  /* associate the socket with the address and port */
  if (UNLIKELY(bind(p->sock, (struct sockaddr *) &p->server_addr,
                    sizeof(p->server_addr)) < 0)) {
    csound->Warning(csound, Str("bind failed"));
    p->thrid = NULL;
#ifndef WIN32
    close(p->sock);
#else
    closesocket(p->sock);
#endif
    return CSOUND_ERROR;
  }
  /* set status flag */
  p->status = 1;
  /* create thread */
  p->thrid = csoundCreateThread(udp_recv, (void *) p);
  return CSOUND_SUCCESS;
}

int csoundUDPServerClose(CSOUND *csound)
{
  UDPCOM *p = (UDPCOM *) csound->QueryGlobalVariable(csound,"::UDPCOM");
  if (p != NULL) {
    /* unset status flag */
    p->status = 0;
    /* wait for server thread to close */
    csoundJoinThread(p->thrid);
    /* close socket */
#ifndef WIN32
    close(p->sock);
#else
    closesocket(p->sock);
#endif
    csound->DestroyGlobalVariable(csound,"::UDPCOM");
    return CSOUND_SUCCESS;
  }
  else return CSOUND_ERROR;
}

int csoundUDPServerStart(CSOUND *csound, unsigned int port){
  UDPCOM *connection;
  csound->CreateGlobalVariable(csound, "::UDPCOM", sizeof(UDPCOM));
  connection = (UDPCOM *) csound->QueryGlobalVariable(csound, "::UDPCOM");
  if (connection != NULL){
    connection->port = port;
    if(connection->status) {
      csound->Warning(csound,  Str("UDP Server: already running"));
      return CSOUND_ERROR;
    }
    else {
      int res = udp_start(csound, connection);
      if (res  != CSOUND_SUCCESS) {
        csound->Warning(csound,  Str("UDP Server: could not start"));
        csound->DestroyGlobalVariable(csound,"::UDPCOM");
        return CSOUND_ERROR;
      }
      else return CSOUND_SUCCESS;
    }
  }
  else {
    csound->Warning(csound,  Str("UDP Server: failed to allocate memory"));
    return CSOUND_ERROR;
  }
}

int csoundUDPServerStatus(CSOUND *csound) {
  UDPCOM *p = (UDPCOM *) csound->QueryGlobalVariable(csound,"::UDPCOM");
  if (p != NULL) {
    return p->port;
  }
  else return CSOUND_ERROR;
}

#define UDPMSG 1024

typedef struct {
  int port;
  const char *addr;
  int sock;
  void (*cb)(CSOUND *csound,int attr, const char *format, va_list args);
} UDPCONS;


static void udp_msg_callback(CSOUND *csound, int attr, const char *format,
                             va_list args) {
  UDPCONS *p;
  p = (UDPCONS *) csound->QueryGlobalVariable(csound, "::UDPCONS");
  if(p) {
    char string[UDPMSG];
    va_list nargs;
    va_copy(nargs, args);
    vsnprintf(string, UDPMSG, format, args);
    udp_socksend(csound, &(p->sock), p->addr, p->port, string);
    if(p->cb)
      p->cb(csound, attr, format, nargs);
     va_end(nargs);
  }
}

static int udp_console_stop(CSOUND *csound, void *pp) {
  UDPCONS *p = (UDPCONS *) pp;
  if(p) {
    csoundSetMessageCallback(csound, p->cb);
#ifndef WIN32
    close(p->sock);
#else
    closesocket(p->sock);
#endif
    csound->DestroyGlobalVariable(csound,"::UDPCONS");
  }
  return CSOUND_SUCCESS;
}


int csoundUDPConsole(CSOUND *csound, const char *addr, int port, int
                     mirror) {
  UDPCONS *p = (UDPCONS *) csound->QueryGlobalVariable(csound, "::UDPCONS");
  if(p == NULL) {
    csound->CreateGlobalVariable(csound, "::UDPCONS", sizeof(UDPCONS));
    p = (UDPCONS *) csound->QueryGlobalVariable(csound, "::UDPCONS");
    if(p) {
      p->port = port;
      p->addr = cs_strdup(csound, (char *) addr);
      p->sock = 0;
      if(mirror)
        p->cb = csound->csoundMessageCallback_;
      csound->SetMessageCallback(csound, udp_msg_callback);
      csound->RegisterResetCallback(csound, p, udp_console_stop);
    } else {
      csound->Warning(csound, "Could not set UDP console\n");
      return CSOUND_ERROR;
    }
    return CSOUND_SUCCESS;
  }
  return CSOUND_ERROR;
}

void csoundStopUDPConsole(CSOUND *csound) {
  UDPCONS *p;
  csound->CreateGlobalVariable(csound, "::UDPCONS", sizeof(UDPCONS));
  p = (UDPCONS *) csound->QueryGlobalVariable(csound, "::UDPCONS");
  udp_console_stop(csound, p);
}
