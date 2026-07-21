
/*
    OSC.c:

    Copyright (C) 2005 by John ffitch

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

/* Haiku 'int32' etc definitions in net headers conflict with sysdep.h */
#define __HAIKU_CONFLICT

#include "csdl.h"
#include "arrays.h"
#include "osc_blob.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif
#include <lo/lo.h>
#include <ctype.h>
#ifndef WIN32
  #include <sys/types.h>
  #include <sys/socket.h>
#endif
//#define OSC_DEBUG

/* structure for real time event */

/* typedef struct rtEvt_s { */
/*     struct rtEvt_s  *nxt; */
/*     EVTBLK  e; */
/* } rtEvt_t; */

#define ARG_CNT (64)

typedef struct {
    OPDS h;             /* default header */
    MYFLT *kwhen;
    STRINGDAT *host;
    MYFLT *port;        /* UDP port */
    STRINGDAT *dest;
    STRINGDAT *type;
    MYFLT *arg[ARG_CNT]; /* only 26 can be used, but add a few more for safety */
    lo_address addr;
    MYFLT last;
    char  *lhost;
    int32_t   cnt;
    int32_t   multicast;
    CSOUND *csound;
    void *thread;
    MYFLT lasta;
} OSCSEND;


typedef struct osc_pat {
    struct osc_pat *next;
    union {
      MYFLT number;
      STRINGDAT string;
      void     *blob;
    } args[ARG_CNT-1];
} OSC_PAT;

typedef struct {
    lo_server_thread thread;
    CSOUND  *csound;
    void    *mutex_;
    void    *oplst;             /* list of opcodes listening on this port */
} OSC_PORT;

/* structure for global variables */

typedef struct {
    CSOUND  *csound;
    /* for OSCinit/OSClisten */
    int32_t   nPorts;
    OSC_PORT  **ports;
    int32_t   osccounter;
    void      *mutex_;
} OSC_GLOBALS;

/* opcode for starting the OSC listener (called once from orchestra header) */
typedef struct {
    OPDS    h;                  /* default header */
    MYFLT   *ihandle;
    MYFLT   *port;              /* Port number on which to listen */
} OSCINIT;

typedef struct {
    OPDS    h;                  /* default header */
    MYFLT   *ihandle;
    STRINGDAT *group;
    MYFLT   *port;              /* Port number on which to listen */
} OSCINITM;

typedef struct osclcommon {
    lo_method method;
    char    *saved_path;
    char    saved_types[ARG_CNT];    /* copy of type list */
    OSC_PAT *patterns;          /* FIFO list of pending messages */
    OSC_PAT *freePatterns;      /* free message stack */
    struct osclcomon *nxt;       /* pointer to next opcode on the same port */
} OSCLCOMMON;

typedef struct {
    OPDS        h;                  /* default header */
    MYFLT       *kans;
    MYFLT       *ihandle;
    STRINGDAT   *dest;
    STRINGDAT   *type;
    MYFLT       *args[ARG_CNT];
    OSC_PORT    *port;
    OSCLCOMMON  c;
    int32_t     malformedBlobWarning;
} OSCLISTEN;

typedef struct {
    OPDS      h;                  /* default header */
    MYFLT     *kans;
    ARRAYDAT  *args;
    MYFLT     *ihandle;
    STRINGDAT *dest;
    STRINGDAT *type;
    OSC_PORT  *port;
    OSCLCOMMON c;
} OSCLISTENA;

static int32_t oscsend_deinit(CSOUND *csound, OSCSEND *p)
{
    lo_address a = (lo_address)p->addr;
    if (LIKELY(a != NULL))
      lo_address_free(a);
    p->addr = NULL;
    csound->Free(csound, p->lhost);
    return OK;
}

static int32_t osc_send_set(CSOUND *csound, OSCSEND *p)
{
    char port[8];
    char *pp = port;
    char *hh;
    //uint32_t i;

    /* with too many args, XINCODE may not work correctly */
    if (UNLIKELY(p->INOCOUNT > ARG_CNT-1))
      return csound->InitError(csound, "%s", Str("Too many arguments to OSCsend"));
/* a-rate arguments are not allowed */
/* for (i = 0; i < p->INOCOUNT-5; i++) { */
/*   if (strcmp("a", GetTypeForArg(p->arg[i])->varTypeName) == 0) { */
/*     return csound->InitError(csound,"%s", Str("No a-rate arguments allowed")); */
/*   } */
/* } */

    if (*p->port<0)
      pp = NULL;
    else
      snprintf(port, 8, "%d", (int32_t) MYFLT2LRND(*p->port));
    hh = (char*) p->host->data;
    if (UNLIKELY(*hh=='\0')) {
      hh = NULL;
      p->lhost = csound->Strdup(csound, "localhost");
    }
    else     p->lhost = csound->Strdup(csound, hh);
    if (hh && isdigit(*hh)) {
      int32_t n = atoi(hh);
      p->multicast = (n>=224 && n<=239);
    }
    else p->multicast = 0;
    //printf("multicast=%d\n", p->multicast);
    p->addr = lo_address_new(hh, pp);
    // MKG: Seems to have been dropped from liblo.
    // But TTL 1 should be the default for multicast.
    if (UNLIKELY(p->multicast)) lo_address_set_ttl(p->addr, 1);
    p->cnt = 0;
    p->last = 0;
    p->thread = NULL;
    return OK;

}

