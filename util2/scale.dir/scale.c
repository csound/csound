/*  
XXX code for 

Copyright (C) 1994  John ffitch

This file is part of Csound.

Csound is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Csound is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Csound; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*******************************************************\
*   scale.c						*
*   scale a sound file by a float factor		*
*   jpff 3 Sep 1994 after code by dpwe 19sep90		*
*   and a certain amount of lifting from Csound itself  *
\*******************************************************/

#include "ustub.h"
#include "soundio.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef LINUX
#include <unistd.h>
#endif

#define SHORTMAX 32767
#define FIND(MSG)   if (*s == '\0')  \
	if (!(--argc) || ((s = *++argv) && *s == '-')) \
	die(MSG);

/* Static function prototypes */

static void  InitScaleTable(double, char *);
static int   SCsndgetset(char *);
static void  ScaleSound(int, int);
static float FindAndReportMax(int);

/* Externs */
extern long getsndin(int, float *, long, SOUNDIN *);
extern void bytrev2(char *, int), bytrev4(char *, int), rewriteheader(int,long);
extern int  openout(char *, int), bytrevhost(void), sfsampsize(int);
extern int  sndgetset(SOUNDIN *);
extern void writeheader(int, char*);
extern char *getstrformat(int);

/* Static global variables */
static SOUNDIN     *p;  /* space allocated by SAsndgetset() */
static unsigned    outbufsiz;
static void	   *outbuf;
static	char	   *choutbuf;		    /* char  pntr to above  */
static	short	   *shoutbuf;		    /* short pntr	    */
static	long	   *lloutbuf;		    /* long  pntr	    */
static	float	   *floutbuf;		    /* float pntr	    */
static  int	   outrange = 0; 	    /* Count samples out of range */
void       err_printf(char*, ...);

static void scale_usage(char *mesg)
{
	err_printf( "%s\n", mesg);
	err_printf(Str(X_19,"Usage:\tscale [-flags] soundfile\n"));
	err_printf(Str(X_9,"Legal flags are:\n"));
	err_printf(Str(X_157,"-o fnam\tsound output filename\n"));
	err_printf(Str(X_96,"-A\tcreate an AIFF format output soundfile\n"));
	err_printf(Str(X_132,"-W\tcreate a WAV format output soundfile\n"));
	err_printf(Str(X_149,"-h\tno header on output soundfile\n"));
	err_printf(Str(X_141,"-c\t8-bit signed_char sound samples\n"));
	err_printf(Str(X_136,"-a\talaw sound samples\n"));
#ifdef ULAW
	err_printf(Str(X_166,"-u\tulaw sound samples\n"));
#endif
	err_printf(Str(X_164,"-s\tshort_int sound samples\n"));
	err_printf(Str(X_153,"-l\tlong_int sound samples\n"));
	err_printf(Str(X_145,"-f\tfloat sound samples\n"));
	err_printf(Str(X_11,"-F fpnum\tamount to scale amplitude\n"));
	err_printf(Str(X_24,
		"-F file \tfile of scaling information (alternative)\n"));
	err_printf(Str(X_427,
		"-M fpnum\tScale file to given maximum\n"));
	err_printf(Str(X_1484,
		"-P fpnum\tscale file to given percentage of full\n"));
	err_printf(
		Str(X_125,
		"-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)\n"));
	err_printf(Str(X_108,
		"-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write\n"));
	err_printf(Str(X_120,
		"-N\tnotify (ring the bell) when score or miditrack is done\n"));
	err_printf(Str(X_90,"-- fnam\tlog output to file\n"));
	err_printf(Str(X_39,"flag defaults: scale -s -otest -F 0.0\n"));
	err_printf(Str(X_41,
		"If scale is 0.0 then reports maximum possible scaling\n"));
	exit(1);
}

