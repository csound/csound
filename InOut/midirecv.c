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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*
    Real time MIDI interface functions:
    -----------------------------------

    int csoundExternalMidiInOpen(void *csound, void **userData,
                                 const char *devName);

      Open MIDI input device 'devName', and store stream specific
      data pointer in *userData. Return value is zero on success,
      and a non-zero error code if an error occured.

    int csoundExternalMidiRead(void *csound, void *userData,
                               unsigned char *buf, int nbytes);

      Read at most 'nbytes' bytes of MIDI data from input stream
      'userData', and store in 'buf'. Returns the actual number of
      bytes read, which may be zero if there were no events, and
      negative in case of an error. Note: incomplete messages (such
      as a note on status without the data bytes) should not be
      returned.

    int csoundExternalMidiInClose(void *csound, void *userData);

      Close MIDI input device associated with 'userData'.
      Return value is zero on success, and a non-zero error
      code on failure.

    int csoundExternalMidiOutOpen(void *csound, void **userData,
                                  const char *devName);

      Open MIDI output device 'devName', and store stream specific
      data pointer in *userData. Return value is zero on success,
      and a non-zero error code if an error occured.

    int csoundExternalMidiWrite(void *csound, void *userData,
                                unsigned char *buf, int nbytes);

      Write 'nbytes' bytes of MIDI data to output stream 'userData'
      from 'buf' (the buffer will not contain incomplete messages).
      Returns the actual number of bytes written, or a negative
      error code.

    int csoundExternalMidiOutClose(void *csound, void *userData);

      Close MIDI output device associated with '*userData'.
      Return value is zero on success, and a non-zero error
      code on failure.

    char *csoundExternalMidiErrorString(void *csound, int errcode);

      Returns pointer to a string constant storing an error massage
      for error code 'errcode'.

    Setting function pointers:
    --------------------------

    void csoundSetExternalMidiInOpenCallback(void *csound,
                                             int (*func)(void*, void**,
                                                         const char*));

    void csoundSetExternalMidiReadCallback(void *csound,
                                           int (*func)(void*, void*,
                                                       unsigned char*, int));

    void csoundSetExternalMidiInCloseCallback(void *csound,
                                              int (*func)(void*, void*));

    void csoundSetExternalMidiOutOpenCallback(void *csound,
                                              int (*func)(void*, void**,
                                                          const char*));

    void csoundSetExternalMidiWriteCallback(void *csound,
                                            int (*func)(void*, void*,
                                                        unsigned char*, int));

    void csoundSetExternalMidiOutCloseCallback(void *csound,
                                               int (*func)(void*, void*));

    void csoundSetExternalMidiErrorStringCallback(void *csound,
                                                  char *(*func)(int));
*/

#include "cs.h"
#include "midiops.h"
#include "oload.h"
#include "midifile.h"

#define MGLOB(x) (((ENVIRON*) csound)->midiGlobals->x)

extern  void    schedofftim(INSDS *), deact(INSDS *), beep(void);
        void    midNotesOff(void);

static MYFLT dsctl_map[12] = {
    FL(1.0), FL(0.0), FL(1.0), FL(0.0), FL(1.0), FL(0.0),
    FL(1.0), FL(0.0), FL(1.0), FL(0.0), FL(1.0), FL(0.0)
};

static const short datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };
/* static short m_clktim = 0; */

static void AllNotesOff(MCHNBLK *);

void MidiOpen(ENVIRON *csound)
                      /* open a Midi event stream for reading, alloc bufs */
{                     /*     callable once from main.c                    */
    /* First set up buffers. */
    int i;
    MGLOB(Midevtblk) = (MEVENT *) mcalloc(csound, (long) sizeof(MEVENT));
    MGLOB(sexp) = 0;
    /* Then open device... */
    if (O.Midiin) {
      i = csoundExternalMidiInOpen(csound, &MGLOB(midiInUserData), O.Midiname);
      if (i != 0) {
        csoundMessage(csound,
                      Str(" *** error opening MIDI in device: %d (%s)\n"),
                      i, csoundExternalMidiErrorString(csound, i));
        longjmp(csound->exitjmp_, 1);
      }
    }
    /* and file. */
    if (O.FMidiin && O.FMidiname != NULL) {
      i = csoundMIDIFileOpen(csound, O.FMidiname);
      if (i != 0) {
        csoundMessage(csound, Str("Failed to load MIDI file.\n"));
        longjmp(csound->exitjmp_, 1);
      }
    }
}

