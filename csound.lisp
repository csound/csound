;; This is an automatically generated file. 
;;Make changes as you feel are necessary (but remember if you try to regenerate this file, your changes will be lost). 

(defpackage :csound
  (:use :common-lisp :ffi)
  (:export
	:CSOUND_SUCCESS
	:CSOUND_ERROR
	:CSOUND_INITIALIZATION
	:CSOUND_PERFORMANCE
	:CSOUND_MEMORY
	:CSOUND_SIGNAL
	:CSOUND_EXITJMP_SUCCESS
	:CSOUNDINIT_NO_SIGNAL_HANDLER
	:CSOUNDINIT_NO_ATEXIT
	:CSOUND_CONTROL_CHANNEL
	:CSOUND_AUDIO_CHANNEL
	:CSOUND_STRING_CHANNEL
	:CSOUND_CHANNEL_TYPE_MASK
	:CSOUND_INPUT_CHANNEL
	:CSOUND_OUTPUT_CHANNEL
	:CSOUND_CONTROL_CHANNEL_INT
	:CSOUND_CONTROL_CHANNEL_LIN
	:CSOUND_CONTROL_CHANNEL_EXP
	:make-csRtAudioParams
	:csRtAudioParams-devName
	:csRtAudioParams-devNum
	:csRtAudioParams-bufSamp_SW
	:csRtAudioParams-bufSamp_HW
	:csRtAudioParams-nChannels
	:csRtAudioParams-sampleFormat
	:csRtAudioParams-sampleRate
	:make-RTCLOCK
	:RTCLOCK-starttime_real
	:RTCLOCK-starttime_CPU
	:make-opcodeListEntry
	:opcodeListEntry-opname
	:opcodeListEntry-outypes
	:opcodeListEntry-intypes
	:make-CsoundRandMTState
	:CsoundRandMTState-mti
	:CsoundRandMTState-mt
	:csoundInitialize
	:csoundCreate
	:csoundPreCompile
	:csoundInitializeCscore
	:csoundQueryInterface
	:csoundDestroy
	:csoundGetVersion
	:csoundGetAPIVersion
	:csoundGetHostData
	:csoundSetHostData
	:csoundGetEnv
	:csoundSetDefaultEnv
	:csoundPerform
	:csoundCompile
	:csoundPerformKsmps
	:csoundPerformKsmpsAbsolute
	:csoundPerformBuffer
	:csoundCleanup
	:csoundReset
	:csoundGetSr
	:csoundGetKr
	:csoundGetKsmps
	:csoundGetNchnls
	:csoundGet0dBFS
	:csoundGetStrVarMaxLen
	:csoundGetSampleFormat
	:csoundGetSampleSize
	:csoundGetInputBufferSize
	:csoundGetOutputBufferSize
	:csoundGetInputBuffer
	:csoundGetOutputBuffer
	:csoundGetSpin
	:csoundGetSpout
	:csoundGetOutputFileName
	:csoundSetHostImplementedAudioIO
	:csoundGetScoreTime
	:csoundIsScorePending
	:csoundSetScorePending
	:csoundGetScoreOffsetSeconds
	:csoundSetScoreOffsetSeconds
	:csoundRewindScore
	:csoundSetCscoreCallback
	:csoundMessage
	:csoundMessageS
	:csoundMessageV
	:csoundThrowMessage
	:csoundThrowMessageV
	:csoundSetMessageCallback
	:csoundSetThrowMessageCallback
	:csoundGetMessageLevel
	:csoundSetMessageLevel
	:csoundInputMessage
	:csoundKeyPress
	:csoundSetInputValueCallback
	:csoundSetOutputValueCallback
	:csoundScoreEvent
	:csoundSetExternalMidiInOpenCallback
	:csoundSetExternalMidiReadCallback
	:csoundSetExternalMidiInCloseCallback
	:csoundSetExternalMidiOutOpenCallback
	:csoundSetExternalMidiWriteCallback
	:csoundSetExternalMidiOutCloseCallback
	:csoundSetExternalMidiErrorStringCallback
	:csoundSetIsGraphable
	:csoundSetMakeGraphCallback
	:csoundSetDrawGraphCallback
	:csoundSetKillGraphCallback
	:csoundSetMakeXYinCallback
	:csoundSetReadXYinCallback
	:csoundSetKillXYinCallback
	:csoundSetExitGraphCallback
	:csoundNewOpcodeList
	:csoundDisposeOpcodeList
	:csoundAppendOpcode
	:csoundOpenLibrary
	:csoundCloseLibrary
	:csoundGetLibrarySymbol
	:csoundSetYieldCallback
	:csoundSetPlayopenCallback
	:csoundSetRtplayCallback
	:csoundSetRecopenCallback
	:csoundSetRtrecordCallback
	:csoundSetRtcloseCallback
	:csoundGetDebug
	:csoundSetDebug
	:csoundTableLength
	:csoundTableGet
	:csoundTableSet
	:csoundGetTable
	:csoundCreateThread
	:csoundJoinThread
	:csoundCreateThreadLock
	:csoundWaitThreadLock
	:csoundWaitThreadLockNoTimeout
	:csoundNotifyThreadLock
	:csoundDestroyThreadLock
	:csoundSleep
	:csoundInitTimerStruct
	:csoundGetRealTime
	:csoundGetCPUTime
	:csoundGetRandomSeedFromTime
	:csoundSetLanguage
	:csoundLocalizeString
	:csoundCreateGlobalVariable
	:csoundQueryGlobalVariable
	:csoundQueryGlobalVariableNoCheck
	:csoundDestroyGlobalVariable
	:csoundGetSizeOfMYFLT
	:csoundGetRtRecordUserData
	:csoundGetRtPlayUserData
	:csoundRegisterSenseEventCallback
	:csoundRunUtility
	:csoundListUtilities
	:csoundGetUtilityDescription
	:csoundGetChannelPtr
	:csoundListChannels
	:csoundSetControlChannelParams
	:csoundGetControlChannelParams
	:csoundRand31
	:csoundSeedRandMT
	:csoundRandMT))

