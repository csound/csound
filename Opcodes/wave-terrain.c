/*  
    wave-terrain.c:

    Copyright (C) 2002 Matt Gilliard, John ffitch

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

#include "csdl.h"
#include "wave-terrain.h"
#include <math.h>

/*  Wave-terrain synthesis opcode
 *
 *  author: m gilliard
 *          en6mjg@bath.ac.uk
 */


int wtinit(WAVETER *p)
{
    /* DECLARE */
    int i;
    FUNC *ftpx = ftfind(p->i_tabx);
    FUNC *ftpy = ftfind(p->i_taby);

    /* CHECK */
    if ((ftpx == NULL)||(ftpy == NULL)) {
      return initerror(Str(X_1788,"wterrain: ftable not found"));
    }

/* printf("WAVE TERRAIN INIT v1.0 - terrain(%d,%d)\n", tabxlen, tabylen);  */

    /* ALLOCATE FOR COPIES OF FTABLES */
    auxalloc (ftpx->flen * sizeof(MYFLT), &p->aux_x);
    auxalloc (ftpy->flen * sizeof(MYFLT), &p->aux_y);

    /* POINT xarr AND yarr AT THE TABLES */
    p->xarr = (MYFLT*)p->aux_x.auxp;
    p->yarr = (MYFLT*)p->aux_y.auxp;

    /* COPY TABLES TO LOCAL */
    for (i=0; i != ftpx->flen; i++) {
      p->xarr[i] = (ftpx->ftable[i]);
    }

    for (i=0; i != ftpy->flen; i++) {
      p->yarr[i] = (ftpy->ftable[i]);
    }

    /* PUT SIZES INTO STRUCT FOR REFERENCE AT PERF TIME */
    p->sizx = (MYFLT)ftpx->flen;
    p->sizy = (MYFLT)ftpy->flen;
    p->theta = FL(0.0);
    return OK;
}

int wtPerf(WAVETER *p)
{
    int i;
    int xloc, yloc;
    MYFLT xc, yc;
    MYFLT amp = *p->kamp;
    MYFLT pch = *p->kpch;

    for (i=0; i<ksmps; i++) {

      /* COMPUTE LOCATION OF SCANNING POINT */
      xc = *(p->kcx) + *(p->krx) * (MYFLT)sin((double)p->theta);
      yc = *(p->kcy) + *(p->kry) * (MYFLT)cos((double)p->theta);

      /* MAP SCANNING POINT TO BE IN UNIT SQUARE */
      xc = xc-(MYFLT)floor(xc);
      yc = yc-(MYFLT)floor(yc);

      /* SCALE TO TABLE-SIZE SQUARE */
      xloc = (int)(xc * p->sizx);
      yloc = (int)(yc * p->sizy);

      /* OUTPUT AM OF TABLE VALUES * KAMP */
      p->aout[i] = p->xarr[xloc] * p->yarr[yloc] * amp;

      /* MOVE SCANNING POINT ROUND THE ELLIPSE */
      p->theta += pch * tpidsr;
    }
    return OK;
}

/* ------------------------------------------------------------ */

/*  Simple scanned synthesis hammer hit opcode
 *
 *  Like tablecopy, except the src may be smaller, and hitting position
 *  and force given.
 *
 *  author: m gilliard
 *          en6mjg@bath.ac.uk
 */


int scanhinit(SCANHAMMER *p)
{
  int srcpos = 0;
  int dstpos = (int)(*p->ipos + FL(0.5));

  FUNC *fsrc = ftfind(p->isrc); /* Source table */
  FUNC *fdst = ftfind(p->idst); /* Destination table */

  if (fsrc->flen > fdst->flen) {
    return initerror(Str(X_1789,
                  "Source table must be same size or smaller than dest table\n"));
  }

  for (srcpos=0; srcpos<fsrc->flen; srcpos++) {

    fdst->ftable[dstpos] = fsrc->ftable[srcpos] * *p->imode;

    if (++dstpos > fdst->flen) {
      dstpos = 0;
    }
  }
  return OK;
}

/* ------------------------------------------------------------ */
/*  Simple scanned synthesis opcode
 *
 *  makes a table update at k-rate for the duration of the note.
 *  also scans the system at a k-rate pitch, and k-rate amplitude.
 *
 *  author: m gilliard
 *          en6mjg@bath.ac.uk
 */


