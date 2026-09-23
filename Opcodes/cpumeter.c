/* cpupercent
 *
 * Copyright *c) 2011 John ffitch, based heavily on code:
 * Copyright (c) 2002, by:      James C. Warner
 *    All rights reserved.      8921 Hilloway Road
 *                              Eden Prairie, Minnesota 55347 USA
 *                             <warnerjc@worldnet.att.net>
 *
 * This file may be used subject to the terms and conditions of the
 * GNU Library General Public License Version 2, or any later version
 * at your option, as published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 */

#ifndef WIN32

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>
#include <float.h>

#if defined(__MACH__) && !defined(LINUX)
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#endif

#define MAXCPUOUTPUTS 32

typedef struct {
    OPDS h;
    cs_float *kk[MAXCPUOUTPUTS], *itrig;
    cs_double cnt, trig;
#if defined(LINUX)
    FILE *fp;
    unsigned long long previous[MAXCPUOUTPUTS][8];
    unsigned char valid[MAXCPUOUTPUTS];
#elif defined(__MACH__)
    processor_info_array_t previous;
    mach_msg_type_number_t previous_count;
    natural_t previous_cpus;
#endif
} CPUMETER;

static int32_t deinit_cpupercent(CSOUND *csound, CPUMETER *p)
{
    IGN(csound);
#if defined(LINUX)
    if (p->fp) fclose(p->fp);
    p->fp = NULL;
#elif defined(__MACH__)
    if (p->previous)
      vm_deallocate(mach_task_self(), (vm_address_t)p->previous,
                    (vm_size_t)p->previous_count * sizeof(integer_t));
    p->previous = NULL;
    p->previous_count = p->previous_cpus = 0;
#endif
    return OK;
}

#if defined(LINUX)
static int32_t cpupercent_renew(CSOUND *csound, CPUMETER *p)
{
    char buf[512], label[32];
    unsigned char seen[MAXCPUOUTPUTS] = {0};
    uint32_t output;
    IGN(csound);
    rewind(p->fp);
    while (fgets(buf, sizeof(buf), p->fp)) {
      unsigned long long ticks[8] = {0};
      unsigned id;
      int field, fields;
      cs_double total = 0, idle = 0;
      if (strncmp(buf, "cpu", 3) != 0) break;
      fields = sscanf(buf, "%31s %llu %llu %llu %llu %llu %llu %llu %llu",
                      label, &ticks[0], &ticks[1], &ticks[2], &ticks[3],
                      &ticks[4], &ticks[5], &ticks[6], &ticks[7]);
      if (fields < 5) return NOTOK;
      if (strcmp(label, "cpu") == 0) output = 0;
      else {
        if (sscanf(label, "cpu%u", &id) != 1) return NOTOK;
        if (id >= p->OUTOCOUNT - 1u) continue;
        output = id + 1;
      }
      if (p->valid[output]) {
        for (field = 0; field < 8; ++field) {
          /* Linux counters can decrease, notably iowait. */
          cs_double delta = ticks[field] >= p->previous[output][field]
              ? (cs_double)(ticks[field] - p->previous[output][field]) : 0;
          total += delta;
          if (field == 3) idle = delta;
        }
      }
      *p->kk[output] = total > 0 ? 100.0 * (total - idle) / total : 0;
      memcpy(p->previous[output], ticks, sizeof(ticks));
      p->valid[output] = seen[output] = 1;
    }
    if (!seen[0] || ferror(p->fp)) return NOTOK;
    for (output = 1; output < p->OUTOCOUNT; ++output) {
      if (!seen[output]) {
        p->valid[output] = 0;
        *p->kk[output] = FL(0.0);
      }
    }
    return OK;
}
#elif defined(__MACH__)
static int32_t cpupercent_renew(CSOUND *csound, CPUMETER *p)
{
    processor_info_array_t info;
    mach_msg_type_number_t count;
    natural_t cpus;
    uint32_t output;
    cs_double total_all = 0, idle_all = 0;
    host_t host = mach_host_self();
    kern_return_t result = host_processor_info(host, PROCESSOR_CPU_LOAD_INFO,
                                             &cpus, &info, &count);
    mach_port_deallocate(mach_task_self(), host);
    if (result != KERN_SUCCESS) return NOTOK;
    if (cpus == 0 || cpus > count / CPU_STATE_MAX) {
      vm_deallocate(mach_task_self(), (vm_address_t)info,
                    (vm_size_t)count * sizeof(integer_t));
      return NOTOK;
    }
    for (output = 0; output < p->OUTOCOUNT; ++output)
      *p->kk[output] = FL(0.0);
    /* A changed CPU count needs a new baseline. */
    if (p->previous && p->previous_cpus == cpus) {
      natural_t cpu;
      for (cpu = 0; cpu < cpus; ++cpu) {
        cs_double total = 0, idle = 0;
        int state;
        for (state = 0; state < CPU_STATE_MAX; ++state) {
          size_t index = (size_t)cpu * CPU_STATE_MAX + state;
          /* Mach exposes wrapping unsigned 32-bit tick counters. */
          uint32_t delta = (uint32_t)info[index] -
                           (uint32_t)p->previous[index];
          total += delta;
          if (state == CPU_STATE_IDLE) idle = delta;
        }
        if (cpu + 1 < p->OUTOCOUNT)
          *p->kk[cpu + 1] = total > 0 ? 100.0 * (total - idle) / total : 0;
        total_all += total;
        idle_all += idle;
      }
      *p->kk[0] = total_all > 0 ?
          100.0 * (total_all - idle_all) / total_all : 0;
    }
    deinit_cpupercent(csound, p);
    p->previous = info;
    p->previous_count = count;
    p->previous_cpus = cpus;
    return OK;
}
#else
static int32_t cpupercent_renew(CSOUND *csound, CPUMETER *p)
{
    IGN(csound); IGN(p);
    return NOTOK;
}
#endif

