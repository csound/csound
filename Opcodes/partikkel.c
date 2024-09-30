/*
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
Partikkel - a granular synthesis module for Csound 5
Copyright (C) 2006-2016 Oeyvind Brandtsegg, Torgeir Strand Henriksen,
Thom Johansen

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "partikkel.h"
#include <limits.h>
#include <math.h>

#define INITERROR(x) csound->InitError(csound, "%s", Str("partikkel: " x))
#define PERFERROR(x) csound->PerfError(csound, &(p->h), "%s", Str("partikkel: " x))
#define WARNING(x) csound->Warning(csound, "%s", Str("partikkel: " x))

/* Assume csound and p pointers are always available */
#define frand() (csound->RandMT(&p->randstate)/(double)(0xffffffff))
/* linear interpolation between x and y by z
 * NOTE: arguments evaluated more than once, do not pass anything with side
 * effects
 */
#define lrp(x, y, z) ((x) + ((y) - (x))*(z))

/* macro used to wrap an index back to start position if it's out of bounds. */
#define clip_index(index, from, to) \
    if (index > (uint32_t)(to) || index < (uint32_t)(from)) \
        index = (uint32_t)(from);

/* here follows routines for maintaining a linked list of grains */

/* initialises a linked list of NODEs */
static void init_pool(GRAINPOOL *s, uint32_t max_grains)
{
    uint32_t i;
    NODE **p = &s->grainlist;
    NODE *grainpool = (NODE *)s->mempool;

    s->free_nodes = max_grains;
    /* build list of grains in pool */
    for (i = 0; i < max_grains; ++i) {
        NODE *node;
        *p = grainpool + i;
        node = *p;
        node->next = NULL;
        p = &(node->next);
    }
}

/* returns pointer to new node */
static NODE *get_grain(GRAINPOOL *s)
{
    NODE *ret = s->grainlist;

    if (s->grainlist)
        s->grainlist = s->grainlist->next;
    s->free_nodes--;
    return ret;
}

/* returns a NODE to the pool. function returns pointer to next node */
static NODE *return_grain(GRAINPOOL *s, NODE *c)
{
    NODE *oldnext = c->next;

    c->next = s->grainlist;
    s->grainlist = c;
    s->free_nodes++;
    return oldnext;
}

/* return oldest grain to the pool, we use this when we're out of grains */
static void kill_oldest_grain(GRAINPOOL *s, NODE *n)
{
    while (n->next->next)
        n = n->next;
    return_grain(s, n->next);
    n->next = NULL;
}

static int32_t setup_globals(CSOUND *csound, PARTIKKEL *p)
{
    PARTIKKEL_GLOBALS *pg;
    PARTIKKEL_GLOBALS_ENTRY **pe;

    pg = csound->QueryGlobalVariable(csound, "partikkel");
    if (pg == NULL) {
      int32_t i;

      if (UNLIKELY(csound->CreateGlobalVariable(csound, "partikkel",
                                                sizeof(PARTIKKEL_GLOBALS)) != 0))
        return INITERROR("could not allocate globals");
      pg = csound->QueryGlobalVariable(csound, "partikkel");
      pg->rootentry = NULL;
      /* build default tables. allocate enough for three, plus extra for the
       * ftable data itself */
      /* we only fill in the entries in the FUNC struct that we use */
      /* table with data [1.0, 1.0, 1.0], used as default by envelopes */
      pg->ooo_tab = (FUNC *)csound->Calloc(csound, sizeof(FUNC));
      pg->ooo_tab->ftable = (MYFLT*)csound->Calloc(csound, 3*sizeof(MYFLT));
      pg->ooo_tab->flen = 2;
      pg->ooo_tab->lobits = 31;
      for (i = 0; i <= 2; ++i)
        pg->ooo_tab->ftable[i] = FL(1.0);
      /* table with data [0.0, 0.0, 0.0], used as default by grain
       * distribution table, channel masks and grain waveforms */
      pg->zzz_tab = (FUNC *)csound->Calloc(csound, sizeof(FUNC));
      pg->zzz_tab->ftable = (MYFLT*)csound->Calloc(csound, 3*sizeof(MYFLT));
      pg->zzz_tab->flen = 2;
      pg->zzz_tab->lobits = 31;
      /* table with data [0.0, 0.0, 1.0], used as default by gain masks,
       * fm index table, and wave start and end freq tables */
      pg->zzo_tab = (FUNC *)csound->Calloc(csound, sizeof(FUNC));
      pg->zzo_tab->ftable = (MYFLT*)csound->Calloc(csound, 4*sizeof(MYFLT));
      pg->zzo_tab->ftable[2] = FL(1.0);
      pg->zzo_tab->flen = 3;  /* JPff */
      /* table with data [0.0, 0.0, 0.5, 0.5, 0.5, 0.5, 0.0], used as default
       * by wave gain table */
      pg->zzhhhhz_tab = (FUNC *)csound->Calloc(csound, sizeof(FUNC));
      pg->zzhhhhz_tab->ftable = (MYFLT*)csound->Calloc(csound, 8*sizeof(MYFLT));
      for (i = 2; i <= 5; ++i)
        pg->zzhhhhz_tab->ftable[i] = FL(0.5);
    }
    p->globals = pg;
    if ((int32_t)*p->opcodeid == 0) {
      /* opcodeid 0 means we do not bother with the sync opcode */
      p->globals_entry = NULL;
      return OK;
    }
    /* try to find entry corresponding to our opcodeid */
    pe = &pg->rootentry;
    while (*pe != NULL && (*pe)->id != *p->opcodeid)
      pe = &((*pe)->next);

    /* check if one already existed, if not, create one */
    if (*pe == NULL) {
      *pe = csound->Malloc(csound, sizeof(PARTIKKEL_GLOBALS_ENTRY));
      (*pe)->id = *p->opcodeid;
      (*pe)->partikkel = p;
      /* allocate table for sync data */
      (*pe)->synctab = csound->Calloc(csound, 2*CS_KSMPS*sizeof(MYFLT));
      (*pe)->next = NULL;
    }
    p->globals_entry = *pe;
    return OK;
}

