// -*- c++ -*-
/*
  slindingm.cu:

  Copyright (C) 2014 Russell Bradford, Victor Lazzarini, John ffitch

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
// slindingm.cu
// experimental cuda opcodes
// using the sliding DFT
// adapted from Russell Bradford & John ffitch
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <inttypes.h>
#include <sys/time.h>
#include <csdl.h>

typedef float real;
#define real(x) x
#define TWO_PI (2.0*M_PI)
#define NBATCH (256)

#define Re(z) ((z).x)
#define Im(z) ((z).y)
#define R(p) ((p).x)
#define Theta(p) ((p).y)

#define RectA (real(1.0))
#define RectB (real(0.0))
#define HannA (real(0.5))
#define HannB (real(-0.5))
#define A HannA
#define B2 (HannB/real(2.0))
#define WINDOW
//#define RB

typedef double2 complex;
typedef double2 phasor;

// Put a few things into constant memory to free up a couple of registers
//__constant__ int N, N2;
__constant__ real binbandwidth, nyquist;
// array alignment
__constant__ int offset;
// process in batches of this size 
__constant__ int nbatch;


#define CUDA(name, args ...) { name(args); checkcuda(csound, #name, __LINE__); }
#define CHK(name) checkcuda(csound, #name, __LINE__)
#define NEXT(p) p += offset

void checkcuda(CSOUND *csound, char* name, int line)
{
  cudaError_t err = cudaGetLastError();

  if (err) {
    csound->Message(csound, "%s: %s\nLine: %d\n", name, cudaGetErrorString(err), line);
    exit(1);
  }
}

__device__ double atomicAdd(double* address, double val) { 
  unsigned long long int* address_as_ull = (unsigned long long int*)address; 
  unsigned long long int old = *address_as_ull, assumed; 
  do { assumed = old; 
    old = atomicCAS(address_as_ull, 
		    assumed, __double_as_longlong(val + __longlong_as_double(assumed))); 
  } while (assumed != old); 
  return __longlong_as_double(old); 
}

__device__ complex conjugate(complex z)
{
  return (complex){ Re(z), -Im(z) };
}

// round up to multiple of 256
__host__ __device__ int roundup(int n)
{
  return 1 + ((n - 1) | 0xff);
}

// Fin/Fout
// thread k does bin k; parallel across bins in a batch item..
__global__ void slide(real deltas[/*nbatch*/], complex Fin[/*N*/],
		      complex Fout[/*N*nbatch*/], 
                      real *sine, real *cosine, int N)
{
  int k = blockIdx.x * blockDim.x + threadIdx.x;
  int b, N2 = N/2;
  complex f;
  real Fr, Fi, c, s;
  
  if (k > N2) return;
  f = Fin[k];

  // in constant memory
  s = sine[k];
  c = cosine[k];
  
  for (b = 0; b < nbatch; b++) {
    Fr = Re(f) + deltas[b];
    Fi = Im(f);
    f = (complex){ Fr*c - Fi*s, Fi*c + Fr*s };
    Fout[k] = f;
    NEXT(Fout);
  }
  Fin[k] = f;
}

__device__ real mod2Pi(real x)
{
  x = fmod(x, (real)TWO_PI);
  if (x > PI) return x - TWO_PI;
  if (x <= -PI) return x + TWO_PI;
  return x;
}

#ifdef RB
// below is one thread per bin, as per other kernels
// data is spread across blocks, but there is no sync across blocks
// thus cannot do in-place window
// parallel across bins in a batch item
// could do bins*batches
__global__ void window(complex Foutw[/*N*nbatch*/],
		       complex Fout[/*N*nbatch*/], int N)
{
  int k = blockIdx.x * blockDim.x + threadIdx.x;
  int b, N2 = N/2;
  complex F, Fm1, Fp1, f;
  real Fr, Fi;

  // in the last block
  if (k > N2) return;

  for (b = 0; b < nbatch; b++) {
    F = Foutw[k];
    // conjugate reflection at edges
    Fp1 = k < N2 ? Foutw[k + 1] : conjugate(Foutw[N2-1]);
    Fm1 = k > 0 ? Foutw[k - 1] : conjugate(Foutw[1]);
    Fr = A*Re(F) + B2*(Re(Fm1) + Re(Fp1));
    Fi = A*Im(F) + B2*(Im(Fm1) + Im(Fp1));

    Re(f) = Fr;
    Im(f) = Fi;
    Fout[k] = f;

    NEXT(Fout);
    NEXT(Foutw);
  }
}

