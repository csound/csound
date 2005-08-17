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
#include <lo/lo.h>

/* structure for real time event */

typedef struct rtEvt_s {
    struct rtEvt_s  *nxt;
    EVTBLK  e;
} rtEvt_t;

/* structure for global variables */

typedef struct {
    CSOUND *csound;
    rtEvt_t *eventQueue;
    void    *threadLock;
    lo_server_thread  st;
    double  baseTime;
    int     absp2mode;
} OSC_GLOBALS;

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

int osc_send_set(CSOUND *csound, OSCSEND *p)
{
    char port[8];
    char *pp= port;
    char *hh;
    lo_address t;
    /* with too many args, XINCODE/XSTRCODE may not work correctly */
    if (p->INOCOUNT > 31)
      return csound->InitError(csound, Str("Too many arguments to OSCsend"));
    /* a-rate arguments are not allowed */
    if (p->XINCODE)
      return csound->InitError(csound, Str("No a-rate arguments allowed"));

    if (*p->port<0)
      pp = NULL;
    else
      sprintf(port, "%d", (int) MYFLT2LRND(*p->port));
    hh = (char*) p->host;
    if (*hh=='\0') hh = NULL;
    t = lo_address_new(hh, pp);
    p->addr = t;
    p->cnt = 0;
    p->last = 0;
    return OK;
}

int osc_send(CSOUND *csound, OSCSEND *p)
{
    /* Types I allow at present:
       0) int
       1) float
       2) string
       3) double
       4) char
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
          if (p->XSTRCODE&msk)
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_int32(msg, (int32_t) MYFLT2LRND(*arg[i]));
          break;
        case 'l':
          if (p->XSTRCODE&msk)
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_int64(msg, (int64_t) MYFLT2LRND(*arg[i]));
          break;
        case 'c':
          if (p->XSTRCODE&msk)
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_char(msg, (char) (*arg[i] + FL(0.5)));
          break;
        case 'm':
          {
            union a {
              int32_t  x;
              uint8_t  m[4];
            } mm;
            if (p->XSTRCODE&msk)
              return csound->PerfError(csound, Str("String not expected"));
            mm.x = *arg[i]+FL(0.5);
            lo_message_add_midi(msg, mm.m);
            break;
          }
        case 'f':
          if (p->XSTRCODE&msk)
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_float(msg, (float)(*arg[i]));
          break;
        case 'd':
          if (p->XSTRCODE&msk)
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_double(msg, (double)(*arg[i]));
          break;
        case 's':
          if (p->XSTRCODE&msk)
            lo_message_add_string(msg, (char*)arg[i]);
          else
            return csound->PerfError(csound, Str("Not a string when needed"));
          break;
        case 'b':               /* Boolean */
          if (p->XSTRCODE&msk)
            return csound->PerfError(csound, Str("String not expected"));
          if (*arg[i]==FL(0.0)) lo_message_add_true(msg);
          else lo_message_add_false(msg);
          break;
        case 't':               /* timestamp */
          {
            lo_timetag tt;
            if (p->XSTRCODE&msk)
              return csound->PerfError(csound, Str("String not expected"));
            tt.sec = (uint32_t)(*arg[i]+FL(0.5));
            msk <<= 1; i++;
            if (type[i]!='t')
              return csound->PerfError(csound,
                                       Str("Time stanmp is two values"));
            tt.frac = (uint32_t)(*arg[i]+FL(0.5));
            lo_message_add_timetag(msg, tt);
            break;
          }
        default:
          csound->Message(csound, "Unknown OSC type %c\n", type[1]);
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

    csound->WaitThreadLock(csound, p->threadLock, 1000);
    while (p->eventQueue != NULL) {
      double  startTime;
      rtEvt_t *ep = p->eventQueue;
      p->eventQueue = ep->nxt;
      csound->NotifyThreadLock(csound, p->threadLock);
      startTime = (p->absp2mode ? p->baseTime : csound->curTime);
      startTime += (double) ep->e.p[2];
      ep->e.p[2] = FL(0.0);
      if (ep->e.pcnt < 3 || ep->e.p[3] < FL(0.0) ||
          ep->e.opcod == 'q' || ep->e.opcod == 'f' || ep->e.opcod == 'e' ||
          startTime + (double) ep->e.p[3] >= csound->curTime) {
        if (startTime < csound->curTime) {
          if (ep->e.pcnt >= 3 && ep->e.p[3] > FL(0.0) &&
              ep->e.opcod != 'q' && ep->e.opcod != 'f')
            ep->e.p[3] -= (MYFLT) (csound->curTime - startTime);
          startTime = csound->curTime;
        }
        if (ep->e.opcod == 'T')
          p->baseTime = csound->curTime;
        else
          csound->insert_score_event(csound, &(ep->e), startTime);
      }
      if (ep->e.strarg != NULL)
        free(ep->e.strarg);
      free(ep);
      csound->WaitThreadLock(csound, p->threadLock, 1000);
    }
    csound->NotifyThreadLock(csound, p->threadLock);
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
    if (argc < 1)
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
    csound->WaitThreadLock(csound, p->threadLock, 1000);
    if (p->eventQueue == NULL)
      p->eventQueue = evt;
    else {
      rtEvt_t *ep = p->eventQueue;
      while (ep->nxt != NULL)
        ep = ep->nxt;
      ep->nxt = evt;
    }
    csound->NotifyThreadLock(csound, p->threadLock);
    return 0;
}

