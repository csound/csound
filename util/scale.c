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

#include "csdl.h"
#include "soundio.h"
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

/* Constants */

#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-')) \
                            csound->Die(csound, MSG);

static const char *usage_txt[] = {
    "Usage:\tscale [-flags] soundfile",
    "Legal flags are:",
    "-o fnam\tsound output filename",
    "-A\tcreate an AIFF format output soundfile",
    "-W\tcreate a WAV format output soundfile",
    "-h\tno header on output soundfile",
    "-c\t8-bit signed_char sound samples",
    "-a\talaw sound samples",
    "-u\tulaw sound samples",
    "-s\tshort_int sound samples",
    "-l\tlong_int sound samples",
    "-f\tfloat sound samples",
    "-F fpnum\tamount to scale amplitude",
    "-F file \tfile of scaling information (alternative)",
    "-M fpnum\tScale file to given maximum",
    "-P fpnum\tscale file to given percentage of full",
    "-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)",
    "-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write",
    "-N\tnotify (ring the bell) when score or miditrack is done",
    "-- fnam\tlog output to file",
    "flag defaults: scale -s -otest -F 0.0",
    "If scale is 0.0 then reports maximum possible scaling",
    NULL
};

static void usage(CSOUND *csound, char *mesg)
{
    int i;
    for (i = 0; usage_txt[i] != NULL; i++)
      csound->Message(csound, "%s\n", Str(usage_txt[i]));
    csound->Die(csound, "\n%s", mesg);
}

static char set_output_format(CSOUND *csound, OPARMS *p, char c, char outformch)
{
    switch (c) {
      case 'a': p->outformat = AE_ALAW;   /* a-law soundfile */
        break;
      case 'c': p->outformat = AE_CHAR;   /* signed 8-bit soundfile */
        break;
      case '8': p->outformat = AE_UNCH;   /* unsigned 8-bit soundfile */
        break;
      case 'f': p->outformat = AE_FLOAT;  /* float soundfile */
        break;
      case 's': p->outformat = AE_SHORT;  /* short_int soundfile*/
        break;
      case 'l': p->outformat = AE_LONG;   /* long_int soundfile */
        break;
      case 'u': p->outformat = AE_ULAW;   /* mu-law soundfile */
        break;
      case '3': p->outformat = AE_24INT;  /* 24bit packed soundfile*/
        break;
      case 'e': p->outformat = AE_FLOAT;  /* float soundfile (for rescaling) */
        break;
      default:
        return outformch; /* do nothing */
    };

    return c;
}

typedef struct scalepoint {
  double y0;
  double y1;
  double yr;
  int x0;
  int x1;
  struct scalepoint *next;
} scalepoint;

static scalepoint stattab = {0.0, 0.0, 0.0, 0, 0, NULL};

typedef struct {
  double     ff;
  int        table_used;
  scalepoint scale_table;
  scalepoint *end_table;
  SOUNDIN    *p;
} SCALE;

/* Static function prototypes */

static void  InitScaleTable(CSOUND *,SCALE *, double, char *);
static SNDFILE *SCsndgetset(CSOUND *, SCALE *, char *);
static void  ScaleSound(CSOUND *, SCALE *, SNDFILE *, SNDFILE *);
static float FindAndReportMax(CSOUND *, SCALE *, SNDFILE *);

