// -*- c++ -*-
/* pvsopsf.cu
   experimental cuda opcodes
   version using single-precision floats
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
#include <cufft.h>

#include <pstream.h>

/* kernel to convert from pvs to rectangular frame */
__global__ void frompvs(float* inframe, float* lastph,
                        float scal, float fac) {

  int k = threadIdx.x + blockIdx.x*blockDim.x + 1;
  int i = k << 1;
  float mag = inframe[i];
  float delta = (inframe[i+1] - k*scal)*fac;
  float phi = fmod((double)lastph[k-1] + delta, TWOPI);
  lastph[k-1] = phi;
  inframe[i] =  (float) (mag*cos(phi));
  inframe[i+1] = (float) (mag*sin(phi));

}

__global__ void winrotate(float* inframe2, float* inframe, float *win,
                          int N, int offset){
  int k = (threadIdx.x + blockIdx.x*blockDim.x);
  inframe2[k] = win[k]*inframe[(k+offset)%N];
}

typedef struct _pvsyn{
  OPDS h;
  MYFLT *asig;
  PVSDAT *fsig;
  float *inframe; /* N */
  float *inframe2;
  float *lastph;  /* N/2 */
  float *win;    /* N */
  int framecount;
  int curframe;
  AUXCH  frames;
  AUXCH  count;
  cufftHandle plan;
  float scal, fac;
  int bblocks, nblocks;
  int bthreads, nthreads;
} PVSYN;

static int destroy_pvsyn(CSOUND *csound, void *pp);

static int pvsynset(CSOUND *csound, PVSYN *p){

  int N = p->fsig->N;

  if((N != 0) && !(N & (N - 1))) {
    int hsize = p->fsig->overlap;
    int size, numframes, i, blockspt;
    MYFLT sum = 0.0, bins = N/2;
    float *win;
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, 0);
    blockspt = deviceProp.maxThreadsPerBlock;
    csound->Message(csound, "CUDAsynth: using device %s (capability %d.%d)\n", 
        deviceProp.name,deviceProp.major, deviceProp.minor);

    if(p->fsig->wintype != 1)
      return csound->InitError(csound,
                               "window type not implemented yet\n");
    numframes = N/hsize;
    size = N*sizeof(float)*numframes;
    if(p->frames.auxp == NULL ||
       p->frames.size < size)
      csound->AuxAlloc(csound, size, &p->frames);
    memset(p->frames.auxp, 0, size);

    size = sizeof(int)*numframes;
    if(p->count.auxp == NULL ||
       p->count.size < size)
      csound->AuxAlloc(csound, size, &p->count);
    *((int *)(p->count.auxp)) =  0;
    for(i=1; i < numframes; i++)
      ((int *)(p->count.auxp))[i] =
              (i + (1.f - (float)i/numframes))*N;

    size = (N+2)*sizeof(float);
    cudaMalloc(&p->inframe, size);
    cudaMemset(p->inframe, 0, size);
     size = (N+2)*sizeof(float);
    cudaMalloc(&p->inframe2, size);
    size = (N+2)*sizeof(float);
    cudaMalloc(&p->lastph, size);
    cudaMemset(p->lastph, 0, size);
    size = N*sizeof(float);
    cudaMalloc(&p->win, size);

    win = (float *) malloc(sizeof(float)*(N+1));
    for(i=0; i <= N; i++)
      win[i] = (float) (0.5 - 0.5*cos(i*TWOPI/N));

    for(i = 0; i < N; i++) sum += win[i];
    sum = FL(2.0) / sum;
    for(i = 0; i < N; i++) win[i] *= sum;
    sum = FL(0.0);
    for(i = 0; i <= N; i+=hsize)
               sum += win[i] * win[i];
    sum = (1.0/N)/(sum);
    for(i = 0; i < N; i++) win[i] *= 3*sum/sqrt(numframes);
    cudaMemcpy(p->win,win,N*sizeof(float),
               cudaMemcpyHostToDevice);
    free(win);

    p->framecount = 0;
    p->curframe = 0;

    p->fac = TWOPI*hsize/csound->GetSr(csound);
    p->scal =csound->GetSr(csound)/N;
    cufftPlan1d(&p->plan, N, CUFFT_C2R, 1);
    cufftSetCompatibilityMode(p->plan, CUFFT_COMPATIBILITY_NATIVE);
    csound->RegisterDeinitCallback(csound, p, destroy_pvsyn);

    p->bblocks = bins > blockspt? bins/blockspt : 1;
    p->nblocks = N > blockspt ? N/blockspt : 1;
    p->bthreads = bins/p->bblocks;
    p->nthreads = N/p->nblocks;
    if(csound->GetDebug(csound))
      csound->Message(csound, "%d (%d each), %d (%d each)\n",
                    p->nblocks, p->nthreads, p->bblocks, p->bthreads);
    return OK;
  }
  return csound->InitError(csound, "fftsize not power-of-two \n");

}

