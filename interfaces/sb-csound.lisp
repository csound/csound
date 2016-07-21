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
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;
; This file is handwritten and should be maintained by keeping it up to date
; with regard to include/csound.h. This file is not intended to be complete
; and essentially defines a Steel Bank Common Lisp interface to a subset of 
; the most useful functions in csound.h. At the present time, only pointers, 
; strings, and other primitive types are used in this interface.

(defpackage :sb-csound
    (:use :common-lisp :sb-alien :sb-c-call)
    (:export 
        :csoundCreate
        :csoundDestroy
        :csoundCompileCsdText
        :csoundCompileOrc
        :csoundReadScore
        :csoundStart
        :csoundSetOption
        :csoundSetControlChanel
        :csoundPerformKsmps
    )
)


(in-package :sb-csound)

(sb-alien:load-shared-object "libcsound64.so")
;(sb-alien:load-shared-object "/home/mkg/csound/cs6make/libcsound64.so")

(declaim (inline csoundCreate))
(define-alien-routine "csoundCreate" integer (host-data integer))
      
(declaim (inline csoundDestroy))
(define-alien-routine "csoundDestroy" sb-alien:void (csound integer))
      
(declaim (inline csoundCompileCsdText))
(define-alien-routine "csoundCompileCsdText" integer (csound integer) (csd-text c-string))

(declaim (inline csoundCompileOrc))
(define-alien-routine "csoundCompileOrc" integer (csound integer) (orc-text c-string))

(declaim (inline csoundReadScore))
(define-alien-routine "csoundReadScore" integer (csound integer) (sco-text c-string))

(declaim (inline csoundStart))
(define-alien-routine "csoundStart" integer (csound integer))

(declaim (inline csoundPerformKsmps))
(define-alien-routine "csoundPerformKsmps" integer (csound integer))

(declaim (inline csoundSetOption))
(define-alien-routine "csoundSetOption" integer (csound integer) (one-option c-string))

(declaim (inline csoundSetControlChannel))
(define-alien-routine "csoundSetControlChannel" sb-alien:void (csound integer) (channel-name c-string) (channel-value double))

#||
(defun cm-event-to-istatement (event) 
    (let ())
)

;;; Given a Common Music event source (event, seq, process, or list), 
;;; translate each event into a Csound "i" statement, then render
;;; the resulting score using the orc-text and options. No monkeying with files.
(defun render-csound (event-source orc-text options)
    (progn
        (format t "Building Csound score...~%")
        (let 
            ((score-list (list)) 
            (cs 0) 
            (result 0) 
            (sco-text))
            (mapcar cm-event-to-istatement event-source score-list)
            (setf sco-text (format nil "~{~A~^, ~}" score-list))
            (setf cs (csound::csoundCreate (cffi:null-pointer)))
            (format t "csoundCreate returned: ~S~%" cs)
            (setf result (csound::csoundCompileOrc cs orc-text))
            (format t "csoundCompileOrc returned: ~D~%" result)
            (setf result (csound::readScore cs sco-text))
            (format t "csound:readScore returned: ~D~%" result)
            (setf result (csound::csoundStart cs))
            (format t "csoundStart returned: ~D~%" result)
            (loop 
                (setf result (csound::csoundPerformKsmps cs))
                (when (not (equal result 0))(return))
            )        
        )
    )
)
||#



