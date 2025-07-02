/*
    grain4.c:

    Copyright (C) 1994, 1995 Allan S C Lee, John ffitch

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

/* Verson 4.0 -    Mar 95                               */
/* Verson 4.1 - 10 Mar 95                               */
/*        Lifted restriction on pitch, now accept       */
/*        anything greater than zero                    */
/*        Improved code in handling the warp-round      */
/*        pointer.                                      */
/* Verson 4.2 - 20 Apr 95                               */
/*        Add optional parameter ifnenv                 */
/*        Function table to be used for the shape of the*/
/*        envelop rise and decade curve                 */
/* Minor changes by John Fitch Dec 1995                 */


#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include "interlocks.h"
#include "grain4.h"
#include <math.h>

#define        RNDMUL  15625L

static MYFLT grand(GRAINV4 *);

static int32_t grainsetv4(CSOUND *csound, GRAINV4 *p)
{
    FUNC        *ftp, *ftp_env;
    int32_t         nvoice, cnt;
    int32_t    tmplong1, tmplong2;
    MYFLT       tmpfloat1;
    MYFLT       pitch[4];

    /* call ftfind() to get the function table...*/
    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
    }
    else {
      return csound->InitError(csound, "%s", Str("granule_set: "
                                           "Unable to find function table"));
    }

    /* call ftfind() to get the function table for the envelop...*/
    if (*p->ifnenv > 0) {
      if (LIKELY((ftp_env = csound->FTFind(csound, p->ifnenv)) != NULL)) {
        p->ftp_env = ftp_env;
      }
      else {
        return csound->InitError(csound, "%s", Str("granule_set: Unable to find "
                                             "function table for envelope"));
      }
    }

    if (UNLIKELY(*p->ivoice > MAXVOICE)) {
      return csound->InitError(csound, "%s", Str("granule_set: Too many voices"));
    }
    if (UNLIKELY(*p->iratio <= 0)) {
      return csound->InitError(csound, "%s", Str("granule_set: "
                                           "iratio must be greater then 0"));
    }
    if (UNLIKELY((*p->imode != 0) && ((*p->imode != -1) && (*p->imode != 1)))) {
      return csound->InitError(csound, "%s", Str("granule_set: "
                                           "imode must be -1, 0 or +1"));
    }
    if (UNLIKELY(*p->ithd < 0)) {
      return csound->InitError(csound, "%s", Str("granule_set: Illegal ithd, "
                                           "must be greater than zero"));
    }
    if (UNLIKELY((*p->ipshift != 1) && (*p->ipshift!=2) && (*p->ipshift!=3) &&
                 (*p->ipshift!=4) && (*p->ipshift!=0) )) {
      return csound->InitError(csound, "%s", Str("granule_set: ipshift must be "
                                           "integer between 0 and 4"));
    }
    if (UNLIKELY(((*p->ipshift >=1) && (*p->ipshift <=4)) &&
                 (*p->ivoice < *p->ipshift))) {
      return csound->InitError(csound, "%s", Str("granule_set: Not enough voices "
                                           "for the number of pitches"));
    }
    if ( *p->ipshift !=FL(0.0) ) {
      if (UNLIKELY(*p->ipitch1 < FL(0.0) )) {
        return
          csound->InitError(csound,
                            "%s", Str("granule_set: ipitch1 must be greater then zero"));
      }
      if (UNLIKELY(*p->ipitch2 < FL(0.0) )) {
        return
          csound->InitError(csound,
                            "%s", Str("granule_set: ipitch2 must be greater then zero"));
      }
      if (UNLIKELY(*p->ipitch3 < FL(0.0) )) {
        return
          csound->InitError(csound,
                            "%s", Str("granule_set: ipitch3 must be greater then zero"));
      }
      if (UNLIKELY(*p->ipitch4 < FL(0.0) )) {
        return
          csound->InitError(csound,
                            "%s", Str("granule_set: ipitch4 must be greater then zero"));
      }
    }

    if (UNLIKELY((*p->igskip < 0) || (*p->igskip * CS_ESR > ftp->flen) )) {
      return csound->InitError(csound, "%s", Str("granule_set: must be positive and "
                                           "less than function table length"));
    }
    if (UNLIKELY(*p->igskip_os < 0)) {
      return csound->InitError(csound, "%s", Str("granule_set: "
                                           "igskip_os must be greater then 0"));
    }

    p->gstart = (int32)(*p->igskip * CS_ESR);
    p->glength = (int32)(*p->ilength * CS_ESR);
    p->gend = p->gstart + p->glength;

    if (UNLIKELY(*p->kgap < 0)) {
      return csound->InitError(csound, "%s", Str("granule_set: "
                                           "kgap must be greater then 0"));
    }
    if (UNLIKELY((*p->igap_os < 0) || (*p->igap_os > 100))) {
      return csound->InitError(csound, "%s", Str("granule_set: "
                                           "igap_os must be 0%% to 100%%"));
    }
    if (UNLIKELY(*p->kgsize < 0)) {
      return csound->InitError(csound, "%s", Str("granule_set: "
                                           "kgsize must be greater then 0"));
    }
    if (UNLIKELY((*p->igsize_os < 0) || (*p->igsize_os >100))) {
      return csound->InitError(csound, "%s", Str("granule_set: "
                                           "igsize_os must be 0%% to 100%%"));
    }
    if (UNLIKELY((*p->iatt < FL(0.0)) || (*p->idec < 0.0) ||
                 ((*p->iatt + *p->idec) > FL(100.0)))) {
      return
        csound->InitError(csound,
                          "%s", Str("granule_set: Illegal value of iatt and/or idec"));
    } /* end if */

    /* Initialize random number generator */
    if (*p->iseed >=0) {
      p->grnd = (int16)(*p->iseed * FL(32768.0));       /* IV - Jul 11 2002 */
    }

                                /* Initialize variables....*/
    p->gskip_os = (int32)(*p->igskip_os * CS_ESR);/* in number of samples */
    p->gap_os = *p->igap_os / FL(100.0);
    p->gsize_os = *p->igsize_os / FL(100.0);

    for (nvoice = 0; nvoice < *p->ivoice; nvoice++) {
      p->fpnt[nvoice] = 0;
      p->cnt[nvoice]  = 0;
      p->phs[nvoice]  = FL(0.0);
      p->gskip[nvoice] = (int32)(*p->igskip * CS_ESR);
      p->gap[nvoice] = (int32)(*p->kgap * CS_ESR);
    }

    if (*p->igap_os != 0) {
      for (nvoice = 0; nvoice < *p->ivoice; nvoice++)
        p->gap[nvoice] += (int32)((MYFLT)p->gap[nvoice] * p->gap_os * grand(p));
    }

    if (*p->imode == 0) {
      for (nvoice = 0; nvoice < *p->ivoice; nvoice++)
        p->mode[nvoice] = (grand(p) < 0) ? -1 : 1;
    }
    else {
      for (nvoice = 0; nvoice < *p->ivoice; nvoice++)
        p->mode[nvoice] = (int32)*p->imode;
    }

    if ((*p->ipshift >=1) && (*p->ipshift <=4)) {
      pitch[0] = *p->ipitch1;
      pitch[1] = *p->ipitch2;
      pitch[2] = *p->ipitch3;
      pitch[3] = *p->ipitch4;
      cnt = 0;
      for (nvoice = 0; nvoice < *p->ivoice; nvoice++) {
        p->pshift[nvoice] = pitch[cnt++];
        cnt = (cnt < *p->ipshift) ? cnt : 0;
      }
    }
    if (*p->ipshift == 0) {
      for (nvoice = 0; nvoice < *p->ivoice; nvoice++) {
        tmpfloat1 = grand(p);
        p->pshift[nvoice] =
          (tmpfloat1 <FL(0.0)) ? (tmpfloat1*FL(0.5))+FL(1.0) : tmpfloat1+1.0f;
      }
    }

    for (nvoice = 0; nvoice < *p->ivoice; nvoice++)
      p->gsize[nvoice] = (int32)(*p->kgsize * CS_ESR * p->pshift[nvoice]);

    if (*p->igsize_os != 0) {
      for (nvoice = 0; nvoice < *p->ivoice; nvoice++)
        p->gsize[nvoice] += (int32)(p->gsize[nvoice] * p->gsize_os * grand(p));
    }

    for (nvoice = 0; nvoice < *p->ivoice; nvoice++)
      p->stretch[nvoice] = p->gsize[nvoice] + p->gap[nvoice];

    if (*p->igskip_os != 0)
      for (nvoice = 0; nvoice < *p->ivoice; nvoice++) {
        tmplong1 = ((p->gskip_os * grand(p)) + (MYFLT)p->gskip[nvoice]);
        p->gskip[nvoice] =
          (tmplong1 < p->gstart) ? p->gstart : tmplong1;
        p->gskip[nvoice]=
          ((p->gskip[nvoice]+p->stretch[nvoice])>(int32)p->gend) ?
          (int32)p->gstart :
          p->gskip[nvoice];
      }

    if (*p->ithd != 0) {        /* Do thresholding.... */
      tmplong2 = 0;
      for (tmplong1=0; tmplong1< (int32_t) ftp->flen; tmplong1++)
        if (fabs(ftp->ftable[tmplong1]) >= *p->ithd )
          ftp->ftable[tmplong2++] = ftp->ftable[tmplong1];
      ftp->flen = tmplong2;
    }

    if (UNLIKELY(p->gend > (int32_t) ftp->flen)) {
      return csound->InitError(csound, "%s", Str("granule_set: Illegal combination "
                                           "of igskip and ilength"));
    }

    //nvoice = (int32_t)*p->ivoice;

    if (UNLIKELY(*p->ilength < (20 * *p->kgsize)))
      csound->Warning(csound,  Str("granule_set: "
                                  "WARNING * ilength may be too short *\n"
                                  "            ilength should be "
                                  "greater than kgsize * max up\n"
                                  "            pitch shift. Also, igsize_os "
                                  "and igskip_os should\n"
                                  "            be taken into consideration.\n"
                                  "ilength is "
                                  "%f Sec, kgsize is %f Sec\n"),
                      *p->ilength, *p->kgsize);

    //p->clock = 0;               /* init clock */
    return OK;
} /* end grainsetv4(p) */

