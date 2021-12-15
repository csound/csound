/*
    midirecv.c:

    Copyright (C) 1995 Barry Vercoe, John ffitch
              (C) 2005 Istvan Varga

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

/*
    Real time MIDI callback functions:
    ----------------------------------

    int (*MidiInOpenCallback)(CSOUND *csound,
                              void **userData, const char *devName);

      Open MIDI input device 'devName', and store stream specific
      data pointer in *userData. Return value is zero on success,
      and a non-zero error code if an error occured.

    int (*MidiReadCallback)(CSOUND *csound,
                            void *userData, unsigned char *buf, int nbytes);

      Read at most 'nbytes' bytes of MIDI data from input stream
      'userData', and store in 'buf'. Returns the actual number of
      bytes read, which may be zero if there were no events, and
      negative in case of an error. Note: incomplete messages (such
      as a note on status without the data bytes) should not be
      returned.

    int (*MidiInCloseCallback)(CSOUND *csound, void *userData);

      Close MIDI input device associated with 'userData'.
      Return value is zero on success, and a non-zero error
      code on failure.

    int (*MidiOutOpenCallback)(CSOUND *csound,
                               void **userData, const char *devName);

      Open MIDI output device 'devName', and store stream specific
      data pointer in *userData. Return value is zero on success,
      and a non-zero error code if an error occured.

    int (*MidiWriteCallback)(CSOUND *csound, void *userData,
                             const unsigned char *buf, int nbytes);

      Write 'nbytes' bytes of MIDI data to output stream 'userData'
      from 'buf' (the buffer will not contain incomplete messages).
      Returns the actual number of bytes written, or a negative
      error code.

    int (*MidiOutCloseCallback)(CSOUND *csound, void *userData);

      Close MIDI output device associated with '*userData'.
      Return value is zero on success, and a non-zero error
      code on failure.

    const char *(*MidiErrorStringCallback)(int errcode);

      Returns pointer to a string constant storing an error massage
      for error code 'errcode'.

    Setting function pointers:
    --------------------------

    void csoundSetExternalMidiInOpenCallback(CSOUND *csound,
                    int (*func)(CSOUND *, void **, const char *));

    void csoundSetExternalMidiReadCallback(CSOUND *csound,
                    int (*func)(CSOUND *, void *, unsigned char *, int));

    void csoundSetExternalMidiInCloseCallback(CSOUND *csound,
                    int (*func)(CSOUND *, void *));

    void csoundSetExternalMidiOutOpenCallback(CSOUND *csound,
                    int (*func)(CSOUND *, void **, const char *));

    void csoundSetExternalMidiWriteCallback(CSOUND *csound,
                    int (*func)(CSOUND *, void *, const unsigned char *, int));

    void csoundSetExternalMidiOutCloseCallback(CSOUND *csound,
                    int (*func)(CSOUND *, void *));

    void csoundSetExternalMidiErrorStringCallback(CSOUND *csound,
                    const char *(*func)(int));
*/

#include "csoundCore.h"
#include "midiops.h"
#include "midifile.h"

#define MGLOB(x) (csound->midiGlobals->x)

static  void    midNotesOff(CSOUND *);

static const MYFLT dsctl_map[12] = {
    FL(1.0), FL(0.0), FL(1.0), FL(0.0), FL(1.0), FL(0.0),
    FL(1.0), FL(0.0), FL(1.0), FL(0.0), FL(1.0), FL(0.0)
};

static const int16 datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };

/* open a Midi event stream for reading, alloc bufs */
/*     callable once from main.c                    */