/* look up a sample from a csound table using linear interpolation
 * tab: csound table pointer
 * index: fixed point index in the range 0..PHMASK inclusive
 * zscale: 1/(1 << tab->lobits)
 * shift: length of phase register in bits minus length of table in bits
 */
static inline MYFLT lrplookup(FUNC *tab, uint32_t phase, MYFLT zscale,
                              uint32_t shift)
{
    const uint32_t index = phase >> shift;
    const uint32_t mask = (1 << shift) - 1;

    MYFLT a = tab->ftable[index];
    MYFLT b = tab->ftable[index + 1];
    MYFLT z = (MYFLT)(phase & mask)*zscale;
    return lrp(a, b, z);
}

/* floating-point phase version */
static inline MYFLT lrplookup_f(FUNC *tab, double phase)
{
    MYFLT    pos = PHMOD1(phase)*tab->flen;
    uint32_t index = (int32_t) phase;
    MYFLT a = tab->ftable[index];
    MYFLT b = tab->ftable[index + 1];
    return lrp(a, b, (pos - index));
}


/* dsf synthesis for trainlets */
static inline MYFLT dsf(FUNC *tab, GRAIN *grain, double beta, MYFLT zscale,
                        uint32_t cosineshift)
{
    MYFLT numerator, denominator, cos_beta;
    MYFLT lastharmonic, result;
    uint32_t fbeta, N = grain->harmonics;
    const MYFLT a = grain->falloff;
    const MYFLT a_pow_N = grain->falloff_pow_N;
    int32_t floatph = !(IS_POW_TWO(tab->flen));
    
    fbeta = (uint32_t)(beta*(double)UINT_MAX);

    if(!floatph)
    cos_beta = lrplookup(tab, fbeta, zscale, cosineshift);
    else
     cos_beta = lrplookup_f(tab, beta);   
    denominator = FL(1.0) - FL(2.0)*a*cos_beta + a*a;
    if (denominator < FL(1e-6) && denominator > FL(-1e-6)) {
        /* handle this special case to avoid divison by zero */
        result = N - FL(1.0);
    } else {
        /* this factor can also serve as a last, fadable harmonic, if we in the
         * future want to fade the number of harmonics smoothly */
      if(floatph) {
         lastharmonic = a_pow_N*lrplookup_f(tab, beta*N);
         numerator = FL(1.0) - a*cos_beta - lastharmonic
           + a*a_pow_N*lrplookup_f(tab, (N - 1)*beta);
      } else {
        lastharmonic = a_pow_N*lrplookup(tab, fbeta*N, zscale, cosineshift);
        numerator = FL(1.0) - a*cos_beta - lastharmonic
            + a*a_pow_N*lrplookup(tab, (N - 1)*fbeta, zscale, cosineshift);
      }
        result = numerator/denominator - FL(1.0);
    }
    return result;
}

