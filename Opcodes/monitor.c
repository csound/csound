/*
    monitor.c:

    Copyright (C) 2006 Istvan Varga

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

#include "csdl.h"

static int notinit_opcode_stub_perf(CSOUND *csound, void *p)
{
    return csound->PerfError(csound, Str("%s: not initialised"),
                                     csound->GetOpcodeName(p));
}

typedef struct MONITOR_OPCODE_ {
    OPDS    h;
    MYFLT   *ar[24];
} MONITOR_OPCODE;

static int monitor_opcode_perf(CSOUND *csound, MONITOR_OPCODE *p)
{
    int     i, j;

    if (csound->spoutactive) {
      int   k = 0;
      i = 0;
      do {
        j = 0;
        do {
          p->ar[j][i] = csound->spout[k++];
        } while (++j < csound->nchnls);
      } while (++i < csound->ksmps);
    }
    else {
      j = 0;
      do {
        i = 0;
        do {
          p->ar[j][i] = FL(0.0);
        } while (++i < csound->ksmps);
      } while (++j < csound->nchnls);
    }
    return OK;
}

static int monitor_opcode_init(CSOUND *csound, MONITOR_OPCODE *p)
{
    if (csound->GetOutputArgCnt(p) != csound->nchnls)
      return csound->InitError(csound, Str("number of arguments != nchnls"));
    p->h.opadr = (SUBR) monitor_opcode_perf;
    return OK;
}

 /* ------------------------------------------------------------------------ */

static OENTRY localops[] = {
  { "monitor",  sizeof(MONITOR_OPCODE), 3,  "mmmmmmmmmmmmmmmmmmmmmmmm", "",
    (SUBR) monitor_opcode_init, (SUBR) notinit_opcode_stub_perf,  (SUBR) NULL }
};

LINKAGE

