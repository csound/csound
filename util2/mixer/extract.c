/*  
extract.c

Copyright (C) 1995 John ffitch

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
*   extract.c                                           *
*   extract a section of a sound file                   *
*   jpff 23 Sep 1994                                    *
*   including lifting from Csound itself                *
\*******************************************************/

/* Notes:
*     This makes a mess of multichannel inputs.
*     Needs to take much more care
*/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "cs.h"
#include "ustub.h"
#include "soundio.h"

/* Constants */
#define NUMBER_OF_SAMPLES	(4096)
#define SHORTMAX 		(32767)
#define FIND(MSG)   if (*s == '\0')  \
	if (!(--argc) || (s = *++argv) && *s == '-') \
	die(MSG);


long        sample;         /* Time file starts in samples */
long	    stop;           /* Time file ends in samples */
long	    numsamps;       /* Length in samples */
float       time;           /* Time file starts in secs */
float       endtime;        /* Time file ends in secs */
float	    dur;	    /* Length in secs */
int	    outputs;	    /* Number of out chanels */

SOUNDIN *   p;              /* Csound structure */

int debug   = 0;

/* Static function prototypes */

static int  EXsndgetset(char *);
static void ExtractSound(int, int);

/* Externs */
extern long getsndin(int, float *, long, SOUNDIN *);
extern void rewriteheader(int,long);
extern int  openout(char *, int), sfsampsize(int);
extern void writeheader(int, char *);
extern char *getstrformat(int);
extern int  sndgetset(SOUNDIN *);

static void usage(char *mesg)
{
	fprintf(stderr, "%s\n", mesg);
	fprintf(stderr,Str("Usage:\textracter [-flags] soundfile\n"));
	fprintf(stderr,Str("Legal flags are:\n"));
	fprintf(stderr,Str("-o fnam\tsound output filename\n"));
	fprintf(stderr,Str("-N\t\tnotify (ring the bell) when done\n"));
	fprintf(stderr,Str("-S integer\tsample number at which to start file\n"));
	fprintf(stderr,Str("-Z integer\tsample number at which to end file\n"));
	fprintf(stderr,Str("-Q integer\tnumber of samples to read\n"));
	fprintf(stderr,Str("-T fpnum\ttime in secs at which to start file\n"));
	fprintf(stderr,Str("-E fpnum\ttime in secs at which to end file\n"));
	fprintf(stderr,Str("-D fpnum\tduration in secs of extract\n"));
	fprintf(stderr,Str("-R\tRewrite header\n"));
	fprintf(stderr,Str("-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write\n"));
	fprintf(stderr,Str("-v\t\tverbose mode for debugging\n"));
	fprintf(stderr,Str("flag defaults: extracter -otest -S 0\n"));
	exit(1);
}

