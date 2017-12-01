'''
    ctcsound.py:
    
    Copyright (C) 2016 Francois Pinot
    
    This file is part of Csound.
    
    This code is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    
    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
'''

from ctypes import *
import numpy as np
import sys

if sys.platform.startswith('linux'):
    libcsound = CDLL("libcsound64.so")
elif sys.platform.startswith('win'):
    libcsound = cdll.csound64
elif sys.platform.startswith('darwin'):
    libcsound = CDLL("CsoundLib64.framework/CsoundLib64")
else:
    sys.exit("Don't know your system! Exiting...")

MYFLT = c_double

class CsoundParams(Structure):
    _fields_ = [("debug_mode", c_int),        # debug mode, 0 or 1
                ("buffer_frames", c_int),     # number of frames in in/out buffers
                ("hardware_buffer_frames", c_int), # ibid. hardware
                ("displays", c_int),          # graph displays, 0 or 1
                ("ascii_graphs", c_int),      # use ASCII graphs, 0 or 1
                ("postscript_graphs", c_int), # use postscript graphs, 0 or 1
                ("message_level", c_int),     # message printout control
                ("tempo", c_int),             # tempo ("sets Beatmode) 
                ("ring_bell", c_int),         # bell, 0 or 1
                ("use_cscore", c_int),        # use cscore for processing
                ("terminate_on_midi", c_int), # terminate performance at the end
                                              #   of midifile, 0 or 1
                ("heartbeat", c_int),         # print heart beat, 0 or 1
                ("defer_gen01_load", c_int),  # defer GEN01 load, 0 or 1
                ("midi_key", c_int),          # pfield to map midi key no
                ("midi_key_cps", c_int),      # pfield to map midi key no as cps
                ("midi_key_oct", c_int),      # pfield to map midi key no as oct
                ("midi_key_pch", c_int),      # pfield to map midi key no as pch
                ("midi_velocity", c_int),     # pfield to map midi velocity
                ("midi_velocity_amp", c_int), # pfield to map midi velocity as amplitude
                ("no_default_paths", c_int),  # disable relative paths from files, 0 or 1
                ("number_of_threads", c_int), # number of threads for multicore performance
                ("syntax_check_only", c_int), # do not compile, only check syntax
                ("csd_line_counts", c_int),   # csd line error reporting
                ("compute_weights", c_int),   # deprecated, kept for backwards comp. 
                ("realtime_mode", c_int),     # use realtime priority mode, 0 or 1
                ("sample_accurate", c_int),   # use sample-level score event accuracy
                ("sample_rate_override", MYFLT),  # overriding sample rate
                ("control_rate_override", MYFLT), # overriding control rate
                ("nchnls_override", c_int),   # overriding number of out channels
                ("nchnls_i_override", c_int), # overriding number of in channels
                ("e0dbfs_override", MYFLT),   # overriding 0dbfs
                ("daemon", c_int),            # daemon mode
                ("ksmps_override", c_int),    # ksmps override
                ("FFT_library", c_int)]       # fft_lib

string64 = c_char * 64

class CsoundAudioDevice(Structure):
    _fields_ = [("device_name", string64),
                ("device_id", string64),
                ("rt_module", string64),
                ("max_nchnls", c_int),
                ("isOutput", c_int)]

class CsoundMidiDevice(Structure):
    _fields_ = [("device_name", string64),
                ("interface_name", string64),
                ("device_id", string64),
                ("midi_module", string64),
                ("isOutput", c_int)]

class CsoundRtAudioParams(Structure):
    _fields_ = [("devName", c_char_p),   # device name (NULL/empty: default)
                ("devNum", c_int),       # device number (0-1023), 1024: default
                ("bufSamp_SW", c_uint),  # buffer fragment size (-b) in sample frames
                ("bufSamp_HW", c_int),   # total buffer size (-B) in sample frames
                ("nChannels", c_int),    # number of channels
                ("sampleFormat", c_int), # sample format (AE_SHORT etc.)
                ("sampleRate", c_float)] # sample rate in Hz

class RtClock(Structure):
    _fields_ = [("starttime_real", c_int64),
                ("starttime_CPU", c_int64)]

class OpcodeListEntry(Structure):
    _fields_ = [("opname", c_char_p),
                ("outypes", c_char_p),
                ("intypes", c_char_p),
                ("flags", c_int)]

class CsoundRandMTState(Structure):
    _fields_ = [("mti", c_int),
                ("mt", c_uint32*624)]

# PVSDATEXT is a variation on PVSDAT used in the pvs bus interface
class PvsdatExt(Structure):
    _fields_ = [("N", c_int32),
                ("sliding", c_int),      # Flag to indicate sliding case
                ("NB", c_int32),
                ("overlap", c_int32),
                ("winsize", c_int32),
                ("wintype", c_int),
                ("format", c_int32),
                ("framecount", c_uint32),
                ("frame", POINTER(c_float))]

# This structure holds the parameter hints for control channels
class ControlChannelHints(Structure):
    _fields_ = [("behav", c_int),
                ("dflt", MYFLT),
                ("min", MYFLT),
                ("max", MYFLT),
                ("x", c_int),
                ("y", c_int),
                ("width", c_int),
                ("height", c_int),
                # This member must be set explicitly to None if not used
                ("attributes", c_char_p)]

class ControlChannelInfo(Structure):
    _fields_ = [("name", c_char_p),
                ("type", c_int),
                ("hints", ControlChannelHints)]

CAPSIZE  = 60

class Windat(Structure):
    _fields_ = [("windid", POINTER(c_uint)),    # set by makeGraph()
                ("fdata", POINTER(MYFLT)),      # data passed to drawGraph()
                ("npts", c_int32),              # size of above array
                ("caption", c_char * CAPSIZE),  # caption string for graph
                ("waitflg", c_int16 ),          # set =1 to wait for ms after Draw
                ("polarity", c_int16),          # controls positioning of X axis
                ("max", MYFLT),                 # workspace .. extrema this frame
                ("min", MYFLT),
                ("absmax", MYFLT),              # workspace .. largest of above
                ("oabsmax", MYFLT),             # Y axis scaling factor
                ("danflag", c_int),             # set to 1 for extra Yaxis mid span
                ("absflag", c_int)]             # set to 1 to skip abs check

# Symbols for Windat.polarity field
NOPOL = 0
NEGPOL = 1
POSPOL = 2
BIPOL = 3

class NamedGen(Structure):
    pass

NamedGen._fields_ = [
    ("name", c_char_p),
    ("genum", c_int),
    ("next", POINTER(NamedGen))]


libcsound.csoundCreate.restype = c_void_p
libcsound.csoundCreate.argtypes = [py_object]

libcsound.csoundDestroy.argtypes = [c_void_p]

libcsound.csoundParseOrc.restype = c_void_p
libcsound.csoundParseOrc.argtypes = [c_void_p, c_char_p]

libcsound.csoundCompileTree.argtypes = [c_void_p, c_void_p]
libcsound.csoundCompileTreeAsync.argtypes = [c_void_p, c_void_p]
libcsound.csoundDeleteTree.argtypes = [c_void_p, c_void_p]
libcsound.csoundCompileOrc.argtypes = [c_void_p, c_char_p]
libcsound.csoundCompileOrcAsync.argtypes = [c_void_p, c_char_p]

libcsound.csoundEvalCode.restype = MYFLT
libcsound.csoundEvalCode.argtypes = [c_void_p, c_char_p]

libcsound.csoundCompileArgs.argtypes = [c_void_p, c_int, POINTER(c_char_p)]
libcsound.csoundStart.argtypes = [c_void_p]
libcsound.csoundCompile.argtypes = [c_void_p, c_int, POINTER(c_char_p)]
libcsound.csoundCompileCsd.argtypes = [c_void_p, c_char_p]
libcsound.csoundCompileCsdText.argtypes = [c_void_p, c_char_p]

libcsound.csoundPerform.argtypes = [c_void_p]
libcsound.csoundPerformKsmps.argtypes = [c_void_p]
libcsound.csoundPerformBuffer.argtypes = [c_void_p]
libcsound.csoundStop.argtypes = [c_void_p]
libcsound.csoundCleanup.argtypes = [c_void_p]
libcsound.csoundReset.argtypes = [c_void_p]

libcsound.csoundUDPServerStart.argtypes = [c_void_p, c_uint]
libcsound.csoundUDPServerStatus.argtypes = [c_void_p]
libcsound.csoundUDPServerClose.argtypes = [c_void_p]
libcsound.csoundUDPConsole.argtypes = [c_void_p, c_char_p, c_uint, c_uint]
libcsound.csoundStopUDPConsole.argtypes = [c_void_p]

libcsound.csoundGetSr.restype = MYFLT
libcsound.csoundGetSr.argtypes = [c_void_p]
libcsound.csoundGetKr.restype = MYFLT
libcsound.csoundGetKr.argtypes = [c_void_p]
libcsound.csoundGetKsmps.restype = c_uint32
libcsound.csoundGetKsmps.argtypes = [c_void_p]
libcsound.csoundGetNchnls.restype = c_uint32
libcsound.csoundGetNchnls.argtypes = [c_void_p]
libcsound.csoundGetNchnlsInput.restype = c_uint32
libcsound.csoundGetNchnlsInput.argtypes = [c_void_p]
libcsound.csoundGet0dBFS.restype = MYFLT
libcsound.csoundGet0dBFS.argtypes = [c_void_p]
libcsound.csoundGetA4.restype = MYFLT
libcsound.csoundGetA4.argtypes = [c_void_p]
libcsound.csoundGetCurrentTimeSamples.restype = c_int64
libcsound.csoundGetCurrentTimeSamples.argtypes = [c_void_p]
libcsound.csoundGetHostData.restype = py_object
libcsound.csoundGetHostData.argtypes = [c_void_p]
libcsound.csoundSetHostData.argtypes = [c_void_p, py_object]
libcsound.csoundSetOption.argtypes = [c_void_p, c_char_p]
libcsound.csoundSetParams.argtypes = [c_void_p, POINTER(CsoundParams)]
libcsound.csoundGetParams.argtypes = [c_void_p, POINTER(CsoundParams)]
libcsound.csoundGetDebug.argtypes = [c_void_p]
libcsound.csoundSetDebug.argtypes = [c_void_p, c_int]

libcsound.csoundGetOutputName.restype = c_char_p
libcsound.csoundGetOutputName.argtypes = [c_void_p]
libcsound.csoundSetOutput.argtypes = [c_void_p, c_char_p, c_char_p, c_char_p]
libcsound.csoundGetOutputFormat.argtypes = [c_void_p, c_char_p, c_char_p]
libcsound.csoundSetInput.argtypes = [c_void_p, c_char_p]
libcsound.csoundSetMIDIInput.argtypes = [c_void_p, c_char_p]
libcsound.csoundSetMIDIFileInput.argtypes = [c_void_p, c_char_p]
libcsound.csoundSetMIDIOutput.argtypes = [c_void_p, c_char_p]
libcsound.csoundSetMIDIFileOutput.argtypes = [c_void_p, c_char_p]
FILEOPENFUNC = CFUNCTYPE(None, c_void_p, c_char_p, c_int, c_int, c_int)
libcsound.csoundSetFileOpenCallback.argtypes = [c_void_p, FILEOPENFUNC]

libcsound.csoundSetRTAudioModule.argtypes = [c_void_p, c_char_p]
libcsound.csoundGetModule.argtypes = [c_void_p, c_int, POINTER(c_char_p), POINTER(c_char_p)]
libcsound.csoundGetInputBufferSize.restype = c_long
libcsound.csoundGetInputBufferSize.argtypes = [c_void_p]
libcsound.csoundGetOutputBufferSize.restype = c_long
libcsound.csoundGetOutputBufferSize.argtypes = [c_void_p]
libcsound.csoundGetInputBuffer.restype = POINTER(MYFLT)
libcsound.csoundGetInputBuffer.argtypes = [c_void_p]
libcsound.csoundGetOutputBuffer.restype = POINTER(MYFLT)
libcsound.csoundGetOutputBuffer.argtypes = [c_void_p]
libcsound.csoundGetSpin.restype = POINTER(MYFLT)
libcsound.csoundGetSpin.argtypes = [c_void_p]
libcsound.csoundClearSpin.argtypes = [c_void_p]
libcsound.csoundAddSpinSample.argtypes = [c_void_p, c_int, c_int, MYFLT]
libcsound.csoundSetSpinSample.argtypes = [c_void_p, c_int, c_int, MYFLT]
libcsound.csoundGetSpout.restype = POINTER(MYFLT)
libcsound.csoundGetSpout.argtypes = [c_void_p]
libcsound.csoundGetSpoutSample.restype = MYFLT
libcsound.csoundGetSpoutSample.argtypes = [c_void_p, c_int, c_int]
libcsound.csoundGetRtRecordUserData.restype = POINTER(c_void_p)
libcsound.csoundGetRtRecordUserData.argtypes = [c_void_p]
libcsound.csoundGetRtPlayUserData.restype = POINTER(c_void_p)
libcsound.csoundGetRtPlayUserData.argtypes = [c_void_p]
libcsound.csoundSetHostImplementedAudioIO.argtypes = [c_void_p, c_int, c_int]
libcsound.csoundGetAudioDevList.argtypes = [c_void_p, c_void_p, c_int]
PLAYOPENFUNC = CFUNCTYPE(c_int, c_void_p, POINTER(CsoundRtAudioParams))
libcsound.csoundSetPlayopenCallback.argtypes = [c_void_p, PLAYOPENFUNC]
RTPLAYFUNC = CFUNCTYPE(None, c_void_p, POINTER(MYFLT), c_int)
libcsound.csoundSetRtplayCallback.argtypes = [c_void_p, RTPLAYFUNC]
RECORDOPENFUNC = CFUNCTYPE(c_int, c_void_p, POINTER(CsoundRtAudioParams))
libcsound.csoundSetRecopenCallback.argtypes = [c_void_p, RECORDOPENFUNC]
RTRECORDFUNC = CFUNCTYPE(c_int, c_void_p, POINTER(MYFLT), c_int)
libcsound.csoundSetRtrecordCallback.argtypes = [c_void_p, RTRECORDFUNC]
RTCLOSEFUNC = CFUNCTYPE(None, c_void_p)
libcsound.csoundSetRtcloseCallback.argtypes = [c_void_p, RTCLOSEFUNC]
AUDIODEVLISTFUNC = CFUNCTYPE(c_int, c_void_p, POINTER(CsoundAudioDevice), c_int)
libcsound.csoundSetAudioDeviceListCallback.argtypes = [c_void_p, AUDIODEVLISTFUNC]

