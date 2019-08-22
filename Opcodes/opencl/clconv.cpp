/*
  clconv: open cl fast partitioned convolution

  Copyright (C) 2019 Victor Lazzarini
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

#include <modload.h>
#include <plugin.h>
#include <vector>

#ifdef __MACH__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

namespace csnd {
  
const char *code = R"(
/* complex type */
typedef float2 cmplx;
/* atomic add */
#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable 
inline void AtomicAdd(volatile __global cmplx *source, const cmplx operand) {
     union {
         ulong intVal;
         cmplx floatVal;
     } newVal;
     union {
         ulong intVal;
         cmplx floatVal;
     } prevVal;
     do {
         prevVal.floatVal = *source;
         newVal.floatVal = prevVal.floatVal + operand;
     } while (atomic_cmpxchg((volatile __global ulong *) source, 
       prevVal.intVal, newVal.intVal) != prevVal.intVal);
}
/* complex product */
inline cmplx prod(cmplx a, cmplx b){
     return (cmplx)(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x); 
}
/* complex conj */
inline cmplx conjg(cmplx a) {
    return (cmplx) (a.x, - a.y);
}
/* rotation by pi */
inline cmplx rot(cmplx a) {
   return (cmplx) (-a.y, a.x);
}  
/* reorder kernel */
kernel void reorder(global cmplx *out, global cmplx *in, global const int *b) {
   int k = get_global_id(0);
   out[k] = in[b[k]];     
}
/* fft kernel */
kernel void fft(global cmplx *s, global const cmplx *w, int N, int n2) {
 int k, i, m, n;
 cmplx e, o;
 k = get_global_id(0)*n2;
 m = k/N; 
 n = n2 >> 1;
 k =  k%N + m;
 i = k + n;
 e = s[k];
 o = prod(s[i],w[m*N/n2]);
 s[k] = e + o;
 s[i] = e - o; 
}
/* conversion kernels */
kernel void r2c(global cmplx *c, global const cmplx *w, int N) {
  int i = get_global_id(0);
  if(!i) {
   c[0] = (cmplx) ((c[0].x + c[0].y)*.5f, (c[0].x - c[0].y)*.5f);
   return;
  }
  int j = N - i;
  cmplx e, o, cj = conjg(c[j]), p;
  e = .5f*(c[i] + cj);
  o = .5f*rot(cj - c[i]);
  p = prod(w[i], o); 
  c[i] = e + p;
  c[j] = conjg(e - p);
}
kernel void c2r(global cmplx *c, global const cmplx *w, int N) {
  int i = get_global_id(0);
  if(!i) {
   c[0] = (cmplx) ((c[0].x + c[0].y), (c[0].x - c[0].y));
   return; 
  }
  int j = N - i;
  cmplx e, o, cj = conjg(c[j]), p;
  e = .5f*(c[i] + cj);
  o = .5f*rot(c[i] - cj);
  p = prod(w[i], o);
  c[i] = e + p;
  c[j] = conjg(e - p); 
}
/* convolution */
kernel void convol(global cmplx *out, global const cmplx *in, 
		    global const cmplx *coef, int rp, int b, 
		    int nparts, int bsize) {

  /* thread count */
  int k = get_global_id(0); /* bin pos */ 
  int n = k%b;  /* inframe pos   */

  /* if beyond the buffer end, exit */
  if(k >= bsize) return;                  
  rp += k/b;       /*  rp pos */

  /* select correct input buffer */
  in += (rp < nparts ? rp : rp%nparts)*b;
  
  /* complex multiplication + sums */
  atomicAdd(&out[n], (n ? in[n]*coef[k] :
                      (cmplx) (in[0].x*coef[k].x, in[0].y*coef[k].x));                                    
}  

/* sample-by-sample overlap-add operation */
kernel void olap(float *buf, float *in, int parts){
   int n = get_global_id(0);
   buf[n] = in[n]/parts + buf[parts+n];
   buf[parts+n] = in[parts+n];
}
)";

class Clconv {

