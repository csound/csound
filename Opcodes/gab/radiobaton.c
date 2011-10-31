/*  Copyright (C) 2007 Gabriel Maldonado

  Csound is free software; you can redistribute it
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

/* #include "csdl.h" */

/*
extern unsigned long MIDIINbufIndex;
extern MIDIMESSAGE  MIDIINbuffer2[];
*/

#include "csoundCore.h"  /* instead of "csd.h" in order to access
                            csound->midiGlobals */

/* These opcodes have not been implemented because csound->midiGlobals
   is not there yet. */

PUBLIC int Sched(CSOUND *csound, MYFLT  *args[], int numargs);


/* -------------------------------------------------------------------- */

typedef struct {
    OPDS   h;
    MYFLT  *x1, *y1, *z1, *x2, *y2, *z2;

    int    local_buf_index;
} RB_XYZ;


static int rbatonXYZ_set(CSOUND *csound, RB_XYZ *p)
{
    p->local_buf_index = csound->midiGlobals->MIDIINbufIndex & MIDIINBUFMSK;
    *p->x1 = *p->y1 = *p->z1 = *p->x2 = *p->y2 = *p->z2 = 0;
    return OK;
}

static int rbatonXYZ(CSOUND *csound, RB_XYZ *p)
{
    int status, data1, data2;
    MGLOBAL* mg = csound->midiGlobals;
    /*
      if      (p->local_buf_index < mg->MIDIINbufIndex) {
      MIDIMESSAGE temp = mg->MIDIINbuffer2[(p->local_buf_index)++ % MIDIINBUFMAX];
      status =  temp.bData[0];
      data1  =  temp.bData[1];
      data2  =  temp.bData[2];
    */
    if  (p->local_buf_index != mg->MIDIINbufIndex) {
      unsigned char *temp;
      temp = &(mg->MIDIINbuffer2[p->local_buf_index++].bData[0]);
      p->local_buf_index &= MIDIINBUFMSK;
      status =  (*temp );
      data1  =  *++temp;
      data2  =  *++temp;
    }
    else return OK;
    if (status == 0xA0) {
      switch (data1) {
      case 8:  *p->x1 = data2/FL(127.0); break;
      case 9:  *p->y1 = data2/FL(127.0); break;
      case 10: *p->z1 = data2/FL(127.0); break;
      case 11: *p->x2 = data2/FL(127.0); break;
      case 12: *p->y2 = data2/FL(127.0); break;
      case 13: *p->z2 = data2/FL(127.0); break;
      }
    }
    return OK;
}
/* -------------------------------------------------------------------- */

typedef struct {
    OPDS   h;
    MYFLT  *pot1, *pot2, *pot3, *pot4, *fsw1, *fsw2, *button;
    int    local_buf_index;
} RB_POT;

static int rbatonPot_set(CSOUND *csound, RB_POT *p)
{
    p->local_buf_index = csound->midiGlobals->MIDIINbufIndex & MIDIINBUFMSK;
    *p->pot1 = *p->pot2 = *p->pot3 = *p->pot4 = 0;
    return OK;
}

static int rbatonPot (CSOUND *csound, RB_POT *p)
{
    int status, data1, data2;
    MGLOBAL* mg = csound->midiGlobals;
    if (p->local_buf_index != csound->midiGlobals->MIDIINbufIndex) {
      unsigned char *temp;
      temp = &(mg->MIDIINbuffer2[p->local_buf_index++].bData[0]);
      p->local_buf_index &= MIDIINBUFMSK;
      status =  (*temp );
      data1  =  *++temp;
      data2  =  *++temp;
    }
    else return OK;
    if (status == 0xA0) {
      switch (data1) {
      case 4: *p->pot1 = data2/FL(127.0); break;
      case 5: *p->pot2 = data2/FL(127.0); break;
      case 6: *p->pot3 = data2/FL(127.0); break;
      case 7: *p->pot4 = data2/FL(127.0); break;
      case 3:
        switch (data2) {
        case 1: *p->button = (*p->button == 0) ? FL(1.0) : FL(0.0);     break;
        case 2: *p->fsw1 = 1; break;
        case 3: *p->fsw1 = 0; break;
        case 4: *p->fsw2 = 1; break;
        case 5: *p->fsw2 = 0; break;
        }
        break;
      }
    }
    return OK;
}
/* -------------------------------------------------------------------- */

#define INCR (0.001f)

