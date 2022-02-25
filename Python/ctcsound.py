#
#   ctcsound.py:
#   
#   Copyright (C) 2016 Francois Pinot
#   
#   This file is part of Csound.
#   
#   This code is free software; you can redistribute it
#   and/or modify it under the terms of the GNU Lesser General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
#   
#   Csound is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#   
#   You should have received a copy of the GNU Lesser General Public
#   License along with Csound; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
#   02110-1301 USA
#

import ctypes as ct
import ctypes.util
import numpy as np
import sys

# This is a workaround to yield the PEP 3118 problem which appeared with
# numpy 1.15.0
if np.__version__ < '1.15':
    arrFromPointer = lambda p : np.ctypeslib.as_array(p)
elif np.__version__ < '1.16':
    sys.exit("ctcsound won't work with numpy 1.15.x. Please revert numpy" +
        " to an older version or update numpy to a version >= 1.16")
else:
    arrFromPointer = lambda p : p.contents

if sys.platform.startswith('linux'):
    libcsound = ct.CDLL("libcsound64.so")
elif sys.platform.startswith('win'):
    if sys.version_info.major <=3 and sys.version_info.minor < 8:
        libcsound = ct.cdll.csound64
    else:
        libcsound = ct.CDLL(ctypes.util.find_library("csound64"))
elif sys.platform.startswith('darwin'):
    libcsound = ct.CDLL(ctypes.util.find_library("CsoundLib64"))
else:
    sys.exit("Don't know your system! Exiting...")

MYFLT = ct.c_double

class CsoundParams(ct.Structure):
    _fields_ = [("debug_mode", ct.c_int),        # debug mode, 0 or 1
                ("buffer_frames", ct.c_int),     # number of frames in in/out buffers
                ("hardware_buffer_frames", ct.c_int), # ibid. hardware
                ("displays", ct.c_int),          # graph displays, 0 or 1
                ("ascii_graphs", ct.c_int),      # use ASCII graphs, 0 or 1
                ("postscript_graphs", ct.c_int), # use postscript graphs, 0 or 1
                ("message_level", ct.c_int),     # message printout control
                ("tempo", ct.c_int),             # tempo ("sets Beatmode) 
                ("ring_bell", ct.c_int),         # bell, 0 or 1
                ("use_cscore", ct.c_int),        # use cscore for processing
                ("terminate_on_midi", ct.c_int), # terminate performance at the end
                                              #   of midifile, 0 or 1
                ("heartbeat", ct.c_int),         # print heart beat, 0 or 1
                ("defer_gen01_load", ct.c_int),  # defer GEN01 load, 0 or 1
                ("midi_key", ct.c_int),          # pfield to map midi key no
                ("midi_key_cps", ct.c_int),      # pfield to map midi key no as cps
                ("midi_key_oct", ct.c_int),      # pfield to map midi key no as oct
                ("midi_key_pch", ct.c_int),      # pfield to map midi key no as pch
                ("midi_velocity", ct.c_int),     # pfield to map midi velocity
                ("midi_velocity_amp", ct.c_int), # pfield to map midi velocity as amplitude
                ("no_default_paths", ct.c_int),  # disable relative paths from files, 0 or 1
                ("number_of_threads", ct.c_int), # number of threads for multicore performance
                ("syntax_check_only", ct.c_int), # do not compile, only check syntax
                ("csd_line_counts", ct.c_int),   # csd line error reporting
                ("compute_weights", ct.c_int),   # deprecated, kept for backwards comp. 
                ("realtime_mode", ct.c_int),     # use realtime priority mode, 0 or 1
                ("sample_accurate", ct.c_int),   # use sample-level score event accuracy
                ("sample_rate_override", MYFLT),  # overriding sample rate
                ("control_rate_override", MYFLT), # overriding control rate
                ("nchnls_override", ct.c_int),   # overriding number of out channels
                ("nchnls_i_override", ct.c_int), # overriding number of in channels
                ("e0dbfs_override", MYFLT),   # overriding 0dbfs
                ("daemon", ct.c_int),            # daemon mode
                ("ksmps_override", ct.c_int),    # ksmps override
                ("FFT_library", ct.c_int)]       # fft_lib

string128 = ct.c_char * 128

class CsoundAudioDevice(ct.Structure):
    _fields_ = [("device_name", string128),
                ("device_id", string128),
                ("rt_module", string128),
                ("max_nchnls", ct.c_int),
                ("isOutput", ct.c_int)]

class CsoundMidiDevice(ct.Structure):
    _fields_ = [("device_name", string128),
                ("interface_name", string128),
                ("device_id", string128),
                ("midi_module", string128),
                ("isOutput", ct.c_int)]

class CsoundRtAudioParams(ct.Structure):
    _fields_ = [("devName", ct.c_char_p),   # device name (NULL/empty: default)
                ("devNum", ct.c_int),       # device number (0-1023), 1024: default
                ("bufSamp_SW", ct.c_uint),  # buffer fragment size (-b) in sample frames
                ("bufSamp_HW", ct.c_int),   # total buffer size (-B) in sample frames
                ("nChannels", ct.c_int),    # number of channels
                ("sampleFormat", ct.c_int), # sample format (AE_SHORT etc.)
                ("sampleRate", ct.c_float)] # sample rate in Hz

class RtClock(ct.Structure):
    _fields_ = [("starttime_real", ct.c_int64),
                ("starttime_CPU", ct.c_int64)]

class OpcodeListEntry(ct.Structure):
    _fields_ = [("opname", ct.c_char_p),
                ("outypes", ct.c_char_p),
                ("intypes", ct.c_char_p),
                ("flags", ct.c_int)]

class CsoundRandMTState(ct.Structure):
    _fields_ = [("mti", ct.c_int),
                ("mt", ct.c_uint32*624)]

# PVSDATEXT is a variation on PVSDAT used in the pvs bus interface
class PvsdatExt(ct.Structure):
    _fields_ = [("N", ct.c_int32),
                ("sliding", ct.c_int),      # Flag to indicate sliding case
                ("NB", ct.c_int32),
                ("overlap", ct.c_int32),
                ("winsize", ct.c_int32),
                ("wintype", ct.c_int),
                ("format", ct.c_int32),
                ("framecount", ct.c_uint32),
                ("frame", ct.POINTER(ct.c_float))]

# This structure holds the parameter hints for control channels
class ControlChannelHints(ct.Structure):
    _fields_ = [("behav", ct.c_int),
                ("dflt", MYFLT),
                ("min", MYFLT),
                ("max", MYFLT),
                ("x", ct.c_int),
                ("y", ct.c_int),
                ("width", ct.c_int),
                ("height", ct.c_int),
                # This member must be set explicitly to None if not used
                ("attributes", ct.c_char_p)]

class ControlChannelInfo(ct.Structure):
    _fields_ = [("name", ct.c_char_p),
                ("type", ct.c_int),
                ("hints", ControlChannelHints)]

CAPSIZE  = 60

class Windat(ct.Structure):
    _fields_ = [("windid", ct.POINTER(ct.c_uint)),    # set by makeGraph()
                ("fdata", ct.POINTER(MYFLT)),      # data passed to drawGraph()
                ("npts", ct.c_int32),              # size of above array
                ("caption", ct.c_char * CAPSIZE),  # caption string for graph
                ("waitflg", ct.c_int16 ),          # set =1 to wait for ms after Draw
                ("polarity", ct.c_int16),          # controls positioning of X axis
                ("max", MYFLT),                 # workspace .. extrema this frame
                ("min", MYFLT),
                ("absmax", MYFLT),              # workspace .. largest of above
                ("oabsmax", MYFLT),             # Y axis scaling factor
                ("danflag", ct.c_int),             # set to 1 for extra Yaxis mid span
                ("absflag", ct.c_int)]             # set to 1 to skip abs check

# Symbols for Windat.polarity field
NOPOL = 0
NEGPOL = 1
POSPOL = 2
BIPOL = 3

class NamedGen(ct.Structure):
    pass

NamedGen._fields_ = [
    ("name", ct.c_char_p),
    ("genum", ct.c_int),
    ("next", ct.POINTER(NamedGen))]


libcsound.csoundSetOpcodedir.argtypes = [ct.c_char_p]
libcsound.csoundCreate.restype = ct.c_void_p
libcsound.csoundCreate.argtypes = [ct.py_object]
libcsound.csoundLoadPlugins.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundDestroy.argtypes = [ct.c_void_p]

libcsound.csoundParseOrc.restype = ct.c_void_p
libcsound.csoundParseOrc.argtypes = [ct.c_void_p, ct.c_char_p]

libcsound.csoundCompileTree.argtypes = [ct.c_void_p, ct.c_void_p]
libcsound.csoundCompileTreeAsync.argtypes = [ct.c_void_p, ct.c_void_p]
libcsound.csoundDeleteTree.argtypes = [ct.c_void_p, ct.c_void_p]
libcsound.csoundCompileOrc.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundCompileOrcAsync.argtypes = [ct.c_void_p, ct.c_char_p]

libcsound.csoundEvalCode.restype = MYFLT
libcsound.csoundEvalCode.argtypes = [ct.c_void_p, ct.c_char_p]

libcsound.csoundCompileArgs.argtypes = [ct.c_void_p, ct.c_int, ct.POINTER(ct.c_char_p)]
libcsound.csoundStart.argtypes = [ct.c_void_p]
libcsound.csoundCompile.argtypes = [ct.c_void_p, ct.c_int, ct.POINTER(ct.c_char_p)]
libcsound.csoundCompileCsd.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundCompileCsdText.argtypes = [ct.c_void_p, ct.c_char_p]

libcsound.csoundPerform.argtypes = [ct.c_void_p]
libcsound.csoundPerformKsmps.argtypes = [ct.c_void_p]
libcsound.csoundPerformBuffer.argtypes = [ct.c_void_p]
libcsound.csoundStop.argtypes = [ct.c_void_p]
libcsound.csoundCleanup.argtypes = [ct.c_void_p]
libcsound.csoundReset.argtypes = [ct.c_void_p]

libcsound.csoundUDPServerStart.argtypes = [ct.c_void_p, ct.c_uint]
libcsound.csoundUDPServerStatus.argtypes = [ct.c_void_p]
libcsound.csoundUDPServerClose.argtypes = [ct.c_void_p]
libcsound.csoundUDPConsole.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_uint, ct.c_uint]
libcsound.csoundStopUDPConsole.argtypes = [ct.c_void_p]

libcsound.csoundGetSr.restype = MYFLT
libcsound.csoundGetSr.argtypes = [ct.c_void_p]
libcsound.csoundGetKr.restype = MYFLT
libcsound.csoundGetKr.argtypes = [ct.c_void_p]
libcsound.csoundGetKsmps.restype = ct.c_uint32
libcsound.csoundGetKsmps.argtypes = [ct.c_void_p]
libcsound.csoundGetNchnls.restype = ct.c_uint32
libcsound.csoundGetNchnls.argtypes = [ct.c_void_p]
libcsound.csoundGetNchnlsInput.restype = ct.c_uint32
libcsound.csoundGetNchnlsInput.argtypes = [ct.c_void_p]
libcsound.csoundGet0dBFS.restype = MYFLT
libcsound.csoundGet0dBFS.argtypes = [ct.c_void_p]
libcsound.csoundGetA4.restype = MYFLT
libcsound.csoundGetA4.argtypes = [ct.c_void_p]
libcsound.csoundGetCurrentTimeSamples.restype = ct.c_int64
libcsound.csoundGetCurrentTimeSamples.argtypes = [ct.c_void_p]
libcsound.csoundGetHostData.restype = ct.py_object
libcsound.csoundGetHostData.argtypes = [ct.c_void_p]
libcsound.csoundSetHostData.argtypes = [ct.c_void_p, ct.py_object]
libcsound.csoundSetOption.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundSetParams.argtypes = [ct.c_void_p, ct.POINTER(CsoundParams)]
libcsound.csoundGetParams.argtypes = [ct.c_void_p, ct.POINTER(CsoundParams)]
libcsound.csoundGetDebug.argtypes = [ct.c_void_p]
libcsound.csoundSetDebug.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundSystemSr.restype = MYFLT
libcsound.csoundSystemSr.argtypes = [ct.c_void_p, MYFLT]

libcsound.csoundGetOutputName.restype = ct.c_char_p
libcsound.csoundGetOutputName.argtypes = [ct.c_void_p]
libcsound.csoundGetInputName.restype = ct.c_char_p
libcsound.csoundGetInputName.argtypes = [ct.c_void_p]
libcsound.csoundSetOutput.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundGetOutputFormat.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundSetInput.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundSetMIDIInput.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundSetMIDIFileInput.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundSetMIDIOutput.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundSetMIDIFileOutput.argtypes = [ct.c_void_p, ct.c_char_p]
FILEOPENFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int)
libcsound.csoundSetFileOpenCallback.argtypes = [ct.c_void_p, FILEOPENFUNC]

