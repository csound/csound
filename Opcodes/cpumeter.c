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

// only available on Linux (no /proc/stat on OSX)
#if defined(LINUX)

#include "csoundCore.h"
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
        int     cnt, trig;
        FILE    *fp;
} CPUMETER;

/*
         * we preserve all cpu data in our CPU_t array which is organized
         * as follows:
         *    cpus[0] thru cpus[n] == tics for each separate cpu
         *    cpus[Cpu_tot]        == tics from the 1st /proc/stat line */


static int cpupercent_renew(CSOUND *csound, CPUMETER* p);

int cpupercent_init(CSOUND *csound, CPUMETER* p)
{
    char buf[SMLBUFSIZ];
    int k, num;
    TIC_t id, u, n, s, i, w, x, y, z;
    if (!(p->fp = fopen("/proc/stat", "r")))
      return
        csound->InitError(csound,
                          Str("Failed to open /proc/stat: %s"), strerror(errno));
    if (!fgets(buf, sizeof(buf), p->fp))
      csound->InitError(csound, Str("failed /proc/stat read"));
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
    p->cnt = (p->trig = (int)(*p->itrig * csound->GetSr(csound)));
    return k;
}

static int cpupercent_renew(CSOUND *csound, CPUMETER* p)
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
      return csound->PerfError(csound, p->h.insdshead,
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
        return csound->PerfError(csound, p->h.insdshead,
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
static int cpupercent(CSOUND *csound, CPUMETER* p)
{
    p->cnt -= CS_KSMPS;
    if (p->cnt< 0) {
      int n = cpupercent_renew(csound, p);
      p->cnt = p->trig;
      return n;
    }
    return OK;
}

typedef struct {
    OPDS   h;
    MYFLT  *ti;
} SYST;


static int systime(CSOUND *csound, SYST *p){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    *p->ti = round((ts.tv_sec + ts.tv_nsec/1.e9)*1000);
    return OK;
}




#define S(x)    sizeof(x)

static OENTRY cpumeter_localops[] = {
  { "cpumeter",   S(CPUMETER),   0,5, "kzzzzzzzz", "i",
    (SUBR)cpupercent_init, (SUBR)cpupercent, NULL   },
{ "systime", S(SYST),0, 3, "k",    "", (SUBR)systime, (SUBR)systime},
{ "systime", S(SYST),0, 1, "i",    "", (SUBR)systime}
};

LINKAGE_BUILTIN(cpumeter_localops)

#endif