void MidiOpen(CSOUND *csound)
{
    MGLOBAL *p = csound->midiGlobals;
    OPARMS  *O = csound->oparms;
    int     err;
    /* First set up buffers. */
    p->Midevtblk = (MEVENT*) csound->Calloc(csound, sizeof(MEVENT));
    p->sexp = 0;
    /* Then open device... */
    if (O->Midiin) {
      if (p->MidiInOpenCallback == NULL)
        csound->Die(csound, Str(" *** no callback for opening MIDI input"));
      if (p->MidiReadCallback == NULL)
        csound->Die(csound, Str(" *** no callback for reading MIDI data"));
      err = p->MidiInOpenCallback(csound, &(p->midiInUserData), O->Midiname);
      if (err != 0) {
        csound->Die(csound, Str(" *** error opening MIDI in device: %d (%s)"),
                            err, csoundExternalMidiErrorString(csound, err));
      }
    }
    /* and file. */
    if (O->FMidiin && O->FMidiname != NULL) {
      if (csoundMIDIFileOpen(csound, O->FMidiname) != 0)
        csound->Die(csound, Str("Failed to load MIDI file."));
    }
}

/* turn off all notes in chnl sust array */
/* called by SUSTAIN_SW_off only if count non-zero */

static void sustsoff(CSOUND *csound, MCHNBLK *chn)
{
    INSDS *ip;
    int   nn;

    if (chn->ksuscnt <= 0) {
      chn->ksuscnt = 0;
      return;
    }
    for (nn = 0; nn < 128; nn++) {
      ip = chn->kinsptr[nn];
      while (ip != NULL) {
        if (ip->m_sust)
          xturnoff(csound, ip);
        ip = ip->nxtolap;
      }
    }
    if (chn->ksuscnt)
      csound->Message(csound, Str("sustain count still %d\n"), chn->ksuscnt);
    chn->ksuscnt = 0;
}

/* reset all controllers for this channel */

void midi_ctl_reset(CSOUND *csound, int16 chan)
{
    MCHNBLK *chn;
    int     i;

    chn = csound->m_chnbp[chan];
    for (i = 1; i <= 135; i++)                  /* from ctlr 1 to ctlr 128 */
      chn->ctl_val[i] = FL(0.0);                /*   reset all ctlrs to 0  */
    /* exceptions:  */
    if (!MGLOB(rawControllerMode)) {
      chn->ctl_val[7]  = FL(127.0);             /*   volume           */
      chn->ctl_val[8]  = FL(64.0);              /*   balance          */
      chn->ctl_val[10] = FL(64.0);              /*   pan              */
      chn->ctl_val[11] = FL(127.0);             /*   expression       */
    }
    else
      chn->ctl_val[0]  = FL(0.0);
    chn->pbensens = FL(2.0);                    /*   pitch bend range */
    chn->datenabl = 0;
    /* reset aftertouch to max value - added by Istvan Varga, May 2002 */
    chn->aftouch = csound->aftouch;
    for (i = 0; i < 128; i++)
      chn->polyaft[i] = csound->aftouch;
    /* controller 64 has just been set to zero: terminate any held notes */
    if (chn->ksuscnt && !MGLOB(rawControllerMode))
      sustsoff(csound, chn);
    chn->sustaining = 0;
    /* reset pitch bend */
    chn->pchbend = FL(0.0);
}

/* execute non-note channel voice and channel mode commands */

