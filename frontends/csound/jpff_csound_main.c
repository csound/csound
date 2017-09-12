/*
    csound_main.c:

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

/* Console Csound using the Csound API. */

#include "csound.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef GNU_GETTEXT
#include <locale.h>
#endif

#ifdef LINUX
extern int set_rt_priority(int argc, char **argv);
#endif

/*  static FILE *logFile = NULL; */

/* static void msg_callback(CSOUND *csound, */
/*                          int attr, const char *format, va_list args) */
/* { */
/*     (void) csound; */
/*     if ((attr & CSOUNDMSG_TYPE_MASK) != CSOUNDMSG_REALTIME) { */
/*       vfprintf(logFile, format, args); */
/*       fflush(logFile); */
/*       return; */
/*      } */
/*     #if defined(WIN32) || defined(MAC) */
/*     switch (attr & CSOUNDMSG_TYPE_MASK) { */
/*         case CSOUNDMSG_ERROR: */
/*         case CSOUNDMSG_WARNING: */
/*         case CSOUNDMSG_REALTIME: */
/*         break; */
/*       default: */
/*         vfprintf(logFile, format, args); */
/*         return; */
/*     } */
/*     #endif */

/*     vfprintf(stderr, format, args); */
/* } */

/* static void nomsg_callback(CSOUND *csound, */
/*   int attr, const char *format, va_list args){ return; } */

extern void do_logging(char *);
extern void end_logging(void);

int main(int argc, char **argv)
{
    CSOUND  *csound;
    char    *fname = NULL;
    int     i, result;
#ifdef GNU_GETTEXT
    const char* lang;
#endif

    /* set stdout to non buffering if not outputing to console window */
    if (!isatty(fileno(stdout))) {
#if !defined(WIN32)
      setvbuf(stdout, (char*) NULL, _IONBF, 0);
#endif
    }

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
    if (set_rt_priority(argc, argv) != 0)
      return -1;

#endif
    /* open log file if specified */
#if 0
    for (i = 1; i < argc; i++) {
      if (strncmp(argv[i], "-O", 2) == 0 && (int) strlen(argv[i]) > 2)
        fname = argv[i] + 2;
      else if (strncmp(argv[i], "--logfile=", 10) == 0 &&
               (int) strlen(argv[i]) > 10)
        fname = argv[i] + 10;
      else if (i < (argc - 1) && strcmp(argv[i], "-O") == 0)
        fname = argv[i + 1];
    }
    if (fname != NULL) { do_logging(fname); }
#endif
    /*  Create Csound. */
    csound = csoundCreate(NULL);

    /*  One complete performance cycle. */
    result = csoundCompile(csound, argc, argv);

     if(!result) csoundPerform(csound);
    /* delete Csound instance */
     csoundDestroy(csound);
#if 0
    /* close log file */
    end_logging();
#endif
#if 0
    /* remove global configuration variables, if there are any */
    csoundDeleteAllGlobalConfigurationVariables();
#endif
    return (result >= 0 ? 0 : result);
}
