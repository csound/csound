/*
  lpred.c:

  Copyright 2020 Victor Lazzarini

  streaming linear prediction 

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

#include <stdlib.h>
#include <math.h>
#include "csoundCore.h"
#include "csound.h"
#include "fftlib.h"
#include "lpred.h"

/** autocorrelation  
    r - output 
    s - input
    size - input size
    returns r
*/
MYFLT *csoundAutoCorrelation(CSOUND *csound, MYFLT *r, MYFLT *s, int size){
  MYFLT sum;
  int n,m,o;
  for(n=0; n < size; n++) {
    sum = FL(0.0);
    for(m=n,o=0; m < size; m++,o++) 
      sum += s[o]*s[m];
    r[n] = sum;
  }
  return r;
}  

typedef struct LPCparam_ {
  MYFLT *r, *E, *b, *k;
  int32_t N, M;
} LPCparam;


/** Set up linear prediction memory for 
    autocorrelation size N and predictor order M
*/
void *csoundLPsetup(CSOUND *csound, int N, int M) {
  LPCparam *p = csound->Calloc(csound, sizeof(LPCparam));
  if (N < M+1) N = M+1;
  p->r = csound->Calloc(csound, sizeof(MYFLT)*N);
  p->E = csound->Calloc(csound, sizeof(MYFLT)*(M+1));
  p->k = csound->Calloc(csound, sizeof(MYFLT)*(M+1));
  p->b = csound->Calloc(csound, sizeof(MYFLT)*(M+1)*(M+1));
  p->N = N;
  p->M = M;
  return p;    
}

/** Free linear prediction memory
 */
void csoundLPfree(CSOUND *csound, void *parm) {
  LPCparam *p = (LPCparam *) parm;
  csound->Free(csound, p->r);
  csound->Free(csound, p->b);
  csound->Free(csound, p->k);
  csound->Free(csound, p->E);
  csound->Free(csound, p);
}
 
/** Linear Prediction function
    output format: M+1 MYFLT array [E,c1,c2,...,cm]
    NB: c0 is always 1
*/
MYFLT *csoundLPread(CSOUND *csound, void *parm, MYFLT *x){

  LPCparam *p = (LPCparam *) parm;
  MYFLT *r = p->r;
  MYFLT *E = p->E;
  MYFLT *b = p->b;
  MYFLT *k = p->k;
  MYFLT s;
  int N = p->N;
  int M = p->M;
  int L = M+1;
  int m,i;
   
  r = csoundAutoCorrelation(csound,r,x,N);
  MYFLT ro = r[0];
  if (ro > FL(0.0)) {
    /* if signal power > 0 , do linear prediction */
    for(i=0;i<L;i++) r[i] /= ro;
    E[0] = r[0];
    b[M*L] = 1.;
    for(m=1;m<L;m++) {
      s = 0.;
      b[(m-1)*L] = 1.;
      for(i=0;i<m;i++) 
        s += b[(m-1)*L+i]*r[m-i];
      k[m] = -(s)/E[m-1];
      b[m*L+m] = k[m];
      for(i=1;i<m;i++)
        b[m*L+i] = b[(m-1)*L+i] + k[m]*b[(m-1)*L+(m-i)];
      E[m] = (1 - k[m]*k[m])*E[m-1];
    }
    /* replace first coeff with E*/
    b[M*L] = E[M];
  }
  /* return E + coeffs */
  return &b[M*L];
} 
 
/** LP coeffs to Cepstrum
    takes an array c of N size
    and an array b of M+1 size with M all-pole coefficients
    and E in place of coefficient 0 [E,c1,...,cM]
    returns N cepstrum coefficients
*/
MYFLT *csoundLPCeps(CSOUND *csound, MYFLT *c, MYFLT *b,
                    int N, int M){
  int n,m;
  MYFLT s;
  c[0] = -LOG(b[0]);
  c[1] = b[1];
  for(n=2;n<N;n++){
    if(n > M)
      c[n] = 0;
    else {
      s = 0.;
      for(m=1;m<n;m++)
        s += (m/n)*c[m]*b[n-m];
      c[n] = b[n] - s;
    }
  }
  for(n=0;n<N;n++) c[n] *= -1;
  return c;    
}

