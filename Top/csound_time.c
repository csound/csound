/*
    csound_time.c: engine time

    Copyright (C) 2025 The Csound Developers

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/
#include "csoundCore.h"

#ifdef HAVE_GETTIMEOFDAY
#undef HAVE_GETTIMEOFDAY
#endif
#if defined(LINUX) || defined(__unix) || defined(__unix__) || defined(__MACH__)
#define HAVE_GETTIMEOFDAY 1
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#include <windows.h>
#endif
#include <time.h>


/* enable use of high resolution timer (Linux/i586/GCC only) */
/* could in fact work under any x86/GCC system, but do not   */
/* know how to query the actual CPU frequency ...            */

// #define HAVE_RDTSC  1

/* ------------------------------------ */

#if defined(HAVE_RDTSC)
#if !(defined(LINUX) && defined(__GNUC__) && defined(__i386__))
#undef HAVE_RDTSC
#endif
#endif

/* hopefully cannot change during performance */
static double timeResolutionSeconds = -1.0;

/* find out CPU frequency based on /proc/cpuinfo */
int32_t get_time_resolution(void) {
#if defined(HAVE_RDTSC)
  FILE *f;
  char buf[256];

  /* if frequency is not known yet */
  f = fopen("/proc/cpuinfo", "r");
  if (UNLIKELY(f == NULL)) {
    fprintf(stderr, Str("Cannot open /proc/cpuinfo. "
                        "Support for RDTSC is not available.\n"));
    return -1;
  }
  /* find CPU frequency */
  while (fgets(buf, 256, f) != NULL) {
    int32_t i;
    char *s = (char *)buf - 1;
    buf[255] = '\0'; /* safety */
    if (strlen(buf) < 9)
      continue; /* too short, skip */
    while (*++s != '\0')
      if (isupper(*s))
        *s = tolower(*s); /* convert to lower case */
    if (strncmp(buf, "cpu mhz", 7) != 0)
      continue;           /* check key name */
    s = strchr(buf, ':'); /* find frequency value */
    if (s == NULL)
      continue; /* invalid entry */
    do {
      s++;
    } while (isblank(*s)); /* skip white space */
    i = CS_SSCANF(s, "%lf", &timeResolutionSeconds);

    if (i < 1 || timeResolutionSeconds < 1.0) {
      timeResolutionSeconds = -1.0; /* invalid entry */
      continue;
    }
  }
  fclose(f);
  if (UNLIKELY(timeResolutionSeconds <= 0.0)) {
    fprintf(stderr, Str("No valid CPU frequency entry "
                        "was found in /proc/cpuinfo.\n"));
    return -1;
  }
  /* MHz -> seconds */
  timeResolutionSeconds = 0.000001 / timeResolutionSeconds;
#elif defined(WIN32)
  LARGE_INTEGER tmp1;
  int_least64_t tmp2;
  QueryPerformanceFrequency(&tmp1);
  tmp2 = (int_least64_t)tmp1.LowPart + ((int_least64_t)tmp1.HighPart << 32);
  timeResolutionSeconds = 1.0 / (double)tmp2;
#elif defined(HAVE_GETTIMEOFDAY)
  timeResolutionSeconds = 0.000001;
#else
  timeResolutionSeconds = 1.0;
#endif
#ifdef CHECK_TIME_RESOLUTION // BETA
  fprintf(stderr, "time resolution is %.3f ns\n",
          1.0e9 * timeResolutionSeconds);
#endif
  return 0;
}

/* function for getting real time */

static inline int_least64_t get_real_time(void) {
#if defined(HAVE_RDTSC)
  /* optimised high resolution timer for Linux/i586/GCC only */
  uint32_t l, h;
#ifndef __STRICT_ANSI__
  asm volatile("rdtsc" : "=a"(l), "=d"(h));
#else
  __asm__ volatile("rdtsc" : "=a"(l), "=d"(h));
#endif
  return ((int_least64_t)l + ((int_least64_t)h << 32));
#elif defined(WIN32)
  /* Win32: use QueryPerformanceCounter - resolution depends on system, */
  /* but is expected to be better than 1 us. GetSystemTimeAsFileTime    */
  /* seems to have much worse resolution under Win95.                   */
  LARGE_INTEGER tmp;
  QueryPerformanceCounter(&tmp);
  return ((int_least64_t)tmp.LowPart + ((int_least64_t)tmp.HighPart << 32));
#elif defined(HAVE_GETTIMEOFDAY)
  /* UNIX: use gettimeofday() - allows 1 us resolution */
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((int_least64_t)tv.tv_usec +
          (int_least64_t)((uint32_t)tv.tv_sec * (uint64_t)1000000));
#else
  /* other systems: use time() - allows 1 second resolution */
  return ((int_least64_t)time(NULL));
#endif
}

/* function for getting CPU time */

static inline int_least64_t get_CPU_time(void) {
  return ((int_least64_t)((uint32_t)clock()));
}

/* initialise a timer structure */

void csoundInitTimerStruct(RTCLOCK *p) {
  p->starttime_real = get_real_time();
  p->starttime_CPU = get_CPU_time();
}

/**
 * return the elapsed real time (in seconds) since the specified timer
 * structure was initialised
 */
double csoundGetRealTime(RTCLOCK *p) {
  return ((double)(get_real_time() - p->starttime_real) *
          (double)timeResolutionSeconds);
}

/**
 * return the elapsed CPU time (in seconds) since the specified timer
 * structure was initialised
 */
double csoundGetCPUTime(RTCLOCK *p) {
  return ((double)((uint32_t)get_CPU_time() - (uint32_t)p->starttime_CPU) *
          (1.0 / (double)CLOCKS_PER_SEC));
}

/* return a 32-bit unsigned integer to be used as seed from current time */

uint32_t csoundGetRandomSeedFromTime(void) {
  return (uint32_t)get_real_time();
}
