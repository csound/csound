/*
    fmidi.c:

    Copyright (C) 1995 Barry Vercoe, John ffitch

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

/* Support for MIDI files which are not covered by PortMIDI */
/* FIXME for NeXT -- sbrandon */
#include "cs.h"                                       /*    FMIDI.C    */
#include "midiops.h"
#include "oload.h"

#ifdef MSVC  /* VL MSVC fix */
#      define  u_char  unsigned char
#      define  u_short unsigned short
#      define  u_int   unsigned int
#      define  u_long  unsigned long
#endif

#define MBUFSIZ   1024
#define ON        1
#define OFF       0
#define MAXLONG   0x7FFFFFFFL

static FILE *mfp = NULL;
static long  MTrkrem;
static void  (*nxtdeltim)(void);
static double FltMidiNxtk, kprdspertick, ekrdQmil;
static u_char *sexp;
static u_char *fsexbuf, *fsexp, *fsexend;
static short m_sensing = 0;
static int sexcnt = 0;
static short datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };

extern int pgm2ins[];   /* IV May 2002 */
extern void  schedofftim(INSDS *), deact(INSDS *), beep(void);
extern void insxtroff(short);
extern void m_chn_init(MEVENT *, short);
extern void m_chanmsg(MEVENT *);

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
      printf(Str(X_718,"end of track in midifile '%s'\n"), O.FMidiname);
      printf(Str(X_33,"%d forced decays, %d extra noteoffs\n"),
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
        printf(Str(X_712,"end of MPU401 midifile '%s'\n"), O.FMidiname);
        printf(Str(X_33,"%d forced decays, %d extra noteoffs\n"),
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

void FMidiOpen(void) /* open a MidiFile for reading, sense MPU401 or standard */
{                    /*     callable once from main.c      */
    short sval;
    long lval, tickspersec;
    u_long deltim;
    char inbytes[16];    /* must be long-aligned, 16 >= MThd maxlen */
#ifdef WORDS_BIGENDIAN
# define natshort(x) (x)
# define natlong(x)  (x)
#else
    extern long natlong(long);
    extern short natshort(short);
#endif

    FMidevtblk = (MEVENT *) mcalloc((long)sizeof(MEVENT));
    fsexbuf = (u_char *) mcalloc((long)MBUFSIZ);
    fsexend = fsexbuf + MBUFSIZ;
    fsexp = NULL;
    if (M_CHNBP[0] == (MCHNBLK*) NULL)      /* IV May 2002: added check */
      m_chn_init(FMidevtblk,(short)0);

    if (strcmp(O.FMidiname,"stdin") == 0) {
      mfp = stdin;
#if defined(mills_macintosh) || defined(SYMANTEC)
      die(Str(X_345,"MidiFile Console input not implemented"));
#endif
    }
    else if (!(mfp = fopen(O.FMidiname, "rb")))
      dies(Str(X_643,"cannot open '%s'"), O.FMidiname);
    if ((inbytes[0] = getc(mfp)) != 'M')
      goto mpu401;
    if ((inbytes[1] = getc(mfp)) != 'T') {
      ungetc(inbytes[1],mfp);
      goto mpu401;
    }
    if (fread(inbytes+2, 1, 6, mfp) < 6)
      dies(Str(X_1323,"unexpected end of '%s'"), O.FMidiname);
    if (strncmp(inbytes, "MThd", 4) != 0)
      dies(Str(X_1377,"we're confused.  file '%s' begins with 'MT',\n"
               "but not a legal header chunk"), O.FMidiname);
    printf(Str(X_72,"%s: found standard midifile header\n"), O.FMidiname);
    if ((lval = natlong(*(long *)(inbytes+4))) < 6 || lval > 16) {
      sprintf(errmsg,Str(X_614,"bad header length %ld in '%s'"),
              lval, O.FMidiname);
      die(errmsg);
    }
    if (fread(inbytes, 1, (int)lval, mfp) < (unsigned long)lval)
      dies(Str(X_1323,"unexpected end of '%s'"), O.FMidiname);
    sval = natshort(*(short *)inbytes);
    if (sval != 0 && sval != 1) { /* Allow Format 1 with single track */
      sprintf(errmsg,Str(X_67,"%s: Midifile format %d not supported"),
              O.FMidiname, sval);
      die(errmsg);
    }
    if ((sval = natshort(*(short *)(inbytes+2))) != 1)
      dies(Str(X_875,"illegal ntracks in '%s'"), O.FMidiname);
    if ((inbytes[4] & 0x80)) {
      short SMPTEformat, SMPTEticks;
      SMPTEformat = -(inbytes[4]);
      SMPTEticks = *(u_char *)inbytes+5;
      if (SMPTEformat == 29)  SMPTEformat = 30;  /* for drop frame */
      printf(Str(X_450,"SMPTE timing, %d frames/sec, %d ticks/frame\n"),
             SMPTEformat, SMPTEticks);
      tickspersec = SMPTEformat * SMPTEticks;
    }
    else {
      short Qticks = natshort(*(short *)(inbytes+4));
      printf(Str(X_344,"Metrical timing, Qtempo = 120.0, Qticks = %d\n"),
             Qticks);
      ekrdQmil = (double)ekr / Qticks / 1000000.0;
      tickspersec = Qticks * 2;
    }
    kprdspertick = (double)ekr / tickspersec;
    printf(Str(X_959,"kperiods/tick = %7.3f\n"), kprdspertick);

 chknxt:
    if (fread(inbytes, 1, 8, mfp) < 8)         /* read a chunk ID & size */
      dies(Str(X_1323,"unexpected end of '%s'"), O.FMidiname);
    if ((lval = natlong(*(long *)(inbytes+4))) <= 0)
      dies(Str(X_895,"improper chunksize in '%s'"), O.FMidiname);
    if (strncmp(inbytes, "MTrk", 4) != 0) {    /* if not an MTrk chunk,  */
      do {
        sval = getc(mfp);                      /*    skip over it        */
      } while (--lval);
      goto chknxt;
    }
    printf(Str(X_1294,"tracksize = %ld\n"), lval);
    MTrkrem = lval;                            /* else we have a track   */
    FltMidiNxtk = 0.0;
    FMidiNxtk = 0;                             /* init the time counters */
    nxtdeltim = Fnxtdeltim;                    /* set approp time-reader */
    nxtdeltim();                               /* incr by 1st delta-time */
    return;

 mpu401:
    printf(Str(X_69,
               "%s: assuming MPU401 midifile format, ticksize = 5 msecs\n"),
           O.FMidiname);
    kprdspertick = (double)ekr / 200.0;
    ekrdQmil = 1.0;                             /* temp ctrl (not needed) */
    MTrkrem = MAXLONG;                         /* no tracksize limit     */
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

static void m_sysex(u_char *sbuf, u_char *sp) /* sys_excl msg, sexbuf: ID + data */
{
    int nbytes = sp - sbuf;
    if (++sexcnt >= 100) {
      printf(Str(X_178,"100th system exclusive $%x, length %d\n"),
             *sbuf, nbytes);
      sexcnt = 0;
    }
}

static void fsexdata(int n) /* place midifile data into a sys_excl buffer */
{
    MTrkrem -= n;
    if (fsexp == NULL)                 /* 1st call, init the ptr */
      fsexp = fsexbuf;
    if (fsexp + n <= fsexend) {
      fread(fsexp, 1, n, mfp);       /* addin the new bytes    */
      fsexp += n;
      if (*(fsexp-1) == 0xF7) {      /* if EOX at end          */
        m_sysex(fsexbuf,fsexp);    /*    execute and clear   */
        fsexp = NULL;
      }
    }
    else {
      unsigned char c;
      printf(Str(X_1262,"system exclusive buffer overflow\n"));
      do {
        c = getc(mfp);
      } while (--n);
      if (c == 0xF7)
        fsexp = NULL;
    }
}

int sensFMidi(void)   /* read a MidiFile event, collect the data & dispatch   */
{                     /* called from kperf(), return(SENSMFIL) if MIDI on/off */
    short  c, type;
    MEVENT *mep = FMidevtblk;
    long len;
    static short datreq;

 nxtevt:
    if (--MTrkrem < 0 || (c = getc(mfp)) == EOF)
      goto Trkend;
    if (!(c & 0x80))               /* no status, assume running */
      goto datcpy;
    if ((type = c & 0xF0) == SYSTEM_TYPE) {     /* STATUS byte:      */
      short lo3;
      switch(c) {
      case 0xF0:                    /* SYS_EX event:  */
        if ((len = vlendatum()) <= 0)
          die(Str(X_1401,"zero length sys_ex event"));
        printf(Str(X_1152,"reading sys_ex event, length %ld\n"),len);
        fsexdata((int)len);
        goto nxtim;
      case 0xF7:                    /* ESCAPE event:  */
        if ((len = vlendatum()) <= 0)
          die(Str(X_1400,"zero length escape event"));
        printf(Str(X_747,"escape event, length %ld\n"),len);
        if (sexp != NULL)
          fsexdata((int)len);       /* if sysex contin, send  */
        else {
          MTrkrem -= len;
          do {
            c = getc(mfp);          /* else for now, waste it */
          } while (--len);
        }
        goto nxtim;
      case 0xFF:                    /* META event:     */
        if (--MTrkrem < 0 || (type = getc(mfp)) == EOF)
          goto Trkend;
        len = vlendatum();
        MTrkrem -= len;
        switch(type) {
          long usecs;
        case 0x51: usecs = 0;       /* set new Tempo       */
          do {
            usecs <<= 8;
            usecs += (c = getc(mfp)) & 0xFF;
          }
          while (--len);
          if (usecs <= 0)
            printf(Str(X_47,"%ld usecs illegal in Tempo event\n"), usecs);
          else {
            kprdspertick = usecs * ekrdQmil;
            /*    printf("Qtempo = %5.1f\n", 60000000. / usecs); */
          }
          break;
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:                   /* print text events  */
        case 0x06:
        case 0x07:
          while (len--) {
            int ch;
            ch = getc(mfp);
            printf("%c", ch);
          }
          break;
        case 0x2F: goto Trkend;      /* normal end of track */
        default:
          printf(Str(X_1192,"skipping meta event type %x\n"),type);
          do {
            c = getc(mfp);
          } while (--len);
        }
        goto nxtim;
      }
      lo3 = (c & 0x07);
      if (c & 0x08) {                /* sys_realtime:     */
        switch (lo3) {               /*   dispatch now    */
        case 0:
        case 1: break;    /* Timing Clk handled in Rnxtdeltim() */
        case 2: /*m_start();*/ break;
        case 3: /*m_contin();*/ break;
        case 4: /*m_stop()*/; break;
        case 6: m_sensing = 1; break;
        case 7: /*m_sysReset()*/; break;
        default: printf(Str(X_1316,"undefined sys-realtime msg %x\n"),c);
        }
        goto nxtim;
      }
      else if (lo3 == 6) {           /* sys_non-realtime status:   */
        /* m_tuneReq();*/            /* do this one immed  */
        goto nxtim;
      }
      else {
        mep->type = type;            /* ident sys_com event  */
        mep->chan = lo3;             /* holding code in chan */
        switch (lo3) {               /* now need some data   */
        case 1:
        case 3: datreq = 1;
          break;
        case 2: datreq = 2;
          break;
        default: sprintf(errmsg,Str(X_1317,"undefined sys_common msg %x\n"), c);
          die(errmsg);
        }
      }
    }
    else {                            /* other status types:  */
      short chan = c & 0xF;
      if (M_CHNBP[chan] == NULL)      /*   chk chnl exists    */
        m_chn_init(mep, chan);
      mep->type = type;               /*   & begin new event  */
      mep->chan = chan;
      datreq = datbyts[(type>>4) & 0x7];
    }
    c = getc(mfp);
    MTrkrem--;

 datcpy:
    mep->dat1 = c;                    /* sav the required data */
    if (datreq == 2) {
      mep->dat2 = getc(mfp);
      MTrkrem--;
    }
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
    if (mep->type > NOTEON_TYPE) {    /* if control or syscom  */
      m_chanmsg(mep);                 /*   handle from here    */
      goto nxtim;
    }
    nxtdeltim();
    return(3);                        /* else it's note_on/off */

 nxtim:
    nxtdeltim();
    if (O.FMidiin && kcounter >= FMidiNxtk)
      goto nxtevt;
    return(0);

 Trkend:
    printf(Str(X_715,"end of midi track in '%s'\n"), O.FMidiname);
    printf(Str(X_33,"%d forced decays, %d extra noteoffs\n"),
           Mforcdecs, Mxtroffs);
    MTrkend = 1;
    O.FMidiin = 0;
    if (O.ringbell && !O.termifend)  beep();
    return(0);
}