/** LP coeffs to Cepstrum
    takes an array c of N size
    and an array b of M+1 size 
    returns M lp coefficients and E in place of 
    of coefficient 0 [E,c1,...,cM]
*/
MYFLT *csoundCepsLP(CSOUND *csound, MYFLT *b, MYFLT *c,
                    int M, int N){
  int n,m;
  MYFLT s;
  b[1] = -c[1];
  for(m=2;m<M+1;m++) {
    s = 0.;
    for(n=1;n<m;n++)
      s -= (m-n)*b[n]*c[m-n];
    b[m] = -c[m] + s/m;
  }
  b[0] = EXP(c[0]);
  return b;
}

/** Computes real cepstrum in place from a 
    non-negative (Hermitian) spectrum

    buf: non-negative spectrum with [DC,Nyq] in the first two positions
    size: size of buf    

    returns: real-valued (and even) cepstrum 
    NB: this uses the power spectrum to compute cepstrum
*/
MYFLT *csoundRealCepstrum(CSOUND *csound, MYFLT *buf, int size){
  int i, lps;
  buf[1] = 0;
  for(i = 2; i < size; i+=2) {
    lps = LOG(buf[i]*buf[i] + buf[i+1]*buf[i+1]);
    buf[i] = lps;
    buf[i+1] = 0;
  }
  csoundInverseRealFFT(csound, buf, size);
  return buf;  
}

/** Computes non-negative power spectrum in place from a 
    real-valued (and even) cepstrum

    buf: real-valued cepstrum 
    size: size of buf    

    returns: non-negative power spectrum in even-index
    array positions
*/
MYFLT *csoundInverseRealCepstrum(CSOUND *csound, MYFLT *buf, int size){
  int i, ps;
  csoundRealFFT(csound, buf, size);
  for(i = 2; i < size; i+=2) {
    ps = EXP(buf[i]);
    buf[i] = ps;
    buf[i+1] = FL(0.0);
  }
  buf[1] = 0;
  return buf;  
} 

/* opcodes */


/* lpcfilter - take lpred input from table */
int32_t lpfil_init(CSOUND *csound, LPCFIL *p) {
 
  FUNC *ft = csound->FTnp2Find(csound, p->ifn);

  if (ft != NULL) {
    MYFLT *coefs;
    int N = *p->isiz < ft->flen ? *p->isiz : ft->flen;
    int Nbytes = N*sizeof(MYFLT);
    int Mbytes = *p->iord*sizeof(MYFLT);
    p->M = *p->iord;
    p->N = N;

    p->setup = csound->LPsetup(csound,N,p->M);
    if(*p->iwin != 0) {
      FUNC *ftw = csound->FTnp2Find(csound, p->iwin);
      MYFLT *buf, incr, k;
      int i;
      p->wlen = ftw->flen;
      p->win = ftw->ftable;
      if(p->buf.auxp == NULL || Nbytes > p->buf.size) 
        csound->AuxAlloc(csound, Nbytes, &p->buf);
      buf = (MYFLT*) p->buf.auxp;
      incr = p->wlen/N;
      for(i=0, k=0; i < N; i++, k+=incr) 
        buf[i] = ft->ftable[i]*p->win[(int)k];
      coefs = csound->LPread(csound,p->setup,buf);
    } 
    else {
      p->win = NULL;
      coefs = csound->LPread(csound,p->setup,ft->ftable);
    }
  
    if(p->coefs.auxp == NULL || Mbytes > p->coefs.size) 
      csound->AuxAlloc(csound, Mbytes, &p->coefs);
    memcpy(p->coefs.auxp, &coefs[1], Mbytes);

    if(p->del.auxp == NULL || Mbytes > p->del.size) 
      csound->AuxAlloc(csound, Mbytes, &p->del);
    memset(p->del.auxp, 0, Mbytes);

    p->g = SQRT(coefs[0]);
    p->rp = 0;
    p->ft = ft;
    return OK;
  }
  csound->InitError(csound, "function table %d not found\n", (int) *p->ifn);
  return NOTOK;
}


