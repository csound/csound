/*
    prototyp.h:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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
                                                         /* PROTOTYP.H */
#ifndef _CSOUND_PROTO_H
#define _CSOUND_PROTO_H

#include <stdlib.h>
#define IGN(X) X = X

#ifndef CSOUND_CSDL_H

#ifdef __cplusplus
extern "C" {
#endif

void    cscorinit(ENVIRON*), cscore(ENVIRON*);
void    *mmalloc(void*, size_t), *mcalloc(void*, size_t);
void    *mrealloc(void*, void*, size_t), mfree(void*, void*);
void    csoundAuxAlloc(void*, long, AUXCH *), auxchfree(void*, INSDS *);
void    fdrecord(ENVIRON *, FDCH *), fdclose(ENVIRON *, FDCH *);
void    fdchclose(ENVIRON *, INSDS *);
#ifdef HAVE_GCC3
__attribute__ ((__format__(__printf__, 2, 3)))
  void    synterr(ENVIRON *, const char *, ...);
__attribute__ ((__noreturn__, __format__(__printf__, 2, 3)))
  void    csoundDie(void *, const char *, ...);
__attribute__ ((__format__(__printf__, 2, 3)))
  int     csoundInitError(void *, const char *, ...);
__attribute__ ((__format__(__printf__, 2, 3)))
  int     csoundPerfError(void *, const char *, ...);
__attribute__ ((__format__(__printf__, 2, 3)))
  void    csoundWarning(void *, const char *, ...);
__attribute__ ((__format__(__printf__, 2, 3)))
  void    csoundDebugMsg(void *, const char *, ...);
#else
void    synterr(ENVIRON *, const char *, ...);
void    csoundDie(void *, const char *, ...);
int     csoundInitError(void *, const char *, ...);
int     csoundPerfError(void *, const char *, ...);
void    csoundWarning(void *, const char *, ...);
void    csoundDebugMsg(void *, const char *, ...);
#endif
void    putop(ENVIRON *, TEXT *), putstrg(char *);
void    rdorchfile(ENVIRON*), otran(ENVIRON*), resetouts(ENVIRON*);
char    argtyp(ENVIRON *, char *);
TEXT    *getoptxt(ENVIRON *, int *);
int     express(ENVIRON *, char *);
int     getopnum(ENVIRON *, char *), lgexist(ENVIRON *, char *);
void    oload(ENVIRON*);
void    reverbinit(ENVIRON*);
void    dispinit(ENVIRON*);
int     init0(ENVIRON*);
int     openin(ENVIRON *, char *), openout(ENVIRON *, char *, int);
void    scsort(ENVIRON *, FILE *, FILE *);
int     scxtract(ENVIRON *, FILE *, FILE *, FILE *);
int     rdscor(ENVIRON *, EVTBLK *);
int     musmon(ENVIRON *);
void    RTLineset(ENVIRON *);
void    fgens(ENVIRON *, EVTBLK *);
FUNC    *csoundFTFind(void*, MYFLT*);
FUNC    *csoundFTFindP(void*, MYFLT*);
FUNC    *csoundFTnp2Find(void*, MYFLT*);
MYFLT   *csoundGetTable(void*, int, int*);
void    cs_beep(ENVIRON *);
MYFLT   intpow(MYFLT, long);
void    list_opcodes(ENVIRON *, int);
short   sfsampsize(int);
void    rewriteheader(SNDFILE* ofd, int verbose);
void    scoreRESET(ENVIRON *p);
int     kperf(ENVIRON*);
void    writeLine(ENVIRON *csound, const char *text, long size);
void    mainRESET(ENVIRON *);
void    create_opcodlst(void *csound);
int     readOptions(void*, FILE*);
int     csoundMain(void *csound, int argc, char **argv);
void    remove_tmpfiles(void*);
void    add_tmpfile(void*, char*);
void    xturnoff(ENVIRON*, INSDS*);
void    xturnoff_now(ENVIRON*, INSDS*);
int     insert_score_event(ENVIRON*, EVTBLK*, double, int);
MEMFIL  *ldmemfile(void*, const char*);
void    rlsmemfiles(void*);
int     delete_memfile(void*, const char*);
int     find_memfile(void*, const char*, MEMFIL**);
void    add_memfil(void*, MEMFIL*);

char    *mytmpnam(ENVIRON *, char *);

void    *SAsndgetset(void*, char*, void*, MYFLT*, MYFLT*, MYFLT*, int);
int     getsndin(void*, void*, MYFLT*, int, void*);
void    *sndgetset(void*, void*);
int     sreadin(void*, void*, MYFLT*, int, void*);

/* to be removed... */
#define printf  use_csoundMessage_instead_of_printf

extern  int     fltk_abort;

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_CSDL_H */

#endif

