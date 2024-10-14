/*
  vbap.c:

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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

/* vbap.c

   assisting functions for VBAP
   functions for loudspeaker table initialization
   Re-written to take flexible number of outputs by JPff 2012 */


#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include "vbap.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "interlocks.h"
#include "zak.h"

#define MATSIZE (4)
#define ATORAD  (TWOPI_F / FL(360.0))

static int64_t GetZaBounds(CSOUND *csound, MYFLT **zastart){
    ZAK_GLOBALS *zz;
    zz = (ZAK_GLOBALS*) csound->QueryGlobalVariable(csound, "_zak_globals");
    if (zz==NULL) {
      *zastart = NULL;
      return -1;
    }
    *zastart = zz->zastart;
    return zz->zalast;
}


/* static void choose_ls_triplets(CSOUND *csound, ls *lss, */
/*                                ls_triplet_chain **ls_triplets, */
/*                                int32_t ls_amount, int32_t channels); */
static int32_t any_ls_inside_triplet(int32_t, int32_t, int32_t, ls[], int32_t);
static void add_ldsp_triplet(CSOUND *csound, int32_t i, int32_t j, int32_t k,
                             ls_triplet_chain **ls_triplets,
                             ls *lss);
static void calculate_3x3_matrixes(CSOUND *csound,
                                   ls_triplet_chain *ls_triplets,
                                   ls lss[], int32_t ls_amount, int32_t ind);
static void choose_ls_tuplets(CSOUND *csound, ls lss[],
                              ls_triplet_chain **ls_triplets,
                              int32_t ls_amount, int32_t ind);
static void sort_2D_lss(ls lss[], int32_t sorted_lss[],
                        int32_t ls_amount);

static inline MYFLT vec_prod(CART_VEC v1, CART_VEC v2)
{
  return (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z);
}

static inline MYFLT vec_length(CART_VEC v1)
{
  return SQRT(v1.x*v1.x + v1.y*v1.y + v1.z*v1.z);
}

static MYFLT *create_ls_table(CSOUND *csound, size_t cnt, int32_t ind)
{
  char name[24];
  snprintf(name, 24, "vbap_ls_table_%d", ind);
  csound->DestroyGlobalVariable(csound, name);
  if (UNLIKELY(csound->CreateGlobalVariable(csound, name,
                                            cnt * sizeof(MYFLT)) != 0)) {
    csound->ErrorMsg(csound, "%s", Str("vbap: error allocating loudspeaker table"));
    return NULL;
  }
  return (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, name));
}

void calc_vbap_gns(int32_t ls_set_am, int32_t dim, LS_SET *sets,
                   MYFLT *gains, int32_t ls_amount,
                   CART_VEC cart_dir)
/* Selects a vector base of a virtual source.
   Calculates gain factors in that base. */
{
  int32_t i,j,k, tmp2;
  MYFLT vec[3], tmp;
  /* direction of the virtual source in cartesian coordinates*/
  vec[0] = cart_dir.x;
  vec[1] = cart_dir.y;
  vec[2] = cart_dir.z;


  for (i=0; i< ls_set_am; i++) {
    sets[i].set_gains[0] = FL(0.0);
    sets[i].set_gains[1] = FL(0.0);
    sets[i].set_gains[2] = FL(0.0);
    sets[i].smallest_wt  = FL(1000.0);
    sets[i].neg_g_am = 0;
  }

  for (i=0; i< ls_set_am; i++) {
    for (j=0; j< dim; j++) {
      for (k=0; k< dim; k++) {
        sets[i].set_gains[j] += vec[k] * sets[i].ls_mx[((dim * j )+ k)];
      }
      if (sets[i].smallest_wt > sets[i].set_gains[j])
        sets[i].smallest_wt = sets[i].set_gains[j];
      if (sets[i].set_gains[j] < -FL(0.05))
        sets[i].neg_g_am++;
    }
  }



  j=0;
  tmp = sets[0].smallest_wt;
  tmp2=sets[0].neg_g_am;
  for (i=1; i< ls_set_am; i++) {
    if (sets[i].neg_g_am < tmp2) {
      tmp = sets[i].smallest_wt;
      tmp2=sets[i].neg_g_am;
      j=i;
    }
    else if (sets[i].neg_g_am == tmp2) {
      if (sets[i].smallest_wt > tmp) {
        tmp = sets[i].smallest_wt;
        tmp2=sets[i].neg_g_am;
        j=i;
      }
    }
  }


  if (sets[j].set_gains[0]<=FL(0.0) &&
      sets[j].set_gains[1]<=FL(0.0) &&
      sets[j].set_gains[2]<=FL(0.0)) {
    sets[j].set_gains[0] = FL(1.0);
    sets[j].set_gains[1] = FL(1.0);
    sets[j].set_gains[2] = FL(1.0);
  }

  memset(gains, 0, ls_amount*sizeof(MYFLT));

  gains[sets[j].ls_nos[0]-1] = sets[j].set_gains[0];
  gains[sets[j].ls_nos[1]-1] = sets[j].set_gains[1];
  if (dim==3) gains[sets[j].ls_nos[2]-1] = sets[j].set_gains[2];

  for (i=0;i<ls_amount;i++) {
    if (gains[i]<LOWEST_ACCEPTABLE_WT)
      gains[i]=FL(0.0);
  }
}

void scale_angles(ANG_VEC *avec)
/* -180 < azi < 180
   -90 < ele < 90 */
{
  while (avec->azi > FL(180.0))
    avec->azi -= FL(360.0);
  while (avec->azi < -FL(180.0))
    avec->azi += FL(360.0);
  if (avec->ele > FL(90.0))
    avec->ele = FL(90.0);
  if (avec->ele < -FL(90.0))
    avec->ele = -FL(90.0);
}

void normalize_wts(OUT_WTS *wts)
/* performs equal-power normalization to gain factors*/
{
  double tmp;
  MYFLT tmp1;
  if (wts->wt1 < 0) wts->wt1 = FL(0.0);
  if (wts->wt2 < 0) wts->wt2 = FL(0.0);
  if (wts->wt3 < 0) wts->wt3 = FL(0.0);

  tmp  = (double)wts->wt1 * wts->wt1;
  tmp += (double)wts->wt2 * wts->wt2;
  tmp += (double)wts->wt3 * wts->wt3;

  tmp = sqrt(tmp);
  tmp1 = (MYFLT)(1.0 / tmp);
  wts->wt1 *= tmp1;
  wts->wt2 *= tmp1;
  wts->wt3 *= tmp1;
}

void angle_to_cart(ANG_VEC avec, CART_VEC *cvec)
/* conversion */
{
  /* length unattended */
  //MYFLT atorad = (TWOPI_F / FL(360.0));
  cvec->x = (MYFLT) (cos((double) (avec.azi * ATORAD)) *
                     cos((double) (avec.ele * ATORAD)));
  cvec->y = (MYFLT) (sin((double) (avec.azi * ATORAD)) *
                     cos((double) (avec.ele * ATORAD)));
  cvec->z = (MYFLT) (sin((double) (avec.ele * ATORAD)));
}

void cart_to_angle(CART_VEC cvec, ANG_VEC *avec)
/* conversion */
{
  MYFLT tmp, tmp2, tmp3, tmp4;
  //MYFLT atorad = (TWOPI_F / FL(360.0));

  tmp3 = SQRT(FL(1.0) - cvec.z*cvec.z);
  if (FABS(tmp3) > FL(0.001)) {
    tmp4 = (cvec.x / tmp3);
    if (tmp4 > FL(1.0)) tmp4 = FL(1.0);
    if (tmp4 < -FL(1.0)) tmp4 = -FL(1.0);
    tmp = ACOS(tmp4 );
  }
  else {
    tmp = FL(10000.0);
  }
  if (FABS(cvec.y) <= FL(0.001))
    tmp2 = FL(1.0);
  else
    tmp2 = cvec.y / FABS(cvec.y);
  tmp *= tmp2;
  if (FABS(tmp) <= PI_F) {
    avec->azi =  tmp;
    avec->azi /= ATORAD;
  }
  avec->ele = ASIN(cvec.z);
  avec->length = SQRT(cvec.x * cvec.x + cvec.y * cvec.y + cvec.z * cvec.z);
  avec->ele /= ATORAD;
}

void angle_to_cart_II(ANG_VEC *from, CART_VEC *to)
/* conversion, double*/
{
  MYFLT ang2rad = TWOPI_F / FL(360.0);
  to->x= COS(from->azi * ang2rad) * COS(from->ele * ang2rad);
  to->y= SIN(from->azi * ang2rad) * COS(from->ele * ang2rad);
  to->z= SIN(from->ele * ang2rad);
}

MYFLT vol_p_side_lgth(int32_t i, int32_t j,int32_t k, ls  lss[] )
{
  /* calculate volume of the parallelepiped defined by the loudspeaker
     direction vectors and divide it with total length of the triangle sides.
     This is used when removing too narrow triangles. */

  MYFLT volper, lgth;
  CART_VEC xprod;
  cross_prod(lss[i].coords, lss[j].coords, &xprod);
  volper = FABS(vec_prod(xprod, lss[k].coords));
  lgth =    FABS(vec_angle(lss[i].coords,lss[j].coords))
    + FABS(vec_angle(lss[i].coords,lss[k].coords))
    + FABS(vec_angle(lss[j].coords,lss[k].coords));
  if (LIKELY(lgth>FL(0.00001)))
    return volper / lgth;
  else
    return FL(0.0);
}

static void choose_ls_triplets(CSOUND *csound, ls *lss,
                               struct ls_triplet_chain **ls_triplets,
                               int32_t ls_amount)
/* Selects the loudspeaker triplets, and
   calculates the inversion matrices for each selected triplet.
   A line (connection) is drawn between each loudspeaker. The lines
   denote the sides of the triangles. The triangles should not be
   intersecting. All crossing connections are searched and the
   longer connection is erased. This yields non-intesecting triangles,
   which can be used in panning.*/
{
  int32_t i, j, k, l, table_size;
  int32_t *connections;
  /*  int32_t *i_ptr; */
  MYFLT *distance_table;
  int32_t *distance_table_i;
  int32_t *distance_table_j;
  MYFLT distance;
  struct ls_triplet_chain *trip_ptr, *prev, *tmp_ptr;

  if (UNLIKELY(ls_amount == 0)) {
    csound->ErrorMsg(csound, "%s", Str("Number of loudspeakers is zero\nExiting"));
    return;
  }

  connections = csound->Calloc(csound, ls_amount * ls_amount * sizeof(int32_t));
  distance_table =
    csound->Calloc(csound, ((ls_amount * (ls_amount - 1)) / 2)* sizeof(MYFLT));
  distance_table_i =
    csound->Calloc(csound, ((ls_amount * (ls_amount - 1)) / 2)* sizeof(int32_t));
  distance_table_j =
    csound->Calloc(csound, ((ls_amount * (ls_amount - 1)) / 2)* sizeof(int32_t));

  /*  i_ptr = (int32_t *) connections; */
  /*  for (i=0;i< ((CHANNELS) * (CHANNELS )); i++) */
  /*    *(i_ptr++) = 0; */

  for (i=0;i<ls_amount;i++)
    for (j=i+1;j<ls_amount;j++)
      for (k=j+1;k<ls_amount;k++) {
        if (vol_p_side_lgth(i,j, k, lss) > MIN_VOL_P_SIDE_LGTH) {
          connections[i+ls_amount*j]=1;
          connections[j+ls_amount*i]=1;
          connections[i+ls_amount*k]=1;
          connections[k+ls_amount*i]=1;
          connections[j+ls_amount*k]=1;
          connections[k+ls_amount*j]=1;
          add_ldsp_triplet(csound, i, j, k, ls_triplets, lss);
        }
      }

  /*calculate distancies between all lss and sorting them*/
  table_size =(((ls_amount - 1) * (ls_amount)) / 2);
  for (i=0;i<table_size; i++)
    distance_table[i] = FL(100000.0);

  for (i=0;i<ls_amount;i++) {
    for (j=(i+1);j<ls_amount; j++) {
      if (connections[i+ls_amount*j] == 1) {
        distance = FABS(vec_angle(lss[i].coords,lss[j].coords));
        k=0;

        while (distance_table[k] < distance)
          k++;
        for (l=(table_size - 1);l > k;l--) {
          distance_table[l] = distance_table[l-1];
          distance_table_i[l] = distance_table_i[l-1];
          distance_table_j[l] = distance_table_j[l-1];
        }
        distance_table[k] = distance;
        distance_table_i[k] = i;
        distance_table_j[k] = j;
      }
      else
        table_size--;
    }
  }

  /* disconnecting connections which are crossing shorter ones,
     starting from shortest one and removing all that cross it,
     and proceeding to next shortest */
  for (i=0; i<(table_size); i++) {
    int32_t fst_ls = distance_table_i[i];
    int32_t sec_ls = distance_table_j[i];
    if (connections[fst_ls+ls_amount*sec_ls] == 1)
      for (j=0; j<ls_amount; j++)
        for (k=j+1; k<ls_amount; k++)
          if ( (j!=fst_ls) && (k != sec_ls) && (k!=fst_ls) && (j != sec_ls))
            if (lines_intersect(fst_ls, sec_ls, j,k,lss) == 1) {
              connections[j+ls_amount*k] = 0;
              connections[k+ls_amount*j] = 0;
            }
  }

  /* remove triangles which had crossing sides
     with smaller triangles or include loudspeakers*/

  trip_ptr = *ls_triplets;
  prev = NULL;
  while (trip_ptr != NULL) {
    i = trip_ptr->ls_nos[0];
    j = trip_ptr->ls_nos[1];
    k = trip_ptr->ls_nos[2];
    if (connections[i+ls_amount*j] == 0 ||
        connections[i+ls_amount*k] == 0 ||
        connections[j+ls_amount*k] == 0 ||
        any_ls_inside_triplet(i,j,k,lss,ls_amount) == 1) {
      if (prev != NULL) {
        prev->next = trip_ptr->next;
        tmp_ptr = trip_ptr;
        trip_ptr = trip_ptr->next;
        csound->Free(csound, tmp_ptr);
      }
      else {
        *ls_triplets = trip_ptr->next;
        tmp_ptr = trip_ptr;
        trip_ptr = trip_ptr->next;
        csound->Free(csound, tmp_ptr);
      }
    }
    else {
      prev = trip_ptr;
      trip_ptr = trip_ptr->next;
    }
  }
  csound->Free(csound,connections);
  csound->Free(csound,distance_table);
  csound->Free(csound,distance_table_i);
  csound->Free(csound,distance_table_j);
}

/* returns 1 if there is loudspeaker(s) inside given ls triplet */

static int32_t any_ls_inside_triplet(int32_t a, int32_t b, int32_t c, ls lss[],
                                     int32_t ls_amount)
{
  MYFLT invdet;
  CART_VEC *lp1, *lp2, *lp3;
  MYFLT invmx[9];
  int32_t i,j;
  MYFLT tmp;
  int32_t any_ls_inside, this_inside;

  lp1 =  &(lss[a].coords);
  lp2 =  &(lss[b].coords);
  lp3 =  &(lss[c].coords);

  /* matrix inversion */
  invdet = FL(1.0) / (  lp1->x * ((lp2->y * lp3->z) - (lp2->z * lp3->y))
                        - lp1->y * ((lp2->x * lp3->z) - (lp2->z * lp3->x))
                        + lp1->z * ((lp2->x * lp3->y) - (lp2->y * lp3->x)));

  invmx[0] = ((lp2->y * lp3->z) - (lp2->z * lp3->y)) * invdet;
  invmx[3] = ((lp1->y * lp3->z) - (lp1->z * lp3->y)) * -invdet;
  invmx[6] = ((lp1->y * lp2->z) - (lp1->z * lp2->y)) * invdet;
  invmx[1] = ((lp2->x * lp3->z) - (lp2->z * lp3->x)) * -invdet;
  invmx[4] = ((lp1->x * lp3->z) - (lp1->z * lp3->x)) * invdet;
  invmx[7] = ((lp1->x * lp2->z) - (lp1->z * lp2->x)) * -invdet;
  invmx[2] = ((lp2->x * lp3->y) - (lp2->y * lp3->x)) * invdet;
  invmx[5] = ((lp1->x * lp3->y) - (lp1->y * lp3->x)) * -invdet;
  invmx[8] = ((lp1->x * lp2->y) - (lp1->y * lp2->x)) * invdet;

  any_ls_inside = 0;
  for (i=0; i< ls_amount; i++) {
    if (i != a && i!=b && i != c) {
      this_inside = 1;
      for (j=0; j< 3; j++) {
        tmp = lss[i].coords.x * invmx[0 + j*3];
        tmp += lss[i].coords.y * invmx[1 + j*3];
        tmp += lss[i].coords.z * invmx[2 + j*3];
        if (tmp < -FL(0.001))
          this_inside = 0;
      }
      if (this_inside == 1)
        any_ls_inside=1;
    }
  }
  return any_ls_inside;
}