// parallel across batches
// <<<nbatch/256,256>>>
// what is memory access pattern?
// small array sum, so not worth a tree reduction?
// middle sample only, so +- on real parts
__global__ void reconstruct(complex f[/*nbatch*N*/], real s[/*nbatch*/], int N)
{
  int k = blockIdx.x * blockDim.x + threadIdx.x;
  complex *F = f + offset*k;
  real sum = real(0.0);
  int bin, N2 = N/2;

  if (k >= nbatch) return;
  for (bin = 0; bin < N; bin += 2) {
    real v0 = bin <= N2 ? Re(F[bin]) : Re(F[N - bin]);
    real v1 = bin <  N2 ? Re(F[bin + 1]) : Re(F[N - bin - 1]);
    sum += v0 - v1;
  }
  s[k] = sum/N;
}

#else
// this parallelises across N*nbatch
__global__ void window(complex Foutw[/*N*nbatch*/],
		       complex Fout[/*N*nbatch*/], int N)
{
  int n = blockIdx.x * blockDim.x + threadIdx.x;
  int N2 = N/2;
  int s = n/N;
  int k = n%N;
  complex F, Fm1, Fp1, f;
  real Fr, Fi;
  Foutw += s*offset;
  Fout += s*offset;

  // in the last block
  if (n > N*nbatch) return;

  F = Foutw[k];
  Fp1 = k < N2 ? Foutw[k + 1] : conjugate(Foutw[N2-1]);
  Fm1 = k > 0 ? Foutw[k - 1] : conjugate(Foutw[1]);

  Fr = A*Re(F) + B2*(Re(Fm1) + Re(Fp1));
  Fi = A*Im(F) + B2*(Im(Fm1) + Im(Fp1));

  Re(f) = Fr;
  Im(f) = Fi;
  Fout[k] = f;
}

// this parallelises across N2*nbatch
__global__ void reconstruct(complex f[/*nbatch*N*/], real s[/*nbatch*/], int N)
{
  int n = blockIdx.x * blockDim.x + threadIdx.x, N2 = N/2;
  int k = n/N2;
  int bin = (n%N2)*2;
  complex *F = &f[offset*k];

  if (n >= nbatch*N2) return;

  real v0 = bin <= N2 ? Re(F[bin]) : Re(F[N - bin]);
  real v1 = bin <  N2 ? Re(F[bin + 1]) : Re(F[N - bin - 1]);
  atomicAdd(&s[k],v0-v1); 
}
#endif


__device__ phasor convert(complex f, real &oldiphase, int bin, int N)
{
  real Fr, Fi;
  real phase, deltaphase;
  phasor pha;

  Fr = Re(f);
  Fi = Im(f);

  // magnitude is easy
  R(pha) = hypot(Fi, Fr);

  phase = atan2(Fi, Fr);
  deltaphase = phase - oldiphase;
  oldiphase = phase;

  // subtract expected phase diff
  deltaphase = mod2Pi(deltaphase - bin*TWO_PI/N);

  // find actual freq
  // scale by bin width to give freq
  Theta(pha) = binbandwidth * (bin + deltaphase*N/TWO_PI);

  return pha;
}

__device__ phasor harmonic_shift(phasor pha, real fm)
{
  real sfreq;

  // tweak the freq using fm
  // same across all channels
  sfreq = Theta(pha)*fm;

  if (sfreq >= nyquist || sfreq <= -nyquist) R(pha) = real(0.0);
  Theta(pha) = sfreq;

  return pha;
}

__device__ real unconvert(phasor pha, real &oldophase, int N)
{
  real bin, phase;
  
  bin = Theta(pha)/binbandwidth;
  phase = bin*TWO_PI/N;
  phase = mod2Pi(oldophase + phase);
  oldophase = phase;

  return R(pha)*cos(phase);
}

