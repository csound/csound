/*
    main.c:

    Copyright (C) 1991-2002 Barry Vercoe, John ffitch

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
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "cs.h"                 /*                               MAIN.C */
#include "soundio.h"
#include "prototyp.h"
#include "csound.h"
#include "csmodule.h"
#include <ctype.h>              /* For isdigit */

#ifdef mills_macintosh
#include <SIOUX.h>
#include "perf.h"
#include"MacTransport.h"

#define PATH_LEN        128

extern struct Transport transport;
extern Boolean displayText;

extern char sfdir_path[];
extern char sadir_path[];
extern char ssdir_path[];
extern char saved_scorename[];
extern unsigned char mytitle[];
extern Boolean util_perf;
static char *foo;
static char listing_file[PATH_LEN];
static int  vbuf;
static int csNGraphs;
static MYFLT temp;
extern int RescaleFloatFile;
#endif

static  char    *sortedscore = NULL;
static  char    *xtractedscore = "score.xtr";
static  char    scnm[255];
static  char    nme[255];
static  char    *playscore = NULL;     /* unless we extract */
static  FILE    *scorin, *scorout, *xfile;
extern  void    dieu(char *);
extern  OPARMS  O;
extern  ENVIRON cenviron;
extern int argdecode(void*, int, char**);
extern void init_pvsys(void);
extern int csoundYield(void *);

#include <signal.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef MSVC
#include <windows.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#if !defined(LINUX) && !defined(SGI) && !defined(__BEOS__) && !defined(__MACH__)
static char *signal_to_string(int sig)
{
    switch(sig) {
#ifdef SIGHUP
    case SIGHUP:        return "Hangup";
#endif
#ifdef SIGINT
    case SIGINT:        return "Interrupt";
#endif
#ifdef SIGQUIT
    case SIGQUIT:       return "Quit";
#endif
#ifdef SIGILL
    case SIGILL:        return "Illegal instruction";
#endif
#ifdef SIGTRAP
    case SIGTRAP:       return "Trace trap";
#endif
#ifdef SIGABRT
    case SIGABRT:       return "Abort";
#endif
#ifdef SIGBUS
    case SIGBUS:        return "BUS error";
#endif
#ifdef SIGFPE
    case SIGFPE:        return "Floating-point exception";
#endif
#ifdef SIGUSR1
    case SIGUSR1:       return "User-defined signal 1";
#endif
#ifdef SIGSEGV
    case SIGSEGV:       return "Segmentation violation";
#endif
#ifdef SIGUSR2
    case SIGUSR2:       return "User-defined signal 2";
#endif
#ifdef SIGPIPE
    case SIGPIPE:       return "Broken pipe";
#endif
#ifdef SIGALRM
    case SIGALRM:       return "Alarm clock";
#endif
#ifdef SIGTERM
    case SIGTERM:       return "Termination";
#endif
#ifdef SIGSTKFLT
    case SIGSTKFLT:     return "???";
#endif
#ifdef SIGCHLD
    case SIGCHLD:       return "Child status has changed";
#endif
#ifdef SIGCONT
    case SIGCONT:       return "Continue";
#endif
#ifdef SIGSTOP
    case SIGSTOP:       return "Stop, unblockable";
#endif
#ifdef SIGTSTP
    case SIGTSTP:       return "Keyboard stop";
#endif
#ifdef SIGTTIN
    case SIGTTIN:       return "Background read from tty";
#endif
#ifdef SIGTTOU
    case SIGTTOU:       return "Background write to tty";
#endif
#ifdef SIGURG
    case SIGURG:        return "Urgent condition on socket ";
#endif
#ifdef SIGXCPU
    case SIGXCPU:       return "CPU limit exceeded";
#endif
#ifdef SIGXFSZ
    case SIGXFSZ:       return "File size limit exceeded ";
#endif
#ifdef SIGVTALRM
    case SIGVTALRM:     return "Virtual alarm clock ";
#endif
#ifdef SIGPROF
    case SIGPROF:       return "Profiling alarm clock";
#endif
#ifdef SIGWINCH
    case SIGWINCH:      return "Window size change ";
#endif
#ifdef SIGIO
    case SIGIO:         return "I/O now possible";
#endif
#ifdef SIGPWR
    case SIGPWR:        return "Power failure restart";
#endif
    default:
      return "???";
    }
}

