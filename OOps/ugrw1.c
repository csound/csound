/*
    ugrw1.c:

    Copyright (C) 1997 Robin Whittle

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

/* These files are based on Robin Whittle's
 *       ugrw1.c of 27 August 1996
 * and   ugrw1.h of 7 January 1995
 *
 * In February 1997, John Fitch reformatted the comments and
 * cleaned up some code in printksset() - which was working fine
 * but was inelegantly coded.
 * In February 1998, John Fitch modified the code wrt types so it
 * compiled with MicroSoft C without warnings.
 *
 *
 * Copyright notice - Robin Whittle  25 February 1997
 *
 * Documentation files, and the original .c and .h files, with more
 * spaced out comments, are available from http://www.firstpr.com.au
 *
 * The code in both ugrw1 and ugrw2 is copyright Robin Whittle.
 * Permission is granted to use this in whole or in part for any
 * purpose, provided this copyright notice remains intact and
 * any alterations to the source code, including comments, are
 * clearly indicated as such.
 */

#include "csoundCore.h"
#include "ugrw1.h"
#include <math.h>
#include <ctype.h>

/*****************************************************************************/
/*****************************************************************************/

/* The zak system - patching i, k and a rate signals in a global set of
 * patch points - one set for i and k,* the other for a rate.
 * See doco at the start of this file.
 */

/* There are four global variables which are used by these ugens. */

/* Starting addresses of zk and za spaces */
/* MYFLT   *zkstart = NULL, *zastart = NULL;  */
/* Number of the last location in zk/za space */
/* int64_t    zklast = 0, zalast = 0; */
/* There are currently no limits on the size of these spaces.  */

/* zakinit is an opcode which must be called once to reserve the memory
 * for zk and za spaces.
 */
int32_t zakinit(CSOUND *csound, ZAKINIT *p)
{
    int32_t    length;

    /* Check to see this is the first time zakinit() has been called.
     * Global variables will be zero if it has not been called.     */

    if (UNLIKELY((csound->zkstart != NULL) || (csound->zastart != NULL))) {
      return csound->InitError(csound,
                               Str("zakinit should only be called once."));
    }

    if (UNLIKELY((*p->isizea <= 0) || (*p->isizek <= 0))) {
      return csound->InitError(csound, Str("zakinit: both isizea and isizek "
                                           "should be > 0."));
    }
    /* Allocate memory for zk space.
     * This is all set to 0 and there will be an error report if the
     * memory cannot be allocated. */

    csound->zklast = (int32_t) *p->isizek;
    length = (csound->zklast + 1L) * sizeof(MYFLT);

    csound->zkstart = (MYFLT*) csound->Calloc(csound, length);

    /* Likewise, allocate memory for za space, but do it in arrays of
     * length ksmps.
     * This is all set to 0 and there will be an error report if the
     * memory cannot be allocated.       */
    csound->zalast = (int32_t) *p->isizea;

    length = (csound->zalast + 1L) * sizeof(MYFLT) * CS_KSMPS;
    csound->zastart = (MYFLT*) csound->Calloc(csound, length);
    return OK;
}

/*---------------------------------------------------------------------------*/

/* I and K rate zak code. */

/* zkset() is called at the init time of the instance of the zir, zkr
 * zir and ziw ugens.  It complains if zk space has not been allocated yet.
 */
int32_t zkset(CSOUND *csound, ZKR *p)
{
    (void) p;
    if (UNLIKELY(csound->zkstart == NULL)) {
      return csound->InitError(csound, Str("No zk space: "
                                           "zakinit has not been called yet."));
    }
    return OK;
}

/*-----------------------------------*/

/* k rate READ code. */

/* zkr reads from zk space at k rate. */
int32_t zkr(CSOUND *csound, ZKR *p)
{
    int32_t    indx;

    /* Check to see this index is within the limits of zk space. */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > csound->zklast)) {
      *p->rslt = FL(0.0);
      csound->Warning(csound, Str("zkr index > isizek. Returning 0."));
    }
    else if (UNLIKELY(indx < 0)) {
      *p->rslt = FL(0.0);
      csound->Warning(csound, Str("zkr index < 0. Returning 0."));
    }
    else {
      MYFLT *readloc;
      /* Now read from the zk space and write to the destination. */
      readloc = csound->zkstart + indx;
      *p->rslt = *readloc;
    }
    return OK;
}

/*-----------------------------------*/

/* zir reads from zk space, but only  at init time.
 *
 * Call zkset() to check that zk space has been allocated, then do
 * similar code to zkr() above, except with csoundInitError() instead of
 * csoundPerfError(). */
int32_t zir(CSOUND *csound, ZKR *p)
{
    /* See zkr() for more comments.  */
    int32_t    indx;

    if (UNLIKELY(zkset(csound, p) != OK))
      return NOTOK;
    /* Check to see this index is within the limits of zk space. */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > csound->zklast)) {
      csound->Warning(csound, Str("zir index > isizek. Returning 0."));
      *p->rslt = FL(0.0);
    }
    else if (UNLIKELY(indx < 0)) {
      csound->Warning(csound, Str("zir index < 0. Returning 0."));
      *p->rslt = FL(0.0);
    }
    else {
      MYFLT *readloc;
      /* Now read from the zk space. */
      readloc = csound->zkstart + indx;
      *p->rslt = *readloc;
    }
    return OK;
}

