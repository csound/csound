/*  partials.c

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

/* PARTIALS

Streaming partial track analysis

ftrk partials ffr, fphs, kthresh, kminpts, kmaxgap, imaxtracks

ftrk - TRACKS streaming spectral signal

ffrs - AMP_FREQ input signal
fphs - AMP_PHASE input signal (unwrapped phase expected)
kthresh - analysis threshold (0 <= kthresh <= 1)
kminpts - minimum number of time-points for a partial track
kmaxgap - max gap between detected peaks in a track
imaxtracks - max number of tracks (<= number of analysis bins)

*/

#include "pvs_ops.h"
#include "pstream.h"

typedef struct _parts {
    OPDS    h;
    PVSDAT *fout;
    PVSDAT *fin1, *fin2;
    MYFLT  *kthresh, *pts, *gap, *mtrks;
    int     tracks, numbins, mtracks, prev, cur;
    unsigned long  accum;
    uint32  lastframe, timecount;
    AUXCH   mags, lmags, index, cflag, trkid, trndx;
    AUXCH   tstart, binex, magex, oldbins, diffs, adthresh;
    AUXCH   pmags, bins, lastpk;
    int     nophase;

} _PARTS;

static int partials_init(CSOUND * csound, _PARTS * p)
{

    int     N = p->fin1->N, maxtracks;
    int     numbins = N / 2 + 1, i;
    int    *trkid;
    int    *trndx;

    p->tracks = 0;
    p->mtracks = *p->mtrks;
    p->timecount = 0;
    p->accum = 0;
    p->numbins = numbins;

    maxtracks = (p->mtracks < numbins ? p->mtracks : numbins);

    p->prev = 0;
    p->cur = maxtracks;

    if (p->mags.auxp == NULL || p->mags.size < sizeof(double) * numbins)
      csound->AuxAlloc(csound, sizeof(double) * numbins, &p->mags);
    else
      memset(p->mags.auxp, 0,sizeof(double) * numbins );

    if (p->lmags.auxp == NULL || p->lmags.size < sizeof(double) * numbins)
      csound->AuxAlloc(csound, sizeof(double) * numbins, &p->lmags);
    else
      memset(p->lmags.auxp, 0,sizeof(double) * numbins );

     if (p->cflag.auxp == NULL || p->cflag.size < sizeof(int) * maxtracks)
      csound->AuxAlloc(csound, sizeof(int) * maxtracks, &p->cflag);
     else
       memset(p->cflag.auxp, 0, sizeof(int) * maxtracks);

     if (p->trkid.auxp == NULL || p->trkid.size < sizeof(int) * maxtracks * 2)
      csound->AuxAlloc(csound, sizeof(int) * maxtracks * 2, &p->trkid);
     else
       memset(p->trkid.auxp, 0, sizeof(int) * maxtracks * 2);

     if (p->trndx.auxp == NULL || p->trndx.size < sizeof(int) * maxtracks)
      csound->AuxAlloc(csound, sizeof(int) * maxtracks, &p->trndx);
     else
       memset(p->trndx.auxp, 0, sizeof(int) * maxtracks );

     if (p->index.auxp == NULL || p->index.size < sizeof(int) * numbins)
      csound->AuxAlloc(csound, sizeof(int) * numbins, &p->index);
     else
       memset(p->index.auxp, 0,sizeof(int) * numbins );

     if (p->tstart.auxp == NULL || p->tstart.size < sizeof(uint32) * maxtracks * 2)
      csound->AuxAlloc(csound, sizeof(uint32) * maxtracks * 2, &p->tstart);
     else
       memset(p->tstart.auxp, 0, sizeof(uint32) * maxtracks * 2);

     if (p->lastpk.auxp == NULL ||
         p->lastpk.size < sizeof(uint32) * maxtracks * 2)
       csound->AuxAlloc(csound, sizeof(uint32) * maxtracks * 2, &p->lastpk);
     else
       memset(p->lastpk.auxp, 0, sizeof(uint32) * maxtracks * 2);

     if (p->binex.auxp == NULL || p->binex.size < sizeof(double) * numbins)
       csound->AuxAlloc(csound, sizeof(double) * numbins, &p->binex);
     else
       memset(p->binex.auxp, 0,sizeof(double) * numbins );

     if (p->magex.auxp == NULL || p->magex.size < sizeof(double) * numbins)
       csound->AuxAlloc(csound, sizeof(double) * numbins, &p->magex);
     else
       memset(p->magex.auxp, 0,sizeof(double) * numbins );

     if (p->bins.auxp == NULL || p->bins.size < sizeof(double) * maxtracks)
       csound->AuxAlloc(csound, sizeof(double) * maxtracks, &p->bins);
     else
       memset(p->bins.auxp, 0, sizeof(double) * maxtracks );

     if (p->oldbins.auxp == NULL ||
         p->oldbins.size < sizeof(double) * maxtracks * 2)
       csound->AuxAlloc(csound, sizeof(double) * maxtracks * 2, &p->oldbins);
     else
       memset(p->oldbins.auxp, 0, sizeof(double) * maxtracks * 2);

     if (p->diffs.auxp == NULL || p->diffs.size < sizeof(double) * numbins)
       csound->AuxAlloc(csound, sizeof(double) * numbins, &p->diffs);
     else
       memset(p->diffs.auxp, 0, sizeof(double) * numbins );

     if (p->pmags.auxp == NULL || p->pmags.size < sizeof(double) * maxtracks * 2)
       csound->AuxAlloc(csound, sizeof(double) * maxtracks * 2, &p->pmags);
     else
       memset(p->pmags.auxp, 0, sizeof(double) * maxtracks * 2);

     if (p->adthresh.auxp == NULL ||
         p->adthresh.size < sizeof(double) * maxtracks * 2)
       csound->AuxAlloc(csound, sizeof(double) * maxtracks * 2, &p->adthresh);
     else
       memset(p->adthresh.auxp, 0, sizeof(double) * maxtracks * 2);

     if (p->fout->frame.auxp == NULL ||
         p->fout->frame.size < sizeof(float) * numbins * 4)
       csound->AuxAlloc(csound, sizeof(float) * numbins * 4, &p->fout->frame);
     else
       memset(p->fout->frame.auxp, 0,sizeof(float) * numbins * 4);

    p->fout->N = N;
    p->fout->overlap = p->fin1->overlap;
    p->fout->winsize = p->fin1->winsize;
    p->fout->wintype = p->fin1->wintype;
    p->fout->framecount = 1;
    p->fout->format = PVS_TRACKS;

    trkid = (int *) p->trkid.auxp;
    trndx = (int *) p->trndx.auxp;
    for (i = 0; i < maxtracks; i++)
      trkid[p->cur + i] = trkid[p->prev + i] = trndx[i] = -1;

    p->mtracks = maxtracks;

    if (UNLIKELY(p->fin1->format != PVS_AMP_FREQ)) {
      return csound->InitError(csound,
                               "partials: first input not in AMP_FREQ format \n");
    }

    if (UNLIKELY(p->fin2->format != PVS_AMP_PHASE)) {
      csound->Warning(csound,
                      "partials: no phase input, tracks will contain "
                      "amp & freq only\n");
      p->nophase = 1;
    }
    else
      p->nophase = 0;

    p->lastframe = 0;

    return OK;
}

