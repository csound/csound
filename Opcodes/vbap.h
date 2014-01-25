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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#define LOWEST_ACCEPTABLE_WT FL(0.0)
#define CHANNELS 64
#define MIN_VOL_P_SIDE_LGTH FL(0.01)

typedef struct {
  MYFLT x;
  MYFLT y;
  MYFLT z;
} CART_VEC;

typedef struct {
  MYFLT azi;
  MYFLT ele;
  MYFLT length;
} ANG_VEC;

/* A struct for gain factors */
typedef struct {
  MYFLT wt1, wt2, wt3;
  MYFLT *out_ptr1, *out_ptr2, *out_ptr3;
} OUT_WTS;

/* A struct for a loudspeaker triplet or pair (set) */
typedef struct {
  int ls_nos[3];
  MYFLT ls_mx[9];
  MYFLT set_gains[3];
  MYFLT smallest_wt;
  int neg_g_am;
} LS_SET;

/* VBAP structure of n loudspeaker panning */
typedef struct {
  int number;
  MYFLT beg_gains[CHANNELS];
  MYFLT curr_gains[CHANNELS];
  MYFLT end_gains[CHANNELS];
  MYFLT updated_gains[CHANNELS];
  int dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int ls_am;
  int ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir;
} VBAP_DATA;

typedef struct {
  OPDS          h;                  /* required header */
  MYFLT         *out_array[CHANNELS];
  MYFLT         *audio, *azi, *ele, *spread, *layout;

  VBAP_DATA     q;
} VBAP;

typedef struct {
  OPDS          h;                  /* required header */
  ARRAYDAT      *tabout;
  MYFLT         *audio, *azi, *ele, *spread, *layout;

  VBAP_DATA     q;
} VBAPA;

typedef struct {
  int number;
  MYFLT gains[CHANNELS];
  int dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int ls_am;
  int ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir;
} VBAP1_DATA;

typedef struct {
  OPDS      h;                  /* required header */
  MYFLT         *out_array[CHANNELS];
  MYFLT         *azi, *ele, *spread, *layout;

  VBAP1_DATA    q;
} VBAP1;

typedef struct {
  OPDS      h;                  /* required header */
  ARRAYDAT      *tabout;
  MYFLT         *azi, *ele, *spread, *layout;

  VBAP1_DATA    q;
} VBAPA1;

/* VBAP structure of loudspeaker moving panning */
typedef struct {
  MYFLT gains[CHANNELS];
  int number;
  int upd_interval;
  int dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int ls_am;
  int ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir, prev_ang_dir, next_ang_dir;
  int point_change_interval, point_change_counter, curr_fld, next_fld;
  MYFLT ele_vel;
  MYFLT end_gains[CHANNELS];
} VBAP1_MOVE_DATA;

typedef struct {
  OPDS      h;                  /* required header */
  MYFLT         *out_array[CHANNELS];
  MYFLT         *dur, *spread, *field_am,
                *fld[VARGMAX]; /* field_am positive: point to point
                                           negative: angle velocities */
  VBAP1_MOVE_DATA q;
} VBAP1_MOVING;

typedef struct {
  OPDS           h;                  /* required header */
  ARRAYDAT      *tabout;
  MYFLT         *dur, *spread, *field_am,
                *fld[VARGMAX]; /* field_am positive: point to point
                                           negative: angle velocities */
  VBAP1_MOVE_DATA q;
} VBAPA1_MOVING;

/* VBAP structure of loudspeaker moving panning */
typedef struct {
  MYFLT beg_gains[CHANNELS];
  MYFLT curr_gains[CHANNELS];
  MYFLT updated_gains[CHANNELS];
  int number;
  int upd_interval;
  int dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int ls_am;
  int ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir, prev_ang_dir, next_ang_dir;
  int point_change_interval, point_change_counter, curr_fld, next_fld;
  MYFLT ele_vel;
  MYFLT end_gains[CHANNELS];
} VBAP_MOVE_DATA;

typedef struct {
  OPDS      h;                  /* required header */
  MYFLT         *out_array[CHANNELS];
  MYFLT         *audio, *dur, *spread, *field_am,
                *fld[VARGMAX]; /* field_am positive: point to point
                                           negative: angle velocities */
  VBAP_MOVE_DATA q;
} VBAP_MOVING;


typedef struct {
  OPDS          h;                  /* required header */
  ARRAYDAT      *tabout;
  MYFLT         *audio, *dur, *spread, *field_am,
                *fld[VARGMAX]; /* field_am positive: point to point
                                           negative: angle velocities */
  VBAP_MOVE_DATA q;
} VBAPA_MOVING;

typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *dim, *ls_amount;
  MYFLT     *f[2*CHANNELS];
} VBAP_LS_INIT;

/* A struct for a loudspeaker instance */
typedef struct {
  CART_VEC coords;
  ANG_VEC angles;
  int channel_nbr;
} ls;

/* A struct for all loudspeakers */
typedef struct ls_triplet_chain {
  int ls_nos[3];
  MYFLT inv_mx[9];
  struct ls_triplet_chain *next;
} ls_triplet_chain;