static int pvsynperf(CSOUND *csound, PVSYN *p){

  int N = p->fsig->N, i;
  int hsize = p->fsig->overlap;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT *asig = p->asig;
  float *frames = (float *) p->frames.auxp;
  int framecount = p->framecount;
  int numframes = N/hsize;
  int *count = (int *) p->count.auxp;

  if (UNLIKELY(offset)) memset(asig, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&asig[nsmps], '\0', early*sizeof(MYFLT));
  }

  for(n=offset; n < nsmps; n++) {

    if(framecount == 0) {
      int curframe = p->curframe;
      /* start offset for current frame */
      int start = N*curframe;
      float *cur = &(frames[start]);
      float *win = (float *) p->win;
      float *inframe = p->inframe;
            float *inframe2 = p->inframe2;
      float *fsig = (float *) p->fsig->frame.auxp;
      /* copy fsig data to device */
      fsig[N+1] = fsig[1] = 0.f;
      cudaMemcpy(inframe,fsig,(N+2)*sizeof(float),
                 cudaMemcpyHostToDevice);
      /* perf pvs to rect conversion */
      frompvs<<<p->bblocks,p->bthreads-1>>>(inframe,p->lastph,p->scal,p->fac);
      /* execute inverse real FFT */
      if(cufftExecC2R(p->plan,(cufftComplex*)inframe,inframe)
         != CUFFT_SUCCESS) csound->Message(csound, "cuda fft error\n");
      if (cudaDeviceSynchronize() != cudaSuccess)
        csound->Message(csound,"Cuda error: Failed to synchronize\n");
      /* window and rotate data on device */
      winrotate<<<p->nblocks,p->nthreads>>>(inframe2,inframe,win,N,hsize*curframe);
      /* copy data to current out frame */
      cudaMemcpy(cur,inframe2,N*sizeof(float),cudaMemcpyDeviceToHost);
      /* reset counter for this frame to the start */
      count[curframe] = start;
      /* move current to next frame circularly */
      p->curframe = ++(curframe) == numframes ? 0 : curframe;
      framecount = hsize;
    }
    asig[n] = FL(0.0);
    for(i=0; i < numframes; i++){
      /* overlap-add */
      asig[n] += frames[count[i]];
      count[i]++;
    }
    framecount--;
  }
  p->framecount = framecount;
  return OK;
}

static int destroy_pvsyn(CSOUND *csound, void *pp){
  PVSYN *p = (PVSYN *) pp;
  cufftDestroy(p->plan);
  cudaFree(p->inframe);
  cudaFree(p->lastph);
  cudaFree(p->win);
  return OK;
}

__device__ float modTwoPi(float x)
{
  x = fmod((double)x,TWOPI);
  return x <= -PI ? x + TWOPI :
    (x > PI ? x - TWOPI : x);
}

