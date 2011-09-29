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

typedef struct rtEvt_s {
    struct rtEvt_s  *nxt;
    EVTBLK  e;
} rtEvt_t;

typedef struct {
    OPDS h;             /* default header */
    MYFLT *kwhen;
    MYFLT *host;
    MYFLT *port;        /* UDP port */
    MYFLT *dest;
    MYFLT *type;
    MYFLT *arg[32];     /* only 26 can be used, but add a few more for safety */
    lo_address addr;
    MYFLT last;
    int   cnt;
} OSCSEND;

typedef struct {
    OPDS    h;
    MYFLT   *i_port;
    MYFLT   *S_path;
    MYFLT   *i_absp2;
} OSCRECV;

typedef struct osc_pat {
    struct osc_pat *next;
    MYFLT   *args[31];
    MYFLT   data[1];
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
    /* for OSCrecv */
    rtEvt_t *eventQueue;
    void    *mutex_;
    lo_server_thread  st;
    double  baseTime;
    int     absp2mode;
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
    MYFLT   *dest;
    MYFLT   *type;
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

    /* with too many args, XINCODE/XSTRCODE may not work correctly */
    if (UNLIKELY(p->INOCOUNT > 31))
      return csound->InitError(csound, Str("Too many arguments to OSCsend"));
    /* a-rate arguments are not allowed */
    if (UNLIKELY(p->XINCODE))
      return csound->InitError(csound, Str("No a-rate arguments allowed"));

    if (*p->port<0)
      pp = NULL;
    else
      sprintf(port, "%d", (int) MYFLT2LRND(*p->port));
    hh = (char*) p->host;
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
    if (p->cnt++ ==0 || *p->kwhen!=p->last) {
      int i=0;
      int msk = 0x20;           /* First argument */
      lo_message msg = lo_message_new();
      char *type = (char*)p->type;
      MYFLT **arg = p->arg;
      p->last = *p->kwhen;
      for (i=0; type[i]!='\0'; i++, msk <<=1) {
        /* Need to add type checks */
        switch (type[i]) {
        case 'i':
          if (UNLIKELY(p->XSTRCODE&msk))
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_int32(msg, (int32_t) MYFLT2LRND(*arg[i]));
          break;
        case 'l':
          if (UNLIKELY(p->XSTRCODE&msk))
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_int64(msg, (int64_t) MYFLT2LRND(*arg[i]));
          break;
        case 'c':
          if (UNLIKELY(p->XSTRCODE&msk))
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_char(msg, (char) (*arg[i] + FL(0.5)));
          break;
        case 'm':
          {
            union a {
              int32_t  x;
              uint8_t  m[4];
            } mm;
            if (UNLIKELY(p->XSTRCODE&msk))
              return csound->PerfError(csound, Str("String not expected"));
            mm.x = *arg[i]+FL(0.5);
            lo_message_add_midi(msg, mm.m);
            break;
          }
        case 'f':
          if (UNLIKELY(p->XSTRCODE&msk))
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_float(msg, (float)(*arg[i]));
          break;
        case 'd':
          if (UNLIKELY(p->XSTRCODE&msk))
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_double(msg, (double)(*arg[i]));
          break;
        case 's':
          if (LIKELY(p->XSTRCODE&msk))
            lo_message_add_string(msg, (char*)arg[i]);
          else
            return csound->PerfError(csound, Str("Not a string when needed"));
          break;
        case 'b':               /* Boolean */
          if (UNLIKELY(p->XSTRCODE&msk))
            return csound->PerfError(csound, Str("String not expected"));
          if (*arg[i]==FL(0.0)) lo_message_add_true(msg);
          else lo_message_add_false(msg);
          break;
        case 't':               /* timestamp */
          {
            lo_timetag tt;
            if (UNLIKELY(p->XSTRCODE&msk))
              return csound->PerfError(csound, Str("String not expected"));
            tt.sec = (uint32_t)(*arg[i]+FL(0.5));
            msk <<= 1; i++;
            if (UNLIKELY(type[i]!='t'))
              return csound->PerfError(csound,
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
            if (UNLIKELY(p->XSTRCODE&msk))
              return csound->PerfError(csound, Str("String not expected"));
            /* make sure fn exists */
            if (LIKELY((ftp=csound->FTFind(csound,arg[i]))!=NULL)) {
              data = ftp->ftable;
              len = ftp->flen-1;        /* and set it up */
            }
            else {
              return csound->PerfError(csound,
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
      lo_send_message(p->addr, (char*)p->dest, msg);
      lo_message_free(msg);
    }
    return OK;
}

/* callback function called by sensevents() once in every control period */

static void event_sense_callback(CSOUND *csound, OSC_GLOBALS *p)
{
    /* are there any pending events ? */
    if (p->eventQueue == NULL)
      return;

    csound->LockMutex(p->mutex_);
    while (p->eventQueue != NULL) {
      long  startTime;
      rtEvt_t *ep = p->eventQueue;
      p->eventQueue = ep->nxt;
      csound->UnlockMutex(p->mutex_);
      startTime = (p->absp2mode ? p->baseTime*csound->esr : csound->icurTime);
      startTime += (double) ep->e.p[2]*csound->esr;
      ep->e.p[2] = FL(0.0);
      if (ep->e.pcnt < 3 || ep->e.p[3] < FL(0.0) ||
          ep->e.opcod == 'q' || ep->e.opcod == 'f' || ep->e.opcod == 'e' ||
          (double) ep->e.p[3] >= csound->icurTime - startTime) {
        if (startTime < csound->icurTime) {
          if (ep->e.pcnt >= 3 && ep->e.p[3] > FL(0.0) &&
              ep->e.opcod != 'q' && ep->e.opcod != 'f')
            ep->e.p[3] -= (MYFLT) (csound->icurTime - startTime)/csound->esr;
          startTime = csound->icurTime;
        }
        if (ep->e.opcod == 'T')
          p->baseTime = csound->icurTime/csound->esr;
        else
          csound->insert_score_event_at_sample(csound, &(ep->e), startTime);
      }
      if (ep->e.strarg != NULL)
        free(ep->e.strarg);
      free(ep);
      csound->LockMutex(p->mutex_);
    }
    csound->UnlockMutex(p->mutex_);
}

/* callback function for OSC thread */
/* NOTE: this function does not run in the main Csound audio thread, */
/* so use of the API or access to CSOUND should be limited or avoided */

static int osc_event_handler(const char *path, const char *types,
                             lo_arg **argv, int argc, lo_message msg,
                             void *user_data)
{
    OSC_GLOBALS *p = (OSC_GLOBALS*) user_data;
    CSOUND      *csound = p->csound;
    rtEvt_t     *evt;
    int         i;
    char        opcod = '\0';

    /* check for valid format */
    if ((unsigned int) (argc - 1) > (unsigned int) PMAX)
      return 1;
    switch ((int) types[0]) {
      case 'i': opcod = (char) argv[0]->i; break;
      case 'f': opcod = (char) MYFLT2LRND((MYFLT) argv[0]->f); break;
      case 's': opcod = (char) argv[0]->s; break;
      default:  return 1;
    }
    switch ((int) opcod) {
      case 'e': break;
      case 'T': if (argc > 1) return 1;
                break;
      case 'f': if (argc < 6) return 1;
                break;
      case 'i':
      case 'q':
      case 'a': if (argc < 4) return 1;
                break;
      default:  return 1;
    }
    /* Create the new event */
    evt = (rtEvt_t*) malloc(sizeof(rtEvt_t));
    if (evt == NULL)
      return 1;
    evt->nxt = NULL;
    evt->e.strarg = NULL;
    evt->e.opcod = opcod;
    evt->e.pcnt = argc - 1;
    evt->e.p[1] = evt->e.p[2] = evt->e.p[3] = FL(0.0);
    for (i = 1; i < argc; i++) {
      switch ((int) types[i]) {
      case 'i':
        evt->e.p[i] = (MYFLT) argv[i]->i;
        break;
      case 'f':
        evt->e.p[i] = (MYFLT) argv[i]->f;
        break;
      case 's':
        /* string argument: cannot have more than one */
        evt->e.p[i] = (MYFLT) SSTRCOD;
        if (evt->e.strarg != NULL) {
          free(evt->e.strarg);
          free(evt);
          return 1;
        }
        evt->e.strarg = (char*) malloc(strlen(&(argv[i]->s)) + 1);
        if (evt->e.strarg == NULL) {
          free(evt);
          return 1;
        }
        strcpy(evt->e.strarg, &(argv[i]->s));
        break;
      }
    }
    /* queue event for handling by main Csound thread */
    csound->LockMutex(p->mutex_);
    if (p->eventQueue == NULL)
      p->eventQueue = evt;
    else {
      rtEvt_t *ep = p->eventQueue;
      while (ep->nxt != NULL)
        ep = ep->nxt;
      ep->nxt = evt;
    }
    csound->UnlockMutex(p->mutex_);
    return 0;
}

static void osc_error_handler(int n, const char *msg, const char *path)
{
    return;
}

/* RESET routine for cleaning up */

static int OSC_reset(CSOUND *csound, OSC_GLOBALS *p)
{
    int i;

    if (p->mutex_ != NULL) {
      /* stop and destroy OSC thread */
      lo_server_thread_stop(p->st);
      lo_server_thread_free(p->st);
      /* delete any pending events */
      csound->LockMutex(p->mutex_);
      while (p->eventQueue != NULL) {
        rtEvt_t *nxt = p->eventQueue->nxt;
        if (p->eventQueue->e.strarg != NULL)
          free(p->eventQueue->e.strarg);
        free(p->eventQueue);
        p->eventQueue = nxt;
      }
      csound->UnlockMutex(p->mutex_);
      csound->DestroyMutex(p->mutex_);
    }
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
                                              sizeof(OSC_GLOBALS)) != 0))
      csound->Die(csound, Str("OSC: failed to allocate globals"));
    pp = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    pp->csound = csound;
    csound->RegisterResetCallback(csound, (void*) pp,
                                  (int (*)(CSOUND *, void *)) OSC_reset);
    return pp;
}

/* OSCrecv opcode (called once from orchestra header) */

static int OSCrecv_init(CSOUND *csound, OSCRECV *p)
{
    OSC_GLOBALS *pp;
    char        portName[256], *pathName;

    /* allocate and initialise the globals structure */
    pp = alloc_globals(csound);
    if (UNLIKELY(pp->mutex_ != NULL))
      return csound->InitError(csound, Str("OSCrecv is already running"));
    pp->eventQueue = NULL;
    pp->mutex_ = csound->Create_Mutex(0);
    pp->baseTime = 0.0;
    pp->absp2mode = (*(p->i_absp2) == FL(0.0) ? 0 : 1);
    /* create OSC thread */
    sprintf(portName, "%d", (int) MYFLT2LRND(*p->i_port));
    pp->st = lo_server_thread_new(portName,
                                  (lo_err_handler) osc_error_handler);
    /* register OSC event handler */
    pathName = (char*) p->S_path;
    if (pathName[0] == '\0')
      pathName = NULL;
    lo_server_thread_add_method(pp->st, pathName, NULL,
                                (lo_method_handler) osc_event_handler, pp);
    /* start thread */
    lo_server_thread_start(pp->st);
    /* register callback function for sensevents() */
    csound->RegisterSenseEventCallback(csound, (void (*)(CSOUND*, void*))
                                                 event_sense_callback, pp);
    return OK;
}

 /* ------------------------------------------------------------------------ */

static CS_NOINLINE OSC_PAT *alloc_pattern(OSCLISTEN *pp)
{
    CSOUND  *csound;
    OSC_PAT *p;
    size_t  nbytes, str_smps;
    int     i, cnt, strArgMask;

    csound = pp->h.insdshead->csound;
    cnt = csound->GetInputArgCnt(pp) - 3;
    strArgMask = (int) (csound->GetInputArgSMask(pp) >> 3);
    /* number of bytes to allocate */
    nbytes = sizeof(OSC_PAT) - sizeof(MYFLT);
    str_smps = (size_t) csound->strVarMaxLen + sizeof(MYFLT) - (size_t) 1;
    str_smps /= sizeof(MYFLT);
    for (i = 0; i < cnt; i++)
      nbytes += (((strArgMask & (1 << i)) ? str_smps : (size_t) 1)
                 * sizeof(MYFLT));
    /* allocate and initialise structure */
    p = (OSC_PAT*) malloc(nbytes);
    if (p == NULL)
      return NULL;
    p->args[0] = &(p->data[0]);
    for (i = 1; i < cnt; i++)
      p->args[i] = (MYFLT*) p->args[i - 1]
                   + ((strArgMask & (1 << (i - 1))) ? (int) str_smps : 1);
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
              *(m->args[i]) = (MYFLT) argv[i]->i; break;
            case 'h':
              *(m->args[i]) = (MYFLT) argv[i]->i64; break;
            case 'c':
              *(m->args[i]) = (MYFLT) argv[i]->c; break;
            case 'f':
              *(m->args[i]) = (MYFLT) argv[i]->f; break;
            case 'd':
              *(m->args[i]) = (MYFLT) argv[i]->d; break;
            case 's':
              {
                char  *src = (char*) &(argv[i]->s), *dst = (char*) m->args[i];
                char  *endp = dst + (pp->csound->strVarMaxLen - 1);
                while (*src != (char) '\0' && dst != endp)
                  *(dst++) = *(src++);
                *dst = (char) '\0';
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
    sprintf(buff, "%d", (int) *(p->port));
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
      free(m);
      m = mm;
    }
    m = p->freePatterns;
    p->freePatterns = NULL;
    while (m != NULL) {
      OSC_PAT *mm = m->next;
      free(m);
      m = mm;
    }
    return OK;
}

static int OSC_list_init(CSOUND *csound, OSCLISTEN *p)
{
    void  *x;
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
    p->saved_path = (char*) csound->Malloc(csound, strlen((char*) p->dest) + 1);
    strcpy(p->saved_path, (char*) p->dest);
    /* check for a valid argument list */
    n = csound->GetInputArgCnt(p) - 3;
    if (UNLIKELY(n < 1 || n > 28))
      return csound->InitError(csound, Str("invalid number of arguments"));
    if (UNLIKELY((int) strlen((char*) p->type) != n))
      return csound->InitError(csound,
                               "argument list inconsistent with format string");
    strcpy(p->saved_types, (char*) p->type);
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
    x = lo_server_thread_add_method(p->port->thread,
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
        if (p->saved_types[i] != 's')
          *(p->args[i]) = *(m->args[i]);
        else
          strcpy((char*) p->args[i], (char*) m->args[i]);
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
{ "OSCsend", S(OSCSEND), 3, "", "kSiSSN", (SUBR)osc_send_set, (SUBR)osc_send },
{ "OSCrecv", S(OSCRECV), 1, "", "iSo",    (SUBR)OSCrecv_init },
{ "OSCinit", S(OSCINIT), 1, "i", "i", (SUBR)osc_listener_init },
{ "OSClisten", S(OSCLISTEN),3, "k", "iSSN", (SUBR)OSC_list_init, (SUBR)OSC_list}
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

