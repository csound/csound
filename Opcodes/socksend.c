/*
  socksend.c:

  Copyright (C) 2006 by John ffitch

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
#include <sys/types.h>
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern  int32_t     inet_aton(const char *cp, struct in_addr *inp);

typedef struct {
  OPDS    h;
  MYFLT   *asig;
  STRINGDAT *ipaddress;
  MYFLT *port, *buffersize;
  MYFLT   *format;
  AUXCH   aux;
  int32_t     sock;
  int32_t     bsize, wp;
  int32_t     ff, bwidth;
  struct sockaddr_in server_addr;
} SOCKSEND;

typedef struct {
  OPDS    h;
  STRINGDAT *str;
  STRINGDAT *ipaddress;
  MYFLT *port, *buffersize;
  MYFLT   *format;
  AUXCH   aux;
  int32_t     sock;
  int32_t     bsize, wp;
  int32_t     ff, bwidth;
  struct sockaddr_in server_addr;
} SOCKSENDT;

typedef struct {
  OPDS    h;
  MYFLT   *asigl, *asigr;
  STRINGDAT *ipaddress;
  MYFLT *port, *buffersize;
  MYFLT   *format;
  AUXCH   aux;
  int32_t     sock;
  int32_t     bsize, wp;
  int32_t     ff, bwidth;
  struct sockaddr_in server_addr;
} SOCKSENDS;

#define MTU (1456)

/* UDP version one channel */
static int32_t init_send(CSOUND *csound, SOCKSEND *p)
{
    int32_t     bsize;
    int32_t     bwidth = sizeof(MYFLT);
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
    p->ff = (int32_t)(*p->format);
    p->bsize = bsize = (int32_t) *p->buffersize;
    /* if (UNLIKELY((sizeof(MYFLT) * bsize) > MTU)) { */
    /*   return csound->InitError(csound,
         Str("The buffersize must be <= %d samples " */
    /*                                        "to fit in a udp-packet."), */
    /*                            (int32_t) (MTU / sizeof(MYFLT))); */
    /* } */
    p->wp = 0;

    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data,
              &p->server_addr.sin_addr);    /* the server IP address */
#endif
    p->server_addr.sin_port = htons((int32_t) *p->port);    /* the port */

    if (p->ff) bwidth = sizeof(int16);
    /* create a buffer to write the interleaved audio to  */
    if (p->aux.auxp == NULL || (uint32_t) (bsize * bwidth) > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, (bsize * bwidth), &p->aux);
    else {
      memset(p->aux.auxp, 0, bwidth * bsize);
    }
    p->bwidth = bwidth;
    return OK;
}

static int32_t send_send(CSOUND *csound, SOCKSEND *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t     wp;
    int32_t     buffersize = p->bsize;
    MYFLT   *asig = p->asig;
    MYFLT   *out = (MYFLT *) p->aux.auxp;
    int16   *outs = (int16 *) p->aux.auxp;
    int32_t     ff = p->ff;

    if (UNLIKELY(early)) nsmps -= early;
    for (i = offset, wp = p->wp; i < nsmps; i++, wp++) {
      if (wp == buffersize) {
        /* send the package when we have a full buffer */
        if (UNLIKELY(sendto(p->sock, (void*)out, buffersize  * p->bwidth, 0, to,
                            sizeof(p->server_addr)) < 0)) {
          return csound->PerfError(csound, p->h.insdshead, Str("sendto failed"));
        }
        wp = 0;
      }
      if (ff) { // Scale for 0dbfs and make LE
        int16 val = (int16)((32768.0*asig[i])/csound->e0dbfs);
        union cheat {
          char  benchar[2];
          int16 bensht;
        } ch;
        ch.benchar[0] = 0xFF & val;
        ch.benchar[1] = 0xFF & (val >> 8);
        outs[wp] = ch.bensht;
      }
      else
        out[wp] = asig[i];
    }
    p->wp = wp;

    return OK;
}