libcsound.csoundSetMIDIModule.argtypes = [c_void_p, c_char_p]
libcsound.csoundSetHostImplementedMIDIIO.argtypes = [c_void_p, c_int]
libcsound.csoundGetMIDIDevList.argtypes = [c_void_p, c_void_p, c_int]
MIDIINOPENFUNC = CFUNCTYPE(c_int, c_void_p, POINTER(c_void_p), c_char_p)
libcsound.csoundSetExternalMidiInOpenCallback.argtypes = [c_void_p, MIDIINOPENFUNC]
MIDIREADFUNC = CFUNCTYPE(c_int, c_void_p, c_void_p, c_char_p, c_int)
libcsound.csoundSetExternalMidiReadCallback.argtypes = [c_void_p, MIDIREADFUNC]
MIDIINCLOSEFUNC = CFUNCTYPE(c_int, c_void_p, c_void_p)
libcsound.csoundSetExternalMidiInCloseCallback.argtypes = [c_void_p, MIDIINCLOSEFUNC]
MIDIOUTOPENFUNC = CFUNCTYPE(c_int, c_void_p, POINTER(c_void_p), c_char_p)
libcsound.csoundSetExternalMidiOutOpenCallback.argtypes = [c_void_p, MIDIOUTOPENFUNC]
MIDIWRITEFUNC = CFUNCTYPE(c_int, c_void_p, c_void_p, c_char_p, c_int)
libcsound.csoundSetExternalMidiWriteCallback.argtypes = [c_void_p, MIDIWRITEFUNC]
MIDIOUTCLOSEFUNC = CFUNCTYPE(c_int, c_void_p, c_void_p)
libcsound.csoundSetExternalMidiOutCloseCallback.argtypes = [c_void_p, MIDIOUTCLOSEFUNC]
MIDIERRORFUNC = CFUNCTYPE(c_char_p, c_int)
libcsound.csoundSetExternalMidiErrorStringCallback.argtypes = [c_void_p, MIDIERRORFUNC]
MIDIDEVLISTFUNC = CFUNCTYPE(c_int, c_void_p, POINTER(CsoundMidiDevice), c_int)
libcsound.csoundSetMIDIDeviceListCallback.argtypes = [c_void_p, MIDIDEVLISTFUNC]

libcsound.csoundReadScore.argtypes = [c_void_p, c_char_p]
libcsound.csoundReadScoreAsync.argtypes = [c_void_p, c_char_p]
libcsound.csoundGetScoreTime.restype = c_double
libcsound.csoundGetScoreTime.argtypes = [c_void_p]
libcsound.csoundIsScorePending.argtypes = [c_void_p]
libcsound.csoundSetScorePending.argtypes = [c_void_p, c_int]
libcsound.csoundGetScoreOffsetSeconds.restype = MYFLT
libcsound.csoundGetScoreOffsetSeconds.argtypes = [c_void_p]
libcsound.csoundSetScoreOffsetSeconds.argtypes = [c_void_p, MYFLT]
libcsound.csoundRewindScore.argtypes = [c_void_p]
CSCOREFUNC = CFUNCTYPE(None, c_void_p)
libcsound.csoundSetCscoreCallback.argtypes = [c_void_p, CSCOREFUNC]

libcsound.csoundMessage.argtypes = [c_void_p, c_char_p, c_char_p]
libcsound.csoundMessageS.argtypes = [c_void_p, c_int, c_char_p, c_char_p]
libcsound.csoundGetMessageLevel.argtypes = [c_void_p]
libcsound.csoundSetMessageLevel.argtypes = [c_void_p, c_int]
libcsound.csoundCreateMessageBuffer.argtypes = [c_void_p, c_int]
libcsound.csoundGetFirstMessage.restype = c_char_p
libcsound.csoundGetFirstMessage.argtypes = [c_void_p]
libcsound.csoundGetFirstMessageAttr.argtypes = [c_void_p]
libcsound.csoundPopFirstMessage.argtypes = [c_void_p]
libcsound.csoundGetMessageCnt.argtypes = [c_void_p]
libcsound.csoundDestroyMessageBuffer.argtypes = [c_void_p]

libcsound.csoundGetChannelPtr.argtypes = [c_void_p, POINTER(POINTER(MYFLT)), c_char_p, c_int]
libcsound.csoundListChannels.argtypes = [c_void_p, POINTER(POINTER(ControlChannelInfo))]
libcsound.csoundDeleteChannelList.argtypes = [c_void_p, POINTER(ControlChannelInfo)]
libcsound.csoundSetControlChannelHints.argtypes = [c_void_p, c_char_p, ControlChannelHints]
libcsound.csoundGetControlChannelHints.argtypes = [c_void_p, c_char_p, POINTER(ControlChannelHints)]
libcsound.csoundGetChannelLock.restype = POINTER(c_int)
libcsound.csoundGetChannelLock.argtypes = [c_void_p, c_char_p]
libcsound.csoundGetControlChannel.restype = MYFLT
libcsound.csoundGetControlChannel.argtypes = [c_void_p, c_char_p, POINTER(c_int)]
libcsound.csoundSetControlChannel.argtypes = [c_void_p, c_char_p, MYFLT]
libcsound.csoundGetAudioChannel.argtypes = [c_void_p, c_char_p, POINTER(c_int)]
libcsound.csoundSetAudioChannel.argtypes = [c_void_p, c_char_p, POINTER(c_int)]
libcsound.csoundGetStringChannel.argtypes = [c_void_p, c_char_p, c_char_p]
libcsound.csoundSetStringChannel.argtypes = [c_void_p, c_char_p, c_char_p]
libcsound.csoundGetChannelDatasize.argtypes = [c_void_p, c_char_p]
CHANNELFUNC = CFUNCTYPE(None, c_void_p, c_char_p, c_void_p, c_void_p)
libcsound.csoundSetInputChannelCallback.argtypes = [c_void_p, CHANNELFUNC]
libcsound.csoundSetOutputChannelCallback.argtypes = [c_void_p, CHANNELFUNC]
libcsound.csoundSetPvsChannel.argtypes = [c_void_p, POINTER(PvsdatExt), c_char_p]
libcsound.csoundGetPvsChannel.argtypes = [c_void_p, POINTER(PvsdatExt), c_char_p]
libcsound.csoundScoreEvent.argtypes = [c_void_p, c_char, POINTER(MYFLT), c_long]
libcsound.csoundScoreEventAsync.argtypes = [c_void_p, c_char, POINTER(MYFLT), c_long]
libcsound.csoundScoreEventAbsolute.argtypes = [c_void_p, c_char, POINTER(MYFLT), c_long, c_double]
libcsound.csoundScoreEventAbsoluteAsync.argtypes = [c_void_p, c_char, POINTER(MYFLT), c_long, c_double]
libcsound.csoundInputMessage.argtypes = [c_void_p, c_char_p]
libcsound.csoundInputMessageAsync.argtypes = [c_void_p, c_char_p]
libcsound.csoundKillInstance.argtypes = [c_void_p, MYFLT, c_char_p, c_int, c_int]
SENSEFUNC = CFUNCTYPE(None, c_void_p, py_object)
libcsound.csoundRegisterSenseEventCallback.argtypes = [c_void_p, SENSEFUNC, py_object]
libcsound.csoundKeyPress.argtypes = [c_void_p, c_char]
KEYBOARDFUNC = CFUNCTYPE(c_int, py_object, c_void_p, c_uint)
libcsound.csoundRegisterKeyboardCallback.argtypes = [c_void_p, KEYBOARDFUNC, py_object, c_uint]
libcsound.csoundRemoveKeyboardCallback.argtypes = [c_void_p, KEYBOARDFUNC]

libcsound.csoundTableLength.argtypes = [c_void_p, c_int]
libcsound.csoundTableGet.restype = MYFLT
libcsound.csoundTableGet.argtypes = [c_void_p, c_int, c_int]
libcsound.csoundTableSet.argtypes = [c_void_p, c_int, c_int, MYFLT]
libcsound.csoundTableCopyOut.argtypes = [c_void_p, c_int, POINTER(MYFLT)]
libcsound.csoundTableCopyOutAsync.argtypes = [c_void_p, c_int, POINTER(MYFLT)]
libcsound.csoundTableCopyIn.argtypes = [c_void_p, c_int, POINTER(MYFLT)]
libcsound.csoundTableCopyInAsync.argtypes = [c_void_p, c_int, POINTER(MYFLT)]
libcsound.csoundGetTable.argtypes = [c_void_p, POINTER(POINTER(MYFLT)), c_int]
libcsound.csoundGetTableArgs.argtypes = [c_void_p, POINTER(POINTER(MYFLT)), c_int]
libcsound.csoundIsNamedGEN.argtypes = [c_void_p, c_int]
libcsound.csoundGetNamedGEN.argtypes = [c_void_p, c_int, c_char_p, c_int]

libcsound.csoundSetIsGraphable.argtypes = [c_void_p, c_int]
MAKEGRAPHFUNC = CFUNCTYPE(None, c_void_p, POINTER(Windat), c_char_p)
libcsound.csoundSetMakeGraphCallback.argtypes = [c_void_p, MAKEGRAPHFUNC]
DRAWGRAPHFUNC = CFUNCTYPE(None, c_void_p, POINTER(Windat))
libcsound.csoundSetDrawGraphCallback.argtypes = [c_void_p, DRAWGRAPHFUNC]
KILLGRAPHFUNC = CFUNCTYPE(None, c_void_p, POINTER(Windat))
libcsound.csoundSetKillGraphCallback.argtypes = [c_void_p, KILLGRAPHFUNC]
EXITGRAPHFUNC = CFUNCTYPE(c_int, c_void_p)
libcsound.csoundSetExitGraphCallback.argtypes = [c_void_p, EXITGRAPHFUNC]

libcsound.csoundGetNamedGens.restype = c_void_p
libcsound.csoundGetNamedGens.argtypes = [c_void_p]
libcsound.csoundNewOpcodeList.argtypes = [c_void_p, POINTER(POINTER(OpcodeListEntry))]
libcsound.csoundDisposeOpcodeList.argtypes = [c_void_p, POINTER(OpcodeListEntry)]
OPCODEFUNC = CFUNCTYPE(c_int, c_void_p, c_void_p)
libcsound.csoundAppendOpcode.argtypes = [c_void_p, c_char_p, c_int, c_int, c_int, \
                                         c_char_p, c_char_p, OPCODEFUNC, OPCODEFUNC, OPCODEFUNC]

YIELDFUNC = CFUNCTYPE(c_int, c_void_p)
libcsound.csoundSetYieldCallback.argtypes = [c_void_p, YIELDFUNC]
THREADFUNC = CFUNCTYPE(POINTER(c_uint), py_object)
libcsound.csoundCreateThread.restype = c_void_p
libcsound.csoundCreateThread.argtypes = [THREADFUNC, py_object]
libcsound.csoundGetCurrentThreadId.restype = c_void_p
libcsound.csoundJoinThread.restype = POINTER(c_uint)
libcsound.csoundJoinThread.argtypes = [c_void_p]
libcsound.csoundCreateThreadLock.restype = c_void_p
libcsound.csoundWaitThreadLock.argtypes = [c_void_p, c_uint]
libcsound.csoundWaitThreadLockNoTimeout.argtypes = [c_void_p]
libcsound.csoundNotifyThreadLock.argtypes = [c_void_p]
libcsound.csoundDestroyThreadLock.argtypes = [c_void_p]
libcsound.csoundCreateMutex.restype = c_void_p
libcsound.csoundCreateMutex.argtypes = [c_int]
libcsound.csoundLockMutex.argtypes = [c_void_p]
libcsound.csoundLockMutexNoWait.argtypes = [c_void_p]
libcsound.csoundUnlockMutex.argtypes = [c_void_p]
libcsound.csoundDestroyMutex.argtypes = [c_void_p]
libcsound.csoundCreateBarrier.restype = c_void_p
libcsound.csoundCreateBarrier.argtypes = [c_uint]
libcsound.csoundDestroyBarrier.argtypes = [c_void_p]
libcsound.csoundWaitBarrier.argtypes = [c_void_p]
libcsound.csoundSleep.argtypes = [c_uint]
hasSpinLock = True
try:
    libcsound.csoundSpinLock.argtypes = [c_void_p]
    libcsound.csoundSpinUnLock.argtypes = [c_void_p]
except AttributeError:
    hasSpinLock = False

