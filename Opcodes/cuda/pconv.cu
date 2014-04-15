// -*- c++ -*-
// pconv.cu
// experimental cuda opcodes
//
// V Lazzarini, 2014

#include <csdl.h>
#include <cufft.h>


/* each kernel processes one bin 
*/
__global__ void pconvol(float *out,float *in, 
			float *coef,int rp, int dftsize, 
			int nparts, int end) {
  float re,im,re2,im2;   

  /* thread count */
  int t = (threadIdx.x + blockIdx.x*blockDim.x);

  int k = t<<1;       /* coef pos      */   
  int n = k%dftsize;  /* inframe pos   */

  /* if beyond the buffer end, exit */
  if(k >= end) return;                  
  rp += k/dftsize;       /*  rp pos */

  /* select correct input buffer */
  in += (rp < nparts ? rp : rp%nparts)*dftsize;

  re = coef[k]; im = coef[k+1];
  re2 = in[n];  im2 = in[n+1];
  
  /* complex multiplication + sums
     deal with 0Hz & Nyquist  (n == 0) */
  atomicAdd(&out[n], n  ? re*re2 - im*im2 : re*re2);
  atomicAdd(&out[n+1], n ? re*im2 + re2*im : im*im2);
}  

typedef struct _pconv{
  OPDS h;
  MYFLT *aout, *asig, *ifn, *parts;
  float *out, *coef, *in;
  AUXCH  bufin, bufout;
  int wp, nparts, dftsize, cnt;
  cufftHandle plan, iplan;
  int threads, blocks;
} PCONV;


int isPowerOfTwo (unsigned int x)
{
  return ((x != 0) && !(x & (x - 1)));
}


static int destroy_pconv(CSOUND *csound, void *pp){
  PCONV *p = (PCONV *) pp;
  cufftDestroy(p->plan);
  cufftDestroy(p->iplan);
  cudaFree(p->coef);
  cudaFree(p->in);
  cudaFree(p->out);
  return OK;
}


int pconv_init(CSOUND *csound, PCONV *p){

  FUNC *ftab = csound->FTnp2Find(csound, p->ifn);
  float *tmp;
  int tlen = ftab->flen;
  int end, i, j, k, parts = *p->parts, dftsize, nparts;
  MYFLT *tab = ftab->ftable;

  if(!isPowerOfTwo(parts))
    return csound->InitError(csound, "partition size needs to be power of two\n");

  if(parts > tlen)
     return csound->InitError(csound, "partition size too big \n");

  end = tlen + parts - 1;

  nparts = end / parts;
  dftsize = parts << 1;
  end = nparts*dftsize;

  cudaMalloc(&p->coef, sizeof(float)*end);   
  cudaMalloc(&p->in, sizeof(float)*end); 
  cudaMalloc(&p->out, sizeof(float)*dftsize); 
  cudaMemset(p->in,0,sizeof(float)*end);
  cudaMemset(p->out, 0, sizeof(float)*dftsize);
  cudaMemset(p->coef, 0, sizeof(float)*end);

  p->wp = 0;

  if(!p->bufin.auxp || p->bufin.size < sizeof(float)*dftsize)
     csound->AuxAlloc(csound, sizeof(float)*dftsize, &p->bufin);
  if(!p->bufout.auxp || p->bufout.size < sizeof(float)*dftsize)
     csound->AuxAlloc(csound, sizeof(float)*dftsize, &p->bufout);

  memset(p->bufout.auxp, 0, sizeof(float)*dftsize);

  tmp = (float *) p->bufin.auxp;
  cufftPlan1d(&p->plan, dftsize, CUFFT_R2C, 1);
  cufftSetCompatibilityMode(p->plan, CUFFT_COMPATIBILITY_NATIVE);
  cufftPlan1d(&p->iplan, dftsize, CUFFT_C2R, 1);
  cufftSetCompatibilityMode(p->iplan, CUFFT_COMPATIBILITY_NATIVE);

  for(i =0, k=0; i < nparts; i++){
    for(j=0; j < dftsize; j++)
      tmp[j] = j < parts && k < tlen ? tab[k++] : 0.f;
    float *pp = p->coef + (nparts - 1 - i)*dftsize;
    cudaMemcpy(pp, tmp, sizeof(float)*dftsize, 
               cudaMemcpyHostToDevice); 
    cufftExecR2C(p->plan,pp,(cufftComplex*)pp);
   }

  cudaDeviceSynchronize();
  cudaDeviceProp deviceProp;
  cudaGetDeviceProperties(&deviceProp, 0);
  int blockspt = deviceProp.maxThreadsPerBlock;
 
  end >>= 1;

  p->blocks = end > blockspt ? ceil(end/blockspt) : 1;
  p->threads = end > blockspt ? blockspt : end;

  csound->RegisterDeinitCallback(csound, p, destroy_pconv);

  OPARMS parms;
  csound->GetOParms(csound, &parms);
  if(parms.odebug)
   csound->Message(csound, 
     "blocks %d - threads/block %d - threads %d - dftsize %d\n", 
      p->blocks, p->threads, end, dftsize);

  p->nparts = nparts;
  p->dftsize = dftsize;

  return OK;
}

