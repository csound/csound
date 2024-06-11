/*  psynth.c

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

/* TRADSYN

Streaming partial track additive synthesis

asig tradsyn  fin, kscal, kpitch, kmaxtracks, ifn

asig - output signal
fin - TRACKS streaming spectral signal
kscal - amplitude scaling
kpitch - pitch scaling
kmaxtracks - max output tracks
ifn - function table containing a sinusoid (sine or cosine)

SINSYN

Streaming partial track additive synthesis with cubic phase interpolation

asig sinsyn  fin, kscal, kmaxtracks, ifn

asig - output signal
fin - TRACKS streaming spectral signal
kscal - amplitude scaling
kmaxtracks - max output tracks
ifn - function table containing a sinusoid (generally a cosine)

RESYN

Streaming partial track additive synthesis with cubic phase interpolation
with pitch scaling and support for timescale-modified input signals.

asig sinsyn  fin, kscal, kpitch, kmaxtracks, ifn

asig - output signal
fin - TRACKS streaming spectral signal
kscal - amplitude scaling
kpitch - pitch scaling
kmaxtracks - max output tracks
ifn - function table containing a sinusoid (generally a cosine)

PLUS a number of track processing opcodes.

*/

#include "pvs_ops.h"
#include "pstream.h"

typedef struct _psyn {
    OPDS    h;
    MYFLT   *out;
    PVSDAT  *fin;
    MYFLT   *scal, *pitch, *maxtracks, *ftb, *thresh;
    int32_t     tracks, pos, numbins, hopsize;
    FUNC    *func;
    AUXCH   sum, amps, freqs, phases, trackID;
    double   factor, facsqr, min;
} _PSYN;

typedef struct _psyn2 {
    OPDS    h;
    MYFLT   *out;
    PVSDAT  *fin;
    MYFLT   *scal, *maxtracks, *ftb, *thresh;
    int32_t     tracks, pos, numbins, hopsize;
    FUNC    *func;
    AUXCH   sum, amps, freqs, phases, trackID;
    double   factor, facsqr, min;
} _PSYN2;

static int32_t psynth_init(CSOUND *csound, _PSYN *p)
{
    int32_t     numbins = p->fin->N / 2 + 1;

    if (UNLIKELY(p->fin->format != PVS_TRACKS)) {
      return csound->InitError(csound,
                               "%s", Str("psynth: first input not in TRACKS format\n"));
    }
    p->func = csound->FTFind(p->h.insdshead->csound, p->ftb);
    if (UNLIKELY(p->func == NULL)) {
      return csound->InitError(csound, "%s", Str("psynth: function table not found\n"));
    }

    p->tracks = 0;
    p->hopsize = p->fin->overlap;
    p->pos = 0;
    p->numbins = numbins;
    p->factor = p->hopsize * CS_ONEDSR;
    p->facsqr = p->factor * p->factor;
    if(*p->thresh == -1) p->min = 0.00002*csound->Get0dBFS(csound);
    else p->min = *p->thresh*csound->Get0dBFS(csound);

    if (p->amps.auxp == NULL ||
        (uint32_t) p->amps.size < sizeof(double) * numbins)
      csound->AuxAlloc(csound, sizeof(double) * numbins, &p->amps);
    else
      memset(p->amps.auxp, 0, sizeof(double) * numbins );
    if (p->freqs.auxp == NULL ||
        (uint32_t) p->freqs.size < sizeof(double) * numbins)
      csound->AuxAlloc(csound, sizeof(double) * numbins, &p->freqs);
    else
      memset(p->freqs.auxp, 0, sizeof(double) * numbins );
    if (p->phases.auxp == NULL ||
        (uint32_t) p->phases.size < sizeof(double) * numbins)
      csound->AuxAlloc(csound, sizeof(double) * numbins, &p->phases);
    else
      memset(p->phases.auxp, 0, sizeof(double) * numbins );
    if (p->sum.auxp == NULL ||
        (uint32_t) p->sum.size < sizeof(double) * p->hopsize)
      csound->AuxAlloc(csound, sizeof(double) * p->hopsize, &p->sum);
    else
      memset(p->sum.auxp, 0, sizeof(double) * p->hopsize );
    if (p->trackID.auxp == NULL ||
        (uint32_t) p->trackID.size < sizeof(int32_t) * numbins)
      csound->AuxAlloc(csound, sizeof(int32_t) * numbins, &p->trackID);
    else
      memset(p->trackID.auxp, 0, sizeof(int32_t) * numbins );

    return OK;
}

