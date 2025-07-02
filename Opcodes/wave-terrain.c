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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/


#include "wave-terrain.h"
#include <math.h>

/*  Wave-terrain synthesis opcode
 *
 *  author: m gilliard
 *          en6mjg@bath.ac.uk
 */

static int32_t wtinit(CSOUND *csound, WAVETER *p)
{
    /* DECLARE */
    FUNC *ftpx = csound->FTFind(csound, p->i_tabx);
    FUNC *ftpy = csound->FTFind(csound, p->i_taby);

    /* CHECK */
    if (UNLIKELY((ftpx == NULL)||(ftpy == NULL))) {
      return csound->InitError(csound, "%s", Str("wterrain: ftable not found"));
    }

    /* POINT xarr AND yarr AT THE TABLES */
    p->xarr = ftpx->ftable;
    p->yarr = ftpy->ftable;

    /* PUT SIZES INTO STRUCT FOR REFERENCE AT PERF TIME */
    p->sizx = (MYFLT)ftpx->flen;
    p->sizy = (MYFLT)ftpy->flen;
    p->theta = 0.0;
    return OK;
}

static int32_t wtPerf(CSOUND *csound, WAVETER *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t xloc, yloc;
    MYFLT xc, yc;
    MYFLT amp = *p->kamp;
    MYFLT pch = *p->kpch;
    MYFLT kcx = *(p->kcx), kcy = *(p->kcy);
    MYFLT krx = *(p->krx), kry = *(p->kry);
    MYFLT sizx = p->sizx, sizy = p->sizy;
    MYFLT theta = p->theta;
    MYFLT dtpidsr = CS_TPIDSR;
    MYFLT *aout = p->aout;

    if (UNLIKELY(offset)) memset(aout, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i=offset; i<nsmps; i++) {

      /* COMPUTE LOCATION OF SCANNING POINT */
      xc = kcx + krx * SIN(theta);
      yc = kcy + kry * COS(theta);

      /* MAP SCANNING POINT TO BE IN UNIT SQUARE */
      xc = xc-FLOOR(xc);
      yc = yc-FLOOR(yc);

      /* SCALE TO TABLE-SIZE SQUARE */
      xloc = (int32_t)(xc * sizx);
      yloc = (int32_t)(yc * sizy);

      /* OUTPUT AM OF TABLE VALUES * KAMP */
      aout[i] = p->xarr[xloc] * p->yarr[yloc] * amp;

      /* MOVE SCANNING POINT ROUND THE ELLIPSE */
      theta += pch * dtpidsr;
    }
    p->theta = FMOD(theta,TWOPI_F);
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

static int32_t scanhinit(CSOUND *csound, SCANHAMMER *p)
{
  uint32_t srcpos = 0;
  uint32_t dstpos = (uint32_t)MYFLT2LONG(*p->ipos);

  FUNC *fsrc = csound->FTFind(csound, p->isrc); /* Source table */
  FUNC *fdst = csound->FTFind(csound, p->idst); /* Destination table */

  if (UNLIKELY(fsrc->flen > fdst->flen)) {
    return csound->InitError(csound,  "%s",  Str("Source table must be same size or "
                                         "smaller than dest table\n"));
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

static int32_t scantinit(CSOUND *csound, SCANTABLE *p)
{
    /* DECLARE */
    FUNC *fpoint = csound->FTFind(csound, p->i_point);
    FUNC *fmass  = csound->FTFind(csound, p->i_mass);
    FUNC *fstiff = csound->FTFind(csound, p->i_stiff);
    FUNC *fdamp  = csound->FTFind(csound, p->i_damp);
    FUNC *fvel   = csound->FTFind(csound, p->i_vel);

    /* CHECK */
    if (UNLIKELY(fpoint == NULL)) {
      return csound->InitError(csound, "%s",
                               Str("Scantable: point table not found"));
    }
    if (UNLIKELY(fmass == NULL)) {
      return csound->InitError(csound, "%s",
                               Str("Scantable: mass table not found"));
    }
    if (UNLIKELY(fstiff == NULL)) {
      return csound->InitError(csound, "%s",
                               Str("Scantable: stiffness table not found"));
    }
    if (UNLIKELY(fdamp == NULL)) {
      return csound->InitError(csound, "%s",
                               Str("Scantable: damping table not found"));
    }
    if (UNLIKELY(fvel == NULL)) {
      return csound->InitError(csound, "%s",
                               Str("Scantable: velocity table not found"));
    }

    /* CHECK ALL TABLES SAME LENGTH */
    if (UNLIKELY(!((fpoint->flen==fmass->flen) &&
          (fdamp->flen==fstiff->flen)  &&
          (fvel->flen==fstiff->flen)   &&
                   (fpoint->flen==fdamp->flen)))) {
      return csound->InitError(csound, "%s", Str("Table lengths do not agree!!"));
    }

    p->fpoint = fpoint;
    p->fmass  = fmass;
    p->fstiff = fstiff;
    p->fdamp  = fdamp;
    p->fvel   = fvel;

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

static int32_t scantPerf(CSOUND *csound, SCANTABLE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    MYFLT force, fc1, fc2;
    int32_t next, last;

    /* DECLARE */
    FUNC *fpoint = p->fpoint;
    FUNC *fmass  = p->fmass;
    FUNC *fstiff = p->fstiff;
    FUNC *fdamp  = p->fdamp;
    FUNC *fvel   = p->fvel;
    MYFLT inc    = p->size * *(p->kpch) * CS_ONEDSR;
    MYFLT amp    = *(p->kamp);
    MYFLT pos    = p->pos;
    MYFLT *aout  = p->aout;

/* CALCULATE NEW POSITIONS
 *
 * fill in newloc and newvel arrays with calculated values
 * this is the string updating function
 *
 * a mass of zero means immovable, so as to allow fixed points
 */

    /* For all except end points  */
    for (i=0; i!=p->size; i++) {

      /* set up conditions for end-points */
      last = i - 1;
      next = i + 1;
      if (UNLIKELY(i == p->size - 1)) {
        next = 0;
      }
      else if (UNLIKELY(i == 0)) {
        last = (int32_t)p->size - 1;
      }

      if (UNLIKELY(fmass->ftable[i] == 0)) {
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
                        - force / (fmass->ftable[i] * CS_EKR))
                       * fdamp->ftable[i];
        p->newloc[i] = fpoint->ftable[i] + p->newvel[i] * CS_ONEDKR;

      }
    }

    if (UNLIKELY(offset)) memset(aout, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i=offset; i<nsmps; i++) {

      /* NO INTERPOLATION */
      aout[i] = fpoint->ftable[(int32_t)pos] * amp;

      pos += inc /* p->size * *(p->kpch) * CS_ONEDSR */;
      if (UNLIKELY(pos > p->size)) {
        pos -= p->size;
      }
    }
    p->pos = pos;

    /* COPY NEW VALUES TO FTABLE
     *
     * replace current values with new ones
     */
    memcpy(fpoint->ftable, p->newloc, p->size*sizeof(MYFLT));
    memcpy(fvel->ftable, p->newvel, p->size*sizeof(MYFLT));
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "wterrain", S(WAVETER), TR,   "a", "kkkkkkii",
    (SUBR)wtinit, (SUBR)wtPerf },
  { "scantable", S(SCANTABLE),TR, "a", "kkiiiii",
    (SUBR)scantinit,(SUBR)scantPerf},
  { "scanhammer",S(SCANHAMMER),TB, "", "iiii", (SUBR)scanhinit, NULL, NULL    }
};

int32_t wave_terrain_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