static int32_t osc_send(CSOUND *csound, OSCSEND *p)
{
    /* Types I allow at present:
       0) int32_t
       1) float
       2) string
       3) double
       4) char
       5) table as blob
    */
    char port[8];
    char *pp = port;
    char *hh;
    int32_t cmpr = 0;

    if(p->INOCOUNT > 4) {
      if(strcmp(GetTypeForArg(p->type)->varTypeName, "S"))
        return csound->InitError(csound,"%s",
                             Str("Message type is not given as a string\n"));
    }

    if (UNLIKELY(*p->port<0))
      pp = NULL;
    else
      snprintf(port, 8, "%d", (int32_t) MYFLT2LRND(*p->port));
    hh = (char*) p->host->data;
    if (UNLIKELY(*hh=='\0')) hh = NULL;
    /*
       can this be done at init time?
       It was note that this could be creating
       a latency penalty
       Yes; cached -- JPff
    */
    // 152269
    //if (!(hh==NULL && p->lhost == NULL) || strcmp(p->lhost, hh)!=0) {
    if (p->thread == NULL) {
      if (hh && p->lhost) cmpr = strcmp(p->lhost, hh);
      if (!(hh==NULL && p->lhost == NULL) || cmpr !=0) {
        if (p->addr != NULL)
          lo_address_free(p->addr);
        p->addr = lo_address_new(hh, pp);
        // MKG: This seems to have been dropped from liblo.
        // if (p->multicast) lo_address_set_ttl(p->addr, 2);
        if (UNLIKELY(p->multicast)) {
          // the code below assumes lo_address is a socket, but it's not so
          // it won't work and needs fixing.
#if 0
                    u_char ttl = 2;
#if defined(LINUX)
          if (UNLIKELY(setsockopt((uintptr_t)p->addr, IPPROTO_IP,
                                  IP_MULTICAST_TTL, &ttl, sizeof(ttl))==-1)) {
            csound->Message(csound, "%s", Str("Failed to set multicast"));
          }
#elif defined(MSVC)
          setsockopt((SOCKET)p->addr, IPPROTO_IP, IP_MULTICAST_TTL,
                     &ttl, sizeof(ttl));
#else
          setsockopt((uintptr_t)p->addr, IPPROTO_IP, IP_MULTICAST_TTL,
                     &ttl, sizeof(ttl));
#endif
#else
          return csound->PerfError(csound, &(p->h), "multicast not supported\n");
#endif

        }
        csound->Free(csound, p->lhost);
        if (hh) p->lhost = csound->Strdup(csound, hh); else p->lhost = NULL;
      }
    }
    if (p->cnt++ ==0 || *p->kwhen!=p->last) {
      int32_t i=0;
      lo_message msg = lo_message_new();
      char *type = p->INOCOUNT > 4  ? (char*)p->type->data : "";
      MYFLT **arg = p->arg;
      p->last = *p->kwhen;
      for (i=0; type[i]!='\0'; i++) {
        /* Need to add type checks */
        switch (type[i]) {
        case 'i':
          lo_message_add_int32(msg, (int32_t) MYFLT2LRND(*arg[i]));
          break;
        case 'l':
        case 'h':
          lo_message_add_int64(msg, (int64_t) MYFLT2LRND(*arg[i]));
          break;
        case 'c':
          lo_message_add_char(msg, (char) (*arg[i] + FL(0.5)));
          break;
        case 'm':
          {
            union a {
              int32_t  x;
              uint8_t  m[4];
            } mm;
            mm.x = *arg[i]+FL(0.5);
            lo_message_add_midi(msg, mm.m);
            break;
          }
        case 'f':
          lo_message_add_float(msg, (float)(*arg[i]));
          break;
        case 'd':
          lo_message_add_double(msg, (double)(*arg[i]));
          break;
        case 's':
          lo_message_add_string(msg, ((STRINGDAT *)arg[i])->data);
          break;
        case 'b':               /* Boolean */
          if (*arg[i]==FL(0.0)) lo_message_add_true(msg);
          else lo_message_add_false(msg);
          break;
        case 't':               /* timestamp */
          {
            lo_timetag tt;
            tt.sec = (uint32_t)(*arg[i]+FL(0.5));
            i++;
            if (UNLIKELY(type[i]!='t'))
              return csound->PerfError(csound, &(p->h),
                                       "%s", Str("Time stamp is two values"));
            tt.frac = (uint32_t)(*arg[i]+FL(0.5));
            lo_message_add_timetag(msg, tt);
            break;
          }
          //#ifdef SOMEFINEDAY
        case 'G':               /* fGen Table/blob */
          {
            lo_blob myblob;
            int32_t     len, olen;
            FUNC    *ftp;
            void *data;
            /* make sure fn exists */
            if (LIKELY((ftp=csound->FTFind(csound,arg[i]))!=NULL)) {
              len = ftp->flen;        /* and set it up */
              data = csound->Malloc(csound,
                                    olen=/*sizeof(FUNC)-sizeof(MYFLT*)+*/
                                         sizeof(MYFLT)*len);
              // memcpy(data, ftp, sizeof(FUNC)-sizeof(MYFLT*));
              memcpy(data/*+sizeof(FUNC)-sizeof(MYFLT*)*/,
                     ftp->ftable, sizeof(MYFLT)*len);
            }
            else {
              return csound->PerfError(csound, &(p->h),
                                       Str("ftable %.2f does not exist"), *arg[i]);
            }
            myblob = lo_blob_new(olen, data);
            lo_message_add_blob(msg, myblob);
            csound->Free(csound, data);
            lo_blob_free(myblob);
            break;
          }
          //#endif
        case 'a':               /* Audio as blob */
          {
            lo_blob myblob;
            MYFLT *data = csound->Malloc(csound, sizeof(MYFLT)*(CS_KSMPS+1));
            data[0] = CS_KSMPS;
            memcpy(&data[1], arg[i], sizeof(MYFLT)*CS_KSMPS);
            myblob = lo_blob_new(sizeof(MYFLT)*(CS_KSMPS+1), data);
            lo_message_add_blob(msg, myblob);
            csound->Free(csound, data);
            lo_blob_free(myblob);
            break;
          }
        case 'A':               /* Array/blob */
          {
            lo_blob myblob;
            int32_t     len = 1;
            ARRAYDAT *ss;
            /* make sure fn exists */
            if (LIKELY((ss = (ARRAYDAT*)arg[i]) !=NULL &&
                       ss->data != NULL)) {
              int32_t j, d;
              for (j=0,d=ss->dimensions; d>0; j++, d--)
                len *= ss->sizes[j];
              len *= sizeof(MYFLT);
            }
            else {
              return csound->PerfError(csound, &(p->h),
                                       Str("argument %d is not an array"), i);
            }
            // two parts needed
            {
              void *dd =
                csound->Malloc(csound, len+sizeof(int32_t)*(1+ss->dimensions));
              memcpy(dd, &ss->dimensions, sizeof(int32_t));
              memcpy((char*)dd+sizeof(int32_t), ss->sizes,
 sizeof(int32_t)*ss->dimensions);
              memcpy((char*)dd+sizeof(int32_t)*(1+ss->dimensions), ss->data, len);
      /* printf("dd length = %d dimensions = %d, %d %d %.8x %.8x %.8x %.8x\n", */
      /*        len+sizeof(int32_t)*(1+ss->dimensions), ss->dimensions, */
      /*        ((int32_t*)dd)[0], ((int32_t*)dd)[1], ((int32_t*)dd)[2],*/
      /* ((int32_t*)dd)[3], */
      /*        ((int32_t*)dd)[4], ((int32_t*)dd)[5]); */
              myblob = lo_blob_new(len, dd);
              csound->Free(csound, dd);
            }
            lo_message_add_blob(msg, myblob);
            lo_blob_free(myblob);
            break;
          }
        case 'S': csound->Warning(csound, "S unimplemented"); break;
          //#endif
        default:
          csound->Warning(csound, Str("Unknown OSC type %c\n"), type[1]);
        }
      }
      lo_send_message(p->addr, (char*)p->dest->data, msg);
      lo_message_free(msg);
    }
    return OK;
}

