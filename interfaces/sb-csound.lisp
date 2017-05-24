; S T E E L   B A N K   C O M M O N   L I S P   F F I   I N T E R F A C E   T O   C S O U N D . H
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
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;
; This file is handwritten and should be maintained by keeping it up to date
; with regard to include/csound.h. This file is not intended to be complete
; and essentially defines a Steel Bank Common Lisp interface to a subset of
; the most useful functions in csound.h. At the present time, only pointers,
; strings, and other primitive types are used in this interface.

(defpackage :sb-csound
    (:use :common-lisp :sb-alien :sb-c-call :cm)
    (:export
        :csoundCompileCsd
        :csoundCompileCsdText
        :csoundCompileOrc
        :csoundCreate
        :csoundDestroy
        :csoundPerformKsmps
        :csoundReadScore
        :csoundSetControlChanel
        :csoundSetOption
        :csoundStart
        :csoundCleanup
     )
)

(in-package :sb-csound)

#+unix (sb-alien:load-shared-object "libcsound64.so")
#+win32 (sb-alien:load-shared-object "csound64.dll")

(declaim (inline csoundCompileCsd))
(define-alien-routine "csoundCompileCsd" integer (csound integer) (csd-pathname c-string))

(declaim (inline csoundCompileCsdText))
(define-alien-routine "csoundCompileCsdText" integer (csound integer) (csd-text c-string))

(declaim (inline csoundCompileOrc))
(define-alien-routine "csoundCompileOrc" integer (csound integer) (orc-text c-string))

(declaim (inline csoundCreate))
(define-alien-routine "csoundCreate" integer (host-data integer))

(declaim (inline csoundDestroy))
(define-alien-routine "csoundDestroy" sb-alien:void (csound integer))

(declaim (inline csoundPerformKsmps))
(define-alien-routine "csoundPerformKsmps" integer (csound integer))

(declaim (inline csoundReadScore))
(define-alien-routine "csoundReadScore" integer (csound integer) (sco-text c-string))

(declaim (inline csoundSetControlChannel))
(define-alien-routine "csoundSetControlChannel" sb-alien:void (csound integer) (channel-name c-string) (channel-value double))

(declaim (inline csoundSetOption))
(define-alien-routine "csoundSetOption" integer (csound integer) (one-option c-string))

(declaim (inline csoundStart))
(define-alien-routine "csoundStart" integer (csound integer))

(declaim (inline csoundCleanup))
(define-alien-routine "csoundCleanup" integer (csound integer))

(set-dispatch-macro-character #\# #\> #'cl-heredoc:read-heredoc)

(in-package :cm)
(use-package :sb-csound)

(defun event-to-istatement (event channel-offset velocity-scale)
"
Translates a Common Music MIDI event to a Csound score event
(i-statement), which is terminated with a newline. An offset, which may
be any number, is added to the MIDI channel number.
"
    (format nil "i ~,6f ~,6f ~,6f ~,6f ~,6f 0 0 0 0 0 0~%" (+ channel-offset (midi-channel event)) (object-time event)(midi-duration event)(keynum (midi-keynum event))(* velocity-scale (midi-amplitude event)))
)
(export 'event-to-istatement)

(defun replace-all (string part replacement &key (test #'char=))
"
Replaces all occurences of the string 'part' in 'string' with 'replacement',
using 'test' for character equality.
"
  (with-output-to-string (out)
    (loop with part-length = (length part)
          for old-pos = 0 then (+ pos part-length)
          for pos = (search part string
                            :start2 old-pos
                            :test test)
          do (write-string string out
                           :start old-pos
                           :end (or pos (length string)))
          when pos do (write-string replacement out)
          while pos)))

(defun render-with-csound (sequence csd-text &optional (channel-offset 1) (velocity-scale 127) (csound nil))
"
Given a Common Music seq 'sequence', translates each of its events into a
Csound 'i' statement, optionally offsetting the channel number and/or
rescaling MIDI velocity, then renders the resulting score using 'csd-text'.
A CSD is used because it can contain any textual Csound input in one block of
raw text. The score generated from 'sequence' is appended to any <CsScore>
lines found in 'csd-text'. This is done so that Csound will quit performing
at the end of the score. It is possible to call csoundReadScore during the
performance. This function returns the Csound object that it uses.

The optional 'csound' parameter is used to call Csound if passed. This enables
'render-with-csound' to be run in a separate thread of execution, and for
the caller to control Csound instrument parameters during real time
performance, e.g.

(setq csound (sb-csound:csoundCreate 0))
(setq my-thread (bt:make-thread (lambda () (render-with-csound cs csd 1 127 csound))))
(sb-csound:csoundSetControlChannel csound 'mychannel' myvalue)
(bt:join-thread my-thread)

"

    (let
        ((score-list (list))
        (cs)
        (sco-text)
        (result)
        (new-csd-text))
        (progn
            (format t "Building Csound score...~%")
            (defun curried-event-to-istatement (event)
                (event-to-istatement event channel-offset velocity-scale))
            (setq score-list (mapcar 'curried-event-to-istatement (subobjects sequence)))
            (setq sco-text (format nil "~{~A~^ ~}" score-list))
            (if csound
                (setq cs csound)
                (progn
                    (setq cs (sb-csound:csoundCreate 0))
                    (format t "csoundCreate returned: ~S.~%" cs)
                )
            )
            (setq new-csd-text (replace-all csd-text "</CsScore>" (concatenate 'string sco-text "</CsScore>")))
            (format t "new-csd-text: ~A~%" new-csd-text)
            (setq result (sb-csound:csoundCompileCsdText cs new-csd-text))
            (format t "csoundCompileCsdText returned: ~D.~%" result)
            (setq result (sb-csound:csoundStart cs))
            (format t "csoundStart returned: ~D.~%" result)
            (loop
                (setq result (sb-csound:csoundPerformKsmps cs))
                (when (not (equal result 0))(return))
            )
            (setf result(sb-csound:csoundCleanup cs))
            (format t "csoundCleanup returned: ~D.~%" result)
            (sleep 5)
            (if (not csound)
                (sb-csound::csoundDestroy cs)
                (format t "csoundDestroy was called.~%")
            )
            (format t "The Csound performance has ended: ~D.~%" result)
        )
    )
)
(export 'render-with-csound)


