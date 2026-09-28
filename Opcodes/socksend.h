/*
  socksend.c:

  Copyright (C) 2006 by John ffitch
                2018 by Victor Lazzarini

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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifndef CSOUND_SOCKSEND_H
#define CSOUND_SOCKSEND_H

typedef struct {
  OPDS    h;
  cs_float   *asig;
  STRINGDAT *ipaddress;
  cs_float *port, *buffersize;
  cs_float   *format;
  AUXCH   aux;
  int32_t     sock, init_done;
  int32_t     bsize, wp;
  int32_t     ff, bwidth;
  struct sockaddr_in server_addr;
} SOCKSEND;

typedef struct {
  OPDS    h;
  STRINGDAT *str;
  STRINGDAT *ipaddress;
  cs_float *port, *buffersize;
  cs_float   *format;
  AUXCH   aux;
  int32_t     sock, init_done;
  int32_t     bsize, wp;
  int32_t     ff, bwidth;
  struct sockaddr_in server_addr;
} SOCKSENDT;

typedef struct {
  OPDS    h;
  cs_float   *asigl, *asigr;
  STRINGDAT *ipaddress;
  cs_float *port, *buffersize;
  cs_float   *format;
  AUXCH   aux;
  int32_t     sock, init_done;
  int32_t     bsize, wp;
  int32_t     ff, bwidth;
  struct sockaddr_in server_addr;
} SOCKSENDS;

typedef struct {
  OPDS h;
  cs_float *kwhen;
  STRINGDAT *ipaddress;
  cs_float *port;        /* UDP port */
  STRINGDAT *dest;
  STRINGDAT *type;
  cs_float *arg[32];     /* only 26 can be used, but add a few more for safety */
  AUXCH   aux;
  AUXCH   types;
#if defined(WIN32) && !defined(__CYGWIN__)
  SOCKET sock;
#else
  int32_t sock;
#endif
  int32_t ntypes;
  cs_float   last;
  struct sockaddr_in server_addr;
  int32_t err_state;
  int32_t init_done;
  int32_t fstime;
} OSCSEND2;

typedef struct {
  OPDS h;
  cs_float *kwhen;
  STRINGDAT *ipaddress;
  cs_float *port;        /* UDP port */
  ARRAYDAT *dest;
  ARRAYDAT *type;
  ARRAYDAT *arg;
  cs_float *imtu;
  int32_t mtu;
  AUXCH   aux;    /* MTU bytes */
  int32_t sock, init_done;
  cs_float   last;
  struct sockaddr_in server_addr;
  int32_t first;
} OSCBUNDLE;

#endif
