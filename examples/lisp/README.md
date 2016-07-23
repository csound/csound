# Csound with Lisp

Michael Gogins

## Introduction

This directory contains documentation and examples for using Csound with [Common Lisp](https://common-lisp.net/) and the Lisp release of the [Common Music](http://commonmusic.sourceforge.net/) algorithmic composition system. The examples demonstrate how to integrate Csound with Common Lisp and Common Music in such a way that the Lisp code embeds all required Csound code, and directly calls the Csound API using a Lisp Foreign Function Interface (FFI). In this way, it is possible to compose in Lisp and yet immediately hear a finished rendering of your piece, often in real time. The documentation and examples use Csound and Steel Bank Common Lisp compiled for 64 bit CPU architecture.

In addition, some composers, for example [Drew Krause](http://www.drew-krause.com/), have themselves made available useful libraries of Lisp code (currently hosted by me at https://github.com/gogins/gogins.github.io).

There are other notable Lisp-based computer music systems such as [OpenMusic](http://repmus.ircam.fr/openmusic/home) and Grace, a newer version of Common Music that is written in Scheme, but these are not discussed here.

## Common Lisp

To integrate Csound with Common Lisp, follow these steps. Installation and configuration are more straightforward on Linux, but certainly possible on OS X and Windows.

1. Install Csound. I recommend installing pre-built binaries for 64 bit CPU architecture from [GitHub](http://csound.github.io/download.html). Of course you can always build Csound from sources, this is fairly straightforward on Linux but not for beginners on Windows or OS X.
2. Install Lisp.  I recommend [sbcl](http://www.sbcl.org/) (Steel Bank Common Lisp).
3. Install Lisp's Common Foreign Function Interface, [cffi](https://common-lisp.net/project/cffi/).  
 1. Install and load [quicklisp](https://www.quicklisp.org/beta/).
 2. Use quicklisp to install and load the cffi package as documented [here](https://www.quicklisp.org/beta/#installation); simply substitute `cffi` for `vecto` in the step-by step installation example.
 3. Install Csound's cffi wrapper. The [interfaces/csound.lisp](http://github.com/csound/csound/blob/develop/interfaces/csound.lisp) file defines a Lisp `cffi` wrapper for many of the most useful functions in the Csound API defined in [include/csound.h](https://github.com/csound/csound/blob/develop/include/csound.h), and documented [here](http://csound.github.io/docs/api/index.html). If you do not find `interfaces/csound.asd` and `interfaces/csound.lisp` in your installation of Csound, download them directly from GitHub [here](http://github.com/csound/csound/blob/develop/interfaces/).
5. Download the Lisp version of Common Music from the SourceForge repository as [this branch](https://sourceforge.net/p/commonmusic/code/HEAD/tree/branches/cm2/), e.g. using Subversion `svn checkout svn://svn.code.sf.net/p/commonmusic/code/branches/cm2`. The Common Music license is compatible with Csound's license if you want to play with source code or link with Csound binaries. Please note that the official documentation for installing and running Common Music do not work with sbcl at this time. Instead, perform the Subversion checkout mentioned above, and then configure your Lisp installation to load Common Music as discussed below.
4.  To use Drew's code:
  1. Install [`clocc`](http://clocc.sourceforge.net/) using Mercurial: `hg clone http://hg.code.sf.net/p/clocc/hg clocc-hg`. 
  2. Install `rsm-mod` with `sudo apt-get install cl-rsm-mod`.
6. Configure your Lisp environment to load all required packages so that you can simply write your pieces. There are _way_ too many ways of doing this, but the easy beginner way is simply to edit your user initialization file for your Lisp implementation to preload everything that you need for composing. For example on my Linux system I have the following in `$HOME/.sbclrc`:
 ```
;;; The following lines added by ql:add-to-init-file:
#-quicklisp
(let ((quicklisp-init (merge-pathnames "quicklisp/setup.lisp"
                                       (user-homedir-pathname))))
  (when (probe-file quicklisp-init)
    (load quicklisp-init)))

(require 'asdf)    

;;; Load the Common Foreign Function Interface.
(ql:quickload "cffi")

;; Load the cl-heredoc library, which is used for 
;; embedding arbitrary Csound code including quotes and escapes
;; into Lisp code.
(ql:quickload "cl-heredoc")

;;; Load Common Music.
(push "/home/mkg/cm2/" asdf:*central-registry*)
(asdf:load-system :cm2)

;;; Load Csound's ffi wrappers.
(push "/home/mkg/csound/csound/interfaces/" asdf:*central-registry*)
(asdf:load-system :csound)
(asdf:load-system :sb-csound)

;;; Load Drew Krause's code.
(load "/home/mkg/nudruz/nudruz.lisp")
```

It is important that all shared libraries be loadable from their filename alone; this is possible if the directories containing the Csound executable, shared library, and plugin opcodes are in the PATH and LD_LIBRARY_PATH environment variables.

4. Test the basic Lisp examples from Csound. The following examples use Steel Bank Common Lisp's native FFI facility `sb-alien`. On Linux if you have built Csound for sources and run Csound from your build environment, the commands are as follows. If you are running pre-built Csound you may need to change some pathnames. In any event these examples are completely self-contained, and should play the example piece "Xanadu" with real-time audio output.
 1. Render "Xanadu" using raw cffi calls (no Lisp wrapper code): `mkg@Sun-Yukong:~/csound/csound$ sbcl --load examples/lisp/sb-test.lisp`.
 2. Render "Xanadu" using Csound's cffi wrapper: `mkg@Sun-Yukong:~/csound/csound$ sbcl --load examples/lisp/sb-wrapper-test.lisp`.
 3. Render the Csound-enabled version of the Common Music tutorial piece "Scales": `mkg@Sun-Yukong:~/csound/csound/examples/lisp$ sbcl --load scales-csound.lisp`.
 

