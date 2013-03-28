(defpackage :csound
    (:use :common-lisp :cffi))

(in-package :csound)

;;;SWIG wrapper code starts here

(defmacro defanonenum (&body enums)
   "Converts anonymous enums to defconstants."
  `(progn ,@(loop for value in enums
                  for index = 0 then (1+ index)
                  when (listp value) do (setf index (second value)
                                              value (first value))
                  collect `(defconstant ,value ,index))))

;;;SWIG wrapper code ends here


(defcenum CSOUND_STATUS
	(:CSOUND_SUCCESS 0)
	(:CSOUND_ERROR -1)
	(:CSOUND_INITIALIZATION -2)
	(:CSOUND_PERFORMANCE -3)
	(:CSOUND_MEMORY -4)
	(:CSOUND_SIGNAL -5))

(defctype CSOUND_STATUS :pointer)

(defconstant CSOUND_EXITJMP_SUCCESS 256)

(defconstant CSOUNDINIT_NO_SIGNAL_HANDLER 1)

(defconstant CSOUNDINIT_NO_ATEXIT 2)

(defconstant CSOUND_CONTROL_CHANNEL 1)

(defconstant CSOUND_AUDIO_CHANNEL 2)

(defconstant CSOUND_STRING_CHANNEL 3)

(defconstant CSOUND_CHANNEL_TYPE_MASK 15)

(defconstant CSOUND_INPUT_CHANNEL 16)

(defconstant CSOUND_OUTPUT_CHANNEL 32)

(defconstant CSOUND_CONTROL_CHANNEL_INT 1)

(defconstant CSOUND_CONTROL_CHANNEL_LIN 2)

(defconstant CSOUND_CONTROL_CHANNEL_EXP 3)

(defconstant CSOUND_CALLBACK_KBD_EVENT 1)

(defconstant CSOUND_CALLBACK_KBD_TEXT 2)

(defctype CSOUND :pointer)

(defctype WINDAT :pointer)

(defctype XYINDAT :pointer)

(defcstruct csRtAudioParams
	(devName :string)
	(devNum :int)
	(bufSamp_SW :int)
	(bufSamp_HW :int)
	(nChannels :int)
	(sampleFormat :int)
	(sampleRate :float))

(defcstruct RTCLOCK
	(starttime_real :pointer)
	(starttime_CPU :pointer))

(defctype RTCLOCK :pointer)

(defcstruct opcodeListEntry
	(opname :string)
	(outypes :string)
	(intypes :string))

(defcstruct CsoundRandMTState
	(mti :int)
	(mt :pointer))

(defctype CsoundRandMTState :pointer)

(defcstruct CsoundChannelListEntry
	(name :string)
	(type :int))

(defctype CsoundChannelListEntry :pointer)

(defcstruct PVSDATEXT
	(N :long)
	(overlap :long)
	(winsize :long)
	(wintype :int)
	(format :long)
	(framecount :unsigned-long)
	(frame :pointer))

(defctype PVSDATEXT :pointer)

(defctype CsoundChannelIOCallback_t :pointer)

(defcfun ("csoundInitialize" csoundInitialize) :int
  (argc :pointer)
  (argv :pointer)
  (flags :int))

(defcfun ("csoundCreate" csoundCreate) :pointer
  (hostData :pointer))

(defcfun ("csoundPreCompile" csoundPreCompile) :int
  (arg0 :pointer))

(defcfun ("csoundInitializeCscore" csoundInitializeCscore) :int
  (arg0 :pointer)
  (insco :pointer)
  (outsco :pointer))

(defcfun ("csoundQueryInterface" csoundQueryInterface) :int
  (name :string)
  (iface :pointer)
  (version :pointer))

(defcfun ("csoundDestroy" csoundDestroy) :void
  (arg0 :pointer))

(defcfun ("csoundGetVersion" csoundGetVersion) :int)

(defcfun ("csoundGetAPIVersion" csoundGetAPIVersion) :int)

(defcfun ("csoundGetHostData" csoundGetHostData) :pointer
  (arg0 :pointer))

(defcfun ("csoundSetHostData" csoundSetHostData) :void
  (arg0 :pointer)
  (hostData :pointer))

(defcfun ("csoundGetEnv" csoundGetEnv) :string
  (csound :pointer)
  (name :string))