static int32_t psynth_process(CSOUND *csound, _PSYN *p)
{
    double  ampnext, amp, freq, freqnext, phase, ratio;
    double  a, f, frac, incra, incrph, factor;
    MYFLT   scale = *p->scal, pitch = *p->pitch;
    int32_t     ndx, size = p->func->flen;
    int32_t     i, j, k, m, id;
    int32_t     notcontin = 0;
    int32_t     contin = 0;
    int32_t     tracks = p->tracks, maxtracks = (int32_t) *p->maxtracks;
    MYFLT   *tab = p->func->ftable, *out = p->out;
    float   *fin = (float *) p->fin->frame.auxp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      pos = p->pos;
    double   *amps = (double *) p->amps.auxp, *freqs = (double *) p->freqs.auxp;
    double   *phases = (double *) p->phases.auxp;
    MYFLT    *outsum = (MYFLT *) p->sum.auxp;
    int32_t     *trackID = (int32_t *) p->trackID.auxp;
    int32_t     hopsize = p->hopsize;
    double  min = p->min;
    ratio = size * CS_ONEDSR;
    factor = p->factor;

    maxtracks = p->numbins > maxtracks ? maxtracks : p->numbins;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      out[n] = outsum[pos];
      pos++;
      if (pos == hopsize) {
        memset(outsum, 0, sizeof(MYFLT) * hopsize);
        /* for each track */
        i = j = k = 0;
        while (i < maxtracks * 4) {

          ampnext = (double) fin[i] * scale;
          freqnext = (double) fin[i + 1] * pitch;
          if ((id = (int32_t) fin[i + 3]) != -1) {
            j = k + notcontin;

            if (k < tracks - notcontin) {
              if (trackID[j] == id) {
                /* if this is a continuing track */
                contin = 1;
                freq = freqs[j];
                phase = phases[j];
                amp = amps[j];

              }
              else {
                /* if this is  a dead track */
                contin = 0;
                freqnext = freq = freqs[j];
                phase = phases[j];
                amp = amps[j];
                ampnext = FL(0.0);
              }
            }
            else {
              /* new track */
              contin = 1;
              freq = freqnext;
              phase = -freq * factor;
              amp = FL(0.0);

            }
            if (amp > min) {
              /* interpolation & track synthesis loop */
              a = amp;
              f = freq;
              incra = (ampnext - amp) / hopsize;
              incrph = (freqnext - freq) / hopsize;
              for (m = 0; m < hopsize; m++) {
                /* table lookup oscillator */
                phase += f * ratio;
                while (phase < 0)
                  phase += size;
                while (phase >= size)
                  phase -= size;
                ndx = (int32_t) phase;
                frac = phase - ndx;
                outsum[m] += a * (tab[ndx] + (tab[ndx + 1] - tab[ndx]) * frac);
                a += incra;
                f += incrph;
              }
            }
            /* keep amp, freq, and phase values for next time */
            if (contin) {

              amps[k] = ampnext;
              freqs[k] = freqnext;
              phases[k] = phase;
              trackID[k] = id;
              i += 4;
              k++;
            }
            else
              notcontin++;
          }
          else
            break;
        }
        pos = 0;
        p->tracks = k;
      }
    }
    p->pos = pos;
    return OK;
}

static int32_t psynth2_init(CSOUND *csound, _PSYN2 *p)
{
    int32_t     numbins = p->fin->N / 2 + 1;

    if (UNLIKELY(p->fin->format != PVS_TRACKS)) {
      return csound->InitError(csound,
                               "%s", Str("psynth: first input not in TRACKS format\n"));
    }
    p->func = csound->FTFind(p->h.insdshead->csound, p->ftb);
    if (UNLIKELY(p->func == NULL)) {
      return csound->InitError(csound, "%s", Str("psynth: function table not found\n"));
    }

    p->tracks = 0;
    p->hopsize = p->fin->overlap;
    p->pos = 0;
    p->numbins = numbins;
    p->factor = p->hopsize * CS_ONEDSR;
    p->facsqr = p->factor * p->factor;
    if(*p->thresh == -1) p->min = 0.00002*csound->Get0dBFS(csound);
    else p->min = *p->thresh*csound->Get0dBFS(csound);

    if (p->amps.auxp == NULL ||
        (uint32_t) p->amps.size < sizeof(double) * numbins)
      csound->AuxAlloc(csound, sizeof(double) * numbins, &p->amps);
    else
      memset(p->amps.auxp, 0, sizeof(double) * numbins );
    if (p->freqs.auxp == NULL ||
        (uint32_t) p->freqs.size < sizeof(double) * numbins)
      csound->AuxAlloc(csound, sizeof(double) * numbins, &p->freqs);
    else
      memset(p->freqs.auxp, 0, sizeof(double) * numbins );
    if (p->phases.auxp == NULL ||
        (uint32_t) p->phases.size < sizeof(double) * numbins)
      csound->AuxAlloc(csound, sizeof(double) * numbins, &p->phases);
    else
      memset(p->phases.auxp, 0, sizeof(double) * numbins  );
    if (p->sum.auxp == NULL ||
        (uint32_t) p->sum.size < sizeof(double) * p->hopsize)
      csound->AuxAlloc(csound, sizeof(double) * p->hopsize, &p->sum);
    else
      memset(p->sum.auxp, 0, sizeof(double) * p->hopsize );
    if (p->trackID.auxp == NULL ||
        (uint32_t) p->trackID.size < sizeof(int32_t) * numbins)
      csound->AuxAlloc(csound, sizeof(int32_t) * numbins, &p->trackID);
    else
      memset(p->trackID.auxp, 0, sizeof(int32_t) * numbins );

    return OK;
}

