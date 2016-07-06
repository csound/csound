# Csound with Lisp

Michael Gogins

This directory contains some examples and this documentation for using Csound with several bodies of algorithmic composition 
code written in [Common Lisp](https://common-lisp.net/) or [Scheme](http://www.schemers.org/).

The Lisp world has produced some notable software for algorthmic composition, including Common Music, its offspring Grace, and Open Music. Some illustrious composers have used Open Music, which also receives contributions from contemporary researchers in mathematical music theory. In addition, some composers, for example Drew Krause, have themselves produced useful libraries of Lisp code.

The objective of this repository and documentation is to simplify the process of integrating Csound tightly with these environments, in such a way that upon generating a piece in Lisp (or Scheme), you will at once hear a finished rendering of that piece by Csound, often in real time. You will be able, if you wish, to embed all of your Csound csd file, or orc and sco files, directly in your Lisp (or Scheme) code, thus simplifying the maintenance and distribution of your work.

## Common Lisp

To achieve this integration of Csound with Common Lisp you may follow these steps:

1. Install Csound.
2. Install Lisp.
3. Install Lisp's Common Foreign Function Interface.
4. Test the basic Lisp examples from Csound.
5. Install your Lisp composition software of choice.
6. Configure your Lisp environment to load all required packages that you can simply write your pieces.
7. Test your composition environment with some of the examples here.

### The Csound Lisp API

The [interfaces/csound.lisp](http://github.com/csound/csound/blob/develop/interfaces/csound.lisp) file defines a Lisp [cffi](https://common-lisp.net/project/cffi/) (Common Foreign Function Interface) wrapper for many of the most useful functions in the Csound API defined in [include/csound.h](https://github.com/csound/csound/blob/develop/include/csound.h), and documented [here](http://csound.github.io/docs/api/index.html).

To use this wrapper, you must install an implementation Lisp (I recommend [sbcl](http://www.sbcl.org/) (Steel Bank Common Lisp)), then install and load [quicklisp](https://www.quicklisp.org/beta/), then use quicklisp to install and load the cffi package as documented [here](https://www.quicklisp.org/beta/#installation); simply substitute `cffi` for `vecto` in the step-by step installation example.
The examples have been tested with [sbcl](http://www.sbcl.org/) (Steel Bank Common Lisp) on Ubuntu 14.04.

## Scheme

The Scheme examples have been tested in Grace.
