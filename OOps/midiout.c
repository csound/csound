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
#include "cs.h"
#include "midiout.h"

#define NUMCHN          (16)
#define EXTRA_TIME      (1)

extern void openMIDIout(void);
/* static MYFLT   invkr; */
void note_on(int chan, int num, int vel);
void note_off(int chan, int num, int vel);
void control_change(int chan, int num, int value);
void after_touch(int chan, int value);
void program_change(int chan, int num);
void pitch_bend(int chan, int lsb, int msb);
void poly_after_touch(int chan, int note_num, int value);
void send_midi_message(int status, int data1, int data2);

int release_set(REL *p)
{
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;
    return OK;
}

int release(REL *p)
{
    if (p->h.insdshead->relesing)
      *p->r=FL(1.0); /* TRUE */
    else
      *p->r=FL(0.0); /* FALSE */
    return OK;
}

int xtratim(XTRADUR *p)
{
    int *xtra = &(p->h.insdshead->xtratim);
    int tim = (int) (*p->extradur * ekr_);
    if (*xtra < tim)  /* gab-a5 revised */
      *xtra = tim;
    return OK;
}

int mclock_set(MCLOCK *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    p->period= ekr_ / *p->freq;
    p->clock_tics = p->period;
    p->beginning_flag=TRUE;
    return OK;
}

int mclock(MCLOCK *p)
{
    if (p->beginning_flag) {    /* first time */
      send_midi_message(0xF8, 0, 0); /* clock message */
      p->beginning_flag=FALSE;
      return OK;
    }
    else if ((MYFLT) kcounter > p->clock_tics) {
      send_midi_message(0xF8, 0, 0); /* clock message */
      p->clock_tics += p->period;
    }
    return OK;
}


int mrtmsg(MRT *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    switch ((int) *p->message) {
    case 0:
      send_midi_message(0xFC, 0, 0); /* stop */
      break;
    case 1:
      send_midi_message(0xFA, 0, 0); /* start */
      break;
    case 2:
      send_midi_message(0xFB, 0, 0); /* continue */
      break;
    case -1:
      send_midi_message(0xFF, 0, 0); /* system_reset */
      break;
    case -2:

      send_midi_message(0xFE, 0, 0); /* active_sensing */
      break;
    default:
      initerror(Str(X_871,"illegal mrtmsg argument"));
    }
    return OK;
}


int iout_on(OUT_ON *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    note_on((int)*p->ichn-1,(int)*p->inum,(int)*p->ivel);
    return OK;
}

int iout_off(OUT_ON *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    note_off((int)*p->ichn-1,(int)*p->inum,(int)*p->ivel);
    return OK;
}

int iout_on_dur_set(OUT_ON_DUR *p)
{
    int temp;
    if (MIDIoutDONE==0) openMIDIout();
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;

    p->chn = (temp = abs((int) *p->ichn-1)) < NUMCHN ? temp : NUMCHN-1;
    p->num = (temp = abs((int) *p->inum)) < 128 ? temp : 127;
    p->vel = (temp = abs((int) *p->ivel)) < 128 ? temp : 127;

    note_on(p->chn,p->num, p->vel);
    p->istart_time = (MYFLT) kcounter * onedkr ;
    p->fl_expired = FALSE;
    p->fl_extra_dur = FALSE;
    return OK;
}


int iout_on_dur(OUT_ON_DUR *p)
{
    if (!(p->fl_expired)) {
      MYFLT actual_dur = (MYFLT) kcounter * onedkr - p->istart_time;
      MYFLT dur = *p->idur;
      if (dur < actual_dur) {
        p->fl_expired = TRUE;
        note_off(p->chn, p->num, p->vel);
      }
      else if (p->h.insdshead->relesing) {
        p->fl_expired = TRUE;
        note_off(p->chn, p->num, p->vel);
      }
    }
    return OK;
}

