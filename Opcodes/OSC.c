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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <lo/lo.h>

/* structure for real time event */

/* typedef struct rtEvt_s { */
/*     struct rtEvt_s  *nxt; */
/*     EVTBLK  e; */
/* } rtEvt_t; */

typedef struct {
    OPDS h;             /* default header */
    MYFLT *kwhen;
    STRINGDAT *host;
    MYFLT *port;        /* UDP port */
    STRINGDAT *dest;
    STRINGDAT *type;
    MYFLT *arg[32];     /* only 26 can be used, but add a few more for safety */
    lo_address addr;
    MYFLT last;
    int   cnt;
} OSCSEND;


typedef struct osc_pat {
    struct osc_pat *next;
  union {
    MYFLT number;
    STRINGDAT string;
   } args[31];
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
    int     nPorts;
    OSC_PORT  *ports;
} OSC_GLOBALS;

/* opcode for starting the OSC listener (called once from orchestra header) */
typedef struct {
    OPDS    h;                  /* default header */
    MYFLT   *ihandle;
    MYFLT   *port;              /* Port number on which to listen */
} OSCINIT;

typedef struct {
    OPDS    h;                  /* default header */
    MYFLT   *kans;
    MYFLT   *ihandle;
    STRINGDAT   *dest;
    STRINGDAT   *type;
    MYFLT   *args[32];
    OSC_PORT  *port;
    char    *saved_path;
    char    saved_types[32];    /* copy of type list */
    OSC_PAT *patterns;          /* FIFO list of pending messages */
    OSC_PAT *freePatterns;      /* free message stack */
    void    *nxt;               /* pointer to next opcode on the same port */
} OSCLISTEN;

static int oscsend_deinit(CSOUND *csound, OSCSEND *p)
{
    lo_address a = (lo_address)p->addr;
    lo_address_free(a);
    return OK;
}

static int osc_send_set(CSOUND *csound, OSCSEND *p)
{
    char port[8];
    char *pp = port;
    char *hh;
    unsigned int i;

    /* with too many args, XINCODE may not work correctly */
    if (UNLIKELY(p->INOCOUNT > 31))
      return csound->InitError(csound, Str("Too many arguments to OSCsend"));
    /* a-rate arguments are not allowed */
    for (i = 0; i < p->INOCOUNT-5; i++) {
      if (strcmp("a", csound->GetTypeForArg(p->arg[i])->varTypeName) == 0) {
        return csound->InitError(csound, Str("No a-rate arguments allowed"));
      }
    }

    if (*p->port<0)
      pp = NULL;
    else
      snprintf(port, 8, "%d", (int) MYFLT2LRND(*p->port));
    hh = (char*) p->host->data;
    if (*hh=='\0') hh = NULL;
    p->addr = lo_address_new(hh, pp);
    p->cnt = 0;
    p->last = 0;
    csound->RegisterDeinitCallback(csound, p,
                                   (int (*)(CSOUND *, void *)) oscsend_deinit);
    return OK;
}