// parallelism across bins*channels
__global__ void fmsyn(real inphase[/*N*/], complex F[/*nbatch*N*/],
		      real fm[/*nbatch*/], real outphase[/*N*/], int N)
{
  int k = blockIdx.x * blockDim.x + threadIdx.x;
  int b, N2 = N/2;
  real oldiphase, oldophase, val;
  phasor pha;

  if (k > N2) return;

  oldiphase = inphase[k];
  oldophase = outphase[k];

  for (b = 0; b < nbatch; b++) {
    pha = convert(F[k], oldiphase, k, N); // updates oldiphase
    pha = harmonic_shift(pha, fm[b]);

    // ignore imag part
    val = unconvert(pha, oldophase, N); // updates oldophase
    Re(F[k]) = val;
    NEXT(F);
  }

  inphase[k] = oldiphase;
  outphase[k] = oldophase;
}


// blocks a multiple of 32 threads
// more blocks than multiprocessors
// perhaps 128 threads per block, as long as you have more blocks than
// multiprocessors
void threadblock(int device, int n, int *nblocks, int *nthreads)
{
  int nt, nb, nproc;
  cudaDeviceProp deviceProp;

  //printf("%d threads needed\n", n);

  cudaGetDeviceProperties(&deviceProp, device);
  nt = deviceProp.warpSize; // usually 32 threads
  nproc = deviceProp.multiProcessorCount;

  nb = 1 + (n - 1)/nt;

  
  //more blocks than procs
  if (nb >= 2*nproc) {
    nt *= 2; // 64 threads
    nb = 1 + (n - 1)/nt;
  }
   
  // // still more blocks than procs
  if (nb >= 2*nproc) {
    nt *= 2; // 128 threads
    nb = 1 + (n - 1)/nt;
  }

  if (nblocks) *nblocks = nb;
  if (nthreads) *nthreads = nt;  
}

typedef struct _SPV {
  OPDS h;
  MYFLT *out, *in, *shift, *iN;
  int nbins, ptr, nbtch, nblocks, nblocks1, nthreads, nthreads1, rblocks, rthreads;
  real *deltas, *ddeltas, *dinphase, *doutphase, *dfm;
  real *sine, *cosine;
  complex *dFin, *dFout, *dFoutw;
  real *samples, srate;
  real *framesin, *framesout, *fm;
  AUXCH aframesfm, aframesout, aframesin;
  uint32_t count;
} SPV;


void init_tables(CSOUND *csound, SPV *p, int nbins)
{
  int k;
  real s[nbins], c[nbins];

  for (k = 0; k < nbins; k++) {
    sincos(TWO_PI*k/nbins, &s[k], &c[k]);
  }

  CUDA(cudaMemcpy, p->sine, s, nbins*sizeof(real), cudaMemcpyHostToDevice);
  CUDA(cudaMemcpy, p->cosine, c, nbins*sizeof(real), cudaMemcpyHostToDevice);
}