static int scale(CSOUND *csound, int argc, char **argv)
{
    char        *inputfile = NULL;
    double      factor = 0.0;
    double      maximum = 0.0;
    char        *factorfile = NULL;
    SNDFILE     *infile = NULL, *outfile;
    void        *fd;
    char        outformch = 's', c, *s, *filnamp;
    const char  *envoutyp;
    SF_INFO     sfinfo;
    OPARMS      *O = csound->oparms;
    SCALE       sc;
    unsigned    outbufsiz;

    memset(&sc, 0, sizeof(SCALE));
    sc.ff = 0.0;
    sc.table_used = 0;
    sc.scale_table = stattab;
    sc.end_table = &sc.scale_table;

    O->filetyp = O->outformat = 0;
    /* Check arguments */
    if ((envoutyp = csound->GetEnv(csound, "SFOUTYP")) != NULL) {
      if (strcmp(envoutyp, "AIFF") == 0)
        O->filetyp = TYP_AIFF;
      else if (strcmp(envoutyp, "WAV") == 0)
        O->filetyp = TYP_WAV;
      else if (strcmp(envoutyp, "IRCAM") == 0)
        O->filetyp = TYP_IRCAM;
      else {
        csound->Die(csound, Str("%s not a recognized SFOUTYP env setting"),
                            envoutyp);
      }
    }
    O->filnamspace = filnamp = csound->Malloc(csound, 1024);
    if (!(--argc))
      usage(csound, Str("Insufficient arguments"));
    do {
      s = *++argv;
      if (*s++ == '-')                        /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND(Str("no outfilename"))
            O->outfilename = filnamp;      /* soundout name */
            while ((*filnamp++ = *s++)); s--;
            if (strcmp(O->outfilename, "stdin") == 0)
              csound->Die(csound, "-o cannot be stdin");
#if defined(mac_classic) || defined(WIN32)
            if (strcmp(O->outfilename, "stdout") == 0) {
              csound->Die(csound, Str("stdout audio not supported"));
            }
#endif
            break;
          case 'A':
            O->filetyp = TYP_AIFF;     /* AIFF output request  */
            break;
          case 'J':
            O->filetyp = TYP_IRCAM;      /* IRCAM output request */
            break;
          case 'W':
            O->filetyp = TYP_WAV;      /* WAV output request  */
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
            maximum = atof(s) * 0.01 * csound->e0dbfs;
            while (*++s);
            break;
          case 'h':
            O->filetyp = TYP_RAW;       /* skip sfheader  */
            break;
          case 'c':
          case 'a':
          case 'u':
          case '8':
          case 's':
          case '3':
          case 'l':
          case 'f':
            outformch = set_output_format(csound, O, c, outformch);
            break;
          case 'R':
            O->rewrt_hdr = 1;
            break;
          case 'H':
            if (isdigit(*s)) {
              int n;
              sscanf(s, "%d%n", &O->heartbeat, &n);
              s += n;
            }
            else O->heartbeat = 1;
            break;
          case 'N':
            O->ringbell = 1;             /* notify on completion */
            break;
          default:
            {
              char  err_msg[64];
              sprintf(err_msg, Str("unknown flag -%c"), c);
              usage(csound, err_msg);
            }
          }
      else if (inputfile == NULL) {
        inputfile = --s;
      }
      else usage(csound, Str("too many arguments"));
    } while (--argc);

 retry:
    /* Read sound file */
    if (!(infile = SCsndgetset(csound, &sc, inputfile))) {
      csound->Message(csound, Str("%s: error while opening %s"),
                              argv[0], inputfile);
      return -1;
    }
    if (factor != 0.0 || factorfile != NULL) {          /* perform scaling */
      if (!O->filetyp)
        O->filetyp = sc.p->filetyp;
      if (!O->outformat)
        O->outformat = sc.p->format;
      O->sfheader = (O->filetyp == TYP_RAW ? 0 : 1);
      O->sfsampsize = csound->sfsampsize(FORMAT2SF(O->outformat));
      if (!O->sfheader)
        O->rewrt_hdr = 0;
      if (O->outfilename == NULL)
        O->outfilename = "test";
      csound->esr = sc.p->sr;
      csound->nchnls = sc.p->nchanls;
      memset(&sfinfo, 0, sizeof(SF_INFO));
      sfinfo.frames = -1;
      sfinfo.samplerate = (int) MYFLT2LRND(csound->esr);
      sfinfo.channels = csound->nchnls;
      sfinfo.format = TYPE2SF(O->filetyp) | FORMAT2SF(O->outformat);
      /* open file for write */
      fd = NULL;
      if (strcmp(O->outfilename, "stdout") == 0 ||
          strcmp(O->outfilename, "-") == 0) {
        outfile = sf_open_fd(1, SFM_WRITE, &sfinfo, 0);
        if (outfile != NULL) {
          if ((fd = csound->CreateFileHandle(csound, &outfile,
                                             CSFILE_SND_W, "stdout")) == NULL) {
            sf_close(outfile);
            csound->Die(csound, Str("Memory allocation failure"));
          }
        }
      }
      else
        fd = csound->FileOpen(csound, &outfile, CSFILE_SND_W,
                                      O->outfilename, &sfinfo,
                                      "SFDIR");
      if (fd == NULL)
        csound->Die(csound, Str("Failed to open output file %s"),
                            O->outfilename);
      outbufsiz = 1024 * O->sfsampsize;    /* calc outbuf size  */
      csound->Message(csound, Str("writing %d-byte blks of %s to %s %s\n"),
                              (int) outbufsiz,
                              csound->getstrformat(O->outformat),
                              O->outfilename,
                              csound->type2string(O->filetyp));
      InitScaleTable(csound, &sc, factor, factorfile);
      ScaleSound(csound, &sc, infile, outfile);
    }
    else if (maximum != 0.0) {
      float mm = FindAndReportMax(csound, &sc, infile);
      factor = maximum / mm;
      goto retry;
    }
    else
      FindAndReportMax(csound, &sc, infile);
    if (O->ringbell)
      csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c", '\007');
    return 0;
}

