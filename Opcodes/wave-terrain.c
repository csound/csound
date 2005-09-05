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

static int wtinit(CSOUND *csound, WAVETER *p)
{
    /* DECLARE */
    int i;
    FUNC *ftpx = csound->FTFind(csound, p->i_tabx);
    FUNC *ftpy = csound->FTFind(csound, p->i_taby);

    /* CHECK */
    if ((ftpx == NULL)||(ftpy == NULL)) {
      return csound->InitError(csound, Str("wterrain: ftable not found"));
    }

    /* ALLOCATE FOR COPIES OF FTABLES */
    csound->AuxAlloc(csound, ftpx->flen * sizeof(MYFLT), &p->aux_x);
    csound->AuxAlloc(csound, ftpy->flen * sizeof(MYFLT), &p->aux_y);

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
    p->theta = 0.0;
    return OK;
}

static int wtPerf(CSOUND *csound, WAVETER *p)
{
    int i, nsmps=csound->ksmps;
    int xloc, yloc;
    MYFLT xc, yc;
    MYFLT amp = *p->kamp;
    double pch = (double)(*p->kpch);
    MYFLT kcx = *(p->kcx), kcy = *(p->kcy);
    MYFLT krx = *(p->krx), kry = *(p->kry);
    MYFLT sizx = p->sizx, sizy = p->sizy;
    double theta = p->theta;
    double dtpidsr = (double) csound->tpidsr;

    for (i=0; i<nsmps; i++) {

      /* COMPUTE LOCATION OF SCANNING POINT */
      xc = kcx + krx * (MYFLT)sin(theta);
      yc = kcy + kry * (MYFLT)cos(theta);

      /* MAP SCANNING POINT TO BE IN UNIT SQUARE */
      xc = xc-(MYFLT)floor(xc);
      yc = yc-(MYFLT)floor(yc);

      /* SCALE TO TABLE-SIZE SQUARE */
      xloc = (int)(xc * sizx);
      yloc = (int)(yc * sizy);

      /* OUTPUT AM OF TABLE VALUES * KAMP */
      p->aout[i] = p->xarr[xloc] * p->yarr[yloc] * amp;

      /* MOVE SCANNING POINT ROUND THE ELLIPSE */
      theta += pch * dtpidsr;
    }
    p->theta = fmod(theta,TWOPI);
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

static int scanhinit(CSOUND *csound, SCANHAMMER *p)
{
  int srcpos = 0;
  int dstpos = (int)(*p->ipos + FL(0.5));

  FUNC *fsrc = csound->FTFind(csound, p->isrc); /* Source table */
  FUNC *fdst = csound->FTFind(csound, p->idst); /* Destination table */

  if (fsrc->flen > fdst->flen) {
    return csound->InitError(csound, Str(
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

static int scantinit(CSOUND *csound, SCANTABLE *p)
{
    /* DECLARE */
    FUNC *fpoint = csound->FTFind(csound, p->i_point);
    FUNC *fmass  = csound->FTFind(csound, p->i_mass);
    FUNC *fstiff = csound->FTFind(csound, p->i_stiff);
    FUNC *fdamp  = csound->FTFind(csound, p->i_damp);
    FUNC *fvel   = csound->FTFind(csound, p->i_vel);

    /* CHECK */
    if (fpoint == NULL) {
      return csound->InitError(csound, Str("Scantable: point table not found"));
    }
    if (fmass == NULL) {
      return csound->InitError(csound, Str("Scantable: mass table not found"));
    }
    if (fstiff == NULL) {
      return csound->InitError(csound,
                               Str("Scantable: stiffness table not found"));
    }
    if (fdamp == NULL) {
      return csound->InitError(csound,
                               Str("Scantable: damping table not found"));
    }
    if (fvel == NULL) {
      return csound->InitError(csound,
                               Str("Scantable: velocity table not found"));
    }

    /* CHECK ALL TABLES SAME LENGTH */
    if (!((fpoint->flen==fmass->flen) &&
          (fdamp->flen==fstiff->flen)  &&
          (fvel->flen==fstiff->flen)   &&
          (fpoint->flen==fdamp->flen))) {
      return csound->InitError(csound, Str("Table lengths do not agree!!"));
    }

    p->size = (MYFLT)fpoint->flen;

    /* ALLOCATE SPACE FOR NEW POINTS AND VELOCITIES */
    csound->AuxAlloc(csound, fpoint->flen * sizeof(MYFLT), &p->newloca);
    csound->AuxAlloc(csound, fvel->flen * sizeof(MYFLT), &p->newvela);

    /* POINT newloc AND newvel AT THE ALLOCATED SPACE */
    p->newloc = (MYFLT*)p->newloca.auxp;
    p->newvel = (MYFLT*)p->newvela.auxp;

    /* SET SCANNING POSITION */
    p->pos = 0;

    return OK;
}

static int scantPerf(CSOUND *csound, SCANTABLE *p)
{
    int i, nsmps = csound->ksmps;
    MYFLT force, fc1, fc2;
    int next, last;

    /* DECLARE */
    FUNC *fpoint = csound->FTFind(csound, p->i_point);
    FUNC *fmass  = csound->FTFind(csound, p->i_mass);
    FUNC *fstiff = csound->FTFind(csound, p->i_stiff);
    FUNC *fdamp  = csound->FTFind(csound, p->i_damp);
    FUNC *fvel   = csound->FTFind(csound, p->i_vel);
    MYFLT inc = p->size * *(p->kpch) * csound->onedsr;
    MYFLT amp = *(p->kamp);
    MYFLT pos = p->pos;

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
        p->newvel[i] = (fvel->ftable[i]
                        - force / (fmass->ftable[i] * csound->ekr))
                       * fdamp->ftable[i];
        p->newloc[i] = fpoint->ftable[i] + p->newvel[i] / csound->ekr;

      }
    }

    for (i=0; i<nsmps; i++) {

      /* NO INTERPOLATION */
      p->aout[i] = fpoint->ftable[(int)pos] * amp;

      pos += inc /* p->size * *(p->kpch) * csound->onedsr */;
      if (pos > p->size) {
        pos -= p->size;
      }
    }
    p->pos = pos;

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

