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

4. The dockerfile reads an encrypted version of Xcode from a server to avoid re-distribution of Xcode.
To encrypt Xcode the following command is used:

```
read -s PASSWORD
cat Xcode_15.1.xip | openssl aes-256-cbc -pbkdf2 -a -salt -pass pass:$PASSWORD > Xcode_15.1.xip.enc
```

5. In the folder containing the encrypted Xcode file start an HTTP server to host the file:

```
python -m http.server 8000
```

6. Build docker image from csound dir:

```bash
cd csound
echo $PASSWORD > secret
docker build --secret id=secret,src=secret -t csound-ioscross ./platform/ioscross
```

7. Build csound:

```bash
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-ioscross './platform/ioscross/build_release.sh'
docker run -it --rm -v $(pwd):$(pwd) --user ${UID}:${1000} -w $(pwd) csound-ioscross './platform/ioscross/build_debug.sh'
```
