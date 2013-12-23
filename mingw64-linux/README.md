Cross-compiling Windows 64-bit (x86\_64) from Linux
==================================================

This directory contains scripts for cross-compiling a Windows 64-bit version 
of Csound from Linux.  These scripts were developed using Debian.

## Instructions

1. Install mingw-64 package using aptitude:

    sudo aptitude install mingw-w64

2. Create a directory called mingw64 in your home directory. This is where the 
dependencies will be built and installed to::

    mkdir ~/mingw64

3. Run the download-deps.sh script.  This script will download source and
binary versions of dependencies that Csound will need. A folder called cache
will be created and files will be downloaded to there.  A second folder, build,
will hold the unarchived versions of the dependencies.

4. Run the build-deps.sh script.  This will be the dependencies and install 
them into ~/mingw64.

5. Run the build.sh script.  This will run Cmake and then compile Csound.

## Notes

Currently not all dependencies have been worked through for building.  See the
build-deps.sh script and see where "exit" is put in.  Anything after exit has 
not yet been updated for building.

* wiiuse - I tried v0.12 binary release of wiiuse from sf.net/projects/wiiuse, 
but found it was compiled for win32 and not w64.  Not including as of now.