static int32_t send_send_k(CSOUND *csound, SOCKSEND *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);

    int32_t     buffersize = p->bsize;
    MYFLT   *ksig = p->asig;
    MYFLT   *out = (MYFLT *) p->aux.auxp;
    int16   *outs = (int16 *) p->aux.auxp;
    int32_t     ff = p->ff;


    if (p->wp == buffersize) {
      /* send the package when we have a full buffer */
      if (UNLIKELY(sendto(p->sock, (void*)out, buffersize  * p->bwidth, 0, to,
                          sizeof(p->server_addr)) < 0)) {
        return csound->PerfError(csound, p->h.insdshead, Str("sendto failed"));
      }
      p->wp = 0;
    }
    if (ff) { // Scale for 0dbfs and make LE
      int16 val = (int16)((32768.0* (*ksig))/csound->e0dbfs);
      union cheat {
        char  benchar[2];
        int16 bensht;
      } ch;
      ch.benchar[0] = 0xFF & val;
      ch.benchar[1] = 0xFF & (val >> 8);
      outs[p->wp] = ch.bensht;
    }
    else out[p->wp++] = *ksig;

    return OK;
}

static int32_t send_send_Str(CSOUND *csound, SOCKSENDT *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);

    int32_t     buffersize = p->bsize;
    char    *out = (char *) p->aux.auxp;
    char    *q = p->str->data;
    int32_t     len = p->str->size;

    if (UNLIKELY(len>=buffersize)) {
      csound->Warning(csound, Str("string truncated in socksend"));
      len = buffersize-1;
    }
    memcpy(out, q, len);
    memset(out+len, 0, buffersize-len);
    /* send the package with the string each time */
    if (UNLIKELY(sendto(p->sock, (void*)out, buffersize, 0, to,
                        sizeof(p->server_addr)) < 0)) {
      return csound->PerfError(csound, p->h.insdshead, Str("sendto failed"));
    }
    return OK;
}



/* UDP version 2 channels */
static int32_t init_sendS(CSOUND *csound, SOCKSENDS *p)
{
    int32_t     bsize;
    int32_t     bwidth = sizeof(MYFLT);
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif

    p->ff = (int32_t)(*p->format);
    p->bsize = bsize = (int32_t) *p->buffersize;
    /* if (UNLIKELY((sizeof(MYFLT) * bsize) > MTU)) { */
    /*   return csound->InitError(csound,
         Str("The buffersize must be <= %d samples " */
    /*                                        "to fit in a udp-packet."), */
    /*                            (int32_t) (MTU / sizeof(MYFLT))); */
    /* } */
    p->wp = 0;

    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data,
              &p->server_addr.sin_addr);    /* the server IP address */
#endif
    p->server_addr.sin_port = htons((int32_t) *p->port);    /* the port */

    if (p->ff) bwidth = sizeof(int16);
    /* create a buffer to write the interleaved audio to */
    if (p->aux.auxp == NULL || (uint32_t) (bsize * bwidth) > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, (bsize * bwidth), &p->aux);
    else {
      memset(p->aux.auxp, 0, bwidth * bsize);
    }
    p->bwidth = bwidth;
    return OK;
}

