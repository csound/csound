;; Brief example indicating a minimal use of the Csound API from CLisp.
;; This file must be run from the csound5 directory,
;; not the csound5/examples directory, like this:
;; "clisp examples/test.lisp"

(load "csound.lisp")
(setq cs (csound::csoundCreate nil))
(csound::csoundCompileCsd cs "trapped.csd")
(setq *keep-playing* t result 0)
(loop while (and *keep-playing* (eq result 0)) do
      (setq result (csound::csoundPerformKsmps cs)))

