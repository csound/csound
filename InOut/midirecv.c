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

#include "cs.h"
#include "midiops.h"
#include "oload.h"

static  int     sexp = 0;
static  int     fsexp = 0;

static  FILE    *mfp = NULL;   /* was stdin */
        MGLOBAL mglob;
static  MYFLT   MastVol = FL(1.0);      /* maps ctlr 7 to ctlr 9 */
static  long    MTrkrem;
static  double  FltMidiNxtk, kprdspertick, ekrdQmil;
        void    m_chn_init(ENVIRON *csound, MEVENT *, short);
static  void    (*nxtdeltim)(void), Fnxtdeltim(void), Rnxtdeltim(void);
extern  void    schedofftim(INSDS *), deact(INSDS *), beep(void);
        void    midNotesOff(void);

static  int     defaultinsno = 0;
extern  int     pgm2ins[];   /* IV May 2002 */
extern  void    insxtroff(short);

extern  void    csoundExternalMidiDeviceOpen(void *csound);
extern  void    csoundExternalMidiDeviceClose(void *csound);
extern  void    OpenMIDIDevice(ENVIRON *csound);
extern  int     GetMIDIData(ENVIRON *csound, unsigned char *mbuf, int nbytes);
extern  void    CloseMIDIDevice(ENVIRON *csound);

#ifdef WORDS_BIGENDIAN
# define natshort(x) (x)
# define natlong(x)  (x)
#else
  extern  long  natlong(long);
  extern  short natshort(short);
#endif

static MYFLT dsctl_map[12] = {
    FL(1.0), FL(0.0), FL(1.0), FL(0.0), FL(1.0), FL(0.0),
    FL(1.0), FL(0.0), FL(1.0), FL(0.0), FL(1.0), FL(0.0)
};

static const short datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };
static short m_clktim = 0;

static void AllNotesOff(MCHNBLK *);

void MidiOpen(ENVIRON *csound)
                      /* open a Midi event stream for reading, alloc bufs */
{                     /*     callable once from main.c                    */
    /* First set up buffers. */
    int i;
    Midevtblk = (MEVENT *) mcalloc(csound, (long)sizeof(MEVENT));
    sexp = 0;
    for (i = 0; i < MAXCHAN; i++) M_CHNBP[i] = NULL; /* Clear array */
    m_chn_init(csound, Midevtblk,(short)0);
    /* Then open device. */
    if (csoundIsExternalMidiEnabled(csound)) {
      csoundExternalMidiDeviceOpen(csound);
    }
    else {
      OpenMIDIDevice(csound);
    }
}

static void sustsoff(MCHNBLK *chn)  /* turnoff all notes in chnl sust array */
{                        /* called by SUSTAIN_SW_off only if count non-zero */
    INSDS *ip, **ipp1, **ipp2;
    short nn, suscnt;

    suscnt = chn->ksuscnt;
    ipp1 = ipp2 = chn->ksusptr + 64;    /* find midpoint of sustain array */
    ipp1--;
    for (nn = 64; nn--; ipp1--, ipp2++ ) {
      if ((ip = *ipp1) != NULL) {
        *ipp1 = NULL;
        do {
          if (ip->xtratim) {
            ip->relesing = 1;
            ip->offtim = (kcounter + ip->xtratim) * onedkr;
            schedofftim(ip);
          }
          else deact(ip);
        } while ((ip = ip->nxtolap) != NULL);
        if (--suscnt == 0)  break;
      }
      if ((ip = *ipp2) != NULL) {
        *ipp2 = NULL;
        do {
          if (ip->xtratim) {
            ip->relesing = 1;
            ip->offtim = (kcounter + ip->xtratim) * onedkr;
            schedofftim(ip);
          }
          else deact(ip);
        } while ((ip = ip->nxtolap) != NULL);
        if (--suscnt == 0)  break;
      }
    }
    if (suscnt) printf(Str("sustain count still %d\n"), suscnt);
    chn->ksuscnt = 0;
}