static int osc_send(CSOUND *csound, OSCSEND *p)
{
    /* Types I allow at present:
       0) int
       1) float
       2) string
       3) double
       4) char
       5) table as blob
    */
    char port[8];
    char *pp = port;
    char *hh;

    if (*p->port<0)
      pp = NULL;
    else
      snprintf(port, 8, "%d", (int) MYFLT2LRND(*p->port));
    hh = (char*) p->host->data;
    if (*hh=='\0') hh = NULL;
    p->addr = lo_address_new(hh, pp);

    if (p->cnt++ ==0 || *p->kwhen!=p->last) {
      int i=0;
      int msk = 0x20;           /* First argument */
      lo_message msg = lo_message_new();
      char *type = (char*)p->type->data;
      MYFLT **arg = p->arg;
      p->last = *p->kwhen;
      for (i=0; type[i]!='\0'; i++, msk <<=1) {
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
            msk <<= 1; i++;
            if (UNLIKELY(type[i]!='t'))
              return csound->PerfError(csound, p->h.insdshead,
                                       Str("Time stamp is two values"));
            tt.frac = (uint32_t)(*arg[i]+FL(0.5));
            lo_message_add_timetag(msg, tt);
            break;
          }
          //#ifdef SOMEFINEDAY
        case 'T':               /* Table/blob */
          {
            lo_blob myblob;
            int     len;
            FUNC    *ftp;
            void *data;
            /* make sure fn exists */
            if (LIKELY((ftp=csound->FTnp2Find(csound,arg[i]))!=NULL)) {
              data = ftp->ftable;
              len = ftp->flen-1;        /* and set it up */
            }
            else {
              return csound->PerfError(csound, p->h.insdshead,
                                       Str("ftable %.2f does not exist"), *arg[i]);
            }
            myblob = lo_blob_new(sizeof(MYFLT)*len, data);
            lo_message_add_blob(msg, myblob);
            lo_blob_free(myblob);
            break;
          }
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
static int OSC_reset(CSOUND *csound, OSC_GLOBALS *p)
{
    int i;
    for (i = 0; i < p->nPorts; i++)
      if (p->ports[i].thread) {
        lo_server_thread_stop(p->ports[i].thread);
        lo_server_thread_free(p->ports[i].thread);
        csound->DestroyMutex(p->ports[i].mutex_);
      }
    csound->DestroyGlobalVariable(csound, "_OSC_globals");
    return OK;
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
      csound->ErrorMsg(csound, Str("OSC: failed to allocate globals"));
      return NULL;
    }
    pp = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    pp->csound = csound;
    csound->RegisterResetCallback(csound, (void*) pp,
                                  (int (*)(CSOUND *, void *)) OSC_reset);
    return pp;
}



 /* ------------------------------------------------------------------------ */

static CS_NOINLINE OSC_PAT *alloc_pattern(OSCLISTEN *pp)
{
    CSOUND  *csound;
    OSC_PAT *p;
    size_t  nbytes;

    csound = pp->h.insdshead->csound;
    /* number of bytes to allocate */
    nbytes = sizeof(OSC_PAT);
    /* allocate and initialise structure */
    p = (OSC_PAT*) csound->Calloc(csound, nbytes);

    return p;
}

static inline OSC_PAT *get_pattern(OSCLISTEN *pp)
{
    OSC_PAT *p;

    if (pp->freePatterns != NULL) {
      p = pp->freePatterns;
      pp->freePatterns = p->next;
      return p;
    }
    return alloc_pattern(pp);
}

static int OSC_handler(const char *path, const char *types,
                       lo_arg **argv, int argc, void *data, void *p)
{
    OSC_PORT  *pp = (OSC_PORT*) p;
    OSCLISTEN *o;
    CSOUND *csound = (CSOUND *) pp->csound;
    int       retval = 1;

    pp->csound->LockMutex(pp->mutex_);
    o = (OSCLISTEN*) pp->oplst;
    while (o != NULL) {
      if (strcmp(o->saved_path, path) == 0 &&
          strcmp(o->saved_types, types) == 0) {
        /* Message is for this guy */
        int     i;
        OSC_PAT *m;
        m = get_pattern(o);
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
              {
                char  *src = (char*) &(argv[i]->s), *dst = m->args[i].string.data;
                if(m->args[i].string.size <= (int) strlen(src)){
                  if(dst != NULL) csound->Free(csound, dst);
                    dst = csound->Strdup(csound, src);
                    m->args[i].string.size = strlen(dst) + 1;
                    m->args[i].string.data = dst;
                }
                else strcpy(dst, src);

              }
              break;
            }
          }

          retval = 0;
        }

        break;

      }


      o = (OSCLISTEN*) o->nxt;
    }

    pp->csound->UnlockMutex(pp->mutex_);
    return retval;
}

static void OSC_error(int num, const char *msg, const char *path)
{
    fprintf(stderr, "OSC server error %d in path %s: %s\n", num, path, msg);
}

static int OSC_deinit(CSOUND *csound, OSCINIT *p)
{
    int n = (int)*p->ihandle;
    OSC_GLOBALS *pp = alloc_globals(csound);
    OSC_PORT    *ports = pp->ports;
    csound->Message(csound, "handle=%d\n", n);
    csound->DestroyMutex(ports[n].mutex_);
    ports[n].mutex_ = NULL;
    lo_server_thread_stop(ports[n].thread);
    lo_server_thread_free(ports[n].thread);
    ports[n].thread =  NULL;
    csound->Message(csound, Str("OSC deinitiatised\n"));
    return OK;
}

static int osc_listener_init(CSOUND *csound, OSCINIT *p)
{
    OSC_GLOBALS *pp;
    OSC_PORT    *ports;
    char        buff[32];
    int         n;

    /* allocate and initialise the globals structure */
    pp = alloc_globals(csound);
    n = pp->nPorts;
    ports = (OSC_PORT*) csound->ReAlloc(csound, pp->ports,
                                        sizeof(OSC_PORT) * (n + 1));
    ports[n].csound = csound;
    ports[n].mutex_ = csound->Create_Mutex(0);
    ports[n].oplst = NULL;
    snprintf(buff, 32, "%d", (int) *(p->port));
    ports[n].thread = lo_server_thread_new(buff, OSC_error);
    lo_server_thread_start(ports[n].thread);
    pp->ports = ports;
    pp->nPorts = n + 1;
    csound->Warning(csound, Str("OSC listener #%d started on port %s\n"), n, buff);
    *(p->ihandle) = (MYFLT) n;
    csound->RegisterDeinitCallback(csound, p,
                                   (int (*)(CSOUND *, void *)) OSC_deinit);
    return OK;
}

