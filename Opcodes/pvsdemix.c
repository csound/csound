/*  pvsdemix.c:
    De-mixing of stereo sources.

    (c) Victor Lazzarini, 2005

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
#include "pvsdemix.h"

static int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2)
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

static int pvsdemix_init(CSOUND *csound, PVSDEMIX *p)
{
    int32 N = p->finleft->N;
    int olap = p->finleft->overlap;
    int M;
    p->beta = (int)(*p->slices);

    if (UNLIKELY(p->finleft->sliding))
      return csound->InitError(csound, Str("SDFT case not implemented yet"));
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

    if (!(p->fout->format==PVS_AMP_FREQ) ||
        (p->fout->format==PVS_AMP_PHASE))
      csound->Die(csound,
                  "pvsdemix: signal format must be amp-phase or amp-freq.\n");

    return OK;
}

static int pvsdemix_process(CSOUND *csound, PVSDEMIX *p)
{
    int n, i, n2, N = p->fout->N, imax;
    int framesize = N+2;
    float sum = 0.0f,sig,g;
    int beta = (int) p->beta, pos;
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
      pos = (int)((azimuth >= 0 ? azimuth : -azimuth)*beta);

      /*  create the azimuth amplitude vectors &
          find the max/min values for channels, per bin */
      for (n=0; n < N/2+1; n++) {
        maxl[n] = maxr[n] = 0.0f;
        minl[n] = minr[n] = FLOATMAX_;
        n2 = n << 1;

        for (i=framesize; i <= imax; i+=framesize){
          g = (float)i/imax;
          sig = sigl[n2] -  g*sigr[n2];
          left[n+(imax - i)]  =  sig < 0 ? -sig : sig;
          maxl[n] = (maxl[n] > left[n+(imax - i)] ?
                     maxl[n] : left[n+(imax - i)]);
          minl[n] = (minl[n] <  left[n+(imax - i)] ?
                     minl[n] : left[n+(imax - i)]);
          sig = sigr[n2] -  g*sigl[n2];
          right[n+(imax - i)] =  sig < 0 ? -sig : sig;
          maxr[n] = (maxr[n] >  right[n+(imax - i)] ?
                     maxr[n] : right[n+(imax - i)]);
          minr[n] = (minr[n] <  right[n+(imax - i)] ?
                     minr[n] : right[n+(imax - i)]);
        }

        /* reverse the nulls into peaks */
        for (i=0; i < imax; i+=framesize) {
          left[n+i] = (left[n+i] == minl[n] ?
                       maxl[n] - minl[n] : 0.f);
          right[n+i] = (right[n+i] == minr[n] ?
                        maxr[n] - minr[n] : 0.f);
        }

        /* resynthesise the signal
           azimuth <= 0 => pos incrs right to left
           azimuth >  0 => pos incrs left to right
        */
        for (i = (int) (pos-range); i < pos+range; i++) {
          if (i < 0)
            sum  += (azimuth <= 0 ? right[n+(beta+i)*framesize]
                     :  left[n+(beta+i)*framesize]);
          else if (i < beta)
            sum +=(azimuth <= 0 ? right[n+i*framesize]
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
    return csound->PerfError(csound, Str("pvsdemix : formats are different.\n"));
 err2:
    return csound->PerfError(csound, Str("pvsdemix : not initialised \n"));
}

static OENTRY localops[] =
  {
    {"pvsdemix", sizeof(PVSDEMIX), 3, "f", "ffkki", (SUBR) pvsdemix_init, (SUBR) pvsdemix_process, (SUBR) NULL }
  };

int pvsdemix_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int) (sizeof(localops) / sizeof(OENTRY)));
}

