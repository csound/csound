; L I S P   C F F I   I N T E R F A C E   F O R   C S O U N D . H
;
; Copyright (C) 2016 Michael Gogins
;
; This file belongs to Csound.
;
; This software is free software; you can redistribute it and/or
; modify it under the terms of the GNU Lesser General Public
; License as published by the Free Software Foundation; either
; version 2.1 of the License, or (at your option) any later version.
;
; This software is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Lesser General Public License for more details.
;
; You should have received a copy of the GNU Lesser General Public
; License along with this software; if not, write to the Free Software
; Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
;
; This file is handwritten and should be maintained by keeping it up to date
; with regard to include/csound.h. This file is not intended to be complete
; and essentially defines a Lisp interface to a subset of the most useful
; functions in csound.h. At the present time, only pointers, strings, and
; other primitive types are used in this interface.

(defpackage :csound
    (:use :common-lisp :cffi))
(cffi:define-foreign-library libcsound64
    (:darwin "libcsound64.dylib")
    (:unix "libcsound64.so")
    (:windows "csound64.dll")
    (t (:default "libcsound64")))
(cffi:use-foreign-library libcsound64)
(in-package :csound)

; You can paste below here new definitions including those created
; e.g. by SWIG. Be sure to TEST any changes you make to this file!

; (cffi:defcfun ("csoundTableLength" csoundTableLength) :int
  ; (csound :pointer)
  ; (table :int))

; (cffi:defcfun ("csoundTableGet" csoundTableGet) :double
  ; (csound :pointer)
  ; (table :int)
  ; (index :int))

; (cffi:defcfun ("csoundTableSet" csoundTableSet) :void
  ; (csound :pointer)
  ; (table :int)
  ; (index :int)
  ; (valu :double))

; (cffi:defcfun ("csoundRunUtility" csoundRunUtility) :int
  ; (csound :pointer)
  ; (nayme :string)
  ; (argc :int)
  ; (argv :pointer))

; (cffi:defcfun ("csoundMessage" csoundMessage) :void
    ; (csound :pointer)
    ; (control :string)
    ; &rest)

; (defun csoundMessage (csound control &rest values)
    ; (cffi:foreign-funcall "csoundMessage" csound :pointer control :pointer &rest values :void))

(cffi:defcfun ("csoundSetControlChannel" csoundSetControlChannel) :void
  (csound :pointer)
  (name :string)
  (value :double))

(cffi:defcfun ("csoundInitialize" csoundInitialize) :int
  (flags :int))

(cffi:defcfun ("csoundCreate" csoundCreate) :pointer
  (host-data :pointer))

(cffi:defcfun ("csoundDestroy" csoundDestroy) :void
  (csound :pointer))

(cffi:defcfun ("csoundGetVersion" csoundGetVersion) :int)

(cffi:defcfun ("csoundGetAPIVersion" csoundGetAPIVersion) :int)

(cffi:defcfun ("csoundCompileOrc" csoundCompileOrc) :int
  (csound :pointer)
  (orc :string))

(cffi:defcfun ("csoundEvalCode" csoundEvalCode) :double
  (csound :pointer)
  (orc :string))

(cffi:defcfun ("csoundCompileArgs" csoundCompileArgs) :int
  (csound :pointer)
  (argc :int)
  (argv :pointer))

(cffi:defcfun ("csoundStart" csoundStart) :int
  (csound :pointer))

(cffi:defcfun ("csoundCompile" csoundCompile) :int
  (csound :pointer)
  (argc :int)
  (argv :pointer))

(cffi:defcfun ("csoundCompileCsd" csoundCompileCsd) :int
  (csound :pointer)
  (csd-pathname :string))

(cffi:defcfun ("csoundCompileCsdText" csoundCompileCsdText) :int
  (csound :pointer)
  (csd-text :string))

(cffi:defcfun ("csoundPerform" csoundPerform) :int
  (csound :pointer))

(cffi:defcfun ("csoundPerformKsmps" csoundPerformKsmps) :int
  (csound :pointer))

(cffi:defcfun ("csoundPerformBuffer" csoundPerformBuffer) :int
  (csound :pointer))

(cffi:defcfun ("csoundStop" csoundStop) :void
  (csound :pointer))

(cffi:defcfun ("csoundCleanup" csoundCleanup) :int
  (csound :pointer))

(cffi:defcfun ("csoundReset" csoundReset) :void
  (csound :pointer))

(cffi:defcfun ("csoundGetSr" csoundGetSr) :double
  (csound :pointer))

