/*  
    scale.c:

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

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#include "cs.h"
#include "ustub.h"
#include "soundio.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Constants */
ENVIRON cenviron;

#define SHORTMAX 32767
#define FIND(MSG)   if (*s == '\0')  \
			if (!(--argc) || ((s = *++argv) && *s == '-')) \
			    die(MSG);

/* Static function prototypes */

static void  InitScaleTable(double, char *);
static SNDFILE *SCsndgetset(char *);
static void  ScaleSound(SNDFILE *, SNDFILE *);
static float FindAndReportMax(SNDFILE *);

/* Externs */
extern long getsndin(SNDFILE *, MYFLT *, long, SOUNDIN *);
extern int  openout(char *, int), getsizformat(int);
extern SNDFILE *sndgetset(SOUNDIN *);
extern void writeheader(int, char*);
extern char *getstrformat(int);
extern char* type2string(int);
extern short sfsampsize(int);
extern int type2sf(int);
#ifdef  USE_DOUBLE
#define sf_write_MYFLT  sf_write_double
#else
#define sf_write_MYFLT  sf_write_float
#endif

/* Static global variables */
static  SOUNDIN    *p;  /* space allocated by SAsndgetset() */
static  unsigned   outbufsiz;
static  MYFLT	   *outbuf;
static  int	   outrange = 0; 	    /* Count samples out of range */
        void       err_printf(char*, ...);
        OPARMS	   OO;

static void usage(char *mesg)
{
    err_printf( "%s\n", mesg);
    err_printf(Str("Usage:\tscale [-flags] soundfile\n"));
    err_printf(Str("Legal flags are:\n"));
    err_printf(Str("-o fnam\tsound output filename\n"));
    err_printf(Str("-A\tcreate an AIFF format output soundfile\n"));
    err_printf(Str("-W\tcreate a WAV format output soundfile\n"));
    err_printf(Str("-h\tno header on output soundfile\n"));
    err_printf(Str("-c\t8-bit signed_char sound samples\n"));
    err_printf(Str("-a\talaw sound samples\n"));
#ifdef ULAW
    err_printf(Str("-u\tulaw sound samples\n"));
#endif
    err_printf(Str("-s\tshort_int sound samples\n"));
    err_printf(Str("-l\tlong_int sound samples\n"));
    err_printf(Str("-f\tfloat sound samples\n"));
    err_printf(Str("-F fpnum\tamount to scale amplitude\n"));
    err_printf(Str(
                   "-F file \tfile of scaling information (alternative)\n"));
    err_printf(Str(
                   "-M fpnum\tScale file to given maximum\n"));
    err_printf(Str(
                   "-P fpnum\tscale file to given percentage of full\n"));
    err_printf(
      Str(
        "-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)\n"));
    err_printf(Str(
        "-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write\n"));
    err_printf(Str(
        "-N\tnotify (ring the bell) when score or miditrack is done\n"));
    err_printf(Str("-- fnam\tlog output to file\n"));
    err_printf(Str("flag defaults: scale -s -otest -F 0.0\n"));
    err_printf(Str(
        "If scale is 0.0 then reports maximum possible scaling\n"));
    exit(1);
}

char set_output_format(char c, char outformch)
{
    if (OO.outformat && (O.msglevel & WARNMSG)) {
      printf(Str("WARNING: Sound format -%c has been overruled by -%c\n"),
             outformch, c);
    }

    switch (c) {
    case 'a':
      OO.outformat = AE_ALAW;    /* a-law soundfile */
      break;

    case 'c':
      OO.outformat = AE_CHAR;    /* signed 8-bit soundfile */
      break;

    case '8':
      OO.outformat = AE_UNCH;    /* unsigned 8-bit soundfile */
      break;

    case 'f':
      OO.outformat = AE_FLOAT;   /* float soundfile */
      break;

    case 's':
      OO.outformat = AE_SHORT;   /* short_int soundfile*/
      break;

    case 'l':
      OO.outformat = AE_LONG;    /* long_int soundfile */
      break;

    case 'u':
      OO.outformat = AE_ULAW;    /* mu-law soundfile */
      break;

    case '3':
      OO.outformat = AE_24INT;   /* 24bit packed soundfile*/
      break;

    case 'e':
      OO.outformat = AE_FLOAT;   /* float soundfile (for rescaling) */
      break;

    default:
      return outformch; /* do nothing */
    };

  return c;
}