  Csound *csound;
  int N, bins;
  int bsize, nparts, wp;
  cl_mem w[2], w2[2], b;
  cl_mem fftin, fftout;
  cl_mem buff, coefs, in;
  cl_context context;
  cl_command_queue commands;
  cl_program program;
  cl_kernel fft_kernel, reorder_kernel;
  cl_kernel r2c_kernel, c2r_kernel;
  cl_kernel convol_kernel, olap_kernel;
  size_t wgs, rwgs;
  size_t crwgs, rcwgs;
  size_t cvwgs, olwgs;

public:
  Clconv(Csound *cs, cl_device_id device_id, int cvs, int pts)
      : csound(cs), N(pts << 1), bins(pts), bsize((cvs / pts) * N),
        nparts(cvs / pts), wp(0), w{NULL, NULL}, w2{NULL, NULL}, b(NULL),
        fftin(NULL), fftout(NULL), buff(NULL), coefs(NULL), in(NULL),
        context(NULL), commands(NULL), program(NULL), fft_kernel(NULL),
        reorder_kernel(NULL), r2c_kernel(NULL), c2r_kernel(NULL),
        convol_kernel(NULL), olap_kernel(NULL), wgs(0), rwgs(0), crwgs(0),
        rcwgs(0), cvwgs(0), olwgs(0) {

    int cl_err;
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &cl_err);
    if (!context) {
      csound->init_error(std::string(cl_error_string(cl_err)));
      return;
    }
    commands = clCreateCommandQueue(context, device_id, 0, &cl_err);
    if (!commands) {
      clReleaseContext(context);
      csound->init_error(std::string(cl_error_string(cl_err)));
      return;
    }
    program = clCreateProgramWithSource(context, 1, (const char **)&code, NULL,
                                        &cl_err);
    if (!program) {
      clReleaseCommandQueue(commands);
      clReleaseContext(context);
      csound->init_error(std::string(cl_error_string(cl_err)));
      return;
    }
    cl_err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (cl_err) {
      char log[2048];
      size_t llen;
      clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG,
                            sizeof(log), log, &llen);
      clReleaseProgram(program);
      clReleaseContext(context);
      clReleaseCommandQueue(commands);
      clReleaseContext(context);
      csound->message(std::string(cl_error_string(cl_err)));
      csound->init_error(std::string(log));
      return;
    }

    reorder_kernel = clCreateKernel(program, "reorder", &cl_err);
    fft_kernel = clCreateKernel(program, "fft", &cl_err);
    r2c_kernel = clCreateKernel(program, "r2c", &cl_err);
    c2r_kernel = clCreateKernel(program, "c2r", &cl_err);
    convol_kernel = clCreateKernel(program, "convol", &cl_err);
    olap_kernel = clCreateKernel(program, "olap", &cl_err);

    clGetKernelWorkGroupInfo(reorder_kernel, device_id,
                             CL_KERNEL_WORK_GROUP_SIZE, sizeof(rwgs), &rwgs,
                             NULL);
    if (rwgs > bins)
      rwgs = bins;
    clGetKernelWorkGroupInfo(fft_kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE,
                             sizeof(wgs), &wgs, NULL);
    if (wgs > bins / 2)
      wgs = bins / 2;
    clGetKernelWorkGroupInfo(r2c_kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE,
                             sizeof(rcwgs), &rcwgs, NULL);
    if (rcwgs > bins / 2)
      rcwgs = bins / 2;
    clGetKernelWorkGroupInfo(c2r_kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE,
                             sizeof(crwgs), &crwgs, NULL);
    if (crwgs > bins / 2)
      crwgs = bins / 2;
    clGetKernelWorkGroupInfo(convol_kernel, device_id,
                             CL_KERNEL_WORK_GROUP_SIZE, sizeof(rcwgs), &cvwgs,
                             NULL);
    if (cvwgs > bsize)
      cvwgs = bsize;
    clGetKernelWorkGroupInfo(olap_kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE,
                             sizeof(olwgs), &olwgs, NULL);
    if (olwgs > bins)
      olwgs = bins;

    fftout = clCreateBuffer(context, 0, bins * sizeof(cl_float2), NULL, NULL);
    fftin = clCreateBuffer(context, 0, bins * sizeof(cl_float2), NULL, NULL);
    w[0] = clCreateBuffer(context, CL_MEM_READ_ONLY, bins * sizeof(cl_float2),
                          NULL, NULL);
    w[1] = clCreateBuffer(context, CL_MEM_READ_ONLY, bins * sizeof(cl_float2),
                          NULL, NULL);
    w2[0] = clCreateBuffer(context, CL_MEM_READ_ONLY, bins * sizeof(cl_float2),
                           NULL, NULL);
    w2[1] = clCreateBuffer(context, CL_MEM_READ_ONLY, bins * sizeof(cl_float2),
                           NULL, NULL);
    b = clCreateBuffer(context, CL_MEM_READ_ONLY, bins * sizeof(cl_int), NULL,
                       NULL);
    coefs = clCreateBuffer(context, CL_MEM_READ_ONLY, bsize * sizeof(cl_float2),
                           NULL, NULL);
    in = clCreateBuffer(context, CL_MEM_READ_ONLY, bsize * sizeof(cl_float2),
                        NULL, NULL);
    buff = clCreateBuffer(context, 0, 2 * bins * sizeof(cl_float), NULL, NULL);

    /* twiddle */
    std::vector<std::complex<float>> wp(bins);
    for (int i = 0; i < bins; i++) {
      wp[i].real(cos(i * 2 * PI / bins));
      wp[i].imag(-sin(i * 2 * PI / bins));
    }
    clEnqueueWriteBuffer(commands, w[0], CL_TRUE, 0, sizeof(cl_float2) * N,
                         (const void *)wp.data(), 0, NULL, NULL);
    for (int i = 0; i < bins; i++) {
      wp[i].real(cos(i * 2 * PI / bins));
      wp[i].imag(sin(i * 2 * PI / bins));
    }
    clEnqueueWriteBuffer(commands, w[1], CL_TRUE, 0, sizeof(cl_float2) * N,
                         (const void *)wp.data(), 0, NULL, NULL);
    for (int i = 0; i < bins; i++) {
      wp[i].real(cos(i * PI / bins));
      wp[i].imag(-sin(i * PI / bins));
    }
    clEnqueueWriteBuffer(commands, w2[0], CL_TRUE, 0, sizeof(cl_float2) * N,
                         (const void *)wp.data(), 0, NULL, NULL);
    for (int i = 0; i < bins; i++) {
      wp[i].real(cos(i * PI / bins));
      wp[i].imag(sin(i * PI / bins));
    }
    clEnqueueWriteBuffer(commands, w2[1], CL_TRUE, 0, sizeof(cl_float2) * N,
                         (const void *)wp.data(), 0, NULL, NULL);

    /* bit-reversed indices */
    std::vector<int> bp(bins);
    for (int i = 0; i < bins; i++)
      bp[i] = i;
    for (int i = 1, n = bins / 2; i<N; i = i << 1, n = n>> 1)
      for (int j = 0; j < i; j++)
        bp[i + j] = bp[j] + n;
    clEnqueueWriteBuffer(commands, b, CL_TRUE, 0, sizeof(cl_int) * N,
                         (const void *)bp.data(), 0, NULL, NULL);

    /* signal arguments */
    clSetKernelArg(reorder_kernel, 1, sizeof(cl_mem), &fftin);  // in
    clSetKernelArg(reorder_kernel, 0, sizeof(cl_mem), &fftout); // out
    clSetKernelArg(fft_kernel, 0, sizeof(cl_mem), &fftout);     // in + out
    clSetKernelArg(r2c_kernel, 0, sizeof(cl_mem), &fftout);     // in + out
    clSetKernelArg(convol_kernel, 1, sizeof(cl_mem), &in);      // in
    clSetKernelArg(convol_kernel, 0, sizeof(cl_mem), &fftin);   // out
    clSetKernelArg(c2r_kernel, 0, sizeof(cl_mem), &fftin);      // in + out
    clSetKernelArg(olap_kernel, 1, sizeof(cl_mem), &fftout);    // in
    clSetKernelArg(olap_kernel, 0, sizeof(cl_mem), &buff);      // out

    clSetKernelArg(reorder_kernel, 2, sizeof(cl_mem), &b);
    clSetKernelArg(fft_kernel, 2, sizeof(cl_int), &bins);
    clSetKernelArg(r2c_kernel, 1, sizeof(cl_mem), &w2[1]);
    clSetKernelArg(r2c_kernel, 2, sizeof(cl_int), &bins);
    clSetKernelArg(c2r_kernel, 1, sizeof(cl_mem), &w2[0]);
    clSetKernelArg(c2r_kernel, 2, sizeof(cl_int), &bins);
    clSetKernelArg(convol_kernel, 4, sizeof(cl_int), &bins);
    clSetKernelArg(convol_kernel, 5, sizeof(cl_int), &nparts);
    clSetKernelArg(convol_kernel, 6, sizeof(cl_int), &bsize);
    clSetKernelArg(convol_kernel, 6, sizeof(cl_int), &bins);
    clSetKernelArg(olap_kernel, 2, sizeof(cl_int), &nparts);
  }

  ~Clconv() {
    clReleaseMemObject(w2[0]);
    clReleaseMemObject(w2[1]);
    clReleaseMemObject(w[0]);
    clReleaseMemObject(w[1]);
    clReleaseMemObject(b);
    clReleaseMemObject(fftout);
    clReleaseMemObject(fftin);
    clReleaseMemObject(buff);
    clReleaseMemObject(coefs);
    clReleaseMemObject(in);
    clReleaseKernel(fft_kernel);
    clReleaseKernel(reorder_kernel);
    clReleaseKernel(r2c_kernel);
    clReleaseKernel(c2r_kernel);
    clReleaseKernel(olap_kernel);
    clReleaseKernel(convol_kernel);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);
  }

