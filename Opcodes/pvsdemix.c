/*  pvsdemix.c:
    De-mixing of stereo sources.

    Copyright (c) Victor Lazzarini, 2005

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

#include "pvs_ops.h"
#include "pvsdemix.h"

static int32_t fsigs_equal(const PVSDAT *f1, const PVSDAT *f2)
{
    if ((f1->overlap    == f2->overlap)
        && (f1->winsize == f2->winsize)
        && (f1->wintype == f2->wintype) /* harsh, maybe... */
        && (f1->N       == f2->N)
        && (f1->format  == f2->format)
        )
      return 1;
    return 0;
}

#define FLOATMAX_ 3.402823466e+38f

static int32_t pvsdemix_init(CSOUND *csound, PVSDEMIX *p)
{
    uint32_t N = p->finleft->N;
    int32_t olap = p->finleft->overlap;
    uint32_t M;
    p->beta = (int32_t)(*p->slices);

    if (UNLIKELY(p->finleft->sliding))
      return csound->InitError(csound, "%s", Str("SDFT case not implemented yet"));
   M = (N+2)*sizeof(float);
    if (p->fout->frame.auxp==NULL || p->fout->frame.size<M)
      csound->AuxAlloc(csound, M,&p->fout->frame);

    M = M*p->beta;
    if (p->left.auxp==NULL || p->left.size<M)
      csound->AuxAlloc(csound, M, &p->left);

    if (p->right.auxp==NULL || p->right.size<M)
      csound->AuxAlloc(csound, M, &p->right);

    M = (N/2+1)*sizeof(float);
      if (p->maxl.auxp==NULL || p->maxl.size<M)
      csound->AuxAlloc(csound, M, &p->maxl);

    if (p->maxr.auxp==NULL || p->maxr.size<M)
      csound->AuxAlloc(csound, M, &p->maxr);

    if (p->minl.auxp==NULL || p->minl.size<M)
      csound->AuxAlloc(csound, M, &p->minl);

    if (p->minr.auxp==NULL || p->minr.size<M)
      csound->AuxAlloc(csound, M, &p->minr);

    p->fout->N =  N;
    p->fout->overlap = olap;
    p->fout->winsize = p->finleft->winsize;
    p->fout->wintype = p->finleft->wintype;
    p->fout->format = p->finleft->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (!((p->fout->format==PVS_AMP_FREQ) ||
          (p->fout->format==PVS_AMP_PHASE)))
      return csound->InitError(csound,
                  "pvsdemix: signal format must be amp-phase or amp-freq.\n");

    return OK;
}

static int32_t pvsdemix_process(CSOUND *csound, PVSDEMIX *p)
{
    int32_t n, i, n2, N = p->fout->N, imax;
    int32_t framesize = N+2;
    float sum = 0.0f,sig,g;
    int32_t beta = (int32_t) p->beta, pos;
    float *sigl = (float *) p->finleft->frame.auxp;
    float *sigr = (float *) p->finright->frame.auxp;
    float *out = (float *) p->fout->frame.auxp;
    float *left = (float *) p->left.auxp;
    float *right = (float *) p->right.auxp;
    float *minl = (float *) p->minl.auxp;
    float *maxr = (float *) p->maxr.auxp;
    float *minr = (float *) p->minr.auxp;
    float *maxl = (float *) p->maxl.auxp;
    MYFLT azimuth = *p->pos;
    MYFLT width = *p->width;
    MYFLT range;

    if (UNLIKELY(!fsigs_equal(p->finleft,p->finright))) goto err1;

    if (UNLIKELY(out==NULL)) goto err2;

    if (p->lastframe < p->finleft->framecount) {

      if (width > beta) width = (MYFLT) beta;
      else if (width < 1) width = FL(1.0);

      if (azimuth < -1) azimuth = -FL(1.0);
      else if (azimuth > 1) azimuth = FL(1.0);

      imax = beta*framesize;
      range = width/FL(2.0);
      pos = (int32_t)((azimuth >= 0 ? azimuth : -azimuth)*beta);

      /*  create the azimuth amplitude vectors &
          find the max/min values for channels, per bin */
      for (n=0; n < N/2+1; n++) {
        maxl[n] = maxr[n] = 0.0f;
        minl[n] = minr[n] = FLOATMAX_;
        n2 = n << 1;
        for (i=framesize; i<=imax; i+=framesize){
          g = (float) i/imax;
          sig = sigl[n2] -  g*sigr[n2];
          left[n+(imax - i)] = sig = sig < 0 ? -sig : sig;
          maxl[n] =  maxl[n] > sig ? maxl[n] : sig;
          minl[n] =  minl[n] <  sig ? minl[n] : sig;
          sig = sigr[n2] -  g*sigl[n2];
          right[n+(imax - i)] = sig =  sig < 0 ? -sig : sig;
          maxr[n] = maxr[n] >  sig ? maxr[n] : sig;
          minr[n] = minr[n] <  sig ? minr[n] : sig;
        }

        /* reverse the nulls into peaks */
        for (i=imax - framesize; i >= 0; i-=framesize) {

          left[n+i] = left[n+i] == minl[n] ? maxl[n] - minl[n] : 0.f;
          right[n+i] = right[n+i] == minr[n] ? maxr[n] - minr[n] : 0.f;

        }

        /* the issue: a source found somewhere in one channel
           will cause an image to be found in the opposite channel
           around 0.
        */


        /* resynthesise the signal
           azimuth <= 0 => pos incrs right to left
           azimuth >  0 => pos incrs left to right
        */
         for (i = (int32_t)(pos - range); i < (pos+range); i++) {
         if (i < 0)
           sum  += (azimuth <= 0 ? right[n+(beta+i)*framesize]
               :  left[n+(beta+i)*framesize]);
          else if (i < beta)
           sum += (azimuth <= 0 ? right[n+i*framesize]
                   :  left[n+i*framesize]);
         }

        out[n2] = sum;
        out[n2+1] = (azimuth < 0 ? sigl[n2+1] :
                     sigr[n2+1]);


        sum=0.f;
      }

      p->fout->framecount = p->lastframe = p->finleft->framecount;
    }

    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("pvsdemix : formats are different.\n"));
 err2:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("pvsdemix : not initialised\n"));
}

static OENTRY localops[] =
  {
    {"pvsdemix", sizeof(PVSDEMIX), 0,  "f", "ffkki",
                 (SUBR) pvsdemix_init, (SUBR) pvsdemix_process, (SUBR) NULL }
  };

int32_t pvsdemix_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t
                                ) (sizeof(localops) / sizeof(OENTRY)));
}
