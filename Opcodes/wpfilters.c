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
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
	02111-1307 USA
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

#include "wpfilters.h"

static int zdf_1pole_init(CSOUND* csound, ZDF_1POLE* p) {
	if (*p->skip == 0) {
		p->z1 = 0.0;
		p->last_cut = -1.0;
	}
	return OK;
}



static int zdf_1pole_perf(CSOUND* csound, ZDF_1POLE* p) {

	double z1 = p->z1;
	double last_cut = p->last_cut;
	double G = p->G;

	uint32_t offset = p->h.insdshead->ksmps_offset;
	uint32_t early = p->h.insdshead->ksmps_no_end;
	uint32_t n, nsmps = CS_KSMPS;

	double T = csound->onedsr;
	double Tdiv2 = T / 2.0;
	double two_div_T = 2.0 / T;

	int cutoff_arate = IS_ASIG_ARG(p->cutoff);

	MYFLT cutoff = cutoff_arate ? 0.0 : *p->cutoff;

	for (n = offset; n < nsmps; n++) {

		if (cutoff_arate) {
			cutoff = p->cutoff[n];
		}

		if (cutoff != last_cut) {
			last_cut = cutoff;

			double wd = TWOPI * cutoff;
			double wa = two_div_T * tan(wd * Tdiv2);
			double g = wa * Tdiv2;
			G = g / (1.0 + g);
		}

		// do the filter, see VA book p. 46
		// form sub-node value v(n)

		double in = p->in[n];
		double v = (in - z1) * G;

		// form output of node + register
		double lp = v + z1;
		double hp = in - lp;

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


static int zdf_ladder_init(CSOUND* csound, ZDF_LADDER* p) {

	if (*p->skip == 0) {
		p->z1 = 0.0;
		p->z2 = 0.0;
		p->z3 = 0.0;
		p->z4 = 0.0;
		p->last_cut = -1.0;
		p->last_res = -1.0;
		p->last_k = 0.0;
		p->last_g = 0.0;
		p->last_G = 0.0;
		p->last_G2 = 0.0;
		p->last_G3 = 0.0;
		p->last_GAMMA = 0.0;
	}

	return OK;
}

static int zdf_ladder_perf(CSOUND* csound, ZDF_LADDER* p) {

	double z1 = p->z1;
	double z2 = p->z2;
	double z3 = p->z3;
	double z4 = p->z4;
	double last_cut = p->last_cut;
	double last_res = p->last_res;
	double k = p->last_k;
	double g = p->last_g;
	double G = p->last_G;
	double G2 = p->last_G2;
	double G3 = p->last_G3;
	double GAMMA = p->last_GAMMA;

	uint32_t offset = p->h.insdshead->ksmps_offset;
	uint32_t early = p->h.insdshead->ksmps_no_end;
	uint32_t n, nsmps = CS_KSMPS;

	double T = csound->onedsr;
	double Tdiv2 = T / 2.0;
	double two_div_T = 2.0 / T;

	int cutoff_arate = IS_ASIG_ARG(p->cutoff);
	int res_arate = IS_ASIG_ARG(p->res);

	MYFLT cutoff = cutoff_arate ? 0.0 : *p->cutoff;
	MYFLT res = res_arate ? 0.0 : *p->res;

	for (n = offset; n < nsmps; n++) {

		if (cutoff_arate) {
			cutoff = p->cutoff[n];
		}
		if (res_arate) {
			res = p->res[n];
		}

		if (res != last_res) {
			// first clamp to range 0.0-1.0
			double R = (res < 0.0) ? 0.0 : (res > 1.0) ? 1.0 : res;
			// move to range 0-0.98, which puts Q between 0.5 and 25
			R = R * 0.98;
			double Q = 1 / (2 * (1 - R));
			last_res = res;
			// Q [0.5,25] = k [0,4.0]
			k = (4.0 * (Q - 0.5)) / (25.0 - 0.5);
		}

		if (cutoff != last_cut) {
			last_cut = cutoff;

			double wd = TWOPI * cutoff;
			double wa = two_div_T * tan(wd * Tdiv2);
			g = wa * Tdiv2;
			G = g / (1.0 + g);
			G2 = G * G;
			G3 = G2 * G;
			GAMMA = G2 * G2;
		}

		double g_plus_1 = g + 1.0;

		double S1 = z1 / g_plus_1;
		double S2 = z2 / g_plus_1;
		double S3 = z3 / g_plus_1;
		double S4 = z4 / g_plus_1;

		double S = (G3 * S1) + (G2 * S2) + (G * S3) + S4;
		double u = (p->in[n] - k *  S) / (1 + k * GAMMA);

		// 1st stage
		double v = (u - z1) * G;
		double lp = v + z1;
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
	p->last_res = last_res;
	p->last_k = k;
	p->last_g = g;
	p->last_G = G;
	p->last_G2 = G2;
	p->last_G3 = G3;
	p->last_GAMMA = GAMMA;

	return OK;
}


static int diode_ladder_init(CSOUND* csound,
	DIODE_LADDER* p) {

	if (*p->skip == 0) {
		int i;
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

static int diode_ladder_perf(CSOUND* csound,
	DIODE_LADDER* p) {

	double a1 = p->a[0];
	double a2 = p->a[1];
	double a3 = p->a[2];
	double a4 = p->a[3];
	double z1 = p->z[0];
	double z2 = p->z[1];
	double z3 = p->z[2];
	double z4 = p->z[3];
	double G1 = p->G[0];
	double G2 = p->G[1];
	double G3 = p->G[2];
	double G4 = p->G[3];
	double beta1 = p->beta[0];
	double beta2 = p->beta[1];
	double beta3 = p->beta[2];
	double beta4 = p->beta[3];
	double delta1 = p->delta[0];
	double delta2 = p->delta[1];
	double delta3 = p->delta[2];
	double epsilon1 = p->epsilon[0];
	double epsilon2 = p->epsilon[1];
	double epsilon3 = p->epsilon[2];
	double gamma1 = p->gamma[0];
	double gamma2 = p->gamma[1];
	double gamma3 = p->gamma[2];
	double SG1 = p->SG[0];
	double SG2 = p->SG[1];
	double SG3 = p->SG[2];
	double SG4 = p->SG[3];
	double GAMMA = p->GAMMA;
	double SIGMA = p->SIGMA;
	double alpha = p->last_alpha;
	double last_cut = p->last_cut;

	uint32_t offset = p->h.insdshead->ksmps_offset;
	uint32_t early = p->h.insdshead->ksmps_no_end;
	uint32_t n, nsmps = CS_KSMPS;

	double T = csound->onedsr;
	double Tdiv2 = T / 2.0;
	double two_div_T = 2.0 / T;

	int cutoff_arate = IS_ASIG_ARG(p->cutoff);
	int k_arate = IS_ASIG_ARG(p->kval);

	MYFLT cutoff = cutoff_arate ? 0.0 : *p->cutoff;
	MYFLT k = k_arate ? 0.0 : *p->kval;

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

			double wd = TWOPI * cutoff;
			double wa = two_div_T * tan(wd * Tdiv2);
			double g = wa * Tdiv2;
			double gp1 = 1.0 + g;
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
		double fb4 = beta4 * z4;
		double fb3 = beta3 * (z3 + fb4 * delta3);
		double fb2 = beta2 * (z2 + fb3 * delta2);

		//feedback process
		double fbo1 = (beta1 * (z1 + fb2 * delta1));
		double fbo2 = (beta2 * (z2 + fb3 * delta2));
		double fbo3 = (beta3 * (z3 + fb4 * delta3));

		SIGMA = (SG1 * fbo1) + (SG2 * fbo2) + (SG3 * fbo3) + (SG4 * fb4);

		// non-linear processing
		if (*p->nlp == 1.0) {
			in = (1.0 / tanh(*p->saturation)) * tanh(*p->saturation * in);
		}
		else if (*p->nlp == 2.0) {
			in = tanh(*p->saturation * in);
		}

		// form input to loop
		double un = (in - k * SIGMA) / (1.0 + k * GAMMA);

		// 1st stage
		double xin = un * gamma1 + fb2 + epsilon1 * fbo1;
		double v = (a1 * xin - z1) * alpha;
		double lp = v + z1;
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



static OENTRY wpfilters_localops[] =
{
  { "zdf_1pole", sizeof(ZDF_LADDER), 0,5,"aa","axo",(SUBR)zdf_1pole_init,NULL,(SUBR)zdf_1pole_perf},
  { "zdf_ladder", sizeof(ZDF_LADDER), 0,5,"a","axxo",(SUBR)zdf_ladder_init,NULL,(SUBR)zdf_ladder_perf},
  { "diode_ladder", sizeof(DIODE_LADDER), 0,5,"a","axxOPo",(SUBR)diode_ladder_init,NULL,(SUBR)diode_ladder_perf},
};

LINKAGE_BUILTIN(wpfilters_localops)