static void add_ldsp_triplet(CSOUND *csound, int32_t i, int32_t j, int32_t k,
                             struct ls_triplet_chain **ls_triplets,
                             ls lss[])
{
  IGN(lss);
  struct ls_triplet_chain *ls_ptr, *prev;
  ls_ptr = *ls_triplets;
  prev = NULL;

  /*printf("Adding triangle %d %d %d %x... \n",i,j,k,ls_ptr);*/
  while (ls_ptr != NULL) {
    /*printf("ls_ptr %x %x\n",ls_ptr,ls_ptr->next);*/
    prev = ls_ptr;
    ls_ptr = ls_ptr->next;
  }
  ls_ptr = (struct ls_triplet_chain*)
    csound->Malloc(csound, sizeof(struct ls_triplet_chain));
  if (prev == NULL)
    *ls_triplets = ls_ptr;
  else
    prev->next = ls_ptr;
  ls_ptr->next = NULL;
  ls_ptr->ls_nos[0] = i;
  ls_ptr->ls_nos[1] = j;
  ls_ptr->ls_nos[2] = k;
  /*printf("added.\n");*/
}

MYFLT angle_in_base(CART_VEC vb1,CART_VEC vb2,CART_VEC vec)
{
  MYFLT tmp1,tmp2;
  tmp1 = vec_prod(vec,vb2);
  if (FABS(tmp1) <= FL(0.001))
    tmp2 = FL(1.0);
  else
    tmp2 = tmp1 / FABS(tmp1);
  return (vec_angle(vb1,vec) * tmp2);
}

MYFLT vec_angle(CART_VEC v1, CART_VEC v2)
{
  MYFLT inner= ((v1.x*v2.x + v1.y*v2.y + v1.z*v2.z)/
                (vec_length(v1) * vec_length(v2)));
  if (inner > FL(1.0))
    inner= FL(1.0);
  if (inner < -FL(1.0))
    inner = -FL(1.0);
  return ACOS(inner);
}

void vec_mean(CART_VEC v1, CART_VEC v2, CART_VEC *v3)
{
  v3->x=(v1.x+v2.x)*FL(0.5);
  v3->y=(v1.y+v2.y)*FL(0.5);
  v3->z=(v1.z+v2.z)*FL(0.5);
}

void cross_prod(CART_VEC v1,CART_VEC v2,
                CART_VEC *res)
{
  MYFLT length;
  res->x = (v1.y * v2.z ) - (v1.z * v2.y);
  res->y = (v1.z * v2.x ) - (v1.x * v2.z);
  res->z = (v1.x * v2.y ) - (v1.y * v2.x);

  length= vec_length(*res);
  res->x /= length;
  res->y /= length;
  res->z /= length;
}

void vec_print(CSOUND *csound, CART_VEC v)
{
  csound->Message(csound, "vec_print %f %f %f\n", v.x, v.y,v.z);

}

int32_t lines_intersect(int32_t i,int32_t j,int32_t k,int32_t l,ls  lss[])
/* checks if two lines intersect on 3D sphere
   see theory in paper Pulkki, V. Lokki, T. "Creating Auditory Displays
   with Multiple Loudspeakers Using VBAP: A Case Study with
   DIVA Project" in International Conference on
   Auditory Displays -98. E-mail Ville.Pulkki@hut.fi
   if you want to have that paper.*/
{
  CART_VEC v1;
  CART_VEC v2;
  CART_VEC v3, neg_v3;
  MYFLT dist_ij,dist_kl,dist_iv3,dist_jv3,dist_inv3,dist_jnv3;
  MYFLT dist_kv3,dist_lv3,dist_knv3,dist_lnv3;

  cross_prod(lss[i].coords,lss[j].coords,&v1);
  cross_prod(lss[k].coords,lss[l].coords,&v2);
  cross_prod(v1,v2,&v3);

  neg_v3.x= FL(0.0) - v3.x;
  neg_v3.y= FL(0.0) - v3.y;
  neg_v3.z= FL(0.0) - v3.z;

  dist_ij = (vec_angle(lss[i].coords,lss[j].coords));
  dist_kl = (vec_angle(lss[k].coords,lss[l].coords));
  dist_iv3 = (vec_angle(lss[i].coords,v3));
  dist_jv3 = (vec_angle(v3,lss[j].coords));
  dist_inv3 = (vec_angle(lss[i].coords,neg_v3));
  dist_jnv3 = (vec_angle(neg_v3,lss[j].coords));
  dist_kv3 = (vec_angle(lss[k].coords,v3));
  dist_lv3 = (vec_angle(v3,lss[l].coords));
  dist_knv3 = (vec_angle(lss[k].coords,neg_v3));
  dist_lnv3 = (vec_angle(neg_v3,lss[l].coords));

  /* if one of loudspeakers is close to crossing point, don't do anything*/
  if (FABS(dist_iv3) <= FL(0.01) || FABS(dist_jv3) <= FL(0.01) ||
      FABS(dist_kv3) <= FL(0.01) || FABS(dist_lv3) <= FL(0.01) ||
      FABS(dist_inv3) <= FL(0.01) || FABS(dist_jnv3) <= FL(0.01) ||
      FABS(dist_knv3) <= FL(0.01) || FABS(dist_lnv3) <= FL(0.01) )
    return(0);

  if (((FABS(dist_ij - (dist_iv3 + dist_jv3)) <= FL(0.01) ) &&
       (FABS(dist_kl - (dist_kv3 + dist_lv3))  <= FL(0.01))) ||
      ((FABS(dist_ij - (dist_inv3 + dist_jnv3)) <= FL(0.01))  &&
       (FABS(dist_kl - (dist_knv3 + dist_lnv3)) <= FL(0.01) ))) {
    return (1);
  }
  else {
    return (0);
  }
}

static inline int32_t vbap_ls_init_sr (CSOUND *csound, int32_t dim, int32_t count,
                                       MYFLT **f, int32_t layout)
/* Inits the loudspeaker data. Calls choose_ls_tuplets or _triplets
   according to current dimension. The inversion matrices are
   stored in transposed form to ease calculation at run time.*/
{
  struct ls_triplet_chain *ls_triplets = NULL;
  ls *lss = malloc(sizeof(ls)*count);

  ANG_VEC a_vector;
  CART_VEC c_vector;
  int32_t i=0,j;

  //dim = (int32_t) *p->dim;
  csound->Message(csound, "dim : %d\n",dim);
  if (UNLIKELY(!((dim==2) || (dim == 3)))) {
    free(lss);
    csound->ErrorMsg(csound,
                     Str("Error in loudspeaker dimension. %d not permitted"),
                     dim);
    return NOTOK;
  }
  //count = (int32_t) *p->ls_amount;
  for (j=1;j<=count;j++) {
    if (dim == 3) {
      a_vector.azi= (MYFLT) *f[2*j-2];
      a_vector.ele= (MYFLT) *f[2*j-1];
    }
    else if (dim == 2) {
      a_vector.azi= (MYFLT) *f[j-1];
      a_vector.ele=FL(0.0);
    }
    angle_to_cart_II(&a_vector,&c_vector);
    lss[i].coords.x = c_vector.x;
    lss[i].coords.y = c_vector.y;
    lss[i].coords.z = c_vector.z;
    lss[i].angles.azi = a_vector.azi;
    lss[i].angles.ele = a_vector.ele;
    lss[i].angles.length = FL(1.0);
    /* printf("**** lss[%d]: (%g %g %g) %g %g\n", i, lss[i].coords.x, */
    /*        lss[i].coords.y, lss[i].coords.z, a_vector.azi, a_vector.ele); */
    i++;
  }
  //ls_amount = (int32_t)*p->ls_amount;
  if (UNLIKELY(count < dim)) {
    free(lss);
    csound->ErrorMsg(csound, "%s", Str("Too few loudspeakers"));
    return NOTOK;
  }

  if (dim == 3) {
    choose_ls_triplets(csound, lss, &ls_triplets, count);
    calculate_3x3_matrixes(csound, ls_triplets, lss, count, layout);
  }
  else if (dim ==2) {
    choose_ls_tuplets(csound, lss, &ls_triplets, count, layout);
  }
  free(lss);
  return OK;
}

int32_t vbap_ls_init (CSOUND *csound, VBAP_LS_INIT *p)
{
  int32_t dim = (int32_t) *p->dim;
  MYFLT  layout = (*p->dim-dim)*100;
  return vbap_ls_init_sr(csound, dim, (int32_t) *p->ls_amount,
                         p->f, round(layout));
}

int32_t vbap_ls_inita (CSOUND *csound, VBAP_LS_INITA *p)
{
  int32_t dim = (int32_t) *p->dim;
  MYFLT  layout = (*p->dim-dim)*100;
  int32_t i, n = (int32_t)*p->ls_amount;
  /* if (n>CHANNELS) */
  /*   return csound->InitError(csound, "%s", Str("Too many speakers (%n)\n"), n); */
  if (UNLIKELY(n>p->a->sizes[0]))
    return csound->InitError(csound, Str("Too little data speakers (%d)\n"),
                             n>p->a->sizes[0]);
  MYFLT  **f = csound->Malloc(csound, 2*sizeof(MYFLT*)*n);
  // Transfer values to pointers
  for (i=0; i<2*n; i++) f[i] = &(p->a->data[i]);
  n = vbap_ls_init_sr(csound, dim, n, f, round(layout));
  csound->Free(csound, f);
  return n;
}

static void calculate_3x3_matrixes(CSOUND *csound,
                                   struct ls_triplet_chain *ls_triplets,
                                   ls lss[], int32_t ls_amount, int32_t ind)
/* Calculates the inverse matrices for 3D */
{
  MYFLT invdet;
  CART_VEC *lp1, *lp2, *lp3;
  MYFLT *ls_table, *invmx;
  MYFLT *ptr;
  struct ls_triplet_chain *tr_ptr = ls_triplets;
  int32_t triplet_amount = 0, i,j,k;

  if (UNLIKELY(tr_ptr == NULL)) {
    csound->ErrorMsg(csound, "%s", Str("Not valid 3-D configuration"));
    return;
  }

  /* counting triplet amount */
  while (tr_ptr != NULL) {
    triplet_amount++;
    tr_ptr = tr_ptr->next;
  }

  /* calculations and data storage to a global array */
  ls_table = create_ls_table(csound, triplet_amount * 12 + 3, ind);
  ls_table[0] = FL(3.0);  /* dimension */
  ls_table[1] = (MYFLT) ls_amount;
  ls_table[2] = (MYFLT) triplet_amount;
  tr_ptr = ls_triplets;
  ptr = (MYFLT *) &(ls_table[3]);
  while (tr_ptr != NULL) {
    lp1 =  &(lss[tr_ptr->ls_nos[0]].coords);
    lp2 =  &(lss[tr_ptr->ls_nos[1]].coords);
    lp3 =  &(lss[tr_ptr->ls_nos[2]].coords);

    /* matrix inversion */
    invmx = tr_ptr->inv_mx;
    invdet = FL(1.0) / (  lp1->x * ((lp2->y * lp3->z) - (lp2->z * lp3->y))
                          - lp1->y * ((lp2->x * lp3->z) - (lp2->z * lp3->x))
                          + lp1->z * ((lp2->x * lp3->y) - (lp2->y * lp3->x)));

    invmx[0] = ((lp2->y * lp3->z) - (lp2->z * lp3->y)) * invdet;
    invmx[3] = ((lp1->y * lp3->z) - (lp1->z * lp3->y)) * -invdet;
    invmx[6] = ((lp1->y * lp2->z) - (lp1->z * lp2->y)) * invdet;
    invmx[1] = ((lp2->x * lp3->z) - (lp2->z * lp3->x)) * -invdet;
    invmx[4] = ((lp1->x * lp3->z) - (lp1->z * lp3->x)) * invdet;
    invmx[7] = ((lp1->x * lp2->z) - (lp1->z * lp2->x)) * -invdet;
    invmx[2] = ((lp2->x * lp3->y) - (lp2->y * lp3->x)) * invdet;
    invmx[5] = ((lp1->x * lp3->y) - (lp1->y * lp3->x)) * -invdet;
    invmx[8] = ((lp1->x * lp2->y) - (lp1->y * lp2->x)) * invdet;
    for (i=0;i<3;i++) {
      *(ptr++) = (MYFLT) tr_ptr->ls_nos[i]+1;
    }
    for (i=0;i<9;i++) {
      *(ptr++) = (MYFLT) invmx[i];
    }
    tr_ptr = tr_ptr->next;
  }

  k = 3;
  csound->Warning(csound, "%s", Str("\nConfigured loudspeakers\n"));
  for (i = 0; i < triplet_amount; i++) {
    csound->Warning(csound,  Str("Triplet %d Loudspeakers: "), i);
    for (j = 0; j < 3; j++) {
      csound->Warning(csound, "%d ", (int32_t) ls_table[k++]);
    }
    csound->Warning(csound, "\n");

    /* printf("\nMatrix ");  */
    /*   for (j = 0; j < 9; j++) { */
    /*     printf("%f ", ls_table[k]);  */
    /*     k++; */
    /*   } */
    /* printf("\n\n"); */
  }
}

static void choose_ls_tuplets(CSOUND *csound,
                              ls lss[],
                              ls_triplet_chain **ls_triplets,
                              int32_t ls_amount, int32_t ind)
