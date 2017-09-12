/*
    bbcut.h:

    Copyright (C) 2001 Nick Collins

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

/* mono and stereo automatic audio cutters */

/* mono version Nick Collins Aug 2 2001 */
/* added stereo Aug 10 2001 */

/* based on SuperCollider classes, latest Breakbeat5 available from
   http://www.axp.mdx.ac.uk/~nicholas15/ at time of writing some corrections
   for realtime audio, removal of random offset chance, panning of cuts and
   rest chance */

/* cmono audio in, i bps, subdiv,barlength,phrasebars,numrepeats, */
/* (optionals) p stutterspeed (defaulting to 1), o stutterchance
   (defaulting to 0=off), enveloping choice (defaulting to 1=on) */

#if !defined(__bbcut_h__)
#   define  __bbcut_h__

/* Csound ugen struct- can't use same struct for Mono and Stereo version */
typedef struct {
      OPDS  h;                              /* defined in cs.h */
  /*inputs and outputs */
  /* output first */
      MYFLT     *aout;
  /* inputs in order */
  /* a rate */
      MYFLT     *ain;
  /* i rate */
      MYFLT     *bps;
      MYFLT     *subdiv;
      MYFLT     *barlength;     /* in beats */
      MYFLT     *phrasebars;
      MYFLT     *numrepeats;
      /* optionals */
      MYFLT     *stutterspeed;  /* default 1 */
      MYFLT     *stutterchance; /* default 0 */
      MYFLT     *envelopingon;  /* default 1 */

      /* integer values */
      int Subdiv,Phrasebars,Numrepeats;
      int Stutterspeed;

  /* internal data */
  /* unsigned long int sampdone; */
      int samplesperunit;
      int repeatlengthsamp;
      int repeatsampdone;

      int numbarsnow;
      /* unitblock can be a float if stutterspeed greater than 1 */
      MYFLT unitblock,unitsleft,unitsdone;
      int totalunits;

      int repeats,repeatsdone;
      int stutteron;

      /* enveloping */
      int Envelopingon,envsize;

      /* to hold repeat data */
      AUXCH repeatbuffer;

} BBCUTMONO;

/*  STEREO VERSION */

typedef struct {
      OPDS  h;                              /*  defined in cs.h*/
      /* inputs and outputs */
      /* output first- stereo */
      MYFLT     *aout1;
      MYFLT     *aout2;
      /* inputs in order */
      /* arate, stereo ins */
      MYFLT     *ain1;
      MYFLT     *ain2;
      /* i rate */
      MYFLT     *bps;
      MYFLT     *subdiv;
      MYFLT     *barlength;     /* in beats */
      MYFLT     *phrasebars;
      MYFLT     *numrepeats;
      /* optionals */
      MYFLT     *stutterspeed;  /* default 1 */
      MYFLT     *stutterchance; /* default 0 */
      MYFLT     *envelopingon;  /* default 1 */

      /* integer values */
      int Subdiv,Phrasebars,Numrepeats;
      int Stutterspeed;

      /* internal data */
      /* unsigned long int sampdone; */
      int samplesperunit;
      int repeatlengthsamp;
      int repeatsampdone;

      int numbarsnow;
      /* unitblock can be a float if stutterspeed greater than 1 */
      MYFLT unitblock,unitsleft,unitsdone;
      int totalunits;

      int repeats,repeatsdone;
      int stutteron;

      /* enveloping */
      int Envelopingon,envsize;

      /* to hold repeat data- interleaved stereo buffer, twice size
         for mono version */
      AUXCH repeatbuffer;

} BBCUTSTEREO;

#endif /* !defined(__bbcut_h__) */