static void OSC_stop_port(CSOUND *csound, OSC_PORT *port)
{
    if (port == NULL)
      return;
    if (port->thread != NULL) {
      lo_server_thread_stop(port->thread);
      lo_server_thread_free(port->thread);
      port->thread = NULL;
    }
    if (port->mutex_ != NULL) {
      csound->DestroyMutex(port->mutex_);
      port->mutex_ = NULL;
    }
}

/* RESET routine for cleaning up */
static int32_t OSC_reset(CSOUND *csound, OSC_GLOBALS *p)
{
    int32_t i;
    for (i = 0; i < p->nPorts; i++) {
      OSC_PORT *port = p->ports[i];
      if (port != NULL) {
        OSC_stop_port(csound, port);
        csound->Free(csound, port);
      }
    }
    csound->Free(csound, p->ports);
    if (p->mutex_ != NULL)
      csound->DestroyMutex(p->mutex_);
    csound->DestroyGlobalVariable(csound, "_OSC_globals");
    return OK;
}

uintptr_t OSCthread(void *pp) {
  OSCSEND *p = (OSCSEND *) pp;
  osc_send(p->csound, p);
  return 0;
}

/* get pointer to globals structure, allocating it on the first call */

static CS_NOINLINE OSC_GLOBALS *alloc_globals(CSOUND *csound)
{
    OSC_GLOBALS *pp;

    pp = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    if (pp != NULL)
      return pp;
    if (UNLIKELY(csound->CreateGlobalVariable(csound, "_OSC_globals",
                                              sizeof(OSC_GLOBALS)) != 0)){
      csound->ErrorMsg(csound, "%s", Str("OSC: failed to allocate globals"));
      return NULL;
    }
    pp = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    pp->csound = csound;
    pp->mutex_ = csound->Create_Mutex(0);
    if (UNLIKELY(pp->mutex_ == NULL)) {
      csound->DestroyGlobalVariable(csound, "_OSC_globals");
      csound->ErrorMsg(csound, "%s", Str("OSC: failed to create mutex"));
      return NULL;
    }
    if (UNLIKELY(csound->RegisterResetCallback(
                   csound, (void*) pp,
                   (int32_t (*)(CSOUND *, void *)) OSC_reset) != OK)) {
      csound->DestroyMutex(pp->mutex_);
      csound->DestroyGlobalVariable(csound, "_OSC_globals");
      csound->ErrorMsg(csound, "%s",
                       Str("OSC: failed to register reset callback"));
      return NULL;
    }
    return pp;
}



 /* ------------------------------------------------------------------------ */

static CS_NOINLINE OSC_PAT *alloc_pattern(CSOUND *csound)
{
    OSC_PAT *p;
    size_t  nbytes;

    /* number of bytes to allocate */
    nbytes = sizeof(OSC_PAT);
    /* allocate and initialise structure */
    p = (OSC_PAT*) csound->Calloc(csound, nbytes);

    return p;
}

static inline OSC_PAT *get_pattern(CSOUND *csound,OSCLCOMMON *pp)
{
    OSC_PAT *p;

    if (pp->freePatterns != NULL) {
      p = pp->freePatterns;
      pp->freePatterns = p->next;
      return p;
    }
    return alloc_pattern(csound);
}

typedef struct {
      OPDS h;             /* default header */
      MYFLT *ans;
} OSCcount;

static int32_t OSCcounter(CSOUND *csound, OSCcount *p)
{
    OSC_GLOBALS *g = alloc_globals(csound);
    if (UNLIKELY(g == NULL))
      return NOTOK;
    *p->ans = (MYFLT)g->osccounter;
    return OK;
}


