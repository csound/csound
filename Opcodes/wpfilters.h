/*
    wpfilters.h:

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

/*
Zero Delay Feedback Filters

Based on code by Will Pirkle, presented in:

http://www.willpirkle.com/Downloads/AN-4VirtualAnalogFilters.2.0.pdf

and in his book "Designing software synthesizer plug-ins in C++ : for
RackAFX, VST3, and Audio Units"

ZDF using Trapezoidal integrator by Vadim Zavalishin, presented in "The Art
of VA Filter Design" (https://www.native-instruments.com/fileadmin/ni_media/
downloads/pdf/VAFilterDesign_1.1.1.pdf)

Csound C versions by Steven Yi
*/

#pragma once

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

typedef struct {
  OPDS h;
  MYFLT *out;
  MYFLT *in, *cutoff, *mode, *skip;
  MYFLT last_cut, G;
  double z1;
} ZDF_1POLE;


typedef struct {
  OPDS h;
  MYFLT *outlp, *outhp;
  MYFLT *in, *cutoff, *skip;
  MYFLT last_cut, G;
  double z1;
} ZDF_1POLE_MODE;

typedef struct {
  OPDS h;
  MYFLT *out;
  MYFLT *in, *cutoff, *q, *mode, *skip;
  double last_cut, last_q, g, R;
  double z1, z2;
} ZDF_2POLE;

typedef struct {
  OPDS h;
  MYFLT *outlp,*outbp, *outhp;
  MYFLT *in, *cutoff, *q, *skip;
  double last_cut, last_q, g, R;
  double z1, z2;
} ZDF_2POLE_MODE;


typedef struct {
  OPDS h;
  MYFLT *out;
  MYFLT *in, *cutoff, *q, *skip;
  double last_cut, last_q, last_k, last_g, last_G, last_G2, last_G3, last_GAMMA;
  double z1, z2, z3, z4;
} ZDF_LADDER;

typedef struct {
  OPDS h;
  MYFLT *out;
  MYFLT *in, *cutoff, *kval, *nlp, *saturation, *skip;
  double a[4], z[4], G[4], beta[4], delta[3], epsilon[3], gamma[3], SG[4];
  double SIGMA, GAMMA, last_alpha, last_cut;
} DIODE_LADDER;

typedef struct {
  OPDS h;
  MYFLT *out;
  MYFLT *in, *cutoff, *q, *nonlinear, *saturation, *skip;
  double z1, z2, z3, last_cut, last_q, g, G, K, S35, alpha, lpf2_beta, hpf1_beta;
} K35_LPF;

typedef struct {
  OPDS h;
  MYFLT *out;
  MYFLT *in, *cutoff, *q, *nonlinear, *saturation, *skip;
  double z1, z2, z3, last_cut, last_q, g, G, K, S35, alpha, hpf2_beta, lpf1_beta;
} K35_HPF;


//typedef struct {
//
//} K35HPF;

static int32_t
zdf_ladder_perf(CSOUND * csound, ZDF_LADDER * p);
