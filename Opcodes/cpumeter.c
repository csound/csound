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

// only available on Linux (no /proc/stat on OSX)
#if defined(LINUX)
/*######  Miscellaneous global stuff  ####################################*/
#define SMLBUFSIZ (512)
#define TEST (0)

typedef unsigned long long TIC_t;
typedef          long long SIC_t;

typedef struct CPU_t {
   TIC_t u, n, s, i, w, x, y, z; // as represented in /proc/stat
   TIC_t u_sav, n_sav, s_sav, i_sav,
         w_sav, x_sav, y_sav, z_sav;
   unsigned id;  // the CPU ID number
} CPU_t;

typedef struct {
        OPDS    h;
        MYFLT   *k0, *kk[8], *itrig;
        AUXCH   cpu_a;
        CPU_t   *cpus;
        uint32_t cpu_max;
        int32_t     cnt, trig;
        FILE    *fp;
} CPUMETER;

/*
         * we preserve all cpu data in our CPU_t array which is organized
         * as follows:
         *    cpus[0] thru cpus[n] == tics for each separate cpu
         *    cpus[Cpu_tot]        == tics from the 1st /proc/stat line */

int32_t deinit_cpupercent(CSOUND *csound, void *pdata)
{
    CPUMETER *p = (CPUMETER *) pdata;
    fclose(p->fp);
    return OK;
}

static int32_t cpupercent_renew(CSOUND *csound, CPUMETER* p);

int32_t cpupercent_init(CSOUND *csound, CPUMETER* p)
{
    char buf[SMLBUFSIZ];
    int32_t k, num;
    TIC_t id, u, n, s, i, w, x, y, z;
    if (!(p->fp = fopen("/proc/stat", "r")))
      return
        csound->InitError(csound,
                          Str("Failed to open /proc/stat: %s"), strerror(errno));
    if (!fgets(buf, sizeof(buf), p->fp))
      return csound->InitError(csound, Str("failed /proc/stat read"));
    num = sscanf(buf, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
                 &u, &n, &s, &i, &w, &x, &y, &z);
    for (k = 0; ; k++) {
      if (!fgets(buf, SMLBUFSIZ, p->fp))
        return csound->InitError(csound,Str("failed /proc/stat read"));
      num = sscanf(buf, "cpu%llu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
                   &id, &u, &n, &s, &i, &w, &x, &y, &z);
      if (num<4) break;
    }
    p->cpu_max = k-1;
    csound->AuxAlloc(csound,k*sizeof(CPU_t), &(p->cpu_a));
    p->cpus = (CPU_t *) p->cpu_a.auxp;
    k = cpupercent_renew(csound, p);
    p->cnt = (p->trig = (int32_t)(*p->itrig * CS_ESR));
    return k;
}

