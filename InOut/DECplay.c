/*  
    DECplay.c:

    Copyright (C) 1991 Dan Ellis

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

/***************************************************************\
*       DECplay.c (really newplay.c)                            *
*       rewrite of lofiplay.c                                   *
*       maybe a bit clearer                                     *
*       dpwe 12mar92 after lofiplay.c                           *
\***************************************************************/
#include <stdio.h>
#include "DECaudio.h"
#include "DECplay.h"

/* lofi memory map globals (from play*.asm code) */
#define ALIVEF      0x0FF0      /* flag set when other regs ready */
#define SRATE       0X0FF1      /* sampling rate requested */
#define HBSIZE      0x0FF2      /* total size of host buffer (samples) */
#define HBBLCK      0x0FF3      /* min block size by which HB advances */
#define HOBBASE     0x0FF4      /* base of host output buffer */
#define HOBPNTR     0x0FF5      /* current upper limit on host buffer */
#define HIBBASE     0x0FF6      /* base of host input buffer */
#define HIBPNTR     0x0FF7      /* upper limit on host buffer */

/* other constants */
#define MAXDBGWAIT      1000    /* how many iterations waiting for sync */
#define LOFI_CHANS      2       /* lofi buffer filled in pairs of samps */

/* Prototype argument wrapper (taken from dpwelib.h) */
/* make fn protos like   void fn PARG((int arg1, char arg2));  */
#ifdef __STDC__
#define PARG(a)         a
#else /* !__STDC__ */
#define PARG(a)         ()
#endif /* __STDC__ */

/* static function prototypes */
static void WaitForLofiSampFrms PARG((int frames));
static void timeout PARG((char *s));
static int ReadDspPos PARG((void));
static void WaitForDspPosAt PARG((int ourpos, int posval));
static long RdDSPadr PARG((int adr));
static void WrDSPadr PARG((int adr, long data));
#ifdef DEBUG_FILE
static void dbg_open PARG((void));
static void dbg_close PARG((void));
static void dbg_s PARG((char *s));
static void dbg_n PARG((int n));
static void dbg_lf PARG((void));
#endif /* DEBUG_FILE */

/* flags set on initialization ... */
static  int lofiOn = 0; /* flag that lofi believed booted */
static  MYFLT usPerFrame = 0;   /* sample rate reciprocal, for pausing */
static  int dspBufSiz = 0;      /* number of frames in dsp buffer */
static  int dspHopSiz = 0;      /* how many samples consumed at 1ce by dsp */
/* pointers to dsp memory mapped in host address space */
static  unsigned long   *playBuf;
static  unsigned long   *playPosPtr;
static  unsigned long   *recBuf;
static  unsigned long   *recPosPtr;
/* flags set by AUOpen */
static int chans   = 0;         /* number of channels we are writing */
static int ourPos = 0;          /* the index of the next frame to write */

int play_set(chs,dsize,srate,scale)     /* set up for fixed size calls  */
    int     chs;
    int     dsize;      /* ignored - assumed = sizeof(short) in what follows */
    MYFLT   srate;      /* passed to LoFi boot if required */
    int     scale;      /* crude gain - ignored here */
    {
#ifdef DEBUG_FILE
    dbg_open();
#endif /* DEBUG_FILE */
    if (MyBootLoFi((float)srate)==0)    return 0;    /* LoFi boot failed */
    chans   = chs;              /* remember how many chans we will get */
    ourPos = -1;                /* flag to resync on next play_on */
#ifdef DEBUG_FILE
    dbg_s("opened");    dbg_lf();
    dbg_s("chs:");      dbg_n(chans);           dbg_lf();
    dbg_s("sr:");       dbg_n((int)srate);      dbg_lf();
#endif /* DEBUG_FILE */
    return 1;
    }

#define SLOP_US         1000    /* how much to underwait */

static void WaitForLofiSampFrms(frames) /* idle for dsp to clear <frames> */
    int         frames;
    {
static struct   timeval timeout;        /* static to avoid alloc time */
    float uS;

    timeout.tv_sec = 0L;                /* needn't be repeated */
    uS = frames * usPerFrame;
    if (uS < SLOP_US)
        return;
    else
        timeout.tv_usec = uS - SLOP_US;
#ifdef DEBUG_FILE
    dbg_s("Wait: frames:");     dbg_n(frames);
    dbg_s(" ->usec:");          dbg_n(timeout.tv_usec); dbg_lf();
#endif /* DEBUG_FILE */
    select(0,NULL,NULL,NULL,&timeout);
    }

