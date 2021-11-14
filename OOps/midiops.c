/*
  midiops.c:

  Copyright (C) 1995 Barry Vercoe, Gabriel Maldonado,
  Istvan Varga, John ffitch

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

#include "csoundCore.h"                                 /*      MIDIOPS.C   */
#include "midiops.h"
#include <math.h>
#include <time.h>
#include "namedins.h"           /* IV - Oct 31 2002 */
#include "arrays.h"

#define dv127   (FL(1.0)/FL(127.0))

extern int32_t m_chinsno(CSOUND *csound, int32_t chan,
                         int32_t insno, int32_t reset_ctls);

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

#define MGLOB(x) (((CSOUND*)csound)->midiGlobals->x)

int32_t midibset(CSOUND*, MIDIKMB*);

/* IV - Oct 31 2002: modified to allow named instruments */

int32_t massign_p(CSOUND *csound, MASSIGN *p)
{
    int32_t   chnl = (int32_t)(*p->chnl + FL(0.5));
    int32_t   resetCtls;
    int32_t   retval = OK;

    resetCtls = (*p->iresetctls == FL(0.0) ? 0 : 1);
    if (--chnl >= 0)
      retval = m_chinsno(csound, chnl, (int32_t) *p->insno, resetCtls);
    else {
      for (chnl = 0; chnl < 16; chnl++) {
        if (m_chinsno(csound, chnl, (int32_t) *p->insno, resetCtls) != OK)
          retval = NOTOK;
      }
    }
    return retval;
}

int32_t massign_S(CSOUND *csound, MASSIGNS *p)
{
    int32_t   chnl = (int32_t)(*p->chnl + FL(0.5));
    int32_t   instno = 0L;
    int32_t   resetCtls;
    int32_t   retval = OK;

    if (UNLIKELY((instno = strarg2insno(csound, p->insno->data, 1)) <= 0L))
      return NOTOK;

    resetCtls = (*p->iresetctls == FL(0.0) ? 0 : 1);
    if (--chnl >= 0)
      retval = m_chinsno(csound, chnl, (int32_t) instno, resetCtls);
    else {
      for (chnl = 0; chnl < 16; chnl++) {
        if (m_chinsno(csound, chnl, (int32_t) instno, resetCtls) != OK)
          retval = NOTOK;
      }
    }
    return retval;
}


int32_t ctrlinit(CSOUND *csound, CTLINIT *p)
{
    int16 chnl = (int16)(*p->chnl - FL(0.5));
    int16 nargs = p->INOCOUNT;
    if (UNLIKELY((nargs & 0x1) == 0)) {
      return csound->InitError(csound, Str("uneven ctrl pairs"));
    }
    else {
      MCHNBLK *chn;
      MYFLT **argp = p->ctrls;
      int16 ctlno, nctls = nargs >> 1;
      chn = csound->m_chnbp[chnl];
      do {
        MYFLT val;
        ctlno = (int16)**argp++;
        if (UNLIKELY(ctlno < 0 || ctlno > 127)) {
          return csound->InitError(csound, Str("illegal ctrl no"));
        }
        val = **argp++;
        if (val < FL(0.0) || val > FL(127.0))
          return csound->InitError(csound, Str("Value out of range [0,127]\n"));
        chn->ctl_val[ctlno] = val;
      } while (--nctls);
      return OK;
    }
}

int32_t ctrlnameinit(CSOUND *csound, CTLINITS *p)
{
    int16 chnl = strarg2insno(csound, ((STRINGDAT *)p->iname)->data, 1);
    int16 nargs = p->INOCOUNT;
    if (UNLIKELY(chnl > 63)) {
      return NOTOK;
    }
    {
      MCHNBLK *chn;
      MYFLT **argp = p->ctrls;
      int16 ctlno, nctls = nargs >> 1;
      chn = csound->m_chnbp[chnl];
      do {
        MYFLT val;
        ctlno = (int16)**argp++;
        if (UNLIKELY(ctlno < 0 || ctlno > 127)) {
          return csound->InitError(csound, Str("illegal ctrl no"));
        }
        val = **argp++;
        if (val < FL(0.0) || val > FL(127.0))
          return csound->InitError(csound, Str("Value out of range [0,127]\n"));
        chn->ctl_val[ctlno] = val;
      } while (--nctls);
      return OK;
    }
}



int32_t notnum(CSOUND *csound, MIDIKMB *p)       /* valid only at I-time */
{
    *p->r = csound->curip->m_pitch;
    return OK;
}