/*-----------------------------------*/

/* Now the i and k rate WRITE code.  zkw writes to zk space at k rate. */
int32_t zkw(CSOUND *csound, ZKW *p)
{
    int32_t    indx;

    /* Check to see this index is within the limits of zk space. */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > csound->zklast)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zkw index > isizek. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zkw index < 0. Not writing."));
    }
    else {
      MYFLT *writeloc;
      /* Now write to the appropriate location in zk space.  */
      writeloc = csound->zkstart + indx;
      *writeloc = *p->sig;
    }
    return OK;
}

/*-----------------------------------*/

/* ziw writes to zk space, but only at init time.
 *
 * Call zkset() to check that zk space has been allocated, then use
 * same code as zkw() except that errors go to csoundInitError().  */
int32_t ziw(CSOUND *csound, ZKW *p)
{
    int32_t    indx;

    if (UNLIKELY(zkset(csound, (ZKR*) p) != OK))
      return NOTOK;
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > csound->zklast)) {
      return csound->InitError(csound, Str("ziw index > isizek. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->InitError(csound, Str("ziw index < 0. Not writing."));
    }
    else {
      MYFLT *writeloc;
      /* Now write to the appropriate location in zk space. */
      writeloc = csound->zkstart + indx;
      *writeloc = *p->sig;
    }
    return OK;
}

/*-----------------------------------*/

/* i and k rate zk WRITE code, with a mix option. */

/* zkwm writes to zk space at k rate. */
int32_t zkwm(CSOUND *csound, ZKWM *p)
{
    int32_t    indx;

    /* Check to see this index is within the limits of zk space.   */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > csound->zklast)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zkwm index > isizek. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zkwm index < 0. Not writing."));
    }
    else {
      MYFLT *writeloc;
      /* Now write to the appropriate location in zk space.  */
      writeloc = csound->zkstart + indx;
      /* If mix parameter is 0, then overwrite the data in the
       * zk space variable, otherwise read the old value, and write
       * the sum of it and the input sig.    */
      if (*p->mix == 0)
        *writeloc = *p->sig;
      else
        *writeloc += *p->sig;
    }
    return OK;
}

/*-----------------------------------*/

/* ziwm writes to zk space, but only at init time - with a mix option.
 *
 * Call zkset() to check that zk space has been allocated, then run
 * similar code to zkwm() to do the work - but with errors to csoundInitError().
 */
int32_t ziwm(CSOUND *csound, ZKWM *p)
{
    int32_t    indx;

    if (UNLIKELY(zkset(csound, (ZKR*) p) != OK))
      return NOTOK;
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > csound->zklast)) {
      return csound->InitError(csound,
                               Str("ziwm index > isizek. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->InitError(csound, Str("ziwm index < 0. Not writing."));
    }
    else {
      MYFLT *writeloc;
      writeloc = csound->zkstart + indx;
      if (*p->mix == 0)
        *writeloc = *p->sig;
      else
        *writeloc += *p->sig;
    }
    return OK;
}

/*-----------------------------------*/

/* k rate ZKMOD subroutine.      */
int32_t zkmod(CSOUND *csound, ZKMOD *p)
{
    MYFLT *readloc;
    int32_t indx;
    int32_t mflag = 0;    /* set to true if should do the modulation with
                         multiplication rather than addition.    */

    /* If zkmod = 0, then just copy input to output. We want to make
     * this as fast as possible, because in many instances, this will be
     * the case.
     *
     * Note that in converting the zkmod index into a long, we want
     * the normal conversion rules to apply to negative numbers -
     * so -2.3 is converted to -2.                               */

    if ((indx = (int32_t)*p->zkmod) == 0) {
      *p->rslt = *p->sig;
      return OK;
    }
    /* Decide whether index is positive or negative. Make it postive. */
    if (UNLIKELY(indx < 0)) {
      indx = - indx;
      mflag = 1;
    }
    /* Check to see this index is within the limits of zk space. */

    if (UNLIKELY(indx > csound->zklast)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zkmod kzkmod > isizek. Not writing."));
    }
    else {
      /* Now read the value from zk space. */
      readloc = csound->zkstart + indx;
      /* If mflag is 0, then add the modulation factor. Otherwise multiply it.*/
      if (mflag == 0)
        *p->rslt = *p->sig + *readloc;
      else
        *p->rslt = *p->sig * *readloc;
    }
    return OK;
}

/*-----------------------------------*/

/* zkcl clears a range of variables in zk space at k rate.       */
int32_t zkcl(CSOUND *csound, ZKCL *p)
{
    MYFLT       *writeloc;
    int32_t first = (int32_t) *p->first, last = (int32_t) *p->last, loopcount;

    /* Check to see both kfirst and klast are within the limits of zk space
     * and that last is >= first.                */
    if (UNLIKELY((first > csound->zklast) || (last > csound->zklast)))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zkcl first or last > isizek. Not clearing."));
    else if (UNLIKELY((first < 0) || (last < 0))) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zkcl first or last < 0. Not clearing."));
    }
    else if (UNLIKELY(first > last)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zkcl first > last. Not clearing."));
    }
    else {
      /* Now clear the appropriate locations in zk space. */
      loopcount = last - first + 1;
      writeloc = csound->zkstart + first;
      memset(writeloc, 0, loopcount*sizeof(MYFLT));
    }
    return OK;
}