static int32_t psynth2_process(CSOUND *csound, _PSYN2 *p)
{
    double   ampnext, amp, freq, freqnext, phase, phasenext;
    double  a2, a3, cph;
    double   phasediff, facsqr, ph;
    double   a, frac, incra, incrph, factor, lotwopi, cnt;
    MYFLT   scale = *p->scal;
    int32_t     ndx, size = p->func->flen;
    int32_t     i=0, j, k, m, id;
    int32_t     notcontin = 0;
    int32_t     contin = 0;
    int32_t     tracks = p->tracks, maxtracks = (int32_t) *p->maxtracks;
    MYFLT   *tab = p->func->ftable, *out = p->out;
    float   *fin = (float *) p->fin->frame.auxp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      pos = p->pos;
    double   *amps = (double *) p->amps.auxp, *freqs = (double *) p->freqs.auxp;
    double   *phases = (double *) p->phases.auxp;
    MYFLT   *outsum = (MYFLT *) p->sum.auxp;
    int32_t     *trackID = (int32_t *) p->trackID.auxp;
    int32_t     hopsize = p->hopsize;
    double  min = p->min;

    incrph = CS_ONEDSR;
    lotwopi = (double)(size) / TWOPI_F;
    factor = p->factor;
    facsqr = p->facsqr;
    maxtracks = p->numbins > maxtracks ? maxtracks : p->numbins;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      out[n] = outsum[pos];
      pos++;
      if (UNLIKELY(pos == hopsize)) {

        memset(outsum, 0, sizeof(MYFLT) * hopsize);
        /* for each track */
        i = j = k = 0;
        while (i < maxtracks * 4) {
          ampnext = (double) fin[i] * scale;
          freqnext = (double) fin[i + 1] * TWOPI_F;
          phasenext = (double) fin[i + 2];
          if ((id = (int32_t) fin[i + 3]) != -1) {

            j = k + notcontin;

            if (k < tracks - notcontin) {
              if (trackID[j] == id) {
                /* if this is a continuing track */
                contin = 1;
                freq = freqs[j];
                phase = phases[j];
                amp = amps[j];
              }
              else {
                /* if this is  a dead track */
                contin = 0;
                freqnext = freq = freqs[j];
                phase = phases[j];
                phasenext = phase + freq * factor;
                amp = amps[j];
                ampnext = FL(0.0);
              }
            }
            else {
              /* new track */
              contin = 1;
              goto cont;
#if 0
              freq = freqnext;
              phase = phasenext - freq * factor;
              amp = FL(0.0);
#endif
            }
            if (amp > min) {
              /* phasediff */
              phasediff = phasenext - phase;
              while (phasediff >= PI_F)
                phasediff -= TWOPI_F;
              while (phasediff < -PI_F)
                phasediff += TWOPI_F;

              /* update phasediff to match the freq */
              cph = ((freq + freqnext) * factor * 0.5 - phasediff) / TWOPI;

              phasediff += TWOPI_F * (int32_t) (cph + 0.5);
              /* interpolation coefs */
              a2 = 3.0 / facsqr * (phasediff -
                                   factor / 3.0 * (2.0 * freq + freqnext));
              a3 = 1.0 / (3.0 * facsqr) * (freqnext - freq - 2.0 * a2 * factor);
              //printf("%f %f \n", a2, a3);
              /* interpolation & track synthesis loop */
              a = amp;
              ph = phase;
              cnt = 0;
              incra = (ampnext - amp) / hopsize;
              for (m = 0; m < hopsize; m++) {
                /* table lookup oscillator */
                ph *= lotwopi;
                while (ph < 0)
                  ph += size;
                while (ph >= size)
                  ph -= size;
                ndx = (int32_t) ph;
                frac = ph - ndx;
                outsum[m] += a * (tab[ndx] + (tab[ndx + 1] - tab[ndx]) * frac);
                a += incra;
                cnt += incrph;
                ph = phase + cnt * (freq + cnt * (a2 + a3 * cnt));
              }
            }
            /* keep amp, freq, and phase values for next time */
          cont:
            if (contin) {
              amps[k] = ampnext;
              freqs[k] = freqnext;
              phases[k] = phasenext;
              trackID[k] = id;
              i += 4;
              k++;
            }
            else
              notcontin++;
          }
          else
            break;

        }
        pos = 0;
        p->tracks = k;
      }

    }
    p->pos = pos;

    return OK;
}

static int32_t psynth3_process(CSOUND *csound, _PSYN *p)
{
    double   ampnext, amp, freq, freqnext, phase, phasenext;
    double  a2, a3, cph=0.0;
    double   phasediff, facsqr, ph;
    double   a, frac, incra, incrph, factor, lotwopi, cnt;
    MYFLT   scale = *p->scal, pitch = *p->pitch;
    int32_t     ndx, size = p->func->flen;
    int32_t     i, j, k, m, id;
    int32_t     notcontin = 0;
    int32_t     contin = 0;
    int32_t     tracks = p->tracks, maxtracks = (int32_t) *p->maxtracks;
    MYFLT   *tab = p->func->ftable, *out = p->out;
    float   *fin = (float *) p->fin->frame.auxp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      pos = p->pos;
    double   *amps = (double *) p->amps.auxp, *freqs = (double *) p->freqs.auxp;
    double   *phases = (double *) p->phases.auxp;
    MYFLT    *outsum = (MYFLT *) p->sum.auxp;
    int32_t     *trackID = (int32_t *) p->trackID.auxp;
    int32_t     hopsize = p->hopsize;
    double  min = p->min;

    incrph = CS_ONEDSR;
    lotwopi = (double) (size) / TWOPI_F;
    factor = p->factor;
    facsqr = p->facsqr;
    maxtracks = p->numbins > maxtracks ? maxtracks : p->numbins;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      out[n] = outsum[pos];
      pos++;
      if (UNLIKELY(pos == hopsize)) {
        memset(outsum, 0, sizeof(MYFLT) * hopsize);
        /* for each track */
        i = j = k = 0;
        while (i < maxtracks * 4) {
          ampnext = (double) fin[i] * scale;
          freqnext = (double) fin[i + 1] * TWOPI_F * pitch;
          phasenext = (double) fin[i + 2];
          if ((id = (int32_t) fin[i + 3]) != -1) {
            j = k + notcontin;

            if (k < tracks - notcontin) {
              if (trackID[j] == id) {
                /* if this is a continuing track */
                contin = 1;
                freq = freqs[j];
                phase = phases[j];
                amp = amps[j];
              }
              else {
                /* if this is  a dead track */
                contin = 0;
                freqnext = freq = freqs[j];
                phase = phases[j];
                phasenext = phase + freq * factor;
                amp = amps[j];
                ampnext = FL(0.0);
              }
            }
            else {
              /* new track */
              contin = 1;
              freq = freqnext;
              phase = phasenext - freq * factor;
              amp = FL(0.0);
            }
            if (amp > min) {
              /* phasediff */
              phasediff = phasenext - phase;
              while (phasediff >= PI_F)
                phasediff -= TWOPI_F;
              while (phasediff < -PI_F)
                phasediff += TWOPI_F;

              /* update phasediff to match the freq */
              cph = ((freq + freqnext) * factor *0.5 - phasediff) / TWOPI;
              phasediff += TWOPI_F * cph;
              /* interpolation coefs */
              a2 = 3.0 / facsqr * (phasediff -
                                   factor / 3.0 * (2.0 * freq + freqnext));
              a3 = 1.0 / (3.0 * facsqr) * (freqnext - freq - 2.0 * a2 * factor);
              /* interpolation & track synthesis loop */
              a = amp;
              ph = phase;
              cnt = 0;
              incra = (ampnext - amp) / hopsize;
              for (m = 0; m < hopsize; m++) {
                /* table lookup oscillator */
                ph *= lotwopi;
                while (ph < 0)
                  ph += size;
                while (ph >= size)
                  ph -= size;
                ndx = (int32_t) ph;
                frac = ph - ndx;
                outsum[m] += a * (tab[ndx] + (tab[ndx + 1] - tab[ndx]) * frac);
                a += incra;
                cnt += incrph;
                ph = phase + cnt * (freq + cnt * (a2 + a3 * cnt));
              }
            }
            /* keep amp, freq, and phase values for next time */
            if (contin) {
              amps[k] = ampnext;
              freqs[k] = freqnext;
              phasenext += (cph - (int32_t) cph) * TWOPI;
              while (phasenext < 0)
                phasenext += TWOPI_F;
              while (phasenext >= TWOPI_F)
                phasenext -= TWOPI_F;
              phases[k] = phasenext;
              trackID[k] = id;
              i += 4;
              k++;
            }
            else
              notcontin++;
          }
          else
            break;
        }
        pos = 0;
        p->tracks = k;

      }
    }
    p->pos = pos;
    return OK;
}