libcsound.csoundRunCommand.restype = c_long 
libcsound.csoundRunCommand.argtypes = [POINTER(c_char_p), c_int]
libcsound.csoundInitTimerStruct.argtypes = [POINTER(RtClock)]
libcsound.csoundGetRealTime.restype = c_double
libcsound.csoundGetRealTime.argtypes = [POINTER(RtClock)]
libcsound.csoundGetCPUTime.restype = c_double
libcsound.csoundGetCPUTime.argtypes = [POINTER(RtClock)]
libcsound.csoundGetRandomSeedFromTime.restype = c_uint32
libcsound.csoundGetEnv.restype = c_char_p
libcsound.csoundGetEnv.argtypes = [c_void_p, c_char_p]
libcsound.csoundSetGlobalEnv.argtypes = [c_char_p, c_char_p]
libcsound.csoundCreateGlobalVariable.argtypes = [c_void_p, c_char_p, c_uint]
libcsound.csoundQueryGlobalVariable.restype = c_void_p
libcsound.csoundQueryGlobalVariable.argtypes = [c_void_p, c_char_p]
libcsound.csoundQueryGlobalVariableNoCheck.restype = c_void_p
libcsound.csoundQueryGlobalVariableNoCheck.argtypes = [c_void_p, c_char_p]
libcsound.csoundDestroyGlobalVariable.argtypes = [c_void_p, c_char_p]
libcsound.csoundRunUtility.argtypes = [c_void_p, c_char_p, c_int, POINTER(c_char_p)]
libcsound.csoundListUtilities.restype = POINTER(c_char_p)
libcsound.csoundListUtilities.argtypes = [c_void_p]
libcsound.csoundDeleteUtilityList.argtypes = [c_void_p, POINTER(c_char_p)]
libcsound.csoundGetUtilityDescription.restype = c_char_p
libcsound.csoundGetUtilityDescription.argtypes = [c_void_p, c_char_p]
libcsound.csoundRand31.argtypes = [POINTER(c_int)]
libcsound.csoundSeedRandMT.argtypes = [POINTER(CsoundRandMTState), POINTER(c_uint32), c_uint32]
libcsound.csoundRandMT.restype = c_uint32
libcsound.csoundRandMT.argtypes = [POINTER(CsoundRandMTState)]
libcsound.csoundCreateCircularBuffer.restype = c_void_p
libcsound.csoundCreateCircularBuffer.argtypes = [c_void_p, c_int, c_int]
libcsound.csoundReadCircularBuffer.argtypes = [c_void_p, c_void_p, c_void_p, c_int]
libcsound.csoundPeekCircularBuffer.argtypes = [c_void_p, c_void_p, c_void_p, c_int]
libcsound.csoundWriteCircularBuffer.argtypes = [c_void_p, c_void_p, c_void_p, c_int]
libcsound.csoundFlushCircularBuffer.argtypes = [c_void_p, c_void_p]
libcsound.csoundDestroyCircularBuffer.argtypes = [c_void_p, c_void_p]
libcsound.csoundOpenLibrary.argtypes = [POINTER(c_void_p), c_char_p]
libcsound.csoundCloseLibrary.argtypes = [c_void_p]
libcsound.csoundGetLibrarySymbol.retype = c_void_p
libcsound.csoundGetLibrarySymbol.argtypes = [c_void_p, c_char_p]


def cchar(s):
    if sys.version_info[0] >= 3:
        return c_char(ord(s[0]))
    return c_char(s[0])

def cstring(s):
    if sys.version_info[0] >= 3 and s != None:
        return bytes(s, 'utf-8')
    return s

def pstring(s):
    if sys.version_info[0] >= 3 and s != None:
        return str(s, 'utf-8')
    return s

def csoundArgList(lst):
    argc = len(lst)
    argv = (POINTER(c_char_p) * argc)()
    for i in range(argc):
        v = cstring(lst[i])
        argv[i] = cast(pointer(create_string_buffer(v)), POINTER(c_char_p))
    return c_int(argc), cast(argv, POINTER(c_char_p))


# message types (only one can be specified)
CSOUNDMSG_DEFAULT = 0x0000       # standard message
CSOUNDMSG_ERROR = 0x1000         # error message (initerror, perferror, etc.)
CSOUNDMSG_ORCH = 0x2000          # orchestra opcodes (e.g. printks)
CSOUNDMSG_REALTIME = 0x3000      # for progress display and heartbeat characters
CSOUNDMSG_WARNING = 0x4000       # warning messages
CSOUNDMSG_STDOUT = 0x5000

# format attributes (colors etc.), use the bitwise OR of any of these:
CSOUNDMSG_FG_BLACK = 0x0100
CSOUNDMSG_FG_RED = 0x0101
CSOUNDMSG_FG_GREEN = 0x0102
CSOUNDMSG_FG_YELLOW = 0x0103
CSOUNDMSG_FG_BLUE = 0x0104
CSOUNDMSG_FG_MAGENTA = 0x0105
CSOUNDMSG_FG_CYAN = 0x0106
CSOUNDMSG_FG_WHITE = 0x0107

CSOUNDMSG_FG_BOLD = 0x0008
CSOUNDMSG_FG_UNDERLINE = 0x0080

CSOUNDMSG_BG_BLACK = 0x0200
CSOUNDMSG_BG_RED = 0x0210
CSOUNDMSG_BG_GREEN = 0x0220
CSOUNDMSG_BG_ORANGE = 0x0230
CSOUNDMSG_BG_BLUE = 0x0240
CSOUNDMSG_BG_MAGENTA = 0x0250
CSOUNDMSG_BG_CYAN = 0x0260
CSOUNDMSG_BG_GREY = 0x0270

CSOUNDMSG_TYPE_MASK = 0x7000
CSOUNDMSG_FG_COLOR_MASK = 0x0107
CSOUNDMSG_FG_ATTR_MASK = 0x0088
CSOUNDMSG_BG_COLOR_MASK = 0x0270


# ERROR DEFINITIONS
CSOUND_SUCCESS = 0               # Completed successfully.
CSOUND_ERROR = -1                # Unspecified failure.
CSOUND_INITIALIZATION = -2       # Failed during initialization.
CSOUND_PERFORMANCE = -3          # Failed during performance.
CSOUND_MEMORY = -4               # Failed to allocate requested memory.
CSOUND_SIGNAL = -5               # Termination requested by SIGINT or SIGTERM.

# Flags for csoundInitialize().
CSOUNDINIT_NO_SIGNAL_HANDLER = 1
CSOUNDINIT_NO_ATEXIT = 2

# Types for keyboard callbacks set in registerKeyboardCallback()
CSOUND_CALLBACK_KBD_EVENT = 1
CSOUND_CALLBACK_KBD_TEXT = 2

# Constants used by the bus interface (csoundGetChannelPtr() etc.).
CSOUND_CONTROL_CHANNEL = 1
CSOUND_AUDIO_CHANNEL  = 2
CSOUND_STRING_CHANNEL = 3
CSOUND_PVS_CHANNEL = 4
CSOUND_VAR_CHANNEL = 5

CSOUND_CHANNEL_TYPE_MASK = 15

CSOUND_INPUT_CHANNEL = 16
CSOUND_OUTPUT_CHANNEL = 32

CSOUND_CONTROL_CHANNEL_NO_HINTS  = 0
CSOUND_CONTROL_CHANNEL_INT  = 1
CSOUND_CONTROL_CHANNEL_LIN  = 2
CSOUND_CONTROL_CHANNEL_EXP  = 3

# list of languages
CSLANGUAGE_DEFAULT = 0
CSLANGUAGE_AFRIKAANS = 1
CSLANGUAGE_ALBANIAN = 2
CSLANGUAGE_ARABIC = 3
CSLANGUAGE_ARMENIAN = 4
CSLANGUAGE_ASSAMESE = 5
CSLANGUAGE_AZERI = 6
CSLANGUAGE_BASQUE = 7
CSLANGUAGE_BELARUSIAN = 8
CSLANGUAGE_BENGALI = 9
CSLANGUAGE_BULGARIAN = 10
CSLANGUAGE_CATALAN = 11
CSLANGUAGE_CHINESE = 12
CSLANGUAGE_CROATIAN = 13
CSLANGUAGE_CZECH = 14
CSLANGUAGE_DANISH = 15
CSLANGUAGE_DUTCH = 16
CSLANGUAGE_ENGLISH_UK = 17
CSLANGUAGE_ENGLISH_US = 18
CSLANGUAGE_ESTONIAN = 19
CSLANGUAGE_FAEROESE = 20
CSLANGUAGE_FARSI = 21
CSLANGUAGE_FINNISH = 22
CSLANGUAGE_FRENCH = 23
CSLANGUAGE_GEORGIAN = 24
CSLANGUAGE_GERMAN = 25
CSLANGUAGE_GREEK = 26
CSLANGUAGE_GUJARATI = 27
CSLANGUAGE_HEBREW = 28
CSLANGUAGE_HINDI = 29
CSLANGUAGE_HUNGARIAN = 30
CSLANGUAGE_ICELANDIC = 31
CSLANGUAGE_INDONESIAN = 32
CSLANGUAGE_ITALIAN = 33
CSLANGUAGE_JAPANESE = 34
CSLANGUAGE_KANNADA = 35
CSLANGUAGE_KASHMIRI = 36
CSLANGUAGE_KAZAK = 37
CSLANGUAGE_KONKANI = 38
CSLANGUAGE_KOREAN = 39
CSLANGUAGE_LATVIAN = 40
CSLANGUAGE_LITHUANIAN = 41 
CSLANGUAGE_MACEDONIAN = 42
CSLANGUAGE_MALAY = 43
CSLANGUAGE_MALAYALAM = 44
CSLANGUAGE_MANIPURI = 45
CSLANGUAGE_MARATHI = 46
CSLANGUAGE_NEPALI = 47
CSLANGUAGE_NORWEGIAN = 48
CSLANGUAGE_ORIYA = 49
CSLANGUAGE_POLISH = 50
CSLANGUAGE_PORTUGUESE = 51
CSLANGUAGE_PUNJABI = 52
CSLANGUAGE_ROMANIAN = 53
CSLANGUAGE_RUSSIAN = 54
CSLANGUAGE_SANSKRIT = 55
CSLANGUAGE_SERBIAN = 56
CSLANGUAGE_SINDHI = 57
CSLANGUAGE_SLOVAK = 58
CSLANGUAGE_SLOVENIAN = 59
CSLANGUAGE_SPANISH = 60
CSLANGUAGE_SWAHILI = 61
CSLANGUAGE_SWEDISH = 62
CSLANGUAGE_TAMIL = 63
CSLANGUAGE_TATAR = 64
CSLANGUAGE_TELUGU = 65
CSLANGUAGE_THAI = 66
CSLANGUAGE_TURKISH = 67 
CSLANGUAGE_UKRAINIAN = 68
CSLANGUAGE_URDU = 69
CSLANGUAGE_UZBEK = 70
CSLANGUAGE_VIETNAMESE = 71
CSLANGUAGE_COLUMBIAN = 72


#Instantiation
def csoundInitialize(flags):
    """Initialize Csound library with specific flags.
    
    This function is called internally by csoundCreate(), so there is generally
    no need to use it explicitly unless you need to avoid default initialization
    that sets signal handlers and atexit() callbacks.
    Return value is zero on success, positive if initialization was
    done already, and negative on error.
    """
    return libcsound.csoundInitialize(flags)


