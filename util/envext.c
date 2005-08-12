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
*   envext.c                                            *
*   Extract an envelope file from a sound file          *
*   jpff 11 Dec 1994                                    *
*   mainly lifted from scale and Csound itself          *
\*******************************************************/

#include "csdl.h"
#include "soundio.h"
#include <stdio.h>
#include <stdlib.h>

/* Constants */

#define SHORTMAX 32767.0
#define FIND(MSG)   if (*s == '\0')  \
        if (!(--argc) || ((s = *++argv) && *s == '-')) \
        csound->Die(csound, MSG);

/* Static function prototypes */

static SNDFILE * SCsndgetset(CSOUND *, SOUNDIN **, char *);
static void FindEnvelope(CSOUND *, SNDFILE *, SOUNDIN *, double);

static char *outname = NULL;

static void envext_usage(CSOUND *csound, char *mesg, ...)
{
    va_list args;

    csound->Message(csound,"Usage:\tenvext [-flags] soundfile\n");
    csound->Message(csound, "Legal flags are:\n");
    csound->Message(csound,"-o fnam\tsound output filename\n");
    csound->Message(csound, "-w time\tSize of window\n");
    csound->Message(csound,"flag defaults: envext -onewenv\n");
    va_start(args, mesg);
    csound->ErrMsgV(csound, Str("envext: error: "), mesg, args);
    va_end(args);
    csound->LongJmp(csound, 1);
}

static int envext(CSOUND *csound, int argc, char **argv)
{
    char        *inputfile = NULL;
    SNDFILE     *infd;
    char        c, *s;
    OPARMS      OO;
    double      window = 0.25;
    SOUNDIN     *p;  /* space allocated by SAsndgetset() */

    memset(&OO, 0, sizeof(OO));

    /* Check arguments */
    if (!(--argc))
      envext_usage(csound, "Insufficient arguments");
    do {
      s = *++argv;
      if (*s++ == '-')                        /* read all flags:  */
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
            envext_usage(csound, "unknown flag -%c", c);
          }
      else if (inputfile == NULL) {
        inputfile = --s;
      }
      else envext_usage(csound, "too many arguments");
    } while (--argc);

    /* Read sound file */
    if ((infd = SCsndgetset(csound, &p, inputfile))==NULL) {
      csound->Message(csound,"%s: error while opening %s", argv[0], inputfile);
      return 1;
    }
    FindEnvelope(csound, infd, p, window);
    return 0;
}

static SNDFILE *
SCsndgetset(CSOUND *csound, SOUNDIN **pp, char *inputfile)
{
    SNDFILE *infd;
    double  dur;
    SOUNDIN *p;

    csound->esr = FL(0.0);      /* set esr 0. with no orchestra   */
    *pp = p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    p->channel = ALLCHNLS;
    p->skiptime = FL(0.0);
    strcpy(p->sfname, inputfile);
    if ((infd = csound->sndgetset(csound, p)) == 0) /*open sndfil, do skiptime*/
      return(0);
    p->getframes = p->framesrem;
    dur = (double) p->getframes / p->sr;
    csound->Message(csound,"enveloping %ld sample frames (%3.1f secs)\n",
           (long) p->getframes, dur);
    return(infd);
}

static void
FindEnvelope(CSOUND *csound, SNDFILE *infd, SOUNDIN *p, double window)
{
    int         chans;
    double      tpersample;
    double      max, min;
    long        mxpos, minpos;
    int         block = 0;
    MYFLT       *buffer;
    int         bufferlen;
    long        read_in;
    int         i;
    FILE *      outfile;

    outfile = fopen((outname == NULL ? "newenv" : outname), "w");
    bufferlen = (int)(window*(double)p->sr);
    buffer = (MYFLT*) malloc(bufferlen*sizeof(MYFLT));
    chans = p->nchanls;
    tpersample = 1.0/(double)p->sr;
    fprintf(outfile, "%.3f\t%.3f\n", 0.0, 0.0);
    while ((read_in = csound->getsndin(csound,infd,buffer,bufferlen,p)) > 0) {
      max = 0.0;        mxpos = 0;
      min = 0.0;        minpos = 0;
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

/* module interface */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "envext", envext);
    if (!retval) {
      retval = csound->SetUtilityDescription(csound, "envext",
                                             "Create a text file of envelope");
    }
    return retval;
}

