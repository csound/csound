// -*- c++ -*-
/* adsyn.cu
   experimental cuda opcodes

  (c) Victor Lazzarini, 2013

  based on M Puckette's pitch tracking algorithm.

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

#include <csdl.h>
#include <pstream.h>

typedef struct cudadsyn_ {
  OPDS h;
  MYFLT *asig;
  PVSDAT *fsig;
  MYFLT *kamp, *kfreq;
  MYFLT *inum;
  float *out;
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
  if(deviceProp.major < 2)
   csound->InitError(csound,
       "this opcode requires device capability 2.0 minimum. Device is %d.%d\n", 
        deviceProp.major, deviceProp.minor );

  p->bins = (p->fsig->N)/2;

  if(*p->inum > 0 && *p->inum < p->bins) p->bins = *p->inum;

  p->vsamps = p->fsig->overlap;
  p->threads = p->bins*p->vsamps;
  p->blocks = p->threads > blockspt ? p->threads/blockspt : 1;
  p->mthreads = p->bins;
  p->mblocks = p->mthreads >  blockspt ? p->mthreads/blockspt : 1;

  p->threads /= p->blocks;
  p->mthreads /= p->mblocks;

  asize =  p->vsamps*sizeof(float);
  ipsize = p->fsig->N*sizeof(int64_t)/2;
  fpsize = p->fsig->N*sizeof(float);

  cudaMalloc(&p->out, asize);
  cudaMalloc(&p->ndx, ipsize);
  cudaMalloc(&p->frame, fpsize);
  cudaMalloc(&p->previous, fpsize/2);
  cudaMemset(p->previous, 0, fpsize/2);
  cudaMemset(p->ndx, 0, ipsize);
  cudaMemset(p->out, 0, sizeof(float)*p->vsamps);

  asize = p->vsamps*sizeof(float);
  if(p->out_.auxp == NULL ||
     p->out_.size < asize)
    csound->AuxAlloc(csound, asize , &p->out_);
 
  csound->RegisterDeinitCallback(csound, p, destroy_cudadsyn);
  p->count = 0;
  return OK;
}

__global__ void sample(float *out, float *frame, float pitch, int64_t *ph,
                       float *amps, int vsize, float sr) {
  
  int t = (threadIdx.x + blockIdx.x*blockDim.x);
  int n =  t%vsize;  /* sample index */
  int h = t/vsize;  /* bin index */
  //if(amps[h] < 0.001) return;
  int k = h<<1;
  int64_t lph;
  float a = amps[h], ascl = ((float)n)/vsize;
  int64_t d =  (int64_t) round(frame[k+1]*pitch*FMAXLEN/sr);
  lph = ph[h] + n*d;
  a += ascl*(frame[k] - a);
  atomicAdd(&out[n], a*sinf((2*PI*lph)/FMAXLEN));
   // if(n) return;
   // ph[h]  =  (ph[h] + vsize*d) & PHMASK;
   // amps[h] = frame[k];
}

__global__ void update(float *frame, float *amps,
      int64_t *ph,float pitch, int vsize, float sr){

 int h = threadIdx.x + blockIdx.x*blockDim.x;
 int k = h << 1;
 /* update phases and amps */
 ph[h]  = (ph[h] + (int64_t)(vsize*round(pitch*frame[k+1]*FMAXLEN/sr))) & PHMASK;
 amps[h] = frame[k];
}

static int perf_cudadsyn(CSOUND *csound, CUDADSYN *p){

  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  float *out_ = (float *) p->out_.auxp;
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
      cudaMemcpy(out_,p->out,vsamps*sizeof(float),cudaMemcpyDeviceToHost);
      update<<<p->mblocks,p->mthreads>>>(p->frame,
                                            p->previous,
                                             p->ndx,
                                             *p->kfreq,
                                             vsamps,
                                             csound->GetSr(csound));
      cudaMemset(p->out, 0, sizeof(float)*vsamps);
      cudaMemcpy(p->frame,p->fp,sizeof(float)*p->bins*2,cudaMemcpyHostToDevice);
      sample<<<p->blocks,p->threads>>>(p->out,p->frame,
                                               *p->kfreq,
                                                p->ndx,
                                                p->previous,
                                                vsamps,
                                                csound->GetSr(csound));
      
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
  cudaFree(p->previous);
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
