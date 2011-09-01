#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <dlfcn.h>
#include <stdarg.h>
#include "H/sysdep.h"

typedef struct CSOUND_  CSOUND;
#ifndef MYFLT
#define MYFLT float
#endif
#define csCfgVariable_t int
typedef int (*SUBR)(CSOUND *, void *);

typedef struct oentry {
    char    *opname;
    unsigned short  dsblksiz;
    unsigned short  thread;
    char    *outypes;
    char    *intypes;
    int     (*iopadr)(void *, void *p);
    int     (*kopadr)(void *, void *p);
    int     (*aopadr)(void *, void *p);
    void    *useropinfo;    /* user opcode parameters */
    int     prvnum;         /* IV - Oct 31 2002 */
} OENTRY;

  typedef struct {
    int     odebug;
    int     sfread, sfwrite, sfheader, filetyp;
    int     inbufsamps, outbufsamps;
    int     informat, outformat;
    int     sfsampsize;
    int     displays, graphsoff, postscript, msglevel;
    /* int     Beatmode, cmdTempo, oMaxLag; */
    /* int     usingcscore, Linein; */
    /* int     RTevents, Midiin, FMidiin, RMidiin; */
    /* int     ringbell, termifend; */
    /* int     rewrt_hdr, heartbeat, gen01defer; */
    /* int     expr_opt;       /\* IV - Jan 27 2005: for --expression-opt *\/ */
    /* float   sr_override, kr_override; */
    /* char    *infilename, *outfilename, *playscore; */
    /* char    *Linename, *Midiname, *FMidiname; */
    /* char    *Midioutname;   /\* jjk 09252000 - MIDI output device, -Q option *\/ */
    /* char    *FMidioutname; */
    /* int     midiKey, midiKeyCps, midiKeyOct, midiKeyPch; */
    /* int     midiVelocity, midiVelocityAmp; */
    /* int     noDefaultPaths;  /\* syy - Oct 25, 2006: for disabling relative paths */
    /*                           from files *\/ */
    /* int     numThreads; */
     /* int     syntaxCheckOnly; */
    /* int     useCsdLineCounts; */
  } OPARMS;

