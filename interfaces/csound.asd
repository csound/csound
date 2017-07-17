;;; 
;;; ASDF 3.1 compliant system definition file for Csound.
;;;
;;; Michael Gogins
;;; 11 July 2016
;;;
(require :asdf)
(asdf::defsystem "csound"
  :description "CFFI interface to the Csound C API."
  :long-description "CFFI interface to the Csound C API."
  :version "1.0"
  :author "Michael Gogins <michael.gogins@gmail.com>"
  :licence "LLGPL"
  :serial t ;; the dependencies are linear.
  :depends-on (:cffi)
  :components  
  ((:file "csound")))