(cffi:defcfun ("csoundGetKr" csoundGetKr) :double
  (csound :pointer))

(cffi:defcfun ("csoundGetKsmps" csoundGetKsmps) :int32
  (csound :pointer))

(cffi:defcfun ("csoundGetNchnls" csoundGetNchnls) :int32
  (csound :pointer))

(cffi:defcfun ("csoundGetNchnlsInput" csoundGetNchnlsInput) :int32
  (csound :pointer))

(cffi:defcfun ("csoundGet0dBFS" csoundGet0dBFS) :double
  (csound :pointer))

(cffi:defcfun ("csoundGetCurrentTimeSamples" csoundGetCurrentTimeSamples) :int64
  (csound :pointer))

(cffi:defcfun ("csoundGetSizeOfMYFLT" csoundGetSizeOfMYFLT) :int)

(cffi:defcfun ("csoundGetHostData" csoundGetHostData) :pointer
  (csound :pointer))

(cffi:defcfun ("csoundSetHostData" csoundSetHostData) :void
  (csound :pointer)
  (hostData :pointer))

(cffi:defcfun ("csoundSetOption" csoundSetOption) :int
  (csound :pointer)
  (option :string))

(cffi:defcfun ("csoundGetOutputName" csoundGetOutputName) :string
  (csound :pointer))

(cffi:defcfun ("csoundSetOutput" csoundSetOutput) :void
  (csound :pointer)
  (nayme :string)
  (tipe :string)
  (format :string))

(cffi:defcfun ("csoundSetInput" csoundSetInput) :void
  (csound :pointer)
  (nayme :string))

(cffi:defcfun ("csoundSetMIDIInput" csoundSetMIDIInput) :void
  (csound :pointer)
  (nayme :string))

(cffi:defcfun ("csoundSetMIDIFileInput" csoundSetMIDIFileInput) :void
  (csound :pointer)
  (nayme :string))

(cffi:defcfun ("csoundSetMIDIOutput" csoundSetMIDIOutput) :void
  (csound :pointer)
  (nayme :string))

(cffi:defcfun ("csoundSetMIDIFileOutput" csoundSetMIDIFileOutput) :void
  (csound :pointer)
  (nayme :string))

(cffi:defcfun ("csoundSetRTAudioModule" csoundSetRTAudioModule) :void
  (csound :pointer)
  (moduule :string))

(cffi:defcfun ("csoundGetInputBufferSize" csoundGetInputBufferSize) :long
  (csound :pointer))

(cffi:defcfun ("csoundGetOutputBufferSize" csoundGetOutputBufferSize) :long
  (csound :pointer))

 (cffi:defcfun ("csoundGetInputBuffer" csoundGetInputBuffer) :pointer
   (csound :pointer))

(cffi:defcfun ("csoundGetOutputBuffer" csoundGetOutputBuffer) :pointer
  (csound :pointer))

(cffi:defcfun ("csoundGetSpin" csoundGetSpin) :pointer
  (csound :pointer))

; (cffi:defcfun ("csoundAddSpinSample" csoundAddSpinSample) :void
  ; (csound :pointer)
  ; (frayme :int)
  ; (channel :int)
  ; (sample :float))

; (cffi:defcfun ("csoundGetSpout" csoundGetSpout) :pointer
  ; (csound :pointer))

; (cffi:defcfun ("csoundGetSpoutSample" csoundGetSpoutSample) :double
  ; (csound :pointer)
  ; (frame :int)
  ; (channel :int))

(cffi:defcfun ("csoundReadScore" csoundReadScore) :int
  (csound :pointer)
  (score :string))

; (cffi:defcfun ("csoundGetScoreTime" csoundGetScoreTime) :double
  ; (csound :pointer))

; (cffi:defcfun ("csoundIsScorePending" csoundIsScorePending) :int
  ; (csound :pointer))

; (cffi:defcfun ("csoundSetScorePending" csoundSetScorePending) :void
  ; (csound :pointer)
  ; (pending :int))

; (cffi:defcfun ("csoundGetScoreOffsetSeconds" csoundGetScoreOffsetSeconds) :double
  ; (csound :pointer))

; (cffi:defcfun ("csoundSetScoreOffsetSeconds" csoundSetScoreOffsetSeconds) :void
  ; (csound :pointer)
  ; (time :double))

; (cffi:defcfun ("csoundRewindScore" csoundRewindScore) :void
  ; (csound :pointer))