static int32_t partikkel_init(CSOUND *csound, PARTIKKEL *p)
{
    uint32_t size;
    int32_t ret;

    if ((ret = setup_globals(csound, p)) != OK)
        return ret;
    p->floatph = 0;
    p->grainroot = NULL;
    /* set grainphase to 1.0 to make grain scheduler create a grain immediately
     * after starting opcode */
    p->grainphase = 1.0;
    p->num_outputs = GetOutputArgCnt((OPDS *)p); /* save for faster access */
    /* resolve tables with no default table handling */
    p->costab = csound->FTFind(csound, p->cosine);
    /* resolve some tables with default table handling */
    p->disttab = *p->dist >= FL(0.0)
                 ? csound->FTFind(csound, p->dist)
                 : p->globals->zzz_tab;
    p->gainmasktab = *p->gainmasks >= FL(0.0)
                     ? csound->FTFind(csound, p->gainmasks)
                     : p->globals->zzo_tab;
    p->channelmasktab = *p->channelmasks >= FL(0.0)
                        ? csound->FTFind(csound, p->channelmasks)
                        : p->globals->zzz_tab;
    p->env_attack_tab = *p->env_attack >= FL(0.0)
                        ? csound->FTFind(csound, p->env_attack)
                        : p->globals->ooo_tab;
    p->floatph |= !(IS_POW_TWO(p->env_attack_tab->flen));
    p->env_decay_tab = *p->env_decay >= FL(0.0)
                       ? csound->FTFind(csound, p->env_decay)
                       : p->globals->ooo_tab;
    p->floatph |= !(IS_POW_TWO(p->env_decay_tab->flen));
    p->env2_tab = *p->env2 >= FL(0.0)
                   ? csound->FTFind(csound, p->env2)
                   : p->globals->ooo_tab;
    p->floatph |= !(IS_POW_TWO(p->env2_tab->flen));
    p->wavfreqstarttab = *p->wavfreq_startmuls >= FL(0.0)
                         ? csound->FTFind(csound, p->wavfreq_startmuls)
                         : p->globals->zzo_tab;
    p->wavfreqendtab = *p->wavfreq_endmuls >= FL(0.0)
                       ? csound->FTFind(csound, p->wavfreq_endmuls)
                       : p->globals->zzo_tab;
    p->fmamptab = *p->fm_indices >= FL(0.0)
                  ? csound->FTFind(csound, p->fm_indices)
                  : p->globals->zzo_tab;
    p->wavgaintab = *p->waveamps >= FL(0.0)
                    ? csound->FTFind(csound, p->waveamps)
                    : p->globals->zzhhhhz_tab;
    if (*p->pantable >= FL(0.0)) {
        p->pantab = csound->FTFind(csound, p->pantable);
        if (!p->pantab)
            return INITERROR("unable to load panning function table");
    } else {
        p->pantab = NULL; /* use default linear panning function */
    }

    if (UNLIKELY(!p->disttab))
        return INITERROR("unable to load distribution table");
    if (UNLIKELY(!p->costab))
        return INITERROR("unable to load cosine table");
    if (UNLIKELY(!p->gainmasktab))
        return INITERROR("unable to load gain mask table");
    if (UNLIKELY(!p->channelmasktab))
        return INITERROR("unable to load channel mask table");
    if (UNLIKELY(!p->env_attack_tab || !p->env_decay_tab || !p->env2_tab))
        return INITERROR("unable to load envelope table");
    if (UNLIKELY(!p->wavfreqstarttab))
        return INITERROR("unable to load start frequency scaler table");
    if (UNLIKELY(!p->wavfreqendtab))
        return INITERROR("unable to load end frequency scaler table");
    if (UNLIKELY(!p->fmamptab))
        return INITERROR("unable to load FM index table");
    if (UNLIKELY(!p->wavgaintab))
        return INITERROR("unable to load wave gain table");

    p->disttabshift = sizeof(uint32_t)*CHAR_BIT -
                      (uint32_t)(log((double)p->disttab->flen)/log(2.0) + 0.5);
    p->cosineshift = sizeof(uint32_t)*CHAR_BIT -
                     (uint32_t)(log((double)p->costab->flen)/log(2.0) + 0.5);
    p->zscale = FL(1.0)/FL(1 << p->cosineshift);
    p->wavfreqstartindex = p->wavfreqendindex = 0;
    p->gainmaskindex = p->channelmaskindex = 0;
    p->wavgainindex = 0;
    p->fmampindex = 0;
    p->distindex = 0;
    p->synced = 0;
    p->graininc = 0.0;

    /* allocate memory for the grain mix buffer */
    size = CS_KSMPS*sizeof(MYFLT);
    if (p->aux.auxp == NULL || p->aux.size < size)
        csound->AuxAlloc(csound, size, &p->aux);
    else
      memset(p->aux.auxp, 0, size);

    /* allocate memory for the grain pool and initialize it*/
    if (UNLIKELY(*p->max_grains < FL(1.0)))
        return INITERROR("maximum number of grains needs to be non-zero "
                         "and positive");
    size = ((uint32_t)*p->max_grains)*sizeof(NODE);
    if (p->aux2.auxp == NULL || p->aux2.size < size)
        csound->AuxAlloc(csound, size, &p->aux2);
    p->gpool.mempool = p->aux2.auxp;
    init_pool(&p->gpool, (uint32_t)*p->max_grains);

    /* find out which of the xrate parameters are arate */
    p->grainfreq_arate = IS_ASIG_ARG(p->grainfreq) ? 1 : 0;
    p->out_of_voices_warning = 0; /* reset user warning indicator */
    csound->SeedRandMT(&p->randstate, NULL, csound->GetRandomSeedFromTime());
    return OK;
}

/* n is sample number for which the grain is to be scheduled
 * offset is time offset for grain in seconds, passed separately for hints */
