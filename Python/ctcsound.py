#
#   ctcsound.py: NEW API - Experimental
#   
#   Copyright (C) 2024 Francois Pinot
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
from enum import Enum

# This is a workaround to yield the PEP 3118 problem which appeared with
# numpy 1.15.0
if np.__version__ < '1.15':
    array_from_pointer = lambda p : np.ctypeslib.as_array(p)
elif np.__version__ < '1.16':
    sys.exit("ctcsound won't work with numpy 1.15.x. Please revert numpy" +
        " to an older version or update numpy to a version >= 1.16")
else:
    array_from_pointer = lambda p : p.contents

if sys.platform.startswith('linux'):
    libcsound = ct.CDLL("libcsound64.so")
elif sys.platform.startswith('win'):
    if sys.version_info.major <=3 and sys.version_info.minor < 8:
        libcsound = ct.cdll.csound64
    else:
        libcsound = ct.CDLL(ctypes.util.find_library("csound64"))
elif sys.platform.startswith('darwin'):
    libcsound = ct.CDLL(ctypes.util.find_library("CsoundLib64"))
    print("imported");
else:
    sys.exit("Don't know your system! Exiting...")

MYFLT = ct.c_double

# ERROR DEFINITIONS
CSOUND_SUCCESS = 0          # Completed successfully.
CSOUND_ERROR = -1           # Unspecified failure.
CSOUND_INITIALIZATION = -2  # Failed during initialization.
CSOUND_PERFORMANCE = -3     # Failed during performance.
CSOUND_MEMORY = -4          # Failed to allocate requested memory.
CSOUND_SIGNAL = -5          # Termination requested by SIGINT or SIGTERM.

#
# Device information
#
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

class OrcToken(ct.Structure):
    _fields_ = [("type", ct.c_int),
                ("lexeme", ct.c_char_p),
                ("value", ct.c_int),
                ("fvalue", ct.c_double),
                ("optype", ct.c_char_p),
                ("next", ct.c_void_p)]

class Tree(ct.Structure):
    _fields_ = [("type", ct.c_int),
                ("value", ct.POINTER(OrcToken)),
                ("rate", ct.c_int),
                ("len", ct.c_int),
                ("line", ct.c_int),
                ("locn", ct.c_uint64),
                ("left", ct.c_void_p),
                ("right", ct.c_void_p),
                ("next", ct.c_void_p),
                ("markup", ct.c_void_p)]

class StrDat(ct.Structure):
    _fields_ = [("data", ct.c_char_p),       # null-terminated string
                ("allocated", ct.c_size_t)]  # size of allocated data

class ArrayDat(ct.Structure):
    _fields_ = [("dimensions", ct.c_int),          # number of array dimensions
                ("sizes", ct.POINTER(ct.c_int32)), # size of each dimensions
                ("arrayMemberSize", ct.c_int),     # size of each item
                ("arrayType", ct.c_void_p),       # type of array
                ("data", ct.POINTER(MYFLT)),       # data
                ("allocated", ct.c_size_t)]        # size of allocated data

#
# Type definition for PVS data (pvs channels)
#
class PvsDatExt(ct.Structure):
    _fields_ = [("N", ct.c_int32),                 # transform size
                ("sliding", ct.c_int),             # sliding flag
                ("NB", ct.c_int32),
                ("overlap", ct.c_int32),           # analysis overlaps
                ("winsize", ct.c_int32),           # window size
                ("wintype", ct.c_int),             # window type: 0 = Hamming, 1 = Hann
                ("format", ct.c_int32),            # data format (see below)
                ("framecount", ct.c_uint32),       # frame counter
                ("frame", ct.POINTER(ct.c_float))] # data frame (see format)

#
# PVS DATA formats
#
PVS_AMP_FREQ = 0    # phase vocoder
PVS_AMP_PHASE = 1   # polar DFT
PVS_COMPLEX = 2     # rectangular DFT
PVS_TRACKS = 3      # amp, freq, phase, ID tracks

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

# Constants used by the bus interface (csoundGetChannelPtr() etc.).
CSOUND_CONTROL_CHANNEL = 1
CSOUND_AUDIO_CHANNEL  = 2
CSOUND_STRING_CHANNEL = 3
CSOUND_PVS_CHANNEL = 4
CSOUND_VAR_CHANNEL = 5
CSOUND_ARRAY_CHANNEL = 6

CSOUND_CHANNEL_TYPE_MASK = 15

CSOUND_INPUT_CHANNEL = 16
CSOUND_OUTPUT_CHANNEL = 32

CSOUND_CONTROL_CHANNEL_NO_HINTS  = 0
CSOUND_CONTROL_CHANNEL_INT  = 1
CSOUND_CONTROL_CHANNEL_LIN  = 2
CSOUND_CONTROL_CHANNEL_EXP  = 3

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
    _fields_ = [("windid", ct.POINTER(ct.c_uint)), # set by makeGraph()
                ("fdata", ct.POINTER(MYFLT)),      # data passed to drawGraph()
                ("npts", ct.c_int32),              # size of above array
                ("caption", ct.c_char * CAPSIZE),  # caption string for graph
                ("waitflg", ct.c_int16 ),          # set =1 to wait for ms after Draw
                ("polarity", ct.c_int16),          # controls positioning of X axis
                ("max", MYFLT),                    # workspace .. extrema this frame
                ("min", MYFLT),
                ("absmax", MYFLT),                 # workspace .. largest of above
                ("oabsmax", MYFLT),                # Y axis scaling factor
                ("danflag", ct.c_int),             # set to 1 for extra Yaxis mid span
                ("absflag", ct.c_int)]             # set to 1 to skip abs check

# Symbols for Windat.polarity field
NOPOL = 0
NEGPOL = 1
POSPOL = 2
BIPOL = 3


def cchar(s):
    if sys.version_info[0] >= 3:
        return ct.c_char(ord(s[0]))
    return ct.c_char(s[0])

libcsound.csoundCreate.restype = ct.c_void_p
libcsound.csoundCreate.argtypes = [ct.py_object, ct.c_char_p]
libcsound.csoundDestroy.argtypes = [ct.c_void_p]

libcsound.csoundGetSr.restype = MYFLT
libcsound.csoundGetSr.argtypes = [ct.c_void_p]
libcsound.csoundGetKr.restype = MYFLT
libcsound.csoundGetKr.argtypes = [ct.c_void_p]
libcsound.csoundGetKsmps.restype = ct.c_uint32
libcsound.csoundGetKsmps.argtypes = [ct.c_void_p]
libcsound.csoundGetChannels.restype = ct.c_uint32
libcsound.csoundGetChannels.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundGet0dBFS.restype = MYFLT
libcsound.csoundGet0dBFS.argtypes = [ct.c_void_p]
libcsound.csoundGetA4.restype = MYFLT
libcsound.csoundGetA4.argtypes = [ct.c_void_p]
libcsound.csoundGetCurrentTimeSamples.restype = ct.c_int64
libcsound.csoundGetCurrentTimeSamples.argtypes = [ct.c_void_p]
libcsound.csoundGetHostData.restype = ct.py_object
libcsound.csoundGetHostData.argtypes = [ct.c_void_p]
libcsound.csoundSetHostData.argtypes = [ct.c_void_p, ct.py_object]
libcsound.csoundGetEnv.restype = ct.c_char_p
libcsound.csoundGetEnv.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundSetGlobalEnv.argtypes = [ct.c_char_p, ct.c_char_p]
libcsound.csoundSetOption.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundSetParams.argtypes = [ct.c_void_p, ct.POINTER(CsoundParams)]
libcsound.csoundGetParams.argtypes = [ct.c_void_p, ct.POINTER(CsoundParams)]
libcsound.csoundGetDebug.argtypes = [ct.c_void_p]
libcsound.csoundSetDebug.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundSystemSr.restype = MYFLT
libcsound.csoundSystemSr.argtypes = [ct.c_void_p, MYFLT]
libcsound.csoundGetModule.argtypes = [ct.c_void_p, ct.c_int,
                                      ct.POINTER(ct.c_char_p), ct.POINTER(ct.c_char_p)]
