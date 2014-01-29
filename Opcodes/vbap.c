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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/* vbap.c

assisting functions for VBAP
functions for loudspeaker table initialization
Re-written to take flexible number of outputs by JPff 2012 */


#include "csoundCore.h"
#include "interlocks.h"
#include "vbap.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "interlocks.h"

static void choose_ls_triplets(CSOUND *csound, ls lss[CHANNELS],
                               ls_triplet_chain **ls_triplets,
                               int ls_amount);
static int any_ls_inside_triplet(int, int, int, ls *, int);
static void add_ldsp_triplet(CSOUND *csound, int i, int j, int k,
                             ls_triplet_chain **ls_triplets,
                             ls *lss);
static void calculate_3x3_matrixes(CSOUND *csound,
                                   ls_triplet_chain *ls_triplets,
                                   ls lss[CHANNELS], int ls_amount, int ind);
static void choose_ls_tuplets(CSOUND *csound, ls lss[CHANNELS],
                              ls_triplet_chain **ls_triplets,
                              int ls_amount, int ind);
static void sort_2D_lss(ls lss[CHANNELS], int sorted_lss[CHANNELS],
                        int ls_amount);

static inline MYFLT vec_prod(CART_VEC v1, CART_VEC v2)
{
    return (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z);
}

static inline MYFLT vec_length(CART_VEC v1)
{
    return SQRT(v1.x*v1.x + v1.y*v1.y + v1.z*v1.z);
}

static MYFLT *create_ls_table(CSOUND *csound, size_t cnt, int ind)
{
    char name[24];
    snprintf(name, 24, "vbap_ls_table_%d", ind);
    csound->DestroyGlobalVariable(csound, name);
    if (UNLIKELY(csound->CreateGlobalVariable(csound, name,
                                              cnt * sizeof(MYFLT)) != 0)) {
      csound->ErrorMsg(csound, Str("vbap: error allocating loudspeaker table"));
      return NULL;
    }
    return (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, name));
}

void calc_vbap_gns(int ls_set_am, int dim, LS_SET *sets,
                   MYFLT *gains, int ls_amount,
                   CART_VEC cart_dir)
     /* Selects a vector base of a virtual source.
        Calculates gain factors in that base. */
{
    int i,j,k, tmp2;
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

    if (wts->wt1 < 0) wts->wt1 = FL(0.0);
    if (wts->wt2 < 0) wts->wt2 = FL(0.0);
    if (wts->wt3 < 0) wts->wt3 = FL(0.0);

    tmp  = (double)wts->wt1 * wts->wt1;
    tmp += (double)wts->wt2 * wts->wt2;
    tmp += (double)wts->wt3 * wts->wt3;

    tmp = sqrt(tmp);
    tmp = FL(1.0) / (MYFLT)tmp;
    wts->wt1 *= (MYFLT)tmp;
    wts->wt2 *= (MYFLT)tmp;
    wts->wt3 *= (MYFLT)tmp;
}

void angle_to_cart(ANG_VEC avec, CART_VEC *cvec)
     /* conversion */
{
    /* length unattended */
    MYFLT atorad = (TWOPI_F / FL(360.0));
    cvec->x = (MYFLT) (cos((double) (avec.azi * atorad)) *
                       cos((double) (avec.ele * atorad)));
    cvec->y = (MYFLT) (sin((double) (avec.azi * atorad)) *
                       cos((double) (avec.ele * atorad)));
    cvec->z = (MYFLT) (sin((double) (avec.ele * atorad)));
}

void cart_to_angle(CART_VEC cvec, ANG_VEC *avec)
     /* conversion */
{
    MYFLT tmp, tmp2, tmp3, tmp4;
    MYFLT atorad = (TWOPI_F / FL(360.0));

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
      avec->azi /= atorad;
    }
    avec->ele = ASIN(cvec.z);
    avec->length = SQRT(cvec.x * cvec.x + cvec.y * cvec.y + cvec.z * cvec.z);
    avec->ele /= atorad;
}