/* selects the loudspeaker pairs, calculates the inversion
   matrices and stores the data to a global array */
{
  IGN(ls_triplets);
  int32_t i, j, k;
  int32_t *sorted_lss = (int32_t*)malloc(sizeof(int32_t)*ls_amount);
  int32_t *exist = (int32_t*)calloc(1,sizeof(int32_t)*ls_amount);
  int32_t amount = 0;
  MYFLT *inv_mat = (MYFLT*)malloc(MATSIZE*sizeof(MYFLT)*ls_amount),
    *ls_table, *ptr;
  //int32_t ftable_size;

  /* sort loudspeakers according their aximuth angle */
  sort_2D_lss(lss,sorted_lss,ls_amount);

  /* adjacent loudspeakers are the loudspeaker pairs to be used.*/
  for (i=0;i<(ls_amount-1);i++) {
    csound->Message(csound, "***%d %d %f %f\n",sorted_lss[i],sorted_lss[i+1],
                    lss[sorted_lss[i]].angles.azi,
                    lss[sorted_lss[i+1]].angles.azi);
    if (LIKELY((lss[sorted_lss[i+1]].angles.azi -
                lss[sorted_lss[i]].angles.azi) <= (PI - 0.0175))) {
      if (LIKELY(calc_2D_inv_tmatrix( lss[sorted_lss[i]].angles.azi,
                                      lss[sorted_lss[i+1]].angles.azi,
                                      &inv_mat[MATSIZE*i]) != 0)) {
        exist[i]=1;
        amount++;
      }
    }
    else  csound->Warning(csound, Str("Pair of speakers at %f and %f ignored\n"),
                          lss[sorted_lss[i]].angles.azi*FL(180.0)/PI_F,
                          lss[sorted_lss[i+1]].angles.azi*FL(180.0)/PI_F);
  }

  if (LIKELY(((TWOPI_F - lss[sorted_lss[ls_amount-1]].angles.azi)
              +lss[sorted_lss[0]].angles.azi) < (PI - 0.0175))) {
    //printf("**less than PI type 2- 0.175\n");
    if (LIKELY(calc_2D_inv_tmatrix(lss[sorted_lss[ls_amount-1]].angles.azi,
                                   lss[sorted_lss[0]].angles.azi,
                                   &inv_mat[MATSIZE*(ls_amount-1)]) != 0)) {
      exist[ls_amount-1]=1;
      amount++;
    }
  }
  else  csound->Warning(csound, Str("Pair of speakers at %f and %f ignored\n"),
                        lss[sorted_lss[ls_amount-1]].angles.azi*FL(180.0)/PI_F,
                        lss[sorted_lss[0]].angles.azi*FL(180.0)/PI_F);

  if (UNLIKELY(amount==0)) {
    csound->InitError(csound, "%s", Str("insufficient valid speakers"));
    free(sorted_lss); free(exist); free(inv_mat);
    return;
  }

#if 0
  if ( amount*6 + 6 <= 16) ftable_size = 16;
  else if ( amount*6 + 6 <= 32) ftable_size = 32;
  else if ( amount*6 + 6 <= 64) ftable_size = 64;
  else if ( amount*6 + 6 <= 128) ftable_size = 128;
  else if ( amount*6 + 6 <= 256) ftable_size = 256;
  else if ( amount*6 + 6 <= 1024) ftable_size = 1024;
  csound->Message(csound,
                  "Loudspeaker matrices calculated with configuration : ");
  for (i=0; i< ls_amount; i++)
    csound->Message(csound, "%.1f ", lss[i].angles.azi / ATORAD);
  csound->Message(csound, "\n");
#endif
  ls_table = create_ls_table(csound, amount * 6 + 3 + 100, ind);
  ls_table[0] = FL(2.0);  /* dimension */
  ls_table[1] = (MYFLT) ls_amount;
  ls_table[2] = (MYFLT) amount;
  ptr = &(ls_table[3]);
  for (i=0;i<ls_amount - 1;i++) {
    if (exist[i] == 1) {
      *(ptr++) = (MYFLT)sorted_lss[i]+1;
      *(ptr++) = (MYFLT)sorted_lss[i+1]+1;
      for (j=0;j<MATSIZE;j++) {
        /*printf("iv_mat i=%d a=%d [%d] %f\n",
          i, ls_amount, i*MATSIZE+j, inv_mat[i*ls_amount+j]); */
        *(ptr++) = inv_mat[i*MATSIZE+j];
      }
    }
  }
  if (exist[ls_amount-1] == 1) {
    *(ptr++) = (MYFLT)sorted_lss[ls_amount-1]+1;
    *(ptr++) = (MYFLT)sorted_lss[0]+1;
    for (j=0;j<MATSIZE;j++) {
      /*         printf("iv_mat[%d] %f\n", (ls_amount-1)*MATSIZE+j, */
      /*                inv_mat[(ls_amount-1)*MATSIZE+j]); */
      *(ptr++) = inv_mat[(ls_amount-1)*MATSIZE+j];
    }
  }
  k=3;
  csound->Message(csound, "%s", Str("\nConfigured loudspeakers\n"));
  for (i=0; i < amount; i++) {
    csound->Message(csound, Str("Pair %d Loudspeakers: "), i);
    for (j=0; j < 2; j++) {
      csound->Message(csound, "%d ", (int32_t) ls_table[k++]);
    }

    csound->Message(csound, "\nMatrix ");
    for (j=0; j < MATSIZE; j++) {
      csound->Message(csound, "%f ", ls_table[k]);
      k++;
    }
    csound->Message(csound, "\n\n");
  }
  free(sorted_lss); free(exist); free(inv_mat);
}

static void sort_2D_lss(ls lss[], int32_t sorted_lss[],
                        int32_t ls_amount)
{
  int32_t i,j,index=-1;
  MYFLT tmp, tmp_azi;

  /* Transforming angles between -180 and 180 */
  for (i=0; i<ls_amount; i++) {
    angle_to_cart_II(&lss[i].angles, &lss[i].coords);
    lss[i].angles.azi = ACOS(lss[i].coords.x);
    if (FABS(lss[i].coords.y) <= FL(0.001))
      tmp = FL(1.0);
    else
      tmp = lss[i].coords.y / FABS(lss[i].coords.y);
    lss[i].angles.azi *= tmp;
    //printf("***tulos %f\n",    lss[i].angles.azi);
  }
  for (i=0;i<ls_amount;i++) {
    tmp = FL(2000.0);
    for (j=0; j<ls_amount;j++) {
      if (lss[j].angles.azi <= tmp) {
        tmp=lss[j].angles.azi;
        index = j;
      }
    }
    sorted_lss[i]=index;
    tmp_azi = (lss[index].angles.azi);
    lss[index].angles.azi = (tmp_azi + FL(4000.0));
  }
  for (i=0;i<ls_amount;i++) {
    tmp_azi = (lss[i].angles.azi);
    lss[i].angles.azi = (tmp_azi - FL(4000.0));
  }
}

int32_t calc_2D_inv_tmatrix(MYFLT azi1,MYFLT azi2, MYFLT inv_mat[MATSIZE])
{
  MYFLT x1,x2,x3,x4; /* x1 x3 */
  MYFLT det;
  x1 = COS(azi1 );
  x2 = SIN(azi1 );
  x3 = COS(azi2 );
  x4 = SIN(azi2 );
  det = (x1 * x4) - ( x3 * x2 );
  if (FABS(det) <= FL(0.001)) {
    //printf("unusable*** pair, det %f\n",det);
    inv_mat[0] = FL(0.0);
    inv_mat[1] = FL(0.0);
    inv_mat[2] = FL(0.0);
    inv_mat[3] = FL(0.0);
    return 0;
  }
  else {
    //printf("***inv x (%f,%f,%f,%f): det=%f\n", x4, -x3, -x2, x1, det);
    inv_mat[0] =  (x4 / det);
    inv_mat[1] =  (-x3 / det);
    inv_mat[2] =  (-x2 / det);
    inv_mat[3] =  (x1 / det);
    return 1;
  }
}

void new_spread_dir(CART_VEC *spreaddir, CART_VEC vscartdir,
                    CART_VEC spread_base, MYFLT azi, MYFLT spread)
{
  MYFLT beta,gamma;
  MYFLT a,b;
  MYFLT power;
  ANG_VEC tmp;
  gamma = ACOS(vscartdir.x * spread_base.x +
               vscartdir.y * spread_base.y +
               vscartdir.z * spread_base.z)/PI_F*FL(180.0);
  if (FABS(gamma) < FL(1.0)) {
    tmp.azi=azi+FL(90.0);
    tmp.ele=FL(0.0); tmp.length=FL(1.0);
    angle_to_cart(tmp, &spread_base);
    gamma = ACOS(vscartdir.x * spread_base.x +
                 vscartdir.y * spread_base.y +
                 vscartdir.z * spread_base.z)/PI_F*FL(180.0);
  }
  beta = FL(180.0) - gamma;
  b=SIN(spread * PI_F / FL(180.0)) /
    SIN(beta * PI_F / FL(180.0));
  a=SIN((FL(180.0)- spread - beta) * PI_F / FL(180.0)) /
    SIN (beta * PI_F / FL(180.0));
  spreaddir->x = a * vscartdir.x + b * spread_base.x;
  spreaddir->y = a * vscartdir.y + b * spread_base.y;
  spreaddir->z = a * vscartdir.z + b * spread_base.z;

  power=SQRT(spreaddir->x*spreaddir->x +
             spreaddir->y*spreaddir->y +
             spreaddir->z*spreaddir->z);
  spreaddir->x /= power;
  spreaddir->y /= power;
  spreaddir->z /= power;
}

void new_spread_base(CART_VEC spreaddir, CART_VEC vscartdir,
                     MYFLT spread, CART_VEC *spread_base)
{
  MYFLT d;
  MYFLT power;

  d = COS(spread/FL(180.0)*PI_F);
  spread_base->x = spreaddir.x - d * vscartdir.x;
  spread_base->y = spreaddir.y - d * vscartdir.y;
  spread_base->z = spreaddir.z - d * vscartdir.z;
  power=SQRT(spread_base->x*spread_base->x +
             spread_base->y*spread_base->y +
             spread_base->z*spread_base->z);
  spread_base->x /= power;
  spread_base->y /= power;
  spread_base->z /= power;
}


static int32_t vbap1_moving_control(CSOUND *, VBAP1_MOVE_DATA *, OPDS *, MYFLT,
                                    MYFLT, MYFLT, MYFLT**);
static int32_t vbap1_control(CSOUND *, VBAP1_DATA *, MYFLT*, MYFLT*, MYFLT*);

int32_t vbap1(CSOUND *csound, VBAP1 *p) /* during note performance: */
{
  int32_t j;
  int32_t cnt = p->q.number;
  vbap1_control(csound,&p->q, p->azi, p->ele, p->spread);

  /* write gains */
  for (j=0; j<cnt; j++) {
    *p->out_array[j] = p->q.gains[j];
  }
  return OK;
}

static int32_t vbap1_control(CSOUND *csound, VBAP1_DATA *p,
                             MYFLT* azi, MYFLT* ele, MYFLT* spread)
{
  CART_VEC spreaddir[16];
  CART_VEC spreadbase[16];
  ANG_VEC atmp;
  int32 i,j, spreaddirnum;
  int32_t cnt = p->number;
  MYFLT *tmp_gains=malloc(sizeof(MYFLT)*cnt),sum=FL(0.0);
  if (UNLIKELY(p->dim == 2 && fabs(*ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    *ele = FL(0.0);
  }

  if (*spread <FL(0.0))
    *spread = FL(0.0);
  else if (*spread >FL(100.0))
    *spread = FL(100.0);
  /* Current panning angles */
  p->ang_dir.azi = *azi;
  p->ang_dir.ele = *ele;
  p->ang_dir.length = FL(1.0);
  angle_to_cart(p->ang_dir, &(p->cart_dir));
  calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                p->gains, cnt, p->cart_dir);

  /* Calculated gain factors of a spreaded virtual source*/
  if (*spread > FL(0.0)) {
    if (p->dim == 3) {
      spreaddirnum = 16;
      /* four orthogonal dirs*/
      new_spread_dir(&spreaddir[0], p->cart_dir,
                     p->spread_base, *azi, *spread);
      new_spread_base(spreaddir[0], p->cart_dir,
                      *spread, &p->spread_base);
      cross_prod(p->spread_base, p->cart_dir, &spreadbase[1]);
      cross_prod(spreadbase[1], p->cart_dir, &spreadbase[2]);
      cross_prod(spreadbase[2], p->cart_dir, &spreadbase[3]);
      /* four between them*/
      vec_mean(p->spread_base, spreadbase[1], &spreadbase[4]);
      vec_mean(spreadbase[1], spreadbase[2], &spreadbase[5]);
      vec_mean(spreadbase[2], spreadbase[3], &spreadbase[6]);
      vec_mean(spreadbase[3], p->spread_base, &spreadbase[7]);

      /* four at half spreadangle*/
      vec_mean(p->cart_dir, p->spread_base, &spreadbase[8]);
      vec_mean(p->cart_dir, spreadbase[1], &spreadbase[9]);
      vec_mean(p->cart_dir, spreadbase[2], &spreadbase[10]);
      vec_mean(p->cart_dir, spreadbase[3], &spreadbase[11]);

      /* four at quarter spreadangle*/
      vec_mean(p->cart_dir, spreadbase[8], &spreadbase[12]);
      vec_mean(p->cart_dir, spreadbase[9], &spreadbase[13]);
      vec_mean(p->cart_dir, spreadbase[10], &spreadbase[14]);
      vec_mean(p->cart_dir, spreadbase[11], &spreadbase[15]);

      for (i=1;i<spreaddirnum;i++) {
        new_spread_dir(&spreaddir[i], p->cart_dir,
                       spreadbase[i],*azi,*spread);
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, cnt, spreaddir[i]);
        for (j=0;j<cnt;j++) {
          p->gains[j] += tmp_gains[j];
        }
      }
    }
    else if (p->dim == 2) {
      spreaddirnum = 6;
      atmp.ele = FL(0.0);
      atmp.azi = *azi - *spread;
      angle_to_cart(atmp, &spreaddir[0]);
      atmp.azi = *azi - *spread/2;
      angle_to_cart(atmp, &spreaddir[1]);
      atmp.azi = *azi - *spread/4;
      angle_to_cart(atmp, &spreaddir[2]);
      atmp.azi = *azi + *spread/4;
      angle_to_cart(atmp, &spreaddir[3]);
      atmp.azi = *azi + *spread/2;
      angle_to_cart(atmp, &spreaddir[4]);
      atmp.azi = *azi + *spread;
      angle_to_cart(atmp, &spreaddir[5]);

      for (i=0;i<spreaddirnum;i++) {
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, cnt, spreaddir[i]);
        for (j=0;j<cnt;j++) {
          p->gains[j] += tmp_gains[j];
        }
      }
    }
  }
  if (*spread > FL(70.0))
    for (i=0;i<cnt ;i++) {
      p->gains[i] +=(*spread - FL(70.0))/FL(30.0) *
        (*spread - FL(70.0))/FL(30.0)*FL(20.0);
    }

  /*normalization*/
  for (i=0;i<cnt;i++) {
    sum=sum+(p->gains[i]*p->gains[i]);
  }

  sum=SQRT(sum);
  for (i=0;i<cnt;i++) {
    p->gains[i] /= sum;
  }
  free(tmp_gains);
  return OK;
}

