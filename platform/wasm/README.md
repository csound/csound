Cross Wasm from Linux
=====================

This directory contains scripts for compiling a Wasm version 
of Csound from Linux.  These scripts were developed using Ubuntu.

## Instructions

1. Install docker using apt:

```bash
sudo apt install docker.io
```

2. Add user to docker group:

```bash
sudo usermod -a -G docker $USER
```

3. Build docker image from csound dir:

```bash
cd csound
docker build -t csound-wasm ./platform/wasm
```

4. Bootstrap vcpkg

```
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-wasm './vcpkg/bootstrap-vcpkg.sh'
```

5. Build csound:

```bash
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-wasm './platform/wasm/build_release.sh'
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-wasm './platform/wasm/build_debug.sh'
```