static void timeout(s)
    char *s;
    {
    err_printf(Str(X_328,"LoFi player: timeout at %s\n"),s);
    exit(1);
    }

static int ReadDspPos()         /* returns frame index of latest used sample */
    {                           /* WILL EQUAL blocksize - 1.. ? */
    long        dsps_next;
    int         pos;

    if ( (dsps_next = ((*playPosPtr)>>8L)/LOFI_CHANS)==0 )
        pos = dspBufSiz - 1;    /* dsp at start -> whole buffer read */
    else
        pos = dsps_next - 1;    /* else read up to sample preceding.. */
#ifdef DEBUG_FILE
    dbg_s("RdDspPos: pos:");    dbg_n(pos);     dbg_lf();
#endif /* DEBUG_FILE */
    return pos;
    }

static void WaitForDspPosAt(ourpos, posval)     /* block until dsp reaches point in buf */
    int         ourpos;         /* where we are now */
    int         posval;         /* minimum place for dsp pointer */
    {
    int         dbg_count;
    int         dsp_pos, dsp_next;

#ifdef DEBUG_FILE
    dbg_s("WaitForPos: from:"); dbg_n(ourpos);
    dbg_s(" for:");     dbg_n(posval);  dbg_lf();
#endif /* DEBUG_FILE */
    dbg_count = 0;
    while((   ((dsp_pos = ReadDspPos()) < posval
              && dsp_pos >= ourpos)
          || ((dsp_next = (dsp_pos+dspHopSiz)%dspBufSiz) < posval
              && dsp_next >= ourpos))
          && dbg_count < MAXDBGWAIT )
        {
        WaitForLofiSampFrms((posval - dsp_pos + dspBufSiz)%dspBufSiz);
        ++dbg_count;
        }
    if (dbg_count >= MAXDBGWAIT)
        timeout(Str(X_1375,"waiting for buffer to open"));
    }

void play_on(src, siz)
    short       *src;           /* 16bit samples (pairs if stereo) */
    long        siz;            /* number of sample-frames in src */
    {
    int         max_at_once;    /* maximum contiguous space at present */
    int         to_write_now;   /* how many frames we will do this pass */
    int         left_to_write;  /* counter for copying samples */
    int         reqd_dsp_pos;   /* frame index we want from dsp */
    unsigned long       *playBufPtr;    /* where to put samples */

#ifdef DEBUG_FILE
    dbg_lf();   dbg_s("play_on: siz:"); dbg_n(siz);     dbg_lf();
#endif /* DEBUG_FILE */
    if (ourPos == -1)           /* first time we are called after init */
        {
        WaitForDspPosAt(dspHopSiz, dspBufSiz - 1);
        /* start whole thing open */
        ourPos = 0;
        }

    /* ourPos records the next element of the buffer we will write to */
    max_at_once = dspBufSiz - ourPos;   /* max we can write at 1ce */
    if (siz > max_at_once)
        to_write_now = max_at_once;
    else
        to_write_now = siz;
    if (to_write_now > dspBufSiz - 2*dspHopSiz)
        to_write_now = dspBufSiz - 2*dspHopSiz; /* biggest gap */
    reqd_dsp_pos = ourPos + to_write_now - 1;   /* minus one is correct */
#ifdef DEBUG_FILE
    dbg_s("play_on: dbsiz:");   dbg_n(dspBufSiz);
    dbg_s(" ourPos:");          dbg_n(ourPos);
    dbg_s(" for_now:");         dbg_n(to_write_now);    dbg_lf();
#endif /* DEBUG_FILE */
    /* wait for enough space to open up in LOFI buffer to write all at 1ce */
    WaitForDspPosAt(ourPos, reqd_dsp_pos);
    /* now have at least to_write_now spaces open in the buffer */
    playBufPtr = playBuf + LOFI_CHANS*ourPos;

    left_to_write = to_write_now;
    while(left_to_write--)      /* copy them all */
        {
        *playBufPtr++ = ((long)*src)<<16;
        *playBufPtr++ = ((long)*(src+chans-1))<<16;
        src += chans;
        }
    if ( (ourPos += to_write_now) == dspBufSiz)
        ourPos = 0;
    siz -= to_write_now;        /* while we still know how many */
#ifdef DEBUG_FILEx
    dbg_s("play_on: sizleft:"); dbg_n(siz);     dbg_lf();
#endif /* DEBUG_FILE */
    if (siz > 0)
        play_on(src, siz);
    }