int32_t vbap1_init(CSOUND *csound, VBAP1 *p)
{                               /* Initializations before run time*/
  int32_t     i, j;
  MYFLT   *ls_table, *ptr;
  LS_SET  *ls_set_ptr;
  char name[24];
  snprintf(name, 24, "vbap_ls_table_%d", (int32_t)*p->layout);
  ls_table = (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, name));
  if (ls_table==NULL)
    return csound->InitError(csound,
                             Str("could not find layout table no.%d"),
                             (int32_t)*p->layout );
  p->q.number = p->OUTOCOUNT;
  p->q.dim       = (int32_t)ls_table[0];   /* reading in loudspeaker info */
  p->q.ls_am     = (int32_t)ls_table[1];
  p->q.ls_set_am = (int32_t)ls_table[2];
  ptr = &(ls_table[3]);
  if (!p->q.ls_set_am)
    return csound->InitError(csound, "%s", Str("vbap system NOT configured.\nMissing"
                                               " vbaplsinit opcode in orchestra?"));
  csound->AuxAlloc(csound, p->q.ls_set_am * sizeof (LS_SET), &p->q.aux);
  if (UNLIKELY(p->q.aux.auxp == NULL)) {
    return csound->InitError(csound, "%s", Str("could not allocate memory"));
  }
  p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
  ls_set_ptr = p->q.ls_sets;
  for (i=0; i < p->q.ls_set_am; i++) {
    ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
    for (j=0 ; j < p->q.dim ; j++) {
      ls_set_ptr[i].ls_nos[j] = (int32_t)*(ptr++);
    }
    memset(ls_set_ptr[i].ls_mx, '\0', 9*sizeof(MYFLT)); // initial setting
    /* for (j=0 ; j < 9; j++) */
    /*   ls_set_ptr[i].ls_mx[j] = FL(0.0);  /\*initial setting*\/ */
    for (j=0 ; j < (p->q.dim) * (p->q.dim); j++) {
      ls_set_ptr[i].ls_mx[j] = (MYFLT)*(ptr++);
    }
  }

  /* other initialization */
  if (UNLIKELY(p->q.dim == 2 && fabs(*p->ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    *p->ele = FL(0.0);
  }
  p->q.ang_dir.azi    = (MYFLT)*p->azi;
  p->q.ang_dir.ele    = (MYFLT)*p->ele;
  p->q.ang_dir.length = FL(1.0);
  angle_to_cart(p->q.ang_dir, &(p->q.cart_dir));
  p->q.spread_base.x  = p->q.cart_dir.y;
  p->q.spread_base.y  = p->q.cart_dir.z;
  p->q.spread_base.z  = -p->q.cart_dir.x;
  vbap1_control(csound,&p->q, p->azi, p->ele, p->spread);
  return OK;
}

int32_t vbap1a(CSOUND *csound, VBAPA1 *p) /* during note performance: */
{
  int32_t j;
  int32_t cnt = p->q.number;
  vbap1_control(csound,&p->q, p->azi, p->ele, p->spread);

  /* write gains */
  for (j=0; j<cnt; j++) {
    p->tabout->data[j] = p->q.gains[j];
  }
  return OK;
}

int32_t vbap1_init_a(CSOUND *csound, VBAPA1 *p)
{                               /* Initializations before run time*/
  int32_t     i, j;
  MYFLT   *ls_table, *ptr;
  LS_SET  *ls_set_ptr;
  char name[24];
  snprintf(name, 24, "vbap_ls_table_%d", (int32_t)*p->layout);
  ls_table = (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, name));
  if (ls_table==NULL)
    return csound->InitError(csound,
                             Str("could not find layout table no.%d"),
                             (int32_t)*p->layout );
  p->q.number = p->tabout->sizes[0];
  p->q.dim       = (int32_t)ls_table[0];   /* reading in loudspeaker info */
  p->q.ls_am     = (int32_t)ls_table[1];
  p->q.ls_set_am = (int32_t)ls_table[2];
  ptr = &(ls_table[3]);
  if (!p->q.ls_set_am)
    return csound->InitError(csound, "%s", Str("vbap system NOT configured.\nMissing"
                                               " vbaplsinit opcode in orchestra?"));
  csound->AuxAlloc(csound, p->q.ls_set_am * sizeof (LS_SET), &p->q.aux);
  if (UNLIKELY(p->q.aux.auxp == NULL)) {
    return csound->InitError(csound, "%s", Str("could not allocate memory"));
  }
  p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
  ls_set_ptr = p->q.ls_sets;
  for (i=0; i < p->q.ls_set_am; i++) {
    ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
    for (j=0 ; j < p->q.dim ; j++) {
      ls_set_ptr[i].ls_nos[j] = (int32_t)*(ptr++);
    }
    memset(ls_set_ptr[i].ls_mx, '\0', 9*sizeof(MYFLT));  /*initial setting*/
    for (j=0 ; j < (p->q.dim) * (p->q.dim); j++) {
      ls_set_ptr[i].ls_mx[j] = (MYFLT)*(ptr++);
    }
  }

  /* other initialization */
  if (UNLIKELY(p->q.dim == 2 && fabs(*p->ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    *p->ele = FL(0.0);
  }
  p->q.ang_dir.azi    = (MYFLT)*p->azi;
  p->q.ang_dir.ele    = (MYFLT)*p->ele;
  p->q.ang_dir.length = FL(1.0);
  angle_to_cart(p->q.ang_dir, &(p->q.cart_dir));
  p->q.spread_base.x  = p->q.cart_dir.y;
  p->q.spread_base.y  = p->q.cart_dir.z;
  p->q.spread_base.z  = -p->q.cart_dir.x;
  vbap1_control(csound,&p->q, p->azi, p->ele, p->spread);
  return OK;
}

int32_t vbap1_moving(CSOUND *csound, VBAP1_MOVING *p)
{                               /* during note performance:   */
  int32_t j;
  int32_t cnt = p->q.number;

  vbap1_moving_control(csound,&p->q, &(p->h), CS_ONEDKR,
                       *p->spread, *p->field_am, p->fld);

  /* write audio to resulting audio streams weighted
     with gain factors*/
  for (j=0; j<cnt ;j++) {
    *p->out_array[j] = p->q.gains[j];
  }
  return OK;
}

int32_t vbap1_moving_a(CSOUND *csound, VBAPA1_MOVING *p)
{                               /* during note performance:   */
  //    int32_t j;
  int32_t cnt = p->q.number;

  vbap1_moving_control(csound,&p->q, &(p->h), CS_ONEDKR,
                       *p->spread, *p->field_am, p->fld);

  /* write audio to resulting audio streams weighted
     with gain factors*/
  memcpy(p->tabout->data, p->q.gains, cnt*sizeof(MYFLT));
  /* for (j=0; j<cnt ;j++) { */
  /*   p->tabout->data[j] = p->q.gains[j]; */
  /* } */
  return OK;
}

static int32_t vbap1_moving_control(CSOUND *csound, VBAP1_MOVE_DATA *p,
                                    OPDS *h, MYFLT ONEDKR,
                                    MYFLT spread, MYFLT field_am, MYFLT **fld)
{
  CART_VEC spreaddir[16];
  CART_VEC spreadbase[16];
  ANG_VEC atmp;
  int32 i,j, spreaddirnum;
  CART_VEC tmp1, tmp2, tmp3;
  MYFLT coeff, angle;
  int32_t cnt = p->number;
  MYFLT *tmp_gains=malloc(sizeof(MYFLT)*cnt),sum=FL(0.0);
#ifdef JPFF
  printf("cnt=%d dim=%d\n", cnt, p->dim);
#endif
  if (UNLIKELY(p->dim == 2 && fabs(p->ang_dir.ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    p->ang_dir.ele = FL(0.0);
  }
  if (spread <FL(0.0))
    spread = FL(0.0);
  else if (spread >FL(100.0))
    spread = FL(100.0);
  if (p->point_change_counter++ >= p->point_change_interval) {
    p->point_change_counter = 0;
    p->curr_fld = p->next_fld;
    if (++p->next_fld >= (int32_t) fabs(field_am)) {
      if (field_am >= FL(0.0)) /* point-to-point */
        p->next_fld = 0;
      else
        p->next_fld = 1;
    }
    if (p->dim == 3) { /*jumping over second field */
      p->curr_fld = p->next_fld;
      if (++p->next_fld >= ((int32_t) fabs(field_am))) {
        if (field_am >= FL(0.0)) /* point-to-point */
          p->next_fld = 0;
        else
          p->next_fld = 1;
      }
    }
    if (UNLIKELY((fld[abs(p->next_fld)]==NULL))) {
      free(tmp_gains);
      return csound->PerfError(csound, h,
                               "%s", Str("Missing fields in vbapmove\n"));
    }
    if (field_am >= FL(0.0) && p->dim == 2) /* point-to-point */
      if (UNLIKELY(fabs(fabs(*fld[p->next_fld] -
                             *fld[p->curr_fld]) - 180.0) < 1.0))
        csound->Warning(csound,
                        "%s", Str("Warning: Ambiguous transition 180 degrees.\n"));
  }
  if (field_am >= FL(0.0)) { /* point-to-point */
    if (p->dim == 3) { /* 3-D*/
      p->prev_ang_dir.azi =  *fld[p->curr_fld-1];
      p->next_ang_dir.azi =  *fld[p->next_fld];
      p->prev_ang_dir.ele = *fld[p->curr_fld];
      p->next_ang_dir.ele = *fld[p->next_fld+1];
      coeff = ((MYFLT) p->point_change_counter) /
        ((MYFLT) p->point_change_interval);
      angle_to_cart( p->prev_ang_dir,&tmp1);
      angle_to_cart( p->next_ang_dir,&tmp2);
      tmp3.x = (FL(1.0)-coeff) * tmp1.x + coeff * tmp2.x;
      tmp3.y = (FL(1.0)-coeff) * tmp1.y + coeff * tmp2.y;
      tmp3.z = (FL(1.0)-coeff) * tmp1.z + coeff * tmp2.z;
      coeff = (MYFLT)sqrt((double)(tmp3.x * tmp3.x +
                                   tmp3.y * tmp3.y +
                                   tmp3.z * tmp3.z));
      tmp3.x /= coeff; tmp3.y /= coeff; tmp3.z /= coeff;
      cart_to_angle(tmp3,&(p->ang_dir));
    }
    else if (p->dim == 2) { /* 2-D */
      p->prev_ang_dir.azi =  *fld[p->curr_fld];
      p->next_ang_dir.azi =  *fld[p->next_fld ];
      p->prev_ang_dir.ele = p->next_ang_dir.ele =  FL(0.0);
      scale_angles(&(p->prev_ang_dir));
      scale_angles(&(p->next_ang_dir));
      angle = (p->prev_ang_dir.azi - p->next_ang_dir.azi);
      while (angle > FL(180.0))
        angle -= FL(360.0);
      while (angle < -FL(180.0))
        angle += FL(360.0);
      coeff = ((MYFLT) p->point_change_counter) /
        ((MYFLT) p->point_change_interval);
      angle  *=  (coeff);
      p->ang_dir.azi = p->prev_ang_dir.azi -  angle;
      p->ang_dir.ele = FL(0.0);
    }
    else {
      free(tmp_gains);
      return csound->PerfError(csound, h,
                               "%s", Str("Missing fields in vbapmove\n"));
    }
  }
  else { /* angular velocities */
    if (p->dim == 2) {
      p->ang_dir.azi =  p->ang_dir.azi +
        (*fld[p->next_fld] * ONEDKR);
      scale_angles(&(p->ang_dir));
    }
    else { /* 3D angular*/
      p->ang_dir.azi =  p->ang_dir.azi +
        (*fld[p->next_fld] * ONEDKR);
      p->ang_dir.ele =  p->ang_dir.ele +
        p->ele_vel * (*fld[p->next_fld+1] * ONEDKR);
      if (p->ang_dir.ele > FL(90.0)) {
        p->ang_dir.ele = FL(90.0);
        p->ele_vel = -p->ele_vel;
      }
      if (p->ang_dir.ele < FL(0.0)) {
        p->ang_dir.ele = FL(0.0);
        p->ele_vel =  -p->ele_vel;
      }
      scale_angles(&(p->ang_dir));
    }
  }
  angle_to_cart(p->ang_dir, &(p->cart_dir));
  calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                p->gains, cnt, p->cart_dir);
  if (spread > FL(0.0)) {
    if (p->dim == 3) {
      spreaddirnum=16;
      /* four orthogonal dirs*/
      new_spread_dir(&spreaddir[0], p->cart_dir,
                     p->spread_base, p->ang_dir.azi, spread);

      new_spread_base(spreaddir[0], p->cart_dir, spread, &p->spread_base);
      cross_prod(p->spread_base, p->cart_dir, &spreadbase[1]);
      cross_prod(spreadbase[1], p->cart_dir, &spreadbase[2]);
      cross_prod(spreadbase[2], p->cart_dir, &spreadbase[3]);
      /* four between them*/
      vec_mean(p->spread_base, spreadbase[1], &spreadbase[4]);
      vec_mean(spreadbase[1], spreadbase[2], &spreadbase[5]);
      vec_mean(spreadbase[2], spreadbase[3], &spreadbase[6]);
      vec_mean(spreadbase[3], p->spread_base, &spreadbase[7]);

      /* four at half spreadangle*/
      vec_mean(p->cart_dir, p->spread_base, &spreadbase[8]);
      vec_mean(p->cart_dir, spreadbase[1], &spreadbase[9]);
      vec_mean(p->cart_dir, spreadbase[2], &spreadbase[10]);
      vec_mean(p->cart_dir, spreadbase[3], &spreadbase[11]);

      /* four at quarter spreadangle*/
      vec_mean(p->cart_dir, spreadbase[8], &spreadbase[12]);
      vec_mean(p->cart_dir, spreadbase[9], &spreadbase[13]);
      vec_mean(p->cart_dir, spreadbase[10], &spreadbase[14]);
      vec_mean(p->cart_dir, spreadbase[11], &spreadbase[15]);

      for (i=1;i<spreaddirnum;i++) {
        new_spread_dir(&spreaddir[i], p->cart_dir,
                       spreadbase[i],p->ang_dir.azi,spread);
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, cnt, spreaddir[i]);
        for (j=0;j<cnt;j++) {
          p->gains[j] += tmp_gains[j];
        }
      }
    }
    else if (p->dim == 2) {
      spreaddirnum=6;
      atmp.ele = FL(0.0);
      atmp.azi = p->ang_dir.azi - spread;
      angle_to_cart(atmp, &spreaddir[0]);
      atmp.azi = p->ang_dir.azi - spread/2;
      angle_to_cart(atmp, &spreaddir[1]);
      atmp.azi = p->ang_dir.azi - spread/4;
      angle_to_cart(atmp, &spreaddir[2]);
      atmp.azi = p->ang_dir.azi + spread/4;
      angle_to_cart(atmp, &spreaddir[3]);
      atmp.azi = p->ang_dir.azi + spread/2;
      angle_to_cart(atmp, &spreaddir[4]);
      atmp.azi = p->ang_dir.azi + spread;
      angle_to_cart(atmp, &spreaddir[5]);

      for (i=0;i<spreaddirnum;i++) {
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, cnt, spreaddir[i]);
        for (j=0;j<cnt;j++) {
          p->gains[j] += tmp_gains[j];
        }
      }
    }
  }
  if (spread > FL(70.0))
    for (i=0;i<cnt ;i++) {
      p->gains[i] +=(spread - FL(70.0))/FL(30.0) *
        (spread - FL(70.0))/FL(30.0)*FL(10.0);
    }
  /*normalization*/
  for (i=0;i<cnt;i++) {
    sum=sum+(p->gains[i]*p->gains[i]);
  }

  sum=SQRT(sum);
  for (i=0;i<cnt;i++) {
    p->gains[i] /= sum;
  }
  free(tmp_gains);
  return OK;
}

int32_t vbap1_moving_init(CSOUND *csound, VBAP1_MOVING *p)
{
  int32_t     i, j;
  MYFLT   *ls_table, *ptr;
  LS_SET  *ls_set_ptr;

  p->q.number = p->OUTCOUNT;
  ls_table =
    (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, "vbap_ls_table_0"));
  /* reading in loudspeaker info */
  p->q.dim       = (int32_t)ls_table[0];
  p->q.ls_am     = (int32_t)ls_table[1];
  p->q.ls_set_am = (int32_t)ls_table[2];
  ptr = &(ls_table[3]);
  if (!p->q.ls_set_am)
    return csound->InitError(csound, "%s", Str("vbap system NOT configured.\n"
                                               "Missing vbaplsinit opcode"
                                               " in orchestra?"));
  csound->AuxAlloc(csound, p->q.ls_set_am * sizeof(LS_SET), &p->q.aux);
  if (UNLIKELY(p->q.aux.auxp == NULL)) {
    return csound->InitError(csound, "%s", Str("could not allocate memory"));
  }
  p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
  ls_set_ptr = p->q.ls_sets;
  for (i=0 ; i < p->q.ls_set_am ; i++) {
    ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
    for (j=0 ; j < p->q.dim ; j++) {
      ls_set_ptr[i].ls_nos[j] = (int32_t)*(ptr++);
    }
    for (j=0 ; j < 9; j++)
      ls_set_ptr[i].ls_mx[j] = FL(0.0);  /*initial setting*/
    for (j=0 ; j < (p->q.dim) * (p->q.dim); j++) {
      ls_set_ptr[i].ls_mx[j] = (MYFLT)*(ptr++);
    }
  }

  /* other initialization */
  p->q.ele_vel = FL(1.0);    /* functions specific to movement */
  if (UNLIKELY(fabs(*p->field_am) < (2+ (p->q.dim - 2)*2))) {
    return csound->InitError(csound,
                             Str("Have to have at least %d directions in vbapmove"),
                             2 + (p->q.dim - 2) * 2);
  }
  if (p->q.dim == 2)
    p->q.point_change_interval =
      (int32_t)(CS_EKR * *p->dur /(fabs(*p->field_am) - 1.0));
  else if (LIKELY(p->q.dim == 3))
    p->q.point_change_interval =
      (int32_t)(CS_EKR * *p->dur /(fabs(*p->field_am)*0.5 - 1.0));
  else
    return csound->InitError(csound, "%s", Str("Wrong dimension"));
  p->q.point_change_counter = 0;
  p->q.curr_fld = 0;
  p->q.next_fld = 1;
  p->q.ang_dir.azi = *p->fld[0];
  if (p->q.dim == 3) {
    p->q.ang_dir.ele = *p->fld[1];
  } else {
    p->q.ang_dir.ele = FL(0.0);
  }
  if (p->q.dim == 3) {
    p->q.curr_fld = 1;
    p->q.next_fld = 2;
  }
  angle_to_cart(p->q.ang_dir, &(p->q.cart_dir));
  p->q.spread_base.x  = p->q.cart_dir.y;
  p->q.spread_base.y  = p->q.cart_dir.z;
  p->q.spread_base.z  = -p->q.cart_dir.x;
  vbap1_moving_control(csound,&p->q, &(p->h), CS_ONEDKR,
                       *p->spread, *p->field_am, p->fld);
  return OK;
}