void m_chanmsg(CSOUND *csound, MEVENT *mep)
{
    MCHNBLK *chn = csound->m_chnbp[mep->chan];
    int16   n;
    MYFLT   *fp;

    switch (mep->type) {
    case PROGRAM_TYPE:                  /* PROGRAM CHANGE */
      chn->pgmno = mep->dat1;
      if (chn->insno <= 0)              /* ignore if channel is muted */
        break;
      n = (int16) chn->pgm2ins[mep->dat1];      /* program change -> INSTR  */
      if (n > 0 &&
          n <= csound->engineState.maxinsno &&  /* if corresp instr exists  */
          csound->engineState.instrtxtp[n] != NULL) {   /* assign as insno  */
        chn->insno = n;                         /* else ignore prog. change */
        csound->Message(csound, Str("midi channel %d now using instr %d\n"),
                        mep->chan + 1, chn->insno);
      }
      break;
    case POLYAFT_TYPE:
      chn->polyaft[mep->dat1] = mep->dat2;      /* Polyphon per-Key Press  */
      break;
    case CONTROL_TYPE:                  /* CONTROL CHANGE MESSAGES: */
      n = mep->dat1;
      if (MGLOB(rawControllerMode)) {           /* "raw" mode:        */
        chn->ctl_val[n] = (MYFLT) mep->dat2;    /*   only store value */
        break;
      }
      if (n >= 111)                             /* if special, redirect */
        goto special;
      if (n == NRPNMSB || n == RPNMSB) {
        chn->dpmsb = mep->dat2;
      }
      else if (n == NRPNLSB || n == RPNLSB) {
        chn->dplsb = mep->dat2;
        if (chn->dplsb == 127 && chn->dpmsb == 127)
          chn->datenabl = 0;
        else
          chn->datenabl = 1;
      }
      else if (n == DATENTRY && chn->datenabl) {
        int   msb = chn->dpmsb;
        int   lsb = chn->dplsb;
        MYFLT fval;
        if (msb == 0 && lsb == 0) {
          chn->pbensens = (MYFLT) mep->dat2;
        }
        else if (msb == 1) {            /* GS system PART PARAMS */
          int ctl;
          switch(lsb) {
          case 8:  ctl = VIB_RATE;        break;
          case 9:  ctl = VIB_DEPTH;       break;
          case 10: ctl = VIB_DELAY;       break;
          case 32: ctl = TVF_CUTOFF;      break;
          case 33: ctl = TVF_RESON;       break;
          case 99: ctl = TVA_RIS;         break;
          case 100:ctl = TVA_DEC;         break;
          case 102:ctl = TVA_RLS;         break;
          default: csound->Message(csound, Str("unknown NPRN lsb %d\n"), lsb);
            goto err;
          }
          fval = (MYFLT) (mep->dat2 - 64);
          chn->ctl_val[ctl] = fval;             /* then store     */
        }
        else {
          if (msb < 24 || msb == 25 || msb == 27 ||
              msb > 31 || lsb < 25  || lsb > 87)
            csound->Message(csound,
                            Str("unknown drum param nos, msb %d lsb %d\n"),
                            (int) msb, (int) lsb);
          else {
            static const int drtab[8] = {0,0,1,1,2,3,4,5};
            int parnum = drtab[msb - 24];
            if (parnum == 0)
              fval = (MYFLT) (mep->dat2 - 64);
            else fval = mep->dat2;
            //            if (dsctl_map != NULL) { always true
            fp = (MYFLT*) &(dsctl_map[parnum*2]);
            //**** FIXME This if statement does nothing as val is not used ****
            if (*fp != FL(0.0)) {
              MYFLT xx = (fval * *fp++);
              fval = xx + *fp;                /* optionally map */
              chn->ctl_val[parnum] = fval;        /* VL: 07.09.20 store it? */
            }
            csound->Message(csound, Str("CHAN %d DRUMKEY %d not in keylst, "
                                        "PARAM %d NOT UPDATED\n"),
                                    (int) mep->chan + 1, (int) lsb, (int) msb);
          }
        }
      }
      else
        chn->ctl_val[n] = (MYFLT) mep->dat2;    /* record data as MYFLT */
    err:
      if (n == SUSTAIN_SW) {                    /* if sustainP changed  */
        if (mep->dat2 > 0)
          chn->sustaining = 1;
        else if (chn->sustaining) {             /*  & going off         */
          chn->sustaining = 0;
          sustsoff(csound, chn);                /*      reles any notes */
        }
      }
      break;

    special:
      if (n < 121) {          /* for ctrlr 111, 112, ... chk inexclus lists */
        if ((csound->oparms->msglevel & 7) == 7)
          csound->Message(csound, Str("ctrl %d has no exclus list\n"), (int) n);
        break;
      }
      /* 121 == RESET ALL CONTROLLERS */
      if (n == 121) {                           /* CHANNEL MODE MESSAGES:  */
        midi_ctl_reset(csound, mep->chan);
      }
      else if (n == 122) {                      /* absorb lcl ctrl data */
/*      int lcl_ctrl = mep->dat2;  ?? */        /* 0:off, 127:on */
      }
      else if (n == 123) midNotesOff(csound);   /* allchnl AllNotesOff */
      else if (n == 126) {                      /* MONO mode */
        if (chn->monobas == NULL) {
          MONPCH *mnew, *mend;
          chn->monobas = (MONPCH *)csound->Calloc(csound, (long)sizeof(MONPCH) * 8);
          mnew = chn->monobas;  mend = mnew + 8;
          do {
            mnew->pch = -1;
          } while (++mnew < mend);
        }
        chn->mono = 1;
      }
      else if (n == 127) {                      /* POLY mode */
        if (chn->monobas != NULL) {
          csound->Free(csound, (char *)chn->monobas);
          chn->monobas = NULL;
        }
        chn->mono = 0;
      }
      else
        csound->Message(csound, Str("chnl mode msg %d not implemented\n"), n);
      break;
    case AFTOUCH_TYPE:
      chn->aftouch = mep->dat1;                 /* chanl (all-key) Press */
      break;
    case PCHBEND_TYPE:
      chn->pchbend = (MYFLT)(((mep->dat2 - 64) << 7) + mep->dat1)/FL(8192.0);
      break;
    case SYSTEM_TYPE:           /* sys_common 1-3 only:  chan contains which */
      switch(mep->chan) {
      case 1:
      case 2:
      case 3:
        break;
      default:
        csound->Die(csound, Str("unrecognised sys_common type %d"), mep->chan);
      }
      break;
    default:
      csound->Die(csound, Str("unrecognised message type %d"), mep->type);
    }
}