libcsound.csoundGetAudioDevList.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_int]
libcsound.csoundGetMIDIDevList.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_int]
libcsound.csoundGetMessageLevel.argtypes = [ct.c_void_p]
libcsound.csoundSetMessageLevel.argtypes = [ct.c_void_p, ct.c_int]

libcsound.csoundParseOrc.restype = ct.POINTER(Tree)
libcsound.csoundParseOrc.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundCompileTree.argtypes = [ct.c_void_p, ct.POINTER(Tree), ct.c_int]
libcsound.csoundCompile.argtypes = [ct.c_void_p, ct.c_int, ct.POINTER(ct.c_char_p)]
libcsound.csoundDeleteTree.argtypes = [ct.c_void_p, ct.POINTER(Tree)]
libcsound.csoundCompileOrc.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_int]
libcsound.csoundEvalCode.restype = MYFLT
libcsound.csoundEvalCode.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundCompileCsd.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_int]
libcsound.csoundStart.argtypes = [ct.c_void_p]
libcsound.csoundPerformKsmps.argtypes = [ct.c_void_p]
libcsound.csoundRunUtility.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_int, ct.POINTER(ct.c_char_p)]
libcsound.csoundReset.argtypes = [ct.c_void_p]

libcsound.csoundSetHostAudioIO.argtypes = [ct.c_void_p]
libcsound.csoundSetRTAudioModule.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundGetSpin.restype = ct.POINTER(MYFLT)
libcsound.csoundGetSpin.argtypes = [ct.c_void_p]
libcsound.csoundGetSpout.restype = ct.POINTER(MYFLT)
libcsound.csoundGetSpout.argtypes = [ct.c_void_p]

libcsound.csoundSetHostMIDIIO.argtypes = [ct.c_void_p]
libcsound.csoundSetMIDIModule.argtypes = [ct.c_void_p, ct.c_char_p]
MIDIINOPENFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(ct.c_void_p), ct.c_char_p)
MIDIREADFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_void_p, ct.c_char_p, ct.c_int)
MIDIINCLOSEFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_void_p)
MIDIOUTOPENFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(ct.c_void_p), ct.c_char_p)
MIDIWRITEFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_void_p, ct.c_char_p, ct.c_int)
MIDIOUTCLOSEFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_void_p)
MIDIERRORFUNC = ct.CFUNCTYPE(ct.c_char_p, ct.c_int)
MIDIDEVLISTFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(CsoundMidiDevice), ct.c_int)
libcsound.csoundSetExternalMidiInOpenCallback.argtypes = [ct.c_void_p, MIDIINOPENFUNC]
libcsound.csoundSetExternalMidiReadCallback.argtypes = [ct.c_void_p, MIDIREADFUNC]
libcsound.csoundSetExternalMidiInCloseCallback.argtypes = [ct.c_void_p, MIDIINCLOSEFUNC]
libcsound.csoundSetExternalMidiOutOpenCallback.argtypes = [ct.c_void_p, MIDIOUTOPENFUNC]
libcsound.csoundSetExternalMidiWriteCallback.argtypes = [ct.c_void_p, MIDIWRITEFUNC]
libcsound.csoundSetExternalMidiOutCloseCallback.argtypes = [ct.c_void_p, MIDIOUTCLOSEFUNC]
libcsound.csoundSetExternalMidiErrorStringCallback.argtypes = [ct.c_void_p, MIDIERRORFUNC]
libcsound.csoundSetMIDIDeviceListCallback.argtypes = [ct.c_void_p, MIDIDEVLISTFUNC]

libcsound.csoundMessage.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundMessageS.argtypes = [ct.c_void_p, ct.c_int, ct.c_char_p, ct.c_char_p]
MSGSTRFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.c_int, ct.c_char_p)
libcsound.csoundSetMessageStringCallback.argtypes = [MSGSTRFUNC]
libcsound.csoundCreateMessageBuffer.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundGetFirstMessage.restype = ct.c_char_p
libcsound.csoundGetFirstMessage.argtypes = [ct.c_void_p]
libcsound.csoundGetFirstMessageAttr.argtypes = [ct.c_void_p]
libcsound.csoundPopFirstMessage.argtypes = [ct.c_void_p]
libcsound.csoundGetMessageCnt.argtypes = [ct.c_void_p]
libcsound.csoundDestroyMessageBuffer.argtypes = [ct.c_void_p]

libcsound.csoundGetChannelPtr.argtypes = [ct.c_void_p, ct.POINTER(ct.c_void_p),
                                          ct.c_char_p, ct.c_int]
libcsound.csoundListChannels.argtypes = [ct.c_void_p, ct.POINTER(ct.POINTER(ControlChannelInfo))]
libcsound.csoundDeleteChannelList.argtypes = [ct.c_void_p, ct.POINTER(ControlChannelInfo)]
libcsound.csoundSetControlChannelHints.argtypes = [ct.c_void_p, ct.c_char_p, ControlChannelHints]
libcsound.csoundGetControlChannelHints.argtypes = [ct.c_void_p, ct.c_char_p,
                                                   ct.POINTER(ControlChannelHints)]
libcsound.csoundGetChannelLock.restype = ct.POINTER(ct.c_int)
libcsound.csoundGetChannelLock.argtypes = [ct.c_void_p, ct.c_char_p]
libcsound.csoundGetControlChannel.restype = MYFLT
libcsound.csoundGetControlChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(ct.c_int)]
libcsound.csoundSetControlChannel.argtypes = [ct.c_void_p, ct.c_char_p, MYFLT]
libcsound.csoundGetAudioChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(MYFLT)]
libcsound.csoundSetAudioChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(MYFLT)]
libcsound.csoundGetStringChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundSetStringChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundGetArrayChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(ArrayDat)]
libcsound.csoundSetArrayChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(ArrayDat)]
libcsound.csoundGetPvsChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(PvsDatExt)]
libcsound.csoundSetPvsChannel.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(PvsDatExt)]
libcsound.csoundGetChannelDatasize.argtypes = [ct.c_void_p, ct.c_char_p]
CHANNELFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.c_char_p, ct.c_void_p, ct.c_void_p)
libcsound.csoundSetInputChannelCallback.argtypes = [ct.c_void_p, CHANNELFUNC]
libcsound.csoundSetOutputChannelCallback.argtypes = [ct.c_void_p, CHANNELFUNC]
libcsound.csoundEvent.argtypes = [ct.c_void_p, ]
libcsound.csoundEventString.argtypes = [ct.c_void_p, ct.c_char_p, ]
libcsound.csoundKeyPress.argtypes = [ct.c_void_p, ct.c_char]
KEYBOARDFUNC = ct.CFUNCTYPE(ct.c_int, ct.py_object, ct.c_void_p, ct.c_uint)
libcsound.csoundRegisterKeyboardCallback.argtypes = [ct.c_void_p, KEYBOARDFUNC, ct.py_object, ct.c_uint]
libcsound.csoundRemoveKeyboardCallback.argtypes = [ct.c_void_p, KEYBOARDFUNC]

