/*
    pmidi.c:

    Copyright (C) 2004 John ffitch after Barry Vercoe

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

#include "cs.h"                                       /*    PMIDI.C    */
#include "midiops.h"
#include "oload.h"
#include <portmidi.h>
#include <porttime.h>
#include <errno.h>

#define MBUFSIZ   512
#define ON        1
#define OFF       0
#define MAXLONG   0x7FFFFFFFL

PmEvent *mbuf, *bufp, *bufend, *endatp;
static PmEvent *sexbuf, *sexp, *sexend;

MGLOBAL mglob;
/* MEVENT  *Midevtblk, *FMidevtblk; */
/* MCHNBLK *m_chnbp[MAXCHAN];    ptrs to chan ctrl blks */
static MYFLT MastVol = FL(1.0);     /* maps ctlr 7 to ctlr 9 */
void  m_chn_init(MEVENT *, short);
extern void  schedofftim(INSDS *), deact(INSDS *), beep(void);
void midNotesOff(void);
int csoundIsExternalMidiEnabled(void*);
void csoundExternalMidiDeviceOpen(void*);
void csoundExternalMidiDeviceClose(void*);
void OpenMIDIDevice(void);

/* static int   LCtl = ON; */
/* static int   NVoices = 1; */
static int   defaultinsno = 0;
/* extern INSDS *insalloc[]; */
/* extern short *insbusy; */
extern OPARMS O;
extern INX inxbas;
extern int pgm2ins[];   /* IV May 2002 */
extern void xturnon(int, long);
extern void xturnoff(INSDS*);
extern void insxtroff(short);
extern void OpenMIDIDevice(void);

PortMidiStream* midistream;
static int not_started = 1;

void OpenMIDIDevice(void)
{
    PmEvent buffer;
    if (not_started) {
      Pm_Initialize();
      Pt_Start(1, NULL, NULL);
    }
    not_started = 0;
    Pm_OpenInput(&midistream, 
                 atoi(O.Midiname),             /* Device number */
                 NULL, 
                 MBUFSIZ, 
                 ((long (*)(void *)) Pt_Time), 
                 NULL);
    Pm_SetFilter(midistream, PM_FILT_ACTIVE | PM_FILT_CLOCK);
    while (Pm_Poll(midistream)) { /* empty the buffer after setting filter */
      Pm_Read(midistream, &buffer, 1);
    }
}

long GetMIDIData(void)
{
    int n;
    extern int csoundIsExternalMidiEnabled(void*);
    extern long csoundExternalMidiRead(void*, u_char *, int);
    /**
     * Reads from user-defined MIDI input.
     */
    if (csoundIsExternalMidiEnabled(&cenviron)) {
      n = csoundExternalMidiRead(&cenviron, mbuf, MBUFSIZ);
      if (n == 0) {
        return 0;
      }
      bufp = mbuf;
      endatp = mbuf + n;
      return n;
    }
    else {
      int retval;
      if ((retval=Pm_Poll(midistream))) {
        if (retval<0) printf(Str(X_1185,"sensMIDI: retval errno %d"),errno);
        if (retval == 0) {
          long n = Pm_Read(midistream, bufp, MBUFSIZ);
          bufp = mbuf;
          endatp = mbuf + n;
          return n;
        }
        else {
          printf(Str(X_1185,"sensMIDI: retval errno %d"),errno);
        }
      }
      return 0;
    }
}

void MidiOpen(void)   /* open a Midi event stream for reading, alloc bufs */
{                     /*     callable once from main.c                    */
    /* First set up buffers. */
    int i;
    Midevtblk = (MEVENT *) mcalloc((long)sizeof(MEVENT));
    mbuf = (PmEvent *) mcalloc((long)MBUFSIZ*sizeof(PmEvent));
    bufend = mbuf + MBUFSIZ;
    bufp = endatp = mbuf;
    sexbuf = (PmEvent *) mcalloc((long)MBUFSIZ*sizeof(PmEvent));
    sexend = sexbuf + MBUFSIZ;
    sexp = NULL;
    for (i=0; i<MAXCHAN; i++) M_CHNBP[i] = NULL; /* Clear array */
    m_chn_init(Midevtblk,(short)0);
    /* Then open device. */
    if(csoundIsExternalMidiEnabled(&cenviron)) {
        csoundExternalMidiDeviceOpen(&cenviron);
    }
    else {
        OpenMIDIDevice();
    }
}


static void AllNotesOff(MCHNBLK *);