/* functions */

void initialize( ls lss[CHANNELS]);
void angle_to_cart_II( ANG_VEC *from,  CART_VEC *to);
int lines_intersect(int i,int j,int k,int l, ls lss[CHANNELS]);
MYFLT vec_angle(CART_VEC v1, CART_VEC v2);
void vec_mean(CART_VEC v1, CART_VEC v2, CART_VEC *v3);
MYFLT angle_in_base(CART_VEC vb1,CART_VEC vb2,CART_VEC vec);
void cross_prod(CART_VEC v1,CART_VEC v2,
                CART_VEC *res) ;
void sort_angles(MYFLT angles[CHANNELS], int sorted_angles[CHANNELS],
                 int ls_amount);
void remove_connections_in_plane(int i,int j,int k,int l,
                                   ls  lss[CHANNELS],
                                    int connections[CHANNELS][CHANNELS]);
int calc_2D_inv_tmatrix(MYFLT azi1,MYFLT azi2, MYFLT inv_mat[4]);

extern void cart_to_angle(CART_VEC cvec, ANG_VEC *avec);
extern void angle_to_cart(ANG_VEC avec, CART_VEC *cvec);
extern void normalize_wts(OUT_WTS *wts);

extern int vbap_control(CSOUND*, VBAP_DATA *p, MYFLT*, MYFLT*, MYFLT*);

void calc_vbap_gns(int ls_set_am, int dim, LS_SET *sets,
                   MYFLT *gains, int ls_amount,
                   CART_VEC cart_dir);
void scale_angles(ANG_VEC *avec);
MYFLT vol_p_side_lgth(int i, int j, int k, ls  lss[CHANNELS]);

void new_spread_dir(CART_VEC *spreaddir, CART_VEC vscartdir,
                    CART_VEC spread_base, MYFLT azi, MYFLT spread);
void new_spread_base(CART_VEC spreaddir, CART_VEC vscartdir,
                     MYFLT spread, CART_VEC *spread_base);

/* VBAP structure for ZAK loudspeaker panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *numb, *ndx, *audio, *azi, *ele, *spread, *layout;
  int       n;
  MYFLT     *out_array;
  AUXCH     auxch;
  AUXCH     aux;
  MYFLT     *curr_gains;
  MYFLT     *beg_gains;
  MYFLT     *end_gains;
  MYFLT     *updated_gains;
  int       dim;
  LS_SET    *ls_sets;
  int       ls_am;
  int       ls_set_am;
  CART_VEC  cart_dir;
  CART_VEC  spread_base;
  ANG_VEC   ang_dir;
} VBAP_ZAK;

/* VBAP structure of ZAK loudspeaker moving panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *numb, *ndx, *audio, *dur, *spread, *field_am,
            *fld[VARGMAX]; /* field_am positive: point to point
                                       negative: angle velocities */
  int       n;
  MYFLT     *out_array;
  AUXCH     auxch;
  AUXCH     aux;
  MYFLT     *curr_gains;
  MYFLT     *beg_gains;
  MYFLT     *end_gains;
  MYFLT     *updated_gains;
  int       dim;
  LS_SET    *ls_sets;
  int       ls_am;
  int       ls_set_am;
  CART_VEC  cart_dir;
  CART_VEC  spread_base;
  ANG_VEC   ang_dir, prev_ang_dir, next_ang_dir;
  int       point_change_interval, point_change_counter, curr_fld, next_fld;
  MYFLT     ele_vel;
} VBAP_ZAK_MOVING;

int     vbap_init(CSOUND *, VBAP *);
int     vbap_init_a(CSOUND *, VBAPA *);
int     vbap(CSOUND *, VBAP *);
int     vbap_a(CSOUND *, VBAPA *);
int     vbap_zak_init(CSOUND *, VBAP_ZAK *);
int     vbap_zak(CSOUND *, VBAP_ZAK *);
int     vbap_ls_init(CSOUND *, VBAP_LS_INIT *);
int     vbap_moving_init(CSOUND *, VBAP_MOVING *);
int     vbap_moving(CSOUND *, VBAP_MOVING *);
int     vbap_moving_init_a(CSOUND *, VBAPA_MOVING *);
int     vbap_moving_a(CSOUND *, VBAPA_MOVING *);
int     vbap_zak_moving_init(CSOUND *, VBAP_ZAK_MOVING *);
int     vbap_zak_moving(CSOUND *, VBAP_ZAK_MOVING *);
int     vbap1_init(CSOUND *, VBAP1 *);
int     vbap1(CSOUND *, VBAP1 *);
int     vbap1_init_a(CSOUND *, VBAPA1 *);
int     vbap1a(CSOUND *, VBAPA1 *);
int     vbap1_moving_init(CSOUND *, VBAP1_MOVING *);
int     vbap1_moving(CSOUND *, VBAP1_MOVING *);
int     vbap1_moving_init_a(CSOUND *, VBAPA1_MOVING *);
int     vbap1_moving_a(CSOUND *, VBAPA1_MOVING *);
