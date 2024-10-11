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
#ifdef __cplusplus
extern "C" {
#endif

void    cscore_(CSOUND *);
void    *mmalloc(CSOUND *, size_t);
void    *mcalloc(CSOUND *, size_t);
void    *mrealloc(CSOUND *, void *, size_t);
void    mfree(CSOUND *, void *);
void    *mmallocDebug(CSOUND *, size_t, char*, int32_t);
void    *mcallocDebug(CSOUND *, size_t, char*, int32_t);
void    *mreallocDebug(CSOUND *, void *, size_t, char*, int32_t);
void    mfreeDebug(CSOUND *, void *, char*, int32_t);
char    *cs_strdup(CSOUND*, const char*);
char    *cs_strndup(CSOUND*, const char*, size_t);
void    csoundAuxAlloc(CSOUND *, size_t, AUXCH *), auxchfree(CSOUND *, INSDS *);
int32_t     csoundAuxAllocAsync(CSOUND *, size_t , AUXCH *,
                            AUXASYNC *, aux_cb , void *);
void    fdrecord(CSOUND *, FDCH *), csound_fd_close(CSOUND *, FDCH *);
void    fdchclose(CSOUND *, INSDS *);
CS_PRINTF2  void    synterr(CSOUND *, const char *, ...);
CS_NORETURN CS_PRINTF2  void    csoundDie(CSOUND *, const char *, ...);
CS_PRINTF2  int32_t     csoundInitError(CSOUND *, const char *, ...);
CS_PRINTF3  int32_t     csoundPerfError(CSOUND *, OPDS *h, const char *, ...);
CS_PRINTF2  void    csoundWarning(CSOUND *, const char *, ...);
CS_PRINTF2  void    csoundDebugMsg(CSOUND *, const char *, ...);
CS_PRINTF2  void    csoundErrorMsg(CSOUND *, const char *, ...);
void    csoundErrorMsgS(CSOUND *, int32_t attr, const char *, ...);
void    csoundErrMsgV(CSOUND *, const char *, const char *, va_list);
CS_NORETURN void    csoundLongJmp(CSOUND *, int32_t retval);
TEXT    *getoptxt(CSOUND *, int32_t *);
void    dispinit(CSOUND *);
int32_t     init0(CSOUND *);
void    scsort(CSOUND *, FILE *, FILE *);
char    *scsortstr(CSOUND *, CORFIL *);
int32_t     scxtract(CSOUND *, CORFIL *, FILE *);
int32_t     rdscor(CSOUND *, EVTBLK *);
int32_t     musmon(CSOUND *);
void    RTLineset(CSOUND *);
FUNC    *csoundFTFind(CSOUND *, MYFLT *);
FUNC    *csoundFTFindP(CSOUND *, MYFLT *);
FUNC    *csoundFTnp2Find(CSOUND *, MYFLT *);
FUNC    *csoundFTnp2Finde(CSOUND *, MYFLT *);
void    list_opcodes(CSOUND *, int32_t);
char    *getstrformat(int32_t format);
int32_t     sfsampsize(int32_t sf_format);
char    *type2string(int32_t type);
int32_t     type2csfiletype(int32_t type, int32_t encoding);
int32_t     sftype2csfiletype(int32_t type);
void    rewriteheader(CSOUND *csound, void *ofd);
#if 0
int32_t     readOptions_file(CSOUND *, FILE *, int32_t);
#else
int32_t     readOptions(CSOUND *, CORFIL *, int32_t);
#endif
PUBLIC int32_t     argdecode(CSOUND *, int32_t, const char **);
void    remove_tmpfiles(CSOUND *);
void    add_tmpfile(CSOUND *, char *);
void    xturnoff(CSOUND *, INSDS *);
void    xturnoff_now(CSOUND *, INSDS *);
int32_t     insert_score_event(CSOUND *, EVTBLK *, double);
//MEMFIL  *ldmemfile(CSOUND *, const char *);
//MEMFIL  *ldmemfile2(CSOUND *, const char *, int32_t);
MEMFIL  *ldmemfile2withCB(CSOUND *csound, const char *filnam, int32_t csFileType,
                          int32_t (*callback)(CSOUND*, MEMFIL*));
void    rlsmemfiles(CSOUND *);
int32_t     delete_memfile(CSOUND *, const char *);
char    *csoundTmpFileName(CSOUND *, const char *);
void    *SAsndgetset(CSOUND *, char *, void *, MYFLT *, MYFLT *, MYFLT *, int32_t);
int32_t     getsndin(CSOUND *, void *, MYFLT *, int32_t, void *);
void    *sndgetset(CSOUND *, void *);
void    dbfs_init(CSOUND *, MYFLT dbfs);
int32_t     csoundLoadExternals(CSOUND *);
SNDMEMFILE  *csoundLoadSoundFile(CSOUND *, const char *name, void *sfinfo);
int32_t     PVOCEX_LoadFile(CSOUND *, const char *fname, PVOCEX_MEMFILE *p);
void    print_opcodedir_warning(CSOUND *);
int32_t     check_rtaudio_name(char *fName, char **devName, int32_t isOutput);
int32_t     csoundLoadOpcodeDB(CSOUND *, const char *);
void    csoundDestroyOpcodeDB(CSOUND *);
int32_t     csoundCheckOpcodePluginFile(CSOUND *, const char *);
//int     csoundLoadAllPluginOpcodes(CSOUND *);
int32_t     csoundLoadAndInitModule(CSOUND *, const char *);
void    csoundNotifyFileOpened(CSOUND *, const char *, int32_t, int32_t, int32_t);
int32_t     insert_score_event_at_sample(CSOUND *, EVTBLK *, int64_t);

char *get_arg_string(CSOUND *, MYFLT);

/**
 * Register a function to be called by csoundReset(), in reverse order
 * of registration, before unloading external modules. The function takes
 * the Csound instance pointer as the first argument, and the pointer
 * passed here as 'userData' as the second, and is expected to return zero
 * on success.
 * The return value of csoundRegisterResetCallback() is zero on success.
 */
int32_t csoundRegisterResetCallback(CSOUND *, void *userData,
                                int32_t (*func)(CSOUND *, void *));

/**
 * Returns the name of the opcode of which the data structure
 * is pointed to by 'p'.
 */
char *csoundGetOpcodeName(void *p);

/**
 * Returns the number of input arguments for opcode 'p'.
 */
int32_t csoundGetInputArgCnt(void *p);


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
char *csoundGetInputArgName(void *p, int32_t n);

/**
 * Returns the number of output arguments for opcode 'p'.
 */
int32_t csoundGetOutputArgCnt(void *p);

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
char *csoundGetOutputArgName(void *p, int32_t n);

/**
 * Set release time in control periods (1 / csound->ekr second units)
 * for opcode 'p' to 'n'. If the current release time is longer than
 * the specified value, it is not changed.
 * Returns the new release time.
 */
int32_t csoundSetReleaseLength(void *p, int32_t n);

/**
 * Set release time in seconds for opcode 'p' to 'n'.
 * If the current release time is longer than the specified value,
 * it is not changed.
 * Returns the new release time in seconds.
 */
MYFLT csoundSetReleaseLengthSeconds(void *p, MYFLT n);



/**
 * Returns pointer to a string constant storing an error massage
 * for error code 'errcode'.
 */
const char *csoundExternalMidiErrorString(CSOUND *, int32_t errcode);

/**
 * Appends a list of opcodes implemented by external software to Csound's
 * internal opcode list. The list should either be terminated with an entry
 * that has a NULL opname, or the number of entries (> 0) should be specified
 * in 'n'.
 * Returns zero on success.
 */
int32_t csoundAppendOpcodes(CSOUND *, const OENTRY *opcodeList, int32_t n);

/**
 * Check system events, yielding cpu time for coopertative multitasking, etc.
 */
int32_t csoundYield(CSOUND *);

/**
 * Register utility with the specified name.
 * Returns zero on success.
 */
int32_t csoundAddUtility(CSOUND *, const char *name,
                     int32_t (*UtilFunc)(CSOUND *, int32_t, char **));

/**
 * Set description text for the specified utility.
 * Returns zero on success.
 */
int32_t csoundSetUtilityDescription(CSOUND *, const char *utilName,
                                const char *utilDesc);

/**
 * Remove all configuration variables of Csound instance 'csound',
 * and free database. This function is called by csoundReset().
 * Return value is CSOUNDCFG_SUCCESS in case of success.
 */
int32_t csoundDeleteAllConfigurationVariables(CSOUND *);

#ifdef __cplusplus
}
#endif

