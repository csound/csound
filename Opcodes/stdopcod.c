/*
    stdopcod.c:

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

extern int ambicode_init_(CSOUND *);
extern int babo_init_(CSOUND *);
extern int bbcut_init_(CSOUND *);
extern int biquad_init_(CSOUND *);
extern int butter_init_(CSOUND *);
extern int clfilt_init_(CSOUND *);
extern int cross2_init_(CSOUND *);
extern int dam_init_(CSOUND *);
extern int dcblockr_init_(CSOUND *);
extern int filter_init_(CSOUND *);
extern int flanger_init_(CSOUND *);
extern int follow_init_(CSOUND *);
extern int fout_init_(CSOUND *);
extern int freeverb_init_(CSOUND *);
extern int ftconv_init_(CSOUND *);
extern int ftgen_init_(CSOUND *);
extern int gab_gab_init_(CSOUND *);
extern int gab_vectorial_init_(CSOUND *);
extern int grain4_init_(CSOUND *);
extern int grain_init_(CSOUND *);
extern int hrtferX_init_(CSOUND *);
extern int ifd_init_(CSOUND *);
extern int locsig_init_(CSOUND *);
extern int lowpassr_init_(CSOUND *);
extern int metro_init_(CSOUND *);
extern int midiops2_init_(CSOUND *);
extern int midiops3_init_(CSOUND *);
extern int newfils_init_(CSOUND *);
extern int nlfilt_init_(CSOUND *);
extern int oscbnk_init_(CSOUND *);
extern int partials_init_(CSOUND *);
extern int phisem_init_(CSOUND *);
extern int pluck_init_(CSOUND *);
extern int psynth_init_(CSOUND *);
extern int pvsbasic_init_(CSOUND *);
extern int pvscent_init_(CSOUND *);
extern int pvsdemix_init_(CSOUND *);
extern int repluck_init_(CSOUND *);
extern int reverbsc_init_(CSOUND *);
extern int scansyn_init_(CSOUND *);
extern int scansynx_init_(CSOUND *);
extern int seqtime_init_(CSOUND *);
extern int sndloop_init_(CSOUND *);
extern int sndwarp_init_(CSOUND *);
extern int space_init_(CSOUND *);
extern int spat3d_init_(CSOUND *);
extern int syncgrain_init_(CSOUND *);
extern int ugens7_init_(CSOUND *);
extern int ugens9_init_(CSOUND *);
extern int ugensa_init_(CSOUND *);
extern int uggab_init_(CSOUND *);
extern int ugmoss_init_(CSOUND *);
extern int ugnorman_init_(CSOUND *);
extern int ugsc_init_(CSOUND *);
extern int wave_terrain_init_(CSOUND *);

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    (void) csound;
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    int     err = 0;

    err |= ambicode_init_(csound);
    err |= babo_init_(csound);
    err |= bbcut_init_(csound);
    err |= biquad_init_(csound);
    err |= butter_init_(csound);
    err |= clfilt_init_(csound);
    err |= cross2_init_(csound);
    err |= dam_init_(csound);
    err |= dcblockr_init_(csound);
    err |= filter_init_(csound);
    err |= flanger_init_(csound);
    err |= follow_init_(csound);
    err |= fout_init_(csound);
    err |= freeverb_init_(csound);
    err |= ftconv_init_(csound);
    err |= ftgen_init_(csound);
    err |= gab_gab_init_(csound);
    err |= gab_vectorial_init_(csound);
    err |= grain4_init_(csound);
    err |= grain_init_(csound);
    err |= hrtferX_init_(csound);
    err |= ifd_init_(csound);
    err |= locsig_init_(csound);
    err |= lowpassr_init_(csound);
    err |= metro_init_(csound);
    err |= midiops2_init_(csound);
    err |= midiops3_init_(csound);
    err |= newfils_init_(csound);
    err |= nlfilt_init_(csound);
    err |= oscbnk_init_(csound);
    err |= partials_init_(csound);
    err |= phisem_init_(csound);
    err |= pluck_init_(csound);
    err |= psynth_init_(csound);
    err |= pvsbasic_init_(csound);
    err |= pvscent_init_(csound);
    err |= pvsdemix_init_(csound);
    err |= repluck_init_(csound);
    err |= reverbsc_init_(csound);
    err |= scansyn_init_(csound);
    err |= scansynx_init_(csound);
    err |= seqtime_init_(csound);
    err |= sndloop_init_(csound);
    err |= sndwarp_init_(csound);
    err |= space_init_(csound);
    err |= spat3d_init_(csound);
    err |= syncgrain_init_(csound);
    err |= ugens7_init_(csound);
    err |= ugens9_init_(csound);
    err |= ugensa_init_(csound);
    err |= uggab_init_(csound);
    err |= ugmoss_init_(csound);
    err |= ugnorman_init_(csound);
    err |= ugsc_init_(csound);
    err |= wave_terrain_init_(csound);

    return (err ? CSOUND_ERROR : CSOUND_SUCCESS);
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}