static int32_t cpupercent_renew(CSOUND *csound, CPUMETER* p)
{
#define TRIMz(x)  ((tz = (SIC_t)(x)) < 0 ? 0 : tz)
    SIC_t u_frme, s_frme, n_frme, i_frme,
      w_frme, x_frme, y_frme, z_frme, tot_frme, tz;
    double scale;
    uint32_t k;
    CPU_t *cpu = p->cpus;
    char buf[SMLBUFSIZ];

    rewind(p->fp);
    fflush(p->fp);
    k = p->cpu_max;
    if (!fgets(buf, SMLBUFSIZ, p->fp))
      return csound->PerfError(csound, &(p->h),
                               Str("failed /proc/stat read"));
    /*num = */sscanf(buf, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
                     &cpu[k].u, &cpu[k].n, &cpu[k].s, &cpu[k].i,
                     &cpu[k].w, &cpu[k].x, &cpu[k].y, &cpu[k].z);
    u_frme = cpu[k].u - cpu[k].u_sav;
    s_frme = cpu[k].s - cpu[k].s_sav;
    n_frme = cpu[k].n - cpu[k].n_sav;
    i_frme = TRIMz(cpu[k].i - cpu[k].i_sav);
    w_frme = cpu[k].w - cpu[k].w_sav;
    x_frme = cpu[k].x - cpu[k].x_sav;
    y_frme = cpu[k].y - cpu[k].y_sav;
    z_frme = cpu[k].z - cpu[k].z_sav;
    tot_frme = u_frme + s_frme + n_frme + i_frme +
               w_frme + x_frme + y_frme + z_frme;
    if (tot_frme < 1) tot_frme = 1;
    scale = 100.0 / (double)tot_frme;
    *p->k0 = 100.0-(double)i_frme * scale;
    if (TEST)
      printf("**%5.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n",
             (double)u_frme * scale,
             (double)s_frme * scale,
             (double)n_frme * scale,
             (double)i_frme * scale,
             (double)w_frme * scale,
             (double)x_frme * scale,
             (double)y_frme * scale,
             (double)z_frme * scale
             );
    // remember for next time around
    cpu[k].u_sav = cpu[k].u;
    cpu[k].s_sav = cpu[k].s;
    cpu[k].n_sav = cpu[k].n;
    cpu[k].i_sav = cpu[k].i;
    cpu[k].w_sav = cpu[k].w;
    cpu[k].x_sav = cpu[k].x;
    cpu[k].y_sav = cpu[k].y;
    cpu[k].z_sav = cpu[k].z;

    for (k=0; k<p->cpu_max && k+1<p->OUTOCOUNT; k++) {
      if (!fgets(buf, SMLBUFSIZ, p->fp))
        return csound->PerfError(csound, &(p->h),
                                 Str("failed /proc/stat read"));
      /*num = */ (void)sscanf(buf, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
                              &cpu[k].u, &cpu[k].n, &cpu[k].s, &cpu[k].i,
                              &cpu[k].w, &cpu[k].x, &cpu[k].y, &cpu[k].z);
      u_frme = cpu[k].u - cpu[k].u_sav;
      s_frme = cpu[k].s - cpu[k].s_sav;
      n_frme = cpu[k].n - cpu[k].n_sav;
      i_frme = TRIMz(cpu[k].i - cpu[k].i_sav);
      w_frme = cpu[k].w - cpu[k].w_sav;
      x_frme = cpu[k].x - cpu[k].x_sav;
      y_frme = cpu[k].y - cpu[k].y_sav;
      z_frme = cpu[k].z - cpu[k].z_sav;
      tot_frme = u_frme + s_frme + n_frme + i_frme +
                 w_frme + x_frme + y_frme + z_frme;
      if (tot_frme < 1) tot_frme = 1;
      scale = 100.0 / (double)tot_frme;
      //if (p->kk[k]==NULL) break;
      *p->kk[k] = 100.0-i_frme * scale;
      if (TEST)
        printf("%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n",
               (double)u_frme * scale,
               (double)s_frme * scale,
               (double)n_frme * scale,
               (double)i_frme * scale,
               (double)w_frme * scale,
               (double)x_frme * scale,
               (double)y_frme * scale,
               (double)z_frme * scale);

      // remember for next time around
      cpu[k].u_sav = cpu[k].u;
      cpu[k].s_sav = cpu[k].s;
      cpu[k].n_sav = cpu[k].n;
      cpu[k].i_sav = cpu[k].i;
      cpu[k].w_sav = cpu[k].w;
      cpu[k].x_sav = cpu[k].x;
      cpu[k].y_sav = cpu[k].y;
      cpu[k].z_sav = cpu[k].z;
      *p->kk[k] = 100.0-i_frme * scale;
    }
    return OK;
#undef TRIMz
}
static int32_t cpupercent(CSOUND *csound, CPUMETER* p)
{
    p->cnt -= CS_KSMPS;
    if (p->cnt< 0) {
      int32_t n = cpupercent_renew(csound, p);
      p->cnt = p->trig;
      return n;
    }
    return OK;
}

#else
typedef struct {
        OPDS    h;
        MYFLT   *k0, *kk[8], *itrig;
} CPUMETER;


int32_t deinit_cpupercent(CSOUND *csound, void *p)
{
   IGN(p);
  csound->Message(csound, "not implemented\n");
    return OK;
}


int32_t cpupercent_init(CSOUND *csound, CPUMETER *p) {
   IGN(p);
  csound->Message(csound, "not implemented\n");
  return OK;
}

int32_t cpupercent(CSOUND *c, CPUMETER *p) {
  IGN(c);
  IGN(p);
  return OK;
}
#endif

typedef struct {
    OPDS   h;
    MYFLT  *ti;
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

#define S(x)    sizeof(x)

static OENTRY cpumeter_localops[] = {
  { "cpumeter",   S(CPUMETER),   0, "kzzzzzzzz", "i",
    (SUBR)cpupercent_init, (SUBR)cpupercent, (SUBR) deinit_cpupercent },
{ "systime", S(SYST),0,  "k",    "", (SUBR)systime, (SUBR)systime},
{ "systime", S(SYST),0,  "i",    "", (SUBR)systime}
};

LINKAGE_BUILTIN(cpumeter_localops)
#endif

