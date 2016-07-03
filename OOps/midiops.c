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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"                                 /*      MIDIOPS.C   */
#include "midiops.h"
#include <math.h>
#include "namedins.h"           /* IV - Oct 31 2002 */

#define dv127   (FL(1.0)/FL(127.0))

/* aops.c, table for CPSOCTL */
/*extern  MYFLT   cpsocfrc[];*/

extern int m_chinsno(CSOUND *csound, int chan, int insno, int reset_ctls);

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

int midibset(CSOUND*, MIDIKMB*);

/* IV - Oct 31 2002: modified to allow named instruments */

int massign_p(CSOUND *csound, MASSIGN *p)
{
    int   chnl = (int)(*p->chnl + FL(0.5));
    int   resetCtls;
    int   retval = OK;

    resetCtls = (*p->iresetctls == FL(0.0) ? 0 : 1);
    if (--chnl >= 0)
      retval = m_chinsno(csound, chnl, (int) *p->insno, resetCtls);
    else {
      for (chnl = 0; chnl < 16; chnl++) {
        if (m_chinsno(csound, chnl, (int) *p->insno, resetCtls) != OK)
          retval = NOTOK;
      }
    }
    return retval;
}

int massign_S(CSOUND *csound, MASSIGNS *p)
{
    int   chnl = (int)(*p->chnl + FL(0.5));
    int32  instno = 0L;
    int   resetCtls;
    int   retval = OK;

   if (UNLIKELY((instno = strarg2insno(csound, p->insno->data, 1)) <= 0L))
        return NOTOK;

    resetCtls = (*p->iresetctls == FL(0.0) ? 0 : 1);
    if (--chnl >= 0)
      retval = m_chinsno(csound, chnl, (int) instno, resetCtls);
    else {
      for (chnl = 0; chnl < 16; chnl++) {
        if (m_chinsno(csound, chnl, (int) instno, resetCtls) != OK)
          retval = NOTOK;
      }
    }
    return retval;
}


int ctrlinit(CSOUND *csound, CTLINIT *p)
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
        ctlno = (int16)**argp++;
        if (UNLIKELY(ctlno < 0 || ctlno > 127)) {
          return csound->InitError(csound, Str("illegal ctrl no"));
        }
        chn->ctl_val[ctlno] = **argp++;
      } while (--nctls);
      return OK;
    }
}

int notnum(CSOUND *csound, MIDIKMB *p)       /* valid only at I-time */
{
    *p->r = csound->curip->m_pitch;
    return OK;
}

/* cpstmid by G.Maldonado */
int cpstmid(CSOUND *csound, CPSTABLE *p)
{
    FUNC  *ftp;
    MYFLT *func;
    int notenum = csound->curip->m_pitch;
    int grade;
    int numgrades;
    int basekeymidi;
    MYFLT basefreq, factor, interval;

    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->tablenum)) == NULL)) {
      return csound->InitError(csound, Str("cpstabm: invalid modulator table"));
    }
    func = ftp->ftable;
    numgrades = (int)*func++;
    interval = *func++;
    basefreq = *func++;
    basekeymidi = (int)*func++;

    if (notenum < basekeymidi) {
      notenum = basekeymidi - notenum;
      grade  = (numgrades-(notenum % numgrades)) % numgrades;
      factor = - (MYFLT)(int)((notenum+numgrades-1) / numgrades) ;
    }
    else {
      notenum = notenum - basekeymidi;
      grade  = notenum % numgrades;
      factor = (MYFLT)(int)(notenum / numgrades);
    }
    factor = POWER(interval, factor);
    *p->r = func[grade] * factor * basefreq;
    return OK;
}

int veloc(CSOUND *csound, MIDIMAP *p)           /* valid only at I-time */
{
    *p->r = *p->ilo + csound->curip->m_veloc*(*p->ihi - *p->ilo) * dv127;
    return OK;
}

int pchmidi(CSOUND *csound, MIDIKMB *p)
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

int pchmidib(CSOUND *csound, MIDIKMB *p)
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

int pchmidib_i(CSOUND *csound, MIDIKMB *p)
{
    midibset(csound, p);
    pchmidib(csound, p);
    return OK;
}

int octmidi(CSOUND *csound, MIDIKMB *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = lcurip->m_pitch / FL(12.0) + FL(3.0);
    return OK;
}

int octmidib(CSOUND *csound, MIDIKMB *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = (lcurip->m_pitch + (pitchbend_value(lcurip->m_chnbp) *
                                p->scale)) / FL(12.0) + FL(3.0);
    return OK;
}

int octmidib_i(CSOUND *csound, MIDIKMB *p)
{
  midibset(csound, p);
  octmidib(csound, p);
  return OK;
}