static int32_t schedule_grain(CSOUND *csound, PARTIKKEL *p, NODE *node, int32 n,
                          double offset)
{
    /* make a new grain */
    MYFLT startfreqscale, endfreqscale;
    MYFLT maskgain, maskchannel;
    GRAIN *grain = &node->grain;
    uint32_t i;
    uint32_t chan;
    MYFLT graingain;
    MYFLT *gainmasks = p->gainmasktab->ftable;
    MYFLT *chanmasks = p->channelmasktab->ftable;
    MYFLT *freqstarts = p->wavfreqstarttab->ftable;
    MYFLT *freqends = p->wavfreqendtab->ftable;
    MYFLT *fmamps = p->fmamptab->ftable;
    MYFLT *wavgains = p->wavgaintab->ftable;
    uint32_t wavgainsindex;

    /* the table boundary limits might well change at any time, so we do the
     * boundary clipping before using it to fetch a value */

    /* get gain mask */
    clip_index(p->gainmaskindex, gainmasks[0], gainmasks[1]);
    maskgain = gainmasks[p->gainmaskindex + 2];
    p->gainmaskindex++;

    /* get channel mask */
    clip_index(p->channelmaskindex, chanmasks[0], chanmasks[1]);
    maskchannel = chanmasks[p->channelmaskindex + 2];
    p->channelmaskindex++;

    /* get frequency sweep start scaler */
    clip_index(p->wavfreqstartindex, freqstarts[0], freqstarts[1]);
    startfreqscale = freqstarts[p->wavfreqstartindex + 2];
    p->wavfreqstartindex++;

    /* get frequency sweep end scaler */
    clip_index(p->wavfreqendindex, freqends[0], freqends[1]);
    endfreqscale = freqends[p->wavfreqendindex + 2];
    p->wavfreqendindex++;

    /* get fm modulation index */
    clip_index(p->fmampindex, fmamps[0], fmamps[1]);
    grain->fmamp = fmamps[p->fmampindex + 2];
    p->fmampindex++;

    /* calculate waveform gain table index for later use */
    clip_index(p->wavgainindex, wavgains[0], wavgains[1]);
    wavgainsindex = 5*p->wavgainindex++;

    graingain = *p->amplitude*maskgain;
    /* check if our mask gain is zero or if stochastic masking takes place */
    if ((fabs(graingain) < FL(1e-8)) || (frand() > 1.0 - *p->randommask)) {
        /* grain is either masked out or has a zero amplitude, so we cancel it
         * and proceed with scheduling our next grain */
        return_grain(&p->gpool, node);
        return OK;
    }

    grain->env2amount = *p->env2_amount;
    grain->envattacklen = (1.0 - *p->sustain_amount)*(*p->a_d_ratio);
    grain->envdecaystart = grain->envattacklen + *p->sustain_amount;
    grain->fmenvtab = p->fmenvtab;

    /* place a grain in between two channels according to channel mask value */
    chan = (uint32_t)maskchannel;
    if (UNLIKELY(chan >= p->num_outputs)) {
        return_grain(&p->gpool, node);
        return PERFERROR("channel mask specifies non-existing output channel");
    }
    /* use panning law table if specified */
    if (p->pantab != NULL) {
        const uint32_t tabsize = p->pantab->flen/8;
        /* offset of pan table for current output pair */
        const uint32_t tab_offset = chan*tabsize;
        const uint32_t offset = (uint32_t)((maskchannel - chan)*(tabsize - 1));
        const uint32_t flip_offset = tabsize - 1 - offset;

        grain->gain1 = p->pantab->ftable[tab_offset + flip_offset];
        grain->gain2 = p->pantab->ftable[tab_offset + offset];
    } else {
        grain->gain1 = FL(1.0) - (maskchannel - chan);
        grain->gain2 = maskchannel - chan;
    }

    grain->chan1 = chan;
    grain->chan2 = p->num_outputs > chan + 1 ? chan + 1 : 0;

    /* duration in samples */
    const double dur_samples = CS_ESR*(*p->duration)/1000.0;
    /* if grainlength is below one sample, we'll just cancel it */
    if (dur_samples < 1.0) {
        return_grain(&p->gpool, node);
        return OK;
    }
    /* the grain is supposed to start at grainphase = 0, so calculate how far
     * we overshot that and correct all relevant wave and envelope phases
     * for proper sub-sample grain placement. if offset != 0, our grains
     * are probably not very synchronous, and will not benefit from this.
     * also only enable it for sufficiently high grain rates. current
     * threshold corresponds to around 150hz */
    const double phase_corr = offset == 0.0 && p->graininc > 0.0032
                            ? p->grainphase/p->graininc
                            : 0.0;
    const double rcp_samples = 1.0/dur_samples;
    grain->start = (uint32_t)((double)n + offset*CS_ESR + phase_corr);
    grain->stop = (uint32_t)(grain->start + dur_samples - phase_corr) + 1;
    /* set up the four wavetables and dsf to use in the grain */
    for (i = 0; i < 5; ++i) {
        WAVEDATA *curwav = &grain->wav[i];
        MYFLT freqmult = i != WAV_TRAINLET
                         ? *(*(&p->wavekey1 + i))*(*p->wavfreq)
                         : *p->trainletfreq;
        MYFLT startfreq = freqmult*startfreqscale;
        MYFLT endfreq = freqmult*endfreqscale;
        MYFLT *samplepos = *(&p->samplepos1 + i);
        MYFLT enddelta;

        curwav->table = i != WAV_TRAINLET ? p->wavetabs[i] : p->costab;
        curwav->gain = wavgains[wavgainsindex + i + 2]*graingain;

        /* drop wavetables with close to zero gain */
        if (fabs(curwav->gain) < FL(1e-8)) {
            curwav->table = NULL;
            continue;
        }

        /* now do some trainlet specific setup */
        if (i == WAV_TRAINLET) {
            double normalize, nh;
            MYFLT maxfreq = startfreq > endfreq ? startfreq : endfreq;

            /* limit dsf harmonics to nyquist to avoid aliasing.
             * minumum number of harmonics is 2, since 1 would yield just dc,
             * which we remove anyway */
            nh = 0.5*CS_ESR/fabs(maxfreq);
            if (nh > fabs(*p->harmonics))
                nh = fabs(*p->harmonics);
            grain->harmonics = (uint32_t)nh + 1;
            if (grain->harmonics < 2)
                grain->harmonics = 2;
            grain->falloff = *p->falloff;
            grain->falloff_pow_N = intpow(grain->falloff, grain->harmonics);
            /* normalize trainlets to uniform peak, using geometric sum */
            if (FABS(grain->falloff) > FL(0.9999) &&
                FABS(grain->falloff) < FL(1.0001))
                /* limit case for falloff = 1 */
                normalize = 1.0/(double)grain->harmonics;
            else
                normalize = (1.0 - fabs(grain->falloff))
                            /(1.0 - fabs(grain->falloff_pow_N));
            curwav->gain *= normalize;
        }

        curwav->delta = startfreq*CS_ONEDSR;
        enddelta = endfreq*CS_ONEDSR;

        if (i != WAV_TRAINLET) {
            /* set wavphase to samplepos parameter */
            curwav->phase = samplepos[n];
        } else {
            /* set to 0.5 so the dsf pulse doesn't occur at the very start of
             * the grain where it'll probably be enveloped away anyway */
            curwav->phase = 0.5;
        }
        /* place grain between samples. this is especially important to make
         * high frequency synchronous grain streams sounds right */
        curwav->phase += phase_corr*startfreq*CS_ONEDSR;

        /* clamp phase in case it's out of bounds */
        curwav->phase = curwav->phase > 1.0 ? 1.0 : curwav->phase;
        curwav->phase = curwav->phase < 0.0 ? 0.0 : curwav->phase;
        /* phase and delta for wavetable synthesis are scaled by table length */
        if (i != WAV_TRAINLET) {
            double tablen = (double)curwav->table->flen;

            curwav->phase *= tablen;
            curwav->delta *= tablen;
            enddelta *= tablen;
        }

        /* the sweep curve generator is a first order iir filter */
        if (curwav->delta == enddelta || *p->freqsweepshape == FL(0.5)) {
            /* special case for linear sweep */
            curwav->sweepdecay = 1.0;
            curwav->sweepoffset = (enddelta - curwav->delta)*rcp_samples;
        } else {
            /* handle extreme cases the generic code doesn't handle too well */
            if (*p->freqsweepshape < FL(0.001)) {
                curwav->sweepdecay = 1.0;
                curwav->sweepoffset = 0.0;
            } else if (*p->freqsweepshape > FL(0.999)) {
                curwav->sweepdecay = 0.0;
                curwav->sweepoffset = enddelta;
            } else {
                double start_offset, total_decay, t;

                t = fabs((*p->freqsweepshape - 1.0)/(*p->freqsweepshape));
                curwav->sweepdecay = pow(t, 2.0*rcp_samples);
                total_decay = t*t; /* pow(curwav->sweepdecay, samples) */
                start_offset = (enddelta - curwav->delta*total_decay)/
                               (1.0 - total_decay);
                curwav->sweepoffset = start_offset*(1.0 - curwav->sweepdecay);
            }
        }
    }

    grain->envinc = rcp_samples;
    grain->envphase = phase_corr*grain->envinc;
    /* link new grain into the list */
    node->next = p->grainroot;
    p->grainroot = node;
    return OK;
}