typedef struct {
    OPDS   h;
    MYFLT  *kDur, *kBaton1instr, *kBaton2Instr, *kFootSw1dn, *kFootSw1up,
           *kFootSw2dn, *kFootSw2up, *kButInstr;
    MYFLT  baton, whack, x, y, p0, p2, p3, p3neg;
    MYFLT  fs1dn, fs1up, fs2dn, fs2up;
    MYFLT  frac1, frac2;
    MYFLT  pot1, pot2, pot3; /* pontentiometer 4 is reserved to stick sensitivity */
    int    local_buf_index;
} RB_PERCPAD;

static int rbatonPercPad_set (CSOUND *csound, RB_PERCPAD *p)
{
    p->local_buf_index = csound->midiGlobals->MIDIINbufIndex & MIDIINBUFMSK;
    p->p0 = 0;
    p->p2 = 0;
    p->p3 = 1;
    p->p3neg = -1;
    p->pot1 = p->pot2 = p->pot3 = 0;
    p->fs1dn = -*p->kFootSw1dn;
    p->fs2dn = -*p->kFootSw2dn;
    p->fs1up = -*p->kFootSw1up;
    p->fs2up = -*p->kFootSw2up;
    p->frac1 = p->frac2 = 0;
    return OK;
}

static int rbatonPercPad(CSOUND *csound, RB_PERCPAD *p)
{
    int status, data1, data2;
    MYFLT *args[9];

    if (p->local_buf_index != csound->midiGlobals->MIDIINbufIndex) {
      MGLOBAL* mg = csound->midiGlobals;
      unsigned char *temp;
      temp = &(mg->MIDIINbuffer2[p->local_buf_index++].bData[0]);
      p->local_buf_index &= MIDIINBUFMSK;
      status =  (*temp );
      data1  =  *++temp;
      data2  =  *++temp;
    }
    else return OK;
    if (status == 0xA0) {
      switch (data1) {
      case 1: /* trigger baton and whack from stick 1 or 2 */
      case 2:
        p->baton = (data1 == 1) ? *p->kBaton1instr : *p->kBaton2Instr;
        p->whack = data2/FL(127.0);
        break;
      case 15: /* X coordinate at trigger */
      case 17:
        p->x = data2/FL(127.0);
        break;

      case 16: /* Y coordinate at trigger */
      case 18:
        p->y = data2/FL(127.0);
        args[0] = &(p->p0);                     /* this is the third baton trigger message */
        args[1] = &(p->baton);          /* so activate corresponding instrument */
        args[2] = &(p->p2);
        args[3] = &(*p->kDur);
        args[4] = &(p->whack);
        args[5] = &(p->x);
        args[6] = &(p->y);
        args[7] = &(p->pot1);
        args[8] = &(p->pot2);
        args[9] = &(p->pot3);
        Sched(csound, args, 10);
        break;
      case 3: /* foot-switches and button */
        /*       *kFootSw1dn, *kFootSw2dn, *kFootSw1up, *kFootSw2up, *kButInstr; */
        switch(data2) {
        case 1:
          if (*p->kButInstr) {  /* if kButInstr is set */
            args[0] = &(p->p0);
            args[1] = &(*p->kButInstr);
            args[2] = &(p->p2);
            args[3] = &(*p->kDur);
            args[4] = &(p->pot1);
            args[5] = &(p->pot2);
            args[6] = &(p->pot3);
            Sched(csound, args, 7);
          }
          printf("B15+ button\n");
          break;
        case 2: /* footswitch 1 down */
          if (*p->kFootSw1dn) {  /* if kFootSw1dn is set */
            p->fs1dn = (int) *p->kFootSw1dn + p->frac1;
            args[0] = &(p->p0);
            args[1] = &(p->fs1dn);
            args[2] = &(p->p2);
            args[3] = (*p->kFootSw1up) ? &(*p->kDur) : &(p->p3neg);
            args[4] = &(p->pot1);
            args[5] = &(p->pot2);
            args[6] = &(p->pot3);
            Sched(csound, args, 7);
          }
          else if (*p->kFootSw1up) { /* turn off the instrument */
            p->fs1up = -((int) *p->kFootSw1up + p->frac1);
            args[0] = &(p->p0);
            args[1] = &(p->fs1up);
            args[2] = &(p->p2);
            args[3] = &(p->p2); /* zero */
            Sched(csound, args, 4);
            p->fs1up = -*p->kFootSw1up;
            p->frac1 += INCR;
            p->frac1 = (p->frac1 >= 1) ? p->frac1 = 0 : p->frac1;
          }
          printf("B14- foot switch down1\n");
          break;
        case 3:
          if (*p->kFootSw1up) {  /* if kFootSw1up is set */
            p->fs1up = (int) *p->kFootSw1up + p->frac1;
            args[0] = &(p->p0);
            args[1] = &(p->fs1up);
            args[2] = &(p->p2);
            args[3] = (*p->kFootSw1dn) ? &(*p->kDur) : &(p->p3neg);
            args[4] = &(p->pot1);
            args[5] = &(p->pot2);
            args[6] = &(p->pot3);
            Sched(csound, args, 7);
          }
          else if (*p->kFootSw1dn) { /* turn off the instrument */
            p->fs1dn = -((int) *p->kFootSw1dn + p->frac1);
            args[0] = &(p->p0);
            args[1] = &(p->fs1dn);
            args[2] = &(p->p2);
            args[3] = &(p->p2); /* zero */
            Sched(csound, args, 4);
            p->frac1 += INCR;
            p->frac1 = (p->frac2 >= 1) ? p->frac1 = 0 : p->frac1;
          }
          printf("B14- foot switch up1\n");
          break;
        case 4:
          if (*p->kFootSw2dn) {  /* if kFootSw1dn is set */
            /* CONTINUARE con frac1 e frac2 */
            p->fs2dn = (int) *p->kFootSw2dn + p->frac2;
            args[0] = &(p->p0);
            args[1] = &(p->fs2dn);
            args[2] = &(p->p2);
            args[3] = (*p->kFootSw2up) ? &(*p->kDur) : &(p->p3neg);
            args[4] = &(p->pot1);
            args[5] = &(p->pot2);
            args[6] = &(p->pot3);
            Sched(csound, args, 7);
          }
          else if (*p->kFootSw2up) { /* turn off the instrument */
            p->fs2up = -((int) *p->kFootSw2up + p->frac2);
            args[0] = &(p->p0);
            args[1] = &(p->fs2up);
            args[2] = &(p->p2);
            args[3] = &(p->p2); /* zero */
            Sched(csound, args, 4);
            p->frac2 += INCR;
            p->frac2 = (p->frac2 >= 1) ? p->frac2 = 0 : p->frac2;
          }
          printf("B15- foot switch down2\n");
          break;
        case 5:
          if (*p->kFootSw2up) {  /* if kFootSw1up is set */
            p->fs2up = (int) *p->kFootSw2up + p->frac2;
            args[0] = &(p->p0);
            args[1] = &(p->fs2up);
            args[2] = &(p->p2);
            args[3] = (*p->kFootSw2dn) ? &(*p->kDur) : &(p->p3neg);
            args[4] = &(p->pot1);
            args[5] = &(p->pot2);
            args[6] = &(p->pot3);
            Sched(csound, args, 7);

          }
          else if (*p->kFootSw2dn) { /* turn off the instrument */
            p->fs2dn = -((int) *p->kFootSw2dn + p->frac2);
            args[0] = &(p->p0);
            args[1] = &(p->fs2dn);
            args[2] = &(p->p2);
            args[3] = &(p->p2); /* zero */
            Sched(csound, args, 4);
            p->frac2 += INCR;
            p->frac2 = (p->frac2 >= 1) ? p->frac2 = 0 : p->frac2;
          }
          printf("B15- foot switch up2\n");
          break;
        }
        break;
      case 4: p->pot1 = data2/FL(127.0); break;
      case 5: p->pot2 = data2/FL(127.0); break;
      case 6: p->pot3 = data2/FL(127.0); break;
      }
    }
    return OK;
}

#define S(x)    sizeof(x)

OENTRY radiobaton_localops[] = {
  { "rbatonPercPad", S(RB_PERCPAD),3, "", "kkkkkkkk",
    (SUBR)rbatonPercPad_set, (SUBR)rbatonPercPad },
  { "rbatonXYZ",     S(RB_XYZ),    3, "kkkkkk",  "",
    (SUBR)rbatonXYZ_set, (SUBR)rbatonXYZ },
  { "rbatonPot",     S(RB_POT),    3, "kkkkkkk", "",
    (SUBR)rbatonPot_set, (SUBR)rbatonPot  }
};

LINKAGE1(radiobaton_localops)

