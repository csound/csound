/*
    syncgrain.c: Synchronous granular synthesis

    Copyright (c) Victor Lazzarini, 2004

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

#include "stdopcod.h"
#include "syncgrain.h"
#include "soundio.h"
#include "interlocks.h"

/*
#ifdef HAVE_VALUES_H
#include <values.h>
#endif

#ifndef MAXINT
#include <limits.h>
#define MAXINT INT_MAX
#endif
*/

static int32_t syncgrain_init(CSOUND *csound, syncgrain *p)
{
    int32_t size;
    p->efunc = csound->FTFind(csound, p->ifn2);
    if (UNLIKELY(p->efunc == NULL))
      return NOTOK;

    p->sfunc = csound->FTFind(csound, p->ifn1);
    if (UNLIKELY(p->sfunc == NULL))
      return NOTOK;

    p->olaps = (int32_t) *p->ols+2;

    if (UNLIKELY(p->olaps < 2))
      p->olaps = 2;

    size =  (p->olaps) * sizeof(double);
    csound->AuxAlloc(csound, size, &p->index);
    csound->AuxAlloc(csound, size, &p->envindex);
    csound->AuxAlloc(csound, size, &p->envincr);
    size = (p->olaps) * sizeof(int32_t);
    csound->AuxAlloc(csound, size, &p->streamon);
 

    p->count = 0;                  /* sampling period counter */

    p->numstreams = 0;                  /* curr num of streams */
    p->firststream = 0;                 /* streams index (first stream)  */
    p->datasize =  p->sfunc->flen;
    p->envtablesize = p->efunc->flen;   /* size of envtable */

    p->start = 0.0f;
    p->frac = 0.0f;

    return OK;
}

