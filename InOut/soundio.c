/*  
soundio.c:

Copyright (C) 1991, 2000 Barry Vercoe, John ffitch, Richard Dobson, Istvan Varga

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

#include "cs.h"                 /*                      SOUNDIO.C       */
#include "soundio.h"
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef mills_macintosh
#include "MacTransport.h"
#endif

#ifdef RTAUDIO
extern  int     (*rtrecord)(char *, int);
extern  void    (*rtplay)(void *, int);
extern  void    (*rtclose)(void);
extern  void    (*recopen)(int, int, float, int);
extern  void    (*playopen)(int, int, float, int);
#endif

void spoutsf(void);
void zerosf(long len);
void writesf(void *outbuf, int nbytes);

/******* RWD 3:2000 requirements and assumtions for sample formats:
* (1)  the assumed 'normal' range for a Csound audio sample is +- 32767.0
*      This will thus translate directly to 16bit storage, with a direct cast.
* (2)  floating-point WAVE and AIFF-C files should contain normalised floats
*      (+- 1.0) so the data must be scaled accordingly, based on (1).
* (3)  32bit integer files contain samples in range +- (2^^31)-1, so again
*      must be scaled from the Csound 'normal' range.
* (4)  (if/when 24bit formats are supported, samples must be similarly scaled
*      into the range (2^^23)-1.
* (5)  PEAK data IN ALL CASES is normalised to +- 1.0. For floatsam files,
*      where the values can exceed this range, the PEAK values will record
*      that directly.  For all integer formats, PEAK will reflect clipping,
*      so will never exceed +-1.0.
*/

char    *sfoutname;                     /* soundout filename    */
char    *inbuf;
char    *outbuf;                        /* contin sndio buffers */
char    *chinbufp, *choutbufp;          /* char  pntr to above  */
short   *shinbufp, *shoutbufp;          /* short pntr           */
long    *llinbufp, *lloutbufp;          /* long  pntr           */
float   *flinbufp, *floutbufp;          /* MYFLT pntr           */
unsigned inbufrem, outbufrem;           /* in monosamps         */
/* (see openin,iotranset)    */
unsigned inbufsiz,  outbufsiz;          /* alloc in sfopenin/out     */
int     isfd, isfopen = 0, infilend = 0;/* (real set in sfopenin)    */
int     osfd, osfopen = 0;              /* (real set in sfopenout)   */
int     pipdevin = 0, pipdevout = 0;    /* mod by sfopenin,sfopenout */

extern unsigned long   nframes;
extern  HEADATA *readheader(int, char*, SOUNDIN*);
#ifdef ULAW
extern  short   ulaw_decode[];
#endif
extern  OPARMS  O;

static  SOUNDIN *p;    /* to be passed via sreadin() */
int     (*audrecv)(char *, int);
void    (*audtran)(char *, int);
static  int     audread(char *, int);
static  void    audwrite(char *, int);
static  void    audwrtrev2(char *, int), audwrtrev4(char *, int);
/*RWD 5:2001*/
static void audwrtrev3(char *,int);

extern  void    bytrev2(char *, int), bytrev4(char *, int);
extern  void    rewriteheader(int, long, int);
extern  int     openin(char *), openout(char *, int);
extern  int     bytrevhost(void), getsizformat(int);
extern  char    *getstrformat(int);

/*RWD 5:2001 24bit support */
extern void     bytrev3(char *,int);
static void     int24ptran(void);
static void     int24pzerotran(long);
static char     *int24outbufp,*int24inbufp;
static void     int24precv(void);

#ifdef RTAUDIO
#define DEVAUDIO 0x7fff         /* unique fd for rtaudio  */
# ifdef sol
extern  int     audiofd;
# endif
#endif
#ifdef PIPES
/*sbrandon: added NeXT to line below*/
# if defined(SGI) || defined(LINUX) || defined(__BEOS__) || defined(NeXT) || defined(__MACH__)
#  define _popen popen
#  define _pclose pclose
# endif
#endif
void (*spinrecv)(void), (*spoutran)(void), (*nzerotran)(long);

void sndwrterr(unsigned nret, unsigned nput) /* report soundfile write(osfd)
																						 error      */
																						 /* called after chk of write() bytecnt  */
{
	void sfcloseout(void);
	printf(Str(X_1203,"soundfile write returned bytecount of %d, not %d\n"),
		nret,nput);
	printf(Str(X_77,"(disk may be full...\n closing the file ...)\n"));
	outbufrem = O.outbufsamps;       /* consider buf is flushed */
	sfcloseout();                    /* & try to close the file */
	die(Str(X_563,"\t... closed\n"));
}