/*---------------------------------------------------------------------------*/

/* AUDIO rate zak code.
 */

/* zaset() is called at the init time of the instance of the zar or zaw ugens.
 * All it has to do is spit the dummy if za space has not been allocated yet.
 */
int32_t zaset(CSOUND *csound, ZAR *p)
{
    IGN(p);
    if  (csound->zastart == NULL) {
      return csound->InitError(csound, Str("No za space: "
                                           "zakinit has not been called yet."));
    }
    else
      return (OK);
}

/*-----------------------------------*/

/* a rate READ code. */

/* zar reads from za space at a rate. */
int32_t zar(CSOUND *csound, ZAR *p)
{
    MYFLT       *readloc, *writeloc;
    int32_t indx;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    /*-----------------------------------*/

    writeloc = p->rslt;

    /* Check to see this index is within the limits of za space.    */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > csound->zalast)) {
      memset(writeloc, 0, nsmps*sizeof(MYFLT));
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zar index > isizea. Returning 0."));
    }
    else if (UNLIKELY(indx < 0)) {
      memset(writeloc, 0, nsmps*sizeof(MYFLT));
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zar index < 0. Returning 0."));
    }
    else {
      /* Now read from the array in za space and write to the destination.
       * See notes in zkr() on pointer arithmetic.     */
      readloc = csound->zastart + (indx * CS_KSMPS);
      if (UNLIKELY(offset)) memset(writeloc, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&writeloc[nsmps], '\0', early*sizeof(MYFLT));
    }
    memcpy(&writeloc[offset], &readloc[offset], (nsmps-offset)*sizeof(MYFLT));

    }
    return OK;
}

/*-----------------------------------*/

/* zarg() reads from za space at audio rate, with gain controlled by a
 * k rate variable. Code is almost identical to zar() above. */
int32_t zarg(CSOUND *csound, ZARG *p)
{
    MYFLT       *readloc, *writeloc;
    MYFLT       kgain;          /* Gain control */
    int32_t        indx;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    /*-----------------------------------*/

    writeloc = p->rslt;
    kgain = *p->kgain;

    /* Check to see this index is within the limits of za space.    */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > csound->zalast)) {
      memset(writeloc, 0, nsmps*sizeof(MYFLT));
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zarg index > isizea. Returning 0."));
    }
    else {
      if (UNLIKELY(indx < 0)) {
        memset(writeloc, 0, nsmps*sizeof(MYFLT));
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("zarg index < 0. Returning 0."));
      }
      else {
        /* Now read from the array in za space multiply by kgain and write
         * to the destination.       */
        readloc = csound->zastart + (indx * CS_KSMPS);
        if (UNLIKELY(offset)) memset(writeloc, '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          nsmps -= early;
          memset(&writeloc[nsmps], '\0', early*sizeof(MYFLT));
        }
        for (n=offset; n<nsmps; n++) {
          writeloc[n] = readloc[n] * kgain;
        }
      }
    }
    return OK;
}

/*-----------------------------------*/

/* a rate WRITE code. */

/* zaw writes to za space at a rate. */
int32_t zaw(CSOUND *csound, ZAW *p)
{
    MYFLT       *readloc, *writeloc;
    int32_t indx;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    /* Set up the pointer for the source of data to write.    */
    readloc = p->sig;
    /* Check to see this index is within the limits of za space.     */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > csound->zalast)) {
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("zaw index > isizea. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("zaw index < 0. Not writing."));
    }
    else {
        /* Now write to the array in za space pointed to by indx.    */
      writeloc = csound->zastart + (indx * CS_KSMPS);
      if (UNLIKELY(offset)) memset(writeloc, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&writeloc[nsmps], '\0', early*sizeof(MYFLT));
      }
      memcpy(&writeloc[offset], &readloc[offset], (nsmps-offset)*sizeof(MYFLT));
    }
    return OK;
}

/*-----------------------------------*/

/* a rate WRITE code with mix facility. */

/* zawm writes to za space at a rate. */
int32_t zawm(CSOUND *csound, ZAWM *p)
{
    MYFLT       *readloc, *writeloc;
    int32_t indx;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    /*-----------------------------------*/

    /* Set up the pointer for the source of data to write. */

    readloc = p->sig;
    /* Check to see this index is within the limits of za space.    */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > csound->zalast)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zaw index > isizea. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zaw index < 0. Not writing."));
    }
    else {
      /* Now write to the array in za space pointed to by indx.    */
      writeloc = csound->zastart + (indx * CS_KSMPS);
      if (*p->mix == 0) {
        /* Normal write mode.  */
        if (UNLIKELY(offset)) memset(writeloc, '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          nsmps -= early;
          memset(&writeloc[nsmps], '\0', early*sizeof(MYFLT));
        }
        memcpy(&writeloc[offset], &readloc[offset], (nsmps-offset)*sizeof(MYFLT));
      }
      else {
        /* Mix mode - add to the existing value.   */
        if (UNLIKELY(early)) nsmps -= early;
        for (n=offset; n<nsmps; n++) {
            writeloc[n] += readloc[n];
        }
      }
    }
    return OK;
}