struct CSOUND_ {
    /* Csound API function pointers (320 total) */
    int (*GetVersion)(void);
    int (*GetAPIVersion)(void);
    void *(*GetHostData)(CSOUND *);
    void (*SetHostData)(CSOUND *, void *hostData);
    CSOUND *(*Create)(void *hostData);
    int (*Compile)(CSOUND *, int argc, char **argv);
    int (*Perform)(CSOUND *);
    int (*PerformKsmps)(CSOUND *);
    int (*PerformBuffer)(CSOUND *);
    int (*Cleanup)(CSOUND *);
    void (*Reset)(CSOUND *);
    void (*Destroy)(CSOUND *);
    MYFLT (*GetSr)(CSOUND *);
    MYFLT (*GetKr)(CSOUND *);
    int (*GetKsmps)(CSOUND *);
    int (*GetNchnls)(CSOUND *);
    int (*GetSampleFormat)(CSOUND *);
    int (*GetSampleSize)(CSOUND *);
    long (*GetInputBufferSize)(CSOUND *);
    long (*GetOutputBufferSize)(CSOUND *);
    MYFLT *(*GetInputBuffer)(CSOUND *);
    MYFLT *(*GetOutputBuffer)(CSOUND *);
    MYFLT *(*GetSpin)(CSOUND *);
    MYFLT *(*GetSpout)(CSOUND *);
    double (*GetScoreTime)(CSOUND *);
    void (*SetMakeXYinCallback)(CSOUND *);
    void (*SetReadXYinCallback)(CSOUND *);
    void (*SetKillXYinCallback)(CSOUND * );
    int (*IsScorePending)(CSOUND *);
    void (*SetScorePending)(CSOUND *, int pending);
    MYFLT (*GetScoreOffsetSeconds)(CSOUND *);
    void (*SetScoreOffsetSeconds)(CSOUND *, MYFLT offset);
    void (*RewindScore)(CSOUND *);
    void (*Message)(CSOUND *, const char *fmt, ...);
    void (*MessageS)(CSOUND *, int attr, const char *fmt, ...);
    void (*MessageV)(CSOUND *, int attr, const char *format, va_list args);
    void (*DeleteUtilityList)(CSOUND *, char **lst);
  void (*DeleteChannelList)(CSOUND * /* , CsoundChannelListEntry *lst*/);
    void (*SetMessageCallback)(CSOUND *,
                void (*csoundMessageCallback)(CSOUND *,
                                              int attr, const char *format,
                                              va_list valist));
    void (*DeleteCfgVarList)(csCfgVariable_t **lst);
    int (*GetMessageLevel)(CSOUND *);
    void (*SetMessageLevel)(CSOUND *, int messageLevel);
    void (*InputMessage)(CSOUND *, const char *message__);
    void (*KeyPress)(CSOUND *, char c__);
    void (*SetInputValueCallback)(CSOUND *,
                void (*inputValueCalback)(CSOUND *, const char *channelName,
                                                    MYFLT *value));
    void (*SetOutputValueCallback)(CSOUND *,
                void (*outputValueCalback)(CSOUND *, const char *channelName,
                                                     MYFLT value));
    int (*ScoreEvent)(CSOUND *,
                      char type, const MYFLT *pFields, long numFields);
    void (*SetExternalMidiInOpenCallback)(CSOUND *,
                int (*func)(CSOUND *, void **, const char *));
    void (*SetExternalMidiReadCallback)(CSOUND *,
                int (*func)(CSOUND *, void *, unsigned char *, int));
    void (*SetExternalMidiInCloseCallback)(CSOUND *,
                int (*func)(CSOUND *, void *));
    void (*SetExternalMidiOutOpenCallback)(CSOUND *,
                int (*func)(CSOUND *, void **, const char *));
    void (*SetExternalMidiWriteCallback)(CSOUND *,
                int (*func)(CSOUND *, void *, const unsigned char *, int));
    void (*SetExternalMidiOutCloseCallback)(CSOUND *,
                int (*func)(CSOUND *, void *));
    void (*SetExternalMidiErrorStringCallback)(CSOUND *,
                const char *(*func)(int));
    int (*SetIsGraphable)(CSOUND *, int isGraphable);
    void (*SetMakeGraphCallback)(CSOUND *,
                void (*makeGraphCallback)(CSOUND * /*, WINDAT *p,
                                                     const char *name*/));
    void (*SetDrawGraphCallback)(CSOUND *,
                                 void (*drawGraphCallback)(CSOUND * /* , WINDAT *p */));
    void (*SetKillGraphCallback)(CSOUND *,
                                 void (*killGraphCallback)(CSOUND * /* , WINDAT *p */));
    void (*SetExitGraphCallback)(CSOUND *, int (*exitGraphCallback)(CSOUND *));
  int (*NewOpcodeList)(CSOUND * /* , opcodeListEntry ** */);
    void (*DisposeOpcodeList)(CSOUND * /* , opcodeListEntry * */);
    int (*AppendOpcode)(CSOUND *, const char *opname, int dsblksiz,
                        int thread, const char *outypes, const char *intypes,
                        int (*iopadr)(CSOUND *, void *),
                        int (*kopadr)(CSOUND *, void *),
                        int (*aopadr)(CSOUND *, void *));
    int (*AppendOpcodes)(CSOUND *, const OENTRY *opcodeList, int n);
    int (*OpenLibrary)(void **library, const char *libraryPath);
    int (*CloseLibrary)(void *library);
    void *(*GetLibrarySymbol)(void *library, const char *procedureName);
    int (*CheckEvents)(CSOUND *);
    void (*SetYieldCallback)(CSOUND *, int (*yieldCallback)(CSOUND *));
    const char *(*GetEnv)(CSOUND *, const char *name);
    char *(*FindInputFile)(CSOUND *, const char *filename, const char *envList);
    char *(*FindOutputFile)(CSOUND *,
                            const char *filename, const char *envList);
    void (*SetPlayopenCallback)(CSOUND *,
                                int (*playopen__)(CSOUND * /* , const csRtAudioParams *parm */));
    void (*SetRtplayCallback)(CSOUND *,
                void (*rtplay__)(CSOUND *, const MYFLT *outBuf, int nbytes));
    void (*SetRecopenCallback)(CSOUND *,
                               int (*recopen__)(CSOUND * /* , const csRtAudioParams *parm */));
    void (*SetRtrecordCallback)(CSOUND *,
                int (*rtrecord__)(CSOUND *, MYFLT *inBuf, int nbytes));
    void (*SetRtcloseCallback)(CSOUND *, void (*rtclose__)(CSOUND *));
  void (*AuxAlloc)(CSOUND *, size_t nbytes /* , AUXCH *auxchp */);
    void *(*Malloc)(CSOUND *, size_t nbytes);
    void *(*Calloc)(CSOUND *, size_t nbytes);
    void *(*ReAlloc)(CSOUND *, void *oldp, size_t nbytes);
    void (*Free)(CSOUND *, void *ptr);
    /* Internal functions that are needed */
  void (*dispset)(CSOUND * /* , WINDAT *, MYFLT *, int32, char *, int, char * */);
  void (*display)(CSOUND * /* , WINDAT * */);
    int (*dispexit)(CSOUND *);
    MYFLT (*intpow)(MYFLT, int32);
    void *(*ldmemfile)(CSOUND *, const char *);  /* use ldmemfile2 instead */
    int32 (*strarg2insno)(CSOUND *, void *p, int is_string);
    char *(*strarg2name)(CSOUND *, char *, void *, const char *, int);
  int (*hfgens)(CSOUND * /* , FUNC **, const EVTBLK *, int */);
  int (*insert_score_event)(CSOUND * /* , EVTBLK *, double */);
    int (*FTAlloc)(CSOUND *, int tableNum, int len);
    int (*FTDelete)(CSOUND *, int tableNum);
    void *(*FTFind)(CSOUND *, MYFLT *argp);
    void *(*FTFindP)(CSOUND *, MYFLT *argp);
    void *(*FTnp2Find)(CSOUND *, MYFLT *argp);
    int (*GetTable)(CSOUND *, MYFLT **tablePtr, int tableNum);
    void *(*LoadSoundFile)(CSOUND *, const char *, void *);
    char *(*getstrformat)(int format);
    int (*sfsampsize)(int format);
    char *(*type2string)(int type);
    void *(*SAsndgetset)(CSOUND *,
                         char *, void *, MYFLT *, MYFLT *, MYFLT *, int);
    void *(*sndgetset)(CSOUND *, void *);
    int (*getsndin)(CSOUND *, void *, MYFLT *, int, void *);
    void (*rewriteheader)(void *ofd);
    int (*Rand31)(int *seedVal);
    void (*FDRecord)(CSOUND *, void *fdchp);
    void (*FDClose)(CSOUND *, void *fdchp);
    void (*SetDebug)(CSOUND *, int d);
    int (*GetDebug)(CSOUND *);
    int (*TableLength)(CSOUND *, int table);
    MYFLT (*TableGet)(CSOUND *, int table, int index);
    void (*TableSet)(CSOUND *, int table, int index, MYFLT value);
    void *(*CreateThread)(uintptr_t (*threadRoutine)(void *), void *userdata);
    uintptr_t (*JoinThread)(void *thread);
    void *(*CreateThreadLock)(void);
    void (*DestroyThreadLock)(void *lock);
    int (*WaitThreadLock)(void *lock, size_t milliseconds);
    void (*NotifyThreadLock)(void *lock);
    void (*WaitThreadLockNoTimeout)(void *lock);
    void (*Sleep)(size_t milliseconds);
    void (*InitTimerStruct)(void *);
    double (*GetRealTime)(void *);
    double (*GetCPUTime)(void *);
    uint32_t (*GetRandomSeedFromTime)(void);
    void (*SeedRandMT)(void *p,
                       const uint32_t *initKey, uint32_t keyLength);
    uint32_t (*RandMT)(void *p);
    int (*PerformKsmpsAbsolute)(CSOUND *);
    char *(*LocalizeString)(const char *);
    int (*CreateGlobalVariable)(CSOUND *, const char *name, size_t nbytes);
    void *(*QueryGlobalVariable)(CSOUND *, const char *name);
    void *(*QueryGlobalVariableNoCheck)(CSOUND *, const char *name);
    int (*DestroyGlobalVariable)(CSOUND *, const char *name);
    int (*CreateConfigurationVariable)(CSOUND *, const char *name,
                                       void *p, int type, int flags,
                                       void *min, void *max,
                                       const char *shortDesc,
                                       const char *longDesc);
    int (*SetConfigurationVariable)(CSOUND *, const char *name, void *value);
    int (*ParseConfigurationVariable)(CSOUND *,
                                      const char *name, const char *value);
    csCfgVariable_t *(*QueryConfigurationVariable)(CSOUND *, const char *name);
    csCfgVariable_t **(*ListConfigurationVariables)(CSOUND *);
    int (*DeleteConfigurationVariable)(CSOUND *, const char *name);
    const char *(*CfgErrorCodeToString)(int errcode);
    int (*GetSizeOfMYFLT)(void);
    void **(*GetRtRecordUserData)(CSOUND *);
    void **(*GetRtPlayUserData)(CSOUND *);
    MYFLT (*GetInverseComplexFFTScale)(CSOUND *, int FFTsize);
    MYFLT (*GetInverseRealFFTScale)(CSOUND *, int FFTsize);
    void (*ComplexFFT)(CSOUND *, MYFLT *buf, int FFTsize);
    void (*InverseComplexFFT)(CSOUND *, MYFLT *buf, int FFTsize);
    void (*RealFFT)(CSOUND *, MYFLT *buf, int FFTsize);
    void (*InverseRealFFT)(CSOUND *, MYFLT *buf, int FFTsize);
    void (*RealFFTMult)(CSOUND *, MYFLT *outbuf, MYFLT *buf1, MYFLT *buf2,
                                  int FFTsize, MYFLT scaleFac);
    void (*RealFFTnp2)(CSOUND *, MYFLT *buf, int FFTsize);
    void (*InverseRealFFTnp2)(CSOUND *, MYFLT *buf, int FFTsize);
    int (*AddUtility)(CSOUND *, const char *name,
                      int (*UtilFunc)(CSOUND *, int, char **));
    int (*RunUtility)(CSOUND *, const char *name, int argc, char **argv);
    char **(*ListUtilities)(CSOUND *);
    int (*SetUtilityDescription)(CSOUND *, const char *utilName,
                                           const char *utilDesc);
    const char *(*GetUtilityDescription)(CSOUND *, const char *utilName);
    int (*RegisterSenseEventCallback)(CSOUND *, void (*func)(CSOUND *, void *),
                                                void *userData);
    int (*RegisterDeinitCallback)(CSOUND *, void *p,
                                            int (*func)(CSOUND *, void *));
    int (*RegisterResetCallback)(CSOUND *, void *userData,
                                           int (*func)(CSOUND *, void *));
    void *(*CreateFileHandle)(CSOUND *, void *, int, const char *);
    /* Do not use FileOpen in new code; it has been replaced by FileOpen2 */
    void *(*FileOpen)(CSOUND *,
                      void *, int, const char *, void *, const char *);
    char *(*GetFileName)(void *);
    int (*FileClose)(CSOUND *, void *);
    /* PVOC-EX system */
    int (*PVOC_CreateFile)(CSOUND *, const char *,
                           uint32, uint32, uint32,
                           uint32, int32, int, int,
                           float, float *, uint32);
    int (*PVOC_OpenFile)(CSOUND *, const char *, void *, void *);
    int (*PVOC_CloseFile)(CSOUND *, int);
    int (*PVOC_PutFrames)(CSOUND *, int, const float *, int32);
    int (*PVOC_GetFrames)(CSOUND *, int, float *, uint32);
    int (*PVOC_FrameCount)(CSOUND *, int);
    int (*PVOC_fseek)(CSOUND *, int, int);
    const char *(*PVOC_ErrorString)(CSOUND *);
    int (*PVOCEX_LoadFile)(CSOUND *, const char *, void *);
    char *(*GetOpcodeName)(void *p);
    int (*GetInputArgCnt)(void *p);
    unsigned long (*GetInputArgAMask)(void *p);
    unsigned long (*GetInputArgSMask)(void *p);
    char *(*GetInputArgName)(void *p, int n);
    int (*GetOutputArgCnt)(void *p);
    unsigned long (*GetOutputArgAMask)(void *p);
    unsigned long (*GetOutputArgSMask)(void *p);
    char *(*GetOutputArgName)(void *p, int n);
    int (*SetReleaseLength)(void *p, int n);
    MYFLT (*SetReleaseLengthSeconds)(void *p, MYFLT n);
    int (*GetMidiChannelNumber)(void *p);
    void *(*GetMidiChannel)(void *p);
    int (*GetMidiNoteNumber)(void *p);
    int (*GetMidiVelocity)(void *p);
    int (*GetReleaseFlag)(void *p);
    double (*GetOffTime)(void *p);
    MYFLT *(*GetPFields)(void *p);
    int (*GetInstrumentNumber)(void *p);
    void (*Die)(CSOUND *, const char *msg, ...);
    int (*InitError)(CSOUND *, const char *msg, ...);
    int (*PerfError)(CSOUND *, const char *msg, ...);
    void (*Warning)(CSOUND *, const char *msg, ...);
    void (*DebugMsg)(CSOUND *, const char *msg, ...);
    void (*LongJmp)(CSOUND *, int);
    void (*ErrorMsg)(CSOUND *, const char *fmt, ...);
    void (*ErrMsgV)(CSOUND *, const char *hdr, const char *fmt, va_list);
    int (*GetChannelPtr)(CSOUND *, MYFLT **p, const char *name, int type);
    int (*ListChannels)(CSOUND *, void **lst);
    int (*SetControlChannelParams)(CSOUND *, const char *name,
                                   int type, MYFLT dflt, MYFLT min, MYFLT max);
    int (*GetControlChannelParams)(CSOUND *, const char *name,
                                   MYFLT *dflt, MYFLT *min, MYFLT *max);
    int (*ChanIKSet)(CSOUND *, MYFLT value, int n);
    int (*ChanOKGet)(CSOUND *, MYFLT *value, int n);
    int (*ChanIASet)(CSOUND *, const MYFLT *value, int n);
    int (*ChanOAGet)(CSOUND *, MYFLT *value, int n);
    void (*dispinit)(CSOUND *);
    void *(*Create_Mutex)(int isRecursive);
    int (*LockMutexNoWait)(void *mutex_);
    void (*LockMutex)(void *mutex_);
    void (*UnlockMutex)(void *mutex_);
    void (*DestroyMutex)(void *mutex_);
    long (*RunCommand)(const char * const *argv, int noWait);
    void *(*GetCurrentThreadID)(void);
    void (*SetChannelIOCallback)(CSOUND *, int func);
    int (*Set_Callback)(CSOUND *, int (*func)(void *, void *, unsigned int),
                                  void *userData, unsigned int typeMask);
    void (*Remove_Callback)(CSOUND *,
                            int (*func)(void *, void *, unsigned int));
    int (*PvsinSet)(CSOUND *, void *value, int n);
    int (*PvsoutGet)(CSOUND *, void *value, int n);
    void (*SetInternalYieldCallback)(CSOUND *,
                       int (*yieldCallback)(CSOUND *));
    void *(*CreateBarrier)(unsigned int max);
    int (*DestroyBarrier)(void *);
    int (*WaitBarrier)(void *);
    void *(*FileOpen2)(CSOUND *, void *, int, const char *, void *,
                      const char *, int, int);
    int (*type2csfiletype)(int type, int encoding);
    void *(*ldmemfile2)(CSOUND *, const char *, int);
    void (*NotifyFileOpened)(CSOUND*, const char*, int, int, int);
    int (*sftype2csfiletype)(int type);
    int (*insert_score_event_at_sample)(CSOUND *, void *, long);
    int *(*GetChannelLock)(CSOUND *, const char *name, int type);
    void *(*ldmemfile2withCB)(CSOUND *, const char *, int,
                                int (*callback)(CSOUND *));
 /* SUBR dummyfn_1; */
    SUBR dummyfn_2[84];
    int           dither_output;
    void          *flgraphGlobals;
    char          *delayederrormessages;
    void          *printerrormessagesflag;
    /* ----------------------- public data fields ----------------------- */
    /** used by init and perf loops */
    void          *ids, *pds;
    int           ksmps, global_ksmps, nchnls, spoutactive;
    long          kcounter, global_kcounter;
    int           reinitflag;
    int           tieflag;
    MYFLT         esr, onedsr, sicvt;
    MYFLT         tpidsr, pidsr, mpidsr, mtpdsr;
    MYFLT         onedksmps;
    MYFLT         ekr, global_ekr;
    MYFLT         onedkr;
    MYFLT         kicvt;
    MYFLT         e0dbfs, dbfs_to_float;
    /** start time of current section    */
    double        timeOffs, beatOffs;
    /** current time in seconds, inc. per kprd */
    long          icurTime;   /* Current time in samples */
    double        curTime_inc;
    /** current time in beats, inc per kprd */
    double        curBeat, curBeat_inc;
    /** beat time = 60 / tempo           */
    long          ibeatTime;   /* Beat time in samples */
    int           spoutlock, spinlock;
    /* Widgets */
    void          *widgetGlobals;
    /** reserved for std opcode library  */
    void          *stdOp_Env;
    MYFLT         *zkstart;
    MYFLT         *zastart;
    long          zklast;
    long          zalast;
    MYFLT         *spin;
    MYFLT         *spout;
    int           nspin;
    int           nspout;
    OPARMS        *oparms;
};

