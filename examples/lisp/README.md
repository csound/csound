# Csound with Lisp

Michael Gogins

This directory contains some examples and this documentation for using Csound with several bodies of algorithmic composition code written in [Common Lisp](https://common-lisp.net/) or [Scheme](http://www.schemers.org/).

The objective of this repository and documentation is to simplify the process of integrating Csound tightly with these environments, in such a way that upon generating a piece in Lisp (or Scheme), you will at once hear a finished rendering of that piece by Csound, often in real time. You will be able, if you wish, to embed all of your Csound csd file, or orc and sco files, directly in your Lisp (or Scheme) code, thus simplifying the maintenance and distribution of your work.

## Common Lisp

To integrate Csound with Common Lisp you may follow these steps. Installation and configuration are more straightforward on Linux, but certainly possible on OS X and Windows.

1. Install Csound. I recommend installing pre-built binaries for 64 bit CPU architecture from [GitHub](http://csound.github.io/download.html). Of course you can always build Csound from sources, this is fairly straightforward on Linux but not for beginners on Windows or OS X.
2. Install Lisp.  I recommend [sbcl](http://www.sbcl.org/) (Steel Bank Common Lisp).
3. Install Lisp's Common Foreign Function Interface (cffi).  
 1. Install and load [quicklisp](https://www.quicklisp.org/beta/).
 2. Use quicklisp to install and load the cffi package as documented [here](https://www.quicklisp.org/beta/#installation); simply substitute `cffi` for `vecto` in the step-by step installation example.
 3. Install Csound's cffi wrapper. The [interfaces/csound.lisp](http://github.com/csound/csound/blob/develop/interfaces/csound.lisp) file defines a Lisp [cffi](https://common-lisp.net/project/cffi/) (Common Foreign Function Interface) wrapper for many of the most useful functions in the Csound API defined in [include/csound.h](https://github.com/csound/csound/blob/develop/include/csound.h), and documented [here](http://csound.github.io/docs/api/index.html). If you do not find interfaces/csound.lisp in your installation of Csound, download it directly from GitHub [here](http://github.com/csound/csound/blob/develop/interfaces/csound.lisp).
4. Test the basic Lisp examples from Csound. On Linux if you have built Csound for sources and run Csound from your build environment, the commands are as follows. If you are running pre-built Csound you may need to change some pathnames. In any event these examples are completely self-contained and should play the example piece "Xanadu" with real-time audio output.
 1. Render "Xanadu" using raw cffi calls (no Lisp wrapper code): `mkg@Sun-Yukong:~/csound/csound$ sbcl --load examples/lisp/test.lisp`.
 2. Render "Xanadu" using Csound's cffi wrapper: `sbcl --load examples/lisp/test-wrapper.lisp`.
5. Install your Lisp composition software of choice. The Lisp world has produced some notable software for algorthmic composition, including:
 1. [Common Music](http://commonmusic.sourceforge.net/).
 2. Common Music's offspring [Grace] (now written in Scheme).
 3. [OpenMusic](http://repmus.ircam.fr/openmusic/home). Some illustrious composers have used OpenMusic, which also receives contributions from contemporary researchers in mathematical music theory. You may find additional OpenMusic libraries [here](http://forumnet.ircam.fr/product/openmusic-libraries-en/) and [here](http://repmus.ircam.fr/openmusic/libraries).
 4. In addition, some composers, for example [Drew Krause], have themselves made available useful libraries of Lisp code.
6. Configure your Lisp environment to load all required packages so that you can simply write your pieces.
7. Test your composition environment with some of the examples here.

## Scheme

The Scheme examples have been tested in Grace.
