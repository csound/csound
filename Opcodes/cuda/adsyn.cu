// -*- c++ -*-
// adsyn.cu
// experimental cuda opcodes
//
// V Lazzarini, 2013

#include <csdl.h>
#include <cufft.h>
#define VSAMPS 64

//__shared__ MYFLT mema[64*20];
#define PFRACLO(x)   ((MYFLT)((x) & lomask) * lodiv)

__global__ void component_table(MYFLT *out, int64_t *ndx, MYFLT *tab,
                          float *amp, int *inc, int vsize,
                          int blocks, int lobits, MYFLT lodiv,
                          int lomask) {

  int h = threadIdx.x*blocks + blockIdx.x;
  int i, offset, n;
  int64_t lndx;
  offset = h*vsize;
  out += offset;

  for(i=0; i < vsize; i++) {
    lndx = ndx[h];
    n = lndx >> lobits;
    out[i] = amp[h]*(tab[n] +  PFRACLO(lndx)*(tab[n+1] - tab[n]));
    ndx[h] = (lndx + inc[h]) & PHMASK;
  }

}

__global__ void component_sine(MYFLT *out, int64_t *ndx,
                          float *amp, int *inc, int vsize,
                          int blocks) {

  int h = threadIdx.x*blocks + blockIdx.x;
  int i, offset;
  int64_t lndx;
  offset = h*vsize;
  out += offset;

  for(i=0; i < vsize; i++) {
    lndx = ndx[h];
    out[i] = amp[h]*sin((PI*2*lndx)/FMAXLEN);
    ndx[h] = (lndx + inc[h]) & PHMASK;
  }

}

__global__  void mixdown_(MYFLT *out, int comps, int vsize, float kamp){
   int h = threadIdx.x;
   int i;
   for(i=1; i < comps; i++){
     out[h] +=  out[h + vsize*i];
   }
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
  int64_t *ndx; 
  int *inc;
  MYFLT *ap, *fp;
  FUNC *itab, *ftab, *atab;
  int N, blocks;
} CUDAOP;

static int init_cudaop(CSOUND *csound, CUDAOP *p){

  int a, b, asize, ipsize, fpsize, tsize;
  int nsmps = CS_KSMPS, blockspt;
  if(nsmps > 1024) return csound->InitError(csound, "ksmps is too large\n");
  cudaDeviceProp deviceProp;
  cudaGetDeviceProperties(&deviceProp, 0);
  blockspt = deviceProp.maxThreadsPerBlock;
  if(deviceProp.major < 3) 
    csound->InitError(csound, 
     "this opcode requires device capability 3.0 minimum\n");

  if(*p->itabn != 0){
  if((p->itab =
      csound->FTFind(csound, p->itabn))== NULL)
    return csound->InitError(csound,
                             "could not find table %.0f\n", *p->itabn);
  } else p->itab = NULL;

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

  p->blocks = p->N > blockspt ? p->N/blockspt : 1;

  asize = p->N*nsmps*sizeof(MYFLT);
  ipsize = p->N*sizeof(int64_t);
  fpsize = p->N*sizeof(float);
  if(p->itab)
   tsize = (p->itab->flen+1)*sizeof(MYFLT);

  cudaMalloc(&p->out, asize);
  cudaMalloc(&p->ndx, ipsize);
  cudaMalloc(&p->amp, fpsize);
  cudaMalloc(&p->inc, ipsize);
  if(p->itab) {
   cudaMalloc(&p->tab, tsize);
   cudaMemcpy(p->tab, p->itab->ftable, tsize, cudaMemcpyHostToDevice);
  }
  cudaMemset(p->ndx, 0, ipsize);

  p->ap = p->atab->ftable;
  p->fp = p->ftab->ftable;

  csound->RegisterDeinitCallback(csound, p, destroy_cudaop);
  csound->Message(csound, "%d threads, %d blocks\n", p->N, p->blocks);
  return OK;
}

static void update_params(CSOUND *csound, CUDAOP *p){

  int ipsize = p->N*sizeof(int);
  int fpsize = p->N*sizeof(float);
  float amp[p->N];
  int inc[p->N], i;
  int N = p->N;

   for(i=0;i < N; i++){
    amp[i] = p->ap[i];
    inc[i] = *p->kfreq*p->fp[i]*FMAXLEN/csound->GetSr(csound);
   }
   cudaMemcpy(p->amp,amp,fpsize, cudaMemcpyHostToDevice);
   cudaMemcpy(p->inc,inc,ipsize, cudaMemcpyHostToDevice);
 
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
  if(p->itab)
   component_table<<<p->blocks,
        p->N/p->blocks>>>(p->out,p->ndx,
                          p->tab,p->amp,
                          p->inc,nsmps,
                          p->blocks,
                          p->itab->lobits,
                          p->itab->lodiv,
                          p->itab->lomask);
  else
   component_sine<<<p->blocks,
        p->N/p->blocks>>>(p->out,p->ndx,
                          p->amp,
                          p->inc,nsmps,
                          p->blocks);
   mixdown_<<<1,nsmps>>>(p->out,p->N,nsmps,*p->kamp);
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
  MYFLT *kamp, *kfreq;
  MYFLT *inum;
  MYFLT *out;
  float *frame;
  int64_t *ndx;
  float *fp, *previous;
  AUXCH out_;
  int N, blocks;
  int count;
  int vsamps, vblocks;
  int framecount;
} CUDAOP2;

static int destroy_cudaop2(CSOUND *csound, void *pp);

