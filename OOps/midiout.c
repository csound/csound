/*
    midiout.c:

    Copyright (C) 1997 Gabriel Maldonado, John ffitch

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

/****************************************/
/** midiout UGs by Gabriel Maldonado   **/
/****************************************/

/* Some modifications by JPff for general use */

#include <math.h>
#include "csoundCore.h"
#include "midiout.h"

#define MGLOB(x) (((CSOUND*)csound)->midiGlobals->x)

#define NUMCHN          (16)
#define EXTRA_TIME      (1)

extern void openMIDIout(CSOUND *);
/* static MYFLT   invkr; */
void note_on(CSOUND *, int chan, int num, int vel);
void note_off(CSOUND *, int chan, int num, int vel);
void control_change(CSOUND *, int chan, int num, int value);
void after_touch(CSOUND *, int chan, int value);
void program_change(CSOUND *, int chan, int num);
void pitch_bend(CSOUND *, int chan, int lsb, int msb);
void poly_after_touch(CSOUND *, int chan, int note_num, int value);
void send_midi_message(CSOUND *, int status, int data1, int data2);

int release_set(CSOUND *csound, REL *p)
{
    IGN(csound);
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;
    return OK;
}

int release(CSOUND *csound, REL *p)
{
    IGN(csound);
    if (p->h.insdshead->relesing)
      *p->r = FL(1.0); /* TRUE */
    else
      *p->r = FL(0.0); /* FALSE */
    return OK;
}

int xtratim(CSOUND *csound, XTRADUR *p)
{
    int *xtra = &(p->h.insdshead->xtratim);
    int tim = (int)(*p->extradur * p->h.insdshead->ekr);
    if (*xtra < tim)  /* gab-a5 revised */
      *xtra = tim;
    return OK;
}

int mclock_set(CSOUND *csound, MCLOCK *p)
{
    p->period= CS_EKR / *p->freq;
    p->clock_tics = p->period;
    p->beginning_flag = TRUE;
    return OK;
}

int mclock(CSOUND *csound, MCLOCK *p)
{
    if (p->beginning_flag) {    /* first time */
      send_midi_message(csound, 0xF8, 0, 0);    /* clock message */
      p->beginning_flag=FALSE;
      return OK;
    }
    else if ((MYFLT) CS_KCNT > p->clock_tics) {
      send_midi_message(csound, 0xF8, 0, 0);    /* clock message */
      p->clock_tics += p->period;
    }
    return OK;
}

int mrtmsg(CSOUND *csound, MRT *p)
{
    switch ((int)*p->message) {
    case 0:
      send_midi_message(csound, 0xFC, 0, 0); /* stop */
      break;
    case 1:
      send_midi_message(csound, 0xFA, 0, 0); /* start */
      break;
    case 2:
      send_midi_message(csound, 0xFB, 0, 0); /* continue */
      break;
    case -1:
      send_midi_message(csound, 0xFF, 0, 0); /* system_reset */
      break;
    case -2:
      send_midi_message(csound, 0xFE, 0, 0); /* active_sensing */
      break;
    default:
      csound->InitError(csound, Str("illegal mrtmsg argument"));
    }
    return OK;
}

int iout_on(CSOUND *csound, OUT_ON *p)
{
    note_on(csound, (int)*p->ichn-1,(int)*p->inum,(int)*p->ivel);
    return OK;
}

int iout_off(CSOUND *csound, OUT_ON *p)
{
    note_off(csound, (int)*p->ichn-1,(int)*p->inum,(int)*p->ivel);
    return OK;
}

int iout_on_dur_set(CSOUND *csound, OUT_ON_DUR *p)
{
    int temp;
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;

    p->chn = (temp = abs((int)*p->ichn-1)) < NUMCHN ? temp : NUMCHN-1;
    p->num = (temp = abs((int)*p->inum)) < 128 ? temp : 127;
    p->vel = (temp = abs((int)*p->ivel)) < 128 ? temp : 127;

    note_on(csound, p->chn,p->num, p->vel);
    p->istart_time = (MYFLT)CS_KCNT * CS_ONEDKR;
    p->fl_expired = FALSE;
    p->fl_extra_dur = FALSE;
    return OK;
}

int iout_on_dur(CSOUND *csound, OUT_ON_DUR *p)
{
    if (!(p->fl_expired)) {
      MYFLT actual_dur = (MYFLT) CS_KCNT * CS_ONEDKR
                         - p->istart_time;
      MYFLT dur = *p->idur;
      if (dur < actual_dur) {
        p->fl_expired = TRUE;
        note_off(csound, p->chn, p->num, p->vel);
      }
      else if (p->h.insdshead->relesing) {
        p->fl_expired = TRUE;
        note_off(csound, p->chn, p->num, p->vel);
      }
    }
    return OK;
}

