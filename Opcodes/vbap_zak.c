/*
    vbap_zak.c:

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

/* vbap_zak.c

   functions specific to n loudspeaker VBAP

   Ville Pulkki & John ffitch
*/


#include "csoundCore.h"
#include "vbap.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int vbap_zak_moving_control(CSOUND *, VBAP_ZAK_MOVING *);
int vbap_zak_control(CSOUND *,VBAP_ZAK *);

int vbap_zak(CSOUND *csound, VBAP_ZAK *p)   /* during note performance: */
{
    MYFLT *outptr, *inptr;
    MYFLT ogain, ngain, gainsubstr;
    MYFLT invfloatn;
    int j;
    int n = p->n;
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

int vbap_zak_control(CSOUND *csound, VBAP_ZAK *p)
{
    CART_VEC spreaddir[16];
    CART_VEC spreadbase[16];
    ANG_VEC atmp;
    int32 i,j, spreaddirnum;
    int n = p->n;
    MYFLT tmp_gains[MAXCHNLS],sum = FL(0.0);
    if (UNLIKELY(p->dim == 2 && fabs(*p->ele) > 0.0)) {
      csound->Warning(csound,
                      Str("Warning: truncating elevation to 2-D plane\n"));
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

int vbap_zak_init(CSOUND *csound, VBAP_ZAK *p)
{                               /* Initializations before run time */
    int     i, j, indx;
    MYFLT   *ls_table, *ptr; /* , *gains; */
    LS_SET  *ls_set_ptr;
    int n = p->n = (int)MYFLT2LONG(*p->numb); /* Set size */
    char name[24];
    /* Check to see this index is within the limits of za space.    */
    indx = (int32) *p->ndx;
    if (UNLIKELY(indx > csound->zalast)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("outz index > isizea. No output"));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("outz index < 0. No output."));
    }
    if ((int)*p->layout==0) strcpy(name, "vbap_ls_table");
    else snprintf(name, 24, "vbap_ls_table_%d", (int)*p->layout==0);
    /* Now read from the array in za space and write to the output. */
    p->out_array     = csound->zastart + (indx * CS_KSMPS);/* outputs */
    csound->AuxAlloc(csound, p->n*sizeof(MYFLT)*4, &p->auxch);
    p->curr_gains    = (MYFLT*)p->auxch.auxp;
    p->beg_gains     = p->curr_gains + p->n;
    p->end_gains     = p->beg_gains + p->n;
    p->updated_gains = p->end_gains + p->n;
    ls_table = (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound, name));
    p->dim           = (int) ls_table[0];   /* reading in loudspeaker info */
    p->ls_am         = (int) ls_table[1];
    p->ls_set_am     = (int) ls_table[2];
    ptr              = &(ls_table[3]);
    csound->AuxAlloc(csound, p->ls_set_am * sizeof (LS_SET), &p->aux);
    if (UNLIKELY(p->aux.auxp == NULL)) {
      return csound->InitError(csound, Str("could not allocate memory"));
    }
    p->ls_sets = (LS_SET*) p->aux.auxp;
    ls_set_ptr = p->ls_sets;
    for (i=0 ; i < p->ls_set_am ; i++) {
      ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
      for (j=0 ; j < p->dim ; j++) {
        ls_set_ptr[i].ls_nos[j] = (int) *(ptr++);
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
                      Str("Warning: truncating elevation to 2-D plane\n"));
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

int vbap_zak_moving(CSOUND *csound, VBAP_ZAK_MOVING *p)
{                                           /* during note performance: */
    MYFLT *outptr, *inptr;
    MYFLT ogain, ngain, gainsubstr;
    MYFLT invfloatn;
    int j;
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

int vbap_zak_moving_control(CSOUND *csound, VBAP_ZAK_MOVING *p)
{
    CART_VEC spreaddir[16];
    CART_VEC spreadbase[16];
    ANG_VEC atmp;
    int32 i,j, spreaddirnum;
    int n = p->n;
    CART_VEC tmp1, tmp2, tmp3;
    MYFLT coeff, angle;
    MYFLT tmp_gains[MAXCHNLS],sum = FL(0.0); /* Array long enough */
    if (UNLIKELY(p->dim == 2 && fabs(p->ang_dir.ele) > 0.0)) {
      csound->Warning(csound,
                      Str("Warning: truncating elevation to 2-D plane\n"));
      p->ang_dir.ele = FL(0.0);
    }

    if (*p->spread <FL(0.0))
      *p->spread = FL(0.0);
    else if (*p->spread >FL(100.0))
      *p->spread = FL(100.0);
    if (p->point_change_counter++ >= p->point_change_interval) {
      p->point_change_counter = 0;
      p->curr_fld = p->next_fld;
      if (++p->next_fld >= (int) fabs(*p->field_am)) {
        if (*p->field_am >= FL(0.0)) /* point-to-point */
          p->next_fld = 0;
        else
          p->next_fld = 1;
      }
      if (p->dim == 3) { /* jumping over second field */
        p->curr_fld = p->next_fld;
        if (++p->next_fld >= ((int) fabs(*p->field_am))) {
          if (*p->field_am >= FL(0.0)) /* point-to-point */
            p->next_fld = 0;
          else
            p->next_fld = 1;
        }
      }
      if (UNLIKELY((p->fld[abs(p->next_fld)]==NULL)))
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("Missing fields in vbapzmove\n"));
      if (*p->field_am >= FL(0.0) && p->dim == 2) /* point-to-point */
        if (UNLIKELY(fabs(fabs(*p->fld[p->next_fld] - *p->fld[p->curr_fld])
                          - 180.0) < 1.0))
          csound->Warning(csound,
                          Str("Warning: Ambiguous transition 180 degrees.\n"));
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
        while(angle > FL(180.0))
          angle -= FL(360.0);
        while(angle < -FL(180.0))
          angle += FL(360.0);
        coeff = ((MYFLT) p->point_change_counter) /
          ((MYFLT) p->point_change_interval);
        angle  *=  (coeff);
        p->ang_dir.azi = p->prev_ang_dir.azi -  angle;
        p->ang_dir.ele = FL(0.0);
      }
      else {
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("Missing fields in vbapzmove\n"));
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

int vbap_zak_moving_init(CSOUND *csound, VBAP_ZAK_MOVING *p)
{
    int     i, j, indx;
    MYFLT   *ls_table, *ptr;
    LS_SET  *ls_set_ptr;
    int n = p->n;
    p->n = (int)MYFLT2LONG(*p->numb); /* Set size */
    /* Check to see this index is within the limits of za space.    */
    indx = (int32) *p->ndx;
    if (UNLIKELY(indx > csound->zalast)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("outz index > isizea. No output"));
    }
    else if (UNLIKELY(indx < 0)) {
      return csound->PerfError(csound, p->h.insdshead,
                               Str("outz index < 0. No output."));
    }
    /* Now read from the array in za space and write to the output. */
    p->out_array     = csound->zastart + (indx * CS_KSMPS);/* outputs */
    csound->AuxAlloc(csound, p->n*sizeof(MYFLT)*4, &p->auxch);
    p->curr_gains    = (MYFLT*)p->auxch.auxp;
    p->beg_gains     = p->curr_gains + p->n;
    p->end_gains     = p->beg_gains + p->n;
    p->updated_gains = p->end_gains + p->n;
    /* reading in loudspeaker info */
    ls_table = (MYFLT*) (csound->QueryGlobalVariableNoCheck(csound,
                                                        "vbap_ls_table_0"));
    p->dim           = (int) ls_table[0];
    p->ls_am         = (int) ls_table[1];
    p->ls_set_am     = (int) ls_table[2];
    ptr              = &(ls_table[3]);
    csound->AuxAlloc(csound, p->ls_set_am * sizeof (LS_SET), &p->aux);
    if (UNLIKELY(p->aux.auxp == NULL)) {
      return csound->InitError(csound, Str("could not allocate memory"));
    }
    p->ls_sets = (LS_SET*) p->aux.auxp;
    ls_set_ptr = p->ls_sets;
    for (i=0 ; i < p->ls_set_am ; i++) {
      ls_set_ptr[i].ls_nos[2] = 0;     /* initial setting */
      for (j=0 ; j < p->dim ; j++) {
        ls_set_ptr[i].ls_nos[j] = (int) *(ptr++);
      }
      for (j=0 ; j < 9; j++)
        ls_set_ptr[i].ls_mx[j] = FL(0.0);  /* initial setting */
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
      p->point_change_interval = (int) (CS_EKR * *p->dur
                                        / (fabs(*p->field_am) - 1.0));
    else if (LIKELY(p->dim == 3))
      p->point_change_interval = (int) (CS_EKR * *p->dur
                                        / (fabs(*p->field_am) * 0.5 - 1.0));
    else
      return csound->InitError(csound, Str("Wrong dimension"));
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