/*-----------------------------------*/

/* audio rate ZAMOD subroutine.
 *
 * See zkmod() for fuller explanation of code.
 */
int32_t zamod(CSOUND *csound, ZAMOD *p)
{
    MYFLT       *writeloc, *readloc;
    MYFLT       *readsig;       /* Array of input floats */
    int32_t indx;
    int32_t mflag = 0;             /* non zero if modulation with multiplication  */
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    /* Make a local copy of the pointer to the input signal, so we can auto-
     * increment it. Likewise the location to write the result to.     */
    readsig = p->sig;
    writeloc = p->rslt;
    if (UNLIKELY(offset)) memset(writeloc, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&writeloc[nsmps], '\0', early*sizeof(MYFLT));
    }
    /* If zkmod = 0, then just copy input to output.    */
    if ((indx = (int32_t) *p->zamod) == 0) {
      memcpy(&writeloc[offset], &readsig[offset], (nsmps-offset)*sizeof(MYFLT));
      return OK;
    }
    /* Decide whether index is positive or negative.  Make it postive.    */
    if (indx < 0) {
      indx = - indx;
      mflag = 1;
    }
    /* Check to see this index is within the limits of za space.    */
    if (UNLIKELY(indx > csound->zalast)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zamod kzamod > isizea. Not writing."));
    }
    else {                      /* Now read the values from za space.    */
      readloc = csound->zastart + (indx * CS_KSMPS);
      if (UNLIKELY(early)) nsmps -= early;
      if (mflag == 0) {
        for (n=offset; n<nsmps; n++) {
          writeloc[n] = readsig[n] + readloc[n];
        }
      }
      else {
        for (n=offset; n<nsmps; n++) {
          writeloc[n] = readsig[n] * readloc[n];
        }
      }
    }
    return OK;
}

/*-----------------------------------*/

/* zacl clears a range of variables in za space at k rate. */
int32_t zacl(CSOUND *csound, ZACL *p)
{
    MYFLT       *writeloc;
    int32_t first, last, loopcount;

    first = (int32_t) *p->first;
    last  = (int32_t) *p->last;
    /* Check to see both kfirst and klast are within the limits of za space
     * and that last is >= first.    */
    if (UNLIKELY((first > csound->zalast) || (last > csound->zalast)))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("zacl first or last > isizea. Not clearing."));
    else {
      if (UNLIKELY((first < 0) || (last < 0))) {
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("zacl first or last < 0. Not clearing."));
      }
      else {
        if (UNLIKELY(first > last)) {
          return csound->PerfError(csound, p->h.insdshead,
                                   Str("zacl first > last. Not clearing."));
        }
        else {  /* Now clear the appropriate locations in za space. */
          loopcount = (last - first + 1) * CS_KSMPS;
          writeloc = csound->zastart + (first * CS_KSMPS);
          memset(writeloc, 0, loopcount*sizeof(MYFLT));
        }
      }
    }
    return OK;
}

/*****************************************************************************/
/*****************************************************************************/

/* Subroutines for reading absolute time. */

/* timek()
 *
 * Called at i rate or k rate, by timek, itimek, timesr or itemes.
 *
 * This is based on global variable kcounter in insert.c.
 * Since is apparently is not declared in a header file, we must declare it
 * an external.
 * Actually moved to the glob structure -- JPff march 2002
 */

int32_t timek(CSOUND *csound, RDTIME *p)
{
    IGN(csound);
    /* Read the global variable kcounter and turn it into a float.   */
    *p->rslt = (MYFLT) CS_KCNT;
    return OK;
}

/* timesr() */
int32_t timesr(CSOUND *csound, RDTIME *p)
{
    /* Read the global variable kcounter divide it by the k rate.    */
    IGN(csound);
    *p->rslt = (MYFLT) CS_KCNT * CS_ONEDKR;
    return OK;
}

/*-----------------------------------*/

/* Subroutines to read time for this instance of the instrument. */

/* instimset() runs at init time and keeps a record of the time then
 * in the RDTIME data structure.
 * Returns 0.
 */
int32_t instimset(CSOUND *csound, RDTIME *p)
{
   IGN(csound);
    p->instartk = CS_KCNT;
    *p->rslt = FL(0.0);
    return OK;
}

/* instimek()
 *
 * Read difference between the global variable kcounter and the starting
 * time of this instance. Return it as a float.
 */
int32_t instimek(CSOUND *csound, RDTIME *p)
{
    IGN(csound);
    *p->rslt = (MYFLT) (CS_KCNT - p->instartk);
    return OK;
}

/* insttimes()
 *
 * Read difference between the global variable kcounter and the starting
 * time of this instance.  Return it as a float in seconds.
 */
int32_t instimes(CSOUND *csound, RDTIME *p)
{
    IGN(csound);
    *p->rslt = (MYFLT) (CS_KCNT - p->instartk) * CS_ONEDKR;
    return OK;
}

