/*
    midiops.c:

    Copyright (C) 1995 Barry Vercoe, Gabriel Maldonado, Istvan Varga, John ffitch

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

#include "cs.h"                                       /*    MIDIOPS.C    */
#include "midiops.h"
#include <math.h>
#include "namedins.h"           /* IV - Oct 31 2002 */

#define dv127   (FL(1.0)/FL(127.0))

/* default MIDI program to instrument assignment - IV May 2002 */

int pgm2ins[128] = {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,
                      11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
                      21,  22,  23,  24,  25,  26,  27,  28,  29,  30,
                      31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
                      41,  42,  43,  44,  45,  46,  47,  48,  49,  50,
                      51,  52,  53,  54,  55,  56,  57,  58,  59,  60,
                      61,  62,  63,  64,  65,  66,  67,  68,  69,  70,
                      71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
                      81,  82,  83,  84,  85,  86,  87,  88,  89,  90,
                      91,  92,  93,  94,  95,  96,  97,  98,  99, 100,
                     101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
                     111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
                     121, 122, 123, 124, 125, 126, 127, 128             };

/* INSDS    *curip valid at I-time */

#define MIDI_VALUE(m,field) ((m != (MCHNBLK *) NULL) ? m->field : FL(0.0))

    /*
     * m (=m_chnbp) can easily be NULL (the only place it gets set, as
     * of 3.484, is in MIDIinsert) so we should check for validity
     *                                  [added by nicb@axnet.it]
     */

/* This line has reverted to checking the null pointer as the code in oload
 * does leaves it null if there is no chanel.  The correct fix is to fix that
 * code so the test is not dynamic, but until I understand it.... */
#define pitchbend_value(m) MIDI_VALUE(m,pchbend)

/* Now in glob extern INSTRTXT **instrtxtp;  gab-A3 (added) */
int midibset(MIDIKMB *);

/* IV - Oct 31 2002: modified to allow named instruments */

int massign(MASSIGN *p)
{
    MCHNBLK *chn;
    short chnl = (short)(*p->chnl - FL(1.0));
    long  instno;

    if ((instno = strarg2insno(p->insno, p->STRARG)) < 1) return NOTOK;

    m_chinsno(chnl, (short) instno);
                                /* Changes from gab */
    if ((chn = M_CHNBP[chnl]) == NULL)
      M_CHNBP[chnl] = chn = (MCHNBLK *) mcalloc((long)sizeof(MCHNBLK));
    /* if corresp instr exists, assign as pgmno */
    chn->pgmno = (short) instno;
    return OK;
}

int ctrlinit(CTLINIT *p)
{
    short chnl = (short)(*p->chnl - FL(1.0));
    short nargs = p->INOCOUNT;
    if ((nargs & 0x1) == 0) {
        initerror(Str(X_1319,"uneven ctrl pairs"));
        return 0;
    }
    else {
        MCHNBLK *chn;
        MYFLT **argp = p->ctrls;
        short ctlno, nctls = nargs >> 1;
        if ((chn = M_CHNBP[chnl]) == NULL)
            chn = m_getchnl(chnl);
        do {
            ctlno = (short) **argp++;
            if (ctlno < 0 || ctlno > 127) {
                initerror(Str(X_845,"illegal ctrl no"));
                return NOTOK;
            }
            chn->ctl_val[ctlno] = **argp++;
        } while (--nctls);
        return OK;
    }
}

int notnum(MIDIKMB *p)      /* valid only at I-time */
{
    *p->r = curip->m_pitch;
    return OK;
}