#ifndef __MACH__
void psignal(int sig, char *str)
{
    err_printf( "%s: %s\n", str, signal_to_string(sig));
}
#endif
#endif

#if defined(__BEOS__)
void psignal(int sig, char *str)
{
    err_printf("%s: %s\n", str, strsignal(sig));
}
#endif

extern int cleanup(void *csound);

static void signal_handler(int sig)
{
#if defined(USE_FLTK) && defined(SIGALRM)
    if (sig == SIGALRM) return;
#endif
    psignal(sig, "Csound tidy up");
    fltk_abort = 1;
    /* FIXME: cannot have a csound pointer here, but signal handling */
    /* should be done by the host application anyway, and not by the */
    /* Csound library */
    cleanup((void*) &cenviron);
#ifdef LINUX
    usleep(250000);
#else
#ifndef MSVC /* VL MSVC fix */
    sleep(1);
#else
    Sleep(1000);
#endif
#endif
    exit(1);
}

static void install_signal_handler(void)
{
    int *x;
#if defined(LINUX) || defined(SGI) || defined(sol)
    int sigs[] = { SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGIOT,
                   SIGBUS, SIGFPE, SIGSEGV, SIGPIPE, SIGALRM, SIGTERM, SIGXCPU,
                   SIGXFSZ, -1};
#elif defined(__MACH__)
    int sigs[] = { SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGIOT,
                   SIGBUS, SIGFPE, SIGSEGV, SIGPIPE,          SIGTERM, SIGXCPU,
                   SIGXFSZ, -1};
#elif defined(CSWIN)
    int sigs[] = { SIGHUP, SIGINT, SIGQUIT, -1};
#elif defined(WIN32)
    int sigs[] = { SIGINT, SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGTERM, -1};
#elif defined(__EMX__)
    int sigs[] = { SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGBUS,
                   SIGFPE, SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGALRM, SIGTERM,
                   SIGCHLD, -1 };
#else
    int sigs[] = { -1};
#endif

    for (x = sigs; *x > 0; x++)
      signal(*x, signal_handler);
#if defined(__MACH__)
    signal(SIGALRM, SIG_IGN);
#endif
}

void create_opcodlst(void *csound)
{
    extern OENTRY opcodlst_1[], opcodlst_2[];
    extern long oplength_1, oplength_2;
    long length = 0;

    /* Basic Entry1 stuff */
    if (opcodlst!=NULL) return;
    opcodlst = (OENTRY*) mmalloc(csound, length = oplength_1);
    memcpy(opcodlst, opcodlst_1, oplength_1);
    oplstend = opcodlst +  length/sizeof(OENTRY);
    /* Add entry2 */
    opcodlst = (OENTRY*) mrealloc(csound, opcodlst, length + oplength_2);
    memcpy(opcodlst+length/sizeof(OENTRY), opcodlst_2, oplength_2);
    length += oplength_2;
    oplstend = opcodlst +  length/sizeof(OENTRY);
    csoundLoadExternals(csound);
}

extern  int    musmon(ENVIRON*), musmon2(ENVIRON*);
extern  char  *getstrformat(int);
extern  short  sfsampsize(int);

/* IV - Jan 28 2005 */
void print_benchmark_info(void *csound, const char *s)
{
    double  rt, ct;
    RTCLOCK *p;

    if ((O.msglevel & 0x80) == 0)
      return;
    p = (RTCLOCK*) csoundQueryGlobalVariable(csound, "csRtClock");
    if (p == NULL)
      return;
    rt = timers_get_real_time(p);
    ct = timers_get_CPU_time(p);
    ((ENVIRON*)csound)->Message(csound,
               Str("Elapsed time at %s: real: %.3fs, CPU: %.3fs\n"),
               (char*) s, rt, ct);
}

