/*
  cladsynth.cpp

  (c) Victor Lazzarini, 2019

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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include <plugin.h>
#include <pstream.h>
#include <iostream>
#include <sstream>

#ifdef __MACH__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

const char *code =  R"(
#define FMAXLEN    ((float)0x40000000) 
#define PHMASK     0x3fffffff 
#define PI         3.1415926f

inline void AtomicAdd(volatile __global float *source, const float operand) {
     union {
         unsigned int intVal;
         float floatVal;
     } newVal;
     union {
         unsigned int intVal;
         float floatVal;
     } prevVal;
     do {
         prevVal.floatVal = *source;
         newVal.floatVal = prevVal.floatVal + operand;
     } while (atomic_cmpxchg((volatile __global unsigned int *)source, 
       prevVal.intVal, newVal.intVal) != prevVal.intVal);
}

kernel void sample(global float *out, global float *frame, 
                    global long *ph,
                    global float *amps, float pitch, int bins, 
                    int vsize,  float sr) {
   int t = get_global_id(0);
   int n =  t%vsize;  /* sample index */
   int h = t/vsize;  /* bin index */
   int k = h<<1;
   long lph;
   float a = amps[h], ascl = ((float)n)/vsize;
   float fscal = pitch*FMAXLEN/sr;
   lph = (ph[h] + (long)(n*round(frame[k+1]*fscal))) & PHMASK;
   a += ascl*(frame[k] - a);
   AtomicAdd(&out[n], a*sin((2*PI*lph)/FMAXLEN));
}

kernel void update(global float *out, global float *frame,
       global long *ph, global float *amps, float pitch, int  bins, 
       int vsize, float sr){
  int h =  get_global_id(0);
  int k = h << 1,i,j;
  /* update phases and amps */
  ph[h]  = (ph[h] + (long)(vsize*round(pitch*frame[k+1]*FMAXLEN/sr))) & PHMASK;
  amps[h] = frame[k];
  if(h >= vsize) return;
  out[h] = 0.f;
}

)";


const char * cl_error_string(int err) {
    switch (err) {
    case CL_SUCCESS:                            return "Success!";
    case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
    case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
    case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
                                   return "Memory object allocation failure";
    case CL_OUT_OF_RESOURCES:                   return "Out of resources";
    case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
                                   return "Profiling information not available";
    case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
    case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
    case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
    case CL_MAP_FAILURE:                        return "Map failure";
    case CL_INVALID_VALUE:                      return "Invalid value";
    case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
    case CL_INVALID_PLATFORM:                   return "Invalid platform";
    case CL_INVALID_DEVICE:                     return "Invalid device";
    case CL_INVALID_CONTEXT:                    return "Invalid context";
    case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
    case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
    case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
    case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
                                   return "Invalid image format descriptor";
    case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
    case CL_INVALID_SAMPLER:                    return "Invalid sampler";
    case CL_INVALID_BINARY:                     return "Invalid binary";
    case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
    case CL_INVALID_PROGRAM:                    return "Invalid program";
    case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
    case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
    case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
    case CL_INVALID_KERNEL:                     return "Invalid kernel";
    case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
    case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
    case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
    case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
    case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
    case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
    case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
    case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
    case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
    case CL_INVALID_EVENT:                      return "Invalid event";
    case CL_INVALID_OPERATION:                  return "Invalid operation";
    case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
    case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
    case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
    default: return "Unknown error";
    }
}

struct Cladsyn : csnd::Plugin<1, 5> {

  cl_mem out;
  cl_mem frame;
  cl_mem ph;
  cl_mem amps;
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
  csnd::AuxMem<float> mix;
  float cs_sr;