int iout_on_dur2(OUT_ON_DUR *p)
{
    if (!(p->fl_expired)) {
      MYFLT actual_dur = (MYFLT) kcounter * onedkr - p->istart_time;
      MYFLT dur = *p->idur;
      if (dur < actual_dur) {
        p->fl_expired = TRUE;
        note_off(p->chn, p->num, p->vel);
      }
      else if (p->h.insdshead->relesing || p->fl_extra_dur) {

        if (!p->fl_extra_dur && dur > actual_dur) {

          p->h.insdshead->offtim +=  dur - actual_dur+ FL(1.0);
          p->h.insdshead->relesing =0;
          p->fl_extra_dur=TRUE;
        }
        else if (dur <= actual_dur) {
          note_off(p->chn, p->num, p->vel);
        }
      }
    }
    return OK;
}


int moscil_set(MOSCIL *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;
    p->istart_time = (MYFLT) kcounter * onedkr;
    p->fl_first_note   = TRUE;
    p->fl_note_expired = TRUE;
    p->fl_end_note     = FALSE;
    return OK;
}

int moscil(MOSCIL *p)
{
    if (p->fl_first_note) {
      p->fl_first_note = FALSE;
      goto first_note;
    }
    if (!(p->fl_note_expired)) {
      if (p->h.insdshead->relesing) {
        p->fl_note_expired = TRUE;
        p->fl_end_note     = TRUE;
        note_off(p->last_chn,p->last_num,p->last_vel);
      }
      else if (p->last_dur < (MYFLT) kcounter * onedkr - p->istart_time) {
        p->fl_note_expired = TRUE;
        note_off(p->last_chn,p->last_num,p->last_vel);
      }
    }
    else {
      if (!p->fl_end_note
          && p->last_pause + p->last_dur <
             (MYFLT) kcounter * onedkr - p->istart_time
          && !(p->h.insdshead->relesing)) {
        MYFLT ftemp;
        p->istart_time = p->istart_time + p->last_pause + p->last_dur;
        p->last_dur   =         /* dur must be at least 1/kr */
          (ftemp = *p->kdur) > 0 ? ftemp : onedkr;
        p->last_pause = (ftemp = *p->kpause) > 0 ? ftemp : onedkr;
 first_note:
        {
          int temp;
          p->last_chn = (temp = abs((int) *p->kchn-1)) < NUMCHN ? temp :
                                                                  NUMCHN-1;
          p->last_num = (temp = abs((int) *p->knum)) < 128    ? temp : 127;
          p->last_vel = (temp = abs((int) *p->kvel)) < 128    ? temp : 127;
        }
        p->fl_note_expired = FALSE;
        note_on(p->last_chn, p->last_num, p->last_vel);
      }
    }
    return OK;
}

int kvar_out_on_set(KOUT_ON *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      /* if not initialised by another opcode */
      p->h.insdshead->xtratim = EXTRA_TIME;
    p->fl_first_note = TRUE;
    return OK;
}

int kvar_out_on(KOUT_ON *p)
{
    if (p->fl_first_note) {
      int temp;

      p->last_chn = (temp = abs((int) *p->kchn-1)) < NUMCHN  ? temp : NUMCHN-1;
      p->last_num = (temp = abs((int) *p->knum)) < 128     ? temp : 127;
      p->last_vel = (temp = abs((int) *p->kvel)) < 128     ? temp : 127;
      p->fl_first_note   = FALSE;
      p->fl_note_expired = FALSE;

      note_on(p->last_chn, p->last_num, p->last_vel);
    }
    else if (p->fl_note_expired) return OK;
    else {
      if (p->h.insdshead->relesing) {
        note_off(p->last_chn,p->last_num,p->last_vel);
        p->fl_note_expired = TRUE;
      }
      else {
        int temp;
        int curr_chn = (temp = abs((int) *p->kchn-1)) < NUMCHN ? temp : NUMCHN-1;
        int curr_num = (temp = abs((int) *p->knum)) < 128    ? temp : 127;
        int curr_vel = (temp = abs((int) *p->kvel)) < 128    ? temp : 127;

        if (  p->last_chn != curr_chn
               || p->last_num != curr_num
               || p->last_vel != curr_vel
              ) {
          note_off(p->last_chn,p->last_num,p->last_vel);

          p->last_chn = curr_chn;
          p->last_num = curr_num;
          p->last_vel = curr_vel;

          note_on(curr_chn, curr_num, curr_vel);
        }
      }
    }
    return OK;
}


