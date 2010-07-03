/*
    extract.c:

    Copyright (C) 1991 Barry Vercoe

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

#include "csoundCore.h"
#include "sysdep.h"                                 /*    EXTRACT.C   */

#define INSMAX  4096

extern  int     realtset(CSOUND *, SRTBLK *);
extern  MYFLT   realt(CSOUND *, MYFLT);

static  void    include(CSOUND *, SRTBLK *);

typedef struct {
    char    inslst[INSMAX];         /*   values set by readxfil         */
    int     sectno, a0done;
    int     onsect, offsect;        /*      "       "       "           */
    MYFLT   onbeat, offbeat;        /*      "       "       "           */
    MYFLT   ontime, offtime;        /* set by readxfil, mod by w-stmnt  */
    SRTBLK  *frstout, *prvout;      /* links for building new outlist   */
    SRTBLK  a0;
    SRTBLK  f0;
    SRTBLK  e;
} EXTRACT_GLOBALS;

#define ST(x)   (((EXTRACT_GLOBALS*) ((CSOUND*) csound)->extractGlobals)->x)

static  const   SRTBLK a0 = {
    NULL, NULL, 0, 3, FL(0.0), FL(0.0), FL(0.0), FL(0.0), FL(0.0),
    0, SP, "a 0 0 0\n"
};

static  const   SRTBLK f0 = {
    NULL, NULL, 0, 2, FL(0.0), FL(0.0), FL(0.0), FL(0.0), FL(0.0),
    0, SP, "f 0 0\n"
};

static  const   SRTBLK e = {
    NULL, NULL, 0, 0, FL(0.0), FL(0.0), FL(0.0), FL(0.0), FL(0.0),
    0, SP, "e\n"
};

static void alloc_globals(CSOUND *csound)
{
    if (csound->extractGlobals == NULL) {
      csound->extractGlobals = csound->Calloc(csound, sizeof(EXTRACT_GLOBALS));
      ST(onbeat) = ST(offbeat) = FL(0.0);
      ST(ontime) = ST(offtime) = FL(0.0);
      memcpy(&ST(a0), &a0, sizeof(SRTBLK));
      memcpy(&ST(f0), &f0, sizeof(SRTBLK));
      memcpy(&ST(e), &e, sizeof(SRTBLK));
    }
}

void readxfil(CSOUND *csound, FILE *xfp)    /* read the extract control file */
{
    int  flag, all;
    char s[82];

    alloc_globals(csound);
    all = 1;
    flag = 'i';                                 /* default -i flag supplied */
    ST(onsect) = 1;     ST(onbeat) = FL(0.0);   /* other default vals   */
    ST(offsect) = 999;  ST(offbeat) = FL(0.0);
    while (fscanf(xfp, "%s", s) != EOF) {
      char *c = s;
      int i;
      switch (*c) {
      case 'i':
        all = 0;
      case 'f':
      case 't':
        flag = *c++;
        break;
      default:
        switch (flag) {
        case 'i':
          sscanf(s, "%d", &i);
          ST(inslst)[i] = 1;
          all = 0;
          break;
        case 'f':
#if defined(USE_DOUBLE)
          sscanf(s, "%d:%lf", &ST(onsect), &ST(onbeat));
#else
          sscanf(s, "%d:%f", &ST(onsect), &ST(onbeat));
#endif
          break;
        case 't':
          ST(offsect) = ST(onsect);       /* default offsect */
#if defined(USE_DOUBLE)
          sscanf(s, "%d:%lf", &ST(offsect), &ST(offbeat));
#else
          sscanf(s, "%d:%f", &ST(offsect), &ST(offbeat));
#endif
        }
      }
    }
    if (all) {
      char *ip;
      for (ip = &ST(inslst)[0]; ip < &ST(inslst)[INSMAX]; *ip++ = 1);
    }
    ST(ontime) = ST(a0).newp3 = ST(a0).p3val = ST(onbeat);
    ST(offtime) = ST(f0).newp2 = ST(f0).p2val = ST(offbeat);
}

