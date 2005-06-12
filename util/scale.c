/*
    scale.c:

    Copyright (C) 1994  John ffitch
                  2005  John ffitch modifications to utility

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
*   scale.c                                             *
*   scale a sound file by a float factor                *
*   jpff 3 Sep 1994 after code by dpwe 19sep90          *
*   and a certain amount of lifting from Csound itself  *
\*******************************************************/

#include "cs.h"
#include "soundio.h"
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

/* Constants */

#define SHORTMAX 32767
#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-')) \
                            csoundDie(csound, MSG);

/* Static function prototypes */

static void  InitScaleTable(ENVIRON *,double, char *);
static SNDFILE *SCsndgetset(ENVIRON *,char *);
static void  ScaleSound(ENVIRON *, SNDFILE *, SNDFILE *);
static float FindAndReportMax(ENVIRON *, SNDFILE *);

/* Externs */
extern void writeheader(ENVIRON *, int, char *);
extern char *getstrformat(int);
extern char* type2string(int);
extern short sfsampsize(int);

/* Static global variables */
static  SOUNDIN    *p;  /* space allocated by SAsndgetset() */
static  unsigned   outbufsiz;
static  MYFLT      *out_buf;

static void usage(ENVIRON *csound, char *mesg)
{
    csound->Message(csound, "%s\n", mesg);
    csound->Message(csound,Str("Usage:\tscale [-flags] soundfile\n"));
    csound->Message(csound,Str("Legal flags are:\n"));
    csound->Message(csound,Str("-o fnam\tsound output filename\n"));
    csound->Message(csound,Str("-A\tcreate an AIFF format output soundfile\n"));
    csound->Message(csound,Str("-W\tcreate a WAV format output soundfile\n"));
    csound->Message(csound,Str("-h\tno header on output soundfile\n"));
    csound->Message(csound,Str("-c\t8-bit signed_char sound samples\n"));
    csound->Message(csound,Str("-a\talaw sound samples\n"));
    csound->Message(csound,Str("-u\tulaw sound samples\n"));
    csound->Message(csound,Str("-s\tshort_int sound samples\n"));
    csound->Message(csound,Str("-l\tlong_int sound samples\n"));
    csound->Message(csound,Str("-f\tfloat sound samples\n"));
    csound->Message(csound,Str("-F fpnum\tamount to scale amplitude\n"));
    csound->Message(csound,Str(
                   "-F file \tfile of scaling information (alternative)\n"));
    csound->Message(csound,Str(
                   "-M fpnum\tScale file to given maximum\n"));
    csound->Message(csound,Str(
                   "-P fpnum\tscale file to given percentage of full\n"));
    csound->Message(csound,
      Str(
        "-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)\n"));
    csound->Message(csound,Str(
        "-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write\n"));
    csound->Message(csound,Str(
        "-N\tnotify (ring the bell) when score or miditrack is done\n"));
    csound->Message(csound,Str("-- fnam\tlog output to file\n"));
    csound->Message(csound,Str("flag defaults: scale -s -otest -F 0.0\n"));
    csound->Message(csound,Str(
        "If scale is 0.0 then reports maximum possible scaling\n"));
    exit(1);
}
static char set_output_format(ENVIRON *csound, OPARMS *p, char c, char outformch)
{
    if (p->outformat && (csound->oparms->msglevel & WARNMSG)) {
      csound->Message(csound,
                      Str("WARNING: Sound format -%c has been overruled by -%c\n"),
                      outformch, c);
    }

    switch (c) {
    case 'a':
      p->outformat = AE_ALAW;    /* a-law soundfile */
      break;

    case 'c':
      p->outformat = AE_CHAR;    /* signed 8-bit soundfile */
      break;

    case '8':
      p->outformat = AE_UNCH;    /* unsigned 8-bit soundfile */
      break;

    case 'f':
      p->outformat = AE_FLOAT;   /* float soundfile */
      break;

    case 's':
      p->outformat = AE_SHORT;   /* short_int soundfile*/
      break;

    case 'l':
      p->outformat = AE_LONG;    /* long_int soundfile */
      break;

    case 'u':
      p->outformat = AE_ULAW;    /* mu-law soundfile */
      break;

    case '3':
      p->outformat = AE_24INT;   /* 24bit packed soundfile*/
      break;

    case 'e':
      p->outformat = AE_FLOAT;   /* float soundfile (for rescaling) */
      break;

    default:
      return outformch; /* do nothing */
    };

  return c;
}