libcsound.csoundSetRTAudioModule.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundGetModule.argtypes = [ct.c_void_p, ct.c_int, ct.POINTER(ct.c_char_p), ct.POINTER(ct.c_char_p)]
libcsound.csoundGetInputBufferSize.restype = ct.c_long
libcsound.csoundGetInputBufferSize.argtypes = [ct.c_void_p]
libcsound.csoundGetOutputBufferSize.restype = ct.c_long
libcsound.csoundGetOutputBufferSize.argtypes = [ct.c_void_p]
libcsound.csoundGetInputBuffer.restype = ct.POINTER(MYFLT)
libcsound.csoundGetInputBuffer.argtypes = [ct.c_void_p]
libcsound.csoundGetOutputBuffer.restype = ct.POINTER(MYFLT)
libcsound.csoundGetOutputBuffer.argtypes = [ct.c_void_p]
libcsound.csoundGetSpin.restype = ct.POINTER(MYFLT)
libcsound.csoundGetSpin.argtypes = [ct.c_void_p]
libcsound.csoundClearSpin.argtypes = [ct.c_void_p]
libcsound.csoundAddSpinSample.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, MYFLT]
libcsound.csoundSetSpinSample.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, MYFLT]
libcsound.csoundGetSpout.restype = ct.POINTER(MYFLT)
libcsound.csoundGetSpout.argtypes = [ct.c_void_p]
libcsound.csoundGetSpoutSample.restype = MYFLT
libcsound.csoundGetSpoutSample.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]
libcsound.csoundGetRtRecordUserData.restype = ct.POINTER(ct.c_void_p)
libcsound.csoundGetRtRecordUserData.argtypes = [ct.c_void_p]
libcsound.csoundGetRtPlayUserData.restype = ct.POINTER(ct.c_void_p)
libcsound.csoundGetRtPlayUserData.argtypes = [ct.c_void_p]
libcsound.csoundSetHostImplementedAudioIO.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]
libcsound.csoundGetAudioDevList.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_int]
PLAYOPENFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(CsoundRtAudioParams))
libcsound.csoundSetPlayopenCallback.argtypes = [ct.c_void_p, PLAYOPENFUNC]
RTPLAYFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.POINTER(MYFLT), ct.c_int)
libcsound.csoundSetRtplayCallback.argtypes = [ct.c_void_p, RTPLAYFUNC]
RECORDOPENFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(CsoundRtAudioParams))
libcsound.csoundSetRecopenCallback.argtypes = [ct.c_void_p, RECORDOPENFUNC]
RTRECORDFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(MYFLT), ct.c_int)
libcsound.csoundSetRtrecordCallback.argtypes = [ct.c_void_p, RTRECORDFUNC]
RTCLOSEFUNC = ct.CFUNCTYPE(None, ct.c_void_p)
libcsound.csoundSetRtcloseCallback.argtypes = [ct.c_void_p, RTCLOSEFUNC]
AUDIODEVLISTFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(CsoundAudioDevice), ct.c_int)
libcsound.csoundSetAudioDeviceListCallback.argtypes = [ct.c_void_p, AUDIODEVLISTFUNC]

libcsound.csoundSetMIDIModule.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundSetHostImplementedMIDIIO.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundGetMIDIDevList.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_int]
MIDIINOPENFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(ct.c_void_p), ct.c_char_p)
libcsound.csoundSetExternalMidiInOpenCallback.argtypes = [ct.c_void_p, MIDIINOPENFUNC]
MIDIREADFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_void_p, ct.c_char_p, ct.c_int)
libcsound.csoundSetExternalMidiReadCallback.argtypes = [ct.c_void_p, MIDIREADFUNC]
MIDIINCLOSEFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_void_p)
libcsound.csoundSetExternalMidiInCloseCallback.argtypes = [ct.c_void_p, MIDIINCLOSEFUNC]
MIDIOUTOPENFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(ct.c_void_p), ct.c_char_p)
libcsound.csoundSetExternalMidiOutOpenCallback.argtypes = [ct.c_void_p, MIDIOUTOPENFUNC]
MIDIWRITEFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_void_p, ct.c_char_p, ct.c_int)
libcsound.csoundSetExternalMidiWriteCallback.argtypes = [ct.c_void_p, MIDIWRITEFUNC]
MIDIOUTCLOSEFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_void_p)
libcsound.csoundSetExternalMidiOutCloseCallback.argtypes = [ct.c_void_p, MIDIOUTCLOSEFUNC]
MIDIERRORFUNC = ct.CFUNCTYPE(ct.c_char_p, ct.c_int)
libcsound.csoundSetExternalMidiErrorStringCallback.argtypes = [ct.c_void_p, MIDIERRORFUNC]
MIDIDEVLISTFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(CsoundMidiDevice), ct.c_int)
libcsound.csoundSetMIDIDeviceListCallback.argtypes = [ct.c_void_p, MIDIDEVLISTFUNC]

libcsound.csoundReadScore.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundReadScoreAsync.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundGetScoreTime.restype = ct.c_double
libcsound.csoundGetScoreTime.argtypes = [ct.c_void_p]
libcsound.csoundIsScorePending.argtypes = [ct.c_void_p]
libcsound.csoundSetScorePending.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundGetScoreOffsetSeconds.restype = MYFLT
libcsound.csoundGetScoreOffsetSeconds.argtypes = [ct.c_void_p]
libcsound.csoundSetScoreOffsetSeconds.argtypes = [ct.c_void_p, MYFLT]
libcsound.csoundRewindScore.argtypes = [ct.c_void_p]
CSCOREFUNC = ct.CFUNCTYPE(None, ct.c_void_p)
libcsound.csoundSetCscoreCallback.argtypes = [ct.c_void_p, CSCOREFUNC]

libcsound.csoundMessage.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundMessageS.argtypes = [ct.c_void_p, ct.c_int, ct.c_char_p, ct.c_char_p]
DEFMSGFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.c_int, ct.c_char_p, ct.c_void_p)
libcsound.csoundSetDefaultMessageCallback.argtypes = [DEFMSGFUNC]
libcsound.csoundGetMessageLevel.argtypes = [ct.c_void_p]
libcsound.csoundSetMessageLevel.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundCreateMessageBuffer.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundGetFirstMessage.restype = ct.c_char_p
libcsound.csoundGetFirstMessage.argtypes = [ct.c_void_p]
libcsound.csoundGetFirstMessageAttr.argtypes = [ct.c_void_p]
libcsound.csoundPopFirstMessage.argtypes = [ct.c_void_p]
libcsound.csoundGetMessageCnt.argtypes = [ct.c_void_p]
libcsound.csoundDestroyMessageBuffer.argtypes = [ct.c_void_p]

libcsound.csoundGetChannelPtr.argtypes = [ct.c_void_p, ct.POINTER(ct.POINTER(MYFLT)), ct.c_char_p, ct.c_int]
libcsound.csoundListChannels.argtypes = [ct.c_void_p, ct.POINTER(ct.POINTER(ControlChannelInfo))]
libcsound.csoundDeleteChannelList.argtypes = [ct.c_void_p, ct.POINTER(ControlChannelInfo)]
libcsound.csoundSetControlChannelHints.argtypes = [ct.c_void_p, ct.c_char_p, ControlChannelHints]
libcsound.csoundGetControlChannelHints.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(ControlChannelHints)]
libcsound.csoundGetChannelLock.restype = ct.POINTER(ct.c_int)
libcsound.csoundGetChannelLock.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundGetControlChannel.restype = MYFLT
libcsound.csoundGetControlChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(ct.c_int)]
libcsound.csoundSetControlChannel.argtypes = [ct.c_void_p, ct.c_char_p, MYFLT]
libcsound.csoundGetAudioChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(ct.c_int)]
libcsound.csoundSetAudioChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(ct.c_int)]
libcsound.csoundGetStringChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundSetStringChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundGetChannelDatasize.argtypes = [ct.c_void_p, ct.c_char_p]
CHANNELFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.c_char_p, ct.c_void_p, ct.c_void_p)
libcsound.csoundSetInputChannelCallback.argtypes = [ct.c_void_p, CHANNELFUNC]
libcsound.csoundSetOutputChannelCallback.argtypes = [ct.c_void_p, CHANNELFUNC]
libcsound.csoundSetPvsChannel.argtypes = [ct.c_void_p, ct.POINTER(PvsdatExt), ct.c_char_p]
libcsound.csoundGetPvsChannel.argtypes = [ct.c_void_p, ct.POINTER(PvsdatExt), ct.c_char_p]
libcsound.csoundScoreEvent.argtypes = [ct.c_void_p, ct.c_char, ct.POINTER(MYFLT), ct.c_long]
libcsound.csoundScoreEventAsync.argtypes = [ct.c_void_p, ct.c_char, ct.POINTER(MYFLT), ct.c_long]
libcsound.csoundScoreEventAbsolute.argtypes = [ct.c_void_p, ct.c_char, ct.POINTER(MYFLT), ct.c_long, ct.c_double]
libcsound.csoundScoreEventAbsoluteAsync.argtypes = [ct.c_void_p, ct.c_char, ct.POINTER(MYFLT), ct.c_long, ct.c_double]
libcsound.csoundInputMessage.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundInputMessageAsync.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundKillInstance.argtypes = [ct.c_void_p, MYFLT, ct.c_char_p, ct.c_int, ct.c_int]
SENSEFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.py_object)
libcsound.csoundRegisterSenseEventCallback.argtypes = [ct.c_void_p, SENSEFUNC, ct.py_object]
libcsound.csoundKeyPress.argtypes = [ct.c_void_p, ct.c_char]
KEYBOARDFUNC = ct.CFUNCTYPE(ct.c_int, ct.py_object, ct.c_void_p, ct.c_uint)
libcsound.csoundRegisterKeyboardCallback.argtypes = [ct.c_void_p, KEYBOARDFUNC, ct.py_object, ct.c_uint]
libcsound.csoundRemoveKeyboardCallback.argtypes = [ct.c_void_p, KEYBOARDFUNC]

libcsound.csoundTableLength.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundTableGet.restype = MYFLT
libcsound.csoundTableGet.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]
libcsound.csoundTableSet.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, MYFLT]
libcsound.csoundTableCopyOut.argtypes = [ct.c_void_p, ct.c_int, ct.POINTER(MYFLT)]
libcsound.csoundTableCopyOutAsync.argtypes = [ct.c_void_p, ct.c_int, ct.POINTER(MYFLT)]
libcsound.csoundTableCopyIn.argtypes = [ct.c_void_p, ct.c_int, ct.POINTER(MYFLT)]
libcsound.csoundTableCopyInAsync.argtypes = [ct.c_void_p, ct.c_int, ct.POINTER(MYFLT)]
libcsound.csoundGetTable.argtypes = [ct.c_void_p, ct.POINTER(ct.POINTER(MYFLT)), ct.c_int]
libcsound.csoundGetTableArgs.argtypes = [ct.c_void_p, ct.POINTER(ct.POINTER(MYFLT)), ct.c_int]
libcsound.csoundIsNamedGEN.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundGetNamedGEN.argtypes = [ct.c_void_p, ct.c_int, ct.c_char_p, ct.c_int]

libcsound.csoundSetIsGraphable.argtypes = [ct.c_void_p, ct.c_int]
MAKEGRAPHFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.POINTER(Windat), ct.c_char_p)
libcsound.csoundSetMakeGraphCallback.argtypes = [ct.c_void_p, MAKEGRAPHFUNC]
DRAWGRAPHFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.POINTER(Windat))
libcsound.csoundSetDrawGraphCallback.argtypes = [ct.c_void_p, DRAWGRAPHFUNC]
KILLGRAPHFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.POINTER(Windat))
libcsound.csoundSetKillGraphCallback.argtypes = [ct.c_void_p, KILLGRAPHFUNC]
EXITGRAPHFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p)
libcsound.csoundSetExitGraphCallback.argtypes = [ct.c_void_p, EXITGRAPHFUNC]

libcsound.csoundGetNamedGens.restype = ct.c_void_p
libcsound.csoundGetNamedGens.argtypes = [ct.c_void_p]
libcsound.csoundNewOpcodeList.argtypes = [ct.c_void_p, ct.POINTER(ct.POINTER(OpcodeListEntry))]
libcsound.csoundDisposeOpcodeList.argtypes = [ct.c_void_p, ct.POINTER(OpcodeListEntry)]
OPCODEFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_void_p)
libcsound.csoundAppendOpcode.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, \
                                         ct.c_char_p, ct.c_char_p, OPCODEFUNC, OPCODEFUNC, OPCODEFUNC]

YIELDFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p)
libcsound.csoundSetYieldCallback.argtypes = [ct.c_void_p, YIELDFUNC]
THREADFUNC = ct.CFUNCTYPE(ct.POINTER(ct.c_uint), ct.py_object)
libcsound.csoundCreateThread.restype = ct.c_void_p
libcsound.csoundCreateThread.argtypes = [THREADFUNC, ct.py_object]
libcsound.csoundCreateThread2.restype = ct.c_void_p
libcsound.csoundCreateThread2.argtypes = [THREADFUNC, ct.c_uint, ct.py_object]
libcsound.csoundGetCurrentThreadId.restype = ct.c_void_p
libcsound.csoundJoinThread.restype = ct.POINTER(ct.c_uint)
libcsound.csoundJoinThread.argtypes = [ct.c_void_p]
libcsound.csoundCreateThreadLock.restype = ct.c_void_p
libcsound.csoundWaitThreadLock.argtypes = [ct.c_void_p, ct.c_uint]
libcsound.csoundWaitThreadLockNoTimeout.argtypes = [ct.c_void_p]
libcsound.csoundNotifyThreadLock.argtypes = [ct.c_void_p]
libcsound.csoundDestroyThreadLock.argtypes = [ct.c_void_p]
libcsound.csoundCreateMutex.restype = ct.c_void_p
libcsound.csoundCreateMutex.argtypes = [ct.c_int]
libcsound.csoundLockMutex.argtypes = [ct.c_void_p]
libcsound.csoundLockMutexNoWait.argtypes = [ct.c_void_p]
libcsound.csoundUnlockMutex.argtypes = [ct.c_void_p]
libcsound.csoundDestroyMutex.argtypes = [ct.c_void_p]
libcsound.csoundCreateBarrier.restype = ct.c_void_p
libcsound.csoundCreateBarrier.argtypes = [ct.c_uint]
libcsound.csoundDestroyBarrier.argtypes = [ct.c_void_p]
libcsound.csoundWaitBarrier.argtypes = [ct.c_void_p]
libcsound.csoundSleep.argtypes = [ct.c_uint]
libcsound.csoundSpinLockInit.argtypes = [ct.POINTER(ct.c_int32)]
libcsound.csoundSpinLock.argtypes = [ct.POINTER(ct.c_int32)]
libcsound.csoundSpinTryLock.argtypes = [ct.POINTER(ct.c_int32)]
libcsound.csoundSpinUnLock.argtypes = [ct.POINTER(ct.c_int32)]