CSOUND cs;

typedef struct {
    char        *opname;
    char        *module;
} opcodeListEntry;

#include <dirent.h>
const char *buf[1024];
char *fname;
opcodeListEntry *db =NULL;
int opListSize = 0;
int opListAlloc = -1;

int myAppendOpcodes(CSOUND *csound, const OENTRY *ops, int n)
{
    int i;
    if (opListSize+n >= opListAlloc) {
      db = (opcodeListEntry*) realloc(db, (opListAlloc += n)*sizeof(opcodeListEntry));
    }
    for (i=0; i<n; i++) {
      db[opListSize].opname = strdup(ops[i].opname);
      db[opListSize].module = strdup(fname+3);
      db[opListSize].module[strlen(fname+3)-3]='\0';
      opListSize++;
      /* printf("\t%s\t:%s:\n", ops[i].opname, fname+3); */
    }
    return 0;
}

int myAppendOpcode1(CSOUND *cs, const char *opname)
{
    if (opListSize >= opListAlloc) {
      db = (opcodeListEntry*) realloc(db, (opListAlloc += 20)*sizeof(opcodeListEntry));
    }
    db[opListSize].opname = strdup(opname);
    db[opListSize].module = strdup(fname+3);
    db[opListSize].module[strlen(fname+3)-3]='\0';
    opListSize++;
    /* printf("\t%s\t:%s:\n", opname, fname+3); */
}