#ifdef MSVC
_declspec(dllexport) /* VL linkage fix 11-04 */
#endif
int csoundCompile(void *csound_, int argc, char **argv)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    char  *s, *orcNameMode;
    char  *filnamp, *envoutyp = NULL;
    int   n;

    /* for debugging only */
    if ((n = setjmp(csound->exitjmp))) {
      fprintf(stderr,
              " *** WARNING: longjmp() called during csoundPreCompile() ***\n");
      return (-(abs(n)));
    }

    /* IV - Feb 05 2005: find out if csoundPreCompile() needs to be called */
    if (csoundQueryGlobalVariable(csound, "_RTAUDIO") == NULL)
      if (csoundPreCompile(csound) != CSOUND_SUCCESS)
        return CSOUND_ERROR;
    if (csoundQueryGlobalVariable(csound, "csRtClock") != NULL)
      if (csoundPreCompile(csound) != CSOUND_SUCCESS)
        return CSOUND_ERROR;

    if ((n = setjmp(csound->exitjmp))) {
      /*
       * Changed from exit(-1) for re-entrancy.
       */
      return (-(abs(n)));
    }

    /* IV - Jan 28 2005 */
    csoundCreateGlobalVariable(csound, "csRtClock", sizeof(RTCLOCK));
    init_pvsys();
    /* utilities depend on this as well as orchs */
    csound->e0dbfs = DFLT_DBFS;         /* may get changed by an orch */
    dbfs_init(csound->e0dbfs);
    timers_struct_init((RTCLOCK*)
                       csoundQueryGlobalVariable(csound, "csRtClock"));
    csoundCreateGlobalVariable(csound, "#CLEANUP", (size_t) 1);
    if (sizeof(MYFLT)==sizeof(float)) {
#ifdef BETA
      csound->Message(csound,"Csound version %s beta (float samples) %s\n",
                 PACKAGE_VERSION, __DATE__);
#else
      csound->Message(csound,"Csound version %s (float samples) %s\n",
                 PACKAGE_VERSION, __DATE__);
#endif
    }
    else {
#ifdef BETA
      csound->Message(csound,"Csound version %s beta (double samples) %s\n",
                 PACKAGE_VERSION, __DATE__);
#else
      csound->Message(csound,"Csound version %s (double samples) %s\n",
                 PACKAGE_VERSION, __DATE__);
#endif
    }
    {
      char buffer[128];
#include <sndfile.h>
      sf_command (NULL, SFC_GET_LIB_VERSION, buffer, 128);
      csound->Message(csound,"%s\n", buffer);
    }

    /* do not know file type yet */
    O.filetyp = -1;
    O.sfheader = 0;
    install_signal_handler();
    O.filnamspace = filnamp = mmalloc(csound, (size_t) 1024);
    peakchunks = 1;
    if (csoundCreateGlobalVariable(csound, "::argdecode::orcNameMode", 8) != 0)
      return -1;
    orcNameMode = (char*) csoundQueryGlobalVariable(csound,
                                                    "::argdecode::orcNameMode");
    if (--argc == 0) {
      dieu(Str("insufficient arguments"));
    }
    /* command line: allow orc/sco/csd name */
    strcpy(orcNameMode, "normal");
    if (argdecode(csound, argc, argv) == 0)
      longjmp(csound->exitjmp, 1);
    /* do not allow orc/sco/csd name in .csoundrc */
    strcpy(orcNameMode, "fail");
    {
      char *csrcname;
      FILE *csrc;
      /* IV - Feb 17 2005 */
      csrcname = csoundGetEnv(csound, "CSOUNDRC");
      csrc = NULL;
      if (csrcname != NULL && csrcname[0] != '\0') {
        csrc = fopen(csrcname, "r");
        if (csrc == NULL)
          csound->Message(csound,Str("WARNING: cannot open csoundrc file %s\n"), csrcname);
      }
      if (csrc == NULL &&
          (csoundGetEnv(csound, "HOME") != NULL &&
           csoundGetEnv(csound, "HOME")[0] != '\0')) {
        /* 11 bytes for DIRSEP, ".csoundrc" (9 chars), and null character */
        csrcname = (char*)
          malloc((size_t) strlen(csoundGetEnv(csound, "HOME")) + (size_t) 11);
        if (csrcname == NULL) {
          csound->Message(csound,Str(" *** memory allocation failure\n"));
          longjmp(csound->exitjmp, 1);
        }
        sprintf(csrcname, "%s%c%s",
                          csoundGetEnv(csound, "HOME"), DIRSEP, ".csoundrc");
        csrc = fopen(csrcname, "r");
        free(csrcname);
      }
      /* read global .csoundrc file (if exists) */
      if (csrc != NULL) {
        readOptions(csound, csrc);
        fclose(csrc);
      }
      /* check for .csoundrc in current directory */
      csrc = fopen(".csoundrc", "r");
      if (csrc != NULL) {
        readOptions(csound, csrc);
        fclose(csrc);
      }
    }
    /* check for CSD file */
    if (orchname == NULL)
      dieu(Str("no orchestra name"));
    else if ((strcmp(orchname+strlen(orchname)-4, ".csd")==0 ||
              strcmp(orchname+strlen(orchname)-4, ".CSD")==0) &&
             (scorename==NULL || strlen(scorename)==0)) {
      int   read_unified_file(void*, char **, char **);
      /* FIXME: allow orc/sco/csd name in CSD file: does this work ? */
      strcpy(orcNameMode, "normal");
      csound->Message(csound,"UnifiedCSD:  %s\n", orchname);
      if (!read_unified_file(csound, &orchname, &scorename)) {
        csound->Die(csound, Str("Decode failed....stopping"));
      }
    }
    /* IV - Feb 19 2005: run a second pass of argdecode so that */
    /* command line options override CSD options */
    /* this assumes that argdecode is safe to run multiple times */
    strcpy(orcNameMode, "ignore");
    argdecode(csound, argc, argv);      /* should not fail this time */
    /* some error checking */
    {
      int *nn;
      nn = (int*) csoundQueryGlobalVariable(csound, "::argdecode::stdinassign");
      if (nn != NULL && *nn != 0 && (*nn & (*nn - 1)) != 0) {
        csound->Die(csound, Str("error: multiple uses of stdin"));
      }
      nn =
        (int*) csoundQueryGlobalVariable(csound, "::argdecode::stdoutassign");
      if (nn != NULL && *nn != 0 && (*nn & (*nn - 1)) != 0) {
        csound->Die(csound, Str("error: multiple uses of stdout"));
      }
    }
    /* done parsing csoundrc, CSD, and command line options */
    /* if sound file type is still not known, check SFOUTYP */
    if (O.filetyp < 0) {
      envoutyp = csoundGetEnv(csound, "SFOUTYP");
      if (envoutyp != NULL && envoutyp[0] != '\0') {
        if (strcmp(envoutyp,"AIFF") == 0)
          O.filetyp = TYP_AIFF;
        else if (strcmp(envoutyp,"WAV") == 0 || strcmp(envoutyp,"WAVE") == 0)
          O.filetyp = TYP_WAV;
        else if (strcmp(envoutyp,"IRCAM") == 0)
          O.filetyp = TYP_IRCAM;
        else if (strcmp(envoutyp, "RAW") == 0)
          O.filetyp = TYP_RAW;
        else {
          sprintf(errmsg, Str("%s not a recognised SFOUTYP env setting"),
                          envoutyp);
          dieu(errmsg);
        }
      }
      else
        O.filetyp = TYP_WAV;    /* default to WAV if even SFOUTYP is unset */
    }
    /* everything other than a raw sound file has a header */
    O.sfheader = (O.filetyp == TYP_RAW ? 0 : 1);

    if (scorename==NULL || strlen(scorename)==0) { /* No scorename yet */
      char *p;
      FILE *scof;
      extern char sconame[];
      void deleteScore(void);
      tmpnam(sconame);              /* Generate score name */
      if ((p=strchr(sconame, '.')) != NULL) *p='\0'; /* with extention */
      strcat(sconame, ".sco");
      scof = fopen(sconame, "w");
      fprintf(scof, "f0 86400\n");
      fclose(scof);
      scorename = sconame;
      add_tmpfile(csound, sconame);     /* IV - Feb 03 2005 */
    }
    if (O.Linein || O.Midiin || O.FMidiin)
      O.RTevents = 1;
    if (O.RTevents || O.sfread)
      O.ksensing = 1;
    if (!O.sfheader)
      O.rewrt_hdr = 0;          /* cannot rewrite header of headerless file */
    if (O.sr_override || O.kr_override) {
      if (!O.sr_override || !O.kr_override)
        dieu(Str("srate and krate overrides must occur jointly"));
    }
    if (!O.outformat)                       /* if no audioformat yet  */
      O.outformat = AE_SHORT;               /*  default to short_ints */
    O.sfsampsize = sfsampsize(O.outformat);
    O.informat = O.outformat;       /* informat defaults; */
    O.insampsiz = O.sfsampsize;     /* resettable by readinheader */
    csound->Message(csound,Str("orchname:  %s\n"), orchname);
    if (scorename != NULL)
      csound->Message(csound,Str("scorename: %s\n"), scorename);
    if (xfilename != NULL)
      csound->Message(csound,Str("xfilename: %s\n"), xfilename);