libcsound.csoundRunCommand.restype = ct.c_long 
libcsound.csoundRunCommand.argtypes = [ct.POINTER(ct.c_char_p), ct.c_int]
libcsound.csoundInitTimerStruct.argtypes = [ct.POINTER(RtClock)]
libcsound.csoundGetRealTime.restype = ct.c_double
libcsound.csoundGetRealTime.argtypes = [ct.POINTER(RtClock)]
libcsound.csoundGetCPUTime.restype = ct.c_double
libcsound.csoundGetCPUTime.argtypes = [ct.POINTER(RtClock)]
libcsound.csoundGetRandomSeedFromTime.restype = ct.c_uint32
libcsound.csoundGetEnv.restype = ct.c_char_p
libcsound.csoundGetEnv.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundSetGlobalEnv.argtypes = [ct.c_char_p, ct.c_char_p]
libcsound.csoundCreateGlobalVariable.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_uint]
libcsound.csoundQueryGlobalVariable.restype = ct.c_void_p
libcsound.csoundQueryGlobalVariable.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundQueryGlobalVariableNoCheck.restype = ct.c_void_p
libcsound.csoundQueryGlobalVariableNoCheck.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundDestroyGlobalVariable.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundRunUtility.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_int, ct.POINTER(ct.c_char_p)]
libcsound.csoundListUtilities.restype = ct.POINTER(ct.c_char_p)
libcsound.csoundListUtilities.argtypes = [ct.c_void_p]
libcsound.csoundDeleteUtilityList.argtypes = [ct.c_void_p, ct.POINTER(ct.c_char_p)]
libcsound.csoundGetUtilityDescription.restype = ct.c_char_p
libcsound.csoundGetUtilityDescription.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundRand31.argtypes = [ct.POINTER(ct.c_int)]
libcsound.csoundSeedRandMT.argtypes = [ct.POINTER(CsoundRandMTState), ct.POINTER(ct.c_uint32), ct.c_uint32]
libcsound.csoundRandMT.restype = ct.c_uint32
libcsound.csoundRandMT.argtypes = [ct.POINTER(CsoundRandMTState)]
libcsound.csoundCreateCircularBuffer.restype = ct.c_void_p
libcsound.csoundCreateCircularBuffer.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]
libcsound.csoundReadCircularBuffer.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_void_p, ct.c_int]
libcsound.csoundPeekCircularBuffer.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_void_p, ct.c_int]
libcsound.csoundWriteCircularBuffer.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_void_p, ct.c_int]
libcsound.csoundFlushCircularBuffer.argtypes = [ct.c_void_p, ct.c_void_p]
libcsound.csoundDestroyCircularBuffer.argtypes = [ct.c_void_p, ct.c_void_p]
libcsound.csoundOpenLibrary.argtypes = [ct.POINTER(ct.c_void_p), ct.c_char_p]
libcsound.csoundCloseLibrary.argtypes = [ct.c_void_p]
libcsound.csoundGetLibrarySymbol.retype = ct.c_void_p
libcsound.csoundGetLibrarySymbol.argtypes = [ct.c_void_p, ct.c_char_p]


def cchar(s):
    if sys.version_info[0] >= 3:
        return ct.c_char(ord(s[0]))
    return ct.c_char(s[0])

def cstring(s):
    if sys.version_info[0] >= 3 and s != None:
        return bytes(s, 'utf-8')
    return s

def pstring(s):
    if sys.version_info[0] >= 3 and s != None:
        return str(s, 'utf-8')
    return s

def csoundArgList(lst):
    if len(lst) == 1 and type(lst[0]) is list:
        lst = lst[0]
    argc = len(lst)
    argv = (ct.POINTER(ct.c_char_p) * argc)()
    for i in range(argc):
        v = cstring(lst[i])
        argv[i] = ct.cast(ct.pointer(ct.create_string_buffer(v)), ct.POINTER(ct.c_char_p))
    return ct.c_int(argc), ct.cast(argv, ct.POINTER(ct.c_char_p))


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
    """Initializes Csound library with specific flags.
    
    This function is called internally by csoundCreate(), so there is generally
    no need to use it explicitly unless you need to avoid default initialization
    that sets signal handlers and atexit() callbacks.
    Return value is zero on success, positive if initialization was
    done already, and negative on error.
    """
    return libcsound.csoundInitialize(flags)

def setOpcodedir(s):
    """Sets an opcodedir override for csoundCreate()."""
    libcsound.csoundSetOpcodedir(cstring(s))

defaultMessageCbRef = None
def setDefaultMessageCallback(function):
    """Not fully implemented. Do not use it yet except for disabling messaging:
    
    def noMessage(csound, attr, flags, *args):
        pass
    ctcsound.setDefaultMessageCallback(noMessage)
    """
    global defaultMessageCbReg
    defaultMessageCbRef = DEFMSGFUNC(function)
    libcsound.csoundSetDefaultMessageCallback(defaultMessageCbRef)
    
