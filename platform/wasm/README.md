Cross Wasm from Linux
=====================

This directory contains scripts for compiling a Wasm version 
of Csound from Linux.  These scripts were developed using Ubuntu.

## Instructions

1. Install docker using apt:

    sudo apt install docker.io

2. Add user to docker group:

    sudo usermod -a -G docker $USER

3. Build docker image from csound dir:

    cd csound
    docker build -t csound-wasm ./platform/wasm

4. Build csound:

    docker run -it --rm -v .:/tmp/workdir --user ${UID}:${1000} -w /tmp/workdir csound-wasm './platform/wasm/build_release.sh'
    docker run -it --rm -v .:/tmp/workdir --user ${UID}:${1000} -w /tmp/workdir csound-wasm './platform/wasm/build_debug.sh'
