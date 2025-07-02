FROM ubuntu:22.04

RUN apt update -y
RUN apt install -y \
    cmake \
    wget \
    build-essential \
    git \
    llvm \
    vim \
    curl zip unzip tar \
    pkg-config \
    autoconf libtool \
    flex bison \
    python3 \
    python3-pip \
    nodejs \
    ninja-build

RUN pip3 install scons

RUN git clone --depth 1 https://github.com/emscripten-core/emsdk.git
RUN /emsdk/emsdk install 3.1.64
RUN /emsdk/emsdk activate 3.1.64

RUN mkdir /home/csound
RUN chmod 777 /home/csound

ENV HOME /home/csound
ENV VCPKG_FORCE_SYSTEM_BINARIES 1