static int32_t cpupercent_init(CSOUND *csound, CPUMETER *p)
{
    uint32_t output;
    deinit_cpupercent(csound, p);
    if (p->OUTOCOUNT == 0)
      return csound->InitError(csound, "%s", Str("cpumeter: no outputs"));
    p->cnt = p->trig = (cs_double)*p->itrig * CS_ESR;
    if (!(p->trig >= 0 && p->trig <= DBL_MAX))
      return csound->InitError(csound, "%s",
                              Str("cpumeter: invalid refresh interval"));
    for (output = 0; output < p->OUTOCOUNT; ++output)
      *p->kk[output] = FL(0.0);
#if defined(LINUX)
    memset(p->valid, 0, sizeof(p->valid));
    p->fp = fopen("/proc/stat", "r");
    if (!p->fp)
      return csound->InitError(csound, Str("Failed to open /proc/stat: %s"),
                              strerror(errno));
#endif
    if (cpupercent_renew(csound, p) != OK) {
      deinit_cpupercent(csound, p);
      return csound->InitError(csound, "%s", Str("cpumeter: cannot read CPU counters"));
    }
    return OK;
}

static int32_t cpupercent(CSOUND *csound, CPUMETER *p)
{
    p->cnt -= CS_KSMPS;
    if (p->cnt <= 0) {
      p->cnt = p->trig;
      if (cpupercent_renew(csound, p) != OK)
        return csound->PerfError(csound, &p->h, "%s",
                                Str("cpumeter: cannot read CPU counters"));
    }
    return OK;
}

typedef struct {
    OPDS   h;
    cs_float  *ti;
} SYST;

static int32_t
systime(CSOUND *csound, SYST *p){
    IGN(csound);
#if HAVE_CLOCK_GETTIME
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    *p->ti = round((ts.tv_sec + ts.tv_nsec/1.e9)*1000);
#else
    *p->ti = FL(0.0);
#endif
    return OK;
}

static OENTRY cpumeter_localops[] = {
  { "cpumeter",   sizeof(CPUMETER),   0, "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", "i",
    (SUBR)cpupercent_init, (SUBR)cpupercent, (SUBR)deinit_cpupercent },
{ "systime", sizeof(SYST),0,  "k",    "", (SUBR)systime, (SUBR)systime},
{ "systime", sizeof(SYST),0,  "i",    "", (SUBR)systime}
};

LINKAGE_BUILTIN(cpumeter_localops)


#endif