typedef struct _ptrans {
    OPDS    h;
    PVSDAT  *fout;
    PVSDAT  *fin;
    MYFLT   *kpar;
    MYFLT   *kgain;
    MYFLT   *pad1;
    MYFLT   *pad2;
    uint32_t lastframe;
    int32_t     numbins;
} _PTRANS;

static int32_t trans_init(CSOUND *csound, _PTRANS *p)
{
    int32_t     numbins;

    if (UNLIKELY(p->fin->format != PVS_TRACKS)) {
      return csound->InitError(csound, "%s", Str("Input not in TRACKS format\n"));
    }

    p->numbins = numbins = (p->fout->N = p->fin->N) / 2 + 1;
    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * numbins * 4)
      csound->AuxAlloc(csound, sizeof(float) * numbins * 4, &p->fout->frame);
    ((float *) p->fout->frame.auxp)[3] = -1.0f;

    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->framecount = 1;
    p->fout->format = PVS_TRACKS;
    p->lastframe = 0;

    return OK;
}

static int32_t trscale_process(CSOUND *csound, _PTRANS *p)
{
    MYFLT   scale = *p->kpar;
    MYFLT   gain = (p->kgain != NULL ? *p->kgain : FL(1.0));
    MYFLT   nyq = CS_ESR * FL(0.5);
    float   *framein = (float *) p->fin->frame.auxp;
    float   *frameout = (float *) p->fout->frame.auxp;
    int32_t i = 0, id /* = (int32_t) framein[3]*/, end = p->numbins * 4;
    MYFLT   outfr;

    if (p->lastframe < p->fin->framecount) {
      do {
        if (gain != FL(1.0))
          frameout[i] = framein[i] * gain;
        else
          frameout[i] = framein[i];
        outfr = framein[i + 1] * scale;
        frameout[i + 1] = (float) (outfr < nyq ? outfr : nyq);
        frameout[i + 2] = framein[i + 2];
        id = (int32_t) framein[i + 3];
        frameout[i + 3] = (float) id;
        i += 4;
      } while (id != -1 && i < end);
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }

    return OK;
}

static int32_t trshift_process(CSOUND *csound, _PTRANS *p)
{
    MYFLT   shift = *p->kpar;
    MYFLT   gain = (p->kgain != NULL ? *p->kgain : FL(1.0));
    MYFLT   nyq = CS_ESR * FL(0.5);
    float   *framein = (float *) p->fin->frame.auxp;
    float   *frameout = (float *) p->fout->frame.auxp;
    int32_t     i = 0, id /* = (int32_t) framein[3]*/, end = p->numbins * 4;
    MYFLT   outfr;

    if (p->lastframe < p->fin->framecount) {
      do {
        if (gain != FL(1.0))
          frameout[i] = framein[i] * gain;
        else
          frameout[i] = framein[i];
        outfr = framein[i + 1] + shift;
        frameout[i + 1] = (float) (outfr < nyq ? outfr : nyq);
        frameout[i + 2] = framein[i + 2];
        id = (int32_t) framein[i + 3];
        frameout[i + 3] = (float) id;
        i += 4;
      } while (id != -1 && i < end);
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }

    return OK;
}

typedef struct _plow {
    OPDS    h;
    PVSDAT  *fout;
    MYFLT   *kfr;
    MYFLT   *kamp;
    PVSDAT  *fin;
    MYFLT   *kpar;
    uint32_t lastframe;
    int32_t     numbins;
} _PLOW;