int myAppendOpcode(CSOUND *cs, const char *opname, int dsblksiz,
                   int thread, const char *outypes, const char *intypes,
                   int (*iopadr)(CSOUND *, void *),
                   int (*kopadr)(CSOUND *, void *),
                   int (*aopadr)(CSOUND *, void *))
{
    myAppendOpcode1(cs, opname);
}

void *myCalloc(CSOUND *cs, size_t nbytes)
{
    return calloc(1, nbytes);
}

int* myQueryConfigurationVariable(CSOUND *cs, char *name)
{
    static int x = 0;
    fprintf(stderr,"myQueryConfigurationVariable %s\n", name);
    return &x;
}

void myErrorMsg(CSOUND *cs, const char *fmt, ...)
{
    return;
}

char *nullc(const char *s)
{
    return (char*)s;
}

int null(CSOUND *cs, const char *s)
{
    return 0;
}

void*  nullp(CSOUND *cs, const char *s)
{
    return NULL;
}

char *csoundLocalizeString(const char *s)
{
    return (char *)s;
}

int mycmp(const void *a, const void *b)
{
    const opcodeListEntry *aa= a, *bb = b;
    return strcmp(aa->opname, bb->opname);
}

int modcmp(const void *a, const void *b)
{
    int n;
    const opcodeListEntry *aa= a, *bb = b;
    n = strcmp(aa->module, bb->module);
    if (n==0) n = strcmp(aa->opname, bb->opname);
    return n;
}

