Cross compiling Android from Linux
==================================

This directory contains scripts for compiling a Android version 
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
docker build -t csound-android ./platform/android
```

4. Build csound:

```bash
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-android './platform/android/build_release.sh'
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-android './platform/android/build_debug.sh'
```