protected:
  const char *cl_error_string(int err) {
    switch (err) {
    case CL_SUCCESS:
      return "Success!";
    case CL_DEVICE_NOT_FOUND:
      return "Device not found.";
    case CL_DEVICE_NOT_AVAILABLE:
      return "Device not available";
    case CL_COMPILER_NOT_AVAILABLE:
      return "Compiler not available";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
      return "Memory object allocation failure";
    case CL_OUT_OF_RESOURCES:
      return "Out of resources";
    case CL_OUT_OF_HOST_MEMORY:
      return "Out of host memory";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
      return "Profiling information not available";
    case CL_MEM_COPY_OVERLAP:
      return "Memory copy overlap";
    case CL_IMAGE_FORMAT_MISMATCH:
      return "Image format mismatch";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
      return "Image format not supported";
    case CL_BUILD_PROGRAM_FAILURE:
      return "Program build failure";
    case CL_MAP_FAILURE:
      return "Map failure";
    case CL_INVALID_VALUE:
      return "Invalid value";
    case CL_INVALID_DEVICE_TYPE:
      return "Invalid device type";
    case CL_INVALID_PLATFORM:
      return "Invalid platform";
    case CL_INVALID_DEVICE:
      return "Invalid device";
    case CL_INVALID_CONTEXT:
      return "Invalid context";
    case CL_INVALID_QUEUE_PROPERTIES:
      return "Invalid queue properties";
    case CL_INVALID_COMMAND_QUEUE:
      return "Invalid command queue";
    case CL_INVALID_HOST_PTR:
      return "Invalid host pointer";
    case CL_INVALID_MEM_OBJECT:
      return "Invalid memory object";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
      return "Invalid image format descriptor";
    case CL_INVALID_IMAGE_SIZE:
      return "Invalid image size";
    case CL_INVALID_SAMPLER:
      return "Invalid sampler";
    case CL_INVALID_BINARY:
      return "Invalid binary";
    case CL_INVALID_BUILD_OPTIONS:
      return "Invalid build options";
    case CL_INVALID_PROGRAM:
      return "Invalid program";
    case CL_INVALID_PROGRAM_EXECUTABLE:
      return "Invalid program executable";
    case CL_INVALID_KERNEL_NAME:
      return "Invalid kernel name";
    case CL_INVALID_KERNEL_DEFINITION:
      return "Invalid kernel definition";
    case CL_INVALID_KERNEL:
      return "Invalid kernel";
    case CL_INVALID_ARG_INDEX:
      return "Invalid argument index";
    case CL_INVALID_ARG_VALUE:
      return "Invalid argument value";
    case CL_INVALID_ARG_SIZE:
      return "Invalid argument size";
    case CL_INVALID_KERNEL_ARGS:
      return "Invalid kernel arguments";
    case CL_INVALID_WORK_DIMENSION:
      return "Invalid work dimension";
    case CL_INVALID_WORK_GROUP_SIZE:
      return "Invalid work group size";
    case CL_INVALID_WORK_ITEM_SIZE:
      return "Invalid work item size";
    case CL_INVALID_GLOBAL_OFFSET:
      return "Invalid global offset";
    case CL_INVALID_EVENT_WAIT_LIST:
      return "Invalid event wait list";
    case CL_INVALID_EVENT:
      return "Invalid event";
    case CL_INVALID_OPERATION:
      return "Invalid operation";
    case CL_INVALID_GL_OBJECT:
      return "Invalid OpenGL object";
    case CL_INVALID_BUFFER_SIZE:
      return "Invalid buffer size";
    case CL_INVALID_MIP_LEVEL:
      return "Invalid mip-map level";
    default:
      return "Unknown error";
    }
  }

  void fft() {
    size_t threads = bins;
    int n2;
    clEnqueueNDRangeKernel(commands, reorder_kernel, 1, NULL, &threads, &rwgs,
                           0, NULL, NULL);
    for (int n = 1; n < N; n *= 2) {
      n2 = n << 1;
      threads = N >> 1;
      clSetKernelArg(fft_kernel, 3, sizeof(cl_int), &n2);
      clEnqueueNDRangeKernel(commands, fft_kernel, 1, NULL, &threads, &wgs, 0,
                             NULL, NULL);
      clFinish(commands);
    }
  }

  void rfft() {
    size_t threads = bins >> 1;
    clSetKernelArg(fft_kernel, 1, sizeof(cl_mem), &w[0]);
    fft();
    clEnqueueNDRangeKernel(commands, r2c_kernel, 1, NULL, &threads, &rcwgs, 0,
                           NULL, NULL);
    clFinish(commands);
  }

  void rifft() {
    size_t threads = N >> 1;
    clSetKernelArg(fft_kernel, 1, sizeof(cl_mem), &w[1]);
    clEnqueueNDRangeKernel(commands, c2r_kernel, 1, NULL, &threads, &crwgs, 0,
                           NULL, NULL);
    clFinish(commands);
    fft();
  }

