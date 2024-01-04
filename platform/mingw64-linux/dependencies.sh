#!/bin/bash

apt update -y
apt install -y mingw-w64-x86-64-dev
apt install -y autoconf libtool
apt install -y bash-completion
apt install -y build-essential
apt install -y gcc-mingw-w64
apt install -y g++-mingw-w64
apt install -y git
apt install -y mingw
apt install -y mingw-w64-tools
apt install -y vim
apt install -y cmake
apt install -y curl
apt install -y zip
apt install -y flex bison
apt install -y wine

update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

# Install PowerShell
apt install -y wget apt-transport-https software-properties-common
source /etc/os-release
wget -q https://packages.microsoft.com/config/ubuntu/$VERSION_ID/packages-microsoft-prod.deb
dpkg -i packages-microsoft-prod.deb
rm packages-microsoft-prod.deb
apt update

apt install -y powershell

mkdir /.wine
chown 1000:1000 /.wine