static void Analysis(CSOUND * csound, _PARTS * p)
{

    float   absthresh, logthresh;
    int     ndx, count = 0, i = 0, n = 0, j = 0;
    float   dbstep;
    double  y1, y2, a, b, dtmp;
    float   ftmp, ftmp2;
    int     numbins = p->numbins, maxtracks = p->mtracks;
    int     prev = p->prev, cur = p->cur, foundcont;
    int     accum = p->accum, minpoints = (int) (*p->pts > 1 ? *p->pts : 1) - 1;
    int     tracks = p->tracks;
    double  *mags = (double *) p->mags.auxp;
    double *lmags = (double *) p->lmags.auxp;
    int    *cflag = (int *) p->cflag.auxp;
    int    *trkid = (int *) p->trkid.auxp;
    int    *trndx = (int *) p->trndx.auxp;
    int    *index = (int *) p->index.auxp;
    uint32 *tstart = (uint32 *) p->tstart.auxp;
    double  *binex = (double *) p->binex.auxp;
    double  *magex = (double *) p->magex.auxp;
    double  *oldbins = (double *) p->oldbins.auxp;
    double  *diffs = (double *) p->diffs.auxp;
    double  *adthresh = (double *) p->adthresh.auxp;
    double  *pmags = (double *) p->pmags.auxp;
    double  *bins = (double *) p->bins.auxp;
    uint32 *lastpk = (uint32 *) p->lastpk.auxp;
    unsigned int timecount = p->timecount,
             maxgap = (unsigned int) (*p->gap > 0 ? *p->gap : 0);
    int     test1 = 1, test2 = 0;

    if(*p->kthresh >= 0) {
    float max = 0.f;
    for (i = 0; i < numbins; i++)
      if (max < mags[i]) {
        max = mags[i];
      }
    absthresh = (float)(*p->kthresh * max);
    } else absthresh = (float)(-*p->kthresh * csound->Get0dBFS(csound));

    logthresh = LOG(absthresh / 5.0f);

    /* Quadratic Interpolation
       obtains bin indexes and magnitudes
       binex & magex respectively
     */

    /* take the logarithm of the magnitudes */
    for (i = 0; i < numbins; i++)
      lmags[i] = log((double)mags[i]);

    for (i = 0; i < numbins - 1; i++) {

      if (i)
        test1 = (lmags[i] > lmags[i - 1] ? 1 : 0);
      else
        test1 = 0;
      test2 = (lmags[i] >= lmags[i + 1] ? 1 : 0);

      if ((lmags[i] > logthresh) && (test1 && test2)) {
        index[n] = i;
        n++;
      }

    }

    for (i = 0; i < n; i++) {
      int     rmax;

      rmax = index[i];

      y1 = lmags[rmax] - (dtmp =
                          (rmax ? lmags[rmax - 1] : lmags[rmax + 1])) +
                          0.000001;
      y2 = (rmax <
            numbins - 1 ? lmags[rmax + 1] : lmags[rmax]) - dtmp + 0.000001;

      a = (y2 - 2.0 * y1) / 2.0;
      b = 1.0 - y1 / a;

      binex[i] = (double) (rmax - 1.0 + b / 2.0);
      magex[i] = (double) exp(dtmp - a * b * b / 4.0);
    }
    /* Track allocation */

    /* reset continuation flags */
    for (i = 0; i < maxtracks; i++) {
      cflag[i] = 0;
    }

    /* loop to the end of tracks (indicate by the 0'd bins)
       find continuation tracks */

    for (j = 0; oldbins[prev + j] != 0.f && j < maxtracks; j++) {

      foundcont = 0;

      if (n > 0) {             /* check for peaks; n will be > 0 */

        ftmp = oldbins[prev + j];

        for (i = 0; i < numbins; i++) {
          diffs[i] = binex[i] - ftmp;  /* differences */
          diffs[i] = (diffs[i] < 0 ? -diffs[i] : diffs[i]);
        }

        ndx = 0;               /* best index */
        for (i = 0; i < numbins; i++)
          if (diffs[i] < diffs[ndx])
            ndx = i;

        /* if difference smaller than 1 bin */
        ftmp2 = ftmp - binex[ndx];
        ftmp2 = (ftmp2 < 0.0f ? -ftmp2 : ftmp2);
        if (ftmp2 < 1.0f) {

          /* if amp jump is too great */
          if (adthresh[prev + j] <
              (dbstep = 20.0f * LOG10(magex[ndx] / pmags[prev + j]))) {
            /* mark for discontinuation */
            cflag[j] = 0;
          }
          else {
            oldbins[prev + j] = binex[ndx];
            pmags[prev + j] = magex[ndx];
            /*
               track index keeps track history
               so we know which ones continue
             */
            cflag[j] = 1;
            binex[ndx] = magex[ndx] = FL(0.0);
            lastpk[prev + j] = timecount;
            foundcont = 1;
            count++;

            /* update adthresh */
            ftmp = dbstep * 1.5f;
            ftmp2 = adthresh[prev + j] -
                (adthresh[prev + j] - 1.5f) * 0.048770575f;
            adthresh[prev + j] = (ftmp > ftmp2 ? ftmp : ftmp2);

          }                     /* else */
        }
      }
      if (foundcont == 0) {
        if ((mags[(int) (oldbins[prev + j] + 0.5f)]) < (0.2 * pmags[prev + j])
            || (timecount - lastpk[prev + j]) > maxgap)
          cflag[j] = 0;
        else {
          cflag[j] = 1;
          count++;
        }
      }

    }                           /* for loop */

    if (count < maxtracks) {

      /* if we have not exceeded available tracks.
         compress the arrays */
      for (i = 0, n = 0; i < maxtracks; i++) {
        if (cflag[i]) {
          oldbins[cur + n] = oldbins[prev + i];
          pmags[cur + n] = pmags[prev + i];
          adthresh[cur + n] = adthresh[prev + i];
          tstart[cur + n] = tstart[prev + i];
          trkid[cur + n] = trkid[prev + i];
          lastpk[cur + n] = lastpk[prev + i];
          n++;
        }                       /* ID == -1 means zeroed track */
        else
          trndx[i] = -1;
      }

      /* now current arrays are the compressed previous
         arrays. Create new tracks */

      for (j = 0; j < numbins && count < maxtracks; j++) {

        if (magex[j] > absthresh) {
          oldbins[cur + count] = binex[j];
          pmags[cur + count] = magex[j];
          adthresh[cur + count] = 400.f;
          /* track ID is a positive number in the
             range of 0 - maxtracks*4 - 1
             it is given when the track starts
             used to identify and match tracks
           */
          tstart[cur + count] = timecount;
          trkid[cur + count] = ((accum++));// % (maxtracks * 1000));
          lastpk[cur + count] = timecount;
          count++;

        }
      }
      for (i = count; i < maxtracks; i++) {
        /* zero the right-hand size of the current arrays */
        pmags[cur + i] = oldbins[cur + i] = adthresh[cur + i] = 0.0f;
        trkid[cur + i] = -1;

      }

    }                           /* if count != maxtracks */

    /* count is the number of continuing tracks + new tracks
       now we check for tracks that have been there for more
       than minpoints hop periods and output them
     */

    tracks = 0;
    for (i = 0; i < maxtracks; i++) {
      if (i < count && tstart[cur + i] <= timecount - minpoints) {
        bins[i] = oldbins[cur + i];
        mags[i] = pmags[cur + i];
        trndx[i] = trkid[cur + i];
        tracks++;
      }
    }
    /* current arrays become previous */
    timecount++;
    p->timecount = timecount;
    p->cur = prev;
    p->prev = cur;
    p->accum = accum;
    p->tracks = tracks;

}