void cuinit(CSOUND *csound, 
            SPV *p, uint32_t nframes, 
            int nbins, int device)
{
  int csize, rsize, off, nbins2;
  real binbw, nyq;
  cudaDeviceProp deviceProp;

  if(!p->aframesin.auxp || p->aframesin.size < sizeof(real)*NBATCH) 
    csound->AuxAlloc(csound, sizeof(real)*NBATCH, &p->aframesin);
  if(!p->aframesout.auxp || p->aframesout.size < sizeof(real)*NBATCH) 
    csound->AuxAlloc(csound, sizeof(real)*NBATCH, &p->aframesout);
  if(!p->aframesfm.auxp || p->aframesfm.size < sizeof(real)*NBATCH) 
    csound->AuxAlloc(csound, sizeof(real)*NBATCH, &p->aframesfm);

  p->framesin = (real *) p->aframesin.auxp;
  p->framesout = (real *) p->aframesout.auxp;
  p->fm = (real *) p->aframesfm.auxp;

  p->nbins = nbins;
  nbins2 = nbins/2;
  p->nbtch = nframes;
  p->srate = (real) csound->GetSr(csound);
  binbw = p->srate/p->nbins;
  nyq = p->srate/real(2.0);

  CUDA(cudaSetDevice, device);

  cudaGetDeviceProperties(&deviceProp, device);
  
  // allocate contiguous arrays to aid cudaMemcpy; round up to align
  // arrays for coalescence
  csize = roundup(p->nbins*sizeof(complex));
  rsize = roundup(p->nbins*sizeof(real));
  off = csize/sizeof(complex);

  if (off != rsize/sizeof(real)) {
    csound->InitError(csound, "Something weird happening with offsets at line %d\n",
		      __LINE__);
  }

  if (!csound->QueryGlobalVariable(csound, "::cusliding::init")) {
    csound->CreateGlobalVariable(csound, "::cusliding::init",1);
    csound->Message(csound, "Sliding PV: using floats on device %s (capability %d.%d)\n", deviceProp.name,
		    deviceProp.major, deviceProp.minor);
    // global constants
    CUDA(cudaMemcpyToSymbol, offset, &off, sizeof(int));
    CUDA(cudaMemcpyToSymbol, nbatch, &p->nbtch, sizeof(int));
    CUDA(cudaMemcpyToSymbol, binbandwidth, &binbw, sizeof(real));
    CUDA(cudaMemcpyToSymbol, nyquist, &nyq, sizeof(real));

  }
  // N complexes
  CUDA(cudaMalloc, &p->dFin, csize);
  CUDA(cudaMemset, p->dFin, 0, csize);

  // N*nbatch complexes
  CUDA(cudaMalloc, &p->dFout, p->nbtch*csize);
  CUDA(cudaMalloc, &p->dFoutw, p->nbtch*csize);

  // N reals
  CUDA(cudaMalloc, &p->dinphase, p->nbins*sizeof(real));
  CUDA(cudaMalloc, &p->doutphase, p->nbins*sizeof(real));
  CUDA(cudaMemset, p->dinphase, 0, p->nbins*sizeof(real));
  CUDA(cudaMemset, p->doutphase, 0, p->nbins*sizeof(real));

  CUDA(cudaMalloc, &p->sine, p->nbins*sizeof(real));
  CUDA(cudaMalloc, &p->cosine, p->nbins*sizeof(real));

  // nbatch reals
  CUDA(cudaMalloc, &p->ddeltas, p->nbtch*sizeof(real));
  CUDA(cudaMalloc, &p->dfm, p->nbtch*sizeof(real));

  // circular history of nbins samples
  p->samples = (real*) csound->Calloc(csound, p->nbins*sizeof(real));
  if (p->samples == NULL) {
    csound->InitError(csound, "calloc failed at line %d\n", __LINE__);
    return;
  }

  // pinned memory is marginally faster
  //CUDA(cudaHostAlloc, &deltas, p->nbtch*sizeof(real), cudaHostAllocDefault);
  p->deltas = (real*) csound->Malloc(csound, p->nbtch*sizeof(real));
  if (p->deltas == NULL) {
    csound->InitError(csound, "malloc failed at line %d\n", __LINE__);
    return;
  }

  init_tables(csound, p, p->nbins);

  // index of oldest sample
  p->ptr = 0;
  
#ifdef RB
  threadblock(device, p->nbtch, &p->rblocks, &p->rthreads);
#else
  threadblock(device, p->nbtch*(nbins2), &p->rblocks, &p->rthreads);
  threadblock(device, p->nbtch*nbins, &p->nblocks1, &p->nthreads1);
#endif
  threadblock(device, (nbins2+1), &p->nblocks, &p->nthreads);

  if(csound->GetDebug(csound)) {
    csound->Message(csound, "%d bins, %d actual\n", p->nbins, nbins2 + 1);
    csound->Message(csound, "csize = %d -> %d bytes, offset %d elements\n", (int) (p->nbins*sizeof(complex)),
		    csize, off);
    csound->Message(csound, "%gHz bin bandwidth, Nyquist %gHz\n", binbw, nyq);
    csound->Message(csound, "period %d samples (%g sec)\n", p->nbtch, (real)p->nbtch/p->srate);
    csound->Message(csound, "%d multiprocessors\n", deviceProp.multiProcessorCount);
    csound->Message(csound, "%d blocks with %d threads = %d\n", p->nblocks, p->nthreads,
		    p->nblocks1*p->nthreads);
    csound->Message(csound, "%d idle threads in last block\n", p->nblocks*p->nthreads - 
		    (nbins2 + 1));
    csound->Message(csound, "%d blocks per SM\n",
		    (int)ceil((double)p->nblocks/deviceProp.multiProcessorCount));
    csound->Message(csound, "reconstruct using %d blocks of %d threads = %d\n", p->rblocks, p->rthreads,
		    p->rblocks*p->rthreads);
  }
}