class Csound:
    #Instantiation
    def __init__(self, hostData=None, pointer_=None):
        """Creates an instance of Csound.
       
        Get an opaque pointer that must be passed to most Csound API
        functions. The hostData parameter can be None, or it can be any
        sort of data; these data can be accessed from the Csound instance
        that is passed to callback routines.
        """
        if pointer_:
            self.cs = pointer_
            self.fromPointer = True
        else:
            self.cs = libcsound.csoundCreate(py_object(hostData))
            self.fromPointer = False
    
    def __del__(self):
        """Destroys an instance of Csound."""
        if not self.fromPointer and libcsound:
            libcsound.csoundDestroy(self.cs)

    def csound(self):
        """Return the opaque pointer to the running Csound instance."""
        return self.cs
    
    def version(self):
        """Returns the version number times 1000 (5.00.0 = 5000)."""
        return libcsound.csoundGetVersion()
    
    def APIVersion(self):
        """Returns the API version number times 100 (1.00 = 100)."""
        return libcsound.csoundGetAPIVersion()
    
    #Performance
    def parseOrc(self, orc):
        """Parse the given orchestra from an ASCII string into a TREE.
        
        This can be called during performance to parse new code.
        """
        return libcsound.csoundParseOrc(self.cs, cstring(orc))
    
    def compileTree(self, tree):
        """Compile the given TREE node into structs for Csound to use.
        
        This can be called during performance to compile a new TREE.
        """
        return libcsound.csoundCompileTree(self.cs, tree)
    
    def compileTreeAsync(self, tree):
        """Asynchronous version of compileTree()."""
        return libcsound.csoundCompileTreeAsync(self.cs, tree)
    
    def deleteTree(self, tree):
        """Free the resources associated with the TREE tree.
        
        This function should be called whenever the TREE was
        created with parseOrc and memory can be deallocated.
        """
        libcsound.csoundDeleteTree(self.cs, tree)

    def compileOrc(self, orc):
        """Parse, and compile the given orchestra from an ASCII string.
        
        Also evaluating any global space code (i-time only).
        This can be called during performance to compile a new orchestra.
        
            orc = "instr 1 \n a1 rand 0dbfs/4 \n out a1 \n"
            cs.compileOrc(orc)
        """
        return libcsound.csoundCompileOrc(self.cs, cstring(orc))
    
    def compileOrcAsync(self, orc):
        """Async version of compileOrc().
        
        The code is parsed and compiled, then placed on a queue for
        asynchronous merge into the running engine, and evaluation.
        The function returns following parsing and compilation.
        """
        return libcsound.csoundCompileOrcAsync(self.cs, cstring(orc))
    
    def evalCode(self, code):
        """Parse and compile an orchestra given on an string.
        
        Evaluating any global space code (i-time only).
        On SUCCESS it returns a value passed to the
        'return' opcode in global space.
        
            code = "i1 = 2 + 2 \n return i1 \n"
            retval = cs.evalCode(code)
        """
        return libcsound.csoundEvalCode(self.cs, cstring(code))
    
    #def initializeCscore(insco, outsco):
    
    def compileArgs(self, *args):
        """Compile args.
        
        Read arguments, parse and compile an orchestra,
        read, process and load a score.
        """
        argc, argv = csoundArgList(args)
        return libcsound.csoundCompileArgs(self.cs, argc, argv)
    
    def start(self):
        """Prepares Csound for performance.
        
        Normally called after compiling a csd file or an orc file, in which
        case score preprocessing is performed and performance terminates
        when the score terminates.
        However, if called before compiling a csd file or an orc file, 
        score preprocessing is not performed and "i" statements are dispatched 
        as real-time events, the <CsOptions> tag is ignored, and performance 
        continues indefinitely or until ended using the API.
        NB: this is called internally by compile_(), therefore
        it is only required if performance is started without
        a call to that function.
        """
        return libcsound.csoundStart(self.cs)
    
    def compile_(self, *args):
        """Compile Csound input files (such as an orchestra and score).
        
        As directed by the supplied command-line arguments,
        but does not perform them. Returns a non-zero error code on failure.
        This function cannot be called during performance, and before a
        repeated call, reset() needs to be called.
        In this (host-driven) mode, the sequence of calls should be as follows:
        
            cs.compile_(args)
            while (cs.performBuffer() == 0)
                pass
            cs.cleanup()
            cs.reset()
        
        Calls start() internally.
        """
        argc, argv = csoundArgList(args)
        return libcsound.csoundCompile(self.cs, argc, argv)
    
    def compileCsd(self, csd_filename):
        """Compile a Csound input file (.csd file).
        
        The input file includes command-line arguments, but does not
        perform the file. Return a non-zero error code on failure.
        In this (host-driven) mode, the sequence of calls should be
        as follows:
        
            cs.compileCsd(args)
            while (cs.performBuffer() == 0)
                pass
            cs.cleanup()
            cs.reset()
        
        NB: this function can be called during performance to
        replace or add new instruments and events.
        On a first call and if called before start(), this function
        behaves similarly to compile_().
        """
        return libcsound.csoundCompileCsd(self.cs, cstring(csd_filename))
    
    def compileCsdText(self, csd_text):
        """Compile a Csound input file contained in a string of text.
        
        The string of text includes command-line arguments, orchestra, score,
        etc., but it is not performed. Returns a non-zero error code on failure.
        In this (host-driven) mode, the sequence of calls should be as follows:
        
            cs.compileCsdText(csd_text);
            while (cs.performBuffer() == 0)
                pass
            cs.cleanup()
            cs.reset()
        
        NB: a temporary file is created, the csd_text is written to the
        temporary file, and compileCsd is called with the name of the temporary
        file, which is deleted after compilation. Behavior may vary by platform.
        """
        return libcsound.csoundCompileCsdText(self.cs, cstring(csd_text))

    def perform(self):
        """Sense input events and performs audio output.
        
        This is done until the end of score is reached (positive return value),
        an error occurs (negative return value), or performance is stopped by
        calling stop() from another thread (zero return value).
        Note that compile_(), or compileOrc(), readScore(), start() must be
        called first.
        In the case of zero return value, perform() can be called again
        to continue the stopped performance. Otherwise, reset() should be
        called to clean up after the finished or failed performance.
        """
        return libcsound.csoundPerform(self.cs)
    
    def performKsmps(self):
        """Sense input events, and performs audio output.
        
        This is done for one control sample worth (ksmps).
        Note that compile_(), or compileOrc(), readScore(), start() must be
        called first.
        Returns False during performance, and True when performance is
        finished. If called until it returns True, will perform an entire
        score.
        Enables external software to control the execution of Csound,
        and to synchronize performance with audio input and output.
        """
        return libcsound.csoundPerformKsmps(self.cs)
    
    def performBuffer(self):
        """Perform Csound, sensing real-time and score events.
        
        Processing one buffer's worth (-b frames) of interleaved audio.
        Note that compile_ must be called first, then call
        outputBuffer() and inputBuffer() to get the pointer
        to csound's I/O buffers.
        Returns false during performance, and true when performance is finished.
        """
        return libcsound.csoundPerformBuffer(self.cs)
    
    def stop(self):
        """Stop a perform() running in another thread.
        
        Note that it is not guaranteed that perform() has already stopped
        when this function returns.
        """
        libcsound.csoundStop(self.cs)
    
    def cleanup(self):
        """Print information and closes audio and MIDI devices.
        
        The information is about the end of a performance.
        Note: after calling cleanup(), the operation of the perform
        functions is undefined.
        """
        return libcsound.csoundCleanup(self.cs)
    
    def reset(self):
        """Reset all internal memory and state.
        
        In preparation for a new performance.
        Enable external software to run successive Csound performances
        without reloading Csound. Implies cleanup(), unless already called.
        """
        libcsound.csoundReset(self.cs)

    #UDP server
    def UDPServerStart(self, port):
        """Starts the UDP server on a supplied port number.
        
        Returns CSOUND_SUCCESS if server has been started successfully,
        otherwise, CSOUND_ERROR.
        """
        return libcsound.csoundUDPServerStart(self.cs, c_uint(port))

    def UDPServerStatus(self):
        """Returns the port number on which the server is running.
        
        If the server is not running, CSOUND_ERROR is returned.
        """
        return libcsound.csoundUDPServerStatus(self.cs)

    def UDPServerClose(self):
        """Closes the UDP server.
        
        Returns CSOUND_SUCCESS if the running server was successfully closed,
        CSOUND_ERROR otherwise.
        """
        return libcsound.csoundUDPServerClose(self.cs)

    def UDPConsole(self, addr, port, mirror):
        """Turns on the transmission of console messages to UDP on addr:port.
        
        If mirror is one, the messages will continue to be sent to the usual
        destination (see setMessaggeCallback()) as well as to UDP.
        Returns CSOUND_SUCCESS or CSOUND_ERROR if the UDP transmission
        could not be set up.
        """
        return libcsound.csoundUDPConsole(self.cs, cstring(addr), c_uint(port), c_uint(mirror))

    def stopUDPConsole(self):
        """Stop transmitting console messages via UDP."""
        libcsound.csoundStopUDPConsole(self.cs)

    #Attributes
    def sr(self):
        """Return the number of audio sample frames per second."""
        return libcsound.csoundGetSr(self.cs)
    
    def kr(self):
        """Return the number of control samples per second."""
        return libcsound.csoundGetKr(self.cs)
    
    def ksmps(self):
        """Return the number of audio sample frames per control sample."""
        return libcsound.csoundGetKsmps(self.cs)
    
    def nchnls(self):
        """Return the number of audio output channels.
        
        Set through the nchnls header variable in the csd file.
        """
        return libcsound.csoundGetNchnls(self.cs)
    
    def nchnlsInput(self): 
        """Return the number of audio input channels.
        
        Set through the nchnls_i header variable in the csd file. If this
        variable is not set, the value is taken from nchnls.
        """
        return libcsound.csoundGetNchnlsInput(self.cs)
    
    def get0dBFS(self):
        """Return the 0dBFS level of the spin/spout buffers."""
        return libcsound.csoundGet0dBFS(self.cs)
    
    def A4(self):
        """Return the A4 frequency reference."""
        return libcsound.csoundGetA4(self.cs)
    
    def currentTimeSamples(self):
        """Return the current performance time in samples."""
        return libcsound.csoundGetCurrentTimeSamples(self.cs)
    
    def sizeOfMYFLT(self):
        """Return the size of MYFLT in bytes."""
        return libcsound.csoundGetSizeOfMYFLT()
    
    def hostData(self):
        """Return host data."""
        return libcsound.csoundGetHostData(self.cs)
    
    def setHostData(self, data):
        """Set host data."""
        libcsound.csoundSetHostData(self.cs, py_object(data))
    
    def setOption(self, option):
        """Set a single csound option (flag).
        
        Returns CSOUND_SUCCESS on success.
        NB: blank spaces are not allowed.
        """
        return libcsound.csoundSetOption(self.cs, cstring(option))
    
    def setParams(self, params):
        """Configure Csound with a given set of parameters.
        
        These parameters are defined in the CsoundParams structure.
        They are the part of the OPARMS struct that are configurable through
        command line flags.
        The CsoundParams structure can be obtained using params().
        These options should only be changed before performance has started.
        """
        libcsound.csoundSetParams(self.cs, byref(params))
    
    def params(self, params):
        """Get the current set of parameters from a CSOUND instance.
        
        These parameters are in a CsoundParams structure. See setParams().
        
            p = CsoundParams()
            cs.params(p)
        """
        libcsound.csoundGetParams(self.cs, byref(params))
    
    def debug(self):
        """Return whether Csound is set to print debug messages.
        
        Those messages are sent through the DebugMsg() internal API function.
        """
        return libcsound.csoundGetDebug(self.cs) != 0
    
    def setDebug(self, debug):
        """Set whether Csound prints debug messages.
        
        The debug argument must have value True or False.
        Those messaged come from the DebugMsg() internal API function.
        """
        libcsound.csoundSetDebug(self.cs, c_int(debug))

    #General Input/Output
    def outputName(self):
        """Return the output audio output name (-o)"""
        s = libcsound.csoundGetOutputName(self.cs)
        return pstring(s)
    
    def setOutput(self, name, type_, format):
        """Set output destination, type and format.
        
        type_ can be one of  "wav", "aiff", "au", "raw", "paf", "svx", "nist",
        "voc", "ircam", "w64", "mat4", "mat5", "pvf", "xi", "htk", "sds",
        "avr", "wavex", "sd2", "flac", "caf", "wve", "ogg", "mpc2k", "rf64",
        or NULL (use default or realtime IO).
        format can be one of "alaw", "schar", "uchar", "float", "double",
        "long", "short", "ulaw", "24bit", "vorbis", or NULL (use default or
        realtime IO).
        For RT audio, use device_id from CS_AUDIODEVICE for a given audio
        device.
        """
        n = cstring(name)
        t = cstring(type_)
        f = cstring(format)
        libcsound.csoundSetOutput(self.cs, n, t, f)
    
    def outputFormat(self):
        """Get output type and format."""
        type_ = create_string_buffer(6)
        format = create_string_buffer(8)
        libcsound.csoundGetOutputFormat(self.cs, type_, format)
        return pstring(string_at(type_)), pstring(string_at(format))

    def setInput(self, name):
        """Set input source."""
        libcsound.csoundSetInput(self.cs, cstring(name))
    
    def setMIDIInput(self, name):
        """Set MIDI input device name/number."""
        libcsound.csoundSetMidiInput(self.cs, cstring(name))
    
    def setMIDIFileInput(self, name):
        """Set MIDI file input name."""
        libcsound.csoundSetMIDIFileInput(self.cs, cstring(name))
    
    def setMIDIOutput(self, name):
        """Set MIDI output device name/number."""
        libcsound.csoundSetMIDIOutput(self.cs, cstring(name))
    
    def setMIDIFileOutput(self, name):
        """Set MIDI file output name."""
        libcsound.csoundSetMIDIFileOutput(self.cs, cstring(name))

    def setFileOpenCallback(self, function):
        """Set a callback for receiving notices whenever Csound opens a file.
        
        The callback is made after the file is successfully opened.
        The following information is passed to the callback:
           bytes  pathname of the file; either full or relative to current dir
           int    a file type code from the enumeration CSOUND_FILETYPES
           int    1 if Csound is writing the file, 0 if reading
           int    1 if a temporary file that Csound will delete; 0 if not
           
        Pass NULL to disable the callback.
        This callback is retained after a csoundReset() call.
        """
        self.fileOpenCbRef = FILEOPENFUNC(function)
        libcsound.csoundSetFileOpenCallback(self.cs, self.fileOpenCbRef)

    #Realtime Audio I/O
    def setRTAudioModule(self, module):
        """Set the current RT audio module."""
        libcsound.csoundSetRTAudioModule(self.cs, cstring(module))
    
    def module(self, number):
        """Retrieve a module name and type given a number.
        
        Type is "audio" or "midi". Modules are added to list as csound loads
        them. Return CSOUND_SUCCESS on success and CSOUND_ERROR if module
        number was not found.
        
            n = 0
            while True:
                name, type_, err = cs.module(n)
                if err == ctcsound.CSOUND_ERROR:
                    break
                print("Module %d:%s (%s)\n" % (n, name, type_))
                n = n + 1
        """
        name = pointer(c_char_p(cstring("dummy")))
        type_ = pointer(c_char_p(cstring("dummy")))
        err = libcsound.csoundGetModule(self.cs, number, name, type_)
        if err == CSOUND_ERROR:
            return None, None, err
        n = pstring(string_at(name.contents))
        t = pstring(string_at(type_.contents))
        return n, t, err
    
    def inputBufferSize(self):
        """Return the number of samples in Csound's input buffer."""
        return libcsound.csoundGetInputBufferSize(self.cs)
    
    def outputBufferSize(self):
        """Return the number of samples in Csound's output buffer."""
        return libcsound.csoundGetOutputBufferSize(self.cs)
    
    def inputBuffer(self):
        """Return the Csound audio input buffer as a numpy array.
        
        Enable external software to write audio into Csound before
        calling performBuffer.
        """
        buf = libcsound.csoundGetInputBuffer(self.cs)
        size = libcsound.csoundGetInputBufferSize(self.cs)
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = cast(buf, arrayType)
        return np.ctypeslib.as_array(p)
    
    def outputBuffer(self):
        """Return the Csound audio output buffer as a numpy array.
        
        Enable external software to read audio from Csound after
        calling performBuffer.
        """
        buf = libcsound.csoundGetOutputBuffer(self.cs)
        size = libcsound.csoundGetOutputBufferSize(self.cs)
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = cast(buf, arrayType)
        return np.ctypeslib.as_array(p)
    
    def spin(self):
        """Return the Csound audio input working buffer (spin) as a numpy array.
        
        Enables external software to write audio into Csound before
        calling performKsmps.
        """
        buf = libcsound.csoundGetSpin(self.cs)
        size = self.ksmps() * self.nchnlsInput()
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = cast(buf, arrayType)
        return np.ctypeslib.as_array(p)
    
    def clearSpin(self):
        """Clear the input buffer (spin)."""
        libcsound.csoundClearSpin(self.cs)
    
    def addSpinSample(self, frame, channel, sample):
        """Add the indicated sample into the audio input working buffer (spin).
        
        This only ever makes sense before calling performKsmps(). The frame
        and channel must be in bounds relative to ksmps and nchnlsInput.
        NB: the spin buffer needs to be cleared at every k-cycle by calling 
        clearSpin().
        """
        libcsound.csoundAddSpinSample(self.cs, frame, channel, sample)
    
    def setSpinSample(self, frame, channel, sample):
        """Set the audio input working buffer (spin) to the indicated sample.
        
        This only ever makes sense before calling performKsmps(). The frame
        and channel must be in bounds relative to ksmps and nchnlsInput.
        """
        libcsound.csoundSetSpinSample(self.cs, frame, channel, sample)
    
    def spout(self):
        """Return the address of the Csound audio output working buffer (spout).
        
        Enable external software to read audio from Csound after
        calling performKsmps.
        """
        buf = libcsound.csoundGetSpout(self.cs)
        size = self.ksmps() * self.nchnls()
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = cast(buf, arrayType)
        return np.ctypeslib.as_array(p)
    
    def spoutSample(self, frame, channel):
        """Return one sample from the Csound audio output working buf (spout).
        
        Only ever makes sense after calling performKsmps(). The frame and
        channel must be in bounds relative to ksmps and nchnls.
        """
        return libcsound.csoundGetSpoutSample(self.cs, frame, channel)
    
    def rtRecordUserData(self):
        """Return pointer to user data pointer for real time audio input."""
        return libcsound.csoundGetRtRecordUserData(self.cs)

    def rtPlaydUserData(self):
        """Return pointer to user data pointer for real time audio output."""
        return libcsound.csoundGetRtPlayUserData(self.cs)

    def setHostImplementedAudioIO(self, state, bufSize):
        """Set user handling of sound I/O.
        
        Calling this function with a True 'state' value between creation of
        the Csound object and the start of performance will disable all default
        handling of sound I/O by the Csound library, allowing the host
        application to use the spin/spout/input/output buffers directly.
        For applications using spin/spout, bufSize should be set to 0.
        If 'bufSize' is greater than zero, the buffer size (-b) will be
        set to the integer multiple of ksmps that is nearest to the value
        specified.
        """
        libcsound.csoundSetHostImplementedAudioIO(self.cs, c_int(state), bufSize)

    def audioDevList(self, isOutput):
        """Return a list of available input or output audio devices.
        
        Each item in the list is a dictionnary representing a device. The
        dictionnary keys are "device_name", "device_id", "rt_module" (value
        type string), "max_nchnls" (value type int), and "isOutput" (value 
        type boolean). Must be called after an orchestra has been compiled
        to get meaningful information.
        """
        n = libcsound.csoundGetAudioDevList(self.cs, None, c_int(isOutput))
        devs = (CsoundAudioDevice * n)()
        libcsound.csoundGetAudioDevList(self.cs, byref(devs), c_int(isOutput))
        lst = []
        for dev in devs:
            d = {}
            d["device_name"] = pstring(dev.device_name)
            d["device_id"] = pstring(dev.device_id)
            d["rt_module"] = pstring(dev.rt_module)
            d["max_nchnls"] = dev.max_nchnls
            d["isOutput"] = (dev.isOutput == 1)
            lst.append(d)
        return lst

    def setPlayOpenCallback(self, function):
        """Set a callback for opening real-time audio playback."""
        self.playOpenCbRef = PLAYOPENFUNC(function)
        libcsound.csoundSetPlayopenCallback(self.cs, self.playOpenCbRef)

    def setRtPlayCallback(self, function):
        """Set a callback for performing real-time audio playback."""
        self.rtPlayCbRef = RTPLAYFUNC(function)
        libcsound.csoundSetRtplayCallback(self.cs, self.rtPlayCbRef)
  
    def setRecordOpenCallback(self, function):
        """Set a callback for opening real-time audio recording."""
        self.recordOpenCbRef = RECORDOPENFUNC(function)
        libcsound.csoundSetRecopenCallback(self.cs, self.recordOpenCbRef)

    def setRtRecordCallback(self, function):
        """Set a callback for performing real-time audio recording."""
        self.rtRecordCbRef = RTRECORDFUNC(function)
        libcsound.csoundSetRtrecordCallback(self.cs, self.rtRecordCbRef)
    
    def setRtCloseCallback(self, function):
        """Set a callback for closing real-time audio playback and recording."""
        self.rtCloseCbRef = RTCLOSEFUNC(function)
        libcsound.csoundSetRtcloseCallback(self.cs, self.rtCloseCbRef)
    
    def setAudioDevListCallback(self, function):
        """Set a callback for obtaining a list of audio devices.
        
        This should be set by rtaudio modules and should not be set by hosts.
        (See audioDevList()).
        """
        self.audioDevListCbRef = AUDIODEVLISTFUNC(function)
        libcsound.csoundSetAudioDeviceListCallback(self.cs, self.audioDevListCbRef)
    
    #Realtime MIDI I/O
    def setMIDIModule(self, module):
        """Sets the current MIDI IO module."""
        libcsound.csoundSetMIDIModule(self.cs, cstring(module))
    
    def setHostImplementedMIDIIO(self, state):
        """Called with state True if the host is implementing via callbacks."""
        libcsound.csoundSetHostImplementedMIDIIO(self.cs, c_int(state))
    
    def midiDevList(self, isOutput):
        """Return a list of available input or output midi devices.
        
        Each item in the list is a dictionnary representing a device. The
        dictionnary keys are "device_name", "interface_name", "device_id",
        "midi_module" (value type string), "isOutput" (value type boolean).
        Must be called after an orchestra has been compiled
        to get meaningful information.
        """
        n = libcsound.csoundGetMIDIDevList(self.cs, None, c_int(isOutput))
        devs = (csoundMidiDevice * n)()
        libcsound.csoundGetMIDIDevList(self.cs, byref(devs), c_int(isOutput))
        lst = []
        for dev in devs:
            d = {}
            d["device_name"] = pstring(dev.device_name)
            d["interface_name"] = pstring(dev.max_nchnlsinterface_name)
            d["device_id"] = pstring(dev.device_id)
            d["midi_module"] = pstring(dev.midi_module)
            d["isOutput"] = (dev.isOutput == 1)
            lst.append(d)
        return lst

    def setExternalMidiInOpenCallback(self, function):
        """Set a callback for opening real-time MIDI input."""
        self.extMidiInOpenCbRef = MIDIINOPENFUNC(function)
        libcsound.csoundSetExternalMidiInOpenCallback(self.cs, self.extMidiInOpenCbRef)

    def setExternalMidiReadCallback(self, function):
        """Set a callback for reading from real time MIDI input."""
        self.extMidiReadCbRef = MIDIREADFUNC(function)
        libcsound.csoundSetExternalMidiReadCallback(self.cs, self.extMidiReadCbRef)
    
    def setExternalMidiInCloseCallback(self, function):                
        """Set a callback for closing real time MIDI input."""
        self.extMidiInCloseCbRef = MIDIINCLOSEFUNC(function)
        libcsound.csoundSetExternalMidiInCloseCallback(self.cs, self.extMidiInCloseCbRef)
    
    def setExternalMidiOutOpenCallback(self, function):
        """Set a callback for opening real-time MIDI input."""
        self.extMidiOutOpenCbRef = MIDIOUTOPENFUNC(function)
        libcsound.csoundSetExternalMidiOutOpenCallback(self.cs, self.extMidiOutOpenCbRef)

    def setExternalMidiWriteCallback(self, function):
        """Set a callback for reading from real time MIDI input."""
        self.extMidiWriteCbRef = MIDIWRITEFUNC(function)
        libcsound.csoundSetExternalMidiWriteCallback(self.cs, self.extMidiWriteCbRef)
    
    def setExternalMidiOutCloseCallback(self, function):
        """Set a callback for closing real time MIDI input."""
        self.extMidiOutCloseCbRef = MIDIOUTCLOSEFUNC(function)
        libcsound.csoundSetExternalMidiOutCloseCallback(self.cs, self.extMidiOutCloseCbRef)

    def setExternalMidiErrorStringCallback(self, function):
        """ Set a callback for converting MIDI error codes to strings."""
        self.extMidiErrStrCbRef = MIDIERRORFUNC(function)
        libcsound.csoundSetExternalMidiErrorStringCallback(self.cs, self.extMidiErrStrCbRef)
    
    def setMidiDevListCallback(self, function):
        """Set a callback for obtaining a list of MIDI devices.
        
        This should be set by IO plugins and should not be set by hosts.
        (See midiDevList()).
        """
        self.midiDevListCbRef = MIDIDEVLISTFUNC(function)
        libcsound.csoundSetMIDIDeviceListCallback(self.cs, self.midiDevListCbRef)

    #Score Handling
    def readScore(self, sco):
        """Read, preprocess, and load a score from an ASCII string.
        
        It can be called repeatedly, with the new score events
        being added to the currently scheduled ones.
        """
        return libcsound.csoundReadScore(self.cs, cstring(sco))
    
    def readScoreAsync(self, sco):
        """Asynchronous version of readScore()."""
        libcsound.csoundReadScoreAsync(self.cs, cstring(sco))
    
    def scoreTime(self):
        """Returns the current score time.
        
        The return value is the time in seconds since the beginning of
        performance.
        """
        return libcsound.csoundGetScoreTime(self.cs)
    
    def isScorePending(self):
        """Tell whether Csound score events are performed or not.
        
        Independently of real-time MIDI events (see setScorePending()).
        """
        return libcsound.csoundIsScorePending(self.cs) != 0
    
    def setScorePending(self, pending):
        """Set whether Csound score events are performed or not.
        
        Real-time events will continue to be performed. Can be used by external
        software, such as a VST host, to turn off performance of score events
        (while continuing to perform real-time events), for example to mute
        a Csound score while working on other tracks of a piece, or to play
        the Csound instruments live.
        """
        libcsound.csoundSetScorePending(self.cs, c_int(pending))
    
    def scoreOffsetSeconds(self):
        """Return the score time beginning midway through a Csound score.
        
        At this time score events will actually immediately be performed
        (see setScoreOffsetSeconds()).
        """
        return libcsound.csoundGetScoreOffsetSeconds(self.cs)

    def setScoreOffsetSeconds(self, time):
        """Csound score events prior to the specified time are not performed.
        
        Performance begins immediately at the specified time (real-time events
        will continue to be performed as they are received). Can be used by
        external software, such as a VST host, to begin score performance
        midway through a Csound score, for example to repeat a loop in a
        sequencer, or to synchronize other events with the Csound score.
        """
        libcsound.csoundSetScoreOffsetSeconds(self.cs, MYFLT(time))
    
    def rewindScore(self):
        """Rewinds a compiled Csound score.
        
        It is rewinded to the time specified with setScoreOffsetSeconds().
        """
        libcsound.csoundRewindScore(self.cs)
    
    def setCscoreCallback(self, function):
        """Set an external callback for Cscore processing.
        
        Pass None to reset to the internal cscore() function (which does
        nothing). This callback is retained after a reset() call.
        """
        self.cscoreCbRef = CSCOREFUNC(function)
        libcsound.csoundSetCscoreCallback(self.cs, self.cscoreCbRef)
    
    #def scoreSort(self, inFile, outFile):
    
    #def scoreExtract(self, inFile, outFile, extractFile)
    
    #Messages and Text
    def message(self, fmt, *args):
        """Display an informational message.
        
        This is a workaround because ctypes does not support variadic functions.
        The arguments are formatted in a string, using the python way, either
        old style or new style, and then this formatted string is passed to
        the csound display message system.
        """
        if fmt[0] == '{':
            s = fmt.format(*args)
        else:
            s = fmt % args
        libcsound.csoundMessage(self.cs, cstring("%s"), cstring(s))
    
    def messageS(self, attr, fmt, *args):
        """Print message with special attributes.
        
        (See msg_attr for the list of available attributes). With attr=0,
        messageS() is identical to message().
        This is a workaround because ctypes does not support variadic functions.
        The arguments are formatted in a string, using the python way, either
        old style or new style, and then this formatted string is passed to
        the csound display message system.
        """
        if fmt[0] == '{':
            s = fmt.format(*args)
        else:
            s = fmt % args
        libcsound.csoundMessageS(self.cs, attr, cstring("%s"), cstring(s))

    #def setDefaultMessageCallback():
    
    #def setMessageCallback():
    
    def messageLevel(self):
        """Return the Csound message level (from 0 to 231)."""
        return libcsound.csoundGetMessageLevel(self.cs)
    
    def setMessageLevel(self, messageLevel):
        """Set the Csound message level (from 0 to 231)."""
        libcsound.csoundSetMessageLevel(self.cs, messageLevel)
    
    def createMessageBuffer(self, toStdOut):
        """Create a buffer for storing messages printed by Csound.
        
        Should be called after creating a Csound instance and the buffer
        can be freed by calling destroyMessageBuffer() before deleting the
        Csound instance. You will generally want to call cleanup() to make
        sure the last messages are flushed to the message buffer before
        destroying Csound.
        If 'toStdOut' is True, the messages are also printed to
        stdout and stderr (depending on the type of the message),
        in addition to being stored in the buffer.
        Using the message buffer ties up the internal message callback, so
        setMessageCallback should not be called after creating the
        message buffer.
        """
        libcsound.csoundCreateMessageBuffer(self.cs,  c_int(toStdOut))
    
    def firstMessage(self):
        """Return the first message from the buffer."""
        s = libcsound.csoundGetFirstMessage(self.cs)
        return pstring(s)
    
    def firstMessageAttr(self):
        """Return the attribute parameter of the first message in the buffer."""
        return libcsound.csoundGetFirstMessageAttr(self.cs)
    
    def popFirstMessage(self):
        """Remove the first message from the buffer."""
        libcsound.csoundPopFirstMessage(self.cs)
    
    def messageCnt(self):
        """Return the number of pending messages in the buffer."""
        return libcsound.csoundGetMessageCnt(self.cs)
    
    def destroyMessageBuffer(self):
        """Release all memory used by the message buffer."""
        libcsound.csoundDestroyMessageBuffer(self.cs)
    
    #Channels, Control and Events
    def channelPtr(self, name, type_):
        """Return a pointer to the specified channel and an error message.
        
        If the channel is a control or an audio channel, the pointer is
        translated to an ndarray of MYFLT. If the channel is a string channel,
        the pointer is casted to c_char_p. The error message is either an empty
        string or a string describing the error that occured.
        
        The channel is created first if it does not exist yet.
        'type_' must be the bitwise OR of exactly one of the following values,
          CSOUND_CONTROL_CHANNEL
            control data (one MYFLT value)
          CSOUND_AUDIO_CHANNEL
            audio data (csoundGetKsmps(csound) MYFLT values)
          CSOUND_STRING_CHANNEL
            string data (MYFLT values with enough space to store
            csoundGetChannelDatasize() characters, including the
            NULL character at the end of the string)
        and at least one of these:
          CSOUND_INPUT_CHANNEL
          CSOUND_OUTPUT_CHANNEL
        If the channel already exists, it must match the data type
        (control, audio, or string), however, the input/output bits are
        OR'd with the new value. Note that audio and string channels
        can only be created after calling csoundCompile(), because the
        storage size is not known until then.

        In the C API, return value is zero on success, or a negative error code,
          CSOUND_MEMORY  there is not enough memory for allocating the channel
          CSOUND_ERROR   the specified name or type is invalid
        or, if a channel with the same name but incompatible type
        already exists, the type of the existing channel. In the case
        of any non-zero return value, *p is set to NULL.
        Note: to find out the type of a channel without actually
        creating or changing it, set 'type' to zero, so that the return
        value will be either the type of the channel, or CSOUND_ERROR
        if it does not exist.
        
        Operations on the pointer are not thread-safe by default. The host is
        required to take care of threadsafety by retrieving the channel lock
        with channelLock() and using spinLock() and spinUnLock() to protect
        access to the pointer.
        See Top/threadsafe.c in the Csound library sources for
        examples.  Optionally, use the channel get/set functions
        provided below, which are threadsafe by default.
        """
        length = 0
        chanType = type_ & CSOUND_CHANNEL_TYPE_MASK
        if chanType == CSOUND_CONTROL_CHANNEL:
            length = 1
        elif chanType == CSOUND_AUDIO_CHANNEL:
            length = libcsound.csoundGetKsmps(self.cs)
        ptr = POINTER(MYFLT)()
        err = ''
        ret = libcsound.csoundGetChannelPtr(self.cs, byref(ptr), cstring(name), type_)
        if ret == CSOUND_SUCCESS:
            if chanType == CSOUND_STRING_CHANNEL:
                return cast(ptr, c_char_p), err
            else:
                arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (length,), 'C_CONTIGUOUS')
                p = cast(ptr, arrayType)
                return np.ctypeslib.as_array(p), err
        elif ret == CSOUND_MEMORY:
            err = 'Not enough memory for allocating channel'
        elif ret == CSOUND_ERROR:
            err = 'The specified channel name or type is not valid'
        elif ret == CSOUND_CONTROL_CHANNEL:
            err = 'A control channel named {} already exists'.format(name)
        elif ret == CSOUND_AUDIO_CHANNEL:
            err = 'An audio channel named {} already exists'.format(name)
        elif ret == CSOUND_STRING_CHANNEL:
            err = 'A string channel named {} already exists'.format(name)
        else:
            err = 'Unknown error'
        return None, err
    
    def listChannels(self):
        """Return a pointer and an error message.
        
        The pointer points to a list of ControlChannelInfo objects for allocated
        channels. A ControlChannelInfo object contains the channel
        characteristics. The error message indicates if there is not enough
        memory for allocating the list or it is an empty string if there is no
        error. In the case of no channels or an error, the pointer is None.

        Notes: the caller is responsible for freeing the list returned by the
        C API with deleteChannelList(). The name pointers may become invalid
        after calling reset().
        """
        cInfos = None
        err = ''
        ptr = cast(POINTER(c_int)(), POINTER(ControlChannelInfo))
        n = libcsound.csoundListChannels(self.cs, byref(ptr))
        if n == CSOUND_MEMORY :
            err = 'There is not enough memory for allocating the list'
        if n > 0:
            cInfos = cast(ptr, POINTER(ControlChannelInfo * n)).contents
        return cInfos, err

    def deleteChannelList(self, lst):
        """Release a channel list previously returned by listChannels()."""
        ptr = cast(lst, POINTER(ControlChannelInfo))
        libcsound.csoundDeleteChannelList(self.cs, ptr)
    
    def setControlChannelHints(self, name, hints):
        """Set parameters hints for a control channel.
        
        These hints have no internal function but can be used by front ends to
        construct GUIs or to constrain values. See the ControlChannelHints
        structure for details.
        Returns zero on success, or a non-zero error code on failure:
          CSOUND_ERROR:  the channel does not exist, is not a control channel,
                         or the specified parameters are invalid
          CSOUND_MEMORY: could not allocate memory
        """
        return libcsound.csoundSetControlChannelHints(self.cs, cstring(name), hints)

    def controlChannelHints(self, name):
        """Return special parameters (if any) of a control channel.
        
        Those parameters have been previously set with setControlChannelHints()
        or the chnparams opcode.
       
        The return values are a ControlChannelHints structure and CSOUND_SUCCESS
        if the channel exists and is a control channel, otherwise, None and an
        error code are returned.
        """
        hints = ControlChannelHints()
        ret = libcsound.csoundGetControlChannelHints(self.cs, cstring(name), byref(hints))
        if ret != CSOUND_SUCCESS:
            hints = None
        return hints, ret
    
    def channelLock(self, name):
        """Recover a pointer to a lock for the specified channel called 'name'.
        
        
        The returned lock can be locked/unlocked  with the spinLock() and
        spinUnLock() functions.
        Return the address of the lock or NULL if the channel does not exist.
        """
        return libcsound.csoundGetChannelLock(self.cs, cstring(name))
    
    def controlChannel(self, name):
        """Retrieve the value of control channel identified by name.
        
        A second value is returned, which the error (or success) code
        finding or accessing the channel.
        """
        err = c_int(0)
        ret = libcsound.csoundGetControlChannel(self.cs, cstring(name), byref(err))
        return ret, err
    
    def setControlChannel(self, name, val):
        """Set the value of control channel identified by name."""
        libcsound.csoundSetControlChannel(self.cs, cstring(name), MYFLT(val))
    
    def audioChannel(self, name, samples):
        """Copy the audio channel identified by name into ndarray samples.
        
        samples should contain enough memory for ksmps MYFLTs.
        """
        ptr = samples.ctypes.data_as(POINTER(MYFLT))
        libcsound.csoundGetAudioChannel(self.cs, cstring(name), ptr)
    
    def setAudioChannel(self, name, samples):
        """Set the audio channel 'name' with data from ndarray 'samples'.
        
        'samples' should contain at least ksmps MYFLTs.
        """
        ptr = samples.ctypes.data_as(POINTER(MYFLT))
        libcsound.csoundSetAudioChannel(self.cs, cstring(name), ptr)
    
    def stringChannel(self, name, string):
        """Copy the string channel identified by name into string.
        
        string should contain enough memory for the string
        (see channelDatasize() below).
        """
        libcsound.csoundGetStringChannel(self.cs, cstring(name), cstring(string))

    def setStringChannel(self, name, string):
        """Set the string channel identified by name with string."""
        libcsound.csoundSetStringChannel(self.cs, cstring(name), cstring(string))
    
    def channelDatasize(self, name):
        """Return the size of data stored in a channel.
        
        For string channels this might change if the channel space gets
        reallocated. Since string variables use dynamic memory allocation in
        Csound6, this function can be called to get the space required for
        stringChannel().
        """
        return libcsound.csoundGetChannelDatasize(self.cs, cstring(name))

    def setInputChannelCallback(self, function):
        """Set the function to call whenever the invalue opcode is used."""
        self.inputChannelCbRef = CHANNELFUNC(function)
        libcsound.csoundSetInputChannelCallback(self.cs, self.inputChannelCbRef)
    
    def setOutputChannelCallback(self, function):
        """Set the function to call whenever the outvalue opcode is used."""
        self.outputChannelCbRef = CHANNELFUNC(function)
        libcsound.csoundSetOutputChannelCallback(self.cs, self.outputChannelCbRef)

    def setPvsChannel(self, fin, name):
        """Send a PvsdatExt fin to the pvsin opcode (f-rate) for channel 'name'.
        
        Return zero on success, CSOUND_ERROR if the index is invalid or
        fsig framesizes are incompatible.
        CSOUND_MEMORY if there is not enough memory to extend the bus.
        """
        return libcsound.csoundSetPvsChannel(self.cs, byref(fin), cstring(name))
    
    def pvsChannel(self, fout, name):
        """Receive a PvsdatExt fout from the pvsout opcode (f-rate) at channel 'name'.
        
        Return zero on success, CSOUND_ERROR if the index is invalid or
        if fsig framesizes are incompatible.
        CSOUND_MEMORY if there is not enough memory to extend the bus.
        """
        return libcsound.csoundGetPvsChannel(self.cs, byref(fout), cstring(name))
    
    def scoreEvent(self, type_, pFields):
        """Send a new score event.
        
        'type_' is the score event type ('a', 'i', 'q', 'f', or 'e').
        'pFields' is tuple, a list, or an ndarray of MYFLTs with all the pfields
        for this event, starting with the p1 value specified in pFields[0].
        """
        p = np.array(pFields).astype(MYFLT)
        ptr = p.ctypes.data_as(POINTER(MYFLT))
        numFields = c_long(p.size)
        return libcsound.csoundScoreEvent(self.cs, cchar(type_), ptr, numFields)
    
    def scoreEventAsync(self, type_, pFields):
        """Asynchronous version of scoreEvent()."""
        p = np.array(pFields).astype(MYFLT)
        ptr = p.ctypes.data_as(POINTER(MYFLT))
        numFields = c_long(p.size)
        libcsound.csoundScoreEventAsync(self.cs, cchar(type_), ptr, numFields)
    
    def scoreEventAbsolute(self, type_, pFields, timeOffset):
        """Like scoreEvent(), this function inserts a score event.
        
        The event is inserted at absolute time with respect to the start of
        performance, or from an offset set with timeOffset.
        """
        p = np.array(pFields).astype(MYFLT)
        ptr = p.ctypes.data_as(POINTER(MYFLT))
        numFields = c_long(p.size)
        return libcsound.csoundScoreEventAbsolute(self.cs, cchar(type_), ptr, numFields, c_double(timeOffset))
    
    def scoreEventAbsoluteAsync(self, type_, pFields, timeOffset):
        """Asynchronous version of scoreEventAbsolute()."""
        p = np.array(pFields).astype(MYFLT)
        ptr = p.ctypes.data_as(POINTER(MYFLT))
        numFields = c_long(p.size)
        libcsound.csoundScoreEventAbsoluteAsync(self.cs, cchar(type_), ptr, numFields, c_double(timeOffset))
    
    def inputMessage(self, message):
        """Input a NULL-terminated string (as if from a console).
        
        Used for line events.
        """
        libcsound.csoundInputMessage(self.cs, cstring(message))
    
    def inputMessageAsync(self, message):
        """Asynchronous version of inputMessage()."""
        libcsound.csoundInputMessageAsync(self.cs, cstring(message))
    
    def killInstance(self, instr, instrName, mode, allowRelease):
        """Kills off one or more running instances of an instrument.
        
        The instances are identified by instr (number) or instrName (name).
        If instrName is None, the instrument number is used.
        Mode is a sum of the following values:
        0, 1, 2: kill all instances (0), oldest only (1), or newest (2)
        4: only turnoff notes with exactly matching (fractional) instr number
        8: only turnoff notes with indefinite duration (p3 < 0 or MIDI).
        If allowRelease is True, the killed instances are allowed to release.
        """
        return libcsound.csoundKillInstance(self.cs, MYFLT(instr), cstring(instrName), mode, c_int(allowRelease))
    
    def registerSenseEventCallback(self, function, userData):
        """Register a function to be called by sensevents().
        
        This function will be called once in every control period. Any number
        of functions may be registered, and will be called in the order of
        registration.
        The callback function takes two arguments: the Csound instance
        pointer, and the userData pointer as passed to this function.
        This facility can be used to ensure a function is called synchronously
        before every csound control buffer processing. It is important
        to make sure no blocking operations are performed in the callback.
        The callbacks are cleared on cleanup().
        Return zero on success.
        """
        self.senseEventCbRef = SENSEFUNC(function)
        return libcsound.csoundRegisterSenseEventCallback(self.cs, self.senseEventCbRef, py_object(userData))
    
    def keyPress(self, c):
        """Set the ASCII code of the most recent key pressed.
        
        This value is used by the 'sensekey' opcode if a callback for
        returning keyboard events is not set (see registerKeyboardCallback()).
        """
        libcsound.csoundKeyPress(self.cs, cchar(c))
    
    def registerKeyboardCallback(self, function, userData, type_):
        """Registers general purpose callback functions for keyboard events.
        
        These callbacks are called on every control period by the sensekey
        opcode.
        The callback is preserved on reset(), and multiple
        callbacks may be set and will be called in reverse order of
        registration. If the same function is set again, it is only moved
        in the list of callbacks so that it will be called first, and the
        user data and type mask parameters are updated. 'type_' can be the
        bitwise OR of callback types for which the function should be called,
        or zero for all types.
        Returns zero on success, CSOUND_ERROR if the specified function
        pointer or type mask is invalid, and CSOUND_MEMORY if there is not
        enough memory.
        
        The callback function takes the following arguments:
          userData
            the "user data" pointer, as specified when setting the callback
          p
            data pointer, depending on the callback type
          type_
            callback type, can be one of the following (more may be added in
            future versions of Csound):
              CSOUND_CALLBACK_KBD_EVENT
              CSOUND_CALLBACK_KBD_TEXT
                called by the sensekey opcode to fetch key codes. The data
                pointer is a pointer to a single value of type 'int', for
                returning the key code, which can be in the range 1 to 65535,
                or 0 if there is no keyboard event.
                For CSOUND_CALLBACK_KBD_EVENT, both key press and release
                events should be returned (with 65536 (0x10000) added to the
                key code in the latter case) as unshifted ASCII codes.
                CSOUND_CALLBACK_KBD_TEXT expects key press events only as the
                actual text that is typed.
        The return value should be zero on success, negative on error, and
        positive if the callback was ignored (for example because the type is
        not known).
        """
        if type_ == CSOUND_CALLBACK_KBD_EVENT:
            self.keyboardCbEventRef = KEYBOARDFUNC(function)
        else:
            self.keyboardCbTextRef = KEYBOARDFUNC(function)
        return libcsound.csoundRegisterKeyboardCallback(self.cs, KEYBOARDFUNC(function), py_object(userData), c_uint(type_))
    
    def removeKeyboardCallback(self, function):
        """Remove a callback previously set with registerKeyboardCallback()."""
        libcsound.csoundRemoveKeyboardCallback(self.cs, KEYBOARDFUNC(function))
    
    #Tables
    def tableLength(self, table):
        """Return the length of a function table.
        
        (not including the guard point).
        If the table does not exist, return -1.
        """
        return libcsound.csoundTableLength(self.cs, table)
    
    def tableGet(self, table, index):
        """Return the value of a slot in a function table.
        
        The table number and index are assumed to be valid.
        """
        return libcsound.csoundTableGet(self.cs, table, index)
    
    def tableSet(self, table, index, value):
        """Set the value of a slot in a function table.
        
        The table number and index are assumed to be valid.
        """
        libcsound.csoundTableSet(self.cs, table, index, MYFLT(value))
    
    def tableCopyOut(self, table, dest):
        """Copy the contents of a function table into a supplied ndarray dest.
        
        The table number is assumed to be valid, and the destination needs to
        have sufficient space to receive all the function table contents.
        """
        ptr = dest.ctypes.data_as(POINTER(MYFLT))
        libcsound.csoundTableCopyOut(self.cs, table, ptr)
    
    def tableCopyOutAsync(self, table, dest):
        """Asynchronous version of tableCopyOut()."""
        ptr = dest.ctypes.data_as(POINTER(MYFLT))
        libcsound.csoundTableCopyOutAsync(self.cs, table, ptr)
    
    def tableCopyIn(self, table, src):
        """Copy the contents of an ndarray src into a given function table.
        
        The table number is assumed to be valid, and the table needs to
        have sufficient space to receive all the array contents.
        """
        ptr = src.ctypes.data_as(POINTER(MYFLT))
        libcsound.csoundTableCopyIn(self.cs, table, ptr)
    
    def tableCopyInAsync(self, table, src):
        """Asynchronous version of tableCopyIn()."""
        ptr = src.ctypes.data_as(POINTER(MYFLT))
        libcsound.csoundTableCopyInAsync(self.cs, table, ptr)
    
    def table(self, tableNum):
        """Return a pointer to function table 'tableNum' as an ndarray.
        
        The ndarray does not include the guard point. If the table does not
        exist, None is returned.
        """
        ptr = POINTER(MYFLT)()
        size = libcsound.csoundGetTable(self.cs, byref(ptr), tableNum)
        if size < 0:
            return None
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = cast(ptr, arrayType)
        return np.ctypeslib.as_array(p)
        
    def tableArgs(self, tableNum):
        """Return a pointer to the args used to generate a function table.
        
        The pointer is returned as an ndarray. If the table does not exist,
        None is returned.
        NB: the argument list starts with the GEN number and is followed by
        its parameters. eg. f 1 0 1024 10 1 0.5  yields the list
        {10.0, 1.0, 0.5}
        """
        ptr = POINTER(MYFLT)()
        size = libcsound.csoundGetTableArgs(self.cs, byref(ptr), tableNum)
        if size < 0:
            return None
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = cast(ptr, arrayType)
        return np.ctypeslib.as_array(p)
    
    def isNamedGEN(self, num):
        """Check if a given GEN number num is a named GEN.
        
        If so, it returns the string length. Otherwise it returns 0.
        """
        return libcsound.csoundIsNamedGEN(self.cs, num)
    
    def namedGEN(self, num, nameLen):
        """Get the GEN name from a GEN number, if this is a named GEN.
        
        The final parameter is the max len of the string.
        """
        s = create_string_buffer(nameLen)
        libcsound.csoundGetNamedGEN(self.cs, num, s, nameLen)
        return pstring(string_at(s, nameLen))
    
    #Function Table Display
    def setIsGraphable(self, isGraphable):
        """Tell Csound whether external graphic table display is supported.
        
        Return the previously set value (initially False).
        """
        ret = libcsound.csoundSetIsGraphable(self.cs, c_int(isGraphable))
        return (ret != 0)
    
    def setMakeGraphCallback(self, function):
        """Called by external software to set Csound's MakeGraph function."""
        self.makeGraphCbRef = MAKEGRAPHFUNC(function)
        libcsound.csoundSetMakeGraphCallback(self.cs, self.makeGraphCbRef)
        
    def setDrawGraphCallback(self, function):
        """Called by external software to set Csound's DrawGraph function."""
        self.drawGraphCbRef = DRAWGRAPHFUNC(function)
        libcsound.csoundSetDrawGraphCallback(self.cs, self.drawGraphCbRef)
    
    def setKillGraphCallback(self, function):
        """Called by external software to set Csound's KillGraph function."""
        self.killGraphCbRef = KILLGRAPHFUNC(function)
        libcsound.csoundSetKillGraphCallback(self.cs, self.killGraphCbRef)
                                                              
    def setExitGraphCallback(self, function):
        """Called by external software to set Csound's ExitGraph function."""
        self.exitGraphCbRef = EXITGRAPHFUNC(function)
        libcsound.csoundSetExitGraphCallback(self.cs, self.exitGraphCbRef)
    
    #Opcodes
    def namedGens(self):
        """Find the list of named gens."""
        lst = []
        ptr = libcsound.csoundGetNamedGens(self.cs)
        ptr = cast(ptr, POINTER(NamedGen))
        while (ptr):
            ng = ptr.contents
            lst.append((pstring(ng.name), int(ng.genum)))
            ptr = ng.next
        return lst
    
    def newOpcodeList(self):
        """Get an alphabetically sorted list of all opcodes.
        
        Should be called after externals are loaded by compile_().
        Return a pointer to the list of OpcodeListEntry structures and the
        number of opcodes, or a negative error code on  failure.
        Make sure to call disposeOpcodeList() when done with the list.
        """
        opcodes = None
        ptr = cast(POINTER(c_int)(), POINTER(OpcodeListEntry))
        n = libcsound.csoundNewOpcodeList(self.cs, byref(ptr))
        if n > 0:
            opcodes = cast(ptr, POINTER(OpcodeListEntry * n)).contents
        return opcodes, n
    
    def disposeOpcodeList(self, lst):
        """Release an opcode list."""
        ptr = cast(lst, POINTER(OpcodeListEntry))
        libcsound.csoundDisposeOpcodeList(self.cs, ptr)

    def appendOpcode(self, opname, dsblksiz, flags, thread, outypes, intypes, iopfunc, kopfunc, aopfunc):
        """Appends an opcode implemented by external software.
        
        This opcode is added to Csound's internal opcode list.
        The opcode list is extended by one slot, and the parameters are copied
        into the new slot.
        Return zero on success.
        """
        return libcsound.csoundAppendOpcode(self.cs, cstring(opname), dsblksiz, flags, thread,\
                                            cstring(outypes), cstring(intypes),\
                                            OPCODEFUNC(iopfunc),\
                                            OPCODEFUNC(kopfunc),
                                            OPCODEFUNC(aopfunc))
    
    #Threading and Concurrency
    def setYieldCallback(self, function):
        """
        Called by external software to set a yield function.
        
        This callback is usef for checking system events, yielding cpu time
        for coopertative multitasking, etc.
        This function is optional. It is often used as a way to 'turn off'
        Csound, allowing it to exit gracefully. In addition, some operations
        like utility analysis routines are not reentrant and you should use
        this function to do any kind of updating during the operation.
        Returns an 'OK to continue' boolean.
        """
        self.yieldCbRef = YIELDFUNC(function)
        libcsound.csoundSetYieldCallback(self.cs, self.yieldCbRef)

    def createThread(self, function, userdata):
        """Create and start a new thread of execution.
        
        Return an opaque pointer that represents the thread on success,
        or None for failure.
        The userdata pointer is passed to the thread routine.
        """
        ret = libcsound.csoundCreateThread(THREADFUNC(function), py_object(userdata))
        if (ret):
            return ret
        return None
    
    def currentThreadId(self):
        """Return the ID of the currently executing thread, or None for failure.
        
        NOTE: The return value can be used as a pointer
        to a thread object, but it should not be compared
        as a pointer. The pointed to values should be compared,
        and the user must free the pointer after use.
        """
        ret = libcsound.csoundGetCurrentThreadId()
        if (ret):
            return ret
        return None
    
    def joinThread(self, thread):
        """Wait until the indicated thread's routine has finished.
        
        Return the value returned by the thread routine.
        """
        return libcsound.csoundJoinThread(thread)
    
    def createThreadLock(self):
        """Create and return a monitor object, or None if not successful.
        
        The object is initially in signaled (notified) state.
        """
        ret = libcsound.csoundCreateThreadLock()
        if (ret):
            return ret
        return None
    
    def waitThreadLock(self, lock, milliseconds):
        """Wait on the indicated monitor object for the indicated period.
        
        The function returns either when the monitor object is notified,
        or when the period has elapsed, whichever is sooner; in the first case,
        zero is returned.
        If 'milliseconds' is zero and the object is not notified, the function
        will return immediately with a non-zero status.
        """
        return libcsound.csoundWaitThreadLock(lock, c_uint(milliseconds))
    
    def waitThreadLockNoTimeout(self, lock):
        """Wait on the indicated monitor object until it is notified.
        
        This function is similar to waitThreadLock() with an infinite
        wait time, but may be more efficient.
        """
        libcsound.csoundWaitThreadLockNoTimeout(lock)
    
    def notifyThreadLock(self, lock):
        """Notify the indicated monitor object."""
        libcsound.csoundNotifyThreadLock(lock)
        
    def destroyThreadLock(self, lock):
        """Destroy the indicated monitor object."""
        libcsound.csoundDestroyThreadLock(lock)
        
    def createMutex(self, isRecursive):
        """Create and return a mutex object, or None if not successful.
        
        Mutexes can be faster than the more general purpose monitor objects
        returned by createThreadLock() on some platforms, and can also
        be recursive, but the result of unlocking a mutex that is owned by
        another thread or is not locked is undefined.
        If 'isRecursive' True, the mutex can be re-locked multiple
        times by the same thread, requiring an equal number of unlock calls;
        otherwise, attempting to re-lock the mutex results in undefined
        behavior.
        Note: the handles returned by createThreadLock() and
        createMutex() are not compatible.
        """
        ret = libcsound.csoundCreateMutex(c_int(isRecursive))
        if ret:
            return ret
        return None
    
    def lockMutex(self, mutex):
        """Acquire the indicated mutex object.
        
        If it is already in use by another thread, the function waits until
        the mutex is released by the other thread.
        """
        libcsound.csoundLockMutex(mutex)
    
    def lockMutexNoWait(self, mutex):
        """Acquire the indicated mutex object.
        
        Returns zero, unless it is already in use by another thread, in which
        case a non-zero value is returned immediately, rather than waiting
        until the mutex becomes available.
        Note: this function may be unimplemented on Windows.
        """
        return libcsound.csoundLockMutexNoWait(mutex)
    
    def unlockMutex(self, mutex):
        """Release the indicated mutex object.
        
        The mutex should be owned by the current thread, otherwise the
        operation of this function is undefined. A recursive mutex needs
        to be unlocked as many times as it was locked previously.
        """
        libcsound.csoundUnlockMutex(mutex)
    
    def destroyMutex(self, mutex):
        """Destroy the indicated mutex object.
        
        Destroying a mutex that is currently owned by a thread results
        in undefined behavior.
        """
        libcsound.csoundDestroyMutex(mutex)
    
    def createBarrier(self, max_):
        """Create a Thread Barrier.
        
        Max value parameter should be equal to the number of child threads
        using the barrier plus one for the master thread.
        """
        ret = libcsound.csoundCreateBarrier(c_uint(max_))
        if (ret):
            return ret
        return None
    
    def destroyBarrier(self, barrier):
        """Destroy a Thread Barrier."""
        return libcsound.csoundDestroyBarrier(barrier)
    
    def waitBarrier(self, barrier):
        """Wait on the thread barrier."""
        return libcsound.csoundWaitBarrier(barrier)

    #def createCondVar(self):
    #def condWait(self, condVar, mutex):
    #def condSignal(self, condVar):
    
    def sleep(self, milliseconds):
        """Wait for at least the specified number of milliseconds.
        
        It yields the CPU to other threads.
        """
        libcsound.csoundSleep(c_uint(milliseconds))
    
    if (hasSpinLock):
        def spinLock(self, spinlock):
            """Lock the specified spinlock.
            
            If the spinlock is not locked, lock it and return;
            if is is locked, wait until it is unlocked, then lock it and return.
            Uses atomic compare and swap operations that are safe across processors
            and safe for out of order operations,
            and which are more efficient than operating system locks.
            Use spinlocks to protect access to shared data, especially in functions
            that do little more than read or write such data, for example:
            
                lock = ctypes.c_int(0)
                def write(cs, frames, signal):
                    cs.spinLock(ctypes.byref(lock))
                    for frame in range(frames) :
                        global_buffer[frame] += signal[frame];
                    cs.spinUnlock(ctypes.byref(lock))
            """
            libcsound.csoundSpinLock(spinlock)
        
        def spinUnlock(self, spinlock):
            """Unlock the specified spinlock ; (see spinlock())."""
            libcsound.csoundSpinUnLock(spinlock)
    else:
        def spinLock(self, spinlock):
            pass
        
        def spinUnlock(self, spinlock):
            pass
    
    #Miscellaneous Functions
    def runCommand(self, args, noWait):
        """Runs an external command with the arguments specified in list 'args'.
        
        args[0] is the name of the program to execute (if not a full path
        file name, it is searched in the directories defined by the PATH
        environment variable).
        If 'noWait' is False, the function waits until the external program
        finishes, otherwise it returns immediately. In the first case, a
        non-negative return value is the exit status of the command (0 to
        255), otherwise it is the PID of the newly created process.
        On error, a negative value is returned.
        """
        n = len(args)
        argv = (POINTER(c_char_p) * (n+1))()
        for i in range(n):
            v = cstring(args[i])
            argv[i] = cast(pointer(create_string_buffer(v)), POINTER(c_char_p))
        argv[n] = None
        return libcsound.csoundRunCommand(cast(argv, POINTER(c_char_p)), c_int(noWait))
    
    def initTimerStruct(self, timerStruct):
        """Initialize a timer structure."""
        libcsound.csoundInitTimerStruct(byref(timerStruct))
    
    def realTime(self, timerStruct):
        """Return the elapsed real time (in seconds).
        
        The time is measured since the specified timer structure was initialised.
        """
        return libcsound.csoundGetRealTime(byref(timerStruct))
    
    def CPUTime(self, timerStruct):
        """Return the elapsed CPU time (in seconds).
        
        The time is measured since the specified timer structure was initialised.
        """
        return libcsound.csoundGetCPUTime(byref(timerStruct))
    
    def randomSeedFromTime(self):
        """Return a 32-bit unsigned integer to be used as seed from current time."""
        return libcsound.csoundGetRandomSeedFromTime()
    
    def setLanguage(self, lang_code):
        """Set language to 'lang_code'.
        
        'lang_code' can be for example CSLANGUAGE_ENGLISH_UK or
        CSLANGUAGE_FRENCH or many others, (see n_getstr.h for the list of
        languages). This affects all Csound instances running in the address
        space of the current process. The special language code
        CSLANGUAGE_DEFAULT can be used to disable translation of messages and
        free all memory allocated by a previous call to setLanguage().
        setLanguage() loads all files for the selected language
        from the directory specified by the CSSTRNGS environment
        variable.
        """
        libcsound.csoundSetLanguage(lang_code)
    
    def env(self, name, withCsoundInstance = True):
        """Get the value of environment variable 'name'.
        
        The searching order is: local environment of 'csound' (if
        'withCsoundInstance' is True), variables set with setGlobalEnv(),
        and system environment variables. If 'withCsoundInstance' is True,
        should be called after compile_().
        Return value is None if the variable is not set.
        """
        if withCsoundInstance:
            ret = libcsound.csoundGetEnv(self.cs, cstring(name))
        else:
            ret = libcsound.csoundGetEnv(None, cstring(name))
        if (ret):
            return pstring(ret)
        return None

    def setGlobalEnv(self, name, value):
        """Set the global value of environment variable 'name' to 'value'.
        
        The variable is deleted if 'value' is None. It is not safe to call this
        function while any Csound instances are active.
        Returns zero on success.
        """
        return libcsound.csoundSetGlobalEnv(cstring(name), cstring(value))
    
    def createGlobalVariable(self, name, nbytes):
        """Allocate nbytes bytes of memory.
        
        This memory can be accessed later by calling queryGlobalVariable()
        with the specified name; the space is cleared to zero.
        Returns CSOUND_SUCCESS on success, CSOUND_ERROR in case of invalid
        parameters (zero nbytes, invalid or already used name), or
        CSOUND_MEMORY if there is not enough memory.
        """
        return libcsound.csoundCreateGlobalVariable(self.cs, cstring(name), c_uint(nbytes))
    
    def queryGlobalVariable(self, name):
        """Get pointer to space allocated with the name 'name'.
        
        Return None if the specified name is not defined.
        """
        ret = libcsound.csoundQueryGlobalVariable(self.cs, cstring(name))
        if (ret):
            return ret
        return None
    
    def queryGlobalVariableNoCheck(self, name):
        """This function is the similar to queryGlobalVariable().
        
        Except the variable is assumed to exist and no error checking is done.
        Faster, but may crash or return an invalid pointer if 'name' is
        not defined.
        """
        return libcsound.csoundQueryGlobalVariableNoCheck(self.cs, cstring(name))
    
    def destroyGlobalVariable(self, name):
        """Free memory allocated for 'name' and remove 'name' from the database.
        
        Return value is CSOUND_SUCCESS on success, or CSOUND_ERROR if the name
        is not defined.
        """
        return libcsound.csoundDestroyGlobalVariable(self.cs, cstring(name))
    
    def runUtility(self, name, args):
        """Run utility with the specified name and command line arguments.
        
        Should be called after loading utility plugins.
        Use reset() to clean up after calling this function.
        Returns zero if the utility was run successfully.
        """
        argc, argv = csoundArgList(args)
        return libcsound.csoundRunUtility(self.cs, cstring(name), argc, argv)
    
    def listUtilities(self):
        """Return a list of registered utility names.
        
        The return value may be None in case of an error.
        """
        ptr = libcsound.csoundListUtilities(self.cs)
        if (ptr):
            i = 0
            lst = []
            while (ptr[i]):
                lst.append(pstring(ptr[i]))
                i += 1
            libcsound.csoundDeleteUtilityList(self.cs, ptr)
            return lst
        return None
    
    def utilityDescription(self, name):
        """Get utility description.
        
        Returns None if the utility was not found, or it has no description,
        or an error occured.
        """
        ptr = libcsound.csoundGetUtilityDescription(self.cs, cstring(name))
        if (ptr):
            return pstring(ptr)
        return None
    
    def rand31(self, seed):
        """Simple linear congruential random number generator.
        
            seed = seed * 742938285 % 2147483647
        the initial value of seed must be in the range 1 to 2147483646.
        Return the next number from the pseudo-random sequence,
        in the range 1 to 2147483646.
        """
        n = c_int(seed)
        return libcsound.csoundRand31(byref(n))
    
    def seedRandMT(self, initKey):
        """Initialize Mersenne Twister (MT19937) random number generator.
        
        initKey can be a single int, a list of int, or an ndarray of int. Those int
        values are converted to unsigned 32 bit values and used for seeding.
        Return a CsoundRandMTState stuct to be used by csoundRandMT().
        """
        state = CsoundRandMTState()
        if type(initKey) == int:
            if initKey < 0:
                initKey = -initKey
            libcsound.csoundSeedRandMT(byref(state), None, c_uint32(initKey))
        elif type(initKey) == list or type(initKey) == np.ndarray:
            n = len(initKey)
            lst = (c_uint32 * n)()
            for i in range(n):
                k = initKey[i]
                if k < 0 :
                    k = -k
                lst[i] = c_uint32(k)
            p = pointer(lst)
            p = cast(p, POINTER(c_uint32))
            libcsound.csoundSeedRandMT(byref(state), p, c_uint32(len(lst)))
        return state
    
    def randMT(self, state):
        """Return next random number from MT19937 generator.
        
        The PRNG must be initialized first by calling csoundSeedRandMT().
        """
        return libcsound.csoundRandMT(byref(state))
    
    def createCircularBuffer(self, numelem, elemsize):
        """Create circular buffer with numelem number of elements.
        
        The element's size is set from elemsize. It should be used like:
            rb = cs.createCircularBuffer(1024, cs.sizeOfMYFLT())
        """
        return libcsound.csoundCreateCircularBuffer(self.cs, numelem, elemsize)
    
    def readCircularBuffer(self, circularBuffer, out, items):
        """Read from circular buffer.
        
            circular_buffer - pointer to an existing circular buffer
            out - preallocated ndarray with at least items number of elements,
                  where buffer contents will be read into
            items - number of samples to be read
        Return the actual number of items read (0 <= n <= items).
        """
        if len(out) < items:
            return 0
        ptr = out.ctypes.data_as(c_void_p)
        return libcsound.csoundReadCircularBuffer(self.cs, circularBuffer, ptr, items)
    
    def peekCircularBuffer(self, circularBuffer, out, items):
        """Read from circular buffer without removing them from the buffer.
        
            circular_buffer - pointer to an existing circular buffer
            out - preallocated ndarray with at least items number of elements,
                  where buffer contents will be read into
            items - number of samples to be read
        Return the actual number of items read (0 <= n <= items).
        """
        if len(out) < items:
            return 0
        ptr = out.ctypes.data_as(c_void_p)
        return libcsound.csoundPeekCircularBuffer(self.cs, circularBuffer, ptr, items)
    
    def writeCircularBuffer(self, circularBuffer, in_, items):
        """Write to circular buffer.
        
            circular_buffer - pointer to an existing circular buffer
            in_ - ndarray with at least items number of elements to be written
                  into circular buffer
            items - number of samples to write
        Return the actual number of items written (0 <= n <= items).
        """
        if len(in_) < items:
            return 0
        ptr = in_.ctypes.data_as(c_void_p)
        return libcsound.csoundWriteCircularBuffer(self.cs, circularBuffer, ptr, items)
    
    def flushCircularBuffer(self, circularBuffer):
        """Empty circular buffer of any remaining data.
        
        This function should only be used if there is no reader actively
        getting data from the buffer.
            circular_buffer - pointer to an existing circular buffer
        """
        libcsound.csoundFlushCircularBuffer(self.cs, circularBuffer)
    
    def destroyCircularBuffer(self, circularBuffer):
        """Free circular buffer."""
        libcsound.csoundDestroyCircularBuffer(self.cs, circularBuffer)

    def openLibrary(self, libraryPath):
        """Platform-independent function to load a shared library."""
        ptr = POINTER(c_int)()
        library = cast(ptr, c_void_p)
        ret = libcsound.csoundOpenLibrary(byref(library), cstring(libraryPath))
        return ret, library
    
    def closeLibrary(self, library):
        """Platform-independent function to unload a shared library."""
        return libcsound.csoundCloseLibrary(library)
    
    def getLibrarySymbol(self, library, symbolName):
        """Platform-independent function to get a symbol address in a shared library."""
        return libcsound.csoundGetLibrarySymbol(library, cstring(symbolName))