static int partials_process(CSOUND * csound, _PARTS * p)
{

    int     pos, ndx, end, fftsize = p->fin1->N;
    int     numbins = fftsize / 2 + 1, i, k;
    int     tracks, nophase = p->nophase;
    float  *fin1 = p->fin1->frame.auxp;
    float  *fin2 = p->fin2->frame.auxp;
    float  *fout = p->fout->frame.auxp;
    double  *mags = p->mags.auxp;
    double  *bins = p->bins.auxp;
    int    *trndx = p->trndx.auxp;
    double   frac, a, b;

    end = numbins * 4;

    if (p->lastframe < p->fin1->framecount) {

      for (i = k = 0; i < fftsize + 2; i += 2, k++)
        mags[k] = fin1[i];
      Analysis(csound, p);
      /* fout holds [amp, freq, pha, ID] */
      tracks = p->tracks;
      for (i = k = 0; i < end; i += 4, k++) {
        if (k < tracks) {
          /* magnitudes */
          ndx = (int) bins[k];
          fout[i] = (float) mags[k];
          /* fractional part of bin indexes */
          frac = (bins[k] - ndx);
          /* freq interpolation */
          pos = ndx * 2 + 1;
          a = fin1[pos];
          b = (bins[k] < numbins - 1 ? (fin1[pos + 2] - a) : 0);
          fout[i + 1] = (float) (a + frac * b);
          if (!nophase){
            float pha = fin2[pos];
            /* while (pha >= PI_F)
              pha -= TWOPI_F;
            while (pha < -PI_F)
            pha += TWOPI_F; */
            fout[i + 2] = pha;  /* phase (truncated) */
          }
          else
            fout[i + 2] = 0.f;
          fout[i + 3] = (float) trndx[k];  /* trk IDs */
        }
        else {                 /* empty tracks */
          fout[i + 3] = trndx[k];
        }
      }

      p->lastframe = p->fout->framecount = p->fin1->framecount;
    }
    return OK;
}

