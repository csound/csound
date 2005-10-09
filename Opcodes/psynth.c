/*  psynth.c

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

*/

#include "csdl.h"
#include "pstream.h"

typedef struct _psyn {
    OPDS    h;
    MYFLT  *out;
    PVSDAT *fin;
    MYFLT  *scal, *pitch, *maxtracks, *ftb;
    int     tracks, pos, numbins, hopsize;
    FUNC   *func;
    AUXCH   sum, amps, freqs, phases, trackID;
    MYFLT   invsr, factor, facsqr;
    double  pi, twopi;
} _PSYN;

typedef struct _psyn2 {
    OPDS    h;
    MYFLT  *out;
    PVSDAT *fin;
    MYFLT  *scal, *maxtracks, *ftb;
    int     tracks, pos, numbins, hopsize;
    FUNC   *func;
    AUXCH   sum, amps, freqs, phases, trackID;
    MYFLT   invsr, factor, facsqr;
    double  pi, twopi;
} _PSYN2;

static int psynth_init(CSOUND * csound, _PSYN * p)
{
    int     numbins = p->fin->N / 2 + 1;

    if (p->fin->format != PVS_TRACKS) {
      csound->InitError(csound, "psynth: first input not in TRACKS format \n");
      return NOTOK;
    }
    p->func = csound->FTnp2Find(p->h.insdshead->csound, p->ftb);
    if (p->func == NULL) {
      csound->InitError(csound, "psynth: function table not found\n");
      return NOTOK;
    }

    p->tracks = 0;
    p->hopsize = p->fin->overlap;
    p->pos = 0;
    p->numbins = numbins;
    p->invsr = (MYFLT) (1. / csound->esr);
    p->factor = p->hopsize / csound->esr;
    p->facsqr = p->factor * p->factor;
    p->pi = 4. * atan(1.);
    p->twopi = 2. * p->pi;

    if (p->amps.auxp == NULL ||
        (unsigned)p->amps.size < sizeof(MYFLT) * numbins)
      csound->AuxAlloc(csound, sizeof(MYFLT) * numbins, &p->amps);
    if (p->freqs.auxp == NULL ||
        (unsigned)p->freqs.size < sizeof(MYFLT) * numbins)
      csound->AuxAlloc(csound, sizeof(MYFLT) * numbins, &p->freqs);
    if (p->phases.auxp == NULL ||
        (unsigned)p->phases.size < sizeof(MYFLT) * numbins)
      csound->AuxAlloc(csound, sizeof(MYFLT) * numbins, &p->phases);
    if (p->sum.auxp == NULL ||
        (unsigned)p->sum.size < sizeof(MYFLT) * p->hopsize)
      csound->AuxAlloc(csound, sizeof(MYFLT) * p->hopsize, &p->sum);
    if (p->trackID.auxp == NULL ||
        (unsigned)p->trackID.size < sizeof(int) * numbins)
      csound->AuxAlloc(csound, sizeof(int) * numbins, &p->trackID);

    return OK;
}