int out_controller (OUT_CONTR *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    if (!(p->h.insdshead->prvinstance)) {
      /* if prev instance already allocated in the same MIDI chan */
      int value;
      MYFLT min = *p->min;
      value =  (int)((*p->value - min) * 127. / (*p->max - min));
      value = (value < 128) ?  value : 127;
      value = (value > -1) ?  value : 0;
      if (value != p->last_value) {
        /* printf("out contr value: %d\n", value); */
        control_change((int)*p->chn-1,(int)*p->num ,value);
        p->last_value = value;
      }
    }
    return OK;
}

int out_aftertouch (OUT_ATOUCH *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    if (!(p->h.insdshead->prvinstance)) {
      /* if prev instance already allocated in the same MIDI chan */
      int value;
      MYFLT min = *p->min;
      value =  (int)((*p->value - min) * 127. / (*p->max - min));
      value = value < 128 ?  value : 127;
      value = value > -1  ?  value : 0;
      if (value != p->last_value) {
        after_touch((int)*p->chn-1, value);
        p->last_value = value;
      }
    }
    return OK;
}

int out_poly_aftertouch (OUT_POLYATOUCH *p)
{
    int value;
    MYFLT min = *p->min;
    if (MIDIoutDONE==0) openMIDIout();
    value =  (int)((*p->value - min) * 127. / (*p->max - min));
    value = value < 128 ?  value : 127;
    value = value > -1  ?  value : 0;
    if (value != p->last_value) {
      poly_after_touch((int) *p->chn-1, (int) *p->num, value);
      p->last_value = value;
    }

    return OK;
}

int out_progchange (OUT_PCHG *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    if (!(p->h.insdshead->prvinstance)) {
      /* if prev instance already allocated in the same MIDI chan */
      int prog_num;
      MYFLT min = *p->min;
      prog_num =  (int)((*p->prog_num - min) * 127. / (*p->max - min));
      prog_num = prog_num < 128 ?  prog_num : 127;
      prog_num = prog_num > -1  ?  prog_num : 0;
      if (prog_num != p->last_prog_num) {
        program_change((int) *p->chn-1, prog_num);
        p->last_prog_num = prog_num;
      }
    }
    return OK;
}



int out_controller14 (OUT_CONTR14 *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    if (!(p->h.insdshead->prvinstance)) {
      /* if prev instance already allocated in the same MIDI chan */
      int value;
      MYFLT min = *p->min;

      value =  (int)((*p->value - min) * 16383. / (*p->max - min));
      value = (value < 16384) ?  value : 16383;
      value = (value > -1) ?  value : 0;

      if (value != p->last_value) {
        unsigned int msb = value >> 7;
        unsigned int lsb = value & 0x7F;
        printf(Str(X_1110,"out contr14 msb:%x lsb:%x\n"), msb, lsb);

        control_change((int) *p->chn-1, (int)*p->msb_num, msb);
        control_change((int)*p->chn-1, (int)*p->lsb_num, lsb);
        p->last_value = value;
      }
    }
    return OK;
}

int out_pitch_bend(OUT_PB *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    if (!(p->h.insdshead->prvinstance)) {
      /* if prev instance already allocated in the same MIDI chan */
      int value;
      MYFLT min = *p->min;
      if (p->h.insdshead->prvinstance != NULL)
        return OK; /* if prev instance already allocated in the same MIDI chan */
      value = (int)((*p->value - min) * 16383. / (*p->max - min));
      value = value < 16384  ?  value : 16383;
      value = value > -1     ?  value : 0;
      if (value != p->last_value) {
        unsigned int msb = value >> 7;
        unsigned int lsb = value & 0x7F;
        pitch_bend((int) *p->chn-1, msb, lsb);
        p->last_value = value;
      }
    }
    return OK;
}


int kon2_set(KON2 *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    if (p->h.insdshead->xtratim < EXTRA_TIME)
      p->h.insdshead->xtratim = EXTRA_TIME; /* if not initialised by another opcode */
    /*p->fl_first_note = TRUE;*/
    p->fl_note_expired = FALSE;

    return OK;
}