static int OSC_listdeinit(CSOUND *csound, OSCLISTEN *p)
{
    OSC_PAT *m;

    csound->LockMutex(p->port->mutex_);
    if (p->port->oplst == (void*) p)
      p->port->oplst = p->nxt;
    else {
      OSCLISTEN *o = (OSCLISTEN*) p->port->oplst;
      for ( ; o->nxt != (void*) p; o = (OSCLISTEN*) o->nxt)
        ;
      o->nxt = p->nxt;
    }
    csound->UnlockMutex(p->port->mutex_);
    lo_server_thread_del_method(p->port->thread, p->saved_path, p->saved_types);
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

static int OSC_list_init(CSOUND *csound, OSCLISTEN *p)
{
    //void  *x;
    int   i, n;

    OSC_GLOBALS *pp = (OSC_GLOBALS*)
                        csound->QueryGlobalVariable(csound, "_OSC_globals");
    if (UNLIKELY(pp == NULL))
      return csound->InitError(csound, Str("OSC not running"));
    /* find port */
    n = (int) *(p->ihandle);
    if (UNLIKELY(n < 0 || n >= pp->nPorts))
      return csound->InitError(csound, Str("invalid handle"));
    p->port = &(pp->ports[n]);
    p->saved_path = (char*) csound->Malloc(csound,
                                           strlen((char*) p->dest->data) + 1);
    strcpy(p->saved_path, (char*) p->dest->data);
    /* check for a valid argument list */
    n = csound->GetInputArgCnt(p) - 3;
    if (UNLIKELY(n < 1 || n > 28))
      return csound->InitError(csound, Str("invalid number of arguments"));
    if (UNLIKELY((int) strlen((char*) p->type->data) != n))
      return csound->InitError(csound,
                               "argument list inconsistent with format string");
    strcpy(p->saved_types, (char*) p->type->data);
    for (i = 0; i < n; i++) {
      const char *s;
      s = csound->GetInputArgName(p, i + 3);
      if (s[0] == 'g')
        s++;
      switch (p->saved_types[i]) {
#ifdef SOMEFINEDAY
      case 'T':
        p->saved_types[i] = 'b';
#endif
      case 'c':
      case 'd':
      case 'f':
      case 'h':
      case 'i':
        if (UNLIKELY(*s != 'k'))
          return csound->InitError(csound, Str("argument list inconsistent "
                                               "with format string"));
        break;
      case 's':
        if (UNLIKELY(*s != 'S'))
          return csound->InitError(csound, Str("argument list inconsistent "
                                               "with format string"));
        break;
      default:
        return csound->InitError(csound, Str("invalid type"));
      }
    }
    csound->LockMutex(p->port->mutex_);
    p->nxt = p->port->oplst;
    p->port->oplst = (void*) p;
    csound->UnlockMutex(p->port->mutex_);
    (void) lo_server_thread_add_method(p->port->thread,
                                       p->saved_path, p->saved_types,
                                       OSC_handler, p->port);
    csound->RegisterDeinitCallback(csound, p,
                                   (int (*)(CSOUND *, void *)) OSC_listdeinit);
    return OK;
}

static int OSC_list(CSOUND *csound, OSCLISTEN *p)
{
    OSC_PAT *m;

    /* quick check for empty queue */
    if (p->patterns == NULL) {
      *p->kans = 0;
      return OK;
    }
    csound->LockMutex(p->port->mutex_);
    m = p->patterns;
    /* check again for thread safety */
    if (m != NULL) {
      int i;
      /* unlink from queue */
      p->patterns = m->next;
      /* copy arguments */
      for (i = 0; p->saved_types[i] != '\0'; i++) {
        if (p->saved_types[i] != 's') {
          *(p->args[i]) = m->args[i].number;
        }
        else {
          char *src = m->args[i].string.data;
          char *dst = ((STRINGDAT*) p->args[i])->data;
          if(src != NULL) {
            if(((STRINGDAT*) p->args[i])->size <= (int) strlen(src)){
               if(dst != NULL) csound->Free(csound, dst);
                  dst = csound->Strdup(csound, src);
                 ((STRINGDAT*) p->args[i])->size = strlen(dst) + 1;
                 ((STRINGDAT*) p->args[i])->data = dst;
           }
          else
          strcpy(dst, src);
          }
      }
      }
      /* push to stack of free message structures */
      m->next = p->freePatterns;
      p->freePatterns = m;
      *p->kans = 1;
    }
    else
      *p->kans = 0;
    csound->UnlockMutex(p->port->mutex_);
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "OSCsend", S(OSCSEND), 0, 3, "", "kSkSSN", (SUBR)osc_send_set, (SUBR)osc_send },
{ "OSCinit", S(OSCINIT), 0, 1, "i", "i", (SUBR)osc_listener_init },
{ "OSClisten", S(OSCLISTEN),0, 3, "k", "iSSN", (SUBR)OSC_list_init, (SUBR)OSC_list}
};

PUBLIC long csound_opcode_init(CSOUND *csound, OENTRY **ep)
{
    IGN(csound);
    *ep = localops;
    return (long) sizeof(localops);
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}