public:
  void push_ir(float *ir) {
    std::vector<float> sig(bins);
    size_t bytes = sizeof(cl_float2) * bins;
    for (int i = 0; i < nparts; i++) {
      std::copy(ir + i * bins, ir + (i + 1) * bins, sig.begin());
      clEnqueueWriteBuffer(commands, fftin, CL_TRUE, 0,
                           sizeof(cl_float2) * bins, sig.data(), 0, NULL, NULL);
      rfft();
      clEnqueueCopyBuffer(commands, fftout, coefs, 0, bytes * (nparts - i - 1),
                          bytes, 0, NULL, NULL);
    }
  }

  void convolution(float *output, float *input) {
    char zro = 0;
    size_t bytes = sizeof(cl_float2) * bins, threads;
    // input is N size
    clEnqueueWriteBuffer(commands, fftin, CL_TRUE, 0, sizeof(cl_float2) * bins,
                         input, 0, NULL, NULL);
    rfft(); // fftin => fftout
    // fftout => in[]
    clEnqueueCopyBuffer(commands, fftout, in, 0, bytes * wp, bytes, 0, NULL,
                        NULL);
    // reset fftin to use in sum
    clEnqueueFillBuffer(commands, fftin, &zro, 1, 0, bytes, 0, NULL, NULL);
    threads = bsize;
    wp = wp != nparts - 1 ? wp + 1 : 0;
    clSetKernelArg(convol_kernel, 3, sizeof(cl_int), &wp);
    clEnqueueNDRangeKernel(commands, convol_kernel, 1, NULL, &threads, &cvwgs,
                           0, NULL, NULL); // in[] => fftin
    rifft();                               // fftin => fftout
    threads = bins;
    clEnqueueNDRangeKernel(commands, olap_kernel, 1, NULL, &threads, &olwgs, 0,
                           NULL, NULL); // fftout => buff
    // output is bins size
    clEnqueueReadBuffer(commands, buff, CL_TRUE, 0, sizeof(cl_float) * bins,
                        output, 0, NULL, NULL);
  }
};