static int32_t OSC_handler(const char *path, const char *types,
                       lo_arg **argv, int32_t argc, lo_message data, void *p)
{
    IGN(argc);  IGN(data);
    OSC_PORT  *pp = (OSC_PORT*) p;
    OSCLCOMMON *o;
    CSOUND    *csound = (CSOUND *) pp->csound;
    int32_t       retval = 1;

    pp->csound->LockMutex(pp->mutex_);
    o = (OSCLCOMMON*) pp->oplst;
    //printf("opst=%p\n", o);
    while (o != NULL) {
      //printf("Looking at %s/%s against %s/%s\n",
      //       o->saved_path, path,o->saved_types, types);
      if (strcmp(o->saved_path, path) == 0 &&
          strcmp(o->saved_types, types) == 0) {
        /* Message is for this guy */
        int32_t     i;
        OSC_PAT *m;
        OSC_GLOBALS *g = alloc_globals(csound);
        pp->csound->LockMutex(g->mutex_);
        g->osccounter++;
        pp->csound->UnlockMutex(g->mutex_);
        m = get_pattern(csound, o);
        if (m != NULL) {
          /* queue message for being read by OSClisten opcode */
          m->next = NULL;
          if (o->patterns == NULL)
            o->patterns = m;
          else {
            OSC_PAT *mm;
            for (mm = o->patterns; mm->next != NULL; mm = mm->next)
              ;
            mm->next = m;
          }
          /* copy argument list */
          for (i = 0; o->saved_types[i] != '\0'; i++) {
            switch (types[i]) {
            default:              /* Should not happen */
            case 'i':
              m->args[i].number = (MYFLT) argv[i]->i; break;
            case 'h':
              m->args[i].number = (MYFLT) argv[i]->i64; break;
            case 'c':
               m->args[i].number= (MYFLT) argv[i]->c; break;
            case 'f':
               m->args[i].number = (MYFLT) argv[i]->f; break;
            case 'd':
               m->args[i].number= (MYFLT) argv[i]->d; break;
            case 's':
              { // ***NO CHECK THAT m->args[i] IS A STRING
                char  *src = (char*) &(argv[i]->s), *dst = m->args[i].string.data;
                if (m->args[i].string.size <= strlen(src)) {
                  if (dst != NULL) csound->Free(csound, dst);
                  dst = csound->Strdup(csound, src);
                  // who sets m->args[i].string.size ??
                  m->args[i].string.data = dst;
                  m->args[i].string.size = strlen(dst)+1;
                }
                else strcpy(dst, src);
                break;
              }
            case 'b':
              {
                int32_t len =
                  lo_blobsize((lo_blob)argv[i]);
                m->args[i].blob =
                  csound->Malloc(csound,len);
                memcpy(m->args[i].blob, argv[i], len);
#ifdef OSC_DEBUG
                {
                  lo_blob *bb = (lo_blob*)m->args[i].blob;
                  int32_t size = lo_blob_datasize(bb);
                  MYFLT *data = lo_blob_dataptr(bb);
                  int32_t   *idata = (int32_t*)data;
                  printf("size=%d data=%.8x %.8x ...\n",size, idata[0], idata[1]);
                }
#endif
              }
            }
          }
          retval = 0;
        }
        break;
      }
      o = (OSCLCOMMON*) o->nxt;
    }

    pp->csound->UnlockMutex(pp->mutex_);
    return retval;
}

static void OSC_error(int32_t num, const char *msg, const char *path)
{
    fprintf(stderr, "OSC server error %d in path %s: %s\n", num, path, msg);
}