void m_chanmsg(ENVIRON *csound, MEVENT *mep)
{                           /* exec non-note chnl_voice & chnl_mode cmnds */
    MCHNBLK *chn = M_CHNBP[mep->chan];
    short n;
    MYFLT *fp;

    switch(mep->type) {
    case PROGRAM_TYPE:
      n = (short) pgm2ins[mep->dat1];       /* program change -> INSTR  */
      if (n > 0 && n <= maxinsno            /* if corresp instr exists  */
          && instrtxtp[n] != NULL) {        /*     assign as pgmno      */
        chn->pgmno = n;                     /* else ignore prog. change */
        printf(Str("midi channel %d now using instr %d\n"),
               mep->chan+1,chn->pgmno);
      }
      break;
    case POLYAFT_TYPE:
      chn->polyaft[mep->dat1] = mep->dat2;     /* Polyphon per-Key Press  */
      break;
    case CONTROL_TYPE:                    /* CONTROL CHANGE MESSAGES: */
      if ((n = mep->dat1) >= 111)         /* if special, redirect */
        goto special;
      if (n == RPNLSB && mep->dat2 == 127 && chn->dpmsb == 127)
        chn->ctl_val[DATENABL] = FL(0.0);
      else if (n == NRPNMSB || n == RPNMSB)
        chn->dpmsb = mep->dat2;
      else if (n == NRPNLSB || n == RPNLSB) {
        chn->dplsb = mep->dat2;
        chn->ctl_val[DATENABL] = FL(1.0);
      }
      else if (n == DATENTRY && chn->ctl_val[DATENABL] != FL(0.0)) {
        int   msb = chn->dpmsb;
        int   lsb = chn->dplsb;
        MYFLT fval;
        if (msb == 0 && lsb == 0) {
          chn->ctl_val[BENDSENS] = mep->dat2;
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
      else chn->ctl_val[n] = (MYFLT) mep->dat2;   /* record data as MYFLT */
    err:
      if (n == VOLUME)
        chn->ctl_val[MOD_VOLUME] = chn->ctl_val[VOLUME] * MastVol;
      else if (n == SUSTAIN_SW) {
        short temp = (mep->dat2 > 0);
        if (chn->sustaining != temp) {            /* if sustainP changed  */
          if (chn->sustaining && chn->ksuscnt)    /*  & going off         */
            sustsoff(chn);                        /*      reles any notes */
          chn->sustaining = temp;
        }
      }
      break;

    special:
      if (n < 121) {          /* for ctrlr 111, 112, ... chk inexclus lists */
        printf(Str("ctrl %ld has no exclus list\n"), (long)n);
        break;
      }
      /* 121 == RESET ALL CONTROLLERS */
      if (n == 121) {                           /* CHANNEL MODE MESSAGES:  */
        MYFLT *fp = chn->ctl_val + 1;           /* from ctlr 1 */
        short nn = 101;                         /* to ctlr 101 */
        do {
          *fp++ = FL(0.0);                      /*   reset all ctlrs to 0 */
        } while (--nn);                         /* exceptions:  */
        chn->ctl_val[7]  = FL(127.0);           /*   volume     */
        chn->ctl_val[8]  = FL(64.0);            /*   balance    */
        chn->ctl_val[10] = FL(64.0);            /*   pan        */
        chn->ctl_val[11] = FL(127.0);           /*   expression */
        chn->ctl_val[BENDSENS] = FL(2.0);
        chn->ctl_val[9]  = chn->ctl_val[7] * MastVol;
        /* reset aftertouch to max value - added by Istvan Varga, May 2002 */
        chn->aftouch = FL(127.0);
        for (nn = 0; nn < 128; nn++) chn->polyaft[nn] = FL(127.0);
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

void m_chn_init(ENVIRON *csound, MEVENT *mep, short chan)
{                               /* alloc a midi control blk for a midi chnl */
    MCHNBLK *chn;               /*  & assign corr instr n+1, else a default */

    if (!defaultinsno) {        /* find lowest instr as default */
      defaultinsno = 1;
      while (instrtxtp[defaultinsno]==NULL) {
        defaultinsno++;
        if (defaultinsno > maxinsno)
          die(Str("midi init cannot find any instrs"));
      }
    }
    if ((chn = M_CHNBP[chan]) == NULL)
      M_CHNBP[chan] = chn = (MCHNBLK *) mcalloc(csound, (long)sizeof(MCHNBLK));
    if (instrtxtp[chan+1] != NULL)           /* if corresp instr exists  */
      chn->pgmno = chan+1;                   /*     assign as pgmno      */
    else chn->pgmno = defaultinsno;          /* else assign the default  */
    mep->type = CONTROL_TYPE;
    mep->chan = chan;
    mep->dat1 = 121;  /* reset all controllers */
    m_chanmsg(csound, mep);
    printf(Str("midi channel %d using instr %d\n"), chan + 1, chn->pgmno);
}

static void ctlreset(ENVIRON *csound, short chan)
{                               /* reset all controllers for this channel */
    MEVENT  mev;
    mev.type = CONTROL_TYPE;
    mev.chan = chan;
    mev.dat1 = 121;
    m_chanmsg(csound, &mev);
}

MCHNBLK *m_getchnl(ENVIRON *csound, short chan)
{                               /* get or create a chnlblk ptr */
    MCHNBLK *chn;
    if (chan < 0 || chan >= MAXCHAN) {
      sprintf(errmsg,Str("illegal midi chnl no %d"), chan+1);
      die(errmsg);
    }
    if ((chn = M_CHNBP[chan]) == NULL) {
      M_CHNBP[chan] = chn = (MCHNBLK *) mcalloc(csound, (long)sizeof(MCHNBLK));
      chn->pgmno = -1;
      chn->insno = -1;
      ctlreset(csound, chan);
    }
    return(chn);
}

void m_chinsno(ENVIRON *csound, short chan, short insno)
{                                         /* assign an insno to a chnl */
    MCHNBLK  *chn = NULL;                 /* =massign: called from i0  */

    if (insno <= 0 || insno >= maxinsno || instrtxtp[insno] == NULL) {
      printf(Str("Insno = %d\n"), insno);
      die(Str("unknown instr"));
    }
    if (M_CHNBP[chan] != NULL)
      printf(Str("massign: chnl %d exists, ctrls now defaults\n"), chan+1);
    chn = m_getchnl(csound, chan);
    chn->insno = insno;
    chn->pchbend = FL(0.0);     /* Mid value */
    /*    chn->posbend = FL(0.5); */          /* for pos pchbend (0 - 1.0) */
    ctlreset(csound, chan);
    printf(Str("chnl %d using instr %d\n"), chan+1, chn->insno);
}

static void AllNotesOff(MCHNBLK *chn)
{
    INSDS *ip, **ipp = chn->kinsptr;
    int nn = 128;

    do {
      if ((ip = *ipp) != NULL) {        /* if find a note in kinsptr slot */
        deact(ip);                      /*    deactivate, clear the slot  */
        *ipp = NULL;
      }
      ipp++;
    } while (--nn);
    if (chn->sustaining)                /* same for notes in sustain list */
      sustsoff(chn);
    insxtroff(chn->insno);              /* finally rm all xtratim hanging */
}

void midNotesOff(void)          /* turnoff ALL curr midi notes, ALL chnls */
{                               /* called by musmon, ctrl 123 & sensFMidi */
    int chan = 0;
    MCHNBLK *chn;
    do {
      if ((chn = M_CHNBP[chan]) != NULL)
        AllNotesOff(chn);
    } while (++chan < MAXCHAN);
}

void setmastvol(short mvdat)    /* set MastVol & adjust all chan modvols */
{
    MCHNBLK *chn;
    int chnl;
    MastVol = (MYFLT)mvdat * (FL(1.0)/FL(128.0));
    for (chnl = 0; chnl < MAXCHAN; chnl++)
      if ((chn = M_CHNBP[chnl]) != NULL)
        chn->ctl_val[MOD_VOLUME] = chn->ctl_val[VOLUME] * MastVol;
}

int sensMidi(ENVIRON *csound)
{                          /* sense a MIDI event, collect the data & dispatch */
    short  c, type;        /*  called from kperf(), return(2) if MIDI on/off  */
    MEVENT          *mep = Midevtblk;
    static    short datreq, datcnt;
    unsigned  char  mbuf[MBUFSIZ], *bufp, *endatp;
    int             n;

    bufp = &(mbuf[0]);
    endatp = bufp;

 nxtchr:
    if (bufp >= endatp) {
      bufp = &(mbuf[0]);
      n = GetMIDIData(csound, bufp, MBUFSIZ);
      if (n < 1)
        return 0;
      endatp = (char*) bufp + (int) n;
    }

    if ((c = *bufp++) & 0x80) {              /* STATUS byte:      */
      type = c & 0xF0;
      if (type == SYSTEM_TYPE) {
        short lo3 = (c & 0x07);
        if (c & 0x08)                    /* sys_realtime:     */
          switch (lo3) {                 /*   dispatch now    */
          case 0: m_clktim++;
          case 2:
          case 3:
          case 4:
          case 6:
          case 7:
            goto nxtchr;
          default: printf(Str("undefined sys-realtime msg %x\n"),c);
            goto nxtchr;
          }
        else {                           /* sys_non-realtime status:   */
          sexp = 0;                      /* implies sys_exclus end     */
          switch (lo3) {                 /* dispatch on lo3:  */
          case 7: goto nxtchr;           /* EOX: already done */
          case 0: sexp = 1;              /* sys_ex begin:     */
            goto nxtchr;                 /*   goto copy data  */
          case 1:                        /* sys_common:       */
          case 3: datreq = 1;            /*   need some data  */
            break;
          case 2: datreq = 2;            /*   (so build evt)  */
            break;
          case 6:
            goto nxtchr;
          default: printf(Str("undefined sys_common msg %x\n"), c);
            datreq = 32767; /* waste any data following */
            datcnt = 0;
            goto nxtchr;
          }
        }
        mep->type = type;               /* begin sys_com event  */
        mep->chan = lo3;                /* holding code in chan */
        datcnt = 0;
        goto nxtchr;
      }
      else {                            /* other status types:  */
        short chan;
        sexp = 0;                       /* also implies sys_exclus end */
        chan = c & 0xF;
        if (M_CHNBP[chan] == NULL)      /* chk chnl exists   */
          m_chn_init(csound, mep, chan);
        mep->type = type;               /* & begin new event */
        mep->chan = chan;
        datreq = datbyts[(type>>4) & 0x7];
        datcnt = 0;
        goto nxtchr;
      }
    }
    if (sexp != 0) {                    /* NON-STATUS byte:      */
      goto nxtchr;
    }
    if (datcnt == 0)
      mep->dat1 = c;                    /* else normal data      */
    else mep->dat2 = c;
    if (++datcnt < datreq)              /* if msg incomplete     */
      goto nxtchr;                      /*   get next char       */
    /* Enter the input event into a buffer used by 'midiin'. */
    if (mep->type != SYSTEM_TYPE) {
      unsigned char *pMessage = &(MIDIINbuffer2[MIDIINbufIndex++].bData[0]);
      MIDIINbufIndex &= MIDIINBUFMSK;
      *pMessage++ = mep->type | mep->chan;
      *pMessage++ = (unsigned char)mep->dat1;
      *pMessage = (datreq < 2 ? (unsigned char) 0 : mep->dat2);
    }
    datcnt = 0;                         /* else allow a repeat   */
    /* NB:  this allows repeat in syscom 1,2,3 too */
    if (mep->type > NOTEON_TYPE) {      /* if control or syscom  */
      m_chanmsg(csound, mep);                   /*   handle from here    */
      goto nxtchr;                      /*   & go look for more  */
    }
    return(2);                          /* else it's note_on/off */
}

static long vlendatum(void)  /* rd variable len datum from input stream */
{
    long datum = 0;
    unsigned char c;
    while ((c = getc(mfp)) & 0x80) {
      datum += c & 0x7F;
      datum <<= 7;
      MTrkrem--;
    }
    datum += c;
    MTrkrem--;
    return(datum);
}

static void Fnxtdeltim(void) /* incr FMidiNxtk by next delta-time */
{                            /* standard, with var-length deltime */
    unsigned long deltim = 0;
    unsigned char c;
    short count = 1;

    if (MTrkrem > 0) {
      while ((c = getc(mfp)) & 0x80) {
        deltim += c & 0x7F;
        deltim <<= 7;
        count++;
      }
      MTrkrem -= count;
      if ((deltim += c) > 0) {                  /* if deltim nonzero */
        FltMidiNxtk += deltim * kprdspertick; /*   accum in double */
        FMidiNxtk = (long) FltMidiNxtk;       /*   the kprd equiv  */
        /*              printf("FMidiNxtk = %ld\n", FMidiNxtk);  */
      }
    }
    else {
      printf(Str("end of track in midifile '%s'\n"), O.FMidiname);
      printf(Str("%d forced decays, %d extra noteoffs\n"),
             Mforcdecs, Mxtroffs);
      MTrkend = 1;
      O.FMidiin = 0;
      if (O.ringbell && !O.termifend)  beep();
    }
}

static void Rnxtdeltim(void)        /* incr FMidiNxtk by next delta-time */
{             /* Roland MPU401 form: F8 time fillers, no Trkrem val, EOF */
    unsigned long deltim = 0;
    int c;

    do {
      if ((c = getc(mfp)) == EOF) {
        printf(Str("end of MPU401 midifile '%s'\n"), O.FMidiname);
        printf(Str("%d forced decays, %d extra noteoffs\n"),
               Mforcdecs, Mxtroffs);
        MTrkend = 1;
        O.FMidiin = 0;
        if (O.ringbell && !O.termifend)  beep();
        return;
      }
      deltim += (c &= 0xFF);
    }
    while (c == 0xF8);      /* loop while sys_realtime tming clock */
    if (deltim) {                             /* if deltim nonzero */
      FltMidiNxtk += deltim * kprdspertick;   /*   accum in double */
      FMidiNxtk = (long) FltMidiNxtk;         /*   the kprd equiv  */
      /*          printf("FMidiNxtk = %ld\n", FMidiNxtk);  */
    }
}

void FMidiOpen(ENVIRON *csound) /* open a MidiFile for reading, sense MPU401 */
{                               /* or standard; callable once from main.c    */
    short sval;
    long lval, tickspersec;
    unsigned long deltim;
    char inbytes[16];    /* must be long-aligned, 16 >= MThd maxlen */

    FMidevtblk = (MEVENT *) mcalloc(csound, (long)sizeof(MEVENT));
    fsexp = 0;
    if (M_CHNBP[0] == (MCHNBLK*) NULL)      /* IV May 2002: added check */
      m_chn_init(csound, FMidevtblk,(short)0);

    if (strcmp(O.FMidiname,"stdin") == 0) {
      mfp = stdin;
    }
    else if (!(mfp = fopen(O.FMidiname, "rb")))
      dies(Str("cannot open '%s'"), O.FMidiname);
    if ((inbytes[0] = getc(mfp)) != 'M')
      goto mpu401;
    if ((inbytes[1] = getc(mfp)) != 'T') {
      ungetc(inbytes[1],mfp);
      goto mpu401;
    }
    if (fread(inbytes+2, 1, 6, mfp) < 6)
      dies(Str("unexpected end of '%s'"), O.FMidiname);
    if (strncmp(inbytes, "MThd", 4) != 0)
      dies(Str("we're confused.  file '%s' begins with 'MT',\n"
               "but not a legal header chunk"), O.FMidiname);
    printf(Str("%s: found standard midifile header\n"), O.FMidiname);
    if ((lval = natlong(*(long *)(inbytes+4))) < 6 || lval > 16) {
      sprintf(errmsg,Str("bad header length %ld in '%s'"),
              lval, O.FMidiname);
      die(errmsg);
    }
    if (fread(inbytes, 1, (int)lval, mfp) < (unsigned long)lval)
      dies(Str("unexpected end of '%s'"), O.FMidiname);
    sval = natshort(*(short *)inbytes);
    if (sval != 0 && sval != 1) { /* Allow Format 1 with single track */
      sprintf(errmsg,Str("%s: Midifile format %d not supported"),
              O.FMidiname, sval);
      die(errmsg);
    }
    if ((sval = natshort(*(short *)(inbytes+2))) != 1)
      dies(Str("illegal ntracks in '%s'"), O.FMidiname);
    if ((inbytes[4] & 0x80)) {
      short SMPTEformat, SMPTEticks;
      SMPTEformat = -(inbytes[4]);
      SMPTEticks = *(unsigned char *)inbytes+5;
      if (SMPTEformat == 29)  SMPTEformat = 30;  /* for drop frame */
      printf(Str("SMPTE timing, %d frames/sec, %d ticks/frame\n"),
             SMPTEformat, SMPTEticks);
      tickspersec = SMPTEformat * SMPTEticks;
    }
    else {
      short Qticks = natshort(*(short *)(inbytes+4));
      printf(Str("Metrical timing, Qtempo = 120.0, Qticks = %d\n"),
             Qticks);
      ekrdQmil = (double)ekr / Qticks / 1000000.0;
      tickspersec = Qticks * 2;
    }
    kprdspertick = (double)ekr / tickspersec;
    printf(Str("kperiods/tick = %7.3f\n"), kprdspertick);

 chknxt:
    if (fread(inbytes, 1, 8, mfp) < 8)         /* read a chunk ID & size */
      dies(Str("unexpected end of '%s'"), O.FMidiname);
    if ((lval = natlong(*(long *)(inbytes+4))) <= 0)
      dies(Str("improper chunksize in '%s'"), O.FMidiname);
    if (strncmp(inbytes, "MTrk", 4) != 0) {    /* if not an MTrk chunk,  */
      do {
        sval = getc(mfp);                      /*    skip over it        */
      } while (--lval);
      goto chknxt;
    }
    printf(Str("tracksize = %ld\n"), lval);
    MTrkrem = lval;                            /* else we have a track   */
    FltMidiNxtk = 0.0;
    FMidiNxtk = 0;                             /* init the time counters */
    nxtdeltim = Fnxtdeltim;                    /* set approp time-reader */
    nxtdeltim();                               /* incr by 1st delta-time */
    return;

 mpu401:
    printf(Str(
               "%s: assuming MPU401 midifile format, ticksize = 5 msecs\n"),
           O.FMidiname);
    kprdspertick = (double)ekr / 200.0;
    ekrdQmil = 1.0;                             /* temp ctrl (not needed) */
    MTrkrem = 0x7FFFFFFF;                       /* no tracksize limit     */
    FltMidiNxtk = 0.0;
    FMidiNxtk = 0;
    nxtdeltim = Rnxtdeltim;                    /* set approp time-reader */
    if ((deltim = (inbytes[0] & 0xFF))) {      /* if 1st time nonzero    */
      FltMidiNxtk += deltim * kprdspertick;  /*     accum in double    */
      FMidiNxtk = (long) FltMidiNxtk;        /*     the kprd equiv     */
/*          printf("FMidiNxtk = %ld\n", FMidiNxtk);   */
      if (deltim == 0xF8)     /* if char was sys_realtime timing clock */
        nxtdeltim();                      /* then also read nxt time */
    }
}

static void fsexdata(int n)   /* place midifile data into a sys_excl buffer */
{
    MTrkrem -= n;
    fsexp = 1;
    fseek(mfp, (long) (n - 1), SEEK_CUR);
    if (getc(mfp) == 0xF7)              /* if EOX at end          */
      fsexp = 0;                        /*    execute and clear   */
}

int sensFMidi(ENVIRON *csound)  /* read a MidiFile event, collect the data  */
{                               /* & dispatch; called from sensevents(),    */
    short  c, type;             /* return(SENSMFIL) if MIDI on/off          */
    MEVENT *mep = FMidevtblk;
    long len;
    static short datreq;

 nxtevt:
    if (--MTrkrem < 0 || (c = getc(mfp)) == EOF)
      goto Trkend;
    if (!(c & 0x80))      /* no status, assume running */
      goto datcpy;
    if ((type = c & 0xF0) == SYSTEM_TYPE) {     /* STATUS byte:      */
      short lo3;
      switch(c) {
      case 0xF0:                          /* SYS_EX event:  */
        if ((len = vlendatum()) <= 0)
          die(Str("zero length sys_ex event"));
        printf(Str("reading sys_ex event, length %ld\n"),len);
        fsexdata((int) len);
        goto nxtim;
      case 0xF7:                          /* ESCAPE event:  */
        if ((len = vlendatum()) <= 0)
          die(Str("zero length escape event"));
        printf(Str("escape event, length %ld\n"),len);
        if (sexp != 0)
          fsexdata((int) len);            /* if sysex contin, send  */
        else {
          MTrkrem -= len;
          do {
            c = getc(mfp);                /* else for now, waste it */
          } while (--len);
        }
        goto nxtim;
      case 0xFF:                          /* META event:     */
        if (--MTrkrem < 0 || (type = getc(mfp)) == EOF)
          goto Trkend;
        len = vlendatum();
        MTrkrem -= len;
        switch(type) {
          long usecs;
        case 0x51: usecs = 0;           /* set new Tempo       */
          do {
            usecs <<= 8;
            usecs += (c = getc(mfp)) & 0xFF;
          }
          while (--len);
          if (usecs <= 0)
            printf(Str("%ld usecs illegal in Tempo event\n"), usecs);
          else {
            kprdspertick = usecs * ekrdQmil;
            /*    printf("Qtempo = %5.1f\n", 60000000. / usecs); */
          }
          break;
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:                      /* print text events  */
        case 0x06:
        case 0x07:
          while (len--) {
            int ch;
            ch = getc(mfp);
            printf("%c", ch);
          }
          break;
        case 0x2F: goto Trkend;         /* normal end of track */
        default:
          printf(Str("skipping meta event type %x\n"),type);
          do {
            c = getc(mfp);
          } while (--len);
        }
        goto nxtim;
      }
      lo3 = (c & 0x07);
      if (c & 0x08) {               /* sys_realtime:     */
        switch (lo3) {              /*   dispatch now    */
        case 0:
        case 1:                     /* Timing Clk handled in Rnxtdeltim() */
        case 2:
        case 3:
        case 4:
        case 6:
        case 7: break;
        default: printf(Str("undefined sys-realtime msg %x\n"),c);
        }
        goto nxtim;
      }
      else if (lo3 == 6) {
        goto nxtim;
      }
      else {
        mep->type = type;         /* ident sys_com event  */
        mep->chan = lo3;          /* holding code in chan */
        switch (lo3) {            /* now need some data   */
        case 1:
        case 3: datreq = 1;
          break;
        case 2: datreq = 2;
          break;
        default: sprintf(errmsg,Str("undefined sys_common msg %x\n"), c);
          die(errmsg);
        }
      }
    }
    else {                              /* other status types:  */
      short chan = c & 0xF;
      if (M_CHNBP[chan] == NULL)      /*   chk chnl exists    */
        m_chn_init(csound, mep, chan);
      mep->type = type;               /*   & begin new event  */
      mep->chan = chan;
      datreq = datbyts[(type>>4) & 0x7];
    }
    c = getc(mfp);
    MTrkrem--;

 datcpy:
    mep->dat1 = c;                        /* sav the required data */
    if (datreq == 2) {
      mep->dat2 = getc(mfp);
      MTrkrem--;
    }
    /* Enter the input event into a buffer used by 'midiin'. */
    if (mep->type != SYSTEM_TYPE) {
      unsigned char *pMessage = &(MIDIINbuffer2[MIDIINbufIndex++].bData[0]);
      MIDIINbufIndex &= MIDIINBUFMSK;
      *pMessage++ = mep->type | mep->chan;
      *pMessage++ = (unsigned char)mep->dat1;
      *pMessage = (datreq < 2 ? (unsigned char) 0 : mep->dat2);
    }
    if (mep->type > NOTEON_TYPE) {        /* if control or syscom  */
      m_chanmsg(csound, mep);                   /*   handle from here    */
      goto nxtim;
    }
    nxtdeltim();
    return(3);                            /* else it's note_on/off */

 nxtim:
    nxtdeltim();
    if (O.FMidiin && kcounter >= FMidiNxtk)
      goto nxtevt;
    return(0);

 Trkend:
    printf(Str("end of midi track in '%s'\n"), O.FMidiname);
    printf(Str("%d forced decays, %d extra noteoffs\n"),
           Mforcdecs, Mxtroffs);
    MTrkend = 1;
    O.FMidiin = 0;
    if (O.ringbell && !O.termifend)  beep();
    return(0);
}

void MidiClose(ENVIRON *csound)
{
    if (csoundIsExternalMidiEnabled(csound)) {
      csoundExternalMidiDeviceClose(csound);
    }
    else {
      CloseMIDIDevice(csound);
    }
    if (mfp != NULL)
      fclose(mfp);
    mfp = NULL;
}