int32_t vbap1_moving_init_a(CSOUND *csound, VBAPA1_MOVING *p)
{
  int32_t     i, j;
  MYFLT   *ls_table, *ptr;
  LS_SET  *ls_set_ptr;

  if (UNLIKELY(p->tabout->data == NULL || p->tabout->dimensions!=1))
    return csound->InitError(csound, "%s", Str("Output array not initialised"));
  p->q.number = p->tabout->sizes[0];
  ls_table =
    (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, "vbap_ls_table_0"));
  /* reading in loudspeaker info */
  p->q.dim       = (int32_t)ls_table[0];
  p->q.ls_am     = (int32_t)ls_table[1];
  p->q.ls_set_am = (int32_t)ls_table[2];
  ptr = &(ls_table[3]);
  if (UNLIKELY(!p->q.ls_set_am))
    return csound->InitError(csound, "%s", Str("vbap system NOT configured.\n"
                                               "Missing vbaplsinit opcode"
                                               " in orchestra?"));
  csound->AuxAlloc(csound, p->q.ls_set_am * sizeof(LS_SET), &p->q.aux);
  if (UNLIKELY(p->q.aux.auxp == NULL)) {
    return csound->InitError(csound, "%s", Str("could not allocate memory"));
  }
  p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
  ls_set_ptr = p->q.ls_sets;
  for (i=0 ; i < p->q.ls_set_am ; i++) {
    ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
    for (j=0 ; j < p->q.dim ; j++) {
      ls_set_ptr[i].ls_nos[j] = (int32_t)*(ptr++);
    }
    for (j=0 ; j < 9; j++)
      ls_set_ptr[i].ls_mx[j] = FL(0.0);  /*initial setting*/
    for (j=0 ; j < (p->q.dim) * (p->q.dim); j++) {
      ls_set_ptr[i].ls_mx[j] = (MYFLT)*(ptr++);
    }
  }

  /* other initialization */
  p->q.ele_vel = FL(1.0);    /* functions specific to movement */
  if (UNLIKELY(fabs(*p->field_am) < (2+ (p->q.dim - 2)*2))) {
    return csound->InitError(csound,
                             Str("Have to have at least %d directions in vbapmove"),
                             2 + (p->q.dim - 2) * 2);
  }
  if (p->q.dim == 2)
    p->q.point_change_interval =
      (int32_t)(CS_EKR * *p->dur /(fabs(*p->field_am) - 1.0));
  else if (LIKELY(p->q.dim == 3))
    p->q.point_change_interval =
      (int32_t)(CS_EKR * *p->dur /(fabs(*p->field_am)*0.5 - 1.0));
  else
    return csound->InitError(csound, "%s", Str("Wrong dimension"));
  p->q.point_change_counter = 0;
  p->q.curr_fld = 0;
  p->q.next_fld = 1;
  p->q.ang_dir.azi = *p->fld[0];
  if (p->q.dim == 3) {
    p->q.ang_dir.ele = *p->fld[1];
  } else {
    p->q.ang_dir.ele = FL(0.0);
  }
  if (p->q.dim == 3) {
    p->q.curr_fld = 1;
    p->q.next_fld = 2;
  }
  angle_to_cart(p->q.ang_dir, &(p->q.cart_dir));
  p->q.spread_base.x  = p->q.cart_dir.y;
  p->q.spread_base.y  = p->q.cart_dir.z;
  p->q.spread_base.z  = -p->q.cart_dir.x;
  vbap1_moving_control(csound,&p->q, &(p->h), CS_ONEDKR,
                       *p->spread, *p->field_am, p->fld);
  return OK;
}

#include "arrays.h"

int32_t vbap_moving_control(CSOUND *, VBAP_MOVE_DATA *, OPDS*, MYFLT,
                            MYFLT *, MYFLT*,MYFLT**);

int32_t vbap(CSOUND *csound, VBAP *p) /* during note performance: */
{
  MYFLT *outptr, *inptr;
  MYFLT ogain, ngain, gainsubstr;
  MYFLT invfloatn;
  int32_t j;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  int32_t cnt = p->q.number;
  vbap_control(csound,&p->q, p->azi, p->ele, p->spread);
  for (j=0; j<cnt; j++) {
    p->q.beg_gains[j] = p->q.end_gains[j];
    p->q.end_gains[j] = p->q.updated_gains[j];
  }

  /* write audio to result audio streams weighted
     with gain factors*/
  if (UNLIKELY(early)) nsmps -= early;
  invfloatn =  FL(1.0)/(nsmps-offset);
  for (j=0; j<cnt; j++) {
    inptr      = p->audio;
    outptr     = p->out_array[j];
    ogain      = p->q.beg_gains[j];
    ngain      = p->q.end_gains[j];
    gainsubstr = ngain - ogain;
    if (UNLIKELY(offset)) memset(outptr, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) memset(&outptr[nsmps], '\0', early*sizeof(MYFLT));
    //printf("cnt%d: ngain=%lf ogain=%f\n", j, ngain, ogain);
    if (ngain != FL(0.0) || ogain != FL(0.0)) {
      if (ngain != ogain) {
        for (i = offset; i < nsmps; i++) {
          outptr[i] = inptr[i] *
            (ogain + (MYFLT)(i+1) * invfloatn * gainsubstr);
        }
        p->q.curr_gains[j]= ogain +
          (MYFLT)(i) * invfloatn * gainsubstr;
      }
      else {
        for (i=offset; i<nsmps; ++i) {
          outptr[i] = inptr[i] * ogain;
        }
      }
    }
    else {
      memset(outptr, 0, nsmps*sizeof(MYFLT));
    }
  }
  return OK;
}

int32_t vbap_a(CSOUND *csound, VBAPA *p) /* during note performance: */
{
  MYFLT *outptr, *inptr;
  MYFLT ogain, ngain, gainsubstr;
  MYFLT invfloatn;
  int32_t j;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  uint32_t ksmps = nsmps;
  int32_t cnt = p->q.number;

  vbap_control(csound,&p->q, p->azi, p->ele, p->spread);
  for (j=0; j<cnt; j++) {
    p->q.beg_gains[j] = p->q.end_gains[j];
    p->q.end_gains[j] = p->q.updated_gains[j];
  }

  /* write audio to result audio streams weighted
     with gain factors*/
  if (UNLIKELY(early)) nsmps -= early;
  invfloatn =  FL(1.0)/(nsmps-offset);
  for (j=0; j<cnt; j++) {
    outptr     = &p->tabout->data[j*ksmps];
    inptr      = p->audio;
    ogain      = p->q.beg_gains[j];
    ngain      = p->q.end_gains[j];
    gainsubstr = ngain - ogain;
    if (UNLIKELY(offset)) memset(outptr, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) memset(&outptr[nsmps], '\0', early*sizeof(MYFLT));
    if (ngain != FL(0.0) || ogain != FL(0.0)) {
      if (ngain != ogain) {
        for (i = offset; i < nsmps; i++) {
          outptr[i] = inptr[i] *
            (ogain + (MYFLT)(i+1) * invfloatn * gainsubstr);
        }
        p->q.curr_gains[j]= ogain +
          (MYFLT)(i) * invfloatn * gainsubstr;
      }
      else {
        for (i=offset; i<nsmps; ++i)
          outptr[i] = inptr[i] * ogain;
      }
    }
    else {
      memset(outptr, 0, nsmps*sizeof(MYFLT));
    }
  }
  return OK;
}

int32_t vbap_control(CSOUND *csound, VBAP_DATA *p,
                     MYFLT *azi, MYFLT *ele, MYFLT *spread)
{
  CART_VEC spreaddir[16];
  CART_VEC spreadbase[16];
  ANG_VEC atmp;
  int32 i,j, spreaddirnum;
  int32_t cnt = p->number;
  MYFLT *tmp_gains = malloc(sizeof(MYFLT)*cnt),sum=FL(0.0);
  if (UNLIKELY(p->dim == 2 && fabs(*ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    *ele = FL(0.0);
  }

  if (*spread <FL(0.0))
    *spread = FL(0.0);
  else if (*spread >FL(100.0))
    *spread = FL(100.0);
  /* Current panning angles */
  p->ang_dir.azi = (MYFLT) *azi;
  p->ang_dir.ele = (MYFLT) *ele;
  p->ang_dir.length = FL(1.0);
  angle_to_cart(p->ang_dir, &(p->cart_dir));
  calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                p->updated_gains, cnt, p->cart_dir);

  /* Calculated gain factors of a spreaded virtual source*/
  if (*spread > FL(0.0)) {
    if (p->dim == 3) {
      spreaddirnum = 16;
      /* four orthogonal dirs*/
      new_spread_dir(&spreaddir[0], p->cart_dir,
                     p->spread_base, *azi, *spread);
      new_spread_base(spreaddir[0], p->cart_dir,
                      *spread, &p->spread_base);
      cross_prod(p->spread_base, p->cart_dir, &spreadbase[1]);
      cross_prod(spreadbase[1], p->cart_dir, &spreadbase[2]);
      cross_prod(spreadbase[2], p->cart_dir, &spreadbase[3]);
      /* four between them*/
      vec_mean(p->spread_base, spreadbase[1], &spreadbase[4]);
      vec_mean(spreadbase[1], spreadbase[2], &spreadbase[5]);
      vec_mean(spreadbase[2], spreadbase[3], &spreadbase[6]);
      vec_mean(spreadbase[3], p->spread_base, &spreadbase[7]);

      /* four at half spreadangle*/
      vec_mean(p->cart_dir, p->spread_base, &spreadbase[8]);
      vec_mean(p->cart_dir, spreadbase[1], &spreadbase[9]);
      vec_mean(p->cart_dir, spreadbase[2], &spreadbase[10]);
      vec_mean(p->cart_dir, spreadbase[3], &spreadbase[11]);

      /* four at quarter spreadangle*/
      vec_mean(p->cart_dir, spreadbase[8], &spreadbase[12]);
      vec_mean(p->cart_dir, spreadbase[9], &spreadbase[13]);
      vec_mean(p->cart_dir, spreadbase[10], &spreadbase[14]);
      vec_mean(p->cart_dir, spreadbase[11], &spreadbase[15]);

      for (i=1;i<spreaddirnum;i++) {
        new_spread_dir(&spreaddir[i], p->cart_dir,
                       spreadbase[i],*azi,*spread);
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, cnt, spreaddir[i]);
        for (j=0;j<cnt;j++) {
          p->updated_gains[j] += tmp_gains[j];
        }
      }
    }
    else if (p->dim == 2) {
      spreaddirnum = 6;
      atmp.ele = FL(0.0);
      atmp.azi = *azi - *spread;
      angle_to_cart(atmp, &spreaddir[0]);
      atmp.azi = *azi - *spread/2;
      angle_to_cart(atmp, &spreaddir[1]);
      atmp.azi = *azi - *spread/4;
      angle_to_cart(atmp, &spreaddir[2]);
      atmp.azi = *azi + *spread/4;
      angle_to_cart(atmp, &spreaddir[3]);
      atmp.azi = *azi + *spread/2;
      angle_to_cart(atmp, &spreaddir[4]);
      atmp.azi = *azi + *spread;
      angle_to_cart(atmp, &spreaddir[5]);

      for (i=0;i<spreaddirnum;i++) {
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, cnt, spreaddir[i]);
        for (j=0;j<cnt;j++) {
          p->updated_gains[j] += tmp_gains[j];
        }
      }
    }
  }
  if (*spread > FL(70.0))
    for (i=0;i<cnt ;i++) {
      p->updated_gains[i] +=(*spread - FL(70.0))/FL(30.0) *
        (*spread - FL(70.0))/FL(30.0)*FL(20.0);
    }

  /*normalization*/
  for (i=0;i<cnt;i++) {
    sum += p->updated_gains[i]*p->updated_gains[i];
  }

  sum=SQRT(sum);
  for (i=0;i<cnt;i++) {
    p->updated_gains[i] /= sum;
  }
  free(tmp_gains);
  return OK;
}

