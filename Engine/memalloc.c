/*
    memalloc.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Richard Dobson

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

#include "cs.h"                         /*              MEMALLOC.C      */
/*RWD 9:2000 for pvocex support */
#include "pvfileio.h"
/*RWD 9:2000 fix memory leak in strings array */
extern void free_strings(void);
/* global here so reachable by all standalones */
extern void rlsmemfiles(void);

#define MEMDEBUG

#ifdef _MSC_VER                           /*RWD want this for console version too */
#include <crtdbg.h>
extern void DisplayMsg(char *, ...);
#endif
typedef struct {
    void *p;
    long  n;
} MEMREC;
static int apsize = 0;

#ifdef MEMDEBUG
extern void *memfiles;
static MEMREC* all = NULL;
static int ap=0;
void all_free(void)
{
    if (!all) return;
    rlsmemfiles();
    while (--ap>=0) {
      if (all[ap].p != NULL) free(all[ap].p);
      all[ap].p = NULL;
    }
    free(all);
    all = NULL;                 /* For safety */
    apsize = ap = 0;
    memfiles = NULL;
    return;
}
#endif

void memRESET(void)
{
#ifdef MEMDEBUG
    all_free();
    ap = 0;
    all = NULL;
#endif
    apsize = 0;
    /*RWD 9:2000 not terribly vital, but good to do this somewhere... */
    pvsys_release();
    free_strings();
}

static void memdie(long nbytes)
{
    err_printf(Str(X_989,"memory allocate failure for %d\n"), nbytes);
#ifdef mills_macintosh
    err_printf(Str(X_1297,"try increasing preferred size setting for "
                   "the Perf Application\n"));
#endif
    longjmp(cenviron.exitjmp_,1);
}

void *mcalloc(long nbytes) /* allocate new memory space, cleared to 0 */
{
    void *p;
#ifdef _DEBUG
    if (!_CrtCheckMemory()) {
      printf("Memory error\n");
    }
#endif
    if ((p = calloc((size_t)nbytes, (size_t)1)) == NULL) {
      if (nbytes==0) return NULL;
      else memdie(nbytes);
    }
#ifdef MEMDEBUG
    if (ap >= apsize) {
      MEMREC *new_all = (MEMREC*)realloc(all, sizeof(MEMREC)*(apsize += 1020));
      if (new_all == NULL)
        err_printf( "Too many allocs\n"), longjmp(cenviron.exitjmp_,1);
      all = new_all;
    }
    all[ap].n   = nbytes;
    all[ap++].p = p;
#endif
    return(p);
}

void *mmalloc(long nbytes) /* allocate new memory space, NOT cleared to 0 */
{
    void *p;

#ifdef _DEBUG
    if (!_CrtCheckMemory()) {
      printf("Memory error\n");
    }
#endif
    if ((p = malloc((size_t)nbytes)) == NULL)
      memdie(nbytes);
#ifdef MEMDEBUG
    if (ap >= apsize) {
      MEMREC *new_all = (MEMREC *)realloc(all, sizeof(MEMREC)*(apsize += 1020));
      if (new_all == NULL) {
        err_printf("Too many allocs\n");
        longjmp(cenviron.exitjmp_,1);
      }
      all = new_all;
    }
    all[ap].n   = nbytes;
    all[ap++].p = p;
#endif
    return(p);
}

void *mrealloc(void *old, long nbytes) /* Packaged realloc */
{
    void *p;

#ifdef _DEBUG
    if (!_CrtCheckMemory()) {
      printf("Memory error\n");
    }
#endif
    if ((p = realloc(old, (size_t)nbytes)) == NULL)
      memdie(nbytes);
#ifdef MEMDEBUG
    if (old != NULL) {
      int oldall = 0;
      while (all[oldall].p != old) {
        oldall++;
        if (oldall > ap) memdie(-nbytes);
      }
      /*        err_printf("Changing %p(%d) to %p(%d)\n",
                old, all[oldall].n, p, nbytes); */
      all[oldall].n = nbytes;
      all[oldall].p = p;
    }
    else {
      if (ap >= apsize) {
        MEMREC *new_all = (MEMREC*)realloc(all, sizeof(MEMREC)*(apsize += 1020));
        if (new_all == NULL) {
          err_printf( "Too many allocs\n");
          longjmp(cenviron.exitjmp_,1);
        }
        all = new_all;
      }
      all[ap].n   = nbytes;
      all[ap++].p = p;
    }
#endif
    return(p);
}

void mfree(void *ptr)
{
#ifdef MEMDEBUG
    int i = 0;
    if (!ptr) return;
    if (all == NULL) {
      return;
    }
    while (all[i].p != ptr) {
      i++;
      if (i>ap) memdie(0);
    }
    /*      err_printf("Freeing %d bytes\n", all[i].n); */
    all[i].p = NULL;
#endif
#ifdef _DEBUG
    if (!_CrtCheckMemory()) {
      printf("Memory error\n");
    }
#endif
    free(ptr);
}

