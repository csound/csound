/*
    prototyp.h:

    Copyright (C) 1991-2005 Barry Vercoe, John ffitch

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
/*  PROTOTYP.H  */
#if defined(__BUILDING_LIBCSOUND) && !defined(_CSOUND_PROTO_H)
#define _CSOUND_PROTO_H
#include <sysdep.h>
#include <sndfile.h>
#ifdef __cplusplus
extern "C" {
#endif

void    cscore_(CSOUND *);
void    *mmalloc(CSOUND *, size_t);
void    *mcalloc(CSOUND *, size_t);
void    *mrealloc(CSOUND *, void *, size_t);
void    mfree(CSOUND *, void *);
void    *mmallocDebug(CSOUND *, size_t, char*, int);
void    *mcallocDebug(CSOUND *, size_t, char*, int);
void    *mreallocDebug(CSOUND *, void *, size_t, char*, int);
void    mfreeDebug(CSOUND *, void *, char*, int);
char    *cs_strdup(CSOUND*, char*);
char    *cs_strndup(CSOUND*, char*, size_t);
void    csoundAuxAlloc(CSOUND *, size_t, AUXCH *), auxchfree(CSOUND *, INSDS *);
int     csoundAuxAllocAsync(CSOUND *, size_t , AUXCH *,
                            AUXASYNC *, aux_cb , void *);
void    fdrecord(CSOUND *, FDCH *), csound_fd_close(CSOUND *, FDCH *);
void    fdchclose(CSOUND *, INSDS *);
CS_PRINTF2  void    synterr(CSOUND *, const char *, ...);
CS_NORETURN CS_PRINTF2  void    csoundDie(CSOUND *, const char *, ...);
CS_PRINTF2  int     csoundInitError(CSOUND *, const char *, ...);
CS_PRINTF3  int     csoundPerfError(CSOUND *, OPDS *h, const char *, ...);
CS_PRINTF2  void    csoundWarning(CSOUND *, const char *, ...);
CS_PRINTF2  void    csoundDebugMsg(CSOUND *, const char *, ...);
CS_PRINTF2  void    csoundErrorMsg(CSOUND *, const char *, ...);
void    csoundErrorMsgS(CSOUND *, int attr, const char *, ...);
void    csoundErrMsgV(CSOUND *, const char *, const char *, va_list);
CS_NORETURN void    csoundLongJmp(CSOUND *, int retval);
TEXT    *getoptxt(CSOUND *, int *);
void    reverbinit(CSOUND *);
void    dispinit(CSOUND *);
int     init0(CSOUND *);
void    scsort(CSOUND *, FILE *, FILE *);
char    *scsortstr(CSOUND *, CORFIL *);
int     scxtract(CSOUND *, CORFIL *, FILE *);
int     rdscor(CSOUND *, EVTBLK *);
int     musmon(CSOUND *);
void    RTLineset(CSOUND *);
FUNC    *csoundFTFind(CSOUND *, MYFLT *);
FUNC    *csoundFTFindP(CSOUND *, MYFLT *);
FUNC    *csoundFTnp2Find(CSOUND *, MYFLT *);
FUNC    *csoundFTnp2Finde(CSOUND *, MYFLT *);
MYFLT   intpow(MYFLT, int32);
void    list_opcodes(CSOUND *, int);
char    *getstrformat(int format);
int     sfsampsize(int sf_format);
char    *type2string(int type);
int     type2csfiletype(int type, int encoding);
int     sftype2csfiletype(int type);
void    rewriteheader(void *ofd);
#if 0
int     readOptions_file(CSOUND *, FILE *, int);
#else
int     readOptions(CSOUND *, CORFIL *, int);
#endif
PUBLIC int     argdecode(CSOUND *, int, const char **);
void    remove_tmpfiles(CSOUND *);
void    add_tmpfile(CSOUND *, char *);
void    xturnoff(CSOUND *, INSDS *);
void    xturnoff_now(CSOUND *, INSDS *);
int     insert_score_event(CSOUND *, EVTBLK *, double);
//MEMFIL  *ldmemfile(CSOUND *, const char *);
//MEMFIL  *ldmemfile2(CSOUND *, const char *, int);
MEMFIL  *ldmemfile2withCB(CSOUND *csound, const char *filnam, int csFileType,
                          int (*callback)(CSOUND*, MEMFIL*));
void    rlsmemfiles(CSOUND *);
int     delete_memfile(CSOUND *, const char *);
char    *csoundTmpFileName(CSOUND *, const char *);
void    *SAsndgetset(CSOUND *, char *, void *, MYFLT *, MYFLT *, MYFLT *, int);
int     getsndin(CSOUND *, void *, MYFLT *, int, void *);
void    *sndgetset(CSOUND *, void *);
void    dbfs_init(CSOUND *, MYFLT dbfs);
int     csoundLoadExternals(CSOUND *);
SNDMEMFILE  *csoundLoadSoundFile(CSOUND *, const char *name, void *sfinfo);
int     PVOCEX_LoadFile(CSOUND *, const char *fname, PVOCEX_MEMFILE *p);
void    print_opcodedir_warning(CSOUND *);
int     check_rtaudio_name(char *fName, char **devName, int isOutput);
int     csoundLoadOpcodeDB(CSOUND *, const char *);
void    csoundDestroyOpcodeDB(CSOUND *);
int     csoundCheckOpcodePluginFile(CSOUND *, const char *);
//int     csoundLoadAllPluginOpcodes(CSOUND *);
int     csoundLoadAndInitModule(CSOUND *, const char *);
void    csoundNotifyFileOpened(CSOUND *, const char *, int, int, int);
int     insert_score_event_at_sample(CSOUND *, EVTBLK *, int64_t);

char *get_arg_string(CSOUND *, MYFLT);

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


/** Returns the CS_TYPE for an opcode's arg pointer */

CS_TYPE* csoundGetTypeForArg(void* argPtr);

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
 * Returns pointer to a string constant storing an error massage
 * for error code 'errcode'.
 */
const char *csoundExternalMidiErrorString(CSOUND *, int errcode);

/**
 * Appends a list of opcodes implemented by external software to Csound's
 * internal opcode list. The list should either be terminated with an entry
 * that has a NULL opname, or the number of entries (> 0) should be specified
 * in 'n'.
 * Returns zero on success.
 */
int csoundAppendOpcodes(CSOUND *, const OENTRY *opcodeList, int n);

/**
 * Check system events, yielding cpu time for coopertative multitasking, etc.
 */
int csoundYield(CSOUND *);

/**
 * Register utility with the specified name.
 * Returns zero on success.
 */
int csoundAddUtility(CSOUND *, const char *name,
                     int (*UtilFunc)(CSOUND *, int, char **));

/**
 * Set description text for the specified utility.
 * Returns zero on success.
 */
int csoundSetUtilityDescription(CSOUND *, const char *utilName,
                                const char *utilDesc);

/**
 * Remove all configuration variables of Csound instance 'csound',
 * and free database. This function is called by csoundReset().
 * Return value is CSOUNDCFG_SUCCESS in case of success.
 */
int csoundDeleteAllConfigurationVariables(CSOUND *);

#ifdef __cplusplus
}
#endif

#endif  /* __BUILDING_LIBCSOUND && !_CSOUND_PROTO_H */
