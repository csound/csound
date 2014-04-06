Csound Build Instructions
=========================

Build instructions for building Csound from the source packages for the
following operating systems

-   Debian/Ubuntu Linux

-   Mac OS X using Homebrew

-   General Instructions for Linux without Root access

-   Raspberry PI standard OS

-   Fedora 18

Instructions for Windows can be found in their own document at the link below

[Csound Windows Build Doc][1]

[1]: <https://github.com/csound/csound/blob/develop/How_to_Build_Csound_on_Windows.doc>

Instructions compiled by Dominic Melville contact via dcamelville at gmail.com
for amendments and updates



Debian/Ubuntu 
--------------

The following instructions will explain how to configure, compile, and install
Csound 6 on Debian and Ubuntu systems.

### Requirements 

You will need root access, primarily to allow installation of dependencies
required for building Csound, as well as for installing into /usr/local.

### Building Csound 6 

The first thing to do is get a copy of the Csound 6 sources. The following
instructions are written for those getting a copy of the source code from Git.
You will need to adjust the steps accordingly when using the source tarball from
the files section.

The first thing to do is make sure you have all of the required dependencies for
building Csound. In a terminal, do the following:

1.  `sudo apt-get build-dep csound`

2.  `sudo apt-get install cmake`

3.  `sudo apt-get install git `

The following numbered steps are to be done in a terminal for building and
installing Csound:

1.  `cd ~ `

2.  `mkdir csound`

3.  `cd csound`

4.  `git clone https://github.com/csound/csound.git csound`

5.  `mkdir cs6make `

6.  `cd cs6make `

7.  `cmake ../csound`

8.  `make -j6 `

9.  `sudo make install `

10. `sudo ldconfig `

At this point, Csound 6 should now be compiled and installed into the /usr/local
set of folders.



Mac OS X using Homebrew
-----------------------

### Introduction 

Homebrew is a package manager for OSX. It is able to download, build, and
install applications, including their dependencies. The following sections will
describe what you will need to do to use Homebrew to install Csound 6.

Note: At this time, this method of installing Csound is currently being tested.
It is considered beta, and users should be aware that there may be some issues
that will be required to be worked through.

### Requirements 

-   Xcode

-   Xcode Command-Line Tools