static void osc_error_handler(int n, const char *msg, const char *path)
{
    return;
}

/* opcode for starting the OSC listener (called once from orchestra header) */

static int osc_listener_init(CSOUND *csound, OSCRECV *p)
{
    OSC_GLOBALS *pp;
    char        portName[256], *pathName;
    /* allocate and initialise the globals structure */
    if (csound->CreateGlobalVariable(csound, "__OSC_globals",
                                             sizeof(OSC_GLOBALS)) != 0)
      csound->Die(csound, Str("OSC: failed to allocate globals"));
    pp = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "__OSC_globals");
    pp->csound = csound;
    pp->eventQueue = NULL;
    pp->threadLock = csound->CreateThreadLock(csound);
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

/* RESET routine for cleaning up */

static int OSC_reset(CSOUND *csound, void *userData)
{
    OSC_GLOBALS *p;
    p = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "__OSC_globals");
    if (p == NULL)
      return OK;    /* nothing to do */
    /* stop and destroy OSC thread */
    lo_server_thread_stop(p->st);
    lo_server_thread_free(p->st);
    /* delete any pending events */
    csound->WaitThreadLock(csound, p->threadLock, 1000);
    while (p->eventQueue != NULL) {
      rtEvt_t *nxt = p->eventQueue->nxt;
      if (p->eventQueue->e.strarg != NULL)
        free(p->eventQueue->e.strarg);
      free(p->eventQueue);
      p->eventQueue = nxt;
    }
    csound->NotifyThreadLock(csound, p->threadLock);
    csound->DestroyThreadLock(csound, p->threadLock);
    csound->DestroyGlobalVariable(csound, "__OSC_globals");
    return OK;
}

#define S(x) sizeof(x)

static OENTRY localops[] = {
{ "OSCsend", S(OSCSEND), 3, "", "kSiSSN", (SUBR)osc_send_set, (SUBR)osc_send },
{ "OSCrecv", S(OSCRECV), 1, "", "iSo",    (SUBR)osc_listener_init }
};

PUBLIC long opcode_size(void)
{
    return (long) sizeof(localops);
}

PUBLIC OENTRY *opcode_init(CSOUND *csound)
{
    csound->RegisterResetCallback(csound, NULL,
                                  (int (*)(CSOUND *, void *)) OSC_reset);
    return localops;
}

