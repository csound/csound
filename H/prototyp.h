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

#define IGN(X) X = X
/* to be removed... */
#define printf  use_csoundMessage_instead_of_printf

#ifdef __BUILDING_LIBCSOUND

#ifdef __cplusplus
extern "C" {
#endif

void    cscorinit(ENVIRON*), cscore(ENVIRON*);
void    *mmalloc(void*, size_t);
void    *mcalloc(void*, size_t);
void    *mrealloc(void*, void*, size_t), mfree(void*, void*);
void    csoundAuxAlloc(void*, long, AUXCH *), auxchfree(void*, INSDS *);
void    fdrecord(ENVIRON *, FDCH *), fdclose(ENVIRON *, FDCH *);
void    fdchclose(ENVIRON *, INSDS *);
CS_PRINTF2  void    synterr(ENVIRON *, const char *, ...);
CS_NORETURN CS_PRINTF2  void    csoundDie(void *, const char *, ...);
CS_PRINTF2  int     csoundInitError(void *, const char *, ...);
CS_PRINTF2  int     csoundPerfError(void *, const char *, ...);
CS_PRINTF2  void    csoundWarning(void *, const char *, ...);
CS_PRINTF2  void    csoundDebugMsg(void *, const char *, ...);
CS_PRINTF2  void    csoundErrorMsg(void *, const char *, ...);
            void    csoundErrMsgV(void *, const char *, const char *, va_list);
CS_NORETURN void    csoundLongJmp(ENVIRON *csound, int retval);
void    putop(ENVIRON *, TEXT *), putstrg(char *);
void    rdorchfile(ENVIRON*), otran(ENVIRON*);
char    argtyp(ENVIRON *, char *);
TEXT    *getoptxt(ENVIRON *, int *);
int     express(ENVIRON *, char *);
int     getopnum(ENVIRON *, char *), lgexist(ENVIRON *, const char *);
void    oload(ENVIRON*);
void    reverbinit(ENVIRON*);
void    dispinit(ENVIRON*);
int     init0(ENVIRON*);
PUBLIC  void    scsort(ENVIRON *, FILE *, FILE *);
PUBLIC  int     scxtract(ENVIRON *, FILE *, FILE *, FILE *);
int     rdscor(ENVIRON *, EVTBLK *);
int     musmon(ENVIRON *);
void    RTLineset(ENVIRON *);
FUNC    *csoundFTFind(void*, MYFLT*);
FUNC    *csoundFTFindP(void*, MYFLT*);
FUNC    *csoundFTnp2Find(void*, MYFLT*);
MYFLT   *csoundGetTable(void*, int, int*);
void    cs_beep(ENVIRON *);
MYFLT   intpow(MYFLT, long);
void    list_opcodes(ENVIRON *, int);
int     sfsampsize(int);
void    rewriteheader(SNDFILE* ofd, int verbose);
void    writeLine(ENVIRON *csound, const char *text, long size);
int     readOptions(void*, FILE*);
void    remove_tmpfiles(void*);
void    add_tmpfile(void*, char*);
void    xturnoff(ENVIRON*, INSDS*);
void    xturnoff_now(ENVIRON*, INSDS*);
int     insert_score_event(ENVIRON*, EVTBLK*, double, int);
MEMFIL  *ldmemfile(void*, const char*);
void    rlsmemfiles(void*);
int     delete_memfile(void*, const char*);
char    *mytmpnam(ENVIRON *, char *);
void    *SAsndgetset(void*, char*, void*, MYFLT*, MYFLT*, MYFLT*, int);
int     getsndin(void*, void*, MYFLT*, int, void*);
void    *sndgetset(void*, void*);
PUBLIC  SNDMEMFILE  *csoundLoadSoundFile(void *csound,
                                         const char *name, SF_INFO *sfinfo);
int     PVOCEX_LoadFile(ENVIRON *csound, const char *fname, PVOCEX_MEMFILE *p);
MCHNBLK *csoundGetMidiChannel(void *p);

#ifdef __cplusplus
};
#endif

#endif      /* __BUILDING_LIBCSOUND */

#endif