int
main(int argc, char **argv)
{
	char	*inputfile = NULL;
	double	factor = 0.0;
	double	maximum = 0.0;
	char        *factorfile = NULL;
	int		infd, outfd;
	char 	outformch = 's', c, *s, *filnamp;
	char	*envoutyp;
	OPARMS	OO;

	e0dbfs = DFLT_DBFS;
	init_getstring(argc, argv);
	memset(&OO, 0, sizeof(OO));
	/* Check arguments */
	{
		char *getenv();
		if ((envoutyp = getenv("SFOUTYP")) != NULL) {
			if (strcmp(envoutyp,"AIFF") == 0)
				OO.filetyp = TYP_AIFF;
			else if (strcmp(envoutyp,"WAV") == 0)
				OO.filetyp = TYP_WAV;
			else {
				err_printf(Str(X_61,"%s not a recognized SFOUTYP env setting"),
					envoutyp);
				exit(1);
			}
		}
	}
	O.filnamspace = filnamp = mmalloc((long)1024);
	if (!(--argc))
		scale_usage(Str(X_939,"Insufficient arguments"));
	do {
		s = *++argv;
		if (*s++ == '-')    		      /* read all flags:  */
			while ((c = *s++) != '\0')
				switch(c) {
					case 'o':
						FIND(Str(X_1052,"no outfilename"))
							O.outfilename = filnamp;		/* soundout name */
						while ((*filnamp++ = *s++)); s--;
						if (strcmp(O.outfilename,"stdin") == 0)
							die("-o cannot be stdin");
						if (strcmp(O.outfilename,"stdout") == 0) {
#ifdef THINK_C
							die(Str(X_1244,"stdout audio not supported"));
#else
							if ((O.stdoutfd = dup(1)) < 0) /* redefine stdout */
								die(Str(X_1290,"too many open files"));
							dup2(2,1);                /* & send 1's to stderr */
#endif
						}
						break;
					case 'A':
						if (OO.filetyp == TYP_WAV) {
							if (envoutyp == NULL) goto outtyp;
							if (O.msglevel & WARNMSG)
								printf(Str(X_95,"-A overriding local default WAV out"));
						}
						OO.filetyp = TYP_AIFF;     /* AIFF output request  */
						break;
					case 'W':
						if (OO.filetyp == TYP_AIFF) {
							if (envoutyp == NULL) goto outtyp;
							if (O.msglevel & WARNMSG)
								printf(Str(X_131,"-W overriding local default AIFF out"));
						}
						OO.filetyp = TYP_WAV;      /* WAV output request  */
						break;
					case 'F':
						FIND(Str(X_42,"no scale factor"));
						if (isdigit(*s) || *s == '-' || *s == '+')
							factor = atof(s);
						else
							factorfile = s;
						while (*++s);
						break;
					case 'M':
						FIND(Str(X_426,"No maximum"));
						maximum = atof(s);
						while (*++s);
						break;
					case 'P':       /* Percentage */
						FIND(Str(X_426,"No maximum"));
						maximum = atof(s);
						if (OO.outformat == AE_FLOAT) maximum = maximum*0.01;
						else maximum = 327.67*maximum;
						while (*++s);
						break;
					case 'h':
						OO.sfheader = 0;           /* skip sfheader  */
						break;
					case 'c':
						if (OO.outformat) goto outform;
						outformch = c;
						OO.outformat = AE_CHAR;	/* 8-bit char soundfile */
						break;
#ifdef never
					case 'a':
						if (OO.outformat) goto outform;
						outformch = c;
						OO.outformat = AE_ALAW;	/* a-law soundfile */
						break;
#endif
#ifdef ULAW
					case 'u':
						if (OO.outformat) goto outform;
						outformch = c;
						OO.outformat = AE_ULAW;	/* mu-law soundfile */
						break;
#endif
					case 's':
						if (OO.outformat) goto outform;
						outformch = c;
						OO.outformat = AE_SHORT;	/* short_int soundfile */
						break;
					case 'l':
						if (OO.outformat) goto outform;
						outformch = c;
						OO.outformat = AE_LONG;	/* long_int soundfile */
						break;
					case 'f':
						if (OO.outformat) goto outform;
						outformch = c;
						OO.outformat = AE_FLOAT;	/* float soundfile */
						break;
					case 'j':
						FIND("");
						while (*s++); s--;
						break;
					case 'R':
						O.rewrt_hdr = 1;
						break;
					case 'H':
						if (isdigit(*s)) {
							int n;
							sscanf(s, "%d%n", &O.heartbeat, &n);
							s += n;
						}
						else O.heartbeat = 1;
						break;
					case 'N':
						O.ringbell = 1;         	/* notify on completion */
						break;
					default:
						sprintf(errmsg,Str(X_1334,"unknown flag -%c"), c);
						scale_usage(errmsg);
				}
		else if (inputfile == NULL) {
			inputfile = --s;
		}
		else scale_usage(Str(X_1286,"too many arguments"));
	} while (--argc);
	dbfs_init(DFLT_DBFS);
retry:
	/* Read sound file */
	if (!(infd = SCsndgetset(inputfile))) {
		err_printf(Str(X_76,"%s: error while opening %s"), argv[0], inputfile);
		exit(1);
	}
	if (factor != 0.0 || factorfile != NULL) {		/* perform scaling */
		if (OO.outformat)                       /* if no audioformat yet  */
			O.outformat = OO.outformat;
		else O.outformat = p->format; /* Copy from input file */
		O.sfsampsize = sfsampsize(O.outformat);
		if (OO.filetyp)
			O.filetyp = OO.filetyp;
		else O.outformat = O.informat; /* Copy from input file */
		if (OO.sfheader)
			O.sfheader = OO.sfheader;
		else O.sfheader = 1;
		if (O.filetyp == TYP_AIFF) {
			if (!O.sfheader)
				die(Str(X_629,"can't write AIFF/WAV soundfile with no header"));
			if (
#ifdef never
				O.outformat == AE_ALAW ||
#endif
#ifdef ULAW
				O.outformat == AE_ULAW ||
#endif
				O.outformat == AE_FLOAT) {
					sprintf(errmsg,Str(X_180,"AIFF does not support %s encoding"),
						getstrformat(O.outformat));
					die(errmsg);
				}
		}
		if (O.filetyp == TYP_WAV) {
			if (!O.sfheader)
				die(Str(X_629,"can't write AIFF/WAV soundfile with no header"));
			if (
#ifdef never
				O.outformat == AE_ALAW ||
#endif
#ifdef ULAW
				O.outformat == AE_ULAW ||
#endif
				O.outformat == AE_FLOAT) {
					sprintf(errmsg,Str(X_181,"WAV does not support %s encoding"),
						getstrformat(O.outformat));
					die(errmsg);
				}
		}
		if (OO.filetyp)
			O.filetyp = OO.filetyp;
		if (O.rewrt_hdr && !O.sfheader)
			die(Str(X_628,"can't rewrite header if no header requested"));
		if (O.outfilename == NULL)  O.outfilename = "test";
		outfd = openout(O.outfilename, 1);
		esr = p->sr;
		nchnls = p->nchanls;
		if (O.sfheader)
			writeheader(outfd, O.outfilename);	/* write header as required     */
		if ((O.filetyp == TYP_AIFF && bytrevhost()) ||
			(O.filetyp == TYP_WAV && !bytrevhost())) {
				if (O.outformat == AE_SHORT)        /* if audio out needs byte rev  */
					audtran = bytrev2;           /*   redirect the audio puts    */
				else if (O.outformat == AE_LONG)
					audtran = bytrev4;
				else audtran = nullfn;
			}
		else audtran = nullfn; 		/* else use standard audio puts */
		outbufsiz = 1024 * O.sfsampsize;/* calc outbuf size  */
		outbuf = mmalloc((long)outbufsiz);                 /*  & alloc bufspace */
		printf(Str(X_1382,"writing %d-byte blks of %s to %s %s\n"),
			outbufsiz, getstrformat(O.outformat), O.outfilename,
			O.filetyp == TYP_AIFF ? "(AIFF)" :
		O.filetyp == TYP_WAV ? "(WAV)" : " ");
		switch(O.outformat) {
			case AE_CHAR:  spoutran = chartran;
				choutbuf = outbuf;
				break;
#ifdef never
			case AE_ALAW:  spoutran = alawtran;
				choutbuf = outbuf;
				break;
#endif
#ifdef ULAW
			case AE_ULAW:  spoutran = ulawtran;
				choutbuf = outbuf;
				break;
#endif
			case AE_SHORT: spoutran = shortran;
				shoutbuf = (short *)outbuf;
				break;
			case AE_LONG:  spoutran = longtran;
				lloutbuf = (long  *)outbuf;
				break;
			case AE_FLOAT: spoutran = floatran;
				floutbuf = (float *)outbuf;
				break;
			default:
				err_printf( "Type is %x\n", O.outformat);
				die(Str(X_1329,"unknown audio_out format"));
		}
		InitScaleTable(factor, factorfile);
		ScaleSound(infd, outfd);
		close(outfd);
	}
	else if (maximum!=0.0) {
		float mm = FindAndReportMax(infd);
		factor = maximum / mm;
		close(infd);
		goto retry;

	}
	else
		FindAndReportMax(infd);
	if (O.ringbell) putc(7, stderr);
	return 0;

outtyp:
	scale_usage(Str(X_1113,"output soundfile cannot be both AIFF and WAV"));

outform:
	sprintf(errmsg,Str(X_1198,"sound output format cannot be both -%c and -%c"),
		outformch, c);
	scale_usage(errmsg);
	exit(1);
}