static void byterecv(void), charrecv(void),
#ifdef never
alawrecv(void),
#endif
#ifdef ULAW
ulawrecv(void),
#endif
shortrecv(void),longrecv(void), floatrecv(void);
static void bytetran(void), chartran(void),
#ifdef never
alawtran(void), 
#endif
#ifdef ULAW
ulawtran(void),
#endif
shortran(void), longtran(void), floatran(void);
static void bytetran_d(void), chartran_d(void), 
#ifdef ULAW
ulawtran_d(void),
#endif
shortran_d(void), longtran_d(void);
static void bzerotran(long), czerotran(long),
#ifdef never
azerotran(long), 
#endif
#ifdef ULAW
uzerotran(long),
#endif
szerotran(long), lzerotran(long), fzerotran(long);
static void floatran_noscl(void), floatrecv_noscl(void); /* IV - Jul 11 2002 */

static int audread(char *inbuf, int nbytes) /* diskfile read option for
																						audrecv's */
																						/*     assigned during sfopenin() */
{
	return(sreadin(isfd,inbuf,nbytes,p));
}

#if !defined(SYMANTEC) && !defined(mac_classic) && !defined(LINUX) && !defined(__BEOS__) && !defined(__MACH__)
extern int write(int, const void*, unsigned int);
#endif

static void audwrite(char *outbuf, int nbytes)
/* diskfile write option for audtran's */
/*      assigned during sfopenout()    */
{
	int n;
	if (osfd<0) return;
	if ((n = write(osfd, outbuf, nbytes)) < nbytes)
		sndwrterr(n, nbytes);
	if (O.rewrt_hdr)
		rewriteheader(osfd, nbytes +(long)nrecs*outbufsiz, 0);
	nrecs++;                /* JPff fix */
	if (O.heartbeat) {
		if (O.heartbeat==1) {
#ifdef SYMANTEC
			nextcurs();
#else
			putc("|/-\\"[nrecs&3], stderr); putc(8,stderr);
#endif
		}
		else if (O.heartbeat==2) putc('.', stderr);
		else if (O.heartbeat==3) {
			int n;
			err_printf( "%d(%.3f)%n", nrecs, nrecs/ekr, &n);
			while (n--) err_printf("\b");
		}
		else err_printf("\a");

	}
}
#ifdef NeXT /*sbrandon: for RT playback */
static void swaprtplay(void *outbuf, int nbytes)
/* soundout write option for audtran's */
/*      assigned during sfopenout()    */
{
	bytrev2((char*)outbuf, nbytes);    /* rev bytes in shorts  */
	rtplay(outbuf, nbytes);     /*   & send the data    */
}
#endif

static void audwrtrev2(char *outbuf, int nbytes)
/* diskfile write option for audtran's */
/*      assigned during sfopenout()    */
{
	bytrev2(outbuf, nbytes);    /* rev bytes in shorts  */
	audwrite(outbuf, nbytes);   /*   & send the data    */
}
/*RWD 5:2001 */
static void audwrtrev3(char *outbuf, int nbytes)
/* diskfile write option for audtran's */
/*      assigned during sfopenout()    */
{
	bytrev3(outbuf, nbytes);    /* rev bytes in shorts  */
	audwrite(outbuf, nbytes);   /*   & send the data    */
}


/*RWD 3:2000  format fixups: reversed.... */
#define OUTFLOATFAC (FL(1.0) / FL(32768.0))
#define OUTLONGFAC (16)
static void audwrtrev4(char *outbuf, int nbytes)
/* diskfile write option for audtran's */
/*      assigned during sfopenout()    */
{
	/* RWD 5:2001 removed format fixups! Now in audtran funcs, so good
	* for rtaudio too */
	bytrev4(outbuf, nbytes);    /* rev bytes in longs   */
	audwrite(outbuf, nbytes);   /*   & send the data    */
}

/*RWD 3:2000 .... and non-byte-reversed files*/
static void audwrt4(char *outbuf,int nbytes)
{
	/* RWD 5:2001 removed format fixups! Now in audtran funcs, so good
	* for rtaudio too */

	audwrite(outbuf, nbytes);   /*  & send the data    */
}

extern int close(int);