static void InitScaleTable(CSOUND *csound, SCALE *thissc,
                           double factor, char *factorfile)
{
    if (factor != 0.0) thissc->ff = factor;
    else {
      FILE    *f;
      double  samplepert = (double)thissc->p->sr;
      double  x, y;
      if (csound->FileOpen(csound, &f, CSFILE_STD, factorfile, "r", NULL)
          == NULL)
        csound->Die(csound, Str("Failed to open %s"), factorfile);
      while (fscanf(f, "%lf %lf\n", &x, &y) == 2) {
        scalepoint *newpoint =
          (scalepoint*) csound->Malloc(csound, sizeof(scalepoint));
        thissc->end_table->next = newpoint;
        newpoint->x0 = thissc->end_table->x1;
        newpoint->y0 = thissc->end_table->y1;
        newpoint->x1 = (int) (x*samplepert);
        newpoint->y1 = y;
        newpoint->yr =
          (x == newpoint->x0 ?
           y - newpoint->y0 :
           (y - newpoint->y0)/((double)(newpoint->x1 - newpoint->x0)));
        newpoint->next = NULL;
        thissc->end_table = newpoint;
      }
      {
        scalepoint *newpoint = (scalepoint*)
          csound->Malloc(csound,sizeof(scalepoint));
        thissc->end_table->next = newpoint;
        newpoint->x0 = thissc->end_table->x1;
        newpoint->y0 = thissc->end_table->y1;
        newpoint->x1 = 0x7fffffff;
        newpoint->y1 = 0.0;
        newpoint->next = NULL;
        newpoint->yr = (x == newpoint->x0 ?
                        -newpoint->y0 :
                        -newpoint->y0/((double)(0x7fffffff-newpoint->x0)));
      }
      thissc->end_table = &thissc->scale_table;
/*      { */
/*          scalepoint *tt = &thissc->scale_table; */
/*          csound->Message(csound, "Scale table is\n"); */
/*          while (tt != NULL) { */
/*              csound->Message(csound, "(%d %f) -> %d %f [%f]\n", */
/*                      tt->x0, tt->y0, tt->x1, tt->y1, tt->yr); */
/*              tt = tt->next; */
/*          } */
/*          csound->Message(csound, "END of Table\n"); */
/*      } */
      thissc->table_used = 1;
    }
}

static double gain(SCALE *thissc, int i)
{
    if (!thissc->table_used) return thissc->ff;
    while (i<thissc->end_table->x0 ||
           i>thissc->end_table->x1) {/* Get correct segment */
/*      csound->Message(csound, "Next table: %d (%d %f) -> %d %f [%f]\n", */
      /*            i, thissc->end_table->x0, thissc->end_table->y0, */
      /*            thissc->end_table->x1, thissc->end_table->y1, */
/*            thissc->end_table->yr); */
        thissc->end_table = thissc->end_table->next;
    }
    return thissc->end_table->y0 +
      thissc->end_table->yr * (double)(i - thissc->end_table->x0);
}

static SNDFILE *
SCsndgetset(CSOUND *csound, SCALE *thissc, char *inputfile)
{
    SNDFILE *infile;
    double  dur;
    SOUNDIN *p;

    csound->esr = FL(0.0);      /* set esr 0. with no orchestra   */
    thissc->p = p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    p->channel = ALLCHNLS;
    p->skiptime = FL(0.0);
    p->analonly = 1;
    strcpy(p->sfname, inputfile);
    if ((infile = csound->sndgetset(csound, p)) == 0) /*open sndfil, do skptim*/
      return(0);
    p->getframes = p->framesrem;
    dur = (double) p->getframes / p->sr;
    csound->Message(csound, "scaling %ld sample frames (%3.1f secs)\n",
                            (long) p->getframes, dur);
    return(infile);
}

#define BUFFER_LEN (1024)

