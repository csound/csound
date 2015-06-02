/*
    vbap1.c:

    Copyright (C) 2000 Ville Pulkki
                  2012 John ffitch

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

/* vbap1.c

functions specific gains for loudspeaker VBAP

Ville Pulkki heavily modified by John ffitch 2012
*/


#include "csoundCore.h"
#include "vbap.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int vbap1_moving_control(CSOUND *, VBAP1_MOVE_DATA *, INSDS *, MYFLT,
                                MYFLT, MYFLT, MYFLT**);
static int vbap1_control(CSOUND *, VBAP1_DATA *, MYFLT*, MYFLT*, MYFLT*);

int vbap1(CSOUND *csound, VBAP1 *p) /* during note performance: */
{
    int j;
    int cnt = p->q.number;
    vbap1_control(csound,&p->q, p->azi, p->ele, p->spread);

    /* write gains */
    for (j=0; j<cnt; j++) {
      *p->out_array[j] = p->q.gains[j];
    }
    return OK;
}

static int vbap1_control(CSOUND *csound, VBAP1_DATA *p,
                         MYFLT* azi, MYFLT* ele, MYFLT* spread)
{
    CART_VEC spreaddir[16];
    CART_VEC spreadbase[16];
    ANG_VEC atmp;
    int32 i,j, spreaddirnum;
    int cnt = p->number;
    MYFLT tmp_gains[CHANNELS],sum=FL(0.0);
    if (UNLIKELY(p->dim == 2 && fabs(*ele) > 0.0)) {
      csound->Warning(csound,
                      Str("Warning: truncating elevation to 2-D plane\n"));
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
    return OK;
}

int vbap1_init(CSOUND *csound, VBAP1 *p)
{                               /* Initializations before run time*/
    int     i, j;
    MYFLT   *ls_table, *ptr;
    LS_SET  *ls_set_ptr;
    char name[24];
    snprintf(name, 24, "vbap_ls_table_%d", (int)*p->layout);
    ls_table = (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, name));
    if (ls_table==NULL)
      return csound->InitError(csound,
                               Str("could not find layout table no.%d"),
                               (int)*p->layout );
    p->q.number = p->OUTOCOUNT;
    p->q.dim       = (int)ls_table[0];   /* reading in loudspeaker info */
    p->q.ls_am     = (int)ls_table[1];
    p->q.ls_set_am = (int)ls_table[2];
    ptr = &(ls_table[3]);
    if (!p->q.ls_set_am)
      return csound->InitError(csound, Str("vbap system NOT configured. \nMissing"
                                           " vbaplsinit opcode in orchestra?"));
    csound->AuxAlloc(csound, p->q.ls_set_am * sizeof (LS_SET), &p->q.aux);
    if (UNLIKELY(p->q.aux.auxp == NULL)) {
      return csound->InitError(csound, Str("could not allocate memory"));
    }
    p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
    ls_set_ptr = p->q.ls_sets;
    for (i=0; i < p->q.ls_set_am; i++) {
      ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
      for (j=0 ; j < p->q.dim ; j++) {
        ls_set_ptr[i].ls_nos[j] = (int)*(ptr++);
      }
      for (j=0 ; j < 9; j++)
        ls_set_ptr[i].ls_mx[j] = FL(0.0);  /*initial setting*/
      for (j=0 ; j < (p->q.dim) * (p->q.dim); j++) {
        ls_set_ptr[i].ls_mx[j] = (MYFLT)*(ptr++);
      }
    }

    /* other initialization */
    if (UNLIKELY(p->q.dim == 2 && fabs(*p->ele) > 0.0)) {
      csound->Warning(csound,
                      Str("Warning: truncating elevation to 2-D plane\n"));
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

int vbap1a(CSOUND *csound, VBAPA1 *p) /* during note performance: */
{
    int j;
    int cnt = p->q.number;
    vbap1_control(csound,&p->q, p->azi, p->ele, p->spread);

    /* write gains */
    for (j=0; j<cnt; j++) {
      p->tabout->data[j] = p->q.gains[j];
    }
    return OK;
}

int vbap1_init_a(CSOUND *csound, VBAPA1 *p)
{                               /* Initializations before run time*/
    int     i, j;
    MYFLT   *ls_table, *ptr;
    LS_SET  *ls_set_ptr;
    char name[24];
    snprintf(name, 24, "vbap_ls_table_%d", (int)*p->layout);
    ls_table = (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, name));
    if (ls_table==NULL)
      return csound->InitError(csound,
                               Str("could not find layout table no.%d"),
                               (int)*p->layout );
    p->q.number = p->tabout->sizes[0];
    p->q.dim       = (int)ls_table[0];   /* reading in loudspeaker info */
    p->q.ls_am     = (int)ls_table[1];
    p->q.ls_set_am = (int)ls_table[2];
    ptr = &(ls_table[3]);
    if (!p->q.ls_set_am)
      return csound->InitError(csound, Str("vbap system NOT configured. \nMissing"
                                           " vbaplsinit opcode in orchestra?"));
    csound->AuxAlloc(csound, p->q.ls_set_am * sizeof (LS_SET), &p->q.aux);
    if (UNLIKELY(p->q.aux.auxp == NULL)) {
      return csound->InitError(csound, Str("could not allocate memory"));
    }
    p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
    ls_set_ptr = p->q.ls_sets;
    for (i=0; i < p->q.ls_set_am; i++) {
      ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
      for (j=0 ; j < p->q.dim ; j++) {
        ls_set_ptr[i].ls_nos[j] = (int)*(ptr++);
      }
      for (j=0 ; j < 9; j++)
        ls_set_ptr[i].ls_mx[j] = FL(0.0);  /*initial setting*/
      for (j=0 ; j < (p->q.dim) * (p->q.dim); j++) {
        ls_set_ptr[i].ls_mx[j] = (MYFLT)*(ptr++);
      }
    }

    /* other initialization */
    if (UNLIKELY(p->q.dim == 2 && fabs(*p->ele) > 0.0)) {
      csound->Warning(csound,
                      Str("Warning: truncating elevation to 2-D plane\n"));
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

int vbap1_moving(CSOUND *csound, VBAP1_MOVING *p)
{                               /* during note performance:   */
    int j;
    int cnt = p->q.number;

    vbap1_moving_control(csound,&p->q, p->h.insdshead, CS_ONEDKR,
                         *p->spread, *p->field_am, p->fld);

    /* write audio to resulting audio streams weighted
       with gain factors*/
    for (j=0; j<cnt ;j++) {
      *p->out_array[j] = p->q.gains[j];
    }
    return OK;
}

int vbap1_moving_a(CSOUND *csound, VBAPA1_MOVING *p)
{                               /* during note performance:   */
    int j;
    int cnt = p->q.number;

    vbap1_moving_control(csound,&p->q, p->h.insdshead, CS_ONEDKR,
                         *p->spread, *p->field_am, p->fld);

    /* write audio to resulting audio streams weighted
       with gain factors*/
    for (j=0; j<cnt ;j++) {
      p->tabout->data[j] = p->q.gains[j];
    }
    return OK;
}

static int vbap1_moving_control(CSOUND *csound, VBAP1_MOVE_DATA *p,
                                INSDS *insdshead, MYFLT ONEDKR,
                                MYFLT spread, MYFLT field_am, MYFLT **fld)
{
    CART_VEC spreaddir[16];
    CART_VEC spreadbase[16];
    ANG_VEC atmp;
    int32 i,j, spreaddirnum;
    CART_VEC tmp1, tmp2, tmp3;
    MYFLT coeff, angle;
    MYFLT tmp_gains[CHANNELS],sum=FL(0.0);
    int cnt = p->number;

    printf("cnt=%d dim=%d\n", cnt, p->dim);

    if (UNLIKELY(p->dim == 2 && fabs(p->ang_dir.ele) > 0.0)) {
      csound->Warning(csound,
                      Str("Warning: truncating elevation to 2-D plane\n"));
      p->ang_dir.ele = FL(0.0);
    }
    if (spread <FL(0.0))
      spread = FL(0.0);
    else if (spread >FL(100.0))
      spread = FL(100.0);
    if (p->point_change_counter++ >= p->point_change_interval) {
      p->point_change_counter = 0;
      p->curr_fld = p->next_fld;
      if (++p->next_fld >= (int) fabs(field_am)) {
        if (field_am >= FL(0.0)) /* point-to-point */
          p->next_fld = 0;
        else
          p->next_fld = 1;
      }
      if (p->dim == 3) { /*jumping over second field */
        p->curr_fld = p->next_fld;
        if (++p->next_fld >= ((int) fabs(field_am))) {
          if (field_am >= FL(0.0)) /* point-to-point */
            p->next_fld = 0;
          else
            p->next_fld = 1;
        }
      }
      if (UNLIKELY((fld[abs(p->next_fld)]==NULL)))
        return csound->PerfError(csound, insdshead,
                                 Str("Missing fields in vbapmove\n"));
      if (field_am >= FL(0.0) && p->dim == 2) /* point-to-point */
        if (UNLIKELY(fabs(fabs(*fld[p->next_fld] -
                               *fld[p->curr_fld]) - 180.0) < 1.0))
          csound->Warning(csound,
                          Str("Warning: Ambiguous transition 180 degrees.\n"));
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
        return csound->PerfError(csound, insdshead,
                                 Str("Missing fields in vbapmove\n"));
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
    return OK;
}

int vbap1_moving_init(CSOUND *csound, VBAP1_MOVING *p)
{
    int     i, j;
    MYFLT   *ls_table, *ptr;
    LS_SET  *ls_set_ptr;

    p->q.number = p->OUTCOUNT;
    ls_table =
      (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, "vbap_ls_table_0"));
    /* reading in loudspeaker info */
    p->q.dim       = (int)ls_table[0];
    p->q.ls_am     = (int)ls_table[1];
    p->q.ls_set_am = (int)ls_table[2];
    ptr = &(ls_table[3]);
    if (!p->q.ls_set_am)
      return csound->InitError(csound, Str("vbap system NOT configured. \n"
                                           "Missing vbaplsinit opcode"
                                           " in orchestra?"));
    csound->AuxAlloc(csound, p->q.ls_set_am * sizeof(LS_SET), &p->q.aux);
    if (UNLIKELY(p->q.aux.auxp == NULL)) {
      return csound->InitError(csound, Str("could not allocate memory"));
    }
    p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
    ls_set_ptr = p->q.ls_sets;
    for (i=0 ; i < p->q.ls_set_am ; i++) {
      ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
      for (j=0 ; j < p->q.dim ; j++) {
        ls_set_ptr[i].ls_nos[j] = (int)*(ptr++);
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
        (int)(CS_EKR * *p->dur /(fabs(*p->field_am) - 1.0));
    else if (LIKELY(p->q.dim == 3))
      p->q.point_change_interval =
        (int)(CS_EKR * *p->dur /(fabs(*p->field_am)*0.5 - 1.0));
    else
      return csound->InitError(csound, Str("Wrong dimension"));
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
    vbap1_moving_control(csound,&p->q, p->h.insdshead, CS_ONEDKR,
                         *p->spread, *p->field_am, p->fld);
    return OK;
}

int vbap1_moving_init_a(CSOUND *csound, VBAPA1_MOVING *p)
{
    int     i, j;
    MYFLT   *ls_table, *ptr;
    LS_SET  *ls_set_ptr;

    if (p->tabout->data == NULL || p->tabout->dimensions!=1)
      return csound->InitError(csound, Str("Output array not initialised"));
    p->q.number = p->tabout->sizes[0];
    ls_table =
      (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, "vbap_ls_table_0"));
    /* reading in loudspeaker info */
    p->q.dim       = (int)ls_table[0];
    p->q.ls_am     = (int)ls_table[1];
    p->q.ls_set_am = (int)ls_table[2];
    ptr = &(ls_table[3]);
    if (!p->q.ls_set_am)
      return csound->InitError(csound, Str("vbap system NOT configured. \n"
                                           "Missing vbaplsinit opcode"
                                           " in orchestra?"));
    csound->AuxAlloc(csound, p->q.ls_set_am * sizeof(LS_SET), &p->q.aux);
    if (UNLIKELY(p->q.aux.auxp == NULL)) {
      return csound->InitError(csound, Str("could not allocate memory"));
    }
    p->q.ls_sets = (LS_SET*) p->q.aux.auxp;
    ls_set_ptr = p->q.ls_sets;
    for (i=0 ; i < p->q.ls_set_am ; i++) {
      ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
      for (j=0 ; j < p->q.dim ; j++) {
        ls_set_ptr[i].ls_nos[j] = (int)*(ptr++);
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
        (int)(CS_EKR * *p->dur /(fabs(*p->field_am) - 1.0));
    else if (LIKELY(p->q.dim == 3))
      p->q.point_change_interval =
        (int)(CS_EKR * *p->dur /(fabs(*p->field_am)*0.5 - 1.0));
    else
      return csound->InitError(csound, Str("Wrong dimension"));
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
    vbap1_moving_control(csound,&p->q, p->h.insdshead, CS_ONEDKR,
                         *p->spread, *p->field_am, p->fld);
    return OK;
}
