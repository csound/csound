#!/bin/bash

apt update -y
apt install -y cmake
apt install -y wget
apt install -y build-essential
apt install -y git
apt install -y llvm
apt install -y vim
apt install -y curl zip unzip tar
apt install -y pkg-config
apt install -y autoconf libtool
apt install -y flex bison
apt install -y python3
apt install -y python3-pip
apt install -y nodejs
pip3 install scons

git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install 3.1.64
./emsdk activate 3.1.64
source ./emsdk_env.sh