int32_t lpfil_perf(CSOUND *csound, LPCFIL *p) {
  MYFLT *cfs = (MYFLT *) p->coefs.auxp;
  MYFLT *yn = (MYFLT *) p->del.auxp;
  MYFLT *out = p->out;
  MYFLT *in = p->in;
  MYFLT y, g = p->g;
  int32_t M = p->M, m;
  int32_t pp, rp = p->rp;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  if (UNLIKELY(offset)) {
    memset(out, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  if(*p->kflag) {
    MYFLT *c;
    int32_t off = *p->koff;
    int32_t len = p->ft->flen;
    if (off + p->N > len)
      off = len - p->N;
    if(p->win) {
      MYFLT *buf, incr, k;
      int i, N = p->N;
      
      buf = (MYFLT*) p->buf.auxp;
      incr = p->wlen/N;
      for(i=0, k=0; i < N; i++, k+=incr) 
        buf[i] = p->ft->ftable[i+off]*p->win[(int)k];
      c = csound->LPread(csound,p->setup,buf);
    } else
      c = csound->LPread(csound,p->setup,
                       p->ft->ftable+off);
    memcpy(p->coefs.auxp, &c[1], M*sizeof(MYFLT));
    g = p->g = SQRT(c[0]);
  }

  for(n=offset; n < nsmps; n++) {
    pp = rp;
    y =  in[n]*g;
    for(m = 0; m < M; m++) {
      // filter convolution
      y -= cfs[M - m - 1]*yn[pp];
      pp = pp != M - 1 ? pp + 1: 0;
    } 
    out[n] = yn[rp] = y;
    rp = rp != M - 1 ? rp + 1: 0;
  }
  p->rp = rp;
  return OK;
}


/* lpcfilter - take lpred input from sig */
int32_t lpfil2_init(CSOUND *csound, LPCFIL2 *p) {
    int Nbytes = *p->isiz*sizeof(MYFLT);
    int Mbytes = *p->iord*sizeof(MYFLT);
    p->M = *p->iord;
    p->N = *p->isiz;

    if(*p->iwin != 0) {
      FUNC *ftw = csound->FTnp2Find(csound, p->iwin);
      p->wlen = ftw->flen;
      p->win = ftw->ftable;
    } else p->win = NULL;
    
    p->setup = csound->LPsetup(csound,p->N,p->M);

    if(p->cbuf.auxp == NULL || Nbytes > p->cbuf.size) 
      csound->AuxAlloc(csound, Nbytes, &p->cbuf);
    if(p->buf.auxp == NULL || Mbytes > p->buf.size) 
      csound->AuxAlloc(csound, Nbytes, &p->buf);
    
    if(p->coefs.auxp == NULL || Mbytes > p->coefs.size) 
      csound->AuxAlloc(csound, Mbytes, &p->coefs);
    if(p->del.auxp == NULL || Mbytes > p->del.size) 
      csound->AuxAlloc(csound, Mbytes, &p->del);
    memset(p->del.auxp, 0, Mbytes);

    p->g = FL(0.0);
    p->rp = p->bp = 0;
    return OK;
}

int32_t lpfil2_perf(CSOUND *csound, LPCFIL2 *p) {
  MYFLT *cfs = (MYFLT *) p->coefs.auxp;
  MYFLT *buf = (MYFLT *) p->buf.auxp;
  MYFLT *cbuf = (MYFLT *) p->cbuf.auxp;
  MYFLT *yn = (MYFLT *) p->del.auxp;
  MYFLT *out = p->out;
  MYFLT *in = p->in;
  MYFLT *sig = p->sig;
  MYFLT y, g = p->g;
  int32_t M = p->M, m;
  int32_t N = p->N;
  int32_t pp, rp = p->rp, bp = p->bp;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  if (UNLIKELY(offset)) {
    memset(out, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for(n=offset; n < nsmps; n++) {
    cbuf[bp] = sig[n];
    bp = bp != N - 1 ? bp + 1 : 0;
    if(rp == 0) {
      MYFLT *c, k, incr = p->wlen/N;
      int32_t j,i;
      for (j=bp,i=0, k=0; i < N; j++,i++,k+=incr) {
        buf[i] = p->win == NULL ? cbuf[j%N] : p->win[(int)k]*cbuf[j%N];
      }
      c = csound->LPread(csound,p->setup,buf);
      memcpy(p->coefs.auxp, &c[1], M*sizeof(MYFLT));
      g = p->g = SQRT(c[0]);
    }
    pp = rp;
    y =  in[n]*g;
    for(m = 0; m < M; m++) {
      // filter convolution
      y -= cfs[M - m - 1]*yn[pp];
      pp = pp != M - 1 ? pp + 1: 0;
    } 
    out[n] = yn[rp] = y;
    rp = rp != M - 1 ? rp + 1: 0;
  }
  p->rp = rp;
  p->bp = bp;
  return OK;
}