int cpsmidi(CSOUND *csound, MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    int32  loct;
/*    loct = (long)(((lcurip->m_pitch +
 *       pitchbend_value(lcurip->m_chnbp) * p->iscal)/ 12.0f + 3.0f) * OCTRES);
 */
    loct = (int32)((lcurip->m_pitch/ FL(12.0) + FL(3.0)) * OCTRES);
    *p->r = CPSOCTL(loct);
    return OK;
}

int icpsmidib(CSOUND *csound, MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    int32  loct;
    MYFLT bend = pitchbend_value(lcurip->m_chnbp);
    p->prvbend = bend;
    loct = (int32)(((lcurip->m_pitch +
                    bend * p->scale) / FL(12.0) + FL(3.0)) * OCTRES);
    *p->r = CPSOCTL(loct);
    return OK;
}

int icpsmidib_i(CSOUND *csound, MIDIKMB *p)
{
    midibset(csound, p);
    icpsmidib(csound, p);
    return OK;
}

int kcpsmidib(CSOUND *csound, MIDIKMB *p)
{
    INSDS *lcurip = p->h.insdshead;
    MYFLT bend = pitchbend_value(lcurip->m_chnbp);
    if (bend == p->prvbend || lcurip->relesing)
        *p->r = p->prvout;
    else {
      int32  loct;
      p->prvbend = bend;
      loct = (int32)(((lcurip->m_pitch +
                      bend * p->scale) / FL(12.0) + FL(3.0)) * OCTRES);
      *p->r = p->prvout = CPSOCTL(loct);
    }
    return OK;
}

int ampmidi(CSOUND *csound, MIDIAMP *p)   /* convert midi veloc to amplitude */
{                                         /*   valid only at I-time          */
    MYFLT amp;
    int32  fno;
    FUNC *ftp;

    amp = csound->curip->m_veloc / FL(128.0);     /* amp = normalised veloc */
    if ((fno = (int32)*p->ifn) > 0) {              /* if valid ftable,       */
      if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifn)) == NULL))
        return NOTOK;                             /*     use amp as index   */
      amp = *(ftp->ftable + (int32)(amp * ftp->flen));
    }
    *p->r = amp * *p->imax;                       /* now scale the output   */
    return OK;
}

/*      MWB 2/11/97  New optional field to set pitch bend range
        I also changed each of the xxxmidib opcodes, adding * p->scale */
int midibset(CSOUND *csound, MIDIKMB *p)
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

int aftset(CSOUND *csound, MIDIKMAP *p)
{
   IGN(csound);
    p->lo = *p->ilo;
    p->scale = (*p->ihi - p->lo) * dv127;
    return OK;
}

int aftouch(CSOUND *csound, MIDIKMAP *p)
{
   IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = p->lo + MIDI_VALUE(lcurip->m_chnbp, aftouch) * p->scale;
    return OK;
}