(defcfun ("csoundSetGlobalEnv" csoundSetGlobalEnv) :int
  (name :string)
  (value :string))

(defcfun ("csoundCompile" csoundCompile) :int
  (arg0 :pointer)
  (argc :int)
  (argv :pointer))

(defcfun ("csoundPerform" csoundPerform) :int
  (arg0 :pointer))

(defcfun ("csoundPerformKsmps" csoundPerformKsmps) :int
  (arg0 :pointer))

(defcfun ("csoundPerformKsmpsAbsolute" csoundPerformKsmpsAbsolute) :int
  (arg0 :pointer))

(defcfun ("csoundPerformBuffer" csoundPerformBuffer) :int
  (arg0 :pointer))

(defcfun ("csoundStop" csoundStop) :void
  (arg0 :pointer))

(defcfun ("csoundCleanup" csoundCleanup) :int
  (arg0 :pointer))

(defcfun ("csoundReset" csoundReset) :void
  (arg0 :pointer))

(defcfun ("csoundGetSr" csoundGetSr) :float
  (arg0 :pointer))

(defcfun ("csoundGetKr" csoundGetKr) :float
  (arg0 :pointer))

(defcfun ("csoundGetKsmps" csoundGetKsmps) :int
  (arg0 :pointer))

(defcfun ("csoundGetNchnls" csoundGetNchnls) :int
  (arg0 :pointer))

(defcfun ("csoundGet0dBFS" csoundGet0dBFS) :float
  (arg0 :pointer))

(defcfun ("csoundGetStrVarMaxLen" csoundGetStrVarMaxLen) :int
  (arg0 :pointer))

;;(defcfun ("csoundGetSampleFormat" csoundGetSampleFormat) :int
;;  (arg0 :pointer))

;;(defcfun ("csoundGetSampleSize" csoundGetSampleSize) :int
;;  (arg0 :pointer))

(defcfun ("csoundGetInputBufferSize" csoundGetInputBufferSize) :long
  (arg0 :pointer))

(defcfun ("csoundGetOutputBufferSize" csoundGetOutputBufferSize) :long
  (arg0 :pointer))

(defcfun ("csoundGetInputBuffer" csoundGetInputBuffer) :pointer
  (arg0 :pointer))

(defcfun ("csoundGetOutputBuffer" csoundGetOutputBuffer) :pointer
  (arg0 :pointer))

(defcfun ("csoundGetSpin" csoundGetSpin) :pointer
  (arg0 :pointer))

(defcfun ("csoundGetSpout" csoundGetSpout) :pointer
  (arg0 :pointer))

(defcfun ("csoundGetOutputFileName" csoundGetOutputFileName) :string
  (arg0 :pointer))

(defcfun ("csoundSetHostImplementedAudioIO" csoundSetHostImplementedAudioIO) :void
  (arg0 :pointer)
  (state :int)
  (bufSize :int))

(defcfun ("csoundGetScoreTime" csoundGetScoreTime) :double
  (arg0 :pointer))

(defcfun ("csoundIsScorePending" csoundIsScorePending) :int
  (arg0 :pointer))

(defcfun ("csoundSetScorePending" csoundSetScorePending) :void
  (arg0 :pointer)
  (pending :int))

(defcfun ("csoundGetScoreOffsetSeconds" csoundGetScoreOffsetSeconds) :float
  (arg0 :pointer))

(defcfun ("csoundSetScoreOffsetSeconds" csoundSetScoreOffsetSeconds) :void
  (arg0 :pointer)
  (time :float))

(defcfun ("csoundRewindScore" csoundRewindScore) :void
  (arg0 :pointer))

(defcfun ("csoundSetCscoreCallback" csoundSetCscoreCallback) :void
  (arg0 :pointer)
  (cscoreCallback_ :pointer))

(defcfun ("csoundScoreSort" csoundScoreSort) :int
  (arg0 :pointer)
  (inFile :pointer)
  (outFile :pointer))

(defcfun ("csoundScoreExtract" csoundScoreExtract) :int
  (arg0 :pointer)
  (inFile :pointer)
  (outFile :pointer)
  (extractFile :pointer))

(defcfun ("csoundMessage" csoundMessage) :void
  (arg0 :pointer)
  (format :string)
  &rest)

(defcfun ("csoundMessageS" csoundMessageS) :void
  (arg0 :pointer)
  (attr :int)
  (format :string)
  &rest)

