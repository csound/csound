/*
 *  Copyright (C) 2005 Andres Cabrera
 *  The dssi4cs library is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  The dssi4cs library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with The dssi4cs library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA
 */

#ifndef DSSI4CS_H
#define DSSI4CS_H

#include "csdl.h"
#include "dssi.h"

/* When changing these remember to change dssiaudio function */
#define DSSI4CS_MAX_IN_CHANNELS 9
#define DSSI4CS_MAX_OUT_CHANNELS 9

enum PluginType {LADSPA, DSSI};

typedef struct DSSI4CS_PLUGIN_ {
    const LADSPA_Descriptor * Descriptor;
    const DSSI_Descriptor * DSSIDescriptor;
    /* For Type 1=LADSPA 2=DSSI */
    enum PluginType Type;
    LADSPA_Handle Handle;
    int32_t Active;
    LADSPA_Data ** control;
    LADSPA_Data ** audio;
    snd_seq_event_t *Events;
    uint64_t EventCount;
    int32_t PluginNumber;
    int32_t * PluginCount;
    void * NextPlugin;
    int32_t ksmps;
    /* float * kinputs_[]; */
    /* float * koutputs_[]; */
} DSSI4CS_PLUGIN;

typedef struct DSSIINIT_ {
    OPDS h;
    /* Inputs. */
    MYFLT *iDSSIHandle;
    MYFLT *iplugin;
    MYFLT *iindex;
    MYFLT *iverbose;
} DSSIINIT ;

typedef struct DSSIACTIVATE_ {
    OPDS h;
    MYFLT *iDSSIhandle;
    MYFLT *ktrigger;
    int32_t printflag;
    DSSI4CS_PLUGIN * DSSIPlugin_;
} DSSIACTIVATE ;

typedef struct DSSIAUDIO_ {
    OPDS h;
    /* Outputs. */
    MYFLT *aout[DSSI4CS_MAX_OUT_CHANNELS];
    /* Inputs. */
    MYFLT *iDSSIhandle;
    MYFLT *ain[DSSI4CS_MAX_IN_CHANNELS];
/*  MYFLT *ain1; */
/*  MYFLT *ain2; */
/*  MYFLT *ain3; */
/*  MYFLT *ain4; */
    int32_t NumInputPorts;
    int32_t NumOutputPorts;
    uint64_t * InputPorts;
    uint64_t * OutputPorts;
    DSSI4CS_PLUGIN * DSSIPlugin_;
    /* State. */
    /* size_t framesPerBlock; */
    /* size_t channels; */
} DSSIAUDIO ;

typedef struct DSSICTLS_ {
    OPDS h;
    MYFLT *iDSSIhandle;
    MYFLT *iport;
    MYFLT *val;
    MYFLT *ktrig;
    /* float *Data; */
    uint64_t PortNumber;
    int32_t HintSampleRate;
    DSSI4CS_PLUGIN * DSSIPlugin_;
} DSSICTLS;

typedef struct DSSISYNTH_ {
    OPDS h;
    MYFLT *aout[DSSI4CS_MAX_OUT_CHANNELS];
    /* Inputs. */
    MYFLT *iDSSIhandle;
    int32_t NumInputPorts;
    int32_t NumOutputPorts;
    uint64_t * InputPorts;
    uint64_t * OutputPorts;
    DSSI4CS_PLUGIN * DSSIPlugin_;
} DSSISYNTH;

typedef struct DSSINOTE_ {
    OPDS h;
    /* Inputs. */
    MYFLT *ktrigger;
    MYFLT *iDSSIhandle;
    MYFLT *knote;
    MYFLT *kveloc;
    MYFLT *kdur;
} DSSINOTE;

typedef struct DSSINOTEON_ {
    OPDS h;
    /* Inputs. */
    MYFLT *ktrigger;
    MYFLT *iDSSIhandle;
    MYFLT *knote;
    MYFLT *kveloc;
} DSSINOTEON;

typedef struct DSSINOTEOFF_ {
    OPDS h;
    /* Inputs. */
    MYFLT *ktrigger;
    MYFLT *iDSSIhandle;
    MYFLT *knote;
    MYFLT *kveloc;
} DSSINOTEOFF;

typedef struct DSSIPGMCH_ {
    OPDS h;
    /* Inputs. */
    MYFLT *ktrigger;
    MYFLT *iDSSIhandle;
    MYFLT *kprogram;
    MYFLT *kbank;
} DSSIPGMCH;

typedef struct DSSILIST_ {
    OPDS h;
} DSSILIST ;

#endif

