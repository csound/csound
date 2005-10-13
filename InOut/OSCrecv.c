/*
    OSCrecv.c:

    Copyright (C) 2005 John ffitch

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <lo/lo.h>
#include "csdl.h"

typedef struct osc_pat {
  struct osc_pat *next;
  char           *path;
  char           *type;
  int            active;
  int            length;
  MYFLT          args[1];
} OSC_PAT;

/* structure for global variables */

typedef struct {
     CSOUND     *csound;
     void       *threadLock;
     lo_server_thread *thread;
     OSC_PAT    *patterns;      /* List of things to do */
     int        nport;          /* Number of active ports */
} OSC_GLOBALS;

static int OSC_handler(const char *path, const char *types,
                        lo_arg **argv, int argc, void *data, void *p)
{
    OSC_GLOBALS *pp = (OSC_GLOBALS*)p;
    OSC_PAT *m = pp->patterns;
    while (m) {
      if (strcmp(m->path, path)==0 && strcmp(m->type, types)==0) {
        /* Message is for this guy */
        int i, len = m->length;
        for (i=0; i<len; i++) {
          MYFLT x;
          switch (types[i]) {
          default:              /* Should not happen */
          case 'i':
            x = (MYFLT)argv[i]->i; break;
          case 'l':
            x = (MYFLT)argv[i]->i64; break;
          case 'c':
            x = (MYFLT)argv[i]->c; break;
          case 'm':
            x = (MYFLT)argv[i]->i32; break;
          case 'f':
            x = (MYFLT)argv[i]->f; break;
          case 'd':
            x = (MYFLT)argv[i]->d; break;
          case 's':
            x = (MYFLT)argv[i]->S; break;
          case 'b':
            x = (MYFLT)argv[i]->i; break;
          case 't':
            m->args[i] = (MYFLT)(argv[i]->i32);
            x = (MYFLT)(argv[++i]->i32); break;
          }
          m->args[i] = (MYFLT) (x);
        }
        pp->csound->NotifyThreadLock(pp->threadLock);
        m->active = 1;
        return 0;
      }
      m = m->next;
    }
    pp->csound->NotifyThreadLock(pp->threadLock);
    return 1;
}

void OSC_error(int num, const char *msg, const char *path)
{
    fprintf(stderr, "OSC server error %d in path %s: %s\n", num, path, msg);
}

/* opcode for starting the OSC listener (called once from orchestra header) */
typedef struct {
    OPDS h;                     /* default header */
    MYFLT *port;                /* Port number on which to listen */
} OSCINIT;

/* RESET routine for cleaning up */

static int OSC_reset(CSOUND *csound, void *userData)
{
    OSC_GLOBALS *p;
    OSC_PAT *m;
    int i;
    p = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    if (p == NULL)
      return OK;    /* nothing to do */
    /* ---- code to stop and destroy OSC thread ---- */
    for (i=0; i<p->nport; i++) {
      lo_server_thread_stop(p->thread[i]);
      lo_server_thread_free(p->thread[i]);
    }
    csound->WaitThreadLock(p->threadLock, 1000);
    m = p->patterns; p->patterns = NULL;
    while (m) {
      OSC_PAT *nxt = m->next;
      free(m->path); free(m->type);
      free(m);
      m = nxt;
    }
    csound->NotifyThreadLock(p->threadLock);
    csound->DestroyThreadLock(p->threadLock);
    csound->DestroyGlobalVariable(csound, "_OSC_globals");
    return OK;
}

