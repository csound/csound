/*
  envext.c:

  Copyright (C) 1994 John ffitch
 
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
*   envext.c						*
*   Extract an envelope file from a sound file		*
*   jpff 11 Dec 1994					*
*   mainly lifted from scale and Csound itself		*
\*******************************************************/

#include "cs.h"
#include "soundio.h"
#include <stdio.h>
#include <stdlib.h>

/* Constants */

#define SHORTMAX 32767.0
#define FIND(MSG)   if (*s == '\0')  \
	if (!(--argc) || ((s = *++argv) && *s == '-')) \
	die(MSG);

/* Static function prototypes */

static SNDFILE *SCsndgetset(char *);
static void FindEnvelope(SNDFILE *);

/* Externs */
extern long getsndin(SNDFILE *, MYFLT *, long, SOUNDIN *);
SNDFILE *sndgetset(SOUNDIN *p);

static char *outname = NULL;
static double window = 0.25;

/* Static global variables */
static SOUNDIN     *p;  /* space allocated by SAsndgetset() */

/*MYFLT e0dbfs = DFLT_DBFS;*/

static void envext_usage(char *mesg)
{
	fprintf(stderr, "%s\n", mesg);
	fprintf(stderr,"Usage:\tenvext [-flags] soundfile\n");
	fprintf(stderr, "Legal flags are:\n");
	fprintf(stderr,"-o fnam\tsound output filename\n");
	fprintf(stderr, "-w time\tSize of window\n");
	fprintf(stderr,"flag defaults: envext -onewenv\n");
	exit(1);
}

int
main(int argc, char **argv)
{
    char	*inputfile = NULL;
    SNDFILE     *infd;
    char 	c, *s;
    OPARMS	OO;

    peakchunks = 1;

    init_getstring(argc, argv);
    csoundPreCompile(csoundCreate(NULL));
    memset(&OO, 0, sizeof(OO));

    /* Check arguments */
    if (!(--argc))
      envext_usage("Insufficient arguments");
    do {
      s = *++argv;
      if (*s++ == '-')    		      /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND("no outfilename")
              outname = s;
            while (*++s);
            break;
          case 'w':
            FIND("No window size");
            window = atof(s);
            while (*++s);
            break;
          default:
            sprintf(errmsg,"unknown flag -%c", c);
            envext_usage(errmsg);
          }
      else if (inputfile == NULL) {
        inputfile = --s;
      }
      else envext_usage("too many arguments");
    } while (--argc);

    dbfs_init(DFLT_DBFS);
    /* Read sound file */
    if ((infd = SCsndgetset(inputfile))==NULL) {
      fprintf(stderr,"%s: error while opening %s", argv[0], inputfile);
      exit(1);
    }
    FindEnvelope(infd);
    return 0;
}

static SNDFILE *
SCsndgetset(char *inputfile)
{
    SNDFILE *infd;
    double          dur;
    static ARGOFFS  argoffs = {0};     /* these for sndgetset */
    static OPTXT    optxt;
    static MYFLT    fzero = 0.0;
    char         quotname[80];
    static MYFLT sstrcod = SSTRCOD;

    sssfinit();                 /* stand-alone init of SFDIR etc. */
    esr = 0.0;                  /* set esr 0. with no orchestra   */
    optxt.t.outoffs = &argoffs; /* point to dummy OUTOCOUNT       */
    p = (SOUNDIN *) mcalloc(&cenviron, (long)sizeof(SOUNDIN));
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
    dur = (double) p->getframes / p->sr;
    printf("enveloping %ld sample frames (%3.1f secs)\n", p->getframes, dur);
    return(infd);
}

static void
FindEnvelope(SNDFILE *infd)
{
    int 	chans;
    double	tpersample;
    double	max, min;
    long	mxpos, minpos;
    int		block = 0;
    MYFLT	*buffer;
    int		bufferlen;
    long	read_in;
    int		i;
    FILE *	outfile;
    
    outfile = fopen((outname == NULL ? "newenv" : outname), "w");
    bufferlen = (int)(window*(double)p->sr);
    buffer = (MYFLT*) malloc(bufferlen*sizeof(MYFLT));
    chans = p->nchanls;
    tpersample = 1.0/(double)p->sr;
    fprintf(outfile, "%.3f\t%.3f\n", 0.0, 0.0);
    while ((read_in = getsndin(infd, buffer, bufferlen, p)) > 0) {
      max = 0.0;	mxpos = 0;
      min = 0.0;	minpos = 0;
      for (i=0; i<read_in; i++) {
        if (buffer[i] > max)
          max = buffer[i], mxpos = i;
        if (buffer[i] < min)
          min = buffer[i], minpos = i;
      }
      if (-min > max) max = -min, mxpos = minpos;
      fprintf(outfile, "%.3f\t%.3f\n",
              block*window+(double)mxpos*tpersample, max/SHORTMAX);
      block++;
    }
    sf_close(infd);
    fclose(outfile);
}