-   Homebrew - [http://www.brew.sh][2]

[2]: <http://www.brew.sh>

Installing Homebrew You will first need to have a working Homebrew setup. This
requires installing Xcode and the Xcode Command-Line tools. More information on
installing Homebrew is available on the Homebrew website as well as their wiki.

In particular, you will need to be mindful to enable user read/write for three
directories: "/usr/local", "/Library/Python/2.7/site-packages", and
"/Library/Java/Extensions". These need to be writable by the user as Csound will
need to install packages into each of those folders. Running the following
commands at a Terminal prompt should allow you to do this:

`sudo chmod +a 'user:YOUR_NAME_HERE allow
add_subdirectory,add_file,delete_child,directory_inherit'
/Library/Python/2.7/site-packages `

`sudo chmod +a 'user:YOUR_NAME_HERE allow
add_subdirectory,add_file,delete_child,directory_inherit'
/Library/Java/Extensions `

where YOUR_NAME_HERE refers to your system username.

### Adding the Csound Tap 

Homebrew has a central repository, but it also allows for adding "taps", which
are additional repositories. A Csound tap has been setup at
https://github.com/kunstmusik/homebrew-csound. You can tap into it by using the
following command:

`brew tap kunstmusik/csound`

### Installing Csound 

Once Homebrew is setup and the csound tap has been tapped, run the following
command at the commandline to install Csound:

`brew install --HEAD csound `

### Known Issues 

There is currently a warning issued when Csound installs:

`Warning: Could not fix CsoundLib64.framework/Versions/6.0/CsoundLib64 in
/usr/local/Cellar/csound/HEAD/bin/srconv `

`Warning: Could not fix CsoundLib64.framework/Versions/6.0/CsoundLib64 in
/usr/local/Cellar/csound/HEAD/bin/sndinfo `

`Warning: Could not fix CsoundLib64.framework/Versions/6.0/CsoundLib64 in
/usr/local/Cellar/csound/HEAD/bin/scsort `

`Warning: Could not fix CsoundLib64.framework/Versions/6.0/CsoundLib64 in
/usr/local/Cellar/csound/HEAD/bin/scope `

....

This is due to how the CsoundLib64.framework is installed into
~/Library/Frameworks and bypasses Homebrew's installation path for Frameworks.
This is a known issue and will be looked into.





General Linux without Root access
---------------------------------

These are generic instructions to build on any Linux system with no root access.
These instructions require a full development system (compilers, flex, bison,
cmake). For RT audio you need to make sure you have the alsa headers / lib
installed.

### 1) set up your local directory 

This creates three directories in your HOME directory and adds your local lib
directory to dynamic library path. Note that this last step needs to be
performed every time you open a new terminal, otherwise the installed Csound
will not find its dynamic library dependencies. If you add that line to your
HOME/.profile script, it will be run automatically when you start a new
terminal. $ $

-   `cd  `

-   `mkdir include  `

-   `mkdir lib `

-   `mkdir bin `

-   `mkdir src`

-   `export LD_LIBRARY_PATH=HOME/lib:LD_LIBRARY_PATH `

(this last command can be added to your HOME/.profile file for convenience)

### 2) get and install libsndfile 

Libsndfile is the only required dependency for a basic Csound build.

Ubuntu users: it appears the libsndfile binaries come as default in your
installation, so this step might not be required. However, there is no harm in
doing it.

-   `cd src `

-   `wget http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.25.tar.gz `

-   `tar xf libsndfile-1.0.25.tar.gz`

-   `cd libsndfile-1.0.25 `

-   `./configure --prefix=HOME`

-   `make install `

### 3) get Csound (latest from git) 

The latest Csound sources are kept in the develop branch. For the latest
released sources, you do not need to change branches.

-   `cd ..`

-   `git clone https://github.com/csound/csound.git csound `

-   `cd csound`

-   `git checkout develop`

-   `mv Custom.cmake.ex Custom.cmake `

### 4) set the include path for the build 

Cmake needs to find your locally-installed headers in HOME/bin. You can add
custom commands to Cmake by using a Custom.cmake file, which Cmake will read if
it exists.

Open or create the Custom.cmake file in the top level Csound sources directory
and add the following line: `include_directories("ENV{HOME}/include") `

### 5) build and install Csound 

The recommended method is to create a separate build directory and run cmake
there.

-   `mkdir build`

-   `cd build `

-   `cmake -DCMAKE_INSTALL_PREFIX=HOME ..`

-   `make install `

### Notes 

This builds a basic system. If you add dependencies to your HOME directories,
then you can run make again to build them. The csound command-line frontend will
be installed in HOME/bin, the libraries in HOME/lib and the include files in
HOME/include. The plugin dir is in \$HOME/lib/csound/plugins64-6.0.

### Dependencies List

If the dependency you are adding uses ./configure, you can the same parameters to it as explained in step 2. If it uses cmake, you can use the same parameters as in step 5. After adding dependencies to your $HOME directories, you can run cmake again to re-build Csound. Check the printed output to see if the added dependency has switched on the build of the desired component.

#### OSC opcodes

liblo - http://liblo.sourceforge.net/ NB: the build for version 0.28 seems to be broken.

#### Fluid opcodes

Fluidsynth - http://sourceforge.net/apps/trac/fluidsynth/ NB: cmake might need to be coerced into finding the fluidsynth headers once it is built. For that, you can use the following cmake command (see step 5):

cmake -DCMAKE_INSTALL_PREFIX=$HOME -DFLUIDSYNTH_H=$HOME/include ..

#### Widget opcodes

FLTK - http://www.fltk.org/index.php NB: make sure you configure the FLTK build with --enable-shared, otherwise there could be problems linking to libfltk on 64bit linux.

#### Faust opcodes

libfaust - use faust2 branch of Faust git sources:

$ git clone git://git.code.sf.net/p/faudiostream/code faust
$ cd faust
$ git checkout faust2

NB: libfaust also requires LLVM 3.0, 3.1, 3.2, 3.3 or 3.4 - http://llvm.org/ LLVM can be built with CMake (as in step 5 above). To build faust, use the following make command (replacing LLVM_32 for LLVM_3* depending on the version you are using, if it is not 3.2)

$ make LLVM_VERSION=LLVM_32 LLVM_CONFIG=llvm-config LLVM_CLANG=g++ CXX=g++ ARCHFLAGS=-fPIC

To install it, you should run

$make PREFIX=$HOME

To switch the faust opcodes build on and coerce cmake into finding the faust library use:

cmake -DCMAKE_INSTALL_PREFIX=$HOME -DBUILD_FAUST_OPCODES=1 -DFAUST_LIBRARY=$HOME/lib/faust/libfaust.a ..

#### Portaudio module

portaudio - http://www.portaudio.com/

#### Portmidi module

portmidi - http://portmedia.sourceforge.net/portmidi/

#### JACK module

Jack connection kit - http://jackaudio.org/

#### Python bindings

swig - http://www.swig.org/ Python headers / library - http://www.python.org

#### Java bindings

swig - http://www.swig.org/ Java SDK - http://www.oracle.com/technetwork/java/javase/downloads/index.html


Raspberry Pi Standard Distro 
-----------------------------

### (Raspian Wheezy)



Preliminary step:

`sudo apt-get build-dep csound `

If you get the following error:

`E: You must put some 'source' URIs in your sources.list `

then add the following line to /etc/apt/sources.list:

`deb-src http://mirrordirector.raspbian.org/raspbian/ wheezy main contrib
non-free rpi `

(This can be done with nano)

1.  `cd ~ `

2.  `mkdir csound`

3.  `cd csound`

4.  `git clone https://github.com/csound/csound.git csound`

5.  `mkdir cs6make`

6.  `cd cs6make`

7.  `cmake ../csound -DBUILD_CSOUND_AC=OFF` (this will not build CsoundAC, that
    gives errors)

8.  `make -j6`

9.  `sudo make install`

10. `sudo ldconfig `

If you want to use the csnd6 Python library, add the following line to .bashrc:

`export PYTHONPATH=/usr/local/lib`



Fedora 18
---------

### Introduction 

This shows how to download the source code from Sourceforge, and build Csound 6
from sources.

### Requirements 

For some steps, you'll need root access.  You may need to install additional
packages - the example shows installing a number of additional packages, but the
exact requirements depend on your system.

You'll need to make sure Cmake, flex, bison, and libsndfile are installed. If
you're logged in as a non-root user, you can execute root commands using su -c.
So here's how to make sure the basic required packages for building are
installed:

`su -c "yum install cmake libsndfile libsndfile-devel flex bison" `

You'll be prompted for your root password.

### Downloading 

Download the latest Csound 6 sources from

[https://sourceforge.net/projects/csound/files/csound6/ ][3]

[3]: <https://sourceforge.net/projects/csound/files/csound6/ >

At the time this was written, the downloaded file was Csound6.00.1.tar.gz

### Compiling 

First uncompress and untar the source code:

`gunzip Csound6.00.1.tar.gz `

`tar xf Csound6.00.1.tar.gz `

Change into the source directory

`cd Csound6.00.1`

In the source directory what gets compiled is controlled by the file
CMakeLists.txt. By default lots of stuff will get built, as long as you have the
required dependencies installed.

The following commands will add most required packages (but note that lua
interfaces and faust opcodes may still not work):

`su - `

`yum install ladspa ladspa-devel `

`yum install fluidsynth fluidsynth-devel `

`yum install boost boost-devel java-devel `

`yum install jack-audio-connection-kit-devel `

`yum install fltk fltk-devel `

`yum install swig swig-devel `

`yum install pulseaudio-libs-devel `

`yum install portaudio portmidi portaudio-devel portmidi-devel `

`yum install fltk-fluid `

`yum install stk stk-devel `

`yum install python-libs `

`yum install python-devel `

`yum install liblo liblo-devel `

`yum install lua lua-devel `

`yum install eigen3-devel eigen3 `

`yum install gmm-devel `

`yum install wiiuse wiiuse-devel `

`yum install bluez-libs-devel `

yum install llvm-devel

`yum install faust faust-tools`

`exit `

The building process takes two steps

1.  use cmake to create a Makefile

2.  use make to build Csound 6

To create the Makefile:

`cmake ./ `

At this point you may see messages saying that something can't be built - in
that case use your skill and judgement to work out which packages are missing.
For instance if you get a message saying fltk cannot be built, you could install
the fltk-devel package.

If you install a new package, you'll need to run cmake again:

`rm CMakeCache.txt `

`cmake ./ `

When you're happy, run make

`make `

This builds the csound library.

### Installing 

By default, Csound 6 will install in /usr/local, so you'll need root
permissions:

`su -c "make install" `

Finally you need to make sure the library can be found by the linker

`su - `

`cd /etc/ld.so.conf.d `

`touch csound6.conf `

`gedit csound6.conf `

In the editor, add one line

`/usr/local/lib `

and close the file.

Finally update the linker search path

`ldconfig `

and log out of root

`exit `

Testing As a basic test, just try typing csound at a command prompt, and you
should get the help message.


