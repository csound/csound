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

#include "cs.h"
#include "sysdep.h"                                          /*    EXTRACT.C   */

#define INSMAX  256

static char    inslst[INSMAX];         /*   values set by readxfil     */
static int     onsect, offsect;        /*      "       "       "       */
static MYFLT   onbeat, offbeat;        /*      "       "       "       */
static MYFLT   ontime, offtime;        /* set by readxfil, mod by w-stmnt */

static SRTBLK *frstout, *prvout;       /* links for building new outlist */

static SRTBLK a0 = { NULL,NULL,0,3,FL(0.0),FL(0.0),FL(0.0),FL(0.0),FL(0.0),
                     SP,"a 0 0 0\n" };
static SRTBLK f0 = { NULL,NULL,0,2,FL(0.0),FL(0.0),FL(0.0),FL(0.0),FL(0.0),
                     SP,"f 0 0\n"   };
static SRTBLK e  = { NULL,NULL,0,0,FL(0.0),FL(0.0),FL(0.0),FL(0.0),FL(0.0),
                     SP,"e\n"       };

static void  include(SRTBLK *);

void readxfil(FILE *xfp)        /* read the extract control file */
{
    int  flag, all;
    char s[82];

    all = 1;
    flag = 'i';                     /* default -i flag supplied */
    onsect = 1;     onbeat = FL(0.0);    /* other default vals   */
    offsect = 999;  offbeat = FL(0.0);
    while (fscanf(xfp, "%s", s) != EOF) {
      char *c = s;
      int i;
      switch(*c) {
      case 'i':
        all = 0;
      case 'f':
      case 't':
        flag = *c++;
        break;
      default:
        switch(flag) {
        case 'i':
          sscanf(s, "%d", &i);
          inslst[i] = 1;
          all = 0;
          break;
        case 'f':
          sscanf(s, "%d:%f", &onsect, &onbeat);
          break;
        case 't':
          offsect = onsect;       /* default offsect */
          sscanf(s, "%d:%f", &offsect, &offbeat);
        }
      }
    }
    if (all) {
      char *ip;
      for (ip = &inslst[0]; ip < &inslst[INSMAX]; *ip++ = 1);
    }
    ontime = a0.newp3 = a0.p3val = onbeat;
    offtime = f0.newp2 = f0.p2val = offbeat;
}

void extract(void)      /* extract instr events within the time period */
{
    SRTBLK *bp;
    MYFLT turnoff, anticip;
    int   warped;
    static  int   sectno, a0done;
    extern  int   realtset(SRTBLK *);
    extern  MYFLT realt(MYFLT);

    if ((bp = frstbp) == NULL)              /* if null file         */
      return;
    if (++sectno > offsect) {               /* or later section,    */
      frstbp = NULL;
      return;                         /*      return          */
    }

    frstout = prvout = NULL;
    if (sectno < onsect) {                  /* for sects preceding, */
      do  switch(bp->text[0]) {
      case 'f':                 /*   include f's at time 0 */
        bp->p2val = bp->newp2 = FL(1.0);        /* time 1 for now!!*/
        include(bp);
        break;
      case 'w':
      case 's':
      case 'e':
        include(bp);            /*   incl w,s,e verbatim  */
        break;
      case 't':
      case 'i':
      case 'a':
        break;                  /*   but skip all t,i,a   */
      }
      while ((bp = bp->nxtblk) != NULL);
    }
    else {                                  /* for sections in timespan: */
      do  switch(bp->text[0]) {
      case 'w':
        warped = realtset(bp);
        if (sectno == onsect && warped)
          ontime = a0.newp3 = realt(onbeat);
        if (sectno == offsect && warped)
          offtime = f0.newp2 = realt(offbeat);
        include(bp);
        break;
      case 't':
        include(bp);
        break;
      case 'f':
      casef: if (sectno == onsect && bp->newp2 < ontime)
        bp->newp2 = ontime;
      else if (sectno == offsect && bp->newp2 > offtime)
        break;
      if (sectno == onsect && !a0done) {
        if (onbeat > 0)
          include(&a0);
        a0done++;
      }
      include(bp);
      break;
      case 'i':
        if (!inslst[bp->insno]) /* skip insnos not required */
          break;
        if (bp->newp3 < 0)      /* treat indef dur like f */
          goto casef;
      case 'a':turnoff = bp->newp2 + bp->newp3;   /* i and a: */
        if (sectno == onsect) {
          if (turnoff < ontime)
            break;
          if ((anticip = ontime - bp->newp2) > 0) {
            if ((bp->newp3 -= anticip) < FL(0.001))
              break;
            bp->p3val -= onbeat - bp->p2val;
            bp->newp2 = ontime;
            bp->p2val = onbeat;
          }
        }
        if (sectno == offsect) {
          if (bp->newp2 >= offtime)
            break;
          if (turnoff > offtime) {
            bp->newp3 = offtime - bp->newp2;
            bp->p3val = offbeat - bp->p2val;
          }
        }
        if (sectno == onsect && !a0done) {
          if (onbeat > 0)
            include(&a0);
          a0done++;
        }
        include(bp);
        break;
      case 's':
      case 'e':
        if (sectno == offsect) {
          include(&f0);
          include(&e);
        }
        else include(bp);
        break;
      }
      while ((bp = bp->nxtblk) != NULL);
    }
    frstbp = frstout;
    if (prvout != NULL)
      prvout->nxtblk = NULL;
}

static void include(SRTBLK *bp) /* wire a srtblk into the outlist */
{
    if (frstout == NULL)                    /* first one is special */
      frstout = bp;
    else prvout->nxtblk = bp;               /* others just add onto list */
    bp->prvblk = prvout;                    /* maintain the backptr      */
    prvout = bp;                            /* and get ready for next    */
}