void angle_to_cart_II(ANG_VEC *from, CART_VEC *to)
     /* conversion, double*/
{
    MYFLT ang2rad = TWOPI_F / FL(360.0);
    to->x= COS(from->azi * ang2rad) * COS(from->ele * ang2rad);
    to->y= SIN(from->azi * ang2rad) * COS(from->ele * ang2rad);
    to->z= SIN(from->ele * ang2rad);
}

MYFLT vol_p_side_lgth(int i, int j,int k, ls  lss[CHANNELS] )
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
    if (lgth>FL(0.00001))
      return volper / lgth;
    else
      return FL(0.0);
}

static void choose_ls_triplets(CSOUND *csound, ls lss[CHANNELS],
                               struct ls_triplet_chain **ls_triplets,
                               int ls_amount)
  /* Selects the loudspeaker triplets, and
     calculates the inversion matrices for each selected triplet.
     A line (connection) is drawn between each loudspeaker. The lines
     denote the sides of the triangles. The triangles should not be
     intersecting. All crossing connections are searched and the
     longer connection is erased. This yields non-intesecting triangles,
     which can be used in panning.*/
{
    int i, j, k, l, table_size;
    int *connections;
/*  int *i_ptr; */
    MYFLT *distance_table;
    int *distance_table_i;
    int *distance_table_j;
    MYFLT distance;
    struct ls_triplet_chain *trip_ptr, *prev, *tmp_ptr;

    if (UNLIKELY(ls_amount == 0)) {
      csound->ErrorMsg(csound, Str("Number of loudspeakers is zero\nExiting"));
      return;
    }

    connections = calloc(CHANNELS * CHANNELS, sizeof(int));
    distance_table = calloc(((CHANNELS * (CHANNELS - 1)) / 2), sizeof(MYFLT));
    distance_table_i = calloc(((CHANNELS * (CHANNELS - 1)) / 2), sizeof(int));
    distance_table_j = calloc(((CHANNELS * (CHANNELS - 1)) / 2), sizeof(int));

/*  i_ptr = (int *) connections; */
/*  for (i=0;i< ((CHANNELS) * (CHANNELS )); i++) */
/*    *(i_ptr++) = 0; */

    for (i=0;i<ls_amount;i++)
    for (j=i+1;j<ls_amount;j++)
      for (k=j+1;k<ls_amount;k++) {
        if (vol_p_side_lgth(i,j, k, lss) > MIN_VOL_P_SIDE_LGTH) {
          connections[i+CHANNELS*j]=1;
          connections[j+CHANNELS*i]=1;
          connections[i+CHANNELS*k]=1;
          connections[k+CHANNELS*i]=1;
          connections[j+CHANNELS*k]=1;
          connections[k+CHANNELS*j]=1;
          add_ldsp_triplet(csound, i, j, k, ls_triplets, lss);
        }
      }

  /*calculate distancies between all lss and sorting them*/
    table_size =(((ls_amount - 1) * (ls_amount)) / 2);
    for (i=0;i<table_size; i++)
      distance_table[i] = FL(100000.0);

    for (i=0;i<ls_amount;i++) {
      for (j=(i+1);j<ls_amount; j++) {
        if (connections[i+CHANNELS*j] == 1) {
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
      int fst_ls = distance_table_i[i];
      int sec_ls = distance_table_j[i];
      if (connections[fst_ls+CHANNELS*sec_ls] == 1)
        for (j=0; j<ls_amount; j++)
          for (k=j+1; k<ls_amount; k++)
            if ( (j!=fst_ls) && (k != sec_ls) && (k!=fst_ls) && (j != sec_ls))
              if (lines_intersect(fst_ls, sec_ls, j,k,lss) == 1) {
                connections[j+CHANNELS*k] = 0;
                connections[k+CHANNELS*j] = 0;
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
      if (connections[i+CHANNELS*j] == 0 ||
          connections[i+CHANNELS*k] == 0 ||
          connections[j+CHANNELS*k] == 0 ||
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
    free(connections);
    free(distance_table);
    free(distance_table_i);
    free(distance_table_j);
}

/* returns 1 if there is loudspeaker(s) inside given ls triplet */

static int any_ls_inside_triplet(int a, int b, int c, ls lss[CHANNELS],
                                 int ls_amount)
{
    MYFLT invdet;
    CART_VEC *lp1, *lp2, *lp3;
    MYFLT invmx[9];
    int i,j;
    MYFLT tmp;
    int any_ls_inside, this_inside;

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

static void add_ldsp_triplet(CSOUND *csound, int i, int j, int k,
                             struct ls_triplet_chain **ls_triplets,
                             ls lss[CHANNELS])
{
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

int lines_intersect(int i,int j,int k,int l,ls  lss[CHANNELS])
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

static inline int vbap_ls_init_sr (CSOUND *csound, int dim, int count,
                            MYFLT **f, int layout)
     /* Inits the loudspeaker data. Calls choose_ls_tuplets or _triplets
        according to current dimension. The inversion matrices are
        stored in transposed form to ease calculation at run time.*/
{
    struct ls_triplet_chain *ls_triplets = NULL;
    ls lss[CHANNELS];

    ANG_VEC a_vector;
    CART_VEC c_vector;
    int i=0,j;

    //dim = (int) *p->dim;
    csound->Warning(csound, "dim : %d\n",dim);
    if (UNLIKELY(!((dim==2) || (dim == 3)))) {
      csound->ErrorMsg(csound, Str("Error in loudspeaker dimension."));
      return NOTOK;
    }
    //count = (int) *p->ls_amount;
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
      i++;
    }
    //ls_amount = (int)*p->ls_amount;
    if (UNLIKELY(count < dim)) {
      csound->ErrorMsg(csound, Str("Too few loudspeakers"));
      return NOTOK;
    }

    if (dim == 3) {
      choose_ls_triplets(csound, lss, &ls_triplets,count);
      calculate_3x3_matrixes(csound, ls_triplets,lss,count, layout);
    }
    else if (dim ==2) {
      choose_ls_tuplets(csound, lss, &ls_triplets,count, layout);
    }
    return OK;
}

int vbap_ls_init (CSOUND *csound, VBAP_LS_INIT *p)
{
    int dim = (int) *p->dim;
    MYFLT  layout = (*p->dim-dim)*100;
    return vbap_ls_init_sr(csound, dim, (int) *p->ls_amount, p->f, round(layout));
}

static void calculate_3x3_matrixes(CSOUND *csound,
                                   struct ls_triplet_chain *ls_triplets,
                                   ls lss[CHANNELS], int ls_amount, int ind)
     /* Calculates the inverse matrices for 3D */
{
    MYFLT invdet;
    CART_VEC *lp1, *lp2, *lp3;
    MYFLT *ls_table, *invmx;
    MYFLT *ptr;
    struct ls_triplet_chain *tr_ptr = ls_triplets;
    int triplet_amount = 0, i,j,k;

    if (UNLIKELY(tr_ptr == NULL)) {
      csound->ErrorMsg(csound, Str("Not valid 3-D configuration"));
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
    csound->Warning(csound, Str("\nConfigured loudspeakers\n"));
    for (i = 0; i < triplet_amount; i++) {
      csound->Warning(csound, Str("Triplet %d Loudspeakers: "), i);
      for (j = 0; j < 3; j++) {
        csound->Warning(csound, "%d ", (int) ls_table[k++]);
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
                              ls lss[CHANNELS],
                              ls_triplet_chain **ls_triplets,
                              int ls_amount, int ind)
     /* selects the loudspeaker pairs, calculates the inversion
        matrices and stores the data to a global array */
{
    int i, j, k;
    int sorted_lss[CHANNELS];
    int exist[CHANNELS];
    int amount = 0;
    MYFLT inv_mat[CHANNELS][4], *ls_table, *ptr;
    //int ftable_size;

    for (i=0;i<CHANNELS;i++) {
      exist[i]=0;
    }

    /* sort loudspeakers according their aximuth angle */
    sort_2D_lss(lss,sorted_lss,ls_amount);

    /* adjacent loudspeakers are the loudspeaker pairs to be used.*/
    for (i=0;i<(ls_amount-1);i++) {
      /*csound->Message(csound, "%d %d %f %f\n",sorted_lss[i],sorted_lss[i+1],
        lss[sorted_lss[i]].angles.azi,
        lss[sorted_lss[i+1]].angles.azi);*/
      if (LIKELY((lss[sorted_lss[i+1]].angles.azi -
                  lss[sorted_lss[i]].angles.azi) <= (PI - 0.0175))) {
        if (LIKELY(calc_2D_inv_tmatrix( lss[sorted_lss[i]].angles.azi,
                                        lss[sorted_lss[i+1]].angles.azi,
                                        inv_mat[i]) != 0)) {
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
      //printf("less than PI type 2- 0.175\n");
      if (LIKELY(calc_2D_inv_tmatrix(lss[sorted_lss[ls_amount-1]].angles.azi,
                                     lss[sorted_lss[0]].angles.azi,
                                     inv_mat[ls_amount-1]) != 0)) {
        exist[ls_amount-1]=1;
        amount++;
      }
    }
    else  csound->Warning(csound, Str("Pair of speakers at %f and %f ignored\n"),
                          lss[sorted_lss[ls_amount-1]].angles.azi*FL(180.0)/PI_F,
                          lss[sorted_lss[0]].angles.azi*FL(180.0)/PI_F);

    if (UNLIKELY(amount==0))
      csound->InitError(csound, Str("insufficient valid speakers"));

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
      csound->Message(csound, "%.1f ", lss[i].angles.azi / atorad);
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
        for (j=0;j<4;j++) {
          *(ptr++) = inv_mat[i][j];
        }
      }
    }
    if (exist[ls_amount-1] == 1) {
      *(ptr++) = (MYFLT)sorted_lss[ls_amount-1]+1;
      *(ptr++) = (MYFLT)sorted_lss[0]+1;
      for (j=0;j<4;j++) {
        *(ptr++) = inv_mat[ls_amount-1][j];
      }
    }
    k=3;
    csound->Warning(csound, Str("\nConfigured loudspeakers\n"));
    for (i=0; i < amount; i++) {
      csound->Warning(csound, Str("Pair %d Loudspeakers: "), i);
      for (j=0; j < 2; j++) {
        csound->Warning(csound, "%d ", (int) ls_table[k++]);
      }
      csound->Warning(csound, "\n");

    csound->Message(csound, "\nMatrix ");
      for (j=0; j < 4; j++) {
      csound->Message(csound, "%f ", ls_table[k]);
        k++;
      }
   csound->Message(csound, "\n\n");
    }
}

static void sort_2D_lss(ls lss[CHANNELS], int sorted_lss[CHANNELS],
                        int ls_amount)
{
    int i,j,index=-1;
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
      /*printf("tulos %f",    lss[i].angles.azi);*/
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

int calc_2D_inv_tmatrix(MYFLT azi1,MYFLT azi2, MYFLT inv_mat[4])
{
    MYFLT x1,x2,x3,x4; /* x1 x3 */
    MYFLT det;
    x1 = COS(azi1 );
    x2 = SIN(azi1 );
    x3 = COS(azi2 );
    x4 = SIN(azi2 );
    det = (x1 * x4) - ( x3 * x2 );
    if (FABS(det) <= FL(0.001)) {
      /*printf("unusable pair, det %f\n",det);*/
      inv_mat[0] = FL(0.0);
      inv_mat[1] = FL(0.0);
      inv_mat[2] = FL(0.0);
      inv_mat[3] = FL(0.0);
      return 0;
    }
    else {
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

#define S(x)    sizeof(x)

/* static */
static OENTRY vbap_localops[] = {
  { "vbap.a",      S(VBAP),
    TR, 5,  "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "akOOo",
    (SUBR) vbap_init,          (SUBR) NULL,    (SUBR) vbap                   },
  { "vbap.A",      S(VBAPA), TR, 5,  "a[]",    "akOOo",
    (SUBR) vbap_init_a,          (SUBR) NULL,    (SUBR) vbap_a               },
  { "vbap4",      S(VBAP),
    TR|_QQ, 5,  "aaaa",  "akOOo", (SUBR) vbap_init, (SUBR) NULL, (SUBR) vbap },
  { "vbap8",      S(VBAP),
    TR|_QQ, 5,  "aaaaaaaa", "akOOo",
    (SUBR) vbap_init,          (SUBR) NULL,    (SUBR) vbap                   },
  { "vbap16",      S(VBAP),
    TR|_QQ, 5,  "aaaaaaaaaaaaaaaa", "akOOo",
    (SUBR) vbap_init,          (SUBR) NULL,    (SUBR) vbap                   },
  { "vbapg.a",      S(VBAP1),             TR, 3,
    "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", "kOOo",
    (SUBR) vbap1_init,         (SUBR) vbap1                                  },
  { "vbapg.A",      S(VBAPA1),            TR, 3,
    "k[]",  "kOOo",
    (SUBR) vbap1_init_a,         (SUBR) vbap1a                               },
  { "vbapz",      S(VBAP_ZAK),     ZW|TR, 5,  "",                 "iiakOOo",
    (SUBR) vbap_zak_init,           (SUBR) NULL,    (SUBR) vbap_zak         },
  { "vbaplsinit",S(VBAP_LS_INIT),TR,1, "", "iioooooooooooooooooooooooooooooooo",
    (SUBR) vbap_ls_init,            (SUBR) NULL,    (SUBR) NULL             },
  { "vbapmove.a", S(VBAP_MOVING),
    TR, 5,  "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "aiiim",
    (SUBR) vbap_moving_init, (SUBR) NULL, (SUBR) vbap_moving },
  { "vbapgmove.a",  S(VBAP1_MOVING),      TR, 5,
    "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", "iiim",
    (SUBR) vbap1_moving_init,   (SUBR) NULL,    (SUBR) vbap1_moving },
  { "vbapmove.A", S(VBAPA_MOVING),
    TR, 5,  "a[]",  "aiiim",
    (SUBR) vbap_moving_init_a, (SUBR) NULL, (SUBR) vbap_moving_a },
  { "vbapgmove.A",  S(VBAPA1_MOVING),      TR, 5,
    "k[]", "iiim",
    (SUBR) vbap1_moving_init_a,   (SUBR) NULL,    (SUBR) vbap1_moving_a },
  { "vbapzmove",  S(VBAP_ZAK_MOVING),    ZW|TR, 5,  "",  "iiaiiim",
    (SUBR) vbap_zak_moving_init,    (SUBR) NULL,    (SUBR) vbap_zak_moving  },
  { "vbap4move", S(VBAP_MOVING),   TR|_QQ, 5,  "aaaa",
   "aiiim",
    (SUBR) vbap_moving_init, (SUBR) NULL, (SUBR) vbap_moving },
  { "vbap8move", S(VBAP_MOVING),
    TR|_QQ, 5,  "aaaaaaaa",
    "aiiim",
    (SUBR) vbap_moving_init, (SUBR) NULL, (SUBR) vbap_moving }

};

LINKAGE_BUILTIN(vbap_localops)