; (cffi:defcfun ("csoundGetMessageLevel" csoundGetMessageLevel) :int
  ; (csound :pointer))

; (cffi:defcfun ("csoundSetMessageLevel" csoundSetMessageLevel) :void
  ; (csound :pointer)
  ; (messageLevel :int))

; (cffi:defcfun ("csoundCreateMessageBuffer" csoundCreateMessageBuffer) :void
  ; (csound :pointer)
  ; (toStdOut :int))

; (cffi:defcfun ("csoundGetFirstMessage" csoundGetFirstMessage) :string
  ; (csound :pointer))

; (cffi:defcfun ("csoundGetFirstMessageAttr" csoundGetFirstMessageAttr) :int
  ; (csound :pointer))

; (cffi:defcfun ("csoundPopFirstMessage" csoundPopFirstMessage) :void
  ; (csound :pointer))

; (cffi:defcfun ("csoundGetMessageCnt" csoundGetMessageCnt) :int
  ; (csound :pointer))

; (cffi:defcfun ("csoundDestroyMessageBuffer" csoundDestroyMessageBuffer) :void
  ; (csound :pointer))

; (cffi:defcfun ("csoundGetControlChannel" csoundGetControlChannel) :double
  ; (csound :pointer)
  ; (nayme :string)
  ; (err :pointer))

; (cffi:defcfun ("csoundGetAudioChannel" csoundGetAudioChannel) :void
  ; (csound :pointer)
  ; (name :string)
  ; (samples :pointer))

; (cffi:defcfun ("csoundSetAudioChannel" csoundSetAudioChannel) :void
  ; (csound :pointer)
  ; (name :string)
  ; (samples :pointer))

; (cffi:defcfun ("csoundGetStringChannel" csoundGetStringChannel) :void
  ; (csound :pointer)
  ; (name :string)
  ; (string :string))

; (cffi:defcfun ("csoundSetStringChannel" csoundSetStringChannel) :void
  ; (csound :pointer)
  ; (name :string)
  ; (string :string))

; (cffi:defcfun ("csoundScoreEvent" csoundScoreEvent) :int
  ; (csound :pointer)
  ; (tipe :char)
  ; (pFields :pointer)
  ; (numFields :long))

; (cffi:defcfun ("csoundScoreEventAbsolute" csoundScoreEventAbsolute) :int
  ; (csound :pointer)
  ; (type :char)
  ; (pfields :pointer)
  ; (numFields :long)
  ; (time_ofs :double))

; (cffi:defcfun ("csoundInputMessage" csoundInputMessage) :void
  ; (csound :pointer)
  ; (message :string))

; (cffi:defcfun ("csoundIsNamedGEN" csoundIsNamedGEN) :int
  ; (csound :pointer)
  ; (num :int))

; (cffi:defcfun ("csoundGetNamedGEN" csoundGetNamedGEN) :void
  ; (csound :pointer)
  ; (num :int)
  ; (name :string)
  ; (len :int))

; (cffi:defcfun ("csoundAppendOpcode" csoundAppendOpcode) :int
  ; (csound :pointer)
  ; (opname :string)
  ; (dsblksiz :int)
  ; (flags :int)
  ; (thread :int)
  ; (outypes :string)
  ; (intypes :string)
  ; (iopadr :pointer)
  ; (kopadr :pointer)
  ; (aopadr :pointer))
  
#||
;;; Given a Common Music event source (event, seq, process, or list), 
;;; translate each event into a Csound "i" statement, then render
;;; the resulting score using the orc-text and options. No monkeying with files.
(defun cm-event-to-istatement (event) 
    (let ())
)
(defun render-csound (event-source orc-text options)
    (progn
        (format t "Building Csound score...~%")
        (defparameter score-list (list)) 
        (mapcar cm-event-to-istatement event-source score-list)
        (let (sco-text (apply #'concatenate 'string list)))
        (defparameter cs 0)
        (defparameter result 0)
        (setq cs (csound::csoundCreate (cffi:null-pointer)))
        (format t "csoundCreate returned: ~S~%" cs)
        (setq result (csound::csoundCompileOrc cs orc-text))
        (format t "csoundCompileOrc returned: ~D~%" result)
        (setq result (csound::readScore cs sco-text))
        (format t "csound:readScore returned: ~D~%" result)
        (setq result (csound::csoundStart cs))
        (format t "csoundStart returned: ~D~%" result)
        (loop 
            (setq result (csound::csoundPerformKsmps cs))
            (when (not (equal result 0))(return))
        )        
    ))
||#