/* kernel to convert from rectangular to pvs frame */
__global__ void topvs(float* aframe, float* oldph,
                      float scal, float fac) {
  int k = threadIdx.x + blockIdx.x*blockDim.x + 1;
  int i = k << 1;

  float re = aframe[i], im = aframe[i+1];
  float mag = sqrtf(re*re + im*im);
  float phi = atan2f(im,re);
  float delta = phi - oldph[k-1];
  oldph[k-1] = phi;
  aframe[i] =  mag;
  aframe[i+1] = (float) ((modTwoPi(delta) + k*scal)*fac);
}

__global__ void rotatewin(float* aframe2, float *aframe, float *win,
                          int N, int offset){
  int k = threadIdx.x + blockIdx.x*blockDim.x;
  aframe2[(k+offset)%N] = win[k]*aframe[k];
}

typedef struct _pvan {
  OPDS  h;
  PVSDAT *fsig;
  MYFLT *asig,*fftsize,*hsize,*winsize,*wintype;

  float *aframe; /* N */
  float *aframe2;
  float *oldph;  /* N/2 */
  float *win;    /* N */

  int framecount;
  int curframe;
  AUXCH  frames;
  AUXCH  count;
  cufftHandle plan;
  float scal, fac;
  int bblocks, nblocks;
  int bthreads, nthreads;
} PVAN;

static int destroy_pvanal(CSOUND *csound, void *pp);

static int pvanalset(CSOUND *csound, PVAN *p){

  int N = *p->fftsize;
  if((N != 0) && !(N & (N - 1))) {
    int size, numframes, i, bins = N/2;
    int hsize = *p->hsize, blockspt;
    float *win;
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, 0);
    blockspt = deviceProp.maxThreadsPerBlock;
    csound->Message(csound, "CUDAnal: using device %s (capability %d.%d)\n", 
        deviceProp.name,deviceProp.major, deviceProp.minor);

    p->fsig->N = N;
    p->fsig->overlap = hsize;
    /* ignore winsize & wintype */
    p->fsig->winsize = N;
    p->fsig->wintype = 1;
    p->fsig->framecount = 0;

    numframes = N/hsize;
    size = N*sizeof(float)*numframes;
    if(p->frames.auxp == NULL ||
       p->frames.size < size)
      csound->AuxAlloc(csound, size, &p->frames);
    memset(p->frames.auxp, 0, size);

    size = N*sizeof(float);
    if(p->fsig->frame.auxp == NULL ||
       p->fsig->frame.size < size)
      csound->AuxAlloc(csound, size, &p->fsig->frame);
    memset(p->fsig->frame.auxp, 0, size);

    size = sizeof(int)*numframes;
    if(p->count.auxp == NULL ||
       p->count.size < size)
      csound->AuxAlloc(csound, size, &p->count);
    *((int *)(p->count.auxp)) =  0;
    for(i=1; i < numframes; i++)
      ((int *)(p->count.auxp))[i] =
              (i + (float)i/numframes)*N;

    size = (N+2)*sizeof(float);
    cudaMalloc(&p->aframe, size);
    size = (N+2)*sizeof(float);
    cudaMalloc(&p->aframe2, size);
    size = (N/2-1)*sizeof(float);
    cudaMalloc(&p->oldph, size);
    cudaMemset(p->oldph, 0, size);
    size = N*sizeof(float);
    cudaMalloc(&p->win, size);

    win = (float *) malloc(sizeof(float)*N);
    for(i=0; i < N; i++)
      win[i] = (float) (0.5 - 0.5*cos(i*TWOPI/N));
    float sum = 0.0;
   for(i = 0; i < N; i++) sum += win[i];
    sum = FL(2.0) / sum;
   for(i = 0; i < N; i++) win[i] *= sum;

    cudaMemcpy(p->win,win,N*sizeof(float),
               cudaMemcpyHostToDevice);
    free(win);


    
    p->framecount = 1;
    p->curframe = numframes-1;
    p->fac = csound->GetSr(csound)/(TWOPI*hsize);
    p->scal = (TWOPI*hsize)/N;
    cufftPlan1d(&p->plan, N, CUFFT_R2C, 1);
    cufftSetCompatibilityMode(p->plan, CUFFT_COMPATIBILITY_NATIVE);
    csound->RegisterDeinitCallback(csound, p, destroy_pvanal);

    p->bblocks = bins > blockspt? bins/blockspt : 1;
    p->nblocks = N > blockspt ? N/blockspt : 1;
    p->bthreads = bins/p->bblocks;
    p->nthreads = N/p->nblocks;
  if(csound->GetDebug(csound))
    csound->Message(csound, "%d (%d each), %d (%d each)\n",
                    p->nblocks, p->nthreads, p->bblocks, p->bthreads);
    return OK;
  }
  return csound->InitError(csound, "fftsize not power-of-two \n");
}