/* cpstmid by G.Maldonado */
int32_t cpstmid(CSOUND *csound, CPSTABLE *p)
{
    FUNC  *ftp;
    MYFLT *func;
    int32_t notenum = csound->curip->m_pitch;
    int32_t grade;
    int32_t numgrades;
    int32_t basekeymidi;
    MYFLT basefreq, factor, interval;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->tablenum)) == NULL)) {
      return csound->InitError(csound, Str("cpstabm: invalid modulator table"));
    }
    func = ftp->ftable;
    numgrades = (int32_t)*func++;
    interval = *func++;
    basefreq = *func++;
    basekeymidi = (int32_t)*func++;

    if (notenum < basekeymidi) {
      notenum = basekeymidi - notenum;
      grade  = (numgrades-(notenum % numgrades)) % numgrades;
      factor = - (MYFLT)(int32_t)((notenum+numgrades-1) / numgrades) ;
    }
    else {
      notenum = notenum - basekeymidi;
      grade  = notenum % numgrades;
      factor = (MYFLT)(int32_t)(notenum / numgrades);
    }
    factor = POWER(interval, factor);
    *p->r = func[grade] * factor * basefreq;
    return OK;
}

int32_t veloc(CSOUND *csound, MIDIMAP *p)           /* valid only at I-time */
{
    *p->r = *p->ilo + csound->curip->m_veloc*(*p->ihi - *p->ilo) * dv127;
    return OK;
}

int32_t pchmidi(CSOUND *csound, MIDIKMB *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    double fract, oct, ioct;
    oct = lcurip->m_pitch / 12.0 + 3.0;
    fract = modf(oct, &ioct);
    fract *= 0.12;
    *p->r = (MYFLT)(ioct + fract);
    return OK;
}

int32_t pchmidib(CSOUND *csound, MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    double fract, oct, ioct;
    MCHNBLK *xxx = csound->curip->m_chnbp;
    MYFLT bend = pitchbend_value(xxx);
    oct = (lcurip->m_pitch + (bend * p->scale)) / FL(12.0) + FL(3.0);
    fract = modf(oct, &ioct);
    fract *= 0.12;
    *p->r = (MYFLT)(ioct + fract);
    return OK;
}

int32_t pchmidib_i(CSOUND *csound, MIDIKMB *p)
{
    midibset(csound, p);
    pchmidib(csound, p);
    return OK;
}

int32_t octmidi(CSOUND *csound, MIDIKMB *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = lcurip->m_pitch / FL(12.0) + FL(3.0);
    return OK;
}

int32_t octmidib(CSOUND *csound, MIDIKMB *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = (lcurip->m_pitch + (pitchbend_value(lcurip->m_chnbp) *
                                p->scale)) / FL(12.0) + FL(3.0);
    return OK;
}

int32_t octmidib_i(CSOUND *csound, MIDIKMB *p)
{
    midibset(csound, p);
    octmidib(csound, p);
    return OK;
}

int32_t cpsmidi(CSOUND *csound, MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    int32_t  loct;
    /*    loct = (int64_t)(((lcurip->m_pitch +
     *       pitchbend_value(lcurip->m_chnbp) * p->iscal)/ 12.0f + 3.0f) * OCTRES);
     */
    loct = (int32_t)((lcurip->m_pitch/ FL(12.0) + FL(3.0)) * OCTRES);
    *p->r = CPSOCTL(loct);
    return OK;
}

int32_t icpsmidib(CSOUND *csound, MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    int32_t  loct;
    MYFLT bend = pitchbend_value(lcurip->m_chnbp);
    p->prvbend = bend;
    loct = (int32_t)(((lcurip->m_pitch +
                     bend * p->scale) / FL(12.0) + FL(3.0)) * OCTRES);
    *p->r = CPSOCTL(loct);
    return OK;
}

int32_t icpsmidib_i(CSOUND *csound, MIDIKMB *p)
{
    midibset(csound, p);
    icpsmidib(csound, p);
    return OK;
}

int32_t kcpsmidib(CSOUND *csound, MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    MYFLT bend = pitchbend_value(lcurip->m_chnbp);

    if (bend == p->prvbend || lcurip->relesing)
      *p->r = p->prvout;
    else {
      int32_t  loct;
      p->prvbend = bend;
      loct = (int32_t)(((lcurip->m_pitch +
                       bend * p->scale) / FL(12.0) + FL(3.0)) * OCTRES);
      *p->r = p->prvout = CPSOCTL(loct);
    }
    return OK;
}

