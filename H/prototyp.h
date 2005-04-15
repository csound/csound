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

int     hetro(int argc, char **argv), lpanal(int argc, char **argv);
int     pvanal(int argc, char **argv), cvanal(int argc, char **argv);
int     sndinfo(int argc, char **argv), pvlook(int argc, char **argv);
int     dnoise(int argc, char **argv);
void    dnoise_usage(int);
void    cscorinit(void), cscore(ENVIRON*);
void    *mmalloc(void*, size_t), *mcalloc(void*, size_t);
void    *mrealloc(void*, void*, size_t), mfree(void*, void*);
void    csoundAuxAlloc(void*, long, AUXCH *), auxchfree(void*, INSDS *);
void    fdrecord(FDCH *), fdclose(FDCH *), fdchclose(INSDS *);
int     csoundInitError(void *, const char *, ...);
int     csoundPerfError(void *, const char *, ...);
void    synterr(char *), synterrp(char *, char *);
void    csoundDie(void *, const char *, ...);
void    csoundWarning(void *, const char *, ...);
void    csoundDebugMsg(void *, const char *, ...);
void    putop(TEXT *), putstrg(char *);
void    rdorchfile(ENVIRON*), otran(ENVIRON*), resetouts(ENVIRON*);
char    argtyp(char *);
TEXT    *getoptxt(int *);
int     express(ENVIRON *, char *);
int     getopnum(char *), pnum(char *), lgexist(char *);
void    oload(ENVIRON*);
void    cpsoctinit(ENVIRON*), reverbinit(void);
void    dispinit(void);
void    sssfinit(void);
int     init0(ENVIRON*);
INSDS   *instance(int);
int     openin(char *), openout(char *, int);
void    scsort(FILE *, FILE *);
int     scxtract(FILE *, FILE *, FILE *);
int     rdscor(EVTBLK *);
int     musmon(ENVIRON*);
void    RTLineset(void);
int     sensLine(ENVIRON *);
void    fgens(ENVIRON *, EVTBLK *);
FUNC    *csoundFTFind(void*, MYFLT*);
FUNC    *csoundFTFindP(void*, MYFLT*);
FUNC    *csoundFTnp2Find(void*, MYFLT*);
MYFLT   *csoundGetTable(void*, int, int*);
void    cs_beep(ENVIRON *);
MYFLT   intpow(MYFLT, long);
void    list_opcodes(int);
short   sfsampsize(int);
void    rewriteheader(SNDFILE* ofd, int verbose);
char    *unquote(char *);
void    scoreRESET(ENVIRON *p);
void    kperf(ENVIRON*);
void    writeLine(const char *text, long size);
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
void    err_printf(char *, ...);

#ifdef WIN32
#define tmpnam mytmpnam
char    *mytmpnam(char *);
#endif

void    *SAsndgetset(void*, char*, void*, MYFLT*, MYFLT*, MYFLT*, int);
int     getsndin(void*, void*, MYFLT*, int, void*);
void    *sndgetset(void*, void*);
int     sreadin(void*, void*, MYFLT*, int, void*);

void csoundPrintf(const char *format, ...);

extern  OPARMS  O;
extern  ENVIRON cenviron;
extern  int     fltk_abort;
extern  MYFLT   *inbuf;
extern  MYFLT   *outbuf;

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_CSDL_H */

#endif

