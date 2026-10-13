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
 *  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
 */

#ifndef DSSI4CS_H
#define DSSI4CS_H
#include "csdl.h"
#include <alsa/asoundlib.h>
#include <dssi.h>

#define DSSI4CS_CHANNELS 9
#define DSSI4CS_EVENTS 1024
#define DSSI4CS_INSTANCES 256

typedef struct {
    int64_t time;
    snd_seq_event_t event;
} DSSI_EVENT;

typedef struct DSSI_PLUGIN_ {
    const LADSPA_Descriptor *ladspa;
    const DSSI_Descriptor *dssi;
    void *library;
    LADSPA_Handle handle;
    LADSPA_Data *control;
    LADSPA_Data **audio;
    int *mapping;
    unsigned long *inputs, *outputs;
    unsigned long input_count, output_count;
    uint32_t capacity, render_ksmps;
    cs_float sample_rate;
    int active;
    OPDS *owner;
    int64_t last_block, rendered_until;
    DSSI_EVENT queue[DSSI4CS_EVENTS];
    unsigned int queued;
    snd_seq_event_t events[DSSI4CS_EVENTS + 16 * (128 + 3)];
    unsigned char notes[16][128];
    int panic;
    unsigned long event_count;
    unsigned int bank[16];
} DSSI_PLUGIN;

typedef struct {
    unsigned int count;
    DSSI_PLUGIN *plugins[DSSI4CS_INSTANCES];
} DSSI_HOST;

typedef struct {
    OPDS h;
    cs_float *result, *filename, *index, *verbose;
} DSSIINIT;

typedef struct {
    OPDS h;
    cs_float *id, *trigger;
    DSSI_PLUGIN *plugin;
} DSSIACTIVATE;

typedef struct {
    OPDS h;
    cs_float *out[DSSI4CS_CHANNELS];
    cs_float *id, *in[DSSI4CS_CHANNELS];
    DSSI_PLUGIN *plugin;
} DSSIAUDIO;

typedef struct {
    OPDS h;
    cs_float *id, *port, *value, *trigger;
    DSSI_PLUGIN *plugin;
    unsigned long index;
} DSSICTLS;

typedef struct {
    OPDS h;
    cs_float *value, *id, *port;
    DSSI_PLUGIN *plugin;
    unsigned long index;
} DSSIGET;

typedef struct {
    OPDS h;
    cs_float *trigger, *id, *note, *velocity, *duration, *channel;
    DSSI_PLUGIN *plugin;
} DSSINOTE;

typedef struct {
    OPDS h;
    cs_float *trigger, *id, *status, *channel, *data1, *data2, *offset;
    DSSI_PLUGIN *plugin;
} DSSIEVENT;

typedef struct {
    OPDS h;
    cs_float *trigger, *id, *channel, *parameter, *value, *offset;
    DSSI_PLUGIN *plugin;
} DSSINRPN;

typedef struct {
    OPDS h;
    cs_float *trigger, *id, *note, *velocity;
    DSSI_PLUGIN *plugin;
} DSSINOTEON;

typedef struct {
    OPDS h;
    cs_float *id, *bank, *program, *trigger;
    DSSI_PLUGIN *plugin;
} DSSIPROGRAM;

typedef struct {
    OPDS h;
    cs_float *id;
    STRINGDAT *key, *value;
} DSSICONFIGURE;

typedef struct {
    OPDS h;
    STRINGDAT *name;
    cs_float *bank, *program, *id, *index;
} DSSIPROGRAMINFO;

typedef struct { OPDS h; cs_float *id; } DSSIINFO;
typedef struct { OPDS h; } DSSILIST;
#endif