int
main(int argc, char **argv)
{
	char        *inputfile = NULL;
	int         infd, outfd;
	char        c, *s, *filnamp;

	init_getstring(argc, argv);
	/*     response_expand(&argc, &argv); /\* Permits "@xxx" response files *\/ */
	/* Check arguments */
	O.filnamspace = filnamp = mmalloc(csound, (long)1024);
	sample = -1; time = -1.0;
	stop  = -1; endtime = -1.0;
	numsamps = -1; dur = -1.0;
	if (!(--argc))
		usage(Str("Insufficient arguments"));
	do {
		s = *++argv;
		if (*s++ == '-')                      /* read all flags:  */
			while ((c = *s++) != '\0')
				switch(c) {
								case 'j':
									FIND("")
										while (*++s);
									break;
								case 'o':
									FIND(Str("no outfilename"))
										O.outfilename = filnamp;            /* soundout name */
									while ((*filnamp++ = *s++)); s--;
									if (strcmp(O.outfilename,"stdin") == 0)
										die(Str("-o cannot be stdin"));
									if (strcmp(O.outfilename,"stdout") == 0) {
#ifdef THINK_C
										die(Str("stdout audio not supported"));
#else
										if ((O.stdoutfd = dup(1)) < 0) /* redefine stdout */
											die(Str("too many open files"));
										dup2(2,1);                /* & send 1's to stderr */
#endif
									}
									break;
								case 'S':
									FIND(Str("no start sample"));
									sample = atoi(s);
									while (*++s);
									if (time >= 0.0) {
										warning(Str("-S overriding -T"));
										time = -1.0;
									}
									break;
								case 'T':
									FIND(Str("no start time"));
									time = (float) atof(s);
									while (*++s);
									if (sample >= 0) {
										warning(Str("-T overriding -S"));
										sample = -1;
									}
									break;
								case 'Z':	/* Last sample */
									FIND(Str("no end sample"));
									stop = atoi(s);
									while (*++s);
									if (endtime >= 0.0) {
										warning(Str("-Z overriding -E"));
										endtime = -1.0;
									}
									if (dur >= 0.0) {
										warning(Str("-Z overriding -D"));
										dur = 0.1;
									}
									if (numsamps >=0) {
										warning(Str("-Z overriding -Q"));
										numsamps = -1;
									}
									break;
								case 'E':	/* Last time */
									FIND(Str("no end time"));
									endtime = (float) atof(s);
									while (*++s);
									if (dur >= 0.0) {
										warning(Str("-E overriding -D"));
										dur = 0.1;
									}
									if (numsamps >=0) {
										warning(Str("-E overriding -Q"));
										numsamps = -1;
									}
									if (stop >= 0) {
										warning(Str("-E overriding -Z"));
										stop = -1;
									}
									break;
								case 'D':
									FIND(Str("no duration"));
									dur = (float) atof(s);
									while (*++s);
									if (endtime >= 0.0) {
										warning(Str("-D overriding -E"));
										endtime = -1.0;
									}
									if (numsamps >=0) {
										warning(Str("-D overriding -Q"));
										numsamps = -1;
									}
									if (stop >= 0) {
										warning(Str("-D overriding -Z"));
										stop = -1;
									}
									break;
								case 'Q':
									FIND(Str("no sample count"));
									numsamps = atoi(s);
									while (*++s);
									if (endtime >= 0.0) {
										warning(Str("-Q overriding -E"));
										endtime = -1.0;
									}
									if (dur >= 0.0) {
										warning(Str("-Q overriding -D"));
										dur = 0.1;
									}
									if (stop >= 0) {
										warning(Str("-Q overriding -Z"));
										stop = -1;
									}
									break;
								case 'H':
									if (isdigit(*s)) {
										int n;
										sscanf(s, "%d%n", &O.heartbeat, &n);
										s += n;
									}
									else O.heartbeat = 1;
									break;
								case 'R':
									O.rewrt_hdr = 1;
									break;
								case 'N':
									O.ringbell = 1;             /* notify on completion */
									break;
								case 'v':                       /* Verbose mode */
									debug = 1;
									break;
								default:
									sprintf(errmsg,Str("unknown flag -%c"), c);
									usage(errmsg);
				}
		else {
			if (inputfile != NULL) usage(Str("Too many input args"));
			inputfile = --s;
		}
	} while (--argc);
	dbfs_init(DFLT_DBFS);
	/* Read sound file */
	if (inputfile == NULL) usage(Str("No input"));

	if (!(infd = EXsndgetset(inputfile))) {
		fprintf(stderr,Str("%s: error while opening %s"),
			argv[0], inputfile);
		exit(1);
	}

	if (debug) {
		fprintf(stderr, Str("Times %f %f %f\nNums %ld %ld %ld\n"),
			time, endtime, dur, sample, stop, numsamps);
	}
	if (time >= 0.0) sample = time*p->sr;
	if (endtime >= 0.0) numsamps = endtime*p->sr - sample;
	else if (dur >= 0.0) numsamps = dur*p->sr;
	else if (stop >= 0) numsamps = stop - sample;
	else if (numsamps < 0) numsamps = p->getframes - sample;

	if (sample<0) sample = 0;
	fprintf(stderr,
		Str("Extracting from sample %ld for %ld samples (%.5f secs)\n"),
		sample, numsamps, (float)numsamps/p->sr);

	outputs = p->nchnls;

	O.outformat = p->format; /* Copy from input file */
	O.sfsampsize = sfsampsize(O.outformat);
	O.filetyp = p->filetyp; /* Copy from input file */
	O.sfheader = 1;
	if (O.outfilename == NULL)  O.outfilename = "test";
	outfd = openout(O.outfilename, 1);
	esr = p->sr;
	nchnls = outputs;
	if (O.sfheader)     
		writeheader(outfd, O.outfilename);      /* write header as required   */

	ExtractSound(infd, outfd);
	close(outfd);
	if (O.ringbell) putc(7, stderr);
	return 0;

outtyp:
	usage(Str("output soundfile cannot be both AIFF and WAV"));

}

static int
EXsndgetset(char *name)
{
	int          infd;
	float        dur;
	static  ARGOFFS  argoffs = {0};     /* these for sndgetset */
	static  OPTXT    optxt;
	static  float    fzero = 0.0;
	char         quotname[80];

	sssfinit();                 /* stand-alone init of SFDIR etc. */
	esr = 0.0;                  /* set esr 0. with no orchestra   */
	optxt.t.outoffs = &argoffs; /* point to dummy OUTOCOUNT       */
	p = (SOUNDIN *) mcalloc(csound, (long)sizeof(SOUNDIN));
	p->channel = ALLCHNLS;
	p->h.optext = &optxt;
	p->ifilno = &sstrcod;
	p->iskptim = &fzero;
	p->iformat = &fzero;
	sprintf(quotname,"%c%s%c",'"',name,'"');
	p->STRARG = quotname;
	if ((infd = sndgetset(p)) == 0)            /* open sndfil, do skiptime */
		return(0);
	p->getframes = p->framesrem;
	dur = (float) p->getframes / p->sr;
	printf(Str("extracting from %ld sample frames (%3.1f secs)\n"),
		p->getframes, dur);
	return(infd);
}

static void 
ExtractSound(int infd, int outfd)
{
	char  buffer[4*NUMBER_OF_SAMPLES];
	long  read_in;
	long  sample = 0;
	int   i, j;
	long  bytes = 0;
	int   block = 0;
	int   size;
	int   this_block;

	lseek(infd, outputs*sample*O.sfsampsize, SEEK_CUR);
	while (numsamps>0) {
		int num = NUMBER_OF_SAMPLES;
		if (numsamps<num) num = numsamps;
		numsamps -= num;
		num *= O.sfsampsize*outputs;
		read_in = read(infd, buffer, num);
		write(outfd, buffer, read_in);
		block++;
		bytes += read_in;
		if (O.rewrt_hdr) {
			rewriteheader(outfd, bytes);
			lseek(outfd, 0L, SEEK_END); /* Place at end again */
		}
		if (O.heartbeat) {
			if (O.heartbeat==1) {
#ifdef SYMANTEC
				nextcurs();
#elif __BEOS__
				putc('.', stderr); fflush(stderr);
#else
				putc("|/-\\"[nrecs&3], stderr); putc(8,stderr);
#endif
			}
			else if (O.heartbeat==2) putc('.', stderr);
			else if (O.heartbeat==3) {
				int n;
				err_printf( "%d(%.3f)%n", nrecs, nrecs/ekr, &n);
				while (n--) putc(8, stderr);
			}
			else putc(7, stderr);
		}
		if (read_in < num) break;
	}
	rewriteheader(outfd, bytes);
	return;
}