  int init() {
    int asize, ipsize, fpsize, err;
    cl_device_id device_ids[32], device_id;
    cl_uint num = 0, nump =  0;
    cl_platform_id platforms[16];
    uint32_t i;
    csnd::pv_frame &fsig = inargs.fsig_data(0);
    int inum = (int) inargs[3];
    int idev = (int) inargs[4];

    if(fsig.hop_size() > 1024)
     return csound->init_error("hopsize is too large\n");

    err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, 32, device_ids, &num);
    if (err != CL_SUCCESS){
      int devs = 0;
      clGetPlatformIDs(16, platforms, &nump);
      for(i=0; i < nump && devs < 32; i++){
        char name[128];
        std::stringstream msg;
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 128, name, NULL);
        msg << "available platform[" << i << "]: " << name << std::endl; 
        csound->message(msg.str());
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL,
                          32-devs, &device_ids[devs], &num);
        if (err != CL_SUCCESS) {
         std::stringstream emsg;
         emsg << "failed to find an OpenCL device!" <<
           cl_error_string(err) << std::endl;
         csound->init_error(emsg.str());
        }
      }
      devs += num;
    }

   for(i=0; i < num; i++){
     char name[128];
     cl_device_type type;
     clGetDeviceInfo(device_ids[i], CL_DEVICE_NAME, 128, name, NULL);
     clGetDeviceInfo(device_ids[i], CL_DEVICE_TYPE, sizeof(cl_device_type),
                  &type, NULL);
     if(type & CL_DEVICE_TYPE_CPU) {
       std::stringstream msg;
       msg <<  "available CPU[device " << i << "] " << name << std::endl;
       csound->message(msg.str());
     }
     else  if(type & CL_DEVICE_TYPE_GPU) {
       std::stringstream msg;
       msg <<  "available GPU[device " << i << "] " << name << std::endl;
       csound->message(msg.str());
     }
     else  if(type & CL_DEVICE_TYPE_ACCELERATOR) {
       std::stringstream msg;
       msg <<  "available ACCELLERATOR[device " << i << "] " << name
           << std::endl;
       csound->message(msg.str());
     }
     else {
       std::stringstream msg;
       msg <<  "available GENERIC[device " << i << "] " << name << std::endl;
       csound->message(msg.str());
     }
  }

  // SELECT THE DEVICE HERE
  if(idev < num)
   device_id = device_ids[idev];
  else
   device_id = device_ids[num-1];

   context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
   if (!context) {
     std::stringstream msg;
     msg << "Failed to create a compute context! " << cl_error_string(err)
         << std::endl;
     return csound->init_error(msg.str());
   }

    // Create commands
    commands = clCreateCommandQueue(context, device_id, 0, &err);
    if (!commands) {
     std::stringstream msg;
     msg << "Failed to create commands! " << cl_error_string(err)
         << std::endl;
     return csound->init_error(msg.str());
    }
    // Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1, (const char **) &code,
                                        NULL, &err);
    if (!program){
     std::stringstream msg;
     msg << "Failed to create program! " << cl_error_string(err)
         << std::endl;
     return csound->init_error(msg.str());
    }
    
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
     size_t len;
     char buffer[2048];
     std::stringstream msg;
     clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG,
                              sizeof(buffer), buffer, &len);
     msg << "Failed to build program executable! " << cl_error_string(err)
         << std::endl << buffer << std::endl;
     return csound->init_error(msg.str());
    }

    kernel1 = clCreateKernel(program, "sample", &err);
    if (!kernel1 || err != CL_SUCCESS) {
     std::stringstream msg;
     msg << "Failed to create sample compute kernel! " << cl_error_string(err)
         << std::endl;
     return csound->init_error(msg.str());
    }

    kernel2 = clCreateKernel(program, "update", &err);
    if (!kernel2 || err != CL_SUCCESS) {
     std::stringstream msg;
     msg << "Failed to create update compute kernel! " << cl_error_string(err)
         << std::endl;
     return csound->init_error(msg.str());
    }

    {
    char name[128];
    std::stringstream msg;
    clGetDeviceInfo(device_id, CL_DEVICE_NAME, 128, name, NULL);
    msg << "using device: " << name << std::endl;
    csound->message(msg.str());
    }

    bins = fsig.nbins() - 1;
    if(inum > 0 && inum < bins) bins = inum; 
    vsamps = fsig.hop_size();
    threads = bins*vsamps;
    mthreads = bins > vsamps ? bins : vsamps;
    asize =  vsamps*sizeof(cl_float);
    ipsize = mthreads*sizeof(cl_long);
    fpsize = fsig.dft_size()*sizeof(cl_float);

    out = clCreateBuffer(context,0, asize, NULL, NULL);
    frame = clCreateBuffer(context, CL_MEM_READ_ONLY, fpsize, NULL, NULL);
    ph = clCreateBuffer(context,0, ipsize, NULL, NULL);
    amps = clCreateBuffer(context,0, mthreads*sizeof(cl_float), NULL, NULL);

   // memset needed?
   asize = vsamps*sizeof(float);
   mix.allocate(csound, asize);
   csound->plugin_deinit(this);
   count = 0;
   cs_sr = csound->sr();

   clGetKernelWorkGroupInfo(kernel1,
       device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs1), &wgs1, NULL);
   clGetKernelWorkGroupInfo(kernel2,
       device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs2), &wgs2, NULL);

   clSetKernelArg(kernel1, 0, sizeof(cl_mem), &out);
   clSetKernelArg(kernel1, 1, sizeof(cl_mem), &frame);
   clSetKernelArg(kernel1, 2, sizeof(cl_mem), &ph);
   clSetKernelArg(kernel1, 3, sizeof(cl_mem), &amps);
   clSetKernelArg(kernel1, 5, sizeof(cl_int), &bins);
   clSetKernelArg(kernel1, 6, sizeof(cl_int), &vsamps);
   clSetKernelArg(kernel1, 7, sizeof(cl_float), &cs_sr);

   clSetKernelArg(kernel2, 0, sizeof(cl_mem), &out);
   clSetKernelArg(kernel2, 1, sizeof(cl_mem), &frame);
   clSetKernelArg(kernel2, 2, sizeof(cl_mem), &ph);
   clSetKernelArg(kernel2, 3, sizeof(cl_mem), &amps);
   clSetKernelArg(kernel2, 5, sizeof(cl_int), &bins);
   clSetKernelArg(kernel2, 6, sizeof(cl_int), &vsamps);
   clSetKernelArg(kernel2, 7, sizeof(cl_float), &cs_sr);
  
   return OK;
  }

  int deinit() {
   clReleaseMemObject(out);
   clReleaseMemObject(ph);
   clReleaseMemObject(frame);
   clReleaseMemObject(amps);
   clReleaseProgram(program);
   clReleaseKernel(kernel1);
   clReleaseKernel(kernel2);
   clReleaseCommandQueue(commands);
   clReleaseContext(context);
   return OK;
  }

  int aperf() {
    
   uint32_t n;
   csnd::AudioSig asig(this, outargs(0));
   float *fp = inargs.fsig_data(0).data();
   
   for (auto &s : asig) {
    if(count == 0) {
     int err;
     float freq = inargs[2];
     clSetKernelArg(kernel1, 4, sizeof(cl_float), &freq);
     clSetKernelArg(kernel2, 4, sizeof(cl_float), &freq);

     clEnqueueWriteBuffer(commands,frame, CL_TRUE, 0, sizeof(cl_float)*bins*2,
                          fp, 0, NULL, NULL);
     err = clEnqueueNDRangeKernel(commands, kernel1, 1, NULL, &threads, &wgs1,
                                  0, NULL, NULL);
     if(err)  {
      std::stringstream msg;
      msg << "Error: Failed to compute sample kernel!" << cl_error_string(err)
         << std::endl;
      csound->message(msg.str());
     }
     clFinish(commands);
     clEnqueueReadBuffer(commands, out,
                         CL_TRUE, 0,vsamps*sizeof(cl_float), mix.data(), 0, NULL, NULL);
     err = clEnqueueNDRangeKernel(commands,kernel2, 1, NULL, &mthreads,
          &wgs2, 0, NULL, NULL);
     if(err) {
      std::stringstream msg;
      msg << "Error: Failed to compute update kernel!" << cl_error_string(err)
         << std::endl;
      csound->message(msg.str());
     }
     count = vsamps;
    }
    
    s = mix[vsamps - count]*inargs[1];
    count--;
   }
   
   return OK;
  }
};

#include <modload.h>
void csnd::on_load(Csound *csound) {
  csnd::plugin<Cladsyn>(csound, "cladsyn", "a", "fkkii", csnd::thread::ia);
}

  