/*****************************************************************************/
/*****************************************************************************/

/* Printing at k rate - printk. */

/* printkset is called when the instance of the instrument is initialised. */

int32_t printkset(CSOUND *csound, PRINTK *p)
{
    /* Set up ctime so that if it was 0 or negative, it is set to a low value
     * to ensure that the print cycle happens every k cycle.  This low value is
     * 1 / ekr     */
    /* Not sure this mattersin revised version.  Would just work! -- JPff */
    if (*p->ptime < CS_ONEDKR)
      p->ctime = FL(0.0);
    else
      p->ctime = *p->ptime * csound->ekr;

    /* Set up the number of spaces.
       Limit to 120 for people with big screens or printers.
     */
    p->pspace = (int32_t) *p->space;
    if (UNLIKELY(p->pspace < 0L))
      p->pspace = 0L;
    else if (UNLIKELY(p->pspace > 120L))
      p->pspace = 120L;

    //printf("printkset: ctime = %f\n", p->ctime);
    p->printat = CS_KCNT;
    p->initialised = -1;
    return OK;
}
/*************************************/

/* printk
 *
 * Called on every k cycle. It must decide when to do a print operation.
 */
int32_t printk(CSOUND *csound, PRINTK *p)
{
    if (UNLIKELY(p->initialised != -1))
      csound->PerfError(csound, p->h.insdshead, Str("printk not initialised"));

    //printf("printk: KCNT = %lu\n", CS_KCNT);
    //printf("printat = %lf\n", p->printat);
    /* Now test if the cycle number has reached the next print time */
    if (p->printat <= CS_KCNT-1) {
      /* Do the print cycle.
       * Print instrument number and time. Instrument number stuff from
       * printv() in disprep.c.
       */
      csound->MessageS(csound, CSOUNDMSG_ORCH, " i%4d ",
                               (int32_t)p->h.insdshead->p1.value);
      csound->MessageS(csound, CSOUNDMSG_ORCH, Str("time %11.5f: "),
                               csound->icurTime/csound->esr-CS_ONEDKR);
      /* Print spaces and then the value we want to read.   */
      if (p->pspace > 0L) {
        char  s[128];   /* p->pspace is limited to 120 in printkset() above */
        memset(s, ' ', 128 /*(size_t) p->pspace */);
        s[p->pspace] = '\0';
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", s);
      }
      if (*p->named)
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s = %11.5f\n",
                         p->h.optext->t.inlist->arg[1], *p->val);
      else
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%11.5f\n", *p->val);
      p->printat += p->ctime;
    }
    return OK;
}

/*---------------------------------------------------------------------------*/

/* printks() and printksset() */

/* Printing at k rate with a string * and up to four variables - printks. */

#define ESC (0x1B)

/* printksset is called when the instance of the instrument is initialised. */
int32_t printksset_(CSOUND *csound, PRINTKS *p, char *sarg)
{
    char        *sdest;
    char        temp, tempn;

    if (*p->ptime < CS_ONEDKR)
      p->ctime = CS_ONEDKR;
    else
      p->ctime = *p->ptime * csound->ekr;
    p->printat = CS_KCNT;
    memset(p->txtstring, 0, 8192);   /* This line from matt ingalls */
    sdest = p->txtstring;
    /* Copy the string to the storage place in PRINTKS.
     *
     * We will look out for certain special codes and write special
     * bytes directly to the string.
     *
     * There is probably a more elegant way of doing this, then using
     * the look flag.  I could use goto - but I would rather not.      */
    /* This is really a if then else if...
     * construct and is currently grotty -- JPff */
    while (*sarg) {
      temp  = *sarg++;
      tempn = *sarg--;
      /* Look for a single caret and insert an escape char.  */
      if ((temp  == '^') && (tempn != '^')) {
        *sdest++ = ESC;
      }
/* Look for a double caret and insert a single caret - stepping forward  one */
      else if ((temp  == '^') && (tempn == '^')) {
        *sdest++ = '^';
        sarg++;
      }
/* Look for a single tilde and insert an escape followed by a '['.
 * ESC[ is the escape sequence for ANSI consoles */
      else if ((temp  == '~') && (tempn != '~')) {
        *sdest++ = ESC;
        *sdest++ = '[';
      }
/* Look for a double tilde and insert a tilde caret - stepping forward one.  */
      else if ((temp  == '~') && (tempn == '~')) {
        *sdest++ = '~';
        sarg++;
      }
      /* Look for \n, \N etc */
      else if (temp == '\\') {
        switch (tempn) {
        case 'r': case 'R':
          *sdest++ = '\r';
          sarg++;
          break;
        case 'n': case 'N':
          *sdest++ = '\n';
          sarg++;
          break;
        case 't': case 'T':
          *sdest++ = '\t';
          sarg++;
          break;
        case 'a': case 'A':
          *sdest++ = '\a';
          sarg++;
          break;
        case 'b': case 'B':
          *sdest++ = '\b';
          sarg++;
          break;
        case '\\':
          *sdest++ = '\\';
          sarg++;
          break;
        default:
          *sdest++ = tempn;
          sarg++;
          break;
        }
      }
      /* This case is from matt ingalls */
      else if (temp == '%' && tempn != '%' ) {
        /* an extra option to specify tab and
           return as %t and %r*/
        /* allowing for %% escape -- VL */
        switch (tempn) {
        case 'r': case 'R':
          *sdest++ = '\r';
          sarg++;
          break;
        case 'n': case 'N':
          *sdest++ = '\n';
          sarg++;
          break;
        case 't': case 'T':
          *sdest++ = '\t';
          sarg++;
          break;
        case '!':     /* and a ';' */
          *sdest++ = ';';
          sarg++;
          break;
          // case '%':             /* Should we do this? JPff */
          // *sdest++ = '%';       /* No. VL */
          // sarg++;
          // break;
        default:
          *sdest++ = temp;
          break;
        }
      }
      else {
        /* If none of these match, then copy the character directly
         * and try again.      */
        *sdest++ = temp;
      }
      /* Increment pointer and process next character until end of string.  */
      ++sarg;
    }
    p->printat = CS_KCNT;
    p->initialised = -1;
    return OK;
}