#if defined(WIN32) || defined(__EMX__)
    {
      if (O.odebug) setvbuf(stdout,0,_IOLBF,0xff);
    }
#else
#if !defined(SYMANTEC) && !defined(mac_classic)
    if (O.odebug) setlinebuf(stdout);
#endif
#endif
    /* IV - Oct 31 2002: moved orchestra compilation here, so that named */
    /* instrument numbers are known at the score read/sort stage */
    create_opcodlst(&cenviron); /* create initial opcode list if not done yet */
    /* IV - Jan 31 2005: initialise external modules */
    if (csoundInitModules(csound) != 0)
      longjmp(csound->exitjmp, 1);
    otran(csound);          /*  read orcfile, setup desblks & spaces    */
    /* IV - Jan 28 2005 */
    print_benchmark_info(csound, Str("end of orchestra compile"));
    if (!csoundYield(csound)) return (-1);
    /* IV - Oct 31 2002: now we can read and sort the score */
    if (scorename == NULL || scorename[0]=='\0') {
      if (O.RTevents) {
        csound->Message(csound, Str("realtime performance using dummy "
                                    "numeric scorefile\n"));
        goto perf;
      }
      else {
        if (keep_tmp) {
          scorename = "score.srt";
        }
        else {
          scorename = tmpnam(scnm);
          add_tmpfile(csound, scorename);       /* IV - Feb 03 2005 */
        }
      }
    }
