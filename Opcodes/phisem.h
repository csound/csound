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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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
    MYFLT       *ar;            /* Output */
    MYFLT       *amp;           /* How loud */
    MYFLT       *dettack;       /* When to switch off */
    MYFLT       *num_beads;     /* Number of beads */
    MYFLT       *damp;
    MYFLT       *shake_max;

    MYFLT       shake_maxSave;
    MYFLT       shakeEnergy;
    MYFLT       outputs0;
    MYFLT       outputs1;
    MYFLT       coeffs0;
    MYFLT       coeffs1;
    MYFLT       sndLevel;
    MYFLT       gain;
    MYFLT       resons;
    MYFLT       soundDecay;
    MYFLT       systemDecay;
    int32       num_objects;
    MYFLT       last_num;
    MYFLT       totalEnergy;
    int         kloop;
} CABASA;

typedef struct Sekere {
    OPDS        h;
    MYFLT       *ar;            /* Output */
    MYFLT       *amp;           /* How loud */
    MYFLT       *dettack;       /* How loud */
    MYFLT       *num_beads;     /* Number of beads */
    MYFLT       *damp;
    MYFLT       *shake_max;

    MYFLT       shake_maxSave;
    MYFLT       shakeEnergy;
    MYFLT       outputs0;
    MYFLT       outputs1;
    MYFLT       coeffs0;
    MYFLT       coeffs1;
    MYFLT       sndLevel;
    MYFLT       gain;
    MYFLT       resons;
    MYFLT       soundDecay;
    MYFLT       systemDecay;
    MYFLT       num_objects;
    MYFLT       last_num;
    MYFLT       totalEnergy;
    MYFLT       finalZ0;
    MYFLT       finalZ1;
    MYFLT       finalZ2;
    int         kloop;
} SEKERE;

typedef struct Guiro {
    OPDS        h;
    MYFLT       *ar;            /* Output */
    MYFLT       *amp;
    MYFLT       *dettack;
    MYFLT       *num_teeth;
    MYFLT       *damp;
    MYFLT       *shake_max;
    MYFLT       *freq;
    MYFLT       *freq2;

    MYFLT       res_freqSave;
    MYFLT       shake_damp;
    MYFLT       shake_maxSave;
    MYFLT       res_freq2;

    MYFLT       shakeEnergy;
    MYFLT       outputs00;
    MYFLT       outputs01;
    MYFLT       outputs10;
    MYFLT       outputs11;
    MYFLT       coeffs00;
    MYFLT       coeffs01;
    MYFLT       coeffs10;
    MYFLT       coeffs11;
    MYFLT       sndLevel;
    MYFLT       baseGain;
    MYFLT       gains0;
    MYFLT       gains1;
    MYFLT       soundDecay;
    MYFLT       systemDecay;
    MYFLT       num_objects;
    MYFLT       totalEnergy;
    MYFLT       ratchet,ratchetDelta;
    int         ratchetPos;
    MYFLT       finalZ0;
    MYFLT       finalZ1;
    MYFLT       finalZ2;
    MYFLT       decayScale;
    int         kloop;
} GUIRO;

typedef struct Tambour {
    OPDS        h;
    MYFLT       *ar;            /* Output */
    MYFLT       *amp;           /* How loud */
    MYFLT       *dettack;       /* How loud */
    MYFLT       *num_timbrels;
    MYFLT       *damp;
    MYFLT       *shake_max;
    MYFLT       *freq;
    MYFLT       *freq1;
    MYFLT       *freq2;

    MYFLT       num_objectsSave;
    MYFLT       shake_maxSave;
    MYFLT       shakeEnergy;
    MYFLT       outputs00;
    MYFLT       outputs01;
    MYFLT       outputs10;
    MYFLT       outputs11;
    MYFLT       outputs20;
    MYFLT       outputs21;
    MYFLT       coeffs00;
    MYFLT       coeffs01;
    MYFLT       coeffs10;
    MYFLT       coeffs11;
    MYFLT       coeffs20;
    MYFLT       coeffs21;
    MYFLT       sndLevel;
    MYFLT       gain;
    MYFLT       gains0;
    MYFLT       gains1;
    MYFLT       gains2;
    MYFLT       resons;
    MYFLT       soundDecay;
    MYFLT       systemDecay;
    MYFLT       num_objects;
    MYFLT       totalEnergy;
    MYFLT       finalZ0;
    MYFLT       finalZ1;
    MYFLT       finalZ2;
    MYFLT       decayScale;
    MYFLT       res_freq;
    MYFLT       res_freq1;
    MYFLT       res_freq2;
    MYFLT       shake_damp;
    int         kloop;
} TAMBOURINE;