int32_t ampmidi(CSOUND *csound, MIDIAMP *p)   /* convert midi veloc to amplitude */
{                                         /*   valid only at I-time          */
    MYFLT amp;
    int32_t  fno;
    FUNC *ftp;

    amp = csound->curip->m_veloc / FL(128.0);     /* amp = normalised veloc */
    if ((fno = (int32_t)*p->ifn) > 0) {              /* if valid ftable,       */
      if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->ifn)) == NULL))
        return NOTOK;                             /*     use amp as index   */
      amp = *(ftp->ftable + (int32_t)(amp * ftp->flen));
    }
    *p->r = amp * *p->imax;                       /* now scale the output   */
    return OK;
}

/*      MWB 2/11/97  New optional field to set pitch bend range
        I also changed each of the xxxmidib opcodes, adding * p->scale */
int32_t midibset(CSOUND *csound, MIDIKMB *p)
{
    MCHNBLK *chn;
    IGN(csound);
    chn = p->h.insdshead->m_chnbp;
    if (*p->iscal > FL(0.0))
      p->scale = *p->iscal;
    else if (chn != NULL)
      p->scale = chn->pbensens;
    else
      p->scale = FL(2.0);
    /* Start from sane position */
    if (chn != NULL)
      p->prvbend = chn->pchbend;
    else
      p->prvbend = FL(0.0);
    return OK;
}

int32_t aftset(CSOUND *csound, MIDIKMAP *p)
{
    IGN(csound);
    p->lo = *p->ilo;
    p->scale = (*p->ihi - p->lo) * dv127;
    return OK;
}

int32_t aftouch(CSOUND *csound, MIDIKMAP *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = p->lo + MIDI_VALUE(lcurip->m_chnbp, aftouch) * p->scale;
    return OK;
}