static double ff = 0.0;
static int table_used = 0;
typedef struct scalepoint {
	double y0;
	double y1;
	double yr;
	int x0;
	int x1;
	struct scalepoint *next;
} scalepoint;
scalepoint scale_table = {0.0, 0.0, 0.0, 0, 0, NULL};
scalepoint *end_table = &scale_table;

static void
InitScaleTable(double factor, char *factorfile)
{
	if (factor != 0.0) ff = factor;
	else {
		FILE *f = fopen(factorfile, "r");
		double samplepert = (double)p->sr;
		double x, y;
		while (fscanf(f, "%lf %lf\n", &x, &y) == 2) {
			scalepoint *newpoint = (scalepoint*) malloc(sizeof(scalepoint));
			if (newpoint == NULL) {
				err_printf( "Insufficient memory\n");
				exit(1);
			}
			end_table->next = newpoint;
			newpoint->x0 = end_table->x1;
			newpoint->y0 = end_table->y1;
			newpoint->x1 = (int) (x*samplepert);
			newpoint->y1 = y;
			newpoint->yr =
				(x == newpoint->x0 ?
				y - newpoint->y0 :
			(y - newpoint->y0)/((double)(newpoint->x1 - newpoint->x0)));
			newpoint->next = NULL;
			end_table = newpoint;
		}
		{
			scalepoint *newpoint = (scalepoint*) malloc(sizeof(scalepoint));
			if (newpoint == NULL) {
				err_printf( "Insufficient memory\n");
				exit(1);
			}
			end_table->next = newpoint;
			newpoint->x0 = end_table->x1;
			newpoint->y0 = end_table->y1;
			newpoint->x1 = 0x7fffffff;
			newpoint->y1 = 0.0;
			newpoint->next = NULL;
			newpoint->yr = (x == newpoint->x0 ?
				-newpoint->y0 :
			-newpoint->y0/((double)(0x7fffffff-newpoint->x0)));
		}
		end_table = &scale_table;
		/* 	{ */
		/* 	    scalepoint *tt = &scale_table; */
		/* 	    err_printf( "Scale table is\n"); */
		/* 	    while (tt != NULL) { */
		/* 		err_printf( "(%d %f) -> %d %f [%f]\n", */
		/* 			tt->x0, tt->y0, tt->x1, tt->y1, tt->yr); */
		/* 		tt = tt->next; */
		/* 	    } */
		/* 	    err_printf( "END of Table\n"); */
		/* 	} */
		table_used = 1;
	}
}