static int32_t send_sendS(CSOUND *csound, SOCKSENDS *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);
    MYFLT   *asigl = p->asigl;
    MYFLT   *asigr = p->asigr;
    MYFLT   *out = (MYFLT *) p->aux.auxp;
    int16   *outs = (int16 *) p->aux.auxp;
    int32_t     wp;
    int32_t     buffersize = p->bsize;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t     ff = p->ff;

    if (UNLIKELY(early)) nsmps -= early;
    /* store the samples of the channels interleaved in the packet */
    /* (left, right) */
    for (i = offset, wp = p->wp; i < nsmps; i++, wp += 2) {
      if (wp == buffersize) {
        /* send the package when we have a full buffer */
        if (UNLIKELY(sendto(p->sock, (void*)out, buffersize * p->bwidth, 0, to,
                            sizeof(p->server_addr)) < 0)) {
          return csound->PerfError(csound, p->h.insdshead, Str("sendto failed"));
        }
        wp = 0;
      }
      if (ff) { // Scale for 0dbfs and make LE
        int16 val = 0x8000*(asigl[i]/csound->e0dbfs);
        union {
          char  benchar[2];
          int16 bensht;
        } ch;

        ch.benchar[0] = 0xFF & val;
        ch.benchar[1] = 0xFF & (val >> 8);
        outs[wp] = ch.bensht;
        val = 0x8000*(asigl[i+1]/csound->e0dbfs);
        ch.benchar[0] = 0xFF & val;
        ch.benchar[1] = 0xFF & (val >> 8);
        outs[wp + 1] = ch.bensht;
      }
      else {
        out[wp] = asigl[i];
        out[wp + 1] = asigr[i];
      }
    }
    p->wp = wp;

    return OK;
}

/* TCP version */
static int32_t init_ssend(CSOUND *csound, SOCKSEND *p)
{
#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif

    /* create a STREAM (TCP) socket in the INET (IP) protocol */
    p->sock = socket(PF_INET, SOCK_STREAM, 0);

    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }

    /* create server address: where we want to connect to */

    /* clear it out */
    memset(&(p->server_addr), 0, sizeof(p->server_addr));

    /* it is an INET address */
    p->server_addr.sin_family = AF_INET;

    /* the server IP address, in network byte order */
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data, &(p->server_addr.sin_addr));
#endif

    /* the port we are going to listen on, in network byte order */
    p->server_addr.sin_port = htons((int32_t) *p->port);

 again:
    if (UNLIKELY(connect(p->sock, (struct sockaddr *) &p->server_addr,
                         sizeof(p->server_addr)) < 0)) {
#ifdef ECONNREFUSED
      if (errno == ECONNREFUSED)
        goto again;
#endif
      return csound->InitError(csound, Str("connect failed (%d)"), errno);
    }

    return OK;
}

static int32_t send_ssend(CSOUND *csound, SOCKSEND *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t n = sizeof(MYFLT) * (CS_KSMPS-offset-early);

    if (UNLIKELY(n != write(p->sock, &p->asig[offset], n))) {
      csound->Message(csound, Str("Expected %d got %d\n"),
                      (int32_t) (sizeof(MYFLT) * CS_KSMPS), n);
      return csound->PerfError(csound, p->h.insdshead,
                               Str("write to socket failed"));
    }

    return OK;
}


typedef struct {
  OPDS h;
  MYFLT *kwhen;
  STRINGDAT *ipaddress;
  MYFLT *port;        /* UDP port */
  STRINGDAT *dest;
  STRINGDAT *type;
  MYFLT *arg[32];     /* only 26 can be used, but add a few more for safety */
  AUXCH   aux;
  AUXCH   types;
  int32_t     sock, iargs;
  MYFLT last;
  struct sockaddr_in server_addr;
} OSCSEND2;


