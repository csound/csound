gcc -dynamiclib -o libclops.dylib -DUSE_DOUBLE -D_FORTIFY_SOURCE=0 cladsynth.c -I../../include -framework OpenCL
