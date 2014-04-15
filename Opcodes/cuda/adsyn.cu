// -*- c++ -*-
// adsyn.cu
// experimental cuda opcodes
//
// V Lazzarini, 2013

#include <csdl.h>
#include <pstream.h>

typedef struct cudadsyn_ {
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
  int bins, blocks, threads;
  int count;
  int vsamps, mblocks, mthreads;
  int framecount;
} CUDADSYN;

static int destroy_cudadsyn(CSOUND *csound, void *pp);

static int init_cudadsyn(CSOUND *csound, CUDADSYN *p){

  int asize, ipsize, fpsize, blockspt;
  if(p->fsig->overlap > 1024)
     return csound->InitError(csound, "overlap is too large\n");
  cudaDeviceProp deviceProp;
  cudaGetDeviceProperties(&deviceProp, 0);
  blockspt = deviceProp.maxThreadsPerBlock;
  if(deviceProp.major < 3)
    csound->InitError(csound,
   "this opcode requires device capability 3.0 minimum\n");

  p->bins = (p->fsig->N)/2;

  if(*p->inum > 0 && *p->inum < p->bins) p->bins = *p->inum;

  p->vsamps = p->fsig->overlap;
  p->threads = p->bins*p->vsamps;
  p->blocks = p->threads > blockspt ? p->threads/blockspt : 1;
  p->mthreads = p->bins > p->vsamps ? p->bins : p->vsamps;
  p->mblocks = p->mthreads >  blockspt ? p->mthreads/blockspt : 1;

  p->threads /= p->blocks;
  p->mthreads /= p->mblocks;

  asize = p->bins*p->vsamps*sizeof(MYFLT);
  ipsize  =p->fsig->N*sizeof(int64_t)/2;
  fpsize = p->fsig->N*sizeof(float)*2;

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

  csound->RegisterDeinitCallback(csound, p, destroy_cudadsyn);
  p->count = 0;
  return OK;
}

//__shared__ int64_t ph[2048];

__global__ void sample(MYFLT *out, float *frame, MYFLT pitch, int64_t *ph,
                       float *amps, int bins, int vsize, MYFLT sr) {

  int t = (threadIdx.x + blockIdx.x*blockDim.x);
  int n =  t%vsize;  /* sample index */
  int h = t/vsize;  /* bin index */
  int k = h<<1;
  int64_t lph;
  float a = amps[h], ascl = ((float)n)/vsize;
  MYFLT fscal = pitch*FMAXLEN/sr;
  lph = (ph[h] + (int64_t)(n*round(frame[k+1]*fscal))) & PHMASK;
  a += ascl*(frame[k] - a);
  out[t] = a*sinf((2*PI*lph)/FMAXLEN);
}

__global__ void updatemix(MYFLT *out, float *frame, float *amps, MYFLT kamp,
           int64_t *ph, MYFLT pitch, int bins, int vsize, MYFLT sr){

 int h = threadIdx.x + blockIdx.x*blockDim.x;
 int k = h << 1, i,j;
 /* update phases and amps */
 ph[h]  = (ph[h] + (int64_t)(vsize*round(pitch*frame[k+1]*FMAXLEN/sr))) & PHMASK;
 amps[h] = frame[k];
 if(h >= vsize) 
   return;
 /* mix all partials */
 for(i=1, j= vsize; i < bins; i++, j+=vsize){
    out[h] +=  out[h + j];
  }
 out[h] *= kamp;
}

static int perf_cudadsyn(CSOUND *csound, CUDADSYN *p){

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
      cudaMemcpy(p->frame,p->fp,sizeof(float)*p->bins*2,cudaMemcpyHostToDevice);
      sample<<<p->blocks,p->threads>>>(p->out,p->frame,
                                               *p->kfreq,
                                                p->ndx,
                                                p->previous,
                                                p->bins,
                                                vsamps,
                                                csound->GetSr(csound));
      if (cudaDeviceSynchronize() != cudaSuccess)
      csound->Message(csound,"Cuda error: Failed to synchronize\n");
      updatemix<<<p->mblocks,p->mthreads>>>(p->out, p->frame,
                                            p->previous, *p->kamp,
                                            p->ndx,
                                            *p->kfreq,
                                            p->bins,
                                            vsamps,
                                            csound->GetSr(csound));
      cudaMemcpy(out_,p->out,vsamps*sizeof(MYFLT),cudaMemcpyDeviceToHost);
      count = vsamps;
    }
    asig[n] = (MYFLT) out_[vsamps - count];
    count--;
  }
  p->count = count;
  return OK;
}

static int destroy_cudadsyn(CSOUND *csound, void *pp){
  CUDADSYN *p = (CUDADSYN *) pp;
  cudaFree(p->out);
  cudaFree(p->ndx);
  cudaFree(p->frame);
  return OK;
}


static OENTRY localops[] = {
  {"cudasynth", sizeof(CUDADSYN),0, 5, "a", "fkko", (SUBR) init_cudadsyn, NULL,
   (SUBR) perf_cudadsyn}
};

extern "C" {
  LINKAGE
}
