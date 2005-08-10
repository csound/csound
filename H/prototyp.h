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
                                                        /*  PROTOTYP.H  */
#ifndef _CSOUND_PROTO_H
#define _CSOUND_PROTO_H

#if defined(__BUILDING_LIBCSOUND) || defined(CSOUND_CSDL_H)
#  define IGN(X)  (void) X
/* to be removed... */
#  define printf  use_csoundMessage_instead_of_printf
#endif

#ifdef __BUILDING_LIBCSOUND

#ifdef __cplusplus
extern "C" {
#endif

void    cscorinit(ENVIRON *), cscore(ENVIRON *);
void    *mmalloc(ENVIRON *, size_t);
void    *mcalloc(ENVIRON *, size_t);
void    *mrealloc(ENVIRON *, void*, size_t), mfree(ENVIRON *, void*);
void    csoundAuxAlloc(ENVIRON *, long, AUXCH *), auxchfree(ENVIRON *, INSDS *);
void    fdrecord(ENVIRON *, FDCH *), fdclose(ENVIRON *, FDCH *);
void    fdchclose(ENVIRON *, INSDS *);
CS_PRINTF2  void    synterr(ENVIRON *, const char *, ...);
CS_NORETURN CS_PRINTF2  void    csoundDie(ENVIRON *, const char *, ...);
CS_PRINTF2  int     csoundInitError(ENVIRON *, const char *, ...);
CS_PRINTF2  int     csoundPerfError(ENVIRON *, const char *, ...);
CS_PRINTF2  void    csoundWarning(ENVIRON *, const char *, ...);
CS_PRINTF2  void    csoundDebugMsg(ENVIRON *, const char *, ...);
CS_PRINTF2  void    csoundErrorMsg(ENVIRON *, const char *, ...);
void    csoundErrMsgV(ENVIRON *, const char *, const char *, va_list);
CS_NORETURN void    csoundLongJmp(ENVIRON *, int retval);
void    putop(ENVIRON *, TEXT *), putstrg(char *);
void    rdorchfile(ENVIRON *), otran(ENVIRON *);
char    argtyp(ENVIRON *, char *);
TEXT    *getoptxt(ENVIRON *, int *);
int     express(ENVIRON *, char *);
int     getopnum(ENVIRON *, char *), lgexist(ENVIRON *, const char *);
void    oload(ENVIRON *);
void    reverbinit(ENVIRON *);
void    dispinit(ENVIRON *);
int     init0(ENVIRON *);
PUBLIC  void    scsort(ENVIRON *, FILE *, FILE *);
PUBLIC  int     scxtract(ENVIRON *, FILE *, FILE *, FILE *);
int     rdscor(ENVIRON *, EVTBLK *);
int     musmon(ENVIRON *);
void    RTLineset(ENVIRON *);
FUNC    *csoundFTFind(ENVIRON *, MYFLT*);
FUNC    *csoundFTFindP(ENVIRON *, MYFLT*);
FUNC    *csoundFTnp2Find(ENVIRON *, MYFLT*);
MYFLT   *csoundGetTable(ENVIRON *, int, int*);
void    cs_beep(ENVIRON *);
MYFLT   intpow(MYFLT, long);
void    list_opcodes(ENVIRON *, int);
int     sfsampsize(int);
void    rewriteheader(SNDFILE* ofd, int verbose);
void    writeLine(ENVIRON *, const char *text, long size);
int     readOptions(ENVIRON *, FILE*);
void    remove_tmpfiles(ENVIRON *);
void    add_tmpfile(ENVIRON *, char*);
void    xturnoff(ENVIRON *, INSDS*);
void    xturnoff_now(ENVIRON *, INSDS*);
int     insert_score_event(ENVIRON *, EVTBLK*, double, int);
MEMFIL  *ldmemfile(ENVIRON *, const char*);
void    rlsmemfiles(ENVIRON *);
int     delete_memfile(ENVIRON *, const char*);
char    *mytmpnam(ENVIRON *, char *);
void    *SAsndgetset(ENVIRON *, char*, void*, MYFLT*, MYFLT*, MYFLT*, int);
int     getsndin(ENVIRON *, void*, MYFLT*, int, void*);
void    *sndgetset(ENVIRON *, void*);
SNDMEMFILE  *csoundLoadSoundFile(ENVIRON *,
                                 const char *name, SF_INFO *sfinfo);
int     PVOCEX_LoadFile(ENVIRON *, const char *fname, PVOCEX_MEMFILE *p);

  /**
   * Returns the name of the opcode of which the data structure
   * is pointed to by 'p'.
   */
  char *csoundGetOpcodeName(void *p);

  /**
   * Returns the number of input arguments for opcode 'p'.
   */
  int csoundGetInputArgCnt(void *p);

  /**
   * Returns a binary value of which bit 0 is set if the first input
   * argument is a-rate, bit 1 is set if the second input argument is
   * a-rate, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetInputArgAMask(void *p);

  /**
   * Returns a binary value of which bit 0 is set if the first input
   * argument is a string, bit 1 is set if the second input argument is
   * a string, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetInputArgSMask(void *p);

  /**
   * Returns the name of input argument 'n' (counting from 0) for opcode 'p'.
   */
  char *csoundGetInputArgName(void *p, int n);

  /**
   * Returns the number of output arguments for opcode 'p'.
   */
  int csoundGetOutputArgCnt(void *p);

  /**
   * Returns a binary value of which bit 0 is set if the first output
   * argument is a-rate, bit 1 is set if the second output argument is
   * a-rate, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetOutputArgAMask(void *p);

  /**
   * Returns a binary value of which bit 0 is set if the first output
   * argument is a string, bit 1 is set if the second output argument is
   * a string, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetOutputArgSMask(void *p);

  /**
   * Returns the name of output argument 'n' (counting from 0) for opcode 'p'.
   */
  char *csoundGetOutputArgName(void *p, int n);

  /**
   * Set release time in control periods (1 / csound->ekr second units)
   * for opcode 'p' to 'n'. If the current release time is longer than
   * the specified value, it is not changed.
   * Returns the new release time.
   */
  int csoundSetReleaseLength(void *p, int n);

  /**
   * Set release time in seconds for opcode 'p' to 'n'.
   * If the current release time is longer than the specified value,
   * it is not changed.
   * Returns the new release time in seconds.
   */
  MYFLT csoundSetReleaseLengthSeconds(void *p, MYFLT n);

  /**
   * Returns MIDI channel number (0 to 15) for the instrument instance
   * that called opcode 'p'.
   * In the case of score notes, -1 is returned.
   */
  int csoundGetMidiChannelNumber(void *p);

  /**
   * Returns a pointer to the MIDI channel structure for the instrument
   * instance that called opcode 'p'.
   * In the case of score notes, NULL is returned.
   */
  MCHNBLK *csoundGetMidiChannel(void *p);

  /**
   * Returns MIDI note number (in the range 0 to 127) for opcode 'p'.
   * If the opcode was not called from a MIDI activated instrument
   * instance, the return value is undefined.
   */
  int csoundGetMidiNoteNumber(void *p);

  /**
   * Returns MIDI velocity (in the range 0 to 127) for opcode 'p'.
   * If the opcode was not called from a MIDI activated instrument
   * instance, the return value is undefined.
   */
  int csoundGetMidiVelocity(void *p);

  /**
   * Returns non-zero if the current note (owning opcode 'p') is releasing.
   */
  int csoundGetReleaseFlag(void *p);

  /**
   * Returns the note-off time in seconds (measured from the beginning of
   * performance) of the current instrument instance, from which opcode 'p'
   * was called. The return value may be negative if the note has indefinite
   * duration.
   */
  double csoundGetOffTime(void *p);

  /**
   * Returns the array of p-fields passed to the instrument instance
   * that owns opcode 'p', starting from p0. Only p1, p2, and p3 are
   * guaranteed to be available. p2 is measured in seconds from the
   * beginning of the current section.
   */
  MYFLT *csoundGetPFields(void *p);

  /**
   * Returns the instrument number (p1) for opcode 'p'.
   */
  int csoundGetInstrumentNumber(void *p);

#ifdef __cplusplus
};
#endif

#endif      /* __BUILDING_LIBCSOUND */

#endif

