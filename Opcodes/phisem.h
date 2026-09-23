/*
    phisem.h:

    Copyright (C) 1997, 2000 Perry Cook, John ffitch

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

/**********************************************************/
/*  PhISEM (Physically Informed Stochastic Event Modeling */
/*    by Perry R. Cook, Princeton, February 1997          */
/*                                                        */
/*  Meta-model that simulates all of:                     */
/*  Maraca Simulation by Perry R. Cook, Princeton, 1996-7 */
/*  Sekere Simulation by Perry R. Cook, Princeton, 1996-7 */
/*  Cabasa Simulation by Perry R. Cook, Princeton, 1996-7 */
/*  Bamboo Windchime Simulation, by Perry R. Cook, 1996-7 */
/*  Water Drops Simulation, by Perry R. Cook, 1996-7      */
/*  Tambourine Simulation, by Perry R. Cook, 1996-7       */
/*  Sleighbells Simulation, by Perry R. Cook, 1996-7      */
/*  Guiro Simulation, by Perry R. Cook, 1996-7            */
/*                                                        */
/**********************************************************/
/*  PhOLIES (Physically-Oriented Library of               */
/*    Imitated Environmental Sounds), Perry Cook, 1997-9  */
/*  Stix1 (walking on brittle sticks)                     */
/*  Crunch1 (like new fallen snow, or not)                */
/*  Wrench (basic socket wrench, friend of guiro)         */
/*  Sandpapr (sandpaper)                                  */
/**********************************************************/

#if !defined(__Phisem_h)
#define __Phisem_h

typedef struct Cabasa {
    OPDS        h;
    cs_float       *ar;            /* Output */
    cs_float       *amp;           /* How loud */
    cs_float       *dettack;       /* When to switch off */
    cs_float       *num_beads;     /* Number of beads */
    cs_float       *damp;
    cs_float       *shake_max;

    cs_float       shake_maxSave;
    cs_float       shakeEnergy;
    cs_float       outputs0;
    cs_float       outputs1;
    cs_float       coeffs0;
    cs_float       coeffs1;
    cs_float       sndLevel;
    cs_float       gain;
    cs_float       resons;
    cs_float       soundDecay;
    cs_float       systemDecay;
    int32       num_objects;
    cs_float       last_num;
    cs_float       totalEnergy;
    int32_t     kloop;
} CABASA;

typedef struct Sekere {
    OPDS        h;
    cs_float       *ar;            /* Output */
    cs_float       *amp;           /* How loud */
    cs_float       *dettack;       /* How loud */
    cs_float       *num_beads;     /* Number of beads */
    cs_float       *damp;
    cs_float       *shake_max;

    cs_float       shake_maxSave;
    cs_float       shakeEnergy;
    cs_float       outputs0;
    cs_float       outputs1;
    cs_float       coeffs0;
    cs_float       coeffs1;
    cs_float       sndLevel;
    cs_float       gain;
    cs_float       resons;
    cs_float       soundDecay;
    cs_float       systemDecay;
    cs_float       num_objects;
    cs_float       last_num;
    cs_float       totalEnergy;
    cs_float       finalZ0;
    cs_float       finalZ1;
    cs_float       finalZ2;
    int32_t         kloop;
} SEKERE;

typedef struct Guiro {
    OPDS        h;
    cs_float       *ar;            /* Output */
    cs_float       *amp;
    cs_float       *dettack;
    cs_float       *num_teeth;
    cs_float       *damp;
    cs_float       *shake_max;
    cs_float       *freq;
    cs_float       *freq2;

    cs_float       res_freqSave;
    cs_float       shake_damp;
    cs_float       shake_maxSave;
    cs_float       res_freq2;

    cs_float       shakeEnergy;
    cs_float       outputs00;
    cs_float       outputs01;
    cs_float       outputs10;
    cs_float       outputs11;
    cs_float       coeffs00;
    cs_float       coeffs01;
    cs_float       coeffs10;
    cs_float       coeffs11;
    cs_float       sndLevel;
    cs_float       baseGain;
    cs_float       gains0;
    cs_float       gains1;
    cs_float       soundDecay;
    cs_float       systemDecay;
    cs_float       num_objects;
    cs_float       totalEnergy;
    cs_float       ratchet,ratchetDelta;
    int32_t         ratchetPos;
    cs_float       finalZ0;
    cs_float       finalZ1;
    cs_float       finalZ2;
    cs_float       decayScale;
    cs_double          kloop;
} GUIRO;

typedef struct Tambour {
    OPDS        h;
    cs_float       *ar;            /* Output */
    cs_float       *amp;           /* How loud */
    cs_float       *dettack;       /* How loud */
    cs_float       *num_timbrels;
    cs_float       *damp;
    cs_float       *shake_max;
    cs_float       *freq;
    cs_float       *freq1;
    cs_float       *freq2;

    cs_float       num_objectsSave;
    cs_float       shake_maxSave;
    cs_float       shakeEnergy;
    cs_float       outputs00;
    cs_float       outputs01;
    cs_float       outputs10;
    cs_float       outputs11;
    cs_float       outputs20;
    cs_float       outputs21;
    cs_float       coeffs00;
    cs_float       coeffs01;
    cs_float       coeffs10;
    cs_float       coeffs11;
    cs_float       coeffs20;
    cs_float       coeffs21;
    cs_float       sndLevel;
    cs_float       gain;
    cs_float       gains0;
    cs_float       gains1;
    cs_float       gains2;
    cs_float       resons;
    cs_float       soundDecay;
    cs_float       systemDecay;
    cs_float       num_objects;
    cs_float       totalEnergy;
    cs_float       finalZ0;
    cs_float       finalZ1;
    cs_float       finalZ2;
    cs_float       decayScale;
    cs_float       res_freq;
    cs_float       res_freq1;
    cs_float       res_freq2;
    cs_float       shake_damp;
    cs_double          kloop;
} TAMBOURINE;