int pconv_perf(CSOUND *csound, PCONV *p){

  int dftsize = p->dftsize, cnt = p->cnt, wp = p->wp, nparts = p->nparts;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  float *bufin = (float *) p->bufin.auxp, *bufout = (float *) p->bufout.auxp;
  MYFLT *asig = p->asig, *aout = p->aout;
  float *in = p->in, *out = p->out, *coef = p->coef;
  int end = nparts*dftsize;
  int parts = *p->parts;

  if (UNLIKELY(offset)) memset(asig, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&asig[nsmps], '\0', early*sizeof(MYFLT));
  }

  for(n = offset; n < nsmps; n++){
    bufin[cnt] = (float) asig[n];
    aout[n] = (MYFLT) bufout[cnt]/dftsize;
   
    if(++cnt == parts){

       /* in buffer pos */
       int pos = wp*dftsize;

       /* increment delay line pos
          so that it points to the oldest partition
       */
       wp += 1;
       if(wp == nparts) wp = 0;

       /* copy current buffer into newest partition */
       cudaMemset(out, 0, sizeof(float)*(dftsize));
       memset(&bufin[parts], 0, sizeof(float)*(parts));
       cudaMemcpy(&in[pos],bufin, sizeof(float)*dftsize,cudaMemcpyHostToDevice);

       /* apply transform */
       if(cufftExecR2C(p->plan,&in[pos],(cufftComplex*)&in[pos])
        != CUFFT_SUCCESS) csound->Message(csound, "cuda fft error\n");
       if (cudaDeviceSynchronize() != cudaSuccess)
        csound->Message(csound,"Cuda error: Failed to synchronize\n");

       cudaMemset(out, 0, sizeof(float)*dftsize);
       /* convolution */
       pconvol<<<p->blocks,p->threads>>>(out, in, coef, wp, dftsize, nparts, end);
       if (cudaDeviceSynchronize() != cudaSuccess)
        csound->Message(csound,"Cuda error: Failed to synchronize\n");

       /* transform output */
       if(cufftExecC2R(p->iplan,(cufftComplex*)out,out) 
          != CUFFT_SUCCESS) csound->Message(csound, "cuda fft error\n"); 
 
       /* copy buffer out */
       cudaMemcpy(bufin, out, sizeof(float)*dftsize,cudaMemcpyDeviceToHost);
 
       /* overlap-save */
       for(int i=0; i < parts; i++) {
	  bufout[i] = bufin[i] + bufout[i+parts];
          bufout[i+parts] = bufin[i+parts];
       }
       cnt = 0;
    }
  }
  p->cnt = cnt;
  p->wp = wp;
  return OK;
}

static OENTRY localops[] = {
  {"cudapconv", sizeof(PCONV),0, 5, "a", "aii", (SUBR) pconv_init, NULL,
    (SUBR) pconv_perf},
};

extern "C" {
  LINKAGE
}
