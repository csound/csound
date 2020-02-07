/*
    zak.c:

    Copyright (C) 1997 Robin Whittle
                  2018 Jhn ffitch

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
 * September 2018 John ffitch moved zak to a new file ultimately for a plugin
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
//#include "csdl.h"
#include "interlocks.h"
#include <math.h>
#include <ctype.h>

#include "zak.h"

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
/* From 6.12 these are in a GlobalVariable anf NOT CSOUND structure -- JPff */

/* zakinit is an opcode which must be called once to reserve the memory
 * for zk and za spaces.
 */

int32_t zakinit(CSOUND *csound, ZAKINIT *p)
{
    int32_t    length;
    ZAK_GLOBALS* zak;
    zak = (ZAK_GLOBALS*) csound->QueryGlobalVariable(csound, "_zak_globals");
    /* Check to see this is the first time zakinit() has been called.
     * Global variables will be zero if it has not been called.     */

    if (UNLIKELY(zak != NULL)) {
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

    if (UNLIKELY(csound->CreateGlobalVariable(csound, "_zak_globals",
                                              sizeof(ZAK_GLOBALS)) != 0))
      return
        csound->InitError(csound, "%s",
                          Str("zakinit: failed to allocate globals"));
    zak = (ZAK_GLOBALS*) csound->QueryGlobalVariable(csound, "_zak_globals");
    zak->zklast = (int32_t) *p->isizek;
    length = (zak->zklast + 1L) * sizeof(MYFLT);
    zak->zalast = (int32_t) *p->isizea;
    zak->zkstart = (MYFLT*) csound->Calloc(csound, length);

    /* Likewise, allocate memory for za space, but do it in arrays of
     * length ksmps.
     * This is all set to 0 and there will be an error report if the
     * memory cannot be allocated.       */


    length = (zak->zalast + 1L) * sizeof(MYFLT) * CS_KSMPS;
    zak->zastart = (MYFLT*) csound->Calloc(csound, length);
    return OK;
}

/*---------------------------------------------------------------------------*/

/* I and K rate zak code. */

/* zkset() is called at the init time of the instance of the zir, zkr
 * zir and ziw ugens.  It complains if zk space has not been allocated yet.
 */
int32_t zkset(CSOUND *csound, ZKR *p)
{
    ZAK_GLOBALS* zak =
      (ZAK_GLOBALS*) csound->QueryGlobalVariable(csound, "_zak_globals");
    if (UNLIKELY(zak->zkstart == NULL)) {
      return csound->InitError(csound, Str("No zk space: "
                                           "zakinit has not been called yet."));
    }
    p->zz = zak;
    return OK;
}

/*-----------------------------------*/

/* k rate READ code. */

/* zkr reads from zk space at k rate. */
int32_t zkr(CSOUND *csound, ZKR *p)
{
    int32_t    indx;
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;

    /* Check to see this index is within the limits of zk space. */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > zak->zklast)) {
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
      readloc = zak->zkstart + indx;
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
    ZAK_GLOBALS* zak;

    if (UNLIKELY(zkset(csound, (ZKR*)p)!=OK))
      return csound->InitError(csound, Str("No zk space: "
                                           "zakinit has not been called yet."));
    zak = (ZAK_GLOBALS*) p->zz;

    /* if (UNLIKELY(zak == NULL)) */
    /*   return NOTOK; */
    /* Check to see this index is within the limits of zk space. */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > zak->zklast)) {
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
      readloc = zak->zkstart + indx;
      *p->rslt = *readloc;
    }
    return OK;
}

/*-----------------------------------*/