#ifdef  USE_DOUBLE
#define sflib_write_MYFLT  sflib_write_double
#define sflib_writef_MYFLT  sflib_writef_double
#define sflib_read_MYFLT   sflib_read_double
#define sflib_readf_MYFLT   sflib_readf_double
#else
#define sflib_write_MYFLT  sflib_write_float
#define sflib_writef_MYFLT  sflib_writef_float
#define sflib_read_MYFLT   sflib_read_float
#define sflib_readf_MYFLT   sflib_readf_float
#endif
 
#ifdef __cplusplus
extern "C" {
#endif
  int32_t sflib_command (void *handle, int32_t cmd, void *data, int32_t datasize);
  void *sflib_open(const char *path, int32_t mode, SFLIB_INFO *sfinfo);
  void *sflib_open_fd(int32_t fd, int32_t mode, SFLIB_INFO *sfinfo, int32_t close_desc);
  int32_t sflib_close(void *sndfile);
  long sflib_seek(void *handle, long frames, int32_t whence);
  long sflib_read_float(void *sndfile, float *ptr, long items);
  long sflib_readf_float(void *handle, float *ptr, long frames);
  long sflib_read_double(void *sndfile, double *ptr, long items);
  long sflib_readf_double(void *handle, double *ptr, long frames);
  long sflib_write_float(void *sndfile, float *ptr, long items);
  long sflib_writef_float(void *handle, float *ptr, long frames);
  long sflib_write_double(void *handle, double *ptr, long items);
  long sflib_writef_double(void *handle, double *ptr, long frames);
  int32_t sflib_set_string(void *sndfile, int32_t str_type, const char* str);
  const char *sflib_strerror(void *);  
#ifdef __cplusplus
}
#endif

#endif  /* __BUILDING_LIBCSOUND && !_CSOUND_PROTO_H */
