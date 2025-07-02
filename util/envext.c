/*
  envext.c:

    Copyright (C) 1994 John ffitch

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
*   envext.c                                            *
*   Extract an envelope file from a sound file          *
*   jpff 11 Dec 1994                                    *
*   mainly lifted from scale and Csound itself          *
\*******************************************************/

#include "std_util.h"
#include "soundio.h"
#include <stdio.h>
#include <stdlib.h>

/* Constants */

#define SHORTMAX 32767.0
#define FIND(MSG)   if (*s == '\0')  \
    if (UNLIKELY(!(--argc) || ((s = *++argv) && *s == '-'))) {          \
      csound->Message(csound, "%s", MSG); csound->LongJmp(csound, 1); }

/* Static function prototypes */

static SNDFILE * SCsndgetset(CSOUND *, SOUNDIN **, char *);
static void FindEnvelope(CSOUND *, SNDFILE *, SOUNDIN *, double, char *);

static void envext_usage(CSOUND *csound, char *mesg, ...)
{
    va_list args;

    csound->Message(csound,"%s", Str("Usage:\tenvext [-flags] soundfile\n"));
    csound->Message(csound,"%s", Str("Legal flags are:\n"));
    csound->Message(csound,"%s", Str("-o fnam\tsound output filename\n"));
    csound->Message(csound, "%s", Str( "-w time\tSize of window\n"));
    csound->Message(csound,"%s", Str("flag defaults: envext -onewenv -w0.25\n"));
    va_start(args, mesg);
    csound->ErrMsgV(csound, Str("envext: error: "), mesg, args);
    va_end(args);
    csound->LongJmp(csound, 1);
}

static int32_t envext(CSOUND *csound, int32_t argc, char **argv)
{
    char        *inputfile = NULL;
    SNDFILE     *infd;
    char        c, *s;
    double      window = 0.25;
    SOUNDIN     *p;  /* space allocated by SAsndgetset() */
    char        *outname = NULL;

    /* Check arguments */
    if (UNLIKELY(!(--argc)))
      envext_usage(csound, "%s", Str("Insufficient arguments"));
    do {
      s = *++argv;
      if (*s++ == '-')                        /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND(Str("no outfilename"))
            outname = s;
            while (*++s);
            break;
          case 'w':
            FIND(Str("No window size"));
            window = atof(s);
            while (*++s);
            break;
          default:
            envext_usage(csound, Str("unknown flag -%c"), c);
          }
      else if (LIKELY(inputfile == NULL)) {
        inputfile = --s;
      }
      else envext_usage(csound, Str("too many arguments"));
    } while (--argc);

    /* Read sound file */
    if (UNLIKELY(inputfile==NULL ||
                 (infd = SCsndgetset(csound, &p, inputfile))==NULL)) {
      csound->Message(csound,Str("%s: error while opening %s"), argv[0], inputfile);
      return 1;
    }
    FindEnvelope(csound, infd, p, window, outname);
    return 0;
}

static SNDFILE *
SCsndgetset(CSOUND *csound, SOUNDIN **pp, char *inputfile)
{
    SNDFILE *infd;
    double  dur;
    SOUNDIN *p;

    (csound->GetUtility(csound))->SetUtilSr(csound, FL(0.0));      /* set esr 0. with no orchestra   */
    *pp = p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    p->channel = ALLCHNLS;
    p->skiptime = FL(0.0);
    strNcpy(p->sfname, inputfile, MAXSNDNAME-1);
    if ((infd = (csound->GetUtility(csound))->SndinGetSet(csound, p)) == 0) /*open sndfil, do skiptime*/
      return(0);
    p->getframes = p->framesrem;
    dur = (double) p->getframes / p->sr;
    csound->Message(csound,Str("enveloping %"PRId64" sample frames (%3.1f secs)\n"),
           (int64_t) p->getframes, dur);
    return(infd);
}

static void
FindEnvelope(CSOUND *csound, SNDFILE *infd, SOUNDIN *p,
             double window, char *outname)
{
    double      tpersample;
    double      max, min;
    int64_t     mxpos, minpos;
    int32_t     block = 0;
    MYFLT       *buffer;
    int32_t     bufferlen;
    int64_t     read_in;
    int32_t     i;
    FILE *      outfile;

    outfile = fopen((outname == NULL ? "newenv" : outname), "w");
    bufferlen = (int32_t)(window*(double)p->sr);
    buffer = (MYFLT*) malloc(bufferlen*sizeof(MYFLT));
    tpersample = 1.0/(double)p->sr;
    fprintf(outfile, "%.3f\t%.3f\n", 0.0, 0.0);
    while ((read_in = (csound->GetUtility(csound))->Sndin(csound,infd,buffer,bufferlen,p)) > 0) {
      max = 0.0;        mxpos = 0;
      min = 0.0;        minpos = 0;
      for (i=0; i<read_in; i++) {
        if ((double)buffer[i] > max)
          max = (double)buffer[i], mxpos = i;
        if ((double)buffer[i] < min)
          min = (double)buffer[i], minpos = i;
      }
      if (-min > max) max = -min, mxpos = minpos;
      fprintf(outfile, "%.3f\t%.3f\n",
              block*window+(double)mxpos*tpersample, max/SHORTMAX);
      block++;
    }
    csound->SndfileClose(csound,infd);
    fclose(outfile);
}

/* module interface */

int32_t envext_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "envext", envext);
    if (!retval) {
      retval =
        (csound->GetUtility(csound))->SetUtilityDescription(csound, "envext",
                                      Str("Create a text file of envelope"));
    }
    return retval;
}

