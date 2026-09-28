/*
    vbap.h:

    Copyright (C) 2000 Ville Pulkki

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

#pragma once

#define LOWEST_ACCEPTABLE_WT FL(0.0)
#define CHANNELS 128
#define MIN_VOL_P_SIDE_LGTH FL(0.01)

typedef struct {
  cs_float x;
  cs_float y;
  cs_float z;
} CART_VEC;

typedef struct {
  cs_float azi;
  cs_float ele;
  cs_float length;
} ANG_VEC;

/* A struct for gain factors */
typedef struct {
  cs_float wt1, wt2, wt3;
  cs_float *out_ptr1, *out_ptr2, *out_ptr3;
} OUT_WTS;

/* A struct for a loudspeaker triplet or pair (set) */
typedef struct {
  int32_t ls_nos[3];
  cs_float ls_mx[9];
  cs_float set_gains[3];
  cs_float smallest_wt;
  int32_t neg_g_am;
} LS_SET;

/* VBAP structure of n loudspeaker panning */
typedef struct {
  int32_t number;
  cs_float beg_gains[CHANNELS];
  cs_float curr_gains[CHANNELS];
  cs_float end_gains[CHANNELS];
  cs_float updated_gains[CHANNELS];
  int32_t dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int32_t ls_am;
  int32_t ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir;
} VBAP_DATA;

typedef struct {
  OPDS          h;                  /* required header */
  cs_float         *out_array[CHANNELS];
  cs_float         *audio, *azi, *ele, *spread, *layout;

  VBAP_DATA     q;
} VBAP;

typedef struct {
  OPDS          h;                  /* required header */
  ARRAYDAT      *tabout;
  cs_float         *audio, *azi, *ele, *spread, *layout;

  VBAP_DATA     q;
} VBAPA;

typedef struct {
  int32_t number;
  cs_float gains[CHANNELS];
  int32_t dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int32_t ls_am;
  int32_t ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir;
} VBAP1_DATA;

typedef struct {
  OPDS      h;                  /* required header */
  cs_float         *out_array[CHANNELS];
  cs_float         *azi, *ele, *spread, *layout;

  VBAP1_DATA    q;
} VBAP1;

typedef struct {
  OPDS      h;                  /* required header */
  ARRAYDAT      *tabout;
  cs_float         *azi, *ele, *spread, *layout;

  VBAP1_DATA    q;
} VBAPA1;

/* VBAP structure of loudspeaker moving panning */
typedef struct {
  cs_float gains[CHANNELS];
  int32_t number;
  int32_t upd_interval;
  int32_t dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int32_t ls_am;
  int32_t ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir, prev_ang_dir, next_ang_dir;
  int32_t point_change_interval, point_change_counter, curr_fld, next_fld;
  cs_float ele_vel;
  cs_float end_gains[CHANNELS];
} VBAP1_MOVE_DATA;

typedef struct {
  OPDS      h;                  /* required header */
  cs_float         *out_array[CHANNELS];
  cs_float         *dur, *spread, *field_am,
                *fld[VARGMAX-3]; /* field_am positive: point to point
                                           negative: angle velocities */
  VBAP1_MOVE_DATA q;
} VBAP1_MOVING;

typedef struct {
  OPDS           h;                  /* required header */
  ARRAYDAT      *tabout;
  cs_float         *dur, *spread, *field_am,
                *fld[VARGMAX-3]; /* field_am positive: point to point
                                           negative: angle velocities */
  VBAP1_MOVE_DATA q;
} VBAPA1_MOVING;

/* VBAP structure of loudspeaker moving panning */
typedef struct {
  cs_float beg_gains[CHANNELS];
  cs_float curr_gains[CHANNELS];
  cs_float updated_gains[CHANNELS];
  int32_t number;
  int32_t upd_interval;
  int32_t dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int32_t ls_am;
  int32_t ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir, prev_ang_dir, next_ang_dir;
  int32_t point_change_interval, point_change_counter, curr_fld, next_fld;
  cs_float ele_vel;
  cs_float end_gains[CHANNELS];
} VBAP_MOVE_DATA;

typedef struct {
  OPDS      h;                  /* required header */
  cs_float         *out_array[CHANNELS];
  cs_float         *audio, *dur, *spread, *field_am,
                *fld[VARGMAX-4]; /* field_am positive: point to point
                                           negative: angle velocities */
  VBAP_MOVE_DATA q;
} VBAP_MOVING;


typedef struct {
  OPDS          h;                  /* required header */
  ARRAYDAT      *tabout;
  cs_float         *audio, *dur, *spread, *field_am,
                *fld[VARGMAX-4]; /* field_am positive: point to point
                                           negative: angle velocities */
  VBAP_MOVE_DATA q;
} VBAPA_MOVING;

typedef struct {
  OPDS      h;                  /* required header */
  cs_float     *dim, *ls_amount;
  cs_float     *f[2*CHANNELS];
} VBAP_LS_INIT;

typedef struct {
  OPDS      h;                  /* required header */
  cs_float     *dim, *ls_amount;
  ARRAYDAT  *a;
} VBAP_LS_INITA;

/* A struct for a loudspeaker instance */
typedef struct {
  CART_VEC coords;
  ANG_VEC angles;
  int32_t channel_nbr;
} ls;

/* A struct for all loudspeakers */
typedef struct ls_triplet_chain {
  int32_t ls_nos[3];
  cs_float inv_mx[9];
  struct ls_triplet_chain *next;
} ls_triplet_chain;

