/* pvsdemix.c:
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

#ifndef OK
#define OK 1
#endif

#ifndef NOTOK
#define NOTOK 0
#endif

int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2)
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

int pvsdemix_init(CSOUND *csound, PVSDEMIX *p)
{
    long N = p->finleft->N;
    int olap = p->finleft->overlap;
    p->beta = (int)(*p->slices);

    if (p->fout->frame.auxp==NULL)
      csound->AuxAlloc(csound, (N+2)*sizeof(float),&p->fout->frame);

    if (p->left.auxp==NULL)
      csound->AuxAlloc(csound, (N+2)*sizeof(float)*p->beta, &p->left);

    if (p->right.auxp==NULL)
      csound->AuxAlloc(csound, (N+2)*sizeof(float)*p->beta, &p->right);

    if (p->maxl.auxp==NULL)
      csound->AuxAlloc(csound, (N/2+1)*sizeof(float), &p->maxl);

    if (p->maxr.auxp==NULL)
      csound->AuxAlloc(csound, (N/2+1)*sizeof(float), &p->maxr);

    if (p->minl.auxp==NULL)
      csound->AuxAlloc(csound, (N/2+1)*sizeof(float), &p->minl);

    if (p->minr.auxp==NULL)
      csound->AuxAlloc(csound, (N/2+1)*sizeof(float), &p->minr);

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

int pvsdemix_process(CSOUND *csound, PVSDEMIX *p)
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

    if (!fsigs_equal(p->finleft,p->finright))
      csound->Die(csound, "pvsdemix : formats are different.\n");

    if (out==NULL)
      csound->Die(csound, "pvsdemix : not initialised \n");

    if (p->lastframe < p->finleft->framecount) {

      if (width > beta) width = (MYFLT) beta;
      else if (width < 1) width = (MYFLT) 1;

      if (azimuth < -1) azimuth = (MYFLT) -1;
      else if (azimuth > 1) azimuth = (MYFLT) 1;

      imax = beta*framesize;
      range = width/FL(2.0);
      pos = (int)((azimuth >= 0 ? azimuth : -azimuth)*beta);

      /*  create the azimuth amplitude vectors &
          find the max/min values for channels, per bin */
      for (n=0; n < N/2+1; n++){
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
}

static OENTRY localops[] = {
{"pvsdemix", sizeof(PVSDEMIX), 3, "f", "ffkki",(SUBR)pvsdemix_init,
                                               (SUBR)pvsdemix_process, NULL }
};

LINKAGE