static void sustsoff(MCHNBLK *chn)  /* turnoff all notes in chnl sust array */
{                        /* called by SUSTAIN_SW_off only if count non-zero */
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
          xturnoff(&cenviron, ip);
        ip = ip->nxtolap;
      }
    }
    if (chn->ksuscnt)
      printf(Str("sustain count still %d\n"), chn->ksuscnt);
    chn->ksuscnt = 0;
}

/* reset all controllers for this channel */

static void ctlreset(ENVIRON *csound, short chan)
{
    MCHNBLK *chn;
    int     i;

    chn = M_CHNBP[chan];
    for (i = 1; i <= 109; i++)                  /* from ctlr 1 to ctlr 109 */
      chn->ctl_val[i] = FL(0.0);                /*   reset all ctlrs to 0  */
    /* exceptions:  */
    chn->ctl_val[7]  = FL(127.0);               /*   volume           */
    chn->ctl_val[8]  = FL(64.0);                /*   balance          */
    chn->ctl_val[10] = FL(64.0);                /*   pan              */
    chn->ctl_val[11] = FL(127.0);               /*   expression       */
    chn->pbensens = FL(2.0);                    /*   pitch bend range */
    chn->datenabl = 0;
    /* reset aftertouch to max value - added by Istvan Varga, May 2002 */
    chn->aftouch = FL(127.0);
    for (i = 0; i < 128; i++)
      chn->polyaft[i] = FL(127.0);
    /* controller 64 has just been set to zero: terminate any held notes */
    if (chn->ksuscnt && !MGLOB(rawControllerMode))
      sustsoff(chn);
    chn->sustaining = 0;
    /* reset pitch bend */
    chn->pchbend = FL(0.0);
}

/* execute non-note channel voice and channel mode commands */