(defcfun ("csoundMessageV" csoundMessageV) :void
  (arg0 :pointer)
  (attr :int)
  (format :string)
  (args :pointer))

(defcfun ("csoundSetMessageCallback" csoundSetMessageCallback) :void
  (arg0 :pointer)
  (csoundMessageCallback_ :pointer))

(defcfun ("csoundGetMessageLevel" csoundGetMessageLevel) :int
  (arg0 :pointer))

(defcfun ("csoundSetMessageLevel" csoundSetMessageLevel) :void
  (arg0 :pointer)
  (messageLevel :int))

(defcfun ("csoundInputMessage" csoundInputMessage) :void
  (arg0 :pointer)
  (message :string))

(defcfun ("csoundKeyPress" csoundKeyPress) :void
  (arg0 :pointer)
  (c :char))

;;;(defcfun ("csoundSetInputValueCallback" csoundSetInputValueCallback) :void
;;;  (arg0 :pointer)
;;;  (inputValueCalback_ :pointer))

;;;(defcfun ("csoundSetOutputValueCallback" csoundSetOutputValueCallback) :void
;;;  (arg0 :pointer)
;;;  (outputValueCalback_ :pointer))

(defcfun ("csoundScoreEvent" csoundScoreEvent) :int
  (arg0 :pointer)
  (type :char)
  (pFields :pointer)
  (numFields :long))

(defcfun ("csoundSetExternalMidiInOpenCallback" csoundSetExternalMidiInOpenCallback) :void
  (arg0 :pointer)
  (func :pointer))

(defcfun ("csoundSetExternalMidiReadCallback" csoundSetExternalMidiReadCallback) :void
  (arg0 :pointer)
  (func :pointer))

(defcfun ("csoundSetExternalMidiInCloseCallback" csoundSetExternalMidiInCloseCallback) :void
  (arg0 :pointer)
  (func :pointer))

(defcfun ("csoundSetExternalMidiOutOpenCallback" csoundSetExternalMidiOutOpenCallback) :void
  (arg0 :pointer)
  (func :pointer))

(defcfun ("csoundSetExternalMidiWriteCallback" csoundSetExternalMidiWriteCallback) :void
  (arg0 :pointer)
  (func :pointer))

(defcfun ("csoundSetExternalMidiOutCloseCallback" csoundSetExternalMidiOutCloseCallback) :void
  (arg0 :pointer)
  (func :pointer))

(defcfun ("csoundSetExternalMidiErrorStringCallback" csoundSetExternalMidiErrorStringCallback) :void
  (arg0 :pointer)
  (func :pointer))

(defcfun ("csoundSetIsGraphable" csoundSetIsGraphable) :int
  (arg0 :pointer)
  (isGraphable :int))

(defcfun ("csoundSetMakeGraphCallback" csoundSetMakeGraphCallback) :void
  (arg0 :pointer)
  (makeGraphCallback_ :pointer))

(defcfun ("csoundSetDrawGraphCallback" csoundSetDrawGraphCallback) :void
  (arg0 :pointer)
  (drawGraphCallback_ :pointer))

(defcfun ("csoundSetKillGraphCallback" csoundSetKillGraphCallback) :void
  (arg0 :pointer)
  (killGraphCallback_ :pointer))

(defcfun ("csoundSetExitGraphCallback" csoundSetExitGraphCallback) :void
  (arg0 :pointer)
  (exitGraphCallback_ :pointer))

(defcfun ("csoundNewOpcodeList" csoundNewOpcodeList) :int
  (arg0 :pointer)
  (opcodelist :pointer))

(defcfun ("csoundDisposeOpcodeList" csoundDisposeOpcodeList) :void
  (arg0 :pointer)
  (opcodelist :pointer))

(defcfun ("csoundAppendOpcode" csoundAppendOpcode) :int
  (arg0 :pointer)
  (opname :string)
  (dsblksiz :int)
  (thread :int)
  (outypes :string)
  (intypes :string)
  (iopadr :pointer)
  (kopadr :pointer)
  (aopadr :pointer))

(defcfun ("csoundOpenLibrary" csoundOpenLibrary) :int
  (library :pointer)
  (libraryPath :string))

(defcfun ("csoundCloseLibrary" csoundCloseLibrary) :int
  (library :pointer))