if sys.platform.startswith('linux'):
    libcspt = CDLL("libcsnd6.so")
elif sys.platform.startswith('win'):
    libcspt = cdll.csnd6
elif sys.platform.startswith('darwin'):
    libcspt = CDLL("libcsnd6.6.0.dylib")
else:
    sys.exit("Don't know your system! Exiting...")

libcspt.NewCsoundPT.restype = c_void_p
libcspt.NewCsoundPT.argtypes = [c_void_p]
libcspt.DeleteCsoundPT.argtypes = [c_void_p]
libcspt.CsoundPTisRunning.argtypes = [c_void_p]
PROCESSFUNC = CFUNCTYPE(None, c_void_p)
libcspt.CsoundPTgetProcessCB.restype = c_void_p
libcspt.CsoundPTgetProcessCB.argtypes = [c_void_p]
libcspt.CsoundPTsetProcessCB.argtypes = [c_void_p, PROCESSFUNC, c_void_p]
libcspt.CsoundPTgetCsound.restype = c_void_p
libcspt.CsoundPTgetCsound.argtypes = [c_void_p]
libcspt.CsoundPTgetStatus.argtypes = [c_void_p]
libcspt.CsoundPTplay.argtypes = [c_void_p]
libcspt.CsoundPTpause.argtypes = [c_void_p]
libcspt.CsoundPTtogglePause.argtypes = [c_void_p]
libcspt.CsoundPTstop.argtypes = [c_void_p]
libcspt.CsoundPTrecord.argtypes = [c_void_p, c_char_p, c_int, c_int]
libcspt.CsoundPTstopRecord.argtypes = [c_void_p]
libcspt.CsoundPTscoreEvent.argtypes = [c_void_p, c_int, c_char, c_int, POINTER(MYFLT)]
libcspt.CsoundPTinputMessage.argtypes = [c_void_p, c_char_p]
libcspt.CsoundPTsetScoreOffsetSeconds.argtypes = [c_void_p, c_double]
libcspt.CsoundPTjoin.argtypes = [c_void_p]
libcspt.CsoundPTflushMessageQueue.argtypes = [c_void_p]