(in-package :csound)

(default-foreign-language :stdc)

;; Pretend these structures are ints, since they are only passed by pointer.

(ffi:def-c-type CSOUND int)

(ffi:def-c-type FILE int)

(ffi:def-c-type WINDAT int)

(ffi:def-c-type XYINDAT int)

(ffi:def-c-type CSRTAUDIOPARAMS int)

(ffi:def-c-type uintptr_t int)

(ffi:def-c-type size_t ffi:long)

(ffi:def-c-type cslanguage_t ffi:int)

(ffi:def-c-enum CSOUND_STATUS (CSOUND_SUCCESS 0)(CSOUND_ERROR -1)(CSOUND_INITIALIZATION -2)(CSOUND_PERFORMANCE -3)(CSOUND_MEMORY -4)(CSOUND_SIGNAL -5))



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



(ffi:def-c-struct csRtAudioParams

(devName ffi:c-string)

(devNum ffi:int)

(bufSamp_SW ffi:int)

(bufSamp_HW ffi:int)

(nChannels ffi:int)

(sampleFormat ffi:int)

(sampleRate SINGLE-FLOAT))



(ffi:def-c-struct RTCLOCK

(starttime_real ffi:long)

(starttime_CPU ffi:long))



(ffi:def-c-struct opcodeListEntry

(opname ffi:c-string)

(outypes ffi:c-string)

(intypes ffi:c-string))



(ffi:def-c-struct CsoundRandMTState

(mti ffi:int)

(mt (ffi:c-array ffi:int 624)))