/* initialise all MIDI channels; called by musmon before oload() */

void m_chn_init_all(CSOUND *csound)
{                               /* alloc a midi control blk for a midi chnl */
    MCHNBLK *chn;               /*  & assign corr instr n+1, else a default */
    int     defaultinsno, n;
    int16   chan;

    defaultinsno = 0;
    while (csound->engineState.instrtxtp &&
           ++defaultinsno <= (int) csound->engineState.maxinsno &&
           csound->engineState.instrtxtp[defaultinsno] == NULL);
    if (defaultinsno > (int) csound->engineState.maxinsno)
      defaultinsno = 0;         /* no instruments */
    for (chan = (int16) 0; chan < (int16) MIDIMAXPORTS; chan++) {
      /* alloc a midi control blk for midi channel */
      /*  & assign default instrument number       */
      csound->m_chnbp[chan] =
        chn = (MCHNBLK*) csound->Calloc(csound, sizeof(MCHNBLK));
      n = (int) chan + 1;
      /* if corresponding instrument exists, assign as insno, */
      if (csound->engineState.instrtxtp &&
          n <= (int) csound->engineState.maxinsno &&
          csound->engineState.instrtxtp[n] != NULL)
        chn->insno = (int16) n;
      else if (defaultinsno > 0)
        chn->insno = (int16) defaultinsno;
      else
        chn->insno = (int16) -1;        /* else mute channel */
      /* reset all controllers */
      chn->pgmno = -1;
      midi_ctl_reset(csound, chan);
      for (n = 0; n < 128; n++)
        chn->pgm2ins[n] = (int16) (n + 1);
      if (csound->oparms->Midiin || csound->oparms->FMidiin) {
        if (chn->insno < 0)
          csound->Message(csound, Str("midi channel %d is muted\n"), chan + 1);
        //else
        //  csound->Message(csound, Str("midi channel %d using instr %d\n"),
        //                  chan + 1, chn->insno);
      }
    }
}

/* assign an insno to a chnl */
/* =massign: called from i0  */

int m_chinsno(CSOUND *csound, int chan, int insno, int reset_ctls)
{
    MCHNBLK  *chn;
    MEVENT   mev;

    if (chan < 0 || chan > 15)
      return csound->InitError(csound, Str("illegal channel number"));
    chn = csound->m_chnbp[chan];
    if (insno <= 0) {
      chn->insno = -1;
      csound->Message(csound, Str("MIDI channel %d muted\n"), chan + 1);
    }
    else {
      if (insno > csound->engineState.maxinsno ||
          csound->engineState.instrtxtp[insno] == NULL) {
        csound->Message(csound, Str("Insno = %d\n"), insno);
        return csound->InitError(csound, Str("unknown instr"));
      }
      chn->insno = (int16) insno;
      csound->Message(csound, Str("chnl %d using instr %d\n"),
                              chan + 1, chn->insno);
      /* check for program change: will override massign if enabled */
      if (chn->pgmno >= 0) {
        mev.type = PROGRAM_TYPE;
        mev.chan = (int16) chan;
        mev.dat1 = chn->pgmno;
        mev.dat2 = 0;
        m_chanmsg(csound, &mev);
      }
    }
    if (reset_ctls)
      midi_ctl_reset(csound, (int16) chan);
    return OK;
}

