#!/bin/sh
swig -cffi -module csound -v -extern-all -nocwrap -generate-typedef -ID:/utah/opt/swigwin-1.3.29/lib/cffi -I. -I.. -ID:/utah/home/mkg/projects/csoundd/H D:/utah/home/mkg/projects/csoundd/include/csound.h
# Then add to the top of csound.lisp:
#(defpackage :csound
#   (:use :common-lisp :cffi))
#
#(in-package :csound)
#
# and replace incorrect SWIG variadic args (e.g. "(arg4 )" with "&rest".
swig -cffi -module filebuilding -v -extern-all -nocwrap -generate-typedef -ID:/utah/opt/swigwin-1.3.29/lib/cffi -I. -I.. -ID:/utah/home/mkg/projects/csoundd/H D:/utah/home/mkg/projects/csoundd/interfaces/filebuilding.h
# Then add to the top of filebuilding.lisp:
#(defpackage :filebuilding
#   (:use :common-lisp :cffi))
#
#(in-package :filebuilding)