void m_chanmsg(ENVIRON *csound, MEVENT *mep)
{
    MCHNBLK *chn = M_CHNBP[mep->chan];
    short n;
    MYFLT *fp;

    switch (mep->type) {
    case PROGRAM_TYPE:                    /* PROGRAM CHANGE */
      chn->pgmno = mep->dat1;
      if (chn->insno <= 0)          /* ignore if channel is muted */
        break;
      n = (short) chn->pgm2ins[mep->dat1];      /* program change -> INSTR  */
      if (n > 0 && n <= maxinsno                /* if corresp instr exists  */
          && instrtxtp[n] != NULL) {            /*     assign as insno      */
        chn->insno = n;                         /* else ignore prog. change */
        printf(Str("midi channel %d now using instr %d\n"),
               mep->chan + 1, chn->insno);
      }
      break;
    case POLYAFT_TYPE:
      chn->polyaft[mep->dat1] = mep->dat2;     /* Polyphon per-Key Press  */
      break;
    case CONTROL_TYPE:                    /* CONTROL CHANGE MESSAGES: */
      n = mep->dat1;
      if (MGLOB(rawControllerMode)) {           /* "raw" mode:        */
        chn->ctl_val[n] = (MYFLT) mep->dat2;    /*   only store value */
        break;
      }
      if (n >= 111)                       /* if special, redirect */
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
          default:printf(Str("unknown NPRN lsb %d\n"), lsb);
            goto err;
          }
          fval = (MYFLT) (mep->dat2 - 64);
          chn->ctl_val[ctl] = fval;           /* then store     */
        }
        else {
          if (msb < 24 || msb == 25 || msb == 27 ||
              msb > 31 || lsb < 25  || lsb > 87)
            printf(Str("unknown drum param nos, msb %ld lsb %ld\n"),
                   (long)msb, (long)lsb);
          else {
            static int drtab[8] = {0,0,1,1,2,3,4,5};
            int parnum = drtab[msb - 24];
            if (parnum == 0)
              fval = (MYFLT) (mep->dat2 - 64);
            else fval = mep->dat2;
            if (dsctl_map != NULL) {
              fp = &dsctl_map[parnum*2];
              if (*fp != FL(0.0)) {
                MYFLT xx = (fval * *fp++);
                fval = xx + *fp;    /* optionally map */
              }
            }
            printf(Str("CHAN %ld DRUMKEY %ld not in keylst,"
                   " PARAM %ld NOT UPDATED\n"),
                   (long)mep->chan+1, (long)lsb, (long)msb);
          }
        }
      }
      else
        chn->ctl_val[n] = (MYFLT) mep->dat2;      /* record data as MYFLT */
    err:
      if (n == SUSTAIN_SW) {                      /* if sustainP changed  */
        if (mep->dat2 > 0)
          chn->sustaining = 1;
        else if (chn->sustaining) {               /*  & going off         */
          chn->sustaining = 0;
          sustsoff(chn);                          /*      reles any notes */
        }
      }
      break;

    special:
      if (n < 121) {          /* for ctrlr 111, 112, ... chk inexclus lists */
        if ((csound->oparms_->msglevel & 7) == 7)
          printf(Str("ctrl %d has no exclus list\n"), (int) n);
        break;
      }
      /* 121 == RESET ALL CONTROLLERS */
      if (n == 121) {                           /* CHANNEL MODE MESSAGES:  */
        ctlreset(csound, mep->chan);
      }
      else if (n == 122) {                      /* absorb lcl ctrl data */
/*      int lcl_ctrl = mep->dat2;  ?? */        /* 0:off, 127:on */
      }
      else if (n == 123) midNotesOff();         /* allchnl AllNotesOff */
      else if (n == 126) {                      /* MONO mode */
        if (chn->monobas == NULL) {
          MONPCH *mnew, *mend;
          chn->monobas = (MONPCH *)mcalloc(csound, (long)sizeof(MONPCH) * 8);
          mnew = chn->monobas;  mend = mnew + 8;
          do {
            mnew->pch = -1;
          } while (++mnew < mend);
        }
        chn->mono = 1;
      }
      else if (n == 127) {                      /* POLY mode */
        if (chn->monobas != NULL) {
          mfree(csound, (char *)chn->monobas);
          chn->monobas = NULL;
        }
        chn->mono = 0;
      }
      else printf(Str("chnl mode msg %d not implemented\n"), n);
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
        sprintf(errmsg,Str("unrecognised sys_common type %d"), mep->chan);
        die(errmsg);
      }
      break;
    default:
      sprintf(errmsg,Str("unrecognised message type %d"), mep->type);
      die(errmsg);
    }
}

/* initialise all MIDI channels; called by musmon before oload() */

void m_chn_init_all(ENVIRON *csound)
{                               /* alloc a midi control blk for a midi chnl */
    MCHNBLK *chn;               /*  & assign corr instr n+1, else a default */
    int     defaultinsno, n;
    short   chan;

    defaultinsno = 0;
    while (++defaultinsno <= (int) maxinsno && instrtxtp[defaultinsno] == NULL);
    if (defaultinsno > (int) maxinsno)
      defaultinsno = 0;         /* no instruments */
    for (chan = (short) 0; chan < (short) 16; chan++) {
      /* alloc a midi control blk for midi channel */
      /*  & assign default instrument number       */
      M_CHNBP[chan] = chn = (MCHNBLK*) mcalloc(csound, sizeof(MCHNBLK));
      n = (int) chan + 1;
      /* if corresponding instrument exists, assign as insno, */
      if (n <= (int) maxinsno && instrtxtp[n] != NULL)
        chn->insno = (short) n;
      else if (defaultinsno > 0)
        chn->insno = (short) defaultinsno;
      else
        chn->insno = (short) -1;        /* else mute channel */
      /* reset all controllers */
      chn->pgmno = -1;
      ctlreset(csound, chan);
      for (n = 0; n < 128; n++)
        chn->pgm2ins[n] = (short) (n + 1);
      if (csound->oparms_->Midiin || csound->oparms_->FMidiin) {
        if (chn->insno > 0)
          printf(Str("midi channel %d using instr %d\n"), chan + 1, chn->insno);
        else
          printf(Str("midi channel %d is muted\n"), chan + 1);
      }
    }
}