(ffi:def-call-out csoundInitialize

(:name "csoundInitialize")

(:arguments (argc (ffi:c-ptr ffi:int))

	(argv (ffi:c-ptr (ffi:c-ptr ffi:c-string)))

	(flags ffi:int))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundCreate

(:name "csoundCreate")

(:arguments (hostData (ffi:c-pointer NIL)))

(:return-type (ffi:c-pointer CSOUND))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundPreCompile

(:name "csoundPreCompile")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundInitializeCscore

(:name "csoundInitializeCscore")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(insco (ffi:c-pointer FILE))

	(outsco (ffi:c-pointer FILE)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundQueryInterface

(:name "csoundQueryInterface")

(:arguments (name ffi:c-string)

	(iface (ffi:c-pointer (ffi:c-pointer NIL)))

	(version (ffi:c-ptr ffi:int)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundDestroy

(:name "csoundDestroy")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetVersion

(:name "csoundGetVersion")

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetAPIVersion

(:name "csoundGetAPIVersion")

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetHostData

(:name "csoundGetHostData")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type (ffi:c-pointer NIL))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetHostData

(:name "csoundSetHostData")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(hostData (ffi:c-pointer NIL)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetEnv

(:name "csoundGetEnv")

(:arguments (csound (ffi:c-pointer CSOUND))

	(name ffi:c-string))

(:return-type ffi:c-string)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetDefaultEnv

(:name "csoundSetDefaultEnv")

(:arguments (name ffi:c-string)

	(value ffi:c-string))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundPerform

(:name "csoundPerform")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(argc ffi:int)

	(argv (ffi:c-ptr ffi:c-string)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundCompile

(:name "csoundCompile")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(argc ffi:int)

	(argv (ffi:c-ptr ffi:c-string)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundPerformKsmps

(:name "csoundPerformKsmps")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundPerformKsmpsAbsolute

(:name "csoundPerformKsmpsAbsolute")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundPerformBuffer

(:name "csoundPerformBuffer")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundCleanup

(:name "csoundCleanup")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundReset

(:name "csoundReset")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetSr

(:name "csoundGetSr")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type SINGLE-FLOAT)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetKr

(:name "csoundGetKr")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type SINGLE-FLOAT)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetKsmps

(:name "csoundGetKsmps")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetNchnls

(:name "csoundGetNchnls")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGet0dBFS

(:name "csoundGet0dBFS")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type SINGLE-FLOAT)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetStrVarMaxLen

(:name "csoundGetStrVarMaxLen")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetSampleFormat

(:name "csoundGetSampleFormat")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetSampleSize

(:name "csoundGetSampleSize")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetInputBufferSize

(:name "csoundGetInputBufferSize")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:long)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetOutputBufferSize

(:name "csoundGetOutputBufferSize")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:long)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetInputBuffer

(:name "csoundGetInputBuffer")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type (ffi:c-ptr SINGLE-FLOAT))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetOutputBuffer

(:name "csoundGetOutputBuffer")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type (ffi:c-ptr SINGLE-FLOAT))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetSpin

(:name "csoundGetSpin")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type (ffi:c-ptr SINGLE-FLOAT))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetSpout

(:name "csoundGetSpout")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type (ffi:c-ptr SINGLE-FLOAT))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetOutputFileName

(:name "csoundGetOutputFileName")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:c-string)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetHostImplementedAudioIO

(:name "csoundSetHostImplementedAudioIO")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(state ffi:int)

	(bufSize ffi:int))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetScoreTime

(:name "csoundGetScoreTime")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type SINGLE-FLOAT)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundIsScorePending

(:name "csoundIsScorePending")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetScorePending

(:name "csoundSetScorePending")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(pending ffi:int))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetScoreOffsetSeconds

(:name "csoundGetScoreOffsetSeconds")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type SINGLE-FLOAT)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetScoreOffsetSeconds

(:name "csoundSetScoreOffsetSeconds")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(time SINGLE-FLOAT))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundRewindScore

(:name "csoundRewindScore")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetCscoreCallback

(:name "csoundSetCscoreCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(cscoreCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND)))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



;;(ffi:def-call-out csoundMessage
;;
;;(:name "csoundMessage")
;;
;;(:arguments (arg0 (ffi:c-pointer CSOUND))
;;
;;	(format ffi:c-string)
;;
;;	(arg2 ...))
;;
;;(:library "_CsoundVST.dll"))



;;(ffi:def-call-out csoundMessageS
;;
;;(:name "csoundMessageS")
;;
;;(:arguments (arg0 (ffi:c-pointer CSOUND))
;;
;;	(attr ffi:int)
;;
;;	(format ffi:c-string)
;;
;;	(arg3 ...))
;;
;;(:library "_CsoundVST.dll"))



;;(ffi:def-call-out csoundMessageV
;;
;;(:name "csoundMessageV")
;;
;;(:arguments (arg0 (ffi:c-pointer CSOUND))
;;
;;	(attr ffi:int)
;;
;;	(format ffi:c-string)
;;
;;	(args va_list))
;;
;;(:library "_CsoundVST.dll"))



;;(ffi:def-call-out csoundThrowMessage
;;
;;(:name "csoundThrowMessage")
;;
;;(:arguments (arg0 (ffi:c-pointer CSOUND))
;;
;;	(format ffi:c-string)
;;
;;	(arg2 ...))
;;
;;(:library "_CsoundVST.dll"))



;;(ffi:def-call-out csoundThrowMessageV
;;
;;(:name "csoundThrowMessageV")
;;
;;(:arguments (arg0 (ffi:c-pointer CSOUND))
;;
;;	(format ffi:c-string)
;;
;;	(args va_list))
;;
;;(:library "_CsoundVST.dll"))



;;(ffi:def-call-out csoundSetMessageCallback
;;
;;(:name "csoundSetMessageCallback")
;;
;;(:arguments (arg0 (ffi:c-pointer CSOUND))
;;
;;	(csoundMessageCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))
;;
;;	(arg1 ffi:int)
;;
;;	(arg2 ffi:c-string)
;;
;;	(arg3 va_list))
;;
;;				(:return-type NIL))))
;;
;;(:library "_CsoundVST.dll"))



;;(ffi:def-call-out csoundSetThrowMessageCallback
;;
;;(:name "csoundSetThrowMessageCallback")
;;
;;(:arguments (arg0 (ffi:c-pointer CSOUND))
;;
;;	(throwMessageCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))
;;
;;	(arg1 ffi:c-string)
;;
;;	(arg2 va_list))
;;
;;				(:return-type NIL))))
;;
;;(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetMessageLevel

(:name "csoundGetMessageLevel")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetMessageLevel

(:name "csoundSetMessageLevel")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(messageLevel ffi:int))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundInputMessage

(:name "csoundInputMessage")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(message ffi:c-string))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundKeyPress

(:name "csoundKeyPress")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(c character))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetInputValueCallback

(:name "csoundSetInputValueCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(inputValueCalback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 ffi:c-string)

	(arg2 (ffi:c-ptr SINGLE-FLOAT)))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetOutputValueCallback

(:name "csoundSetOutputValueCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(outputValueCalback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 ffi:c-string)

	(arg2 SINGLE-FLOAT))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundScoreEvent

(:name "csoundScoreEvent")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(type character)

	(pFields (ffi:c-ptr SINGLE-FLOAT))

	(numFields ffi:long))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetExternalMidiInOpenCallback

(:name "csoundSetExternalMidiInOpenCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(func (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer (ffi:c-pointer NIL)))

	(arg2 ffi:c-string))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetExternalMidiReadCallback

(:name "csoundSetExternalMidiReadCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(func (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer NIL))

	(arg2 (ffi:c-pointer ffi:uchar))

	(arg3 ffi:int))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetExternalMidiInCloseCallback

(:name "csoundSetExternalMidiInCloseCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(func (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer NIL)))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetExternalMidiOutOpenCallback

(:name "csoundSetExternalMidiOutOpenCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(func (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer (ffi:c-pointer NIL)))

	(arg2 ffi:c-string))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetExternalMidiWriteCallback

(:name "csoundSetExternalMidiWriteCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(func (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer NIL))

	(arg2 (ffi:c-pointer ffi:uchar))

	(arg3 ffi:int))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetExternalMidiOutCloseCallback

(:name "csoundSetExternalMidiOutCloseCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(func (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer NIL)))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetExternalMidiErrorStringCallback

(:name "csoundSetExternalMidiErrorStringCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(func (ffi:c-function (:arguments (arg0 ffi:int))

				(:return-type ffi:c-string))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetIsGraphable

(:name "csoundSetIsGraphable")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(isGraphable ffi:int))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetMakeGraphCallback

(:name "csoundSetMakeGraphCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(makeGraphCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer WINDAT))

	(arg2 ffi:c-string))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetDrawGraphCallback

(:name "csoundSetDrawGraphCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(drawGraphCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer WINDAT)))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetKillGraphCallback

(:name "csoundSetKillGraphCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(killGraphCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer WINDAT)))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetMakeXYinCallback

(:name "csoundSetMakeXYinCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(makeXYinCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer XYINDAT))

	(arg2 SINGLE-FLOAT)

	(arg3 SINGLE-FLOAT))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetReadXYinCallback

(:name "csoundSetReadXYinCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(readXYinCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer XYINDAT)))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetKillXYinCallback

(:name "csoundSetKillXYinCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(killXYinCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer XYINDAT)))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetExitGraphCallback

(:name "csoundSetExitGraphCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(exitGraphCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND)))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundNewOpcodeList

(:name "csoundNewOpcodeList")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(opcodelist (ffi:c-pointer (ffi:c-pointer opcodeListEntry))))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundDisposeOpcodeList

(:name "csoundDisposeOpcodeList")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(opcodelist (ffi:c-pointer opcodeListEntry)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundAppendOpcode

(:name "csoundAppendOpcode")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(opname ffi:c-string)

	(dsblksiz ffi:int)

	(thread ffi:int)

	(outypes ffi:c-string)

	(intypes ffi:c-string)

	(iopadr (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer NIL)))

				(:return-type ffi:int)))

	(kopadr (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer NIL)))

				(:return-type ffi:int)))

	(aopadr (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer NIL)))

				(:return-type ffi:int))))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundOpenLibrary

(:name "csoundOpenLibrary")

(:arguments (libraryPath ffi:c-string))

(:return-type (ffi:c-pointer NIL))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundCloseLibrary

(:name "csoundCloseLibrary")

(:arguments (library (ffi:c-pointer NIL)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetLibrarySymbol

(:name "csoundGetLibrarySymbol")

(:arguments (library (ffi:c-pointer NIL))

	(symbolName ffi:c-string))

(:return-type (ffi:c-pointer NIL))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetYieldCallback

(:name "csoundSetYieldCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(yieldCallback (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND)))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetPlayopenCallback

(:name "csoundSetPlayopenCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(playopen__ (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer csRtAudioParams)))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetRtplayCallback

(:name "csoundSetRtplayCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(rtplay__ (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-ptr SINGLE-FLOAT))

	(arg2 ffi:int))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetRecopenCallback

(:name "csoundSetRecopenCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(recopen_ (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer csRtAudioParams)))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetRtrecordCallback

(:name "csoundSetRtrecordCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(rtrecord__ (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-ptr SINGLE-FLOAT))

	(arg2 ffi:int))

				(:return-type ffi:int))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetRtcloseCallback

(:name "csoundSetRtcloseCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(rtclose__ (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND)))

				(:return-type NIL))))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetDebug

(:name "csoundGetDebug")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetDebug

(:name "csoundSetDebug")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(debug ffi:int))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundTableLength

(:name "csoundTableLength")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(table ffi:int))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundTableGet

(:name "csoundTableGet")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(table ffi:int)

	(index ffi:int))

(:return-type SINGLE-FLOAT)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundTableSet

(:name "csoundTableSet")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(table ffi:int)

	(index ffi:int)

	(value SINGLE-FLOAT))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetTable

(:name "csoundGetTable")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(tableNum ffi:int)

	(tableLength (ffi:c-ptr ffi:int)))

(:return-type (ffi:c-ptr SINGLE-FLOAT))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundCreateThread

(:name "csoundCreateThread")

(:arguments (threadRoutine (ffi:c-function (:arguments (arg0 (ffi:c-pointer NIL)))

				(:return-type uintptr_t)))

	(userdata (ffi:c-pointer NIL)))

(:return-type (ffi:c-pointer NIL))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundJoinThread

(:name "csoundJoinThread")

(:arguments (thread (ffi:c-pointer NIL)))

(:return-type uintptr_t)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundCreateThreadLock

(:name "csoundCreateThreadLock")

(:return-type (ffi:c-pointer NIL))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundWaitThreadLock

(:name "csoundWaitThreadLock")

(:arguments (lock (ffi:c-pointer NIL))

	(milliseconds size_t))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundWaitThreadLockNoTimeout

(:name "csoundWaitThreadLockNoTimeout")