static int scale(void *csound_, int argc, char **argv)
{
    ENVIRON     *csound = (ENVIRON*) csound_;
    char        *inputfile = NULL;
    double      factor = 0.0;
    double      maximum = 0.0;
    char        *factorfile = NULL;
    SNDFILE     *infile = 0, *outfile;
    FILE        *outfd;
    char        outformch = 's', c, *s, *filnamp;
    char        *envoutyp;
    SF_INFO     sfinfo;
    OPARMS      OO;

    init_getstring(argc, argv);
    csoundPreCompile(csoundCreate(NULL));
    memset(&OO, 0, sizeof(OO));
    /* Check arguments */
    {
      if ((envoutyp = csoundGetEnv(csound, "SFOUTYP")) != NULL) {
        if (strcmp(envoutyp,"AIFF") == 0)
          OO.filetyp = TYP_AIFF;
        else if (strcmp(envoutyp,"WAV") == 0)
          OO.filetyp = TYP_WAV;
        else if (strcmp(envoutyp,"IRCAM") == 0)
          OO.filetyp = TYP_IRCAM;
        else {
          csound->Message(csound,Str("%s not a recognized SFOUTYP env setting"),
                     envoutyp);
          exit(1);
        }
      }
    }
    csound->oparms->filnamspace = filnamp = mmalloc(csound, (long)1024);
    if (!(--argc))
      usage(csound, Str("Insufficient arguments"));
    do {
      s = *++argv;
      if (*s++ == '-')                        /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND(Str("no outfilename"))
            csound->oparms->outfilename = filnamp;            /* soundout name */
            while ((*filnamp++ = *s++)); s--;
            if (strcmp(csound->oparms->outfilename,"stdin") == 0)
              csoundDie(csound, "-o cannot be stdin");
            if (strcmp(csound->oparms->outfilename,"stdout") == 0) {
#if defined mac_classic || defined SYMANTEC || defined BCC || defined __WATCOMC__ || defined WIN32
              csoundDie(csound, Str("stdout audio not supported"));
#else
              if ((csound->oparms->stdoutfd = dup(1)) < 0) /* redefine stdout */
                csoundDie(csound, Str("too many open files"));
              dup2(2,1);                /* & send 1's to stderr */
#endif
            }
            break;
          case 'A':
            if (OO.filetyp == TYP_WAV) {
              if (envoutyp == NULL) goto outtyp;
              if (csound->oparms->msglevel & WARNMSG)
                csound->Message(csound,Str("-A overriding local default WAV out"));
            }
            OO.filetyp = TYP_AIFF;     /* AIFF output request  */
            break;
          case 'J':
            if (OO.filetyp == TYP_AIFF ||
                OO.filetyp == TYP_WAV) {
              if (envoutyp == NULL) goto outtyp;
              if (csound->oparms->msglevel & WARNMSG)
                csound->Message(csound,Str("WARNING: -J overriding local default AIFF/WAV out\n"));
            }
            OO.filetyp = TYP_IRCAM;      /* IRCAM output request */
            break;
          case 'W':
            if (OO.filetyp == TYP_AIFF) {
              if (envoutyp == NULL) goto outtyp;
              if (csound->oparms->msglevel & WARNMSG)
                csound->Message(csound,Str("-W overriding local default AIFF out"));
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
            outformch = set_output_format(csound, &OO, c, outformch);
            break;
          case 'R':
            csound->oparms->rewrt_hdr = 1;
            break;
          case 'H':
            if (isdigit(*s)) {
              int n;
              sscanf(s, "%d%n", &csound->oparms->heartbeat, &n);
              s += n;
            }
            else csound->oparms->heartbeat = 1;
            break;
          case 'N':
            csound->oparms->ringbell = 1;             /* notify on completion */
            break;
          default:
            sprintf(csound->errmsg,Str("unknown flag -%c"), c);
            usage(csound, csound->errmsg);
          }
      else if (inputfile == NULL) {
        inputfile = --s;
      }
      else usage(csound, Str("too many arguments"));
    } while (--argc);
    dbfs_init(csound, DFLT_DBFS);

 retry:
    /* Read sound file */
    if (!(infile = SCsndgetset(csound,inputfile))) {
      csound->Message(csound,Str("%s: error while opening %s"), argv[0], inputfile);
      exit(1);
    }
    if (factor != 0.0 || factorfile != NULL) {          /* perform scaling */
      if (OO.outformat)                       /* if no audioformat yet  */
        csound->oparms->outformat = OO.outformat;
      else csound->oparms->outformat = p->format; /* Copy from input file */
      csound->oparms->sfsampsize = sfsampsize(csound->oparms->outformat);
      if (OO.filetyp)
        csound->oparms->filetyp = OO.filetyp;
      else csound->oparms->outformat = csound->oparms->informat; /* Copy from input file */
      if (OO.sfheader)
        csound->oparms->sfheader = OO.sfheader;
      else csound->oparms->sfheader = 1;
      if (csound->oparms->filetyp == TYP_AIFF) {
        if (!csound->oparms->sfheader)
          csoundDie(csound, Str("can't write AIFF/WAV soundfile with no header"));
      }
      if (csound->oparms->filetyp == TYP_WAV) {
        if (!csound->oparms->sfheader)
          csoundDie(csound, Str("can't write AIFF/WAV soundfile with no header"));
      }
      if (OO.filetyp)
        csound->oparms->filetyp = OO.filetyp;
      if (csound->oparms->rewrt_hdr && !csound->oparms->sfheader)
        csoundDie(csound, Str("can't rewrite header if no header requested"));
      if (csound->oparms->outfilename == NULL)  csound->oparms->outfilename = "test";
      sfinfo.frames = -1;
      sfinfo.samplerate = (int)(csound->esr = p->sr);
      sfinfo.channels = csound->nchnls = p->nchanls;
      sfinfo.format = TYPE2SF(csound->oparms->filetyp) | FORMAT2SF(csound->oparms->outformat);
      sfinfo.sections = 0;
      sfinfo.seekable = 0;
      outfd = csound->FileOpen(csound, outfd, CSFILE_STD,
                               csound->oparms->outfilename, "w", NULL);
      outfile = sf_open_fd(fileno(outfd), SFM_WRITE, &sfinfo, 1);
      outbufsiz = 1024 * csound->oparms->sfsampsize; /* calc outbuf size  */
      out_buf = mmalloc(csound, (long)outbufsiz);    /*  & alloc bufspace */
      csound->Message(csound,Str("writing %d-byte blks of %s to %s %s\n"),
                      outbufsiz, getstrformat(csound->oparms->outformat),
                      csound->oparms->outfilename,
                      type2string(csound->oparms->filetyp));
      InitScaleTable(csound, factor, factorfile);
      ScaleSound(csound, infile, outfile);
      sf_close(outfile);
    }
    else if (maximum!=0.0) {
      float mm = FindAndReportMax(csound,infile);
      factor = maximum / mm;
      sf_close(infile);
      goto retry;
    }
    else
      FindAndReportMax(csound, infile);
    if (csound->oparms->ringbell) putc(7, stderr);
    return 0;

 outtyp:
    sprintf(csound->errmsg,
            Str("sound output format cannot be both -%c and -%c"),
            outformch, c);
    usage(csound, csound->errmsg);
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

static void InitScaleTable(ENVIRON *csound, double factor, char *factorfile)
{
    if (factor != 0.0) ff = factor;
    else {
      FILE *f = fopen(factorfile, "r");
      double samplepert = (double)p->sr;
      double x, y;
      while (fscanf(f, "%lf %lf\n", &x, &y) == 2) {
        scalepoint *newpoint = (scalepoint*) malloc(sizeof(scalepoint));
        if (newpoint == NULL) {
          csound->Message(csound, "Insufficient memory\n");
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
          csound->Message(csound, "Insufficient memory\n");
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
/*      { */
/*          scalepoint *tt = &scale_table; */
/*          csound->Message(csound, "Scale table is\n"); */
/*          while (tt != NULL) { */
/*              csound->Message(csound, "(%d %f) -> %d %f [%f]\n", */
/*                      tt->x0, tt->y0, tt->x1, tt->y1, tt->yr); */
/*              tt = tt->next; */
/*          } */
/*          csound->Message(csound, "END of Table\n"); */
/*      } */
      table_used = 1;
    }
}

static double gain(int i)
{
    if (!table_used) return ff;
    while (i<end_table->x0 || i>end_table->x1){/* Get correct segment */
/*      csound->Message(csound, "Next table: %d (%d %f) -> %d %f [%f]\n", */
/*            i, end_table->x0, end_table->y0, end_table->x1, end_table->y1, */
/*            end_table->yr); */
        end_table = end_table->next;
    }
    return end_table->y0 + end_table->yr * (double)(i - end_table->x0);
}

static SNDFILE *
SCsndgetset(ENVIRON *csound, char *inputfile)
{
    SNDFILE *infile;
    double  dur;

    csoundInitEnv(csound);      /* stand-alone init of SFDIR etc. */
    csound->esr = FL(0.0);      /* set esr 0. with no orchestra   */
    p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    p->channel = ALLCHNLS;
    p->skiptime = FL(0.0);
    strcpy(p->sfname, inputfile);
    if ((infile = csound->sndgetset(csound, p)) == 0) /*open sndfil, do skptim*/
      return(0);
    p->getframes = p->framesrem;
    dur = (double) p->getframes / p->sr;
    csound->Message(csound,"scaling %ld sample frames (%3.1f secs)\n",
           (long) p->getframes, dur);
    return(infile);
}

#define BUFFER_LEN (1024)

static void
ScaleSound(ENVIRON *csound, SNDFILE *infile, SNDFILE *outfd)
{
    MYFLT buffer[BUFFER_LEN];
    long  read_in;
    float tpersample;
    float max, min;
    long  mxpos, minpos;
    int   maxtimes, mintimes;
    int   i, chans;
    long  bytes = 0;
    int   block = 0;

    chans = p->nchanls;
    tpersample = 1.0/(float)p->sr;
    max = 0.0;  mxpos = 0; maxtimes = 0;
    min = 0.0;  minpos = 0; mintimes = 0;
    while ((read_in=csound->getsndin(csound,infile,buffer,BUFFER_LEN,p)) > 0) {
      for (i=0; i<read_in; i++) {
        buffer[i] = buffer[i] * gain(i+BUFFER_LEN*block);
        if (buffer[i] >= max) ++maxtimes;
        if (buffer[i] <= min) ++mintimes;
        if (buffer[i] > max)
          max = buffer[i], mxpos = i+BUFFER_LEN*block, maxtimes = 1;
        if (buffer[i] < min)
          min = buffer[i], minpos = i+BUFFER_LEN*block; mintimes = 1;
      }
      sf_write_MYFLT(outfd, buffer, read_in / csound->nchnls);
      block++;
      bytes += read_in*csound->oparms->sfsampsize;
      if (csound->oparms->heartbeat) {
        putc("|/-\\"[block&3], stderr);
        putc('\b',stderr);
      }
    }
    sf_close(infile);
    csound->Message(csound,"Max val %d at index %ld (time %.4f, chan %d) %d times\n",
           (int)max,mxpos,tpersample*(mxpos/chans),(int)mxpos%chans, maxtimes);
    csound->Message(csound,"Min val %d at index %ld (time %.4f, chan %d) %d times\n",
           (int)min,minpos,tpersample*(minpos/chans),(int)minpos%chans, mintimes);
    csound->Message(csound,"Max scale factor = %.3f\n",
           (float)SHORTMAX/(float)((max>-min)?max:-min) );
    return;
}

static float
FindAndReportMax(ENVIRON *csound, SNDFILE *infile)
{
    int      chans;
    double   tpersample;
    double   max, min;
    long     mxpos, minpos;
    int      maxtimes, mintimes;
    int      block = 0;
    MYFLT    buffer[BUFFER_LEN];
    long     read_in;
    int      i;

    chans = p->nchanls;
    tpersample = 1.0/(double)p->sr;
    max = 0.0;  mxpos = 0; maxtimes = 0;
    min = 0.0;  minpos = 0; mintimes = 0;
    while ((read_in=csound->getsndin(csound,infile,buffer,BUFFER_LEN,p)) > 0) {
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
    csound->Message(csound,"Max val %d at index %ld (time %.4f, chan %d) %d times\n",
           (int)max,mxpos,tpersample*(mxpos/chans),(int)mxpos%chans,maxtimes);
    csound->Message(csound,"Min val %d at index %ld (time %.4f, chan %d) %d times\n",
           (int)min,minpos,tpersample*(minpos/chans),(int)minpos%chans,mintimes);
    csound->Message(csound,"Max scale factor = %.3f\n",
           (float)SHORTMAX/(float)((max>-min)?max:-min) );
    return (max>-min ? max : -min);
}

/* module interface */

PUBLIC int csoundModuleCreate(void *csound)
{
    int retval = ((ENVIRON*) csound)->AddUtility(csound, "scale", scale);
    if (!retval) {
      retval = ((ENVIRON*) csound)->SetUtilityDescription(csound, "scale",
                    "Report and/or adjusts maximum gain");
    }
    return retval;
}
