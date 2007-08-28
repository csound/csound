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
#define FOUR 4
#define EIGHT 8
#define SIXTEEN 16
#define THIRTYTWO 32

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

/* VBAP structure for FOUR loudspeaker panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *out_array[FOUR];
  MYFLT     *audio, *azi, *ele, *spread;

  MYFLT beg_gains[FOUR];
  MYFLT curr_gains[FOUR];
  MYFLT end_gains[FOUR];
  MYFLT updated_gains[FOUR];
  int dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int ls_am;
  int ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir;
} VBAP_FOUR;

/* VBAP structure of FOUR loudspeaker moving panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *out_array[FOUR];
  MYFLT     *audio, *dur, *spread, *field_am,
                  *fld[VARGMAX]; /* field_am positive: point to point
                                             negative: angle velocities */
  MYFLT beg_gains[FOUR];
  MYFLT curr_gains[EIGHT];
  MYFLT end_gains[FOUR];
  MYFLT updated_gains[FOUR];
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
} VBAP_FOUR_MOVING;

/* VBAP structure of eight loudspeaker panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT         *out_array[EIGHT];
  MYFLT         *audio, *azi, *ele, *spread;

  MYFLT beg_gains[EIGHT];
  MYFLT curr_gains[EIGHT];
  MYFLT end_gains[EIGHT];
  MYFLT updated_gains[EIGHT];
  int dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int ls_am;
  int ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir;
} VBAP_EIGHT;

/* VBAP structure of EIGHT loudspeaker moving panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT         *out_array[EIGHT];
  MYFLT         *audio, *dur, *spread, *field_am,
                *fld[VARGMAX]; /* field_am positive: point to point
                                           negative: angle velocities */
  MYFLT beg_gains[EIGHT];
  MYFLT curr_gains[EIGHT];
  MYFLT updated_gains[EIGHT];
/*   int counter; */
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
   MYFLT end_gains[EIGHT];
} VBAP_EIGHT_MOVING;

/* VBAP structure of SIXTEEN loudspeaker panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *out_array[SIXTEEN];
  MYFLT     *audio, *azi, *ele, *spread;

  MYFLT beg_gains[SIXTEEN];
  MYFLT curr_gains[SIXTEEN];
  MYFLT end_gains[SIXTEEN];
  MYFLT updated_gains[SIXTEEN];
/*   int counter; */
  int upd_interval;
  int dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int ls_am;
  int ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir;
} VBAP_SIXTEEN;

/* VBAP structure of SIXTEEN loudspeaker moving panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *out_array[SIXTEEN];
  MYFLT     *audio, *dur, *spread, *field_am,
            *fld[VARGMAX]; /* field_am positive: point to point
                                       negative: angle velocities */
  MYFLT beg_gains[SIXTEEN];
  MYFLT curr_gains[SIXTEEN];
  MYFLT end_gains[SIXTEEN];
  MYFLT updated_gains[SIXTEEN];
/*   int counter; */
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
} VBAP_SIXTEEN_MOVING;

/* VBAP structure of THIRTYTWO loudspeaker panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *out_array[THIRTYTWO];
  MYFLT     *audio, *azi, *ele, *spread;

  MYFLT beg_gains[THIRTYTWO];
  MYFLT curr_gains[THIRTYTWO];
  MYFLT end_gains[THIRTYTWO];
  MYFLT updated_gains[THIRTYTWO];
/*   int counter; */
  int upd_interval;
  int dim;
  AUXCH aux;
  LS_SET *ls_sets;
  int ls_am;
  int ls_set_am;
  CART_VEC cart_dir;
  CART_VEC spread_base;
  ANG_VEC ang_dir;
} VBAP_THIRTYTWO;

/* VBAP structure of THIRTYTWO loudspeaker moving panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *out_array[THIRTYTWO];
  MYFLT     *audio, *dur, *spread, *field_am,
             *fld[VARGMAX]; /* field_am positive: point to point
                                        negative: angle velocities */
  MYFLT beg_gains[THIRTYTWO];
  MYFLT curr_gains[THIRTYTWO];
  MYFLT end_gains[THIRTYTWO];
  MYFLT updated_gains[THIRTYTWO];
/*   int counter; */
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
} VBAP_THIRTYTWO_MOVING;

typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *dim, *ls_amount;
  MYFLT     *f[64];
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
MYFLT vec_prod(CART_VEC v1, CART_VEC v2);
MYFLT vec_length(CART_VEC v1);
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

extern int vbap_FOUR_control(CSOUND*, VBAP_FOUR *p);
extern int vbap_EIGHT_control(CSOUND*, VBAP_EIGHT *p);
extern int vbap_SIXTEEN_control(CSOUND*, VBAP_SIXTEEN *p);

void calc_vbap_gns(int ls_set_am, int dim, LS_SET *sets,
                   MYFLT *gains, int ls_amount,
                   CART_VEC cart_dir);
void scale_angles(ANG_VEC *avec);
MYFLT vol_p_side_lgth(int i, int j, int k, ls  lss[CHANNELS]);

void new_spread_dir(CART_VEC *spreaddir, CART_VEC vscartdir, CART_VEC spread_base, MYFLT azi, MYFLT spread);
void new_spread_base(CART_VEC spreaddir, CART_VEC vscartdir, MYFLT spread, CART_VEC *spread_base);

/* VBAP structure for ZAK loudspeaker panning */
typedef struct {
  OPDS      h;                  /* required header */
  MYFLT     *numb, *ndx, *audio, *azi, *ele, *spread;
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

int     vbap_FOUR_init(CSOUND *, VBAP_FOUR *);
int     vbap_FOUR(CSOUND *, VBAP_FOUR *);
int     vbap_EIGHT_init(CSOUND *, VBAP_EIGHT *);
int     vbap_EIGHT(CSOUND *, VBAP_EIGHT *);
int     vbap_SIXTEEN_init(CSOUND *, VBAP_SIXTEEN *);
int     vbap_SIXTEEN(CSOUND *, VBAP_SIXTEEN *);
int     vbap_zak_init(CSOUND *, VBAP_ZAK *);
int     vbap_zak(CSOUND *, VBAP_ZAK *);
int     vbap_ls_init(CSOUND *, VBAP_LS_INIT *);
int     vbap_FOUR_moving_init(CSOUND *, VBAP_FOUR_MOVING *);
int     vbap_FOUR_moving(CSOUND *, VBAP_FOUR_MOVING *);
int     vbap_EIGHT_moving_init(CSOUND *, VBAP_EIGHT_MOVING *);
int     vbap_EIGHT_moving(CSOUND *, VBAP_EIGHT_MOVING *);
int     vbap_SIXTEEN_moving_init(CSOUND *, VBAP_SIXTEEN_MOVING *);
int     vbap_SIXTEEN_moving(CSOUND *, VBAP_SIXTEEN_MOVING *);
int     vbap_zak_moving_init(CSOUND *, VBAP_ZAK_MOVING *);
int     vbap_zak_moving(CSOUND *, VBAP_ZAK_MOVING *);

static inline MYFLT *get_ls_table(CSOUND *csound)
{
    return (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound,
                                                        "vbap_ls_table"));
}

