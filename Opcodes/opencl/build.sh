gcc -O3 -dynamiclib -o libclop1.dylib -DUSE_DOUBLE -D_FORTIFY_SOURCE=0 cladsynth.c -I../../include -framework OpenCL