class Csound:
    #Instantiation
    def __init__(self, hostData=None, pointer_=None):
        """Creates an instance of Csound.
       
        Gets an opaque pointer that must be passed to most Csound API
        functions. The *hostData* parameter can be :code:`None`, or it can be
        any sort of data; these data can be accessed from the Csound instance
        that is passed to callback routines.
        """
        if pointer_:
            self.cs = pointer_
            self.fromPointer = True
        else:
            self.cs = libcsound.csoundCreate(ct.py_object(hostData))
            self.fromPointer = False
    
    def loadPlugins(self, directory):
        """Loads all plugins from a given directory."""
        return libcsound.csoundLoadPlugins(self.cs, cstring(directory))
    
    def __del__(self):
        """Destroys an instance of Csound."""
        if not self.fromPointer and libcsound:
            libcsound.csoundDestroy(self.cs)

    def csound(self):
        """Returns the opaque pointer to the running Csound instance."""
        return self.cs
    
    def version(self):
        """Returns the version number times 1000 (5.00.0 = 5000)."""
        return libcsound.csoundGetVersion()
    
    def APIVersion(self):
        """Returns the API version number times 100 (1.00 = 100)."""
        return libcsound.csoundGetAPIVersion()
    
    #Performance
    def parseOrc(self, orc):
        """Parses the given orchestra from an ASCII string into a TREE.
        
        This can be called during performance to parse new code.
        """
        return libcsound.csoundParseOrc(self.cs, cstring(orc))
    
    def compileTree(self, tree):
        """Compiles the given TREE node into structs for Csound to use.
        
        This can be called during performance to compile a new TREE.
        """
        return libcsound.csoundCompileTree(self.cs, tree)
    
    def compileTreeAsync(self, tree):
        """Asynchronous version of :py:meth:`compileTree()`."""
        return libcsound.csoundCompileTreeAsync(self.cs, tree)
    
    def deleteTree(self, tree):
        """Frees the resources associated with the TREE *tree*.
        
        This function should be called whenever the TREE was
        created with :py:meth:`parseOrc` and memory can be deallocated.
        """
        libcsound.csoundDeleteTree(self.cs, tree)

    def compileOrc(self, orc):
        """Parses, and compiles the given orchestra from an ASCII string.
        
        Also evaluating any global space code (i-time only).
        This can be called during performance to compile a new orchestra::
        
            orc = 'instr 1 \\n a1 rand 0dbfs/4 \\n out a1 \\n'
            cs.compileOrc(orc)
        """
        return libcsound.csoundCompileOrc(self.cs, cstring(orc))
    
    def compileOrcAsync(self, orc):
        """Async version of :py:meth:`compileOrc()`.
        
        The code is parsed and compiled, then placed on a queue for
        asynchronous merge into the running engine, and evaluation.
        The function returns following parsing and compilation.
        """
        return libcsound.csoundCompileOrcAsync(self.cs, cstring(orc))
    
    def evalCode(self, code):
        """Parses and compiles an orchestra given on an string.
        
        Evaluating any global space code (i-time only).
        On SUCCESS it returns a value passed to the
        'return' opcode in global space::
        
            code = 'i1 = 2 + 2 \\n return i1 \\n'
            retval = cs.evalCode(code)
        """
        return libcsound.csoundEvalCode(self.cs, cstring(code))
    
    #def initializeCscore(insco, outsco):
    
    def compileArgs(self, *args):
        """Compiles *args*.
        
        Reads arguments, parses and compiles an orchestra,
        reads, processes and loads a score.
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
        
        NB: this is called internally by :py:meth:`compile_()`, therefore
        it is only required if performance is started without
        a call to that function.
        """
        return libcsound.csoundStart(self.cs)
    
    def compile_(self, *args):
        """Compiles Csound input files (such as an orchestra and score).
        
        As directed by the supplied command-line arguments,
        but does not perform them. Returns a non-zero error code on failure.
        This function cannot be called during performance, and before a
        repeated call, :py:meth:`reset()` needs to be called.
        In this (host-driven) mode, the sequence of calls should be as follows::
        
            cs.compile_(args)
            while cs.performBuffer() == 0:
                pass
            cs.cleanup()
            cs.reset()
        
        Calls :py:meth:`start()` internally.
        """
        argc, argv = csoundArgList(args)
        return libcsound.csoundCompile(self.cs, argc, argv)
    
    def compileCsd(self, csd_filename):
        """Compiles a Csound input file (.csd file).
        
        The input file includes command-line arguments, but does not
        perform the file. Returns a non-zero error code on failure.
        In this (host-driven) mode, the sequence of calls should be
        as follows::
        
            cs.compileCsd(args)
            while cs.performBuffer() == 0:
                pass
            cs.cleanup()
            cs.reset()
        
        NB: this function can be called during performance to
        replace or add new instruments and events.
        On a first call and if called before :py:meth:`start()`, this function
        behaves similarly to :py:meth:`compile_()`.
        """
        return libcsound.csoundCompileCsd(self.cs, cstring(csd_filename))
    
    def compileCsdText(self, csd_text):
        """Compiles a Csound input file contained in a string of text.
        
        The string of text includes command-line arguments, orchestra, score,
        etc., but it is not performed. Returns a non-zero error code on failure.
        In this (host-driven) mode, the sequence of calls should be as follows::
        
            cs.compileCsdText(csd_text)
            while cs.performBuffer() == 0:
                pass
            cs.cleanup()
            cs.reset()
        
        NB: a temporary file is created, the csd_text is written to the
        temporary file, and :py:meth:`compileCsd` is called with the name of
        the temporary file, which is deleted after compilation. Behavior may
        vary by platform.
        """
        return libcsound.csoundCompileCsdText(self.cs, cstring(csd_text))

    def perform(self):
        """Senses input events and performs audio output.
        
        This is done until the end of score is reached (positive return value),
        an error occurs (negative return value), or performance is stopped by
        calling :py:meth:`stop()` from another thread (zero return value).
        
        Note that :py:meth:`compile_()`, or :py:meth:`compileOrc()`,
        :py:meth:`readScore()`, :py:meth:`start()` must be
        called first.
        
        In the case of zero return value, :py:meth:`perform()` can be called
        again to continue the stopped performance. Otherwise, :py:meth:`reset()`
        should be called to clean up after the finished or failed performance.
        """
        return libcsound.csoundPerform(self.cs)
    
    def performKsmps(self):
        """Senses input events, and performs audio output.
        
        This is done for one control sample worth (ksmps).
        
        Note that :py:meth:`compile_()`, or :py:meth:`compileOrc()`,
        :py:meth:`readScore()`, :py:meth:`start()` must be called first.
        
        Returns :code:`False` during performance, and :code:`True` when
        performance is finished. If called until it returns :code:`True`,
        it will perform an entire score.
        
        Enables external software to control the execution of Csound,
        and to synchronize performance with audio input and output.
        """
        return libcsound.csoundPerformKsmps(self.cs)
    
    def performBuffer(self):
        """Performs Csound, sensing real-time and score events.
        
        Processing one buffer's worth (-b frames) of interleaved audio.
        
        Note that :py:meth:`compile_()` must be called first, then call
        :py:meth:`outputBuffer()` and :py:meth:`inputBuffer(`) to get ndarrays
        pointing to Csound's I/O buffers.
        
        Returns :code:`False` during performance, and :code:`true` when
        performance is finished.
        """
        return libcsound.csoundPerformBuffer(self.cs)
    
    def stop(self):
        """Stops a :py:meth:`perform()` running in another thread.
        
        Note that it is not guaranteed that :py:meth:`perform()` has already
        stopped when this function returns.
        """
        libcsound.csoundStop(self.cs)
    
    def cleanup(self):
        """Prints information and closes audio and MIDI devices.
        
        | The information is about the end of a performance.
        | Note: after calling cleanup(), the operation of the perform
          function is undefined.
        """
        return libcsound.csoundCleanup(self.cs)
    
    def reset(self):
        """Resets all internal memory and state.
        
        In preparation for a new performance.
        Enable external software to run successive Csound performances
        without reloading Csound. Implies :py:meth:`cleanup()`, unless already
        called.
        """
        libcsound.csoundReset(self.cs)

    #UDP server
    def UDPServerStart(self, port):
        """Starts the UDP server on a supplied port number.
        
        Returns CSOUND_SUCCESS if server has been started successfully,
        otherwise, CSOUND_ERROR.
        """
        return libcsound.csoundUDPServerStart(self.cs, ct.c_uint(port))

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
        destination (see :py:meth:`setMessageCallback()`) as well as to UDP.
        Returns CSOUND_SUCCESS or CSOUND_ERROR if the UDP transmission
        could not be set up.
        """
        return libcsound.csoundUDPConsole(self.cs, cstring(addr), ct.c_uint(port), ct.c_uint(mirror))

    def stopUDPConsole(self):
        """Stops transmitting console messages via UDP."""
        libcsound.csoundStopUDPConsole(self.cs)

    #Attributes
    def sr(self):
        """Returns the number of audio sample frames per second."""
        return libcsound.csoundGetSr(self.cs)
    
    def kr(self):
        """Returns the number of control samples per second."""
        return libcsound.csoundGetKr(self.cs)
    
    def ksmps(self):
        """Returns the number of audio sample frames per control sample."""
        return libcsound.csoundGetKsmps(self.cs)
    
    def nchnls(self):
        """Returns the number of audio output channels.
        
        Set through the :code:`nchnls` header variable in the csd file.
        """
        return libcsound.csoundGetNchnls(self.cs)
    
    def nchnlsInput(self): 
        """Returns the number of audio input channels.
        
        Set through the :code:`nchnls_i` header variable in the csd file. If
        this variable is not set, the value is taken from :code:`nchnls`.
        """
        return libcsound.csoundGetNchnlsInput(self.cs)
    
    def get0dBFS(self):
        """Returns the 0dBFS level of the spin/spout buffers."""
        return libcsound.csoundGet0dBFS(self.cs)
    
    def A4(self):
        """Returns the A4 frequency reference."""
        return libcsound.csoundGetA4(self.cs)
    
    def currentTimeSamples(self):
        """Returns the current performance time in samples."""
        return libcsound.csoundGetCurrentTimeSamples(self.cs)
    
    def sizeOfMYFLT(self):
        """Returns the size of MYFLT in bytes."""
        return libcsound.csoundGetSizeOfMYFLT()
    
    def hostData(self):
        """Returns host data."""
        return libcsound.csoundGetHostData(self.cs)
    
    def setHostData(self, data):
        """Sets host data."""
        libcsound.csoundSetHostData(self.cs, ct.py_object(data))
    
    def setOption(self, option):
        """Sets a single csound option (flag).
        
        | Returns CSOUND_SUCCESS on success.
        | NB: blank spaces are not allowed.
        """
        return libcsound.csoundSetOption(self.cs, cstring(option))
    
    def setParams(self, params):
        """Configures Csound with a given set of parameters.
        
        These parameters are defined in the CsoundParams structure.
        They are the part of the OPARMS struct that are configurable through
        command line flags.
        The CsoundParams structure can be obtained using :py:meth:`params()`.
        These options should only be changed before performance has started.
        """
        libcsound.csoundSetParams(self.cs, ct.byref(params))
    
    def params(self, params):
        """Gets the current set of parameters from a CSOUND instance.
        
        These parameters are in a CsoundParams structure. See
        :py:meth:`setParams()`::
        
            p = CsoundParams()
            cs.params(p)
        """
        libcsound.csoundGetParams(self.cs, ct.byref(params))
    
    def debug(self):
        """Returns whether Csound is set to print debug messages.
        
        Those messages are sent through the :code:`DebugMsg()` internal API
        function.
        """
        return libcsound.csoundGetDebug(self.cs) != 0
    
    def setDebug(self, debug):
        """Sets whether Csound prints debug messages.
        
        The debug argument must have value :code:`True` or :code:`False`.
        Those messages come from the :code:`DebugMsg()` internal API function.
        """
        libcsound.csoundSetDebug(self.cs, ct.c_int(debug))

    def systemSr(self, val):
        """If val > 0, sets the internal variable holding the system HW sr.
        
        Returns the stored value containing the system HW sr."""
        return libcsound.csoundSystemSr(self.cs, val)

    #General Input/Output
    def outputName(self):
        """Returns the audio output name (-o)"""
        s = libcsound.csoundGetOutputName(self.cs)
        return pstring(s)
    
    def inputName(self):
        """Returns the audio input name (-i)"""
        s = libcsound.csoundGetInputName(self.cs)
        return pstring(s)
    
    def setOutput(self, name, type_, format):
        """Sets output destination, type and format.
        
        | *type_* can be one of  "wav", "aiff", "au", "raw", "paf", "svx", "nist",
          "voc", "ircam", "w64", "mat4", "mat5", "pvf", "xi", "htk", "sds",
          "avr", "wavex", "sd2", "flac", "caf", "wve", "ogg", "mpc2k", "rf64",
          or NULL (use default or realtime IO).
        | *format* can be one of "alaw", "schar", "uchar", "float", "double",
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
        """Gets output type and format."""
        type_ = ct.create_string_buffer(6)
        format = ct.create_string_buffer(8)
        libcsound.csoundGetOutputFormat(self.cs, type_, format)
        return pstring(string_at(type_)), pstring(string_at(format))

    def setInput(self, name):
        """Sets input source."""
        libcsound.csoundSetInput(self.cs, cstring(name))
    
    def setMIDIInput(self, name):
        """Sets MIDI input device name/number."""
        libcsound.csoundSetMidiInput(self.cs, cstring(name))
    
    def setMIDIFileInput(self, name):
        """Sets MIDI file input name."""
        libcsound.csoundSetMIDIFileInput(self.cs, cstring(name))
    
    def setMIDIOutput(self, name):
        """Sets MIDI output device name/number."""
        libcsound.csoundSetMIDIOutput(self.cs, cstring(name))
    
    def setMIDIFileOutput(self, name):
        """Sets MIDI file output name."""
        libcsound.csoundSetMIDIFileOutput(self.cs, cstring(name))

    def setFileOpenCallback(self, function):
        """Sets a callback for receiving notices whenever Csound opens a file.
        
        The callback is made after the file is successfully opened.
        The following information is passed to the callback:
        
        bytes
            pathname of the file; either full or relative to current dir
        int
            a file type code from the enumeration CSOUND_FILETYPES
        int
            1 if Csound is writing the file, 0 if reading
        int
            1 if a temporary file that Csound will delete; 0 if not
           
        Pass NULL to disable the callback.
        This callback is retained after a :py:meth:`reset()` call.
        """
        self.fileOpenCbRef = FILEOPENFUNC(function)
        libcsound.csoundSetFileOpenCallback(self.cs, self.fileOpenCbRef)

    #Realtime Audio I/O
    def setRTAudioModule(self, module):
        """Sets the current RT audio module."""
        libcsound.csoundSetRTAudioModule(self.cs, cstring(module))
    
    def module(self, number):
        """Retrieves a module name and type given a number.
        
        Type is "audio" or "midi". Modules are added to list as csound loads
        them. Return CSOUND_SUCCESS on success and CSOUND_ERROR if module
        number was not found::
        
            n = 0
            while True:
                name, type_, err = cs.module(n)
                if err == ctcsound.CSOUND_ERROR:
                    break
                print("Module %d:%s (%s)\\n" % (n, name, type_))
                n = n + 1
        """
        name = ct.pointer(ct.c_char_p(cstring("dummy")))
        type_ = ct.pointer(ct.c_char_p(cstring("dummy")))
        err = libcsound.csoundGetModule(self.cs, number, name, type_)
        if err == CSOUND_ERROR:
            return None, None, err
        n = pstring(string_at(name.contents))
        t = pstring(string_at(type_.contents))
        return n, t, err
    
    def inputBufferSize(self):
        """Returns the number of samples in Csound's input buffer."""
        return libcsound.csoundGetInputBufferSize(self.cs)
    
    def outputBufferSize(self):
        """Returns the number of samples in Csound's output buffer."""
        return libcsound.csoundGetOutputBufferSize(self.cs)
    
    def inputBuffer(self):
        """Returns the Csound audio input buffer as an ndarray.
        
        Enables external software to write audio into Csound before
        calling :py:meth:`performBuffer()`.
        """
        buf = libcsound.csoundGetInputBuffer(self.cs)
        size = libcsound.csoundGetInputBufferSize(self.cs)
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = ct.cast(buf, arrayType)
        return arrFromPointer(p)
    
    def outputBuffer(self):
        """Returns the Csound audio output buffer as an ndarray.
        
        Enables external software to read audio from Csound after
        calling :py:meth:`performBuffer()`.
        """
        buf = libcsound.csoundGetOutputBuffer(self.cs)
        size = libcsound.csoundGetOutputBufferSize(self.cs)
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = ct.cast(buf, arrayType)
        return arrFromPointer(p)
    
    def spin(self):
        """Returns the Csound audio input working buffer (spin) as an ndarray.
        
        Enables external software to write audio into Csound before
        calling :py:meth:`performKsmps()`.
        """
        buf = libcsound.csoundGetSpin(self.cs)
        size = self.ksmps() * self.nchnlsInput()
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = ct.cast(buf, arrayType)
        return arrFromPointer(p)
    
    def clearSpin(self):
        """Clears the input buffer (spin)."""
        libcsound.csoundClearSpin(self.cs)
    
    def addSpinSample(self, frame, channel, sample):
        """Adds the indicated sample into the audio input working buffer (spin).
        
        This only ever makes sense before calling :py:meth:`performKsmps()`.
        The frame and channel must be in bounds relative to :py:meth:`ksmps()`
        and :py:meth:`nchnlsInput()`.
        
        NB: the spin buffer needs to be cleared at every k-cycle by calling 
        :py:meth:`clearSpin()`.
        """
        libcsound.csoundAddSpinSample(self.cs, frame, channel, sample)
    
    def setSpinSample(self, frame, channel, sample):
        """Sets the audio input working buffer (spin) to the indicated sample.
        
        This only ever makes sense before calling :py:meth:`performKsmps()`.
        The frame and channel must be in bounds relative to :py:meth:`ksmps()`
        and :py:meth:`nchnlsInput()`.
        """
        libcsound.csoundSetSpinSample(self.cs, frame, channel, sample)
    
    def spout(self):
        """Returns the address of the Csound audio output working buffer (spout).
        
        Enables external software to read audio from Csound after
        calling :py:meth:`performKsmps`.
        """
        buf = libcsound.csoundGetSpout(self.cs)
        size = self.ksmps() * self.nchnls()
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = ct.cast(buf, arrayType)
        return arrFromPointer(p)
    
    def spoutSample(self, frame, channel):
        """Returns one sample from the Csound audio output working buf (spout).
        
        Only ever makes sense after calling :py:meth:`performKsmps()`. The
        *frame* and *channel* must be in bounds relative to :py:meth:`ksmps()`
        and :py:meth:`nchnls()`.
        """
        return libcsound.csoundGetSpoutSample(self.cs, frame, channel)
    
    def rtRecordUserData(self):
        """Returns pointer to user data pointer for real time audio input."""
        return libcsound.csoundGetRtRecordUserData(self.cs)

    def rtPlaydUserData(self):
        """Returns pointer to user data pointer for real time audio output."""
        return libcsound.csoundGetRtPlayUserData(self.cs)

    def setHostImplementedAudioIO(self, state, bufSize):
        """Sets user handling of sound I/O.
        
        Calling this function with a :code:`True` *state* value between creation
        of the Csound object and the start of performance will disable all
        default handling of sound I/O by the Csound library, allowing the host
        application to use the spin/spout/input/output buffers directly.
        
        For applications using spin/spout, *bufSize* should be set to 0.
        If *bufSize* is greater than zero, the buffer size (-b) will be
        set to the integer multiple of :py:meth:`ksmps()` that is nearest to the
        value specified.
        """
        libcsound.csoundSetHostImplementedAudioIO(self.cs, ct.c_int(state), bufSize)

    def audioDevList(self, isOutput):
        """Returns a list of available input or output audio devices.
        
        Each item in the list is a dictionnary representing a device. The
        dictionnary keys are *device_name*, *device_id*, *rt_module* (value
        type string), *max_nchnls* (value type int), and *isOutput* (value 
        type boolean).
        
        Must be called after an orchestra has been compiled
        to get meaningful information.
        """
        n = libcsound.csoundGetAudioDevList(self.cs, None, ct.c_int(isOutput))
        devs = (CsoundAudioDevice * n)()
        libcsound.csoundGetAudioDevList(self.cs, ct.byref(devs), ct.c_int(isOutput))
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
        """Sets a callback for opening real-time audio playback."""
        self.playOpenCbRef = PLAYOPENFUNC(function)
        libcsound.csoundSetPlayopenCallback(self.cs, self.playOpenCbRef)

    def setRtPlayCallback(self, function):
        """Sets a callback for performing real-time audio playback."""
        self.rtPlayCbRef = RTPLAYFUNC(function)
        libcsound.csoundSetRtplayCallback(self.cs, self.rtPlayCbRef)
  
    def setRecordOpenCallback(self, function):
        """Sets a callback for opening real-time audio recording."""
        self.recordOpenCbRef = RECORDOPENFUNC(function)
        libcsound.csoundSetRecopenCallback(self.cs, self.recordOpenCbRef)

    def setRtRecordCallback(self, function):
        """Sets a callback for performing real-time audio recording."""
        self.rtRecordCbRef = RTRECORDFUNC(function)
        libcsound.csoundSetRtrecordCallback(self.cs, self.rtRecordCbRef)
    
    def setRtCloseCallback(self, function):
        """Sets a callback for closing real-time audio playback and recording."""
        self.rtCloseCbRef = RTCLOSEFUNC(function)
        libcsound.csoundSetRtcloseCallback(self.cs, self.rtCloseCbRef)
    
    def setAudioDevListCallback(self, function):
        """Sets a callback for obtaining a list of audio devices.
        
        This should be set by rtaudio modules and should not be set by hosts.
        (See :py:meth:`audioDevList()`).
        """
        self.audioDevListCbRef = AUDIODEVLISTFUNC(function)
        libcsound.csoundSetAudioDeviceListCallback(self.cs, self.audioDevListCbRef)
    
    #Realtime MIDI I/O
    def setMIDIModule(self, module):
        """Sets the current MIDI IO module."""
        libcsound.csoundSetMIDIModule(self.cs, cstring(module))
    
    def setHostImplementedMIDIIO(self, state):
        """Called with *state* :code:`True` if the host is implementing via callbacks."""
        libcsound.csoundSetHostImplementedMIDIIO(self.cs, ct.c_int(state))
    
    def midiDevList(self, isOutput):
        """Returns a list of available input or output midi devices.
        
        Each item in the list is a dictionnary representing a device. The
        dictionnary keys are *device_name*, *interface_name*, *device_id*,
        *midi_module* (value type string), *isOutput* (value type boolean).
        
        Must be called after an orchestra has been compiled
        to get meaningful information.
        """
        n = libcsound.csoundGetMIDIDevList(self.cs, None, ct.c_int(isOutput))
        devs = (CsoundMidiDevice * n)()
        libcsound.csoundGetMIDIDevList(self.cs, ct.byref(devs), ct.c_int(isOutput))
        lst = []
        for dev in devs:
            d = {}
            d["device_name"] = pstring(dev.device_name)
            d["interface_name"] = pstring(dev.interface_name)
            d["device_id"] = pstring(dev.device_id)
            d["midi_module"] = pstring(dev.midi_module)
            d["isOutput"] = (dev.isOutput == 1)
            lst.append(d)
        return lst

    def setExternalMidiInOpenCallback(self, function):
        """Sets a callback for opening real-time MIDI input."""
        self.extMidiInOpenCbRef = MIDIINOPENFUNC(function)
        libcsound.csoundSetExternalMidiInOpenCallback(self.cs, self.extMidiInOpenCbRef)

    def setExternalMidiReadCallback(self, function):
        """Sets a callback for reading from real time MIDI input."""
        self.extMidiReadCbRef = MIDIREADFUNC(function)
        libcsound.csoundSetExternalMidiReadCallback(self.cs, self.extMidiReadCbRef)
    
    def setExternalMidiInCloseCallback(self, function):                
        """Sets a callback for closing real time MIDI input."""
        self.extMidiInCloseCbRef = MIDIINCLOSEFUNC(function)
        libcsound.csoundSetExternalMidiInCloseCallback(self.cs, self.extMidiInCloseCbRef)
    
    def setExternalMidiOutOpenCallback(self, function):
        """Sets a callback for opening real-time MIDI input."""
        self.extMidiOutOpenCbRef = MIDIOUTOPENFUNC(function)
        libcsound.csoundSetExternalMidiOutOpenCallback(self.cs, self.extMidiOutOpenCbRef)

    def setExternalMidiWriteCallback(self, function):
        """Sets a callback for reading from real time MIDI input."""
        self.extMidiWriteCbRef = MIDIWRITEFUNC(function)
        libcsound.csoundSetExternalMidiWriteCallback(self.cs, self.extMidiWriteCbRef)
    
    def setExternalMidiOutCloseCallback(self, function):
        """Sets a callback for closing real time MIDI input."""
        self.extMidiOutCloseCbRef = MIDIOUTCLOSEFUNC(function)
        libcsound.csoundSetExternalMidiOutCloseCallback(self.cs, self.extMidiOutCloseCbRef)

    def setExternalMidiErrorStringCallback(self, function):
        """ Sets a callback for converting MIDI error codes to strings."""
        self.extMidiErrStrCbRef = MIDIERRORFUNC(function)
        libcsound.csoundSetExternalMidiErrorStringCallback(self.cs, self.extMidiErrStrCbRef)
    
    def setMidiDevListCallback(self, function):
        """Sets a callback for obtaining a list of MIDI devices.
        
        This should be set by IO plugins and should not be set by hosts.
        (See :py:meth:`midiDevList()`).
        """
        self.midiDevListCbRef = MIDIDEVLISTFUNC(function)
        libcsound.csoundSetMIDIDeviceListCallback(self.cs, self.midiDevListCbRef)

    #Score Handling
    def readScore(self, sco):
        """Reads, preprocesses, and loads a score from an ASCII string.
        
        It can be called repeatedly, with the new score events
        being added to the currently scheduled ones.
        """
        return libcsound.csoundReadScore(self.cs, cstring(sco))
    
    def readScoreAsync(self, sco):
        """Asynchronous version of :py:meth:`readScore()`."""
        libcsound.csoundReadScoreAsync(self.cs, cstring(sco))
    
    def scoreTime(self):
        """Returns the current score time.
        
        The return value is the time in seconds since the beginning of
        performance.
        """
        return libcsound.csoundGetScoreTime(self.cs)
    
    def isScorePending(self):
        """Tells whether Csound score events are performed or not.
        
        Independently of real-time MIDI events (see :py:meth:`setScorePending()`).
        """
        return libcsound.csoundIsScorePending(self.cs) != 0
    
    def setScorePending(self, pending):
        """Sets whether Csound score events are performed or not.
        
        Real-time events will continue to be performed. Can be used by external
        software, such as a VST host, to turn off performance of score events
        (while continuing to perform real-time events), for example to mute
        a Csound score while working on other tracks of a piece, or to play
        the Csound instruments live.
        """
        libcsound.csoundSetScorePending(self.cs, ct.c_int(pending))
    
    def scoreOffsetSeconds(self):
        """Returns the score time beginning midway through a Csound score.
        
        At this time score events will actually immediately be performed
        (see :py:meth:`setScoreOffsetSeconds()`).
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
        
        It is rewinded to the time specified with :py:meth:`setScoreOffsetSeconds()`.
        """
        libcsound.csoundRewindScore(self.cs)
    
    def setCscoreCallback(self, function):
        """Sets an external callback for Cscore processing.
        
        Pass :code:`None` to reset to the internal :code:`cscore()` function
        (which does nothing). This callback is retained after a
        :py:meth:`reset()` call.
        """
        self.cscoreCbRef = CSCOREFUNC(function)
        libcsound.csoundSetCscoreCallback(self.cs, self.cscoreCbRef)
    
    #def scoreSort(self, inFile, outFile):
    
    #def scoreExtract(self, inFile, outFile, extractFile)
    
    #Messages and Text
    def message(self, fmt, *args):
        """Displays an informational message.
        
        This is a workaround because :program:`ctypes` does not support
        variadic functions.
        The arguments are formatted in a string, using the python way, either
        old style or new style, and then this formatted string is passed to
        the Csound display message system.
        """
        if fmt[0] == '{':
            s = fmt.format(*args)
        else:
            s = fmt % args
        libcsound.csoundMessage(self.cs, cstring("%s"), cstring(s))
    
    def messageS(self, attr, fmt, *args):
        """Prints message with special attributes.
        
        (See msg_attr for the list of available attributes). With attr=0,
        messageS() is identical to :py:meth:`message()`.
        This is a workaround because :program:`ctypes` does not support
        variadic functions.
        The arguments are formatted in a string, using the python way, either
        old style or new style, and then this formatted string is passed to
        the csound display message system.
        """
        if fmt[0] == '{':
            s = fmt.format(*args)
        else:
            s = fmt % args
        libcsound.csoundMessageS(self.cs, attr, cstring("%s"), cstring(s))

    #def setMessageCallback():
    
    #def setMessageStringCallback()
    
    def messageLevel(self):
        """Returns the Csound message level (from 0 to 231)."""
        return libcsound.csoundGetMessageLevel(self.cs)
    
    def setMessageLevel(self, messageLevel):
        """Sets the Csound message level (from 0 to 231)."""
        libcsound.csoundSetMessageLevel(self.cs, messageLevel)
    
    def createMessageBuffer(self, toStdOut):
        """Creates a buffer for storing messages printed by Csound.
        
        Should be called after creating a Csound instance and the buffer
        can be freed by calling :py:meth:`destroyMessageBuffer()` before
        deleting the Csound instance. You will generally want to call
        :py:meth:`cleanup()` to make sure the last messages are flushed to
        the message buffer before destroying Csound.
        
        If *toStdOut* is :code:`True`, the messages are also printed to
        stdout and stderr (depending on the type of the message),
        in addition to being stored in the buffer.
        
        Using the message buffer ties up the internal message callback, so
        :py:meth:`setMessageCallback()` should not be called after creating the
        message buffer.
        """
        libcsound.csoundCreateMessageBuffer(self.cs,  ct.c_int(toStdOut))
    
    def firstMessage(self):
        """Returns the first message from the buffer."""
        s = libcsound.csoundGetFirstMessage(self.cs)
        return pstring(s)
    
    def firstMessageAttr(self):
        """Returns the attribute parameter of the first message in the buffer."""
        return libcsound.csoundGetFirstMessageAttr(self.cs)
    
    def popFirstMessage(self):
        """Removes the first message from the buffer."""
        libcsound.csoundPopFirstMessage(self.cs)
    
    def messageCnt(self):
        """Returns the number of pending messages in the buffer."""
        return libcsound.csoundGetMessageCnt(self.cs)
    
    def destroyMessageBuffer(self):
        """Releases all memory used by the message buffer."""
        libcsound.csoundDestroyMessageBuffer(self.cs)
    
    #Channels, Control and Events
    def channelPtr(self, name, type_):
        """Returns a pointer to the specified channel and an error message.
        
        If the channel is a control or an audio channel, the pointer is
        translated to an ndarray of MYFLT. If the channel is a string channel,
        the pointer is casted to :code:`ct.c_char_p`. The error message is either
        an empty string or a string describing the error that occured.
        
        The channel is created first if it does not exist yet.
        *type_* must be the bitwise OR of exactly one of the following values,
        
        CSOUND_CONTROL_CHANNEL
            control data (one MYFLT value)
        CSOUND_AUDIO_CHANNEL
            audio data (:py:meth:`ksmps()` MYFLT values)
        CSOUND_STRING_CHANNEL
            string data (MYFLT values with enough space to store
            :py:meth:`getChannelDatasize()` characters, including the
            NULL character at the end of the string)
        
        and at least one of these:
        
        CSOUND_INPUT_CHANNEL
        
        CSOUND_OUTPUT_CHANNEL
        
        If the channel already exists, it must match the data type
        (control, audio, or string), however, the input/output bits are
        OR'd with the new value. Note that audio and string channels
        can only be created after calling :py:meth:`compile_()`, because the
        storage size is not known until then.

        Return value is zero on success, or a negative error code,
        
        CSOUND_MEMORY
            there is not enough memory for allocating the channel
        CSOUND_ERROR
            the specified name or type is invalid
            
        or, if a channel with the same name but incompatible type
        already exists, the type of the existing channel. In the case
        of any non-zero return value, \*p is set to NULL.
        Note: to find out the type of a channel without actually
        creating or changing it, set *type_* to zero, so that the return
        value will be either the type of the channel, or CSOUND_ERROR
        if it does not exist.
        
        Operations on the pointer are not thread-safe by default. The host is
        required to take care of threadsafety by retrieving the channel lock
        with :py:meth:`channelLock()` and using :py:meth:`spinLock()` and
        :py:meth:`spinUnLock()` to protect access to the pointer.
        
        See Top/threadsafe.c in the Csound library sources for
        examples. Optionally, use the channel get/set functions
        provided below, which are threadsafe by default.
        """
        length = 0
        chanType = type_ & CSOUND_CHANNEL_TYPE_MASK
        if chanType == CSOUND_CONTROL_CHANNEL:
            length = 1
        elif chanType == CSOUND_AUDIO_CHANNEL:
            length = libcsound.csoundGetKsmps(self.cs)
        ptr = ct.POINTER(MYFLT)()
        err = ''
        ret = libcsound.csoundGetChannelPtr(self.cs, ct.byref(ptr), cstring(name), type_)
        if ret == CSOUND_SUCCESS:
            if chanType == CSOUND_STRING_CHANNEL:
                return ct.cast(ptr, ct.c_char_p), err
            else:
                arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (length,), 'C_CONTIGUOUS')
                p = ct.cast(ptr, arrayType)
                return arrFromPointer(p), err
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
        """Returns a pointer and an error message.
        
        The pointer points to a list of ControlChannelInfo objects for allocated
        channels. A ControlChannelInfo object contains the channel
        characteristics. The error message indicates if there is not enough
        memory for allocating the list or it is an empty string if there is no
        error. In the case of no channels or an error, the pointer is
        :code:`None`.

        Notes: the caller is responsible for freeing the list returned by the
        C API with :py:meth:`deleteChannelList()`. The name pointers may become
        invalid after calling :py:meth:`reset()`.
        """
        cInfos = None
        err = ''
        ptr = ct.cast(ct.POINTER(ct.c_int)(), ct.POINTER(ControlChannelInfo))
        n = libcsound.csoundListChannels(self.cs, ct.byref(ptr))
        if n == CSOUND_MEMORY :
            err = 'There is not enough memory for allocating the list'
        if n > 0:
            cInfos = ct.cast(ptr, ct.POINTER(ControlChannelInfo * n)).contents
        return cInfos, err

    def deleteChannelList(self, lst):
        """Releases a channel list previously returned by :py:meth:`listChannels()`."""
        ptr = ct.cast(lst, ct.POINTER(ControlChannelInfo))
        libcsound.csoundDeleteChannelList(self.cs, ptr)
    
    def setControlChannelHints(self, name, hints):
        """Sets parameters hints for a control channel.
        
        These hints have no internal function but can be used by front ends to
        construct GUIs or to constrain values. See the ControlChannelHints
        structure for details.
        Returns zero on success, or a non-zero error code on failure:
        
        CSOUND_ERROR
            the channel does not exist, is not a control channel,
            or the specified parameters are invalid
        CSOUND_MEMORY
            could not allocate memory
        """
        return libcsound.csoundSetControlChannelHints(self.cs, cstring(name), hints)

    def controlChannelHints(self, name):
        """Returns special parameters (if any) of a control channel.
        
        Those parameters have been previously set with 
        :py:meth:`setControlChannelHints()` or the :code:`chnparams` opcode.
       
        The return values are a ControlChannelHints structure and
        CSOUND_SUCCESS if the channel exists and is a control channel,
        otherwise, :code:`None` and an error code are returned.
        """
        hints = ControlChannelHints()
        ret = libcsound.csoundGetControlChannelHints(self.cs, cstring(name), ct.byref(hints))
        if ret != CSOUND_SUCCESS:
            hints = None
        return hints, ret
    
    def channelLock(self, name):
        """Recovers a pointer to a lock for the specified channel called *name*.
        
        The returned lock can be locked/unlocked  with the :py:meth:`spinLock()`
        and :py:meth:`spinUnLock()` functions.
        Returns the address of the lock or NULL if the channel does not exist.
        """
        return libcsound.csoundGetChannelLock(self.cs, cstring(name))
    
    def controlChannel(self, name):
        """Retrieves the value of control channel identified by *name*.
        
        A second value is returned, which is the error (or success) code
        finding or accessing the channel.
        """
        err = ct.c_int(0)
        ret = libcsound.csoundGetControlChannel(self.cs, cstring(name), ct.byref(err))
        return ret, err
    
    def setControlChannel(self, name, val):
        """Sets the value of control channel identified by *name*."""
        libcsound.csoundSetControlChannel(self.cs, cstring(name), MYFLT(val))
    
    def audioChannel(self, name, samples):
        """Copies the audio channel identified by *name* into ndarray samples.
        
        *samples* should contain enough memory for :py:meth:`ksmps()` MYFLTs.
        """
        ptr = samples.ctypes.data_as(ct.POINTER(MYFLT))
        libcsound.csoundGetAudioChannel(self.cs, cstring(name), ptr)
    
    def setAudioChannel(self, name, samples):
        """Sets the audio channel *name* with data from the ndarray *samples*.
        
        *samples* should contain at least :py:meth:`ksmps()` MYFLTs.
        """
        ptr = samples.ctypes.data_as(ct.POINTER(MYFLT))
        libcsound.csoundSetAudioChannel(self.cs, cstring(name), ptr)
    
    def stringChannel(self, name, string):
        """Copies the string channel identified by *name* into *string*.
        
        *string* should contain enough memory for the string
        (see :py:meth:`channelDatasize()` below).
        """
        libcsound.csoundGetStringChannel(self.cs, cstring(name), cstring(string))

    def setStringChannel(self, name, string):
        """Sets the string channel identified by *name* with *string*."""
        libcsound.csoundSetStringChannel(self.cs, cstring(name), cstring(string))
    
    def channelDatasize(self, name):
        """Returns the size of data stored in a channel.
        
        For string channels this might change if the channel space gets
        reallocated. Since string variables use dynamic memory allocation in
        Csound6, this function can be called to get the space required for
        :py:meth:`stringChannel()`.
        """
        return libcsound.csoundGetChannelDatasize(self.cs, cstring(name))

    def setInputChannelCallback(self, function):
        """Sets the function to call whenever the :code:`invalue` opcode is used."""
        self.inputChannelCbRef = CHANNELFUNC(function)
        libcsound.csoundSetInputChannelCallback(self.cs, self.inputChannelCbRef)
    
    def setOutputChannelCallback(self, function):
        """Sets the function to call whenever the :code:`outvalue` opcode is used."""
        self.outputChannelCbRef = CHANNELFUNC(function)
        libcsound.csoundSetOutputChannelCallback(self.cs, self.outputChannelCbRef)

    def setPvsChannel(self, fin, name):
        """Sends a PvsdatExt *fin* to the :code:`pvsin` opcode (f-rate) for channel *name*.
        
        | Returns zero on success, CSOUND_ERROR if the index is invalid or
          fsig framesizes are incompatible.
        | CSOUND_MEMORY if there is not enough memory to extend the bus.
        """
        return libcsound.csoundSetPvsChannel(self.cs, ct.byref(fin), cstring(name))
    
    def pvsChannel(self, fout, name):
        """Receives a PvsdatExt *fout* from the :code:`pvsout` opcode (f-rate) at channel *name*.
        
        | Returns zero on success, CSOUND_ERROR if the index is invalid or
          if fsig framesizes are incompatible.
        | CSOUND_MEMORY if there is not enough memory to extend the bus.
        """
        return libcsound.csoundGetPvsChannel(self.cs, ct.byref(fout), cstring(name))
    
    def scoreEvent(self, type_, pFields):
        """Sends a new score event.
        
        | *type_* is the score event type ('a', 'i', 'q', 'f', or 'e').
        | *pFields* is a tuple, a list, or an ndarray of MYFLTs with all the
          pfields for this event, starting with the p1 value specified in
          pFields[0].
        """
        p = np.asarray(pFields, dtype=MYFLT)
        ptr = p.ctypes.data_as(ct.POINTER(MYFLT))
        numFields = ct.c_long(p.size)
        return libcsound.csoundScoreEvent(self.cs, cchar(type_), ptr, numFields)
    
    def scoreEventAsync(self, type_, pFields):
        """Asynchronous version of :py:meth:`scoreEvent()`."""
        p = np.asarray(pFields, dtype=MYFLT)
        ptr = p.ctypes.data_as(ct.POINTER(MYFLT))
        numFields = ct.c_long(p.size)
        libcsound.csoundScoreEventAsync(self.cs, cchar(type_), ptr, numFields)
    
    def scoreEventAbsolute(self, type_, pFields, timeOffset):
        """Like :py:meth:`scoreEvent()`, this function inserts a score event.
        
        The event is inserted at absolute time with respect to the start of
        performance, or from an offset set with timeOffset.
        """
        p = np.asarray(pFields, dtype=MYFLT)
        ptr = p.ctypes.data_as(ct.POINTER(MYFLT))
        numFields = ct.c_long(p.size)
        return libcsound.csoundScoreEventAbsolute(self.cs, cchar(type_), ptr, numFields, ct.c_double(timeOffset))
    
    def scoreEventAbsoluteAsync(self, type_, pFields, timeOffset):
        """Asynchronous version of :py:meth:`scoreEventAbsolute()`."""
        p = np.asarray(pFields, dtype=MYFLT)
        ptr = p.ctypes.data_as(ct.POINTER(MYFLT))
        numFields = ct.c_long(p.size)
        libcsound.csoundScoreEventAbsoluteAsync(self.cs, cchar(type_), ptr, numFields, ct.c_double(timeOffset))
    
    def inputMessage(self, message):
        """Inputs a NULL-terminated string (as if from a console).
        
        Used for line events.
        """
        libcsound.csoundInputMessage(self.cs, cstring(message))
    
    def inputMessageAsync(self, message):
        """Asynchronous version of :py:meth:`inputMessage()`."""
        libcsound.csoundInputMessageAsync(self.cs, cstring(message))
    
    def killInstance(self, instr, instrName, mode, allowRelease):
        """Kills off one or more running instances of an instrument.
        
        The instances are identified by *instr* (number) or *instrName* (name).
        If *instrName* is :code:`None`, the instrument number is used.
        
        | *mode* is a sum of the following values:
        | 0, 1, 2: kill all instances (0), oldest only (1), or newest (2)
        | 4: only turnoff notes with exactly matching (fractional) instr number
        | 8: only turnoff notes with indefinite duration (p3 < 0 or MIDI).
        
        If *allowRelease* is :code:`True`, the killed instances are allowed to
        release.
        """
        return libcsound.csoundKillInstance(self.cs, MYFLT(instr), cstring(instrName), mode, ct.c_int(allowRelease))
    
    def registerSenseEventCallback(self, function, userData):
        """Registers a function to be called by :code:`sensevents()`.
        
        This function will be called once in every control period. Any number
        of functions may be registered, and will be called in the order of
        registration.
        
        The callback function takes two arguments: the Csound instance
        pointer, and the *userData* pointer as passed to this function.
        
        This facility can be used to ensure a function is called synchronously
        before every csound control buffer processing. It is important
        to make sure no blocking operations are performed in the callback.
        The callbacks are cleared on :py:meth:`cleanup()`.
        
        Returns zero on success.
        """
        self.senseEventCbRef = SENSEFUNC(function)
        return libcsound.csoundRegisterSenseEventCallback(self.cs, self.senseEventCbRef, ct.py_object(userData))
    
    def keyPress(self, c):
        """Sets the ASCII code of the most recent key pressed.
        
        This value is used by the :code:`sensekey` opcode if a callback for
        returning keyboard events is not set (see
        :py:meth:`registerKeyboardCallback()`).
        """
        libcsound.csoundKeyPress(self.cs, cchar(c))
    
    def registerKeyboardCallback(self, function, userData, type_):
        """Registers general purpose callback functions for keyboard events.
        
        These callbacks are called on every control period by the sensekey
        opcode.
        
        The callback is preserved on :py:meth:`reset()`, and multiple
        callbacks may be set and will be called in reverse order of
        registration. If the same function is set again, it is only moved
        in the list of callbacks so that it will be called first, and the
        user data and type mask parameters are updated. *type_* can be the
        bitwise OR of callback types for which the function should be called,
        or zero for all types.
        
        Returns zero on success, CSOUND_ERROR if the specified function
        pointer or type mask is invalid, and CSOUND_MEMORY if there is not
        enough memory.
        
        The callback function takes the following arguments:
        
        *userData*
            the "user data" pointer, as specified when setting the callback
        *p*
            data pointer, depending on the callback type
        *type_*
            callback type, can be one of the following (more may be added in
            future versions of Csound):
            
            CSOUND_CALLBACK_KBD_EVENT

            CSOUND_CALLBACK_KBD_TEXT
                called by the :code:`sensekey` opcode to fetch key codes. The
                data pointer is a pointer to a single value of type :code:`int`,
                for returning the key code, which can be in the range 1 to
                65535, or 0 if there is no keyboard event.
                
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
        return libcsound.csoundRegisterKeyboardCallback(self.cs, KEYBOARDFUNC(function), ct.py_object(userData), ct.c_uint(type_))
    
    def removeKeyboardCallback(self, function):
        """Removes a callback previously set with :py:meth:`registerKeyboardCallback()`."""
        libcsound.csoundRemoveKeyboardCallback(self.cs, KEYBOARDFUNC(function))
    
    #Tables
    def tableLength(self, table):
        """Returns the length of a function table.
        
        (Not including the guard point).
        If the table does not exist, returns -1.
        """
        return libcsound.csoundTableLength(self.cs, table)
    
    def tableGet(self, table, index):
        """Returns the value of a slot in a function table.
        
        The *table* number and *index* are assumed to be valid.
        """
        return libcsound.csoundTableGet(self.cs, table, index)
    
    def tableSet(self, table, index, value):
        """Sets the value of a slot in a function table.
        
        The *table* number and *index* are assumed to be valid.
        """
        libcsound.csoundTableSet(self.cs, table, index, MYFLT(value))
    
    def tableCopyOut(self, table, dest):
        """Copies the contents of a function table into a supplied ndarray *dest*.
        
        The *table* number is assumed to be valid, and the destination needs to
        have sufficient space to receive all the function table contents.
        """
        ptr = dest.ctypes.data_as(ct.POINTER(MYFLT))
        libcsound.csoundTableCopyOut(self.cs, table, ptr)
    
    def tableCopyOutAsync(self, table, dest):
        """Asynchronous version of :py:meth:`tableCopyOut()`."""
        ptr = dest.ctypes.data_as(ct.POINTER(MYFLT))
        libcsound.csoundTableCopyOutAsync(self.cs, table, ptr)
    
    def tableCopyIn(self, table, src):
        """Copies the contents of an ndarray *src* into a given function *table*.
        
        The *table* number is assumed to be valid, and the table needs to
        have sufficient space to receive all the array contents.
        """
        ptr = src.ctypes.data_as(ct.POINTER(MYFLT))
        libcsound.csoundTableCopyIn(self.cs, table, ptr)
    
    def tableCopyInAsync(self, table, src):
        """Asynchronous version of :py:meth:`tableCopyIn()`."""
        ptr = src.ctypes.data_as(ct.POINTER(MYFLT))
        libcsound.csoundTableCopyInAsync(self.cs, table, ptr)
    
    def table(self, tableNum):
        """Returns a pointer to function table *tableNum* as an ndarray.
        
        The ndarray does not include the guard point. If the table does not
        exist, :code:`None` is returned.
        """
        ptr = ct.POINTER(MYFLT)()
        size = libcsound.csoundGetTable(self.cs, ct.byref(ptr), tableNum)
        if size < 0:
            return None
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = ct.cast(ptr, arrayType)
        return arrFromPointer(p)
        
    def tableArgs(self, tableNum):
        """Returns a pointer to the args used to generate a function table.
        
        The pointer is returned as an ndarray. If the table does not exist,
        :code:`None` is returned.
        
        NB: the argument list starts with the GEN number and is followed by
        its parameters. eg. f 1 0 1024 10 1 0.5  yields the list
        {10.0, 1.0, 0.5}
        """
        ptr = ct.POINTER(MYFLT)()
        size = libcsound.csoundGetTableArgs(self.cs, ct.byref(ptr), tableNum)
        if size < 0:
            return None
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = ct.cast(ptr, arrayType)
        return arrFromPointer(p)
    
    def isNamedGEN(self, num):
        """Checks if a given GEN number *num* is a named GEN.
        
        If so, it returns the string length. Otherwise it returns 0.
        """
        return libcsound.csoundIsNamedGEN(self.cs, num)
    
    def namedGEN(self, num, nameLen):
        """Gets the GEN name from a GEN number, if this is a named GEN.
        
        The final parameter is the max len of the string.
        """
        s = ct.create_string_buffer(nameLen)
        libcsound.csoundGetNamedGEN(self.cs, num, s, nameLen)
        return pstring(string_at(s, nameLen))
    
    #Function Table Display
    def setIsGraphable(self, isGraphable):
        """Tells Csound whether external graphic table display is supported.
        
        Return the previously set value (initially False).
        """
        ret = libcsound.csoundSetIsGraphable(self.cs, ct.c_int(isGraphable))
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
        """Finds the list of named gens."""
        lst = []
        ptr = libcsound.csoundGetNamedGens(self.cs)
        ptr = ct.cast(ptr, ct.POINTER(NamedGen))
        while (ptr):
            ng = ptr.contents
            lst.append((pstring(ng.name), int(ng.genum)))
            ptr = ng.next
        return lst
    
    def newOpcodeList(self):
        """Gets an alphabetically sorted list of all opcodes.
        
        Should be called after externals are loaded by :py:meth:`compile_()`.
        Returns a pointer to the list of OpcodeListEntry structures and the
        number of opcodes, or a negative error code on  failure.
        Make sure to call :py:meth:`disposeOpcodeList()` when done with the
        list.
        """
        opcodes = None
        ptr = ct.cast(ct.POINTER(ct.c_int)(), ct.POINTER(OpcodeListEntry))
        n = libcsound.csoundNewOpcodeList(self.cs, ct.byref(ptr))
        if n > 0:
            opcodes = ct.cast(ptr, ct.POINTER(OpcodeListEntry * n)).contents
        return opcodes, n
    
    def disposeOpcodeList(self, lst):
        """Releases an opcode list."""
        ptr = ct.cast(lst, ct.POINTER(OpcodeListEntry))
        libcsound.csoundDisposeOpcodeList(self.cs, ptr)

    def appendOpcode(self, opname, dsblksiz, flags, thread, outypes, intypes, iopfunc, kopfunc, aopfunc):
        """Appends an opcode implemented by external software.
        
        This opcode is added to Csound's internal opcode list.
        The opcode list is extended by one slot, and the parameters are copied
        into the new slot.
        
        Returns zero on success.
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
        
        This callback is used for checking system events, yielding cpu time
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
        """Creates and starts a new thread of execution.
        
        Returns an opaque pointer that represents the thread on success,
        or :code:`None` for failure.
        The *userdata* pointer is passed to the thread routine.
        """
        ret = libcsound.csoundCreateThread(THREADFUNC(function), ct.py_object(userdata))
        if (ret):
            return ret
        return None
    
    def createThread2(self, function, stack, userdata):
        """Creates and starts a new thread of execution with a user-defined
        stack size.
        
        Returns an opaque pointer that represents the thread on success,
        or :code:`None` for failure.
        The *userdata* pointer is passed to the thread routine.
        """
        ret = libcsound.csoundCreateThread2(THREADFUNC(function),
        	ct.c_uint(stack), ct.py_object(userdata))
        if (ret):
            return ret
        return None
    
    def currentThreadId(self):
        """Returns the ID of the currently executing thread, or :code:`None`
        for failure.
        
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
        """Waits until the indicated thread's routine has finished.
        
        Returns the value returned by the thread routine.
        """
        return libcsound.csoundJoinThread(thread)
    
    def createThreadLock(self):
        """Creates and returns a monitor object, or :code:`None` if not successful.
        
        The object is initially in signaled (notified) state.
        """
        ret = libcsound.csoundCreateThreadLock()
        if (ret):
            return ret
        return None
    
    def waitThreadLock(self, lock, milliseconds):
        """Waits on the indicated monitor object for the indicated period.
        
        The function returns either when the monitor object is notified,
        or when the period has elapsed, whichever is sooner; in the first case,
        zero is returned.
        
        If *milliseconds* is zero and the object is not notified, the function
        will return immediately with a non-zero status.
        """
        return libcsound.csoundWaitThreadLock(lock, ct.c_uint(milliseconds))
    
    def waitThreadLockNoTimeout(self, lock):
        """Waits on the indicated monitor object until it is notified.
        
        This function is similar to :py:meth:`waitThreadLock()` with an infinite
        wait time, but may be more efficient.
        """
        libcsound.csoundWaitThreadLockNoTimeout(lock)
    
    def notifyThreadLock(self, lock):
        """Notifies the indicated monitor object."""
        libcsound.csoundNotifyThreadLock(lock)
        
    def destroyThreadLock(self, lock):
        """Destroys the indicated monitor object."""
        libcsound.csoundDestroyThreadLock(lock)
        
    def createMutex(self, isRecursive):
        """Creates and returns a mutex object, or :code:`None` if not successful.
        
        Mutexes can be faster than the more general purpose monitor objects
        returned by :py:meth:`createThreadLock()` on some platforms, and can
        also be recursive, but the result of unlocking a mutex that is owned by
        another thread or is not locked is undefined.
        
        If *isRecursive'* id :code:`True`, the mutex can be re-locked multiple
        times by the same thread, requiring an equal number of unlock calls;
        otherwise, attempting to re-lock the mutex results in undefined
        behavior.
        
        Note: the handles returned by :py:meth:`createThreadLock()` and
        :py:meth:`createMutex()` are not compatible.
        """
        ret = libcsound.csoundCreateMutex(ct.c_int(isRecursive))
        if ret:
            return ret
        return None
    
    def lockMutex(self, mutex):
        """Acquires the indicated mutex object.
        
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
        """Releases the indicated mutex object.
        
        The mutex should be owned by the current thread, otherwise the
        operation of this function is undefined. A recursive mutex needs
        to be unlocked as many times as it was locked previously.
        """
        libcsound.csoundUnlockMutex(mutex)
    
    def destroyMutex(self, mutex):
        """Destroys the indicated mutex object.
        
        Destroying a mutex that is currently owned by a thread results
        in undefined behavior.
        """
        libcsound.csoundDestroyMutex(mutex)
    
    def createBarrier(self, max_):
        """Creates a Thread Barrier.
        
        Max value parameter should be equal to the number of child threads
        using the barrier plus one for the master thread.
        """
        ret = libcsound.csoundCreateBarrier(ct.c_uint(max_))
        if (ret):
            return ret
        return None
    
    def destroyBarrier(self, barrier):
        """Destroys a Thread Barrier."""
        return libcsound.csoundDestroyBarrier(barrier)
    
    def waitBarrier(self, barrier):
        """Waits on the thread barrier."""
        return libcsound.csoundWaitBarrier(barrier)

    #def createCondVar(self):
    #def condWait(self, condVar, mutex):
    #def condSignal(self, condVar):
    #def destroyCondVar(self, condVar):
    
    def sleep(self, milliseconds):
        """Waits for at least the specified number of *milliseconds*.
        
        It yields the CPU to other threads.
        """
        libcsound.csoundSleep(ct.c_uint(milliseconds))

    def spinLockInit(self, spinlock):
        """Inits the spinlock.
        
        If the spinlock is not locked, locks it and returns;
        if is is locked, waits until it is unlocked, then locks it and returns.
        Uses atomic compare and swap operations that are safe across processors
        and safe for out of order operations,
        and which are more efficient than operating system locks.
        
        Use spinlocks to protect access to shared data, especially in functions
        that do little more than read or write such data, for example::
        
            lock = ctypes.ct.c_int32(0)
            cs.spinLockInit(lock)
            def write(cs, frames, signal):
                cs.spinLock(lock)
                for frame in range(frames) :
                    global_buffer[frame] += signal[frame];
                cs.spinUnlock(lock)
        """
        return libcsound.csoundSpinLockInit(ct.byref(spinlock))

    def spinLock(self, spinlock):
        """Locks the spinlock."""
        libcsound.csoundSpinLock(ct.byref(spinlock))

    def spinTryLock(self,spinlock):
        """Tries the spinlock.
        
        returns CSOUND_SUCCESS if lock could be acquired,
        CSOUND_ERROR, otherwise.
        """
        return libcsound.csoundSpinLock(ct.byref(spinlock))

    def spinUnlock(self, spinlock):
        """Unlocks the spinlock."""
        libcsound.csoundSpinUnLock(ct.byref(spinlock))
    
    #Miscellaneous Functions
    def runCommand(self, args, noWait):
        """Runs an external command with the arguments specified in list *args*.
        
        args[0] is the name of the program to execute (if not a full path
        file name, it is searched in the directories defined by the PATH
        environment variable).
        
        If *noWait* is :code:`False`, the function waits until the external
        program finishes, otherwise it returns immediately. In the first case,
        a non-negative return value is the exit status of the command (0 to
        255), otherwise it is the PID of the newly created process.
        On error, a negative value is returned.
        """
        n = len(args)
        argv = (ct.POINTER(ct.c_char_p) * (n+1))()
        for i in range(n):
            v = cstring(args[i])
            argv[i] = ct.cast(ct.pointer(ct.create_string_buffer(v)), ct.POINTER(ct.c_char_p))
        argv[n] = None
        return libcsound.csoundRunCommand(ct.cast(argv, ct.POINTER(ct.c_char_p)), ct.c_int(noWait))
    
    def initTimerStruct(self, timerStruct):
        """Initializes a timer structure."""
        libcsound.csoundInitTimerStruct(ct.byref(timerStruct))
    
    def realTime(self, timerStruct):
        """Returns the elapsed real time (in seconds).
        
        The time is measured since the specified timer structure was initialised.
        """
        return libcsound.csoundGetRealTime(ct.byref(timerStruct))
    
    def CPUTime(self, timerStruct):
        """Returns the elapsed CPU time (in seconds).
        
        The time is measured since the specified timer structure was initialised.
        """
        return libcsound.csoundGetCPUTime(ct.byref(timerStruct))
    
    def randomSeedFromTime(self):
        """Returns a 32-bit unsigned integer to be used as seed from current time."""
        return libcsound.csoundGetRandomSeedFromTime()
    
    def setLanguage(self, lang_code):
        """Sets language to *lang_code*.
        
        *lang_code* can be for example CSLANGUAGE_ENGLISH_UK or
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
        """Gets the value of environment variable *name*.
        
        The searching order is: local environment of Csound (if
        *withCsoundInstance* is :code:`True`), variables set with
        :py:meth:`setGlobalEnv()`,
        and system environment variables. If *withCsoundInstance* is
        :code:`True`, should be called after :py:meth:`compile_()`.
        
        Return value is :code:`None` if the variable is not set.
        """
        if withCsoundInstance:
            ret = libcsound.csoundGetEnv(self.cs, cstring(name))
        else:
            ret = libcsound.csoundGetEnv(None, cstring(name))
        if (ret):
            return pstring(ret)
        return None

    def setGlobalEnv(self, name, value):
        """Sets the global value of environment variable *name* to *value*.
        
        The variable is deleted if *value* is :code:`None`. It is not safe to
        call this function while any Csound instances are active.
        
        Returns zero on success.
        """
        return libcsound.csoundSetGlobalEnv(cstring(name), cstring(value))
    
    def createGlobalVariable(self, name, nbytes):
        """Allocates *nbytes* bytes of memory.
        
        This memory can be accessed later by calling
        :py:meth`queryGlobalVariable()` with the specified name; the space is
        cleared to zero.
        
        Returns CSOUND_SUCCESS on success, CSOUND_ERROR in case of invalid
        parameters (zero *nbytes*, invalid or already used *name*), or
        CSOUND_MEMORY if there is not enough memory.
        """
        return libcsound.csoundCreateGlobalVariable(self.cs, cstring(name), ct.c_uint(nbytes))
    
    def queryGlobalVariable(self, name):
        """Gets pointer to space allocated with the name *name*.
        
        Return :code:`None` if the specified *name* is not defined.
        """
        ret = libcsound.csoundQueryGlobalVariable(self.cs, cstring(name))
        if (ret):
            return ret
        return None
    
    def queryGlobalVariableNoCheck(self, name):
        """This function is the similar to :py:meth`queryGlobalVariable()`.
        
        Except the variable is assumed to exist and no error checking is done.
        Faster, but may crash or return an invalid pointer if *name* is
        not defined.
        """
        return libcsound.csoundQueryGlobalVariableNoCheck(self.cs, cstring(name))
    
    def destroyGlobalVariable(self, name):
        """Frees memory allocated for *name* and remove *name* from the database.
        
        Return value is CSOUND_SUCCESS on success, or CSOUND_ERROR if the *name*
        is not defined.
        """
        return libcsound.csoundDestroyGlobalVariable(self.cs, cstring(name))
    
    def runUtility(self, name, args):
        """Runs utility with the specified *name* and command line arguments.
        
        Should be called after loading utility plugins.
        Use :py:meth`reset()` to clean up after calling this function.
        Returns zero if the utility was run successfully.
        """
        argc, argv = csoundArgList(args)
        return libcsound.csoundRunUtility(self.cs, cstring(name), argc, argv)
    
    def listUtilities(self):
        """Returns a list of registered utility names.
        
        The return value may be :code:`None` in case of an error.
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
        """Gets utility description.
        
        Returns :code:`None` if the utility was not found, or it has no
        description, or an error occured.
        """
        ptr = libcsound.csoundGetUtilityDescription(self.cs, cstring(name))
        if (ptr):
            return pstring(ptr)
        return None
    
    def rand31(self, seed):
        """Simple linear congruential random number generator::
        
            seed = seed * 742938285 % 2147483647
        
        The initial value of *seed* must be in the range 1 to 2147483646.
        Returns the next number from the pseudo-random sequence, in the range
        1 to 2147483646.
        """
        n = ct.c_int(seed)
        return libcsound.csoundRand31(ct.byref(n))
    
    def seedRandMT(self, initKey):
        """Initializes Mersenne Twister (MT19937) random number generator.
        
        *initKey* can be a single int, a list of int, or an ndarray of int.
        Those int values are converted to unsigned 32 bit values and used for
        seeding.
        
        Returns a CsoundRandMTState stuct to be used by :py:meth`csoundRandMT()`.
        """
        state = CsoundRandMTState()
        if type(initKey) == int:
            if initKey < 0:
                initKey = -initKey
            libcsound.csoundSeedRandMT(ct.byref(state), None, ct.c_uint32(initKey))
        elif type(initKey) == list or type(initKey) == np.ndarray:
            n = len(initKey)
            lst = (ct.c_uint32 * n)()
            for i in range(n):
                k = initKey[i]
                if k < 0 :
                    k = -k
                lst[i] = ct.c_uint32(k)
            p = ct.pointer(lst)
            p = ct.cast(p, ct.POINTER(ct.c_uint32))
            libcsound.csoundSeedRandMT(ct.byref(state), p, ct.c_uint32(len(lst)))
        return state
    
    def randMT(self, state):
        """Returns next random number from MT19937 generator.
        
        The PRNG must be initialized first by calling :py:meth`csoundSeedRandMT()`.
        """
        return libcsound.csoundRandMT(ct.byref(state))
    
    def createCircularBuffer(self, numelem, elemsize):
        """Creates a circular buffer with *numelem* number of elements.
        
        The element's size is set from *elemsize*. It should be used like::
        
            rb = cs.createCircularBuffer(1024, cs.sizeOfMYFLT())
        """
        return libcsound.csoundCreateCircularBuffer(self.cs, numelem, elemsize)
    
    def readCircularBuffer(self, circularBuffer, out, items):
        """Reads from circular buffer.
        
        *circular_buffer*
            pointer to an existing circular buffer
        *out*
            preallocated ndarray with at least items number of elements,
            where buffer contents will be read into
        *items*
            number of samples to be read
        
        Returns the actual number of items read (0 <= n <= items).
        """
        if len(out) < items:
            return 0
        ptr = out.ctypes.data_as(ct.c_void_p)
        return libcsound.csoundReadCircularBuffer(self.cs, circularBuffer, ptr, items)
    
    def peekCircularBuffer(self, circularBuffer, out, items):
        """Reads from circular buffer without removing them from the buffer.
        
        *circular_buffer*
            pointer to an existing circular buffer
        *out*
            preallocated ndarray with at least items number of elements,
            where buffer contents will be read into
        *items*
            number of samples to be read
        
        Returns the actual number of items read (0 <= n <= items).
        """
        if len(out) < items:
            return 0
        ptr = out.ctypes.data_as(ct.c_void_p)
        return libcsound.csoundPeekCircularBuffer(self.cs, circularBuffer, ptr, items)
    
    def writeCircularBuffer(self, circularBuffer, in_, items):
        """Writes to circular buffer.
        
        *circular_buffer*
            pointer to an existing circular buffer
        *in_*
            ndarray with at least items number of elements to be written
            into circular buffer
        *items*
            number of samples to write
            
        Returns the actual number of items written (0 <= n <= items).
        """
        if len(in_) < items:
            return 0
        ptr = in_.ctypes.data_as(ct.c_void_p)
        return libcsound.csoundWriteCircularBuffer(self.cs, circularBuffer, ptr, items)
    
    def flushCircularBuffer(self, circularBuffer):
        """Empties circular buffer of any remaining data.
        
        This function should only be used if there is no reader actively
        getting data from the buffer.
        
        *circular_buffer*
            pointer to an existing circular buffer
        """
        libcsound.csoundFlushCircularBuffer(self.cs, circularBuffer)
    
    def destroyCircularBuffer(self, circularBuffer):
        """Frees circular buffer."""
        libcsound.csoundDestroyCircularBuffer(self.cs, circularBuffer)

    def openLibrary(self, libraryPath):
        """Platform-independent function to load a shared library."""
        ptr = ct.POINTER(ct.c_int)()
        library = ct.cast(ptr, ct.c_void_p)
        ret = libcsound.csoundOpenLibrary(ct.byref(library), cstring(libraryPath))
        return ret, library
    
    def closeLibrary(self, library):
        """Platform-independent function to unload a shared library."""
        return libcsound.csoundCloseLibrary(library)
    
    def getLibrarySymbol(self, library, symbolName):
        """Platform-independent function to get a symbol address in a shared library."""
        return libcsound.csoundGetLibrarySymbol(library, cstring(symbolName))

