/* Console Csound using the Csound API. */

#include "csound.h"
#include "csmodule.h"
#include <stdio.h>
#include <stdarg.h>

#if defined(LINUX)

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>

static int set_rt_priority(int argc, char **argv)
{
    int     rtmode;
    struct sched_param p;
    int     i, not_root, priority;

    if ((int) geteuid() == 0)
      not_root = 0;
    else
      not_root = 1;

    memset(&p, 0, sizeof(struct sched_param));
    priority = sched_get_priority_max(SCHED_RR);
    rtmode = 0;
    if (argc > 2) {
      for (i = 1; i <= (argc - 2); i++) {
        if (!(strcmp(argv[i], "-o")) &&                 /* check if input   */
            (!(strncmp(argv[i + 1], "dac", 3)) ||       /* or output is     */
             !(strncmp(argv[i + 1], "devaudio", 8))))   /* audio device     */
          rtmode |= 2;
        if (!(strcmp(argv[i], "-i")) &&
            (!(strncmp(argv[i + 1], "adc", 3)) ||
             !(strncmp(argv[i + 1], "devaudio", 8))))
          rtmode |= 2;
      }
    }
    if (argc > 1) {
      for (i = 1; i <= (argc - 1); i++) {
        if (!(strncmp(argv[i], "-iadc", 5))) rtmode |= 2;
        if (!(strncmp(argv[i], "-odac", 5))) rtmode |= 2;
        if (!(strncmp(argv[i], "-idevaudio", 10))) rtmode |= 2;
        if (!(strncmp(argv[i], "-odevaudio", 10))) rtmode |= 2;
        /* also check for --sched option, and -d */
        if (!(strcmp(argv[i], "--sched"))) rtmode |= 1;
        if (!(strncmp(argv[i], "--sched=", 8))) {
          /* priority value specified */
          priority = (int) atoi(&(argv[i][8]));
          if (priority < -20 || priority > sched_get_priority_max(SCHED_RR)) {
            fprintf(stderr, "--sched: invalid priority value: '%s'\n",
                            &(argv[i][8]));
            fprintf(stderr, "The allowed range is:\n");
            fprintf(stderr, "  -20 to -1: set nice level\n");
            fprintf(stderr, "          0: normal scheduling, "
                            "but lock memory\n");
            fprintf(stderr, "    1 to %d: SCHED_RR with the specified priority "
                            "(DANGEROUS)\n", sched_get_priority_max(SCHED_RR));
            return -1;
          }
          rtmode |= 1;
        }
        if (!(strcmp(argv[i], "-d"))) rtmode |= 4;
      }
    }

    if (rtmode != 7) {          /* all the above are required to enable */
      setuid(getuid());         /* error: give up root privileges */
      if (rtmode & 1) {
        fprintf(stderr, "csound: --sched requires -d and either -o dac ");
        fprintf(stderr, "or -o devaudio\n");
        return -1;
      }
      return 0;
    }

    if (not_root) {
      fprintf(stderr, "WARNING: not running as root, --sched ignored\n");
      return 0;
    }

#ifndef __FreeBSD__
    /* lock all pages into physical memory */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
      fprintf(stderr, "csound: cannot lock memory pages: %s\n",
                      strerror(errno));
      return -1;
    }
#endif

    /* set scheduling policy and priority */
    if (priority > 0) {
      p.sched_priority = priority;
      if (sched_setscheduler(0, SCHED_RR, &p) != 0) {
        fprintf(stderr, "csound: cannot set scheduling policy to "
                        "SCHED_RR: %s\n", strerror(errno));
        return -1;
      }
    }
    else if (priority == 0) {
      p.sched_priority = priority;
      sched_setscheduler(0, SCHED_OTHER, &p);   /* hope this does not fail ! */
    }
    else {
      /* nice requested */
      if (setpriority(PRIO_PROCESS, 0, priority) != 0) {
        fprintf(stderr, "csound: cannot set nice level to %d: %s\n",
                        priority, strerror(errno));
        return -1;
      }
    }
    /* give up root privileges */
    setuid(getuid());
    return 0;
}

#endif  /* LINUX */

int main(int argc, char **argv)
{
    ENVIRON *csound;
    int     result;
    /* set stdout to non buffering if not outputing to console window */
    if (!isatty(fileno(stdout))) {
        setvbuf(stdout, (char *)NULL, _IONBF, 0);
    }
    /* Real-time priority on Linux by Istvan Varga (Jan 6 2002) */
    /* This function is called before anything else to avoid    */
    /* running "normal" Csound code with setuid root.           */
    /* set_rt_priority gives up root privileges after setting   */
    /* priority and locking memory; init_getstring () and other */
    /* functions below will be run as a normal user (unless     */
    /* Csound was actually run as root, and not setuid root).   */
#if defined(LINUX)
    if (set_rt_priority(argc, argv) != 0)
      return -1;
#endif

    /* initialise Csound library */
    csoundInitialize(&argc, &argv, 0);

    /*  Create Csound. */
    csound = csoundCreate(NULL);

    /*  One complete performance cycle. */
    result = csoundCompile(csound, argc, argv);
    if (!result) {
      if (csoundGetOutputBufferSize(csound)
            <= (csoundGetKsmps(csound) * csoundGetNchnls(csound))) {
        /* do not need csoundYield(), kperf() will call it */
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
    /* remove global configuration variables, if there are any */
    csoundDeleteAllGlobalConfigurationVariables();

    return (result >= 0 ? 0 : result);
}

