Cross compiling MacOS from Linux
================================

This directory contains scripts for compiling a MacOS version 
of Csound from Linux.  These scripts were developed using Ubuntu.

**[Please ensure you have read and understood the Xcode license
terms first before using it.](https://www.apple.com/legal/sla/docs/xcode.pdf)**

## Instructions

1. Install docker using apt:

    sudo apt install docker.io

2. Add user to docker group:

    sudo usermod -a -G docker $USER

3. Build docker image from csound dir:

    cd csound
    docker build -t csound-osxcross ./platform/osxcross

4. Build csound:

    docker run -it --rm -v .:/tmp/workdir --user ${UID}:${1000} -w /tmp/workdir csound-osxcross './platform/osxcross/build_release.sh'
    docker run -it --rm -v .:/tmp/workdir --user ${UID}:${1000} -w /tmp/workdir csound-osxcross './platform/osxcross/build_debug.sh'