static int32_t trlowest_init(CSOUND *csound, _PLOW *p)
{
    int32_t     numbins;

    if (UNLIKELY(p->fin->format != PVS_TRACKS)) {
      return csound->InitError(csound, "%s", Str("Input not in TRACKS format\n"));
    }

    p->numbins = numbins = (p->fout->N = p->fin->N) / 2 + 1;
    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * numbins * 4)
      csound->AuxAlloc(csound, sizeof(float) * numbins * 4, &p->fout->frame);
    ((float *) p->fout->frame.auxp)[3] = -1.0f;

    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->framecount = 1;
    p->fout->format = PVS_TRACKS;
    p->lastframe = 0;

    return OK;
}

static int32_t trlowest_process(CSOUND *csound, _PLOW *p)
{
    MYFLT   scale = *p->kpar;
    MYFLT   nyq = CS_ESR * FL(0.5);
    float   lowest = (float) nyq, outamp = 0.0f, outph = 0.0f, outid = -1.0f;
    float   *framein = (float *) p->fin->frame.auxp;
    float   *frameout = (float *) p->fout->frame.auxp;
    int32_t i = 0, id /* = (int32_t) framein[3]*/, end = p->numbins * 4;

    if (p->lastframe < p->fin->framecount) {
      do {
        if (framein[i + 1] < lowest && framein[i] > 0.0f) {
          lowest = framein[i + 1];
          outamp = framein[i];
          outph = framein[i + 2];
          outid = framein[i + 3];
        }
        id = (int32_t) framein[i + 3];
        i += 4;
      } while (id != -1 && i < end);
      frameout[0] = outamp * scale;
      frameout[1] = lowest;
      frameout[2] = outph;
      frameout[3] = outid;
      frameout[7] = -1.0f;
      *p->kfr = (MYFLT) lowest;
      *p->kamp = (MYFLT) frameout[0];
      p->fout->framecount = p->lastframe = p->fin->framecount;
/*csound->Message(csound, "lowest %f\n", lowest);*/
    }

    return OK;
}

static int32_t trhighest_process(CSOUND *csound, _PLOW *p)
{
    IGN(csound);
    MYFLT   scale = *p->kpar;
    float   highest = 0.0f, outamp = 0.0f, outph = 0.0f, outid = -1.0f;
    float   *framein = (float *) p->fin->frame.auxp;
    float   *frameout = (float *) p->fout->frame.auxp;
    int32_t i = 0, id /* = (int32_t) framein[3]*/, end = p->numbins * 4;

    if (p->lastframe < p->fin->framecount) {
      do {
        if (framein[i + 1] > highest && framein[i] > 0.0f) {
          highest = framein[i + 1];
          outamp = framein[i];
          outph = framein[i + 2];
          outid = framein[i + 3];
        }
        id = (int32_t) framein[i + 3];
        i += 4;
      } while (id != -1 && i < end);
      frameout[0] = outamp * scale;
      frameout[1] = highest;
      frameout[2] = outph;
      frameout[3] = outid;
      frameout[7] = -1.0f;
      *p->kfr = (MYFLT) highest;
      *p->kamp = (MYFLT) frameout[0];
      p->fout->framecount = p->lastframe = p->fin->framecount;
/*csound->Message(csound, "lowest %f\n", lowest);*/
    }

    return OK;
}

typedef struct _psplit {
    OPDS    h;
    PVSDAT  *fsig1;
    PVSDAT  *fsig2;
    PVSDAT  *fsig3;
    MYFLT   *kpar;
    MYFLT   *kgain1;
    MYFLT   *kgain2;
    MYFLT   *pad1;
    MYFLT   *pad2;
    uint32_t lastframe;
    int32_t     numbins;
} _PSPLIT;

static int32_t trsplit_init(CSOUND *csound, _PSPLIT *p)
{
    int32_t     numbins;

    if (UNLIKELY(p->fsig3->format != PVS_TRACKS)) {
      return csound->InitError(csound, "%s", Str("trsplit: input not "
                                           "in TRACKS format\n"));
    }

    p->numbins = numbins = (p->fsig2->N = p->fsig1->N = p->fsig3->N) / 2 + 1;
    if (p->fsig1->frame.auxp == NULL ||
        p->fsig1->frame.size < sizeof(float) * numbins * 4)
      csound->AuxAlloc(csound, sizeof(float) * numbins * 4, &p->fsig1->frame);
    ((float *) p->fsig1->frame.auxp)[3] = -1.0f;

    p->fsig1->overlap = p->fsig3->overlap;
    p->fsig1->winsize = p->fsig3->winsize;
    p->fsig1->wintype = p->fsig3->wintype;
    p->fsig1->framecount = 1;
    p->fsig1->format = PVS_TRACKS;
    if (p->fsig2->frame.auxp == NULL ||
        p->fsig2->frame.size < sizeof(float) * numbins * 4)
      csound->AuxAlloc(csound, sizeof(float) * numbins * 4, &p->fsig2->frame);

    ((float *) p->fsig2->frame.auxp)[3] = -1.0f;
    p->fsig2->overlap = p->fsig3->overlap;
    p->fsig2->winsize = p->fsig3->winsize;
    p->fsig2->wintype = p->fsig3->wintype;
    p->fsig2->framecount = 1;
    p->fsig2->format = PVS_TRACKS;

    p->lastframe = 0;

    return OK;
}