typedef struct  _partxt{
  OPDS h;
  STRINGDAT *fname;
  PVSDAT *tracks;
  FDCH  fdch;
  FILE *f;
  uint32 lastframe;
} PARTXT;


int part2txt_init(CSOUND *csound, PARTXT *p){

    if (p->fdch.fd != NULL)
      fdclose(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, p->fname->data,
                                   "w", "", CSFTYPE_FLOATS_TEXT, 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), p->fname->data);

    p->lastframe = 0;
    return OK;
}

int part2txt_perf(CSOUND *csound, PARTXT *p){
    float *tracks = (float *) p->tracks->frame.auxp;
    int i = 0;
    if (p->tracks->framecount > p->lastframe){
      for (i=0; tracks[i+3] != -1; i+=4){
        fprintf(p->f, "%f %f %f %d\n",tracks[i],tracks[i+1],
                tracks[i+2], (int) tracks[i+3]);
      }
      fprintf(p->f, "-1.0 -1.0 -1.0 -1\n");
      p->lastframe = p->tracks->framecount;
    }
    return OK;
}

static OENTRY localops[] =
  {
    { "partials", sizeof(_PARTS), 0, 3, "f", "ffkkki",
                            (SUBR) partials_init, (SUBR) partials_process },
    { "part2txt", sizeof(_PARTS), 0, 3, "", "Sf",
                            (SUBR) part2txt_init, (SUBR) part2txt_perf }
  };

int partials_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int) (sizeof(localops) / sizeof(OENTRY)));
}
