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
#ifndef __PROTO_H
#include <stdlib.h>
#define IGN(X) X = X

#ifdef __cplusplus
extern "C" {
#endif

int hetro(int argc, char **argv), lpanal(int argc, char **argv);
int pvanal(int argc, char **argv), cvanal(int argc, char **argv);
int sndinfo(int argc, char **argv), pvlook(int argc, char **argv);
int dnoise(int argc, char **argv);
void dnoise_usage(int);
void cscorinit(void), cscore(ENVIRON*);
void *mmalloc(void*, size_t), *mcalloc(void*, size_t);
void *mrealloc(void*, void*, size_t), mfree(void*, void*);
void auxalloc(void*, long, AUXCH *), auxchfree(void*, INSDS *);
void fdrecord(FDCH *), fdclose(FDCH *), fdchclose(INSDS *);
void warning(char *);
int  initerror(char *), perferror(char *);
void synterr(char *), synterrp(char *, char *);
void die(char *), dies(char *, char *);
void putop(TEXT *), putstrg(char *);
void rdorchfile(void), otran(void), resetouts(void);
char argtyp(char *);
TEXT *getoptxt(int *);
int  express(char *), getopnum(char *), pnum(char *), lgexist(char *);
void oload(ENVIRON*);
void cpsoctinit(void), reverbinit(void);
void dispinit(void);
void sssfinit(void);
int  init0(ENVIRON*);
INSDS *instance(int);
int  isfullpath(char *), dispexit(void);
char *catpath(char *, char *);
int  openin(char *), openout(char *, int);
void scsort(FILE *, FILE *);
int  scxtract(FILE *, FILE *, FILE *);
int  rdscor(EVTBLK *);
int  musmon(ENVIRON*);
void RTLineset(void);
int  sensLine(void);
void fgens(ENVIRON *, EVTBLK *);
FUNC *ftnp2find(ENVIRON *,MYFLT *), *ftfindp(ENVIRON *,MYFLT *);
void beep(void), rlsmemfiles(void);
MYFLT intpow(MYFLT, long);
void list_opcodes(int);
short sfsampsize(int);
void rewriteheader(SNDFILE* ofd, int verbose);
char *unquote(char *);
void scoreRESET(ENVIRON *p);
void kperf(ENVIRON*);
void writeLine(const char *text, long size);
void newevent(void *, char type, MYFLT *pfields, long count);
void csoundDefaultMidiOpen(void*);
void mainRESET(ENVIRON *);
void create_opcodlst(void *csound);
int readOptions(void*, FILE*);
int csoundMain(void *csound, int argc, char **argv);
void remove_tmpfiles(void*);
void add_tmpfile(void*, char*);

extern OPARMS O;
extern ENVIRON cenviron;
extern int fltk_abort;
extern MYFLT *inbuf;
extern MYFLT *outbuf;

#ifdef __cplusplus
}
#endif

#define __PROTO_H
#endif