# VL 20.09.21 now csPerfThread is part of CsoundLib
libcspt = libcsound
libcspt.NewCsoundPT.restype = ct.c_void_p
libcspt.NewCsoundPT.argtypes = [ct.c_void_p]
libcspt.DeleteCsoundPT.argtypes = [ct.c_void_p]
libcspt.CsoundPTisRunning.argtypes = [ct.c_void_p]
PROCESSFUNC = ct.CFUNCTYPE(None, ct.c_void_p)
libcspt.CsoundPTgetProcessCB.restype = ct.c_void_p
libcspt.CsoundPTgetProcessCB.argtypes = [ct.c_void_p]
libcspt.CsoundPTsetProcessCB.argtypes = [ct.c_void_p, PROCESSFUNC, ct.c_void_p]
libcspt.CsoundPTgetCsound.restype = ct.c_void_p
libcspt.CsoundPTgetCsound.argtypes = [ct.c_void_p]
libcspt.CsoundPTgetStatus.argtypes = [ct.c_void_p]
libcspt.CsoundPTplay.argtypes = [ct.c_void_p]
libcspt.CsoundPTpause.argtypes = [ct.c_void_p]
libcspt.CsoundPTtogglePause.argtypes = [ct.c_void_p]
libcspt.CsoundPTstop.argtypes = [ct.c_void_p]
libcspt.CsoundPTrecord.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_int, ct.c_int]
libcspt.CsoundPTstopRecord.argtypes = [ct.c_void_p]
libcspt.CsoundPTscoreEvent.argtypes = [ct.c_void_p, ct.c_int, ct.c_char, ct.c_int, ct.POINTER(MYFLT)]
libcspt.CsoundPTinputMessage.argtypes = [ct.c_void_p, ct.c_char_p]
libcspt.CsoundPTsetScoreOffsetSeconds.argtypes = [ct.c_void_p, ct.c_double]
libcspt.CsoundPTjoin.argtypes = [ct.c_void_p]
libcspt.CsoundPTflushMessageQueue.argtypes = [ct.c_void_p]


