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

int tabler_init(CSOUND *csound, TABL *p){

  int ndx, len;
  int mask;

  if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->InitError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
  mask = p->ftp->lenmask;
  p->np2 = mask ? 0 : 1;
  len = p->ftp->flen;

  if (*p->mode)
      p->mul = len;
    else
      p->mul = 1;
 
  ndx = MYFLOOR((*p->ndx + *p->offset)*p->mul);
  if(*p->wrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  *p->sig = p->ftp->ftable[ndx]; 
  return OK;
}

int tabl_setup(CSOUND *csound, TABL *p){
   
   if (UNLIKELY(p->XINCODE != p->XOUTCODE)) {
      if (csound->GetKsmps(csound) != 1)
        return csound->InitError(csound, Str("table: index type inconsistent with output"));
    }

   if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->InitError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
    p->np2 = p->ftp->lenmask ? 0 : 1;

  if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1; 
  p->len = p->ftp->flen;
  p->iwrap = (int32) *p->wrap;
  return OK;
}

int tabler_kontrol(CSOUND *csound, TABL *p){
  int ndx, len = p->len;
  int mask = p->ftp->lenmask;
  IGN(csound);

  ndx = MYFLOOR((*p->ndx + *p->offset)*p->mul);
  if(p->iwrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  *p->sig = p->ftp->ftable[ndx]; 
  return OK;
}



int tabler_audio(CSOUND *csound, TABL *p){
  int ndx, len = p->len, n, nsmps = csound->GetKsmps(csound);
  int mask = p->ftp->lenmask;
  MYFLT *sig = p->sig;
  MYFLT *ndx_f = p->ndx;
  MYFLT *func = p->ftp->ftable;
  MYFLT offset = *p->offset;
  MYFLT mul = p->mul;
  int32 iwrap = p->iwrap;
  uint32_t    koffset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;

 if (koffset) memset(sig, '\0', koffset*sizeof(MYFLT));
    if (early) {
      nsmps -= early;
      memset(&sig[nsmps], '\0', early*sizeof(MYFLT));
    }
  
 for(n=koffset; n < nsmps; n++){
   ndx = MYFLOOR((ndx_f[n] + offset)*mul);
   if(iwrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  p->sig[n] = func[ndx]; 
}
  return OK;
}

int tableir_kontrol(CSOUND *csound, TABL *p){
  int ndx, len = p->len;
  int mask = p->ftp->lenmask;
  MYFLT tmp, frac;
  MYFLT x1, x2;
  IGN(csound);

  tmp = (*p->ndx + *p->offset)*p->mul;
  ndx = MYFLOOR(tmp);
  frac = tmp - ndx;
  
  if(p->iwrap) {
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
  *p->sig = x1 + (x2 - x1)*frac;
  return OK;
}

int tableir_audio(CSOUND *csound, TABL *p){
  int ndx, len = p->len, n, nsmps = csound->GetKsmps(csound);
  int mask = p->ftp->lenmask;
  MYFLT *sig = p->sig;
  MYFLT *ndx_f = p->ndx;
  MYFLT *func = p->ftp->ftable;
  MYFLT offset = *p->offset;
  MYFLT mul = p->mul, tmp, frac;
  int32 iwrap = p->iwrap;
  uint32_t    koffset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;

 if (koffset) memset(sig, '\0', koffset*sizeof(MYFLT));
    if (early) {
      nsmps -= early;
      memset(&sig[nsmps], '\0', early*sizeof(MYFLT));
    }
  
 for(n=koffset; n < nsmps; n++){
  MYFLT x1, x2;
  tmp = (ndx_f[n] + offset)*mul;
  ndx = MYFLOOR(tmp);
  frac = tmp - ndx;
  if(iwrap) {
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
  p->sig[n] = x1 + (x2 - x1)*frac; 
}
  return OK;
}

int table3r_kontrol(CSOUND *csound, TABL *p){
  int ndx, len = p->len;
  int mask = p->ftp->lenmask;
  MYFLT tmp, frac;
  MYFLT x0, x1, x2, x3;
  MYFLT *func  =p->ftp->ftable; 
  MYFLT fracub, fracsq, temp1;

  IGN(csound);
  
  tmp = (*p->ndx + *p->offset)*p->mul;
  ndx = MYFLOOR(tmp);
  frac = tmp - ndx;
  
  if(p->iwrap) {
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
    *p->sig = x1 + (x2 - x1)*frac;
  } else {
    x0 = func[ndx-1];
    x1 = func[ndx];
    x2 = func[ndx+1];
    x3 = func[ndx+2]; 
    fracsq = frac*frac;
    fracub = fracsq*frac;
    temp1 = x3+FL(3.0)*x1;
    *p->sig =  x1 + FL(0.5)*fracub + 
      frac*(x2 - fracub/FL(6.0) - temp1/FL(6.0) - x0/FL(3.0)) +   
      fracub*(temp1/FL(6.0) - FL(0.5)*x2) + fracsq*(FL(0.5)*x2 - x1);
  }
  return OK;
}

int table3r_audio(CSOUND *csound, TABL *p){
  int ndx, len = p->len, n, nsmps = csound->GetKsmps(csound);
  int mask = p->ftp->lenmask;
  MYFLT *sig = p->sig;
  MYFLT *ndx_f = p->ndx;
  MYFLT *func = p->ftp->ftable;
  MYFLT offset = *p->offset;
  MYFLT mul = p->mul, tmp, frac;
  int32 iwrap = p->iwrap;
  uint32_t    koffset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;

 if (koffset) memset(sig, '\0', koffset*sizeof(MYFLT));
    if (early) {
      nsmps -= early;
      memset(&sig[nsmps], '\0', early*sizeof(MYFLT));
    }
  
 for(n=koffset; n < nsmps; n++){
  MYFLT x0,x1,x2,x3,temp1,fracub,fracsq;
  tmp = (ndx_f[n] + offset)*mul;
  ndx = MYFLOOR(tmp);
  frac = tmp - ndx;
  if(iwrap) {
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
    p->sig[n] = x1 + (x2 - x1)*frac;
  } else {
    x0 = func[ndx-1];
    x1 = func[ndx];
    x2 = func[ndx+1];
    x3 = func[ndx+2]; 
    fracsq = frac*frac;
    fracub = fracsq*frac;
    temp1 = x3+FL(3.0)*x1;
    p->sig[n] =  x1 + FL(0.5)*fracub + 
      frac*(x2 - fracub/FL(6.0) - temp1/FL(6.0) - x0/FL(3.0)) +   
      fracub*(temp1/FL(6.0) - FL(0.5)*x2) + fracsq*(FL(0.5)*x2 - x1);
  }
}
  return OK;
}

int tablkt_setup(CSOUND *csound, TABL *p){
   
   if (UNLIKELY(p->XINCODE != p->XOUTCODE)) {
      if (csound->GetKsmps(csound) != 1)
        return csound->InitError(csound, Str("tablekt: index type inconsistent with output"));
    }
  if (*p->mode)
      p->mul = p->ftp->flen;
    else
      p->mul = 1; 
  p->len = p->ftp->flen;
  p->iwrap = (int32) *p->wrap;
  return OK;
}

int tablerkt_kontrol(CSOUND *csound, TABL *p){

  if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
   p->np2 = p->ftp->lenmask ? 0 : 1;
   
  return tabler_kontrol(csound,p);
}


int tablerkt_audio(CSOUND *csound, TABL *p){

  if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
   p->np2 = p->ftp->lenmask ? 0 : 1;
   
  return tabler_audio(csound,p);;
}

int tableirkt_kontrol(CSOUND *csound, TABL *p){

  if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
   p->np2 = p->ftp->lenmask ? 0 : 1;
   
  return tableir_kontrol(csound,p);;
}

int tableirkt_audio(CSOUND *csound, TABL *p){

  if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
   p->np2 = p->ftp->lenmask ? 0 : 1; 
   
  return tableir_audio(csound,p);;
}

int table3rkt_kontrol(CSOUND *csound, TABL *p){

  if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
   p->np2 = p->ftp->lenmask ? 0 : 1; 
   
  return table3r_kontrol(csound,p);;
}

int table3rkt_audio(CSOUND *csound, TABL *p){

  if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
   p->np2 = p->ftp->lenmask ? 0 : 1;
   
  return table3r_audio(csound,p);
}

int tablew_init(CSOUND *csound, TABL *p){

  int ndx, len;
  int mask;
  MYFLT *func = p->ftp->ftable;
  int32 iwrap = *p->wrap;

  if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->InitError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
  mask = p->ftp->lenmask;
  p->np2 = mask ? 0 : 1;
  len = p->ftp->flen;

  if (*p->mode)
      p->mul = len;
    else
      p->mul = 1;
 
  ndx = MYFLOOR((*p->ndx + *p->offset)*p->mul  + (iwrap==2 ? 0.5:0));
  if(iwrap) {
    ndx = iwrap == 2 ? MYFLOOR(ndx+0.5) : ndx;
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  p->ftp->ftable[ndx] = *p->sig;
  if(ndx == 0 && iwrap==2) func[len] = func[ndx]; 
  return OK;
}

int tablew_kontrol(CSOUND *csound, TABL *p){
  int ndx, len = p->len;
  int mask = p->ftp->lenmask;
  MYFLT *func = p->ftp->ftable;
  int32 iwrap = p->iwrap;
  IGN(csound);

  ndx = MYFLOOR((*p->ndx + *p->offset)*p->mul + (iwrap==2 ? 0.5:0));
  if(iwrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
  func[ndx] = *p->sig; 
  if(ndx == 0 && iwrap==2) func[len] = func[ndx];
  return OK;
}

int tablew_audio(CSOUND *csound, TABL *p){
  int ndx, len = p->len, n, nsmps = csound->GetKsmps(csound);
  int mask = p->ftp->lenmask;
  MYFLT *sig = p->sig;
  MYFLT *ndx_f = p->ndx;
  MYFLT *func = p->ftp->ftable;
  MYFLT offset = *p->offset;
  MYFLT mul = p->mul;
  int32 iwrap = p->iwrap;
  uint32_t    koffset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;

 if (early) nsmps -= early;

 for(n=koffset; n < nsmps; n++){
   ndx = MYFLOOR((ndx_f[n] + offset)*mul + (iwrap==2 ? 0.5:0));
   if(iwrap) {
    if(p->np2){
      while(ndx >= len) ndx -= len;
      while(ndx < 0)  ndx += len;
    }
    else ndx &= mask;
  } else {
    if(UNLIKELY(ndx >= len)) ndx = len - 1;
    else if (UNLIKELY(ndx < 0)) ndx = 0;
  }
   func[ndx] = sig[n];
   if(iwrap==2 && ndx == 0) func[len] = func[ndx];  
}
  return OK;
}

int tablewkt_kontrol(CSOUND *csound, TABL *p){

  if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
   p->np2 = p->ftp->lenmask ? 0 : 1;
   
  return tablew_kontrol(csound,p);
}


int tablewkt_audio(CSOUND *csound, TABL *p){

  if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->ftable)) == NULL))
      return csound->PerfError(csound, Str("table: could not find ftable %d"), (int) *p->ftable);
   p->np2 = p->ftp->lenmask ? 0 : 1;
   
  return tablew_audio(csound,p);;
}

int table_length(CSOUND *csound, TLEN *p){
  FUNC *ftp;
  if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ftable)) == NULL)){
    csound->Warning(csound,  Str("table: could not find ftable %d"), (int) *p->ftable);
    *p->ans = FL(-1.0);
    return NOTOK;
  }
  else *p->ans = (MYFLT) ftp->flen;
  return OK;
}

int table_gpw(CSOUND *csound, TGP *p){
  FUNC *ftp;
  if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ftable)) == NULL)){
    csound->Warning(csound,  Str("table: could not find ftable %d"), (int) *p->ftable);
    return NOTOK;
  }
  ftp->ftable[ftp->flen] = ftp->ftable[0];
  return OK;
}

int table_copy(CSOUND *csound, TGP *p){
  FUNC *dest, *src;
  int32 len1, len2, i, rp;
  if (UNLIKELY((dest = csound->FTnp2Find(csound, p->ftable)) == NULL ||
	       (src = csound->FTnp2Find(csound, p->ftsrc)) == NULL)){
	csound->Warning(csound,  Str("table: could not find ftables %d and/or %d"), (int) *p->ftable, (int) *p->ftsrc);
    return NOTOK;
  }
  len1 = dest->flen;
  len2 = src->flen;
  for(i=rp=0; i<len1;i++){
    dest->ftable[i] = src->ftable[rp];
    rp = rp == len2 ? 0 : rp+1; 
  }
  return OK;
}