static void
ScaleSound(CSOUND *csound, SCALE *thissc, SNDFILE *infile, SNDFILE *outfd)
{
    MYFLT buffer[BUFFER_LEN];
    long  read_in;
    double tpersample;
    double max, min;
    long  mxpos, minpos;
    int   maxtimes, mintimes;
    int   i, j, chans = thissc->p->nchanls;
    int   block = 0;
    int   bufferLenFrames = (int) BUFFER_LEN / chans;
    int   bufferLenSamples = bufferLenFrames * chans;

    tpersample = 1.0 / (double) thissc->p->sr;
    max = 0.0;  mxpos = 0; maxtimes = 0;
    min = 0.0;  minpos = 0; mintimes = 0;
    while ((read_in = csound->getsndin(csound, infile, buffer,
                                       bufferLenSamples, thissc->p)) > 0) {
      for (i = 0; i < read_in; i++) {
        j = (i / chans) + (bufferLenFrames * block);
        buffer[i] = buffer[i] * gain(thissc, j);
        if (buffer[i] >= max) ++maxtimes;
        if (buffer[i] <= min) ++mintimes;
        if (buffer[i] > max)
          max = buffer[i], mxpos = i + bufferLenSamples * block, maxtimes = 1;
        if (buffer[i] < min)
          min = buffer[i], minpos = i + bufferLenSamples * block, mintimes = 1;
        buffer[i] *= csound->dbfs_to_float;
      }
      sf_write_MYFLT(outfd, buffer, read_in);
      block++;
      if (csound->oparms->heartbeat) {
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\b", "|/-\\"[block&3]);
      }
    }
    csound->Message(csound, "Max val %.3f at index %ld (time %.4f, chan %d) "
                            "%d times\n", max, (long) mxpos / (long) chans,
                            tpersample * (double) mxpos / (double) chans,
                            ((int) mxpos % chans) + 1, (int) maxtimes);
    csound->Message(csound, "Min val %.3f at index %ld (time %.4f, chan %d) "
                            "%d times\n", min, (long) minpos / (long) chans,
                            tpersample * (double) minpos / (double) chans,
                            ((int) minpos % chans) + 1, (int) mintimes);
    csound->Message(csound, "Max scale factor = %.3f\n",
                            (double) csound->e0dbfs / (max > -min ? max:-min));
}

static float FindAndReportMax(CSOUND *csound, SCALE *thissc, SNDFILE *infile)
{
    MYFLT   buffer[BUFFER_LEN];
    long    read_in;
    double  tpersample;
    double  max, min;
    long    mxpos, minpos;
    int     maxtimes, mintimes;
    int     i, j, chans = thissc->p->nchanls;
    int     block = 0;
    int     bufferLenFrames = (int) BUFFER_LEN / chans;
    int     bufferLenSamples = bufferLenFrames * chans;

    tpersample = 1.0 / (double) thissc->p->sr;
    max = 0.0;  mxpos = 0; maxtimes = 0;
    min = 0.0;  minpos = 0; mintimes = 0;
    while ((read_in = csound->getsndin(csound, infile, buffer,
                                       bufferLenSamples, thissc->p)) > 0) {
      for (i = 0; i < read_in; i++) {
        j = (i / chans) + (bufferLenFrames * block);
        if (buffer[i] >= max) ++maxtimes;
        if (buffer[i] <= min) ++mintimes;
        if (buffer[i] > max)
          max = buffer[i], mxpos = i + bufferLenSamples * block, maxtimes = 1;
        if (buffer[i] < min)
          min = buffer[i], minpos = i + bufferLenSamples * block, mintimes = 1;
      }
      block++;
      if (csound->oparms->heartbeat) {
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\b", "|/-\\"[block&3]);
      }
    }
    csound->Message(csound, "Max val %.3f at index %ld (time %.4f, chan %d) "
                            "%d times\n", max, (long) mxpos / (long) chans,
                            tpersample * (double) mxpos / (double) chans,
                            ((int) mxpos % chans) + 1, (int) maxtimes);
    csound->Message(csound, "Min val %.3f at index %ld (time %.4f, chan %d) "
                            "%d times\n", min, (long) minpos / (long) chans,
                            tpersample * (double) minpos / (double) chans,
                            ((int) minpos % chans) + 1, (int) mintimes);
    csound->Message(csound, "Max scale factor = %.3f\n",
                            (double) csound->e0dbfs / (max > -min ? max:-min));
    return (float) (max > -min ? max : -min);
}

/* module interface */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "scale", scale);
    if (retval)
      return retval;
    return csound->SetUtilityDescription(csound, "scale",
                                         "Reports and/or adjusts maximum gain");
}