/* this function schedules the grains that are bound to happen this k-period */
static int32_t schedule_grains(CSOUND *csound, PARTIKKEL *p)
{
    uint32_t koffset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    NODE *node;
    MYFLT **waveformparams = &p->waveform1;
    MYFLT grainfreq = fabs(*p->grainfreq);

    /* krate table lookup, first look up waveform ftables */
    for (n = 0; n < 4; ++n) {
        p->wavetabs[n] = *waveformparams[n] >= FL(0.0)
                         ? csound->FTFind(csound, waveformparams[n])
                         : p->globals->zzz_tab;
        if (UNLIKELY(p->wavetabs[n] == NULL))
            return PERFERROR("unable to load waveform table");
    }
    /* look up fm envelope table for use in grains scheduled this kperiod */
    p->fmenvtab = *p->fm_env >= FL(0.0)
                  ? csound->FTFind(csound, p->fm_env)
                  : p->globals->ooo_tab;
    if (UNLIKELY(!p->fmenvtab))
        return PERFERROR("unable to load FM envelope table");
    p->floatph = !(IS_POW_TWO(p->fmenvtab->flen));
    

    if (UNLIKELY(early)) nsmps -= early;
    /* start grain scheduling */
    for (n = koffset; n < nsmps; ++n) {
        if (p->sync[n] >= FL(1.0)) {
            /* we got a full sync pulse, hardsync grain clock if needed */
            if (!p->synced) {
                p->grainphase = 1.0;
                p->synced = 1;
            } else {
                /* if sync is held high, stop the grain clock until it goes
                 * back to zero or below again */
                p->graininc = 0.0;
            }
        } else {
            /* softsync-like functionality where we advance the grain clock by
             * the amount given by the sync value */
            if (p->sync[n]) {
                p->grainphase += p->sync[n];
                p->grainphase = p->grainphase > 1.0 ? 1.0 : p->grainphase;
                p->grainphase = p->grainphase < 0.0 ? 0.0 : p->grainphase;
            }
            p->synced = 0;
        }

        if (p->grainphase >= 1.0) {
            int32_t floatph = !(IS_POW_TWO(p->disttab->flen)),
            flen = p->disttab->flen;
            double offset;
            do
                p->grainphase -= 1.0;
            while (UNLIKELY(p->grainphase >= 1.0));
            /* schedule new synchronous or synced grain */
            /* first determine time offset for grain */
            if (*p->distribution >= FL(0.0)) {
                /* positive distrib, choose random point in table */
                uint32_t rnd = csound->RandMT(&p->randstate);
                if(floatph) offset = p->disttab->ftable[(int32_t)((float)flen*rnd/0xffffffff)]; 
                else offset = p->disttab->ftable[rnd >> p->disttabshift];
                offset *= *p->distribution;
            } else {
                /* negative distrib, choose sequential point in table */
                offset = p->disttab->ftable[p->distindex++];
                offset *= -*p->distribution;
                if ((uint32_t)p->distindex >= p->disttab->flen)
                    p->distindex = 0;
            }
            /* convert offset to seconds, also limiting it to 10 seconds to
             * avoid accidentally filling grain pool with grains which will
             * spawn in half a day */
            if (grainfreq < FL(0.001)) {
                /* avoid div by zero */
                offset = 0;
            } else {
                offset /= grainfreq;
                if (offset > 10.0) offset = 10.0;
            }
            /* check if there are any grains left in the pool */
            if (!p->gpool.free_nodes) {
                if (!p->out_of_voices_warning) {
                    WARNING("maximum number of grains reached");
                    p->out_of_voices_warning = 1; /* we only warn once */
                }
                kill_oldest_grain(&p->gpool, p->grainroot);
            }
            /* add a new grain */
            node = get_grain(&p->gpool);
            /* check first, in case we'll change the above behaviour of
             * killing a grain */
            if (node) {
                int32_t ret = schedule_grain(csound, p, node, n, offset);

                if (ret != OK)
                    return ret;
            }
            /* create a sync pulse for use in partikkelsync */
            if (p->globals_entry)
                p->globals_entry->synctab[n] = FL(1.0);
        }

        /* store away the scheduler phase for use in partikkelsync */
        if (p->globals_entry)
            p->globals_entry->synctab[CS_KSMPS + n] = p->grainphase;

        if (p->grainfreq_arate)
            grainfreq = fabs(p->grainfreq[n]);
        p->graininc = grainfreq*CS_ONEDSR;
        p->grainphase += p->graininc;
    }
    return OK;
}

