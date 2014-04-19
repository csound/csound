
#include <csdl.h>
#include <pstream.h>

#ifdef __MACH__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif


const char *code = 
  "#define FMAXLEN    ((float)0x40000000) \n"
  "#define PHMASK     0x3fffffff \n"
  "#define PI         3.1415926f\n"
  "inline void AtomicAdd(volatile __global float *source, const float operand) {\n"
  "    union {\n"
  "        unsigned int intVal;\n"
  "        float floatVal;\n"
  "    } newVal;\n"
  "    union {\n"
  "        unsigned int intVal;\n"
  "        float floatVal;\n"
  "    } prevVal;\n"
  "    do {\n"
  "        prevVal.floatVal = *source;\n"
  "        newVal.floatVal = prevVal.floatVal + operand;\n"
  "    } while (atomic_cmpxchg((volatile __global unsigned int *)source, \n"
  "      prevVal.intVal, newVal.intVal) != prevVal.intVal);\n"
  "}\n"
  "kernel void sample(global float *out, global float *frame, \n"
  "                   global long *ph,\n"
  "                   global float *amps, float pitch, int bins, \n"
  "                   int vsize,  float sr) {\n"
  "  int t = get_global_id(0);\n"
  "  int n =  t%vsize;  /* sample index */\n"
  "  int h = t/vsize;  /* bin index */\n"
  "  int k = h<<1;\n"
  "  long lph;\n"
  "  float a = amps[h], ascl = ((float)n)/vsize;\n"
  "  float fscal = pitch*FMAXLEN/sr;\n"
  "  lph = (ph[h] + (long)(n*round(frame[k+1]*fscal))) & PHMASK;\n"
  "  a += ascl*(frame[k] - a);\n"
  "  AtomicAdd(&out[n], a*sin((2*PI*lph)/FMAXLEN));\n"
  "}\n"
  "kernel void update(global float *out, global float *frame,\n"
  "		   global long *ph, global float *amps, float pitch, int  bins, \n"
  "      int vsize, float sr){\n"
  " int h =  get_global_id(0);\n"
  " int k = h << 1,i,j;\n"
  " /* update phases and amps */\n"
  " ph[h]  = (ph[h] + (long)(vsize*round(pitch*frame[k+1]*FMAXLEN/sr))) & PHMASK;\n"
  " amps[h] = frame[k];\n"
  " if(h >= vsize) return;\n"
  " out[h] = 0.f;\n"
  "}\n"
  " \n";

typedef struct cladsyn_ {
  OPDS h;
  MYFLT *asig;
  PVSDAT *fsig;
  MYFLT *kamp, *kfreq;
  MYFLT *inum;
  cl_mem out;
  cl_mem frame;
  cl_mem ph;
  float *fp;
  cl_mem amps;
  AUXCH out_;
  int bins;
  size_t threads;
  int count;
  int vsamps;
  size_t mthreads;
  int framecount;
  cl_context context;                
  cl_command_queue commands;
  cl_program program;                
  cl_kernel kernel1, kernel2;
  size_t wgs1, wgs2;
  float sr;
} CLADSYN;


static int destroy_cladsyn(CSOUND *csound, void *pp);