static int32_t osc_send2_init(CSOUND *csound, OSCSEND2 *p)
{
    uint32_t     bsize;

    if (UNLIKELY(p->INOCOUNT > 4 && p->INOCOUNT < (uint32_t) p->type->size + 4))
       return csound->InitError(csound,
                             Str("insufficient number of arguments for "
                                 "OSC message types\n"));

#if defined(WIN32) && !defined(__CYGWIN__)
    WSADATA wsaData = {0};
    int32_t err;
    if (UNLIKELY((err=WSAStartup(MAKEWORD(2,2), &wsaData))!= 0))
      return csound->InitError(csound, Str("Winsock2 failed to start: %d"), err);
#endif
    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
#if defined(WIN32) && !defined(__CYGWIN__)
    p->server_addr.sin_addr.S_un.S_addr =
      inet_addr((const char *) p->ipaddress->data);
#else
    inet_aton((const char *) p->ipaddress->data,
              &p->server_addr.sin_addr);    /* the server IP address */
#endif
    p->server_addr.sin_port = htons((int32_t) *p->port);    /* the port */

    if(p->INCOUNT > 4) {
              if (p->types.auxp == NULL || strlen(p->type->data) > p->types.size)
      /* allocate space for the types buffer */
      csound->AuxAlloc(csound, strlen(p->type->data), &p->types);
    memcpy(p->types.auxp, p->type->data, strlen(p->type->data));

    // todo: parse type to allocate memory
    int32_t i, iarg = 0;
    STRINGDAT *s;
    ARRAYDAT *ar;
    FUNC *ft;
    int32_t j;
    bsize = 0;
    for(i=0; i < p->type->size-1; i++) {
      switch(p->type->data[i]){
      case 't':
        if (UNLIKELY(p->INOCOUNT < (uint32_t) p->type->size + 5))
          return csound->InitError(csound, "extra argument needed for type t\n");
        bsize += 8;
        iarg+=2;
        break;
      case 'f':
      case 'i':
      case 'c':
      case 'm':
        bsize += 4;
        iarg++;
        break;
      case 's':
        if (UNLIKELY(!IS_STR_ARG(p->arg[i])))
          return csound->InitError(csound, Str("expecting a string argument\n"));
        s = (STRINGDAT *)p->arg[i];
        bsize += strlen(s->data) + 64;
        iarg++;
        break;
      case 'l':
      case 'h': /* OSC-accepted type name for 64bit int32_t */
        p->type->data[i] = 'h';
        /* fall through */
      case 'd':
        bsize += 8;
        iarg++;
        break;
      case 'b': case 'T': case 'F': case 'I': case 'N':
        iarg++;
        break;
      case 'a':
        bsize += (sizeof(MYFLT)*CS_KSMPS);
        iarg++;
        break;
      case 'G':
        ft = csound->FTnp2Find(csound, p->arg[i]);
        bsize += (sizeof(MYFLT)*ft->flen);
        iarg++;
        break;
      case 'A':
      case 'D':
        ar = (ARRAYDAT *) p->arg[i];
        for(j=0; j < ar->dimensions; j++)
          bsize += (sizeof(MYFLT)*ar->sizes[j]);
        bsize += 12;
        iarg++;
        break;
      default:
        return csound->InitError(csound, Str("%c: data type not supported\n"),
                                 p->type->data[i]);
      }
    }

    bsize += (strlen(p->dest->data) + strlen(p->type->data) + 11);
    bsize *= 2;
    if (p->aux.auxp == NULL || bsize > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, bsize, &p->aux);
    else {
      memset(p->aux.auxp, 0, bsize);
    }
    p->iargs = iarg;
    } else {
      bsize = strlen(p->dest->data)+1;
      bsize = ceil(bsize/4.)*4;
      bsize += 8;
    if (p->aux.auxp == NULL || bsize > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, bsize, &p->aux);
    else {
      memset(p->aux.auxp, 0, bsize);
    }
    }

    p->last = FL(0.0);
    return OK;
}

static inline char le_test(){
    union _le {
      char c[2];
      short s;
    } le = {{0x0001}};
    return le.c[0];
}

static inline char *byteswap(char *p, int32_t N){
    if(le_test()) {
      char tmp;
      int32_t j ;
      for(j = 0; j < N/2; j++) {
        tmp = p[j];
        p[j] = p[N - j - 1];
        p[N - j - 1] = tmp;
      }
    }
    return p;
}

static inline int32_t aux_realloc(CSOUND *csound, size_t size, AUXCH *aux) {
    char *p = aux->auxp;
    aux->auxp = csound->ReAlloc(csound, p, size);
    aux->size = size;
    aux->endp = (char*)aux->auxp + size;
    return size;
}