int m_chinsno(ENVIRON *csound, short chan, short insno)
{                                         /* assign an insno to a chnl */
    MCHNBLK  *chn;                        /* =massign: called from i0  */
    MEVENT   mev;

    if (chan < 0 || chan > 15)
      return initerror(Str("illegal channel number"));
    chn = M_CHNBP[chan];
    if (insno <= 0) {
      chn->insno = -1;
      printf(Str("MIDI channel %d muted\n"), (int) chan + 1);
    }
    else {
      if (insno > maxinsno || instrtxtp[insno] == NULL) {
        printf(Str("Insno = %d\n"), insno);
        return initerror(Str("unknown instr"));
      }
      chn->insno = insno;
      printf(Str("chnl %d using instr %d\n"), chan+1, chn->insno);
      /* check for program change: will override massign if enabled */
      if (chn->pgmno >= 0) {
        mev.type = PROGRAM_TYPE;
        mev.chan = chan;
        mev.dat1 = chn->pgmno;
        mev.dat2 = 0;
        m_chanmsg(csound, &mev);
      }
    }
    ctlreset(csound, chan);
    return OK;
}

static void AllNotesOff(MCHNBLK *chn)
{
    INSDS   *ip;
    int     nn;

    for (nn = 0; nn < 128; nn++) {
      ip = chn->kinsptr[nn];
      while (ip != NULL) {
        xturnoff_now(&cenviron, ip);
        ip = ip->nxtolap;
      }
    }
}

void midNotesOff(void)          /* turnoff ALL curr midi notes, ALL chnls */
{                               /* called by musmon, ctrl 123 & sensMidi  */
    int chan = 0;
    do {
      AllNotesOff(M_CHNBP[chan]);
    } while (++chan < MAXCHAN);
}