void extract(CSOUND *csound) /* extract instr events within the time period */
{
    SRTBLK  *bp;
    MYFLT   turnoff, anticip;
    int     warped;

    alloc_globals(csound);

    if ((bp = csound->frstbp) == NULL)      /* if null file         */
      return;
    if (++ST(sectno) > ST(offsect)) {       /* or later section,    */
      csound->frstbp = NULL;
      return;                               /*      return          */
    }

    ST(frstout) = ST(prvout) = NULL;
    if (ST(sectno) < ST(onsect)) {          /* for sects preceding, */
      do {
        switch (bp->text[0]) {
        case 'f':                           /* include f's at time 0 */
          bp->p2val = bp->newp2 = FL(1.0);  /* time 1 for now!!     */
          include(csound, bp);
          break;
        case 'w':
        case 's':
        case 'e':
          include(csound, bp);              /*   incl w,s,e verbatim  */
          break;
        case 't':
        case 'i':
        case 'a':
          break;                            /*   but skip all t,i,a   */
        }
      } while ((bp = bp->nxtblk) != NULL);
    }
    else {                                  /* for sections in timespan: */
      do {
        switch(bp->text[0]) {
        case 'w':
          warped = realtset(csound, bp);
          if (ST(sectno) == ST(onsect) && warped)
            ST(ontime) = ST(a0).newp3 = realt(csound, ST(onbeat));
          if (ST(sectno) == ST(offsect) && warped)
            ST(offtime) = ST(f0).newp2 = realt(csound, ST(offbeat));
          include(csound, bp);
          break;
        case 't':
          include(csound, bp);
          break;
        case 'f':
        casef: if (ST(sectno) == ST(onsect) && bp->newp2 < ST(ontime))
          bp->newp2 = ST(ontime);
        else if (ST(sectno) == ST(offsect) && bp->newp2 > ST(offtime))
          break;
        if (ST(sectno) == ST(onsect) && !ST(a0done)) {
          if (ST(onbeat) > 0)
            include(csound, &ST(a0));
          ST(a0done)++;
        }
        include(csound, bp);
        break;
        case 'i':
          if (!ST(inslst)[bp->insno])   /* skip insnos not required */
            break;
          if (bp->newp3 < 0)            /* treat indef dur like f */
            goto casef;
        case 'a':turnoff = bp->newp2 + bp->newp3;   /* i and a: */
          if (ST(sectno) == ST(onsect)) {
            if (turnoff < ST(ontime))
              break;
            if ((anticip = ST(ontime) - bp->newp2) > 0) {
              if ((bp->newp3 -= anticip) < FL(0.001))
                break;
              bp->p3val -= ST(onbeat) - bp->p2val;
              bp->newp2 = ST(ontime);
              bp->p2val = ST(onbeat);
            }
          }
          if (ST(sectno) == ST(offsect)) {
            if (bp->newp2 >= ST(offtime))
              break;
            if (turnoff > ST(offtime)) {
              bp->newp3 = ST(offtime) - bp->newp2;
              bp->p3val = ST(offbeat) - bp->p2val;
            }
          }
          if (ST(sectno) == ST(onsect) && !ST(a0done)) {
            if (ST(onbeat) > 0)
              include(csound, &ST(a0));
            ST(a0done)++;
          }
          include(csound, bp);
          break;
        case 's':
        case 'e':
          if (ST(sectno) == ST(offsect)) {
            include(csound, &ST(f0));
            include(csound, &ST(e));
          }
          else include(csound, bp);
          break;
        }
      } while ((bp = bp->nxtblk) != NULL);
    }
    csound->frstbp = ST(frstout);
    if (ST(prvout) != NULL)
      ST(prvout)->nxtblk = NULL;
}

/* wire a srtblk into the outlist */

static void include(CSOUND *csound, SRTBLK *bp)
{
    if (ST(frstout) == NULL)                /* first one is special */
      ST(frstout) = bp;
    else ST(prvout)->nxtblk = bp;           /* others just add onto list */
    bp->prvblk = ST(prvout);                /* maintain the backptr      */
    ST(prvout) = bp;                        /* and get ready for next    */
}