static void AllNotesOff(CSOUND *csound, MCHNBLK *chn)
{
    INSDS   *ip;
    int     nn;

    for (nn = 0; nn < 128; nn++) {
      ip = chn->kinsptr[nn];
      while (ip != NULL) {
/*      xturnoff_now(csound, ip);   */
        xturnoff(csound, ip);   /* allow release - is this correct ? */
        ip = ip->nxtolap;
      }
    }
}

/* turn off ALL curr midi notes, ALL chnls */
/*  called by musmon, ctrl 123 & sensMidi  */

static void midNotesOff(CSOUND *csound)
{
    int chan = 0;
    do {
      AllNotesOff(csound, csound->m_chnbp[chan]);
    } while (++chan < MAXCHAN);
}

/* sense a MIDI event, collect the data & dispatch */
/* called from sensevents(), returns 2 if MIDI on/off */

int sensMidi(CSOUND *csound)
{
    MGLOBAL *p = csound->midiGlobals;
    MEVENT  *mep = p->Midevtblk;
    OPARMS  *O = csound->oparms;
    int     n;
    int16   c, type;

 nxtchr:
    if (p->bufp >= p->endatp) {
      p->bufp = &(p->mbuf[0]);
      p->endatp = p->bufp;
      if (O->Midiin && !csound->advanceCnt) {   /* read MIDI device */
        n = p->MidiReadCallback(csound, p->midiInUserData, p->bufp, MBUFSIZ);
        if (n < 0)
          csoundErrorMsg(csound, Str(" *** error reading MIDI device: %d (%s)"),
                                 n, csoundExternalMidiErrorString(csound, n));
        else
          p->endatp += (int) n;
      }
      if (O->FMidiin) {                         /* read MIDI file */
        n = csoundMIDIFileRead(csound, p->endatp,
                               MBUFSIZ - (int) (p->endatp - p->bufp));
        if (n > 0)
          p->endatp += (int) n;
      }
      if (p->endatp <= p->bufp)
        return 0;               /* no events were received */
    }

    if ((c = *(p->bufp++)) & 0x80) {    /* STATUS byte:         */
      type = c & 0xF0;
      if (type == SYSTEM_TYPE) {
        int16 lo3 = (c & 0x07);
        if (c & 0x08)                   /* sys_realtime:        */
          switch (lo3) {                /*   dispatch now       */
          case 0: /* m_clktim++; */     /* timing clock         */
          case 2:                       /* start                */
          case 3:                       /* continue             */
          case 4:                       /* stop                 */
          case 6:                       /* active sensing       */
          case 7:                       /* system reset         */
            goto nxtchr;
          default:
            csound->Message(csound, Str("undefined sys-realtime msg %x\n"), c);
            goto nxtchr;
          }
        else {                          /* sys_non-realtime status: */
          p->sexp = 0;                  /* implies sys_exclus end   */
          switch (lo3) {                /* dispatch on lo3:     */
          case 7: goto nxtchr;          /* EOX: already done    */
          case 0: p->sexp = 1;          /* sys_ex begin:        */
            goto nxtchr;                /*   goto copy data     */
          /* sys_common: need some data, so build evt */
          case 1:                       /* MTC quarter frame    */
          case 3: p->datreq = 1;        /* song select          */
            break;
          case 2: p->datreq = 2;        /* song position        */
            break;
          case 6:                       /* tune request         */
            goto nxtchr;
          default:
            csound->Message(csound, Str("undefined sys_common msg %x\n"), c);
            p->datreq = 32767;          /* waste any data following */
            p->datcnt = 0;
            goto nxtchr;
          }
        }
        mep->type = type;               /* begin sys_com event  */
        mep->chan = lo3;                /* holding code in chan */
        p->datcnt = 0;
        goto nxtchr;
      }
      else {                            /* other status types:  */
        int16 chan;
        p->sexp = 0;                    /* also implies sys_exclus end */
        chan = c & 0xF;
        mep->type = type;               /* & begin new event    */
        mep->chan = chan;
        p->datreq = datbyts[(type>>4) & 0x7];
        if(*p->bufp & 0x80  && p->datreq > 0)
        {/*
            if there is another status byte inserted after
            a chan msg status byte, this implies a port number
            has been stored before the data bytes
            *ONLY* if there are data bytes following
            (not sure if there are times when this is not the case)
            MIDI backend modules can now insert a port number coded in
            this way to allow for separate mapping of devices
          */
          int port = *(p->bufp++) & 0x0F;
          if(port >= MIDIMAXPORTS) {
            csoundWarning(csound, Str("port: %d exceeds max number of ports %d"
                                      ", mapping to port 0"), port, MIDIMAXPORTS);
          } else mep->chan += 16*port;
        }
        p->datcnt = 0;
        goto nxtchr;
      }
    }
    if (p->sexp != 0) {                 /* NON-STATUS byte:     */
      goto nxtchr;
    }
    if (p->datcnt == 0)
      mep->dat1 = c;                    /* else normal data     */
    else mep->dat2 = c;
    if (++p->datcnt < p->datreq)        /* if msg incomplete    */
      goto nxtchr;                      /*   get next char      */
    /* Enter the input event into a buffer used by 'midiin'. */
    /* VL -- changed to allow higher-mapped channels */
    if (mep->type != SYSTEM_TYPE) {
      unsigned char *pMessage =
                    &(p->MIDIINbuffer2[p->MIDIINbufIndex++].bData[0]);
       p->MIDIINbufIndex &= MIDIINBUFMSK;
      *pMessage++ = mep->type; // | mep->chan;
      *pMessage++ = mep->chan + 1;
      *pMessage++ = (unsigned char) mep->dat1;
      *pMessage = (p->datreq < 2 ? (unsigned char) 0 : mep->dat2);
    }
    p->datcnt = 0;                      /* else allow a repeat  */
    /* NB:  this allows repeat in syscom 1,2,3 too */
    if (mep->type > NOTEON_TYPE) {      /* if control or syscom */
      m_chanmsg(csound, mep);           /*   handle from here   */
      goto nxtchr;                      /*   & go look for more */
    }
    return 2;                           /* else it's note_on/off */
}