int sensMidi(ENVIRON *csound)
{                          /* sense a MIDI event, collect the data & dispatch */
    short   c, type;       /*  called from kperf(), return(2) if MIDI on/off  */
    MEVENT  *mep = MGLOB(Midevtblk);
    int     n;

 nxtchr:
    if (MGLOB(bufp) >= MGLOB(endatp)) {
      MGLOB(bufp) = &(MGLOB(mbuf)[0]);
      MGLOB(endatp) = MGLOB(bufp);
      if (O.Midiin) {                   /* read MIDI device */
        n = csoundExternalMidiRead(csound, MGLOB(midiInUserData),
                                   MGLOB(bufp), MBUFSIZ);
        if (n < 0)
          csoundMessage(csound,
                        Str(" *** error reading MIDI device: %d (%s)\n"),
                        n, csoundExternalMidiErrorString(csound, n));
        else
          MGLOB(endatp) += (int) n;
      }
      if (O.FMidiin) {                  /* read MIDI file */
        n = csoundMIDIFileRead(csound, MGLOB(endatp),
                               MBUFSIZ - (int) (MGLOB(endatp) - MGLOB(bufp)));
        if (n > 0)
          MGLOB(endatp) += (int) n;
      }
      if (MGLOB(endatp) <= MGLOB(bufp))
        return 0;               /* no events were received */
    }

    if ((c = *(MGLOB(bufp)++)) & 0x80) { /* STATUS byte:      */
      type = c & 0xF0;
      if (type == SYSTEM_TYPE) {
        short lo3 = (c & 0x07);
        if (c & 0x08)                    /* sys_realtime:     */
          switch (lo3) {                 /*   dispatch now    */
          case 0: /* m_clktim++; */      /* timing clock      */
          case 2:                        /* start             */
          case 3:                        /* continue          */
          case 4:                        /* stop              */
          case 6:                        /* active sensing    */
          case 7:                        /* system reset      */
            goto nxtchr;
          default: printf(Str("undefined sys-realtime msg %x\n"),c);
            goto nxtchr;
          }
        else {                           /* sys_non-realtime status:   */
          MGLOB(sexp) = 0;               /* implies sys_exclus end     */
          switch (lo3) {                 /* dispatch on lo3:  */
          case 7: goto nxtchr;           /* EOX: already done */
          case 0: MGLOB(sexp) = 1;       /* sys_ex begin:     */
            goto nxtchr;                 /*   goto copy data  */
          /* sys_common: need some data, so build evt */
          case 1:                        /* MTC quarter frame */
          case 3: MGLOB(datreq) = 1;     /* song select       */
            break;
          case 2: MGLOB(datreq) = 2;     /* song position     */
            break;
          case 6:                        /* tune request      */
            goto nxtchr;
          default: printf(Str("undefined sys_common msg %x\n"), c);
            MGLOB(datreq) = 32767;       /* waste any data following */
            MGLOB(datcnt) = 0;
            goto nxtchr;
          }
        }
        mep->type = type;               /* begin sys_com event  */
        mep->chan = lo3;                /* holding code in chan */
        MGLOB(datcnt) = 0;
        goto nxtchr;
      }
      else {                            /* other status types:  */
        short chan;
        MGLOB(sexp) = 0;                /* also implies sys_exclus end */
        chan = c & 0xF;
        mep->type = type;               /* & begin new event */
        mep->chan = chan;
        MGLOB(datreq) = datbyts[(type>>4) & 0x7];
        MGLOB(datcnt) = 0;
        goto nxtchr;
      }
    }
    if (MGLOB(sexp) != 0) {             /* NON-STATUS byte:      */
      goto nxtchr;
    }
    if (MGLOB(datcnt) == 0)
      mep->dat1 = c;                    /* else normal data      */
    else mep->dat2 = c;
    if (++MGLOB(datcnt) < MGLOB(datreq))  /* if msg incomplete     */
      goto nxtchr;                        /*   get next char       */
    /* Enter the input event into a buffer used by 'midiin'. */
    if (mep->type != SYSTEM_TYPE) {
      unsigned char *pMessage =
                    &(MGLOB(MIDIINbuffer2)[MGLOB(MIDIINbufIndex)++].bData[0]);
      MGLOB(MIDIINbufIndex) &= MIDIINBUFMSK;
      *pMessage++ = mep->type | mep->chan;
      *pMessage++ = (unsigned char)mep->dat1;
      *pMessage = (MGLOB(datreq) < 2 ? (unsigned char) 0 : mep->dat2);
    }
    MGLOB(datcnt) = 0;                  /* else allow a repeat   */
    /* NB:  this allows repeat in syscom 1,2,3 too */
    if (mep->type > NOTEON_TYPE) {      /* if control or syscom  */
      m_chanmsg(csound, mep);           /*   handle from here    */
      goto nxtchr;                      /*   & go look for more  */
    }
    return(2);                          /* else it's note_on/off */
}

void MidiClose(ENVIRON *csound)
{
    int retval;

    retval = csoundExternalMidiInClose(csound, MGLOB(midiInUserData));
    if (retval != 0)
      csoundMessage(csound, Str("Error closing MIDI in device: %d (%s)\n"),
                    retval, csoundExternalMidiErrorString(csound, retval));
    if (MGLOB(MIDIoutDONE)) {
      retval = csoundExternalMidiOutClose(csound, MGLOB(midiOutUserData));
      if (retval != 0)
        csoundMessage(csound, Str("Error closing MIDI out device: %d (%s)\n"),
                      retval, csoundExternalMidiErrorString(csound, retval));
      MGLOB(MIDIoutDONE) = 0;
    }
    if (MGLOB(midiFileData) != NULL) {
      csoundMIDIFileClose(csound);
      MGLOB(midiFileData) = NULL;
    }
}

