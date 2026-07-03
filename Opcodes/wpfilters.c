/*
        wpfilters.c:

        Copyright (C) 2017 Steven Yi

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
        Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/*
Zero Delay Feedback Filters

Based on code by Will Pirkle, presented in:

http://www.willpirkle.com/Downloads/AN-4VirtualAnalogFilters.2.0.pdf
http://www.willpirkle.com/Downloads/AN-5Korg35_V3.pdf
http://www.willpirkle.com/Downloads/AN-6DiodeLadderFilter.pdf
http://www.willpirkle.com/Downloads/AN-7Korg35HPF_V2.pdf

and in his book "Designing software synthesizer plug-ins in C++ : for
RackAFX, VST3, and Audio Units"

ZDF using Trapezoidal integrator by Vadim Zavalishin, presented in "The Art
of VA Filter Design" (https://www.native-instruments.com/fileadmin/ni_media/
downloads/pdf/VAFilterDesign_1.1.1.pdf)

Csound C versions by Steven Yi
*/

#include "wpfilters.h"

static int32_t zdf_1pole_mode_init(CSOUND* csound, ZDF_1POLE_MODE* p) {
     IGN(csound);
    if (*p->skip == 0) {
      p->z1 = 0.0;
      p->last_cut = -1.0;
    }
    return OK;
}