/* Main synthesis loops */
/* NOTE: the main synthesis loop is duplicated for both wavetable and
 * trainlet synthesis for speed */
static inline void render_wave(PARTIKKEL *p, GRAIN *grain, WAVEDATA *wav,
                               MYFLT *buf, uint32_t stop)
{
    uint32_t n;
    double fmenvphase = grain->envphase;
    int32_t flen = p->fmenvtab->flen;
    int32_t floatph = p->floatph;

    /* wavetable synthesis */
    for (n = grain->start; n < stop; ++n) {
        double tablen = (double)wav->table->flen;
        uint32_t x0;
        MYFLT frac, fmenv;

        /* make sure phase accumulator stays within bounds */
        while (UNLIKELY(wav->phase >= tablen))
            wav->phase -= tablen;
        while (UNLIKELY(wav->phase < 0.0))
            wav->phase += tablen;

        /* sample table lookup with linear interpolation */
        x0 = (uint32_t)wav->phase;
        frac = (MYFLT)(wav->phase - x0);
        buf[n] += lrp(wav->table->ftable[x0], wav->table->ftable[x0 + 1],
                      frac)*wav->gain;
        if(floatph) 
          fmenv = grain->fmenvtab->ftable[(size_t) (fmenvphase*flen)];
        else 
        fmenv = grain->fmenvtab->ftable[(size_t)(fmenvphase*FMAXLEN)
                                        >> grain->fmenvtab->lobits];
        fmenvphase += grain->envinc;
        wav->phase += wav->delta + wav->delta*p->fm[n]*grain->fmamp*fmenv;
        /* apply sweep */
        wav->delta = wav->delta*wav->sweepdecay + wav->sweepoffset;
     }
}

static inline void render_trainlet(PARTIKKEL *p, GRAIN *grain, WAVEDATA *wav,
                                   MYFLT *buf, uint32_t stop)
{
    uint32_t n;
    double fmenvphase = grain->envphase;
    int32_t flen = p->fmenvtab->flen;
    int32_t floatph = p->floatph;

    /* trainlet synthesis */
    for (n = grain->start; n < stop; ++n) {
        MYFLT fmenv;

        while (UNLIKELY(wav->phase >= 1.0))
            wav->phase -= 1.0;
        while (UNLIKELY(wav->phase < 0.0))
            wav->phase += 1.0;

        /* dsf/trainlet synthesis */
        buf[n] += wav->gain*dsf(p->costab, grain, wav->phase, p->zscale,
                                p->cosineshift);
        if(floatph) 
          fmenv = grain->fmenvtab->ftable[(size_t) (fmenvphase*flen)];
        else 
        fmenv = grain->fmenvtab->ftable[(size_t)(fmenvphase*FMAXLEN)
                                        >> grain->fmenvtab->lobits];
        fmenvphase += grain->envinc;
        wav->phase += wav->delta + wav->delta*p->fm[n]*grain->fmamp*fmenv;
        wav->delta = wav->delta*wav->sweepdecay + wav->sweepoffset;
    }
}

