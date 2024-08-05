Cross compiling iOS from Linux
==============================

This directory contains scripts for compiling a iOS version 
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

3. Download XCode with the iOS SDK (you must be logged into an Apple ID to download Xcode).

4. Build docker image from csound dir:

```bash
cd csound
docker build -t csound-ioscross ./platform/ioscross
```

5. Build csound:

```bash
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-ioscross './platform/ioscross/build_release.sh'
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-ioscross './platform/ioscross/build_debug.sh'
```

## Build Github tar.gz

1. To build image used for github build using docker:

```bash
docker build -t github-ioscross --build-arg="BASE_DIR=/home/runner/work/csound/csound" ./platform/ioscross
```

2. Create ioscross.tar.gz:

```bash
docker run github-ioscross cat /home/runner/work/csound/csound/ioscross.tar.gz > ioscross.tar.gz
```