int32_t imidictl(CSOUND *csound, MIDICTL *p)
{
    int32_t  ctlno;
    if (UNLIKELY((ctlno = (int32_t)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else *p->r = MIDI_VALUE(csound->curip->m_chnbp, ctl_val[ctlno])
           * (*p->ihi - *p->ilo) * dv127 + *p->ilo;
    return OK;
}

int32_t mctlset(CSOUND *csound, MIDICTL *p)
{
    int32_t  ctlno;
    if (UNLIKELY((ctlno = (int32_t)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else {
      p->ctlno = ctlno;
      p->scale = (*p->ihi - *p->ilo) * dv127;
      p->lo = *p->ilo;
    }
    return OK;
}

int32_t midictl(CSOUND *csound, MIDICTL *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = MIDI_VALUE(lcurip->m_chnbp, ctl_val[p->ctlno]) * p->scale + p->lo;
    return OK;
}

int32_t imidiaft(CSOUND *csound, MIDICTL *p)
{
    int32_t  ctlno;
    if (UNLIKELY((ctlno = (int32_t)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else *p->r = MIDI_VALUE(csound->curip->m_chnbp, polyaft[ctlno])
           * (*p->ihi - *p->ilo) * dv127 + *p->ilo;
    return OK;
}

int32_t maftset(CSOUND *csound, MIDICTL *p)
{
    int32_t  ctlno;
    if (UNLIKELY((ctlno = (int32_t)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else {
      p->ctlno = ctlno;
      p->scale = (*p->ihi - *p->ilo) * dv127;
      p->lo = *p->ilo;
    }
    return OK;
}

int32_t midiaft(CSOUND *csound, MIDICTL *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = MIDI_VALUE(lcurip->m_chnbp, polyaft[p->ctlno]) * p->scale + p->lo;
    return OK;
}

/* midichn opcode - get MIDI channel number or 0 for score notes */
/* written by Istvan Varga, May 2002 */

int32_t midichn(CSOUND *csound, MIDICHN *p)
{
    *(p->ichn) = (MYFLT) (csound->GetMidiChannelNumber(p) + 1);
    return OK;
}

/* pgmassign - assign MIDI program to instrument */

int32_t pgmassign_(CSOUND *csound, PGMASSIGN *p, int32_t instname)
{
    int32_t pgm, ins, chn;

    chn = (int32_t)(*p->ichn + 0.5);
    if (UNLIKELY(chn < 0 || chn > 16))
      return csound->InitError(csound, Str("illegal channel number"));
    /* IV - Oct 31 2002: allow named instruments */
    if (instname || csound->ISSTRCOD(*p->inst)) {
      MYFLT buf[128];
      csound->strarg2name(csound, (char*) buf, p->inst, "", 1);
      ins = (int32_t)strarg2insno(csound, buf, 1);
    }
    else
      ins = (int32_t)(*(p->inst) + FL(0.5));
    if (*(p->ipgm) < FL(0.5)) {         /* program <= 0: assign all pgms */
      if (!chn) {                           /* on all channels */
        for (chn = 0; chn < 16; chn++)
          for (pgm = 0; pgm < 128; pgm++)
            csound->m_chnbp[chn]->pgm2ins[pgm] = ins;
      }
      else {                                /* or selected channel only */
        chn--;
        for (pgm = 0; pgm < 128; pgm++)
          csound->m_chnbp[chn]->pgm2ins[pgm] = ins;
      }
    }
    else {                              /* program > 0: assign selected pgm */
      pgm = (int32_t)(*(p->ipgm) - FL(0.5));
      if (UNLIKELY(pgm < 0 || pgm > 127)) {
        return csound->InitError(csound, Str("pgmassign: invalid program number"));
      }
      if (!chn) {                           /* on all channels */
        for (chn = 0; chn < 16; chn++)
          csound->m_chnbp[chn]->pgm2ins[pgm] = ins;
      }
      else {                                /* or selected channel only */
        chn--;
        csound->m_chnbp[chn]->pgm2ins[pgm] = ins;
      }
    }
    return OK;
}

int32_t pgmassign_S(CSOUND *csound, PGMASSIGN *p) {
    return pgmassign_(csound,p,1);
}

int32_t pgmassign(CSOUND *csound, PGMASSIGN *p) {
    return pgmassign_(csound,p,0);
}

int32_t ichanctl(CSOUND *csound, CHANCTL *p)
{
    int32_t  ctlno, chan = (int32_t)(*p->ichano - FL(1.0));
    if (UNLIKELY(chan < 0 || chan > 15 || csound->m_chnbp[chan] == NULL))
      return csound->InitError(csound, Str("illegal channel number"));
    if (UNLIKELY((ctlno = (int32_t)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else *p->r = csound->m_chnbp[chan]->ctl_val[ctlno] * (*p->ihi - *p->ilo)
           * dv127 + *p->ilo;
    return OK;
}

int32_t chctlset(CSOUND *csound, CHANCTL *p)
{
    int32_t  ctlno, chan = (int32_t)(*p->ichano - FL(1.0));
    if (UNLIKELY(chan < 0 || chan > 15 || csound->m_chnbp[chan] == NULL)) {
      return csound->InitError(csound, Str("illegal channel number"));
    }
    p->chano = chan;
    if (UNLIKELY((ctlno = (int32_t)*p->ictlno) < 0 || ctlno > 127)) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    else {
      p->ctlno = ctlno;
      p->scale = (*p->ihi - *p->ilo) * dv127;
      p->lo = *p->ilo;
    }
    return OK;
}

int32_t chanctl(CSOUND *csound, CHANCTL *p)
{
    *p->r = csound->m_chnbp[p->chano]->ctl_val[p->ctlno] * p->scale + p->lo;
    return OK;
}

int32_t ipchbend(CSOUND *csound, MIDIMAP *p)
{
    IGN(csound);
    *p->r = *p->ilo + (*p->ihi - *p->ilo) *
      pitchbend_value(p->h.insdshead->m_chnbp);
    return OK;
}

int32_t kbndset(CSOUND *csound, MIDIKMAP *p)
{
    IGN(csound);
    p->lo = *p->ilo;
    p->scale = (*p->ihi - p->lo);
    /* { */
    /*   printf("lo hi =%f %f\tr=-1 v = %f / r=0 v = %f / r=1 v = %f\n", */
    /*          p->lo, *p->ihi, p->lo + (-1) * p->scale, */
    /*          p->lo + 0 * p->scale, */
    /*          p->lo + 1 * p->scale);  */
    /* } */
    return OK;
}

int32_t kpchbend(CSOUND *csound, MIDIKMAP *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = p->lo + pitchbend_value(lcurip->m_chnbp) * p->scale;
    return OK;
}

int32_t midiin_set(CSOUND *csound, MIDIIN *p)
{
    p->local_buf_index = MGLOB(MIDIINbufIndex) & MIDIINBUFMSK;
    return OK;
}

int32_t midiin(CSOUND *csound, MIDIIN *p)
{
    unsigned char *temp;                        /* IV - Nov 30 2002 */
    if (p->local_buf_index != MGLOB(MIDIINbufIndex)) {
      temp = &(MGLOB(MIDIINbuffer2)[p->local_buf_index++].bData[0]);
      p->local_buf_index &= MIDIINBUFMSK;
      *p->status = (MYFLT) *temp; //(*temp & (unsigned char) 0xf0);
      *p->chan   = (MYFLT) *++temp; //((*temp & 0x0f) + 1);
      *p->data1  = (MYFLT) *++temp;
      *p->data2  = (MYFLT) *++temp;
    }
    else *p->status = FL(0.0);
    return OK;
}

int32_t pgmin_set(CSOUND *csound, PGMIN *p)
{
    p->local_buf_index = MGLOB(MIDIINbufIndex) & MIDIINBUFMSK;
    p->watch =(int32_t)*p->ochan;
    return OK;
}

int32_t pgmin(CSOUND *csound, PGMIN *p)
{
    unsigned char *temp;
    if (p->local_buf_index != MGLOB(MIDIINbufIndex)) {
      int32_t st,ch,d1;
      temp = &(MGLOB(MIDIINbuffer2)[p->local_buf_index++].bData[0]);
      st = *temp & (unsigned char) 0xf0;
      ch = (*temp & 0x0f) + 1;
      d1 = *++temp;
      /*       d2 = *++temp; */
      if (st == 0xC0 && (p->watch==0 || p->watch==ch)) {
        *p->pgm = (MYFLT)1+d1;
        *p->chn = (MYFLT)ch;
      }
      else {
        *p->pgm = FL(-1.0);
        *p->chn = FL(0.0);
      }
      p->local_buf_index &= MIDIINBUFMSK;
    }
    else {
      *p->pgm = FL(-1.0);
      *p->chn = FL(0.0);
    }
    return OK;
}

int32_t ctlin_set(CSOUND *csound, CTLIN *p)
{
    p->local_buf_index = MGLOB(MIDIINbufIndex) & MIDIINBUFMSK;
    p->watch1 =(int32_t)*p->ochan;
    p->watch2 =(int32_t)*p->onum;
    return OK;
}

int32_t ctlin(CSOUND *csound, CTLIN *p)
{
    unsigned char *temp;
    if  (p->local_buf_index != MGLOB(MIDIINbufIndex)) {
      int32_t st,ch,d1,d2;
      temp = &(MGLOB(MIDIINbuffer2)[p->local_buf_index++].bData[0]);
      st = *temp & (unsigned char) 0xf0;
      ch = (*temp & 0x0f) + 1;
      d1 = *++temp;
      d2 = *++temp;
      if (st == 0xB0 &&
          (p->watch1==0 || p->watch1==ch) &&
          (p->watch2==0 || p->watch2==d2)) {
        *p->data = (MYFLT)d1;
        *p->numb = (MYFLT)d2;
        *p->chn = (MYFLT)ch;
      }
      else {
        *p->data = FL(-1.0);
        *p->numb = FL(-1.0);
        *p->chn = FL(0.0);
      }
      p->local_buf_index &= MIDIINBUFMSK;
    }
    else {
      *p->data = FL(-1.0);
      *p->numb = FL(-1.0);
      *p->chn = FL(0.0);
    }
    return OK;

}


/* MIDIARP by Rory Walsh, 2016
 */

int32_t midiarp_set(CSOUND *csound, MIDIARP *p)
/* MIDI Arp - Jan 2017 - RW */
{
    int32_t cnt;
    srand(time(NULL));
    p->flag=1, p->direction=2, p->noteIndex=9;
    p->maxNumNotes=10, p->noteCnt=0, p->status=0, p->chan=0;
    p->data1=0, p->data2=0;

    p->local_buf_index = MGLOB(MIDIINbufIndex) & MIDIINBUFMSK;

    for (cnt=0;cnt<10;cnt++)
      p->notes[cnt] = 0;

    return OK;
}

void sort_notes(int32_t notes[], int32_t n)
{
    int32_t j,i,tmp;
    for (i = 0; i < n; ++i) {
      for (j = i + 1; j < n; ++j) {
        if (notes[i] > notes[j]) {
          tmp =  notes[i];
          notes[i] = notes[j];
          notes[j] = tmp;
        }
      }
    }
}

void zeroNoteFromArray(int32_t notes[], int32_t noteNumber, int32_t size)
{
    int32_t i;
    for (i=0;i<size;i++) {
      if (notes[i]==noteNumber)
        notes[i]=0;
    }
}

int32_t metroCounter(MIDIARP *p)
{
    double phs = p->curphs;
    if (phs == 0.0 && p->flag) {
      p->metroTick = FL(1.0);
      p->flag = 0;
    }
    else if ((phs += *p->arpRate * CS_ONEDKR) >= 1.0) {
      p->metroTick = FL(1.0);
      phs -= 1.0;
      p->flag = 0;
    }
    else
      p->metroTick = FL(0.0);

    p->curphs = phs;
    return p->metroTick;
}


int32_t midiarp(CSOUND *csound, MIDIARP *p)
{
    int32_t i=0;
    unsigned char *temp;
    int32_t arpmode = (int32_t)*p->arpMode;

    if (p->local_buf_index != MGLOB(MIDIINbufIndex))
      {
        temp = &(MGLOB(MIDIINbuffer2)[p->local_buf_index++].bData[0]);
        p->local_buf_index &= MIDIINBUFMSK;
        p->status = (MYFLT) (*temp & (unsigned char) 0xf0);
        p->chan   = (MYFLT) ((*temp & 0x0f) + 1);
        p->data1  = (MYFLT) *++temp;
        p->data2  = (MYFLT) *++temp;

        if (p->status==144 && p->data2>0) {
          p->notes[p->noteCnt] = p->data2;

          for (i = 0 ; i < p->maxNumNotes ; i++)
            p->sortedNotes[i] = p->notes[i];

          p->noteCnt = (p->noteCnt>p->maxNumNotes-1 ?
                        p->maxNumNotes-1 : p->noteCnt+1);
          sort_notes(p->sortedNotes, 10);

        }
        else if (p->status==128 || (p->status==144 && p->data2==0)) {
          zeroNoteFromArray(p->notes, p->data2, p->maxNumNotes);

          for (i = 0 ; i < p->maxNumNotes ; i++)
            p->sortedNotes[i] = p->notes[i];

          p->noteCnt = (p->noteCnt<0 ? 0 : p->noteCnt-1);
          sort_notes(p->sortedNotes, p->maxNumNotes);
        }
      }
    else p->status = FL(0.0);

    if (p->noteCnt != 0) {
      // only when some note/s are pressed
      *p->counter = metroCounter(p);
      if (*p->counter == 1) {

        if (p->noteIndex<p->maxNumNotes && p->sortedNotes[p->noteIndex]!=0)
          *p->noteOut = p->sortedNotes[p->noteIndex];

        if (arpmode==0)
        {
          //up and down pattern
            if(p->direction>0) {
                p->noteIndex = (p->noteIndex < p->maxNumNotes-1
                                ? p->noteIndex+1 : p->maxNumNotes - p->noteCnt);
                if(p->noteIndex==p->maxNumNotes-1)
                    p->direction = -2;
            }
            else{
                p->noteIndex = (p->noteIndex >= p->maxNumNotes - p->noteCnt
                                ? p->noteIndex-1 : p->maxNumNotes-1);
                if(p->noteIndex==p->maxNumNotes-p->noteCnt)
                    p->direction = 2;

            }
        }
        else if (arpmode==1) {
          //up only pattern
          p->noteIndex = (p->noteIndex < p->maxNumNotes-1
                          ? p->noteIndex+1 : p->maxNumNotes - p->noteCnt);
        }
        else if (arpmode==2) {
          //down only pattern
          p->noteIndex = (p->noteIndex > p->maxNumNotes - p->noteCnt
                          ? p->noteIndex-1 : p->maxNumNotes-1);
        }
        else if (arpmode==3) {
          //random pattern
          int32_t randIndex = ((rand() % 100)/100.f)*(p->noteCnt);
          p->noteIndex = p->maxNumNotes-randIndex-1;
        }
        else{
          csound->Message(csound,
                          Str("Invalid arp mode selected:"
                              " %d. Valid modes are 0, 1, 2, and 3\n"),
                          arpmode);
        }
      }
    }

    return OK;
}

/* The internal data structure is an array onanin
 * length, chan, ctrl1, val1, ctrl2, val2, .....
 */

int savectrl_init(CSOUND *csound, SAVECTRL *p)
{
    int16 chnl = (int16)(*p->chnl - FL(0.5));
    int16 i, j, nargs = p->INOCOUNT-1;
    MYFLT **argp = p->ctrls;
    int16 ctlno;
    p->ivals = (csound->m_chnbp[chnl])->ctl_val;
    for (i=0; i<nargs; i++) {
      ctlno = (int16)*argp[i];
      if (ctlno < FL(0.0) || ctlno > FL(127.0))
        return csound->InitError(csound, Str("Value out of range [0,127]\n"));
    }
    tabinit(csound, p->arr, 2+2*nargs);
    p->arr->data[0] = nargs;    /* length */
    p->arr->data[1] = chnl+1;   /* channel */
    for (i=0, j=2; i<nargs; i++, j+=2) {
      p->arr->data[j] = *argp[i];
      p->arr->data[j+1] = FL(0.0);
    }
    p->nargs = nargs;
    return OK;
}

int savectrl_perf(CSOUND *csound, SAVECTRL *p)
{
    int16 nargs = p->nargs, i, j;
    MYFLT **argp = p->ctrls;
    MYFLT *ctlval = p->ivals;
    tabcheck(csound, p->arr, 2+2*nargs, &p->h);
    for (i=0, j=3; i<nargs; i++, j+=2) {
      MYFLT val = ctlval[(int16)*argp[i]];
      p->arr->data[j] = val;
    }
    return OK;
}

int printctrl_init(CSOUND *csound, PRINTCTRL *p)
{
    p->fout = stdout;
    if (p->fout==NULL) return NOTOK;
    return OK;
}

int printctrl_init1(CSOUND *csound, PRINTCTRL *p)
{
    p->fout = fopen(p->file->data, "a");
    if (p->fout==NULL) return NOTOK;
    return OK;
}


int printctrl(CSOUND *csound, PRINTCTRL *p)
{
    MYFLT *d = p->arr->data;
    int n = (int)d[0], i;
    fprintf(p->fout, "\n ctrlinit\t%d", (int)d[1]);
    for (i=0; i<n; i++)
      fprintf(p->fout, ", %d,%d", (int)d[2+2*i], (int)d[3+2*i]);
    fprintf(p->fout, "\n\n");
    fflush(p->fout);
    return OK;
}

int presetctrl_init(CSOUND *csound, PRESETCTRL *p)
{
    PRESET_GLOB *q =
      (PRESET_GLOB*)csound->QueryGlobalVariable(csound, "presetGlobals_");
    if (q==NULL) {
      if (UNLIKELY(csound->CreateGlobalVariable(csound, "presetGlobals_",
                                                sizeof(PRESET_GLOB)) != 0))
        return
          csound->InitError(csound, "%s",
                            Str("ctrlpreset: failed to allocate globals"));
      q = (PRESET_GLOB*)csound->QueryGlobalVariable(csound, "presetGlobals_");
      q->max_num = 10;
      q->presets = (int**)csound->Calloc(csound, 10*sizeof(int*));
    }
    p->q = q;
    return OK;
}

// Store a set of crtrlinits as a preset, allocating a number if necessary
int presetctrl_perf(CSOUND *csound, PRESETCTRL *p)
{
    PRESET_GLOB *q = p->q;
    int *slot;
    int i;
    int tag = (int)*p->itag - 1;
    if (tag<0) {
      for (i=0; i<q->max_num; i++)
        if (q->presets[i]==NULL) { tag=i; break;}
      if (i>=q->max_num) tag = q->max_num;
    }
    if (tag >= q->max_num) {
      int** tt = q->presets;
      int size = tag-q->max_num;
      if (size<10) size = 10;
      tt = (int**)csound->ReAlloc(csound,
                                    tt, (q->max_num+size)*sizeof(int*));
      if (tt == NULL)
        return csound->InitError(csound, "%s",
                                 Str("Failed to allocate presets\n"));
      for (i=0; i<size; i++) tt[i+q->max_num] = 0;
      q->presets = tt;
      q->max_num += size;
    }
    slot = q->presets[tag];
    if (slot) csound->Free(csound, slot);
    q->presets[tag] = (int*) csound->Malloc(csound, sizeof(int)*(p->INOCOUNT));
    slot = q->presets[tag];
    slot[0] = p->INOCOUNT;
    slot[1] = (int)(*p->chnl);
    for (i=0; i<slot[0]-2; i++)
      slot[i+2]= (int)*p->ctrls[i];
    /* for (i=0; i<slot[0];i++) printf("%d ", slot[i]); */
    /* printf("\n"); */
    *p->inum = (MYFLT)tag+1;
    return OK;
}

int presetctrl1_init(CSOUND *csound, PRESETCTRL1 *p)
{
    PRESET_GLOB *q =
      (PRESET_GLOB*)csound->QueryGlobalVariable(csound, "presetGlobals_");
    if (q==NULL) {
      if (UNLIKELY(csound->CreateGlobalVariable(csound, "presetGlobals_",
                                                sizeof(PRESET_GLOB)) != 0))
        return
          csound->InitError(csound, "%s",
                            Str("ctrlpreset: failed to allocate globals"));
      q = (PRESET_GLOB*)csound->QueryGlobalVariable(csound, "presetGlobals_");
      q->max_num = 10;
      q->presets = (int**)csound->Calloc(csound, 10*sizeof(int*));
    }
    p->q = q;
    return OK;
}

// Store a set of crtrlinits as a preset, allocating a number if necessary
int presetctrl1_perf(CSOUND *csound, PRESETCTRL1 *p)
{
    PRESET_GLOB *q = p->q;
    int *slot;
    int i;
    int tag = (int)*p->itag - 1;
    if (tag<0) {
      for (i=0; i<q->max_num; i++)
        if (q->presets[i]==NULL) { tag=i; break;}
      if (i>=q->max_num) tag = q->max_num;
    }
    if (tag >= q->max_num) {
      int** tt = q->presets;
      int size = tag-q->max_num;
      if (size<10) size = 10;
      tt = (int**)csound->ReAlloc(csound,
                                  tt, (q->max_num+size)*sizeof(int*));
      if (tt == NULL)
        return csound->InitError(csound, "%s",
                                 Str("Failed to allocate presets\n"));
      for (i=0; i<size; i++) tt[i+q->max_num] = 0;
      q->presets = tt;
      q->max_num += size;
    }
    slot = q->presets[tag];
    if (slot) csound->Free(csound, slot);
    q->presets[tag] = (int*) csound->Malloc(csound,
                                            sizeof(int)*(1+p->arr->sizes[0]));
    slot = q->presets[tag];
    slot[0] = p->arr->sizes[0];
    slot[1] = (int)(p->arr->data[1]);
    for (i=2; i<=slot[0]; i++)
      slot[i]= (int)p->arr->data[i];
    /* for (i=0; i<slot[0];i++) printf("%d ", slot[i]); */
    /* printf("\n"); */
    *p->inum = (MYFLT)tag+1;
    return OK;
}

int selectctrl_init(CSOUND *csound, SELECTCTRL *p)
{
    PRESET_GLOB *q =
      (PRESET_GLOB*)csound->QueryGlobalVariable(csound, "presetGlobals_");
    if (q==NULL) {
      return csound->InitError(csound, Str("No presets stored"));
    }
    p->q = q;
    return OK;
}

int selectctrl_perf(CSOUND *csound, SELECTCTRL *p)
{
    PRESET_GLOB *q = p->q;
    int tag = (int)*p->inum-1;
    int i;
    int* slot;
    if (tag>=q->max_num ||NULL==(slot = q->presets[tag])) {
      return csound->PerfError(csound, &p->h, Str("No such preset %d\n"), tag+1);
    }
    {
      int nargs = slot[0];
      int16 chnl = (int16)(slot[1]-1); /* Count from zero */
      MYFLT *ctlval = (csound->m_chnbp[chnl])->ctl_val;
      for (i=2; i<nargs; i+=2) {
        int val = slot[i+1];
        ctlval[slot[i]] = val;
        printf("control %d value %d\n", slot[i], val);
      }
    }
    return OK;
}

int printpresets_perf(CSOUND *csound, PRINTPRESETS *p)
{
    int j;
    FILE *ff = p->fout;
    PRESET_GLOB *q =
      (PRESET_GLOB*)csound->QueryGlobalVariable(csound, "presetGlobals_");
    if (q==NULL) {
      return csound->InitError(csound, Str("No presets stored"));
    }
    for (j=0; j<q->max_num; j++)
      if (q->presets[j]) {
        int i;
        int *slot = q->presets[j];
        fprintf(ff, "\n kpre%d ctrlpreset\t%d ", j+1, j+1);
        for (i=1; i<slot[0]; i++)
          fprintf(ff, ", %d", slot[i]);
        fprintf(ff, "\n");
      }
    fprintf(ff, "\n\n");
    fflush(ff);
    return OK;
}

int printpresets_init(CSOUND *csound, PRINTPRESETS *p)
{
    p->fout = stdout;
    if (p->fout==NULL) return NOTOK;
    return OK;
}

int printpresets_init1(CSOUND *csound, PRINTPRESETS *p)
{
    p->fout = fopen(p->file->data, "a");
    if (p->fout==NULL) return NOTOK;
    return OK;
}