static double
gain(int i)
{
	if (!table_used) return ff;
	while (i<end_table->x0 || i>end_table->x1){/* Get correct segment */
		/* 	err_printf( "Next table: %d (%d %f) -> %d %f [%f]\n", */
		/* 	      i, end_table->x0, end_table->y0, end_table->x1, end_table->y1, */
		/* 	      end_table->yr); */
		end_table = end_table->next;
	}
	return end_table->y0 + end_table->yr * (double)(i - end_table->x0);
}



static int
SCsndgetset(char *inputfile)
{
	int          infd;
	float        dur;
	static  ARGOFFS  argoffs = {0};     /* these for sndgetset */
	static	OPTXT    optxt;
	static  float    fzero = 0.0;
	char         quotname[80];
	static MYFLT sstrcod = SSTRCOD;

	sssfinit();                 /* stand-alone init of SFDIR etc. */
	esr = 0.0;                  /* set esr 0. with no orchestra   */
	optxt.t.outoffs = &argoffs; /* point to dummy OUTOCOUNT       */
	p = (SOUNDIN *) mcalloc((long)sizeof(SOUNDIN));
	p->channel = ALLCHNLS;
	p->h.optext = &optxt;
	p->ifilno = &sstrcod;
	p->iskptim = &fzero;
	p->iformat = &fzero;
	sprintf(quotname,"%c%s%c",'"',inputfile,'"');
	p->STRARG = quotname;
	if ((infd = sndgetset(p)) == 0)            /* open sndfil, do skiptime */
		return(0);
	p->getframes = p->framesrem;
	dur = (float) p->getframes / p->sr;
	printf("scaling %ld sample frames (%3.1f secs)\n",
		p->getframes, dur);
	return(infd);
}