int32_t printksset_S(CSOUND *csound, PRINTKS *p){
    char *sarg;
    sarg = ((STRINGDAT*)p->ifilcod)->data;
    if (sarg == NULL) return csoundInitError(csound, Str("null string\n"));
    p->old = cs_strdup(csound, sarg);
    return printksset_(csound, p, sarg);
}

int32_t printksset(CSOUND *csound, PRINTKS *p){
    char* arg_string = get_arg_string(csound, *p->ifilcod);

    if (arg_string == NULL) {
        return csoundInitError(csound, Str("null string\n"));
    }
    return printksset_(csound, p, arg_string);
}


//perform a sprintf-style format  -- matt ingalls
/* void sprints_local(char *outstring, char *fmt, MYFLT **kvals, int32_t numVals) */
/* { */
/*     char strseg[8192]; */
/*     int32_t i = 0, j = 0; */
/*     char *segwaiting = 0; */
/*     puts(fmt); */
/*     while (*fmt) { */
/*       if (*fmt == '%') { */
/*         /\* if already a segment waiting, then lets print it *\/ */
/*         if (segwaiting) { */
/*           MYFLT xx = (j>=numVals? FL(0.0) : *kvals[j]); */
/*           /\* printf("***xx = %f (int32_t)(xx+.5)=%d round=%d mode=%d\n", *\/ */
/*           /\*        xx, (int32_t)(xx+.5), MYFLT2LRND(xx), fegetround()); *\/ */
/*           strseg[i] = '\0'; */

/*           switch (*segwaiting) { */
/*           case 'd': */
/*           case 'i': */
/*           case 'o': */
/*           case 'x': */
/*           case 'X': */
/*           case 'u': */
/*           case 'c': */
/*             snprintf(outstring, 8196, strseg, (int32_t)MYFLT2LRND(xx)); */
/*             break; */
/*           case 'h': */
/*             snprintf(outstring, 8196, strseg, (int32_t16)MYFLT2LRND(xx)); */
/*             break; */
/*           case 'l': */
/*             snprintf(outstring, 8196, strseg, (int32_t32)MYFLT2LRND(xx)); */
/*             break; */

/*           default: */
/*          printf("strseg:%s - %c\n", strseg, *segwaiting); */
/*             CS_SPRINTF(outstring, strseg, xx); */
/*             break; */
/*           } */
/*           outstring += strlen(outstring); */

/*           i = 0; */
/*           segwaiting = 0; */

/* // prevent potential problems if user didnt give enough input params */
/*           if (j < numVals-1) */
/*             j++; */
/*         } */

/*         /\* copy the '%' *\/ */
/*         strseg[i++] = *fmt++; */

/*         /\* find the format code *\/ */
/*         segwaiting = fmt; */
/*         while (*segwaiting && !isalpha(*segwaiting)) */
/*           segwaiting++; */
/*       } */
/*       else */
/*         strseg[i++] = *fmt++; */
/*     } */

/*     if (i) { */
/*       strseg[i] = '\0'; */
/*       if (segwaiting) { */
/*         MYFLT xx = (j>=numVals? FL(0.0) : *kvals[j]); */
/*            /\* printf("***xx = %f (int32_t)(xx+.5)=%d round=%d mode=%d\n", *\/ */
/*            /\*       xx, (int32_t)(xx+.5), MYFLT2LRND(xx), fegetround()); *\/ */
/*        switch (*segwaiting) { */
/*         case 'd': */
/*         case 'i': */
/*         case 'o': */
/*         case 'x': */
/*         case 'X': */
/*         case 'u': */
/*         case 'c': */
/*           snprintf(outstring, 8196, strseg, (int32_t)MYFLT2LRND(xx)); */
/*           break; */
/*         case 'h': */
/*           snprintf(outstring, 8196, strseg, (int16)MYFLT2LRND(xx)); */
/*           break; */
/*         case 'l': */
/*           snprintf(outstring, 8196, strseg, (int32_t)MYFLT2LRND(xx)); */
/*           break; */