#ifdef mills_macintosh
    {
      char *c;
      strcpy(saved_scorename,scorename);
      strcpy((char *)mytitle,scorename);
      c = (char *)&mytitle[0] + strlen((char *)mytitle);
      while (*c != DIRSEP && c != (char *)mytitle) c -= 1;
      if (c != (char *) mytitle) c += 1;
      strcpy((char *)mytitle,c);
      strcat((char *)mytitle," listing");
      SIOUXSetTitle((unsigned char *)CtoPstr((char *)mytitle));
    }
#endif
    if ((n = strlen(scorename)) > 4            /* if score ?.srt or ?.xtr */
        && (!strcmp(scorename+n-4,".srt") ||
            !strcmp(scorename+n-4,".xtr"))) {
      csound->Message(csound,Str("using previous %s\n"),scorename);
      playscore = sortedscore = scorename;            /*   use that one */
    }
    else {
      if (keep_tmp) {
        playscore = sortedscore = "score.srt";
      }
      else {
        playscore = sortedscore = tmpnam(nme);
        add_tmpfile(csound, playscore);         /* IV - Feb 03 2005 */
      }
      if (!(scorin = fopen(scorename, "r")))          /* else sort it   */
        csoundDie(csound, Str("cannot open scorefile %s"), scorename);
      if (!(scorout = fopen(sortedscore, "w")))
        csoundDie(csound, Str("cannot open %s for writing"), sortedscore);
      csound->Message(csound,Str("sorting score ...\n"));
      scsort(scorin, scorout);
      fclose(scorin);
      fclose(scorout);
    }
    if (xfilename != NULL) {                        /* optionally extract */
      if (!strcmp(scorename,"score.xtr"))
        csoundDie(csound, Str("cannot extract %s, name conflict"), scorename);
      if (!(xfile = fopen(xfilename, "r")))
        csoundDie(csound, Str("cannot open extract file %s"), xfilename);
      if (!(scorin = fopen(sortedscore, "r")))
        csoundDie(csound, Str("cannot reopen %s"), sortedscore);
      if (!(scorout = fopen(xtractedscore, "w")))
        csoundDie(csound, Str("cannot open %s for writing"), xtractedscore);
      csound->Message(csound,Str("  ... extracting ...\n"));
      scxtract(scorin, scorout, xfile);
      fclose(scorin);
      fclose(scorout);
      playscore = xtractedscore;
    }
    csound->Message(csound,Str("\t... done\n"));
    s = playscore;
    O.playscore = filnamp;
    while ((*filnamp++ = *s++));    /* copy sorted score name */
    /* IV - Jan 28 2005 */
    print_benchmark_info(csound, Str("end of score sort"));
 perf:
    O.filnamsize = filnamp - O.filnamspace;
    /* open MIDI output (moved here from argdecode) */
    {
      extern void openMIDIout(void);
      if (O.Midioutname != NULL && O.Midioutname[0] != '\0')
        openMIDIout();
    }
    return musmon(csound);
}