int iout_on_dur2(CSOUND *csound, OUT_ON_DUR *p)
{
    if (!(p->fl_expired)) {
      MYFLT actual_dur = (MYFLT)CS_KCNT * CS_ONEDKR
                         - p->istart_time;
      MYFLT dur = *p->idur;
      if (dur < actual_dur) {
        p->fl_expired = TRUE;
        note_off(csound, p->chn, p->num, p->vel);
      }
      else if (p->h.insdshead->relesing || p->fl_extra_dur) {

        if (!p->fl_extra_dur && dur > actual_dur) {

          p->h.insdshead->offtim +=  dur - actual_dur+ FL(1.0);
          p->h.insdshead->relesing =0;
          p->fl_extra_dur=TRUE;
        }
        else if (dur <= actual_dur) {
          note_off(csound, p->chn, p->num, p->vel);
        }
      }
    }
    return OK;
}

int moscil_set(CSOUND *csound, MOSCIL *p)
{
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;
    p->istart_time = (MYFLT)CS_KCNT * CS_ONEDKR;
    p->fl_first_note   = TRUE;
    p->fl_note_expired = TRUE;
    p->fl_end_note     = FALSE;
    return OK;
}

int moscil(CSOUND *csound, MOSCIL *p)
{
    if (p->fl_first_note) {
      p->fl_first_note = FALSE;
      goto first_note;
    }
    if (!(p->fl_note_expired)) {
      if (p->h.insdshead->relesing) {
        p->fl_note_expired = TRUE;
        p->fl_end_note     = TRUE;
        note_off(csound, p->last_chn, p->last_num, p->last_vel);
      }
      else if (p->last_dur < (MYFLT)CS_KCNT * CS_ONEDKR
                             - p->istart_time) {
        p->fl_note_expired = TRUE;
        note_off(csound, p->last_chn, p->last_num, p->last_vel);
      }
    }
    else {
      if (!p->fl_end_note
          && p->last_pause + p->last_dur <
             (MYFLT)CS_KCNT * CS_ONEDKR - p->istart_time
          && !(p->h.insdshead->relesing)) {
        MYFLT ftemp;
        p->istart_time = p->istart_time + p->last_pause + p->last_dur;
        p->last_dur   =         /* dur must be at least 1/kr */
          (ftemp = *p->kdur) > 0 ? ftemp : CS_ONEDKR;
        p->last_pause = (ftemp = *p->kpause) > 0 ? ftemp : CS_ONEDKR;
 first_note:
        {
          int temp;
          p->last_chn = (temp = abs((int)*p->kchn-1)) < NUMCHN ? temp :
                                                                  NUMCHN-1;
          p->last_num = (temp = abs((int)*p->knum)) < 128    ? temp : 127;
          p->last_vel = (temp = abs((int)*p->kvel)) < 128    ? temp : 127;
        }
        p->fl_note_expired = FALSE;
        note_on(csound, p->last_chn, p->last_num, p->last_vel);
      }
    }
    return OK;
}

int kvar_out_on_set(CSOUND *csound, KOUT_ON *p)
{
    IGN(csound);
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;
    p->fl_first_note = TRUE;
    return OK;
}

int kvar_out_on(CSOUND *csound, KOUT_ON *p)
{
    if (p->fl_first_note) {
      int temp;

      p->last_chn = (temp = abs((int)*p->kchn-1)) < NUMCHN  ? temp : NUMCHN-1;
      p->last_num = (temp = abs((int)*p->knum)) < 128     ? temp : 127;
      p->last_vel = (temp = abs((int)*p->kvel)) < 128     ? temp : 127;
      p->fl_first_note   = FALSE;
      p->fl_note_expired = FALSE;

      note_on(csound, p->last_chn, p->last_num, p->last_vel);
    }
    else if (p->fl_note_expired) return OK;
    else {
      if (p->h.insdshead->relesing) {
        note_off(csound, p->last_chn, p->last_num, p->last_vel);
        p->fl_note_expired = TRUE;
      }
      else {
        int temp;
        int curr_chn = (temp = abs((int)*p->kchn-1)) < NUMCHN ? temp : NUMCHN-1;
        int curr_num = (temp = abs((int)*p->knum)) < 128    ? temp : 127;
        int curr_vel = (temp = abs((int)*p->kvel)) < 128    ? temp : 127;

        if (  p->last_chn != curr_chn
               || p->last_num != curr_num
               || p->last_vel != curr_vel
              ) {
          note_off(csound, p->last_chn, p->last_num, p->last_vel);

          p->last_chn = curr_chn;
          p->last_num = curr_num;
          p->last_vel = curr_vel;

          note_on(csound, curr_chn, curr_num, curr_vel);
        }
      }
    }
    return OK;
}

