/*
    ugtabs.c:  new implementation of table readers and writers

    Copyright (C) 2013 V Lazzarini

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

#include "csoundCore.h" 
#include "ugtabs.h"
#include <math.h>

#define MYFLOOR(x) (x >= FL(0.0) ? (int32)x : (int32)((double)x - 0.99999999))
extern FUNC *csoundFTFind2(CSOUND *csound, MYFLT *argp);

int tabler_init(CSOUND *csound, TABLER *p){

  int ndx, len = p->len;
  int mask = p->ftp->lenmask;

  if (UNLIKELY((p->ftp = csoundFTFind2(csound, p->ftable)) == NULL)){
    p->np2 = 1;  /* non pow of two table */
    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->InitError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
  } else p->np2 = 0;  /* pow of two table */

  if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1;
 
  p->len = p->ftp->flen;

  ndx = MYFLOOR((*p->ndx + *p->offset)*p->mul);
  if(p->wrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  *p->ans = p->ftp->ftable[ndx]; 
  return OK;
}

int tabler_setup(CSOUND *csound, TABLER *p){
   
   if (UNLIKELY(p->XINCODE != p->XOUTCODE)) {
      if (csound->GetKsmps(csound) != 1)
        return csound->InitError(csound, Str("table: index type inconsistent with output"));
    }
  if (UNLIKELY((p->ftp = csoundFTFind2(csound, p->ftable)) == NULL)){
    p->np2 = 1;  /* non pow of two table */
    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->InitError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
  } else p->np2 = 0;  /* pow of two table */
  if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1; 
  p->len = p->ftp->flen;
  return OK;
}

int tabler_kontrol(CSOUND *csound, TABLER *p){
  int ndx, len = p->len;
  int mask = p->ftp->lenmask;

  ndx = MYFLOOR((*p->ndx + *p->offset)*p->mul);
  if(p->wrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  *p->ans = p->ftp->ftable[ndx]; 
  return OK;
}

int tabler_audio(CSOUND *csound, TABLER *p){
  int ndx, len = p->len, n, nsmps = csound->GetKsmps(csound);
  int mask = p->ftp->lenmask;
  MYFLT *ans = p->ans;
  MYFLT *ndx_f = p->ndx;
  MYFLT *func = p->ftp->ftable;
  MYFLT offset = *p->offset;
  MYFLT mul = p->mul;
  uint32_t    koffset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;

 if (offset) memset(ans, '\0', koffset*sizeof(MYFLT));
    if (early) {
      nsmps -= early;
      memset(&ans[nsmps], '\0', early*sizeof(MYFLT));
    }
  
 for(n=koffset; n < nsmps; n++){
   ndx = MYFLOOR((ndx_f[n] + offset)*mul);
  if(p->wrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  p->ans[n] = func[ndx]; 
}
  return OK;
}

int tableir_kontrol(CSOUND *csound, TABLER *p){
  int ndx, len = p->len;
  int mask = p->ftp->lenmask;
  MYFLT tmp, frac;
  MYFLT x1, x2;

  tmp = (*p->ndx + *p->offset)*p->mul;
  ndx = MYFLOOR(tmp);
  frac = tmp - ndx;
  
  if(p->wrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  x1 = p->ftp->ftable[ndx];
  x2 = p->ftp->ftable[ndx+1];
  *p->ans = x1 + (x2 - x1)*frac;
  return OK;
}

int tableir_audio(CSOUND *csound, TABLER *p){
  int ndx, len = p->len, n, nsmps = csound->GetKsmps(csound);
  int mask = p->ftp->lenmask;
  MYFLT *ans = p->ans;
  MYFLT *ndx_f = p->ndx;
  MYFLT *func = p->ftp->ftable;
  MYFLT offset = *p->offset;
  MYFLT mul = p->mul, tmp, frac;
  uint32_t    koffset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;

 if (offset) memset(ans, '\0', koffset*sizeof(MYFLT));
    if (early) {
      nsmps -= early;
      memset(&ans[nsmps], '\0', early*sizeof(MYFLT));
    }
  
 for(n=koffset; n < nsmps; n++){
  MYFLT x1, x2;
  tmp = (ndx_f[n] + offset)*mul;
  ndx = MYFLOOR(tmp);
  frac = tmp - ndx;
  if(p->wrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  x1 = func[ndx];
  x2 = func[ndx+1];
  p->ans[n] = x1 + (x2 - x1)*frac; 
}
  return OK;
}

int table3r_kontrol(CSOUND *csound, TABLER *p){
  int ndx, len = p->len;
  int mask = p->ftp->lenmask;
  MYFLT tmp, frac;
  MYFLT x0, x1, x2, x3;
  MYFLT *func  =p->ftp->ftable; 
  MYFLT fracub, fracsq, temp1;
  
  tmp = (*p->ndx + *p->offset)*p->mul;
  ndx = MYFLOOR(tmp);
  frac = tmp - ndx;
  
  if(p->wrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  if(UNLIKELY(ndx<1 || ndx==len-1 || len <4)) {
    x1 = func[ndx];
    x2 = func[ndx+1];
    *p->ans = x1 + (x2 - x1)*frac;
  } else {
    x0 = func[ndx-1];
    x1 = func[ndx];
    x2 = func[ndx+1];
    x3 = func[ndx+2]; 
    fracsq = frac*frac;
    fracub = fracsq*frac;
    temp1 = x3+FL(3.0)*x1;
    *p->ans =  x1 + FL(0.5)*fracub + 
      frac*(x2 - fracub/FL(6.0) - temp1/FL(6.0) - x0/FL(3.0)) +   
      fracub*(temp1/FL(6.0) - FL(0.5)*x2) + fracsq*(FL(0.5)*x2 - x1);
  }
  return OK;
}

int table3r_audio(CSOUND *csound, TABLER *p){
  int ndx, len = p->len, n, nsmps = csound->GetKsmps(csound);
  int mask = p->ftp->lenmask;
  MYFLT *ans = p->ans;
  MYFLT *ndx_f = p->ndx;
  MYFLT *func = p->ftp->ftable;
  MYFLT offset = *p->offset;
  MYFLT mul = p->mul, tmp, frac;
  uint32_t    koffset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;

 if (offset) memset(ans, '\0', koffset*sizeof(MYFLT));
    if (early) {
      nsmps -= early;
      memset(&ans[nsmps], '\0', early*sizeof(MYFLT));
    }
  
 for(n=koffset; n < nsmps; n++){
  MYFLT x0,x1,x2,x3,temp1,fracub,fracsq;
  tmp = (ndx_f[n] + offset)*mul;
  ndx = MYFLOOR(tmp);
  frac = tmp - ndx;
  if(p->wrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  
   if(UNLIKELY(ndx<1 || ndx==len-1 || len <4)) {
    x1 = func[ndx];
    x2 = func[ndx+1];
    p->ans[n] = x1 + (x2 - x1)*frac;
  } else {
    x0 = func[ndx-1];
    x1 = func[ndx];
    x2 = func[ndx+1];
    x3 = func[ndx+2]; 
    fracsq = frac*frac;
    fracub = fracsq*frac;
    temp1 = x3+FL(3.0)*x1;
    p->ans[n] =  x1 + FL(0.5)*fracub + 
      frac*(x2 - fracub/FL(6.0) - temp1/FL(6.0) - x0/FL(3.0)) +   
      fracub*(temp1/FL(6.0) - FL(0.5)*x2) + fracsq*(FL(0.5)*x2 - x1);
  }
}
  return OK;
}