/* cpstmid by G.Maldonado */
int cpstmid(CPSTABLE *p)
{
    FUNC  *ftp;
    MYFLT *func;
    int notenum = curip->m_pitch;
    int grade;
    int numgrades;
    int basekeymidi;
    MYFLT basefreq, factor,interval;

    if ((ftp = ftfind(p->h.insdshead->csound, p->tablenum)) == NULL) {
      initerror(Str(X_675,"cpstabm: invalid modulator table"));
      return NOTOK;
    }
    func = ftp->ftable;
    numgrades = (int) *func++;
    interval = *func++;
    basefreq = *func++;
    basekeymidi = (int) *func++;

    if (notenum < basekeymidi) {
      notenum = basekeymidi - notenum;
      grade  = (numgrades-(notenum % numgrades)) % numgrades;
      factor = - (MYFLT)(int) ((notenum+numgrades-1) / numgrades) ;
    }
    else {
      notenum = notenum - basekeymidi;
      grade  = notenum % numgrades;
      factor = (MYFLT)(int) (notenum / numgrades);
    }
    factor = (MYFLT)pow((double)interval, (double)factor);
    *p->r = func[grade] * factor * basefreq;
    return OK;
}

int veloc(MIDIMAP *p)          /* valid only at I-time */
{
    *p->r = *p->ilo + curip->m_veloc * (*p->ihi - *p->ilo) * dv127;
    return OK;
}

int pchmidi(MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    double fract, oct, ioct;
    oct = lcurip->m_pitch / 12.0 + 3.0;
    fract = modf(oct, &ioct);
    fract *= 0.12;
    *p->r = (MYFLT)(ioct + fract);
    return OK;
}

int pchmidib(MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    double fract, oct, ioct;
    MCHNBLK *xxx = curip->m_chnbp;
    MYFLT bend = pitchbend_value(xxx);
    oct = (lcurip->m_pitch + (bend * p->scale)) / FL(12.0) + FL(3.0);
    fract = modf(oct, &ioct);
    fract *= 0.12;
    *p->r = (MYFLT)(ioct + fract);
    return OK;
}

int pchmidib_i(MIDIKMB *p)
{
    midibset(p);
    pchmidib(p);
    return OK;
}

int octmidi(MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    *p->r = lcurip->m_pitch / FL(12.0) + FL(3.0);
    return OK;
}

int octmidib(MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    *p->r = (lcurip->m_pitch + (pitchbend_value(lcurip->m_chnbp) *
                                p->scale)) / FL(12.0) + FL(3.0);
    return OK;
}

int octmidib_i(MIDIKMB *p)
{
  midibset(p);
  octmidib(p);
  return OK;
}

int cpsmidi(MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    long  loct;
/*    loct = (long)(((lcurip->m_pitch + */
/*       pitchbend_value(lcurip->m_chnbp) * p->iscal)/ 12.0f + 3.0f) * OCTRES); */
    loct = (long)((lcurip->m_pitch/ FL(12.0) + FL(3.0)) * OCTRES);
    *p->r = CPSOCTL(loct);
    return OK;
}

int icpsmidib(MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    long  loct;
    MYFLT bend = pitchbend_value(lcurip->m_chnbp);
    p->prvbend = bend;
    loct = (long)(((lcurip->m_pitch +
                    bend * p->scale) / FL(12.0) + FL(3.0)) * OCTRES);
    *p->r = CPSOCTL(loct);
    return OK;
}

int icpsmidib_i(MIDIKMB *p)
{
    midibset(p);
    icpsmidib(p);
    return OK;
}

int kcpsmidib(MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    MYFLT bend = pitchbend_value(lcurip->m_chnbp);
    if (bend == p->prvbend || lcurip->relesing)
        *p->r = p->prvout;
    else {
      long  loct;
      p->prvbend = bend;
      loct = (long)(((lcurip->m_pitch +
                      bend * p->scale) / FL(12.0) + FL(3.0)) * OCTRES);
      *p->r = p->prvout = CPSOCTL(loct);
    }
    return OK;
}

int ampmidi(MIDIAMP *p)        /* convert midi veloc to amplitude */
                                /*   valid only at I-time          */
{
    MYFLT amp;
    long  fno;
    FUNC *ftp;

    amp = curip->m_veloc / FL(128.0);             /* amp = normalised veloc */
    if ((fno = (long)*p->ifn) > 0) {
                                /* if valid ftable,       */
      if ((ftp = ftfind(p->h.insdshead->csound, p->ifn)) == NULL)
        return NOTOK;                                /*     use amp as index   */
      amp = *(ftp->ftable + (long)(amp * ftp->flen));
    }
    *p->r = amp * *p->imax;                       /* now scale the output   */
    return OK;
}