libcsound.csoundTableLength.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundGetTable.argtypes = [ct.c_void_p, ct.POINTER(ct.POINTER(MYFLT)), ct.c_int]
libcsound.csoundGetTableArgs.argtypes = [ct.c_void_p, ct.POINTER(ct.POINTER(MYFLT)), ct.c_int]

libcsound.csoundGetScoreTime.restype = ct.c_double
libcsound.csoundGetScoreTime.argtypes = [ct.c_void_p]
libcsound.csoundIsScorePending.argtypes = [ct.c_void_p]
libcsound.csoundSetScorePending.argtypes = [ct.c_void_p, ct.c_int]
libcsound.csoundGetScoreOffsetSeconds.restype = MYFLT
libcsound.csoundGetScoreOffsetSeconds.argtypes = [ct.c_void_p]
libcsound.csoundSetScoreOffsetSeconds.argtypes = [ct.c_void_p, MYFLT]
libcsound.csoundRewindScore.argtypes = [ct.c_void_p]
libcsound.csoundSleep.argtypes = [ct.c_uint]

libcsound.csoundLoadPlugins.argtypes = [ct.c_void_p, ct.c_char_p]
OPCODEFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_void_p)
libcsound.csoundAppendOpcode.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_int,
    ct.c_int, ct.c_char_p, ct.c_char_p,
    OPCODEFUNC, OPCODEFUNC, OPCODEFUNC]

libcsound.csoundSetIsGraphable.argtypes = [ct.c_void_p, ct.c_int]
MAKEGRAPHFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.POINTER(Windat), ct.c_char_p)
libcsound.csoundSetMakeGraphCallback.argtypes = [ct.c_void_p, MAKEGRAPHFUNC]
DRAWGRAPHFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.POINTER(Windat))
libcsound.csoundSetDrawGraphCallback.argtypes = [ct.c_void_p, DRAWGRAPHFUNC]
KILLGRAPHFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.POINTER(Windat))
libcsound.csoundSetKillGraphCallback.argtypes = [ct.c_void_p, KILLGRAPHFUNC]
EXITGRAPHFUNC = ct.CFUNCTYPE(ct.c_int, ct.c_void_p)
libcsound.csoundSetExitGraphCallback.argtypes = [ct.c_void_p, EXITGRAPHFUNC]

libcsound.csoundCreateCircularBuffer.restype = ct.c_void_p
libcsound.csoundCreateCircularBuffer.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]
libcsound.csoundReadCircularBuffer.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_void_p, ct.c_int]
libcsound.csoundPeekCircularBuffer.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_void_p, ct.c_int]
libcsound.csoundWriteCircularBuffer.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_void_p, ct.c_int]
libcsound.csoundFlushCircularBuffer.argtypes = [ct.c_void_p, ct.c_void_p]
libcsound.csoundDestroyCircularBuffer.argtypes = [ct.c_void_p, ct.c_void_p]


def cstring(s):
    if sys.version_info[0] >= 3 and s != None:
        return bytes(s, 'utf-8')
    return s

def pstring(s):
    if sys.version_info[0] >= 3 and s != None:
        return str(s, 'utf-8')
    return s

def csound_arg_list(lst):
    if len(lst) == 1 and type(lst[0]) is list:
        lst = lst[0]
    argc = len(lst)
    argv = (ct.POINTER(ct.c_char_p) * argc)()
    for i in range(argc):
        v = cstring(lst[i])
        argv[i] = ct.cast(ct.pointer(ct.create_string_buffer(v)),
                          ct.POINTER(ct.c_char_p))
    return ct.c_int(argc), ct.cast(argv, ct.POINTER(ct.c_char_p))


#Instantiation
def csound_initialize(flags):
    """Initializes Csound library with specific flags.
    
    This function is called internally by csound_create(), so there is generally
    no need to use it explicitly unless you need to avoid default initialization
    that sets signal handlers and atexit() callbacks.
    Return value is zero on success, positive if initialization was
    done already, and negative on error.
    """
    return libcsound.csoundInitialize(flags)


