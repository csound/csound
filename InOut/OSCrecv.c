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
     ENVIRON    *csound;
     void       *threadLock;
     lo_server_thread thread;
     OSC_PAT    *patterns;      /* List of things to do */
} OSC_GLOBALS;

#if 0
/* callback function called by sensevents() once in every control period */

static void event_sense_callback(ENVIRON *csound, OSC_GLOBALS *p)
{
     /* are there any pending events ? */
     if (p->patterns != NULL) {
       OSC_PAT *m = p->patterns;
       csound->WaitThreadLock(csound, p->threadLock, 1000);
       while (m) {
         csound->NotifyThreadLock(csound, p->threadLock);
         csound->WaitThreadLock(csound, p->threadLock, 1000);
       }
       csound->NotifyThreadLock(csound, p->threadLock);
     }
}
#endif

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
        pp->csound->NotifyThreadLock(pp->csound, pp->threadLock);
        m->active = 1;
        return 0;
      }
      m = m->next;
    }
    pp->csound->NotifyThreadLock(pp->csound, pp->threadLock);
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

static int osc_listener_init(ENVIRON *csound, OSCINIT *p)
{
    OSC_GLOBALS *pp;
    char buff[12];
    /* allocate and initialise the globals structure */
    if (csound->CreateGlobalVariable(csound, "_OSC_globals",
                                     sizeof(OSC_GLOBALS)) != 0)
      csound->Die(csound, Str("OSC: failed to allocate globals"));
    pp = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    pp->csound = csound;
    pp->threadLock = csound->CreateThreadLock(csound);
    /* ---- initialise any other data in OSC_GLOBALS here ---- */
    /* ... */
    pp->patterns = NULL;
    /* ---- code to create and start the OSC thread ---- */
    sprintf(buff, "%d", (int)(*p->port));
    pp->thread = lo_server_thread_new(buff, OSC_error);
/*     lo_server_thread_add_method(pp->thread, NULL, NULL, OSC_handler, pp); */
    lo_server_thread_start(pp->thread);
    /* register callback function for sensevents() */
    /* the function will be called once in every control period */
/*      csound->RegisterSenseEventCallback(csound, (void (*)(void*, void*)) */
/*                                                   event_sense_callback, pp); */
    csound->Message(csound,
                    "OSC listener started\n");
    return OK;
}

/* RESET routine for cleaning up */

static int OSC_reset(ENVIRON *csound, void *userData)
{
    OSC_GLOBALS *p;
    OSC_PAT *m;
    p = (OSC_GLOBALS*) csound->QueryGlobalVariable(csound, "_OSC_globals");
    if (p == NULL)
      return OK;    /* nothing to do */
    /* ---- code to stop and destroy OSC thread ---- */
    lo_server_thread_stop(p->thread);
    lo_server_thread_free(p->thread);
    csound->WaitThreadLock(csound, p->threadLock, 1000);
    m = p->patterns; p->patterns = NULL;
    while (m) {
      OSC_PAT *nxt = m->next;
      free(m->path); free(m->type);
      free(m);
      m = nxt;
    }
    csound->NotifyThreadLock(csound, p->threadLock);
    csound->DestroyThreadLock(csound, p->threadLock);
    csound->DestroyGlobalVariable(csound, "_OSC_globals");
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

int OSC_list_init(ENVIRON *csound, OSCLISTEN *p)
{
    void *x;
    OSC_PAT *m = (OSC_PAT*)malloc(sizeof(OSC_PAT)+sizeof(MYFLT)*(p->INOCOUNT-2));
    /* Add a pattern to the list of recognised things */
    OSC_GLOBALS *pp = (OSC_GLOBALS*)
                             csound->QueryGlobalVariable(csound, "_OSC_globals");
    if (pp == NULL) {
      csound->Message(csound, "OSC not running\n");
      return NOTOK;
    }
    m->path = strdup((char*) p->dest);
    m->type = strdup((char*) p->type);
    m->active = 0;
    m->length = p->INOCOUNT-2;
    m->next = pp->patterns;
    pp->patterns = m;
    p->pat = m;
    x = lo_server_thread_add_method(pp->thread, (char*)p->dest, (char*)p->type,
                                    OSC_handler, pp);
    csound->Message(csound, "%p: Looking for dest \"%s\" with types %s %d args\n",
                    x, m->path, m->type, m->length);
    return OK;
}

int OSC_list(ENVIRON *csound, OSCLISTEN *p)
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

PUBLIC OENTRY *opcode_init(ENVIRON *csound)
{
    csound->Message(csound,
                    "****OSC: liblo started****\n");
    csound->RegisterResetCallback(csound, NULL,
                                  (int (*)(void *, void *)) OSC_reset);
    return localops;
}

