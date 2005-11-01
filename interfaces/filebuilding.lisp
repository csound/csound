;; This is an automatically generated file. 
;;Make changes as you feel are necessary (but remember if you try to regenerate this file, your changes will be lost). 

(defpackage :filebuilding
  (:use :common-lisp :ffi)
  (:export
	:csoundCsdCreate
	:csoundCsdSetOptions
	:csoundCsdGetOptions
	:csoundCsdSetOrchestra
	:csoundCsdGetOrchestra
	:csoundCsdAddScoreLine
	:csoundCsdAddEvent11
	:csoundCsdAddEvent10
	:csoundCsdAddEvent9
	:csoundCsdAddEvent8
	:csoundCsdAddEvent7
	:csoundCsdAddEvent6
	:csoundCsdAddEvent5
	:csoundCsdAddEvent4
	:csoundCsdAddEvent3
	:csoundCsdSave
	:csoundCsdCompile
	:csoundCsdPerform
	:csoundCompileCsd
	:csoundPerformCsd))

(in-package :filebuilding)

(default-foreign-language :stdc)


;; Pretend these structures are ints, since they are only passed by pointer.

(ffi:def-c-type CSOUND int)

(ffi:def-call-out csoundCsdCreate

(:name "csoundCsdCreate")

(:arguments (csound (ffi:c-pointer CSOUND)))

(:library "_csnd"))



(ffi:def-call-out csoundCsdSetOptions

(:name "csoundCsdSetOptions")

(:arguments (csound (ffi:c-pointer CSOUND))

	(options ffi:c-string))

(:library "_csnd"))



(ffi:def-call-out csoundCsdGetOptions

(:name "csoundCsdGetOptions")

(:arguments (csound (ffi:c-pointer CSOUND)))

(:return-type ffi:c-string)

(:library "_csnd"))



(ffi:def-call-out csoundCsdSetOrchestra

(:name "csoundCsdSetOrchestra")

(:arguments (csound (ffi:c-pointer CSOUND))

	(orchestra ffi:c-string))

(:library "_csnd"))



(ffi:def-call-out csoundCsdGetOrchestra

(:name "csoundCsdGetOrchestra")

(:arguments (csound (ffi:c-pointer CSOUND)))

(:return-type ffi:c-string)

(:library "_csnd"))



(ffi:def-call-out csoundCsdAddScoreLine

(:name "csoundCsdAddScoreLine")

(:arguments (csound (ffi:c-pointer CSOUND))

	(line ffi:c-string))

(:library "_csnd"))



(ffi:def-call-out csoundCsdAddEvent11

(:name "csoundCsdAddEvent11")

(:arguments (csound (ffi:c-pointer CSOUND))

	(p1 DOUBLE-FLOAT)

	(p2 DOUBLE-FLOAT)

	(p3 DOUBLE-FLOAT)

	(p4 DOUBLE-FLOAT)

	(p5 DOUBLE-FLOAT)

	(p6 DOUBLE-FLOAT)

	(p7 DOUBLE-FLOAT)

	(p8 DOUBLE-FLOAT)

	(p9 DOUBLE-FLOAT)

	(p10 DOUBLE-FLOAT)

	(p11 DOUBLE-FLOAT))

(:library "_csnd"))



(ffi:def-call-out csoundCsdAddEvent10

(:name "csoundCsdAddEvent10")

(:arguments (csound (ffi:c-pointer CSOUND))

	(p1 DOUBLE-FLOAT)

	(p2 DOUBLE-FLOAT)

	(p3 DOUBLE-FLOAT)

	(p4 DOUBLE-FLOAT)

	(p5 DOUBLE-FLOAT)

	(p6 DOUBLE-FLOAT)

	(p7 DOUBLE-FLOAT)

	(p8 DOUBLE-FLOAT)

	(p9 DOUBLE-FLOAT)

	(p10 DOUBLE-FLOAT))

(:library "_csnd"))



