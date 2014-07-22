#!/bin/sh
# use -Xptxas="-v" to check register usage and --maxrregcount 32 to limit it
echo "building cuda opcodes ..."
nvcc -O3 -shared -o libcudaop1.dylib adsyn.cu -use_fast_math -I../../debug/CsoundLib64.framework/Headers -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
nvcc -O3 -shared -o libcudaop2.dylib pvsops.cu    -g -I../../debug/CsoundLib64.framework/Headers -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
nvcc -O3 -shared -o libcudaop3.dylib slidingm.cu  -I../../debug/CsoundLib64.framework/Headers -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
nvcc -O3 -shared -o libcudaop4.dylib conv.cu   -I../../debug/CsoundLib64.framework/Headers -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
nvcc -O3 -shared -o libcudaop5.dylib pconv.cu -I../../debug/CsoundLib64.framework/Headers -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
echo "...done"