static void sustsoff(MCHNBLK *chn)  /* turnoff all notes in chnl sust array */
{                        /* called by SUSTAIN_SW_off only if count non-zero */
    INSDS *ip, **ipp1, **ipp2;
    short nn, suscnt;

    suscnt = chn->ksuscnt;
    ipp1 = ipp2 = chn->ksusptr + 64;          /* find midpoint of sustain array */
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
    if (suscnt) printf(Str(X_1251,"sustain count still %d\n"), suscnt);
    chn->ksuscnt = 0;
}

static void m_timcod_QF(int a, int b)
{ IGN(a); IGN(b);}  /* dummy sys_common targets */
static void m_song_pos(long a) { IGN(a);}
static void m_song_sel(long a) { IGN(a);}
static MYFLT dsctl_map[12] = {FL(1.0),FL(0.0),FL(1.0),FL(0.0),FL(1.0),FL(0.0),
                              FL(1.0),FL(0.0),FL(1.0),FL(0.0),FL(1.0),FL(0.0)};

void m_chanmsg(MEVENT *mep) /* exec non-note chnl_voice & chnl_mode cmnds */
{
    MCHNBLK *chn = M_CHNBP[mep->chan];
    short n;
    MYFLT *fp;

    switch(mep->type) {
    case PROGRAM_TYPE:
      n = (short) pgm2ins[mep->dat1];       /* program change -> INSTR  */
      if (n > 0 && n <= maxinsno            /* if corresp instr exists  */
          && instrtxtp[n] != NULL) {        /*     assign as pgmno      */
        chn->pgmno = n;                     /* else ignore prog. change */
        printf(Str(X_991,"midi channel %d now using instr %d\n"),
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
          default:printf(Str(X_1327,"unknown NPRN lsb %d\n"), lsb);
            goto err;
          }
          fval = (MYFLT) (mep->dat2 - 64);
          chn->ctl_val[ctl] = fval;           /* then store     */
        }
        else {
          if (msb < 24 || msb == 25 || msb == 27 ||
              msb > 31 || lsb < 25  || lsb > 87)
            printf(Str(X_1333,"unknown drum param nos, msb %ld lsb %ld\n"),
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
            printf(Str(X_195,"CHAN %ld DRUMKEY %ld not in keylst,"
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
#ifdef INEXCLUSIVE
        int index = mep->dat2;                    /*    for the index given */
        INX *inxp = &inxbas;    /* ***THIS CODE IS WRONG AS inxbas HAS NO VALUE
                                   ***Requires inexclus opcode which is ADI */
        while ((inxp = inxp->nxtinx) != NULL)
          if (inxp->ctrlno == n) {                /* if found ctrlno xclist */
            int *insp, cnt = inxp->inscnt;
            if (index <= cnt) {                   /*   & the index in-range */
              INSDS *ip;
              long xtratim = 0;                   /*     turnoff all instrs */
              for (insp = inxp->inslst; cnt--; insp++)
                if ((ip = instrtxtp[*insp]->instance) != NULL) {
                  do {
                    if (ip->actflg) {
                      if (ip->xtratim > xtratim)
                        xtratim = ip->xtratim;
                      xturnoff(ip);
                    }
                  } while ((ip = ip->nxtinstance) != NULL);
                }
              if (index) {
                int insno = inxp->inslst[index-1];
                xturnon(insno, xtratim);          /*     & schedstart this */
                printf(Str(X_934,"instr %ld now on\n"), (long)insno);
              }
            }
            else printf(Str(X_908,"index %ld exceeds ctrl %ld exclus list\n"),
                        (long)index, (long)n);
            return;
          }
#endif
        printf(Str(X_678,"ctrl %ld has no exclus list\n"), (long)n);
        break;
      }
/* modemsg: */
      if (n == 121) {                           /* CHANNEL MODE MESSAGES:  */
        MYFLT *fp = chn->ctl_val + 1;           /* from ctlr 1 */
        short nn = 101;                         /* to ctlr 101 */
        do {
          *fp++ = FL(0.0);                      /*   reset all ctlrs to 0 */
        } while (--nn);
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
          chn->monobas = (MONPCH *)mcalloc((long)sizeof(MONPCH) * 8);
          mnew = chn->monobas;  mend = mnew + 8;
          do {
            mnew->pch = -1;
          } while (++mnew < mend);
        }
        chn->mono = 1;
      }
      else if (n == 127) {                      /* POLY mode */
        if (chn->monobas != NULL) {
          mfree((char *)chn->monobas);
          chn->monobas = NULL;
        }
        chn->mono = 0;
      }
      else printf(Str(X_661,"chnl mode msg %d not implemented\n"), n);
      break;
    case AFTOUCH_TYPE:
      chn->aftouch = mep->dat1;                 /* chanl (all-key) Press */
      break;
    case PCHBEND_TYPE:
      chn->pchbend = (MYFLT)(((mep->dat2 - 64) << 7) + mep->dat1)/FL(8192.0);
/*        chn->posbend = (MYFLT)((mep->dat2 << 7) + mep->dat1) / FL(16384.0); */
      break;
    case SYSTEM_TYPE:              /* sys_common 1-3 only:  chan contains which */
      switch(mep->chan) {
      case 1:
        m_timcod_QF((int)((mep->dat1)>>4) & 0x7, (int)mep->dat1 & 0xF);
        break;
      case 2:
        m_song_pos((((long)mep->dat2)<<7) + mep->dat1);
        break;
      case 3:
        m_song_sel((long)mep->dat1);
        break;
      default:
        sprintf(errmsg,Str(X_1353,"unrecognised sys_common type %d"), mep->chan);
        die(errmsg);
      }
      break;
    default:
      sprintf(errmsg,Str(X_1351,"unrecognised message type %d"), mep->type);
      die(errmsg);
    }
}

void m_chn_init(MEVENT *mep, short chan)
    /* alloc a midi control blk for a midi chnl */
    /*  & assign corr instr n+1, else a default */
{
    MCHNBLK *chn;

    if (!defaultinsno) {        /* find lowest instr as default */
      defaultinsno = 1;
      while (instrtxtp[defaultinsno]==NULL) {
        defaultinsno++;
        if (defaultinsno>maxinsno)
          die(Str(X_993,"midi init cannot find any instrs"));
      }
    }
    if ((chn = M_CHNBP[chan]) == NULL)
      M_CHNBP[chan] = chn = (MCHNBLK *) mcalloc((long)sizeof(MCHNBLK));
    if (instrtxtp[chan+1] != NULL)           /* if corresp instr exists  */
      chn->pgmno = chan+1;                   /*     assign as pgmno      */
    else chn->pgmno = defaultinsno;          /* else assign the default  */
    mep->type = CONTROL_TYPE;
    mep->chan = chan;
    mep->dat1 = 121;  /* reset all controllers */
    m_chanmsg(mep);
    printf(Str(X_992,"midi channel %d using instr %d\n"), chan + 1, chn->pgmno);
}

static void ctlreset(short chan)    /* reset all controllers for this channel */
{
    MEVENT  mev;
    mev.type = CONTROL_TYPE;
    mev.chan = chan;
    mev.dat1 = 121;
    m_chanmsg(&mev);
}

MCHNBLK *m_getchnl(short chan)          /* get or create a chnlblk ptr */
{
    MCHNBLK *chn;
    if (chan < 0 || chan >= MAXCHAN) {
      sprintf(errmsg,Str(X_870,"illegal midi chnl no %d"), chan+1);
      die(errmsg);
    }
    if ((chn = M_CHNBP[chan]) == NULL) {
      M_CHNBP[chan] = chn = (MCHNBLK *) mcalloc((long)sizeof(MCHNBLK));
      chn->pgmno = -1;
      chn->insno = -1;
      ctlreset(chan);
    }
    return(chn);
}

void m_chinsno(short chan, short insno)   /* assign an insno to a chnl */
{                                         /* =massign: called from i0  */
    MCHNBLK  *chn = NULL;

    if (insno <= 0 /* || insno >= maxinsno */ || instrtxtp[insno] == NULL) {
      printf(Str(X_310,"Insno = %d\n"), insno);
      die(Str(X_1336,"unknown instr"));
    }
    if (M_CHNBP[chan] != NULL)
      printf(Str(X_987,"massign: chnl %d exists, ctrls now defaults\n"), chan+1);
    chn = m_getchnl(chan);
    chn->insno = insno;
    chn->pchbend = FL(0.0);     /* Mid value */
    /*    chn->posbend = FL(0.5); */          /* for pos pchbend (0 - 1.0) */
    ctlreset(chan);
    printf(Str(X_660,"chnl %d using instr %d\n"), chan+1, chn->insno);
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


static void m_start(void) {}      /* dummy sys_realtime targets */
static void m_contin(void) {}
static void m_stop(void) {}
static void m_sysReset(void) {}
static void m_tuneReq(void) {}

static int sexcnt = 0;
static void m_sysex(PmEvent *sbuf, PmEvent *sp) /* sys_excl msg, sexbuf: ID + data */
{
    int nbytes = sp - sbuf;
    if (++sexcnt >= 100) {
      printf(Str(X_178,"100th system exclusive $%x, length %d\n"),
             *sbuf, nbytes);
      sexcnt = 0;
    }
}

static short datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };
static short m_clktim = 0;
static short m_sensing = 0;
extern long GetMIDIData(void);

int sensMidi(void)         /* sense a MIDI event, collect the data & dispatch */
{                          /*  called from kperf(), return(2) if MIDI on/off  */
    long midiev;
    short  c, type;
    MEVENT *mep = Midevtblk;
    static  short datreq, datcnt;

 nxtchr:
    if (bufp >= endatp) {
      if (!GetMIDIData())
        return (0);
    }

    midiev = bufp->message;
    bufp++;
    if ((c = Pm_MessageStatus(midiev))) { /* STATUS byte:      */
      type = c & 0xF0;
      if (type == SYSTEM_TYPE) {
        short lo3 = (c & 0x07);
        if (c & 0x08)                    /* sys_realtime:     */
          switch (lo3) {                 /*   dispatch now    */
          case 0: m_clktim++;
            goto nxtchr;
          case 2: m_start();
            goto nxtchr;
          case 3: m_contin();
            goto nxtchr;
          case 4: m_stop();
            goto nxtchr;
          case 6: m_sensing = 1;
            goto nxtchr;
          case 7: m_sysReset();
            goto nxtchr;
          default: printf(Str(X_1316,"undefined sys-realtime msg %x\n"),c);
            goto nxtchr;
          }
        else {                           /* sys_non-realtime status:   */
          if (sexp != NULL) {            /* implies           */
            m_sysex(sexbuf,sexp);        /*   sys_exclus end  */
            sexp = NULL;
          }
          switch (lo3) {                 /* dispatch on lo3:  */
          case 7: goto nxtchr;           /* EOX: already done */
          case 0: sexp = sexbuf;         /* sys_ex begin:     */
            goto nxtchr;                 /*   goto copy data  */
          case 1:                        /* sys_common:       */
          case 3: datreq = 1;            /*   need some data  */
            break;
          case 2: datreq = 2;            /*   (so build evt)  */
            break;
          case 6: m_tuneReq();           /*   this do immed   */
            goto nxtchr;
          default: printf(Str(X_1317,"undefined sys_common msg %x\n"), c);
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
        if (sexp != NULL) {             /* also implies      */
          m_sysex(sexbuf,sexp);         /*   sys_exclus end  */
          sexp = NULL;
        }
        chan = c & 0xF;
        if (M_CHNBP[chan] == NULL)      /* chk chnl exists   */
          m_chn_init(mep, chan);
        mep->type = type;               /* & begin new event */
        mep->chan = chan;
        datreq = datbyts[(type>>4) & 0x7];
        datcnt = 0;
        goto nxtchr;
      }
    }
    if (sexp != NULL) {                 /* NON-STATUS byte:      */
      if (sexp < sexend) {              /* if sys_excl           */
        sexp->message = c;              /*    special data sav   */
        sexp++;
      }
      else printf(Str(X_1262,"system exclusive buffer overflow\n"));
      goto nxtchr;
    }
    if (datcnt == 0)
      mep->dat1 = c;                    /* else normal data      */
    else mep->dat2 = c;
    if (++datcnt < datreq)              /* if msg incomplete     */
      goto nxtchr;                      /*   get next char       */
    /*
     *  Enter the input event into a buffer used by 'midiin'.
     *  This is a horrible hack that emulates what DirectCsound does,
     *  in an attempt to make 'midiin' work.  It might be usable
     *  by other OSes than BeOS, but it should be cleaned up first.
     *
     *                  jjk 09262000
     */
    /* IV - Nov 30 2002: should work on other systems too */
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
      m_chanmsg(mep);                   /*   handle from here    */
      goto nxtchr;                      /*   & go look for more  */
    }
    return(2);                          /* else it's note_on/off */
}

void MidiClose(void)
{
    if(csoundIsExternalMidiEnabled(&cenviron)) {
      csoundExternalMidiDeviceClose(&cenviron);
    }
    else {
      Pm_Close(midistream);
    }
}