(:arguments (lock (ffi:c-pointer NIL)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundNotifyThreadLock

(:name "csoundNotifyThreadLock")

(:arguments (lock (ffi:c-pointer NIL)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundDestroyThreadLock

(:name "csoundDestroyThreadLock")

(:arguments (lock (ffi:c-pointer NIL)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSleep

(:name "csoundSleep")

(:arguments (milliseconds size_t))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundInitTimerStruct

(:name "csoundInitTimerStruct")

(:arguments (arg0 (ffi:c-pointer RTCLOCK)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetRealTime

(:name "csoundGetRealTime")

(:arguments (arg0 (ffi:c-pointer RTCLOCK)))

(:return-type DOUBLE-FLOAT)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetCPUTime

(:name "csoundGetCPUTime")

(:arguments (arg0 (ffi:c-pointer RTCLOCK)))

(:return-type DOUBLE-FLOAT)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetRandomSeedFromTime

(:name "csoundGetRandomSeedFromTime")

(:return-type uint32)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetLanguage

(:name "csoundSetLanguage")

(:arguments (lang_code cslanguage_t))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundLocalizeString

(:name "csoundLocalizeString")

(:arguments (s ffi:c-string))

(:return-type ffi:c-string)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundCreateGlobalVariable

(:name "csoundCreateGlobalVariable")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(name ffi:c-string)

	(nbytes size_t))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundQueryGlobalVariable

(:name "csoundQueryGlobalVariable")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(name ffi:c-string))

(:return-type (ffi:c-pointer NIL))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundQueryGlobalVariableNoCheck

(:name "csoundQueryGlobalVariableNoCheck")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(name ffi:c-string))

(:return-type (ffi:c-pointer NIL))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundDestroyGlobalVariable

(:name "csoundDestroyGlobalVariable")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(name ffi:c-string))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetSizeOfMYFLT

(:name "csoundGetSizeOfMYFLT")

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetRtRecordUserData

(:name "csoundGetRtRecordUserData")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type (ffi:c-pointer (ffi:c-pointer NIL)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetRtPlayUserData

(:name "csoundGetRtPlayUserData")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type (ffi:c-pointer (ffi:c-pointer NIL)))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundRegisterSenseEventCallback

(:name "csoundRegisterSenseEventCallback")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(func (ffi:c-function (:arguments (arg0 (ffi:c-pointer CSOUND))

	(arg1 (ffi:c-pointer NIL)))

				(:return-type NIL)))

	(userData (ffi:c-pointer NIL)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundRunUtility

(:name "csoundRunUtility")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(name ffi:c-string)

	(argc ffi:int)

	(argv (ffi:c-ptr ffi:c-string)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundListUtilities

(:name "csoundListUtilities")

(:arguments (arg0 (ffi:c-pointer CSOUND)))

(:return-type (ffi:c-ptr ffi:c-string))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetUtilityDescription

(:name "csoundGetUtilityDescription")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(utilName ffi:c-string))

(:return-type ffi:c-string)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetChannelPtr

(:name "csoundGetChannelPtr")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(p (ffi:c-ptr (ffi:c-ptr SINGLE-FLOAT)))

	(name ffi:c-string)

	(type ffi:int))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundListChannels

(:name "csoundListChannels")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(names (ffi:c-ptr (ffi:c-ptr ffi:c-string)))

	(types (ffi:c-ptr (ffi:c-ptr ffi:int))))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSetControlChannelParams

(:name "csoundSetControlChannelParams")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(name ffi:c-string)

	(type ffi:int)

	(dflt SINGLE-FLOAT)

	(min SINGLE-FLOAT)

	(max SINGLE-FLOAT))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundGetControlChannelParams

(:name "csoundGetControlChannelParams")

(:arguments (arg0 (ffi:c-pointer CSOUND))

	(name ffi:c-string)

	(dflt (ffi:c-ptr SINGLE-FLOAT))

	(min (ffi:c-ptr SINGLE-FLOAT))

	(max (ffi:c-ptr SINGLE-FLOAT)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundRand31

(:name "csoundRand31")

(:arguments (seedVal (ffi:c-ptr ffi:int)))

(:return-type ffi:int)

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundSeedRandMT

(:name "csoundSeedRandMT")

(:arguments (p (ffi:c-pointer CsoundRandMTState))

	(initKey (ffi:c-pointer uint32))

	(keyLength uint32))

(:library "_CsoundVST.dll"))



(ffi:def-call-out csoundRandMT

(:name "csoundRandMT")

(:arguments (p (ffi:c-pointer CsoundRandMTState)))

(:return-type uint32)

(:library "_CsoundVST.dll"))