#define BUFFER_LEN (1024)

static void
ScaleSound(int infd, int outfd)
{
	float buffer[BUFFER_LEN];
	long  read_in;
	float tpersample;
	float max, min;
	long  mxpos, minpos;
	int   maxtimes, mintimes;
	int   i, chans;
	long  bytes = 0;
	int	  block = 0;

	chans = p->nchanls;
	tpersample = 1.0/(float)p->sr;
	max = 0.0;	mxpos = 0; maxtimes = 0;
	min = 0.0;	minpos = 0; mintimes = 0;
	while ((read_in = getsndin(infd, buffer, BUFFER_LEN, p)) > 0) {
		for (i=0; i<read_in; i++) {
			buffer[i] = buffer[i] * gain(i+BUFFER_LEN*block);
			if (buffer[i] >= max) ++maxtimes;
			if (buffer[i] <= min) ++mintimes;
			if (buffer[i] > max)
				max = buffer[i], mxpos = i+BUFFER_LEN*block, maxtimes = 1;
			if (buffer[i] < min)
				min = buffer[i], minpos = i+BUFFER_LEN*block; mintimes = 1;
		}
		spoutran(buffer, read_in);
		audtran(outbuf, read_in*O.sfsampsize);
		write(outfd, outbuf, read_in*O.sfsampsize);
		block++;
		bytes += read_in*O.sfsampsize;
		if (O.rewrt_hdr) {
			rewriteheader(outfd, bytes);
			lseek(outfd, 0, 2);	/* Place at end again */
		}
		if (O.heartbeat) {
			putc("|/-\\"[block&3], stderr);
			putc('\b',stderr);
		}
	}
	rewriteheader(outfd, bytes);
	close(infd);
	printf("Max val %d at index %ld (time %.4f, chan %d) %d times\n",
		(int)max,mxpos,tpersample*(mxpos/chans),(int)mxpos%chans, maxtimes);
	printf("Min val %d at index %ld (time %.4f, chan %d) %d times\n",
		(int)min,minpos,tpersample*(minpos/chans),(int)minpos%chans, mintimes);
	if (outrange)
		printf("%d sample%sout of range\n",
		outrange, outrange==1 ? " " : "s ");
	else
		printf("Max scale factor = %.3f\n",
		(float)SHORTMAX/(float)((max>-min)?max:-min) );
	return;
}

static float
FindAndReportMax(int infd)
{
	int 	chans;
	float	tpersample;
	float	max, min;
	long	mxpos, minpos;
	int         maxtimes, mintimes;
	int		block = 0;
	float	buffer[BUFFER_LEN];
	long	read_in;
	int		i;

	chans = p->nchanls;
	tpersample = 1.0/(float)p->sr;
	max = 0.0;	mxpos = 0; maxtimes = 0;
	min = 0.0;	minpos = 0; mintimes = 0;
	while ((read_in = getsndin(infd, buffer, BUFFER_LEN, p)) > 0) {
		for (i=0; i<read_in; i++) {
			if (buffer[i] >= max) ++maxtimes;
			if (buffer[i] <= min) ++mintimes;
			if (buffer[i] > max)
				max = buffer[i], mxpos = i+BUFFER_LEN*block, maxtimes = 1;
			if (buffer[i] < min)
				min = buffer[i], minpos = i+BUFFER_LEN*block, mintimes = 1;
		}
		block++;
	}
	close(infd);
	printf("Max val %d at index %ld (time %.4f, chan %d) %d times\n",
		(int)max,mxpos,tpersample*(mxpos/chans),(int)mxpos%chans,maxtimes);
	printf("Min val %d at index %ld (time %.4f, chan %d) %d times\n",
		(int)min,minpos,tpersample*(minpos/chans),(int)minpos%chans,mintimes);
	printf("Max scale factor = %.3f\n",
		(float)SHORTMAX/(float)((max>-min)?max:-min) );
	return (max>-min ? max : -min);
}
