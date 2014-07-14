#!/bin/sh
echo "building cuda opcodes ..."
nvcc -O3 -shared -o libcudaop1.dylib adsyn.cu -I../../include -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
nvcc -O3 -shared -o libcudaop2.dylib pvsops.cu -g -I../../include -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
nvcc -O3 -shared -o libcudaop3.dylib slidingm.cu -I../../include -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
nvcc -O3 -shared -o libcudaop4.dylib conv.cu -I../../include -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
nvcc -O3 -shared -o libcudaop5.dylib pconv.cu -I../../include -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
echo "...done"