int get_playbuf_remains()               /* return number of unplayed samples */
    {
    WaitForLofiSampFrms((int)1000/usPerFrame);  /* approx 1ms pause */
    return (ourPos - ReadDspPos() + dspBufSiz) % dspBufSiz;
    }

void play_rls()                         /* release the play channel */
    {
#ifdef DEBUG_FILE
    dbg_s("play_rls");  dbg_lf();
#endif /* DEBUG_FILE */
        {
        LoFiClose();
        lofiOn = 0;
        }
    }

static long RdDSPadr(adr)               /* read long from DSP valid address */
    int adr;
    {
    return( (*(LoFiMap(RRAM, adr, 0)))>>8L );
    }

static void WrDSPadr(adr, data)
    int adr;
    long data;
    {
    *(LoFiMap(RRAM, adr, 1)) = data<<8L;
    }

int MyBootLoFi(srate)
    float srate;
    {
    long timeout;
    char *bootFile;
    char bootPath[128];
    char *loddir, *getenv(char *);

    if (lofiOn)         return 1;       /* lofi already on */
    if ((void *)LoFiOpen("/dev/lofi") == NULL)
        { err_printf(Str(X_211,"Cannot open LoFi\n"));
          return(0);    }
    if ((loddir = getenv("LODDIR")) == NULL)
        {
/*      err_printf("no LODDIR env variable\n"); return(0);      */
        loddir = "/usr/local/lib/lod";  /* give it a go if no env */
        }
    /* assume only decaudio supported.  Choose poss sampling rate */
    if (srate > 26000)        { bootFile = "play44ext.lod";
                               usPerFrame = 22.676; }
    else if (srate > 18000)   { bootFile = "play22ext.lod";
                               usPerFrame = 45.351; }
    else                     { bootFile = "play11ext.lod";
                               usPerFrame = 90.703; }

    sprintf(bootPath,"%s/%s",loddir,bootFile);
    err_printf(Str(X_618,"bootfile: %s\n"),bootPath);   /* */

    LoFiSetCSR(FED, 0);         /* disable DSP */
    dspLoad(bootPath);          /* load code */
    WrDSPadr(ALIVEF, 0L);       /* mark DSP as unbooted */
    WrDSPadr(SRATE, (long)srate);   /* notify requested sample rate */
    LoFiSetCSR(FED, 1);         /* reboot DSP */

    timeout = 1000;
    while( RdDSPadr(ALIVEF) == 0L && --timeout)
        WaitForLofiSampFrms((int)1000/usPerFrame);      /* approx 1ms pause */
    if (timeout == 0L)
        {
        err_printf(Str(X_493,"Timeout waiting for DSP to boot\n"));
        return(0);
        }
    /* buffer details - only set these once at boot */
    playBuf = LoFiMap(RRAM,(int)RdDSPadr(HOBBASE),1);
    recBuf  = LoFiMap(RRAM,(int)RdDSPadr(HIBBASE),1);
    dspBufSiz = (int)RdDSPadr(HBSIZE)/LOFI_CHANS;
    dspHopSiz = (int)RdDSPadr(HBBLCK)/LOFI_CHANS;
    playPosPtr = LoFiMap(RRAM, HIBPNTR, 0);
    recPosPtr = LoFiMap(RRAM, HOBPNTR, 0); /* addresses to read ptrs from */
#ifdef DEBUG_FILE
    dbg_s("BootLofi: dsp_buf:");        dbg_n(dspBufSiz);
    dbg_s(" hop_siz:"); dbg_n(dspHopSiz);       dbg_lf();
#endif /* DEBUG_FILE */
    err_printf( Str(X_622,"bufsize = %ld frames\n"), dspBufSiz);
    lofiOn = 1;
    return 1;
    }

/* some debugging hooks */
#ifdef DEBUG_FILE
static FILE *dbgFile = NULL;
static char *dbgFname = "dbg.out";
static void dbg_open()
{
    if ((dbgFile = fopen(dbgFname, "w"))==NULL) {
      err_printf( Str(X_682,"dbg_open: failed on %s\n"), dbgFname);
      exit(1);  }
}
static void dbg_close()
{
    fclose(dbgFile);
}
static void dbg_s(char *s)
{
    fprintf(dbgFile, "%s", s);
}
static void dbg_n(int n)
{
    fprintf(dbgFile, "%d", n);
}
static void dbg_lf()
    {
    fprintf(dbgFile, "\n");
    }
#endif /* DEBUG_FILE */