int imidictl(CSOUND *csound, MIDICTL *p)
{
    int32  ctlno;
    if (UNLIKELY((ctlno = (int32)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else *p->r = MIDI_VALUE(csound->curip->m_chnbp, ctl_val[ctlno])
                 * (*p->ihi - *p->ilo) * dv127 + *p->ilo;
    return OK;
}

int mctlset(CSOUND *csound, MIDICTL *p)
{
    int32  ctlno;
    if (UNLIKELY((ctlno = (int32)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else {
      p->ctlno = ctlno;
      p->scale = (*p->ihi - *p->ilo) * dv127;
      p->lo = *p->ilo;
    }
    return OK;
}

int midictl(CSOUND *csound, MIDICTL *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = MIDI_VALUE(lcurip->m_chnbp, ctl_val[p->ctlno]) * p->scale + p->lo;
    return OK;
}

int imidiaft(CSOUND *csound, MIDICTL *p)
{
    int32  ctlno;
    if (UNLIKELY((ctlno = (int32)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else *p->r = MIDI_VALUE(csound->curip->m_chnbp, polyaft[ctlno])
                 * (*p->ihi - *p->ilo) * dv127 + *p->ilo;
    return OK;
}

int maftset(CSOUND *csound, MIDICTL *p)
{
    int32  ctlno;
    if (UNLIKELY((ctlno = (int32)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else {
      p->ctlno = ctlno;
      p->scale = (*p->ihi - *p->ilo) * dv127;
      p->lo = *p->ilo;
    }
    return OK;
}

int midiaft(CSOUND *csound, MIDICTL *p)
{
    IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = MIDI_VALUE(lcurip->m_chnbp, polyaft[p->ctlno]) * p->scale + p->lo;
    return OK;
}

/* midichn opcode - get MIDI channel number or 0 for score notes */
/* written by Istvan Varga, May 2002 */

int midichn(CSOUND *csound, MIDICHN *p)
{
    *(p->ichn) = (MYFLT) (csound->GetMidiChannelNumber(p) + 1);
    return OK;
}

/* pgmassign - assign MIDI program to instrument */

int pgmassign_(CSOUND *csound, PGMASSIGN *p, int instname)
{
    int pgm, ins, chn;

    chn = (int)(*p->ichn + 0.5);
    if (UNLIKELY(chn < 0 || chn > 16))
      return csound->InitError(csound, Str("illegal channel number"));
    /* IV - Oct 31 2002: allow named instruments */
    if (instname || csound->ISSTRCOD(*p->inst)) {
      MYFLT buf[128];
      csound->strarg2name(csound, (char*) buf, p->inst, "", 1);
      ins = (int)strarg2insno(csound, buf, 1);
    }
    else
      ins = (int)(*(p->inst) + FL(0.5));
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
      pgm = (int)(*(p->ipgm) - FL(0.5));
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

int pgmassign_S(CSOUND *csound, PGMASSIGN *p){
  return pgmassign_(csound,p,1);
}

int pgmassign(CSOUND *csound, PGMASSIGN *p){
  return pgmassign_(csound,p,0);
}

int ichanctl(CSOUND *csound, CHANCTL *p)
{
    int32  ctlno, chan = (int32)(*p->ichano - FL(1.0));
    if (UNLIKELY(chan < 0 || chan > 15 || csound->m_chnbp[chan] == NULL))
        return csound->InitError(csound, Str("illegal channel number"));
    if (UNLIKELY((ctlno = (int32)*p->ictlno) < 0 || ctlno > 127))
        return csound->InitError(csound, Str("illegal controller number"));
    else *p->r = csound->m_chnbp[chan]->ctl_val[ctlno] * (*p->ihi - *p->ilo)
                 * dv127 + *p->ilo;
    return OK;
}

int chctlset(CSOUND *csound, CHANCTL *p)
{
    int32  ctlno, chan = (int32)(*p->ichano - FL(1.0));
    if (UNLIKELY(chan < 0 || chan > 15 || csound->m_chnbp[chan] == NULL)) {
      return csound->InitError(csound, Str("illegal channel number"));
    }
    p->chano = chan;
    if (UNLIKELY((ctlno = (int32)*p->ictlno) < 0 || ctlno > 127)) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    else {
      p->ctlno = ctlno;
      p->scale = (*p->ihi - *p->ilo) * dv127;
      p->lo = *p->ilo;
    }
    return OK;
}

int chanctl(CSOUND *csound, CHANCTL *p)
{
    *p->r = csound->m_chnbp[p->chano]->ctl_val[p->ctlno] * p->scale + p->lo;
    return OK;
}

int ipchbend(CSOUND *csound, MIDIMAP *p)
{
    IGN(csound);
    *p->r = *p->ilo + (*p->ihi - *p->ilo) *
            pitchbend_value(p->h.insdshead->m_chnbp);
    return OK;
}

int kbndset(CSOUND *csound, MIDIKMAP *p)
{
   IGN(csound);
    p->lo = *p->ilo;
    p->scale = *p->ihi - *p->ilo;
    return OK;
}

int kpchbend(CSOUND *csound, MIDIKMAP *p)
{
   IGN(csound);
    INSDS *lcurip = p->h.insdshead;
    *p->r = p->lo + pitchbend_value(lcurip->m_chnbp) * p->scale;
    return OK;
}

int midiin_set(CSOUND *csound, MIDIIN *p)
{
    p->local_buf_index = MGLOB(MIDIINbufIndex) & MIDIINBUFMSK;
    return OK;
}

int midiin(CSOUND *csound, MIDIIN *p)
{
    unsigned char *temp;                        /* IV - Nov 30 2002 */
    if  (p->local_buf_index != MGLOB(MIDIINbufIndex)) {
      temp = &(MGLOB(MIDIINbuffer2)[p->local_buf_index++].bData[0]);
      p->local_buf_index &= MIDIINBUFMSK;
      *p->status = (MYFLT) (*temp & (unsigned char) 0xf0);
      *p->chan   = (MYFLT) ((*temp & 0x0f) + 1);
      *p->data1  = (MYFLT) *++temp;
      *p->data2  = (MYFLT) *++temp;
    }
    else *p->status = FL(0.0);
    return OK;
}

int pgmin_set(CSOUND *csound, PGMIN *p)
{
    p->local_buf_index = MGLOB(MIDIINbufIndex) & MIDIINBUFMSK;
    p->watch =(int)*p->ochan;
    return OK;
}

int pgmin(CSOUND *csound, PGMIN *p)
{
    unsigned char *temp;
    if  (p->local_buf_index != MGLOB(MIDIINbufIndex)) {
      int st,ch,d1;
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

int ctlin_set(CSOUND *csound, CTLIN *p)
{
    p->local_buf_index = MGLOB(MIDIINbufIndex) & MIDIINBUFMSK;
    p->watch1 =(int)*p->ochan;
    p->watch2 =(int)*p->onum;
    return OK;
}

int ctlin(CSOUND *csound, CTLIN *p)
{
    unsigned char *temp;
    if  (p->local_buf_index != MGLOB(MIDIINbufIndex)) {
      int st,ch,d1,d2;
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