/*      MWB 2/11/97  New optional field to set pitch bend range
        I also changed each of the xxxmidib opcodes, adding * p->scale*/
int midibset(MIDIKMB *p)
{
    if (*p->iscal > 0) {
      p->scale = *p->iscal;
    }
    else {
      p->scale = FL(2.0);
    }
    p->prvbend = FL(0.0);          /* Start from sane position */
    return OK;
}

int aftset(MIDIKMAP *p)
{
    p->lo = *p->ilo;
    p->scale = (*p->ihi - p->lo) * dv127;
    return OK;
}

int aftouch(MIDIKMAP *p)
{
    INSDS *lcurip = p->h.insdshead;
    *p->r = p->lo + MIDI_VALUE(lcurip->m_chnbp, aftouch) * p->scale;
    return OK;
}

/* void chpress(MIDIKMB *p) */
/* { */
/*     INSDS *lcurip = p->h.insdshead; */
/*     *p->r = lcurip->m_chnbp->chnpress * p->scale; */
/* } */

/* void pbenset(MIDIKMB *p) */
/* { */
/*     p->scale = *p->iscal; */
/* } */

/* void pchbend(MIDIKMB *p) */
/* { */
/*     INSDS *lcurip = p->h.insdshead; */
/*     *p->r = lcurip->m_chnbp->pchbend * p->scale; */
/* } */

int imidictl(MIDICTL *p)
{
    long  ctlno;
    if ((ctlno = (long)*p->ictlno) < 0 || ctlno > 127)
      initerror(Str(X_844,"illegal controller number"));
    else *p->r = MIDI_VALUE(curip->m_chnbp, ctl_val[ctlno])
           * (*p->ihi - *p->ilo) * dv127 + *p->ilo;
    return OK;
}

int mctlset(MIDICTL *p)
{
    long  ctlno;
    if ((ctlno = (long)*p->ictlno) < 0 || ctlno > 127)
      initerror(Str(X_844,"illegal controller number"));
    else {
      p->ctlno = ctlno;
      p->scale = (*p->ihi - *p->ilo) * dv127;
      p->lo = *p->ilo;
#ifdef never                                            /* IV - Jul 11 2002 */
      /* MWB If the controller value is 0, it is initialised to the optional
         value */
      /*
       * the m_chnbp pointer may very well be null here, so we better check
       * [nicb@axnet.it]
       */
      if (p->h.insdshead->m_chnbp != (MCHNBLK *) NULL)
      if (p->h.insdshead->m_chnbp->ctl_val[p->ctlno] == 0) {
        p->h.insdshead->m_chnbp->ctl_val[p->ctlno] = p->lo;
      }
#endif
    }
    return OK;
}

int midictl(MIDICTL *p)
{
    INSDS *lcurip = p->h.insdshead;
    *p->r = MIDI_VALUE(lcurip->m_chnbp,ctl_val[p->ctlno]) * p->scale + p->lo;
    return OK;
}


int imidiaft(MIDICTL *p)
{
    long  ctlno;
    if ((ctlno = (long)*p->ictlno) < 0 || ctlno > 127)
      initerror(Str(X_844,"illegal controller number"));
    else *p->r = MIDI_VALUE(curip->m_chnbp, polyaft[ctlno])
           * (*p->ihi - *p->ilo) * dv127 + *p->ilo;
    return OK;
}

int maftset(MIDICTL *p)
{
    long  ctlno;
    if ((ctlno = (long)*p->ictlno) < 0 || ctlno > 127)
      initerror(Str(X_844,"illegal controller number"));
    else {
      p->ctlno = ctlno;
      p->scale = (*p->ihi - *p->ilo) * dv127;
      p->lo = *p->ilo;
#ifdef never                                            /* IV - Jul 11 2002 */
      /* MWB If the controller value is 0, it is initialised to the optional
         value */
      /*
       * the m_chnbp pointer may very well be null here, so we better check
       * [nicb@axnet.it]
       */
      if (p->h.insdshead->m_chnbp != (MCHNBLK *) NULL)
      if (p->h.insdshead->m_chnbp->polyaft[p->ctlno] == 0) {
        p->h.insdshead->m_chnbp->polyaft[p->ctlno] = p->lo;
      }
#endif
    }
    return OK;
}

