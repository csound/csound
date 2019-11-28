/*
    csound_main.c:

    Copyrght 2004 The Csound Core Developers

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

/* Console Csound using the Csound API. */
#include "csound.h"
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#if defined(HAVE_UNISTD_H) || defined(MACOSX)
#include <unistd.h>
#endif
#ifdef GNU_GETTEXT
#include <locale.h>
#endif
#define IGN(x) (void) x
#ifdef LINUX
extern int set_rt_priority(int argc, const char **argv);
#endif

extern int csoundErrCnt(CSOUND*);

static FILE *logFile = NULL;

static void msg_callback(CSOUND *csound,
                         int attr, const char *format, va_list args)
{
    (void) csound;
    if ((attr & CSOUNDMSG_TYPE_MASK) != CSOUNDMSG_REALTIME) {
      vfprintf(logFile, format, args);
      fflush(logFile);
      return;
     }
    #if defined(WIN32) || defined(MAC)
    switch (attr & CSOUNDMSG_TYPE_MASK) {
        case CSOUNDMSG_ERROR:
        case CSOUNDMSG_WARNING:
        case CSOUNDMSG_REALTIME:
        break;
      default:
        vfprintf(logFile, format, args);
        return;
    }
    #endif

    vfprintf(stderr, format, args);
}

static void nomsg_callback(CSOUND *csound,
  int attr, const char *format, va_list args){
  IGN(csound); IGN(attr);  IGN(format);  IGN(args);
}


#if defined(ANDROID) || (!defined(LINUX) && !defined(SGI) && \
                         !defined(__HAIKU__) && !defined(__BEOS__) && \
                         !defined(__MACH__) && !defined(__EMSCRIPTEN__))
static char *signal_to_string(int sig)
{
    switch(sig) {
#ifdef SIGHUP
    case SIGHUP:
      return "Hangup";
#endif
#ifdef SIGINT
    case SIGINT:
      return "Interrupt";
#endif
#ifdef SIGQUIT
    case SIGQUIT:
      return "Quit";
#endif
#ifdef SIGILL
    case SIGILL:
      return "Illegal instruction";
#endif
#ifdef SIGTRAP
    case SIGTRAP:
      return "Trace trap";
#endif
#ifdef SIGABRT
    case SIGABRT:
      return "Abort";
#endif
#ifdef SIGBUS
    case SIGBUS:
      return "BUS error";
#endif
#ifdef SIGFPE
    case SIGFPE:
      return "Floating-point exception";
#endif
#ifdef SIGUSR1
    case SIGUSR1:
      return "User-defined signal 1";
#endif
#ifdef SIGSEGV
    case SIGSEGV:
      return "Segmentation violation";
#endif
#ifdef SIGUSR2
    case SIGUSR2:
      return "User-defined signal 2";
#endif
#ifdef SIGPIPE
    case SIGPIPE:
      return "Broken pipe";
#endif
#ifdef SIGALRM
    case SIGALRM:
      return "Alarm clock";
#endif
#ifdef SIGTERM
    case SIGTERM:
      return "Termination";
#endif
#ifdef SIGSTKFLT
    case SIGSTKFLT:
      return "???";
#endif
#ifdef SIGCHLD
    case SIGCHLD:
      return "Child status has changed";
#endif
#ifdef SIGCONT
    case SIGCONT:
      return "Continue";
#endif
#ifdef SIGSTOP
    case SIGSTOP:
      return "Stop, unblockable";
#endif
#ifdef SIGTSTP
    case SIGTSTP:
      return "Keyboard stop";
#endif
#ifdef SIGTTIN
    case SIGTTIN:
      return "Background read from tty";
#endif
#ifdef SIGTTOU
    case SIGTTOU:
      return "Background write to tty";
#endif
#ifdef SIGURG
    case SIGURG:
      return "Urgent condition on socket ";
#endif
#ifdef SIGXCPU
    case SIGXCPU:
      return "CPU limit exceeded";
#endif
#ifdef SIGXFSZ
    case SIGXFSZ:
      return "File size limit exceeded ";
#endif
#ifdef SIGVTALRM
    case SIGVTALRM:
      return "Virtual alarm clock ";
#endif
#ifdef SIGPROF
    case SIGPROF:
      return "Profiling alarm clock";
#endif
#ifdef SIGWINCH
    case SIGWINCH:
      return "Window size change ";
#endif
#ifdef SIGIO
    case SIGIO:
      return "I/O now possible";
#endif
#ifdef SIGPWR
    case SIGPWR:
      return "Power failure restart";
#endif
    default:
      return "???";
    }
}

