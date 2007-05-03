;; Brief example indicating a minimal use of the Csound API from CLisp.
;; This file must be run from the interfaces directory,
;; e.g. "cd interfaces; clisp test.lisp"
;; You may need to edit directories specified below.
(load "d:/utah/opt/cm/src/cm.lisp")
(use-system :cffi)
(load "csound.lisp")
(load "filebuilding.lisp")
(cffi:load-foreign-library "csnd.dll")
(setq cs (csound::csoundCreate nil))
(cffi:with-foreign-string (csd "../examples/trapped.csd")
			  (filebuilding::csoundCompileCsd cs csd))
(setq *keep-playing* t result 0)
(loop while (and *keep-playing* (eq result 0)) do
      (setq result (csound::csoundPerformKsmps cs)))