(defcfun ("csoundGetLibrarySymbol" csoundGetLibrarySymbol) :pointer
  (library :pointer)
  (symbolName :string))

(defcfun ("csoundSetYieldCallback" csoundSetYieldCallback) :void
  (arg0 :pointer)
  (yieldCallback_ :pointer))

(defcfun ("csoundSetPlayopenCallback" csoundSetPlayopenCallback) :void
  (arg0 :pointer)
  (playopen__ :pointer))

(defcfun ("csoundSetRtplayCallback" csoundSetRtplayCallback) :void
  (arg0 :pointer)
  (rtplay__ :pointer))

(defcfun ("csoundSetRecopenCallback" csoundSetRecopenCallback) :void
  (arg0 :pointer)
  (recopen_ :pointer))

(defcfun ("csoundSetRtrecordCallback" csoundSetRtrecordCallback) :void
  (arg0 :pointer)
  (rtrecord__ :pointer))

(defcfun ("csoundSetRtcloseCallback" csoundSetRtcloseCallback) :void
  (arg0 :pointer)
  (rtclose__ :pointer))

(defcfun ("csoundGetDebug" csoundGetDebug) :int
  (arg0 :pointer))

(defcfun ("csoundSetDebug" csoundSetDebug) :void
  (arg0 :pointer)
  (debug :int))

(defcfun ("csoundTableLength" csoundTableLength) :int
  (arg0 :pointer)
  (table :int))

(defcfun ("csoundTableGet" csoundTableGet) :float
  (arg0 :pointer)
  (table :int)
  (index :int))

(defcfun ("csoundTableSet" csoundTableSet) :void
  (arg0 :pointer)
  (table :int)
  (index :int)
  (value :float))

(defcfun ("csoundGetTable" csoundGetTable) :int
  (arg0 :pointer)
  (tablePtr :pointer)
  (tableNum :int))

(defcfun ("csoundCreateThread" csoundCreateThread) :pointer
  (threadRoutine :pointer)
  (userdata :pointer))

(defcfun ("csoundGetCurrentThreadId" csoundGetCurrentThreadId) :pointer)

(defcfun ("csoundJoinThread" csoundJoinThread) :pointer
  (thread :pointer))

(defcfun ("csoundRunCommand" csoundRunCommand) :long
  (argv :pointer)
  (noWait :int))

(defcfun ("csoundCreateThreadLock" csoundCreateThreadLock) :pointer)

(defcfun ("csoundWaitThreadLock" csoundWaitThreadLock) :int
  (lock :pointer)
  (milliseconds :pointer))

(defcfun ("csoundWaitThreadLockNoTimeout" csoundWaitThreadLockNoTimeout) :void
  (lock :pointer))

(defcfun ("csoundNotifyThreadLock" csoundNotifyThreadLock) :void
  (lock :pointer))

(defcfun ("csoundDestroyThreadLock" csoundDestroyThreadLock) :void
  (lock :pointer))

(defcfun ("csoundCreateMutex" csoundCreateMutex) :pointer
  (isRecursive :int))

(defcfun ("csoundLockMutex" csoundLockMutex) :void
  (mutex_ :pointer))

(defcfun ("csoundLockMutexNoWait" csoundLockMutexNoWait) :int
  (mutex_ :pointer))

(defcfun ("csoundUnlockMutex" csoundUnlockMutex) :void
  (mutex_ :pointer))

(defcfun ("csoundDestroyMutex" csoundDestroyMutex) :void
  (mutex_ :pointer))

(defcfun ("csoundSleep" csoundSleep) :void
  (milliseconds :pointer))

(defcfun ("csoundInitTimerStruct" csoundInitTimerStruct) :void
  (arg0 :pointer))

(defcfun ("csoundGetRealTime" csoundGetRealTime) :double
  (arg0 :pointer))

(defcfun ("csoundGetCPUTime" csoundGetCPUTime) :double
  (arg0 :pointer))

(defcfun ("csoundGetRandomSeedFromTime" csoundGetRandomSeedFromTime) :pointer)

(defcfun ("csoundSetLanguage" csoundSetLanguage) :void
  (lang_code :pointer))

(defcfun ("csoundLocalizeString" csoundLocalizeString) :string
  (s :string))