/*         default: */
/*           CS_SPRINTF(outstring, strseg, xx); */
/*           break; */
/*         } */
/*       } */
/*       else */
/*         snprintf(outstring, 8196, "%s", strseg); */
/*     } */
/* } */
/* VL - rewritten 1/16
   escaping %% correctly now.
 */
static int32_t sprints(char *outstring,  char *fmt, MYFLT **kvals, int32_t numVals)
{
    char tmp[8],cc;
    int32_t j = 0;
    int32_t len = 8192;
    while (*fmt) {
      if (*fmt == '%') {
        if (*(fmt+1) == '%') {
          *outstring++ = *fmt++;
          /* *outstring++ = */ fmt++;
          len-=1;
        }
        else if (*(fmt+1) && isspace(*(fmt+1))) {
          *outstring++ = *fmt++;
          *outstring++ = '%';
          *outstring++ = *fmt++;
          len-=3;
        }
        else {
          int32_t n = 1;
          char check='%';
          tmp[0] = check;
          while (*(fmt+n) &&
                !isblank(*(fmt+n))) {
            tmp[n] = *(fmt+n);
            if (isalpha(tmp[n])) { check = tmp[n]; break;}
            n++;
          }
          tmp[n] = *(fmt+n);
          tmp[n+1] = '\0';
          n++;
          if (j>=numVals) return NOTOK;
          switch (check) {
          case 'd':
          case 'i':
          case 'o':
          case 'x':
          case 'X':
          case 'u':
            snprintf(outstring, len, tmp, MYFLT2LRND(*kvals[j]));
            break;
          case 'c':
            cc  = (char) MYFLT2LRND(*kvals[j]);
            if (cc == '%') {
              *outstring++ = '%';
            }
            snprintf(outstring, len, tmp, cc);
            break;
          default:
            //puts(fmt);
            snprintf(outstring, len, tmp, *kvals[j]);
            break;
          }
          if (j < numVals-1)
            j++;
          fmt += n;
          outstring += strlen(outstring);
          len -= strlen(outstring);
        }
      }
      else {
        *outstring++ = *fmt++; *outstring = '\0';
        len--;
      }
    }
    return OK;
}


/*************************************/

/* printks is called on every k cycle
 * It must decide when to do a
 * print operation.
 */
int32_t printks(CSOUND *csound, PRINTKS *p)
{
    char        string[8192]; /* matt ingals replacement */

    if (csound->ISSTRCOD(*p->ifilcod) == 0) {
      char *sarg;
      sarg = ((STRINGDAT*)p->ifilcod)->data;
      if (sarg == NULL)
        return csoundPerfError(csound, p->h.insdshead, Str("null string\n"));
      if (strcmp(sarg, p->old) != 0) {
        printksset_(csound, p, sarg);
        csound->Free(csound, p->old);
        p->old = cs_strdup(csound, sarg);
      }
    }

    /*-----------------------------------*/
    if (UNLIKELY(p->initialised != -1))
      csound->PerfError(csound, p->h.insdshead, Str("printks not initialised"));
    if (p->printat <= CS_KCNT-1) {
      //string[0]='\0';           /* incase of empty string */
      memset(string,0,8192);
      if (sprints(string, p->txtstring, p->kvals, p->INOCOUNT-2)==NOTOK)
        return
          csound->PerfError(csound,  p->h.insdshead,
                            Str("Insufficient arguments in formatted printing"));
      csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", string);
      p->printat += p->ctime;
    }
    return OK;
}

/* matt ingalls --  i-rate prints */
int32_t printsset(CSOUND *csound, PRINTS *p)
{
    PRINTKS pk;
    char    string[8192];
    MYFLT ptime = 1;
    string[0] = '\0';    /* necessary as sprints is not nice */
    pk.h = p->h;
    pk.ifilcod = p->ifilcod;
    pk.ptime = &ptime;
    printksset(csound, &pk);
    memset(string,0,8192);
    if (sprints(string, pk.txtstring, p->kvals, p->INOCOUNT-1)==NOTOK)
        return
          csound->InitError(csound,
                            Str("Insufficient arguments in formatted printing"));
    csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", string);
    return OK;
}

int32_t printsset_S(CSOUND *csound, PRINTS *p)
{
    PRINTKS pk;
    char   string[8192];
    MYFLT ptime = 1;
    string[0] = '\0';    /* necessary as sprints is not nice */
    pk.h = p->h;
    pk.ifilcod = p->ifilcod;
    pk.ptime = &ptime;
    printksset_S(csound, &pk);
    if (strlen(pk.txtstring) < 8191){
      memset(string,0,8192);
    if (sprints(string, pk.txtstring, p->kvals, p->INOCOUNT-1)==NOTOK)
        return
          csound->InitError(csound,
                            Str("Insufficient arguments in formatted printing"));
    csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", string);
    } else {
      csound->Warning(csound,
                      Str("Formatting string too long: %s"), pk.txtstring);
    }
    return OK;
}

/*****************************************************************************/

/* peakk and peak ugens */

/* peakk()
 *
 * Write the absolute value of the input argument to the output if the former
 * is higher. */