/* Now the i and k rate WRITE code.  zkw writes to zk space at k rate. */
int32_t zkw(CSOUND *csound, ZKW *p)
{
    int32_t    indx;
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;

    /* Check to see this index is within the limits of zk space. */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > zak->zklast)) {
      return csound->PerfError(csound, &(p->h),
                               Str("zkw index > isizek. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->PerfError(csound, &(p->h),
                               Str("zkw index < 0. Not writing."));
    }
    else {
      MYFLT *writeloc;
      /* Now write to the appropriate location in zk space.  */
      writeloc = zak->zkstart + indx;
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
    ZAK_GLOBALS* zak;

    if (UNLIKELY(zkset(csound, (ZKR*)p)!= OK))
      return csound->InitError(csound, Str("No zk space: "
                                           "zakinit has not been called yet."));
    zak = p->zz;
    /* if (UNLIKELY(zak==NULL)) */
    /*   return NOTOK; */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > zak->zklast)) {
      return csound->InitError(csound, Str("ziw index > isizek. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->InitError(csound, Str("ziw index < 0. Not writing."));
    }
    else {
      MYFLT *writeloc;
      /* Now write to the appropriate location in zk space. */
      writeloc = zak->zkstart + indx;
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
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;

    /* Check to see this index is within the limits of zk space.   */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > zak->zklast)) {
      return csound->PerfError(csound, &(p->h),
                               Str("zkwm index > isizek. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->PerfError(csound, &(p->h),
                               Str("zkwm index < 0. Not writing."));
    }
    else {
      MYFLT *writeloc;
      /* Now write to the appropriate location in zk space.  */
      writeloc = zak->zkstart + indx;
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
    ZAK_GLOBALS* zak;

    if (UNLIKELY(zkset(csound, (ZKR*) p) != OK))
      return NOTOK;
    zak  = (ZAK_GLOBALS*) p->zz;
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > zak->zklast)) {
      return csound->InitError(csound,
                               Str("ziwm index > isizek. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->InitError(csound, Str("ziwm index < 0. Not writing."));
    }
    else {
      MYFLT *writeloc;
      writeloc = zak->zkstart + indx;
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
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;
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

    if (UNLIKELY(indx > zak->zklast)) {
      return csound->PerfError(csound, &(p->h),
                               Str("zkmod kzkmod > isizek. Not writing."));
    }
    else {
      /* Now read the value from zk space. */
      readloc = zak->zkstart + indx;
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
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;
    int32_t first = (int32_t) *p->first, last = (int32_t) *p->last, loopcount;

    /* Check to see both kfirst and klast are within the limits of zk space
     * and that last is >= first.                */
    if (UNLIKELY((first > zak->zklast) || (last > zak->zklast)))
      return csound->PerfError(csound, &(p->h),
                               Str("zkcl first or last > isizek. Not clearing."));
    else if (UNLIKELY((first < 0) || (last < 0))) {
      return csound->PerfError(csound, &(p->h),
                               Str("zkcl first or last < 0. Not clearing."));
    }
    else if (UNLIKELY(first > last)) {
      return csound->PerfError(csound, &(p->h),
                               Str("zkcl first > last. Not clearing."));
    }
    else {
      /* Now clear the appropriate locations in zk space. */
      loopcount = last - first + 1;
      writeloc = zak->zkstart + first;
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
    ZAK_GLOBALS* zak =
      (ZAK_GLOBALS*) csound->QueryGlobalVariable(csound, "_zak_globals");
     IGN(p);
    if  (zak == NULL) {
      return csound->InitError(csound, Str("No za space: "
                                           "zakinit has not been called yet."));
    }
    p->zz = zak;
    return (OK);
}

/*-----------------------------------*/

/* a rate READ code. */

/* zar reads from za space at a rate. */
int32_t zar(CSOUND *csound, ZAR *p)
{
    MYFLT       *readloc, *writeloc;
    int32_t indx;
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    /*-----------------------------------*/

    writeloc = p->rslt;

    /* Check to see this index is within the limits of za space.    */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > zak->zalast)) {
      memset(writeloc, 0, nsmps*sizeof(MYFLT));
      return csound->PerfError(csound, &(p->h),
                               Str("zar index > isizea. Returning 0."));
    }
    else if (UNLIKELY(indx < 0)) {
      memset(writeloc, 0, nsmps*sizeof(MYFLT));
      return csound->PerfError(csound, &(p->h),
                               Str("zar index < 0. Returning 0."));
    }
    else {
      /* Now read from the array in za space and write to the destination.
       * See notes in zkr() on pointer arithmetic.     */
      readloc = zak->zastart + (indx * CS_KSMPS);
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
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;
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
    if (UNLIKELY(indx > zak->zalast)) {
      memset(writeloc, 0, nsmps*sizeof(MYFLT));
      return csound->PerfError(csound, &(p->h),
                               Str("zarg index > isizea. Returning 0."));
    }
    else {
      if (UNLIKELY(indx < 0)) {
        memset(writeloc, 0, nsmps*sizeof(MYFLT));
        return csound->PerfError(csound, &(p->h),
                                 Str("zarg index < 0. Returning 0."));
      }
      else {
        /* Now read from the array in za space multiply by kgain and write
         * to the destination.       */
        readloc = zak->zastart + (indx * CS_KSMPS);
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
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;
    int32_t indx;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    /* Set up the pointer for the source of data to write.    */
    readloc = p->sig;
    /* Check to see this index is within the limits of za space.     */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > zak->zalast)) {
      return csound->PerfError(csound, &(p->h),
                                 Str("zaw index > isizea. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->PerfError(csound, &(p->h),
                                 Str("zaw index < 0. Not writing."));
    }
    else {
        /* Now write to the array in za space pointed to by indx.    */
      writeloc = zak->zastart + (indx * CS_KSMPS);
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
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;
    int32_t indx;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    /*-----------------------------------*/

    /* Set up the pointer for the source of data to write. */

    readloc = p->sig;
    /* Check to see this index is within the limits of za space.    */
    indx = (int32_t) *p->ndx;
    if (UNLIKELY(indx > zak->zalast)) {
      return csound->PerfError(csound, &(p->h),
                               Str("zaw index > isizea. Not writing."));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->PerfError(csound, &(p->h),
                               Str("zaw index < 0. Not writing."));
    }
    else {
      /* Now write to the array in za space pointed to by indx.    */
      writeloc = zak->zastart + (indx * CS_KSMPS);
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
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;
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
    if (UNLIKELY(indx > zak->zalast)) {
      return csound->PerfError(csound, &(p->h),
                               Str("zamod kzamod > isizea. Not writing."));
    }
    else {                      /* Now read the values from za space.    */
      readloc = zak->zastart + (indx * CS_KSMPS);
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
    ZAK_GLOBALS* zak = (ZAK_GLOBALS*) p->zz;
    int32_t first, last, loopcount;

    first = (int32_t) *p->first;
    last  = (int32_t) *p->last;
    if(last == -1)
        last = first;

    /* Check to see both kfirst and klast are within the limits of za space
     * and that last is >= first.    */
    if (UNLIKELY((first > zak->zalast) || (last > zak->zalast)))
      return
        csound->PerfError(csound, &(p->h),
                          Str("zacl first or last > isizea. Not clearing."));
    else {
      if (UNLIKELY((first < 0) || (last < 0))) {
        return csound->PerfError(csound, &(p->h),
                                 Str("zacl first or last < 0. Not clearing."));
      }
      else {
        if (UNLIKELY(first > last)) {
          return csound->PerfError(csound, &(p->h),
                                   Str("zacl first > last. Not clearing."));
        }
        else {  /* Now clear the appropriate locations in za space. */
          loopcount = (last - first + 1) * CS_KSMPS;
          writeloc = zak->zastart + (first * CS_KSMPS);
          memset(writeloc, 0, loopcount*sizeof(MYFLT));
        }
      }
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY zak_localops[] = {
  { "zakinit", S(ZAKINIT), ZB, 1,  "",   "ii",   (SUBR)zakinit, NULL,  NULL      },
  { "zir",    S(ZKR),ZR,  1,   "i",  "i",    (SUBR)zir,     NULL,  NULL      },
  { "zkr",    S(ZKR),ZR,  3,   "k",  "k",    (SUBR)zkset,   (SUBR)zkr,   NULL},
  { "ziw",    S(ZKW),ZW, 1,   "",   "ii",   (SUBR)ziw,     NULL,  NULL      },
  { "zkw",    S(ZKW),     ZW, 3,   "",   "kk",   (SUBR)zkset,   (SUBR)zkw,   NULL},
  { "ziwm",   S(ZKWM),    ZB, 1,   "",   "iip",  (SUBR)ziwm,    NULL,  NULL      },
  { "zkwm",   S(ZKWM),    ZB, 3,   "",   "kkp",  (SUBR)zkset,   (SUBR)zkwm,  NULL},
  { "zkmod",  S(ZKMOD),   ZB, 3,   "k",  "kk",   (SUBR)zkset,   (SUBR)zkmod, NULL},
  { "zkcl",   S(ZKCL),    ZW, 3,   "",  "kk",   (SUBR)zkset,   (SUBR)zkcl,  NULL },
  { "zar",    S(ZAR),ZR,  3,   "a", "k",    (SUBR)zaset,  (SUBR)zar  },
  { "zarg",   S(ZARG),   ZB, 3,   "a", "kk",   (SUBR)zaset,  (SUBR)zarg },
  { "zaw",    S(ZAW),    ZW, 3,   "",  "ak",   (SUBR)zaset,  (SUBR)zaw  },
  { "zawm",   S(ZAWM),   ZB, 3,   "",  "akp",  (SUBR)zaset,  (SUBR)zawm },
  { "zamod",  S(ZAMOD),  ZB, 3,   "a", "ak",   (SUBR)zaset,  (SUBR)zamod},
  { "zacl",   S(ZACL),   ZW, 3,   "",  "kJ",   (SUBR)zaset,  (SUBR)zacl}
};

LINKAGE_BUILTIN(zak_localops)
