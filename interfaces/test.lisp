;; Brief example indicating a minimal use of the Csound API from CLisp.
;; This file must be run from the csound5 directory,
;; not the csound5/examples directory, like this:
;; "clisp examples/test.lisp"
(load "d:/utah/opt/cm/src/cm.lisp")
(use-system :cffi)
(load "csound.lisp")
(load "filebuilding.lisp")
(cffi:load-foreign-library "d:/utah/home/mkg/projects/csoundd/_csnd.dll")
(setq cs (csound::csoundCreate nil))
(cffi:with-foreign-string (csd "d:/utah/home/mkg/projects/csoundd/examples/trapped.csd")
			  (filebuilding::csoundCompileCsd cs csd))
(setq *keep-playing* t result 0)
(loop while (and *keep-playing* (eq result 0)) do
      (setq result (csound::csoundPerformKsmps cs)))

