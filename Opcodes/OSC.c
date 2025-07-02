
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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

/* Haiku 'int32' etc definitions in net headers conflict with sysdep.h */
#define __HAIKU_CONFLICT

#include "csdl.h"
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
    OSC_PORT  *ports;
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
            memcpy(&data[1], arg[i], data[0]);
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

/* RESET routine for cleaning up */
static int32_t OSC_reset(CSOUND *csound, OSC_GLOBALS *p)
{
    int32_t i;
    for (i = 0; i < p->nPorts; i++)
      if (p->ports[i].thread) {
        lo_server_thread_stop(p->ports[i].thread);
        lo_server_thread_free(p->ports[i].thread);
        csound->DestroyMutex(p->ports[i].mutex_);
      }
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
    csound->RegisterResetCallback(csound, (void*) pp,
                                  (int32_t (*)(CSOUND *, void *)) OSC_reset);
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
    OSC_GLOBALS *pp = alloc_globals(csound);
    OSC_PORT    *ports;
    if (UNLIKELY(pp==NULL)) return NOTOK;
    ports = pp->ports;
    csound->Message(csound, "handle=%d\n", n);
    csound->DestroyMutex(ports[n].mutex_);
    ports[n].mutex_ = NULL;
    lo_server_thread_stop(ports[n].thread);
    lo_server_thread_free(ports[n].thread);
    ports[n].thread =  NULL;
    csound->Message(csound, "%s", Str("OSC deinitialised\n"));
    return OK;
}

static int32_t osc_listener_init(CSOUND *csound, OSCINIT *p)
{
    OSC_GLOBALS *pp;
    OSC_PORT    *ports;
    char        buff[32];
    int32_t         n;

    /* allocate and initialise the globals structure */
    pp = alloc_globals(csound);
    n = pp->nPorts;
    ports = (OSC_PORT*) csound->ReAlloc(csound, pp->ports,
                                        sizeof(OSC_PORT) * (n + 1));
    ports[n].csound = csound;
    ports[n].mutex_ = csound->Create_Mutex(0);
    ports[n].oplst = NULL;
    snprintf(buff, 32, "%d", (int32_t) *(p->port));
    ports[n].thread = lo_server_thread_new(buff, OSC_error);
    if (UNLIKELY(ports[n].thread==NULL))
      return csound->InitError(csound,
                               Str("cannot start OSC listener on port %s\n"),
                               buff);
    ///if (lo_server_thread_start(ports[n].thread)<0)
    ///  return csound->InitError(csound,
    ///                           Str("cannot start OSC listener on port %s\n"),
    ///                           buff);
    lo_server_thread_start(ports[n].thread);
    pp->ports = ports;
    pp->nPorts = n + 1;
    csound->Warning(csound, Str("OSC listener #%d started on port %s\n"), n, buff);
    *(p->ihandle) = (MYFLT) n;
    //csound->RegisterDeinitCallback(csound, p,
    //                             (int32_t (*)(CSOUND *, void *)) OSC_deinit);
    return OK;
}

static int32_t osc_listener_initMulti(CSOUND *csound, OSCINITM *p)
{
    OSC_GLOBALS *pp;
    OSC_PORT    *ports;
    char        buff[32];
    int32_t     n;

    /* allocate and initialise the globals structure */
    pp = alloc_globals(csound);
    n = pp->nPorts;
    ports = (OSC_PORT*) csound->ReAlloc(csound, pp->ports,
                                        sizeof(OSC_PORT) * (n + 1));
    ports[n].csound = csound;
    ports[n].mutex_ = csound->Create_Mutex(0);
    ports[n].oplst = NULL;
    snprintf(buff, 32, "%d", (int32_t) *(p->port));
    ports[n].thread = lo_server_thread_new_multicast(p->group->data,
                                                     buff, OSC_error);
    if (UNLIKELY(ports[n].thread==NULL))
      return csound->InitError(csound,
                               Str("cannot start OSC listener on port %s\n"),
                               buff);
    ///if (lo_server_thread_start(ports[n].thread)<0)
    ///  return csound->InitError(csound,
    ///                           Str("cannot start OSC listener on port %s\n"),
    ///                           buff);
    lo_server_thread_start(ports[n].thread);
    pp->ports = ports;
    pp->nPorts = n + 1;
    csound->Warning(csound,
                    Str("OSC multicast listener #%d started on port %s\n"),
                    n, buff);
    *(p->ihandle) = (MYFLT) n;

    return OK;
}

