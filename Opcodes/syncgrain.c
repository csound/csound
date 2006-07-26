/*
    syncgrain.c: Synchronous granular synthesis

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
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"
#include "syncgrain.h"

static int syncgrain_init(CSOUND *csound, syncgrain *p)
{
    p->efunc = csound->FTFind(csound, p->ifn2);
    if (p->efunc == NULL)
      return NOTOK;

    p->sfunc = csound->FTnp2Find(csound, p->ifn1);
    if (p->sfunc == NULL)
      return NOTOK;

    p->olaps = (int) *p->ols;

    if (p->olaps < 1)
      p->olaps = 1;

    csound->AuxAlloc(csound, p->olaps * sizeof(float), &p->index);
    csound->AuxAlloc(csound, p->olaps * sizeof(float), &p->envindex);
    csound->AuxAlloc(csound, p->olaps * sizeof(int), &p->streamon);

    p->count = 0xFFFFFFFF;              /* sampling period counter */
    p->numstreams = 0;                  /* curr num of streams */
    p->firststream = 0;                 /* streams index (first stream)  */
    p->datasize =  p->sfunc->flen;
    p->envtablesize = p->efunc->flen;   /* size of envtable */

    p->start = 0.0f;
    p->frac = 0.0f;

    return OK;
}

static int syncgrain_process(CSOUND *csound, syncgrain *p)
{
    MYFLT   sig, pitch, amp, grsize, envincr, period, fperiod, prate;
    MYFLT   *output = p->output;
    MYFLT   *datap = p->sfunc->ftable;
    MYFLT   *ftable = p->efunc->ftable;
    int     *streamon = (int *) p->streamon.auxp;
    float   start = p->start, frac = p->frac;
    float   *index = (float *) p->index.auxp;
    float   *envindex = (float *) p->envindex.auxp;
    int     vecpos, vecsize=csound->ksmps, firststream = p->firststream;
    int     numstreams = p->numstreams, olaps = p->olaps;
    int     count = p->count, i,j, newstream;
    int     datasize = p->datasize, envtablesize = p->envtablesize;

    pitch  = *p->pitch;
    fperiod = csound->esr/(*p->fr);
    amp =    *p->amp;
    grsize = csound->esr * *p->grsize;
    if (grsize<1) {
      csound->PerfError(csound, "grain size smaller than 1 sample\n");
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


static int syncgrainloop_init(CSOUND *csound, syncgrainloop *p)
{
    p->efunc = csound->FTFind(csound, p->ifn2);
    if (p->efunc == NULL)
      return NOTOK;

    p->sfunc = csound->FTnp2Find(csound, p->ifn1);
    if (p->sfunc == NULL)
      return NOTOK;
    
    p->datasize =  p->sfunc->flen;
    p->envtablesize = p->efunc->flen;   /* size of envtable */
    p->olaps = (int) *p->ols;

    if (p->olaps < 1)
      p->olaps = 1;

    if(*p->iskip == 0){ 
    csound->AuxAlloc(csound, p->olaps * sizeof(float), &p->index);
    csound->AuxAlloc(csound, p->olaps * sizeof(float), &p->envindex);
    csound->AuxAlloc(csound, p->olaps * sizeof(int), &p->streamon);
    p->count = 0xFFFFFFFF;              /* sampling period counter */
    p->numstreams = 0;                  /* curr num of streams */
    p->firststream = 0;                 /* streams index (first stream)  */
    p->start = *p->startpos*(csound->GetSr(csound));
    p->frac = 0.0f;
    p->firsttime = 1;
    }
    return OK;
}

static int syncgrainloop_process(CSOUND *csound, syncgrainloop *p)
{
    MYFLT   sig, pitch, amp, grsize, envincr, period, fperiod, prate;
    MYFLT   *output = p->output;
    MYFLT   *datap = p->sfunc->ftable;
    MYFLT   *ftable = p->efunc->ftable;
    int     *streamon = (int *) p->streamon.auxp;
    float   start = p->start, frac = p->frac;
    float   *index = (float *) p->index.auxp;
    float   *envindex = (float *) p->envindex.auxp;
    int     vecpos, vecsize=csound->ksmps, firststream = p->firststream;
    int     numstreams = p->numstreams, olaps = p->olaps;
    int     count = p->count, i,j, newstream;
    int     datasize = p->datasize, envtablesize = p->envtablesize;
    int     loop_start;
    int     loop_end;
    int     loopsize;
    int     firsttime = p->firsttime;
    MYFLT   sr = csound->GetSr(csound);

    /* loop points & checks */
    loop_start = *p->loop_start*sr;
    loop_end = *p->loop_end*sr;
    if(loop_start < 0) loop_start = 0; 
    if(loop_start >= datasize) loop_start = datasize-1;
    if(firsttime) start = start > loop_start ? loop_start : start;
    loop_end = (loop_start > loop_end ? loop_start : loop_end);
    loopsize = loop_end - loop_start;

    pitch  = *p->pitch;
    fperiod = csound->esr/(*p->fr);
    amp =    *p->amp;
    grsize = csound->esr * *p->grsize;
    if (grsize<1) {
      csound->PerfError(csound, "grain size smaller than 1 sample\n");
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
        /* this will keep syncgrain looping within the
           loop boundaries */
        while (start >= loop_end) {
                  firsttime = 0;
                  start -= loopsize;
        }
        while (start < loop_start && !firsttime)
	              start += loopsize;
      }
      /* depending on pitch transpsition a
         grain can extend beyond the loop points. 
         it will be wrapped up at the ends of the
         table.
       */
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
    p->firsttime = firsttime;
    return OK;
}


static OENTRY localops[] = {
{"syncgrain", sizeof(syncgrain), 5, "a", "kkkkkiii",
 (SUBR)syncgrain_init, NULL,(SUBR)syncgrain_process },
{"syncloop", sizeof(syncgrainloop), 5, "a", "kkkkkkkiiioo",
                            (SUBR)syncgrainloop_init, NULL,(SUBR)syncgrainloop_process }

};

int syncgrain_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

