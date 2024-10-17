/*
    scale.c:

    Copyright (C) 1994  John ffitch
                  2005  John ffitch modifications to utility

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

/*******************************************************\
*   scale.c                                             *
*   scale a sound file by a float factor                *
*   jpff 3 Sep 1994 after code by dpwe 19sep90          *
*   and a certain amount of lifting from Csound itself  *
\*******************************************************/

#include "std_util.h"
#include "soundio.h"
#include <ctype.h>

/* Constants */

#define FIND(MSG)   if (*s == '\0')  \
    if (UNLIKELY(!(--argc) || ((s = *++argv) && *s == '-')))    \
      csound->Die(csound, "%s", MSG);

static const char *usage_txt[] = {
  Str_noop("Usage:\tscale [-flags] soundfile"),
  Str_noop("Legal flags are:"),
  Str_noop("-o fnam\tsound output filename"),
  Str_noop("-A\tcreate an AIFF format output soundfile"),
  Str_noop("-W\tcreate a WAV format output soundfile"),
  Str_noop("-h\tno header on output soundfile"),
  Str_noop("-c\t8-bit signed_char sound samples"),
  Str_noop("-a\talaw sound samples"),
  Str_noop("-u\tulaw sound samples"),
  Str_noop("-s\tshort_int sound samples"),
  Str_noop("-l\tlong_int sound samples"),
  Str_noop("-f\tfloat sound samples"),
  Str_noop("-F fpnum\tamount to scale amplitude"),
  Str_noop("-F file \tfile of scaling information (alternative)"),
  Str_noop("-M fpnum\tScale file to given maximum"),
  Str_noop("-P fpnum\tscale file to given percentage of full"),
  Str_noop("-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)"),
  Str_noop("-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write"),
  Str_noop("-N\tnotify (ring the bell) when score or miditrack is done"),
  Str_noop("-- fnam\tlog output to file"),
  Str_noop("flag defaults: scale -s -otest -F 0.0"),
  Str_noop("If scale is 0.0 then reports maximum possible scaling"),
    NULL
};

static void usage(CSOUND *csound, char *mesg)
{
    int32_t i;
    for (i = 0; usage_txt[i] != NULL; i++)
      csound->Message(csound, "%s\n", Str(usage_txt[i]));
    csound->Die(csound, "\n%s", mesg);
}

static char set_output_format(OPARMS *p, char c, char outformch)
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
  int32_t x0;
  int32_t x1;
  struct scalepoint *next;
} scalepoint;

static const scalepoint stattab = { 0.0, 0.0, 0.0, 0, 0, NULL };

typedef struct {
  double     ff;
  int32_t        table_used;
  scalepoint scale_table;
  scalepoint *end_table;
  SOUNDIN    *p;
} SCALE;

/* Static function prototypes */

static void  InitScaleTable(CSOUND *,SCALE *, double, char *);
static SNDFILE *SCsndgetset(CSOUND *, SCALE *, char *);
static void  ScaleSound(CSOUND *, SCALE *, SNDFILE *, SNDFILE *, OPARMS *);
static float FindAndReportMax(CSOUND *, SCALE *, SNDFILE *, OPARMS *);