typedef struct Bamboo {
    OPDS        h;
    MYFLT       *ar;            /* Output */
    MYFLT       *amp;           /* How loud */
    MYFLT       *dettack;       /* How loud */
    MYFLT       *num_tubes;
    MYFLT       *damp;
    MYFLT       *shake_max;
    MYFLT       *freq;
    MYFLT       *freq1;
    MYFLT       *freq2;

    MYFLT       num_objectsSave;
    MYFLT       shake_maxSave;
    MYFLT       shakeEnergy;
    MYFLT       outputs00;
    MYFLT       outputs01;
    MYFLT       outputs10;
    MYFLT       outputs11;
    MYFLT       outputs20;
    MYFLT       outputs21;
    MYFLT       coeffs00;
    MYFLT       coeffs01;
    MYFLT       coeffs10;
    MYFLT       coeffs11;
    MYFLT       coeffs20;
    MYFLT       coeffs21;
    MYFLT       sndLevel;
    MYFLT       gain;
    MYFLT       resons;
    MYFLT       soundDecay;
    MYFLT       systemDecay;
    MYFLT       num_objects;
    MYFLT       totalEnergy;
    MYFLT       decayScale;
    MYFLT       res_freq0;
    MYFLT       res_freq1;
    MYFLT       res_freq2;
    MYFLT       shake_damp;
    int         kloop;
} BAMBOO;

typedef struct Wuter {
    OPDS        h;
    MYFLT       *ar;            /* Output */
    MYFLT       *amp;           /* How loud */
    MYFLT       *dettack;       /* How loud */
    MYFLT       *num_tubes;
    MYFLT       *damp;
    MYFLT       *shake_max;
    MYFLT       *freq;
    MYFLT       *freq1;
    MYFLT       *freq2;

    MYFLT       num_objectsSave;
    MYFLT       shake_maxSave;
    MYFLT       shakeEnergy;
    MYFLT       outputs00;
    MYFLT       outputs01;
    MYFLT       outputs10;
    MYFLT       outputs11;
    MYFLT       outputs20;
    MYFLT       outputs21;
    MYFLT       coeffs00;
    MYFLT       coeffs01;
    MYFLT       coeffs10;
    MYFLT       coeffs11;
    MYFLT       coeffs20;
    MYFLT       coeffs21;
    MYFLT       finalZ0;
    MYFLT       finalZ1;
    MYFLT       finalZ2;
    MYFLT       sndLevel;
    MYFLT       gains0;
    MYFLT       gains1;
    MYFLT       gains2;
    MYFLT       center_freqs0;
    MYFLT       center_freqs1;
    MYFLT       center_freqs2;
    MYFLT       soundDecay;
    MYFLT       systemDecay;
    MYFLT       num_objects;
    MYFLT       totalEnergy;
    MYFLT       decayScale;
    MYFLT       res_freq0;
    MYFLT       res_freq1;
    MYFLT       res_freq2;
    MYFLT       shake_damp;
    int         kloop;
} WUTER;

typedef struct Sleighbells {
    OPDS        h;
    MYFLT       *ar;            /* Output */
    MYFLT       *amp;           /* How loud */
    MYFLT       *dettack;       /* How loud */
    MYFLT       *num_bells;
    MYFLT       *damp;
    MYFLT       *shake_max;
    MYFLT       *freq;
    MYFLT       *freq1;
    MYFLT       *freq2;

    MYFLT       num_objectsSave;
    MYFLT       shake_maxSave;
    MYFLT       shakeEnergy;
    MYFLT       outputs00;
    MYFLT       outputs01;
    MYFLT       outputs10;
    MYFLT       outputs11;
    MYFLT       outputs20;
    MYFLT       outputs21;
    MYFLT       outputs30;
    MYFLT       outputs31;
    MYFLT       outputs40;
    MYFLT       outputs41;
    MYFLT       coeffs00;
    MYFLT       coeffs01;
    MYFLT       coeffs10;
    MYFLT       coeffs11;
    MYFLT       coeffs20;
    MYFLT       coeffs21;
    MYFLT       coeffs30;
    MYFLT       coeffs31;
    MYFLT       coeffs40;
    MYFLT       coeffs41;
    MYFLT       finalZ0;
    MYFLT       finalZ1;
    MYFLT       finalZ2;
    MYFLT       sndLevel;
    MYFLT       gain;
    MYFLT       soundDecay;
    MYFLT       systemDecay;
    MYFLT       num_objects;
    MYFLT       totalEnergy;
    MYFLT       decayScale;
    MYFLT       res_freq0;
    MYFLT       res_freq1;
    MYFLT       res_freq2;
    MYFLT       res_freq3;
    MYFLT       res_freq4;
    MYFLT       shake_damp;
    int         kloop;
} SLEIGHBELLS;
#endif