extern void csoundCloseMidiOutFile(CSOUND *);

void MidiClose(CSOUND *csound)
{
    MGLOBAL *p = csound->midiGlobals;
    int     retval;

    if (p==NULL) {
      printf(Str("No MIDI\n"));
      return;
    }
    if (p->MidiInCloseCallback != NULL) {
      retval = p->MidiInCloseCallback(csound, p->midiInUserData);
      if (retval != 0)
        csoundErrorMsg(csound, Str("Error closing MIDI in device: %d (%s)"),
                       retval, csoundExternalMidiErrorString(csound, retval));
    }
    p->midiInUserData = NULL;
    if (p->MIDIoutDONE && p->MidiOutCloseCallback != NULL) {
      retval = p->MidiOutCloseCallback(csound, p->midiOutUserData);
      if (retval != 0)
        csoundErrorMsg(csound, Str("Error closing MIDI out device: %d (%s)"),
                       retval, csoundExternalMidiErrorString(csound, retval));
    }
    p->MIDIoutDONE = 0;
    p->midiOutUserData = NULL;
    if (p->midiFileData != NULL) {
      csoundMIDIFileClose(csound);
      p->midiFileData = NULL;
    }
    if (p->midiOutFileData != NULL) {
      csoundCloseMidiOutFile(csound);
      p->midiOutFileData = NULL;
    }
}
