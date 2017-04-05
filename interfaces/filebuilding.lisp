(defpackage :filebuilding
    (:use :common-lisp :cffi))

(in-package :filebuilding)

;;;SWIG wrapper code starts here

(defmacro defanonenum (&body enums)
   "Converts anonymous enums to defconstants."
  `(progn ,@(loop for value in enums
                  for index = 0 then (1+ index)
                  when (listp value) do (setf index (second value)
                                              value (first value))
                  collect `(defconstant ,value ,index))))

;;;SWIG wrapper code ends here


(defcfun ("csoundCsdCreate" csoundCsdCreate) :void
  (csound :pointer))

(defcfun ("csoundCsdSetOptions" csoundCsdSetOptions) :void
  (csound :pointer)
  (options :string))

(defcfun ("csoundCsdGetOptions" csoundCsdGetOptions) :string
  (csound :pointer))

(defcfun ("csoundCsdSetOrchestra" csoundCsdSetOrchestra) :void
  (csound :pointer)
  (orchestra :string))

(defcfun ("csoundCsdGetOrchestra" csoundCsdGetOrchestra) :string
  (csound :pointer))

(defcfun ("csoundCsdAddScoreLine" csoundCsdAddScoreLine) :void
  (csound :pointer)
  (line :string))

(defcfun ("csoundCsdAddEvent11" csoundCsdAddEvent11) :void
  (csound :pointer)
  (p1 :double)
  (p2 :double)
  (p3 :double)
  (p4 :double)
  (p5 :double)
  (p6 :double)
  (p7 :double)
  (p8 :double)
  (p9 :double)
  (p10 :double)
  (p11 :double))

(defcfun ("csoundCsdAddEvent10" csoundCsdAddEvent10) :void
  (csound :pointer)
  (p1 :double)
  (p2 :double)
  (p3 :double)
  (p4 :double)
  (p5 :double)
  (p6 :double)
  (p7 :double)
  (p8 :double)
  (p9 :double)
  (p10 :double))

(defcfun ("csoundCsdAddEvent9" csoundCsdAddEvent9) :void
  (csound :pointer)
  (p1 :double)
  (p2 :double)
  (p3 :double)
  (p4 :double)
  (p5 :double)
  (p6 :double)
  (p7 :double)
  (p8 :double)
  (p9 :double))

(defcfun ("csoundCsdAddEvent8" csoundCsdAddEvent8) :void
  (csound :pointer)
  (p1 :double)
  (p2 :double)
  (p3 :double)
  (p4 :double)
  (p5 :double)
  (p6 :double)
  (p7 :double)
  (p8 :double))

(defcfun ("csoundCsdAddEvent7" csoundCsdAddEvent7) :void
  (csound :pointer)
  (p1 :double)
  (p2 :double)
  (p3 :double)
  (p4 :double)
  (p5 :double)
  (p6 :double)
  (p7 :double))

(defcfun ("csoundCsdAddEvent6" csoundCsdAddEvent6) :void
  (csound :pointer)
  (p1 :double)
  (p2 :double)
  (p3 :double)
  (p4 :double)
  (p5 :double)
  (p6 :double))

(defcfun ("csoundCsdAddEvent5" csoundCsdAddEvent5) :void
  (csound :pointer)
  (p1 :double)
  (p2 :double)
  (p3 :double)
  (p4 :double)
  (p5 :double))

(defcfun ("csoundCsdAddEvent4" csoundCsdAddEvent4) :void
  (csound :pointer)
  (p1 :double)
  (p2 :double)
  (p3 :double)
  (p4 :double))

(defcfun ("csoundCsdAddEvent3" csoundCsdAddEvent3) :void
  (csound :pointer)
  (p1 :double)
  (p2 :double)
  (p3 :double))

(defcfun ("csoundCsdSave" csoundCsdSave) :int
  (csound :pointer)
  (filename :string))

(defcfun ("csoundCsdCompile" csoundCsdCompile) :int
  (csound :pointer)
  (filename :string))

(defcfun ("csoundCsdPerform" csoundCsdPerform) :int
  (csound :pointer)
  (filename :string))

(defcfun ("csoundCompileCsd" csoundCompileCsd) :int
  (arg0 :pointer)
  (csdFilename :string))

(defcfun ("csoundPerformCsd" csoundPerformCsd) :int
  (arg0 :pointer)
  (csdFilename :string))


