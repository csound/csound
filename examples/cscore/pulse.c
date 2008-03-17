/*  pulse.c: for cscore

    Copyright (C) 1991 Barry Vercoe

    This file is part of Csound.

    Csound is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "cscore.h"                                         /* CSCORE_PULSE.C */

/* program to apply interpretive durational pulse to an  */
/* existing score in 3/4 time, first beats on 0, 3, 6 .. */

static float four[4] = { 1.05, 0.97, 1.03, 0.95 };     /* pulse width for 4's */
static float three[3] = { 1.03, 1.05, .92 };           /* pulse width for 3's */

cscore(CSOUND* cs)              /* This example should be called from Csound  */
{
    EVLIST  *a, *b;
    EVENT  *e, **ep;
    float pulse16[4*4*4*4*3*4];    /* 16th-note array, 3/4 time, 256 measures */
    float acc16, acc1,inc1, acc3,inc3, acc12,inc12, acc48,inc48, acc192,inc192;
    float *p = pulse16;
    int  n16, n1, n3, n12, n48, n192;

    /* fill the array with interpreted ontimes  */
    for (acc192=0.,n192=0; n192<4; acc192+=192.*inc192,n192++)
      for (acc48=acc192,inc192=four[n192],n48=0; n48<4; acc48+=48.*inc48,n48++)
        for (acc12=acc48,inc48=inc192*four[n48],n12=0;n12<4; acc12+=12.*inc12,n12++)
          for (acc3=acc12,inc12=inc48*four[n12],n3=0; n3<4; acc3+=3.*inc3,n3++)
            for (acc1=acc3,inc3=inc12*four[n3],n1=0; n1<3; acc1+=inc1,n1++)
              for (acc16=acc1,inc1=inc3*three[n1],n16=0;
                   n16<4;
                   acc16+=.25*inc1*four[n16],n16++)
                *p++ = acc16;


    /* for (p = pulse16, n1 = 48; n1--; p += 4)  /* show vals & diffs */
    /*   printf("%g %g %g %g %g %g %g %g\n", *p, *(p+1), *(p+2), *(p+3),
    /*     *(p+1)-*p, *(p+2)-*(p+1), *(p+3)-*(p+2), *(p+4)-*(p+3)); */

    a = cscoreListGetSection(cs); /* read sect from tempo-warped score */
    b = cscoreListSeparateTWF(cs, a);            /* separate warp & fn statements */
    cscoreListPlay(cs, b);                       /* and send these to performance */
    a = cscoreListAppendStringEvent(cs, a, "s"); /* append a sect statement to note list */
    cscoreListPlay(cs, a); /* play the note-list without interpretation */
    for (ep = &a->e[1], n1 = a->nevents; n1--; ) { /* now pulse-modifiy it */
        e = *ep++;
        if (e->op == 'i') {
            e->p[2] = pulse16[(int)(4. * e->p2orig)];
            e->p[3] = pulse16[(int)(4. * (e->p2orig + e->p3orig))] - e->p[2];
        }
    }

    cscoreListPlay(cs, a);                       /* now play modified list */
    return;
}
