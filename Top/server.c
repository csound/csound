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
#ifdef WIN32
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

static uintptr_t udp_recv(void *pdata)
{
  struct sockaddr from;
  socklen_t clilen = sizeof(from);
  UDPCOM *p = (UDPCOM *) pdata;
  CSOUND *csound = p->cs;
  int port = p->port;
  char   *orchestra = csound->Malloc(csound, MAXSTR);

  csound->Message(csound, "UDP server started on port %d \n",port);
  while(recvfrom(p->sock, (void *)orchestra, MAXSTR, 0, &from, &clilen) > 0) {
   if(csound->oparms->odebug) 
    csound->Message(csound, "orchestra: \n%s\n", orchestra);
   if(strncmp("##close##",orchestra,9)==0) break;
    csoundCompileOrc(csound, orchestra);
    memset(orchestra,0, MAXSTR);
  }
  csound->Message(csound, "UDP server on port %d stopped\n",port);
  csound->Free(csound, orchestra);
  // csound->Message(csound, "orchestra dealloc\n");
  return (uintptr_t) 0;

}

static int udp_start(CSOUND *csound, UDPCOM *p)
{
#ifdef WIN32
  WSADATA wsaData = {0};
  int err;
  if ((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0)
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
		    sizeof(p->server_addr)) < 0)){
  csound->Warning(csound, Str("bind failed"));
  return NOTOK;
  }
  /* create thread */
  p->thrid = csoundCreateThread(udp_recv, (void *) p);

  return OK;
}

int UDPServerClose(CSOUND *csound)
{   
  UDPCOM *p = (UDPCOM *) csound->QueryGlobalVariable(csound,"::UDPCOM"); 
  
  if(p != NULL){
    ssize_t ret;
    const char *mess = "##close##"; 
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);
    //do{
    ret = sendto(p->sock,mess,sizeof(mess)+1,0,to,sizeof(p->server_addr)); 
    //} while(ret != -1);
    csoundJoinThread(p->thrid);
#ifndef WIN32
    close(p->sock);
#else 
    closesocket(p->sock);
#endif
    csound->DestroyGlobalVariable(csound,"::UDPCOM"); 
  }
  return OK;
}

int UDPServerStart(CSOUND *csound, int port){
  UDPCOM *connection;
  csound->CreateGlobalVariable(csound, "::UDPCOM", sizeof(UDPCOM));
  connection = (UDPCOM *) csound->QueryGlobalVariable(csound, "::UDPCOM");
  if(connection != NULL){
    connection->port = port;
    return udp_start(csound, connection);
  } 
  else return NOTOK;
}