static void bytetran(void)                           /* J. Mohr  1995 Oct 17 */
/* same as above, but 8-bit unsigned char output */
{             /*   sends HI-ORDER 8 bits of shortsamp, converted to unsigned */
	MYFLT       *sp, *maxampp;
	unsigned long       *maxps;
	long        longsmp, *rngp;
	int         n, spoutrem;
	MYFLT       absamp;

	sp = spout;                 /* adr spout    */
	spoutrem = nspout;          /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;            /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		if ((longsmp = (long)*sp) >= 0) { /* +ive samp:   */
			if (*sp > *maxampp) {           /*  maxamp this seg  */
				*maxampp = *sp;
				*maxps = nframes;
			}
			if (longsmp > 32767) {          /* out of range?     */
				longsmp = 32767;      /*   clip and report */
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		else {                                    /* ditto -ive samp */
			if ((absamp = -*sp) > *maxampp) {
				*maxampp = absamp;
				*maxps = nframes;
			}
			if (longsmp < -32768) {
				longsmp = -32768;
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		if (osfopen)
			*choutbufp++ = (unsigned char)(longsmp >> 8)^0x80;
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			choutbufp = outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

static void bytetran_d(void)/* J. Mohr  1995 Oct 17 */
/* same as above, but 8-bit unsigned char output */
{             /*   sends HI-ORDER 8 bits of shortsamp, converted to unsigned */
	MYFLT       *sp, *maxampp;
	unsigned long       *maxps;
	long        longsmp, *rngp;
	int         n, spoutrem;
	MYFLT       absamp;

	sp = spout;                 /* adr spout    */
	spoutrem = nspout;          /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem)/* if nspout remaining > buf rem, */
		n = outbufrem;                    /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		longsmp = (long)floor(*sp + 128.0*(MYFLT)rand()/ RAND_MAX);
		if ((longsmp) >= 0) {             /* +ive samp:   */
			if (*sp > *maxampp) {           /*  maxamp this seg  */
				*maxampp = *sp;
				*maxps = nframes;
			}
			if (longsmp > 32767) {          /* out of range?     */
				longsmp = 32767;              /*   clip and report */
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		else {                            /* ditto -ive samp */
			if ((absamp = -*sp) > *maxampp) {
				*maxampp = absamp;
				*maxps = nframes;
			}
			if (longsmp < -32768) {
				longsmp = -32768;
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		if (osfopen)
			*choutbufp++ = (unsigned char)(longsmp >> 8)^0x80;
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			choutbufp = outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

static void shortran(void)      /* fix spout vals and put in outbuf */
{                               /*      write buffer when full      */
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	MYFLT   longsmp;               /* MUST br MYFLT, as 0dbfs may be 1.0 !*/
	long *rngp;
	int    n, spoutrem;
	MYFLT   absamp;

	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		longsmp = *sp;
		if (longsmp >= 0) { /* +ive samp:   */
			if (*sp > *maxampp) {             /*  maxamp this seg  */
				*maxampp = *sp;
				*maxps = nframes;
			}
			if (longsmp > e0dbfs) {          /* out of range?     */
				longsmp = e0dbfs;        /*   clip and report */
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		else {                            /* ditto -ive samp */
			if ((absamp = -*sp) > *maxampp) {
				*maxampp = absamp;
				*maxps = nframes;
			}
			if (longsmp < -e0dbfs) {
				longsmp = -e0dbfs;
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		if (osfopen)
			*shoutbufp++ = (short) (longsmp * dbfs_to_short);
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			shoutbufp = (short *) outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

static void shortran_d(void)    /* fix spout vals and put in outbuf */
{                               /*      write buffer when full      */
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	long   longsmp, *rngp;
	int    n, spoutrem;
	MYFLT   absamp;

	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		longsmp = (long)floor(*sp + (MYFLT)rand()/ RAND_MAX);
		if (longsmp >= 0) { /* +ive samp:   */
			if (*sp > *maxampp) {             /*  maxamp this seg  */
				*maxampp = *sp;
				*maxps = nframes;
			}
			if (longsmp > 32767) {          /* out of range?     */
				longsmp = 32767;        /*   clip and report */
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		else {                            /* ditto -ive samp */
			if ((absamp = -*sp) > *maxampp) {
				*maxampp = absamp;
				*maxps = nframes;
			}
			if (longsmp < -32768) {
				longsmp = -32768;
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		if (osfopen)
			*shoutbufp++ = (short) longsmp;
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			shoutbufp = (short *) outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

static void chartran(void)      /* same as above, but 8-bit char output */
{                               /*   sends HI-ORDER 8 bits of shortsamp */
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	long   longsmp, *rngp;
	int    n, spoutrem;
	MYFLT   absamp;

	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		longsmp = (long)*sp;
		if (longsmp >= 0) { /* +ive samp:   */
			if (*sp > *maxampp) {           /*  maxamp this seg  */
				*maxampp = *sp;
				*maxps = nframes;
			}
			if (longsmp > 32767) {          /* out of range?     */
				longsmp = 32767;        /*   clip and report */
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		else {                                  /* ditto -ive samp */
			if ((absamp = (MYFLT)((long)(-(*sp)))) > *maxampp) {
				*maxampp = absamp;
				*maxps = nframes;
			}
			if (longsmp < -32768) {
				longsmp = -32768;
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		if (osfopen)
			*choutbufp++ = (char)(longsmp >> 8);
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			choutbufp = outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

static void chartran_d(void)    /* same as above, but 8-bit char output */
{                               /*   sends HI-ORDER 8 bits of shortsamp */
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	long   longsmp, *rngp;
	int    n, spoutrem;
	MYFLT   absamp;

	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		longsmp = (long)floor(*sp + 128.0*(MYFLT)rand()/ RAND_MAX);
		if (longsmp >= 0) { /* +ive samp:   */
			if (*sp > *maxampp) {           /*  maxamp this seg  */
				*maxampp = *sp;
				*maxps = nframes;
			}
			if (longsmp > 32767) {          /* out of range?     */
				longsmp = 32767;        /*   clip and report */
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		else {                                  /* ditto -ive samp */
			if ((absamp = (MYFLT)((long)(-(*sp)))) > *maxampp) {
				*maxampp = absamp;
				*maxps = nframes;
			}
			if (longsmp < -32768) {
				longsmp = -32768;
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		if (osfopen)
			*choutbufp++ = (char)(longsmp >> 8);
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			choutbufp = outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

#ifdef never
static void alawtran(void)
{
	die(Str(X_590,"alaw not yet implemented"));
}
#endif

#define MUCLIP  32635
#define BIAS    0x84
#define MUZERO  0x02
#define ZEROTRAP

#ifdef ULAW
static void ulawtran(void)      /* ulaw-encode spout vals & put in outbuf */
{                               /*      write buffer when full      */
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	long   longsmp, *rngp;
	int    n, spoutrem, sign;
	extern  char    exp_lut[];               /* mulaw encoding table */

	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		longsmp = (long)*sp;
		if (longsmp < 0L) {          /* if sample negative   */
			sign = 0x80;
			longsmp = - longsmp;        /*  make abs, save sign */
		}
		else sign = 0;
		if (longsmp > *maxampp) {             /* save maxamp this seg  */
			*maxampp = (MYFLT)longsmp;
			*maxps = nframes;
		}
		if (longsmp > MUCLIP) {             /* out of range?     */
			longsmp = MUCLIP;           /*   clip and report */
			rngp = rngcnt + (maxampp - maxamp);
			(*rngp)++;
			rngflg = 1;
		}
		if (osfopen) {
			int sample, exponent, mantissa, ulawbyte;
			sample = (int)(longsmp + BIAS);
			exponent = exp_lut[( sample >> 8 ) & 0x7F];
			mantissa = ( sample >> (exponent+3) ) & 0x0F;
			ulawbyte = ~ (sign | (exponent << 4) | mantissa );
#ifdef ZEROTRAP
			if (ulawbyte == 0) ulawbyte = MUZERO;    /* optional CCITT trap */
#endif
			*choutbufp++ = ulawbyte;
		}
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			choutbufp = outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

static void ulawtran_d(void)      /* ulaw-encode spout vals & put in outbuf */
{                                 /*      write buffer when full      */
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	long   longsmp, *rngp;
	int    n, spoutrem, sign;
	extern  char    exp_lut[];               /* mulaw encoding table */

	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		longsmp = (long)floor(*sp + (MYFLT)rand()/ RAND_MAX);
		if (longsmp < 0L) {          /* if sample negative   */
			sign = 0x80;
			longsmp = - longsmp;        /*  make abs, save sign */
		}
		else sign = 0;
		if (longsmp > *maxampp) {             /* save maxamp this seg  */
			*maxampp = (MYFLT)longsmp;
			*maxps = nframes;
		}
		if (longsmp > MUCLIP) {             /* out of range?     */
			longsmp = MUCLIP;           /*   clip and report */
			rngp = rngcnt + (maxampp - maxamp);
			(*rngp)++;
			rngflg = 1;
		}
		if (osfopen) {
			int sample, exponent, mantissa, ulawbyte;
			sample = (int)(longsmp + BIAS);
			exponent = exp_lut[( sample >> 8 ) & 0x7F];
			mantissa = ( sample >> (exponent+3) ) & 0x0F;
			ulawbyte = ~ (sign | (exponent << 4) | mantissa );
#ifdef ZEROTRAP
			if (ulawbyte == 0) ulawbyte = MUZERO;    /* optional CCITT trap */
#endif
			*choutbufp++ = ulawbyte;
		}
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			choutbufp = outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}
#endif

static void longtran(void)          /* send long_int spout vals to outbuf */
{                                   /*      write buffer when full      */
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	int    n, spoutrem;
	MYFLT  absamp,fltsmp,flimit;    /* RWD 1:3:2001 added fltsmp for 0dbfs = 1 */
	long   longsmp, *rngp;          /*RWD Nov 2001 */

	/* IV Jun 2002: added to avoid overflows with 32-bit floats */
	flimit = (sizeof(MYFLT) < 8 ? FL(0.999999) * e0dbfs : e0dbfs);
	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		if ((absamp = *sp) < FL(0.0))
			absamp = -absamp;
		if (absamp > *maxampp) {          /*  maxamp this seg  */
			*maxampp = absamp;
			*maxps = nframes;
		}
		fltsmp = (MYFLT) *sp;
		/*RWD Nov 2001 add warnings */
		if (fltsmp >= 0) { /* +ive samp:   */
			if (fltsmp > flimit) {          /* out of range?     */
				fltsmp = flimit;        /*   clip and report */
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		else {                            /* ditto -ive samp */
			if (fltsmp < -flimit) {
				fltsmp = -flimit;
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		longsmp = (long)(fltsmp * dbfs_to_long);
		if (osfopen)
			*lloutbufp++ = longsmp;
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			lloutbufp = (long *) outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

static void longtran_d(void)            /* send long_int spout vals to outbuf */
{                                       /*      write buffer when full      */
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	int    n, spoutrem;
	MYFLT  absamp;
	long   longsmp, *rngp;              /*RWD Nov 2001 */

	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		if ((absamp = *sp) < FL(0.0))
			absamp = -absamp;
		if (absamp > *maxampp) {          /*  maxamp this seg  */
			*maxampp = absamp;
			*maxps = nframes;
		}
		longsmp = (long)floor(*sp + (MYFLT)rand()/ RAND_MAX);

		/*RWD Nov 2001 add warnings */
		if (longsmp >= 0) { /* +ive samp:   */
			if (longsmp > 32767) {          /* out of range?     */
				longsmp = 32767;        /*   clip and report */
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		else {                            /* ditto -ive samp */
			if (longsmp < -32768) {
				longsmp = -32768;
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		if (osfopen)
			*lloutbufp++ = longsmp;
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			lloutbufp = (long *) outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

static void floatran(void)              /* send float spout vals to outbuf */
{                                       /*      write buffer when full      */
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	int    n, spoutrem;
	MYFLT  absamp;
	float       fltsmp;
	long   *rngp;                       /*RWD Nov 2001 */
	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		if ((absamp = *sp) < 0.0f)
			absamp = -absamp;
		if (absamp > *maxampp) {         /*  maxamp this seg  */
			*maxampp = absamp;
			*maxps = nframes;
		}
		/*RWD Nov 2001: range translated version: add WARNINGS for over-range */
		fltsmp = (float) *sp;
		if (fltsmp >= 0) { /* +ive samp:   */
			if (fltsmp > e0dbfs) {          /* out of range?     */
				/*   report it*/
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		else {                            /* ditto -ive samp */
			if (fltsmp < -e0dbfs) {
				rngp = rngcnt + (maxampp - maxamp);
				(*rngp)++;
				rngflg = 1;
			}
		}
		fltsmp = (float)(*sp * dbfs_to_float);
		if (osfopen)
			*floutbufp++ = fltsmp;
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			floutbufp = (float *) outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

/*RWD 5:2001 */
/* 24:24 packed data */
static void int24ptran(void)
{
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	long   *rngp;
	int    n, spoutrem;
	MYFLT  absamp, fltsmp = FL(0.0);/* RWD 1:3:2001 need fltsmp for 0dbfs = 1 */
	SAMP24   s24;

	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;

nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		if ((absamp = *sp) < FL(0.0))
			absamp = -absamp;
		if (absamp > *maxampp) {          /*  maxamp this seg  */
			*maxampp = absamp;
			*maxps = nframes;
		}
		/*  incorporate overrange detection, before scaling to 24bit range...
		*  but ONLY for WAVE/AIFF/AIFF_C
		*/
		if (O.filetyp==TYP_WAV || O.filetyp== TYP_AIFF) {
			fltsmp = (MYFLT) *sp;
			if (fltsmp >= 0) { /* +ive samp:   */
				if (*sp > *maxampp) {             /*  maxamp this seg  */
					*maxampp = *sp;
					*maxps = nframes;
				}
				if (fltsmp > e0dbfs) {          /* out of range?     */
					fltsmp = e0dbfs;        /*   clip and report */
					rngp = rngcnt + (maxampp - maxamp);
					(*rngp)++;
					rngflg = 1;
				}
			}
			else {                            /* ditto -ive samp */
				if ((absamp = -*sp) > *maxampp) {
					*maxampp = absamp;
					*maxps = nframes;
				}
				if (fltsmp < -e0dbfs) {
					fltsmp = -e0dbfs;
					rngp = rngcnt + (maxampp - maxamp);
					(*rngp)++;
					rngflg = 1;
				}
			}
		}
		s24.lsamp = (long) (fltsmp * dbfs_to_long);

		if (osfopen) {
			/* this is for direct 24bit packed output */
#if defined(_macintosh)||defined(SGI)||defined(LINUX_BE)||defined(LINUX_PPC)
			/* and all similar byte order machines */
			*int24outbufp++ = s24.bytes[0];
			*int24outbufp++ = s24.bytes[1];
			*int24outbufp++ = s24.bytes[2];
#else
			*int24outbufp++ = s24.bytes[1];
			*int24outbufp++ = s24.bytes[2];
			*int24outbufp++ = s24.bytes[3];
#endif
		}
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else
			nframes++;
		sp++;
	}
	while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			int24outbufp = (char *) outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

static void int24pzerotran(long kcnt)
{
	int   n, smpsrem, clearcnt = 0;

	if (!osfopen)
		return;
	smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
nchk:
	if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	smpsrem -= n;
	outbufrem -= n;
	if (clearcnt < O.outbufsamps) {
		clearcnt += n;          /* clear buf only till clean */
		do {
			*int24outbufp++ = 0x00;
			*int24outbufp++ = 0x00;
			*int24outbufp++ = 0x00;
		} while (--n);
	}
	else
		int24outbufp += n;
	if (!outbufrem) {
		audtran(outbuf,outbufsiz);
		int24outbufp = outbuf;
		outbufrem = O.outbufsamps;
		if (smpsrem)
			goto nchk;
	}
}

static void szerotran(long kcnt) /* copy kcnt zerospouts to short soundbuf, */
/*      sending buffer whenever full     */
{
	int   n, smpsrem, clearcnt = 0;

	if (!osfopen)  return;
	smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
nchk:
	if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	smpsrem -= n;
	outbufrem -= n;
	if (clearcnt < O.outbufsamps) {
		clearcnt += n;          /* clear buf only till clean */
		do *shoutbufp++ = (short) 0;
		while (--n);
	}
	else shoutbufp += n;
	if (!outbufrem) {
		audtran(outbuf,outbufsiz);
		shoutbufp = (short *) outbuf;
		outbufrem = O.outbufsamps;
		if (smpsrem) goto nchk;
	}
}

static void bzerotran(long kcnt)                 /* J. Mohr  1995 Oct 17 */
/* copy kcnt zerospouts to (unsigned) char soundbuf, */
/* sending buffer whenever full */
{
	int n, smpsrem, clearcnt = 0;

	if (!osfopen)  return;
	smpsrem = nspout * (int)kcnt;       /* calculate total smps to go   */
nchk:
	if ((n = smpsrem) > (int)outbufrem) /* if smps remaining > buf rem, */
		n = outbufrem;                    /*      prepare to send in parts  */
	smpsrem -= n;
	outbufrem -= n;
	if (clearcnt < O.outbufsamps) {
		clearcnt += n;                    /* clear buf only till clean */
		do *choutbufp++ = (const char)0x80;
		while (--n);
	}
	else choutbufp += n;
	if (!outbufrem) {
		audtran(outbuf,outbufsiz);
		choutbufp = outbuf;
		outbufrem = O.outbufsamps;
		if (smpsrem) goto nchk;
	}
}

static void czerotran(long kcnt)
/* copy kcnt zerospouts to (signed) char soundbuf, */
/*      sending buffer whenever full     */
{
	int   n, smpsrem, clearcnt = 0;

	if (!osfopen)  return;
	smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
nchk:
	if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	smpsrem -= n;
	outbufrem -= n;
	if (clearcnt < O.outbufsamps) {
		clearcnt += n;          /* clear buf only till clean */
		do *choutbufp++ = 0x00;
		while (--n);
	}
	else choutbufp += n;
	if (!outbufrem) {
		audtran(outbuf,outbufsiz);
		choutbufp = outbuf;
		outbufrem = O.outbufsamps;
		if (smpsrem) goto nchk;
	}
}

#ifdef never
static void azerotran(long kcnt)
{
	die(Str(X_590,"alaw not yet implemented"));
}
#endif

#ifdef ULAW
static void uzerotran(long kcnt)/* copy kcnt zerospouts to ulaw soundbuf, */
/*      sending buffer whenever full     */
{
	int   n, smpsrem, clearcnt = 0;

	if (!osfopen)  return;
	smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
nchk:
	if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	smpsrem -= n;
	outbufrem -= n;
	if (clearcnt < O.outbufsamps) {
		clearcnt += n;          /* clear buf only till clean */
		do *choutbufp++ = (const char) 0xFF; /* no signal is 0xFF in mulaw */
		while (--n);
	}
	else choutbufp += n;
	if (!outbufrem) {
		audtran(outbuf,outbufsiz);
		choutbufp = outbuf;
		outbufrem = O.outbufsamps;
		if (smpsrem) goto nchk;
	}
}
#endif

static void lzerotran(long kcnt)
/* copy kcnt zerospouts to long_int soundbuf, */
/*      sending buffer whenever full          */
{
	int   n, smpsrem, clearcnt = 0;

	if (!osfopen)  return;
	smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
nchk:
	if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
		n = outbufrem;                     /*    prepare to send in parts  */
	smpsrem -= n;
	outbufrem -= n;
	if (clearcnt < O.outbufsamps) {
		clearcnt += n;                     /* clear buf only till clean */
		do *lloutbufp++ = 0L;
		while (--n);
	}
	else lloutbufp += n;
	if (!outbufrem) {
		audtran(outbuf,outbufsiz);
		lloutbufp = (long *) outbuf;
		outbufrem = O.outbufsamps;
		if (smpsrem) goto nchk;
	}
}

static void fzerotran(long kcnt)    /* copy kcnt zerospouts to float soundbuf, */
/*      sending buffer whenever full     */
{
	int   n, smpsrem, clearcnt = 0;

	if (!osfopen)  return;
	smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
nchk:
	if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	smpsrem -= n;
	outbufrem -= n;
	if (clearcnt < O.outbufsamps) {
		clearcnt += n;          /* clear buf only till clean */
		do *floutbufp++ = 0.0f;
		while (--n);
	}
	else floutbufp += n;
	if (!outbufrem) {
		audtran(outbuf,outbufsiz);
		floutbufp = (float *) outbuf;
		outbufrem = O.outbufsamps;
		if (smpsrem) goto nchk;
	}
}

static void clrspin1(MYFLT *r, int spinrem) /* clear remainder of spinbuf
																						to zeros */
{                                           /* called only once, at EOF   */
	infilend = 1;                        /* 1st filend pass:   */
	while (spinrem--)                    /*   clear spin rem   */
		*r++ = FL(0.0);
}

static void clrspin2(void)              /* clear spinbuf to zeros   */
{                                       /* called only once, at EOF */
	MYFLT *r = spin;
	int n = nspin;
	infilend = 2;                            /* at 2nd filend pass  */
	do *r++ = FL(0.0);                       /*   clr whole spinbuf */
	while (--n);
	printf(Str(X_713,"end of audio_in file\n"));
}

static void byterecv(void)                    /* J. Mohr  1995 Oct 17 */
/* get spin values from byte inbuf */
{
	MYFLT *r = spin;
	int   n, spinrem = nspin;

	if (infilend == 2) return;
	if (!inbufrem)  goto echk;
nchk:
	if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
		n = inbufrem;               /*       prepare to get in parts  */
	spinrem -= n;
	inbufrem -= n;
	do *r++ = (MYFLT) ( (short)((*(unsigned char*)chinbufp++)^0x80) << 8 );
	while (--n);
	if (!inbufrem) {
echk:
		if (!infilend) {
			if ((n = audrecv(inbuf, inbufsiz)) != 0) {
				chinbufp = inbuf;
				inbufrem = n / sizeof(char);
				if (spinrem) goto nchk;
			}
			else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
		}
		else clrspin2();           /* 2nd filend pass: zero the spinbuf */
	}
}

static void charrecv(void)              /* get spin values from char inbuf */
{
	MYFLT *r = spin;
	int   n, spinrem = nspin;

	if (infilend == 2) return;
	if (!inbufrem)  goto echk;
nchk:
	if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
		n = inbufrem;               /*       prepare to get in parts  */
	spinrem -= n;
	inbufrem -= n;
	do *r++ = (MYFLT) ( (short)*(chinbufp++) << 8 );
	while (--n);
	if (!inbufrem) {
echk:
		if (!infilend) {
			if ((n = audrecv(inbuf, inbufsiz)) != 0) {
				chinbufp = inbuf;
				inbufrem = n / sizeof(char);
				if (spinrem) goto nchk;
			}
			else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
		}
		else clrspin2();           /* 2nd filend pass: zero the spinbuf */
	}
}

#ifdef never
static void alawrecv(void)
{
	die(Str(X_588,"alaw audio_in not yet implemented"));
}
#endif

#ifdef ULAW
static void ulawrecv(void)              /* get spin values from ulaw inbuf */
{
	MYFLT *r = spin;
	int   n, spinrem = nspin;

	if (infilend == 2) return;
	if (!inbufrem)  goto echk;
nchk:
	if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
		n = inbufrem;               /*       prepare to get in parts  */
	spinrem -= n;
	inbufrem -= n;
	do *r++ = (MYFLT) ulaw_decode[*(unsigned char *)chinbufp++];
	while (--n);
	if (!inbufrem) {
echk:
		if (!infilend) {
			if ((n = audrecv(inbuf, inbufsiz)) != 0) {
				chinbufp = inbuf;
				inbufrem = n / sizeof(char);
				if (spinrem) goto nchk;
			}
			else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
		}
		else clrspin2();           /* 2nd filend pass: zero the spinbuf */
	}
}
#endif

static void shortrecv(void)      /* get spin values from short_int inbuf */
{
	MYFLT *r = spin;
	int   n, spinrem = nspin;

	if (infilend == 2) return;
	if (!inbufrem)  goto echk;
nchk:
	if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
		n = inbufrem;               /*       prepare to get in parts  */
	spinrem -= n;
	inbufrem -= n;
	do {
		*r++ = short_to_dbfs * (MYFLT) *shinbufp++;
	} while (--n);
	if (!inbufrem) {
echk:
		if (!infilend) {
			if ((n = audrecv(inbuf, inbufsiz)) != 0) {
				shinbufp = (short *) inbuf;
				inbufrem = n / sizeof(short);
				if (spinrem) goto nchk;
			}
			else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
		}
		else clrspin2();           /* 2nd filend pass: zero the spinbuf */
	}
}

static void longrecv(void)              /* get spin values from long_int inbuf */
{
	MYFLT *r = spin;
	int   n, spinrem = nspin;

	if (infilend == 2) return;
	if (!inbufrem)  goto echk;
nchk:
	if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
		n = inbufrem;               /*       prepare to get in parts  */
	spinrem -= n;
	inbufrem -= n;
	do {
		*r++ = long_to_dbfs * (MYFLT) *llinbufp++;
	}
	while (--n);
	if (!inbufrem) {
echk:
		if (!infilend) {
			if ((n = audrecv(inbuf, inbufsiz)) != 0) {
				llinbufp = (long *) inbuf;
				inbufrem = n / sizeof(long);
				if (spinrem) goto nchk;
			}
			else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
		}
		else clrspin2();           /* 2nd filend pass: zero the spinbuf */
	}
}

static void floatrecv(void)              /* get spin values from float inbuf */
{
	MYFLT *r = spin;
	int   n, spinrem = nspin;

	if (infilend == 2) return;
	if (!inbufrem)  goto echk;
nchk:
	if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
		n = inbufrem;                      /*       prepare to get in parts  */
	spinrem -= n;
	inbufrem -= n;
	do {
		*r++ = *flinbufp++ * float_to_dbfs;
	} while (--n);
	if (!inbufrem) {
echk:
		if (!infilend) {
			if ((n = audrecv(inbuf, inbufsiz)) != 0) {
				flinbufp = (float*) inbuf;
				inbufrem = n / sizeof(float);
				if (spinrem) goto nchk;
			}
			else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
		}
		else clrspin2();           /* 2nd filend pass: zero the spinbuf */
	}
}

static void int24precv(void)     /* get spin values from 24bit packed inbuf */
{
	MYFLT *r = spin;
	SAMP24 s24;
	int   n, spinrem = nspin;

	if (infilend == 2)
		return;
	if (!inbufrem)
		goto echk;
nchk:
	if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
		n = inbufrem;               /*       prepare to get in parts  */
	spinrem -= n;
	inbufrem -= n;
	s24.lsamp = 0L;
	do {
		s24.bytes[1] = *int24inbufp++;
		s24.bytes[2] = *int24inbufp++;
		s24.bytes[3] = *int24inbufp++;
		*r++ = (MYFLT) (s24.lsamp * INLONGFAC);
	}
	while (--n);
	if (!inbufrem) {
echk:
		if (!infilend) {
			if ((n = audrecv(inbuf, inbufsiz)) != 0) {
				int24inbufp = inbuf;
				inbufrem = n / 3;
				if (spinrem)
					goto nchk;
			}
			else
				clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
		}
		else
			clrspin2();           /* 2nd filend pass: zero the spinbuf */
	}
}

/* --------------------------- IV (Nov 05 2001) ---------------------------> */
static void floatran_noscl(void)         /* send float spout vals to outbuf */
{                                       /*      write buffer when full      */
	MYFLT  *sp, *maxampp;
	unsigned long       *maxps;
	int    n, spoutrem;
	MYFLT  absamp;
	float       fltsmp;

	sp = spout;                     /* adr spout    */
	spoutrem = nspout;              /* smps to go   */
	maxampp = maxamp;
	maxps = maxpos;
nchk:
	if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
		n = outbufrem;          /*      prepare to send in parts  */
	spoutrem -= n;
	outbufrem -= n;
	do {
		if ((absamp = *sp) < 0.0f)
			absamp = -absamp;
		if (absamp > *maxampp) {         /*  maxamp this seg  */
			*maxampp = absamp;
			*maxps = nframes;
		}
		fltsmp = (float) *sp;
		if (osfopen)
			*floutbufp++ = fltsmp;
		if (multichan) {
			maxps++;
			if (++maxampp >= maxampend)
				maxampp = maxamp, maxps = maxpos, nframes++;
		}
		else nframes++;
		sp++;
	} while (--n);
	if (!outbufrem) {
		if (osfopen) {
			audtran(outbuf,outbufsiz);
			floutbufp = (float *) outbuf;
		}
		outbufrem = O.outbufsamps;
		if (spoutrem) goto nchk;
	}
}

static void floatrecv_noscl(void)        /* get spin values from float inbuf */
{                                                       /* IV - Jul 11 2002 */
	MYFLT *r = spin;
	int   n, spinrem = nspin;

	if (infilend == 2) return;
	if (!inbufrem)  goto echk;
nchk:
	if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
		n = inbufrem;                      /*       prepare to get in parts  */
	spinrem -= n;
	inbufrem -= n;
	do {
		*r++ = (MYFLT) *flinbufp++;
	} while (--n);
	if (!inbufrem) {
echk:
		if (!infilend) {
			if ((n = audrecv(inbuf, inbufsiz)) != 0) {
				flinbufp = (float*) inbuf;
				inbufrem = n / sizeof(float);
				if (spinrem) goto nchk;
			}
			else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
		}
		else clrspin2();           /* 2nd filend pass: zero the spinbuf */
	}
}

/* <-------------------------- IV (Nov 05 2001) ---------------------------- */

