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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/
#ifdef NACL
typedef unsigned int u_int32_t;
#endif

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
} UDPCOM;

#define MAXSTR 1048576 /* 1MB */

static void udp_socksend(CSOUND *csound, int *sock, const char *addr, int port, const char *msg) {
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
   }
    server_addr.sin_family = AF_INET;    /* it is an INET address */
#if defined(WIN32) && !defined(__CYGWIN__)
    server_addr.sin_addr.S_un.S_addr = inet_addr(addr);
#else
    inet_aton(addr, &server_addr.sin_addr);    /* the server IP address */
#endif
   server_addr.sin_port = htons((int) port);    /* the port */

   if (UNLIKELY(sendto(*sock, (void*) msg, strlen(msg)+1, 0, (const struct sockaddr *) &server_addr,
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


  csound->Message(csound, "UDP server started on port %d \n",port);
  while (recvfrom(p->sock, (void *)orchestra, MAXSTR, 0, &from, &clilen) > 0) {
    if(strlen(orchestra) < 2) continue;
    if (csound->oparms->odebug)
      csound->Message(csound, "message: \n%s\n", orchestra);
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
     int sport, err;
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
	int *lock = csoundGetChannelLock(csound, (char*) chn);
        msg = (char *) csound->Calloc(csound, strlen(chn) + size);
       if (lock != NULL)
          csoundSpinLock(lock);
        sprintf(msg, "%s::%s", chn, stringdat->data);
       if (lock != NULL)
         csoundSpinUnLock(lock);
      } else err = -1;
     }
     else err = -1;
     if(!err) udp_socksend(csound, &sock, addr, sport,msg);
     else csound->Warning(csound, "csould not retrieve channel %s", chn);
     csound->Free(csound, msg);
   }
    else {
      csoundCompileOrcAsync(csound, orchestra);
    }
  }
  csound->Message(csound, "UDP server on port %d stopped\n",port);
  csound->Free(csound, orchestra);
  // csound->Message(csound, "orchestra dealloc\n");
  if(sock > 0)
    #ifndef WIN32
    close(p->sock);
#else
    closesocket(p->sock);
#endif
  return (uintptr_t) 0;

}

static int udp_start(CSOUND *csound, UDPCOM *p)
{
#if defined(WIN32) && !defined(__CYGWIN__)
  WSADATA wsaData = {0};
  int err;
  if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
    csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
  p->cs = csound;
  p->sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (UNLIKELY(p->sock < 0)) {
    return csound->InitError
      (csound, Str("creating socket"));
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
    return NOTOK;
  }
  /* create thread */
  p->thrid = csoundCreateThread(udp_recv, (void *) p);

  return OK;
}

int UDPServerClose(CSOUND *csound)
{
  UDPCOM *p = (UDPCOM *) csound->QueryGlobalVariable(csound,"::UDPCOM");

  if (p != NULL) {
#ifndef WIN32
    close(p->sock);
#else
    closesocket(p->sock);
#endif
    //ssize_t ret;
    //const char *mess = "!!close!!";
    //const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);
    //do{
    /* ret = */
    //(void)sendto(p->sock,mess,
    //                    sizeof(mess)+1,0,to,sizeof(p->server_addr));
    //} while(ret != -1);

    //csoundJoinThread(p->thrid);
    csound->DestroyGlobalVariable(csound,"::UDPCOM");
  }
  return OK;
}

int UDPServerStart(CSOUND *csound, int port){
  UDPCOM *connection;
  csound->CreateGlobalVariable(csound, "::UDPCOM", sizeof(UDPCOM));
  connection = (UDPCOM *) csound->QueryGlobalVariable(csound, "::UDPCOM");
  if (connection != NULL){
    connection->port = port;
    return udp_start(csound, connection);
  }
  else return NOTOK;
}
