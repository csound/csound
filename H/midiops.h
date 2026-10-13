/*
    midiops.h:

    Copyright (C) 1995 Barry Vercoe, Gabriel maldonado,
                       Istvan Varga, John ffitch

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
                                /*                      MIDIOPS.H       */
#ifndef MIDIOPS_H
#define MIDIOPS_H

#define NOTEOFF_TYPE  0x80
#define NOTEON_TYPE   0x90
#define POLYAFT_TYPE  0xA0
#define CONTROL_TYPE  0xB0
#define PROGRAM_TYPE  0xC0
#define AFTOUCH_TYPE  0xD0
#define PCHBEND_TYPE  0xE0
#define SYSTEM_TYPE   0xF0

#define DATENTRY        6
#define VOLUME          7
#define SUSTAIN_SW      64
#define NRPNLSB         98
#define NRPNMSB         99
#define RPNLSB          100
#define RPNMSB          101

#define VIB_RATE        128     /* ROLAND EXTENDED CTRLS */
#define VIB_DEPTH       129     /* in unused ctl_val spc */
#define VIB_DELAY       130
#define TVF_CUTOFF      131
#define TVF_RESON       132
#define TVA_RIS         133
#define TVA_DEC         134
#define TVA_RLS         135

typedef struct {
    OPDS      h;
    cs_float     *chnl;
    STRINGDAT *insno;
    cs_float     *iresetctls;
} MASSIGNS;

typedef struct {
    OPDS      h;
    cs_float     *chnl;
    cs_float     *insno;
    cs_float     *iresetctls;
} MASSIGN;

typedef struct {
    OPDS    h;
    cs_float   *chnl, *ctrls[64];
} CTLINIT;

typedef struct {
    OPDS    h;
    STRINGDAT  *iname;
    cs_float   *ctrls[64];
} CTLINITS;

typedef struct {
    OPDS    h;
    cs_float   *r, *imax, *ifn;
} MIDIAMP;

typedef struct {
    OPDS    h;
    cs_float   *r, *ictlno, *ilo, *ihi;
    int32   ctlno;
    cs_float   scale, lo;
} MIDICTL;

typedef struct {
    OPDS    h;
    cs_float   *r, *ichano, *ictlno, *ilo, *ihi;
    int32   chano, ctlno;
    cs_float   scale, lo;
} CHANCTL;

typedef struct {
    OPDS    h;
    cs_float   *r, *iscal;
    cs_float   scale, prvbend, prvout;
} MIDIKMB;

typedef struct {
    OPDS    h;
    cs_float   *r, *ilo, *ihi;
} MIDIMAP;

typedef struct {
    OPDS    h;
    cs_float   *r, *ilo, *ihi;
    cs_float   scale, lo;
} MIDIKMAP;

typedef struct {
    OPDS    h;
    cs_float   *olap;
} MIDIOLAP;

typedef struct {
    OPDS    h;
    cs_float   *r;
} MIDIAGE;

typedef struct {
    OPDS    h;
    cs_float   *r, *tablenum;
    /* *numgrades, *interval, *basefreq, *basekeymidi; */
} CPSTABLE;

typedef struct {
    OPDS    h;
    cs_float   *ans;
} GTEMPO;

typedef struct {
    OPDS    h;
    cs_float   *ichn;
} MIDICHN;

typedef struct {
    OPDS    h;
    cs_float   *ipgm, *inst, *ichn;
} PGMASSIGN;

typedef struct {
    OPDS    h;
    cs_float   *status, *chan, *data1, *data2;
    int32_t     local_buf_index;        /* IV - Nov 30 2002 */
} MIDIIN;

typedef struct {
    OPDS    h;
    cs_float   *pgm, *chn, *ochan;
    int32_t     local_buf_index;        /* IV - Nov 30 2002 */
    int32_t     watch;
} PGMIN;

typedef struct {
    OPDS    h;
    cs_float   *data, *numb, *chn, *ochan, *onum;
    int32_t     local_buf_index;        /* IV - Nov 30 2002 */
    int32_t     watch1, watch2;
} CTLIN;

typedef struct {
  OPDS    h;
  cs_float   *noteOut, *counter;
  cs_float   *arpRate, *arpMode;
  cs_double  curphs;
  int32_t flag;
  int32_t noteCnt, noteIndex, direction;
  int32_t notes[10];
  int32_t channels[10];
  int32_t     local_buf_index;
} MIDIARP;

typedef struct {
  OPDS    h;
  ARRAYDAT *arr;
  cs_float   *chnl, *ctrls[64];
  cs_float   *ivals;
  int16   nargs;
} SAVECTRL;

typedef struct {
  OPDS    h;
  ARRAYDAT *arr;
  STRINGDAT *file;
  FILE    *fout;
  FDCH    fdch;
} PRINTCTRL;

typedef struct {
  int32_t           max_num;
  int32_t           **presets;
} PRESET_GLOB;

typedef struct {
  OPDS    h;
  cs_float   *inum;
  cs_float   *itag;
  cs_float   *chnl, *ctrls[64];
  cs_float   *ivals;
  int16   nargs;
  PRESET_GLOB *q;
} PRESETCTRL;

typedef struct {
  OPDS    h;
  cs_float   *inum;
  cs_float   *itag;
  ARRAYDAT *arr;
  PRESET_GLOB *q;
} PRESETCTRL1;

typedef struct {
  OPDS    h;
  cs_float   *inum;
  PRESET_GLOB *q;
} SELECTCTRL;

typedef struct {
  OPDS    h;
  STRINGDAT *file;
  FILE    *fout;
  FDCH    fdch;
} PRINTPRESETS;

int32_t event_type(CSOUND *csound, void *p);
int32_t midi_clock_in(CSOUND *csound, void *p);
int32_t midi_stop(CSOUND *csound, void *p);
int32_t midi_start(CSOUND *csound, void *p);
int32_t midi_continue(CSOUND *csound, void *p);
int32_t midi_clock_freq(CSOUND *csound, void *p);
#endif
