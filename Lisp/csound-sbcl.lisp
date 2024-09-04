;;;;
;;;;  Copyright (C) 2024 Victor Lazzarini
;;;;
;;;;  SBCL foreign-function interface bindings for Csound
;;;;  
;;;;  This file is part of Csound.
;;;;
;;;;  The Csound Library is free software; you can redistribute it
;;;;  and/or modify it under the terms of the GNU Lesser General Public
;;;;  License as published by the Free Software Foundation; either
;;;;  version 2.1 of the License, or (at your option) any later version.
;;;;
;;;;  Csound is distributed in the hope that it will be useful,
;;;;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;;;  GNU Lesser General Public License for more details.
;;;;
;;;;  You should have received a copy of the GNU Lesser General Public
;;;;  License along with Csound; if not, write to the Free Software
;;;;  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
;;;;  02111-1307 USA
;;;;

(defpackage :csound
  (:import-from :sb-ext
                #:posix-getenv #:*posix-argv*)
  (:import-from :sb-alien
                #:load-shared-object
                #:define-alien-routine
                #:deref
                #:c-string
                #:int
                #:double
                #:make-alien
                #:free-alien
                #:void
                #:cast
                )
  (:export #:csound-create
           #:csound-compile
           #:csound-compile-orc
           #:csound-compile-csd
           #:csound-set-option
           #:csound-get-ksmps
           #:csound-get-kr
           #:csound-get-sr
           #:csound-get-channels
           #:csound-perform-ksmps
           #:csound-get-0dbfs
           #:csound-start
           #:csound-reset
           #:csound-destroy
           #:csound-set-control-channel
           #:csound-get-control-channel
           #:csound-event-string
           #:csound-get-spin
           #:csound-get-spout
           #:csound-spin
           #:csound-spout
           #:csound-get-table
           #:csound-get-table-value
           #:csound-set-table-value
           #:csound-create-performance-thread
           #:csound-performance-thread-play
           #:csound-performance-thread-pause
           #:csound-performance-thread-toggle-pause
           #:csound-performance-thread-stop
           #:csound-performance-thread-join
           #:csound-performance-thread-record
           #:csound-performance-thread-stop-record
           #:csound-performance-thread-is-running
           #:csound-destroy-performance-thread
           )
  (:use :common-lisp))

(in-package :csound)



(defvar *libcsound*
  (concatenate 'string (posix-getenv "HOME")
               "/Library/Frameworks/CsoundLib64.framework/CsoundLib64"))
(if (not (probe-file *libcsound*))
    (setf *libcsound*
          "/Library/Frameworks/CsoundLib64.framework/CsoundLib64"))
;;; then local directory - linux .so 
(if (not (probe-file *libcsound*))
    (setf *libcsound* "libcsound64.so"))
;;; if libcsound was not found
(if (not (probe-file *libcsound*)) (quit))

;;; sbcl FFI interface
(load-shared-object *libcsound*)
(define-alien-routine "csoundCreate" (* T) (a (* T)) (b c-string))
(define-alien-routine "csoundCompile" int (a (* T)) (b int) (c (* c-string)))
(define-alien-routine "csoundCompileOrc" int (a (* T)) (b c-string) (c int))
(define-alien-routine "csoundCompileCSD" int (a (* T)) (b c-string) (c int))
(define-alien-routine "csoundSetOption" int (a (* T)) (b c-string))
(define-alien-routine "csoundGetKr" double (a (* T)))
(define-alien-routine "csoundGetSr" double (a (* T)))
(define-alien-routine "csoundGetKsmps" int (a (* T)))
(define-alien-routine "csoundGetChannels" int (a (* T)) (b int))
(define-alien-routine "csoundGet0dBFS" double (a (* T)))
(define-alien-routine "csoundEventString" int (a (* T)) (b c-string) (c int))
(define-alien-routine "csoundStart" int (a (* T)))
(define-alien-routine "csoundPerformKsmps" int (a (* T)))
(define-alien-routine "csoundGetSpout" (* double) (a (* T)))
(define-alien-routine "csoundGetSpin" (* double) (a (* T)))
(define-alien-routine "csoundDestroy" void (a (* T)))
(define-alien-routine "csoundReset" void (a (* T)))
(define-alien-routine "csoundSetControlChannel" void (a (* T))
  (b c-string) (c double))
(define-alien-routine "csoundGetControlChannel" double (a (* T))
  (b c-string) (c int :out))
(define-alien-routine "csoundCreatePerformanceThread" (* T) (a (* T)))
(define-alien-routine "csoundPerformanceThreadPlay" void (a (* T)))
(define-alien-routine "csoundPerformanceThreadTogglePause" void (a (* T)))
(define-alien-routine "csoundPerformanceThreadPause" void (a (* T)))
(define-alien-routine "csoundPerformanceThreadStop" void (a (* T)))
(define-alien-routine "csoundPerformanceThreadRecord" void (a (* T))
  (b c-string) (c int) (d int))
(define-alien-routine "csoundPerformanceThreadStopRecord" void (a (* T)))
(define-alien-routine "csoundPerformanceThreadIsRunning" int (a (* T)))
(define-alien-routine "csoundPerformanceThreadJoin" void (a (* T)))
(define-alien-routine "csoundDestroyPerformanceThread" void (a (* T)))
(define-alien-routine "csoundGetTable" int (a (* T)) (b (* (* double))) (c int))


(defun csound-create (&key hostdata opcodedir)
  " Create a Csound engine instance "
  (csoundCreate hostdata opcodedir))

(defun csound-compile (csound arglist)
  " Compile code from command-line argument list "
  (let ((argc (length arglist))
        (argv (make-alien (array c-string 32))))
    (if (> argc 32)
        (error "too many arguments, limit is 32"))
    (dotimes (n argc)
      (setf (deref (deref argv) n) (nth n arglist)))
    (csoundCompile csound argc (cast argv (* c-string)))
    ))

(defun csound-compile-orc (csound orc &key (async 0))
  " Compile code from string, optionally asynchronously "
  (csoundCompileOrc csound orc async))

(defun csound-compile-csd (csound csd &key (is-text 0))
  " Compile code in CSD file or optionally from a text string "
  (csoundCompileCSD csound csd is-text))

(defun csound-set-option (csound option)
  " Set one or more engine options, called before csound-start " 
  (csoundSetOption csound option))

(defun csound-get-ksmps (csound)
  " Get the number of sample periods in an audio block (ksmps) "
  (csoundGetKsmps csound))

(defun csound-get-sr (csound)
  " Get the sampling rate "
  (csoundGetSr csound))

(defun csound-get-kr (csound)
  " Get the control rate "
  (csoundGetKr csound))

(defun csound-get-0dbfs (csound)
  " Get the 0dB full-scale level "
  (csoundGet0dBFS csound))

(defun csound-get-channels (csound &key (is-input 0))
  " Get the 0dB full-scale level "
  (csoundGetChannels csound is-input))

(defun csound-start (csound)
    " Start the Csound engine "
  (csoundStart csound))

(defun csound-destroy (csound)
  " Destroy an engine instance "
  (csoundDestroy csound))

(defun csound-reset (csound)
  " Reset an engine instance "
  (csoundReset csound))

(defun csound-perform-ksmps (csound)
  " Compute one ksmps audio block "
  (csoundPerformKsmps csound))

(defun csound-event-string (csound event &key (async 0))
  " Send in events defined in a string, optionally asynchronously "
  (csoundEventString csound event async))

(defun csound-set-control-channel (csound name value)
  " Set the control channel name to value "
  (csoundSetControlChannel csound name value))

(defun csound-get-control-channel (csound name &key err)
  " Get the value from control channel name"
  (csoundSetControlChannel csound name err))

(defun csound-get-spout (csound)
  " Get the pointer to the main output (spout), use dref to access data "
  (csoundGetSpout csound))

(defun csound-spout (csound n)
  " Get a single audio sample from the main output "
  (deref (csoundGetSpout csound) n))

(defun csound-get-spin (csound)
  " Get the pointer to the main input (spin), use dref to access data "
  (csoundGetSpin csound))

(defun csound-spin (csound n samp)
  " Set a single audio sample into the main input "
  (setf (deref (csoundGetSpin csound) n) samp)
)

(defun csound-get-table (csound table tablenum)
  " Gets a function table tablenum,
    table needs to be an alien (* (* double)) type, 
    returns table size "
   (csoundGetTable csound table tablenum))

(defun csound-get-table-value (csound tablenum n)
  " Gets a single value from function table tablenum at pos n "
  (let* ((ftab (make-alien (* double)))
         ret)
    (csoundGetTable csound ftab tablenum)
    (setf ret (deref (deref ftab) n))
    (free-alien ftab)
    (return-from csound-get-table-value ret)))
  

(defun csound-set-table-value (csound tablenum n value)
  " Sets a single value from function table tablenum at pos n "
  (let* ((ftab (make-alien (* double))))
    (csoundGetTable csound ftab tablenum)
    (setf(deref (deref ftab) n) (coerce value 'double-float))
    (free-alien ftab)))

(defun csound-create-performance-thread (csound)
  " Create a performance thread for a Csound instance "
  (csoundCreatePerformanceThread csound))

(defun csound-performance-thread-play (performance-thread)
  " Set the performance thread to play " 
  (csoundPerformanceThreadPlay performance-thread))

(defun csound-performance-thread-pause (performance-thread)
  " Set the performance thread to pause " 
  (csoundPerformanceThreadPause performance-thread))

(defun csound-performance-thread-toggle-pause (performance-thread)
  " Toggles the performance thread into pause/play " 
  (csoundPerformanceThreadTogglePause performance-thread))

(defun csound-performance-thread-stop (performance-thread)
  " Stops the performance thread " 
  (csoundPerformanceThreadStop performance-thread))

(defun csound-performance-thread-record (performance-thread
                                         name &key (samplebits 16) (numbuffs 4))
  " Starts recording the performance thread audio output " 
  (csoundPerformanceThreadRecord performance-thread name samplebits numbuffs))

(defun csound-performance-thread-stop-record (performance-thread)
  " Stops recording the performance thread audio output " 
  (csoundPerformanceThreadStopRecord performance-thread))

(defun csound-performance-thread-is-running (performance-thread)
  " Returns the status of a performance thread " 
  (csoundPerformanceThreadIsRunning performance-thread))

(defun csound-performance-thread-join (performance-thread)
  " Joins the performance thread to pause " 
  (csoundPerformanceThreadJoin performance-thread))

(defun csound-destroy-performance-thread (performance-thread)
  " Destroy a performance thread instance " 
  (csoundPerformanceThreadJoin performance-thread))