int32_t vbap_init(CSOUND *csound, VBAP *p)
{                               /* Initializations before run time*/
  int32_t     i, j;
  MYFLT   *ls_table, *ptr;
  LS_SET  *ls_set_ptr;
  int32_t cnt = p->q.number = (int32_t)(p->OUTOCOUNT);
  char name[24];

  snprintf(name, 24, "vbap_ls_table_%d", (p->layout==NULL?0:(int32_t)*p->layout));
  ls_table = (MYFLT*) (csound->QueryGlobalVariable(csound, name));

  if (UNLIKELY(ls_table==NULL))
    return csound->InitError(csound,
                             Str("could not find layout table no.%d"),
                             (int32_t)*p->layout );

  p->q.dim       = (int32_t)ls_table[0];   /* reading in loudspeaker info */
  p->q.ls_am     = (int32_t)ls_table[1];
  p->q.ls_set_am = (int32_t)ls_table[2];
  ptr = &(ls_table[3]);
  if (UNLIKELY(!p->q.ls_set_am))
    return csound->InitError(csound,
                             "%s", Str("vbap system NOT configured.\nMissing"
                                       " vbaplsinit opcode in orchestra?"));
  csound->AuxAlloc(csound, p->q.ls_set_am * sizeof (LS_SET), &p->q.aux);
  if (UNLIKELY(p->q.aux.auxp == NULL)) {
    return csound->InitError(csound, "%s", Str("could not allocate memory"));
  }
  p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
  ls_set_ptr = p->q.ls_sets;
  for (i=0; i < p->q.ls_set_am; i++) {
    ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
    for (j=0 ; j < p->q.dim ; j++) {
      ls_set_ptr[i].ls_nos[j] = (int32_t)*(ptr++);
    }
    memset(ls_set_ptr[i].ls_mx, '\0', 9*sizeof(MYFLT)); // initial setting
    /* for (j=0 ; j < 9; j++) */
    /*   ls_set_ptr[i].ls_mx[j] = FL(0.0);  /\*initial setting*\/ */
    for (j=0 ; j < (p->q.dim) * (p->q.dim); j++) {
      ls_set_ptr[i].ls_mx[j] = (MYFLT)*(ptr++);
    }
  }

  /* other initialization */
  if (UNLIKELY(p->q.dim == 2 && fabs(p->ele==NULL?0:*p->ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    *p->ele = FL(0.0);
  }
  p->q.ang_dir.azi    = (MYFLT)*p->azi;
  p->q.ang_dir.ele    = (MYFLT)*p->ele;
  p->q.ang_dir.length = FL(1.0);
  angle_to_cart(p->q.ang_dir, &(p->q.cart_dir));
  p->q.spread_base.x  = p->q.cart_dir.y;
  p->q.spread_base.y  = p->q.cart_dir.z;
  p->q.spread_base.z  = -p->q.cart_dir.x;
  vbap_control(csound,&(p->q), p->azi, p->ele, p->spread);
  for (i=0;i<cnt;i++) {
    p->q.beg_gains[i] = p->q.updated_gains[i];
    p->q.end_gains[i] = p->q.updated_gains[i];
  }
  return OK;
}

int32_t vbap_init_a(CSOUND *csound, VBAPA *p)
{                               /* Initializations before run time*/
  int32_t     i, j;
  MYFLT   *ls_table, *ptr;
  LS_SET  *ls_set_ptr;
  int32_t cnt;
  char name[24];

  snprintf(name, 24, "vbap_ls_table_%d", (int32_t)*p->layout);
  ls_table = (MYFLT*) (csound->QueryGlobalVariable(csound, name));

  if (UNLIKELY(ls_table==NULL))
    return csound->InitError(csound,
                             Str("could not find layout table no.%d"),
                             (int32_t)*p->layout );

  p->q.dim       = (int32_t)ls_table[0];   /* reading in loudspeaker info */
  p->q.ls_am     = (int32_t)ls_table[1];
  p->q.ls_set_am = (int32_t)ls_table[2];
  ptr = &(ls_table[3]);
  if (UNLIKELY(!p->q.ls_set_am))
    return csound->InitError(csound,
                             "%s", Str("vbap system NOT configured.\nMissing"
                                       " vbaplsinit opcode in orchestra?"));
  //printf("**** size = %d\n", p->q.ls_set_am);
  tabinit(csound,  p->tabout, p->q.ls_set_am, &(p->h));
  cnt = p->q.number = p->tabout->sizes[0];
  csound->AuxAlloc(csound, p->q.ls_set_am * sizeof(LS_SET), &p->q.aux);
  if (UNLIKELY(p->q.aux.auxp == NULL)) {
    return csound->InitError(csound, "%s", Str("could not allocate memory"));
  }
  p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
  ls_set_ptr = p->q.ls_sets;
  for (i=0; i < p->q.ls_set_am; i++) {
    ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
    for (j=0 ; j < p->q.dim ; j++) {
      ls_set_ptr[i].ls_nos[j] = (int32_t)*(ptr++);
    }
    memset(ls_set_ptr[i].ls_mx, '\0', 9*sizeof(MYFLT));
    /* for (j=0 ; j < 9; j++) */
    /*   ls_set_ptr[i].ls_mx[j] = FL(0.0);  /\*initial setting*\/ */
    for (j=0 ; j < (p->q.dim) * (p->q.dim); j++) {
      ls_set_ptr[i].ls_mx[j] = (MYFLT)*(ptr++);
    }
  }

  /* other initialization */
  if (UNLIKELY(p->q.dim == 2 && fabs(*p->ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    *p->ele = FL(0.0);
  }
  p->q.ang_dir.azi    = *p->azi;
  p->q.ang_dir.ele    = *p->ele;
  p->q.ang_dir.length = FL(1.0);
  angle_to_cart(p->q.ang_dir, &(p->q.cart_dir));
  p->q.spread_base.x  = p->q.cart_dir.y;
  p->q.spread_base.y  = p->q.cart_dir.z;
  p->q.spread_base.z  = -p->q.cart_dir.x;
  vbap_control(csound,&(p->q), p->azi, p->ele, p->spread);
  for (i=0;i<cnt;i++) {
    p->q.beg_gains[i] = p->q.updated_gains[i];
    p->q.end_gains[i] = p->q.updated_gains[i];
  }
  return OK;
}

int32_t vbap_moving(CSOUND *csound, VBAP_MOVING *p)
{                               /* during note performance:   */
  MYFLT *outptr, *inptr;
  MYFLT ogain, ngain, gainsubstr;
  MYFLT invfloatn;
  int32_t j;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  int32_t cnt = p->q.number;

  vbap_moving_control(csound,&p->q, &(p->h), CS_ONEDKR,
                      p->spread, p->field_am, p->fld);
  //    vbap_moving_control(csound,p);
  for (j=0;j<cnt; j++) {
    p->q.beg_gains[j] = p->q.end_gains[j];
    p->q.end_gains[j] = p->q.updated_gains[j];
  }

  /* write audio to resulting audio streams weighted
     with gain factors*/
  if (UNLIKELY(early)) nsmps -= early;
  invfloatn = FL(1.0)/(nsmps-offset);
  for (j=0; j<cnt ;j++) {
    inptr = p->audio;
    outptr = p->out_array[j];
    if (UNLIKELY(offset)) memset(outptr, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) memset(&outptr[nsmps], '\0', early*sizeof(MYFLT));
    ogain  = p->q.beg_gains[j];
    ngain  = p->q.end_gains[j];
    gainsubstr = ngain - ogain;
    if (ngain != FL(0.0) || ogain != FL(0.0))
      if (ngain != ogain) {
        for (i = offset; i < nsmps; i++) {
          outptr[i] = inptr[i] *
            (ogain + (MYFLT)(i+1) * invfloatn * gainsubstr);
        }
        p->q.curr_gains[j]= ogain +
          (MYFLT)(i) * invfloatn * gainsubstr;
      }
      else
        for (i=offset; i<nsmps; ++i)
          outptr[i] = inptr[i] * ogain;
    else
      memset(outptr, 0, nsmps*sizeof(MYFLT));
  }
  return OK;
}

int32_t vbap_moving_control(CSOUND *csound, VBAP_MOVE_DATA *p, OPDS *h,
                            MYFLT ONEDKR, MYFLT* spread, MYFLT* field_am, MYFLT *fld[])
{
  CART_VEC spreaddir[16];
  CART_VEC spreadbase[16];
  ANG_VEC atmp;
  int32 i,j, spreaddirnum;
  CART_VEC tmp1, tmp2, tmp3;
  MYFLT coeff, angle;
  int32_t cnt = p->number;
  MYFLT *tmp_gains=malloc(sizeof(MYFLT)*cnt),sum=FL(0.0);

  if (UNLIKELY(p->dim == 2 && fabs(p->ang_dir.ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    p->ang_dir.ele = FL(0.0);
  }
  if (*spread <FL(0.0))
    *spread = FL(0.0);
  else if (*spread >FL(100.0))
    *spread = FL(100.0);
  if (p->point_change_counter++ >= p->point_change_interval) {
    p->point_change_counter = 0;
    p->curr_fld = p->next_fld;
    if (++p->next_fld >= (int32_t) fabs(*field_am)) {
      if (*field_am >= FL(0.0)) /* point-to-point */
        p->next_fld = 0;
      else
        p->next_fld = 1;
    }
    if (p->dim == 3) { /*jumping over second field */
      p->curr_fld = p->next_fld;
      if (++p->next_fld >= ((int32_t) fabs(*field_am))) {
        if (*field_am >= FL(0.0)) /* point-to-point */
          p->next_fld = 0;
        else
          p->next_fld = 1;
      }
    }
    if (UNLIKELY((fld[abs(p->next_fld)]==NULL))) {
      free(tmp_gains);
      return csound->PerfError(csound, h,
                               "%s", Str("Missing fields in vbapmove\n"));
    }
    if (*field_am >= FL(0.0) && p->dim == 2) /* point-to-point */
      if (UNLIKELY(fabs(fabs(*fld[p->next_fld] -
                             *fld[p->curr_fld]) - 180.0) < 1.0))
        csound->Warning(csound,
                        "%s", Str("Warning: Ambiguous transition 180 degrees.\n"));
  }
  if (*field_am >= FL(0.0)) { /* point-to-point */
    if (p->dim == 3) { /* 3-D*/
      p->prev_ang_dir.azi =  *fld[p->curr_fld-1];
      p->next_ang_dir.azi =  *fld[p->next_fld];
      p->prev_ang_dir.ele = *fld[p->curr_fld];
      p->next_ang_dir.ele = *fld[p->next_fld+1];
      coeff = ((MYFLT) p->point_change_counter) /
        ((MYFLT) p->point_change_interval);
      angle_to_cart( p->prev_ang_dir,&tmp1);
      angle_to_cart( p->next_ang_dir,&tmp2);
      tmp3.x = (FL(1.0)-coeff) * tmp1.x + coeff * tmp2.x;
      tmp3.y = (FL(1.0)-coeff) * tmp1.y + coeff * tmp2.y;
      tmp3.z = (FL(1.0)-coeff) * tmp1.z + coeff * tmp2.z;
      coeff = (MYFLT)sqrt((double)(tmp3.x * tmp3.x +
                                   tmp3.y * tmp3.y +
                                   tmp3.z * tmp3.z));
      tmp3.x /= coeff; tmp3.y /= coeff; tmp3.z /= coeff;
      cart_to_angle(tmp3,&(p->ang_dir));
    }
    else if (LIKELY(p->dim == 2)) { /* 2-D */
      p->prev_ang_dir.azi =  *fld[p->curr_fld];
      p->next_ang_dir.azi =  *fld[p->next_fld ];
      p->prev_ang_dir.ele = p->next_ang_dir.ele =  FL(0.0);
      scale_angles(&(p->prev_ang_dir));
      scale_angles(&(p->next_ang_dir));
      angle = (p->prev_ang_dir.azi - p->next_ang_dir.azi);
      while (angle > FL(180.0))
        angle -= FL(360.0);
      while (angle < -FL(180.0))
        angle += FL(360.0);
      coeff = ((MYFLT) p->point_change_counter) /
        ((MYFLT) p->point_change_interval);
      angle  *=  (coeff);
      p->ang_dir.azi = p->prev_ang_dir.azi -  angle;
      p->ang_dir.ele = FL(0.0);
    }
    else {
      free(tmp_gains);
      return csound->PerfError(csound, h,
                               "%s", Str("Missing fields in vbapmove\n"));
    }
  }
  else { /* angular velocities */
    if (p->dim == 2) {
      p->ang_dir.azi =  p->ang_dir.azi +
        (*fld[p->next_fld] * ONEDKR);
      scale_angles(&(p->ang_dir));
    }
    else { /* 3D angular*/
      p->ang_dir.azi =  p->ang_dir.azi +
        (*fld[p->next_fld] * ONEDKR);
      p->ang_dir.ele =  p->ang_dir.ele +
        p->ele_vel * (*fld[p->next_fld+1] * ONEDKR);
      if (p->ang_dir.ele > FL(90.0)) {
        p->ang_dir.ele = FL(90.0);
        p->ele_vel = -p->ele_vel;
      }
      if (p->ang_dir.ele < FL(0.0)) {
        p->ang_dir.ele = FL(0.0);
        p->ele_vel =  -p->ele_vel;
      }
      scale_angles(&(p->ang_dir));
    }
  }
  angle_to_cart(p->ang_dir, &(p->cart_dir));
  calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                p->updated_gains, cnt, p->cart_dir);
  if (*spread > FL(0.0)) {
    if (p->dim == 3) {
      spreaddirnum=16;
      /* four orthogonal dirs*/
      new_spread_dir(&spreaddir[0], p->cart_dir,
                     p->spread_base, p->ang_dir.azi, *spread);

      new_spread_base(spreaddir[0], p->cart_dir,*spread, &p->spread_base);
      cross_prod(p->spread_base, p->cart_dir, &spreadbase[1]);
      cross_prod(spreadbase[1], p->cart_dir, &spreadbase[2]);
      cross_prod(spreadbase[2], p->cart_dir, &spreadbase[3]);
      /* four between them*/
      vec_mean(p->spread_base, spreadbase[1], &spreadbase[4]);
      vec_mean(spreadbase[1], spreadbase[2], &spreadbase[5]);
      vec_mean(spreadbase[2], spreadbase[3], &spreadbase[6]);
      vec_mean(spreadbase[3], p->spread_base, &spreadbase[7]);

      /* four at half spreadangle*/
      vec_mean(p->cart_dir, p->spread_base, &spreadbase[8]);
      vec_mean(p->cart_dir, spreadbase[1], &spreadbase[9]);
      vec_mean(p->cart_dir, spreadbase[2], &spreadbase[10]);
      vec_mean(p->cart_dir, spreadbase[3], &spreadbase[11]);

      /* four at quarter spreadangle*/
      vec_mean(p->cart_dir, spreadbase[8], &spreadbase[12]);
      vec_mean(p->cart_dir, spreadbase[9], &spreadbase[13]);
      vec_mean(p->cart_dir, spreadbase[10], &spreadbase[14]);
      vec_mean(p->cart_dir, spreadbase[11], &spreadbase[15]);

      for (i=1;i<spreaddirnum;i++) {
        new_spread_dir(&spreaddir[i], p->cart_dir,
                       spreadbase[i],p->ang_dir.azi,*spread);
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, cnt, spreaddir[i]);
        for (j=0;j<cnt;j++) {
          p->updated_gains[j] += tmp_gains[j];
        }
      }
    }
    else if (p->dim == 2) {
      spreaddirnum=6;
      atmp.ele = FL(0.0);
      atmp.azi = p->ang_dir.azi - *spread;
      angle_to_cart(atmp, &spreaddir[0]);
      atmp.azi = p->ang_dir.azi - *spread/2;
      angle_to_cart(atmp, &spreaddir[1]);
      atmp.azi = p->ang_dir.azi - *spread/4;
      angle_to_cart(atmp, &spreaddir[2]);
      atmp.azi = p->ang_dir.azi + *spread/4;
      angle_to_cart(atmp, &spreaddir[3]);
      atmp.azi = p->ang_dir.azi + *spread/2;
      angle_to_cart(atmp, &spreaddir[4]);
      atmp.azi = p->ang_dir.azi + *spread;
      angle_to_cart(atmp, &spreaddir[5]);

      for (i=0;i<spreaddirnum;i++) {
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, cnt, spreaddir[i]);
        for (j=0;j<cnt;j++) {
          p->updated_gains[j] += tmp_gains[j];
        }
      }
    }
  }
  if (*spread > FL(70.0))
    for (i=0;i<cnt ;i++) {
      p->updated_gains[i] +=(*spread - FL(70.0))/FL(30.0) *
        (*spread - FL(70.0))/FL(30.0)*FL(10.0);
    }
  /*normalization*/
  for (i=0;i<cnt;i++) {
    sum=sum+(p->updated_gains[i]*p->updated_gains[i]);
  }

  sum=SQRT(sum);
  for (i=0;i<cnt;i++) {
    p->updated_gains[i] /= sum;
  }
  free(tmp_gains);
  return OK;
}

int32_t vbap_moving_init(CSOUND *csound, VBAP_MOVING *p)
{
  int32_t     i, j;
  MYFLT   *ls_table, *ptr;
  LS_SET  *ls_set_ptr;
  int32_t cnt = (int32_t)p->h.optext->t.outArgCount;
  if ((!strncmp(p->h.optext->t.opcod, "vbapmove", 8)) == 0) {
    p->audio = p->out_array[cnt];
    p->dur = p->out_array[cnt+1];
    p->spread = p->out_array[cnt+2];
    p->field_am = p->out_array[cnt+3];
    memcpy(p->fld, &(p->out_array[cnt+4]),
           sizeof(MYFLT *)*(p->h.optext->t.inArgCount-4));
  }

  ls_table = (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound,
                                                          "vbap_ls_table_0"));
  if (UNLIKELY(ls_table==NULL))
    return csound->InitError(csound, "%s", Str("could not find layout table no.0"));
  p->q.number = cnt;
  /* reading in loudspeaker info */
  p->q.dim       = (int32_t)ls_table[0];
  p->q.ls_am     = (int32_t)ls_table[1];
  p->q.ls_set_am = (int32_t)ls_table[2];
  ptr = &(ls_table[3]);
  if (UNLIKELY(!p->q.ls_set_am))
    return csound->InitError(csound, "%s", Str("vbap system NOT configured.\nMissing"
                                               " vbaplsinit opcode in orchestra?"));
  csound->AuxAlloc(csound, p->q.ls_set_am * sizeof(LS_SET), &p->q.aux);
  if (UNLIKELY(p->q.aux.auxp == NULL)) {
    return csound->InitError(csound, "%s", Str("could not allocate memory"));
  }
  p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
  ls_set_ptr = p->q.ls_sets;
  for (i=0 ; i < p->q.ls_set_am ; i++) {
    ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
    for (j=0 ; j < p->q.dim ; j++) {
      ls_set_ptr[i].ls_nos[j] = (int32_t)*(ptr++);
    }
    for (j=0 ; j < 9; j++)
      ls_set_ptr[i].ls_mx[j] = FL(0.0);  /*initial setting*/
    for (j=0 ; j < (p->q.dim) * (p->q.dim); j++) {
      ls_set_ptr[i].ls_mx[j] = (MYFLT)*(ptr++);
    }
  }

  /* other initialization */
  p->q.ele_vel = FL(1.0);    /* functions specific to movement */
  if (UNLIKELY(fabs(*p->field_am) < (2+ (p->q.dim - 2)*2))) {
    return csound->InitError(csound,
                             Str("Have to have at least %d directions in vbapmove"),
                             2 + (p->q.dim - 2) * 2);
  }
  if (p->q.dim == 2)
    p->q.point_change_interval =
      (int32_t)(CS_EKR * *p->dur /(fabs(*p->field_am) - 1.0));
  else if (LIKELY(p->q.dim == 3))
    p->q.point_change_interval =
      (int32_t)(CS_EKR * *p->dur /(fabs(*p->field_am)*0.5 - 1.0));
  else
    return csound->InitError(csound, "%s", Str("Wrong dimension"));
  p->q.point_change_counter = 0;
  p->q.curr_fld = 0;
  p->q.next_fld = 1;
  p->q.ang_dir.azi = *p->fld[0];
  if (p->q.dim == 3) {
    p->q.ang_dir.ele = *p->fld[1];
  } else {
    p->q.ang_dir.ele = FL(0.0);
  }
  if (p->q.dim == 3) {
    p->q.curr_fld = 1;
    p->q.next_fld = 2;
  }
  angle_to_cart(p->q.ang_dir, &(p->q.cart_dir));
  p->q.spread_base.x  = p->q.cart_dir.y;
  p->q.spread_base.y  = p->q.cart_dir.z;
  p->q.spread_base.z  = -p->q.cart_dir.x;
  vbap_moving_control(csound,&p->q, &(p->h), CS_ONEDKR,
                      p->spread, p->field_am, p->fld);
  for (i = 0; i<cnt; i++) {
    p->q.beg_gains[i] = p->q.updated_gains[i];
    p->q.end_gains[i] = p->q.updated_gains[i];
  }
  return OK;
}

int32_t vbap_moving_a(CSOUND *csound, VBAPA_MOVING *p)
{                               /* during note performance:   */
  MYFLT *outptr, *inptr;
  MYFLT ogain, ngain, gainsubstr;
  MYFLT invfloatn;
  int32_t j;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  uint32_t ksmps = nsmps;
  int32_t cnt = p->q.number;

  vbap_moving_control(csound,&p->q, &(p->h), CS_ONEDKR,
                      p->spread, p->field_am, p->fld);
  //    vbap_moving_control(csound,p);
  for (j=0;j<cnt; j++) {
    p->q.beg_gains[j] = p->q.end_gains[j];
    p->q.end_gains[j] = p->q.updated_gains[j];
  }

  /* write audio to resulting audio streams weighted
     with gain factors*/
  if (UNLIKELY(early)) nsmps -= early;
  invfloatn = FL(1.0)/(nsmps-offset);
  //outptr = p->tabout->data;
  for (j=0; j<cnt ;j++) {
    inptr = p->audio;
    outptr = &p->tabout->data[j*ksmps];
    if (UNLIKELY(offset)) memset(outptr, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) memset(&outptr[nsmps], '\0', early*sizeof(MYFLT));
    ogain  = p->q.beg_gains[j];
    ngain  = p->q.end_gains[j];
    gainsubstr = ngain - ogain;
    if (ngain != FL(0.0) || ogain != FL(0.0))
      if (ngain != ogain) {
        for (i = offset; i < nsmps; i++) {
          outptr[i] = inptr[i] *
            (ogain + (MYFLT)(i+1) * invfloatn * gainsubstr);
        }
        p->q.curr_gains[j]= ogain +
          (MYFLT)(i) * invfloatn * gainsubstr;
      }
      else
        for (i=offset; i<nsmps; ++i)
          outptr[i] = inptr[i] * ogain;
    else
      memset(outptr, 0, nsmps*sizeof(MYFLT));
  }
  return OK;
}

int32_t vbap_moving_init_a(CSOUND *csound, VBAPA_MOVING *p)
{
  int32_t     i, j;
  MYFLT   *ls_table, *ptr;
  LS_SET  *ls_set_ptr;
  int32_t cnt;

  if (UNLIKELY(p->tabout->data==NULL)) {
    return csound->InitError(csound,
                             "%s", Str("Output array in vpabmove not initialised"));
  }
  cnt = p->tabout->sizes[0];

  ls_table = (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound,
                                                          "vbap_ls_table_0"));
  if (UNLIKELY(ls_table==NULL))
    return csound->InitError(csound, "%s", Str("could not find layout table no.0"));
  p->q.number = cnt;
  /* reading in loudspeaker info */
  p->q.dim       = (int32_t)ls_table[0];
  p->q.ls_am     = (int32_t)ls_table[1];
  p->q.ls_set_am = (int32_t)ls_table[2];
  ptr = &(ls_table[3]);
  if (UNLIKELY(!p->q.ls_set_am))
    return csound->InitError(csound,
                             "%s", Str("vbap system NOT configured.\nMissing"
                                       " vbaplsinit opcode in orchestra?"));
  csound->AuxAlloc(csound, p->q.ls_set_am * sizeof(LS_SET), &p->q.aux);
  if (UNLIKELY(p->q.aux.auxp == NULL)) {
    return csound->InitError(csound, "%s", Str("could not allocate memory"));
  }
  p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
  ls_set_ptr = p->q.ls_sets;
  for (i=0 ; i < p->q.ls_set_am ; i++) {
    ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
    for (j=0 ; j < p->q.dim ; j++) {
      ls_set_ptr[i].ls_nos[j] = (int32_t)*(ptr++);
    }
    memset(ls_set_ptr[i].ls_mx, '\0', 9*sizeof(MYFLT));
    /* for (j=0 ; j < 9; j++) */
    /*   ls_set_ptr[i].ls_mx[j] = FL(0.0);  /\*initial setting*\/ */
    for (j=0 ; j < (p->q.dim) * (p->q.dim); j++) {
      ls_set_ptr[i].ls_mx[j] = (MYFLT)*(ptr++);
    }
  }

  /* other initialization */
  p->q.ele_vel = FL(1.0);    /* functions specific to movement */
  if (UNLIKELY(fabs(*p->field_am) < (2+ (p->q.dim - 2)*2))) {
    return csound->InitError(csound,
                             Str("Have to have at least %d directions in vbapmove"),
                             2 + (p->q.dim - 2) * 2);
  }
  if (p->q.dim == 2)
    p->q.point_change_interval =
      (int32_t)(CS_EKR * *p->dur /(fabs(*p->field_am) - 1.0));
  else if (LIKELY(p->q.dim == 3))
    p->q.point_change_interval =
      (int32_t)(CS_EKR * *p->dur /(fabs(*p->field_am)*0.5 - 1.0));
  else
    return csound->InitError(csound, "%s", Str("Wrong dimension"));
  p->q.point_change_counter = 0;
  p->q.curr_fld = 0;
  p->q.next_fld = 1;
  p->q.ang_dir.azi = *p->fld[0];
  if (p->q.dim == 3) {
    p->q.ang_dir.ele = *p->fld[1];
  } else {
    p->q.ang_dir.ele = FL(0.0);
  }
  if (p->q.dim == 3) {
    p->q.curr_fld = 1;
    p->q.next_fld = 2;
  }
  angle_to_cart(p->q.ang_dir, &(p->q.cart_dir));
  p->q.spread_base.x  = p->q.cart_dir.y;
  p->q.spread_base.y  = p->q.cart_dir.z;
  p->q.spread_base.z  = -p->q.cart_dir.x;
  vbap_moving_control(csound,&p->q, &(p->h), CS_ONEDKR,
                      p->spread, p->field_am, p->fld);
  for (i = 0; i<cnt; i++) {
    p->q.beg_gains[i] = p->q.updated_gains[i];
    p->q.end_gains[i] = p->q.updated_gains[i];
  }
  return OK;
}

int32_t vbap_zak_moving_control(CSOUND *, VBAP_ZAK_MOVING *);
int32_t vbap_zak_control(CSOUND *,VBAP_ZAK *);

int32_t vbap_zak(CSOUND *csound, VBAP_ZAK *p)   /* during note performance: */
{
  MYFLT *outptr, *inptr;
  MYFLT ogain, ngain, gainsubstr;
  MYFLT invfloatn;
  int32_t j;
  int32_t n = p->n;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;

  vbap_zak_control(csound,p);
  for (j=0; j<n; j++) {
    p->beg_gains[j] = p->end_gains[j];
    p->end_gains[j] = p->updated_gains[j];
  }

  /* write audio to result audio streams weighted
     with gain factors */
  outptr = p->out_array;
  if (UNLIKELY(early)) nsmps -= early;
  invfloatn =  FL(1.0)/(nsmps-offset);
  for (j=0; j<n; j++) {
    inptr = p->audio;
    ogain = p->beg_gains[j];
    ngain = p->end_gains[j];
    gainsubstr = ngain - ogain;
    if (UNLIKELY(offset)) memset(outptr, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) memset(&outptr[nsmps], '\0', early*sizeof(MYFLT));
    if (ngain != FL(0.0) || ogain != FL(0.0))
      if (ngain != ogain) {
        for (i = offset; i < nsmps; i++) {
          outptr[i] = inptr[i] *
            (ogain + (MYFLT) (i+1) * invfloatn * gainsubstr);
        }
        p->curr_gains[j]= ogain +
          (MYFLT) (i) * invfloatn * gainsubstr;
      }
      else {
        for (i=offset; i<nsmps; ++i)
          outptr[i] = inptr[i] * ogain;
      }
    else
      memset(outptr, 0, nsmps*sizeof(MYFLT));
    outptr += nsmps;
  }
  return OK;
}

int32_t vbap_zak_control(CSOUND *csound, VBAP_ZAK *p)
{
  CART_VEC spreaddir[16];
  CART_VEC spreadbase[16];
  ANG_VEC atmp;
  int32 i,j, spreaddirnum;
  int32_t n = p->n;
  MYFLT tmp_gains[MAXCHNLS],sum = FL(0.0);
  if (UNLIKELY(p->dim == 2 && fabs(*p->ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    *p->ele = FL(0.0);
  }
  if (*p->spread <FL(0.0))
    *p->spread=FL(0.0);
  else if (*p->spread >FL(100.0))
    *p->spread=FL(100.0);
  /* Current panning angles */
  p->ang_dir.azi = (MYFLT) *p->azi;
  p->ang_dir.ele = (MYFLT) *p->ele;
  p->ang_dir.length = FL(1.0);
  angle_to_cart(p->ang_dir, &(p->cart_dir));
  calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                p->updated_gains, n, p->cart_dir);

  /* Calculated gain factors of a spreaded virtual source */
  if (*p->spread > FL(0.0)) {
    if (p->dim == 3) {
      spreaddirnum=16;
      /* four orthogonal dirs */
      new_spread_dir(&spreaddir[0], p->cart_dir,
                     p->spread_base, *p->azi, *p->spread);
      new_spread_base(spreaddir[0], p->cart_dir,*p->spread, &p->spread_base);
      cross_prod(p->spread_base, p->cart_dir, &spreadbase[1]);
      cross_prod(spreadbase[1], p->cart_dir, &spreadbase[2]);
      cross_prod(spreadbase[2], p->cart_dir, &spreadbase[3]);
      /* four between them */
      vec_mean(p->spread_base, spreadbase[1], &spreadbase[4]);
      vec_mean(spreadbase[1], spreadbase[2], &spreadbase[5]);
      vec_mean(spreadbase[2], spreadbase[3], &spreadbase[6]);
      vec_mean(spreadbase[3], p->spread_base, &spreadbase[7]);

      /* four at half spreadangle */
      vec_mean(p->cart_dir, p->spread_base, &spreadbase[8]);
      vec_mean(p->cart_dir, spreadbase[1], &spreadbase[9]);
      vec_mean(p->cart_dir, spreadbase[2], &spreadbase[10]);
      vec_mean(p->cart_dir, spreadbase[3], &spreadbase[11]);

      /* four at quarter spreadangle */
      vec_mean(p->cart_dir, spreadbase[8], &spreadbase[12]);
      vec_mean(p->cart_dir, spreadbase[9], &spreadbase[13]);
      vec_mean(p->cart_dir, spreadbase[10], &spreadbase[14]);
      vec_mean(p->cart_dir, spreadbase[11], &spreadbase[15]);

      for (i=1;i<spreaddirnum;i++) {
        new_spread_dir(&spreaddir[i], p->cart_dir,
                       spreadbase[i],*p->azi,*p->spread);
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, n, spreaddir[i]);
        for (j=0;j<n;j++) {
          p->updated_gains[j] += tmp_gains[j];
        }
      }
    }
    else if (p->dim == 2) {
      spreaddirnum = 6;
      atmp.ele = FL(0.0);
      atmp.azi = *p->azi - *p->spread;
      angle_to_cart(atmp, &spreaddir[0]);
      atmp.azi = *p->azi - *p->spread/2;
      angle_to_cart(atmp, &spreaddir[1]);
      atmp.azi = *p->azi - *p->spread/4;
      angle_to_cart(atmp, &spreaddir[2]);
      atmp.azi = *p->azi + *p->spread/4;
      angle_to_cart(atmp, &spreaddir[3]);
      atmp.azi = *p->azi + *p->spread/2;
      angle_to_cart(atmp, &spreaddir[4]);
      atmp.azi = *p->azi + *p->spread;
      angle_to_cart(atmp, &spreaddir[5]);

      for (i=0;i<spreaddirnum;i++) {
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, n, spreaddir[i]);
        for (j=0;j<n;j++) {
          p->updated_gains[j] += tmp_gains[j];
        }
      }
    }
  }
  if (*p->spread > FL(70.0))
    for (i=0;i<n ;i++) {
      p->updated_gains[i] +=(*p->spread - FL(70.0))/FL(30.0) *
        (*p->spread - FL(70.0))/FL(30.0)*FL(20.0);
    }

  /* normalization */
  for (i=0;i<n;i++) {
    sum = sum+(p->updated_gains[i]*p->updated_gains[i]);
  }

  sum = SQRT(sum);
  for (i=0;i<n;i++) {
    p->updated_gains[i] /= sum;
  }
  return OK;
}