(ffi:def-call-out csoundCsdAddEvent9

(:name "csoundCsdAddEvent9")

(:arguments (csound (ffi:c-pointer CSOUND))

	(p1 DOUBLE-FLOAT)

	(p2 DOUBLE-FLOAT)

	(p3 DOUBLE-FLOAT)

	(p4 DOUBLE-FLOAT)

	(p5 DOUBLE-FLOAT)

	(p6 DOUBLE-FLOAT)

	(p7 DOUBLE-FLOAT)

	(p8 DOUBLE-FLOAT)

	(p9 DOUBLE-FLOAT))

(:library "_csnd"))



(ffi:def-call-out csoundCsdAddEvent8

(:name "csoundCsdAddEvent8")

(:arguments (csound (ffi:c-pointer CSOUND))

	(p1 DOUBLE-FLOAT)

	(p2 DOUBLE-FLOAT)

	(p3 DOUBLE-FLOAT)

	(p4 DOUBLE-FLOAT)

	(p5 DOUBLE-FLOAT)

	(p6 DOUBLE-FLOAT)

	(p7 DOUBLE-FLOAT)

	(p8 DOUBLE-FLOAT))

(:library "_csnd"))



(ffi:def-call-out csoundCsdAddEvent7

(:name "csoundCsdAddEvent7")

(:arguments (csound (ffi:c-pointer CSOUND))

	(p1 DOUBLE-FLOAT)

	(p2 DOUBLE-FLOAT)

	(p3 DOUBLE-FLOAT)

	(p4 DOUBLE-FLOAT)

	(p5 DOUBLE-FLOAT)

	(p6 DOUBLE-FLOAT)

	(p7 DOUBLE-FLOAT))

(:library "_csnd"))



(ffi:def-call-out csoundCsdAddEvent6

(:name "csoundCsdAddEvent6")

(:arguments (csound (ffi:c-pointer CSOUND))

	(p1 DOUBLE-FLOAT)

	(p2 DOUBLE-FLOAT)

	(p3 DOUBLE-FLOAT)

	(p4 DOUBLE-FLOAT)

	(p5 DOUBLE-FLOAT)

	(p6 DOUBLE-FLOAT))

(:library "_csnd"))



(ffi:def-call-out csoundCsdAddEvent5

(:name "csoundCsdAddEvent5")

(:arguments (csound (ffi:c-pointer CSOUND))

	(p1 DOUBLE-FLOAT)

	(p2 DOUBLE-FLOAT)

	(p3 DOUBLE-FLOAT)

	(p4 DOUBLE-FLOAT)

	(p5 DOUBLE-FLOAT))

(:library "_csnd"))



(ffi:def-call-out csoundCsdAddEvent4

(:name "csoundCsdAddEvent4")

(:arguments (csound (ffi:c-pointer CSOUND))

	(p1 DOUBLE-FLOAT)

	(p2 DOUBLE-FLOAT)

	(p3 DOUBLE-FLOAT)

	(p4 DOUBLE-FLOAT))

(:library "_csnd"))



(ffi:def-call-out csoundCsdAddEvent3

(:name "csoundCsdAddEvent3")

(:arguments (csound (ffi:c-pointer CSOUND))

	(p1 DOUBLE-FLOAT)

	(p2 DOUBLE-FLOAT)

	(p3 DOUBLE-FLOAT))

(:library "_csnd"))



(ffi:def-call-out csoundCsdSave

(:name "csoundCsdSave")

(:arguments (csound (ffi:c-pointer CSOUND))

	(filename ffi:c-string))

(:return-type ffi:int)

(:library "_csnd"))



(ffi:def-call-out csoundCsdCompile

(:name "csoundCsdCompile")

(:arguments (csound (ffi:c-pointer CSOUND))

	(filename ffi:c-string))

(:return-type ffi:int)

(:library "_csnd"))



(ffi:def-call-out csoundCsdPerform

(:name "csoundCsdPerform")

(:arguments (csound (ffi:c-pointer CSOUND))

	(filename ffi:c-string))

(:return-type ffi:int)

(:library "_csnd"))



(ffi:def-call-out csoundCompileCsd

(:name "csoundCompileCsd")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(csdFilename ffi:c-string))

(:return-type ffi:int)

(:library "_csnd"))



(ffi:def-call-out csoundPerformCsd

(:name "csoundPerformCsd")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(csdFilename ffi:c-string))

(:return-type ffi:int)

(:library "_csnd"))