static int32_t trsplit_process(CSOUND *csound, _PSPLIT *p)
{
    IGN(csound);
    MYFLT   split = *p->kpar;
    MYFLT   gain1 = (p->kgain1 != NULL ? *p->kgain1 : FL(1.0));
    MYFLT   gain2 = (p->kgain2 != NULL ? *p->kgain2 : FL(1.0));
    float   *framein = (float *) p->fsig3->frame.auxp;
    float   *frameout1 = (float *) p->fsig1->frame.auxp;
    float   *frameout2 = (float *) p->fsig2->frame.auxp;
    int32_t i = 0, id /* = (int32_t) framein[3]*/, end = p->numbins * 4;
    int32_t     trkcnt1 = 0, trkcnt2 = 0;

    if (p->lastframe < p->fsig3->framecount) {
      do {

        if (framein[i + 1] < split) {
          if (gain1 != FL(1.0))
            frameout1[trkcnt1] = framein[i] * gain1;
          else
            frameout1[trkcnt1] = framein[i];
          frameout1[trkcnt1 + 1] = framein[i + 1];
          frameout1[trkcnt1 + 2] = framein[i + 2];
          id = (int32_t) framein[i + 3];
          frameout1[trkcnt1 + 3] = (float) id;
          trkcnt1 += 4;
        }
        else {
          if (gain2 != FL(1.0))
            frameout2[trkcnt2] = framein[i] * gain2;
          else
            frameout2[trkcnt2] = framein[i];
          frameout2[trkcnt2 + 1] = framein[i + 1];
          frameout2[trkcnt2 + 2] = framein[i + 2];
          id = (int32_t) framein[i + 3];
          frameout2[trkcnt2 + 3] = (float) id;
          trkcnt2 += 4;
        }
        i += 4;
      } while (id != -1 && i < end);
      if (trkcnt1)
        frameout1[trkcnt1 - 1] = -1.0f;
      if (trkcnt2)
        frameout2[trkcnt2 - 1] = -1.0f;
      p->fsig2->framecount = p->fsig1->framecount = p->lastframe =
          p->fsig3->framecount;
/*csound->Message(csound, "split %d : %d\n", trkcnt1/4, trkcnt2/4);*/
    }

    return OK;
}

typedef struct _psmix {
    OPDS    h;
    PVSDAT  *fsig1;
    PVSDAT  *fsig2;
    PVSDAT  *fsig3;
    uint32_t lastframe;
    int32_t     numbins;
} _PSMIX;

static int32_t trmix_init(CSOUND *csound, _PSMIX *p)
{
    int32_t     numbins;

    if (UNLIKELY(p->fsig2->format != PVS_TRACKS)) {
      return csound->InitError(csound,
                               "%s", Str("trmix: first input not in TRACKS format\n"));
    }

    if (UNLIKELY(p->fsig3->format != PVS_TRACKS)) {
      return csound->InitError(csound,
                               "%s", Str("trmix: second input not in TRACKS format\n"));
    }

    p->numbins = numbins = (p->fsig1->N = p->fsig2->N) / 2 + 1;
    if (p->fsig1->frame.auxp == NULL ||
        p->fsig1->frame.size < sizeof(float) * numbins * 4)
      csound->AuxAlloc(csound, sizeof(float) * numbins * 4, &p->fsig1->frame);
    ((float *) p->fsig1->frame.auxp)[3] = -1.0f;

    p->fsig1->overlap = p->fsig2->overlap;
    p->fsig1->winsize = p->fsig2->winsize;
    p->fsig1->wintype = p->fsig2->wintype;
    p->fsig1->framecount = 1;
    p->fsig1->format = PVS_TRACKS;
    p->lastframe = 0;

    return OK;
}

static int32_t trmix_process(CSOUND *csound, _PSMIX *p)
{
     IGN(csound);
    float   *framein2 = (float *) p->fsig3->frame.auxp;
    float   *frameout = (float *) p->fsig1->frame.auxp;
    float   *framein1 = (float *) p->fsig2->frame.auxp;
    int32_t   i = 0, j = 0, id = (int32_t) framein1[3], end = p->numbins * 4;

    if (p->lastframe < p->fsig2->framecount) {

      while (id != -1 && i < end) {
        frameout[i] = framein1[i];
        frameout[i + 1] = framein1[i + 1];
        frameout[i + 2] = framein1[i + 2];
        frameout[i + 3] = (float) id;
        i += 4;
        id = (int32_t) framein1[i + 3];
      }
      id = (int32_t) framein2[3];
      while (id != -1 && i < end && j < end) {
        frameout[i] = framein2[j];
        frameout[i + 1] = framein2[j + 1];
        frameout[i + 2] = framein2[j + 2];
        frameout[i + 3] = (float) id;
        i += 4;
        j += 4;
        id = (int32_t) framein2[j + 3];
      }
      if (i + 3 < p->numbins * 4)
        frameout[i + 3] = -1.0f;
      p->fsig1->framecount = p->lastframe = p->fsig2->framecount;

    }

    return OK;
}

typedef struct _psfil {
    OPDS    h;
    PVSDAT  *fout;
    PVSDAT  *fin;
    MYFLT   *kpar;
    MYFLT   *ifn;
    FUNC    *tab;
    int32_t     len;
    uint32_t lastframe;
    int32_t     numbins;
} _PSFIL;