int32_t peakk(CSOUND *csound, PEAK *p)
{
   IGN(csound);
    if (*p->kpeakout < FABS(*p->xsigin)) {
      *p->kpeakout = FABS(*p->xsigin);
    }
    return OK;
}

/* peaka()
 *
 * Similar to peakk, but looks at an a rate input variable. */
int32_t peaka(CSOUND *csound, PEAK *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *peak, pp;
    MYFLT   *asigin;

    asigin = p->xsigin;
    peak = p->kpeakout;
    pp = *peak;
    if (UNLIKELY(early)) nsmps -= early;
    for (n=offset;n<nsmps;n++) {
      MYFLT x = FABS(asigin[n]);
      if (pp < x) pp = x;
    }
    *peak = pp;
    return OK;
}

/*****************************************************************************/

/* Gab 21-8-97 */
/* print a k variable each time it changes (useful for MIDI control sliders) */

int32_t printk2set(CSOUND *csound, PRINTK2 *p)
{
    IGN(csound);
    p->pspace = (int32_t)*p->space;
    if (UNLIKELY(p->pspace < 0))
      p->pspace = 0;
    else if (UNLIKELY(p->pspace > 120))
      p->pspace = 120;
    p->oldvalue = FL(-1.12123e35);  /* hack to force printing first value */
    return OK;
}

/* Gab 21-8-97 */
/* print a k variable each time it changes (useful for MIDI control sliders) */

int32_t printk2(CSOUND *csound, PRINTK2 *p)
{
    MYFLT   value = *p->val;

    if (p->oldvalue != value) {
      csound->MessageS(csound, CSOUNDMSG_ORCH, " i%d ",
                                               (int32_t)p->h.insdshead->p1.value);
      if (p->pspace > 0) {
        char  s[128];   /* p->pspace is limited to 120 in printk2set() above */
        memset(s, ' ', (size_t) p->pspace);
        s[p->pspace] = '\0';
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", s);
      }
      if (*p->named)
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s = %11.5f\n",
                         *p->h.optext->t.inlist->arg, *p->val);
      else
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%11.5f\n", *p->val);
      p->oldvalue = value;
    }
    return OK;
}

int32_t printk3set(CSOUND *csound, PRINTK3 *p)
{
    IGN(csound);
    p->oldvalue = FL(-1.12123e35);  /* hack to force printing first value */
    p->sarg = ((STRINGDAT*)p->iformat)->data;
    return OK;
}

int32_t printk3(CSOUND *csound, PRINTK3 *p)
{
    MYFLT   value = *p->val;

    if (p->oldvalue != value) {
      char buff[8196];
      MYFLT *vv[1];
      vv[0] = &value;
      buff[0] = '\0';
      if (sprints(buff, p->sarg, vv, 1)==NOTOK)
        return
          csound->PerfError(csound,  p->h.insdshead,
                            Str("Insufficient arguments in formatted printing"));
      csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", buff);
      p->oldvalue = value;
    }
    //else printf("....%f %f\n", p->oldvalue, value);
    return OK;
}

/* inz writes to za space at a rate as many channels as can. */
int32_t inz(CSOUND *csound, IOZ *p)
{
    int32_t    indx, i;
    int32_t     nchns = csound->nchnls;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    /* Check to see this index is within the limits of za space.     */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx + csound->nchnls >= csound->zalast)) goto err1;
    else if (UNLIKELY(indx < 0)) goto err2;
    else {
      MYFLT *writeloc;
      /* Now write to the array in za space pointed to by indx.    */
      writeloc = csound->zastart + (indx * nsmps);
      early = nsmps - early;
      for (i = 0; i < nchns; i++)
        for (n = 0; n < nsmps; n++)
          *writeloc++ = ((n>=offset && n<early) ?
                         CS_SPIN[i * nsmps+n] : FL(0.0));
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("inz index > isizea. Not writing."));
 err2:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("inz index < 0. Not writing."));
}

/* outz reads from za space at a rate to output. */
int32_t outz(CSOUND *csound, IOZ *p)
{
    int32_t    indx;
    int32_t     i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     nchns = csound->nchnls;
    MYFLT *spout = csound->spraw;

    /* Check to see this index is within the limits of za space.    */
    indx = (int32) *p->ndx;
    if (UNLIKELY((indx + csound->nchnls) >= csound->zalast)) goto err1;
    else if (UNLIKELY(indx < 0)) goto err2;
    else {
      MYFLT *readloc;
      /* Now read from the array in za space and write to the output. */
      readloc = csound->zastart + (indx * nsmps);
      early = nsmps-early;
      if (!csound->spoutactive) {
        memset(spout, '\0', nchns*nsmps*sizeof(MYFLT));
        for (i = 0; i < nchns; i++) {
          memcpy(&spout[i * nsmps+offset], readloc+offset,
                 (early-offset)*sizeof(MYFLT));
          readloc += nsmps;
        }
        csound->spoutactive = 1;
      }
      else {
        for (i = 0; i < nchns; i++) {
          for (n = offset; n < nsmps-early; n++) {
            spout[n + i*nsmps] += readloc[n];
          }
          readloc += nsmps;
        }
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("outz index > isizea. No output"));
 err2:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("outz index < 0. No output."));
}