static int32_t OSC_listendeinit(CSOUND *csound, OSC_PORT *port, OSCLCOMMON *p)
{
    OSC_PAT *m;

    if (port->mutex_==NULL) return NOTOK;
    csound->LockMutex(port->mutex_);
    if (port->oplst == (void*)p)
      port->oplst = p->nxt;
    else {
      OSCLCOMMON *o = (OSCLCOMMON*) port->oplst;
      for ( ; o->nxt != (void*) p; o = (OSCLCOMMON*) o->nxt)
        ;
      o->nxt = p->nxt;
    }
    csound->UnlockMutex(port->mutex_);
#ifdef LIBLO29
    //Would like to use this call but requires liblo2.29
    lo_server_thread_del_lo_method (port->thread, p->method);
#else
    lo_server_thread_del_method(port->thread, p->saved_path, p->saved_types);
#endif
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

    OSC_GLOBALS *pp =
      (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    if (UNLIKELY(pp == NULL))
      return csound->InitError(csound, "%s", Str("OSC not running"));
    /* find port */
    n = (int32_t) *(p->ihandle);
    if (UNLIKELY(n < 0 || n >= pp->nPorts))
      return csound->InitError(csound, "%s", Str("invalid handle"));
    p->port = &(pp->ports[n]);
    p->c.saved_path = (char*) csound->Malloc(csound,
                                           strlen((char*) p->dest->data) + 1);
    strcpy(p->c.saved_path, (char*) p->dest->data);
    /* check for a valid argument list */
    n = GetInputArgCnt((OPDS *)p) - 3;
    if (UNLIKELY(n < 0 || n > ARG_CNT-4))
      return csound->InitError(csound, "%s", Str("invalid number of arguments"));
    if (UNLIKELY((int32_t) strlen((char*) p->type->data) != n))
      return csound->InitError(csound,
                               "%s", Str("argument list inconsistent with "
                                   "format string"));
    strcpy(p->c.saved_types, (char*) p->type->data);
    for (i = 0; i < n; i++) {
      const char *s;
      s = GetInputArgName((OPDS *)p, i + 3);
      if (s[0] == 'g')
        s++;
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
        if (UNLIKELY(*s != 'k'))
          return csound->InitError(csound, "%s", Str("argument list inconsistent "
                                               "with format string"));
        break;
      case 's':
        if (UNLIKELY(*s != 'S'))
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

static int32_t OSC_list(CSOUND *csound, OSCLISTEN *p)
{
    OSC_PAT *m;

    /* quick check for empty queue */
    if (p->c.patterns == NULL) {
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
        if (p->c.saved_types[i] == 's') {
          char *src = m->args[i].string.data;
          char *dst = ((STRINGDAT*) p->args[i])->data;
          if (src != NULL) {
            if (((STRINGDAT*) p->args[i])->size <= strlen(src)){
              if (dst != NULL) csound->Free(csound, dst);
              dst = csound->Strdup(csound, src);
              ((STRINGDAT*) p->args[i])->size = strlen(dst) + 1;
              ((STRINGDAT*) p->args[i])->data = dst;
           }
          else
            strcpy(dst, src);
          }
        }
        else if (p->c.saved_types[i]=='b') {
          char c = p->type->data[i];
          int32_t len =  lo_blob_datasize(m->args[i].blob);
          //printf("blob found %p type %c\n", m->args[i].blob, c);
          //printf("length = %d\n", lo_blob_datasize(m->args[i].blob));
          int32_t *idata = lo_blob_dataptr(m->args[i].blob);
          if (c == 'D') {
            int32_t j;
            MYFLT *data = (MYFLT *) idata;
            ARRAYDAT* arr = (ARRAYDAT*)p->args[i];
            int32_t asize = 1;
            for (j=0; j < arr->dimensions; j++) {
              asize *= arr->sizes[j];
            }
            len /= sizeof(MYFLT);
            if (asize < len) {
              arr->data = (MYFLT *)
                csound->ReAlloc(csound, arr->data, len*sizeof(MYFLT));
              asize = len;
             for (j = 0; j < arr->dimensions-1; j++)
              asize /= arr->sizes[j];
             arr->sizes[arr->dimensions-1] = asize;
            }
            memcpy(arr->data,data,len*sizeof(MYFLT));
           }
          else if (c == 'A') {       /* Decode an numeric array */
            int32_t j;
            MYFLT* data = (MYFLT*)(&idata[1+idata[0]]);
            int32_t size = 1;
            ARRAYDAT* foo = (ARRAYDAT*)p->args[i];
            foo->dimensions = idata[0];
            csound->Free(csound, foo->sizes);
            foo->sizes = (int32_t*)csound->Malloc(csound, sizeof(int32_t)*idata[0]);
#ifdef OSC_DEBUG
            printf("dimension=%d\n", idata[0]);
#endif
            for (j=0; j<idata[0]; j++) {
              foo->sizes[j] = idata[j+1];
#ifdef OSC_DEBUG
              printf("sizes[%d] = %d\n", j, idata[j+1]);
#endif
              size*=idata[j+1];
            }
#ifdef OSC_DEBUG
            printf("idata = %i %i %i %i %i %i %i ...\n",
                   idata[0], idata[1], idata[2], idata[3],
                   idata[4], idata[5], idata[6]);
            printf("data = %f, %f, %f...\n", data[0], data[1], data[2]);
#endif
            foo->data = (MYFLT*)csound->Malloc(csound, sizeof(MYFLT)*size);
            memcpy(foo->data, data, sizeof(MYFLT)*size);
            //printf("data = %f %f ...\n", foo->data[0], foo->data[1]);
          }
          else if (c == 'a') {

            MYFLT *data= (MYFLT*)idata;
            uint32_t len = (uint32_t)data[0];
            if (len>CS_KSMPS) len = CS_KSMPS;
            memcpy(p->args[i], &data[1], len*sizeof(MYFLT));
          }
          else if (c == 'G') {  /* ftable received */
            //FUNC* data = (FUNC*)idata;
            MYFLT *data = (MYFLT *) idata;
            int32_t fno = MYFLT2LRND(*p->args[i]);
            FUNC *ftp;
            if (UNLIKELY(fno <= 0))
              return csound->PerfError(csound, &(p->h),
                                       Str("Invalid ftable no. %d"), fno);

            ftp = csound->FTFind(csound, p->args[i]);
            if (UNLIKELY(ftp==NULL)) {
              return csound->PerfError(csound, &(p->h),
                                       "%s", Str("OSC internal error"));
            }
            if (len > (int32_t)  (ftp->flen*sizeof(MYFLT)))
              ftp->ftable = (MYFLT*)csound->ReAlloc(csound, ftp->ftable,
                                                    len*sizeof(MYFLT));
            memcpy(ftp->ftable,data,len);

#if 0
            ftp = csound->FTFind(csound, p->args[i]);
            if (UNLIKELY(ftp==NULL)) { // need to allocate ***FIXME***
              return csound->PerfError(csound, &(p->h),
                                       "%s", Str("OSC internal error"));
            }
            memcpy(ftp, data, sizeof(FUNC)-sizeof(MYFLT*));
            ftp->fno = fno;
#ifdef OSC_DEBUG
            printf("%d\n", len);
#endif
            if (len > ftp->flen*sizeof(MYFLT))
              ftp->ftable =
                (MYFLT*)csound->ReAlloc(csound, ftp->ftable,
                                        len-sizeof(FUNC)+sizeof(MYFLT*));
#endif
            {
#ifdef OSC_DEBUG
              MYFLT* dst = ftp->ftable;
              MYFLT* src = (MYFLT*)(&(data->ftable));

              //int32_t j;
              printf("copy data: from %p to %p length %d %d\n",
                     src, dst, len-sizeof(FUNC)+sizeof(MYFLT*), data->flen);
              printf("was %f %f %f ...\n", dst[0], dst[1], dst[2]);
              printf("will be %f %f %f ...\n", src[0],src[1], src[2]);
              memcpy(dst, src, len-sizeof(FUNC)+sizeof(MYFLT*));
#endif
              //for (j=0; j<data->flen;j++) dst[j]=src[j];
              //printf("now %f %f %f ...\n", dst[0], dst[1], dst[2]);
            }
          }
          else if (c == 'S') {
          }
          else return csound->PerfError(csound,  &(p->h), "Oh dear");
          csound->Free(csound, m->args[i].blob);
        }
        else
          *(p->args[i]) = m->args[i].number;
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

#include "arrays.h"

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
    p->port = &(pp->ports[n]);
    p->c.saved_path = (char*) csound->Malloc(csound,
                                           strlen((char*) p->dest->data) + 1);
    strcpy(p->c.saved_path, (char*) p->dest->data);
    /* check for a valid argument list */
    tabinit(csound, p->args, n= (int32_t)strlen((char*) p->type->data), p->h.insdshead);
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
    /* quick check for empty queue */
    if (p->c.patterns == NULL) {
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
    (SUBR)osc_send_set, (SUBR)osc_send, (SUBR) oscsend_deinit, NULL },
  { "OSCinit", S(OSCINIT), 0, "i", "i",
    (SUBR)osc_listener_init, NULL, (SUBR) OSC_deinit , NULL },
  { "OSCinitM", S(OSCINITM), 0,  "i", "Si",
    (SUBR)osc_listener_initMulti, NULL, (SUBR) OSC_deinit , NULL },
  { "OSClisten", S(OSCLISTEN),0,  "k", "iSSN",
    (SUBR)OSC_list_init, (SUBR)OSC_list, (SUBR) OSC_listdeinit, NULL },
  { "OSClisten", S(OSCLISTEN),0,  "k", "iSS",
    (SUBR)OSC_list_init, (SUBR)OSC_list, (SUBR) OSC_listdeinit, NULL },
  { "OSClisten", S(OSCLISTENA),0,  "kk[]", "iSS",
    (SUBR)OSC_alist_init, (SUBR)OSC_alist, (SUBR) OSC_listadeinit, NULL },
  { "OSCcount", S(OSCcount), 0,  "k", "",
    (SUBR)OSCcounter, (SUBR)OSCcounter, NULL }
};

PUBLIC int64_t csound_opcode_init(CSOUND *csound, OENTRY **ep)
{
    IGN(csound);
    *ep = localops;
    return (int64_t) sizeof(localops);
}

PUBLIC int32_t csoundModuleInfo(void)
{
    return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT));
}