class Csound:
    #
    # Instantiation
    #
    def __init__(self, host_data=None, opcode_dir=None):
        """Creates an instance of Csound.
       
        Returns an opaque pointer that must be passed to most Csound API
        functions. The host_data parameter can be None, or it can be
        any sort of data; these data can be accessed from the Csound instance
        that is passed to callback routines.
        If not None the opcode_dir parameter sets an override for
        the plugin module/opcode directory search.
        """
        self.cs = libcsound.csoundCreate(ct.py_object(host_data),
                                         cstring(opcode_dir))

    def __del__(self):
        """Destroys an instance of Csound."""
        if libcsound:
            libcsound.csoundDestroy(self.cs)
    
    def csound(self):
        """Returns the opaque pointer to the underlying CSOUND struct.
        
        This pointer is needed to instantiate a CsoundPerformanceThread object.
        """
        return self.cs

    #
    # Attributes
    #
    def version(self):
        """Returns the version number times 1000 (5.00.0 = 5000)."""
        return libcsound.csoundGetVersion()
    
    def API_version(self):
        """Returns the API version number times 100 (1.00 = 100)."""
        return libcsound.csoundGetAPIVersion()

    def sr(self):
        """Returns the number of audio sample frames per second."""
        return libcsound.csoundGetSr(self.cs)
    
    def kr(self):
        """Returns the number of control samples per second."""
        return libcsound.csoundGetKr(self.cs)
    
    def ksmps(self):
        """Returns the number of audio sample frames per control sample."""
        return libcsound.csoundGetKsmps(self.cs)

    def channels(self, is_input=False):
        """Returns the number of audio channels in the Csound instance.

        If is_input = False, the value of nchnls is returned,
        otherwise nchnls_i.
        """
        return libcsound.csoundGetChannels(self.cs, ct.c_int(is_input))

    def get_0dBFS(self):
        """Returns the 0dBFS level of the spin/spout buffers."""
        return libcsound.csoundGet0dBFS(self.cs)
    
    def A4(self):
        """Returns the A4 frequency reference."""
        return libcsound.csoundGetA4(self.cs)
    
    def current_time_samples(self):
        """Returns the current performance time in samples."""
        return libcsound.csoundGetCurrentTimeSamples(self.cs)
    
    def size_of_MYFLT(self):
        """Returns the size of MYFLT in bytes."""
        return libcsound.csoundGetSizeOfMYFLT()
    
    def host_data(self):
        """Returns host data."""
        return libcsound.csoundGetHostData(self.cs)
    
    def set_host_data(self, data):
        """Sets host data."""
        libcsound.csoundSetHostData(self.cs, ct.py_object(data))

    def env(self, name):
        """Gets the value of environment variable name.
        
        The searching order is: local environment of Csound, variables set with
        set_global_env(), and system environment variables.
        Return value is None if the variable is not set.
        """
        ret = libcsound.csoundGetEnv(self.cs, cstring(name))
        if (ret):
            return pstring(ret)
        return None

    def set_global_env(self, name, value):
        """Set the global value of environment variable name to value.
        
        The variable is deleted if value is None.
        It is not safe to call this function while any Csound instances
        are active.
        Returns zero on success..
        """
        return libcsound.csoundSetGlobalEnv(cstring(name), cstring(value))
        
    def set_option(self, option):
        """Sets a single csound option (flag).
        
        Returns CSOUND_SUCCESS on success.
        This needs to be called before any code is compiled.
        NB: blank spaces are not allowed.
        """
        return libcsound.csoundSetOption(self.cs, cstring(option))
    
    def set_params(self, params):
        """Configures Csound with a given set of parameters.
        
        These parameters are defined in the CsoundParams structure.
        They are the part of the OPARMS struct that are configurable through
        command line flags.
        The CsoundParams structure can be obtained using params().
        These options should only be changed before performance has started.
        """
        libcsound.csoundSetParams(self.cs, ct.byref(params))
    
    def params(self, params):
        """Gets the current set of parameters from a CSOUND instance.
        
        These parameters are in a CsoundParams structure. See set_params():
        
            p = ctcsound.CsoundParams()
            cs.params(p)
        """
        libcsound.csoundGetParams(self.cs, ct.byref(params))

    def debug(self):
        """Returns whether Csound is set to print debug messages.
        
        Those messages are sent through the DebugMsg() internal API
        function.
        """
        return libcsound.csoundGetDebug(self.cs) != 0

    def set_debug(self, debug):
        """Sets whether Csound prints debug messages.
        
        The debug argument must have value `True` or `False`.
        Those messages come from the DebugMsg() internal API function.
        """
        libcsound.csoundSetDebug(self.cs, ct.c_int(debug))

    def system_sr(self, val):
        """If val > 0, sets the internal variable holding the system HW sr.
        
        Returns the stored value containing the system HW sr.
        """
        return libcsound.csoundSystemSr(self.cs, val)

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

    def audio_dev_list(self, is_output):
        """Returns a list of available input or output audio devices.
        
        Each item in the list is a dictionnary representing a device. The
        dictionnary keys are device_name, device_id, rt_module (value
        type string), max_nchnls (value type int), and isOutput (value 
        type boolean).
        
        Must be called after an orchestra has been compiled
        to get meaningful information.
        """
        n = libcsound.csoundGetAudioDevList(self.cs, None, ct.c_int(is_output))
        devs = (CsoundAudioDevice * n)()
        libcsound.csoundGetAudioDevList(self.cs, ct.byref(devs), ct.c_int(is_output))
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

    def midi_dev_list(self, is_output):
        """Returns a list of available input or output midi devices.
        
        Each item in the list is a dictionnary representing a device. The
        dictionnary keys are device_name, interface_name, device_id,
        midi_module (value type string), isOutput (value type boolean).
        
        Must be called after an orchestra has been compiled
        to get meaningful information.
        """
        n = libcsound.csoundGetMIDIDevList(self.cs, None, ct.c_int(is_output))
        devs = (CsoundMidiDevice * n)()
        libcsound.csoundGetMIDIDevList(self.cs, ct.byref(devs), ct.c_int(is_output))
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

    def message_level(self):
        """Returns the Csound message level (from 0 to 231)."""
        return libcsound.csoundGetMessageLevel(self.cs)
    
    def set_message_level(self, message_level):
        """Sets the Csound message level (from 0 to 231)."""
        libcsound.csoundSetMessageLevel(self.cs, ct.c_int(message_level))

    #
    # Performance
    #
    def parse_orc(self, orc):
        """Parses the given orchestra from an ASCII string into a TREE.
        
        This can be called during performance to parse new code.
        """
        return libcsound.csoundParseOrc(self.cs, cstring(orc))

    def compile_tree(self, tree, async_mode=False):
        """Compiles the given TREE node into structs for Csound.
        
        These structs are to be used in synchronous  or asynchronous
        (async_mode = True) mode.
        """
        return libcsound.csoundCompileTree(self.cs, ct.byref(tree), ct.c_int(async_mode))

    def compile_(self, *args):
        """Compiles Csound input files (such as an orchestra and score, or CSD).
        
        As directed by the supplied command-line arguments,
        but does not perform them. Returns a non-zero error code on failure.
        In this mode, the sequence of calls should be as follows::
        
            cs.compile_(args)
            while cs.perform_ksmps() == 0:
                pass
            cs.reset()
        """
        argc, argv = csound_arg_list(args)
        return libcsound.csoundCompile(self.cs, argc, argv)

    def delete_tree(self, tree):
        """Frees the resources associated with the TREE tree.
        
        This function should be called whenever the TREE was
        created with parse_orc and memory can be deallocated.
        """
        libcsound.csoundDeleteTree(self.cs, ct.byref(tree))

    def compile_orc(self, orc, async_mode=False):
        """Parses, and compiles the given orchestra from an ASCII string.
        
        Also evaluating any global space code (i-time only)
        in synchronous or asynchronous -aync_mode = False) mode.
        This can be called during performance to compile a new orchestra::
        
            orc = 'instr 1 \n a1 rand 0dbfs/4 \n out a1 \n endin \n'
            cs.compile_orc(orc, False)
        """
        return libcsound.csoundCompileOrc(self.cs, cstring(orc), ct.c_int(async_mode))

    def eval_code(self, code):
        """Parses and compiles an orchestra given on an string.
        
        Evaluating any global space code (i-time only).
        On SUCCESS it returns a value passed to the
        'return' opcode in global space:
        
            code = 'i1 = 2 + 2 \n return i1 \n'
            retval = cs.eval_code(code)
        """
        return libcsound.csoundEvalCode(self.cs, cstring(code))

    def compile_csd(self, csd, mode):
        """Compiles a Csound input file (.csd file) or a text string.
        
        Returns a non-zero error code on failure.

        If start is called before compile_csd, the <CsOptions>
        element is ignored (but set_option can be called any number of
        times), the <CsScore> element is not pre-processed, but dispatched as
        real-time events; and performance continues indefinitely, or until
        ended by calling stop or some other logic. In this "real-time"
        mode, the sequence of calls should be:

            cs.set_option("--an_option")
            cs.set_option("--another_option")
            cs.start()
            cs.compile_csd(csd_filename, 0)
            while True:
               cs.perform_ksmps()
               # Something to break out of the loop
               # when finished here...
            cs.reset()

        NB: this function can be called repeatedly during performance to
        replace or add new instruments and events.

        But if compile_csd is called before start, the <CsOptions>
        element is used, the <CsScore> section is pre-processed and dispatched
        normally, and performance terminates when the score terminates, or
        stop is called. In this "non-real-time" mode (which can still
        output real-time audio and handle real-time events), the sequence of
        calls should be:

            cs.compile_csd(csd_filename, 0)
            cs.start()
            while True:
                finished = cs.perform_ksmps()
                if finished:
                    break
            cs.reset()

        if mode = 1, csd contains a full CSD code (rather than a filename).
        This is convenient when it is desirable to package the csd as part of
        an application or a multi-language piece.
        """
        return libcsound.csoundCompileCsd(self.cs, cstring(csd), ct.c_int(mode))

    def start(self):
        """Prepares Csound for performance.
        
        Normally called after compiling a csd file or an orc file, in which
        case score preprocessing is performed and performance terminates
        when the score terminates.
        
        However, if called before compiling a csd file or an orc file, 
        score preprocessing is not performed and "i" statements are dispatched 
        as real-time events, the <CsOptions> tag is ignored, and performance 
        continues indefinitely or until ended using the API.
        """
        return libcsound.csoundStart(self.cs)

    def perform_ksmps(self):
        """Senses input events, and performs audio output.
        
        This is done for one control sample worth (ksmps).
        start() must be called first.
        Returns False during performance, and True when
        performance is finished. If called until it returns True,
        it will perform an entire score.
        Enables external software to control the execution of Csound,
        and to synchronize performance with audio input and output.
        """
        return libcsound.csoundPerformKsmps(self.cs)

    def run_utility(self, name, args):
        """Runs utility with the specified name and command line arguments.
        
        Should be called after loading utility plugins.
        Use reset() to clean up after calling this function.
        Returns zero if the utility was run successfully.
        """
        argc, argv = csound_arg_list(args)
        return libcsound.csoundRunUtility(self.cs, cstring(name), argc, argv)

    def reset(self):
        """Resets all internal memory and state.
        
        In preparation for a new performance.
        Enable external software to run successive Csound performances
        without reloading Csound.
        """
        libcsound.csoundReset(self.cs)

    #
    # Realtime Audio I/O
    #
    def set_host_audio_IO(self):
        """Disable all default handling of sound I/O.
        
        Calling this function after the creation of a Csound object
        and before the start of performance will disable all default
        handling of sound I/O by the Csound library via its audio
        backend module.
        Host application should in this case use the spin/spout
        buffers directly.
        """        
        libcsound.csoundSetHostAudioIO(self.cs)

    def set_RT_audio_module(self, module):
        """Sets the current RT audio module."""
        libcsound.csoundSetRTAudioModule(self.cs, cstring(module))

    def spin(self):
        """Returns the Csound audio input working buffer (spin) as an ndarray.
        
        Enables external software to write audio into Csound before
        calling perform_ksmps().
        """
        buf = libcsound.csoundGetSpin(self.cs)
        size = self.ksmps() * self.channels(is_input=True)
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = ct.cast(buf, arrayType)
        return array_from_pointer(p)

    def spout(self):
        """Returns the address of the Csound audio output working buffer (spout).
        
        Enables external software to read audio from Csound after
        calling perform_ksmps().
        """
        buf = libcsound.csoundGetSpout(self.cs)
        size = self.ksmps() * self.channels()
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = ct.cast(buf, arrayType)
        return array_from_pointer(p)

    #
    # Realtime MIDI I/O
    #
    def set_host_midi_IO(self):
        """Disable all default handling of MIDI I/O.
        
        Call this function after csound_create() 
        and before the start of performance to implement
        MIDI via the callbacks below.
        """
        libcsound.csoundSetHostMIDIIO(self.cs)

    def set_midi_module(self, module):
        """Sets the current MIDI IO module."""
        libcsound.csoundSetMIDIModule(self.cs, cstring(module))

    def set_external_midi_in_open_callback(self, function):
        """Sets a callback for opening real-time MIDI input."""
        self.ext_midi_in_open_cb_ref = MIDIINOPENFUNC(function)
        libcsound.csoundSetExternalMidiInOpenCallback(self.cs,
            self.ext_midi_in_open_cb_ref)

    def set_external_midi_read_callback(self, function):
        """Sets a callback for reading from real time MIDI input."""
        self.ext_midi_read_cb_ref = MIDIREADFUNC(function)
        libcsound.csoundSetExternalMidiReadCallback(self.cs,
            self.ext_midi_read_cb_ref)
    
    def set_external_midi_in_close_callback(self, function):                
        """Sets a callback for closing real time MIDI input."""
        self.ext_midi_in_close_cb_ref = MIDIINCLOSEFUNC(function)
        libcsound.csoundSetExternalMidiInCloseCallback(self.cs,
            self.ext_midi_in_close_cb_ref)
    
    def set_external_midi_out_open_callback(self, function):
        """Sets a callback for opening real-time MIDI input."""
        self.ext_midi_out_open_cb_ref = MIDIOUTOPENFUNC(function)
        libcsound.csoundSetExternalMidiOutOpenCallback(self.cs,
            self.ext_midi_out_open_cb_ref)

    def set_external_midi_write_callbackk(self, function):
        """Sets a callback for reading from real time MIDI input."""
        self.ext_midi_write_cb_ref = MIDIWRITEFUNC(function)
        libcsound.csoundSetExternalMidiWriteCallback(self.cs,
            self.ext_midi_write_cb_ref)
    
    def set_external_midi_out_close_callback(self, function):
        """Sets a callback for closing real time MIDI input."""
        self.ext_midi_out_close_cb_ref = MIDIOUTCLOSEFUNC(function)
        libcsound.csoundSetExternalMidiOutCloseCallback(self.cs,
            self.ext_midi_out_close_cb_ref)

    def set_external_midi_error_string_callback(self, function):
        """ Sets a callback for converting MIDI error codes to strings."""
        self.ext_midi_err_str_cb_ref = MIDIERRORFUNC(function)
        libcsound.csoundSetExternalMidiErrorStringCallback(self.cs,
            self.ext_midi_err_str_cb_ref)
    
    def set_midi_device_list_callback(self, function):
        """Sets a callback for obtaining a list of MIDI devices."""
        self.midi_dev_list_cb_ref = MIDIDEVLISTFUNC(function)
        libcsound.csoundSetMIDIDeviceListCallback(self.cs,
            self.midi_dev_list_cb_ref)

    #
    # Csound Messages and Text
    #
    def message(self, fmt, *args):
        """Displays an informational message.
        
        This is a workaround because does not support variadic functions.
        The arguments are formatted in a string, using the python way, either
        old style or new style, and then this formatted string is passed to
        the Csound display message system.
        """
        if fmt[0] == '{':
            s = fmt.format(*args)
        else:
            s = fmt % args
        libcsound.csoundMessage(self.cs, cstring("%s"), cstring(s))
    
    def message_S(self, attr, fmt, *args):
        """Prints message with special attributes.
        
        (See msg_attr.h for the list of available attributes). With attr=0,
        message_S() is identical to message().
        This is a workaround because ctypes does not support variadic functions.
        The arguments are formatted in a string, using the python way, either
        old style or new style, and then this formatted string is passed to
        the csound display message system.
        """
        if fmt[0] == '{':
            s = fmt.format(*args)
        else:
            s = fmt % args
        libcsound.csoundMessageS(self.cs, ct.c_int(attr), cstring("%s"), cstring(s))

    def set_message_string_callback(self, attr, function):
        """Sets an alternative message print function.

        This function is to be called by Csound to print an
        informational message, using a less granular signature.
        This callback can be set for --realtime mode.
        This callback is cleared after reset.
        """
        self.message_string_cb = MSGSTRFUNC(function)
        libcsound.csoundSetMessageStringCallback(self.cs, ct.c_int(attr),
            self.message_string_cb)                                                      

    def create_message_buffer(self, to_stdout):
        """Creates a buffer for storing messages printed by Csound.
        
        Should be called after creating a Csound instance and the buffer
        can be freed by calling destroyMessageBuffer() before
        deleting the Csound instance. You will generally want to call
        cleanup() to make sure the last messages are flushed to
        the message buffer before destroying Csound.
        
        If to_stdout* is True, the messages are also printed to
        stdout and stderr (depending on the type of the message),
        in addition to being stored in the buffer.
        
        Using the message buffer ties up the internal message callback, so
        set_message_callback() should not be called after creating the
        message buffer.
        """
        libcsound.csoundCreateMessageBuffer(self.cs, ct.c_int(to_stdout))

    def first_message(self):
        """Returns the first message from the buffer."""
        s = libcsound.csoundGetFirstMessage(self.cs)
        return pstring(s)

    def first_message_attr(self):
        """Returns the attribute parameter of the first message in the buffer."""
        return libcsound.csoundGetFirstMessageAttr(self.cs)

    def pop_first_message(self):
        """Removes the first message from the buffer."""
        libcsound.csoundPopFirstMessage(self.cs)

    def message_cnt(self):
        """Returns the number of pending messages in the buffer."""
        return libcsound.csoundGetMessageCnt(self.cs)

    def destroy_message_buffer(self):
        """Releases all memory used by the message buffer."""
        libcsound.csoundDestroyMessageBuffer(self.cs)

    #
    # Channels, Controls and Events
    #
    def channel_ptr(self, name, type_):
        """Get a pointer to the specified channel and an error message.
        
        The channel is created first if it does not exist yet.
        type_ must be the bitwise OR of exactly one of the following values,
        
        CSOUND_CONTROL_CHANNEL
            control data (one MYFLT value) - (MYFLT **) pp
        CSOUND_AUDIO_CHANNEL
            audio data (ksmps() MYFLT values) - (MYFLT **) pp
        CSOUND_STRING_CHANNEL
            string data as a STRDAT structure - (STRDAT **) pp
        CSOUND_ARRAY_CHANNEL
            array data as an ARRAYDAT structure - (ARRAYDAT **) pp
        CSOUND_PVS_CHANNEL
            pvs data as a PVSDATEXT structure - (PVSDATEXT **) pp
        
        and at least one of these:
        
        CSOUND_INPUT_CHANNEL
        CSOUND_OUTPUT_CHANNEL
        
        If the channel is a control or an audio channel, the pointer is
        translated to an ndarray of MYFLT. If the channel is a string channel,
        the pointer is casted to ct.c_char_p. The error message is either
        an empty string or a string describing the error that occured.
        
        If the channel already exists, it must match the data type
        (control, audio, or string), however, the input/output bits are
        OR'd with the new value. Note that audio and string channels
        can only be created after calling compile_(), because the
        storage size is not known until then.

        Return value is zero on success, or a negative error code,
        
        CSOUND_MEMORY
            there is not enough memory for allocating the channel
        CSOUND_ERROR
            the specified name or type is invalid
            
        or, if a channel with the same name but incompatible type
        already exists, the type of the existing channel. In the case
        of any non-zero return value, the pointer is set to NULL.
        Note: to find out the type of a channel without actually
        creating or changing it, set type_ to zero, so that the return
        value will be either the type of the channel, or CSOUND_ERROR
        if it does not exist.
        
        Operations on the pointer are not thread-safe by default. The host is
        required to take care of threadsafety by
        1) retrieving the channel lock
        with :py:meth:`channelLock()` and using :py:meth:`spinLock()` and
        :py:meth:`spinUnLock()` to protect access to the pointer.
        
        See Top/threadsafe.c in the Csound library sources for
        examples. Optionally, use the channel get/set functions
        provided below, which are threadsafe by default.
        """
        length = 0
        chan_type = type_ & CSOUND_CHANNEL_TYPE_MASK
        if chan_type == CSOUND_CONTROL_CHANNEL:
            length = 1
        elif chan_type == CSOUND_AUDIO_CHANNEL:
            length = libcsound.csoundGetKsmps(self.cs)
        ptr = ct.POINTER(MYFLT)()
        err = ''
        ret = libcsound.csoundGetChannelPtr(self.cs, ct.byref(ptr), cstring(name), type_)
        if ret == CSOUND_SUCCESS:
            if chan_type == CSOUND_STRING_CHANNEL:
                return ct.cast(ptr, ct.c_char_p), err
            else:
                array_type = np.ctypeslib.ndpointer(MYFLT, 1, (length,), 'C_CONTIGUOUS')
                p = ct.cast(ptr, array_type)
                return array_from_pointer(p), err
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

    def list_channels(self):
        """Returns a list of allocated channels and an error message.
        
        A ControlChannelInfo object contains the channel characteristics.
        The error message indicates if there is not enough
        memory for allocating the list or it is an empty string if there is no
        error. In the case of no channels or an error, the list is None.

        Notes: the caller is responsible for freeing the list returned by the
        C API with delete_channel_list(). The name pointers may become
        invalid after calling reset().
        """
        chn_infos = None
        err = ''
        ptr = ct.cast(ct.POINTER(ct.c_int)(), ct.POINTER(ControlChannelInfo))
        n = libcsound.csoundListChannels(self.cs, ct.byref(ptr))
        if n == CSOUND_MEMORY :
            err = 'There is not enough memory for allocating the list'
        if n > 0:
            chn_infos = ct.cast(ptr, ct.POINTER(ControlChannelInfo * n)).contents
        return chn_infos, err

    def delete_channel_list(self, lst):
        """Releases a channel list previously returned by list_channels()."""
        ptr = ct.cast(lst, ct.POINTER(ControlChannelInfo))
        libcsound.csoundDeleteChannelList(self.cs, ptr)

    def set_control_channel_hints(self, name, hints):
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

    def control_channel_hints(self, name):
        """Returns special parameters (if any) of a control channel.
        
        Those parameters have been previously set with 
        set_control_channel_hints() or the chnparams opcode.
       
        The return values are a ControlChannelHints structure and
        CSOUND_SUCCESS if the channel exists and is a control channel,
        otherwise, None and an error code are returned.
        """
        hints = ControlChannelHints()
        ret = libcsound.csoundGetControlChannelHints(self.cs, cstring(name), ct.byref(hints))
        if ret != CSOUND_SUCCESS:
            hints = None
        return hints, ret

    def channel_lock(self, name):
        """Recovers a pointer to a lock for the specified channel called name.
        
        The returned lock can be locked/unlocked  with the spin_lock()
        and spin_unlock() functions.
        Returns the address of the lock or NULL if the channel does not exist.
        """
        ret = libcsound.csoundGetChannelLock(self.cs, cstring(name))
        if bool(ret) == False:
            return None
        return ret

    def control_channel(self, name):
        """Retrieves the value of control channel identified by name.
        
        A second value is returned, which, if not None, is the error
        (or success) code finding or accessing the channel.
        """
        err = ct.c_int(0)
        ret = libcsound.csoundGetControlChannel(self.cs, cstring(name), ct.byref(err))
        if bool(ret) == False:
            err = None
        return ret, err
    
    def set_control_channel(self, name, val):
        """Sets the value of control channel identified by name."""
        libcsound.csoundSetControlChannel(self.cs, cstring(name), MYFLT(val))

    def audio_channel(self, name, samples):
        """Copies the audio channel identified by name into ndarray samples.
        
        samples should contain enough memory for ksmps() MYFLTs.
        """
        ptr = samples.ctypes.data_as(ct.POINTER(MYFLT))
        libcsound.csoundGetAudioChannel(self.cs, cstring(name), ptr)
    
    def set_audio_channel(self, name, samples):
        """Sets the audio channel name with data from the ndarray samples.
        
        samples should contain at least ksmps() MYFLTs.
        """
        ptr = samples.ctypes.data_as(ct.POINTER(MYFLT))
        libcsound.csoundSetAudioChannel(self.cs, cstring(name), ptr)
    
    def string_channel(self, name):
        """Return a string from the string channel identified by name."""
        n = libcsound.csoundGetChannelDatasize(self.cs, cstring(name))
        if n > 0:
            s = ct.create_string_buffer(n)
            libcsound.csoundGetStringChannel(self.cs, cstring(name),
                ct.cast(s, ct.POINTER(ct.c_char)))
            return pstring(ctypes.string_at(s))
        return ""

    def set_string_channel(self, name, string):
        """Sets the string channel identified by name with string."""
        libcsound.csoundSetStringChannel(self.cs, cstring(name), cstring(string))

    def array_channel(self, name):
        """Receives an ARRAYDAT from channel name.
        
        array data is copied from the channel
        NB: array data needs to be allocated externally 
        and only the number of allocated bytes are copied
        returns 0 if successful, non-zero otherwise
        """
        array = ArrayDat()
        ret = libcsound.csoundGetArrayChannel(self.cs, cstring(name),
            ct.byref(array))
        if ret != 0:
            array = None
        return array, ret

    def set_array_channel(self, name, array):
        """Sends an ARRAYDAT array to the channel name
        
        array data is copied into the channel
        NB: the array in the channel receives only the amount of data it
        has allocated space for
        returns 0 if successful, non-zero otherwise
        """
        return libcsound.csoundSetArrayChannel(self.cs, cstring(name),
            ct.byref(array))

    def pvs_channel(self, name):
        """Receives a PVSDAT fout from the pvsout opcode.
        
        (f-rate) at channel 'name'
        Returns zero on success, CSOUND_ERROR if the index is invalid or
        if fsig framesizes are incompatible.
        CSOUND_MEMORY if there is not enough memory to extend the bus
        """
        fout = PvsDatExt()
        ret = libcsound.csoundGetPvsChannel(self.cs, cstring(name),
            ct.byref(fout))
        if ret != 0 :
            fout = None
        return fout, ret

    def set_pvs_channel(self, name, fin):
        """Sends a PVSDATEX fin to the pvsin opcode.

        (f-rate) for channel 'name'.
        Returns zero on success, CSOUND_ERROR if the index is invalid or
        fsig framesizes are incompatible.
        CSOUND_MEMORY if there is not enough memory to extend the bus.
        """
        return libcsound.csoundSetPvsChannel(self.cs, cstring(name),
            ct.byref(fin))

    def channel_datasize(self, name):
        """Returns the size of data stored in a channel."""
        return libcsound.csoundGetChannelDatasize(self.cs, cstring(name))

    def set_input_channel_callback(self, function):
        """Sets the function to call whenever the :code:`invalue` opcode is used."""
        self.input_channel_cb_ref = CHANNELFUNC(function)
        libcsound.csoundSetInputChannelCallback(self.cs, self.input_channel_cb_ref)
    
    def set_output_channel_callback(self, function):
        """Sets the function to call whenever the :code:`outvalue` opcode is used."""
        self.output_channel_cb_ref = CHANNELFUNC(function)
        libcsound.csoundSetOutputChannelCallback(self.cs, self.output_channel_cb_ref)

    def event(self, type_, params, async_mode=False):
        """Send a new event. 'type_' is the event type.
        
        type_ 0 - instrument instance     CS_INSTR_EVENT
        type_ 1 - function table instance CS_TABLE_EVENT
        event parameters is a tuple, a list, or an ndarray of MYFLTs with all
        the pfields for this event parameters (p-fields)
        optionally run asynchronously (async_mode = True)
        """
        p = np.asarray(pFields, dtype=MYFLT)
        ptr = p.ctypes.data_as(ct.POINTER(MYFLT))
        n_fields = ct.c_long(p.size)
        libcsound.csoundEvent(self.cs, ct.c_int(type_), ptr, n_fields,
            ct.c_int(async_mode))

    def event_string(self, message, async_mode=False):
        """Send a new event as a string.
        
        Multiple events separated by newlines are possible
        and score preprocessing (carry, etc) is applied.
        optionally run asynchronously (async_mode = True).
        """
        libcsound.csoundEventString(self.cs, cstring(message), ct.c_int(async_mode))

    def key_press(self, c):
        """Sets the ASCII code of the most recent key pressed.
        
        This value is used by the sensekey opcode if a callback for
        returning keyboard events is not set (see
        register_keyboard_callback()).
        """
        libcsound.csoundKeyPress(self.cs, cchar(c))

    def register_keyboard_callback(self, function, user_data, type_mask):
        """Registers general purpose callback functions for keyboard events.
        
        These callbacks are called on every control period by the sensekey
        opcode.
        
        The callback is preserved on reset(), and multiple
        callbacks may be set and will be called in reverse order of
        registration. If the same function is set again, it is only moved
        in the list of callbacks so that it will be called first, and the
        user data and type mask parameters are updated. type_mask can be the
        bitwise OR of callback types for which the function should be called,
        or zero for all types.
        
        Returns zero on success, CSOUND_ERROR if the specified function
        pointer or type mask is invalid, and CSOUND_MEMORY if there is not
        enough memory.
        
        The callback function takes the following arguments:
        
        user_data
            the "user data" pointer, as specified when setting the callback
        p
            data pointer, depending on the callback type
        type_
            callback type, can be one of the following (more may be added in
            future versions of Csound):
            
            CSOUND_CALLBACK_KBD_EVENT

            CSOUND_CALLBACK_KBD_TEXT
                called by the sensekey opcode to fetch key codes. The
                data pointer is a pointer to a single value of type int,
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
            self.keyboard_cb_event_ref = KEYBOARDFUNC(function)
        else:
            self.keyboard_cb_text_ref = KEYBOARDFUNC(function)
        return libcsound.csoundRegisterKeyboardCallback(self.cs, KEYBOARDFUNC(function),
            ct.py_object(user_data), ct.c_uint(type_mask))

    def remove_keyboard_callback(self, function):
        """Removes a callback previously set with register_keyboard_callback()."""
        libcsound.csoundRemoveKeyboardCallback(self.cs, KEYBOARDFUNC(function))

    #
    # Tables
    #
    def table_length(self, table):
        """Returns the length of a function table.
        
        (Not including the guard point).
        If the table does not exist, returns -1.
        """
        return libcsound.csoundTableLength(self.cs, ct.c_int(table))

    def table(self, tableNum):
        """Returns a pointer to function table tableNum as an ndarray.
        
        The ndarray does not include the guard point. If the table does not
        exist, None is returned.
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
        None is returned.
        
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

    #
    # Score Handling
    #
    def score_time(self):
        """Returns the current score time.
        
        The return value is the time in seconds since the beginning of
        performance.
        """
        return libcsound.csoundGetScoreTime(self.cs)
    
    def is_score_pending(self):
        """Tells whether Csound score events are performed or not.
        
        Independently of real-time MIDI events (see set_score_pending()).
        """
        return libcsound.csoundIsScorePending(self.cs) != 0

    def set_score_pending(self, pending):
        """Sets whether Csound score events are performed or not.
        
        Real-time events will continue to be performed. Can be used by external
        software, such as a VST host, to turn off performance of score events
        (while continuing to perform real-time events), for example to mute
        a Csound score while working on other tracks of a piece, or to play
        the Csound instruments live.
        """
        libcsound.csoundSetScorePending(self.cs, ct.c_int(pending))

    def score_offset_seconds(self):
        """Returns the score time beginning.
        
        At this time score events will actually immediately be performed
        (see set_score_offset_seconds()).
        """
        return libcsound.csoundGetScoreOffsetSeconds(self.cs)

    def set_score_offset_seconds(self, time_):
        """Csound score events prior to the specified time are not performed.
        
        Performance begins immediately at the specified time (real-time events
        will continue to be performed as they are received). Can be used by
        external software, such as a VST host, to begin score performance
        midway through a Csound score, for example to repeat a loop in a
        sequencer, or to synchronize other events with the Csound score.
        """
        libcsound.csoundSetScoreOffsetSeconds(self.cs, MYFLT(time_))
    
    def rewind_score(self):
        """Rewinds a compiled Csound score.
        
        It is rewinded to the time specified with set_score_offset_seconds().
        """
        libcsound.csoundRewindScore(self.cs)

    def sleep(self, milliseconds):
        """Waits for at least the specified number of milliseconds.
        
        It yields the CPU to other threads.
        """
        libcsound.csoundSleep(ct.c_uint(milliseconds))

    #
    # Opcodes
    #
    def load_plugins(self, directory):
        """Loads all plugins from a given directory.
        
        Generally called immediatly after csound_create() to make new
        opcodes/modules available for compilation and performance.
        """
        return libcsound.csoundLoadPlugins(self.cs, cstring(directory))

    def append_opcode(self, opname, dsblksiz, flags, outypes, intypes,
        initfunc, perffunc, deinitfunc):
        """Appends an opcode implemented by external software.
        
        This opcode is added to Csound's internal opcode list.
        The opcode list is extended by one slot, and the parameters are copied
        into the new slot.
        
        Returns zero on success.
        """
        return libcsound.csoundAppendOpcode(self.cs, cstring(opname), dsblksiz,
            flags, cstring(outypes), cstring(intypes),
            OPCODEFUNC(initfunc), OPCODEFUNC(perffunc), OPCODEFUNC(deinitfunc))

    #
    # Table Display
    #
    def set_is_graphable(self, is_graphable):
        """Tells Csound whether external graphic table display is supported.
        
        Return the previously set value (initially False).
        """
        ret = libcsound.csoundSetIsGraphable(self.cs, ct.c_int(isGraphable))
        return (ret != 0)
    
    def set_make_graph_callback(self, function):
        """Called by external software to set Csound's MakeGraph function."""
        self.make_graph_cb_ref = MAKEGRAPHFUNC(function)
        libcsound.csoundSetMakeGraphCallback(self.cs, self.make_graph_cb_ref)
        
    def set_draw_graph_callback(self, function):
        """Called by external software to set Csound's DrawGraph function."""
        self.draw_graph_cb_ref = DRAWGRAPHFUNC(function)
        libcsound.csoundSetDrawGraphCallback(self.cs, self.draw_graph_cb_ref)
    
    def set_kill_graph_callback(self, function):
        """Called by external software to set Csound's KillGraph function."""
        self.kill_graph_cb_ref = KILLGRAPHFUNC(function)
        libcsound.csoundSetKillGraphCallback(self.cs, self.kill_graph_cb_ref)
                                                              
    def set_exit_graph_callback(self, function):
        """Called by external software to set Csound's ExitGraph function."""
        self.exit_graph_cb_ref = EXITGRAPHFUNC(function)
        libcsound.csoundSetExitGraphCallback(self.cs, self.exit_graph_cb_ref)

    #
    # Circular Buffer Functios
    #
    def create_circular_buffer(self, numelem, elemsize):
        """Creates a circular buffer with numelem number of elements.
        
        The element's size is set from elemsize. It should be used like::
        
            rb = cs.create_circular_buffer(1024, cs.size_of_MYFLT())
        """
        return libcsound.csoundCreateCircularBuffer(self.cs, numelem, elemsize)
    
    def read_circular_buffer(self, circular_buffer, out, items):
        """Reads from circular buffer.
        
        circular_buffer
            pointer to an existing circular buffer
        out
            preallocated ndarray with at least items number of elements,
            where buffer contents will be read into
        items
            number of samples to be read
        
        Returns the actual number of items read (0 <= n <= items).
        """
        if len(out) < items:
            return 0
        ptr = out.ctypes.data_as(ct.c_void_p)
        return libcsound.csoundReadCircularBuffer(self.cs, circular_buffer, ptr, items)
    
    def peek_circular_buffer(self, circular_buffer, out, items):
        """Reads from circular buffer without removing them from the buffer.
        
        circular_buffer
            pointer to an existing circular buffer
        out
            preallocated ndarray with at least items number of elements,
            where buffer contents will be read into
        items
            number of samples to be read
        
        Returns the actual number of items read (0 <= n <= items).
        """
        if len(out) < items:
            return 0
        ptr = out.ctypes.data_as(ct.c_void_p)
        return libcsound.csoundPeekCircularBuffer(self.cs, circular_buffer, ptr, items)
    
    def write_circular_buffer(self, circular_buffer, in_, items):
        """Writes to circular buffer.
        
        circular_buffer
            pointer to an existing circular buffer
        in_
            ndarray with at least items number of elements to be written
            into circular buffer
        items
            number of samples to write
            
        Returns the actual number of items written (0 <= n <= items).
        """
        if len(in_) < items:
            return 0
        ptr = in_.ctypes.data_as(ct.c_void_p)
        return libcsound.csoundWriteCircularBuffer(self.cs, circular_buffer, ptr, items)
    
    def flush_circular_buffer(self, circular_buffer):
        """Empties circular buffer of any remaining data.
        
        This function should only be used if there is no reader actively
        getting data from the buffer.
        
        circular_buffer
            pointer to an existing circular buffer
        """
        libcsound.csoundFlushCircularBuffer(self.cs, circular_buffer)
    
    def destroy_circular_buffer(self, circular_buffer):
        """Frees circular buffer."""
        libcsound.csoundDestroyCircularBuffer(self.cs, circular_buffer)


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
    stop(), or if an error occurs.
    The constructor takes a Csound instance pointer as argument; it assumes
    that ctcsound.compile_() was called successfully before creating
    the performance thread. Once the playback is stopped for one of the above
    mentioned reasons, the performance thread returns.
    """
    def __init__(self, csp):
        self.cpt = libcspt.NewCsoundPT(csp)
    
    def __del__(self):
        libcspt.DeleteCsoundPT(self.cpt)
    
    def is_running(self):
        """Returns True if the performance thread is running, False otherwise."""
        return libcspt.CsoundPTisRunning(self.cpt) != 0
    
    def process_cb(self):
        """Returns the process callback."""
        return PROCESSFUNC(libcspt.CsoundPTgetProcessCB(self.cpt))
    
    def set_process_cb(self, function, data):
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
        """Pauses performance (can be continued by calling play())."""
        libcspt.CsoundPTpause(self.cpt)
    
    def toggle_pause(self):
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
    
    def stop_record(self):
        """Stops recording and closes audio file."""
        libcspt.CsoundPTstopRecord(self.cpt)
    
    def score_event(self, absp2mode, opcod, pFields):
        """Sends a score event.
        
        The event has type opcod (e.g. 'i' for a note event).
        pFields is tuple, a list, or an ndarray of MYFLTs with all the pfields
        for this event, starting with the p1 value specified in pFields[0].
        If absp2mode is non-zero, the start time of the event is measured
        from the beginning of performance, instead of the default of relative
        to the current time.
        """
        p = np.array(pFields).astype(MYFLT)
        ptr = p.ctypes.data_as(ct.POINTER(MYFLT))
        numFields = p.size
        libcspt.CsoundPTscoreEvent(self.cpt, ct.c_int(absp2mode), cchar(opcod), numFields, ptr)
    
    def input_message(self, s):
        """Sends a score event as a string, similarly to line events (-L)."""
        libcspt.CsoundPTinputMessage(self.cpt, cstring(s))
    
    def set_score_offset_seconds(self, timeVal):
        """Sets the playback time pointer to the specified value (in seconds)."""
        libcspt.CsoundPTsetScoreOffsetSeconds(self.cpt, ct.c_double(timeVal))
    
    def join(self):
        """Waits until the performance is finished or fails.
        
        Returns a positive value if the end of score was reached or
        stop() was called, and a negative value if an error occured.
        Also releases any resources associated with the performance thread
        object.
        """
        return libcspt.CsoundPTjoin(self.cpt)
    
    def flush_message_queue(self):
        """Waits until all pending messages are actually received.
        
        (pause, send score event, etc.)
        """
        libcspt.CsoundPTflushMessageQueue(self.cpt)
