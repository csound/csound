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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"
#include "sysdep.h"                                 /*    EXTRACT.C   */
#include "extract.h"

extern  int     realtset(CSOUND *, SRTBLK *);
extern  MYFLT   realt(CSOUND *, MYFLT);

static  void    include(EXTRACT_STATICS*, SRTBLK *);

//#define STA(x)   (extractStatics->x)

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

static void alloc_globals(EXTRACT_STATICS* extractStatics)
{
    memcpy(&extractStatics->a0, &a0, sizeof(SRTBLK));
    memcpy(&extractStatics->f0, &f0, sizeof(SRTBLK));
    memcpy(&extractStatics->e, &e, sizeof(SRTBLK));
}

void readxfil(CSOUND *csound, EXTRACT_STATICS* extractStatics,
              FILE *xfp)    /* read the extract control file */
{
    int  flag, all;
    char s[128];

    alloc_globals(extractStatics);
    all = 1;
    flag = 'i';                                 /* default -i flag supplied */
    extractStatics->onsect = 1;
    extractStatics->onbeat = FL(0.0);   /* other default vals   */
    extractStatics->offsect = 999;  extractStatics->offbeat = FL(0.0);
    while (fscanf(xfp, "%100s", s) > 0) {
      char *c = s;
      int i;
      //printf("string: %s\n", s);
      switch (*c) {
      case 'i':
        all = 0;
        // intended no break here
        /* FALLTHRU */
      case 'f':
      case 't':
        flag = *c++;
        break;
      default:
        switch (flag) {
        case 'i':
          sscanf(s, "%d", &i);
          //printf("i: %d\n", i);
          if (LIKELY(i>=0 && i < INSMAX)) extractStatics->inslst[i] = 1;
          else csound->Message(csound, Str("instrument number out of range"));
          all = 0;
          break;
        case 'f':
          //printf("f: %s\n", s);
#if defined(USE_DOUBLE)
          CS_SSCANF(s, "%d:%lf", &extractStatics->onsect, &extractStatics->onbeat);
#else
          CS_SSCANF(s, "%d:%f", &extractStatics->onsect, &extractStatics->onbeat);
#endif
          break;
        case 't':
          //printf("t: %s\n");
          extractStatics->offsect = extractStatics->onsect; /* default offsect */
#if defined(USE_DOUBLE)
          CS_SSCANF(s, "%d:%lf", &extractStatics->offsect,&extractStatics->offbeat);
#else
          CS_SSCANF(s, "%d:%f", &extractStatics->offsect, &extractStatics->offbeat);
#endif
        }
      }
    }
    //printf("extract read\n");
    if (all) {
      char *ip;
      for (ip = &extractStatics->inslst[0];
           ip < &extractStatics->inslst[INSMAX]; *ip++ = 1);
    }
    extractStatics->ontime = extractStatics->a0.newp3 =
      extractStatics->a0.p3val = extractStatics->onbeat;
    extractStatics->offtime = extractStatics->f0.newp2 =
      extractStatics->f0.p2val = extractStatics->offbeat;
}

void extract(CSOUND *csound, EXTRACT_STATICS* extractStatics)
 /* extract instr events within the time period */
{
    SRTBLK  *bp;
    MYFLT   turnoff, anticip;
    int     warped;

    alloc_globals(extractStatics);

    if ((bp = csound->frstbp) == NULL)      /* if null file         */
      return;
    if (++extractStatics->sectno > extractStatics->offsect) {
      /* or later section,    */
      csound->frstbp = NULL;
      return;                               /*      return          */
    }

    extractStatics->frstout = extractStatics->prvout = NULL;
    if (extractStatics->sectno < extractStatics->onsect) {
      /* for sects preceding, */
      do {
        switch (bp->text[0]) {
        case 'f':                           /* include f's at time 0 */
          bp->p2val = bp->newp2 = FL(1.0);  /* time 1 for now!!     */
          include(extractStatics, bp);
          break;
        case 'w':
        case 's':
        case 'e':
          include(extractStatics, bp); /*   incl w,s,e verbatim  */
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
          if (extractStatics->sectno == extractStatics->onsect && warped)
            extractStatics->ontime = extractStatics->a0.newp3 =
              realt(csound, extractStatics->onbeat);
          if (extractStatics->sectno == extractStatics->offsect && warped)
            extractStatics->offtime = extractStatics->f0.newp2 =
              realt(csound, extractStatics->offbeat);
          include(extractStatics, bp);
          break;
        case 't':
          include(extractStatics, bp);
          break;
        case 'f':
        casef: if (extractStatics->sectno == extractStatics->onsect &&
                   bp->newp2 < extractStatics->ontime)
          bp->newp2 = extractStatics->ontime;
        else if (extractStatics->sectno == extractStatics->offsect &&
                 bp->newp2 > extractStatics->offtime)
          break;
        if (extractStatics->sectno == extractStatics->onsect &&
            !extractStatics->a0done) {
          if (extractStatics->onbeat > 0)
            include(extractStatics, &extractStatics->a0);
          extractStatics->a0done++;
        }
        include(extractStatics, bp);
        break;
        case 'i':
          if (!extractStatics->inslst[bp->insno])   /* skip insnos not required */
            break;
          if (bp->newp3 < 0)            /* treat indef dur like f */
            goto casef;
          /* FALLTHRU */
        case 'a':turnoff = bp->newp2 + bp->newp3;   /* i and a: */
          if (extractStatics->sectno == extractStatics->onsect) {
            if (turnoff < extractStatics->ontime)
              break;
            if ((anticip = extractStatics->ontime - bp->newp2) > 0) {
              if ((bp->newp3 -= anticip) < FL(0.001))
                break;
              bp->p3val -= extractStatics->onbeat - bp->p2val;
              bp->newp2 = extractStatics->ontime;
              bp->p2val = extractStatics->onbeat;
            }
          }
          if (extractStatics->sectno == extractStatics->offsect) {
            if (bp->newp2 >= extractStatics->offtime)
              break;
            if (turnoff > extractStatics->offtime) {
              bp->newp3 = extractStatics->offtime - bp->newp2;
              bp->p3val = extractStatics->offbeat - bp->p2val;
            }
          }
          if (extractStatics->sectno == extractStatics->onsect &&
              !extractStatics->a0done) {
            if (extractStatics->onbeat > 0)
              include(extractStatics, &extractStatics->a0);
            extractStatics->a0done++;
          }
          include(extractStatics, bp);
          break;
        case 's':
        case 'e':
          if (extractStatics->sectno == extractStatics->offsect) {
            include(extractStatics, &extractStatics->f0);
            include(extractStatics, &extractStatics->e);
          }
          else include(extractStatics, bp);
          break;
        }
      } while ((bp = bp->nxtblk) != NULL);
    }
    csound->frstbp = extractStatics->frstout;
    if (extractStatics->prvout != NULL)
      extractStatics->prvout->nxtblk = NULL;
}

/* wire a srtblk into the outlist */

static void include(EXTRACT_STATICS* extractStatics, SRTBLK *bp)
{
    if (extractStatics->frstout == NULL)      /* first one is special */
      extractStatics->frstout = bp;
    else extractStatics->prvout->nxtblk = bp; /* others just add onto list */
    bp->prvblk = extractStatics->prvout;      /* maintain the backptr      */
    extractStatics->prvout = bp;              /* and get ready for next    */
}