static void psignal(int sig, char *str)
{
    fprintf(stderr, "%s: %s\n", str, signal_to_string(sig));
}
#elif defined(__BEOS__)
static void psignal(int sig, char *str)
{
    fprintf(stderr, "%s: %s\n", str, strsignal(sig));
}
#endif

static CSOUND *_csound = NULL;
static int _result = 0;
static void signal_handler(int sig)
{
#if defined(SIGPIPE)
    if (sig == (int) SIGPIPE) {
      psignal(sig, "Csound ignoring SIGPIPE");
      return;
    }
#endif
    psignal(sig, "\ncsound command");
    if ((sig == (int) SIGINT || sig == (int) SIGTERM)) {
      if (_csound) {
        csoundStop(_csound);
        csoundDestroy(_csound);
      }
      //_result = -1;
      if (logFile != NULL)
        fclose(logFile);
      exit(1);
      //return;
    }
    exit(1);
}

static const int sigs[] = {
#if defined(LINUX) || defined(SGI) || defined(sol) || defined(__MACH__)
  SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGIOT, SIGBUS,
  SIGFPE, SIGSEGV, SIGPIPE, SIGTERM, SIGXCPU, SIGXFSZ,
#elif defined(WIN32)
  SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGTERM,
#elif defined(__EMX__)
  SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGFPE,
  SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGTERM, SIGCHLD,
#endif
  -1
};

static void install_signal_handler(void)
{
    unsigned int i;
    for (i = 0; sigs[i] >= 0; i++) {
      signal(sigs[i], signal_handler);
    }
}

int main(int argc, char **argv)
{
    CSOUND  *csound;
    char    *fname = NULL;
    int     i, result, errs, nomessages=0;
#ifdef GNU_GETTEXT
    const char* lang;
#endif
    install_signal_handler();
    csoundInitialize(CSOUNDINIT_NO_SIGNAL_HANDLER);

    /* set stdout to non buffering if not outputing to console window */
#if !defined(WIN32)
    if (!isatty(fileno(stdout))) {
      setvbuf(stdout, (char*) NULL, _IONBF, 0);
    }
#endif
#ifdef GNU_GETTEXT
    /* We need to set the locale for the translations to work */
    lang = csoundGetEnv(NULL, "CS_LANG");
    /* If set, use that. Otherwise use the system locale */
    if(lang == NULL)
        lang = setlocale(LC_MESSAGES, "");
    else
        lang = setlocale(LC_MESSAGES, lang);
    /* Should we warn if we couldn't set the locale (lang == NULL)? */
    /* If the strings for this binary are ever translated,
     * the textdomain should be set here */
#endif

    /* Real-time scheduling on Linux by Istvan Varga (Jan 6 2002) */
#ifdef LINUX
    if (set_rt_priority(argc, (const char **)argv) != 0)
      return -1;

#endif
    /* open log file if specified */
    for (i = 1; i < argc; i++) {
      if (strncmp(argv[i], "-O", 2) == 0 && (int) strlen(argv[i]) > 2)
        fname = argv[i] + 2;
      else if (strncmp(argv[i], "--logfile=", 10) == 0 &&
               (int) strlen(argv[i]) > 10)
        fname = argv[i] + 10;
      else if (i < (argc - 1) && strcmp(argv[i], "-O") == 0)
        fname = argv[i + 1];
    }
    if (fname != NULL) {
      if (!strcmp(fname, "NULL") || !strcmp(fname, "null"))
               nomessages = 1;
      else if ((logFile = fopen(fname, "w")) == NULL) {
        fprintf(stderr, "Error opening log file '%s': %s\n",
                        fname, strerror(errno));
        return -1;
      }
    }
    /* if logging to file, set message callback */
    if (logFile != NULL)
      csoundSetDefaultMessageCallback(msg_callback);
    else if (nomessages)
      csoundSetDefaultMessageCallback(nomsg_callback);

    /*  Create Csound. */
    csound = csoundCreate(NULL);
    _csound = csound;

    /*  One complete performance cycle. */
    result = csoundCompile(csound, argc, (const char **)argv);

     if (!result) result = csoundPerform(csound);
     //printf("**** result = %d\n", result);
     errs = csoundErrCnt(csound);
    /* delete Csound instance */
     csoundDestroy(csound);
     _csound = NULL;
    /* close log file */
    if (logFile != NULL)
      fclose(logFile);

    if (result == 0 && _result != 0) result = _result;
    //printf("csound returned with value: %d \n", result);
#if 0
    /* remove global configuration variables, if there are any */
    csoundDeleteAllGlobalConfigurationVariables();
#endif
    //printf("**** return %d\n",  (result >= 0 ? errs : -result));
    return (result >= 0 ? errs : -result);
}
