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
#if defined(__BUILDING_LIBCSOUND) && !defined(_CSOUND_PROTO_H)
#define _CSOUND_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

void    cscorinit(CSOUND *), cscore(CSOUND *);
void    *mmalloc(CSOUND *, size_t);
void    *mcalloc(CSOUND *, size_t);
void    *mrealloc(CSOUND *, void*, size_t), mfree(CSOUND *, void*);
void    csoundAuxAlloc(CSOUND *, long, AUXCH *), auxchfree(CSOUND *, INSDS *);
void    fdrecord(CSOUND *, FDCH *), fdclose(CSOUND *, FDCH *);
void    fdchclose(CSOUND *, INSDS *);
CS_PRINTF2  void    synterr(CSOUND *, const char *, ...);
CS_NORETURN CS_PRINTF2  void    csoundDie(CSOUND *, const char *, ...);
CS_PRINTF2  int     csoundInitError(CSOUND *, const char *, ...);
CS_PRINTF2  int     csoundPerfError(CSOUND *, const char *, ...);
CS_PRINTF2  void    csoundWarning(CSOUND *, const char *, ...);
CS_PRINTF2  void    csoundDebugMsg(CSOUND *, const char *, ...);
CS_PRINTF2  void    csoundErrorMsg(CSOUND *, const char *, ...);
void    csoundErrMsgV(CSOUND *, const char *, const char *, va_list);
CS_NORETURN void    csoundLongJmp(CSOUND *, int retval);
void    putop(CSOUND *, TEXT *), putstrg(char *);
void    rdorchfile(CSOUND *), otran(CSOUND *);
char    argtyp(CSOUND *, char *);
TEXT    *getoptxt(CSOUND *, int *);
int     express(CSOUND *, char *);
int     getopnum(CSOUND *, char *), lgexist(CSOUND *, const char *);
void    oload(CSOUND *);
void    reverbinit(CSOUND *);
void    dispinit(CSOUND *);
int     init0(CSOUND *);
PUBLIC  void    scsort(CSOUND *, FILE *, FILE *);
PUBLIC  int     scxtract(CSOUND *, FILE *, FILE *, FILE *);
int     rdscor(CSOUND *, EVTBLK *);
int     musmon(CSOUND *);
void    RTLineset(CSOUND *);
FUNC    *csoundFTFind(CSOUND *, MYFLT*);
FUNC    *csoundFTFindP(CSOUND *, MYFLT*);
FUNC    *csoundFTnp2Find(CSOUND *, MYFLT*);
MYFLT   *csoundGetTable(CSOUND *, int, int*);
void    cs_beep(CSOUND *);
MYFLT   intpow(MYFLT, long);
void    list_opcodes(CSOUND *, int);
int     sfsampsize(int);
void    rewriteheader(SNDFILE* ofd, int verbose);
void    writeLine(CSOUND *, const char *text, long size);
int     readOptions(CSOUND *, FILE*);
void    remove_tmpfiles(CSOUND *);
void    add_tmpfile(CSOUND *, char*);
void    xturnoff(CSOUND *, INSDS*);
void    xturnoff_now(CSOUND *, INSDS*);
int     insert_score_event(CSOUND *, EVTBLK *, double);
MEMFIL  *ldmemfile(CSOUND *, const char*);
void    rlsmemfiles(CSOUND *);
int     delete_memfile(CSOUND *, const char*);
char    *mytmpnam(CSOUND *, char *);
void    *SAsndgetset(CSOUND *, char*, void*, MYFLT*, MYFLT*, MYFLT*, int);
int     getsndin(CSOUND *, void*, MYFLT*, int, void*);
void    *sndgetset(CSOUND *, void*);
void    dbfs_init(CSOUND *, MYFLT dbfs);
SNDMEMFILE  *csoundLoadSoundFile(CSOUND *, const char *name, SF_INFO *sfinfo);
int         PVOCEX_LoadFile(CSOUND *, const char *fname, PVOCEX_MEMFILE *p);

  /**
   * Register a function to be called at note deactivation.
   * Should be called from the initialisation routine of an opcode.
   * 'p' is a pointer to the OPDS structure of the opcode, and 'func'
   * is the function to be called, with the same arguments and return
   * value as in the case of opcode init/perf functions.
   * The functions are called in reverse order of registration.
   * Returns zero on success.
   */
  int csoundRegisterDeinitCallback(CSOUND *, void *p,
                                   int (*func)(CSOUND *, void *));

  /**
   * Register a function to be called by csoundReset(), in reverse order
   * of registration, before unloading external modules. The function takes
   * the Csound instance pointer as the first argument, and the pointer
   * passed here as 'userData' as the second, and is expected to return zero
   * on success.
   * The return value of csoundRegisterResetCallback() is zero on success.
   */
  int csoundRegisterResetCallback(CSOUND *, void *userData,
                                  int (*func)(CSOUND *, void *));

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

  /**
   * Open MIDI input device 'devName', and store stream specific
   * data pointer in *userData. Return value is zero on success,
   * and a non-zero error code if an error occured.
   */
  int csoundExternalMidiInOpen(CSOUND *, void **userData, const char *devName);

  /**
   * Read at most 'nbytes' bytes of MIDI data from input stream
   * 'userData', and store in 'buf'. Returns the actual number of
   * bytes read, which may be zero if there were no events, and
   * negative in case of an error. Note: incomplete messages (such
   * as a note on status without the data bytes) should not be
   * returned.
   */
  int csoundExternalMidiRead(CSOUND *,
                             void *userData, unsigned char *buf, int nbytes);

  /**
   * Close MIDI input device associated with 'userData'.
   * Return value is zero on success, and a non-zero error
   * code on failure.
   */
  int csoundExternalMidiInClose(CSOUND *, void *userData);

  /**
   * Open MIDI output device 'devName', and store stream specific
   * data pointer in *userData. Return value is zero on success,
   * and a non-zero error code if an error occured.
   */
  int csoundExternalMidiOutOpen(CSOUND *,
                                void **userData, const char *devName);

  /**
   * Write 'nbytes' bytes of MIDI data to output stream 'userData'
   * from 'buf' (the buffer will not contain incomplete messages).
   * Returns the actual number of bytes written, or a negative
   * error code.
   */
  int csoundExternalMidiWrite(CSOUND *,
                              void *userData, unsigned char *buf, int nbytes);

  /**
   * Close MIDI output device associated with '*userData'.
   * Return value is zero on success, and a non-zero error
   * code on failure.
   */
  int csoundExternalMidiOutClose(CSOUND *, void *userData);

  /**
   * Returns pointer to a string constant storing an error massage
   * for error code 'errcode'.
   */
  char *csoundExternalMidiErrorString(CSOUND *, int errcode);

#ifdef __cplusplus
};
#endif

#endif