static int32_t trfil_init(CSOUND *csound, _PSFIL *p)
{
    int32_t     numbins;

    if (UNLIKELY(p->fin->format != PVS_TRACKS)) {
      return csound->InitError(csound,
                               "%s", Str("trfil: input not in TRACKS format\n"));
    }
    p->tab = csound->FTFind(csound, p->ifn);
    if (UNLIKELY(p->tab == NULL)) {
      return csound->InitError(csound,
                               "%s", Str("trfil: could not find function table\n"));
    }
    p->len = p->tab->flen;
    p->numbins = numbins = (p->fout->N = p->fin->N) / 2 + 1;
    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * numbins * 4)
      csound->AuxAlloc(csound, sizeof(float) * numbins * 4, &p->fout->frame);
    ((float *) p->fout->frame.auxp)[3] = -1.0f;

    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->framecount = 1;
    p->fout->format = PVS_TRACKS;
    p->lastframe = 0;

    return OK;
}

static int32_t trfil_process(CSOUND *csound, _PSFIL *p)
{
    MYFLT   amnt = *p->kpar, gain = FL(1.0);
    MYFLT   nyq = CS_ESR * FL(0.5);
    MYFLT   *fil = p->tab->ftable;
    float   *framein = (float *) p->fin->frame.auxp;
    float   *frameout = (float *) p->fout->frame.auxp;
    int32_t i = 0, id /* = (int32_t) framein[3]*/, len = p->len, end = p->numbins * 4;

    if (p->lastframe < p->fin->framecount) {
      MYFLT   fr, pos = FL(0.0), frac = FL(0.0);
      int32_t     posi = 0;

      if (UNLIKELY(amnt > 1))
        amnt = 1;
      if (UNLIKELY(amnt < 0))
        amnt = 0;
      do {
        fr = framein[i + 1];
        if (UNLIKELY(fr > nyq))
          fr = nyq;
        //if (fr < 0)
        fr = FABS(fr);
        pos = fr * len / nyq;
        posi = (int32_t) pos;
        frac = pos - posi;
        gain = fil[posi] + frac * (fil[posi + 1] - fil[posi]);
        frameout[i] = (float) (framein[i] * (FL(1.0) - amnt + gain * amnt));
        frameout[i + 1] = fr;
        frameout[i + 2] = framein[i + 2];
        id = (int32_t) framein[i + 3];
        frameout[i + 3] = (float) id;
        i += 4;
      } while (id != -1 && i < end);
      if (i - 1 < p->numbins * 4)
        frameout[i - 1] = -1.0f;
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }

    return OK;
}

typedef struct _pscross {
    OPDS    h;
    PVSDAT  *fsig1;
    PVSDAT  *fsig2;
    PVSDAT  *fsig3;
    MYFLT   *kpar1;
    MYFLT   *kpar2;
    MYFLT   *kpar3;
    uint32_t lastframe;
    int32_t     numbins;
} _PSCROSS;

static int32_t trcross_init(CSOUND *csound, _PSCROSS *p)
{
    int32_t     numbins;

    if (UNLIKELY(p->fsig2->format != PVS_TRACKS)) {
      return csound->InitError(csound,
                               "%s", Str("trmix: first input not in TRACKS format\n"));
    }

    if (UNLIKELY(p->fsig3->format != PVS_TRACKS)) {
      return csound->InitError(csound,
                               "%s", Str("trmix: second input not in TRACKS format\n"));
    }

    p->numbins = numbins = (p->fsig1->N = p->fsig2->N) / 2 + 1;
    if (p->fsig1->frame.auxp == NULL ||
        p->fsig1->frame.size < sizeof(float) * numbins * 4)
      csound->AuxAlloc(csound, sizeof(float) * numbins * 4, &p->fsig1->frame);
    ((float *) p->fsig1->frame.auxp)[3] = -1.0f;

    p->fsig1->overlap = p->fsig2->overlap;
    p->fsig1->winsize = p->fsig2->winsize;
    p->fsig1->wintype = p->fsig2->wintype;
    p->fsig1->framecount = 1;
    p->fsig1->format = PVS_TRACKS;
    p->lastframe = 0;

    return OK;
}

static int32_t trcross_process(CSOUND *csound, _PSCROSS *p)
{
     IGN(csound);
    MYFLT   interval = *p->kpar1, bal = *p->kpar2;
    int32_t mode = p->kpar3 != NULL ? (int32_t) *p->kpar3 : 0;
    float   *framein2 = (float *) p->fsig3->frame.auxp;
    float   *frameout = (float *) p->fsig1->frame.auxp;
    float   *framein1 = (float *) p->fsig2->frame.auxp;
    int32_t     i = 0, j = 0, nomatch = 1, id, end = p->numbins * 4;
    float   max = 0;
    MYFLT   boundup, boundown;
    int32_t     maxj = -1;

    id = (int32_t) framein1[3];

    if (p->lastframe < p->fsig2->framecount) {
      if (bal > 1)
        bal = FL(1.0);
      if (bal < 0)
        bal = FL(0.0);
      if (mode < 1)
        for (i = 0; i < end && framein2[i + 3] != -1; i += 4)
          max = framein2[i] > max ? framein2[i] : max;

      for (i = 0; id != -1 && i < end; i += 4, id = (int32_t) framein1[i + 3]) {

        boundup = framein1[i + 1] * interval;
        boundown = framein1[i + 1] * (FL(1.0) / interval);
        for (j = 0; j < end && framein2[j + 3] != -1; j += 4) {
          if ((framein2[j + 1] > boundown) && (framein2[j + 1] <= boundup)) {
            if (maxj != -1)
              maxj = framein2[j] > framein2[maxj] ? j : maxj;
            else
              maxj = j;
            nomatch = 0;
          }
        }
        if (!nomatch) {
          if (mode < 1)
            frameout[i] =
                (float) ((framein1[i] * (max ? framein2[maxj] / max : 1.0f)) *
                         bal + framein1[i] * (FL(1.0) - bal));
          else
            frameout[i] =
                (float) (framein2[maxj] * bal + framein1[i] * (FL(1.0) - bal));
          frameout[i + 1] = framein1[i + 1];
          frameout[i + 2] = framein1[i + 2];
          frameout[i + 3] = (float) id;
        }
        else {
          frameout[i] = (float) (framein1[i] * (FL(1.0) - bal));
          frameout[i + 1] = framein1[i + 1];
          frameout[i + 2] = framein1[i + 2];
          frameout[i + 3] = (float) id;
        }
        nomatch = 1;
        maxj = -1;
      }

      if (i + 3 < p->numbins * 4)
        frameout[i + 3] = -1.0f;
      p->fsig1->framecount = p->lastframe = p->fsig2->framecount;

/*csound->Message(csound, "mix %d : %d\n", k/4, j/4);*/
    }

    return OK;
}