class CsoundPerformanceThread:
    """Performs a score in a separate thread until the end of score is reached.
    
    The playback (which is paused by default) is stopped by calling
    :py:meth:`stop()`, or if an error occurs.
    The constructor takes a Csound instance pointer as argument; it assumes
    that :py:meth:`ctcsound.compile_()` was called successfully before creating
    the performance thread. Once the playback is stopped for one of the above
    mentioned reasons, the performance thread calls :py:meth:`ctcsound.cleanup()`
    and returns.
    """
    def __init__(self, csp):
        self.cpt = libcspt.NewCsoundPT(csp)
    
    def __del__(self):
        libcspt.DeleteCsoundPT(self.cpt)
    
    def isRunning(self):
        """Returns :code:`True` if the performance thread is running, :code:`False` otherwise."""
        return libcspt.CsoundPTisRunning(self.cpt) != 0
    
    def processCB(self):
        """Returns the process callback."""
        return PROCESSFUNC(libcspt.CsoundPTgetProcessCB(self.cpt))
    
    def setProcessCB(self, function, data):
        """Sets the process callback."""
        libcspt.CsoundPTsetProcessCB(self.cpt, PROCESSFUNC(function), ct.byref(data))
    
    def csound(self):
        """Returns the Csound instance pointer."""
        return libcspt.CsoundPTgetCsound(self.cpt)
    
    def status(self):
        """Returns the current status.
        
        Zero if still playing, positive if the end of score was reached or
        performance was stopped, and negative if an error occured.
        """
        return libcspt.CsoundPTgetStatus(self.cpt)
    
    def play(self):
        """Continues performance if it was paused."""
        libcspt.CsoundPTplay(self.cpt)
    
    def pause(self):
        """Pauses performance (can be continued by calling :py:meth:`play()`)."""
        libcspt.CsoundPTpause(self.cpt)
    
    def togglePause(self):
        """Pauses or continues performance, depending on current state."""
        libcspt.CsoundPTtogglePause(self.cpt)
    
    def stop(self):
        """Stops performance (cannot be continued)."""
        libcspt.CsoundPTstop(self.cpt)
    
    def record(self, filename, samplebits, numbufs):
        """Starts recording the output from Csound.
        
        The sample rate and number of channels are taken directly from the
        running Csound instance.
        """
        libcspt.CsoundPTrecord(self.cpt, cstring(filename), samplebits, numbufs)
    
    def stopRecord(self):
        """Stops recording and closes audio file."""
        libcspt.CsoundPTstopRecord(self.cpt)
    
    def scoreEvent(self, absp2mode, opcod, pFields):
        """Sends a score event.
        
        The event has type *opcod* (e.g. 'i' for a note event).
        *pFields* is tuple, a list, or an ndarray of MYFLTs with all the pfields
        for this event, starting with the p1 value specified in *pFields[0]*.
        If *absp2mode* is non-zero, the start time of the event is measured
        from the beginning of performance, instead of the default of relative
        to the current time.
        """
        p = np.array(pFields).astype(MYFLT)
        ptr = p.ctypes.data_as(ct.POINTER(MYFLT))
        numFields = p.size
        libcspt.CsoundPTscoreEvent(self.cpt, ct.c_int(absp2mode), cchar(opcod), numFields, ptr)
    
    def inputMessage(self, s):
        """Sends a score event as a string, similarly to line events (-L)."""
        libcspt.CsoundPTinputMessage(self.cpt, cstring(s))
    
    def setScoreOffsetSeconds(self, timeVal):
        """Sets the playback time pointer to the specified value (in seconds)."""
        libcspt.CsoundPTsetScoreOffsetSeconds(self.cpt, ct.c_double(timeVal))
    
    def join(self):
        """Waits until the performance is finished or fails.
        
        Returns a positive value if the end of score was reached or
        :py:meth:`stop()` was called, and a negative value if an error occured.
        Also releases any resources associated with the performance thread
        object.
        """
        return libcspt.CsoundPTjoin(self.cpt)
    
    def flushMessageQueue(self):
        """Waits until all pending messages are actually received.
        
        (pause, send score event, etc.)
        """
        libcspt.CsoundPTflushMessageQueue(self.cpt)