/* do the actual waveform synthesis */
static inline void render_grain(CSOUND *csound, PARTIKKEL *p, GRAIN *grain)
{
    IGN(csound);
    int32_t i;
    uint32_t n;
    MYFLT *out1 = *(&(p->output1) + grain->chan1);
    MYFLT *out2 = *(&(p->output1) + grain->chan2);
    uint32_t stop = grain->stop > CS_KSMPS
                    ? CS_KSMPS : grain->stop;
    MYFLT *buf = (MYFLT *)p->aux.auxp;
    int32_t floatph = p->floatph, flen2 = p->env2_tab->flen;

    if (grain->start >= CS_KSMPS)
        return; /* grain starts at a later kperiod */
    for (i = 0; i < 5; ++i) {
        WAVEDATA *curwav = &grain->wav[i];

        /* check if ftable is to be rendered */
        if (curwav->table == NULL)
            continue;

        if (i != WAV_TRAINLET)
            render_wave(p, grain, curwav, buf, stop);
        else
            render_trainlet(p, grain, curwav, buf, stop);
    }

    /* apply envelopes */
    for (n = grain->start; n < stop; ++n) {
        MYFLT env, env2, output;
        double envphase;
        FUNC *envtable;
        int32_t flen1;

        /* apply envelopes */
        if (grain->envphase < grain->envattacklen) {
            envtable = p->env_attack_tab;
            flen1 = envtable->flen;
            envphase = grain->envphase/grain->envattacklen;
        } else if (grain->envphase < grain->envdecaystart) {
            /* for sustain, use last sample in attack table */
            envtable = p->env_attack_tab;
            flen1 = envtable->flen;
            envphase = 1.0;
        } else if (grain->envphase < 1.0) {
            envtable = p->env_decay_tab;
            flen1 = envtable->flen;
            envphase = (grain->envphase - grain->envdecaystart)/(1.0 -
                       grain->envdecaystart);
        } else {
            /* clamp envelope phase because of round-off errors */
            envtable = grain->envdecaystart < 1.0 ?
                       p->env_decay_tab : p->env_attack_tab;
            flen1 = envtable->flen;
            envphase = grain->envphase = 1.0;
        }

        /* fetch envelope values */
        if(floatph) {
          env = envtable->ftable[(size_t)(envphase*flen1)];
          env2 = p->env2_tab->ftable[(size_t)(grain->envphase*flen2)];
        }else {
        env = envtable->ftable[(size_t)(envphase*FMAXLEN)
                                >> envtable->lobits];
        env2 = p->env2_tab->ftable[(size_t)(grain->envphase*FMAXLEN)
                                   >> p->env2_tab->lobits];
        }
        env2 = FL(1.0) - grain->env2amount + grain->env2amount*env2;

        
        grain->envphase += grain->envinc;
        /* generate grain output sample */
        output = buf[n]*env*env2;
        /* now distribute this grain to the output channels it's supposed to
         * end up in, as decided by the channel mask */
        out1[n] += output*grain->gain1;
        out2[n] += output*grain->gain2;
    }
    /* now clear the area we just worked in */
    memset(buf + grain->start, 0, (stop - grain->start)*sizeof(MYFLT));
}

static int32_t partikkel(CSOUND *csound, PARTIKKEL *p)
{
    int32_t ret;
    uint32_t n;
    NODE **nodeptr;
    MYFLT **outputs = &p->output1;

    if (UNLIKELY(p->aux.auxp == NULL || p->aux2.auxp == NULL))
        return PERFERROR("not initialised");

    if ((ret = schedule_grains(csound, p)) != OK)
        return ret;

    /* clear output buffers, we'll be accumulating our outputs */
    for (n = 0; n < p->num_outputs; ++n)
        memset(outputs[n], 0, sizeof(MYFLT)*CS_KSMPS);

    /* prepare to traverse grain list */
    nodeptr = &p->grainroot;
    while (*nodeptr) {
        GRAIN *grain = &((*nodeptr)->grain);

        /* render current grain to outputs */
        render_grain(csound, p, grain);
        /* check if grain is finished */
        if (grain->stop <= CS_KSMPS) {
            /* grain is finished, deactivate it */
            *nodeptr = return_grain(&p->gpool, *nodeptr);
        } else {
            /* extend grain lifetime with one k-period and find next grain */
            if (CS_KSMPS > grain->start)
                grain->start = 0; /* grain is active */
            else
                grain->start -= CS_KSMPS; /* grain is not yet active */
            grain->stop -= CS_KSMPS;
            nodeptr = &((*nodeptr)->next);
        }
    }
    return OK;
}

