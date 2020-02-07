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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
void note_on(CSOUND *, int32_t chan, int32_t num, int32_t vel);
void note_off(CSOUND *, int32_t chan, int32_t num, int32_t vel);
void control_change(CSOUND *, int32_t chan, int32_t num, int32_t value);
void after_touch(CSOUND *, int32_t chan, int32_t value);
void program_change(CSOUND *, int32_t chan, int32_t num);
void pitch_bend(CSOUND *, int32_t chan, int32_t lsb, int32_t msb);
void poly_after_touch(CSOUND *, int32_t chan, int32_t note_num, int32_t value);
void send_midi_message(CSOUND *, int32_t status, int32_t data1, int32_t data2);

int32_t release_set(CSOUND *csound, REL *p)
{
    IGN(csound);
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;
    return OK;
}

int32_t release(CSOUND *csound, REL *p)
{
    IGN(csound);
    if (p->h.insdshead->relesing)
      *p->r = FL(1.0); /* TRUE */
    else
      *p->r = FL(0.0); /* FALSE */
    return OK;
}

int32_t xtratim(CSOUND *csound, XTRADUR *p)
{
    IGN(csound);
    int32_t *xtra = &(p->h.insdshead->xtratim);
    int32_t tim = (int32_t)(*p->extradur * p->h.insdshead->ekr);
    if (*xtra < tim)  /* gab-a5 revised */
      *xtra = tim;
    return OK;
}

int32_t mclock_set(CSOUND *csound, MCLOCK *p)
{
    IGN(csound);
    p->period= CS_EKR / *p->freq;
    p->clock_tics = p->period;
    p->beginning_flag = TRUE;
    return OK;
}

int32_t mclock(CSOUND *csound, MCLOCK *p)
{
    if (UNLIKELY(p->beginning_flag)) {    /* first time */
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

int32_t mrtmsg(CSOUND *csound, MRT *p)
{
    switch ((int32_t)*p->message) {
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
      return csound->InitError(csound, Str("illegal mrtmsg argument"));
    }
    return OK;
}

int32_t iout_on(CSOUND *csound, OUT_ON *p)
{
    note_on(csound, (int32_t)*p->ichn-1,(int32_t)*p->inum,(int32_t)*p->ivel);
    return OK;
}

int32_t iout_off(CSOUND *csound, OUT_ON *p)
{
    note_off(csound, (int32_t)*p->ichn-1,(int32_t)*p->inum,(int32_t)*p->ivel);
    return OK;
}

int32_t iout_on_dur_set(CSOUND *csound, OUT_ON_DUR *p)
{
    int32_t temp;
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;

    p->chn = (temp = abs((int32_t)*p->ichn-1)) < NUMCHN ? temp : NUMCHN-1;
    p->num = (temp = abs((int32_t)*p->inum)) < 128 ? temp : 127;
    p->vel = (temp = abs((int32_t)*p->ivel)) < 128 ? temp : 127;

    note_on(csound, p->chn,p->num, p->vel);
    p->istart_time = (MYFLT)CS_KCNT * CS_ONEDKR;
    p->fl_expired = FALSE;
    p->fl_extra_dur = FALSE;
    return OK;
}

int32_t iout_on_dur(CSOUND *csound, OUT_ON_DUR *p)
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

int32_t iout_on_dur2(CSOUND *csound, OUT_ON_DUR *p)
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

int32_t moscil_set(CSOUND *csound, MOSCIL *p)
{
    IGN(csound);
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;
    p->istart_time = (MYFLT)CS_KCNT * CS_ONEDKR;
    p->fl_first_note   = TRUE;
    p->fl_note_expired = TRUE;
    p->fl_end_note     = FALSE;
    return OK;
}

int32_t moscil(CSOUND *csound, MOSCIL *p)
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
          int32_t temp;
          p->last_chn = (temp = abs((int32_t)*p->kchn-1)) < NUMCHN ? temp :
                                                                  NUMCHN-1;
          p->last_num = (temp = abs((int32_t)*p->knum)) < 128    ? temp : 127;
          p->last_vel = (temp = abs((int32_t)*p->kvel)) < 128    ? temp : 127;
        }
        p->fl_note_expired = FALSE;
        note_on(csound, p->last_chn, p->last_num, p->last_vel);
      }
    }
    return OK;
}

int32_t kvar_out_on_set(CSOUND *csound, KOUT_ON *p)
{
    IGN(csound);
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;
    p->fl_first_note = TRUE;
    return OK;
}

