/*
    sequencer.c:

    Copyright (C) 2021    John ffitch

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

#include "csoundCore.h"
#include <math.h>

typedef struct {
    OPDS        h;
    MYFLT       *res;           /*  state */
    ARRAYDAT    *riff;          /* initial note row */
    ARRAYDAT    *instr;         /* renderers for each note */
    ARRAYDAT    *data;          /* extra data for pitch info */
    MYFLT       *kbpm;          /* speed of sequence */
    MYFLT       *klen;          /* Length of sequece to use */
    MYFLT       *mode;          /* Mode; -1 backward,
                                   0 loop frward;
                                   +ve mutate
                                   -2 pause
                                   -3 random
                                */
    MYFLT       *verbos;
 // Internals
    int32_t     max_length;
    int         cnt;            /* Count loops for mi=uator */
    int         next;           /* net step nuber */
    int         time;           /* time in samples to nxt step */
    int         seq[64];
} SEQ;

static int32_t sequencer_init(CSOUND *csound, SEQ *p)
{
    int i;
    p->max_length = p->riff->sizes[0];
    if (p->max_length != p->instr->sizes[0] ||
        p->max_length != p->data->sizes[0] ||
        p->max_length >= 64) {
            return csound->InitError(csound, Str("Format error"));
    }
    p->time = 0;
    p->next = 0;
    p->cnt = 1;
    for (i = 0; i<p->riff->sizes[0]; i++)
      p->seq[i] = i;
    /* for (i=0; i<p->riff->sizes[0]; i++) */
    /*   printf("%d: %d %f\n", i, (int)(p->instr->data[i]), p->riff->data[i]);b */
    return OK;
}


static int32_t sequencer(CSOUND *csound, SEQ *p)
{
    int len = (int)*p->klen;
    int i = p->next;
    int mode = (int)*p->mode;

    if (len<=0) len = 1;
    if (len>=p->max_length) len= p->max_length;
    if (p->time > 0) {         /* Not yet time to act */
      p->time -= csound->ksmps;
      *p->res = -FL(1.0);
      return OK;
    }
    /* Time for an event */
    // First check mode for next state
    if (i >= len && mode >= 0) { // End of cycle
      i = p->next = 0;
    }
    else if (i < 0 && mode == -1) {
      p->next = i = len-1;
    }
    else if (i>=len && mode == -2) {
      *p->res = -FL(1.0);
      return OK;
    }
    else if (mode == -3) i = rand()%len;
    {

      MYFLT inst = p->instr->data[p->seq[i]];
      if (inst != 0) {
        char buff[30];
        sprintf(buff, "i %0.2f 0 %f %f\n",
                inst, 60.0/(*p->kbpm)*p->riff->data[p->seq[i]],
                p->data->data[p->seq[i]]);
        //printf("%s", buff);
        csoundReadScore(csound, buff);
      }
      p->time = (p->riff->data[i] * csound->esr * 60.0) / *p->kbpm;
      if (*p->verbos)
        printf("Step %d riff %d instr %d len %f\n",
               i,p->seq[i], (int)(p->instr->data[p->seq[i]]), p->riff->data[p->seq[i]]);
      // Mutate every mode events
      if (mode > 0 && len>1 && p->cnt%mode == 0) {
        int r1, r2;
        do {
          r1 = rand()%len;
          r2 = rand()%len;
        } while (r1==r2);
        {
          int tm = p->seq[r1];
          p->seq[r1] = p->seq[r2];
          p->seq[r2] = tm;
          if (*p->verbos)
            printf("swap %d and %d\n", r1, r2);
        }
      }
      *p->res = (MYFLT)i;
      if (*p->mode >=0) p->next++;
      else if (mode == -1) p->next--;
      if (mode != -2) p->cnt++;
    }
    if (*p->verbos)
      printf("Next Step %d time = %d samples\n", p->next, p->time);
    return OK;
    }

static OENTRY sequencer_localops[] =
  {
   { "sequ", sizeof(SEQ), 0, 3, "k",
     "i[]i[]i[]kkOo",
     (SUBR) sequencer_init, (SUBR) sequencer
  },
};

LINKAGE_BUILTIN(sequencer_localops)
