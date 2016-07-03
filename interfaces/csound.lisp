(defpackage :csound
   (:use :common-lisp :cffi))
(in-package :csound)
(cffi:define-foreign-library libcsound64
    (:darwin "libcsound64.dylib")
    (:unix "libcsound64.so")
    (:windows "csound64.dll")
    (t (:default "libcsound64")))
(cffi:use-foreign-library libcsound64)
(cffi:defcfun "csoundCreate" :pointer 
    (host-data :pointer))
(cffi:defcfun ("csoundCompileCsdText" csoundCompileCsdText) :int64
    (cs :pointer)
    (csdtext :string))
(cffi:defcfun ("csoundStart" csoundStart) :int64
    (cs :pointer))
(cffi:defcfun ("csoundPerformKsmps" csoundPerformKsmps) :int64
    (cs :pointer))