static int32_t syncgrain_process(CSOUND *csound, syncgrain *p)
{
    MYFLT   sig, pitch, amp, grsize, envincr, period, fperiod, prate;
    MYFLT   *output = p->output;
    MYFLT   *datap = p->sfunc->ftable;
    MYFLT   *ftable = p->efunc->ftable;

    float   start = p->start, frac = p->frac;
    double  *index = (double *) p->index.auxp;
    double  *envindex = (double *) p->envindex.auxp;
    double  *envincrn = (double *) p->envincr.auxp;
    int32_t     *streamon = (int32_t *) p->streamon.auxp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t vecpos, vecsize=CS_KSMPS;
    int32_t firststream = p->firststream;
    int32_t     numstreams = p->numstreams, olaps = p->olaps;
    int32_t     count = p->count, j, newstream;
    int32_t     datasize = p->datasize, envtablesize = p->envtablesize;
    MYFLT      pscale =  p->sfunc->gen01args.sample_rate/CS_ESR;

    pitch  = *p->pitch * pscale;
    fperiod = FABS(p->sfunc->gen01args.sample_rate/(*p->fr));
    //if (UNLIKELY(fperiod  < 0)) fperiod = -fperiod;
    amp =    *p->amp;
    grsize = p->sfunc->gen01args.sample_rate * *p->grsize;
    if (UNLIKELY(grsize<1)) goto err1;
    envincr = envtablesize/grsize;
    prate = *p->prate * pscale;

    if (UNLIKELY(offset)) memset(output, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      vecsize -= early;
      memset(&output[vecsize], '\0', early*sizeof(MYFLT));
    }
    for (vecpos = offset; vecpos < vecsize; vecpos++) {
      sig = FL(0.0);
      /* if a grain has finished, clean up */
      if (UNLIKELY((!streamon[firststream]) && (numstreams) )) {
        //numstreams--; /* decrease the no of streams */
        firststream=(firststream+1)%olaps; /* first stream is the next */
      }

      /* if a fund period has elapsed */
      /* start a new grain */
      period = fperiod - frac;
      if (count == 0 || count >= period) {
        if (count) frac = count - period; /* frac part to be accummulated */
        newstream =(firststream+numstreams)%olaps;
        streamon[newstream] = 1; /* turn the stream on */
        envindex[newstream] = 0.0;
        envincrn[newstream]  = envtablesize/grsize;
        index[newstream] = start;
        numstreams++; /* increase the stream count */
        count = 0;
        start += prate*grsize;
        while (UNLIKELY(start >= datasize)) start-=datasize;
        while (UNLIKELY(start < 0)) start+=datasize;
      }

      //for (i=numstreams,
      //      j=firststream; i; i--, j=(j+1)%olaps) {
      for(j=0; j < olaps; j++){
        if(!streamon[j]) continue;

        /* modulus */
        while (UNLIKELY(index[j] >= datasize))
          index[j] -= datasize;
        while (UNLIKELY(index[j] < 0))
          index[j] += datasize;

       if (UNLIKELY(envindex[j] < envtablesize)){
        /* sum all the grain streams */
        sig += ((datap[(int32_t)index[j]] +
                 (index[j] - (int32_t)index[j])*
                 (datap[(int32_t)index[j]+1] - datap[(int32_t)index[j]])
                 ) *
                (ftable[(int32_t)envindex[j]] +
                 (envindex[j] - (int32_t)envindex[j])*
                 (ftable[(int32_t)envindex[j]+1] - ftable[(int32_t)envindex[j]])
                 )
                );
        }
        /* increment the indexes */
        /* for each grain */
        index[j] += pitch;
        envindex[j] += envincr;

        /* if the envelope is finished */
        /* the grain is also finished */
        if (UNLIKELY(envindex[j] >= envtablesize)) {
          streamon[j] = 0;
          numstreams--;
        }
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
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("grain size smaller than 1 sample\n"));
}


static int32_t syncgrainloop_init(CSOUND *csound, syncgrainloop *p)
{
    p->efunc = csound->FTFind(csound, p->ifn2);
    if (UNLIKELY(p->efunc == NULL))
      return NOTOK;

    p->sfunc = csound->FTFind(csound, p->ifn1);
    if (UNLIKELY(p->sfunc == NULL))
      return NOTOK;

    p->datasize =  p->sfunc->flen;
    p->envtablesize = p->efunc->flen;   /* size of envtable */
    p->olaps = (int32_t) *p->ols+1;

    if (UNLIKELY(p->olaps <2))
      p->olaps = 2;

    if (*p->iskip == 0) {
      int32_t size =  (p->olaps) * sizeof(double);
      if (p->index.auxp == NULL || p->index.size < (uint32_t)size)
        csound->AuxAlloc(csound, size, &p->index);
      if (p->envindex.auxp == NULL || p->envindex.size < (uint32_t)size)
        csound->AuxAlloc(csound, size, &p->envindex);
      size = (p->olaps) * sizeof(int32_t);
      if (p->streamon.auxp == NULL || p->streamon.size < (uint32_t)size)
        csound->AuxAlloc(csound, size, &p->streamon);
      p->count = 0;                  /* sampling period counter */
      p->numstreams = 0;                  /* curr num of streams */
      p->firststream = 0;                 /* streams index (first stream)  */
      p->start = *p->startpos*(p->sfunc->gen01args.sample_rate);
      p->frac = 0.0f;
      p->firsttime = 1;
    }
    return OK;
}

static int32_t syncgrainloop_process(CSOUND *csound, syncgrainloop *p)
{
    MYFLT   sig, pitch, amp, grsize, envincr, period, fperiod, prate;
    MYFLT   *output = p->output;
    MYFLT   *datap = p->sfunc->ftable;
    MYFLT   *ftable = p->efunc->ftable;
    int32_t     *streamon = (int32_t *) p->streamon.auxp;
    float   start = p->start, frac = p->frac;
    double  *index = (double *) p->index.auxp;
    double  *envindex = (double *) p->envindex.auxp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t vecpos, vecsize=CS_KSMPS;
    int32_t      firststream = p->firststream;
    int32_t     numstreams = p->numstreams, olaps = p->olaps;
    int32_t     count = p->count, i,j, newstream;
    int32_t     datasize = p->datasize, envtablesize = p->envtablesize;
    int32_t     loop_start;
    int32_t     loop_end;
    int32_t     loopsize;
    int32_t     firsttime = p->firsttime;
    MYFLT   sr = p->sfunc->gen01args.sample_rate;
    MYFLT pscale = sr/CS_ESR;
    /* loop points & checks */
    loop_start = (int32_t) (*p->loop_start*sr);
    loop_end = (int32_t) (*p->loop_end*sr);
    if (UNLIKELY(loop_start < 0)) loop_start = 0;
    if (UNLIKELY(loop_start >= datasize)) loop_start = datasize-1;
    loop_end = (loop_start > loop_end ? loop_start : loop_end);
    loopsize = loop_end - loop_start;
    /*csound->Message(csound, "st:%d, end:%d, loopsize=%d\n",
                              loop_start, loop_end, loopsize);     */

    pitch  = *p->pitch * pscale;
    fperiod = FABS(sr/(*p->fr));
    //if (UNLIKELY(fperiod  < 0)) fperiod = -fperiod;
    amp =    *p->amp;
    grsize = sr * *p->grsize;
    if (UNLIKELY(grsize<1)) goto err1;
    if (loopsize <= 0) loopsize = grsize;
    envincr = envtablesize/grsize;
    prate = *p->prate * pscale;

    if (UNLIKELY(offset)) memset(output, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      vecsize -= early;
      memset(&output[vecsize], '\0', early*sizeof(MYFLT));
    }
    for (vecpos = offset; vecpos < vecsize; vecpos++) {
      sig = FL(0.0);
      /* if a grain has finished, clean up */
      if (UNLIKELY((!streamon[firststream]) && (numstreams) )) {
        numstreams--; /* decrease the no of streams */
        firststream=(firststream+1)%olaps; /* first stream is the next */
      }

      /* if a fund period has elapsed */
      /* start a new grain */
      period = fperiod - frac;
      if (UNLIKELY(count == 0 || count >= period)) {
        if (count) frac = count - period; /* frac part to be accummulated */
        newstream =(firststream+numstreams)%olaps;
        streamon[newstream] = 1; /* turn the stream on */
        envindex[newstream] = 0.0;
        index[newstream] = start;
        numstreams++; /* increase the stream count */
        count = 0;
        start += prate*grsize;
        /* this will keep syncgrain looping within the
           loop boundaries */
        while (UNLIKELY(start >= loop_end)) {
          firsttime = 0;
          start -= loopsize;
          /*csound->Message(csound, "st:%d, end:%d, loopsize=%d\n",
                                    loop_start, loop_end, loopsize); */
        }
        while (UNLIKELY(start < loop_start && !firsttime))
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

        if (UNLIKELY(envindex[j] < envtablesize)){

          /* sum all the grain streams */
          sig += ((datap[(int32_t)index[j]] +
                   (index[j] - (int32_t)index[j])*
                   (datap[(int32_t)index[j]+1] - datap[(int32_t)index[j]])
                   ) *
                  (ftable[(int32_t)envindex[j]] +
                   (envindex[j] - (int32_t)envindex[j])*
                   (ftable[(int32_t)envindex[j]+1] - ftable[(int32_t)envindex[j]])
                   )
                  );

          /* increment the indexes */
          /* for each grain */
          index[j] += pitch;
          envindex[j] += envincr;
        }
        /* if the envelope is finished */
        /* the grain is also finished */

        if (UNLIKELY(envindex[j] > envtablesize))
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
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("grain size smaller than 1 sample\n"));
}

#define DGRAIN_MAXCHAN 4
#define DGRAIN_OUTTYPES "mmmm"

typedef struct _filegrain {
    OPDS    h;
    MYFLT   *output[DGRAIN_MAXCHAN];
    STRINGDAT   *fname;
    MYFLT   *amp;
    MYFLT   *fr;
    MYFLT   *pitch;
    MYFLT   *grsize;
    MYFLT   *prate;
    MYFLT   *ifn2;
    MYFLT   *ols;
    MYFLT   *max;
    MYFLT   *ioff;
    FUNC    *efunc;
    SNDFILE *sf;
    AUXCH   buffer;
    int32_t     count, numstreams, firststream;
    int32_t     dataframes, envtablesize, olaps;
    AUXCH   streamon;
    AUXCH   index;
    AUXCH   envindex;
    float   start,frac;
    int32_t     read1,read2;
    uint32  pos;
    float   trigger;
    int32_t     nChannels;
    int64_t  flen;
    MYFLT pscale;
    MYFLT sr;
} filegrain;

#define MINFBUFSIZE  88200

static int32_t filegrain_init(CSOUND *csound, filegrain *p)
{
    int32_t size;
    void *fd;
    MYFLT *buffer;
    SFLIB_INFO sfinfo;
    char *fname = p->fname->data;

    p->nChannels = (int32_t) (p->OUTOCOUNT);
    if (UNLIKELY(p->nChannels < 1 || p->nChannels > DGRAIN_MAXCHAN)) {
      return csound->InitError(csound,
                               "%s", Str("diskgrain: invalid number of channels"));
    }
    p->efunc = csound->FTFind(csound, p->ifn2);
    if (UNLIKELY(p->efunc == NULL))
      return NOTOK;

    p->olaps = (int32_t) *p->ols + 1;
    p->dataframes = (int32_t)(*p->max*CS_ESR*4);
    if (p->dataframes < MINFBUFSIZE)
      p->dataframes =  MINFBUFSIZE;
    if (UNLIKELY(p->olaps < 2))
      p->olaps = 2;

    size =  (p->olaps) * sizeof(double);
    if (p->index.auxp == NULL || p->index.size < (uint32_t)size)
      csound->AuxAlloc(csound, size, &p->index);
    if (p->envindex.auxp == NULL || p->envindex.size < (uint32_t)size)
      csound->AuxAlloc(csound, size, &p->envindex);
    size = (p->olaps) * sizeof(int32_t);
    if (p->streamon.auxp == NULL || p->streamon.size < (uint32_t)size)
      csound->AuxAlloc(csound, size, &p->streamon);
    if (p->buffer.auxp == NULL ||
        p->buffer.size < (p->dataframes+1)*sizeof(MYFLT)*p->nChannels)
      csound->AuxAlloc(csound,
                       (p->dataframes+1)*sizeof(MYFLT)*p->nChannels, &p->buffer);

    buffer = (MYFLT *) p->buffer.auxp;
    memset(&sfinfo, '\0', sizeof(sfinfo)); /* for Valgrind */
    /* open file and read the first block using *p->ioff */
    fd = csound->FileOpen(csound, &(p->sf), CSFILE_SND_R, fname, &sfinfo,
                            "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    memset(buffer, 0,p->buffer.size);
    if (UNLIKELY(fd == NULL)) {
      return csound->InitError(csound, Str("diskgrain: could not open file: %s\n"),
                               Str(csound->FileError(csound,NULL)));
    }
    if (UNLIKELY(sfinfo.channels != p->nChannels)) {
      return
        csound->InitError(csound, "%s", Str("diskgrain: soundfile channel numbers "
                                      "do not match the number of outputs\n"));
    }

    p->sr = sfinfo.samplerate;
    p->pscale = p->sr/CS_ESR;

    if (*p->ioff >= 0)
      csound->SndfileSeek(csound, p->sf,*p->ioff * p->sr, SEEK_SET);

    if (LIKELY(csound->SndfileRead(csound, p->sf,buffer,p->dataframes/2) != 0)) {
      p->read1 = 1;
      p->read2 = 0;
    }
    else {
      return csound->InitError(csound, "%s", Str("diskgrain: could not read file\n"));
    }

   /* -===-  */
    p->count =  0;                      /* sampling period counter */
    p->numstreams = 0;                  /* curr num of streams */
    p->firststream = 0;                 /* streams index (first stream)  */
    p->envtablesize = p->efunc->flen;   /* size of envtable */

    p->start = 0.0f;
    p->frac = 0.0f;
    p->pos = *p->ioff*p->sr;
    p->trigger = 0.0f;
    p->flen = sfinfo.frames;

    return OK;
}

static int32_t filegrain_process(CSOUND *csound, filegrain *p)
{
    MYFLT   sig[DGRAIN_MAXCHAN], pitch, amp, grsize, envincr, period,
            fperiod, prate;
    MYFLT   **output = p->output;
    MYFLT   *datap = (MYFLT *) p->buffer.auxp;
    MYFLT   *ftable = p->efunc->ftable;
    int32_t     *streamon = (int32_t *) p->streamon.auxp;
    float   start = p->start, frac = p->frac, jump;
    double  *index = (double *) p->index.auxp;
    double  *envindex = (double *) p->envindex.auxp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t vecpos, vecsize=CS_KSMPS;
    int32_t     firststream = p->firststream;
    int32_t     numstreams = p->numstreams, olaps = p->olaps;
    int32_t     count = p->count, i,j, newstream;
    int32_t     datasize, hdatasize, envtablesize = p->envtablesize;
    int32_t     dataframes = p->dataframes, hdataframes = p->dataframes/2;
    int32_t     read1 = p->read1, read2 = p->read2;
    int32_t     items, chans = p->nChannels, tndx,endx,n, negpos;
    uint32  pos = p->pos;
    int64_t    flen = p->flen;
    float   trigger = p->trigger, incr;

    memset(sig, 0, DGRAIN_MAXCHAN*sizeof(MYFLT));

    datasize = dataframes*chans;
    hdatasize = hdataframes*chans;

    pitch  = *p->pitch * p->pscale;
    fperiod = FABS(p->sr/(*p->fr));
    //if (UNLIKELY(fperiod  < FL(0.0))) fperiod = -fperiod;
    amp =    *p->amp;
    grsize = p->sr * *p->grsize;
    if (UNLIKELY(grsize<1)) goto err1;
    if (grsize > hdataframes) grsize = hdataframes;
    envincr = envtablesize/grsize;
    prate = *p->prate * p->pscale;

    if (UNLIKELY(offset)) memset(output, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      vecsize -= early;
      memset(&output[vecsize], '\0', early*sizeof(MYFLT));
    }
    for (vecpos = offset; vecpos < vecsize; vecpos++) {
      /* sig = (MYFLT) 0; */
      /* if a grain has finished, clean up */
      if (UNLIKELY((!streamon[firststream]) && (numstreams) )) {
        numstreams--; /* decrease the no of streams */
        firststream=(firststream+1)%olaps; /* first stream is the next */
      }
      /* if a fund period has elapsed */
      /* start a new grain */
      period = fperiod - frac;
      if (count ==0  || count >= period) {
        if (count) frac = count - period;
        newstream =(firststream+numstreams)%olaps;
        streamon[newstream] = 1;
        envindex[newstream] = 0.0;
        index[newstream] = start;
        numstreams++;
        count = 0;
        incr = prate*grsize;
        start += (incr);
        trigger += (incr);
        jump = grsize*(pitch > 0 ? pitch : -pitch);
        if (incr >= 0) {
          if (trigger >= (dataframes - jump)) {

            trigger -= (dataframes);

            if (!read1) {
              pos += hdataframes;
              csound->SndfileSeek(csound,p->sf,pos,SEEK_SET);

              items = (int32_t)
                csound->SndfileRead(csound, p->sf, datap, hdatasize/chans);
              if (items < hdatasize) {
                csound->SndfileSeek(csound,p->sf, 0, 0);
                csound->SndfileRead(csound,p->sf,datap+items, (hdatasize-items)/chans);
              }
              for (n=0; n < chans; n++)
                datap[hdatasize+n] = datap[hdatasize-chans+n];

              read1 = 1;
              read2 = 0;
            }
          }
          else if (trigger >= (hdataframes - jump)) {

            if (!read2) {

              pos += hdataframes;
              csound->SndfileSeek(csound,p->sf,pos,SEEK_SET);
              items = (int32_t)
                csound->SndfileRead(csound, p->sf, datap, hdatasize/chans);
              if (items < hdatasize) {
                csound->SndfileSeek(csound,p->sf, 0, SEEK_SET);
                csound->SndfileRead(csound,p->sf,datap+items+hdatasize, (hdatasize-items)/chans);
              }
              for (n=0; n < chans; n++)
                datap[datasize+n] = datap[datasize-chans+n];
              read2 = 1;
              read1 = 0;
            }
          }
        }
        else {
          if (trigger < jump) {
            trigger += (dataframes);
            if (!read1) {

              /*this roundabout code is to
                allow us to use an uint64_t
                to hold the file position
                whilst allowing for pos to go negative
              */

              negpos = pos;
              negpos -= hdataframes;
              if (negpos < 0) {
                while(negpos < 0) negpos += flen;
                pos = negpos;
              }
              else pos -= hdataframes;


              /*
                pos -= hdataframes;
                if (pos < 0)  pos += flen;
              */

              csound->SndfileSeek(csound,p->sf,pos,SEEK_SET);
              items = (int32_t)
                csound->SndfileRead(csound,p->sf,datap+hdatasize,hdatasize/chans);
              if (items < hdatasize) {
                csound->SndfileSeek(csound,p->sf,items-hdatasize,SEEK_END);
                csound->SndfileRead(csound,p->sf,datap+hdatasize+items, (hdatasize-items)/chans);
              }

              for (n=0; n < chans; n++)
                datap[datasize+n] = datap[datasize-chans+n];
              read1 = 1;
              read2 = 0;
            }
          }
          else if (trigger <= (hdataframes + jump)) {
            if (!read2) {

              negpos = pos;
              negpos -= hdataframes;
              if (negpos < 0) {
                while(negpos < 0) negpos += flen;
                pos = negpos;
              }
              else pos -= hdataframes;
              /*
                pos -= hdataframes;
                if (pos < 0)  pos += flen;
              */
              csound->SndfileSeek(csound,p->sf,pos,SEEK_SET);
              items = (int32_t)
                csound->SndfileRead(csound, p->sf,datap,hdatasize/chans);
              if (items < hdatasize) {
                csound->SndfileSeek(csound,p->sf,items-hdatasize,SEEK_END);
                csound->SndfileRead(csound,p->sf,datap+items,(hdatasize-items)/chans);
              }
              for (n=0; n < chans; n++)
                datap[hdatasize+n] = datap[hdatasize-chans+n];

              read2 = 1;
              read1 = 0;
            }
          }
        }

        if (start >= dataframes) start -= dataframes;
        if (start < 0) start += dataframes;
      }

      for (i=numstreams,
             j=firststream; i; i--, j=(j+1)%olaps) {

        /* modulus */
        if (index[j] >= dataframes)
          index[j] -= dataframes;
        if (index[j]  < 0)
          index[j] += dataframes;

        /* sum all the grain streams */
        tndx = (int32_t)index[j]*chans;
        endx = (int32_t) envindex[j];
        /* sig[0] = sig[1] = sig[2] = sig[3] = 0.0; */
        for (n=0; n < chans; n++) {

          sig[n] += ((datap[tndx+n] +
                      (index[j] - (int32_t)index[j])*
                      (datap[tndx+n+chans] - datap[tndx+n])
                      ) *
                     (ftable[endx] +
                      (envindex[j] - endx)*
                      (ftable[endx+1] - ftable[endx])
                      )
                     );
        }

        /* increment the indexes */
        /* for each grain */
        index[j] += (pitch);
        envindex[j] += envincr;

        /* if the envelope is finished */
        /* the grain is also finished */
        if (envindex[j] > envtablesize)
          streamon[j] = 0;
      }

      /* increment the period counter */
      count++;
      /* scale the output */
      for (n=0; n < chans; n++) {
        output[n][vecpos] = sig[n]*amp;
        sig[n] = 0;
      }
    }

    p->firststream = firststream;
    p->numstreams = numstreams;
    p->count = count;
    p->start = start;
    p->frac = frac;
    p->trigger = trigger;
    p->read1 = read1;
    p->read2 = read2;
    p->pos = pos;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("grain size smaller than 1 sample\n"));
}



static OENTRY localops[] =
  {
   {"syncgrain", sizeof(syncgrain), TR,  "a", "kkkkkiii",
    (SUBR)syncgrain_init,(SUBR)syncgrain_process },
   {"syncloop", sizeof(syncgrainloop), TR, "a", "kkkkkkkiiioo",
    (SUBR)syncgrainloop_init,(SUBR)syncgrainloop_process },
   {"diskgrain", sizeof(filegrain), TR,  DGRAIN_OUTTYPES, "Skkkkkiipo",
    (SUBR)filegrain_init,(SUBR)filegrain_process }

};

int32_t syncgrain_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