/* functions */

void angle_to_cart_II( ANG_VEC *from,  CART_VEC *to);
int32_t lines_intersect(int32_t i,int32_t j,int32_t k,int32_t l, ls lss[]);
cs_float vec_angle(CART_VEC v1, CART_VEC v2);
void vec_mean(CART_VEC v1, CART_VEC v2, CART_VEC *v3);
cs_float angle_in_base(CART_VEC vb1,CART_VEC vb2,CART_VEC vec);
void cross_prod(CART_VEC v1,CART_VEC v2,
                CART_VEC *res) ;
/* void sort_angles(cs_float angles[], int32_t sorted_angles[], */
/*                  int32_t ls_amount); */
/* void remove_connections_in_planey(int32_t i,int32_t j,int32_t k,int32_t l, */
/*                                    ls  lss[CHANNELS], */
/*                                     int32_t connections[CHANNELS][CHANNELS]); */
int32_t calc_2D_inv_tmatrix(cs_float azi1,cs_float azi2, cs_float inv_mat[4]);

extern void cart_to_angle(CART_VEC cvec, ANG_VEC *avec);
extern void angle_to_cart(ANG_VEC avec, CART_VEC *cvec);
extern void normalize_wts(OUT_WTS *wts);

extern int32_t vbap_control(CSOUND*, VBAP_DATA *p, cs_float*, cs_float*, cs_float*);

void calc_vbap_gns(int32_t ls_set_am, int32_t dim, LS_SET *sets,
                   cs_float *gains, int32_t ls_amount,
                   CART_VEC cart_dir);
void scale_angles(ANG_VEC *avec);
cs_float vol_p_side_lgth(int32_t i, int32_t j, int32_t k, ls  lss[]);

void new_spread_dir(CART_VEC *spreaddir, CART_VEC vscartdir,
                    CART_VEC spread_base, cs_float azi, cs_float spread);
void new_spread_base(CART_VEC spreaddir, CART_VEC vscartdir,
                     cs_float spread, CART_VEC *spread_base);

/* VBAP structure for ZAK loudspeaker panning */
typedef struct {
  OPDS      h;                  /* required header */
  cs_float     *numb, *ndx, *audio, *azi, *ele, *spread, *layout;
  int32_t       n;
  cs_float     *out_array;
  AUXCH     auxch;
  AUXCH     aux;
  cs_float     *curr_gains;
  cs_float     *beg_gains;
  cs_float     *end_gains;
  cs_float     *updated_gains;
  int32_t       dim;
  LS_SET    *ls_sets;
  int32_t       ls_am;
  int32_t       ls_set_am;
  CART_VEC  cart_dir;
  CART_VEC  spread_base;
  ANG_VEC   ang_dir;
} VBAP_ZAK;

/* VBAP structure of ZAK loudspeaker moving panning */
typedef struct {
  OPDS      h;                  /* required header */
  cs_float     *numb, *ndx, *audio, *dur, *spread, *field_am,
            *fld[VARGMAX-6]; /* field_am positive: point to point
                                       negative: angle velocities */
  int32_t   n;
  cs_float     *out_array;
  AUXCH     auxch;
  AUXCH     aux;
  cs_float     *curr_gains;
  cs_float     *beg_gains;
  cs_float     *end_gains;
  cs_float     *updated_gains;
  int32_t   dim;
  LS_SET    *ls_sets;
  int32_t   ls_am;
  int32_t   ls_set_am;
  CART_VEC  cart_dir;
  CART_VEC  spread_base;
  ANG_VEC   ang_dir, prev_ang_dir, next_ang_dir;
  int32_t   point_change_interval, point_change_counter, curr_fld, next_fld;
  cs_float     ele_vel;
} VBAP_ZAK_MOVING;

int32_t     vbap_init(CSOUND *, VBAP *);
int32_t     vbap_init_a(CSOUND *, VBAPA *);
int32_t     vbap(CSOUND *, VBAP *);
int32_t     vbap_a(CSOUND *, VBAPA *);
int32_t     vbap_zak_init(CSOUND *, VBAP_ZAK *);
int32_t     vbap_zak(CSOUND *, VBAP_ZAK *);
int32_t     vbap_ls_init(CSOUND *, VBAP_LS_INIT *);
int32_t     vbap_moving_init(CSOUND *, VBAP_MOVING *);
int32_t     vbap_moving(CSOUND *, VBAP_MOVING *);
int32_t     vbap_moving_init_a(CSOUND *, VBAPA_MOVING *);
int32_t     vbap_moving_a(CSOUND *, VBAPA_MOVING *);
int32_t     vbap_zak_moving_init(CSOUND *, VBAP_ZAK_MOVING *);
int32_t     vbap_zak_moving(CSOUND *, VBAP_ZAK_MOVING *);
int32_t     vbap1_init(CSOUND *, VBAP1 *);
int32_t     vbap1(CSOUND *, VBAP1 *);
int32_t     vbap1_init_a(CSOUND *, VBAPA1 *);
int32_t     vbap1a(CSOUND *, VBAPA1 *);
int32_t     vbap1_moving_init(CSOUND *, VBAP1_MOVING *);
int32_t     vbap1_moving(CSOUND *, VBAP1_MOVING *);
int32_t     vbap1_moving_init_a(CSOUND *, VBAPA1_MOVING *);
int32_t
vbap1_moving_a(CSOUND *, VBAPA1_MOVING *);

