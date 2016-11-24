#!/bin/sh
#swig -cffi -module lcsound -v -nocwrap -noswig-lisp -generate-typedef -I. -I.. -I${CSOUND_HOME}/H -I${CSOUND_HOME}/include ${CSOUND_HOME}/include/csound.h
swig -DUSE_DOUBLE  -cffi -module lcsound -v -nocwrap -swig-lisp -generate-typedef -outdir ${CSOUND_HOME}/interfaces -I. -I.. -I${CSOUND_HOME}/H -I${CSOUND_HOME}/include ${CSOUND_HOME}/include/csound.h
# Then add to the top of lcsound.lisp:
#(defpackage :csound
#   (:use :common-lisp :cffi))
#(in-package :csound)
#(cffi:define-foreign-library libcsound64
#    (:darwin "libcsound64.dylib")
#    (:unix "libcsound64.so")
#    (:windows "csound64.dll")
#    (t (:default "libcsound64")))
#(cffi:use-foreign-library libcsound64)
#
# and replace incorrect SWIG variadic args (e.g. "(arg4 )" with "&rest".

