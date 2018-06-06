/*
    tabsave.c:

    Copyright (C) 2018 John ffitch

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
#include "interlocks.h"
#include <pthread.h>

typedef struct {
    OPDS    h;
    MYFLT   *kans;
    MYFLT   *trig;
    MYFLT   *itab;
    STRINGDAT *file;
    MYFLT   *sync;
    
    /* Local */
} TABSAVE;

typedef struct {
    MYFLT*   t;
    uint32_t size;
    FILE*    ff;
    MYFLT*   ans;
    CSOUND*  csound;
    INSDS   *insdshead;
} SAVE_THREAD;

static void *write_tab(void* pp)
{
    SAVE_THREAD *p = (SAVE_THREAD*)pp;
    MYFLT*   t = p->t;
    uint32_t size = p->size;
    FILE*    ff = p->ff;
    MYFLT*   ans = p->ans;
    CSOUND*  csound = p->csound;
    INSDS   *insdshead = p->insdshead;
    free(pp);
    //printf("t=%p size=%d ff=%p\n", t, size, ff);
    if (fwrite(t, sizeof(MYFLT), size, ff) != size) {
      fclose(ff);
      csound->PerfError(csound, insdshead,
                           Str("tabsave: failed to write data %d"),size);
      *ans = -FL(1.0);
    }
    else *ans = FL(1.0);
    fclose(ff);
    return NULL;
}

static int32_t tabsave(CSOUND *csound, TABSAVE *p)
{
    if (*p->trig) {
      FUNC  *ftp;
      MYFLT *t;
      uint32_t size;
      FILE  *ff;
      if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->itab)) == NULL)) {
        return csound->InitError(csound, Str("tabsave: No table"));
      }
      *p->kans = FL(0.0);
      t = ftp->ftable;
      size = ftp->flen;

      //printf("t=%p size=%d file=%s\n", t, size, p->file->data);
      ff = fopen(p->file->data, "wb");
      if (ff==NULL)
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("tabsave: failed to open file %s"),
                                 p->file->data);
      if (*p->sync==FL(0.0)) {  /* write in perf thread */
        if (fwrite(t, sizeof(MYFLT), size, ff) != size) {
          fclose(ff);
          return csound->PerfError(csound, p->h.insdshead,
                                   Str("tabsave: failed to write data %d"),size);
        }
        fclose(ff);
      }
      else {                    /* Use a detached helper thread */
        SAVE_THREAD *q = (SAVE_THREAD*)malloc(sizeof(SAVE_THREAD));
        pthread_t write_thread;
        q->t = t;
        q->size = size;
        q->ff = ff;
        q->ans = p->kans;
        q->csound = csound;
        q->insdshead = p->h.insdshead;
        if (pthread_create(&write_thread, NULL, write_tab, q)) {
          INSDS * i = q->insdshead;
          free(q);
          return csound->PerfError(csound, i,
                                   Str("Error creating thread"));
        }
        //pthread_detach(write_thread);
      }
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY tabsave_localops[] = {
{ "tabsave",     S(TABSAVE),     TR, 2,  "k",    "kkSp",  NULL, (SUBR)tabsave },
};

LINKAGE_BUILTIN(tabsave_localops)