static int32_t OSC_deinit(CSOUND *csound, OSCINIT *p)
{
    int32_t n = (int32_t)*p->ihandle;
    OSC_GLOBALS *pp =
      (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    OSC_PORT *port;
    if (UNLIKELY(pp == NULL || n < 0 || n >= pp->nPorts))
      return NOTOK;
    port = pp->ports[n];
    if (UNLIKELY(port == NULL))
      return NOTOK;
    OSC_stop_port(csound, port);
    csound->Message(csound, "%s", Str("OSC deinitialised\n"));
    return OK;
}

static int32_t OSC_start_port(CSOUND *csound, OSC_GLOBALS *globals,
                              lo_server_thread thread, const char *portName,
                              MYFLT *handle)
{
    int32_t n = globals->nPorts;
    OSC_PORT *port = (OSC_PORT*) csound->Calloc(csound, sizeof(OSC_PORT));
    OSC_PORT **ports;

    if (UNLIKELY(port == NULL)) {
      lo_server_thread_free(thread);
      csound->ErrorMsg(csound, "%s",
                       "OSC: Failed to allocate memory for ports\n");
      return NOTOK;
    }
    port->csound = csound;
    port->mutex_ = csound->Create_Mutex(0);
    if (UNLIKELY(port->mutex_ == NULL)) {
      lo_server_thread_free(thread);
      csound->Free(csound, port);
      csound->ErrorMsg(csound, "%s",
                       "OSC: Failed to create listener mutex\n");
      return NOTOK;
    }
    port->thread = thread;
    if (UNLIKELY(lo_server_thread_start(thread) < 0)) {
      lo_server_thread_free(thread);
      port->thread = NULL;
      csound->DestroyMutex(port->mutex_);
      csound->Free(csound, port);
      return csound->InitError(
        csound, Str("cannot start OSC listener on port %s\n"), portName);
    }
    ports = (OSC_PORT**) csound->ReAlloc(
      csound, globals->ports, sizeof(OSC_PORT*) * (n + 1));
    if (UNLIKELY(ports == NULL)) {
      OSC_stop_port(csound, port);
      csound->Free(csound, port);
      csound->ErrorMsg(csound, "%s",
                       "OSC: Failed to allocate memory for ports\n");
      return NOTOK;
    }
    globals->ports = ports;
    globals->ports[n] = port;
    globals->nPorts = n + 1;
    *handle = (MYFLT) n;
    return n;
}

static int32_t osc_listener_init(CSOUND *csound, OSCINIT *p)
{
    OSC_GLOBALS *pp;
    lo_server_thread thread;
    char buff[32];
    int32_t n;

    /* allocate and initialise the globals structure */
    pp = alloc_globals(csound);
    if (UNLIKELY(pp == NULL))
      return NOTOK;
    snprintf(buff, 32, "%d", (int32_t) *(p->port));
    thread = lo_server_thread_new(buff, OSC_error);
    if (UNLIKELY(thread == NULL))
      return csound->InitError(csound,
                               Str("cannot start OSC listener on port %s\n"),
                               buff);
    n = OSC_start_port(csound, pp, thread, buff, p->ihandle);
    if (UNLIKELY(n < 0))
      return NOTOK;
    csound->Warning(csound, Str("OSC listener #%d started on port %s\n"), n, buff);
    return OK;
}

static int32_t osc_listener_initMulti(CSOUND *csound, OSCINITM *p)
{
    OSC_GLOBALS *pp;
    lo_server_thread thread;
    char buff[32];
    int32_t n;

    /* allocate and initialise the globals structure */
    pp = alloc_globals(csound);
    if (UNLIKELY(pp == NULL))
      return NOTOK;
    snprintf(buff, 32, "%d", (int32_t) *(p->port));
    thread = lo_server_thread_new_multicast(p->group->data, buff, OSC_error);
    if (UNLIKELY(thread == NULL))
      return csound->InitError(csound,
                               Str("cannot start OSC listener on port %s\n"),
                               buff);
    n = OSC_start_port(csound, pp, thread, buff, p->ihandle);
    if (UNLIKELY(n < 0))
      return NOTOK;
    csound->Warning(csound,
                    Str("OSC multicast listener #%d started on port %s\n"),
                    n, buff);
    return OK;
}

static int32_t OSC_listendeinit(CSOUND *csound, OSC_PORT *port, OSCLCOMMON *p)
{
    OSC_PAT *m;

    if (port != NULL && port->mutex_ != NULL) {
      csound->LockMutex(port->mutex_);
      if (port->oplst == (void*)p)
        port->oplst = p->nxt;
      else {
        OSCLCOMMON *o = (OSCLCOMMON*) port->oplst;
        while (o != NULL && o->nxt != (void*) p)
          o = (OSCLCOMMON*) o->nxt;
        if (o != NULL)
          o->nxt = p->nxt;
      }
      csound->UnlockMutex(port->mutex_);
      if (port->thread != NULL) {
#ifdef LIBLO29
        /* lo_server_thread_del_lo_method requires liblo 0.29 or newer. */
        lo_server_thread_del_lo_method(port->thread, p->method);
#else
        lo_server_thread_del_method(port->thread, p->saved_path, p->saved_types);
#endif
      }
    }
    csound->Free(csound, p->saved_path);
    p->saved_path = NULL;
    p->nxt = NULL;
    m = p->patterns;
    p->patterns = NULL;
    while (m != NULL) {
      OSC_PAT *mm = m->next;
      csound->Free(csound, m);
      m = mm;
    }
    m = p->freePatterns;
    p->freePatterns = NULL;
    while (m != NULL) {
      OSC_PAT *mm = m->next;
      csound->Free(csound, m);
      m = mm;
    }
    return OK;
}

static int32_t OSC_listdeinit(CSOUND *csound, OSCLISTEN *p)
{
    OSC_PORT *port = p->port;
    return OSC_listendeinit(csound, port, &p->c);
}

static int32_t OSC_listadeinit(CSOUND *csound, OSCLISTENA *p)
{
    OSC_PORT *port = p->port;
    return OSC_listendeinit(csound, port, &p->c);
}


static int32_t OSC_list_init(CSOUND *csound, OSCLISTEN *p)
{
    //void  *x;
    int32_t   i, n;

    p->malformedBlobWarning = 0;
    OSC_GLOBALS *pp =
      (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    if (UNLIKELY(pp == NULL))
      return csound->InitError(csound, "%s", Str("OSC not running"));
    /* find port */
    n = (int32_t) *(p->ihandle);
    if (UNLIKELY(n < 0 || n >= pp->nPorts))
      return csound->InitError(csound, "%s", Str("invalid handle"));
    p->port = pp->ports[n];
    if (UNLIKELY(p->port == NULL || p->port->thread == NULL ||
                 p->port->mutex_ == NULL))
      return csound->InitError(csound, "%s", Str("invalid handle"));
    p->c.saved_path = (char*) csound->Malloc(csound,
                                           strlen((char*) p->dest->data) + 1);
    strcpy(p->c.saved_path, (char*) p->dest->data);
    /* check for a valid argument list */
    n = GetInputArgCnt((OPDS *)p) - 3;
    if (UNLIKELY(n < 0 || n > ARG_CNT-4))
      return csound->InitError(csound, "%s", Str("invalid number of arguments"));
    if (UNLIKELY((int32_t) strlen((char*) p->type->data) != n))
      return csound->InitError(csound,
                               "%s", Str("-- argument list inconsistent with "
                                   "format string"));
    strcpy(p->c.saved_types, (char*) p->type->data);
    for (i = 0; i < n; i++) {
      switch (p->c.saved_types[i]) {
      case 'G':
      case 'A':
      case 'D':
      case 'a':
      case 'S':
        p->c.saved_types[i] = 'b';
        break;
      case 'c':
      case 'd':
      case 'f':
      case 'h':
      case 'i':
        if (!IS_KSIG_ARG(p->args[i]))
          return csound->InitError(csound, "%s", Str("argument list inconsistent "
                                               "with format string"));
        break;
      case 's':
        if (!IS_STR_ARG(p->args[i]))
          return csound->InitError(csound, "%s", Str("argument list inconsistent "
                                               "with format string"));
        break;
      default:
        return csound->InitError(csound, "%s", Str("invalid type"));
      }
    }
    csound->LockMutex(p->port->mutex_);
    p->c.nxt = p->port->oplst;
    p->port->oplst = (void*) &p->c;
    csound->UnlockMutex(p->port->mutex_);
    p->c.method = lo_server_thread_add_method(p->port->thread,
                                              p->c.saved_path, p->c.saved_types,
                                              OSC_handler, p->port);
    return OK;
}

#define OSC_BLOB_DROPPED (1)

static int32_t osc_malformed_blob(CSOUND *csound, OSCLISTEN *p, char type)
{
    if (!p->malformedBlobWarning) {
      csound->Warning(csound, Str("OSC: ignoring malformed '%c' blob\n"), type);
      p->malformedBlobWarning = 1;
    }
    return OSC_BLOB_DROPPED;
}

/* The array decoders copy raw MYFLTs, so the destination's element type
   must be a scalar MYFLT. A fresh array has no arrayMemberSize yet; size a
   probe variable from its element type, exactly as
   csound_array_ensure_capacity() would when allocating. */
static int32_t osc_array_element_is_myflt(CSOUND *csound,
                                          const ARRAYDAT *array, INSDS *ctx)
{
    CS_VARIABLE *var;
    int32_t isMyflt;

    if (array->data != NULL)
      return array->arrayMemberSize == (int32_t)sizeof(MYFLT);
    if (array->arrayType == NULL)
      return 0;
    var = array_element_create_variable(csound, array->arrayType, ctx);
    if (var == NULL)
      return 0;
    isMyflt = var->memBlockSize == (int32_t)sizeof(MYFLT);
    csound->Free(csound, var);
    return isMyflt;
}

static int32_t osc_decode_direct_array(CSOUND *csound, OSCLISTEN *p,
                                       ARRAYDAT *array, const void *payload,
                                       size_t payloadBytes)
{
    OSC_MYFLT_BLOB_VIEW view;
    size_t currentCount;
    size_t prefixCount = 1;
    int32_t newLastSize = 0;
    int32_t resize = 0;
    int32_t i;

    if (osc_blob_parse_myflts(payload, payloadBytes, &view) != OK ||
        array == NULL || array->dimensions <= 0 || array->sizes == NULL ||
        csound_array_has_managed_elements(array) ||
        !osc_array_element_is_myflt(csound, array, p->h.insdshead) ||
        csound_array_member_count(array, &currentCount) != OK) {
      return osc_malformed_blob(csound, p, 'D');
    }
    if (view.count > currentCount) {
      for (i = 0; i < array->dimensions - 1; i++) {
        if (array->sizes[i] <= 0 ||
            (size_t)array->sizes[i] > SIZE_MAX / prefixCount) {
          return osc_malformed_blob(csound, p, 'D');
        }
        prefixCount *= (size_t)array->sizes[i];
      }
      if (prefixCount == 0 || view.count % prefixCount != 0 ||
          view.count / prefixCount > INT32_MAX) {
        return osc_malformed_blob(csound, p, 'D');
      }
      newLastSize = (int32_t)(view.count / prefixCount);
      resize = 1;
    }
    if (view.count == 0) {
      return OK;
    }
    if (UNLIKELY(csound_array_prepare_write(
                   csound, array, p->h.insdshead) != OK ||
                 csound_array_ensure_capacity(
                   csound, array, view.count, p->h.insdshead) != OK ||
                 array->data == NULL)) {
      csound->ErrorMsg(csound, "%s",
                       Str("OSC: Failed to allocate memory for array\n"));
      return OK;
    }
    memcpy(array->data, view.data, view.count * sizeof(MYFLT));
    if (resize) {
      array->sizes[array->dimensions - 1] = newLastSize;
    }
    return OK;
}

static int32_t osc_decode_array(CSOUND *csound, OSCLISTEN *p,
                                ARRAYDAT *array, const void *payload,
                                size_t payloadBytes)
{
    OSC_ARRAY_BLOB_VIEW view;
    int32_t *newSizes;
    int32_t *oldSizes;
    size_t capacity;
    int32_t i;

    if (osc_blob_parse_array(payload, payloadBytes, &view) != OK ||
        array == NULL || array->arrayType == NULL ||
        csound_array_has_managed_elements(array) ||
        !osc_array_element_is_myflt(csound, array, p->h.insdshead) ||
        (array->data != NULL && array->allocated == 0 &&
         array->storage == NULL)) {
      return osc_malformed_blob(csound, p, 'A');
    }
    newSizes = (int32_t *)csound->Malloc(
      csound, (size_t)view.dimensions * sizeof(int32_t));
    if (UNLIKELY(newSizes == NULL)) {
      csound->ErrorMsg(csound, "%s",
                       Str("OSC: Failed to allocate memory for array\n"));
      return OK;
    }
    for (i = 0; i < view.dimensions; i++) {
      if (UNLIKELY(osc_blob_array_size(&view, i, &newSizes[i]) != OK)) {
        csound->Free(csound, newSizes);
        return osc_malformed_blob(csound, p, 'A');
      }
    }
    capacity = view.values.count > 0 ? view.values.count : 1;
    if (UNLIKELY(csound_array_prepare_write(
                   csound, array, p->h.insdshead) != OK ||
                 csound_array_ensure_capacity(
                   csound, array, capacity, p->h.insdshead) != OK ||
                 array->data == NULL)) {
      csound->Free(csound, newSizes);
      csound->ErrorMsg(csound, "%s",
                       Str("OSC: Failed to allocate memory for array\n"));
      return OK;
    }
    oldSizes = array->sizes;
    array->dimensions = view.dimensions;
    array->sizes = newSizes;
    csound->Free(csound, oldSizes);
    if (view.values.count != 0) {
      memcpy(array->data, view.values.data,
             view.values.count * sizeof(MYFLT));
    }
    return OK;
}

static int32_t osc_decode_audio(CSOUND *csound, OSCLISTEN *p, int32_t index,
                                const void *payload, size_t payloadBytes)
{
    OSC_MYFLT_BLOB_VIEW view;
    MYFLT *output = p->args[index];

    if (osc_blob_parse_audio(payload, payloadBytes, (size_t)CS_KSMPS,
                             &view) != OK) {
      return osc_malformed_blob(csound, p, 'a');
    }
    if (view.count != 0) {
      memcpy(output, view.data, view.count * sizeof(MYFLT));
    }
    if (view.count < (size_t)CS_KSMPS) {
      memset(output + view.count, 0,
             ((size_t)CS_KSMPS - view.count) * sizeof(MYFLT));
    }
    return OK;
}

static int32_t osc_decode_ftable(CSOUND *csound, OSCLISTEN *p, int32_t index,
                                 const void *payload, size_t payloadBytes)
{
    OSC_MYFLT_BLOB_VIEW view;
    int32_t fno = MYFLT2LRND(*p->args[index]);
    FUNC *ftp;

    if (osc_blob_parse_myflts(payload, payloadBytes, &view) != OK ||
        view.count > INT32_MAX) {
      return osc_malformed_blob(csound, p, 'G');
    }
    if (UNLIKELY(fno <= 0)) {
      return csound->PerfError(csound, &(p->h),
                               Str("Invalid ftable no. %d"), fno);
    }
    ftp = csound->FTFind(csound, p->args[index]);
    if (UNLIKELY(ftp == NULL)) {
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("OSC internal error"));
    }
    if (view.count > ftp->flen) {
      if (UNLIKELY(csound->FTAlloc(
                     csound, fno, (int32_t)view.count) != OK)) {
        csound->ErrorMsg(csound, "%s",
                         Str("OSC: Failed to allocate memory for ftable\n"));
        return OK;
      }
      ftp = csound->FTFind(csound, p->args[index]);
      if (UNLIKELY(ftp == NULL)) {
        return csound->PerfError(csound, &(p->h),
                                 "%s", Str("OSC internal error"));
      }
    }
    if (view.count != 0) {
      memcpy(ftp->ftable, view.data, view.count * sizeof(MYFLT));
      if (view.count == ftp->flen) {
        ftp->ftable[ftp->flen] = ftp->ftable[0];
      }
    }
    return OK;
}

static int32_t osc_decode_blob(CSOUND *csound, OSCLISTEN *p, int32_t index,
                               char type, const void *payload,
                               size_t payloadBytes)
{
    switch (type) {
    case 'D':
      return osc_decode_direct_array(
        csound, p, (ARRAYDAT *)p->args[index], payload, payloadBytes);
    case 'A':
      return osc_decode_array(
        csound, p, (ARRAYDAT *)p->args[index], payload, payloadBytes);
    case 'a':
      return osc_decode_audio(csound, p, index, payload, payloadBytes);
    case 'G':
      return osc_decode_ftable(csound, p, index, payload, payloadBytes);
    case 'S':
      return OK;
    default:
      return csound->PerfError(csound, &(p->h),
                               Str("OSC: invalid blob type '%c'"), type);
    }
}

static int32_t OSC_list(CSOUND *csound, OSCLISTEN *p)
{
    OSC_PAT *m;
    int32_t status = OK;

    if (UNLIKELY(p->port->mutex_ == NULL)) {
      *p->kans = 0;
      return OK;
    }
    csound->LockMutex(p->port->mutex_);
    m = p->c.patterns;
    /* check again for thread safety */
    if (m != NULL) {
      int32_t i, malformedBlob = 0;
      /* unlink from queue */
      p->c.patterns = m->next;
      /* copy arguments */
      //printf("copying args\n");
      for (i = 0; p->c.saved_types[i] != '\0'; i++) {
        //printf("%d: type %c\n", i, p->c.saved_types[i]);
        if (p->c.saved_types[i] == 's') {
          char *src = m->args[i].string.data;
          STRINGDAT* dest = (STRINGDAT*) p->args[i];
          if (src != NULL) {
            size_t len = strlen(src);
            if (dest->size <= len) {
              char *temp = csound->ReAlloc(csound, dest->data, len + 1);
              if (temp != NULL) {
                dest->data = temp;
                dest->size = len + 1;
              } else {
                /* Allocation failed, preserve original dest->data/size */
                csound->ErrorMsg(csound, "%s",
                                 "OSC: Failed to allocate memory for string\n");
                continue; /* Skip this argument */
              }
            }
            if (dest->data != NULL)
              strcpy(dest->data, src);
          }
        }
        else if (p->c.saved_types[i]=='b') {
          char c = p->type->data[i];
          int32_t blobBytes = lo_blob_datasize(m->args[i].blob);
          int32_t blobStatus;
          if (blobBytes < 0) {
            blobStatus = osc_malformed_blob(csound, p, c);
          }
          else {
            blobStatus = osc_decode_blob(
              csound, p, i, c, lo_blob_dataptr(m->args[i].blob),
              (size_t)blobBytes);
          }
          csound->Free(csound, m->args[i].blob);
          m->args[i].blob = NULL;
          if (blobStatus == OSC_BLOB_DROPPED) {
            malformedBlob = 1;
            blobStatus = OK;
          }
          if (status == OK && blobStatus != OK) {
            status = blobStatus;
          }
        }
        else
          *(p->args[i]) = m->args[i].number;
      }
      if (!malformedBlob)
        p->malformedBlobWarning = 0;
      /* push to stack of free message structures */
      m->next = p->c.freePatterns;
      p->c.freePatterns = m;
      *p->kans = 1;
      OSC_GLOBALS *g = alloc_globals(csound);
      csound->LockMutex(g->mutex_);
      g->osccounter--;
      csound->UnlockMutex(g->mutex_);
    }
    else
      *p->kans = 0;
    csound->UnlockMutex(p->port->mutex_);
    return status;
}

/* ******** ARRAY VERSION **** EXPERIMENTAL *** */

static int32_t OSC_ahandler(const char *path, const char *types,
                       lo_arg **argv, int32_t argc, lo_message data, void *p)
{
    IGN(argc);  IGN(data);
    OSC_PORT  *pp = (OSC_PORT*) p;
    OSCLCOMMON *o;
    CSOUND    *csound = (CSOUND *) pp->csound;
    int32_t   retval = 1;
    //printf("***in ahandler\n");
    csound->LockMutex(pp->mutex_);
    o = (OSCLCOMMON*) pp->oplst;
    //printf("opst=%p\n", o);
    while (o != NULL) {
      //printf("Looking at %s/%s against %s/%s\n",
      //       o->saved_path, path,o->saved_types, types);
      if (strcmp(o->saved_path, path) == 0 &&
          strcmp(o->saved_types, types) == 0) {
        /* Message is for this guy */
        int32_t     i;
        OSC_PAT *m;
        OSC_GLOBALS *g = alloc_globals(csound);
        csound->LockMutex(g->mutex_);
        g->osccounter++;
        csound->UnlockMutex(g->mutex_);
        //printf("handler found message\n");
        m = get_pattern(csound, o);
        if (m != NULL) {
          /* queue message for being read by OSClisten opcode */
          m->next = NULL;
          if (o->patterns == NULL)
            o->patterns = m;
          else {
            OSC_PAT *mm;
            for (mm = o->patterns; mm->next != NULL; mm = mm->next)
              ;
            mm->next = m;
          }
          /* copy argument list */
          for (i = 0; o->saved_types[i] != '\0'; i++) {
            switch (types[i]) {
            default:              /* Should not happen */
            case 'i':
              m->args[i].number = (MYFLT) argv[i]->i; break;
            case 'h':
              m->args[i].number = (MYFLT) argv[i]->i64; break;
            case 'c':
              m->args[i].number= (MYFLT) argv[i]->c; break;
            case 'f':
              m->args[i].number = (MYFLT) argv[i]->f; break;
            case 'd':
              m->args[i].number= (MYFLT) argv[i]->d; break;
            }
          }
          retval = 0;
        }
        break;
      }
      o = (OSCLCOMMON*) o->nxt;
    }

    pp->csound->UnlockMutex(pp->mutex_);
    return retval;
}

static int32_t OSC_alist_init(CSOUND *csound, OSCLISTENA *p)
{
    //void  *x;
    int32_t   i, n;

    OSC_GLOBALS *pp =
      (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    if (UNLIKELY(pp == NULL))
      return csound->InitError(csound, "%s", Str("OSC not running"));
    /* find port */
    n = (int32_t) *(p->ihandle);
    if (UNLIKELY(n < 0 || n >= pp->nPorts))
      return csound->InitError(csound, "%s", Str("invalid handle"));
    p->port = pp->ports[n];
    if (UNLIKELY(p->port == NULL || p->port->thread == NULL ||
                 p->port->mutex_ == NULL))
      return csound->InitError(csound, "%s", Str("invalid handle"));
    p->c.saved_path = (char*) csound->Malloc(csound,
                                           strlen((char*) p->dest->data) + 1);
    strcpy(p->c.saved_path, (char*) p->dest->data);
    /* check for a valid argument list */
    n = (int32_t)strlen((char *)p->type->data);
    if (UNLIKELY(tabinit(csound, p->args, n, p->h.insdshead) != OK))
      return csound_array_init_resize_error(csound);
    strcpy(p->c.saved_types, (char*) p->type->data);
    for (i = 0; i < n; i++) {
      switch (p->c.saved_types[i]) {
      case 'c':
      case 'd':
      case 'f':
      case 'h':
      case 'i':
        break;
      default:
        return csound->InitError(csound, "%s", Str("invalid type"));
      }
    }
    csound->LockMutex(p->port->mutex_);
    p->c.nxt = p->port->oplst;
    p->port->oplst = (void*) &p->c;
    csound->UnlockMutex(p->port->mutex_);
    p->c.method = lo_server_thread_add_method(p->port->thread,
                                              p->c.saved_path, p->c.saved_types,
                                              OSC_ahandler, p->port);
    return OK;
}

static int32_t OSC_alist(CSOUND *csound, OSCLISTENA *p)
{
    OSC_PAT *m;
    if (UNLIKELY(p->port->mutex_ == NULL)) {
      *p->kans = 0;
      return OK;
    }
    csound->LockMutex(p->port->mutex_);
    m = p->c.patterns;
    /* check again for thread safety */
    if (m != NULL) {
      int32_t i;
      /* unlink from queue */
      p->c.patterns = m->next;
      /* copy arguments */
      //printf("copying args\n");
      for (i = 0; p->c.saved_types[i] != '\0'; i++) {
        //printf("%d: type %c\n", i, p->c.saved_types[i]);
        ((MYFLT*)p->args->data)[i] = m->args[i].number;
      }
      /* push to stack of free message structures */
      m->next = p->c.freePatterns;
      p->c.freePatterns = m;
      *p->kans = 1;
      OSC_GLOBALS *g = alloc_globals(csound);
      csound->LockMutex(g->mutex_);
      g->osccounter--;
      csound->UnlockMutex(g->mutex_);
    }
    else
      *p->kans = 0;
    csound->UnlockMutex(p->port->mutex_);
    return OK;
}



#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "OSCsend_lo", S(OSCSEND), 0,  "", "kSkSN",
    (SUBR)osc_send_set, (SUBR)osc_send, (SUBR) oscsend_deinit, NULL, 2 },
    { "OSCinit", S(OSCINIT), 0, "i", "i",
    (SUBR)osc_listener_init, NULL, (SUBR) OSC_deinit , NULL, 2 },
    { "OSCinitM", S(OSCINITM), 0,  "i", "Si",
    (SUBR)osc_listener_initMulti, NULL, (SUBR) OSC_deinit , NULL, 2 },
    { "OSClisten", S(OSCLISTEN),0,  "k", "iSSN",
    (SUBR)OSC_list_init, (SUBR)OSC_list, (SUBR) OSC_listdeinit, NULL, 2 },
    { "OSClisten", S(OSCLISTEN),0,  "k", "iSS",
    (SUBR)OSC_list_init, (SUBR)OSC_list, (SUBR) OSC_listdeinit, NULL, 2 },
    { "OSClisten", S(OSCLISTENA),0,  "kk[]", "iSS",
    (SUBR)OSC_alist_init, (SUBR)OSC_alist, (SUBR) OSC_listadeinit, NULL, 2 },
    { "OSCcount", S(OSCcount), 0,  "k", "",
    (SUBR)OSCcounter, (SUBR)OSCcounter, NULL, NULL, 2 },
  /* aliases */
  { "oscsendlo", S(OSCSEND), 0,  "", "kSkSN",
    (SUBR)osc_send_set, (SUBR)osc_send, (SUBR) oscsend_deinit, NULL},
  { "oscinit", S(OSCINIT), 0, "i", "i",
    (SUBR)osc_listener_init, NULL, (SUBR) OSC_deinit , NULL },
  { "oscinitm", S(OSCINITM), 0,  "i", "Si",
    (SUBR)osc_listener_initMulti, NULL, (SUBR) OSC_deinit , NULL },
  { "osclisten", S(OSCLISTEN),0,  "k", "iSSN",
    (SUBR)OSC_list_init, (SUBR)OSC_list, (SUBR) OSC_listdeinit, NULL },
  { "osclisten", S(OSCLISTEN),0,  "k", "iSS",
    (SUBR)OSC_list_init, (SUBR)OSC_list, (SUBR) OSC_listdeinit, NULL },
  { "osclisten", S(OSCLISTENA),0,  "kk[]", "iSS",
    (SUBR)OSC_alist_init, (SUBR)OSC_alist, (SUBR) OSC_listadeinit, NULL },
  { "osccount", S(OSCcount), 0,  "k", "",
    (SUBR)OSCcounter, (SUBR)OSCcounter, NULL }
};

 int64_t csound_opcode_init(CSOUND *csound, OENTRY **ep)
{
    IGN(csound);
    *ep = localops;
    return (int64_t) sizeof(localops);
}

 int32_t csoundModuleInfo(void)
{
    return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT));
}