/* partikkelsync stuff */
static int32_t partikkelsync_init(CSOUND *csound, PARTIKKEL_SYNC *p)
{
    PARTIKKEL_GLOBALS *pg;
    PARTIKKEL_GLOBALS_ENTRY *pe;

    if (UNLIKELY((int32_t)*p->opcodeid == 0))
        return csound->InitError(csound,
            "%s", Str("partikkelsync: opcode id needs to be a non-zero integer"));
    pg = csound->QueryGlobalVariable(csound, "partikkel");
    if (UNLIKELY(pg == NULL || pg->rootentry == NULL))
        return csound->InitError(csound,
            "%s", Str("partikkelsync: could not find opcode id"));
    pe = pg->rootentry;
    while (pe->id != *p->opcodeid && pe->next != NULL)
        pe = pe->next;
    if (UNLIKELY(pe->id != *p->opcodeid))
        return csound->InitError(csound,
            "%s", Str("partikkelsync: could not find opcode id"));
    p->ge = pe;
    /* find out if we're supposed to output grain scheduler phase too */
    p->output_schedphase = GetOutputArgCnt((OPDS *)p) > 1;
    return OK;
}

static int32_t partikkelsync(CSOUND *csound, PARTIKKEL_SYNC *p)
{
   IGN(csound);
    /* write sync pulse data */
    memcpy(p->syncout, p->ge->synctab, CS_KSMPS*sizeof(MYFLT));
    /* write scheduler phase data, if user wanted it */
    if (p->output_schedphase) {
        memcpy(p->schedphaseout, p->ge->synctab + CS_KSMPS,
               CS_KSMPS*sizeof(MYFLT));
    }
    /* clear first half of sync table to get rid of old sync pulses */
    memset(p->ge->synctab, 0, CS_KSMPS*sizeof(MYFLT));
    return OK;
}

static int32_t get_global_entry(CSOUND *csound, PARTIKKEL_GLOBALS_ENTRY **entry,
                            MYFLT opcodeid, const char *prefix)
{
    PARTIKKEL_GLOBALS *pg;
    PARTIKKEL_GLOBALS_ENTRY *pe;

    pg = csound->QueryGlobalVariable(csound, "partikkel");
    if (UNLIKELY(pg == NULL))
        return csound->InitError(csound,
                                 Str("%s: partikkel not initialized"), prefix);
    /* try to find entry corresponding to our opcodeid */
    pe = pg->rootentry;
    while (pe != NULL && pe->id != opcodeid)
        pe = pe->next;

    if (UNLIKELY(pe == NULL))
        return csound->InitError(csound,
                                 Str("%s: could not find opcode id"), prefix);
    *entry = pe;
    return OK;
}

static int32_t partikkelget_init(CSOUND *csound, PARTIKKEL_GET *p)
{
    return get_global_entry(csound, &p->ge, *p->opcodeid, "partikkelget");
}

static int32_t partikkelget(CSOUND *csound, PARTIKKEL_GET *p)
{
    IGN(csound);
    PARTIKKEL *partikkel = p->ge->partikkel;

    switch ((int32_t)*p->index) {
    case 0:
        *p->valout = (MYFLT)partikkel->gainmaskindex;
        break;
    case 1:
        *p->valout = (MYFLT)partikkel->wavfreqstartindex;
        break;
    case 2:
        *p->valout = (MYFLT)partikkel->wavfreqendindex;
        break;
    case 3:
        *p->valout = (MYFLT)partikkel->fmampindex;
        break;
    case 4:
        *p->valout = (MYFLT)partikkel->channelmaskindex;
        break;
    case 5:
        *p->valout = (MYFLT)partikkel->wavgainindex;
        break;
    }
    return OK;
}

static int32_t partikkelset_init(CSOUND *csound, PARTIKKEL_SET *p)
{
    return get_global_entry(csound, &p->ge, *p->opcodeid, "partikkelset");
}

static int32_t partikkelset(CSOUND *csound, PARTIKKEL_SET *p)
{
    IGN(csound);
    PARTIKKEL *partikkel = p->ge->partikkel;

    switch ((int32_t)*p->index) {
    case 0:
        partikkel->gainmaskindex = (uint32_t)*p->value;
        break;
    case 1:
        partikkel->wavfreqstartindex = (uint32_t)*p->value;
        break;
    case 2:
        partikkel->wavfreqendindex = (uint32_t)*p->value;
        break;
    case 3:
        partikkel->fmampindex = (uint32_t)*p->value;
        break;
    case 4:
        partikkel->channelmaskindex = (uint32_t)*p->value;
        break;
    case 5:
        partikkel->wavgainindex = (uint32_t)*p->value;
        break;
    }
    return OK;
}

static OENTRY partikkel_localops[] = {
    {
     "partikkel", sizeof(PARTIKKEL), TR, 
        "ammmmmmm",
        "xkiakiiikkkkikkiiaikikkkikkkkkiaaaakkkkioj",
        (SUBR)partikkel_init,
        (SUBR)partikkel
    },
    {
     "partikkelsync", sizeof(PARTIKKEL_SYNC), TR, 
        "am", "i",
        (SUBR)partikkelsync_init,
        (SUBR)partikkelsync
    },
    {
        "partikkelget", sizeof(PARTIKKEL_GET), TR, 
        "k", "ki",
        (SUBR)partikkelget_init,
        (SUBR)partikkelget,
        (SUBR)NULL
    },
    {
        "partikkelset", sizeof(PARTIKKEL_SET), TR, 
        "", "kki",
        (SUBR)partikkelset_init,
        (SUBR)partikkelset,
        (SUBR)NULL
    }
};

LINKAGE_BUILTIN(partikkel_localops)