static int osc_listener_init(CSOUND *csound, OSCINIT *p)
{
    OSC_GLOBALS *pp;
    char buff[20];
    int port = (int)(*p->port+FL(0.5));
    /* allocate and initialise the globals structure */
    if (csound->CreateGlobalVariable(csound, "_OSC_globals",
                                     sizeof(OSC_GLOBALS)) ==  0) {
      pp = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
      pp->csound = csound;
      pp->threadLock = csound->CreateThreadLock();
      pp->patterns = NULL;
      pp->thread = (lo_server_thread*)csound->Malloc(csound, sizeof(lo_server_thread));
      pp->nport = 1;
      csound->RegisterResetCallback(csound, pp,
                                    (int (*)(CSOUND *, void *)) OSC_reset);
    }
    else {
      pp = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
      pp->nport++;
      pp->thread = (lo_server_thread*)csound->ReAlloc(csound, pp->thread,
                                         sizeof(lo_server_thread)*pp->nport);
    }
    /* ---- code to create and start the OSC thread ---- */
    sprintf(buff, "%d", port);
    pp->thread[pp->nport-1] = lo_server_thread_new(buff, OSC_error);
    lo_server_thread_start(pp->thread[pp->nport-1]);
    csound->Message(csound, "OSC listener on port %d started\n", port);
    return OK;
}

/* ---- implement OSC opcodes ---- */
typedef struct {
    OPDS h;                     /* default header */
    MYFLT  *kans;
    MYFLT  *dest;
    MYFLT  *type;
    MYFLT  *args[25];
    OSC_PAT *pat;
} OSCLISTEN;

int OSC_listdeinit(CSOUND *csound, OSCLISTEN *p)
{
    /* remove p->pat and also the handler which seems difficult */
    OSC_PAT *m;
    OSC_GLOBALS *pp = (OSC_GLOBALS*)
                        csound->QueryGlobalVariable(csound, "_OSC_globals");
    if (pp == NULL) {
      csound->Message(csound, "OSC not running\n");
      return NOTOK;
    }
    m = pp->patterns;
    if (m==p->pat) {
      pp->patterns = m->next;
      free(m->path); free(m->type);
      free(m);
    }
    else {
      OSC_PAT *n;
      while (m->next != p->pat) {
        m = m->next;
        if (m==NULL) return NOTOK;
      }
      n = m;
      m = m->next;
      n->next = m->next;
      free(m->path); free(m->type);
      free(m);
    }
    return OK;
}

int OSC_list_init(CSOUND *csound, OSCLISTEN *p)
{
    void *x;
    OSC_PAT *m;
    int i;

    /* Add a pattern to the list of recognised things */
    OSC_GLOBALS *pp = (OSC_GLOBALS*)
                        csound->QueryGlobalVariable(csound, "_OSC_globals");
    if (pp == NULL) {
      return csound->InitError(csound, "OSC not running\n");
    }
    m = (OSC_PAT*)malloc(sizeof(OSC_PAT)+sizeof(MYFLT)*(p->INOCOUNT-2));
    m->path = strdup((char*) p->dest);
    m->type = strdup((char*) p->type);
    m->active = 0;
    m->length = p->INOCOUNT-2;
    m->next = pp->patterns;
    pp->patterns = m;
    p->pat = m;
    for (i=0; i<pp->nport; i++)
      x = lo_server_thread_add_method(pp->thread[i], (char*)p->dest,
                                      (char*)p->type, OSC_handler, pp);
    csound->RegisterDeinitCallback(csound,
                                   p, (int (*)(CSOUND*,void*)) OSC_listdeinit);
    return OK;
}

int OSC_list(CSOUND *csound, OSCLISTEN *p)
{
    OSC_PAT *m = p->pat;
    if (m->active) {
      int i;
      for (i=0; i<m->length; i++)
        *p->args[i] = m->args[i];
      m->active = 0;
      *p->kans = 1;
    }
    else
      *p->kans = 0;
    return OK;
}

/* ... */

#define S(x) sizeof(x)

static OENTRY localops[] = {
  { "OSCinit", S(OSCINIT), 1, "", "i", (SUBR)osc_listener_init},
  { "OSClisten", S(OSCLISTEN), 3, "k", "SSz", (SUBR)OSC_list_init, (SUBR)OSC_list }
};

PUBLIC long opcode_size(void)
{
     return (long) sizeof(localops);
}

PUBLIC OENTRY *opcode_init(CSOUND *csound)
{
#ifdef BETA
    csound->Message(csound, "****OSC: liblo started****\n");
#endif
    return localops;
}

