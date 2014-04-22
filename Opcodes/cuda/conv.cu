// -*- c++ -*-
// conv.cu
// experimental cuda opcodes
//
// V Lazzarini, 2014

#include <csdl.h>

__global__ void convol(MYFLT *out, MYFLT *del, MYFLT *coefs, int irsize, int rp, int vsize) {
  int t = (threadIdx.x + blockIdx.x*blockDim.x);
  if(t >= irsize*vsize) return;
  int n =  t%vsize;  /* sample index */
  int h =  t/vsize;  /* coeff index */
  int end = irsize+vsize;
  rp += n + h; /* read point, oldest -> newest */
  out[t] = del[rp < end ? rp : rp%end]*coefs[irsize-1-h];  /* single tap */
  if(t >= vsize) return;
  syncthreads();
  MYFLT a = 0.0;
  for(int i=1, j=vsize; i < irsize; i++, j+=vsize)
    a +=  out[n + j]; /* mix all taps */   
  out[n] += a;    
}

// __device__ double atomicAdd(double* address, double val)
// {
//     unsigned long long int* address_as_ull =
//                               (unsigned long long int*)address;
//     unsigned long long int old = *address_as_ull, assumed;
//     do {
//         assumed = old;
//         old = atomicCAS(address_as_ull, assumed,
//                         __double_as_longlong(val +
//                                __longlong_as_double(assumed)));
//     } while (assumed != old);
//     return __longlong_as_double(old);
// }

// __global__ void convol2(MYFLT *out, MYFLT *del, MYFLT *coefs, int irsize, int rp, int vsize) {
//   int t = (threadIdx.x + blockIdx.x*blockDim.x);
//   if(t >= irsize*vsize) return;
//   int n =  t%vsize;  /* sample index */
//   int h =  t/vsize;  /* coeff index */
//   int end = irsize+vsize;
//   rp += n + h; /* read point, oldest -> newest */
//   MYFLT s = del[rp < end ? rp : rp%end]*coefs[irsize-1-h];  /* single tap */
//   t == n ? out[n] = s : atomicAdd(&out[n], s);
// }

typedef struct _CONV {
  OPDS h;
  MYFLT *aout, *asig, *ifn;
  MYFLT *coeffs, *out, *del;
  int wp, irsize;
  int blocks, threads;
} CONV;


static int destroy_conv(CSOUND *csound, void *pp){
  CONV *p = (CONV *) pp;
  cudaFree(p->coeffs);
  cudaFree(p->del);
  cudaFree(p->out);
  return OK;
}

static int conv_init(CSOUND *csound, CONV *p){

  FUNC *ftab = csound->FTnp2Find(csound, p->ifn);
  int irsize = ftab->flen;
  int nsmps = CS_KSMPS;
  int threads = irsize*nsmps;
 
  cudaMalloc(&p->coeffs, sizeof(MYFLT)*irsize);   
  cudaMemcpy(p->coeffs, ftab->ftable, sizeof(MYFLT)*irsize, 
            cudaMemcpyHostToDevice); 
   
  cudaMalloc(&p->del, sizeof(MYFLT)*(irsize+nsmps)); 
  cudaMalloc(&p->out, sizeof(MYFLT)*threads); 
  cudaMemset(p->del,0,sizeof(MYFLT)*(irsize+nsmps));
  cudaMemset(p->out, 0, sizeof(MYFLT)*threads);
  
  p->wp = 0;
  p->irsize = irsize;

  cudaDeviceProp deviceProp;
  cudaGetDeviceProperties(&deviceProp, 0);
  int blockspt = deviceProp.maxThreadsPerBlock;
  csound->Message(csound, "CUDAconv: using device %s (capability %d.%d)\n", 
        deviceProp.name,deviceProp.major, deviceProp.minor);

  p->blocks = threads > blockspt ? ceil(threads/blockspt) : 1;
  p->threads = threads > blockspt ? blockspt : threads;

  csound->RegisterDeinitCallback(csound, p, destroy_conv);
  OPARMS parms;
  csound->GetOParms(csound, &parms);
  if(parms.odebug)
   csound->Message(csound, "blocks %d, threads %d - %d\n", p->blocks, p->threads, threads);

  return OK;

}
/* the delay size is irsize + vsize so that
   we can shift in a whole block of samples */
int conv_perf(CSOUND *csound, CONV *p){

   int nsmps = CS_KSMPS;
   MYFLT *sig = p->asig, *aout = p->aout;
   MYFLT *del = p->del, *out = p->out, *coefs = p->coeffs;
   int irsize = p->irsize;
   int wp = p->wp;

  if(wp > irsize) {
     int front = wp - irsize; 
     cudaMemcpy(&del[wp], sig, sizeof(MYFLT)*(nsmps-front), cudaMemcpyHostToDevice);
     cudaMemcpy(del, &sig[nsmps-front], sizeof(MYFLT)*front, cudaMemcpyHostToDevice);
  } 
  else cudaMemcpy(&del[wp], sig, sizeof(MYFLT)*nsmps, cudaMemcpyHostToDevice); 
  
  wp = (wp+nsmps)%(irsize+nsmps); /* wp is now the oldest sample in the delay */
  convol<<<p->blocks,p->threads>>>(out, del, coefs, irsize, wp, nsmps);

  cudaMemcpy(aout, out, sizeof(MYFLT)*nsmps, cudaMemcpyDeviceToHost); 
  p->wp = wp;
  return OK;
}

static OENTRY localops[] = {
  {"cudaconv", sizeof(CONV),0, 5, "a", "ai", (SUBR) conv_init, NULL,
   (SUBR) conv_perf},
};

extern "C" {
  LINKAGE
}