typedef struct Bamboo {
    OPDS        h;
    cs_float       *ar;            /* Output */
    cs_float       *amp;           /* How loud */
    cs_float       *dettack;       /* How loud */
    cs_float       *num_tubes;
    cs_float       *damp;
    cs_float       *shake_max;
    cs_float       *freq;
    cs_float       *freq1;
    cs_float       *freq2;

    cs_float       num_objectsSave;
    cs_float       shake_maxSave;
    cs_float       shakeEnergy;
    cs_float       outputs00;
    cs_float       outputs01;
    cs_float       outputs10;
    cs_float       outputs11;
    cs_float       outputs20;
    cs_float       outputs21;
    cs_float       coeffs00;
    cs_float       coeffs01;
    cs_float       coeffs10;
    cs_float       coeffs11;
    cs_float       coeffs20;
    cs_float       coeffs21;
    cs_float       sndLevel;
    cs_float       gain;
    cs_float       resons;
    cs_float       soundDecay;
    cs_float       systemDecay;
    cs_float       num_objects;
    cs_float       totalEnergy;
    cs_float       decayScale;
    cs_float       res_freq0;
    cs_float       res_freq1;
    cs_float       res_freq2;
    cs_float       shake_damp;
    int32_t         kloop;
} BAMBOO;

typedef struct Wuter {
    OPDS        h;
    cs_float       *ar;            /* Output */
    cs_float       *amp;           /* How loud */
    cs_float       *dettack;       /* How loud */
    cs_float       *num_tubes;
    cs_float       *damp;
    cs_float       *shake_max;
    cs_float       *freq;
    cs_float       *freq1;
    cs_float       *freq2;

    cs_float       num_objectsSave;
    cs_float       shake_maxSave;
    cs_float       shakeEnergy;
    cs_float       outputs00;
    cs_float       outputs01;
    cs_float       outputs10;
    cs_float       outputs11;
    cs_float       outputs20;
    cs_float       outputs21;
    cs_float       coeffs00;
    cs_float       coeffs01;
    cs_float       coeffs10;
    cs_float       coeffs11;
    cs_float       coeffs20;
    cs_float       coeffs21;
    cs_float       finalZ0;
    cs_float       finalZ1;
    cs_float       finalZ2;
    cs_float       sndLevel;
    cs_float       gains0;
    cs_float       gains1;
    cs_float       gains2;
    cs_float       center_freqs0;
    cs_float       center_freqs1;
    cs_float       center_freqs2;
    cs_float       soundDecay;
    cs_float       systemDecay;
    cs_float       num_objects;
    cs_float       totalEnergy;
    cs_float       decayScale;
    cs_float       res_freq0;
    cs_float       res_freq1;
    cs_float       res_freq2;
    cs_float       shake_damp;
    int32_t         kloop;
} WUTER;

typedef struct Sleighbells {
    OPDS        h;
    cs_float       *ar;            /* Output */
    cs_float       *amp;           /* How loud */
    cs_float       *dettack;       /* How loud */
    cs_float       *num_bells;
    cs_float       *damp;
    cs_float       *shake_max;
    cs_float       *freq;
    cs_float       *freq1;
    cs_float       *freq2;

    cs_float       num_objectsSave;
    cs_float       shake_maxSave;
    cs_float       shakeEnergy;
    cs_float       outputs00;
    cs_float       outputs01;
    cs_float       outputs10;
    cs_float       outputs11;
    cs_float       outputs20;
    cs_float       outputs21;
    cs_float       outputs30;
    cs_float       outputs31;
    cs_float       outputs40;
    cs_float       outputs41;
    cs_float       coeffs00;
    cs_float       coeffs01;
    cs_float       coeffs10;
    cs_float       coeffs11;
    cs_float       coeffs20;
    cs_float       coeffs21;
    cs_float       coeffs30;
    cs_float       coeffs31;
    cs_float       coeffs40;
    cs_float       coeffs41;
    cs_float       finalZ0;
    cs_float       finalZ1;
    cs_float       finalZ2;
    cs_float       sndLevel;
    cs_float       gain;
    cs_float       soundDecay;
    cs_float       systemDecay;
    cs_float       num_objects;
    cs_float       totalEnergy;
    cs_float       decayScale;
    cs_float       res_freq0;
    cs_float       res_freq1;
    cs_float       res_freq2;
    cs_float       res_freq3;
    cs_float       res_freq4;
    cs_float       shake_damp;
    cs_double      kloop;
} SLEIGHBELLS;
#endif