static int init_cladsyn(CSOUND *csound, CLADSYN *p){

  int asize, ipsize, fpsize, err;
  cl_device_id device_ids[4], device_id;             
  cl_context context;                
  cl_command_queue commands;          
  cl_program program;                
  cl_kernel kernel1, kernel2;                 
  cl_uint num;

  if(p->fsig->overlap > 1024)
     return csound->InitError(csound, "overlap is too large\n");
  
  err = clGetDeviceIDs(NULL,CL_DEVICE_TYPE_GPU, 4, device_ids, &num);
  if (err != CL_SUCCESS)
  {
        csound->Message(csound, "Error: Failed to create a device group!\n");
        return NOTOK;
  }
  
  for(int i=0; i < num; i++){
  char name[128];
  clGetDeviceInfo(device_ids[i], CL_DEVICE_NAME, 128, name, NULL);
  fprintf(stdout, "GPU[%d] %s\n",i, name);
  }
  device_id = device_ids[0];

   context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
   if (!context)
    {
        csound->Message(csound, "Failed to create a compute context!\n");
        return  NOTOK;
    }

    // Create a command commands
    //
    commands = clCreateCommandQueue(context, device_id, 0, &err);
    if (!commands)
    {
       csound->Message(csound, "Failed to create a command commands!\n");
        return NOTOK;
    }

    // Create the compute program from the source buffer
    //
    program = clCreateProgramWithSource(context, 1, (const char **) &code, NULL, &err);
    if (!program)
    {
      csound->Message(csound, "Error: Failed to create compute program!\n");
        return NOTOK;
    }
  
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];
        csound->Message(csound, "Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        csound->Message(csound, "%s\n", buffer);
        return NOTOK;
    }

    // Create the compute kernel in the program we wish to run
    //
    kernel1 = clCreateKernel(program, "sample", &err);
    if (!kernel1 || err != CL_SUCCESS)
    {
        csound->Message(csound,"Error: Failed to create sample compute kernel!\n");
        return NOTOK;
    }

   kernel2 = clCreateKernel(program, "update", &err);
    if (!kernel2 || err != CL_SUCCESS)
    {
        csound->Message(csound,"Error: Failed to create update compute kernel!\n");
        return NOTOK;
    }

 
  char name[128];
  clGetDeviceInfo(device_id, CL_DEVICE_NAME, 128, name, NULL);
    fprintf(stdout, "Using GPU: %s\n", name);

  p->bins = (p->fsig->N)/2;

  if(*p->inum > 0 && *p->inum < p->bins) p->bins = *p->inum;

  p->vsamps = p->fsig->overlap;
  p->threads = p->bins*p->vsamps;
  p->mthreads = p->bins;

  asize =  p->vsamps*sizeof(cl_float);
  ipsize = p->fsig->N*sizeof(cl_long)/2;
  fpsize = p->fsig->N*sizeof(cl_float);

  p->out = clCreateBuffer(context,0, asize, NULL, NULL);
  p->frame =   clCreateBuffer(context, CL_MEM_READ_ONLY, fpsize, NULL, NULL);
  p->ph =  clCreateBuffer(context,0, ipsize, NULL, NULL);
  p->amps =  clCreateBuffer(context,0, fpsize/2, NULL, NULL);
 
  // memset needed?

  asize = p->vsamps*sizeof(float);
  if(p->out_.auxp == NULL ||
     p->out_.size < asize)
    csound->AuxAlloc(csound, asize , &p->out_);

  csound->RegisterDeinitCallback(csound, p, destroy_cladsyn);
  p->count = 0;
  p->context = context;
  p->program = program;
  p->commands = commands;
  p->kernel1 = kernel1;
  p->kernel2 = kernel2;
 
  clGetKernelWorkGroupInfo(p->kernel1, 
       device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(p->wgs1), &p->wgs1, NULL);
  clGetKernelWorkGroupInfo(p->kernel2, 
       device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(p->wgs1), &p->wgs2, NULL);
 
  p->sr = csound->GetSr(csound); 
  clSetKernelArg(p->kernel1, 0, sizeof(cl_mem), &p->out);
  clSetKernelArg(p->kernel1, 1, sizeof(cl_mem), &p->frame);
  clSetKernelArg(p->kernel1, 2, sizeof(cl_mem), &p->ph);
  clSetKernelArg(p->kernel1, 3, sizeof(cl_mem), &p->amps);
  clSetKernelArg(p->kernel1, 5, sizeof(cl_int), &p->bins);
  clSetKernelArg(p->kernel1, 6, sizeof(cl_int), &p->vsamps);
  clSetKernelArg(p->kernel1, 7, sizeof(cl_float), &p->sr);

  clSetKernelArg(p->kernel2, 0, sizeof(cl_mem), &p->out);
  clSetKernelArg(p->kernel2, 1, sizeof(cl_mem), &p->frame);
  clSetKernelArg(p->kernel2, 2, sizeof(cl_mem), &p->ph);
  clSetKernelArg(p->kernel2, 3, sizeof(cl_mem), &p->amps);
  clSetKernelArg(p->kernel2, 5, sizeof(cl_int), &p->bins);
  clSetKernelArg(p->kernel2, 6, sizeof(cl_int), &p->vsamps);
  clSetKernelArg(p->kernel2, 7, sizeof(cl_float),  &p->sr); 
  return OK;
}

static int perf_cladsyn(CSOUND *csound, CLADSYN *p){

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
     int err;
     float freq = *p->kfreq;
     clSetKernelArg(p->kernel1, 4, sizeof(cl_float), &freq);
     clSetKernelArg(p->kernel2, 4, sizeof(cl_float), &freq);

     clEnqueueWriteBuffer(p->commands,p->frame, CL_TRUE, 0,
                          sizeof(cl_float)*p->bins*2,
                          p->fp, 0, NULL, NULL);

     err = clEnqueueNDRangeKernel(p->commands,p->kernel1, 1, NULL, &p->threads, 
            &p->wgs1, 0, NULL, NULL);
     if(err) 
       csound->Message(csound,"Error: Failed to compute sample kernel! %d \n", err);
     clFinish(p->commands);
     clEnqueueReadBuffer(p->commands,p->out, 
               CL_TRUE, 0,vsamps*sizeof(cl_float),out_, 0, NULL, NULL);
     err = clEnqueueNDRangeKernel(p->commands,p->kernel2, 1, NULL, &p->mthreads, 
          &p->wgs2, 0, NULL, NULL);
     if(err) 
       csound->Message(csound,"Error: Failed to compute update kernel! %d \n",err);
     count = vsamps;
    }
    asig[n] = (MYFLT) out_[vsamps - count];
    count--;
  }
  p->count = count;
  return OK;
}

static int destroy_cladsyn(CSOUND *csound, void *pp){
  CLADSYN *p = (CLADSYN *) pp;
  clReleaseMemObject(p->out);
  clReleaseMemObject(p->ph);
  clReleaseMemObject(p->frame);
  clReleaseMemObject(p->amps);
  clReleaseProgram(p->program);
  clReleaseKernel(p->kernel1);
  clReleaseKernel(p->kernel2);
  clReleaseCommandQueue(p->commands);
  clReleaseContext(p->context);

  return OK;
}

static OENTRY localops[] = {
  {"cladsynth", sizeof(CLADSYN),0, 5, "a", "fkko", (SUBR) init_cladsyn, NULL,
   (SUBR) perf_cladsyn}
};

LINKAGE