#ifndef POLL_EVENTS
int POLL_EVENTS(void)
{
    return (1);
}
#endif

void *memfiles = NULL;
void rlsmemfiles(void) {}
void pvsys_release(void) {}

int
main(int argc, char **argv)
{
    char	*inputfile = NULL;
    double	factor = 0.0;
    double	maximum = 0.0;
    char        *factorfile = NULL;
    SNDFILE     *infile = 0, *outfile;
    int         outfd;
    char 	outformch = 's', c, *s, *filnamp;
    char	*envoutyp;
    SF_INFO     sfinfo;

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
        else if (strcmp(envoutyp,"IRCAM") == 0)
          OO.filetyp = TYP_IRCAM;
        else {
          err_printf(Str("%s not a recognized SFOUTYP env setting"),
                     envoutyp);
          exit(1);
        }
      }
    }
    O.filnamspace = filnamp = mmalloc((long)1024);
    if (!(--argc))
      usage(Str("Insufficient arguments"));
    do {
      s = *++argv;
      if (*s++ == '-')    		      /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND(Str("no outfilename"))
            O.outfilename = filnamp;		/* soundout name */
            while ((*filnamp++ = *s++)); s--;
            if (strcmp(O.outfilename,"stdin") == 0)
              die("-o cannot be stdin");
            if (strcmp(O.outfilename,"stdout") == 0) {
#if defined mac_classic || defined SYMANTEC || defined BCC || defined __WATCOMC__ || defined WIN32
              die(Str("stdout audio not supported"));
#else
              if ((O.stdoutfd = dup(1)) < 0) /* redefine stdout */
                die(Str("too many open files"));
              dup2(2,1);                /* & send 1's to stderr */
#endif
            }
            break;
          case 'A':
            if (OO.filetyp == TYP_WAV) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                printf(Str("-A overriding local default WAV out"));
            }
            OO.filetyp = TYP_AIFF;     /* AIFF output request  */
            break;
          case 'J':
            if (OO.filetyp == TYP_AIFF ||
                OO.filetyp == TYP_WAV) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                printf(Str("WARNING: -J overriding local default AIFF/WAV out\n"));
            }
            OO.filetyp = TYP_IRCAM;      /* IRCAM output request */
            break;
          case 'W':
            if (OO.filetyp == TYP_AIFF) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                printf(Str("-W overriding local default AIFF out"));
            }
            OO.filetyp = TYP_WAV;      /* WAV output request  */
            break;
          case 'F':
            FIND(Str("no scale factor"));
            if (isdigit(*s) || *s == '-' || *s == '+')
              factor = atof(s);
            else
              factorfile = s;
            while (*++s);
            break;
          case 'M':
            FIND(Str("No maximum"));
            maximum = atof(s);
            while (*++s);
            break;
          case 'P':       /* Percentage */
            FIND(Str("No maximum"));
            maximum = atof(s);
            if (OO.outformat == AE_FLOAT) maximum = maximum*0.01;
            else maximum = 327.67*maximum;
            while (*++s);
            break;
          case 'h':
            OO.sfheader = 0;           /* skip sfheader  */
            break;
          case 'c':
          case 'a':
          case 'u':
          case '8':
          case 's':
          case '3':
          case 'l':
          case 'f':
            outformch = set_output_format(c, outformch);
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
            sprintf(errmsg,Str("unknown flag -%c"), c);
            usage(errmsg);
          }
      else if (inputfile == NULL) {
        inputfile = --s;
      }
      else usage(Str("too many arguments"));
    } while (--argc);
    dbfs_init(DFLT_DBFS);
 
 retry:
    /* Read sound file */
    if (!(infile = SCsndgetset(inputfile))) {
      err_printf(Str("%s: error while opening %s"), argv[0], inputfile);
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
          die(Str("can't write AIFF/WAV soundfile with no header"));
      }
      if (O.filetyp == TYP_WAV) {
        if (!O.sfheader)
          die(Str("can't write AIFF/WAV soundfile with no header"));
      }
      if (OO.filetyp)
        O.filetyp = OO.filetyp;
      if (O.rewrt_hdr && !O.sfheader)
        die(Str("can't rewrite header if no header requested"));
      if (O.outfilename == NULL)  O.outfilename = "test";
      sfinfo.frames = -1;
      sfinfo.samplerate = (int)(esr = p->sr);
      sfinfo.channels = nchnls = p->nchanls ;
      sfinfo.format = type2sf(O.filetyp)|format2sf(O.outformat);
      sfinfo.sections = 0;
      sfinfo.seekable = 0;
      outfd = openout(O.outfilename, 1);
      outfile = sf_open_fd(outfd, SFM_WRITE, &sfinfo, 1);
      outbufsiz = 1024 * O.sfsampsize;/* calc outbuf size  */
      outbuf = mmalloc((long)outbufsiz);                 /*  & alloc bufspace */
      printf(Str("writing %d-byte blks of %s to %s %s\n"),
             outbufsiz, getstrformat(O.outformat), O.outfilename,
             type2string(O.filetyp));
      InitScaleTable(factor, factorfile);
      ScaleSound(infile, outfile);
      sf_close(outfile);
    }
    else if (maximum!=0.0) {
      float mm = FindAndReportMax(infile);
      factor = maximum / mm;
      sf_close(infile);
      goto retry;
    }
    else
      FindAndReportMax(infile);
    if (O.ringbell) putc(7, stderr);
    return 0;

 outtyp:
    usage(Str("output soundfile cannot be both AIFF and WAV"));
    sprintf(errmsg,Str("sound output format cannot be both -%c and -%c"),
	    outformch, c);
    usage(errmsg);
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



