// -*- c++ -*-
// pvsops.cu
// experimental cuda opcodes
//
// V Lazzarini, 2013

#include <csdl.h>
#include <cufft.h>

#define THREADS_PER_BLOCK 1024

#include <pstream.h>

/* kernel to convert from pvs to rectangular frame */
__global__ void frompvs(float* inframe, double* lastph,
                        double scal, double fac) {

  int k = threadIdx.x + blockIdx.x*blockDim.x + 1;
  int i = k << 1;
  float mag = inframe[i];
  double delta = (inframe[i+1] - k*scal)*fac;
  double phi = fmod(lastph[k-1] + delta, TWOPI);
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
  double *lastph;  /* N/2 */
  float *win;    /* N */
  int framecount;
  int curframe;
  AUXCH  frames;
  AUXCH  count;
  cufftHandle plan;
  double scal, fac;
  int blockspt;
} PVSYN;

static int destroy_pvsyn(CSOUND *csound, void *pp);

static int pvsynset(CSOUND *csound, PVSYN *p){

  int N = p->fsig->N;

  if((N != 0) && !(N & (N - 1))) {
    int hsize = p->fsig->overlap;
    int size, numframes, i;
    MYFLT sum = 0.0;
    float *win;
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, 0);
    p->blockspt = deviceProp.maxThreadsPerBlock;

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
    size = (N+2)*sizeof(float);
    cudaMalloc(&p->inframe2, size);
    size = (N/2-1)*sizeof(double);
    cudaMalloc(&p->lastph, size);
    cudaMemset(p->lastph, 0, size);
    size = N*sizeof(float);
    cudaMalloc(&p->win, size);

    win = (float *) malloc(sizeof(float)*N);
    for(i=0; i < N; i++)
      win[i] = (float) (0.5 - 0.5*cos(i*TWOPI/N));
    for(i = 0; i < N; i++) sum += win[i];
    sum = FL(2.0) / sum;
    for(i = 0; i < N; i++) win[i] *= sum;
    sum = FL(0.0);
    for(i = 0; i <= N; i+=hsize)
              sum += win[i] * win[i];
    sum = 2.0/(sum*N);
    for(i = 0; i < N; i++) win[i] *= sum;
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
      int bins = N/2;
      int blocks;
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
      //csound->Message(csound, "syn %f, %f\n",fsig[41],  fsig[40]);
      cudaMemcpy(inframe,fsig,(N+2)*sizeof(float),
                 cudaMemcpyHostToDevice);
      /* perf pvs to rect conversion */
      blocks = bins > p->blockspt? bins/p->blockspt : 1;
      frompvs<<<blocks,
        bins/blocks-1>>>(inframe,p->lastph,
                       p->scal,p->fac);
      /* execute inverse real FFT */
      if(cufftExecC2R(p->plan,(cufftComplex*)inframe,inframe)
         != CUFFT_SUCCESS) csound->Message(csound, "cuda fft error\n");
      if (cudaDeviceSynchronize() != cudaSuccess)
        csound->Message(csound,"Cuda error: Failed to synchronize\n");
      /* window and rotate data on device */
      blocks =  N > p->blockspt ? N/p->blockspt : 1;
      winrotate<<<blocks,
        N/blocks>>>(inframe2,inframe,win,N,hsize*curframe);
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

__device__ double modTwoPi(double x)
{
  x = fmod(x,TWOPI);
  return x <= -PI ? x + TWOPI :
    (x > PI ? x - TWOPI : x);
}

/* kernel to convert from rectangular to pvs frame */
__global__ void topvs(float* aframe, double* oldph,
                      double scal, double fac) {
  int k = threadIdx.x + blockIdx.x*blockDim.x + 1;
  int i = k << 1;

  float re = aframe[i], im = aframe[i+1];
  float mag = sqrtf(re*re + im*im);
  double phi = atan2(im,re);
  double delta = phi - oldph[k-1];
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
  double *oldph;  /* N/2 */
  float *win;    /* N */

  int framecount;
  int curframe;
  AUXCH  frames;
  AUXCH  count;
  cufftHandle plan;
  double scal, fac;
  int blockspt;
} PVAN;

static int destroy_pvanal(CSOUND *csound, void *pp);

static int pvanalset(CSOUND *csound, PVAN *p){

  int N = *p->fftsize;
  if((N != 0) && !(N & (N - 1))) {
    int size, numframes, i;
    int hsize = *p->hsize;
    float *win;
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, 0);
    p->blockspt = deviceProp.maxThreadsPerBlock;

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
    size = (N/2-1)*sizeof(double);
    cudaMalloc(&p->oldph, size);
    cudaMemset(p->oldph, 0, size);
    size = N*sizeof(float);
    cudaMalloc(&p->win, size);

    win = (float *) malloc(sizeof(float)*N);
    for(i=0; i < N; i++)
      win[i] = (float) (0.5 - 0.5*cos(i*TWOPI/N))*(4./N);
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
    csound->Message(csound, "%d threads, %d blocks\n", N, N/THREADS_PER_BLOCK);
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
      int bins = N/2;
      int blocks;
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
      blocks =  N > p->blockspt ? N/p->blockspt : 1;
      rotatewin<<<blocks,
       N/blocks>>>(aframe2,aframe,win,N,hsize*(numframes-curframe));
       /* execute inverse real FFT */
      if(cufftExecR2C(p->plan,aframe2,(cufftComplex*)aframe2)
      != CUFFT_SUCCESS) csound->Message(csound, "cuda fft error\n");
      if (cudaDeviceSynchronize() != cudaSuccess)
      // csound->Message(csound,"Cuda error: Failed to synchronize\n");
       /* perf rect to pvs conversion */
      blocks = bins > p->blockspt ? bins/p->blockspt : 1;
      topvs<<<blocks,
      bins/blocks-1>>>(aframe2,p->oldph,p->scal,p->fac);
       /* copy data to current out frame */
      cudaMemcpy(fsig,aframe2,(N+2)*sizeof(float),cudaMemcpyDeviceToHost);
      /* reset counter for this frame to the start */
      fsig[N+1] = fsig[1] = 0.f;
      //printf("%f \n", fsig[43]);
      //printf("%f \n", fsig[42]);
      //csound->Message(csound, "%f, %f\n",fsig[41],  fsig[40]);
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