int main(int argc, char *argv[])
{
    DIR             *dir;
    struct dirent   *f;
    char *dname;
    OENTRY *ops;
    int msort = 0;

    if (argc>1 && (strcmp(argv[1], "-m")==0)) {
      argc--; argv++; msort = 1;
    }
    if (argc>1) dname = argv[1]; else dname = ".";
    dir = opendir(dname);

    cs.AppendOpcodes = myAppendOpcodes;
    cs.AppendOpcode = myAppendOpcode;
    cs.QueryGlobalVariable = (void *(*)(CSOUND *, const char *name))null;
    cs.CreateGlobalVariable = (int (*)(CSOUND *, const char *, size_t))null;
    cs.DestroyGlobalVariable = (int (*)(CSOUND *, const char *name))null;
    cs.QueryGlobalVariableNoCheck = (void *(*)(CSOUND *, const char *name))null;
    cs.Calloc = myCalloc;
    cs.QueryConfigurationVariable = (csCfgVariable_t *(*)(CSOUND *, const char *))myQueryConfigurationVariable;
    cs.ErrorMsg = myErrorMsg;
    cs.Die = (void (*)(CSOUND *, const char *, ...))null;
    /* cs.SetExternalMidiInOpenCallback = null; */
    /* cs.SetExternalMidiReadCallback = null; */
    /* cs.SetExternalMidiInCloseCallback = null; */
    /* cs.SetExternalMidiOutOpenCallback = null; */
    /* cs.SetExternalMidiWriteCallback = null; */
    /* cs.SetExternalMidiOutCloseCallback = null; */
    cs.GetEnv = (const char *(*)(CSOUND *, const char *))nullp;
    cs.LocalizeString = (char *(*)(const char *))nullc;

    cs.oparms = (OPARMS*)calloc(1, sizeof(OPARMS));
    cs.widgetGlobals = (void*)null; /* ie something */
    if (dir == (DIR*) NULL) {
      fprintf(stderr, "Error opening plugin directory '%s': %s",
              dname, strerror(errno));
      return -1;
    }
    while ((f = readdir(dir)) != NULL) {
      int i, n, len;
      void *library;
      long    (*lib_init)(void *, OENTRY **);
      int     (*lib_init0)(CSOUND*);

      fname = &(f->d_name[0]);
      len = (int) strlen(fname);
      if (len<3 || fname[len-3] != '.')      /* Plugins not . files */
        continue;
      if (strncmp(fname, "lib", 3)!=0) continue;
      if (strcmp(fname+len-3, ".so")!=0) continue;
      /* found a dynamic library, attempt to open it */
      if (((int) strlen(dname) + len + 2) > 1024) {
        fprintf(stderr,"path name too long, skipping '%s'", fname);
        continue;
      }
      if (strncmp(fname, "libpmidi", 8)==0) continue;
      if (strncmp(fname, "librtalsa", 9)==0) continue;
      if (strncmp(fname, "libwidgets", 10)==0) continue; /* This needs to be fixed */
      sprintf((char*)buf, "%s/%s", dname, fname);
      if ((library = (void*) dlopen((const char *)buf, RTLD_NOW))==NULL) {
        fprintf(stderr, "Failed to open %s\n", buf);
        continue;
      }
      lib_init = (long (*)(void *, OENTRY **))
        dlsym(library, "csound_opcode_init"); /* Function to give length */
      if (lib_init!=NULL) {
        n = lib_init(&cs, &ops)/sizeof(OENTRY);
        //        fprintf(stderr, "Module %s\n", fname);
        for (i=0; i<n; i++) {
          myAppendOpcode1(&cs, ops[i].opname);
          //          printf("\t%s\t:%s:\n", ops[i].opname, fname+3);
        }
        dlclose(library);
        continue;
      }
      else {
        lib_init0 = (int (*)(CSOUND *))dlsym(library, "csoundModuleInit");
        if (lib_init0==NULL) {
          if (strstr((char*)buf, "libcsound.so")==NULL)
            fprintf(stderr, "No csound_opcode_init in %s\n", buf);
          dlclose(library);
          continue;
        }

        // fprintf(stderr,"Module %s\n", buf);
        // printf("QueryConfigurationVariable=%p\n", cs.QueryConfigurationVariable);
        lib_init0(&cs);
        dlclose(library);
      }
    }
    closedir(dir);
    /* { */
    /*   int i; */
    /*   for (i=0; i<opListSize; i++) */
    /*     printf("\t%s\t%s\n", db[i].opname, db[i].module); */
    /* } */
/* This sort seems to fail actually sort */
    if (msort) qsort(db, opListSize, sizeof(opcodeListEntry), modcmp);
    else qsort(db, opListSize, sizeof(opcodeListEntry), mycmp);
    {
      int i;
      for (i=0; i<opListSize; i++) {
        int m, n = strlen(db[i].opname);
        printf("%s", db[i].opname);
        for (m=n; m<24; m++) putchar(' ');
        printf("%s\n", db[i].module);
        //printf("%s\t%s\n", db[i].opname, db[i].module);
      }
    }
}