int out_controller (CSOUND *csound, OUT_CONTR *p)
{
    if (!(p->h.insdshead->prvinstance)) {
      /* if prev instance already allocated in the same MIDI chan */
      int value;
      MYFLT min = *p->min;
      value =  (int)((*p->value - min) * FL(127.0) / (*p->max - min));
      value = (value < 128) ?  value : 127;
      value = (value > -1) ?  value : 0;
      if (value != p->last_value ||
          *p->chn != p->lastchn ||
          *p->num != p->lastctrl) {
        /* csound->Message(csound, "out contr value: %d\n", value); */
        control_change(csound, (int)*p->chn-1,(int)*p->num ,value);
        p->last_value = value;
        p->lastchn = *p->chn;
        p->lastctrl = *p->num;
        }
    }
    return OK;
}

int out_aftertouch (CSOUND *csound, OUT_ATOUCH *p)
{
    if (!(p->h.insdshead->prvinstance)) {
      /* if prev instance already allocated in the same MIDI chan */
      int value;
      MYFLT min = *p->min;
      value =  (int)((*p->value - min) * FL(127.0) / (*p->max - min));
      value = value < 128 ?  value : 127;
      value = value > -1  ?  value : 0;
      if (value != p->last_value || *p->chn != p->lastchn) {
        after_touch(csound, (int)*p->chn-1, value);
        p->last_value = value;
        p->lastchn = *p->chn;
       }
    }
    return OK;
}

int out_poly_aftertouch (CSOUND *csound, OUT_POLYATOUCH *p)
{
    int value;
    MYFLT min = *p->min;
    value =  (int)((*p->value - min) * FL(127.0) / (*p->max - min));
    value = value < 128 ?  value : 127;
    value = value > -1  ?  value : 0;
    if (value != p->last_value ||
        *p->chn != p->lastchn  ||
        *p->num != p->lastctrl) {
      poly_after_touch(csound, (int)*p->chn-1, (int)*p->num, value);
      p->last_value = value;
      p->lastchn = *p->chn;
      p->lastctrl = *p->num;
}

    return OK;
}

int out_progchange (CSOUND *csound, OUT_PCHG *p)
{
    if (!(p->h.insdshead->prvinstance)) {
      /* if prev instance already allocated in the same MIDI chan */
      int prog_num;
      MYFLT min = *p->min;
      prog_num =  (int)((*p->prog_num - min) * FL(127.0) / (*p->max - min));
      prog_num = prog_num < 128 ?  prog_num : 127;
      prog_num = prog_num > -1  ?  prog_num : 0;
      if (prog_num != p->last_prog_num || *p->chn != p->lastchn) {
        program_change(csound, (int)*p->chn-1, prog_num);
        p->last_prog_num = prog_num;
        p->lastchn = *p->chn;
        }
    }
    return OK;
}

int out_controller14 (CSOUND *csound, OUT_CONTR14 *p)
{
    if (!(p->h.insdshead->prvinstance)) {
      /* if prev instance already allocated in the same MIDI chan */
      int value;
      MYFLT min = *p->min;

      value =  (int)((*p->value - min) * FL(16383.0) / (*p->max - min));
      value = (value < 16384) ?  value : 16383;
      value = (value > -1) ?  value : 0;

      if (value != p->last_value  ||
          *p->chn != p->lastchn   ||
          *p->msb_num != p->lastctrl) {
        unsigned int msb = value >> 7;
        unsigned int lsb = value & 0x7F;
        csound->Warning(csound, Str("out contr14 msb:%x lsb:%x\n"), msb, lsb);
        control_change(csound, (int)*p->chn-1, (int)*p->msb_num, msb);
        control_change(csound, (int)*p->chn-1, (int)*p->lsb_num, lsb);
        p->last_value = value;
        p->lastchn = *p->chn;
        p->lastctrl = *p->msb_num;
         }
    }
    return OK;
}

int out_pitch_bend(CSOUND *csound, OUT_PB *p)
{
    if (p->h.insdshead->prvinstance) {
      /* if prev instance already allocated in the same MIDI chan */
      return OK;
    }
    else {
      int   value;
      MYFLT min = *p->min;

      value = (int)((*p->value - min) * FL(16383.0) / (*p->max - min));
      value = (value < 16384  ?  value : 16383);
      value = (value > -1     ?  value : 0);
      if (value != p->last_value || *p->chn != p->lastchn )  {
        unsigned int msb = value >> 7;
        unsigned int lsb = value & 0x7F;
        pitch_bend(csound, (int)*p->chn - 1, lsb, msb);
        p->last_value = value;
        p->lastchn = *p->chn;
        }
    }
    return OK;
}