int32_t vbap_zak_init(CSOUND *csound, VBAP_ZAK *p)
{                               /* Initializations before run time */
  int32_t     i, j, indx;
  MYFLT   *ls_table, *ptr; /* , *gains; */
  LS_SET  *ls_set_ptr;
  int32_t n = p->n = (int32_t)MYFLT2LONG(*p->numb); /* Set size */
  char name[24];
  /* Check to see this index is within the limits of za space.    */
  MYFLT* zastart;
  int64_t zalast = GetZaBounds(csound, &zastart);
  indx = (int32) *p->ndx;
  if (UNLIKELY(indx > zalast)) {
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("outz index > isizea. No output"));
  }
  else if (UNLIKELY(indx < 0)) {
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("outz index < 0. No output."));
  }
  if ((int32_t)*p->layout==0) strcpy(name, "vbap_ls_table");
  else snprintf(name, 24, "vbap_ls_table_%d", (int32_t)*p->layout==0);
  /* Now read from the array in za space and write to the output. */
  p->out_array     = zastart + (indx * CS_KSMPS);/* outputs */
  csound->AuxAlloc(csound, p->n*sizeof(MYFLT)*4, &p->auxch);
  p->curr_gains    = (MYFLT*)p->auxch.auxp;
  p->beg_gains     = p->curr_gains + p->n;
  p->end_gains     = p->beg_gains + p->n;
  p->updated_gains = p->end_gains + p->n;
  ls_table = (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, name));
  p->dim           = (int32_t) ls_table[0];   /* reading in loudspeaker info */
  p->ls_am         = (int32_t) ls_table[1];
  p->ls_set_am     = (int32_t) ls_table[2];
  ptr              = &(ls_table[3]);
  csound->AuxAlloc(csound, p->ls_set_am * sizeof (LS_SET), &p->aux);
  if (UNLIKELY(p->aux.auxp == NULL)) {
    return csound->InitError(csound, "%s", Str("could not allocate memory"));
  }
  p->ls_sets = (LS_SET*) p->aux.auxp;
  ls_set_ptr = p->ls_sets;
  for (i=0 ; i < p->ls_set_am ; i++) {
    ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
    for (j=0 ; j < p->dim ; j++) {
      ls_set_ptr[i].ls_nos[j] = (int32_t) *(ptr++);
    }
    for (j=0 ; j < 9; j++)
      ls_set_ptr[i].ls_mx[j] = FL(0.0);  /* initial setting */
    for (j=0 ; j < (p->dim) * (p->dim); j++) {
      ls_set_ptr[i].ls_mx[j] = (MYFLT) *(ptr++);
    }
  }

  /* other initialization */
  if (UNLIKELY(p->dim == 2 && fabs(*p->ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    *p->ele = FL(0.0);
  }
  p->ang_dir.azi = (MYFLT) *p->azi;
  p->ang_dir.ele = (MYFLT) *p->ele;
  p->ang_dir.length = FL(1.0);
  angle_to_cart(p->ang_dir, &(p->cart_dir));
  p->spread_base.x = p->cart_dir.y;
  p->spread_base.y = p->cart_dir.z;
  p->spread_base.z = -p->cart_dir.x;
  vbap_zak_control(csound,p);
  for (i=0;i<n;i++) {
    p->beg_gains[i] = p->updated_gains[i];
    p->end_gains[i] = p->updated_gains[i];
  }
  return OK;
}

int32_t vbap_zak_moving(CSOUND *csound, VBAP_ZAK_MOVING *p)
{                                           /* during note performance: */
  MYFLT *outptr, *inptr;
  MYFLT ogain, ngain, gainsubstr;
  MYFLT invfloatn;
  int32_t j;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;

  vbap_zak_moving_control(csound,p);
  for (j=0;j< p->n; j++) {
    p->beg_gains[j] = p->end_gains[j];
    p->end_gains[j] = p->updated_gains[j];
  }

  /* write audio to resulting audio streams weighted
     with gain factors */
  invfloatn =  FL(1.0)/(nsmps-offset);
  outptr = p->out_array;
  if (UNLIKELY(offset)) memset(outptr, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&outptr[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (j=0; j<p->n ;j++) {
    inptr = p->audio;
    ogain = p->beg_gains[j];
    ngain = p->end_gains[j];
    gainsubstr = ngain - ogain;
    if (ngain != FL(0.0) || ogain != FL(0.0))
      if (ngain != ogain) {
        for (i = offset; i < nsmps; i++) {
          outptr[i] = inptr[i] *
            (ogain + (MYFLT) (i+1) * invfloatn * gainsubstr);
        }
        p->curr_gains[j] =  ogain +
          (MYFLT) (i) * invfloatn * gainsubstr;
      }
      else
        for (i=offset; i<nsmps; ++i)
          outptr[i] = inptr[i] * ogain;
    else
      memset(outptr, 0, nsmps*sizeof(MYFLT));
  }
  return OK;
}

int32_t vbap_zak_moving_control(CSOUND *csound, VBAP_ZAK_MOVING *p)
{
  CART_VEC spreaddir[16];
  CART_VEC spreadbase[16];
  ANG_VEC atmp;
  int32 i,j, spreaddirnum;
  int32_t n = p->n;
  CART_VEC tmp1, tmp2, tmp3;
  MYFLT coeff, angle;
  MYFLT tmp_gains[MAXCHNLS],sum = FL(0.0); /* Array long enough */
  if (UNLIKELY(p->dim == 2 && fabs(p->ang_dir.ele) > 0.0)) {
    csound->Warning(csound,
                    "%s", Str("Warning: truncating elevation to 2-D plane\n"));
    p->ang_dir.ele = FL(0.0);
  }

  if (*p->spread <FL(0.0))
    *p->spread = FL(0.0);
  else if (*p->spread >FL(100.0))
    *p->spread = FL(100.0);
  if (p->point_change_counter++ >= p->point_change_interval) {
    p->point_change_counter = 0;
    p->curr_fld = p->next_fld;
    if (++p->next_fld >= (int32_t) fabs(*p->field_am)) {
      if (*p->field_am >= FL(0.0)) /* point-to-point */
        p->next_fld = 0;
      else
        p->next_fld = 1;
    }
    if (p->dim == 3) { /* jumping over second field */
      p->curr_fld = p->next_fld;
      if (++p->next_fld >= ((int32_t) fabs(*p->field_am))) {
        if (*p->field_am >= FL(0.0)) /* point-to-point */
          p->next_fld = 0;
        else
          p->next_fld = 1;
      }
    }
    if (UNLIKELY((p->fld[abs(p->next_fld)]==NULL)))
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("Missing fields in vbapzmove\n"));
    if (*p->field_am >= FL(0.0) && p->dim == 2) /* point-to-point */
      if (UNLIKELY(fabs(fabs(*p->fld[p->next_fld] - *p->fld[p->curr_fld])
                        - 180.0) < 1.0))
        csound->Warning(csound,
                        "%s", Str("Warning: Ambiguous transition 180 degrees.\n"));
  }
  if (*p->field_am >= FL(0.0)) { /* point-to-point */
    if (p->dim == 3) { /* 3-D */
      p->prev_ang_dir.azi =  *p->fld[p->curr_fld-1];
      p->next_ang_dir.azi =  *p->fld[p->next_fld];
      p->prev_ang_dir.ele = *p->fld[p->curr_fld];
      p->next_ang_dir.ele = *p->fld[p->next_fld+1];
      coeff = ((MYFLT) p->point_change_counter) /
        ((MYFLT) p->point_change_interval);
      angle_to_cart( p->prev_ang_dir,&tmp1);
      angle_to_cart( p->next_ang_dir,&tmp2);
      tmp3.x = (FL(1.0)-coeff) * tmp1.x + coeff * tmp2.x;
      tmp3.y = (FL(1.0)-coeff) * tmp1.y + coeff * tmp2.y;
      tmp3.z = (FL(1.0)-coeff) * tmp1.z + coeff * tmp2.z;
      coeff = (MYFLT)sqrt((double)(tmp3.x * tmp3.x +
                                   tmp3.y * tmp3.y +
                                   tmp3.z * tmp3.z));
      tmp3.x /= coeff; tmp3.y /= coeff; tmp3.z /= coeff;
      cart_to_angle(tmp3,&(p->ang_dir));
    }
    else if (p->dim == 2) { /* 2-D */
      p->prev_ang_dir.azi =  *p->fld[p->curr_fld];
      p->next_ang_dir.azi =  *p->fld[p->next_fld ];
      p->prev_ang_dir.ele = p->next_ang_dir.ele =  FL(0.0);
      scale_angles(&(p->prev_ang_dir));
      scale_angles(&(p->next_ang_dir));
      angle = (p->prev_ang_dir.azi - p->next_ang_dir.azi);
      while (angle > FL(180.0))
        angle -= FL(360.0);
      while (angle < -FL(180.0))
        angle += FL(360.0);
      coeff = ((MYFLT) p->point_change_counter) /
        ((MYFLT) p->point_change_interval);
      angle  *=  (coeff);
      p->ang_dir.azi = p->prev_ang_dir.azi -  angle;
      p->ang_dir.ele = FL(0.0);
    }
    else {
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("Missing fields in vbapzmove\n"));
    }
  }
  else { /* angular velocities */
    if (p->dim == 2) {
      p->ang_dir.azi = p->ang_dir.azi +
        (*p->fld[p->next_fld] * CS_ONEDKR);
      scale_angles(&(p->ang_dir));
    }
    else { /* 3D angular */
      p->ang_dir.azi = p->ang_dir.azi +
        (*p->fld[p->next_fld] * CS_ONEDKR);
      p->ang_dir.ele = p->ang_dir.ele +
        p->ele_vel * (*p->fld[p->next_fld+1] * CS_ONEDKR);
      if (p->ang_dir.ele > FL(90.0)) {
        p->ang_dir.ele = FL(90.0);
        p->ele_vel = -p->ele_vel;
      }
      if (p->ang_dir.ele < FL(0.0)) {
        p->ang_dir.ele = FL(0.0);
        p->ele_vel =  -p->ele_vel;
      }
      scale_angles(&(p->ang_dir));
    }
  }
  angle_to_cart(p->ang_dir, &(p->cart_dir));
  calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                p->updated_gains, n, p->cart_dir);
  if (*p->spread > FL(0.0)) {
    if (p->dim == 3) {
      spreaddirnum=16;
      /* four orthogonal dirs */
      new_spread_dir(&spreaddir[0], p->cart_dir,
                     p->spread_base, p->ang_dir.azi, *p->spread);

      new_spread_base(spreaddir[0], p->cart_dir,*p->spread, &p->spread_base);
      cross_prod(p->spread_base, p->cart_dir, &spreadbase[1]);
      cross_prod(spreadbase[1], p->cart_dir, &spreadbase[2]);
      cross_prod(spreadbase[2], p->cart_dir, &spreadbase[3]);
      /* four between them */
      vec_mean(p->spread_base, spreadbase[1], &spreadbase[4]);
      vec_mean(spreadbase[1], spreadbase[2], &spreadbase[5]);
      vec_mean(spreadbase[2], spreadbase[3], &spreadbase[6]);
      vec_mean(spreadbase[3], p->spread_base, &spreadbase[7]);

      /* four at half spreadangle */
      vec_mean(p->cart_dir, p->spread_base, &spreadbase[8]);
      vec_mean(p->cart_dir, spreadbase[1], &spreadbase[9]);
      vec_mean(p->cart_dir, spreadbase[2], &spreadbase[10]);
      vec_mean(p->cart_dir, spreadbase[3], &spreadbase[11]);

      /* four at quarter spreadangle */
      vec_mean(p->cart_dir, spreadbase[8], &spreadbase[12]);
      vec_mean(p->cart_dir, spreadbase[9], &spreadbase[13]);
      vec_mean(p->cart_dir, spreadbase[10], &spreadbase[14]);
      vec_mean(p->cart_dir, spreadbase[11], &spreadbase[15]);

      for (i=1;i<spreaddirnum;i++) {
        new_spread_dir(&spreaddir[i], p->cart_dir,
                       spreadbase[i],p->ang_dir.azi,*p->spread);
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, n, spreaddir[i]);
        for (j=0;j<n;j++) {
          p->updated_gains[j] += tmp_gains[j];
        }
      }
    }
    else if (p->dim == 2) {
      spreaddirnum=6;
      atmp.ele=FL(0.0);
      atmp.azi=p->ang_dir.azi - *p->spread;
      angle_to_cart(atmp, &spreaddir[0]);
      atmp.azi=p->ang_dir.azi - *p->spread/2;
      angle_to_cart(atmp, &spreaddir[1]);
      atmp.azi=p->ang_dir.azi - *p->spread/4;
      angle_to_cart(atmp, &spreaddir[2]);
      atmp.azi=p->ang_dir.azi + *p->spread/4;
      angle_to_cart(atmp, &spreaddir[3]);
      atmp.azi=p->ang_dir.azi + *p->spread/2;
      angle_to_cart(atmp, &spreaddir[4]);
      atmp.azi=p->ang_dir.azi + *p->spread;
      angle_to_cart(atmp, &spreaddir[5]);

      for (i=0;i<spreaddirnum;i++) {
        calc_vbap_gns(p->ls_set_am, p->dim,  p->ls_sets,
                      tmp_gains, n, spreaddir[i]);
        for (j=0;j<n;j++) {
          p->updated_gains[j] += tmp_gains[j];
        }
      }
    }
  }
  if (*p->spread > FL(70.0))
    for (i=0;i<n ;i++) {
      p->updated_gains[i] += (*p->spread - FL(70.0))/FL(30.0) *
        (*p->spread - FL(70.0))/FL(30.0)*FL(10.0);
    }
  /* normalization */
  for (i=0;i<n;i++) {
    sum += (p->updated_gains[i]*p->updated_gains[i]);
  }

  sum = SQRT(sum);
  for (i=0;i<n;i++) {
    p->updated_gains[i] /= sum;
  }
  return OK;
}

