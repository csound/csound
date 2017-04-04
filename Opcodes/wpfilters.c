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


static int zdf_ladder_init(CSOUND* csound,
	ZDF_LADDER* p) {

	if (*p->skip != 0) {
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

static int zdf_ladder_perf(CSOUND* csound,
	ZDF_LADDER* p) {

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

		if(cutoff_arate) {
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



static OENTRY wpfilters_localops[] =
{
  { "zdf_ladder", sizeof(ZDF_LADDER), 0,5,"a","axxo",(SUBR)zdf_ladder_init,NULL,(SUBR)zdf_ladder_perf},
};

LINKAGE_BUILTIN(wpfilters_localops)