static int init_cudaop2(CSOUND *csound, CUDAOP2 *p){

  int asize, ipsize, fpsize, blockspt;
  if(p->fsig->overlap > 1024)
     return csound->InitError(csound, "overlap is too large\n");
  cudaDeviceProp deviceProp;
  cudaGetDeviceProperties(&deviceProp, 0);
  blockspt = deviceProp.maxThreadsPerBlock;
  if(deviceProp.major < 3) 
    csound->InitError(csound, 
   "this opcode requires device capability 3.0 minimum\n");

  p->N = (p->fsig->N)/2;

  if(*p->inum > 0 && *p->inum < p->N) p->N = *p->inum;

  p->blocks = p->N > blockspt ? p->N/blockspt : 1;
  p->vsamps = p->fsig->overlap < VSAMPS ? VSAMPS : p->fsig->overlap;
  p->vblocks = p->vsamps >  blockspt ? p->vsamps/blockspt : 1;

  asize = p->N*p->vsamps*sizeof(MYFLT);
  ipsize = p->N*sizeof(int64_t);
  fpsize = p->N*sizeof(float)*2;

  cudaMalloc(&p->out, asize);
  cudaMalloc(&p->ndx, ipsize);
  cudaMalloc(&p->frame, fpsize);
  cudaMalloc(&p->previous, fpsize);
  cudaMemset(p->previous, 0, fpsize);
  cudaMemset(p->ndx, 0, ipsize);

  asize = p->vsamps*sizeof(MYFLT);
  if(p->out_.auxp == NULL ||
     p->out_.size < asize)
    csound->AuxAlloc(csound, asize , &p->out_);

  csound->RegisterDeinitCallback(csound, p, destroy_cudaop2);
  p->count = 0;
  return OK;
}


__global__ void partial(MYFLT *out, float *frame, MYFLT pitch, int64_t *ph, float *amps, 
                      int vsize, MYFLT sr) {

  int h = threadIdx.x + blockIdx.x*blockDim.x;
  int i, offset;
  int64_t inc,lph;
  int k = h << 1;
  float amp = amps[h];
  float ampinc = (frame[k] - amp)/vsize;
  offset = h*vsize;
  out += offset;
  lph = ph[h];
  inc =  round(pitch*frame[k+1]*FMAXLEN/sr);
  for(i=0; i < vsize; i++) {
    out[i] = 
    amp*SIN((2*PI*lph)/FMAXLEN);
    lph = (lph + inc) & PHMASK;
    amp += ampinc;
  }
  amps[h] = amp;
  ph[h] = lph;
}

__global__  void mixdown(MYFLT *out, int comps, int vsize, float kamp){
   int h =  threadIdx.x + blockIdx.x*blockDim.x;
   int i;
   for(i=1; i < comps; i++){
     out[h] +=  out[h + vsize*i];
   }
   out[h] *= kamp;
}

__global__ void sample(MYFLT *out, float *frame, MYFLT amp, MYFLT pitch, int64_t *ph, float *amps, 
                       int bins, int vsize, MYFLT sr) {

  int n = threadIdx.x + blockIdx.x*blockDim.x, h,k;
  int64_t lph;
  float a, ascl = ((float)n)/vsize;
  MYFLT scal = pitch*FMAXLEN/sr;
  out[n] = 0;
  for(h=k=0; h < bins; h++, k+=2){
          lph = (ph[h] + (int64_t)(n*round(frame[k+1]*scal))) & PHMASK;
          a = amps[h] + ascl*(frame[k] - amps[h]);
          out[n] += a*SIN((2*PI*lph)/FMAXLEN);
  }
  out[n] *= amp;   
}

__global__ void update(float *frame, float *amps, int64_t *ph, MYFLT pitch, int vsize, MYFLT sr){

 int h = threadIdx.x + blockIdx.x*blockDim.x;
 int k = h << 1;
 ph[h]  = (ph[h] + (int64_t)(vsize*round(pitch*frame[k+1]*FMAXLEN/sr))) & PHMASK;
 amps[h] =  amps[h] + vsize*(frame[k] - amps[h])/vsize;

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
      cudaMemcpy(p->frame,p->fp,sizeof(float)*p->N*2,cudaMemcpyHostToDevice);
      if(p->N > vsamps) {
       partial<<<p->blocks,p->N/p->blocks>>>
                           (p->out,p->frame, 
                            *p->kfreq, p->ndx, p->previous,
                            p->vsamps,
                            csound->GetSr(csound));
       mixdown<<<p->vblocks,vsamps/p->vblocks>>>(p->out,
                                                 p->N,
                                                 vsamps,
                                                 *p->kamp);
      } else {   	
       sample<<<p->vblocks,vsamps/p->vblocks>>>(p->out,
						p->frame,
						*p->kamp,
						*p->kfreq,
						p->ndx,
						p->previous,
						p->N,
						vsamps, 
                                                csound->GetSr(csound));
       update<<<p->blocks,p->N/p->blocks>>>(p->frame,
					    p->previous,
					    p->ndx,
					    *p->kfreq,
					    vsamps,
					    csound->GetSr(csound));
       }
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
  cudaFree(p->frame);
  return OK;
}


static OENTRY localops[] = {
  {"cudasynth", sizeof(CUDAOP),0, 5, "a", "kkiiio", (SUBR) init_cudaop, NULL,
   (SUBR) perf_cudaop},
  {"cudasynth", sizeof(CUDAOP2),0, 5, "a", "fkko", (SUBR) init_cudaop2, NULL,
   (SUBR) perf_cudaop2}
};

extern "C" {
  LINKAGE
}
