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

#if !defined(__Shakers_h)
#define __Shakers_h

#define MAX_FREQS 5
#define NUM_INSTR 13

typedef struct NShakers {
    OPDS        h;
    MYFLT       *ar;                  /* Output */
    MYFLT       *inst;          /* Which shaking type */
    MYFLT       *amp;           /* How loud */
    MYFLT       *dettack;       /* How loud */
    MYFLT       *mod_num_objects;

    int instType;
    int ratchetPos, lastRatchetPos;
    MYFLT shakeEnergy;
    MYFLT inputs[MAX_FREQS];
    MYFLT outputs[MAX_FREQS][2];
    MYFLT coeffs[MAX_FREQS][2];
    MYFLT sndLevel;
    MYFLT baseGain;
    MYFLT gains[MAX_FREQS];
    int num_freqs;
    MYFLT t_center_freqs[MAX_FREQS];
    MYFLT center_freqs[MAX_FREQS];
    MYFLT resons[MAX_FREQS];
    MYFLT freq_rand[MAX_FREQS];
    int freqalloc[MAX_FREQS];
    MYFLT soundDecay;
    MYFLT systemDecay;
    MYFLT num_objects;
/*    MYFLT collLikely; */
    MYFLT totalEnergy;
    MYFLT ratchet,ratchetDelta;
    MYFLT finalZ[3];
    MYFLT finalZCoeffs[3];
    MYFLT defObjs;
    MYFLT defDecays;
    MYFLT decayScale;
    int         kloop;
} NSHAKER ;

void Shakers_setFreqAndReson(NSHAKER *p, int which, MYFLT freq, MYFLT reson);

#endif
