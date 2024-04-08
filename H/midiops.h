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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
    MYFLT     *chnl;
    STRINGDAT *insno;
    MYFLT     *iresetctls;
} MASSIGNS;

typedef struct {
    OPDS      h;
    MYFLT     *chnl;
    MYFLT     *insno;
    MYFLT     *iresetctls;
} MASSIGN;

typedef struct {
    OPDS    h;
    MYFLT   *chnl, *ctrls[64];
} CTLINIT;

typedef struct {
    OPDS    h;
    STRINGDAT  *iname;
    MYFLT   *ctrls[64];
} CTLINITS;

typedef struct {
    OPDS    h;
    MYFLT   *r, *imax, *ifn;
} MIDIAMP;

typedef struct {
    OPDS    h;
    MYFLT   *r, *ictlno, *ilo, *ihi;
    int32   ctlno;
    MYFLT   scale, lo;
} MIDICTL;

typedef struct {
    OPDS    h;
    MYFLT   *r, *ichano, *ictlno, *ilo, *ihi;
    int32   chano, ctlno;
    MYFLT   scale, lo;
} CHANCTL;

typedef struct {
    OPDS    h;
    MYFLT   *r, *iscal;
    MYFLT   scale, prvbend, prvout;
} MIDIKMB;

typedef struct {
    OPDS    h;
    MYFLT   *r, *ilo, *ihi;
} MIDIMAP;

typedef struct {
    OPDS    h;
    MYFLT   *r, *ilo, *ihi;
    MYFLT   scale, lo;
} MIDIKMAP;

typedef struct {
    OPDS    h;
    MYFLT   *olap;
} MIDIOLAP;

typedef struct {
    OPDS    h;
    MYFLT   *r;
} MIDIAGE;

typedef struct {
    OPDS    h;
    MYFLT   *r, *tablenum;
    /* *numgrades, *interval, *basefreq, *basekeymidi; */
} CPSTABLE;

typedef struct {
    OPDS    h;
    MYFLT   *ans;
} GTEMPO;

typedef struct {
    OPDS    h;
    MYFLT   *ichn;
} MIDICHN;

typedef struct {
    OPDS    h;
    MYFLT   *ipgm, *inst, *ichn;
} PGMASSIGN;

typedef struct {
    OPDS    h;
    MYFLT   *status, *chan, *data1, *data2;
    int     local_buf_index;        /* IV - Nov 30 2002 */
} MIDIIN;

typedef struct {
    OPDS    h;
    MYFLT   *pgm, *chn, *ochan;
    int     local_buf_index;        /* IV - Nov 30 2002 */
    int     watch;
} PGMIN;

typedef struct {
    OPDS    h;
    MYFLT   *data, *numb, *chn, *ochan, *onum;
    int     local_buf_index;        /* IV - Nov 30 2002 */
    int     watch1, watch2;
} CTLIN;

typedef struct {
  OPDS    h;
  MYFLT   *noteOut, *counter;
  MYFLT   *arpRate, *arpMode;

  int metroTick;
  double  curphs;
  int flag;
  int status, chan, data1, data2,
    noteCnt, noteIndex, maxNumNotes,
    direction;
  int notes[10];
  int sortedNotes[10];
  int     local_buf_index;
} MIDIARP;

typedef struct {
  OPDS    h;
  ARRAYDAT *arr;
  MYFLT   *chnl, *ctrls[64];
  MYFLT   *ivals;
  int16   nargs;
} SAVECTRL;

typedef struct {
  OPDS    h;
  ARRAYDAT *arr;
  STRINGDAT *file;
  FILE    *fout;
} PRINTCTRL;

typedef struct {
  int           max_num;
  int           **presets;
} PRESET_GLOB;

typedef struct {
  OPDS    h;
  MYFLT   *inum;
  MYFLT   *itag;
  MYFLT   *chnl, *ctrls[64];
  MYFLT   *ivals;
  int16   nargs;
  PRESET_GLOB *q;
} PRESETCTRL;

typedef struct {
  OPDS    h;
  MYFLT   *inum;
  MYFLT   *itag;
  ARRAYDAT *arr;
  PRESET_GLOB *q;
} PRESETCTRL1;

typedef struct {
  OPDS    h;
  MYFLT   *inum;
  PRESET_GLOB *q;
} SELECTCTRL;

typedef struct {
  OPDS    h;
  STRINGDAT *file;
  FILE    *fout;
} PRINTPRESETS;
#endif