int cushutdown(CSOUND *csound, void *pp)
{
  SPV *p = (SPV *) pp;
  csound->Free(csound, p->samples);

  CUDA(cudaFree, p->dinphase);
  CUDA(cudaFree, p->doutphase);
  CUDA(cudaFree, p->ddeltas);
  CUDA(cudaFree, p->dFoutw);
  CUDA(cudaFree, p->dFout);
  CUDA(cudaFree, p->dFin);
  CUDA(cudaFree, p->sine);
  CUDA(cudaFree, p->cosine);

  //CUDA(cudaFreeHost, deltas);
  csound->Free(csound, p->deltas);

  if(csound->GetDebug(csound))
    csound->Message(csound, "cuda shutdown\n");

  return OK;
}

// in samples with channels muxed 0 1 2 0 1 2 ...
void cuprocess(CSOUND *csound, SPV *p, real in[/*nbatch*/], real out[/*nbatch*/],
	       real fm[/*nbatch*/])
{
  int b;

  for (b = 0; b < p->nbtch; b++) {
    p->deltas[b] = in[b] - p->samples[p->ptr];
    p->samples[p->ptr] = in[b];
    p->ptr = (p->ptr + 1) % (p->nbins);
  }

  CUDA(cudaMemcpy, p->ddeltas, p->deltas, p->nbtch*sizeof(real),
       cudaMemcpyHostToDevice);

  slide<<<p->nblocks,p->nthreads>>>(p->ddeltas, p->dFin, p->dFoutw, 
				    p->sine, p->cosine, p->nbins);
  cudaDeviceSynchronize(); CHK(slide);

  // Foutw and Fout must be separate to avoid data race
#ifdef RB
  window<<<p->nblocks,p->nthreads>>>(p->dFoutw, p->dFout, p->nbins);
#else
  window<<<p->nblocks1,p->nthreads1>>>(p->dFoutw, p->dFout, p->nbins);
#endif
  cudaDeviceSynchronize(); CHK(window);

  CUDA(cudaMemcpy, p->dfm, fm, p->nbtch*sizeof(real), cudaMemcpyHostToDevice);

  fmsyn<<<p->nblocks,p->nthreads>>>(p->dinphase, p->dFout, p->dfm, p->doutphase, p->nbins);

  // reuse ddeltas array
  reconstruct<<<p->rblocks,p->rthreads>>>(p->dFout, p->ddeltas, p->nbins);
  CHK(reconstruct);
  // is is faster to transfer to pinned memory then copy; or just
  // transfer directly to out?
  CUDA(cudaMemcpy, out, p->ddeltas, p->nbtch*sizeof(real),
       cudaMemcpyDeviceToHost);

#ifndef RB
  for(b=0; b < p->nbtch; b++) out[b] /= p->nbins;
#endif

}


int spv_init(CSOUND *csound, SPV *p) {
  cuinit(csound,p,NBATCH,*p->iN,0);
  csound->RegisterDeinitCallback(csound, p, cushutdown);
  return OK;
}

int spv_perf(CSOUND *csound, SPV *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  uint32_t count = p->count;

  if (UNLIKELY(offset)) memset(p->out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for(n=offset; n < nsmps; n++) {
    p->framesin[count] = p->in[n];
    p->out[n] = p->framesout[count];
    p->fm[count] = p->shift[n];
    count++;
    if(count == NBATCH) {
      cuprocess(csound, p,p->framesin,p->framesout,p->fm);
      count = 0;
    }
    
  }

  p->count = count;
  return OK;
}

static OENTRY localops[] = {
  {"cudasliding", sizeof(SPV),0, 5, "a", "aai", (SUBR) spv_init, NULL,
   (SUBR) spv_perf}
};

extern "C" {
  LINKAGE
}