static int32_t osc_send2(CSOUND *csound, OSCSEND2 *p)
{
    if(*p->kwhen != p->last) {
      const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);

      int32_t buffersize = 0, size, i, bsize = p->aux.size;
      char *out = (char *) p->aux.auxp;

      memset(out,0,bsize);
      /* package destination in 4-byte zero-padded block */
      size = strlen(p->dest->data)+1;
      memcpy(out,p->dest->data,size);
      size = ceil(size/4.)*4;
      buffersize += size;
      if(p->INCOUNT > 4) {
      /* package type in a 4-byte zero-padded block;
         add a comma to the beginning of the type string.
      */
      out[buffersize] = ',';
      size = strlen(p->type->data)+1;
      /* check for b type before copying */
      for(i = 0; i < p->iargs; i++) {
        if(p->type->data[i] == 'b') {
          if(*p->arg[i] == FL(0.0)) ((char *)p->types.auxp)[i] = 'F';
          else  ((char *)p->types.auxp)[i] = 'T';
        }
        else if(p->type->data[i] == 'D' ||
                p->type->data[i] == 'A' ||
                p->type->data[i] == 'G' ||
                p->type->data[i] == 'a')
          ((char *)p->types.auxp)[i] = 'b';
      }
      memcpy(out+buffersize+1,p->types.auxp,size-1);
      size = ceil((size+1)/4.)*4;
      buffersize += size;
      /* add data to message */
      float fdata;
      double ddata;
      MYFLT mdata;
      int32_t data;
      int64_t ldata;
      uint64_t udata;
      STRINGDAT *s;
      ARRAYDAT *ar;
      FUNC *ft;
      int32_t j;
      for(i = 0; i < p->iargs; i++) {
        switch(p->type->data[i]){
        case 'f':
          /* realloc if necessary */
          if(buffersize + 4 > bsize) {
            aux_realloc(csound, buffersize + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          fdata = (float) *p->arg[i];
          byteswap((char *) &fdata, 4);
          memcpy(out+buffersize,&fdata, 4);
          buffersize += 4;
          break;
        case 'd':
          /* realloc if necessary */
          if(buffersize + 8 > bsize) {
            aux_realloc(csound, buffersize + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          ddata = *p->arg[i];
          byteswap((char *) &ddata, 8);
          memcpy(out+buffersize,&ddata, 8);
          buffersize += 8;
          break;
        case 't':
          /* realloc if necessary */
          if(buffersize + 4 > bsize) {
            aux_realloc(csound, buffersize + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          udata = (uint64_t) MYFLT2LRND(*p->arg[i++]);
          udata <<= 4;
          udata |= (uint64_t) MYFLT2LRND(*p->arg[i++]);
          byteswap((char *) &udata, 8);
          memcpy(out+buffersize,&udata, 8);
          buffersize += 8;
          break;
        case 'i':
        case 'm':
        case 'c':
          /* realloc if necessary */
          if(buffersize + 4 > bsize) {
            aux_realloc(csound, buffersize + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          data = MYFLT2LRND(*p->arg[i]);
          byteswap((char *) &data, 4);
          memcpy(out+buffersize,&data, 4);
          buffersize += 4;
          break;
        case 'h':
          /* realloc if necessary */
          if(buffersize + 8 > bsize) {
            aux_realloc(csound, buffersize + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          ldata = (int64_t) (*p->arg[i]+0.5);
          byteswap((char *) &ldata, 8);
          memcpy(out+buffersize,&ldata, 8);
          buffersize += 8;
          break;
        case 's':
          s = (STRINGDAT *)p->arg[i];
          size = strlen(s->data)+1;
          size = ceil(size/4.)*4;
          /* realloc if necessary */
          if(buffersize + size > bsize) {
            aux_realloc(csound, buffersize + size + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          memcpy(out+buffersize, s->data, s->size);
          buffersize += size;
          break;
        case 'G':
          ft = csound->FTnp2Find(csound, p->arg[i]);
          size = (int32_t)(sizeof(MYFLT)*ft->flen);
          if(buffersize + size + 4 > bsize) {
            aux_realloc(csound, buffersize + size + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          data = size;
          byteswap((char *)&data,4);
          memcpy(out+buffersize,&data,4);
          buffersize += 4;
          memcpy(out+buffersize,ft->ftable,size);
          buffersize += size;
          break;
        case 'A':
          ar = (ARRAYDAT *) p->arg[i];
          size = 0;
          for(j=0; j < ar->dimensions; j++) {
            size += (int32_t)ar->sizes[j]*sizeof(MYFLT);
          }
          if(buffersize + size + 12 > bsize) {
            aux_realloc(csound, buffersize + size + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          data = size + 8;
          byteswap((char *)&data,4);
          memcpy(out+buffersize,&data,4);
          buffersize += 4;
          memcpy(out+buffersize,&(ar->dimensions),4);
          buffersize += 4;
          memcpy(out+buffersize,&ar->sizes[0],ar->dimensions*4);
          buffersize += ar->dimensions*4;
          memcpy(out+buffersize,ar->data,size);
          buffersize += size;
          break;
        case 'D':
          ar = (ARRAYDAT *) p->arg[i];
          size = 0;
          for(j=0; j < ar->dimensions; j++) {
            size += (int32_t)ar->sizes[j]*sizeof(MYFLT);
          }
          if(buffersize + size + 12 > bsize) {
            aux_realloc(csound, buffersize + size + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          data = size;
          byteswap((char *)&data,4);
          memcpy(out+buffersize,&data,4);
          buffersize += 4;
          memcpy(out+buffersize,ar->data,size);
          buffersize += size;
          break;
        case 'a':
          size = (int32_t) (CS_KSMPS+1)*sizeof(MYFLT);
          if(buffersize + size + 4 > bsize) {
            aux_realloc(csound, buffersize + size + 128, &p->aux);
            out = (char *) p->aux.auxp;
            bsize = p->aux.size;
          }
          data = size;
          byteswap((char *)&data,4);
          memcpy(out+buffersize,&data,4);
          buffersize += 4;
          mdata = CS_KSMPS;
          memcpy(out+buffersize,&mdata,sizeof(MYFLT));
          memcpy(out+buffersize+sizeof(MYFLT),p->arg[i],CS_KSMPS*sizeof(MYFLT));
          buffersize += size;
          break;
        case 'T':
        case 'F':
        case 'I':
        case 'N':
        default:
          break;
        }
      }
      } else {
        out[buffersize] = ',';
        memset(out+buffersize+1, 0, 3);
        buffersize += 4;
      }
      if (UNLIKELY(sendto(p->sock, (void*)out, buffersize, 0, to,
                          sizeof(p->server_addr)) < 0)) {
        return csound->PerfError(csound, p->h.insdshead, Str("OSCsend2 failed"));
      }
      p->last = *p->kwhen;
    }
    return OK;
}





#define S(x)    sizeof(x)

static OENTRY socksend_localops[] =
  {
   { "socksend.a", S(SOCKSEND), 0, 3, "", "aSiio", (SUBR) init_send,
     (SUBR) send_send },
   { "socksend.k", S(SOCKSEND), 0, 3, "", "kSiio", (SUBR) init_send,
     (SUBR) send_send_k, NULL },
   { "socksend.S", S(SOCKSENDT), 0, 3, "", "SSiio", (SUBR) init_send,
     (SUBR) send_send_Str, NULL },
   { "socksends", S(SOCKSENDS), 0, 3, "", "aaSiio", (SUBR) init_sendS,
     (SUBR) send_sendS },
   { "stsend", S(SOCKSEND), 0, 3, "", "aSi", (SUBR) init_ssend,
     (SUBR) send_ssend },
   { "OSCsend", S(OSCSEND2), 0, 3, "", "kSk*", (SUBR)osc_send2_init,
     (SUBR)osc_send2 }
};

LINKAGE_BUILTIN(socksend_localops)