static int32_t scale(CSOUND *csound, int32_t argc, char **argv)
{
    char        *inputfile = NULL;
    double      factor = 0.0;
    double      maximum = 0.0;
    char        *factorfile = NULL;
    SNDFILE     *infile = NULL, *outfile;
    void        *fd;
    char        outformch = 's', c, *s;
    const char  *envoutyp;
    SFLIB_INFO     sfinfo;
    OPARMS *O =(OPARMS *) csound->Calloc(csound, sizeof(OPARMS));
    SCALE       sc;
    unsigned    outbufsiz;

    memcpy(O,csound->GetOParms(csound), sizeof(OPARMS));
    memset(&sc, 0, sizeof(SCALE));
    sc.ff = 0.0;
    sc.table_used = 0;
    sc.scale_table = stattab;
    sc.end_table = &sc.scale_table;

    O->filetyp = O->outformat = 0;
    O->ringbell = O->heartbeat = 0;
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
    if (UNLIKELY(!(--argc)))
      usage(csound, Str("Insufficient arguments"));
    do {
      s = *++argv;
      if (*s++ == '-')                  /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND(Str("no outfilename"))
            O->outfilename = s;         /* soundout name */
            for ( ; *s != '\0'; s++) ;
            if (UNLIKELY(strcmp(O->outfilename, "stdin") == 0))
              csound->Die(csound, "%s", Str("-o cannot be stdin"));
#if defined(WIN32)
            if (UNLIKELY(strcmp(O->outfilename, "stdout") == 0)) {
              csound->Die(csound, "%s", Str("stdout audio not supported"));
            }
#endif
            break;
          case 'A':
            O->filetyp = TYP_AIFF;      /* AIFF output request  */
            break;
          case 'J':
            O->filetyp = TYP_IRCAM;     /* IRCAM output request */
            break;
          case 'W':
            O->filetyp = TYP_WAV;       /* WAV output request  */
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
            maximum = atof(s) * 0.01 * csound->Get0dBFS(csound);
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
            outformch = set_output_format(O, c, outformch);
            break;
          case 'R':
            O->rewrt_hdr = 1;
            break;
          case 'H':
            if (isdigit(*s)) {
              int32_t n;
              csound->Sscanf(s, "%d%n", O->heartbeat, &n);
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
              snprintf(err_msg, 64, Str("unknown flag -%c"), c);
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
    if (UNLIKELY(inputfile == NULL)) return -1;
    if (UNLIKELY(!(infile = SCsndgetset(csound, &sc, inputfile)))) {
      csound->Message(csound, Str("%s: error while opening %s"),
                              argv[0], inputfile);
      return -1;
    }
    if (factor != 0.0 || factorfile != NULL) {          /* perform scaling */
      if (!O->filetyp)
        O->filetyp = sc.p->filetyp;
      if (!O->outformat)
        O->outformat = sc.p->format;
      O->sfsampsize = csound->SndfileSampleSize(FORMAT2SF(O->outformat));
      if (O->filetyp == TYP_RAW)
        O->rewrt_hdr = 0;
      if (O->outfilename == NULL)
        O->outfilename = "test";
      (csound->GetUtility(csound))->SetUtilSr(csound, (MYFLT)sc.p->sr);
      (csound->GetUtility(csound))->SetUtilNchnls(csound, sc.p->nchanls);


      memset(&sfinfo, 0, sizeof(SFLIB_INFO));
      //sfinfo.frames = 0/*was -1*/;
      sfinfo.samplerate = (int32_t) ( sc.p->sr); // p->sr is int32_t already
      sfinfo.channels = sc.p->nchanls;
      sfinfo.format = TYPE2SF(O->filetyp) | FORMAT2SF(O->outformat);
      /* open file for write */
      fd = NULL;
      if (strcmp(O->outfilename, "stdout") == 0 ||
          strcmp(O->outfilename, "-") == 0) {
        outfile = csound->SndfileOpenFd(csound,1, SFM_WRITE, &sfinfo, 0);
        if (outfile != NULL) {
          if (UNLIKELY((fd =
                        csound->CreateFileHandle(csound, &outfile,
                                                 CSFILE_SND_W, "stdout")) == NULL)) {
            csound->SndfileClose(csound,outfile);
            csound->Die(csound, "%s", Str("Memory allocation failure"));
          }
        }
      }
      else
        fd = csound->FileOpen(csound, &outfile, CSFILE_SND_W,
                       O->outfilename, &sfinfo, "SFDIR",
                       csound->Type2CsfileType(O->filetyp, O->outformat), 0);
      if (UNLIKELY(fd == NULL))
        csound->Die(csound, Str("Failed to open output file %s: %s"),
                    O->outfilename, Str(csound->SndfileStrError(csound,NULL)));
      outbufsiz = 1024 * O->sfsampsize;    /* calc outbuf size  */
      csound->Message(csound, Str("writing %d-byte blks of %s to %s %s\n"),
                              (int32_t) outbufsiz,
                              csound->GetStrFormat(O->outformat),
                              O->outfilename,
                             csound->Type2String(O->filetyp));
      InitScaleTable(csound, &sc, factor, factorfile);
      ScaleSound(csound, &sc, infile, outfile, O);
    }
    else if (maximum != 0.0) {
      float mm = FindAndReportMax(csound, &sc, infile, O) ;
      factor = maximum / mm;
      goto retry;
    }
    else
      FindAndReportMax(csound, &sc, infile, O);
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
      if (UNLIKELY(csound->FileOpen(csound, &f, CSFILE_STD, factorfile, "r", NULL,
                                     CSFTYPE_FLOATS_TEXT, 0) == NULL))
        csound->Die(csound, Str("Failed to open %s"), factorfile);
      while (fscanf(f, "%lf %lf\n", &x, &y) == 2) {
        scalepoint *newpoint =
          (scalepoint*) csound->Malloc(csound, sizeof(scalepoint));
        thissc->end_table->next = newpoint;
        newpoint->x0 = thissc->end_table->x1;
        newpoint->y0 = thissc->end_table->y1;
        newpoint->x1 = (int32_t) (x*samplepert);
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

static double gain(SCALE *thissc, int32_t i)
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

    (csound->GetUtility(csound))->SetUtilSr(csound, FL(0.0));         /* set esr 0. with no orchestra */
    thissc->p = p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    p->channel = ALLCHNLS;
    p->skiptime = FL(0.0);
    p->analonly = 1;
    strNcpy(p->sfname, inputfile, MAXSNDNAME-1);//p->sfname[MAXSNDNAME-1]='\0';
    if ((infile = (csound->GetUtility(csound))->SndinGetSet(csound, p)) == 0) /*open sndfil, do skptim*/
      return(0);
    p->getframes = p->framesrem;
    dur = (double) p->getframes / p->sr;
    csound->Message(csound, "%s %" PRId64 " %s (%3.1f secs)\n",
                    Str("scaling"), p->getframes, Str("sample frame"), dur);
    return(infile);
}

#define BUFFER_LEN (1024)

static void
ScaleSound(CSOUND *csound, SCALE *thissc, SNDFILE *infile,
           SNDFILE *outfd, OPARMS *oparms)
{
    MYFLT buffer[BUFFER_LEN];
    long  read_in;
    double tpersample;
    double max, min;
    long  mxpos, minpos;
    int32_t   maxtimes, mintimes;
    int32_t   i, j, chans = thissc->p->nchanls;
    int32_t   block = 0;
    int32_t   bufferLenFrames = (int32_t) BUFFER_LEN / chans;
    int32_t   bufferLenSamples = bufferLenFrames * chans;

    tpersample = 1.0 / (double) thissc->p->sr;
    max = 0.0;  mxpos = 0; maxtimes = 0;
    min = 0.0;  minpos = 0; mintimes = 0;
    while ((read_in = (csound->GetUtility(csound))->Sndin(csound, infile, buffer,
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
        buffer[i] *= (1.0/csound->Get0dBFS(csound));
      }
      csound->SndfileWriteSamples(csound, outfd, buffer, read_in);
      block++;
      if (oparms->heartbeat) {
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\b", "|/-\\"[block&3]);
      }
    }
    csound->Message(csound, Str("Max val %.3f at index %ld (time %.4f, chan %d) "
                                "%d times\n"), max, (long) mxpos / (long) chans,
                            tpersample * (double) mxpos / (double) chans,
                            ((int32_t) mxpos % chans) + 1, (int32_t) maxtimes);
    csound->Message(csound, Str("Min val %.3f at index %ld (time %.4f, chan %d) "
                                "%d times\n"), min, (long) minpos / (long) chans,
                            tpersample * (double) minpos / (double) chans,
                            ((int32_t) minpos % chans) + 1, (int32_t) mintimes);
    csound->Message(csound, Str("Max scale factor = %.3f\n"),
                            (double) csound->Get0dBFS(csound) / (max > -min ?
                                                                 max:-min));
}

static float FindAndReportMax(CSOUND *csound, SCALE *thissc,
                              SNDFILE *infile, OPARMS *oparms)
{
    MYFLT   buffer[BUFFER_LEN];
    long    read_in;
    double  tpersample;
    double  max, min;
    long    mxpos, minpos;
    int32_t     maxtimes, mintimes;
    int32_t     i, chans = thissc->p->nchanls;
    int32_t     block = 0;
    int32_t     bufferLenFrames = (int32_t) BUFFER_LEN / chans;
    int32_t     bufferLenSamples = bufferLenFrames * chans;

    tpersample = 1.0 / (double) thissc->p->sr;
    max = 0.0;  mxpos = 0; maxtimes = 0;
    min = 0.0;  minpos = 0; mintimes = 0;
    while ((read_in = (csound->GetUtility(csound))->Sndin(csound, infile, buffer,
                                       bufferLenSamples, thissc->p)) > 0) {
      for (i = 0; i < read_in; i++) {
        //j = (i / chans) + (bufferLenFrames * block);
        if (buffer[i] >= max) ++maxtimes;
        if (buffer[i] <= min) ++mintimes;
        if (buffer[i] > max)
          max = buffer[i], mxpos = i + bufferLenSamples * block, maxtimes = 1;
        if (buffer[i] < min)
          min = buffer[i], minpos = i + bufferLenSamples * block, mintimes = 1;
      }
      block++;
      if (oparms->heartbeat) {
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\b", "|/-\\"[block&3]);
      }
    }
    csound->Message(csound, Str("Max val %.3f at index %ld (time %.4f, chan %d) "
                                "%d times\n"), max, (long) mxpos / (long) chans,
                            tpersample * (double) mxpos / (double) chans,
                            ((int32_t) mxpos % chans) + 1, (int32_t) maxtimes);
    csound->Message(csound, Str("Min val %.3f at index %ld (time %.4f, chan %d) "
                                "%d times\n"), min, (long) minpos / (long) chans,
                            tpersample * (double) minpos / (double) chans,
                            ((int32_t) minpos % chans) + 1, (int32_t) mintimes);
    csound->Message(csound, Str("Max scale factor = %.3f\n"),
                            (double) csound->Get0dBFS(csound)/ (max > -min ?
                                                                max:-min));
    return (float) (max > -min ? max : -min);
}

/* module interface */

int32_t scale_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "scale", scale);
    if (retval)
      return retval;
    return
      (csound->GetUtility(csound))->SetUtilityDescription(csound, "scale",
                                    Str("Reports and/or adjusts maximum gain"));
}
