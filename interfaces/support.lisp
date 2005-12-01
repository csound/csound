; utility routines for users of Verrazano-generated packages

(defpackage "VERRAZANO-SUPPORT"
  (:nicknames "VZN")
  (:use "COMMON-LISP" "CFFI")
  (:export "VTABLE-LOOKUP"
	   "VIRTUAL-FUNCALL"))

(in-package "VERRAZANO-SUPPORT")

; lookup the pointer to a given function
(defun vtable-lookup (pobj indx coff)
  (let ((vptr (cffi:mem-ref pobj :pointer coff)))
    (cffi:mem-aref vptr :pointer (- indx 2))))

; macro for emitting a virtual function call
(defmacro virtual-funcall (pobj indx coff &rest body)
  `(foreign-funcall (vtable-lookup ,pobj ,indx ,coff) ,@body))

