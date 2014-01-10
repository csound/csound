// -*- c++ -*-
// adsyn.cu
// experimental cuda opcodes
//
// V Lazzarini, 2013

#include <csdl.h>
#include <cufft.h>
#define VSAMPS 16
#define MAXBLOCK 8192

#define PFRACLO(x)   ((MYFLT)((x) & lomask) * lodiv)

__global__ void component(MYFLT *out, int *ndx, MYFLT *tab,
                          float *amp, int *inc, int vsize,
                          int blocks, int lobits, MYFLT lodiv,
                          int lomask) {

  int h = threadIdx.x*blocks + blockIdx.x;
  int i, offset, n, lndx;

  offset = h*vsize;
  out += offset;

  for(i=0; i < vsize; i++) {
    lndx = ndx[h];
    n = lndx >> lobits;
    out[i] = amp[h]*(tab[n] +  PFRACLO(lndx)*(tab[n+1] - tab[n]));
    ndx[h] = (lndx + inc[h]) & PHMASK;
  }

}

__global__  void mixdown(MYFLT *out, int comps, int vsize, float kamp){
   int h = threadIdx.x;
   int i;
   for(i=0; i < comps; i++)
     out[h] += out[h + vsize*i];
   out[h] *= kamp;
}



static int destroy_cudaop(CSOUND *csound, void *pp);

typedef struct cudaop_ {
  OPDS h;
  MYFLT *asig;
  MYFLT *kamp, *kfreq, *itabn;
  MYFLT *ftabn, *atabn, *inum;
  MYFLT *out;
  float *amp;
  MYFLT *tab;
  int *ndx, *inc;
  MYFLT *ap, *fp;
  FUNC *itab, *ftab, *atab;
  int N, blocks;
} CUDAOP;

static int init_cudaop(CSOUND *csound, CUDAOP *p){

  int a, b, asize, ipsize, fpsize, tsize;
  int nsmps = CS_KSMPS;
  if(nsmps > 1024) return csound->InitError(csound, "ksmps is too large\n");

  if((p->itab =
      csound->FTFind(csound, p->itabn))== NULL)
    return csound->InitError(csound,
                             "could not find table %.0f\n", *p->itabn);

  if((p->ftab =
      csound->FTnp2Find(csound, p->ftabn))== NULL)
    return csound->InitError(csound,
                             "could not find table %.0f\n", *p->ftabn);

  if((p->atab =
      csound->FTnp2Find(csound, p->atabn))== NULL)
    return csound->InitError(csound,
                             "could not find table %.0f\n", *p->atabn);

  a = p->ftab->flen;
  b = p->atab->flen;
  p->N = a < b ? a : b;

  if(*p->inum > 0 && *p->inum < p->N) p->N = *p->inum;

  p->blocks = p->N > 1024 ? p->N/1024 : 1;

  asize = p->N*nsmps*sizeof(MYFLT);
  ipsize = p->N*sizeof(int);
  fpsize = p->N*sizeof(float);
  tsize = (p->itab->flen+1)*sizeof(MYFLT);

  cudaMalloc(&p->out, asize);
  cudaMalloc(&p->ndx, ipsize);
  cudaMalloc(&p->amp, fpsize);
  cudaMalloc(&p->inc, ipsize);
  cudaMalloc(&p->tab, tsize);
  cudaMemset(p->ndx, 0, ipsize);
  cudaMemcpy(p->tab, p->itab->ftable, tsize, cudaMemcpyHostToDevice);

  p->ap = p->atab->ftable;
  p->fp = p->ftab->ftable;

  csound->RegisterDeinitCallback(csound, p, destroy_cudaop);
  csound->Message(csound, "%d threads, %d blocks\n", p->N, p->blocks);
  return OK;
}

static void update_params(CSOUND *csound, CUDAOP *p){

  int ipsize = p->N*sizeof(int);
  int fpsize = p->N*sizeof(float);
  float amp[MAXBLOCK];
  int inc[MAXBLOCK], i, j;
  int N = p->N > MAXBLOCK ? MAXBLOCK : p->N;

  for(j=0; N > 0; j++,  N = p->N - N) {
   for(i=0;i < N; i++){
    amp[i] = p->ap[i];
    inc[i] = *p->kfreq*p->fp[i]*FMAXLEN/csound->GetSr(csound);
   }
  cudaMemcpy(&p->amp[N*j],amp,fpsize, cudaMemcpyHostToDevice);
  cudaMemcpy(&p->inc[N*j],inc,ipsize, cudaMemcpyHostToDevice);
 }

}

static int perf_cudaop(CSOUND *csound, CUDAOP *p){

  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS;
  p->ap = p->atab->ftable;
  p->fp = p->ftab->ftable;

  if (UNLIKELY(offset)) memset(p->asig, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&(p->asig[nsmps]), '\0', early*sizeof(MYFLT));
  }

  update_params(csound, p);
  component<<<p->blocks,
        p->N/p->blocks>>>(p->out,p->ndx,
                          p->tab,p->amp,
                          p->inc,nsmps,
                          p->blocks,
                          p->itab->lobits,
                          p->itab->lodiv,
                          p->itab->lomask);
   mixdown<<<1,nsmps>>>(p->out,p->N,nsmps,*p->kamp);
   cudaMemcpy(p->asig,p->out,nsmps*sizeof(MYFLT),cudaMemcpyDeviceToHost);

  return OK;
}