int kon2(KON2 *p)
{
    /*
        if (p->fl_first_note) {
                register int temp;

                p->last_chn = (temp = abs((int) *p->kchn)) < NUMCHN  ? temp : NUMCHN-1;
                p->last_num = (temp = abs((int) *p->knum)) < 128     ? temp : 127;
                p->last_vel = (temp = abs((int) *p->kvel)) < 128     ? temp : 127;
                p->fl_first_note   = FALSE;
                p->fl_note_expired = FALSE;

                note_on(p->last_chn, p->last_num, p->last_vel);
        }

    */
    /*else */
    if (p->fl_note_expired) return OK;
    else {
      if (p->h.insdshead->relesing) {
        note_off(p->last_chn,p->last_num,p->last_vel);
        p->fl_note_expired = TRUE;
      }
      else {
        int temp;

        int curr_chn = (temp = abs((int) *p->kchn-1)) <= NUMCHN ? temp : NUMCHN;
        int curr_num = (temp = abs((int) *p->knum)) < 128    ? temp : 127;
        int curr_vel = (temp = abs((int) *p->kvel)) < 128    ? temp : 127;

        if ((int)(*p->ktrig +.5) != 0  )/* i.e. equal to 1  */
                                /*   p->last_chn != curr_chn
                                || p->last_num != curr_num
                                || p->last_vel != curr_vel
                                ) */
          {
            note_off(p->last_chn,p->last_num,p->last_vel);
            p->last_chn = curr_chn;
            p->last_num = curr_num;
            p->last_vel = curr_vel;
            note_on(curr_chn, curr_num, curr_vel);
          }
      }
    }
    return OK;
}


int midiout(MIDIOUT *p)        /*gab-A6 fixed*/
{
    int kstatus ;
    if (MIDIoutDONE==0) openMIDIout();
    if ((kstatus = (int) *p->in_type))
      send_midi_message( ((int) *p->in_type) | ((int) *p->in_chan-1),
                         (int) *p->in_dat1, (int) *p->in_dat2 );
    return OK;
}


int nrpn(NRPN *p)
{
    int chan = (int) *p->chan-1, parm = (int) *p->parm_num, value = (int) *p->parm_value;
    if (MIDIoutDONE==0) openMIDIout();
    if (chan != p->old_chan || parm != p->old_parm || value != p->old_value) {
      int status = 176 | chan;
      int parm_msb =  parm >> 7;
      int parm_lsb =  parm  & 0x7f;

      int value_msb = (value + 8192) >> 7;
      int value_lsb = (value + 8192) % 128;

      send_midi_message(status, 99, parm_msb);
      send_midi_message(status, 98, parm_lsb);
      send_midi_message(status, 6       , value_msb);
      send_midi_message(status, 38, value_lsb);
      p->old_chan = chan;
      p->old_parm = parm;
      p->old_value = value;
    }
    return OK;
}


int mdelay_set(MDELAY *p)
{
    if (MIDIoutDONE==0) openMIDIout();
    p->read_index = 0;
    p->write_index = 0;
    memset(p->status, 0, DELTAB_LENGTH);
    return OK;
}


int mdelay(MDELAY *p)                                                                          /*gab-A6 fixed*/
{
    int read_index = p->read_index % DELTAB_LENGTH;
    int write_index = p->write_index % DELTAB_LENGTH;
    MYFLT present_time =  kcounter * onedkr;

    if (((int) *p->in_status == 0x90 || (int) *p->in_status == 0x80)) {
      p->status[write_index] = (int) *p->in_status;
      p->chan[write_index] = (int) *p->in_chan-1;
      p->dat1[write_index] = (int) *p->in_dat1;
      p->dat2[write_index] = (int) *p->in_dat2;
      p->time[write_index] = present_time;
      (p->write_index)++;
    }
    if (p->status[read_index] &&
        p->time[read_index] + *p->kdelay <= present_time) {
      int number = p->dat1[read_index];
      int velocity = p->dat2[read_index];
      send_midi_message( p->status[read_index] | p->chan[read_index],
                        ((number   > 127) ? 127 : number),
                        ((velocity > 127) ? 127 : velocity));
      (p->read_index)++;
    }
    return OK;
}