typedef struct _psbin {
    OPDS    h;
    PVSDAT  *fsig1;
    PVSDAT  *fsig2;
    MYFLT   *ipar;
    int32_t     N;
    uint32_t lastframe;
    int32_t     numbins;
} _PSBIN;

static int32_t binit_init(CSOUND *csound, _PSBIN *p)
{
    int32_t     numbins, N;

    if (UNLIKELY(p->fsig2->format != PVS_TRACKS)) {
      return csound->InitError(csound,
                               "%s", Str("binit: first input not in TRACKS format\n"));
    }

    N = p->N = (int32_t) *p->ipar;
    p->numbins = numbins = p->fsig2->N / 2 + 1;
    if (p->fsig1->frame.auxp == NULL ||
        p->fsig1->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, sizeof(float) * (N + 2), &p->fsig1->frame);

    p->fsig1->overlap = p->fsig2->overlap;
    p->fsig1->winsize = p->fsig2->winsize;
    p->fsig1->wintype = p->fsig2->wintype;
    p->fsig1->framecount = 1;
    p->fsig1->format = PVS_AMP_FREQ;
    p->fsig1->N = N;
    p->lastframe = 0;

    return OK;
}

static int32_t binit_process(CSOUND *csound, _PSBIN *p)
{
    int32_t N = (int32_t) p->N;
    float   *frameout = (float *) p->fsig1->frame.auxp;
    float   *framein = (float *) p->fsig2->frame.auxp;
    int32_t i = 0, n = 0, id = (int32_t) framein[3], end = p->numbins * 4;
    int32_t     maxi = -1;
    MYFLT   bw = CS_ESR / (MYFLT)N, boundup, boundown;
    MYFLT   nyq = CS_ESR * FL(0.5), centre;

    if (p->lastframe < p->fsig2->framecount) {

      for (n = 2; n < N; n += 2) {

        centre = (n / 2) * bw;
        boundup = (n == N - 2 ? nyq : centre + (bw / 2));
        boundown = (n == 2 ? 0 : centre - (bw / 2));

        for (i = 0; id != -1 && i < end; i += 4, id = (int32_t) framein[i + 3]) {
          if ((framein[i + 1] > boundown) && (framein[i + 1] <= boundup)) {
            if (maxi != -1)
              maxi = (framein[i] > framein[maxi] ? i : maxi);
            else
              maxi = i;
          }
        }

        if (maxi != -1) {
          frameout[n] = framein[maxi];
          frameout[n + 1] = framein[maxi + 1];
          maxi = -1;
        }
        else {
          frameout[n] = 0.f;
          frameout[n + 1] = 0.f;
        }
        id = (int32_t) framein[3];
      }

      frameout[0] = 0.f;
      frameout[N] = 0.f;
      p->fsig1->framecount = p->lastframe = p->fsig2->framecount;
    }

    return OK;
}

static OENTRY localops[] =
  {
   {"tradsyn", sizeof(_PSYN),0,  "a", "fkkkij", (SUBR) psynth_init,
     (SUBR) psynth_process}
    ,
   {"sinsyn", sizeof(_PSYN2), TR, "a", "fkkij", (SUBR) psynth2_init,
     (SUBR) psynth2_process}
    ,
   {"resyn", sizeof(_PSYN), TR, "a", "fkkkij", (SUBR) psynth_init,
     (SUBR) psynth3_process}
    ,
    {"trscale", sizeof(_PTRANS),0,  "f", "fz", (SUBR) trans_init,
     (SUBR) trscale_process}
    ,
    {"trshift", sizeof(_PTRANS),0,  "f", "fz", (SUBR) trans_init,
     (SUBR) trshift_process}
    ,
    {"trsplit", sizeof(_PSPLIT),0,  "ff", "fz", (SUBR) trsplit_init,
     (SUBR) trsplit_process}
    ,
    {"trmix", sizeof(_PSMIX),0,  "f", "ff", (SUBR) trmix_init,
     (SUBR) trmix_process}
    ,
    {"trlowest", sizeof(_PLOW),0,  "fkk", "fk", (SUBR) trlowest_init,
     (SUBR) trlowest_process}
    ,
    {"trhighest", sizeof(_PLOW),0,  "fkk", "fk", (SUBR) trlowest_init,
     (SUBR) trhighest_process}
    ,
    {"trfilter", sizeof(_PSFIL),0,  "f", "fki", (SUBR) trfil_init,
     (SUBR) trfil_process}
    ,
    {"trcross", sizeof(_PSCROSS),0,  "f", "ffkz", (SUBR) trcross_init,
     (SUBR) trcross_process}
    ,
    {"binit", sizeof(_PSBIN),0,  "f", "fi", (SUBR) binit_init,
     (SUBR) binit_process}
  };

int32_t psynth_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t
                                ) (sizeof(localops) / sizeof(OENTRY)));
}