static int destroy_cudaop(CSOUND *csound, void *pp){
  CUDAOP *p = (CUDAOP *) pp;
  cudaFree(p->out);
  cudaFree(p->ndx);
  cudaFree(p->tab);
  cudaFree(p->amp);
  cudaFree(p->inc);
  return OK;
}


#include <pstream.h>

typedef struct cudaop2_ {
  OPDS h;
  MYFLT *asig;
  PVSDAT *fsig;
  MYFLT *kamp, *kfreq, *itabn;
  MYFLT *inum;
  MYFLT *out;
  float *amp;
  MYFLT *tab;
  int *ndx, *inc;
  float *fp;
  AUXCH out_;
  FUNC *itab;
  int N, blocks;
  int count;
  int vsamps;
  int framecount;
} CUDAOP2;

static int destroy_cudaop2(CSOUND *csound, void *pp);

static int init_cudaop2(CSOUND *csound, CUDAOP2 *p){

  int asize, ipsize, fpsize, tsize;
  if(p->fsig->overlap > 1024)
     return csound->InitError(csound, "overlap is too large\n");

  if((p->itab =
      csound->FTFind(csound, p->itabn))== NULL)
    return csound->InitError(csound,
                             "could not find table %.0f\n", *p->itabn);
  p->N = (p->fsig->N)/2;

  if(*p->inum > 0 && *p->inum < p->N) p->N = *p->inum;

  p->blocks = p->N > 1024 ? p->N/1024 : 1;
  p->vsamps = p->fsig->overlap < VSAMPS ? VSAMPS : p->fsig->overlap;

  asize = p->N*p->vsamps*sizeof(MYFLT);
  ipsize = p->N*sizeof(int);
  fpsize = p->N*sizeof(float);
  tsize = (p->itab->flen+1)*sizeof(MYFLT);

  cudaMalloc(&p->out, asize);
  cudaMalloc(&p->ndx, ipsize);
  cudaMalloc(&p->amp, fpsize);
  cudaMalloc(&p->inc, ipsize);
  cudaMalloc(&p->tab, tsize);
  cudaMemset(p->ndx, 0, ipsize);
  cudaMemcpy(p->tab,p->itab->ftable,tsize, cudaMemcpyHostToDevice);

  asize = p->vsamps*sizeof(MYFLT);
  if(p->out_.auxp == NULL ||
     p->out_.size < asize)
    csound->AuxAlloc(csound, asize , &p->out_);

  csound->RegisterDeinitCallback(csound, p, destroy_cudaop2);
  p->count = 0;

  csound->Message(csound, "%d threads, %d blocks\n", p->N, p->blocks);

  return OK;
}

static void update_params2(CSOUND *csound, CUDAOP2 *p){

  int ipsize = p->N*sizeof(int);
  int fpsize = p->N*sizeof(float);
  float amp[MAXBLOCK];
  int inc[MAXBLOCK], i, j, k;
  int N = p->N > MAXBLOCK ?  MAXBLOCK : p->N;

 for(k=0; N > 0; k++,  N = p->N - N) {
  for(j=i=0;i < N; i++, j+=2){
    amp[i] = p->fp[j];
    inc[i] = MYFLT2LONG(*p->kfreq * p->fp[j+1]*FMAXLEN/csound->GetSr(csound));
  }
  cudaMemcpy(&p->amp[k*N],amp,fpsize, cudaMemcpyHostToDevice);
  cudaMemcpy(&p->inc[k*N],inc,ipsize, cudaMemcpyHostToDevice);
 }
}

static int perf_cudaop2(CSOUND *csound, CUDAOP2 *p){

  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT *out_ = (MYFLT *) p->out_.auxp;
  MYFLT      *asig = p->asig;
  int count = p->count,  vsamps = p->vsamps;
  p->fp = (float *) (p->fsig->frame.auxp);

  if (UNLIKELY(offset)) memset(asig, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&asig[nsmps], '\0', early*sizeof(MYFLT));
  }

  for(n=offset; n < nsmps; n++){
    if(count == 0) {
      update_params2(csound, p);
      component<<<p->blocks,
        p->N/p->blocks>>>(p->out,p->ndx,
                          p->tab,p->amp,
                          p->inc,p->vsamps,
                          p->blocks,
                          p->itab->lobits,
                          p->itab->lodiv,
                          p->itab->lomask);
      mixdown<<<1,vsamps>>>(p->out,p->N,vsamps,*p->kamp);
      cudaMemcpy(out_,p->out,vsamps*sizeof(MYFLT),cudaMemcpyDeviceToHost);
      count = vsamps;
    }
    asig[n] = (MYFLT) out_[vsamps - count];
    count--;
  }
  p->count = count;
  return OK;
}

static int destroy_cudaop2(CSOUND *csound, void *pp){
  CUDAOP2 *p = (CUDAOP2 *) pp;
  cudaFree(p->out);
  cudaFree(p->ndx);
  cudaFree(p->tab);
  cudaFree(p->amp);
  cudaFree(p->inc);
  return OK;
}


static OENTRY localops[] = {
  {"cudasynth", sizeof(CUDAOP),0, 5, "a", "kkiiio", (SUBR) init_cudaop, NULL,
   (SUBR) perf_cudaop},
  {"cudasynth", sizeof(CUDAOP2),0, 5, "a", "fkkio", (SUBR) init_cudaop2, NULL,
   (SUBR) perf_cudaop2}
};

extern "C" {
  LINKAGE
}
