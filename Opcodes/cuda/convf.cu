// -*- c++ -*-
// conv.cu
// experimental cuda opcodes
//
// V Lazzarini, 2014

#include <csdl.h>

__global__ void convol(float *out, float *del, float *coefs, int irsize, int rp, int vsize) {
  int t = (threadIdx.x + blockIdx.x*blockDim.x);
  if(t >= irsize*vsize) return;
  int n =  t%vsize;  /* sample index */
  int h =  t/vsize;  /* coeff index */
  int end = irsize+vsize;
  rp += n + h; /* read point, oldest -> newest */
  out[t] = del[rp < end ? rp : rp%end]*coefs[irsize-1-h];  /* single tap */
  if(t >= vsize) return;
  syncthreads();
  float a = 0.0;
  for(int i=1, j=vsize; i < irsize; i++, j+=vsize)
    a +=  out[n + j]; /* mix all taps */   
  out[n] += a;    
}

typedef struct _CONV {
  OPDS h;
  MYFLT *aout, *asig, *ifn;
  float *coeffs, *out, *del;
  int wp, irsize;
  AUXCH buf;
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
  int nsmps = CS_KSMPS,i;
  int threads = irsize*nsmps;
  float *tmp;

  cudaMalloc(&p->coeffs, sizeof(float)*irsize);   

  tmp = (float*) malloc(sizeof(float)*irsize);
  for(i=0; i< irsize; i++)
    tmp[i] = (float) ftab->ftable[i];
  cudaMemcpy(p->coeffs,tmp, sizeof(float)*irsize, 
            cudaMemcpyHostToDevice); 
  free(tmp);
   
  cudaMalloc(&p->del, sizeof(float)*(irsize+nsmps)); 
  cudaMalloc(&p->out, sizeof(float)*threads); 
  cudaMemset(p->del,0,sizeof(float)*(irsize+nsmps));
  cudaMemset(p->out, 0, sizeof(float)*threads);
  
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
  if(p->buf.auxp == NULL)
    csound->AuxAlloc(csound, sizeof(float)*CS_KSMPS, &p->buf);

  return OK;

}
/* the delay size is irsize + vsize so that
   we can shift in a whole block of samples */
int conv_perf(CSOUND *csound, CONV *p){

   int nsmps = CS_KSMPS;
   MYFLT *sig = p->asig, *aout = p->aout;
   float *del = p->del, *out = p->out, *coefs = p->coeffs, *buf = (float *)p->buf.auxp;
   int irsize = p->irsize;
   int wp = p->wp, i;
  
  for(i=0; i < nsmps; i++) buf[i] = (float) sig[i]; 
  if(wp > irsize) {
     int front = wp - irsize; 
     cudaMemcpy(&del[wp], buf, sizeof(float)*(nsmps-front), cudaMemcpyHostToDevice);
     cudaMemcpy(del, &buf[nsmps-front], sizeof(float)*front, cudaMemcpyHostToDevice);
  } 
  else cudaMemcpy(&del[wp], buf, sizeof(float)*nsmps, cudaMemcpyHostToDevice); 
  
  wp = (wp+nsmps)%(irsize+nsmps); /* wp is now the oldest sample in the delay */
  convol<<<p->blocks,p->threads>>>(out, del, coefs, irsize, wp, nsmps);

  cudaMemcpy(buf, out, sizeof(float)*nsmps, cudaMemcpyDeviceToHost); 

  for(i=0; i < nsmps; i++) aout[i] = (float) buf[i]; 
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