static int psynth_process(CSOUND * csound, _PSYN * p)
{

    MYFLT   ampnext, amp, freq, freqnext, phase, ratio;
    MYFLT   a, f, frac, incra, incrph, factor;
    MYFLT   scale = *p->scal, pitch = *p->pitch;
    int     ndx, size = p->func->flen;
    int     i, j, k, n, m, id;
    int     notcontin = 0;
    int     contin = 0;
    int     tracks = p->tracks, maxtracks = *p->maxtracks;
    MYFLT  *tab = p->func->ftable, *out = p->out;
    float  *fin = (float *) p->fin->frame.auxp;
    int     ksmps = csound->ksmps, pos = p->pos;
    MYFLT  *amps = (MYFLT *) p->amps.auxp, *freqs = (MYFLT *) p->freqs.auxp;
    MYFLT  *phases = (MYFLT *) p->phases.auxp, *outsum = (MYFLT *) p->sum.auxp;
    int    *trackID = (int *) p->trackID.auxp;
    int     hopsize = p->hopsize;

    ratio = size / csound->esr;
    factor = p->factor;

    maxtracks = p->numbins > maxtracks ? maxtracks : p->numbins;

    for (n = 0; n < ksmps; n++) {
      out[n] = outsum[pos];
      pos++;
      if (pos == hopsize) {
        memset(outsum, 0, sizeof(MYFLT) * hopsize);
        /* for each track */
        i = j = k = 0;
        while (i < maxtracks * 4) {
          ampnext = (MYFLT) fin[i] * scale;
          freqnext = (MYFLT) fin[i + 1] * pitch;
          if ((id = (int) fin[i + 3]) != -1) {
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
                ampnext = 0.f;
              }
            }
            else {
              /* new track */
              contin = 1;
              freq = freqnext;
              phase = -freq * factor;
              amp = 0.f;

            }
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
              ndx = (int) phase;
              frac = phase - ndx;
              outsum[m] += a * (tab[ndx] + (tab[ndx + 1] - tab[ndx]) * frac);
              a += incra;
              f += incrph;
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

static int psynth2_init(CSOUND * csound, _PSYN2 * p)
{
    int     numbins = p->fin->N / 2 + 1;

    if (p->fin->format != PVS_TRACKS) {
      csound->InitError(csound, "psynth: first input not in TRACKS format \n");
      return NOTOK;
    }
    p->func = csound->FTnp2Find(p->h.insdshead->csound, p->ftb);
    if (p->func == NULL) {
      csound->InitError(csound, "psynth: function table not found\n");
      return NOTOK;
    }

    p->tracks = 0;
    p->hopsize = p->fin->overlap;
    p->pos = 0;
    p->numbins = numbins;
    p->invsr = (MYFLT) (1. / csound->esr);
    p->factor = p->hopsize / csound->esr;
    p->facsqr = p->factor * p->factor;
    p->pi = 4. * atan(1.);
    p->twopi = 2. * p->pi;

    if (p->amps.auxp == NULL ||
        (unsigned)p->amps.size < sizeof(MYFLT) * numbins)
      csound->AuxAlloc(csound, sizeof(MYFLT) * numbins, &p->amps);
    if (p->freqs.auxp == NULL ||
        (unsigned)p->freqs.size < sizeof(MYFLT) * numbins)
      csound->AuxAlloc(csound, sizeof(MYFLT) * numbins, &p->freqs);
    if (p->phases.auxp == NULL ||
        (unsigned)p->phases.size < sizeof(MYFLT) * numbins)
      csound->AuxAlloc(csound, sizeof(MYFLT) * numbins, &p->phases);
    if (p->sum.auxp == NULL ||
        (unsigned)p->sum.size < sizeof(MYFLT) * p->hopsize)
      csound->AuxAlloc(csound, sizeof(MYFLT) * p->hopsize, &p->sum);
    if (p->trackID.auxp == NULL ||
        (unsigned)p->trackID.size < sizeof(int) * numbins)
      csound->AuxAlloc(csound, sizeof(int) * numbins, &p->trackID);

    return OK;
}

static int psynth2_process(CSOUND * csound, _PSYN2 * p)
{

    MYFLT   ampnext, amp, freq, freqnext, phase, phasenext, ratio;
    double  a2, a3, cph, pi, twopi;
    MYFLT   phasediff, facsqr, ph;
    MYFLT   a, frac, incra, incrph, factor, lotwopi, cnt;
    MYFLT   scale = *p->scal;
    int     ndx, size = p->func->flen;
    int     i, j, k, n, m, id;
    int     notcontin = 0;
    int     contin = 0;
    int     tracks = p->tracks, maxtracks = *p->maxtracks;
    MYFLT  *tab = p->func->ftable, *out = p->out;
    float  *fin = (float *) p->fin->frame.auxp;
    int     ksmps = csound->ksmps, pos = p->pos;
    MYFLT  *amps = (MYFLT *) p->amps.auxp, *freqs = (MYFLT *) p->freqs.auxp;
    MYFLT  *phases = (MYFLT *) p->phases.auxp, *outsum = (MYFLT *) p->sum.auxp;
    int    *trackID = (int *) p->trackID.auxp;
    int     hopsize = p->hopsize;

    incrph = p->invsr;
    pi = p->pi;
    twopi = p->twopi;
    lotwopi = (MYFLT) (size / twopi);
    ratio = size / csound->esr;
    factor = p->factor;
    facsqr = p->facsqr;
    maxtracks = p->numbins > maxtracks ? maxtracks : p->numbins;

    for (n = 0; n < ksmps; n++) {
      out[n] = outsum[pos];
      pos++;
      if (pos == hopsize) {
        memset(outsum, 0, sizeof(MYFLT) * hopsize);
        /* for each track */
        i = j = k = 0;
        while (i < maxtracks * 4) {
          ampnext = (MYFLT) fin[i] * scale;
          freqnext = (MYFLT) fin[i + 1] * twopi;
          phasenext = (MYFLT) fin[i + 2];
          if ((id = (int) fin[i + 3]) != -1) {
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
                ampnext = 0.f;
              }
            }
            else {
              /* new track */
              contin = 1;
              freq = freqnext;
              phase = phasenext - freq * factor;
              amp = 0.f;
            }
            /* phasediff */
            phasediff = phasenext - phase;

            while (phasediff >= pi)
              phasediff -= twopi;
            while (phasediff < -pi)
              phasediff += twopi;

            /* update phasediff to match the freq */
            cph = ((freq + freqnext) * factor / 2. - phasediff) / twopi;
            phasediff += twopi * (int) (cph + 0.5);
            /* interpolation coefs */
            a2 = 3. / facsqr * (phasediff -
                                factor / 3. * (2. * freq + freqnext));
            a3 = 1. / (3 * facsqr) * (freqnext - freq - 2. * a2 * factor);
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
              ndx = (int) ph;
              frac = ph - ndx;
              outsum[m] += a * (tab[ndx] + (tab[ndx + 1] - tab[ndx]) * frac);
              a += incra;
              cnt += incrph;
              ph = phase + cnt * (freq + cnt * (a2 + a3 * cnt));
            }
            /* keep amp, freq, and phase values for next time */
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

static int psynth3_process(CSOUND * csound, _PSYN *p)
{

    MYFLT   ampnext, amp, freq, freqnext, phase, phasenext, ratio;
    double  a2, a3, cph, pi, twopi;
    MYFLT   phasediff, facsqr, ph;
    MYFLT   a, frac, incra, incrph, factor, lotwopi, cnt;
    MYFLT   scale = *p->scal, pitch = *p->pitch;
    int     ndx, size = p->func->flen;
    int     i, j, k, n, m, id;
    int     notcontin = 0;
    int     contin = 0;
    int     tracks = p->tracks, maxtracks = *p->maxtracks;
    MYFLT  *tab = p->func->ftable, *out = p->out;
    float  *fin = (float *) p->fin->frame.auxp;
    int     ksmps = csound->ksmps, pos = p->pos;
    MYFLT  *amps = (MYFLT *) p->amps.auxp, *freqs = (MYFLT *) p->freqs.auxp;
    MYFLT  *phases = (MYFLT *) p->phases.auxp, *outsum = (MYFLT *) p->sum.auxp;
    int    *trackID = (int *) p->trackID.auxp;
    int     hopsize = p->hopsize;

    incrph = p->invsr;
    pi = p->pi;
    twopi = p->twopi;
    lotwopi = (MYFLT) (size / twopi);
    ratio = size / csound->esr;
    factor = p->factor;
    facsqr = p->facsqr;
    maxtracks = p->numbins > maxtracks ? maxtracks : p->numbins;

    for (n = 0; n < ksmps; n++) {
      out[n] = outsum[pos];
      pos++;
      if (pos == hopsize) {
        memset(outsum, 0, sizeof(MYFLT) * hopsize);
        /* for each track */
        i = j = k = 0;
        while (i < maxtracks * 4) {
          ampnext = (MYFLT) fin[i] * scale;
          freqnext = (MYFLT) fin[i + 1] * twopi * pitch;
          phasenext = (MYFLT) fin[i + 2];
          if ((id = (int) fin[i + 3]) != -1) {
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
                ampnext = 0.f;
              }
            }
            else {
              /* new track */
              contin = 1;
              freq = freqnext;
              phase = phasenext - freq * factor;
              amp = 0.f;
            }
            /* phasediff */
            phasediff = phasenext - phase;

            while (phasediff >= pi)
              phasediff -= twopi;
            while (phasediff < -pi)
              phasediff += twopi;

            /* update phasediff to match the freq */
            cph = ((freq + freqnext) * factor / 2. - phasediff) / twopi;
            phasediff += twopi * cph;
            /* interpolation coefs */
            a2 = 3. / facsqr * (phasediff -
                                factor / 3. * (2. * freq + freqnext));
            a3 = 1. / (3 * facsqr) * (freqnext - freq - 2. * a2 * factor);
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
              ndx = (int) ph;
              frac = ph - ndx;
              outsum[m] += a * (tab[ndx] + (tab[ndx + 1] - tab[ndx]) * frac);
              a += incra;
              cnt += incrph;
              ph = phase + cnt * (freq + cnt * (a2 + a3 * cnt));
            }
            /* keep amp, freq, and phase values for next time */
            if (contin) {
              amps[k] = ampnext;
              freqs[k] = freqnext;
              phasenext += (cph - (int) cph) * twopi;
              while (phasenext < 0)
                phasenext += twopi;
              while (phasenext >= twopi)
                phasenext -= twopi;
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

static OENTRY localops[] = {
    {"tradsyn", sizeof(_PSYN), 5, "a", "fkkki", (SUBR) psynth_init, NULL,
     (SUBR) psynth_process}
    ,
    {"sinsyn", sizeof(_PSYN2), 5, "a", "fkki", (SUBR) psynth2_init, NULL,
     (SUBR) psynth2_process}
    ,
    {"resyn", sizeof(_PSYN), 5, "a", "fkkki", (SUBR) psynth_init, NULL,
     (SUBR) psynth3_process}
};

LINKAGE