static int32_t graingenv4(CSOUND *csound, GRAINV4 *p)
{
    FUNC        *ftp, *ftp_env;
    MYFLT       *ar, *ftbl, *ftbl_env=NULL;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t         nvoice;
    int32       tmplong1, tmplong2, tmplong3, tmpfpnt, flen_env=0;
    MYFLT       fract, v1, tmpfloat1;
    int32       att_len, dec_len, att_sus;
    MYFLT       envlop;

    /* Optimisations */
    int32       gstart  = p->gstart;
    int32       gend    = p->gend;
    int32       glength = p->glength;
    MYFLT       iratio  = *p->iratio;

 /* Recover parameters from previous call.... */
   ftp = p->ftp;
   if (UNLIKELY(p->ftp==NULL)) goto err1;          /* RWD fix */
   ftbl = ftp->ftable;

   if (*p->ifnenv > 0) {
     ftp_env = p->ftp_env;
     flen_env = ftp_env->flen;
     ftbl_env = ftp_env->ftable;
   }

   /* Recover audio output pointer... */
   ar   = p->ar;
   if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
   if (UNLIKELY(early)) {
     nsmps -= early;
     memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
   }
   /* *** Start the loop .... *** */
   for (n=offset; n<nsmps; n++) {
                                /* Optimisations */
     int32      *fpnt = p->fpnt, *cnt = p->cnt, *gskip = p->gskip;
     int32      *gap = p->gap, *gsize = p->gsize;
     int32      *stretch = p->stretch, *mode = p->mode;
     MYFLT      *pshift = p->pshift, *phs = p->phs;
     ar[n] = FL(0.0);

     for (nvoice = 0; nvoice <  *p->ivoice ; nvoice++) {
       if (*fpnt >= (*gsize -1)) {
         ar[n] += 0;            /* Is this necessary?? */
         *cnt +=1L;
       }
       else {
         fract = *phs - *fpnt;

         if (*mode < 0) {
           tmplong1 = *gskip - gstart;
           if (*fpnt >= tmplong1) {
             tmplong1= *fpnt - tmplong1;
             tmplong2= tmplong1/glength;
             tmplong1 -= tmplong2 * glength;
             tmpfpnt = gend - tmplong1;
           }
           else
             tmpfpnt = *gskip - *fpnt;
         }
         else {
           tmplong1 = gend - *gskip;
           if (*fpnt >= tmplong1) {
             tmplong1= *fpnt - tmplong1;
             tmplong2= tmplong1/glength;
             tmplong1 -= tmplong2 * glength;
             tmpfpnt = gstart + tmplong1;
           }
           else
             tmpfpnt = *gskip + *fpnt;
         }

         att_len = (int32)(*gsize * *p->iatt * FL(0.01));
         dec_len = (int32)(*gsize * *p->idec * FL(0.01));
         att_sus =  *gsize -  dec_len;

         if (*fpnt < att_sus) {
           tmpfloat1 = (FL(1.0) * *fpnt) / att_len;
           envlop = ((tmpfloat1 >=FL(1.0)) ? FL(1.0) : tmpfloat1);
         }
         else
           envlop =
             ((MYFLT)(dec_len - (MYFLT)(*fpnt - att_sus)))/((MYFLT)dec_len);

         v1 = *(ftbl + tmpfpnt);

         tmpfpnt = tmpfpnt + *mode;
         if (tmpfpnt < gstart)
           tmpfpnt = gend - (gstart - tmpfpnt) + 1;
         if (tmpfpnt > gend)
           tmpfpnt = gstart + (tmpfpnt - gend) - 1;

         if (*p->ifnenv > 0) {
           tmplong3 = (int32)(envlop * flen_env) -1L;
           envlop = *(ftbl_env + tmplong3);
         }

         ar[n] +=(v1 + ( *(ftbl + tmpfpnt)   - v1) * fract ) * envlop ;

         *phs += *pshift;
         *fpnt = (int32)*phs;
         *cnt  = (int32)*phs;
       } /* end if (*fpnt >= (*gsize -1)) */

       if (*cnt >= *stretch) {
         *cnt = 0;
         *fpnt= 0;
         *phs = FL(0.0);

         /* pick up new values... */

         /* Use the old value of the pshift, gsize and gap */
         /*           to determine the time advanced */
         /*           *gskip+=
                      ((*gsize / *pshift) +
                      *gap) * iratio;
                      */
         *gskip += (int32)((*gsize / *pshift) * iratio);

         if (*p->igskip_os != 0)
           *gskip  += (int32)(p->gskip_os * grand(p));

         if (*gskip >= gend) {
           tmplong1 = *gskip - gend;
           tmplong2 = tmplong1 /glength;
           tmplong1 -= tmplong2 * glength;
           *gskip = gstart + tmplong1;
         }

         if (*gskip < gstart) *gskip = gstart;

         if (*p->imode == 0) {
           *mode = (grand(p) < 0) ? -1 : 1;
         }

         if (*p->ipshift == 0) {
           tmpfloat1 = grand(p);
           *pshift = (tmpfloat1 < FL(0.0)) ?
             (tmpfloat1*FL(0.5))+FL(1.0) : tmpfloat1+FL(1.0);
         }

         *gap = (int32)(*p->kgap * CS_ESR);
         if (*p->igap_os != 0) {
           *gap += (int32)((*gap * p->gap_os) * grand(p));
         }

         *gsize = (int32)(*p->kgsize * CS_ESR * *pshift);
         if (*p->igsize_os != 0)
           *gsize += (int32)((*gsize * p->gsize_os) * grand(p));

         *stretch = *gsize + *gap;

       }
       fpnt++; cnt++; gskip++; gap++; gsize++;
       stretch++; mode++; pshift++; phs++;
     }
  /* p->clock++; */
     ar[n] *= *p->xamp;     /* increment audio pointer and multiply the xamp */
   }
   return OK;
 err1:
   return csound->PerfError(csound, &(p->h),
                            "%s", Str("grain4: not initialised"));

} /* end graingenv4(p) */

/* Function return a float random number between -1 to +1 */
static MYFLT grand( GRAINV4 *p)
{
   p->grnd *= (int32_t
               )RNDMUL;
   p->grnd += 1;
   return ((MYFLT) p->grnd * DV32768);  /* IV - Jul 11 2002 */
} /* end grand(p) */

#define S(x)    sizeof(x)






static OENTRY grain4_localops[] = {
  { "granule", S(GRAINV4), TR,  "a", "xiiiiiiiiikikiiivppppo",
             (SUBR)grainsetv4, (SUBR)graingenv4},
};

LINKAGE_BUILTIN(grain4_localops)
