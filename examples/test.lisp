;; Rudimentary demonstration of how to use the Csound 5 API from LISP.
;; This file assumes that Common Music and CFFI have been installed
;; for your LISP. You will have to change pathnames appropriately,
;; and change the cffi:load-foreign-library call to suit your installation.
;; Author: Michael Gogins

(load "c:/utah/home/mkg/projects/cm/src/cm.lisp")
(use-system :cffi)
(cffi:load-foreign-library "c:/utah/home/mkg/projects/csound5/_csnd.dll")
(load "c:/utah/home/mkg/projects/csound5/interfaces/csound5.lisp")
(setq cs (csound5::csound-create nil))
(cffi:with-foreign-string (csd "c:/utah/home/mkg/projects/csound5/examples/trapped.csd")
			  (csound5::csound-compile-csd cs csd))
(setq *keep-playing* t result 0)
(loop while (and *keep-playing* (eq result 0)) do
      (setq result (csound5::csound-perform-ksmps-1 cs)))