int csoundMain(void *csound, int argc, char **argv)
{
    jmp_buf lj;
    int returnvalue;
    extern void csoundMessage(void *, const char *, ...);
    if ((returnvalue = setjmp(lj))) {
      csoundMessage(csound, "Error return.");
      return returnvalue;
    }

    returnvalue = csoundCompile(csound, argc, argv);
    printf("Compile returns %d\n", returnvalue);
    if (returnvalue) return returnvalue;
    printf("musmon returns %d\n", returnvalue);
    if (returnvalue) return returnvalue;
    return musmon2(csound);
}

void mainRESET(ENVIRON *p)
{
    void adsynRESET(void);
    void cscoreRESET(void);
    void disprepRESET(void);
    void expRESET(ENVIRON *);
    void ftRESET(ENVIRON *);
    void insertRESET(ENVIRON *);
    void lpcRESET(void);
    void memRESET(void*);
    void musRESET(void);
    void oloadRESET(ENVIRON *);
    void orchRESET(void);
    void soundinRESET(void);
    void tranRESET(void);

#if defined(USE_FLTK) && defined(never)        /* IV - Nov 30 2002 */
    void widgetRESET(void);     /* N.B. this is not used yet, */
                                /* because it was not fully tested, */
    widgetRESET();              /* and may crash on some systems */
#endif
    cscoreRESET();
    expRESET(p);
    ftRESET(p);
    disprepRESET();
    insertRESET(p);
    musRESET();
    tranRESET();
    orchRESET();
    soundinRESET();
    adsynRESET();
    lpcRESET();
    while (reset_list) {
      RESETTER *x = reset_list->next;
      (*reset_list->fn)(p);
      mfree(p, reset_list);
      reset_list = x;
    }
    scoreRESET(p);
    oloadRESET(p);              /* should be called last but changed!! */
    memRESET(p);
}

/**
* For re-entrancy.
*/

void csoundMainCleanup(void *csound)
{
    cleanup(csound);            /* IV - Feb 03 2005 */
}

