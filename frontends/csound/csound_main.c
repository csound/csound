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

#ifdef LINUX
extern int set_rt_priority(int argc, char **argv);
#endif

static FILE *logFile = NULL;

static void msg_callback(CSOUND *csound,
                         int attr, const char *format, va_list args)
{
    (void) csound;
    if ((attr & CSOUNDMSG_TYPE_MASK) != CSOUNDMSG_REALTIME) {
      vfprintf(logFile, format, args);
      fflush(logFile);
    }
#if defined(WIN32) || defined(MAC)
    switch (attr & CSOUNDMSG_TYPE_MASK) {
      case CSOUNDMSG_ERROR:
      case CSOUNDMSG_WARNING:
      case CSOUNDMSG_REALTIME:
        break;
      default:
        vfprintf(stdout, format, args);
        return;
    }
#endif
    vfprintf(stderr, format, args);
}

int main(int argc, char **argv)
{
    CSOUND  *csound;
    char    *fname = NULL;
    int     i, result;

    /* set stdout to non buffering if not outputing to console window */
    if (!isatty(fileno(stdout))) {
      setvbuf(stdout, (char*) NULL, _IONBF, 0);
    }

    /* Real-time scheduling on Linux by Istvan Varga (Jan 6 2002) */
#ifdef LINUX
    if (set_rt_priority(argc, argv) != 0)
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
      if ((logFile = fopen(fname, "a")) == NULL) {
        fprintf(stderr, "Error opening log file '%s': %s\n",
                        fname, strerror(errno));
        return -1;
      }
    }

    /* initialise Csound library */
    csoundInitialize(&argc, &argv, 0);

    /*  Create Csound. */
    csound = csoundCreate(NULL);

    /* if logging to file, set message callback */
    if (logFile != NULL)
      csoundSetMessageCallback(csound, msg_callback);

    /*  One complete performance cycle. */
    result = csoundCompile(csound, argc, argv);
    if (!result) {
      if (csoundGetOutputBufferSize(csound)
            <= (csoundGetKsmps(csound) * csoundGetNchnls(csound))) {
        while ((result = csoundPerformKsmps(csound)) == 0)
          ;
      }
      else {
        while ((result = csoundPerformBuffer(csound)) == 0)
          ;
      }
    }
    /* delete Csound instance */
    csoundDestroy(csound);
    /* close log file */
    if (logFile != NULL)
      fclose(logFile);
    /* remove global configuration variables, if there are any */
    csoundDeleteAllGlobalConfigurationVariables();

    return (result >= 0 ? 0 : result);
}