static int pvanalperf(CSOUND *csound, PVAN *p){

  int N = p->fsig->N, i;
  int hsize = p->fsig->overlap;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT *asig = p->asig;
  float *frames = (float *) p->frames.auxp;
  int framecount = p->framecount;
  int numframes = N/hsize;
  int *count = (int *) p->count.auxp;

  if (UNLIKELY(offset)) memset(asig, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&asig[nsmps], '\0', early*sizeof(MYFLT));
  }

  for(n=offset; n < nsmps; n++) {
   for(i=0; i < numframes; i++){
      frames[count[i]] = asig[n];
      count[i]++;
    }
    framecount++;

    if(framecount == hsize) {
      int curframe = p->curframe;
      /* start offset for current frame */
      int start = N*curframe;
      float *cur = &(frames[start]);
      float *win = (float *) p->win;
      float *aframe = p->aframe;
      float *aframe2 = p->aframe2;
      float *fsig = (float *) p->fsig->frame.auxp;
      /* copy fsig data to device */
      cudaMemcpy(aframe,cur,N*sizeof(float),
             cudaMemcpyHostToDevice);
      /* window and rotate data on device */
      rotatewin<<<p->nblocks,p->nthreads>>>(aframe2,aframe,win,N,
                                            hsize*(numframes-curframe));
       /* execute inverse real FFT */
      if(cufftExecR2C(p->plan,aframe2,(cufftComplex*)aframe2)
      != CUFFT_SUCCESS) csound->Message(csound, "cuda fft error\n");
       if (cudaDeviceSynchronize() != cudaSuccess)
         csound->Message(csound,"Cuda error: Failed to synchronize\n");
       /* perf rect to pvs conversion */
       topvs<<<p->bblocks,p->bthreads-1>>>(aframe2,p->oldph,p->scal,p->fac);
       /* copy data to current out frame */
      cudaMemcpy(fsig,aframe2,(N+2)*sizeof(float),cudaMemcpyDeviceToHost);
      /* reset counter for this frame to the start */
      fsig[N+1] = fsig[1] = 0.f;
      count[curframe] = start;
      /* move current to next frame circularly */
      p->curframe = --(curframe) < 0 ? numframes-1 : curframe;
      framecount = 0;
      p->fsig->framecount++;
    }
  }
  p->framecount = framecount;
  return OK;
}

static int destroy_pvanal(CSOUND *csound, void *pp){
  PVAN *p = (PVAN *) pp;
  cufftDestroy(p->plan);
  cudaFree(p->aframe);
  cudaFree(p->oldph);
  cudaFree(p->win);
  return OK;
}

static OENTRY localops[] = {
  {"cudasynth", sizeof(PVSYN),0, 5, "a", "f", (SUBR) pvsynset, NULL,
   (SUBR) pvsynperf},
   {"cudanal", sizeof(PVAN),0, 5, "f", "aiiii", (SUBR) pvanalset, NULL,
   (SUBR) pvanalperf}
};

extern "C" {
  LINKAGE
}