class CsoundPerformanceThread:
    """Perform a score in a separate thread until the end of score is reached.
    
    The playback (which is paused by default) is stopped by calling
    stop(), or if an error occurs.
    The constructor takes a Csound instance pointer as argument; it assumes
    that csound.compile_() was called successfully before creating the
    performance thread. Once the playback is stopped for one of the above
    mentioned reasons, the performance thread calls csound.cleanup() and
    returns.
    """
    def __init__(self, csp):
        self.cpt = libcspt.NewCsoundPT(csp)
    
    def __del__(self):
        libcspt.DeleteCsoundPT(self.cpt)
    
    def isRunning(self):
        """Return True if the performance thread is running, False otherwise."""
        return libcspt.CsoundPTisRunning(self.cpt) != 0
    
    def processCB(self):
        """Return the process callback."""
        return PROCESSFUNC(libcspt.CsoundPTgetProcessCB(self.cpt))
    
    def setProcessCB(self, function, data):
        """Set the process callback."""
        libcspt.CsoundPTsetProcessCB(self.cpt, PROCESSFUNC(function), byref(data))
    
    def csound(self):
        """Return the Csound instance pointer."""
        return libcspt.CsoundPTgetCsound(self.cpt)
    
    def status(self):
        """Return the current status.
        
        Zero if still playing, positive if the end of score was reached or
        performance was stopped, and negative if an error occured.
        """
        return libcspt.CsoundPTgetStatus(self.cpt)
    
    def play(self):
        """Continue performance if it was paused."""
        libcspt.CsoundPTplay(self.cpt)
    
    def pause(self):
        """Pause performance (can be continued by calling Play())."""
        libcspt.CsoundPTpause(self.cpt)
    
    def togglePause(self):
        """Pause or continue performance, depending on current state."""
        libcspt.CsoundPTtogglePause(self.cpt)
    
    def stop(self):
        """Stop performance (cannot be continued)."""
        libcspt.CsoundPTstop(self.cpt)
    
    def record(self, filename, samplebits, numbufs):
        """Start recording the output from Csound.
        
        The sample rate and number of channels are taken directly from the
        running Csound instance.
        """
        libcspt.CsoundPTrecord(self.cpt, cstring(filename), samplebits, numbufs)
    
    def stopRecord(self):
        """Stop recording and closes audio file."""
        libcspt.CsoundPTstopRecord(self.cpt)
    
    def scoreEvent(self, absp2mode, opcod, pFields):
        """Send a score event.
        
        The event has type 'opcod' (e.g. 'i' for a note event).
        'pFields' is tuple, a list, or an ndarray of MYFLTs with all the pfields
        for this event, starting with the p1 value specified in pFields[0].
        If 'absp2mode' is non-zero, the start time of the event is measured
        from the beginning of performance, instead of the default of relative
        to the current time.
        """
        p = np.array(pFields).astype(MYFLT)
        ptr = p.ctypes.data_as(POINTER(MYFLT))
        numFields = p.size
        libcspt.CsoundPTscoreEvent(self.cpt, c_int(absp2mode), cchar(opcod), numFields, ptr)
    
    def inputMessage(self, s):
        """Send a score event as a string, similarly to line events (-L)."""
        libcspt.CsoundPTinputMessage(self.cpt, cstring(s))
    
    def setScoreOffsetSeconds(self, timeVal):
        """Set the playback time pointer to the specified value (in seconds)."""
        libcspt.CsoundPTsetScoreOffsetSeconds(self.cpt, c_double(timeVal))
    
    def join(self):
        """Wait until the performance is finished or fails.
        
        Return a positive value if the end of score was reached or Stop() was
        called, and a negative value if an error occured. Also releases any
        resources associated with the performance thread object.
        """
        return libcspt.CsoundPTjoin(self.cpt)
    
    def flushMessageQueue(self):
        """Wait until all pending messages are actually received.
        
        (pause, send score event, etc.)
        """
        libcspt.CsoundPTflushMessageQueue(self.cpt)