int scantinit(SCANTABLE *p)
{
    /* DECLARE */
    FUNC *fpoint = ftfind(p->i_point);
    FUNC *fmass  = ftfind(p->i_mass);
    FUNC *fstiff = ftfind(p->i_stiff);
    FUNC *fdamp  = ftfind(p->i_damp);
    FUNC *fvel   = ftfind(p->i_vel);

    /* CHECK */
    if (fpoint == NULL) {
      return initerror(Str(X_1790,"Scantable: point table not found"));
    }
    if (fmass == NULL) {
      return initerror(Str(X_1791,"Scantable: mass table not found"));
    }
    if (fstiff == NULL) {
      return initerror(Str(X_1792,"Scantable: stiffness table not found"));
    }
    if (fdamp == NULL) {
      return initerror(Str(X_1793,"Scantable: damping table not found"));
    }
    if (fvel == NULL) {
      return initerror(Str(X_1794,"Scantable: velocity table not found"));
    }

    /* CHECK ALL TABLES SAME LENGTH */
    if (!((fpoint->flen==fmass->flen) &&
          (fdamp->flen==fstiff->flen)  &&
          (fvel->flen==fstiff->flen)   &&
          (fpoint->flen==fdamp->flen))) {
      return initerror(Str(X_1795,"Table lengths do not agree!!"));
    }

    p->size = (MYFLT)fpoint->flen;

    /* ALLOCATE SPACE FOR NEW POINTS AND VELOCITIES */
    auxalloc (fpoint->flen * sizeof(MYFLT), &p->newloca);
    auxalloc (fvel->flen * sizeof(MYFLT), &p->newvela);

    /* POINT newloc AND newvel AT THE ALLOCATED SPACE */
    p->newloc = (MYFLT*)p->newloca.auxp;
    p->newvel = (MYFLT*)p->newvela.auxp;

    /* SET SCANNING POSITION */
    p->pos = 0;

/*     printf("SCANTABLE INIT v0.1\n");  */
    return OK;
}



int scantPerf(SCANTABLE *p)
{
    int i;
    MYFLT force, fc1, fc2;
    int next, last;

    /* DECLARE */
    FUNC *fpoint = ftfind(p->i_point);
    FUNC *fmass  = ftfind(p->i_mass);
    FUNC *fstiff = ftfind(p->i_stiff);
    FUNC *fdamp  = ftfind(p->i_damp);
    FUNC *fvel   = ftfind(p->i_vel);



/* CALCULATE NEW POSITIONS
 *
 * fill in newloc and newvel arrays with caluclated values
 * this is the string updating function
 *
 * a mass of zero means immovable, so as to allow fixed points
 */

    /* For all except end points  */
    for (i=0; i!=p->size; i++) {

      /* set up conditions for end-points */
      last = i - 1;
      next = i + 1;
      if (i == p->size - 1) {
        next = 0;
      }
      else if (i == 0) {
        last = (int)p->size - 1;
      }

      if (fmass->ftable[i] == 0) {
        /* if the mass is zero... */
        p->newloc[i] = fpoint->ftable[i];
        p->newvel[i] = 0;
      }
      else {
        /* update point according to scheme */
        fc1 = (fpoint->ftable[i] - fpoint->ftable[last]) * fstiff->ftable[last];
        fc2 = (fpoint->ftable[i] - fpoint->ftable[next]) * fstiff->ftable[i];
        force = fc1 + fc2;
        p->newvel[i] = (fvel->ftable[i] - force / (fmass->ftable[i] * ekr)) *
                       fdamp->ftable[i];
        p->newloc[i] = fpoint->ftable[i] + p->newvel[i] / ekr;

      }
    }

/*  printf ("p->size:\t%f\n",p->size); */

    for (i=0; i<ksmps; i++) {

      /* NO INTERPOLATION */
      p->aout[i] = fpoint->ftable[(int)(p->pos)] * *(p->kamp);

      p->pos += p->size * *(p->kpch) * onedsr;
      if (p->pos > p->size) {
        p->pos -= p->size;
      }
    }

    /* COPY NEW VALUES TO FTABLE
     *
     * replace current values with new ones
     */

    for (i=0; i<p->size; i++) {
      fpoint->ftable[i] = p->newloc[i];
      fvel->ftable[i]   = p->newvel[i];
    }
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "wterrain", S(WAVETER), 5,  "a", "kkkkkkii",(SUBR)wtinit, NULL, (SUBR)wtPerf },
{ "scantable", S(SCANTABLE),5,"a", "kkiiiii",(SUBR)scantinit,NULL,(SUBR)scantPerf},
{ "scanhammer",S(SCANHAMMER),1,"", "iiii", (SUBR)scanhinit, NULL, NULL    },
};

LINKAGE

