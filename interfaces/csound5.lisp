
(defpackage "csound5"
            (:use #:cffi)
            (:nicknames "cs5")
            (:export "CS-CFG-VARIABLE-U" "CS-CFG-VARIABLE-INT-S" "S-FILEX"
             "CS-CFG-VARIABLE-MYFLT-S" "CSOUND-" "OPCODE-LIST-ENTRY" "S-FILE"
             "CS-RT-AUDIO-PARAMS" "CS-CFG-VARIABLE-DOUBLE-S"
             "CS-CFG-VARIABLE-HEAD-S" "XYINDAT-" "RTCLOCK-S"
             "CS-CFG-VARIABLE-BOOLEAN-S" "CS-CFG-VARIABLE-FLOAT-S"
             "CSOUND-RAND-MTSTATE-" "CS-CFG-VARIABLE-STRING-S" "SBUF" "WINDAT-"
             "CSOUND-CHANNEL-LIST-ENTRY-" "UINT32-T" "XYINDAT" "QUAD-T"
             "CS-CFG-VARIABLE-HEAD-T" "FILE" "RTCLOCK" "U-INT32-T" "VA-LIST"
             "CS-CFG-VARIABLE-DOUBLE-T" "CSOUND-1" "INT64-T" "INT-LEAST64-T"
             "CS-CFG-VARIABLE-BOOLEAN-T" "CS-CFG-VARIABLE-FLOAT-T"
             "CSOUND-CHANNEL-LIST-ENTRY" "CSOUND-RAND-MTSTATE" "GNUC-VA-LIST"
             "CS-CFG-VARIABLE-STRING-T" "SIZE-T" "FPOS-T" "CS-CFG-VARIABLE-T"
             "OFF-T" "CS-CFG-VARIABLE-INT-T" "CS-CFG-VARIABLE-MYFLT-T"
             "UINTPTR-T" "WINDAT" "CSOUND-CLEANUP" "CSOUND-SET-IS-GRAPHABLE"
             "CSOUND-STOP" "CSOUND-SET-CONFIGURATION-VARIABLE"
             "CSOUND-JOIN-THREAD" "CSOUND-SET-RECOPEN-CALLBACK-1"
             "CSOUND-GET-CHANNEL" "CSOUND-COMPILE-5" "CSOUND-NEW-1"
             "CSOUND-TIMER-NEW-1-" "CSOUND-SET-RTCLOSE-CALLBACK"
             "CSOUND-GET-SIZE-OF-MYFLT" "CSOUND-CREATE" "CSOUND-CHAN-OAGET"
             "CSOUND-SCORE-SORT" "CSOUND-GET-INPUT-BUFFER-SIZE-1"
             "CSOUND-APPEND-OPCODE-1" "CSOUND-DELETE-UTILITY-LIST-1"
             "CSOUND-PERFORM" "CSOUND-DESTROY-THREAD-LOCK" "CSOUND-GET-SR"
             "CSOUND-MESSAGE-V" "CSOUND-SET-RTCLOSE-CALLBACK-1"
             "CSOUND-SET-DEBUG-1" "CSOUND-SCORE-EXTRACT-1"
             "CSOUND-RAND-MT-DELETE" "CSOUND-SCORE-EVENT-1"
             "CSOUND-PERFORM-KSMPS-1" "CSOUND-GET-CHANNEL-PTR"
             "CSOUND-TABLE-SET" "CSOUND-RUN-UTILITY-1"
             "CSOUND-THREAD-LOCK-UNLOCK" "CSOUND-THREAD-LOCK-LOCK-1"
             "CSOUND-GET-OUTPUT-BUFFER" "CSOUND-LIST-CHANNELS-1"
             "CSOUND-CHAN-OAGET-1" "CSOUND-COPY-GLOBAL-CONFIGURATION-VARIABLES"
             "CSOUND-SET-HOST-IMPLEMENTED-AUDIO-IO-1" "CSOUND-GET-DEBUG-1"
             "CSOUND-SET-SCORE-OFFSET-SECONDS" "CSOUND-GET-RT-RECORD-USER-DATA"
             "CSOUND-RAND-MT-SEED-1" "CSOUND-MESSAGE-S-1"
             "CSOUND-SET-GLOBAL-ENV"
             "CSOUND-SET-EXTERNAL-MIDI-OUT-OPEN-CALLBACK"
             "CSOUND-GET-APIVERSION" "CSOUND-RAND-MT-NEW-2-"
             "CSOUND-TIMER-GET-CPUTIME" "CSOUND-SET-KILL-XYIN-CALLBACK-1"
             "CSOUND-PERFORM-5" "CSOUND-SET-MESSAGE-CALLBACK-1"
             "CSOUND-GET-ENV-1" "CSOUND-SET-EXTERNAL-MIDI-OUT-OPEN-CALLBACK-1"
             "CSOUND-SET-YIELD-CALLBACK-1" "CSOUND-GET0D-BFS"
             "CSOUND-MESSAGE-V-1" "CSOUND-SET-KILL-GRAPH-CALLBACK-1"
             "CSOUND-GET-INPUT-BUFFER-1" "CSOUND-GET-INPUT-BUFFER-SIZE"
             "CSOUND-GET-SCORE-TIME-1" "CSOUND-NEW-2"
             "CSOUND-SET-RECOPEN-CALLBACK" "CSOUND-DELETE-UTILITY-LIST"
             "CSOUND-GET-CPUTIME" "CSOUND-SET-CONTROL-CHANNEL-PARAMS-1"
             "CSOUND-GET-HOST-DATA"
             "CSOUND-SET-EXTERNAL-MIDI-ERROR-STRING-CALLBACK-1"
             "CSOUND-SET-RTRECORD-CALLBACK" "CSOUND-COMPILE-1"
             "CSOUND-CHAN-IASET-1"
             "CSOUND-SET-EXTERNAL-MIDI-OUT-CLOSE-CALLBACK"
             "CSOUND-DESTROY-GLOBAL-VARIABLE" "CSOUND-PERFORM-BUFFER"
             "CSOUND-GET-SCORE-OFFSET-SECONDS-1" "CSOUND-NEW-OPCODE-LIST"
             "CSOUND-SET-READ-XYIN-CALLBACK-1"
             "CSOUND-SET-INPUT-VALUE-CALLBACK" "CSOUND-RAND-MT-SEED"
             "CSOUND-SET-MESSAGE-LEVEL" "CSOUND-QUERY-GLOBAL-VARIABLE-1"
             "CSOUND-NEW-OPCODE-LIST-1" "CSOUND-SET-MAKE-XYIN-CALLBACK"
             "CSOUND-DESTROY-GLOBAL-VARIABLE-1"
             "CSOUND-SET-RTRECORD-CALLBACK-1" "CSOUND-GET-RT-PLAY-USER-DATA-1"
             "CSOUND-TIMER-NEW-1" "CSOUND-SET-KILL-XYIN-CALLBACK"
             "CSOUND-REWIND-SCORE" "CSOUND-DISPOSE-OPCODE-LIST"
             "CSOUND-INPUT-MESSAGE" "CSOUND-APPEND-OPCODE"
             "CSOUND-INITIALIZE-CSCORE-1" "CSOUND-GET-CONTROL-CHANNEL-PARAMS"
             "CSOUND-INPUT-MESSAGE-1" "CSOUND-CHAN-IKSET"
             "CSOUND-WAIT-THREAD-LOCK" "CSOUND-SET-MESSAGE-CALLBACK"
             "CSOUND-SET-EXTERNAL-MIDI-WRITE-CALLBACK"
             "CSOUND-DELETE-CHANNEL-LIST-1" "CSOUND-RAND-MT-NEW-1"
             "CSOUND-CHAN-IASET" "CSOUND-GET-CHANNEL-PTR-1"
             "CSOUND-SET-CSCORE-CALLBACK" "CSOUND-RUN-UTILITY"
             "CSOUND-LIST-CHANNELS" "CSOUND-GET0D-BFS-1" "CSOUND-PRE-COMPILE-1"
             "CSOUND-QUERY-GLOBAL-VARIABLE" "CSOUND-SET-RTPLAY-CALLBACK"
             "CSOUND-COPY-GLOBAL-CONFIGURATION-VARIABLE" "CSOUND-INITIALIZE"
             "CSOUND-SET-MAKE-XYIN-CALLBACK-1" "CSOUND-RAND-MT-NEW-2"
             "CSOUND-SLEEP" "CSOUND-NEW-1-" "CSOUND-PERFORM-1"
             "CSOUND-THREAD-LOCK-NEW-1" "CSOUND-GET-OUTPUT-BUFFER-1"
             "CSOUND-TIMER-RESET" "CSOUND-SET-MESSAGE-LEVEL-1"
             "CSOUND-SET-OUTPUT-VALUE-CALLBACK-1" "CSOUND-PERFORM-6"
             "CSOUND-SET-SCORE-PENDING" "CSOUND-RAND-MT" "CSOUND-GET-KR-1"
             "CSOUND-QUERY-GLOBAL-VARIABLE-NO-CHECK-1"
             "CSOUND-SET-INPUT-VALUE-CALLBACK-1" "CSOUND-GET-SPOUT"
             "CSOUND-SCORE-EXTRACT" "CSOUND-GET-KSMPS" "CSOUND-COMPILE-2"
             "CSOUND-GET-SAMPLE-FORMAT" "CSOUND-RAND-MT-RANDOM"
             "CSOUND-SET-SCORE-PENDING-1"
             "CSOUND-SET-HOST-IMPLEMENTED-AUDIO-IO" "CSOUND-SET-YIELD-CALLBACK"
             "CSOUND-GET-NCHNLS-1" "CSOUND-MESSAGE-S"
             "CSOUND-GET-SAMPLE-FORMAT-1" "CSOUND-SET-KILL-GRAPH-CALLBACK"
             "CSOUND-SET-EXTERNAL-MIDI-READ-CALLBACK-1"
             "CSOUND-WAIT-THREAD-LOCK-NO-TIMEOUT" "CSOUND-LOCALIZE-STRING"
             "CSOUND-GET-VERSION" "CSOUND-GET-ENV" "CSOUND-KEY-PRESS"
             "CSOUND-REWIND-SCORE-1"
             "CSOUND-SET-EXTERNAL-MIDI-ERROR-STRING-CALLBACK"
             "CSOUND-GET-NCHNLS" "CSOUND-GET-RT-RECORD-USER-DATA-1"
             "CSOUND-SET-CHANNEL-1" "CSOUND-SET-CSCORE-CALLBACK-1"
             "CSOUND-REGISTER-SENSE-EVENT-CALLBACK-1"
             "CSOUND-SET-MAKE-GRAPH-CALLBACK-1"
             "CSOUND-SET-DRAW-GRAPH-CALLBACK-1" "CSOUND-SET-PLAYOPEN-CALLBACK"
             "CSOUND-CREATE-THREAD-LOCK" "CSOUND-GET-SPOUT-1"
             "CSOUND-SET-EXIT-GRAPH-CALLBACK-1" "CSOUND-KEY-PRESS-1"
             "CSOUND-THREAD-LOCK-LOCK" "CSOUND-INITIALIZE-CSCORE"
             "CSOUND-GET-UTILITY-DESCRIPTION-1" "CSOUND-PERFORM-KSMPS"
             "CSOUND-GET-UTILITY-DESCRIPTION" "CSOUND-GET-DEBUG"
             "CSOUND-QUERY-CONFIGURATION-VARIABLE" "CSOUND-RAND-MT-NEW-3"
             "CSOUND-GET-KSMPS-1" "CSOUND-PARSE-CONFIGURATION-VARIABLE"
             "CSOUND-COMPILE" "CSOUND-PERFORM-2" "CSOUND-TABLE-LENGTH"
             "CSOUND-GET-SCORE-OFFSET-SECONDS" "CSOUND-GET-HOST-DATA-1"
             "CSOUND-GET-STR-VAR-MAX-LEN-1" "CSOUND-THREAD-LOCK-NEW-2"
             "CSOUND-DESTROY" "CSOUND-IS-SCORE-PENDING-1"
             "CSOUND-CREATE-CONFIGURATION-VARIABLE" "CSOUND-RAND-MT-NEW-3-"
             "CSOUND-SET-EXTERNAL-MIDI-IN-CLOSE-CALLBACK" "CSOUND-COMPILE-3"
             "CSOUND-NEW-2-" "CSOUND-GET-SCORE-TIME"
             "CSOUND-SET-READ-XYIN-CALLBACK"
             "CSOUND-SET-EXTERNAL-MIDI-IN-CLOSE-CALLBACK-1"
             "CSOUND-GET-STR-VAR-MAX-LEN" "CSOUND-PERFORM-KSMPS-ABSOLUTE-1"
             "CSOUND-GET-OUTPUT-BUFFER-SIZE-1"
             "CSOUND-SET-EXTERNAL-MIDI-OUT-CLOSE-CALLBACK-1" "CSOUND-RESET-1"
             "CSOUND-DELETE-CONFIGURATION-VARIABLE"
             "CSOUND-PERFORM-KSMPS-ABSOLUTE" "CSOUND-SCORE-SORT-1"
             "CSOUND-GET-CONTROL-CHANNEL-PARAMS-1" "CSOUND-LIST-UTILITIES-1"
             "CSOUND-TABLE-SET-1" "CSOUND-IS-SCORE-PENDING"
             "CSOUND-SET-CHANNEL" "CSOUND-GET-REAL-TIME" "CSOUND-GET-TABLE-1"
             "CSOUND-RAND-MT-NEW-1-" "CSOUND-DISPOSE-OPCODE-LIST-1"
             "CSOUND-SET-RTPLAY-CALLBACK-1" "CSOUND-NOTIFY-THREAD-LOCK"
             "CSOUND-SET-OUTPUT-VALUE-CALLBACK"
             "CSOUND-LIST-CONFIGURATION-VARIABLES"
             "CSOUND-SET-DRAW-GRAPH-CALLBACK" "CSOUND-GET-OUTPUT-FILE-NAME-1"
             "CSOUND-GET-OUTPUT-BUFFER-SIZE" "CSOUND-CLEANUP-1"
             "CSOUND-SET-EXTERNAL-MIDI-READ-CALLBACK"
             "CSOUND-CREATE-GLOBAL-VARIABLE" "CSOUND-STOP-1"
             "CSOUND-SET-IS-GRAPHABLE-1"
             "CSOUND-QUERY-GLOBAL-VARIABLE-NO-CHECK"
             "CSOUND-SET-EXTERNAL-MIDI-IN-OPEN-CALLBACK" "CSOUND-PRE-COMPILE"
             "CSOUND-TABLE-GET" "CSOUND-SET-SCORE-OFFSET-SECONDS-1"
             "CSOUND-DELETE-CHANNEL-LIST" "CSOUND-RAND31"
             "CSOUND-SET-EXTERNAL-MIDI-IN-OPEN-CALLBACK-1" "CSOUND-GET-SR-1"
             "CSOUND-GET-SPIN" "CSOUND-GET-RT-PLAY-USER-DATA"
             "CSOUND-SET-HOST-DATA-1" "CSOUND-PERFORM-3" "CSOUND-DELETE"
             "CSOUND-PERFORM-BUFFER-1" "CSOUND-CREATE-GLOBAL-VARIABLE-1"
             "CSOUND-THREAD-LOCK-DELETE"
             "CSOUND-SET-EXTERNAL-MIDI-WRITE-CALLBACK-1" "CSOUND-COMPILE-4"
             "CSOUND-GET-MESSAGE-LEVEL" "CSOUND-GET-TABLE"
             "CSOUND-SET-MAKE-GRAPH-CALLBACK" "CSOUND-MESSAGE"
             "CSOUND-SET-DEBUG" "CSOUND-THREAD-LOCK-TRY-LOCK"
             "CSOUND-GET-SAMPLE-SIZE-1" "CSOUND-SET-PLAYOPEN-CALLBACK-1"
             "CSOUND-OPEN-LIBRARY" "CSOUND-THREAD-LOCK-NEW-2-" "CSOUND-GET-KR"
             "CSOUND-QUERY-INTERFACE" "CSOUND-SET-HOST-DATA"
             "CSOUND-SCORE-EVENT" "CSOUND-CHAN-OKGET-1"
             "CSOUND-SET-EXIT-GRAPH-CALLBACK" "CSOUND-GET-SPIN-1"
             "CSOUND-CHAN-OKGET" "CSOUND-CHAN-IKSET-1"
             "CSOUND-THREAD-LOCK-NEW-1-" "CSOUND-TABLE-LENGTH-1"
             "CSOUND-TABLE-GET-1" "CSOUND-MESSAGE-1" "CSOUND-SET-LANGUAGE"
             "CSOUND-SET-CONTROL-CHANNEL-PARAMS" "CSOUND-SEED-RAND-MT"
             "CSOUND-GET-LIBRARY-SYMBOL" "CSOUND-GET-RANDOM-SEED-FROM-TIME"
             "CSOUND-LIST-UTILITIES" "CSOUND-RESET"
             "CSOUND-GET-MESSAGE-LEVEL-1" "CSOUND-TIMER-GET-REAL-TIME"
             "CSOUND-TIMER-DELETE" "CSOUND-GET-INPUT-BUFFER"
             "CSOUND-INIT-TIMER-STRUCT" "CSOUND-GET-OUTPUT-FILE-NAME"
             "CSOUND-REGISTER-SENSE-EVENT-CALLBACK" "CSOUND-CREATE-THREAD"
             "CSOUND-CLOSE-LIBRARY" "CSOUND-PERFORM-4" "CSOUND-GET-SAMPLE-SIZE"
             "CSOUND-THREAD-LOCK" "CSOUND-TIMER" "CSOUND" "CSOUND-RAND-MT"
             "CSOUND-CONTROL-CHANNEL-EXP" "CSOUNDINIT-NO-ATEXIT" "CS-APISUBVER"
             "CS-APIVERSION" "CSOUND-CONTROL-CHANNEL" "CSOUND-OUTPUT-CHANNEL"
             "CSOUND-CHANNEL-TYPE-MASK" "CSOUNDINIT-NO-SIGNAL-HANDLER"
             "CSOUND-STRING-CHANNEL" "CSOUND-CONTROL-CHANNEL-INT"
             "CSOUND-CONTROL-CHANNEL-LIN" "CSOUND-INPUT-CHANNEL"
             "CSOUND-AUDIO-CHANNEL" "CSLANGUAGE-T" "CSOUND-STATUS"))

(in-package "CSOUND5")
(asdf:operate 'asdf:load-op 'verrazano-support)
(cffi:define-foreign-type csound-1 () 'csound-)

(cffi:defcstruct csound-)
(cffi:defcstruct csound (data :char 8))
(cffi:define-foreign-type file () 's-file)
(cffi:defcstruct sbuf (base :pointer) (size :int))
(cffi:define-foreign-type fpos-t () 'off-t)
(cffi:define-foreign-type off-t () 'quad-t)
(cffi:define-foreign-type quad-t () 'int64-t)
(cffi:define-foreign-type int64-t () ':long)
(cffi:defcstruct s-filex)
(cffi:defcstruct s-file (p :pointer) (r :int) (w :int) (flags :short) 
		 (file :short) (bf sbuf) (lbfsize :int) (cookie :pointer) 
		 (close :pointer) (read :pointer) (seek :pointer) (write :pointer) 
		 (ub sbuf) (extra :pointer) (ur :int) (ubuf :unsigned-char 2) 
		 (nbuf :unsigned-char 0) (lb sbuf) (blksize :int) (offset fpos-t))
(cffi:define-foreign-type va-list () 'gnuc-va-list)
(cffi:define-foreign-type gnuc-va-list () ':pointer)
(cffi:define-foreign-type windat () 'windat-)
(cffi:defcstruct windat-)
(cffi:define-foreign-type xyindat () 'xyindat-)
(cffi:defcstruct xyindat-)
(cffi:defcstruct opcode-list-entry (opname :pointer) (outypes :pointer) (intypes :pointer))
(cffi:defcstruct cs-rt-audio-params (dev-name :pointer) (dev-num :int) (buf-samp-sw :int) (buf-samp-hw :int) (n-channels :int) (sample-format :int) (sample-rate :float))
(cffi:define-foreign-type size-t () ':unsigned-long)
(cffi:define-foreign-type csound-channel-list-entry () 'csound-channel-list-entry-)
(cffi:defcstruct csound-channel-list-entry- (name :pointer) (type :int))
(cffi:define-foreign-type cs-cfg-variable-t () 'cs-cfg-variable-u)
(cffi:define-foreign-type cs-cfg-variable-head-t () 'cs-cfg-variable-head-s)
(cffi:defcstruct cs-cfg-variable-head-s (nxt :pointer) (name :pointer) (p :pointer) (type :int) (flags :int) (short-desc :pointer) (long-desc :pointer))
(cffi:define-foreign-type cs-cfg-variable-int-t () 'cs-cfg-variable-int-s)
(cffi:defcstruct cs-cfg-variable-int-s (nxt :pointer) (name :pointer) (p :pointer) (type :int) (flags :int) (short-desc :pointer) (long-desc :pointer) (min :int) (max :int))
(cffi:define-foreign-type cs-cfg-variable-boolean-t () 'cs-cfg-variable-boolean-s)
(cffi:defcstruct cs-cfg-variable-boolean-s (nxt :pointer) (name :pointer) (p :pointer) (type :int) (flags :int) (short-desc :pointer) (long-desc :pointer))
(cffi:define-foreign-type cs-cfg-variable-float-t () 'cs-cfg-variable-float-s)
(cffi:defcstruct cs-cfg-variable-float-s (nxt :pointer) (name :pointer) (p :pointer) (type :int) (flags :int) (short-desc :pointer) (long-desc :pointer) (min :float) (max :float))
(cffi:define-foreign-type cs-cfg-variable-double-t () 'cs-cfg-variable-double-s)
(cffi:defcstruct cs-cfg-variable-double-s (nxt :pointer) (name :pointer) (p :pointer) (type :int) (flags :int) (short-desc :pointer) (long-desc :pointer) (min :double) (max :double))
(cffi:define-foreign-type cs-cfg-variable-myflt-t () 'cs-cfg-variable-myflt-s)
(cffi:defcstruct cs-cfg-variable-myflt-s (nxt :pointer) (name :pointer) (p :pointer) (type :int) (flags :int) (short-desc :pointer) (long-desc :pointer) (min :float) (max :float))
(cffi:define-foreign-type cs-cfg-variable-string-t () 'cs-cfg-variable-string-s)
(cffi:defcstruct cs-cfg-variable-string-s (nxt :pointer) (name :pointer) (p :pointer) (type :int) (flags :int) (short-desc :pointer) (long-desc :pointer) (maxlen :int))
(cffi:defcunion cs-cfg-variable-u (h cs-cfg-variable-head-t) (i cs-cfg-variable-int-t) (b cs-cfg-variable-boolean-t) (f cs-cfg-variable-float-t) (d cs-cfg-variable-double-t) (m cs-cfg-variable-myflt-t) (s cs-cfg-variable-string-t))
(cffi:defcstruct csound-thread-lock (data :char 8))
(cffi:define-foreign-type csound-rand-mtstate () 'csound-rand-mtstate-)
(cffi:define-foreign-type uint32-t () 'u-int32-t)
(cffi:define-foreign-type u-int32-t () ':unsigned-int)
(cffi:defcstruct csound-rand-mtstate- (mti :int) (mt uint32-t 623))
(cffi:defcstruct csound-rand-mt (data :char 2504))
(cffi:define-foreign-type rtclock () 'rtclock-s)
(cffi:define-foreign-type int-least64-t () 'int64-t)
(cffi:defcstruct rtclock-s (starttime-real int-least64-t) (starttime-cpu int-least64-t))
(cffi:defcstruct csound-timer (data :char 24))

(cffi:defcenum cslanguage-t
               (:cslanguage-default 0)
               (:cslanguage-afrikaans 1)
               (:cslanguage-albanian 2)
               (:cslanguage-arabic 3)
               (:cslanguage-armenian 4)
               (:cslanguage-assamese 5)
               (:cslanguage-azeri 6)
               (:cslanguage-basque 7)
               (:cslanguage-belarusian 8)
               (:cslanguage-bengali 9)
               (:cslanguage-bulgarian 10)
               (:cslanguage-catalan 11)
               (:cslanguage-chinese 12)
               (:cslanguage-croatian 13)
               (:cslanguage-czech 14)
               (:cslanguage-danish 15)
               (:cslanguage-dutch 16)
               (:cslanguage-english-uk 17)
               (:cslanguage-english-us 18)
               (:cslanguage-estonian 19)
               (:cslanguage-faeroese 20)
               (:cslanguage-farsi 21)
               (:cslanguage-finnish 22)
               (:cslanguage-french 23)
               (:cslanguage-georgian 24)
               (:cslanguage-german 25)
               (:cslanguage-greek 26)
               (:cslanguage-gujarati 27)
               (:cslanguage-hebrew 28)
               (:cslanguage-hindi 29)
               (:cslanguage-hungarian 30)
               (:cslanguage-icelandic 31)
               (:cslanguage-indonesian 32)
               (:cslanguage-italian 33)
               (:cslanguage-japanese 34)
               (:cslanguage-kannada 35)
               (:cslanguage-kashmiri 36)
               (:cslanguage-kazak 37)
               (:cslanguage-konkani 38)
               (:cslanguage-korean 39)
               (:cslanguage-latvian 40)
               (:cslanguage-lithuanian 41)
               (:cslanguage-macedonian 42)
               (:cslanguage-malay 43)
               (:cslanguage-malayalam 44)
               (:cslanguage-manipuri 45)
               (:cslanguage-marathi 46)
               (:cslanguage-nepali 47)
               (:cslanguage-norwegian 48)
               (:cslanguage-oriya 49)
               (:cslanguage-polish 50)
               (:cslanguage-portuguese 51)
               (:cslanguage-punjabi 52)
               (:cslanguage-romanian 53)
               (:cslanguage-russian 54)
               (:cslanguage-sanskrit 55)
               (:cslanguage-serbian 56)
               (:cslanguage-sindhi 57)
               (:cslanguage-slovak 58)
               (:cslanguage-slovenian 59)
               (:cslanguage-spanish 60)
               (:cslanguage-swahili 61)
               (:cslanguage-swedish 62)
               (:cslanguage-tamil 63)
               (:cslanguage-tatar 64)
               (:cslanguage-telugu 65)
               (:cslanguage-thai 66)
               (:cslanguage-turkish 67)
               (:cslanguage-ukrainian 68)
               (:cslanguage-urdu 69)
               (:cslanguage-uzbek 70)
               (:cslanguage-vietnamese 71))

(cffi:define-foreign-type uintptr-t () ':unsigned-long)
(cffi:defcenum csound-status (:csound-success 0) (:csound-error -1) (:csound-initialization -2) (:csound-performance -3) (:csound-memory -4) (:csound-signal -5))


(progn (defun csound-pre-compile (this)
         (verrazano-support:virtual-funcall this 2 0 :pointer this :int))
       (defun csound-initialize-cscore (this insco outsco)
         (verrazano-support:virtual-funcall
           this
           3
           0
           :pointer
           this
           :pointer
           insco
           :pointer
           outsco
           :int))
       (defun csound-get-host-data (this)
         (verrazano-support:virtual-funcall this 4 0 :pointer this :pointer))
       (defun csound-set-host-data (this host-data)
         (verrazano-support:virtual-funcall
           this
           5
           0
           :pointer
           this
           :pointer
           host-data
           :void))
       (defun csound-get-env (this name)
         (verrazano-support:virtual-funcall
           this
           6
           0
           :pointer
           this
           :pointer
           name
           :pointer))
       (defun csound-compile (this argc argv)
         (verrazano-support:virtual-funcall
           this
           7
           0
           :pointer
           this
           :int
           argc
           :pointer
           argv
           :int))
       (defun csound-compile-1 (this csd-name)
         (verrazano-support:virtual-funcall
           this
           8
           0
           :pointer
           this
           :pointer
           csd-name
           :int))
       (defun csound-compile-2 (this orc-name sco-name)
         (verrazano-support:virtual-funcall
           this
           9
           0
           :pointer
           this
           :pointer
           orc-name
           :pointer
           sco-name
           :int))
       (defun csound-compile-3 (this arg1 arg2 arg3)
         (verrazano-support:virtual-funcall
           this
           10
           0
           :pointer
           this
           :pointer
           arg1
           :pointer
           arg2
           :pointer
           arg3
           :int))
       (defun csound-compile-4 (this arg1 arg2 arg3 arg4)
         (verrazano-support:virtual-funcall
           this
           11
           0
           :pointer
           this
           :pointer
           arg1
           :pointer
           arg2
           :pointer
           arg3
           :pointer
           arg4
           :int))
       (defun csound-compile-5 (this arg1 arg2 arg3 arg4 arg5)
         (verrazano-support:virtual-funcall
           this
           12
           0
           :pointer
           this
           :pointer
           arg1
           :pointer
           arg2
           :pointer
           arg3
           :pointer
           arg4
           :pointer
           arg5
           :int))
       (defun csound-perform (this)
         (verrazano-support:virtual-funcall this 13 0 :pointer this :int))
       (defun csound-perform-1 (this argc argv)
         (verrazano-support:virtual-funcall
           this
           14
           0
           :pointer
           this
           :int
           argc
           :pointer
           argv
           :int))
       (defun csound-perform-2 (this csd-name)
         (verrazano-support:virtual-funcall
           this
           15
           0
           :pointer
           this
           :pointer
           csd-name
           :int))
       (defun csound-perform-3 (this orc-name sco-name)
         (verrazano-support:virtual-funcall
           this
           16
           0
           :pointer
           this
           :pointer
           orc-name
           :pointer
           sco-name
           :int))
       (defun csound-perform-4 (this arg1 arg2 arg3)
         (verrazano-support:virtual-funcall
           this
           17
           0
           :pointer
           this
           :pointer
           arg1
           :pointer
           arg2
           :pointer
           arg3
           :int))
       (defun csound-perform-5 (this arg1 arg2 arg3 arg4)
         (verrazano-support:virtual-funcall
           this
           18
           0
           :pointer
           this
           :pointer
           arg1
           :pointer
           arg2
           :pointer
           arg3
           :pointer
           arg4
           :int))
       (defun csound-perform-6 (this arg1 arg2 arg3 arg4 arg5)
         (verrazano-support:virtual-funcall
           this
           19
           0
           :pointer
           this
           :pointer
           arg1
           :pointer
           arg2
           :pointer
           arg3
           :pointer
           arg4
           :pointer
           arg5
           :int))
       (defun csound-perform-ksmps (this)
         (verrazano-support:virtual-funcall this 20 0 :pointer this :int))
       (defun csound-perform-ksmps-absolute (this)
         (verrazano-support:virtual-funcall this 21 0 :pointer this :int))
       (defun csound-perform-buffer (this)
         (verrazano-support:virtual-funcall this 22 0 :pointer this :int))
       (defun csound-stop (this)
         (verrazano-support:virtual-funcall this 23 0 :pointer this :void))
       (defun csound-cleanup (this)
         (verrazano-support:virtual-funcall this 24 0 :pointer this :int))
       (defun csound-reset (this)
         (verrazano-support:virtual-funcall this 25 0 :pointer this :void))
       (defun csound-get-sr (this)
         (verrazano-support:virtual-funcall this 26 0 :pointer this :float))
       (defun csound-get-kr (this)
         (verrazano-support:virtual-funcall this 27 0 :pointer this :float))
       (defun csound-get-ksmps (this)
         (verrazano-support:virtual-funcall this 28 0 :pointer this :int))
       (defun csound-get-nchnls (this)
         (verrazano-support:virtual-funcall this 29 0 :pointer this :int))
       (defun csound-get0d-bfs (this)
         (verrazano-support:virtual-funcall this 30 0 :pointer this :float))
       (defun csound-get-str-var-max-len (this)
         (verrazano-support:virtual-funcall this 31 0 :pointer this :int))
       (defun csound-get-sample-format (this)
         (verrazano-support:virtual-funcall this 32 0 :pointer this :int))
       (defun csound-get-sample-size (this)
         (verrazano-support:virtual-funcall this 33 0 :pointer this :int))
       (defun csound-get-input-buffer-size (this)
         (verrazano-support:virtual-funcall this 34 0 :pointer this :long))
       (defun csound-get-output-buffer-size (this)
         (verrazano-support:virtual-funcall this 35 0 :pointer this :long))
       (defun csound-get-input-buffer (this)
         (verrazano-support:virtual-funcall this 36 0 :pointer this :pointer))
       (defun csound-get-output-buffer (this)
         (verrazano-support:virtual-funcall this 37 0 :pointer this :pointer))
       (defun csound-get-spin (this)
         (verrazano-support:virtual-funcall this 38 0 :pointer this :pointer))
       (defun csound-get-spout (this)
         (verrazano-support:virtual-funcall this 39 0 :pointer this :pointer))
       (defun csound-get-output-file-name (this)
         (verrazano-support:virtual-funcall this 40 0 :pointer this :pointer))
       (defun csound-set-host-implemented-audio-io (this state buf-size)
         (verrazano-support:virtual-funcall
           this
           41
           0
           :pointer
           this
           :int
           state
           :int
           buf-size
           :void))
       (defun csound-get-score-time (this)
         (verrazano-support:virtual-funcall this 42 0 :pointer this :float))
       (defun csound-is-score-pending (this)
         (verrazano-support:virtual-funcall this 43 0 :pointer this :int))
       (defun csound-set-score-pending (this pending)
         (verrazano-support:virtual-funcall
           this
           44
           0
           :pointer
           this
           :int
           pending
           :void))
       (defun csound-get-score-offset-seconds (this)
         (verrazano-support:virtual-funcall this 45 0 :pointer this :float))
       (defun csound-set-score-offset-seconds (this time)
         (verrazano-support:virtual-funcall
           this
           46
           0
           :pointer
           this
           :float
           time
           :void))
       (defun csound-rewind-score (this)
         (verrazano-support:virtual-funcall this 47 0 :pointer this :void))
       (defun csound-set-cscore-callback (this cscore-callback-)
         (verrazano-support:virtual-funcall
           this
           48
           0
           :pointer
           this
           :pointer
           cscore-callback-
           :void))
       (defun csound-score-sort (this in-file out-file)
         (verrazano-support:virtual-funcall
           this
           49
           0
           :pointer
           this
           :pointer
           in-file
           :pointer
           out-file
           :int))
       (defun csound-score-extract (this in-file out-file extract-file)
         (verrazano-support:virtual-funcall
           this
           50
           0
           :pointer
           this
           :pointer
           in-file
           :pointer
           out-file
           :pointer
           extract-file
           :int))
       (defun csound-message (this format anonymous4637)
         (verrazano-support:virtual-funcall
           this
           51
           0
           :pointer
           this
           :pointer
           format
           :void))
       (defun csound-message-s (this attr format anonymous4638)
         (verrazano-support:virtual-funcall
           this
           52
           0
           :pointer
           this
           :int
           attr
           :pointer
           format
           :void))
       (defun csound-message-v (this attr format args)
         (verrazano-support:virtual-funcall
           this
           53
           0
           :pointer
           this
           :int
           attr
           :pointer
           format
           va-list
           args
           :void))
       (defun csound-set-message-callback (this csound-message-callback-)
         (verrazano-support:virtual-funcall
           this
           54
           0
           :pointer
           this
           :pointer
           csound-message-callback-
           :void))
       (defun csound-get-message-level (this)
         (verrazano-support:virtual-funcall this 55 0 :pointer this :int))
       (defun csound-set-message-level (this message-level)
         (verrazano-support:virtual-funcall
           this
           56
           0
           :pointer
           this
           :int
           message-level
           :void))
       (defun csound-input-message (this message)
         (verrazano-support:virtual-funcall
           this
           57
           0
           :pointer
           this
           :pointer
           message
           :void))
       (defun csound-key-press (this c)
         (verrazano-support:virtual-funcall
           this
           58
           0
           :pointer
           this
           :char
           c
           :void))
       (defun csound-set-input-value-callback (this input-value-callback-)
         (verrazano-support:virtual-funcall
           this
           59
           0
           :pointer
           this
           :pointer
           input-value-callback-
           :void))
       (defun csound-set-output-value-callback (this output-value-callback-)
         (verrazano-support:virtual-funcall
           this
           60
           0
           :pointer
           this
           :pointer
           output-value-callback-
           :void))
       (defun csound-score-event (this type p-fields num-fields)
         (verrazano-support:virtual-funcall
           this
           61
           0
           :pointer
           this
           :char
           type
           :pointer
           p-fields
           :long
           num-fields
           :int))
       (defun csound-set-external-midi-in-open-callback (this func)
         (verrazano-support:virtual-funcall
           this
           62
           0
           :pointer
           this
           :pointer
           func
           :void))
       (defun csound-set-external-midi-read-callback (this func)
         (verrazano-support:virtual-funcall
           this
           63
           0
           :pointer
           this
           :pointer
           func
           :void))
       (defun csound-set-external-midi-in-close-callback (this func)
         (verrazano-support:virtual-funcall
           this
           64
           0
           :pointer
           this
           :pointer
           func
           :void))
       (defun csound-set-external-midi-out-open-callback (this func)
         (verrazano-support:virtual-funcall
           this
           65
           0
           :pointer
           this
           :pointer
           func
           :void))
       (defun csound-set-external-midi-write-callback (this func)
         (verrazano-support:virtual-funcall
           this
           66
           0
           :pointer
           this
           :pointer
           func
           :void))
       (defun csound-set-external-midi-out-close-callback (this func)
         (verrazano-support:virtual-funcall
           this
           67
           0
           :pointer
           this
           :pointer
           func
           :void))
       (defun csound-set-external-midi-error-string-callback (this func)
         (verrazano-support:virtual-funcall
           this
           68
           0
           :pointer
           this
           :pointer
           func
           :void))
       (defun csound-set-is-graphable (this is-graphable)
         (verrazano-support:virtual-funcall
           this
           69
           0
           :pointer
           this
           :int
           is-graphable
           :int))
       (defun csound-set-make-graph-callback (this make-graph-callback-)
         (verrazano-support:virtual-funcall
           this
           70
           0
           :pointer
           this
           :pointer
           make-graph-callback-
           :void))
       (defun csound-set-draw-graph-callback (this draw-graph-callback-)
         (verrazano-support:virtual-funcall
           this
           71
           0
           :pointer
           this
           :pointer
           draw-graph-callback-
           :void))
       (defun csound-set-kill-graph-callback (this kill-graph-callback-)
         (verrazano-support:virtual-funcall
           this
           72
           0
           :pointer
           this
           :pointer
           kill-graph-callback-
           :void))
       (defun csound-set-make-xyin-callback (this make-xyin-callback-)
         (verrazano-support:virtual-funcall
           this
           73
           0
           :pointer
           this
           :pointer
           make-xyin-callback-
           :void))
       (defun csound-set-read-xyin-callback (this read-xyin-callback-)
         (verrazano-support:virtual-funcall
           this
           74
           0
           :pointer
           this
           :pointer
           read-xyin-callback-
           :void))
       (defun csound-set-kill-xyin-callback (this kill-xyin-callback-)
         (verrazano-support:virtual-funcall
           this
           75
           0
           :pointer
           this
           :pointer
           kill-xyin-callback-
           :void))
       (defun csound-set-exit-graph-callback (this exit-graph-callback-)
         (verrazano-support:virtual-funcall
           this
           76
           0
           :pointer
           this
           :pointer
           exit-graph-callback-
           :void))
       (defun csound-new-opcode-list (this opcodelist)
         (verrazano-support:virtual-funcall
           this
           77
           0
           :pointer
           this
           :pointer
           opcodelist
           :int))
       (defun csound-dispose-opcode-list (this opcodelist)
         (verrazano-support:virtual-funcall
           this
           78
           0
           :pointer
           this
           :pointer
           opcodelist
           :void))
       (defun csound-append-opcode (this opname dsblksiz thread outypes intypes
                                    iopadr kopadr aopadr)
         (verrazano-support:virtual-funcall
           this
           79
           0
           :pointer
           this
           :pointer
           opname
           :int
           dsblksiz
           :int
           thread
           :pointer
           outypes
           :pointer
           intypes
           :pointer
           iopadr
           :pointer
           kopadr
           :pointer
           aopadr
           :int))
       (defun csound-set-yield-callback (this yield-callback-)
         (verrazano-support:virtual-funcall
           this
           80
           0
           :pointer
           this
           :pointer
           yield-callback-
           :void))
       (defun csound-set-playopen-callback (this playopen--)
         (verrazano-support:virtual-funcall
           this
           81
           0
           :pointer
           this
           :pointer
           playopen--
           :void))
       (defun csound-set-rtplay-callback (this rtplay--)
         (verrazano-support:virtual-funcall
           this
           82
           0
           :pointer
           this
           :pointer
           rtplay--
           :void))
       (defun csound-set-recopen-callback (this recopen-)
         (verrazano-support:virtual-funcall
           this
           83
           0
           :pointer
           this
           :pointer
           recopen-
           :void))
       (defun csound-set-rtrecord-callback (this rtrecord--)
         (verrazano-support:virtual-funcall
           this
           84
           0
           :pointer
           this
           :pointer
           rtrecord--
           :void))
       (defun csound-set-rtclose-callback (this rtclose--)
         (verrazano-support:virtual-funcall
           this
           85
           0
           :pointer
           this
           :pointer
           rtclose--
           :void))
       (defun csound-get-debug (this)
         (verrazano-support:virtual-funcall this 86 0 :pointer this :int))
       (defun csound-set-debug (this debug)
         (verrazano-support:virtual-funcall
           this
           87
           0
           :pointer
           this
           :int
           debug
           :void))
       (defun csound-table-length (this table)
         (verrazano-support:virtual-funcall
           this
           88
           0
           :pointer
           this
           :int
           table
           :int))
       (defun csound-table-get (this table index)
         (verrazano-support:virtual-funcall
           this
           89
           0
           :pointer
           this
           :int
           table
           :int
           index
           :float))
       (defun csound-table-set (this table index value)
         (verrazano-support:virtual-funcall
           this
           90
           0
           :pointer
           this
           :int
           table
           :int
           index
           :float
           value
           :void))
       (defun csound-get-table (this table-num table-length)
         (verrazano-support:virtual-funcall
           this
           91
           0
           :pointer
           this
           :int
           table-num
           :pointer
           table-length
           :pointer))
       (defun csound-create-global-variable (this name nbytes)
         (verrazano-support:virtual-funcall
           this
           92
           0
           :pointer
           this
           :pointer
           name
           size-t
           nbytes
           :int))
       (defun csound-query-global-variable (this name)
         (verrazano-support:virtual-funcall
           this
           93
           0
           :pointer
           this
           :pointer
           name
           :pointer))
       (defun csound-query-global-variable-no-check (this name)
         (verrazano-support:virtual-funcall
           this
           94
           0
           :pointer
           this
           :pointer
           name
           :pointer))
       (defun csound-destroy-global-variable (this name)
         (verrazano-support:virtual-funcall
           this
           95
           0
           :pointer
           this
           :pointer
           name
           :int))
       (defun csound-get-rt-record-user-data (this)
         (verrazano-support:virtual-funcall this 96 0 :pointer this :pointer))
       (defun csound-get-rt-play-user-data (this)
         (verrazano-support:virtual-funcall this 97 0 :pointer this :pointer))
       (defun csound-register-sense-event-callback (this func user-data)
         (verrazano-support:virtual-funcall
           this
           98
           0
           :pointer
           this
           :pointer
           func
           :pointer
           user-data
           :int))
       (defun csound-run-utility (this name argc argv)
         (verrazano-support:virtual-funcall
           this
           99
           0
           :pointer
           this
           :pointer
           name
           :int
           argc
           :pointer
           argv
           :int))
       (defun csound-list-utilities (this)
         (verrazano-support:virtual-funcall this 100 0 :pointer this :pointer))
       (defun csound-delete-utility-list (this lst)
         (verrazano-support:virtual-funcall
           this
           101
           0
           :pointer
           this
           :pointer
           lst
           :void))
       (defun csound-get-utility-description (this util-name)
         (verrazano-support:virtual-funcall
           this
           102
           0
           :pointer
           this
           :pointer
           util-name
           :pointer))
       (defun csound-get-channel-ptr (this p name type)
         (verrazano-support:virtual-funcall
           this
           103
           0
           :pointer
           this
           :pointer
           p
           :pointer
           name
           :int
           type
           :int))
       (defun csound-list-channels (this lst)
         (verrazano-support:virtual-funcall
           this
           104
           0
           :pointer
           this
           :pointer
           lst
           :int))
       (defun csound-delete-channel-list (this lst)
         (verrazano-support:virtual-funcall
           this
           105
           0
           :pointer
           this
           :pointer
           lst
           :void))
       (defun csound-set-control-channel-params (this name type dflt min max)
         (verrazano-support:virtual-funcall
           this
           106
           0
           :pointer
           this
           :pointer
           name
           :int
           type
           :float
           dflt
           :float
           min
           :float
           max
           :int))
       (defun csound-get-control-channel-params (this name dflt min max)
         (verrazano-support:virtual-funcall
           this
           107
           0
           :pointer
           this
           :pointer
           name
           :pointer
           dflt
           :pointer
           min
           :pointer
           max
           :int))
       (defun csound-set-channel (this name value)
         (verrazano-support:virtual-funcall
           this
           108
           0
           :pointer
           this
           :pointer
           name
           :float
           value
           :void))
       (defun csound-set-channel-1 (this name value)
         (verrazano-support:virtual-funcall
           this
           109
           0
           :pointer
           this
           :pointer
           name
           :pointer
           value
           :void))
       (defun csound-get-channel (this name)
         (verrazano-support:virtual-funcall
           this
           110
           0
           :pointer
           this
           :pointer
           name
           :float))
       (defun csound-chan-ikset (this value n)
         (verrazano-support:virtual-funcall
           this
           111
           0
           :pointer
           this
           :float
           value
           :int
           n
           :int))
       (defun csound-chan-okget (this value n)
         (verrazano-support:virtual-funcall
           this
           112
           0
           :pointer
           this
           :pointer
           value
           :int
           n
           :int))
       (defun csound-chan-iaset (this value n)
         (verrazano-support:virtual-funcall
           this
           113
           0
           :pointer
           this
           :pointer
           value
           :int
           n
           :int))
       (defun csound-chan-oaget (this value n)
         (verrazano-support:virtual-funcall
           this
           114
           0
           :pointer
           this
           :pointer
           value
           :int
           n
           :int))
       (defun csound-create-configuration-variable (this
                                                    name
                                                    p
                                                    type
                                                    flags
                                                    min
                                                    max
                                                    short-desc
                                                    long-desc)
         (verrazano-support:virtual-funcall
           this
           115
           0
           :pointer
           this
           :pointer
           name
           :pointer
           p
           :int
           type
           :int
           flags
           :pointer
           min
           :pointer
           max
           :pointer
           short-desc
           :pointer
           long-desc
           :int))
       (defun csound-copy-global-configuration-variable (this name p)
         (verrazano-support:virtual-funcall
           this
           116
           0
           :pointer
           this
           :pointer
           name
           :pointer
           p
           :int))
       (defun csound-copy-global-configuration-variables (this)
         (verrazano-support:virtual-funcall this 117 0 :pointer this :int))
       (defun csound-set-configuration-variable (this name value)
         (verrazano-support:virtual-funcall
           this
           118
           0
           :pointer
           this
           :pointer
           name
           :pointer
           value
           :int))
       (defun csound-parse-configuration-variable (this name value)
         (verrazano-support:virtual-funcall
           this
           119
           0
           :pointer
           this
           :pointer
           name
           :pointer
           value
           :int))
       (defun csound-query-configuration-variable (this name)
         (verrazano-support:virtual-funcall
           this
           120
           0
           :pointer
           this
           :pointer
           name
           :pointer))
       (defun csound-list-configuration-variables (this)
         (verrazano-support:virtual-funcall this 121 0 :pointer this :pointer))
       (defun csound-delete-configuration-variable (this name)
         (verrazano-support:virtual-funcall
           this
           122
           0
           :pointer
           this
           :pointer
           name
           :int))
       (progn (cffi:defcfun ("_ZN6CsoundC1Ev" csound-new-1-)
                            :void
                            (this :pointer))
              (defun csound-new-1 ()
                (let ((-pobj- (cffi:foreign-alloc 'csound)))
                  (csound-new-1- -pobj-)
                  -pobj-)))
       (progn (cffi:defcfun ("_ZN6CsoundC1EPv" csound-new-2-)
                            :void
                            (this :pointer)
                            (host-data :pointer))
              (defun csound-new-2 (host-data)
                (let ((-pobj- (cffi:foreign-alloc 'csound)))
                  (csound-new-2- -pobj- host-data)
                  -pobj-)))
       (defun csound-delete (this)
         (verrazano-support:virtual-funcall this 124 0 :pointer this :void))
       (defun csound-thread-lock-lock (this milliseconds)
         (verrazano-support:virtual-funcall
           this
           2
           0
           :pointer
           this
           size-t
           milliseconds
           :int))
       (defun csound-thread-lock-lock-1 (this)
         (verrazano-support:virtual-funcall this 3 0 :pointer this :void))
       (defun csound-thread-lock-try-lock (this)
         (verrazano-support:virtual-funcall this 4 0 :pointer this :int))
       (defun csound-thread-lock-unlock (this)
         (verrazano-support:virtual-funcall this 5 0 :pointer this :void))
       (progn (cffi:defcfun ("_ZN16CsoundThreadLockC1Ev"
                             csound-thread-lock-new-1-)
                            :void
                            (this :pointer))
              (defun csound-thread-lock-new-1 ()
                (let ((-pobj- (cffi:foreign-alloc 'csound-thread-lock)))
                  (csound-thread-lock-new-1- -pobj-)
                  -pobj-)))
       (progn (cffi:defcfun ("_ZN16CsoundThreadLockC1Ei"
                             csound-thread-lock-new-2-)
                            :void
                            (this :pointer)
                            (locked :int))
              (defun csound-thread-lock-new-2 (locked)
                (let ((-pobj- (cffi:foreign-alloc 'csound-thread-lock)))
                  (csound-thread-lock-new-2- -pobj- locked)
                  -pobj-)))
       (defun csound-thread-lock-delete (this)
         (verrazano-support:virtual-funcall this 7 0 :pointer this :void))
       (defun csound-rand-mt-random (this)
         (verrazano-support:virtual-funcall this 2 0 :pointer this uint32-t))
       (defun csound-rand-mt-seed (this seed-val)
         (verrazano-support:virtual-funcall
           this
           3
           0
           :pointer
           this
           uint32-t
           seed-val
           :void))
       (defun csound-rand-mt-seed-1 (this init-key key-length)
         (verrazano-support:virtual-funcall
           this
           4
           0
           :pointer
           this
           :pointer
           init-key
           :int
           key-length
           :void))
       (progn (cffi:defcfun ("_ZN12CsoundRandMTC1Ev" csound-rand-mt-new-1-)
                            :void
                            (this :pointer))
              (defun csound-rand-mt-new-1 ()
                (let ((-pobj- (cffi:foreign-alloc 'csound-rand-mt)))
                  (csound-rand-mt-new-1- -pobj-)
                  -pobj-)))
       (progn (cffi:defcfun ("_ZN12CsoundRandMTC1Ej" csound-rand-mt-new-2-)
                            :void
                            (this :pointer)
                            (seed-val uint32-t))
              (defun csound-rand-mt-new-2 (seed-val)
                (let ((-pobj- (cffi:foreign-alloc 'csound-rand-mt)))
                  (csound-rand-mt-new-2- -pobj- seed-val)
                  -pobj-)))
       (progn (cffi:defcfun ("_ZN12CsoundRandMTC1EPKji" csound-rand-mt-new-3-)
                            :void
                            (this :pointer)
                            (init-key :pointer)
                            (key-length :int))
              (defun csound-rand-mt-new-3 (init-key key-length)
                (let ((-pobj- (cffi:foreign-alloc 'csound-rand-mt)))
                  (csound-rand-mt-new-3- -pobj- init-key key-length)
                  -pobj-)))
       (defun csound-rand-mt-delete (this)
         (verrazano-support:virtual-funcall this 6 0 :pointer this :void))
       (defun csound-timer-get-real-time (this)
         (verrazano-support:virtual-funcall this 2 0 :pointer this :double))
       (defun csound-timer-get-cputime (this)
         (verrazano-support:virtual-funcall this 3 0 :pointer this :double))
       (defun csound-timer-reset (this)
         (verrazano-support:virtual-funcall this 4 0 :pointer this :void))
       (progn (cffi:defcfun ("_ZN11CsoundTimerC1Ev" csound-timer-new-1-)
                            :void
                            (this :pointer))
              (defun csound-timer-new-1 ()
                (let ((-pobj- (cffi:foreign-alloc 'csound-timer)))
                  (csound-timer-new-1- -pobj-)
                  -pobj-)))
       (defun csound-timer-delete (this)
         (verrazano-support:virtual-funcall this 6 0 :pointer this :void))
       (cffi:defcfun ("csoundChanOAGet" csound-chan-oaget-1)
                     :int
                     (anonymous4685 :pointer)
                     (value :pointer)
                     (n :int))
       (cffi:defcfun ("csoundChanIASet" csound-chan-iaset-1)
                     :int
                     (anonymous4686 :pointer)
                     (value :pointer)
                     (n :int))
       (cffi:defcfun ("csoundChanOKGet" csound-chan-okget-1)
                     :int
                     (anonymous4687 :pointer)
                     (value :pointer)
                     (n :int))
       (cffi:defcfun ("csoundChanIKSet" csound-chan-ikset-1)
                     :int
                     (anonymous4688 :pointer)
                     (value :float)
                     (n :int))
       (cffi:defcfun ("csoundRandMT" csound-rand-mt) uint32-t (p :pointer))
       (cffi:defcfun ("csoundSeedRandMT" csound-seed-rand-mt)
                     :void
                     (p :pointer)
                     (init-key :pointer)
                     (key-length uint32-t))
       (cffi:defcfun ("csoundRand31" csound-rand31) :int (seed-val :pointer))
       (cffi:defcfun ("csoundGetControlChannelParams"
                      csound-get-control-channel-params-1)
                     :int
                     (anonymous4689 :pointer)
                     (name :pointer)
                     (dflt :pointer)
                     (min :pointer)
                     (max :pointer))
       (cffi:defcfun ("csoundSetControlChannelParams"
                      csound-set-control-channel-params-1)
                     :int
                     (anonymous4690 :pointer)
                     (name :pointer)
                     (type :int)
                     (dflt :float)
                     (min :float)
                     (max :float))
       (cffi:defcfun ("csoundDeleteChannelList" csound-delete-channel-list-1)
                     :void
                     (anonymous4691 :pointer)
                     (lst :pointer))
       (cffi:defcfun ("csoundListChannels" csound-list-channels-1)
                     :int
                     (anonymous4692 :pointer)
                     (lst :pointer))
       (cffi:defcfun ("csoundGetChannelPtr" csound-get-channel-ptr-1)
                     :int
                     (anonymous4693 :pointer)
                     (p :pointer)
                     (name :pointer)
                     (type :int))
       (cffi:defcfun ("csoundGetUtilityDescription"
                      csound-get-utility-description-1)
                     :pointer
                     (anonymous4694 :pointer)
                     (util-name :pointer))
       (cffi:defcfun ("csoundDeleteUtilityList" csound-delete-utility-list-1)
                     :void
                     (anonymous4695 :pointer)
                     (lst :pointer))
       (cffi:defcfun ("csoundListUtilities" csound-list-utilities-1)
                     :pointer
                     (anonymous4696 :pointer))
       (cffi:defcfun ("csoundRunUtility" csound-run-utility-1)
                     :int
                     (anonymous4697 :pointer)
                     (name :pointer)
                     (argc :int)
                     (argv :pointer))
       (cffi:defcfun ("csoundRegisterSenseEventCallback"
                      csound-register-sense-event-callback-1)
                     :int
                     (anonymous4698 :pointer)
                     (func :pointer)
                     (user-data :pointer))
       (cffi:defcfun ("csoundGetRtPlayUserData" csound-get-rt-play-user-data-1)
                     :pointer
                     (anonymous4699 :pointer))
       (cffi:defcfun ("csoundGetRtRecordUserData"
                      csound-get-rt-record-user-data-1)
                     :pointer
                     (anonymous4700 :pointer))
       (cffi:defcfun ("csoundGetSizeOfMYFLT" csound-get-size-of-myflt) :int)
       (cffi:defcfun ("csoundDestroyGlobalVariable"
                      csound-destroy-global-variable-1)
                     :int
                     (anonymous4701 :pointer)
                     (name :pointer))
       (cffi:defcfun ("csoundQueryGlobalVariableNoCheck"
                      csound-query-global-variable-no-check-1)
                     :pointer
                     (anonymous4702 :pointer)
                     (name :pointer))
       (cffi:defcfun ("csoundQueryGlobalVariable"
                      csound-query-global-variable-1)
                     :pointer
                     (anonymous4703 :pointer)
                     (name :pointer))
       (cffi:defcfun ("csoundCreateGlobalVariable"
                      csound-create-global-variable-1)
                     :int
                     (anonymous4704 :pointer)
                     (name :pointer)
                     (nbytes size-t))
       (cffi:defcfun ("csoundLocalizeString" csound-localize-string)
                     :pointer
                     (s :pointer))
       (cffi:defcfun ("csoundSetLanguage" csound-set-language)
                     :void
                     (lang-code cslanguage-t))
       (cffi:defcfun ("csoundGetRandomSeedFromTime"
                      csound-get-random-seed-from-time)
                     uint32-t)
       (cffi:defcfun ("csoundGetCPUTime" csound-get-cputime)
                     :double
                     (anonymous4705 :pointer))
       (cffi:defcfun ("csoundGetRealTime" csound-get-real-time)
                     :double
                     (anonymous4706 :pointer))
       (cffi:defcfun ("csoundInitTimerStruct" csound-init-timer-struct)
                     :void
                     (anonymous4707 :pointer))
       (cffi:defcfun ("csoundSleep" csound-sleep) :void (milliseconds size-t))
       (cffi:defcfun ("csoundDestroyThreadLock" csound-destroy-thread-lock)
                     :void
                     (lock :pointer))
       (cffi:defcfun ("csoundNotifyThreadLock" csound-notify-thread-lock)
                     :void
                     (lock :pointer))
       (cffi:defcfun ("csoundWaitThreadLockNoTimeout"
                      csound-wait-thread-lock-no-timeout)
                     :void
                     (lock :pointer))
       (cffi:defcfun ("csoundWaitThreadLock" csound-wait-thread-lock)
                     :int
                     (lock :pointer)
                     (milliseconds size-t))
       (cffi:defcfun ("csoundCreateThreadLock" csound-create-thread-lock)
                     :pointer)
       (cffi:defcfun ("csoundJoinThread" csound-join-thread)
                     uintptr-t
                     (thread :pointer))
       (cffi:defcfun ("csoundCreateThread" csound-create-thread)
                     :pointer
                     (thread-routine :pointer)
                     (userdata :pointer))
       (cffi:defcfun ("csoundGetTable" csound-get-table-1)
                     :pointer
                     (anonymous4709 :pointer)
                     (table-num :int)
                     (table-length :pointer))
       (cffi:defcfun ("csoundTableSet" csound-table-set-1)
                     :void
                     (anonymous4710 :pointer)
                     (table :int)
                     (index :int)
                     (value :float))
       (cffi:defcfun ("csoundTableGet" csound-table-get-1)
                     :float
                     (anonymous4711 :pointer)
                     (table :int)
                     (index :int))
       (cffi:defcfun ("csoundTableLength" csound-table-length-1)
                     :int
                     (anonymous4712 :pointer)
                     (table :int))
       (cffi:defcfun ("csoundSetDebug" csound-set-debug-1)
                     :void
                     (anonymous4713 :pointer)
                     (debug :int))
       (cffi:defcfun ("csoundGetDebug" csound-get-debug-1)
                     :int
                     (anonymous4714 :pointer))
       (cffi:defcfun ("csoundSetRtcloseCallback" csound-set-rtclose-callback-1)
                     :void
                     (anonymous4715 :pointer)
                     (rtclose-- :pointer))
       (cffi:defcfun ("csoundSetRtrecordCallback"
                      csound-set-rtrecord-callback-1)
                     :void
                     (anonymous4716 :pointer)
                     (rtrecord-- :pointer))
       (cffi:defcfun ("csoundSetRecopenCallback" csound-set-recopen-callback-1)
                     :void
                     (anonymous4717 :pointer)
                     (recopen- :pointer))
       (cffi:defcfun ("csoundSetRtplayCallback" csound-set-rtplay-callback-1)
                     :void
                     (anonymous4718 :pointer)
                     (rtplay-- :pointer))
       (cffi:defcfun ("csoundSetPlayopenCallback"
                      csound-set-playopen-callback-1)
                     :void
                     (anonymous4719 :pointer)
                     (playopen-- :pointer))
       (cffi:defcfun ("csoundSetYieldCallback" csound-set-yield-callback-1)
                     :void
                     (anonymous4720 :pointer)
                     (yield-callback- :pointer))
       (cffi:defcfun ("csoundGetLibrarySymbol" csound-get-library-symbol)
                     :pointer
                     (library :pointer)
                     (symbol-name :pointer))
       (cffi:defcfun ("csoundCloseLibrary" csound-close-library)
                     :int
                     (library :pointer))
       (cffi:defcfun ("csoundOpenLibrary" csound-open-library)
                     :int
                     (library :pointer)
                     (library-path :pointer))
       (cffi:defcfun ("csoundAppendOpcode" csound-append-opcode-1)
                     :int
                     (anonymous4721 :pointer)
                     (opname :pointer)
                     (dsblksiz :int)
                     (thread :int)
                     (outypes :pointer)
                     (intypes :pointer)
                     (iopadr :pointer)
                     (kopadr :pointer)
                     (aopadr :pointer))
       (cffi:defcfun ("csoundDisposeOpcodeList" csound-dispose-opcode-list-1)
                     :void
                     (anonymous4722 :pointer)
                     (opcodelist :pointer))
       (cffi:defcfun ("csoundNewOpcodeList" csound-new-opcode-list-1)
                     :int
                     (anonymous4723 :pointer)
                     (opcodelist :pointer))
       (cffi:defcfun ("csoundSetExitGraphCallback"
                      csound-set-exit-graph-callback-1)
                     :void
                     (anonymous4724 :pointer)
                     (exit-graph-callback- :pointer))
       (cffi:defcfun ("csoundSetKillXYinCallback"
                      csound-set-kill-xyin-callback-1)
                     :void
                     (anonymous4725 :pointer)
                     (kill-xyin-callback- :pointer))
       (cffi:defcfun ("csoundSetReadXYinCallback"
                      csound-set-read-xyin-callback-1)
                     :void
                     (anonymous4726 :pointer)
                     (read-xyin-callback- :pointer))
       (cffi:defcfun ("csoundSetMakeXYinCallback"
                      csound-set-make-xyin-callback-1)
                     :void
                     (anonymous4727 :pointer)
                     (make-xyin-callback- :pointer))
       (cffi:defcfun ("csoundSetKillGraphCallback"
                      csound-set-kill-graph-callback-1)
                     :void
                     (anonymous4728 :pointer)
                     (kill-graph-callback- :pointer))
       (cffi:defcfun ("csoundSetDrawGraphCallback"
                      csound-set-draw-graph-callback-1)
                     :void
                     (anonymous4729 :pointer)
                     (draw-graph-callback- :pointer))
       (cffi:defcfun ("csoundSetMakeGraphCallback"
                      csound-set-make-graph-callback-1)
                     :void
                     (anonymous4730 :pointer)
                     (make-graph-callback- :pointer))
       (cffi:defcfun ("csoundSetIsGraphable" csound-set-is-graphable-1)
                     :int
                     (anonymous4731 :pointer)
                     (is-graphable :int))
       (cffi:defcfun ("csoundSetExternalMidiErrorStringCallback"
                      csound-set-external-midi-error-string-callback-1)
                     :void
                     (anonymous4732 :pointer)
                     (func :pointer))
       (cffi:defcfun ("csoundSetExternalMidiOutCloseCallback"
                      csound-set-external-midi-out-close-callback-1)
                     :void
                     (anonymous4733 :pointer)
                     (func :pointer))
       (cffi:defcfun ("csoundSetExternalMidiWriteCallback"
                      csound-set-external-midi-write-callback-1)
                     :void
                     (anonymous4734 :pointer)
                     (func :pointer))
       (cffi:defcfun ("csoundSetExternalMidiOutOpenCallback"
                      csound-set-external-midi-out-open-callback-1)
                     :void
                     (anonymous4735 :pointer)
                     (func :pointer))
       (cffi:defcfun ("csoundSetExternalMidiInCloseCallback"
                      csound-set-external-midi-in-close-callback-1)
                     :void
                     (anonymous4736 :pointer)
                     (func :pointer))
       (cffi:defcfun ("csoundSetExternalMidiReadCallback"
                      csound-set-external-midi-read-callback-1)
                     :void
                     (anonymous4737 :pointer)
                     (func :pointer))
       (cffi:defcfun ("csoundSetExternalMidiInOpenCallback"
                      csound-set-external-midi-in-open-callback-1)
                     :void
                     (anonymous4738 :pointer)
                     (func :pointer))
       (cffi:defcfun ("csoundScoreEvent" csound-score-event-1)
                     :int
                     (anonymous4739 :pointer)
                     (type :char)
                     (p-fields :pointer)
                     (num-fields :long))
       (cffi:defcfun ("csoundSetOutputValueCallback"
                      csound-set-output-value-callback-1)
                     :void
                     (anonymous4740 :pointer)
                     (output-value-calback- :pointer))
       (cffi:defcfun ("csoundSetInputValueCallback"
                      csound-set-input-value-callback-1)
                     :void
                     (anonymous4741 :pointer)
                     (input-value-calback- :pointer))
       (cffi:defcfun ("csoundKeyPress" csound-key-press-1)
                     :void
                     (anonymous4742 :pointer)
                     (c :char))
       (cffi:defcfun ("csoundInputMessage" csound-input-message-1)
                     :void
                     (anonymous4743 :pointer)
                     (message :pointer))
       (cffi:defcfun ("csoundSetMessageLevel" csound-set-message-level-1)
                     :void
                     (anonymous4744 :pointer)
                     (message-level :int))
       (cffi:defcfun ("csoundGetMessageLevel" csound-get-message-level-1)
                     :int
                     (anonymous4745 :pointer))
       (cffi:defcfun ("csoundSetMessageCallback" csound-set-message-callback-1)
                     :void
                     (anonymous4746 :pointer)
                     (csound-message-callback- :pointer))
       (cffi:defcfun ("csoundMessageV" csound-message-v-1)
                     :void
                     (anonymous4747 :pointer)
                     (attr :int)
                     (format :pointer)
                     (args va-list))
       (cffi:defcfun ("csoundMessageS" csound-message-s-1)
                     :void
                     (anonymous4748 :pointer)
                     (attr :int)
                     (format :pointer))
       (cffi:defcfun ("csoundMessage" csound-message-1)
                     :void
                     (anonymous4750 :pointer)
                     (format :pointer))
       (cffi:defcfun ("csoundScoreExtract" csound-score-extract-1)
                     :int
                     (anonymous4752 :pointer)
                     (in-file :pointer)
                     (out-file :pointer)
                     (extract-file :pointer))
       (cffi:defcfun ("csoundScoreSort" csound-score-sort-1)
                     :int
                     (anonymous4753 :pointer)
                     (in-file :pointer)
                     (out-file :pointer))
       (cffi:defcfun ("csoundSetCscoreCallback" csound-set-cscore-callback-1)
                     :void
                     (anonymous4754 :pointer)
                     (cscore-callback- :pointer))
       (cffi:defcfun ("csoundRewindScore" csound-rewind-score-1)
                     :void
                     (anonymous4755 :pointer))
       (cffi:defcfun ("csoundSetScoreOffsetSeconds"
                      csound-set-score-offset-seconds-1)
                     :void
                     (anonymous4756 :pointer)
                     (time :float))
       (cffi:defcfun ("csoundGetScoreOffsetSeconds"
                      csound-get-score-offset-seconds-1)
                     :float
                     (anonymous4757 :pointer))
       (cffi:defcfun ("csoundSetScorePending" csound-set-score-pending-1)
                     :void
                     (anonymous4758 :pointer)
                     (pending :int))
       (cffi:defcfun ("csoundIsScorePending" csound-is-score-pending-1)
                     :int
                     (anonymous4759 :pointer))
       (cffi:defcfun ("csoundGetScoreTime" csound-get-score-time-1)
                     :float
                     (anonymous4760 :pointer))
       (cffi:defcfun ("csoundSetHostImplementedAudioIO"
                      csound-set-host-implemented-audio-io-1)
                     :void
                     (anonymous4761 :pointer)
                     (state :int)
                     (buf-size :int))
       (cffi:defcfun ("csoundGetOutputFileName" csound-get-output-file-name-1)
                     :pointer
                     (anonymous4762 :pointer))
       (cffi:defcfun ("csoundGetSpout" csound-get-spout-1)
                     :pointer
                     (anonymous4763 :pointer))
       (cffi:defcfun ("csoundGetSpin" csound-get-spin-1)
                     :pointer
                     (anonymous4764 :pointer))
       (cffi:defcfun ("csoundGetOutputBuffer" csound-get-output-buffer-1)
                     :pointer
                     (anonymous4765 :pointer))
       (cffi:defcfun ("csoundGetInputBuffer" csound-get-input-buffer-1)
                     :pointer
                     (anonymous4766 :pointer))
       (cffi:defcfun ("csoundGetOutputBufferSize"
                      csound-get-output-buffer-size-1)
                     :long
                     (anonymous4767 :pointer))
       (cffi:defcfun ("csoundGetInputBufferSize"
                      csound-get-input-buffer-size-1)
                     :long
                     (anonymous4768 :pointer))
       (cffi:defcfun ("csoundGetSampleSize" csound-get-sample-size-1)
                     :int
                     (anonymous4769 :pointer))
       (cffi:defcfun ("csoundGetSampleFormat" csound-get-sample-format-1)
                     :int
                     (anonymous4770 :pointer))
       (cffi:defcfun ("csoundGetStrVarMaxLen" csound-get-str-var-max-len-1)
                     :int
                     (anonymous4771 :pointer))
       (cffi:defcfun ("csoundGet0dBFS" csound-get0d-bfs-1)
                     :float
                     (anonymous4772 :pointer))
       (cffi:defcfun ("csoundGetNchnls" csound-get-nchnls-1)
                     :int
                     (anonymous4773 :pointer))
       (cffi:defcfun ("csoundGetKsmps" csound-get-ksmps-1)
                     :int
                     (anonymous4774 :pointer))
       (cffi:defcfun ("csoundGetKr" csound-get-kr-1)
                     :float
                     (anonymous4775 :pointer))
       (cffi:defcfun ("csoundGetSr" csound-get-sr-1)
                     :float
                     (anonymous4776 :pointer))
       (cffi:defcfun ("csoundReset" csound-reset-1)
                     :void
                     (anonymous4777 :pointer))
       (cffi:defcfun ("csoundCleanup" csound-cleanup-1)
                     :int
                     (anonymous4778 :pointer))
       (cffi:defcfun ("csoundStop" csound-stop-1)
                     :void
                     (anonymous4779 :pointer))
       (cffi:defcfun ("csoundPerformBuffer" csound-perform-buffer-1)
                     :int
                     (anonymous4780 :pointer))
       (cffi:defcfun ("csoundPerformKsmpsAbsolute"
                      csound-perform-ksmps-absolute-1)
                     :int
                     (anonymous4781 :pointer))
       (cffi:defcfun ("csoundPerformKsmps" csound-perform-ksmps-1)
                     :int
                     (anonymous4782 :pointer))
       (cffi:defcfun ("csoundPerform" csound-perform-1)
                     :int
                     (anonymous4783 :pointer))
       (cffi:defcfun ("csoundCompile" csound-compile-1)
                     :int
                     (anonymous4784 :pointer)
                     (argc :int)
                     (argv :pointer))
       (cffi:defcfun ("csoundSetGlobalEnv" csound-set-global-env)
                     :int
                     (name :pointer)
                     (value :pointer))
       (cffi:defcfun ("csoundGetEnv" csound-get-env-1)
                     :pointer
                     (csound :pointer)
                     (name :pointer))
       (cffi:defcfun ("csoundSetHostData" csound-set-host-data-1)
                     :void
                     (anonymous4785 :pointer)
                     (host-data :pointer))
       (cffi:defcfun ("csoundGetHostData" csound-get-host-data-1)
                     :pointer
                     (anonymous4786 :pointer))
       (cffi:defcfun ("csoundGetAPIVersion" csound-get-apiversion) :int)
       (cffi:defcfun ("csoundGetVersion" csound-get-version) :int)
       (cffi:defcfun ("csoundDestroy" csound-destroy)
                     :void
                     (anonymous4787 :pointer))
       (cffi:defcfun ("csoundQueryInterface" csound-query-interface)
                     :int
                     (name :pointer)
                     (iface :pointer)
                     (version :pointer))
       (cffi:defcfun ("csoundInitializeCscore" csound-initialize-cscore-1)
                     :int
                     (anonymous4788 :pointer)
                     (insco :pointer)
                     (outsco :pointer))
       (cffi:defcfun ("csoundPreCompile" csound-pre-compile-1)
                     :int
                     (anonymous4789 :pointer))
       (cffi:defcfun ("csoundCreate" csound-create)
                     :pointer
                     (host-data :pointer))
       (cffi:defcfun ("csoundInitialize" csound-initialize)
                     :int
                     (argc :pointer)
                     (argv :pointer)
                     (flags :int))
       (defparameter cs-apisubver 0)
       (defparameter cs-apiversion 1)
       (defparameter csound-control-channel-exp 3)
       (defparameter csound-control-channel-lin 2)
       (defparameter csound-control-channel-int 1)
       (defparameter csound-output-channel 32)
       (defparameter csound-input-channel 16)
       (defparameter csound-channel-type-mask 15)
       (defparameter csound-string-channel 3)
       (defparameter csound-audio-channel 2)
       (defparameter csound-control-channel 1)
       (defparameter csoundinit-no-atexit 2)
       (defparameter csoundinit-no-signal-handler 1))