int kon2_set(CSOUND *csound, KON2 *p)
{
   IGN(csound);
    /* if not initialised by another opcode */
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      p->h.insdshead->xtratim = EXTRA_TIME;
    /*p->fl_first_note = TRUE;*/
    p->fl_note_expired = FALSE;

    return OK;
}

int kon2(CSOUND *csound, KON2 *p)
{
    /*
        if (p->fl_first_note) {
          register int temp;

          p->last_chn = (temp = abs((int)*p->kchn)) < NUMCHN  ? temp : NUMCHN-1;
          p->last_num = (temp = abs((int)*p->knum)) < 128     ? temp : 127;
          p->last_vel = (temp = abs((int)*p->kvel)) < 128     ? temp : 127;
          p->fl_first_note   = FALSE;
          p->fl_note_expired = FALSE;

          note_on(csound, p->last_chn, p->last_num, p->last_vel);
        }

    */
    /*else */
    if (p->fl_note_expired) return OK;
    else {
      if (p->h.insdshead->relesing) {
        note_off(csound, p->last_chn,p->last_num,p->last_vel);
        p->fl_note_expired = TRUE;
      }
      else {
        int temp;

        int curr_chn = (temp = abs((int)*p->kchn-1)) <= NUMCHN ? temp : NUMCHN;
        int curr_num = (temp = abs((int)*p->knum)) < 128    ? temp : 127;
        int curr_vel = (temp = abs((int)*p->kvel)) < 128    ? temp : 127;

        if ((int)(*p->ktrig +FL(0.5)) != 0  )/* i.e. equal to 1  */
                                /*   p->last_chn != curr_chn
                                || p->last_num != curr_num
                                || p->last_vel != curr_vel
                                ) */
          {
            note_off(csound, p->last_chn, p->last_num, p->last_vel);
            p->last_chn = curr_chn;
            p->last_num = curr_num;
            p->last_vel = curr_vel;
            note_on(csound, curr_chn, curr_num, curr_vel);
          }
      }
    }
    return OK;
}

int midiout(CSOUND *csound, MIDIOUT *p)         /*gab-A6 fixed*/
{
    int st, ch, d1, d2;

    st = (int)(*p->in_type + FL(0.5));
    if (!st)
      return OK;
    st = (st & 0x70) | 0x80;
    ch = (int)(*p->in_chan - FL(0.5)) & 0x0F;
    d1 = (int)(*p->in_dat1 + FL(0.5)) & 0x7F;
    d2 = (int)(*p->in_dat2 + FL(0.5)) & 0x7F;
    send_midi_message(csound, st | ch, d1, d2);
    return OK;
}

int nrpn(CSOUND *csound, NRPN *p)
{
    int chan = (int)*p->chan-1, parm = (int)*p->parm_num;
    int value = (int)*p->parm_value;
    if (chan != p->old_chan || parm != p->old_parm || value != p->old_value) {
      int status = 176 | chan;
      int parm_msb =  parm >> 7;
      int parm_lsb =  parm  & 0x7f;

      int value_msb = (value + 8192) >> 7;
      int value_lsb = (value + 8192) % 128;

      send_midi_message(csound, status, 99, parm_msb);
      send_midi_message(csound, status, 98, parm_lsb);
      send_midi_message(csound, status,  6, value_msb);
      send_midi_message(csound, status, 38, value_lsb);
      p->old_chan = chan;
      p->old_parm = parm;
      p->old_value = value;
    }
    return OK;
}

int mdelay_set(CSOUND *csound, MDELAY *p)
{
   IGN(csound);
    p->read_index = 0;
    p->write_index = 0;
    memset(p->status, 0, DELTAB_LENGTH);
    return OK;
}

int mdelay(CSOUND *csound, MDELAY *p)                   /*gab-A6 fixed*/
{
    int read_index = p->read_index % DELTAB_LENGTH;
    int write_index = p->write_index % DELTAB_LENGTH;
    MYFLT present_time =  CS_KCNT * CS_ONEDKR;

    if (((int)*p->in_status == 0x90 || (int)*p->in_status == 0x80)) {
      p->status[write_index] = (int)*p->in_status;
      p->chan[write_index] = (int)*p->in_chan-1;
      p->dat1[write_index] = (int)*p->in_dat1;
      p->dat2[write_index] = (int)*p->in_dat2;
      p->time[write_index] = present_time;
      (p->write_index)++;
    }
    if (p->status[read_index] &&
        p->time[read_index] + *p->kdelay <= present_time) {
      int number = p->dat1[read_index];
      int velocity = p->dat2[read_index];
      send_midi_message(csound,  p->status[read_index] | p->chan[read_index],
                                 ((number   > 127) ? 127 : number),
                                 ((velocity > 127) ? 127 : velocity));
      (p->read_index)++;
    }
    return OK;
}
