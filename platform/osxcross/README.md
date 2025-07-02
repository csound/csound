Cross compiling MacOS from Linux
================================

This directory contains scripts for compiling a MacOS version 
of Csound from Linux.  These scripts were developed using Ubuntu.

**[Please ensure you have read and understood the Xcode license
terms first before using it.](https://www.apple.com/legal/sla/docs/xcode.pdf)**

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
docker build -t csound-osxcross ./platform/osxcross
```

4. Build csound:

```bash
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-osxcross './platform/osxcross/build_release.sh'
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-osxcross './platform/osxcross/build_debug.sh'
```

## Build Github tar.gz

1. To build image used for github build using docker:

```bash
docker build -t github-osxcross --build-arg="BASE_DIR=/home/runner/work/csound/csound" ./platform/osxcross
```

2. Create osxcross.tar.gz:

```bash
docker run github-osxcross cat /home/runner/work/csound/csound/osxcross.tar.gz > osxcross.tar.gz
```