static SNDFILE *
SCsndgetset(char *inputfile)
{
    SNDFILE      *infile;
    double       dur;
static  ARGOFFS  argoffs = {0};     /* these for sndgetset */
static	OPTXT    optxt;
static  MYFLT    fzero = 0.0;
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
    if ((infile = sndgetset(p)) == 0)            /* open sndfil, do skiptime */
      return(0);
    p->getframes = p->framesrem;
    dur = (double) p->getframes / p->sr;
    printf("scaling %ld sample frames (%3.1f secs)\n",
           p->getframes, dur);
    return(infile);
}

#define BUFFER_LEN (1024)

static void
ScaleSound(SNDFILE *infile, SNDFILE *outfd)
{
    MYFLT buffer[BUFFER_LEN];
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
    while ((read_in = getsndin(infile, buffer, BUFFER_LEN, p)) > 0) {
      for (i=0; i<read_in; i++) {
        buffer[i] = buffer[i] * gain(i+BUFFER_LEN*block);
        if (buffer[i] >= max) ++maxtimes;
        if (buffer[i] <= min) ++mintimes;
        if (buffer[i] > max)
          max = buffer[i], mxpos = i+BUFFER_LEN*block, maxtimes = 1;
        if (buffer[i] < min)
          min = buffer[i], minpos = i+BUFFER_LEN*block; mintimes = 1;
      }
      sf_write_MYFLT(outfd, buffer, read_in/nchnls);
      block++;
      bytes += read_in*O.sfsampsize;
      if (O.heartbeat) {
        putc("|/-\\"[block&3], stderr);
        putc('\b',stderr);
      }
    }
    sf_close(infile);
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
FindAndReportMax(SNDFILE *infile)
{
    int 	chans;
    double      tpersample;
    double	max, min;
    long	mxpos, minpos;
    int         maxtimes, mintimes;
    int		block = 0;
    MYFLT	buffer[BUFFER_LEN];
    long	read_in;
    int		i;

    chans = p->nchanls;
    tpersample = 1.0/(double)p->sr;
    max = 0.0;	mxpos = 0; maxtimes = 0;
    min = 0.0;	minpos = 0; mintimes = 0;
    while ((read_in = getsndin(infile, buffer, BUFFER_LEN, p)) > 0) {
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
    sf_close(infile);
    printf("Max val %d at index %ld (time %.4f, chan %d) %d times\n",
           (int)max,mxpos,tpersample*(mxpos/chans),(int)mxpos%chans,maxtimes);
    printf("Min val %d at index %ld (time %.4f, chan %d) %d times\n",
           (int)min,minpos,tpersample*(minpos/chans),(int)minpos%chans,mintimes);
    printf("Max scale factor = %.3f\n",
	   (float)SHORTMAX/(float)((max>-min)?max:-min) );
    return (max>-min ? max : -min);
}