struct PConv : Plugin<1, 6> {
  Clconv *clconv;
  Table ir;
  int parts, cnt;
  csnd::AuxMem<float> bufin, bufout;

  int init() {
    int size;
    int err;
    cl_device_id device_ids[32], id;
    cl_uint num = 0, nump = 0;
    char name[128];

    err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, 32, device_ids, &num);
    if (err != CL_SUCCESS) {
      csound->init_error("failed to find an OpenCL device!\n");
      return -1;
    }
    id = device_ids[(int)inargs[3]];
    clGetDeviceInfo(id, CL_DEVICE_NAME, 128, name, NULL);
    csound->message("using device: ");
    csound->message(name);
    csound->message("\n");

    ir.init(csound, inargs(1));
    parts = inargs[2];
    size = inargs[5] == 0 ? ir.len() : inargs[5];
    size -= inargs[4];
    clconv = new Clconv(csound, id, size, parts);
    std::vector<float> coefs(size);
    for (int i = 0; i < size; i++)
      coefs[i] = ir[i];
    clconv->push_ir(coefs.data());

    bufout.allocate(csound, parts);
    bufin.allocate(csound, parts << 1);
    cnt = 0;

    csound->plugin_deinit(this);
    return OK;
  }

  int deinit() {
    delete clconv;
    return OK;
  }

  int aperf() {
    AudioSig asig(this, inargs(0));
    AudioSig aout(this, outargs(0));
      
    for (int n = offset; n < 0; n++) {
      bufin[cnt] = (float)asig[n];
      aout[n] = (MYFLT)bufout[cnt];
      if (++cnt == parts) {
        clconv->convolution(bufout.data(), bufin.data());
        cnt = 0;
      }
    }
    return OK;
  }
};

void on_load(Csound *csound) {
  plugin<PConv>(csound, "clconv", "a", "aiiioo", csnd::thread::ia);
}
}
