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
#include "extract.h"

extern  int     realtset(CSOUND *, SRTBLK *);
extern  MYFLT   realt(CSOUND *, MYFLT);

static  void    include(CSOUND *, EXTRACT_STATICS*, SRTBLK *);

#define STA(x)   (extractStatics->x)

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

static void alloc_globals(CSOUND *csound, EXTRACT_STATICS* extractStatics)
{
/* if (csound->extractGlobals == NULL) { */
/*   csound->extractGlobals = csound->Calloc(csound, sizeof(EXTRACT_GLOBALS)); */
      /* STA(onbeat) = STA(offbeat) = FL(0.0); */
      /* STA(ontime) = STA(offtime) = FL(0.0); */
      memcpy(&STA(a0), &a0, sizeof(SRTBLK));
      memcpy(&STA(f0), &f0, sizeof(SRTBLK));
      memcpy(&STA(e), &e, sizeof(SRTBLK));
    /* } */
}

void readxfil(CSOUND *csound, EXTRACT_STATICS* extractStatics,
              FILE *xfp)    /* read the extract control file */
{
    int  flag, all;
    char s[82];

    alloc_globals(csound, extractStatics);
    all = 1;
    flag = 'i';                                 /* default -i flag supplied */
    STA(onsect) = 1;     STA(onbeat) = FL(0.0);   /* other default vals   */
    STA(offsect) = 999;  STA(offbeat) = FL(0.0);
    //    while (fscanf(xfp, s) != EOF) {
     while (fscanf(xfp, "%s", s) != EOF) {
       //while (fgets(s, 82, xfp) != NULL) {
      char *c = s;
      int i;
      switch (*c) {
      case 'i':
        all = 0;
        // intended no break here
      case 'f':
      case 't':
        flag = *c++;
        break;
      default:
        switch (flag) {
        case 'i':
          sscanf(s, "%d", &i);
          //printf("%s %d\n", s, i);
          if (i>=0 && i < INSMAX) STA(inslst)[i] = 1;
          else csound->Die(csound, Str("instrument number out of range"));
          all = 0;
          break;
        case 'f':
#if defined(USE_DOUBLE)
          CS_SSCANF(s, "%d:%lf", &STA(onsect), &STA(onbeat));
#else
          CS_SSCANF(s, "%d:%f", &STA(onsect), &STA(onbeat));
#endif
          break;
        case 't':
          STA(offsect) = STA(onsect);       /* default offsect */
#if defined(USE_DOUBLE)
          CS_SSCANF(s, "%d:%lf", &STA(offsect), &STA(offbeat));
#else
          CS_SSCANF(s, "%d:%f", &STA(offsect), &STA(offbeat));
#endif
        }
      }
    }
    if (all) {
      char *ip;
      for (ip = &STA(inslst)[0]; ip < &STA(inslst)[INSMAX]; *ip++ = 1);
    }
    STA(ontime) = STA(a0).newp3 = STA(a0).p3val = STA(onbeat);
    STA(offtime) = STA(f0).newp2 = STA(f0).p2val = STA(offbeat);
}

void extract(CSOUND *csound, EXTRACT_STATICS* extractStatics)
 /* extract instr events within the time period */
{
    SRTBLK  *bp;
    MYFLT   turnoff, anticip;
    int     warped;

    alloc_globals(csound, extractStatics);

    if ((bp = csound->frstbp) == NULL)      /* if null file         */
      return;
    if (++STA(sectno) > STA(offsect)) {       /* or later section,    */
      csound->frstbp = NULL;
      return;                               /*      return          */
    }

    STA(frstout) = STA(prvout) = NULL;
    if (STA(sectno) < STA(onsect)) {          /* for sects preceding, */
      do {
        switch (bp->text[0]) {
        case 'f':                           /* include f's at time 0 */
          bp->p2val = bp->newp2 = FL(1.0);  /* time 1 for now!!     */
          include(csound, extractStatics, bp);
          break;
        case 'w':
        case 's':
        case 'e':
          include(csound, extractStatics, bp); /*   incl w,s,e verbatim  */
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
          if (STA(sectno) == STA(onsect) && warped)
            STA(ontime) = STA(a0).newp3 = realt(csound, STA(onbeat));
          if (STA(sectno) == STA(offsect) && warped)
            STA(offtime) = STA(f0).newp2 = realt(csound, STA(offbeat));
          include(csound, extractStatics, bp);
          break;
        case 't':
          include(csound, extractStatics, bp);
          break;
        case 'f':
        casef: if (STA(sectno) == STA(onsect) && bp->newp2 < STA(ontime))
          bp->newp2 = STA(ontime);
        else if (STA(sectno) == STA(offsect) && bp->newp2 > STA(offtime))
          break;
        if (STA(sectno) == STA(onsect) && !STA(a0done)) {
          if (STA(onbeat) > 0)
            include(csound, extractStatics, &STA(a0));
          STA(a0done)++;
        }
        include(csound, extractStatics, bp);
        break;
        case 'i':
          if (!STA(inslst)[bp->insno])   /* skip insnos not required */
            break;
          if (bp->newp3 < 0)            /* treat indef dur like f */
            goto casef;
        case 'a':turnoff = bp->newp2 + bp->newp3;   /* i and a: */
          if (STA(sectno) == STA(onsect)) {
            if (turnoff < STA(ontime))
              break;
            if ((anticip = STA(ontime) - bp->newp2) > 0) {
              if ((bp->newp3 -= anticip) < FL(0.001))
                break;
              bp->p3val -= STA(onbeat) - bp->p2val;
              bp->newp2 = STA(ontime);
              bp->p2val = STA(onbeat);
            }
          }
          if (STA(sectno) == STA(offsect)) {
            if (bp->newp2 >= STA(offtime))
              break;
            if (turnoff > STA(offtime)) {
              bp->newp3 = STA(offtime) - bp->newp2;
              bp->p3val = STA(offbeat) - bp->p2val;
            }
          }
          if (STA(sectno) == STA(onsect) && !STA(a0done)) {
            if (STA(onbeat) > 0)
              include(csound, extractStatics, &STA(a0));
            STA(a0done)++;
          }
          include(csound, extractStatics, bp);
          break;
        case 's':
        case 'e':
          if (STA(sectno) == STA(offsect)) {
            include(csound, extractStatics, &STA(f0));
            include(csound, extractStatics, &STA(e));
          }
          else include(csound, extractStatics, bp);
          break;
        }
      } while ((bp = bp->nxtblk) != NULL);
    }
    csound->frstbp = STA(frstout);
    if (STA(prvout) != NULL)
      STA(prvout)->nxtblk = NULL;
}

/* wire a srtblk into the outlist */

static void include(CSOUND *csound, EXTRACT_STATICS* extractStatics, SRTBLK *bp)
{
    if (STA(frstout) == NULL)                /* first one is special */
      STA(frstout) = bp;
    else STA(prvout)->nxtblk = bp;           /* others just add onto list */
    bp->prvblk = STA(prvout);                /* maintain the backptr      */
    STA(prvout) = bp;                        /* and get ready for next    */
}

