/*
    sched.c:

    Copyright (C) 2002-2005 Istvan Varga

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

#include "csound.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>

static  int     cpuMax = 0;
static  int     secs = 0;

static CS_NOINLINE int err_msg(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    return -1;
}

static CS_NOINLINE int parse_number(const char *s, int *nn)
{
    int n = 0, sign = 0, cnt = 0;

    if ((s[0] == '+' || s[0] == '-') && (s[1] >= '0' && s[1] <= '9')) {
      if (s[0] == '-')
        sign++;
      cnt++;
    }
    for ( ; s[cnt] >= '0' && s[cnt] <= '9'; cnt++)
      n = (n * 10) + ((int) s[cnt] - (int) '0');
    *nn = (sign ? -n : n);
    return cnt;
}

static int parse_sched_opt(const char *s, int *priority, int *cpuMax, int *secs)
{
    if (strcmp(s, "--sched") == 0) {
      *priority = (int) sched_get_priority_max(SCHED_RR);
      return 1;
    }
    if (strncmp(s, "--sched=", 8) == 0) {
      int   cnt;
      s += 8;
      cnt = parse_number(s, priority);
      if (cnt < 1)
        goto fmt_err;
      if (s[cnt] == '\0')
        return 1;
      if (s[cnt] != ',')
        goto fmt_err;
      s += (cnt + 1);
      cnt = parse_number(s, cpuMax);
      if (cnt < 1 || s[cnt] != ',')
        goto fmt_err;
      s += (cnt + 1);
      cnt = parse_number(s, secs);
      if (cnt >= 1 && s[cnt] == '\0')
        return 2;
 fmt_err:
      return err_msg("--sched: invalid format");
    }
    return 0;
}

static void *wd_thread_routine(void *dummy)
{
    uint32_t t0, t1;
    double   p;

    (void) dummy;
    for ( ; ; ) {
      t0 = (uint32_t) clock();
      csoundSleep((size_t) (secs * 1000));
      t1 = (uint32_t) clock();
      p = (double) ((int32_t) (t1 - t0)) * (100.0 / (double) CLOCKS_PER_SEC);
      if ((p / (double) secs) > (double) cpuMax) {
        kill(getpid(), SIGTERM); csoundSleep(1500);
        kill(getpid(), SIGKILL); csoundSleep(1500);
        exit(-1);
      }
    }
}

int set_rt_priority(int argc, const char **argv)
{
    int     rtmode;
    struct sched_param p;
    int     i, err, priority, wd_enabled = 0;
    //int dummy = 0;
    /* int     not_root; */

    /* if ((int) geteuid() == 0) */
    /*   not_root = 0; */
    /* else */
    /*   not_root = 1; */

    memset(&p, 0, sizeof(struct sched_param));
    priority = sched_get_priority_max(SCHED_RR);
    rtmode = 0;
    if (argc > 2) {
      for (i = 1; i <= (argc - 2); i++) {
        if (!(strcmp(argv[i], "-o")) &&                 /* check if input   */
            (!(strncmp(argv[i + 1], "dac", 3)) ||       /* or output is     */
             !(strncmp(argv[i + 1], "devaudio", 8))))   /* audio device     */
          rtmode |= 2;
        else if (!(strcmp(argv[i], "-i")) &&
            (!(strncmp(argv[i + 1], "adc", 3)) ||
             !(strncmp(argv[i + 1], "devaudio", 8))))
          rtmode |= 2;
      }
    }
    if (argc > 1) {
      for (i = 1; i <= (argc - 1); i++) {
        if (!(strncmp(argv[i], "-iadc", 5))) rtmode |= 2;
        else if (!(strncmp(argv[i], "-odac", 5))) rtmode |= 2;
        else if (!(strncmp(argv[i], "-idevaudio", 10))) rtmode |= 2;
        else if (!(strncmp(argv[i], "-odevaudio", 10))) rtmode |= 2;
        /* also check for --sched option, and -d */
        err = parse_sched_opt(argv[i], &priority, &cpuMax, &secs);
        if (err < 0)
          return -1;
        else if (err > 0) {
          if (priority < -20 || priority > sched_get_priority_max(SCHED_RR)) {
            err_msg("--sched: invalid priority value; the allowed range is:");
            err_msg("  -20 to -1: set nice level");
            err_msg("          0: normal scheduling, but lock memory");
            err_msg("    1 to %d: SCHED_RR with the specified priority "
                    "(DANGEROUS)", sched_get_priority_max(SCHED_RR));
            return -1;
          }
          rtmode |= 1;
          if (err > 1)
            wd_enabled = 1;
        }
        if (!(strcmp(argv[i], "-d"))) rtmode |= 4;
      }
    }

    if (rtmode != 7) {          /* all the above are required to enable */
      if (setuid(getuid()) < 0) {       /* error: give up root privileges */
        err_msg("csound: Could not drop privileges");
        return -1;
      }

      if (rtmode & 1) {
        err_msg("csound: --sched requires -d and either -o dac or -o devaudio");
        return -1;
      }
      return 0;
    }

/*     if (not_root) { */
/*       err_msg("WARNING: not running as root, --sched ignored"); */
/*       csoundSleep(1000); */
/*       return 0; */
/*     } */

#ifndef __FreeBSD__
    /* lock all pages into physical memory */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
      return err_msg("csound: cannot lock memory pages: %s", strerror(errno));
    }
#endif

    /* create watchdog thread if enabled */
    if (wd_enabled && priority > 0) {
      pthread_t th;
      if (cpuMax < 1 || cpuMax > 99)
        return err_msg("csound: --sched: invalid CPU percentage");
      if (secs < 1 || secs > 60)
        return err_msg("csound: --sched: invalid time interval");
      if (pthread_create(&th, (pthread_attr_t*) NULL, wd_thread_routine, NULL)
          != 0)
        return err_msg("csound: --sched: error creating watchdog thread");
      p.sched_priority = sched_get_priority_max(SCHED_RR);
      pthread_setschedparam(th, SCHED_RR, &p);
      pthread_detach(th);
    }

    /* set scheduling policy and priority */
    if (priority > 0) {
      p.sched_priority = priority;
      if (sched_setscheduler(0, SCHED_RR, &p) != 0) {
        return err_msg("csound: cannot set scheduling policy to SCHED_RR: %s",
                       strerror(errno));
      }
    }
    else if (priority == 0) {
      p.sched_priority = priority;
      sched_setscheduler(0, SCHED_OTHER, &p);   /* hope this does not fail ! */
    }
    else {
      /* nice requested */
      if (setpriority(PRIO_PROCESS, 0, priority) != 0) {
        return err_msg("csound: cannot set nice level to %d: %s",
                       priority, strerror(errno));
      }
    }
    /* give up root privileges */
    if (setuid(getuid()) < 0) {
      err_msg("csound: Could not drop privileges");
      return -1;
    }
    return 0;
}