int32_t kvar_out_on(CSOUND *csound, KOUT_ON *p)
{
    if (p->fl_first_note) {
      int32_t temp;

      p->last_chn = (temp = abs((int32_t)*p->kchn-1)) < NUMCHN  ? temp : NUMCHN-1;
      p->last_num = (temp = abs((int32_t)*p->knum)) < 128     ? temp : 127;
      p->last_vel = (temp = abs((int32_t)*p->kvel)) < 128     ? temp : 127;
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
        int32_t tt;
        int32_t curr_chn = (tt = abs((int32_t)*p->kchn-1)) < NUMCHN ? tt : NUMCHN-1;
        int32_t curr_num = (tt = abs((int32_t)*p->knum)) < 128    ? tt : 127;
        int32_t curr_vel = (tt = abs((int32_t)*p->kvel)) < 128    ? tt : 127;

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

int32_t out_controller (CSOUND *csound, OUT_CONTR *p)
{
    /* if (!(p->h.insdshead->prvinstance)) JPff/VL */ {
      /* if prev instance already allocated in the same MIDI chan */
      int32_t value;
      MYFLT min = *p->min;
      value =  (int32_t)((*p->value - min) * FL(127.0) / (*p->max - min));
      value = (value < 128) ?  value : 127;
      value = (value > -1) ?  value : 0;
      if (value != p->last_value ||
          *p->chn != p->lastchn ||
          *p->num != p->lastctrl) {
        /* csound->Message(csound, "out contr value: %d\n", value); */
        control_change(csound, (int32_t)*p->chn-1,(int32_t)*p->num ,value);
        p->last_value = value;
        p->lastchn = *p->chn;
        p->lastctrl = *p->num;
        }
    }
    return OK;
}

int32_t out_aftertouch (CSOUND *csound, OUT_ATOUCH *p)
{
    /* if (!(p->h.insdshead->prvinstance)) JPff/VL */ {
      /* if prev instance already allocated in the same MIDI chan */
      int32_t value;
      MYFLT min = *p->min;
      value =  (int32_t)((*p->value - min) * FL(127.0) / (*p->max - min));
      value = value < 128 ?  value : 127;
      value = value > -1  ?  value : 0;
      if (value != p->last_value || *p->chn != p->lastchn) {
        after_touch(csound, (int32_t)*p->chn-1, value);
        p->last_value = value;
        p->lastchn = *p->chn;
       }
    }
    return OK;
}

int32_t out_poly_aftertouch (CSOUND *csound, OUT_POLYATOUCH *p)
{
    int32_t value;
    MYFLT min = *p->min;
    value =  (int32_t)((*p->value - min) * FL(127.0) / (*p->max - min));
    value = value < 128 ?  value : 127;
    value = value > -1  ?  value : 0;
    if (value != p->last_value ||
        *p->chn != p->lastchn  ||
        *p->num != p->lastctrl) {
      poly_after_touch(csound, (int32_t)*p->chn-1, (int32_t)*p->num, value);
      p->last_value = value;
      p->lastchn = *p->chn;
      p->lastctrl = *p->num;
}

    return OK;
}

int32_t out_progchange (CSOUND *csound, OUT_PCHG *p)
{
    /* if (!(p->h.insdshead->prvinstance)) JPff/VL */ {
      /* if prev instance already allocated in the same MIDI chan */
      int32_t prog_num;
      MYFLT min = *p->min;
      prog_num =  (int32_t)((*p->prog_num - min) * FL(127.0) / (*p->max - min));
      prog_num = prog_num < 128 ?  prog_num : 127;
      prog_num = prog_num > -1  ?  prog_num : 0;
      if (prog_num != p->last_prog_num || *p->chn != p->lastchn) {
        program_change(csound, (int32_t)*p->chn-1, prog_num);
        p->last_prog_num = prog_num;
        p->lastchn = *p->chn;
        }
    }
    return OK;
}

int32_t out_controller14 (CSOUND *csound, OUT_CONTR14 *p)
{
    /* if (!(p->h.insdshead->prvinstance)) JPff/VL */ {
      /* if prev instance already allocated in the same MIDI chan */
      int32_t value;
      MYFLT min = *p->min;

      value =  (int32_t)((*p->value - min) * FL(16383.0) / (*p->max - min));
      value = (value < 16384) ?  value : 16383;
      value = (value > -1) ?  value : 0;

      if (value != p->last_value  ||
          *p->chn != p->lastchn   ||
          *p->msb_num != p->lastctrl) {
        uint32_t msb = value >> 7;
        uint32_t lsb = value & 0x7F;
        csound->Warning(csound, Str("out contr14 msb:%x lsb:%x\n"), msb, lsb);
        control_change(csound, (int32_t)*p->chn-1, (int32_t)*p->msb_num, msb);
        control_change(csound, (int32_t)*p->chn-1, (int32_t)*p->lsb_num, lsb);
        p->last_value = value;
        p->lastchn = *p->chn;
        p->lastctrl = *p->msb_num;
         }
    }
    return OK;
}

int32_t out_pitch_bend(CSOUND *csound, OUT_PB *p)
{
    /* if (p->h.insdshead->prvinstance) { */
    /*   /\* if prev instance already allocated in the same MIDI chan *\/ */
    /*   return OK; */
    /* } */
    /* JPff/VL else */ {
      int32_t   value;
      MYFLT min = *p->min;

      value = (int32_t)((*p->value - min) * FL(16383.0) / (*p->max - min));
      value = (value < 16384  ?  value : 16383);
      value = (value > -1     ?  value : 0);
      if (value != p->last_value || *p->chn != p->lastchn )  {
        uint32_t msb = value >> 7;
        uint32_t lsb = value & 0x7F;
        pitch_bend(csound, (int32_t)*p->chn - 1, lsb, msb);
        p->last_value = value;
        p->lastchn = *p->chn;
        }
    }
    return OK;
}

int32_t kon2_set(CSOUND *csound, KON2 *p)
{
   IGN(csound);
    /* if not initialised by another opcode */
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      p->h.insdshead->xtratim = EXTRA_TIME;
    /*p->fl_first_note = TRUE;*/
    p->fl_note_expired = FALSE;

    return OK;
}

int32_t kon2(CSOUND *csound, KON2 *p)
{
    /*
        if (p->fl_first_note) {
          register int32_t tt;

          p->last_chn = (tt = abs((int32_t)*p->kchn)) < NUMCHN  ? tt : NUMCHN-1;
          p->last_num = (tt = abs((int32_t)*p->knum)) < 128     ? tt : 127;
          p->last_vel = (tt = abs((int32_t)*p->kvel)) < 128     ? tt : 127;
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
        int32_t tt;

        int32_t curr_chn = (tt = abs((int32_t)*p->kchn-1)) <= NUMCHN ? tt : NUMCHN;
        int32_t curr_num = (tt = abs((int32_t)*p->knum)) < 128    ? tt : 127;
        int32_t curr_vel = (tt = abs((int32_t)*p->kvel)) < 128    ? tt : 127;

        if ((int32_t)(*p->ktrig +FL(0.5)) != 0  )/* i.e. equal to 1  */
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

int32_t midiout(CSOUND *csound, MIDIOUT *p)         /*gab-A6 fixed*/
{
    int32_t st, ch, d1, d2;

    st = (int32_t)(*p->in_type + FL(0.5));
    if (!st)
      return OK;
    st = (st & 0x70) | 0x80;
    ch = (int32_t)(*p->in_chan - FL(0.5)) & 0x0F;
    d1 = (int32_t)(*p->in_dat1 + FL(0.5)) & 0x7F;
    d2 = (int32_t)(*p->in_dat2 + FL(0.5)) & 0x7F;
    send_midi_message(csound, st | ch, d1, d2);
    return OK;
}

int32_t nrpn(CSOUND *csound, NRPN *p)
{
    int32_t chan = (int32_t)*p->chan-1, parm = (int32_t)*p->parm_num;
    int32_t value = (int32_t)*p->parm_value;
    if (chan != p->old_chan || parm != p->old_parm || value != p->old_value) {
      int32_t status = 176 | chan;
      int32_t parm_msb =  parm >> 7;
      int32_t parm_lsb =  parm  & 0x7f;

      int32_t value_msb = (value + 8192) >> 7;
      int32_t value_lsb = (value + 8192) % 128;

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

int32_t mdelay_set(CSOUND *csound, MDELAY *p)
{
   IGN(csound);
    p->read_index = 0;
    p->write_index = 0;
    memset(p->status, 0, DELTAB_LENGTH);
    return OK;
}

int32_t mdelay(CSOUND *csound, MDELAY *p)                   /*gab-A6 fixed*/
{
    int32_t read_index = p->read_index % DELTAB_LENGTH;
    int32_t write_index = p->write_index % DELTAB_LENGTH;
    MYFLT present_time =  CS_KCNT * CS_ONEDKR;

    if (((int32_t)*p->in_status == 0x90 || (int32_t)*p->in_status == 0x80)) {
      p->status[write_index] = (int32_t)*p->in_status;
      p->chan[write_index] = (int32_t)*p->in_chan-1;
      p->dat1[write_index] = (int32_t)*p->in_dat1;
      p->dat2[write_index] = (int32_t)*p->in_dat2;
      p->time[write_index] = present_time;
      (p->write_index)++;
    }
    if (p->status[read_index] &&
        p->time[read_index] + *p->kdelay <= present_time) {
      int32_t number = p->dat1[read_index];
      int32_t velocity = p->dat2[read_index];
      send_midi_message(csound,  p->status[read_index] | p->chan[read_index],
                                 ((number   > 127) ? 127 : number),
                                 ((velocity > 127) ? 127 : velocity));
      (p->read_index)++;
    }
    return OK;
}