int midiaft(MIDICTL *p)
{
    INSDS *lcurip = p->h.insdshead;
    *p->r = MIDI_VALUE(lcurip->m_chnbp,polyaft[p->ctlno]) * p->scale + p->lo;
    return OK;
}


/* midichn opcode - get MIDI channel number or 0 for score notes */
/* written by Istvan Varga, May 2002 */

int midichn(MIDICHN *p)
{
    int nn;

    if (p->h.insdshead->m_chnbp == (MCHNBLK*) NULL)
      *(p->ichn) = FL(0.0);
    else {
      nn = 0;
      while (nn < 16 && M_CHNBP[nn] != p->h.insdshead->m_chnbp) nn++;
      *(p->ichn) = (nn < 16 ? (MYFLT) (nn + 1) : FL(0.0));
    }
    return OK;
}

/* pgmassign - assign MIDI program to instrument */

int pgmassign(PGMASSIGN *p)
{
    int pgm, ins;

    /* IV - Oct 31 2002: allow named instruments */
    if (*p->inst == SSTRCOD) {
      if (p->STRARG != NULL) 
        ins = (int) strarg2insno(p->inst, p->STRARG);
      else
        ins = (int) strarg2insno(p->inst, unquote(currevent->strarg));
    } else
      ins = (int) (*(p->inst) + FL(0.5));
    if (*(p->ipgm) == FL(0.0)) {        /* program = 0: assign all pgms */
      for (pgm = 0; pgm < 128; pgm++) pgm2ins[pgm] = ins;
    }
    else {
      pgm = (int) (*(p->ipgm) + FL(0.5)) - 1;
      if (pgm < 0 || pgm > 127) {
        initerror(Str(X_74,"pgmassign: invalid program number"));
        return NOTOK;
      }
      pgm2ins[pgm] = ins;
    }
    return OK;
}

int ichanctl(CHANCTL *p)
{
    long  ctlno, chan = (long)(*p->ichano - FL(1.0));
    if (chan < 0 || chan > 15 || M_CHNBP[chan] == NULL)
        initerror(Str(X_839,"illegal channel number"));
    if ((ctlno = (long)*p->ictlno) < 0 || ctlno > 127)
        initerror(Str(X_844,"illegal controller number"));
    else *p->r = M_CHNBP[chan]->ctl_val[ctlno] * (*p->ihi - *p->ilo) * dv127
                + *p->ilo;
    return OK;
}

int chctlset(CHANCTL *p)
{
    long  ctlno, chan = (long)(*p->ichano - FL(1.0));
    if (chan < 0 || chan > 15 || M_CHNBP[chan] == NULL) {
      initerror(Str(X_839,"illegal channel number"));
      return NOTOK;
    }
    p->chano = chan;
    if ((ctlno = (long)*p->ictlno) < 0 || ctlno > 127) {
      initerror(Str(X_844,"illegal controller number"));
      return NOTOK;
    }
    else {
      p->ctlno = ctlno;
      p->scale = (*p->ihi - *p->ilo) * dv127;
      p->lo = *p->ilo;
    }
    return OK;
}

int chanctl(CHANCTL *p)
{
    *p->r = M_CHNBP[p->chano]->ctl_val[p->ctlno] * p->scale + p->lo;
    return OK;
}

int ipchbend(MIDIMAP *p)
{
    *p->r = *p->ilo + (*p->ihi - *p->ilo) *
            pitchbend_value(p->h.insdshead->m_chnbp);
    return OK;
}

int kbndset(MIDIKMAP *p)
{
    p->lo = *p->ilo;
    p->scale = *p->ihi - *p->ilo;
    return OK;
}

int kpchbend(MIDIKMAP *p)
{
    INSDS *lcurip = p->h.insdshead;
    *p->r = p->lo + pitchbend_value(lcurip->m_chnbp) * p->scale;
    return OK;
}