int32_t vbap_zak_moving_init(CSOUND *csound, VBAP_ZAK_MOVING *p)
{
  int32_t     i, j, indx;
  MYFLT   *ls_table, *ptr;
  LS_SET  *ls_set_ptr;
  int32_t n = p->n;
  p->n = (int32_t)MYFLT2LONG(*p->numb); /* Set size */
  /* Check to see this index is within the limits of za space.    */
  MYFLT* zastart;
  int64_t zalast = GetZaBounds(csound, &zastart);
  indx = (int32) *p->ndx;
  if (UNLIKELY(indx > zalast)) {
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("outz index > isizea. No output"));
  }
  else if (UNLIKELY(indx < 0)) {
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("outz index < 0. No output."));
  }
  /* Now read from the array in za space and write to the output. */
  p->out_array     = zastart + (indx * CS_KSMPS);/* outputs */
  csound->AuxAlloc(csound, p->n*sizeof(MYFLT)*4, &p->auxch);
  p->curr_gains    = (MYFLT*)p->auxch.auxp;
  p->beg_gains     = p->curr_gains + p->n;
  p->end_gains     = p->beg_gains + p->n;
  p->updated_gains = p->end_gains + p->n;
  /* reading in loudspeaker info */
  ls_table = (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound,
                                                          "vbap_ls_table_0"));
  p->dim           = (int32_t) ls_table[0];
  p->ls_am         = (int32_t) ls_table[1];
  p->ls_set_am     = (int32_t) ls_table[2];
  ptr              = &(ls_table[3]);
  csound->AuxAlloc(csound, p->ls_set_am * sizeof (LS_SET), &p->aux);
  if (UNLIKELY(p->aux.auxp == NULL)) {
    return csound->InitError(csound, "%s", Str("could not allocate memory"));
  }
  p->ls_sets = (LS_SET*) p->aux.auxp;
  ls_set_ptr = p->ls_sets;
  for (i=0 ; i < p->ls_set_am ; i++) {
    ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
    for (j=0 ; j < p->dim ; j++) {
      ls_set_ptr[i].ls_nos[j] = (int32_t) *(ptr++);
    }
    memset(ls_set_ptr[i].ls_mx, '\0', 9*sizeof(MYFLT));
    /* for (j=0 ; j < 9; j++) */
    /*   ls_set_ptr[i].ls_mx[j] = FL(0.0);  /\* initial setting *\/ */
    for (j=0 ; j < (p->dim) * (p->dim); j++) {
      ls_set_ptr[i].ls_mx[j] = (MYFLT) *(ptr++);
    }
  }

  /* other initialization */
  p->ele_vel = FL(1.0);    /* functions specific to movement */
  if (UNLIKELY(fabs(*p->field_am) < (2+ (p->dim - 2)*2))) {
    return csound->InitError(csound,
                             Str("Have to have at least %d directions in vbapzmove"),
                             2 + (p->dim - 2) * 2);
  }
  if (p->dim == 2)
    p->point_change_interval = (int32_t) (CS_EKR * *p->dur
                                          / (fabs(*p->field_am) - 1.0));
  else if (LIKELY(p->dim == 3))
    p->point_change_interval = (int32_t) (CS_EKR * *p->dur
                                          / (fabs(*p->field_am) * 0.5 - 1.0));
  else
    return csound->InitError(csound, "%s", Str("Wrong dimension"));
  p->point_change_counter = 0;
  p->curr_fld = 0;
  p->next_fld = 1;
  p->ang_dir.azi = *p->fld[0];
  if (p->dim == 3) {
    p->ang_dir.ele = *p->fld[1];
  } else {
    p->ang_dir.ele = FL(0.0);
  }
  if (p->dim == 3) {
    p->curr_fld = 1;
    p->next_fld = 2;
  }
  angle_to_cart(p->ang_dir, &(p->cart_dir));
  p->spread_base.x = p->cart_dir.y;
  p->spread_base.y = p->cart_dir.z;
  p->spread_base.z = -p->cart_dir.x;
  vbap_zak_moving_control(csound,p);
  for (i=0;i<n;i++) {
    p->beg_gains[i] = p->updated_gains[i];
    p->end_gains[i] = p->updated_gains[i];
  }
  return OK;
}



#define S(x)    sizeof(x)

/* static */
static OENTRY vbap_localops[] = {
  { "vbap.a",      S(VBAP),
    TR,   "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
    "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "akOOo",
    (SUBR) vbap_init,    (SUBR) vbap                   },
  { "vbap.A",      S(VBAPA), TR,   "a[]",    "akOOo",
    (SUBR) vbap_init_a,    (SUBR) vbap_a               },
  { "vbap4",      S(VBAP),
    TR|_QQ,   "aaaammmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
    "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "akOOo", (SUBR) vbap_init, (SUBR) vbap },
  { "vbap8",      S(VBAP),
    TR|_QQ,   "aaaaaaaammmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
    "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "akOOo",
    (SUBR) vbap_init,    (SUBR) vbap                   },
  { "vbap16",      S(VBAP),
    TR|_QQ,   "aaaaaaaaaaaaaaaammmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
    "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "akOOo", (SUBR) vbap_init,    (SUBR) vbap                   },
  { "vbapg.a",      S(VBAP1),             TR, 
    "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
    "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", "kOOo",
    (SUBR) vbap1_init,         (SUBR) vbap1                                  },
  { "vbapg.A",      S(VBAPA1),            TR, 
    "k[]",  "kOOo",
    (SUBR) vbap1_init_a,         (SUBR) vbap1a                               },
  { "vbapz",      S(VBAP_ZAK),     ZW|TR,   "",                 "iiakOOo",
    (SUBR) vbap_zak_init,    (SUBR) vbap_zak         },
  { "vbaplsinit",S(VBAP_LS_INIT),TR, "",
    "ii"
    "oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
    "oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo",
    (SUBR) vbap_ls_init, (SUBR) NULL, (SUBR) NULL, (SUBR) NULL         },
  { "vbaplsinit",S(VBAP_LS_INIT),TR, "", "iii[]",
    (SUBR) vbap_ls_inita, (SUBR) NULL, (SUBR) NULL, (SUBR) NULL         },
  { "vbapmove.a", S(VBAP_MOVING),
    TR,   "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
    "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "aiiim",
    (SUBR) vbap_moving_init, (SUBR) vbap_moving },
  { "vbapgmove.a",  S(VBAP1_MOVING),      TR, 
    "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
    "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", "iiim",
    (SUBR) vbap1_moving_init,    (SUBR) vbap1_moving },
  { "vbapmove.A", S(VBAPA_MOVING),
    TR,   "a[]",  "aiiim",
    (SUBR) vbap_moving_init_a, (SUBR) vbap_moving_a },
  { "vbapgmove.A",  S(VBAPA1_MOVING),      TR, 
    "k[]", "iiim",
    (SUBR) vbap1_moving_init_a,    (SUBR) vbap1_moving_a },
  { "vbapzmove",  S(VBAP_ZAK_MOVING),    ZW|TR,   "",  "iiaiiim",
    (SUBR) vbap_zak_moving_init,    (SUBR) vbap_zak_moving  },
  { "vbap4move", S(VBAP_MOVING),   TR|_QQ,   "aaaa",
    "aiiim",
    (SUBR) vbap_moving_init, (SUBR) vbap_moving },
  { "vbap8move", S(VBAP_MOVING),
    TR|_QQ,   "aaaaaaaa",
    "aiiim",
    (SUBR) vbap_moving_init, (SUBR) vbap_moving }

};

LINKAGE_BUILTIN(vbap_localops)

