/* syncgrain.c:
   Synchronous granular synthesis

   (c) Victor Lazzarini, 2004

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
    License along with Csound; if not, write t    p->pi = 4*atan(1.);
o the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA

*/


#include "csdl.h"
#include "syncgrain.h"


/*extern FUNC *ftnp2find(strunc ENVIRON_*, MYFLT*); */

int syncgrain_init(ENVIRON *csound, syncgrain *p)
{
    p->efunc = ftfind(p->h.insdshead->csound,p->ifn2);
    if (p->efunc==NULL) {
      initerror("function table not found\n");
      return NOTOK;
    }

    p->sfunc = ftnp2find(p->h.insdshead->csound,p->ifn1);
    if (p->sfunc==NULL) {
      initerror("function table not found\n");
      return NOTOK;
    }

   p->olaps = (int) *p->ols;

   if (p->olaps < 1) p->olaps = 1;

   if (p->index.auxp==NULL)
     auxalloc(p->olaps*sizeof(float),&p->index);
   if (p->envindex.auxp==NULL)
     auxalloc(p->olaps*sizeof(float),&p->envindex);
   if (p->streamon.auxp==NULL)
     auxalloc(p->olaps*sizeof(int),&p->streamon);

   p->count = 0xFFFFFFFF;    /* sampling period counter */
   p->numstreams = 0;  /* curr num of streams */
   p->firststream = 0; /* streams index (first stream)  */
   p->datasize =  p->sfunc->flen;
   p->envtablesize = p->efunc->flen; /* size of envtable */

   memset(p->streamon.auxp,0,sizeof(int)*p->olaps);
   memset(p->index.auxp,0,sizeof(float)*p->olaps);
   memset(p->streamon.auxp,0,sizeof(float)*p->olaps);

   p->start = 0.0f;
   p->frac = 0.0f;

   return OK;
}

int syncgrain_process(ENVIRON *csound, syncgrain *p)
{
    MYFLT sig, pitch, amp, grsize, envincr, period, fperiod, prate;
    MYFLT *output = p->output;
    MYFLT *datap = p->sfunc->ftable;
    MYFLT *ftable = p->efunc->ftable;
    int *streamon = (int *)p->streamon.auxp;
    float start = p->start, frac = p->frac;
    float *index = (float *) p->index.auxp, *envindex = (float *) p->envindex.auxp;
    int vecpos, vecsize=ksmps, firststream = p->firststream;
    int numstreams = p->numstreams, olaps = p->olaps;
    int count = p->count, i,j, newstream;
    int datasize = p->datasize, envtablesize = p->envtablesize;

    pitch  = *p->pitch;
    fperiod = esr/(*p->fr);
    amp =    *p->amp;
    grsize = esr * *p->grsize;
    if (grsize<1) {
      perferror("grain size smaller than 1 sample\n");
      return NOTOK;
    }
    envincr = envtablesize/grsize;
    prate = *p->prate;

    for(vecpos = 0; vecpos < vecsize; vecpos++) {
      sig = (MYFLT) 0;
      /* if a grain has finished, clean up */
      if ((!streamon[firststream]) && (numstreams) ) {
        numstreams--; /* decrease the no of streams */
        firststream=(firststream+1)%olaps; /* first stream is the next */
      }

      /* if a fund period has elapsed */
      /* start a new grain */
      period = fperiod + frac;
      if (count >= (period-1)) {
        frac = count - period; /* frac part to be accummulated */
        newstream =(firststream+numstreams)%olaps;
        streamon[newstream] = 1; /* turn the stream on */
        envindex[newstream] = 0.f;
        index[newstream] = start;
        numstreams++; /* increase the stream count */
        count = 0;
        start += prate*grsize;
        while (start >= datasize) start-=datasize;
        while (start < 0) start+=datasize;
      }

      for (i=numstreams,
             j=firststream; i; i--, j=(j+1)%olaps) {

        /* modulus */
        while (index[j] >= datasize)
          index[j] -= datasize;
        while(index[j] < 0)
          index[j] += datasize;

        /* sum all the grain streams */
        sig += ((datap[(int)index[j]] +
                 (index[j] - (int)index[j])*
                 (datap[(int)index[j]+1] - datap[(int)index[j]])
                 ) *
                (ftable[(int)envindex[j]] +
                 (envindex[j] - (int)envindex[j])*
                 (ftable[(int)envindex[j]+1] - ftable[(int)envindex[j]])
                 )
                );

        /* increment the indexes */
        /* for each grain */
        index[j] += pitch;
        envindex[j] += envincr;

        /* if the envelope is finished */
        /* the grain is also finished */

        if (envindex[j] > envtablesize)
          streamon[j] = 0;
      }

      /* increment the period counter */
      count++;
      /* scale the output */
      output[vecpos] = sig*amp;
    }

    p->firststream = firststream;
    p->numstreams = numstreams;
    p->count = count;
    p->start = start;
    p->frac = frac;

    return OK;
}


static OENTRY localops[] = {
{"syncgrain", sizeof(syncgrain), 5, "a", "kkkkkiii",(SUBR)syncgrain_init, NULL,(SUBR)syncgrain_process }
};


LINKAGE