static int32_t zdf_1pole_mode_perf(CSOUND* csound, ZDF_1POLE_MODE* p) {
     IGN(csound);
    MYDBL z1 = p->z1;
    MYDBL last_cut = p->last_cut;
    MYDBL G = p->G;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    MYDBL T = CS_ONEDSR;
    MYDBL Tdiv2 = T / 2.0;
    MYDBL two_div_T = 2.0 / T;

    int32_t cutoff_arate = IS_ASIG_ARG(p->cutoff);

    MYFLT cutoff = cutoff_arate ? 0.0 : *p->cutoff;

    if (UNLIKELY(offset)) {
      memset(p->outlp, '\0', offset*sizeof(MYFLT));
      memset(p->outhp, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->outlp[nsmps], '\0', early*sizeof(MYFLT));
      memset(&p->outhp[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n = offset; n < nsmps; n++) {

      if (cutoff_arate) {
        cutoff = p->cutoff[n];
      }

      if (cutoff != last_cut) {
        last_cut = cutoff;

        MYDBL wd = TWOPI * cutoff;
        MYDBL wa = two_div_T * TAN(wd * Tdiv2);
        MYDBL g = wa * Tdiv2;
        G = g / (1.0 + g);
      }

      // do the filter, see VA book p. 46
      // form sub-node value v(n)

      MYDBL in = p->in[n];
      MYDBL v = (in - z1) * G;

      // form output of node + register
      MYDBL lp = v + z1;
      MYDBL hp = in - lp;

      // z1 register update
      z1 = lp + v;

      p->outlp[n] = lp;
      p->outhp[n] = hp;
    }

    p->z1 = z1;
    p->last_cut = last_cut;
    p->G = G;

    return OK;
}

static int32_t zdf_1pole_init(CSOUND* csound, ZDF_1POLE* p) {
   IGN(csound);
    if (*p->skip == 0) {
      p->z1 = 0.0;
      p->last_cut = -1.0;
    }
    return OK;
}

static int32_t zdf_1pole_perf(CSOUND* csound, ZDF_1POLE* p) {

    MYDBL z1 = p->z1;
    MYDBL last_cut = p->last_cut;
    MYDBL G = p->G;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    MYDBL T = CS_ONEDSR;
    MYDBL Tdiv2 = T / 2.0;
    MYDBL two_div_T = 2.0 / T;
    int32_t mode = MYFLT2LONG(*p->mode);

    int32_t cutoff_arate = IS_ASIG_ARG(p->cutoff);

    MYFLT cutoff = cutoff_arate ? 0.0 : *p->cutoff;

    if (UNLIKELY(offset)) {
      memset(p->out, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {

      if (cutoff_arate) {
        cutoff = p->cutoff[n];
      }

      if (cutoff != last_cut) {
        last_cut = cutoff;

        MYDBL wd = TWOPI * cutoff;
        MYDBL wa = two_div_T * TAN(wd * Tdiv2);
        MYDBL g = wa * Tdiv2;
        G = g / (1.0 + g);
      }

      // do the filter, see VA book p. 46
      // form sub-node value v(n)

      MYDBL in = p->in[n];
      MYDBL v = (in - z1) * G;

      // form output of node + register
      MYDBL lp = v + z1;

      if (mode == 0) { // low-pass
        p->out[n] = lp;
      }
      else if (mode == 1) { // high-pass
        MYDBL hp = in - lp;
        p->out[n] = hp;
      }
      else if (mode == 2) { // allpass
        MYDBL hp = in - lp;
        p->out[n] = lp - hp;
      }
      // TODO Implement low-shelf and high-shelf
      //else if (mode == 3) { // low-shelf
      //}
      //else if (mode == 4) { // high-shelf
      //}

      // z1 register update
      z1 = lp + v;
    }

    p->z1 = z1;
    p->last_cut = last_cut;
    p->G = G;

    return OK;
}


static int32_t zdf_2pole_mode_init(CSOUND* csound, ZDF_2POLE_MODE* p) {
     IGN(csound);
    if (*p->skip == 0) {
      p->z1 = 0.0;
      p->z2 = 0.0;
      p->last_cut = -1.0;
      p->last_q = -1.0;
      p->g = 0.0;
      p->R = 0.0;
    }
    return OK;
}


static int32_t zdf_2pole_mode_perf(CSOUND* csound, ZDF_2POLE_MODE* p) {
    MYDBL z1 = p->z1;
    MYDBL z2 = p->z2;
    MYDBL last_cut = p->last_cut;
    MYDBL last_q = p->last_q;
    MYDBL g = p->g;
    MYDBL R = p->R;
    MYDBL g2 = g * g;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    MYDBL T = CS_ONEDSR;
    MYDBL Tdiv2 = T / 2.0;
    MYDBL two_div_T = 2.0 / T;

    int32_t cutoff_arate = IS_ASIG_ARG(p->cutoff);
    int32_t q_arate = IS_ASIG_ARG(p->q);

    MYFLT cutoff = *p->cutoff;
    MYFLT q = *p->q;

    if (UNLIKELY(offset)) {
      memset(p->outlp, '\0', offset*sizeof(MYFLT));
      memset(p->outhp, '\0', offset*sizeof(MYFLT));
      memset(p->outbp, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->outlp[nsmps], '\0', early*sizeof(MYFLT));
      memset(&p->outhp[nsmps], '\0', early*sizeof(MYFLT));
      memset(&p->outbp[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {

      if (cutoff_arate) {
        cutoff = p->cutoff[n];
      }
      if (q_arate) {
        q = p->q[n];
      }

      if (cutoff != last_cut) {
        last_cut = cutoff;

        MYDBL wd = TWOPI * cutoff;
        MYDBL wa = two_div_T * TAN(wd * Tdiv2);
        g = wa * Tdiv2;
        g2 = g * g;
      }

      if (q != last_q) {
        last_q = q;
        R = 1.0 / (2.0 * q);
      }

      MYDBL in = p->in[n];
      MYDBL hp = (in - (2.0 * R + g) * z1 - z2) / (1.0 + (2.0 * R * g) + g2);
      MYDBL bp = g * hp + z1;
      MYDBL lp = g * bp + z2;
      //              MYDBL notch = in - (2.0 * R * bp);

      // register updates
      z1 = g * hp + bp;
      z2 = g * bp + lp;

      p->outlp[n] = lp;
      p->outhp[n] = hp;
      p->outbp[n] = bp;
      //              p->outnotch[n] = notch;
    }

    p->z1 = z1;
    p->z2 = z2;
    p->last_cut = last_cut;
    p->last_q = last_q;
    p->g = g;
    p->R = R;

    return OK;
}


static int32_t zdf_2pole_init(CSOUND* csound, ZDF_2POLE* p) {
     IGN(csound);
    if (*p->skip == 0) {
      p->z1 = 0.0;
      p->z2 = 0.0;
      p->last_cut = -1.0;
      p->last_q = -1.0;
      p->g = 0.0;
      p->R = 0.0;
    }
    return OK;
}

static int32_t zdf_2pole_perf(CSOUND* csound, ZDF_2POLE* p) {
    MYDBL z1 = p->z1;
    MYDBL z2 = p->z2;
    MYDBL last_cut = p->last_cut;
    MYDBL last_q = p->last_q;
    int32_t mode = MYFLT2LONG(*p->mode);
    MYDBL g = p->g;
    MYDBL R = p->R;
    MYDBL g2 = g * g;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    MYDBL T = CS_ONEDSR;
    MYDBL Tdiv2 = T / 2.0;
    MYDBL two_div_T = 2.0 / T;

    int32_t cutoff_arate = IS_ASIG_ARG(p->cutoff);
    int32_t q_arate = IS_ASIG_ARG(p->q);

    MYFLT cutoff = *p->cutoff;
    MYFLT q = *p->q;

    if (UNLIKELY(offset)) {
      memset(p->out, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {

      if (cutoff_arate) {
        cutoff = p->cutoff[n];
      }
      if (q_arate) {
        q = p->q[n];
      }

      if (cutoff != last_cut) {
        last_cut = cutoff;

        MYDBL wd = TWOPI * cutoff;
        MYDBL wa = two_div_T * TAN(wd * Tdiv2);
        g = wa * Tdiv2;
        g2 = g * g;
      }

      if (q != last_q) {
        last_q = q;
        R = 1.0 / (2.0 * q);
      }

      MYDBL in = p->in[n];
      MYDBL hp = (in - (2.0 * R + g) * z1 - z2) / (1.0 + (2.0 * R * g) + g2);
      MYDBL bp = g * hp + z1;
      MYDBL lp = g * bp + z2;

      if (mode == 0) { // low-pass
        p->out[n] = lp;
      }
      else if (mode == 1) { // high-pass
        p->out[n] = hp;
      }
      else if (mode == 2) { // band-pass
        p->out[n] = bp;
      }
      else if (mode == 3) { // unity-gain band-pass
        p->out[n] = 2.0 * R * bp;
      }
      else if (mode == 4) { // notch
        p->out[n] = in - 2.0 * R * bp;
      }
      else if (mode == 5) { // all-pass filter
        p->out[n] = in - 4.0 * R * bp;
      }
      else if (mode == 6) { // peak filter
        p->out[n] = lp - hp;
      }
      //else if (mode == 7) { // band shelf - not implemented
      //      p->out[n] = in + 2.0 * K * R * bp;
      //}

      // register updates
      z1 = g * hp + bp;
      z2 = g * bp + lp;

    }

    p->z1 = z1;
    p->z2 = z2;
    p->last_cut = last_cut;
    p->last_q = last_q;
    p->g = g;
    p->R = R;

    return OK;
}

static int32_t zdf_ladder_init(CSOUND* csound, ZDF_LADDER* p) {
     IGN(csound);
    if (*p->skip == 0) {
      p->z1 = 0.0;
      p->z2 = 0.0;
      p->z3 = 0.0;
      p->z4 = 0.0;
      p->last_cut = -1.0;
      p->last_q = -1.0;
      p->last_k = 0.0;
      p->last_g = 0.0;
      p->last_G = 0.0;
      p->last_G2 = 0.0;
      p->last_G3 = 0.0;
      p->last_GAMMA = 0.0;
    }

    return OK;
}

static int32_t zdf_ladder_perf(CSOUND* csound, ZDF_LADDER* p) {

    MYDBL z1 = p->z1;
    MYDBL z2 = p->z2;
    MYDBL z3 = p->z3;
    MYDBL z4 = p->z4;
    MYDBL last_cut = p->last_cut;
    MYDBL last_q = p->last_q;
    MYDBL k = p->last_k;
    MYDBL g = p->last_g;
    MYDBL G = p->last_G;
    MYDBL G2 = p->last_G2;
    MYDBL G3 = p->last_G3;
    MYDBL GAMMA = p->last_GAMMA;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    MYDBL T = CS_ONEDSR;
    MYDBL Tdiv2 = T / 2.0;
    MYDBL two_div_T = 2.0 / T;

    int32_t cutoff_arate = IS_ASIG_ARG(p->cutoff);
    int32_t q_arate = IS_ASIG_ARG(p->q);

    MYFLT cutoff = cutoff_arate ? 0.0 : *p->cutoff;
    MYFLT q = q_arate ? 0.0 : *p->q;

    if (UNLIKELY(offset)) {
      memset(p->out, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {

      if (cutoff_arate) {
        cutoff = p->cutoff[n];
      }
      if (q_arate) {
        q = p->q[n];
        q = (q < 0.5) ? 0.5 : (q > 25.0) ? 25.0 : q;
      }

      if (q != last_q) {
        last_q = q;
        // Q [0.5,25] = k [0,4.0]
        k = (4.0 * (q - 0.5)) / (25.0 - 0.5);
      }

      if (cutoff != last_cut) {
        last_cut = cutoff;

        MYDBL wd = TWOPI * cutoff;
        MYDBL wa = two_div_T * TAN(wd * Tdiv2);
        g = wa * Tdiv2;
        G = g / (1.0 + g);
        G2 = G * G;
        G3 = G2 * G;
        GAMMA = G2 * G2;
      }

      MYDBL g_plus_1 = g + 1.0;

      MYDBL S1 = z1 / g_plus_1;
      MYDBL S2 = z2 / g_plus_1;
      MYDBL S3 = z3 / g_plus_1;
      MYDBL S4 = z4 / g_plus_1;

      MYDBL S = (G3 * S1) + (G2 * S2) + (G * S3) + S4;
      MYDBL u = (p->in[n] - k *  S) / (1 + k * GAMMA);

      // 1st stage
      MYDBL v = (u - z1) * G;
      MYDBL lp = v + z1;
      z1 = lp + v;

      // 2nd stage
      v = (lp - z2) * G;
      lp = v + z2;
      z2 = lp + v;

      // 3rd stage
      v = (lp - z3) * G;
      lp = v + z3;
      z3 = lp + v;

      // 4th stage
      v = (lp - z4) * G;
      lp = v + z4;
      z4 = lp + v;

      p->out[n] = lp;
    }

    p->z1 = z1;
    p->z2 = z2;
    p->z3 = z3;
    p->z4 = z4;

    p->last_cut = last_cut;
    p->last_q = last_q;
    p->last_k = k;
    p->last_g = g;
    p->last_G = G;
    p->last_G2 = G2;
    p->last_G3 = G3;
    p->last_GAMMA = GAMMA;

    return OK;
}


static int32_t diode_ladder_init(CSOUND* csound,
                             DIODE_LADDER* p) {
     IGN(csound);
    if (*p->skip == 0) {
      int32_t i;
      p->a[0] = 1.0;
      p->a[1] = 0.5;
      p->a[2] = 0.5;
      p->a[3] = 0.5;

      for (i = 0; i < 4; i++) {
        p->z[i] = 0.0;
        p->G[i] = 0.0;
        p->beta[i] = 0.0;
        p->SG[i] = 0.0;
      }

      for (i = 0; i < 3; i++) {
        p->delta[i] = 0.0;
        p->epsilon[i] = 0.0;
        p->gamma[i] = 0.0;
      }
      p->GAMMA = 0.0;
      p->SIGMA = 0.0;
      p->last_cut = -1.0;
    }

    return OK;
}

static int32_t diode_ladder_perf(CSOUND* csound,
                             DIODE_LADDER* p) {

    MYDBL a1 = p->a[0];
    MYDBL a2 = p->a[1];
    MYDBL a3 = p->a[2];
    MYDBL a4 = p->a[3];
    MYDBL z1 = p->z[0];
    MYDBL z2 = p->z[1];
    MYDBL z3 = p->z[2];
    MYDBL z4 = p->z[3];
    MYDBL G1 = p->G[0];
    MYDBL G2 = p->G[1];
    MYDBL G3 = p->G[2];
    MYDBL G4 = p->G[3];
    MYDBL beta1 = p->beta[0];
    MYDBL beta2 = p->beta[1];
    MYDBL beta3 = p->beta[2];
    MYDBL beta4 = p->beta[3];
    MYDBL delta1 = p->delta[0];
    MYDBL delta2 = p->delta[1];
    MYDBL delta3 = p->delta[2];
    MYDBL epsilon1 = p->epsilon[0];
    MYDBL epsilon2 = p->epsilon[1];
    MYDBL epsilon3 = p->epsilon[2];
    MYDBL gamma1 = p->gamma[0];
    MYDBL gamma2 = p->gamma[1];
    MYDBL gamma3 = p->gamma[2];
    MYDBL SG1 = p->SG[0];
    MYDBL SG2 = p->SG[1];
    MYDBL SG3 = p->SG[2];
    MYDBL SG4 = p->SG[3];
    MYDBL GAMMA = p->GAMMA;
    MYDBL SIGMA = p->SIGMA;
    MYDBL alpha = p->last_alpha;
    MYDBL last_cut = p->last_cut;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    MYDBL T = CS_ONEDSR;
    MYDBL Tdiv2 = T / 2.0;
    MYDBL two_div_T = 2.0 / T;

    int32_t cutoff_arate = IS_ASIG_ARG(p->cutoff);
    int32_t k_arate = IS_ASIG_ARG(p->kval);

    MYFLT cutoff = cutoff_arate ? 0.0 : *p->cutoff;
    MYFLT k = k_arate ? 0.0 : *p->kval;

    if (UNLIKELY(offset)) {
      memset(p->out, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      MYFLT in = p->in[n];

      if (cutoff_arate) {
        cutoff = p->cutoff[n];
      }
      if (k_arate) {
        k = p->kval[n];
      }

      if (cutoff != last_cut) {
        last_cut = cutoff;

        MYDBL wd = TWOPI * cutoff;
        MYDBL wa = two_div_T * TAN(wd * Tdiv2);
        MYDBL g = wa * Tdiv2;
        MYDBL gp1 = 1.0 + g;
        G4 = 0.5 * g / gp1;
        G3 = 0.5 * g / (gp1 - 0.5 * g * G4);
        G2 = 0.5 * g / (gp1 - 0.5 * g * G3);
        G1 = g / (gp1 - g * G2);
        GAMMA = G4 * G3 * G2 * G1;

        SG1 = G4 * G3 * G2;
        SG2 = G4 * G3;
        SG3 = G4;
        SG4 = 1.0;

        alpha = g / gp1;

        beta1 = 1.0 / (gp1 - g * G2);
        beta2 = 1.0 / (gp1 - 0.5 * g * G3);
        beta3 = 1.0 / (gp1 - 0.5 * g * G4);
        beta4 = 1.0 / gp1;

        gamma1 = 1.0 + G1 * G2;
        gamma2 = 1.0 + G2 * G3;
        gamma3 = 1.0 + G3 * G4;

        delta1 = g;
        delta2 = delta3 = 0.5 * g;

        epsilon1 = G2;
        epsilon2 = G3;
        epsilon3 = G4;
      }

      //feedback inputs
      MYDBL fb4 = beta4 * z4;
      MYDBL fb3 = beta3 * (z3 + fb4 * delta3);
      MYDBL fb2 = beta2 * (z2 + fb3 * delta2);

      //feedback process
      MYDBL fbo1 = (beta1 * (z1 + fb2 * delta1));
      MYDBL fbo2 = (beta2 * (z2 + fb3 * delta2));
      MYDBL fbo3 = (beta3 * (z3 + fb4 * delta3));

      SIGMA = (SG1 * fbo1) + (SG2 * fbo2) + (SG3 * fbo3) + (SG4 * fb4);

      // non-linear processing
      if (*p->nlp == 1.0) {
        in = (1.0 / TANH(*p->saturation)) * TANH(*p->saturation * in);
      }
      else if (*p->nlp == 2.0) {
        in = TANH(*p->saturation * in);
      }

      // form input to loop
      MYDBL un = (in - k * SIGMA) / (1.0 + k * GAMMA);

      // 1st stage
      MYDBL xin = un * gamma1 + fb2 + epsilon1 * fbo1;
      MYDBL v = (a1 * xin - z1) * alpha;
      MYDBL lp = v + z1;
      z1 = lp + v;

      // 2nd stage
      xin = lp * gamma2 + fb3 + epsilon2 * fbo2;
      v = (a2 * xin - z2) * alpha;
      lp = v + z2;
      z2 = lp + v;

      // 3rd stage
      xin = lp * gamma3 + fb4 + epsilon3 * fbo3;
      v = (a3 * xin - z3) * alpha;
      lp = v + z3;
      z3 = lp + v;

      // 4th stage
      v = (a4 * lp - z4) * alpha;
      lp = v + z4;
      z4 = lp + v;

      p->out[n] = lp;
    }

    p->a[0] = a1;
    p->a[1] = a2;
    p->a[2] = a3;
    p->a[3] = a4;
    p->z[0] = z1;
    p->z[1] = z2;
    p->z[2] = z3;
    p->z[3] = z4;
    p->G[0] = G1;
    p->G[1] = G2;
    p->G[2] = G3;
    p->G[3] = G4;
    p->beta[0] = beta1;
    p->beta[1] = beta2;
    p->beta[2] = beta3;
    p->beta[3] = beta4;
    p->delta[0] = delta1;
    p->delta[1] = delta2;
    p->delta[2] = delta3;
    p->epsilon[0] = epsilon1;
    p->epsilon[1] = epsilon2;
    p->epsilon[2] = epsilon3;
    p->gamma[0] = gamma1;
    p->gamma[1] = gamma2;
    p->gamma[2] = gamma3;
    p->SG[0] = SG1;
    p->SG[1] = SG2;
    p->SG[2] = SG3;
    p->SG[3] = SG4;
    p->GAMMA = GAMMA;
    p->SIGMA = SIGMA;
    p->last_alpha = alpha;
    p->last_cut = last_cut;

    return OK;
}


static int32_t k35_lpf_init(CSOUND* csound, K35_LPF* p) {
     IGN(csound);
    if (*p->skip == 0.0) {
      p->z1 = 0.0;
      p->z2 = 0.0;
      p->z3 = 0.0;
      p->last_cut = -1.0;
      p->last_q = -1.0;
      p->g = 0.0;
      p->G = 0.0;
      p->S35 = 0.0;
      p->alpha = 0.0;
      p->lpf2_beta = 0.0;
      p->hpf1_beta = 0.0;
    }

    return OK;
}


static int32_t k35_lpf_perf(CSOUND* csound, K35_LPF* p) {

    MYDBL z1 = p->z1;
    MYDBL z2 = p->z2;
    MYDBL z3 = p->z3;
    MYDBL last_cut = p->last_cut;
    MYDBL last_q = p->last_q;
    MYDBL g = p->g;
    MYDBL G = p->G;
    MYDBL K = p->K;
    MYDBL S35 = p->S35;
    MYDBL alpha = p->alpha;
    MYDBL lpf2_beta = p->lpf2_beta;
    MYDBL hpf1_beta = p->hpf1_beta;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    MYDBL T = CS_ONEDSR;
    MYDBL Tdiv2 = T / 2.0;
    MYDBL two_div_T = 2.0 / T;

    int32_t cutoff_arate = IS_ASIG_ARG(p->cutoff);
    int32_t q_arate = IS_ASIG_ARG(p->q);

    MYFLT cutoff = cutoff_arate ? 0.0 : *p->cutoff;
    MYFLT q = q_arate ? 0.0 : *p->q;

    int32_t nonlinear = MYFLT2LONG(*p->nonlinear);
    MYDBL saturation = *p->saturation;

    if (UNLIKELY(offset)) {
      memset(p->out, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      MYFLT in = p->in[n];

      if (cutoff_arate) {
        cutoff = p->cutoff[n];
      }
      if (q_arate) {
        q = p->q[n];
        // clamp from 1.0 to 10.0
        q = (q > 10.0) ? 10.0 : (q < 1.0) ? 1.0 : q;
      }

      if (cutoff != last_cut) {
        MYDBL wd = TWOPI * cutoff;
        MYDBL wa = two_div_T * TAN(wd * Tdiv2);
        g = wa * Tdiv2;
        G = g / (1.0 + g);
      }

      if (q != last_q) {
        K = 0.01 + ((2.0 - 0.01) * (q / 10.0));
      }

      if ((cutoff != last_cut) || (q != last_q)) {
        lpf2_beta = (K - (K * G)) / (1.0 + g);
        hpf1_beta = -1.0 / (1.0 + g);
        alpha = 1.0 / (1.0 - (K * G) + (K * G * G));
      }


      last_cut = cutoff;
      last_q = q;

      // LPF1
      MYDBL v1 = (in - z1) * G;
      MYDBL lp1 = v1 + z1;
      z1 = lp1 + v1;

      MYDBL u = alpha * (lp1 + S35);

      if (nonlinear) {
        u = TANH(u * saturation);
      }

      // LPF2
      MYDBL v2 = (u - z2) * G;
      MYDBL lp2 = v2 + z2;
      z2 = lp2 + v2;
      MYDBL y = K * lp2;

      // HPF1

      MYDBL v3 = (y - z3) * G;
      MYDBL lp3 = v3 + z3;
      z3 = lp3 + v3;
      // MYDBL hp1 = y - lp3; /* FIXME: not used */

      S35 = (lpf2_beta * z2) + (hpf1_beta * z3);
      MYDBL out = (K > 0) ? (y / K) : y;

      p->out[n] = out;
    }

    p->z1 = z1;
    p->z2 = z2;
    p->z3 = z3;
    p->last_cut = last_cut;
    p->last_q = last_q;
    p->g = g;
    p->G = G;
    p->K = K;
    p->S35 = S35;
    p->alpha = alpha;
    p->lpf2_beta = lpf2_beta;
    p->hpf1_beta = hpf1_beta;

    return OK;
}


static int32_t k35_hpf_init(CSOUND* csound, K35_HPF* p) {
     IGN(csound);
    if (*p->skip == 0.0) {
      p->z1 = 0.0;
      p->z2 = 0.0;
      p->z3 = 0.0;
      p->last_cut = -1.0;
      p->last_q = -1.0;
      p->g = 0.0;
      p->G = 0.0;
      p->S35 = 0.0;
      p->alpha = 0.0;
      p->hpf2_beta = 0.0;
      p->lpf1_beta = 0.0;
    }

    return OK;
}


static int32_t k35_hpf_perf(CSOUND* csound, K35_HPF* p) {

    MYDBL z1 = p->z1;
    MYDBL z2 = p->z2;
    MYDBL z3 = p->z3;
    MYDBL last_cut = p->last_cut;
    MYDBL last_q = p->last_q;
    MYDBL g = p->g;
    MYDBL G = p->G;
    MYDBL K = p->K;
    MYDBL S35 = p->S35;
    MYDBL alpha = p->alpha;
    MYDBL hpf2_beta = p->hpf2_beta;
    MYDBL lpf1_beta = p->lpf1_beta;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    MYDBL T = CS_ONEDSR;
    MYDBL Tdiv2 = T / 2.0;
    MYDBL two_div_T = 2.0 / T;

    int32_t cutoff_arate = IS_ASIG_ARG(p->cutoff);
    int32_t q_arate = IS_ASIG_ARG(p->q);

    MYFLT cutoff = cutoff_arate ? 0.0 : *p->cutoff;
    MYFLT q = q_arate ? 0.0 : *p->q;

    int32_t
      nonlinear = MYFLT2LONG(*p->nonlinear);
    MYDBL saturation = *p->saturation;

    if (UNLIKELY(offset)) {
      memset(p->out, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      MYFLT in = p->in[n];

      if (cutoff_arate) {
        cutoff = p->cutoff[n];
      }
      if (q_arate) {
        q = p->q[n];
        // clamp from 1.0 to 10.0
        q = (q > 10.0) ? 10.0 : (q < 1.0) ? 1.0 : q;
      }

      if (cutoff != last_cut) {
        MYDBL wd = TWOPI * cutoff;
        MYDBL wa = two_div_T * TAN(wd * Tdiv2);
        g = wa * Tdiv2;
        G = g / (1.0 + g);
      }

      if (q != last_q) {
        K = 0.01 + ((2.0 - 0.01) * (q / 10.0));
      }

      if ((cutoff != last_cut) || (q != last_q)) {
        hpf2_beta = -G / (1.0 + g);
        lpf1_beta = 1.0 / (1.0 + g);
        alpha = 1.0 / (1.0 - (K * G) + (K * G * G));
      }


      last_cut = cutoff;
      last_q = q;

      // HPF1
      MYDBL v1 = (in - z1) * G;
      MYDBL lp1 = v1 + z1;
      z1 = lp1 + v1;
      MYDBL y1 = in - lp1;

      MYDBL u = alpha * (y1 + S35);
      MYDBL y = K * u;

      if (nonlinear) {
        y = TANH(y * saturation);
      }

      // HPF2
      MYDBL v2 = (y - z2) * G;
      MYDBL lp2 = v2 + z2;
      z2 = lp2 + v2;
      MYDBL hp2 = y - lp2;

      // LPF1
      MYDBL v3 = (hp2 - z3) * G;
      MYDBL lp3 = v3 + z3;
      z3 = lp3 + v3;

      S35 = (hpf2_beta * z2) + (lpf1_beta * z3);
      MYDBL out = (K > 0) ? (y / K) : y;

      p->out[n] = out;
    }

    p->z1 = z1;
    p->z2 = z2;
    p->z3 = z3;
    p->last_cut = last_cut;
    p->last_q = last_q;
    p->g = g;
    p->G = G;
    p->K = K;
    p->S35 = S35;
    p->alpha = alpha;
    p->hpf2_beta = hpf2_beta;
    p->lpf1_beta = lpf1_beta;

    return OK;
}


static OENTRY wpfilters_localops[] =
  {
  { "zdf_1pole", sizeof(ZDF_1POLE), 0,"a","axOo", 
    (SUBR)zdf_1pole_init, (SUBR)zdf_1pole_perf, NULL, NULL, 2},
  { "zdf_1pole_mode", sizeof(ZDF_1POLE_MODE), 0,"aa","axo", 
    (SUBR)zdf_1pole_mode_init, (SUBR)zdf_1pole_mode_perf, NULL, NULL, 2},
  { "zdf_2pole", sizeof(ZDF_2POLE), 0,"a","axxOo", 
    (SUBR)zdf_2pole_init, (SUBR)zdf_2pole_perf, NULL, NULL, 2},
  { "zdf_2pole_mode", sizeof(ZDF_2POLE_MODE), 0,"aaa","axxo", 
    (SUBR)zdf_2pole_mode_init, (SUBR)zdf_2pole_mode_perf, NULL, NULL, 2},
  { "zdf_ladder", sizeof(ZDF_LADDER), 0,"a","axxo", 
    (SUBR)zdf_ladder_init, (SUBR)zdf_ladder_perf, NULL, NULL, 2},
  { "diode_ladder", sizeof(DIODE_LADDER), 0,"a","axxOPo", 
    (SUBR)diode_ladder_init, (SUBR)diode_ladder_perf, NULL, NULL, 2},
  { "K35_lpf", sizeof(K35_LPF), 0,"a","axxOPo", 
    (SUBR)k35_lpf_init, (SUBR)k35_lpf_perf, NULL, NULL, 2},
  { "K35_hpf", sizeof(K35_LPF), 0,"a","axxOPo", 
    (SUBR)k35_hpf_init, (SUBR)k35_hpf_perf, NULL, NULL, 2},
   /* aliases */
   { "zdf1pole", sizeof(ZDF_1POLE), 0,"a","axOo",
      (SUBR)zdf_1pole_init,(SUBR)zdf_1pole_perf},
   { "zdf1polemode", sizeof(ZDF_1POLE_MODE), 0,"aa","axo",
      (SUBR)zdf_1pole_mode_init,(SUBR)zdf_1pole_mode_perf},
   { "zdf2pole", sizeof(ZDF_2POLE), 0,"a","axxOo",
      (SUBR)zdf_2pole_init,(SUBR)zdf_2pole_perf},
   { "zdf2polemode", sizeof(ZDF_2POLE_MODE), 0,"aaa","axxo",
      (SUBR)zdf_2pole_mode_init,(SUBR)zdf_2pole_mode_perf},
   { "zdfladder", sizeof(ZDF_LADDER), 0,"a","axxo",
      (SUBR)zdf_ladder_init,(SUBR)zdf_ladder_perf},
   { "diodeladder", sizeof(DIODE_LADDER), 0,"a","axxOPo",
      (SUBR)diode_ladder_init,(SUBR)diode_ladder_perf},
   { "k35lpf", sizeof(K35_LPF), 0,"a","axxOPo",
      (SUBR)k35_lpf_init,(SUBR)k35_lpf_perf},
   { "k35hpf", sizeof(K35_LPF), 0,"a","axxOPo",(SUBR)
      k35_hpf_init,(SUBR)k35_hpf_perf},
  };

LINKAGE_BUILTIN(wpfilters_localops)