(defcfun ("csoundCreateGlobalVariable" csoundCreateGlobalVariable) :int
  (arg0 :pointer)
  (name :string)
  (nbytes :pointer))

(defcfun ("csoundQueryGlobalVariable" csoundQueryGlobalVariable) :pointer
  (arg0 :pointer)
  (name :string))

(defcfun ("csoundQueryGlobalVariableNoCheck" csoundQueryGlobalVariableNoCheck) :pointer
  (arg0 :pointer)
  (name :string))

(defcfun ("csoundDestroyGlobalVariable" csoundDestroyGlobalVariable) :int
  (arg0 :pointer)
  (name :string))

(defcfun ("csoundGetSizeOfMYFLT" csoundGetSizeOfMYFLT) :int)

(defcfun ("csoundGetRtRecordUserData" csoundGetRtRecordUserData) :pointer
  (arg0 :pointer))

(defcfun ("csoundGetRtPlayUserData" csoundGetRtPlayUserData) :pointer
  (arg0 :pointer))

(defcfun ("csoundRegisterSenseEventCallback" csoundRegisterSenseEventCallback) :int
  (arg0 :pointer)
  (func :pointer)
  (userData :pointer))

(defcfun ("csoundRunUtility" csoundRunUtility) :int
  (arg0 :pointer)
  (name :string)
  (argc :int)
  (argv :pointer))

(defcfun ("csoundListUtilities" csoundListUtilities) :pointer
  (arg0 :pointer))

(defcfun ("csoundDeleteUtilityList" csoundDeleteUtilityList) :void
  (arg0 :pointer)
  (lst :pointer))

(defcfun ("csoundGetUtilityDescription" csoundGetUtilityDescription) :string
  (arg0 :pointer)
  (utilName :string))

(defcfun ("csoundGetChannelPtr" csoundGetChannelPtr) :int
  (arg0 :pointer)
  (p :pointer)
  (name :string)
  (type :int))

(defcfun ("csoundListChannels" csoundListChannels) :int
  (arg0 :pointer)
  (lst :pointer))

(defcfun ("csoundDeleteChannelList" csoundDeleteChannelList) :void
  (arg0 :pointer)
  (lst :pointer))

(defcfun ("csoundSetControlChannelHints" csoundSetControlChannelHints) :int
  (arg0 :pointer)
  (name :string)
  (type :int)
  (dflt :float)
  (min :float)
  (max :float))

(defcfun ("csoundGetControlChannelHints" csoundGetControlChannelHints) :int
  (arg0 :pointer)
  (name :string)
  (dflt :pointer)
  (min :pointer)
  (max :pointer))

;;;(defcfun ("csoundSetChannelIOCallback" csoundSetChannelIOCallback) :void
;;;  (arg0 :pointer)
;;;  (func :pointer))

(defcfun ("csoundRand31" csoundRand31) :int
  (seedVal :pointer))

(defcfun ("csoundSeedRandMT" csoundSeedRandMT) :void
  (p :pointer)
  (initKey :pointer)
  (keyLength :pointer))

(defcfun ("csoundRandMT" csoundRandMT) :pointer
  (p :pointer))

;;(defcfun ("csoundChanIKSet" csoundChanIKSet) :int
;;  (arg0 :pointer)
;;  (value :float)
;;  (n :int))

;;(defcfun ("csoundChanOKGet" csoundChanOKGet) :int
;;  (arg0 :pointer)
;;  (value :pointer)
;;  (n :int))

;;(defcfun ("csoundChanIASet" csoundChanIASet) :int
;;  (arg0 :pointer)
;;  (value :pointer)
;;  (n :int))

;;(defcfun ("csoundChanOAGet" csoundChanOAGet) :int
;;  (arg0 :pointer)
;;  (value :pointer)
;;  (n :int))

(defcfun ("csoundPvsinSet" csoundPvsinSet) :int
  (arg0 :pointer)
  (fin :pointer)
  (n :int))

(defcfun ("csoundPvsoutGet" csoundPvsoutGet) :int
  (csound :pointer)
  (fout :pointer)
  (n :int))

(defcfun ("csoundSetCallback" csoundSetCallback) :int
  (arg0 :pointer)
  (func :pointer)
  (userData :pointer)
  (typeMask :unsigned-int))

(defcfun ("csoundRemoveCallback" csoundRemoveCallback) :void
  (arg0 :pointer)
  (func :pointer))


