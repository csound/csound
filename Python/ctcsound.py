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
#   Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
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

#
# ERROR DEFINITIONS
#
CSOUND_SUCCESS = 0          # Completed successfully.
CSOUND_ERROR = -1           # Unspecified failure.
CSOUND_INITIALIZATION = -2  # Failed during initialization.
CSOUND_PERFORMANCE = -3     # Failed during performance.
CSOUND_MEMORY = -4          # Failed to allocate requested memory.
CSOUND_SIGNAL = -5          # Termination requested by SIGINT or SIGTERM.

#
# Compilation or performance aborted, but not as a result of an error
# (e.g. --help, or running an utility with -U).
#
CSOUND_EXITJMP_SUCCESS = 256

#
# Flags for csoundInitialize().
#
CSOUNDINIT_NO_SIGNAL_HANDLER = 1
CSOUNDINIT_NO_ATEXIT = 2

#
# Types for keyboard callbacks set in csoundRegisterKeyboardCallback()
#
CSOUND_CALLBACK_KBD_EVENT = ct.c_uint32(1)
CSOUND_CALLBACK_KBD_TEXT = ct.c_uint32(2)

#
# Opaque pointers for Csound structs
#
CSOUND_p = ct.c_void_p
STRINGDAT_p = ct.c_void_p
ARRAYDAT_p = ct.c_void_p
PVSDAT_p = ct.c_void_p

# Opaque pointers for UGen API structs
UGEN_p = ct.c_void_p
UGEN_FACTORY_p = ct.c_void_p
UGEN_CONTEXT_p = ct.c_void_p
UGEN_GRAPH_p = ct.c_void_p

#
# csound configuration structure, mirrors part of
# OPARMS, uses more meaningful names
#
class CsoundParams(ct.Structure):
    _fields_ = [("debug_mode", ct.c_int32), # debug flag
        ("sf_read", ct.c_int32),            # sound input read flag
        ("sf_write", ct.c_int32),           # sound output write flab (-s)
        ("file_type", ct.c_int32),          # soundfile type code
        ("in_buffer_samples", ct.c_int32),  # input buffer size in samples
        ("out_buffer_samples", ct.c_int32), # output buffer size in samples
        ("in_format", ct.c_int32),          # input soundfile format
        ("out_format", ct.c_int32),         # output soundfile format
        ("sf_sample_size", ct.c_int32),     # sample size
        ("displays", ct.c_int32),           # displays flag
        ("graphs_off", ct.c_int32),         # graphs flag
        ("postscript", ct.c_int32),         # postscript graphs flag
        ("message_level", ct.c_int32),      # message level (-m)
        ("beat_mode", ct.c_int32),          # beat mode
        ("max_lag", ct.c_int32),            # hardware buffer size (samples)
        ("line_in", ct.c_int32),            # linevents flag (-L)
        ("rt_events", ct.c_int32),          # realtime events flag (scoreless, -L, -F, -M)
        ("midi_in", ct.c_int32),            # midi input flag (-M)
        ("f_midi_in", ct.c_int32),          # midi file input flag (-F)
        ("r_midi_in", ct.c_int32),          # remote events flag
        ("ringbell", ct.c_int32),           # ringbell flag
        ("term_mf_end", ct.c_int32),        # terminate on midi file input flag (-T)
        ("rewrite_header", ct.c_int32),     # rewrite header flag
        ("heartbeat", ct.c_int32),          # heartbeat flag
        ("gen01_defer", ct.c_int32),        # GEN01 defer allocation flag
        ("cmd_tempo", ct.c_double),         # tempo value (-t)
        ("sr_override", MYFLT),             # sampling rate override (-r)
        ("kr_override", MYFLT),             # controle rate override (-k)
        ("nchnls_override", ct.c_int32),    # nchnls override
        ("nchnls_i_override", ct.c_int32),  # nchnls_i override
        ("in_filename", ct.c_char_p),       # input file name (-i)
        ("out_filename", ct.c_char_p),      # output file name (-o)
        ("linename", ct.c_char_p),          # line events source (-L)
        ("midiname", ct.c_char_p),          # midi input device name (-M)
        ("f_midiname", ct.c_char_p),        # midi input file name (-F)
        ("midi_out_name", ct.c_char_p),     # midi output device name (-M)
        ("f_midi_out_name", ct.c_char_p),   # midi output file name (-F)
        ("midi_key", ct.c_int32),           # midi key pfield mapping
        ("midi_key_cps", ct.c_int32),       # midi key-cps pfield mapping
        ("midi_key_oct", ct.c_int32),       # midi key-oct pfield mapping
        ("midi_key_pch", ct.c_int32),       # midi key-pch pfield mapping
        ("midi_velocity", ct.c_int32),      # midi vel pfield mapping
        ("midi_velocity_amp", ct.c_int32),  # midi vel-amp pfield mapping
        ("no_default_paths", ct.c_int32),   # default paths flag
        ("number_of_threads", ct.c_int32),  # multicore number of threads (-j)
        ("syntax_check_only", ct.c_int32),  # syntax check only flag
        ("use_csd_line_counts", ct.c_int32), # csd line nums option
        ("sample_accurate", ct.c_int32),    # sample accurate flag
        ("realtime", ct.c_int32),           # realtime priority flag
        ("e0dbfs_override", MYFLT),         # 0dbfs override
        ("daemon", ct.c_int32),             # daemon mode flag
        ("quality", ct.c_double),           # OGG encoding quality
        ("ksmps_override", ct.c_int32),     # ksmps override
        ("fft_lib", ct.c_int32),            # FFT library option
        ("echo", ct.c_int32),               # UDP echo commands flag
        ("limiter", MYFLT),                 # audio output limiter option
        ("sr_default", MYFLT),              # default sampling rate
        ("kr_default", MYFLT),              # default control rate
        ("mp3_mode", ct.c_int32),           # MP3 encoding mode
        ("redef", ct.c_int32)]              # instr redefinition flag

#
# Device information
#
string128 = ct.c_char * 128

class CsoundAudioDevice(ct.Structure):
    _fields_ = [("device_name", string128),
                ("device_id", string128),
                ("rt_module", string128),
                ("max_nchnls", ct.c_int32),
                ("isOutput", ct.c_int32)]

class CsoundMidiDevice(ct.Structure):
    _fields_ = [("device_name", string128),
                ("interface_name", string128),
                ("device_id", string128),
                ("midi_module", string128),
                ("isOutput", ct.c_int32)]

#
# PVSDAT window types
#
PVS_WIN_HAMMING = 0
PVS_WIN_HANN = 1
PVS_WIN_KAISER = 2
PVS_WIN_CUSTOM = 3
PVS_WIN_BLACKMAN = 4
PVS_WIN_BLACKMAN_EXACT = 5
PVS_WIN_NUTTALLC3 = 6
PVS_WIN_BHARRIS_3 = 7
PVS_WIN_BHARRIS_MIN = 8
PVS_WIN_RECT = 9

#
# PVSDAT formats
#
PVS_AMP_FREQ = 0   # phase vocoder
PVS_AMP_PHASE = 1  # polar DFT
PVS_COMPLEX = 2    # rectangular DFT
PVS_TRACKS = 3     # amp, freq, phase, ID tracks

#
# Constants used by the bus interface (csoundGetChannelPtr() etc.).
#
CSOUND_CONTROL_CHANNEL = 1
CSOUND_AUDIO_CHANNEL  = 2
CSOUND_STRING_CHANNEL = 3
CSOUND_PVS_CHANNEL = 4
CSOUND_VAR_CHANNEL = 5
CSOUND_ARRAY_CHANNEL = 6
CSOUND_CHANNEL_TYPE_MASK = 15
CSOUND_INPUT_CHANNEL = 16
CSOUND_OUTPUT_CHANNEL = 32

CSOUND_CONTROL_CHANNEL_NO_HINTS = 0
CSOUND_CONTROL_CHANNEL_INT = 1
CSOUND_CONTROL_CHANNEL_LIN = 2
CSOUND_CONTROL_CHANNEL_EXP = 3

#
# This structure holds the parameter hints for control channels
#
class ControlChannelHints(ct.Structure):
    _fields_ = [("behav", ct.c_int32),
                ("dflt", MYFLT),
                ("min", MYFLT),
                ("max", MYFLT),
                ("x", ct.c_int32),
                ("y", ct.c_int32),
                ("width", ct.c_int32),
                ("height", ct.c_int32),
                # This member must be set explicitly to None if not used
                ("attributes", ct.c_char_p)]

class ControlChannelInfo(ct.Structure):
    _fields_ = [("name", ct.c_char_p),
                ("type", ct.c_int32),
                ("hints", ControlChannelHints)]

#
# Event types
#
CS_INSTR_EVENT = 0
CS_TABLE_EVENT = 1
CS_END_EVENT = 2


# Instantiation
libcsound.csoundInitialize.restype = ct.c_int32
libcsound.csoundInitialize.argtypes = [ct.c_int32]
libcsound.csoundCreate.restype = CSOUND_p
libcsound.csoundCreate.argtypes = [ct.py_object, ct.c_char_p]
libcsound.csoundDestroy.argtypes = [CSOUND_p]

# Attributes
libcsound.csoundGetVersion.restype = ct.c_int32
libcsound.csoundGetSr.restype = MYFLT
libcsound.csoundGetSr.argtypes = [CSOUND_p]
libcsound.csoundGetKr.restype = MYFLT
libcsound.csoundGetKr.argtypes = [CSOUND_p]
libcsound.csoundGetKsmps.restype = ct.c_uint32
libcsound.csoundGetKsmps.argtypes = [CSOUND_p]
libcsound.csoundGetChannels.restype = ct.c_uint32
libcsound.csoundGetChannels.argtypes = [CSOUND_p, ct.c_int32]
libcsound.csoundGet0dBFS.restype = MYFLT
libcsound.csoundGet0dBFS.argtypes = [CSOUND_p]
libcsound.csoundGetA4.restype = MYFLT
libcsound.csoundGetA4.argtypes = [CSOUND_p]
libcsound.csoundGetCurrentTimeSamples.restype = ct.c_int64
libcsound.csoundGetCurrentTimeSamples.argtypes = [CSOUND_p]
libcsound.csoundGetSizeOfMYFLT.restype = ct.c_int32
libcsound.csoundGetHostData.restype = ct.py_object
libcsound.csoundGetHostData.argtypes = [CSOUND_p]
libcsound.csoundSetHostData.argtypes = [CSOUND_p, ct.py_object]
libcsound.csoundGetEnv.restype = ct.c_char_p
libcsound.csoundGetEnv.argtypes = [CSOUND_p, ct.c_char_p]
libcsound.csoundSetGlobalEnv.restype = ct.c_int32
libcsound.csoundSetGlobalEnv.argtypes = [ct.c_char_p, ct.c_char_p]
libcsound.csoundSetOption.restype = ct.c_int32
libcsound.csoundSetOption.argtypes = [CSOUND_p, ct.c_char_p]
libcsound.csoundGetParams.argtypes = [CSOUND_p, ct.POINTER(CsoundParams)]
libcsound.csoundGetDebug.restype = ct.c_int32
libcsound.csoundGetDebug.argtypes = [CSOUND_p]
libcsound.csoundSetDebug.argtypes = [CSOUND_p, ct.c_int32]
libcsound.csoundSystemSr.restype = MYFLT
libcsound.csoundSystemSr.argtypes = [CSOUND_p, MYFLT]
libcsound.csoundGetModule.restype = ct.c_int32
libcsound.csoundGetModule.argtypes = [CSOUND_p, ct.c_int,
                                      ct.POINTER(ct.c_char_p), ct.POINTER(ct.c_char_p)]
libcsound.csoundGetAudioDevList.restype = ct.c_int32
libcsound.csoundGetAudioDevList.argtypes = [CSOUND_p, ct.c_void_p, ct.c_int32]
libcsound.csoundGetMIDIDevList.restype = ct.c_int32
libcsound.csoundGetMIDIDevList.argtypes = [CSOUND_p, ct.c_void_p, ct.c_int32]
libcsound.csoundGetMessageLevel.restype = ct.c_int32
libcsound.csoundSetMessageLevel.argtypes = [CSOUND_p, ct.c_int32]

# Performance
libcsound.csoundCompile.restype = ct.c_int32
libcsound.csoundCompile.argtypes = [CSOUND_p, ct.c_int32, ct.POINTER(ct.c_char_p)]
libcsound.csoundCompileOrc.restype = ct.c_int32
libcsound.csoundCompileOrc.argtypes = [CSOUND_p, ct.c_char_p, ct.c_int32]
libcsound.csoundEvalCode.restype = MYFLT
libcsound.csoundEvalCode.argtypes = [CSOUND_p, ct.c_char_p]
libcsound.csoundCompileCSD.restype = ct.c_int32
libcsound.csoundCompileCSD.argtypes = [CSOUND_p, ct.c_char_p, ct.c_int32, ct.c_int32]
libcsound.csoundStart.restype = ct.c_int32
libcsound.csoundStart.argtypes = [CSOUND_p]
libcsound.csoundPerformKsmps.restype = ct.c_int32
libcsound.csoundPerformKsmps.argtypes = [CSOUND_p]
libcsound.csoundRunUtility.restype = ct.c_int32
libcsound.csoundRunUtility.argtypes = [CSOUND_p, ct.c_char_p, ct.c_int32, ct.POINTER(ct.c_char_p)]
libcsound.csoundReset.argtypes = [CSOUND_p]

# Realtime Audio I/O
libcsound.csoundSetHostAudioIO.argtypes = [CSOUND_p]
libcsound.csoundSetRTAudioModule.argtypes = [CSOUND_p, ct.c_char_p]
libcsound.csoundGetSpin.restype = ct.POINTER(MYFLT)
libcsound.csoundGetSpin.argtypes = [CSOUND_p]
libcsound.csoundGetSpout.restype = ct.POINTER(MYFLT)
libcsound.csoundGetSpout.argtypes = [CSOUND_p]

# Realtime MIDI I/O
libcsound.csoundSetHostMIDIIO.argtypes = [CSOUND_p]
libcsound.csoundSetMIDIModule.argtypes = [CSOUND_p, ct.c_char_p]
MIDIINOPENFUNC = ct.CFUNCTYPE(ct.c_int32, CSOUND_p, ct.POINTER(ct.c_void_p), ct.c_char_p)
MIDIREADFUNC = ct.CFUNCTYPE(ct.c_int32, CSOUND_p, ct.c_void_p, ct.c_char_p, ct.c_int32)
MIDIINCLOSEFUNC = ct.CFUNCTYPE(ct.c_int32, CSOUND_p, ct.c_void_p)
MIDIOUTOPENFUNC = ct.CFUNCTYPE(ct.c_int32, CSOUND_p, ct.POINTER(ct.c_void_p), ct.c_char_p)
MIDIWRITEFUNC = ct.CFUNCTYPE(ct.c_int32, CSOUND_p, ct.c_void_p, ct.c_char_p, ct.c_int32)
MIDIOUTCLOSEFUNC = ct.CFUNCTYPE(ct.c_int32, CSOUND_p, ct.c_void_p)
MIDIERRORFUNC = ct.CFUNCTYPE(ct.c_char_p, ct.c_int32)
MIDIDEVLISTFUNC = ct.CFUNCTYPE(ct.c_int32, CSOUND_p, ct.POINTER(CsoundMidiDevice), ct.c_int32)
libcsound.csoundSetExternalMidiInOpenCallback.argtypes = [CSOUND_p, MIDIINOPENFUNC]
libcsound.csoundSetExternalMidiReadCallback.argtypes = [CSOUND_p, MIDIREADFUNC]
libcsound.csoundSetExternalMidiInCloseCallback.argtypes = [CSOUND_p, MIDIINCLOSEFUNC]
libcsound.csoundSetExternalMidiOutOpenCallback.argtypes = [CSOUND_p, MIDIOUTOPENFUNC]
libcsound.csoundSetExternalMidiWriteCallback.argtypes = [CSOUND_p, MIDIWRITEFUNC]
libcsound.csoundSetExternalMidiOutCloseCallback.argtypes = [CSOUND_p, MIDIOUTCLOSEFUNC]
libcsound.csoundSetExternalMidiErrorStringCallback.argtypes = [CSOUND_p, MIDIERRORFUNC]
libcsound.csoundSetMIDIDeviceListCallback.argtypes = [CSOUND_p, MIDIDEVLISTFUNC]

# Messages
libcsound.csoundMessage.argtypes = [CSOUND_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundMessageS.argtypes = [CSOUND_p, ct.c_int32, ct.c_char_p, ct.c_char_p]
MSGSTRFUNC = ct.CFUNCTYPE(None, CSOUND_p, ct.c_int32, ct.c_char_p)
libcsound.csoundSetMessageStringCallback.argtypes = [CSOUND_p, MSGSTRFUNC]
libcsound.csoundCreateMessageBuffer.argtypes = [CSOUND_p, ct.c_int32]
libcsound.csoundGetFirstMessage.restype = ct.c_char_p
libcsound.csoundGetFirstMessage.argtypes = [CSOUND_p]
libcsound.csoundGetFirstMessageAttr.restype = ct.c_int32
libcsound.csoundGetFirstMessageAttr.argtypes = [CSOUND_p]
libcsound.csoundPopFirstMessage.argtypes = [CSOUND_p]
libcsound.csoundGetMessageCnt.restype = ct.c_int32
libcsound.csoundGetMessageCnt.argtypes = [CSOUND_p]
libcsound.csoundDestroyMessageBuffer.argtypes = [CSOUND_p]

# Channels, Controls and Events
libcsound.csoundGetChannelPtr.restype = ct.c_int32
libcsound.csoundGetChannelPtr.argtypes = [CSOUND_p, ct.POINTER(ct.c_void_p),
                                          ct.c_char_p, ct.c_int32]
libcsound.csoundGetChannelVarTypeName.restype = ct.c_char_p
libcsound.csoundGetChannelVarTypeName.argtypes = [CSOUND_p, ct.c_char_p]
libcsound.csoundListChannels.restype = ct.c_int32
libcsound.csoundListChannels.argtypes = [CSOUND_p, ct.POINTER(ct.POINTER(ControlChannelInfo))]
libcsound.csoundDeleteChannelList.argtypes = [CSOUND_p, ct.POINTER(ControlChannelInfo)]
libcsound.csoundSetControlChannelHints.restype = ct.c_int32
libcsound.csoundSetControlChannelHints.argtypes = [CSOUND_p, ct.c_char_p, ControlChannelHints]
libcsound.csoundGetControlChannelHints.restype = ct.c_int32
libcsound.csoundGetControlChannelHints.argtypes = [CSOUND_p, ct.c_char_p,
                                                   ct.POINTER(ControlChannelHints)]
libcsound.csoundLockChannel.argtypes = [CSOUND_p, ct.c_char_p]
libcsound.csoundUnlockChannel.argtypes = [CSOUND_p, ct.c_char_p]
libcsound.csoundGetControlChannel.restype = MYFLT
libcsound.csoundGetControlChannel.argtypes = [CSOUND_p, ct.c_char_p, ct.POINTER(ct.c_int32)]
libcsound.csoundSetControlChannel.argtypes = [CSOUND_p, ct.c_char_p, MYFLT]
libcsound.csoundGetAudioChannel.argtypes = [CSOUND_p, ct.c_char_p, ct.POINTER(MYFLT)]
libcsound.csoundSetAudioChannel.argtypes = [CSOUND_p, ct.c_char_p, ct.POINTER(MYFLT)]
libcsound.csoundGetStringChannel.argtypes = [CSOUND_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundSetStringChannel.argtypes = [CSOUND_p, ct.c_char_p, ct.c_char_p]

libcsound.csoundInitArrayChannel.restype = ARRAYDAT_p
libcsound.csoundInitArrayChannel.argtypes = [CSOUND_p, ct.c_char_p, ct.c_char_p,
                                             ct.c_int32, ct.POINTER(ct.c_int32)]
libcsound.csoundArrayDataType.restype = ct.c_char_p
libcsound.csoundArrayDataType.argtypes = [ARRAYDAT_p]
libcsound.csoundArrayDataDimensions.restype = ct.c_int32
libcsound.csoundArrayDataDimensions.argtypes = [ARRAYDAT_p]
libcsound.csoundArrayDataSizes.restype = ct.POINTER(ct.c_int32)
libcsound.csoundArrayDataSizes.argtypes = [ARRAYDAT_p]
libcsound.csoundSetArrayData.argtypes = [ARRAYDAT_p, ct.c_void_p]
libcsound.csoundGetArrayData.restype = ct.c_void_p
libcsound.csoundGetArrayData.argtypes = [ARRAYDAT_p]
libcsound.csoundGetStringData.restype = ct.c_char_p
libcsound.csoundGetStringData.argtypes = [CSOUND_p, STRINGDAT_p]
libcsound.csoundSetStringData.argtypes = [CSOUND_p, STRINGDAT_p, ct.c_char_p]
libcsound.csoundInitPvsChannel.restype = PVSDAT_p
libcsound.csoundInitPvsChannel.argtypes = [CSOUND_p, ct.c_char_p,
                                           ct.c_int32, ct.c_int32, ct.c_int32,
                                           ct.c_int32, ct.c_int32]
libcsound.csoundPvsDataFFTSize.restype = ct.c_int32
libcsound.csoundPvsDataFFTSize.argtypes = [PVSDAT_p]
libcsound.csoundPvsDataOverlap.restype = ct.c_int32
libcsound.csoundPvsDataOverlap.argtypes = [PVSDAT_p]
libcsound.csoundPvsDataWindowSize.restype = ct.c_int32
libcsound.csoundPvsDataWindowSize.argtypes = [PVSDAT_p]
libcsound.csoundPvsDataFormat.restype = ct.c_int32
libcsound.csoundPvsDataFormat.argtypes = [PVSDAT_p]
libcsound.csoundPvsDataFramecount.restype = ct.c_uint32
libcsound.csoundPvsDataFramecount.argtypes = [PVSDAT_p]
libcsound.csoundGetPvsData.restype = ct.POINTER(ct.c_float)
libcsound.csoundGetPvsData.argtypes = [PVSDAT_p]
libcsound.csoundSetPvsData.argtypes = [PVSDAT_p, ct.POINTER(ct.c_float)]
libcsound.csoundGetChannelDatasize.restype = ct.c_int32
libcsound.csoundGetChannelDatasize.argtypes = [CSOUND_p, ct.c_char_p]
CHANNELFUNC = ct.CFUNCTYPE(None, CSOUND_p, ct.c_char_p, ct.c_void_p, ct.c_void_p)
libcsound.csoundSetInputChannelCallback.argtypes = [CSOUND_p, CHANNELFUNC]
libcsound.csoundSetOutputChannelCallback.argtypes = [CSOUND_p, CHANNELFUNC]
libcsound.csoundEvent.argtypes = [CSOUND_p, ct.c_int32, ct.POINTER(MYFLT),
                                  ct.c_int32, ct.c_int32]
libcsound.csoundEventString.argtypes = [CSOUND_p, ct.c_char_p, ct.c_int32]
libcsound.csoundGetInstrNumber.argtypes = [CSOUND_p, ct.c_char_p]
libcsound.csoundKeyPress.argtypes = [CSOUND_p, ct.c_char]
KEYBOARDFUNC = ct.CFUNCTYPE(ct.c_int32, ct.py_object, ct.c_void_p, ct.c_uint32)
libcsound.csoundRegisterKeyboardCallback.restype = ct.c_int32
libcsound.csoundRegisterKeyboardCallback.argtypes = [CSOUND_p, KEYBOARDFUNC,
                                                     ct.py_object, ct.c_uint32]
libcsound.csoundRemoveKeyboardCallback.argtypes = [CSOUND_p, KEYBOARDFUNC]

# Tables
libcsound.csoundTableLength.restype = ct.c_int32
libcsound.csoundTableLength.argtypes = [CSOUND_p, ct.c_int32]
libcsound.csoundGetTable.restype = ct.c_int32
libcsound.csoundGetTable.argtypes = [CSOUND_p, ct.POINTER(ct.POINTER(MYFLT)), ct.c_int32]
libcsound.csoundGetTableArgs.restype = ct.c_int32
libcsound.csoundGetTableArgs.argtypes = [CSOUND_p, ct.POINTER(ct.POINTER(MYFLT)), ct.c_int32]

# Score Handling
libcsound.csoundGetScoreTime.restype = ct.c_double
libcsound.csoundGetScoreTime.argtypes = [CSOUND_p]
libcsound.csoundIsScorePending.restype = ct.c_int32
libcsound.csoundIsScorePending.argtypes = [CSOUND_p]
libcsound.csoundSetScorePending.argtypes = [CSOUND_p, ct.c_int32]
libcsound.csoundGetScoreOffsetSeconds.restype = MYFLT
libcsound.csoundGetScoreOffsetSeconds.argtypes = [CSOUND_p]
libcsound.csoundSetScoreOffsetSeconds.argtypes = [CSOUND_p, MYFLT]
libcsound.csoundRewindScore.argtypes = [CSOUND_p]
libcsound.csoundSleep.argtypes = [ct.c_size_t]

# Opcodes
libcsound.csoundLoadPlugins.restype = ct.c_int32
libcsound.csoundLoadPlugins.argtypes = [CSOUND_p, ct.c_char_p]
OPCODEFUNC = ct.CFUNCTYPE(ct.c_int32, CSOUND_p, ct.c_void_p)
libcsound.csoundAppendOpcode.restype = ct.c_int32
libcsound.csoundAppendOpcode.argtypes = [CSOUND_p, ct.c_char_p, ct.c_int32,
    ct.c_int32, ct.c_char_p, ct.c_char_p,
    OPCODEFUNC, OPCODEFUNC, OPCODEFUNC]

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
                ("danflag", ct.c_int32),           # set to 1 for extra Yaxis mid span
                ("absflag", ct.c_int32)]           # set to 1 to skip abs check

# Symbols for Windat.polarity field
NOPOL = 0
NEGPOL = 1
POSPOL = 2
BIPOL = 3

# Table display (from graph_display.h)
libcsound.csoundSetIsGraphable.argtypes = [CSOUND_p, ct.c_int]


MAKEGRAPHFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.POINTER(Windat), ct.c_char_p)
libcsound.csoundSetMakeGraphCallback.argtypes = [CSOUND_p, MAKEGRAPHFUNC]
DRAWGRAPHFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.POINTER(Windat))
libcsound.csoundSetDrawGraphCallback.argtypes = [CSOUND_p, DRAWGRAPHFUNC]
KILLGRAPHFUNC = ct.CFUNCTYPE(None, ct.c_void_p, ct.POINTER(Windat))
libcsound.csoundSetKillGraphCallback.argtypes = [CSOUND_p, KILLGRAPHFUNC]
EXITGRAPHFUNC = ct.CFUNCTYPE(ct.c_int32, ct.c_void_p)
libcsound.csoundSetExitGraphCallback.argtypes = [CSOUND_p, EXITGRAPHFUNC]

# Circular buffer functions (from circular_buffer.h)
libcsound.csoundCreateCircularBuffer.restype = ct.c_void_p
libcsound.csoundCreateCircularBuffer.argtypes = [CSOUND_p, ct.c_int32, ct.c_int32]
libcsound.csoundReadCircularBuffer.restype = ct.c_int32
libcsound.csoundReadCircularBuffer.argtypes = [CSOUND_p, ct.c_void_p, ct.c_void_p, ct.c_int32]
libcsound.csoundPeekCircularBuffer.restype = ct.c_int32
libcsound.csoundPeekCircularBuffer.argtypes = [CSOUND_p, ct.c_void_p, ct.c_void_p, ct.c_int32]
libcsound.csoundWriteCircularBuffer.restype = ct.c_int32
libcsound.csoundWriteCircularBuffer.argtypes = [CSOUND_p, ct.c_void_p, ct.c_void_p, ct.c_int32]
libcsound.csoundFlushCircularBuffer.argtypes = [CSOUND_p, ct.c_void_p]
libcsound.csoundDestroyCircularBuffer.argtypes = [CSOUND_p, ct.c_void_p]

OPENSOUNDFILEFUNC = ct.CFUNCTYPE(ct.c_void_p, CSOUND_p, ct.c_char_p, ct.c_int32, ct.c_void_p)
libcsound.csoundSetOpenSoundFileCallback.argtypes = [CSOUND_p, OPENSOUNDFILEFUNC]
OPENFILEFUNC = ct.CFUNCTYPE(ct.c_void_p, CSOUND_p, ct.c_char_p, ct.c_char_p)
libcsound.csoundSetOpenFileCallback.argtypes = [CSOUND_p, OPENFILEFUNC]

#
# UGen API types and function signatures
#

# UGEN_ARG_TYPE enum values
UGEN_ARG_TYPE_I       = 0
UGEN_ARG_TYPE_K       = 1
UGEN_ARG_TYPE_A       = 2
UGEN_ARG_TYPE_S       = 3
UGEN_ARG_TYPE_F       = 4
UGEN_ARG_TYPE_UNKNOWN = 5

class UgenOpcodeInfo(ct.Structure):
    """Mirrors the C UGEN_OPCODE_INFO struct."""
    _fields_ = [("opname", ct.c_char_p),
                ("outypes", ct.c_char_p),
                ("intypes", ct.c_char_p),
                ("dsblksiz", ct.c_size_t),
                ("flags", ct.c_int32)]

# Factory API
libcsound.csoundUgenFactoryNew.restype = UGEN_FACTORY_p
libcsound.csoundUgenFactoryNew.argtypes = [CSOUND_p]
libcsound.csoundUgenFactoryDelete.restype = ct.c_bool
libcsound.csoundUgenFactoryDelete.argtypes = [UGEN_FACTORY_p]

# Context API
libcsound.csoundUgenContextNew.restype = UGEN_CONTEXT_p
libcsound.csoundUgenContextNew.argtypes = [UGEN_FACTORY_p]
libcsound.csoundUgenContextDelete.restype = ct.c_bool
libcsound.csoundUgenContextDelete.argtypes = [UGEN_CONTEXT_p]
libcsound.csoundUgenSetContext.restype = ct.c_bool
libcsound.csoundUgenSetContext.argtypes = [UGEN_p, UGEN_CONTEXT_p]

# UGen creation/destruction
libcsound.csoundUgenNew.restype = UGEN_p
libcsound.csoundUgenNew.argtypes = [UGEN_FACTORY_p, ct.c_char_p, ct.c_char_p, ct.c_char_p]
libcsound.csoundUgenDelete.restype = ct.c_bool
libcsound.csoundUgenDelete.argtypes = [UGEN_p]

# Argument handling: by pointer
libcsound.csoundUgenSetOutput.restype = ct.c_bool
libcsound.csoundUgenSetOutput.argtypes = [UGEN_p, ct.c_int32, ct.c_void_p]
libcsound.csoundUgenSetInput.restype = ct.c_bool
libcsound.csoundUgenSetInput.argtypes = [UGEN_p, ct.c_int32, ct.c_void_p]

# Argument handling: by value
libcsound.csoundUgenSetOutputValue.restype = ct.c_bool
libcsound.csoundUgenSetOutputValue.argtypes = [UGEN_p, ct.c_int32, ct.c_void_p]
libcsound.csoundUgenSetInputValue.restype = ct.c_bool
libcsound.csoundUgenSetInputValue.argtypes = [UGEN_p, ct.c_int32, ct.c_void_p]
libcsound.csoundUgenGetOutputValue.restype = ct.c_size_t
libcsound.csoundUgenGetOutputValue.argtypes = [UGEN_p, ct.c_int32, ct.c_void_p]
libcsound.csoundUgenGetInputValue.restype = ct.c_size_t
libcsound.csoundUgenGetInputValue.argtypes = [UGEN_p, ct.c_int32, ct.c_void_p]

# Argument query
libcsound.csoundUgenGetInCount.restype = ct.c_int32
libcsound.csoundUgenGetInCount.argtypes = [UGEN_p]
libcsound.csoundUgenGetOutCount.restype = ct.c_int32
libcsound.csoundUgenGetOutCount.argtypes = [UGEN_p]
libcsound.csoundUgenGetInType.restype = ct.c_int32
libcsound.csoundUgenGetInType.argtypes = [UGEN_p, ct.c_int32]
libcsound.csoundUgenGetOutType.restype = ct.c_int32
libcsound.csoundUgenGetOutType.argtypes = [UGEN_p, ct.c_int32]
libcsound.csoundUgenGetInArgSize.restype = ct.c_size_t
libcsound.csoundUgenGetInArgSize.argtypes = [UGEN_p, ct.c_int32]
libcsound.csoundUgenGetOutArgSize.restype = ct.c_size_t
libcsound.csoundUgenGetOutArgSize.argtypes = [UGEN_p, ct.c_int32]

# Init/Perform
libcsound.csoundUgenInit.restype = ct.c_int32
libcsound.csoundUgenInit.argtypes = [UGEN_p]
libcsound.csoundUgenPerform.restype = ct.c_int32
libcsound.csoundUgenPerform.argtypes = [UGEN_p]

# Opcode listing API
libcsound.csoundUgenListOpcodes.restype = ct.c_int32
libcsound.csoundUgenListOpcodes.argtypes = [UGEN_FACTORY_p, ct.POINTER(ct.POINTER(UgenOpcodeInfo)), ct.POINTER(ct.c_int32)]
libcsound.csoundUgenFreeOpcodeList.restype = None
libcsound.csoundUgenFreeOpcodeList.argtypes = [UGEN_FACTORY_p, ct.POINTER(UgenOpcodeInfo)]
libcsound.csoundUgenFindOpcode.restype = ct.c_bool
libcsound.csoundUgenFindOpcode.argtypes = [UGEN_FACTORY_p, ct.c_char_p, ct.c_char_p, ct.c_char_p]

# Graph API
libcsound.csoundUgenGraphNew.restype = UGEN_GRAPH_p
libcsound.csoundUgenGraphNew.argtypes = [UGEN_FACTORY_p]
libcsound.csoundUgenGraphAdd.restype = ct.c_int32
libcsound.csoundUgenGraphAdd.argtypes = [UGEN_GRAPH_p, UGEN_p]
libcsound.csoundUgenGraphConnect.restype = ct.c_bool
libcsound.csoundUgenGraphConnect.argtypes = [UGEN_p, ct.c_int32, UGEN_p, ct.c_int32]
libcsound.csoundUgenGraphInit.restype = ct.c_int32
libcsound.csoundUgenGraphInit.argtypes = [UGEN_GRAPH_p]
libcsound.csoundUgenGraphPerform.restype = ct.c_int32
libcsound.csoundUgenGraphPerform.argtypes = [UGEN_GRAPH_p]
libcsound.csoundUgenGraphDelete.restype = ct.c_bool
libcsound.csoundUgenGraphDelete.argtypes = [UGEN_GRAPH_p]
libcsound.csoundUgenGraphDeleteAll.restype = ct.c_bool
libcsound.csoundUgenGraphDeleteAll.argtypes = [UGEN_GRAPH_p]


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

def csound_arg_list(lst):
    if len(lst) == 1 and type(lst[0]) is list:
        lst = lst[0]
    argc = len(lst)
    argv = (ct.POINTER(ct.c_char_p) * argc)()
    for i in range(argc):
        v = cstring(lst[i])
        argv[i] = ct.cast(ct.pointer(ct.create_string_buffer(v)),
                          ct.POINTER(ct.c_char_p))
    return ct.c_int32(argc), ct.cast(argv, ct.POINTER(ct.c_char_p))


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
    def __init__(self, host_data=None, opcode_dir=None, pointer_=None):
        """Creates an instance of Csound.

        Returns an opaque pointer that must be passed to most Csound API
        functions. The host_data parameter can be None, or it can be
        any sort of data; these data can be accessed from the Csound instance
        that is passed to callback routines.
        If not None the opcode_dir parameter sets an override for
        the plugin module/opcode directory search.
        """
        if pointer_:
            self.cs = pointer_
            self.from_pointer = True
        else:
            self.cs = libcsound.csoundCreate(ct.py_object(host_data),
                                             cstring(opcode_dir))
            self.from_pointer = False

    def __del__(self):
        """Destroys an instance of Csound."""
        if not self.from_pointer and libcsound:
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

    def sr(self):
        """Returns the number of audio sample frames per second."""
        return libcsound.csoundGetSr(self.cs)

    def kr(self):
        """Returns the number of control samples per second."""
        return libcsound.csoundGetKr(self.cs)

    def ksmps(self):
        """Returns the audio vector size in frames (= sr/kr)."""
        return libcsound.csoundGetKsmps(self.cs)

    def channels(self, is_input=False):
        """Returns the number of audio channels in the Csound instance.

        If is_input = False, the value of nchnls is returned,
        otherwise nchnls_i. If this variable is not set,
        the value is always taken from nchnls.
        """
        return libcsound.csoundGetChannels(self.cs, ct.c_int32(is_input))

    def get_0dBFS(self):
        """Returns the 0dBFS level of the spin/spout buffers."""
        return libcsound.csoundGet0dBFS(self.cs)

    def A4(self):
        """Returns the A4 frequency reference."""
        return libcsound.csoundGetA4(self.cs)

    def current_time_samples(self):
        """Returns the current performance time in sample frames."""
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

        The searching order is: local environment of Csound ,
        variables set with set_global_env(), and system environment variables.
        Should be called after compile_().
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
        """Set csound options (flag).

        This needs to be called before any code is compiled.
        Multiple options are allowed in one string.
        Returns zero on success.
        """
        return libcsound.csoundSetOption(self.cs, cstring(option))

    def params(self, params):
        """Gets the current set of parameters from a CSOUND instance.

        These parameters are in a CsoundParams structure. See set_params():
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

        The debug argument must have value True or False.
        Those messages come from the DebugMsg() internal API function.
        """
        libcsound.csoundSetDebug(self.cs, ct.c_int32(debug))

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
                print("Module {}: {} ({})\n".format(n, name, type_))
                n += 1
        """
        name = ct.pointer(ct.c_char_p(cstring("dummy")))
        type_ = ct.pointer(ct.c_char_p(cstring("dummy")))
        err = libcsound.csoundGetModule(self.cs, ct.c_int32(number), name, type_)
        if err == CSOUND_ERROR:
            return None, None, err
        n = pstring(ct.string_at(name.contents))
        t = pstring(ct.string_at(type_.contents))
        return n, t, err

    def audio_dev_list(self, is_output=False):
        """Returns a list of available input or output audio devices.

        Each item in the list is a dictionnary representing a device. The
        dictionnary keys are device_name, device_id, rt_module (value
        type string), max_nchnls (value type int), and isOutput (value
        type boolean).

        Must be called after an orchestra has been compiled
        to get meaningful information.
        """
        n = libcsound.csoundGetAudioDevList(self.cs, None, ct.c_int32(is_output))
        devs = (CsoundAudioDevice * n)()
        libcsound.csoundGetAudioDevList(self.cs, ct.byref(devs), ct.c_int32(is_output))
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

    def midi_dev_list(self, is_output=False):
        """Returns a list of available input or output midi devices.

        Each item in the list is a dictionnary representing a device. The
        dictionnary keys are device_name, interface_name, device_id,
        midi_module (value type string), isOutput (value type boolean).

        Must be called after an orchestra has been compiled
        to get meaningful information.
        """
        n = libcsound.csoundGetMIDIDevList(self.cs, None, ct.c_int32(is_output))
        devs = (CsoundMidiDevice * n)()
        libcsound.csoundGetMIDIDevList(self.cs, ct.byref(devs), ct.c_int32(is_output))
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
        libcsound.csoundSetMessageLevel(self.cs, ct.c_int32(message_level))

    #
    # Performance
    #
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

    def compile_orc(self, orc, async_=False):
        """Parses, and compiles the given orchestra given on a string.

        Also evaluating any global space code (i-time only)
        in synchronous or asynchronous (async_ = True) mode.

            orc = "instr 1 \n a1 rand 0dbfs/4 \n out a1 \n endin \n"
            cs.compile_orc(orc)
        """
        return libcsound.csoundCompileOrc(self.cs, cstring(orc), ct.c_int32(async_))

    def eval_code(self, code):
        """Parses and compiles an orchestra given on an string, synchronously.

        It evaluates synchronously any global space code (i-time only).
        On SUCCESS it returns a value passed to the
        'return' opcode in global space:

            code = 'i1 = 2 + 2 \n return i1 \n'
            retval = cs.eval_code(code)
        """
        return libcsound.csoundEvalCode(self.cs, cstring(code))

    def compile_csd(self, csd, mode, async_ = False):
        """Compiles a Csound input file (.csd file) or a text string.

        In synchronous or asynchronous (async_ = True) mode.
        Returns a non-zero error code on failure.

        If start is called before compile_csd, the <CsOptions>
        element is ignored (but set_option can be called any number of
        times), the <CsScore> element is dispatched as  score events (e.g.
        as it is done by event_string()).

            cs.set_option("option(s)")
            cs.start()
            cs.compile_csd(csd_filename, 0)
            while True:
               cs.perform_ksmps()
               # Something to break out of the loop
               # when finished here...

        NB: this function can be called repeatedly during performance to
        replace or add new instruments and events.

        But if compile_csd is called before start, the <CsOptions>
        element is used, the <CsScore> section is pre-processed and dispatched
        normally, and performance terminates when the score terminates.

            cs.compile_csd(csd_filename, 0)
            cs.start()
            while True:
                finished = cs.perform_ksmps()
                if finished:
                    break

        if mode = 1, csd contains a full CSD code (rather than a filename).
        This is convenient when it is desirable to package the csd as part of
        an application or a multi-language piece.
        """
        return libcsound.csoundCompileCSD(self.cs, cstring(csd), ct.c_int32(mode), ct.c_int32(async_))

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
        """Senses input events, and performs one block of audio output.

        The block contains ksmps frames. start() must be called first.
        Returns False during performance, and True when performance is finished.
        If called until it returns True, it will perform an entire score.
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
        """Resets all internal memory and state in preparation for a new performance.

        Enables external software to run successive Csound performances
        without reloading Csound.
        """
        libcsound.csoundReset(self.cs)

    #
    # Audio I/O
    #
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
        """Returns the Csound audio output working buffer (spout) as an ndarray.

        Enables external software to read audio from Csound after
        calling perform_ksmps().
        """
        buf = libcsound.csoundGetSpout(self.cs)
        size = self.ksmps() * self.channels()
        arrayType = np.ctypeslib.ndpointer(MYFLT, 1, (size,), 'C_CONTIGUOUS')
        p = ct.cast(buf, arrayType)
        return array_from_pointer(p)

    #
    # Csound Messages and Text
    #
    def message(self, fmt, *args):
        """Displays an informational message.

        This is a workaround because we do not support variadic functions.
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
        This is a workaround because we do not support variadic functions.
        The arguments are formatted in a string, using the python way, either
        old style or new style, and then this formatted string is passed to
        the csound display message system.
        """
        if fmt[0] == '{':
            s = fmt.format(*args)
        else:
            s = fmt % args
        libcsound.csoundMessageS(self.cs, ct.c_int32(attr), cstring("%s"), cstring(s))

    def set_message_string_callback(self, attr, function):
        """Sets an alternative message print function.

        This function is to be called by Csound to print an
        informational message, using a less granular signature.
        This callback can be set for --realtime mode.
        This callback is cleared after reset.
        """
        self.message_string_cb = MSGSTRFUNC(function)
        libcsound.csoundSetMessageStringCallback(self.cs, ct.c_int32(attr),
            self.message_string_cb)

    def create_message_buffer(self, to_stdout):
        """Creates a buffer for storing messages printed by Csound.

        Should be called after creating a Csound instance and the buffer
        can be freed by calling destroy_message_buffer() before
        deleting the Csound instance.

        If to_stdout is True, the messages are also printed to
        stdout and stderr (depending on the type of the message),
        in addition to being stored in the buffer.

        Using the message buffer ties up the internal message callback, so
        set_message_callback() should not be called after creating the
        message buffer.
        """
        libcsound.csoundCreateMessageBuffer(self.cs, ct.c_int32(to_stdout))

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
            string data as a STRINGDAT structure - (STRINGDAT **) pp
            (see string_data() and set_string_data())
        CSOUND_ARRAY_CHANNEL
            array data as an ARRAYDAT structure - (ARRAYDAT **) pp
            (see array_data(), set_array_data(), and init_array_channel())
        CSOUND_PVS_CHANNEL
            pvs data as a PVSDATEXT structure - (PVSDATEXT **) pp
            (see pvs_data(), set_pvs_data(), and init_pvs_channel())
        and at least one of these:

        CSOUND_INPUT_CHANNEL
        CSOUND_OUTPUT_CHANNEL

        If the channel is a control or an audio channel, the pointer is
        translated to an ndarray of MYFLT. If the channel is a string channel,
        the pointer is casted to ct.c_char_p. The error message is either
        an empty string or a string describing the error that occured.

        If the channel already exists, it must match the data type
        (control, string, audio, pvs or array), however, the input/output bits
        are OR'd with the new value. Note that audio and string channels
        can only be created after calling compile_(), because the
        storage size is not known until then.

        Return value is zero on success, or a negative error code,

        CSOUND_MEMORY
            there is not enough memory for allocating the channel
        CSOUND_ERROR
            the specified name or type is invalid

        or, if a channel with the same name but incompatible type
        already exists, the type of the existing channel. In the case
        of any non-zero return value, the pointer is set to None.
        Note: to find out the type of a channel without actually
        creating or changing it, set type_ to zero, so that the return
        value will be either the type of the channel, or CSOUND_ERROR
        if it does not exist.

        Operations on the pointer are not thread-safe by default. The host is
        required to take care of threadsafety by using lock_channel() and
        unlock_channel() to protect access to the pointer.

        See Top/threadsafe.c in the Csound library sources for
        examples. Optionally, use the channel get/set functions
        provided below, which are threadsafe by default.
        """
        length = 1  # default buf length for CSOUND_CONTROL_CHANNEL:
        ptr = ct.c_void_p()
        chan_type = type_ & CSOUND_CHANNEL_TYPE_MASK
        err = ''
        ret = libcsound.csoundGetChannelPtr(self.cs, ct.byref(ptr), cstring(name), type_)
        if ret == CSOUND_SUCCESS:
            if chan_type == CSOUND_STRING_CHANNEL:
                return ct.cast(ptr, STRINGDAT_p), err
            elif chan_type == CSOUND_ARRAY_CHANNEL:
                return ct.cast(ptr, ARRAYDAT_p), err
            elif chan_type == CSOUND_PVS_CHANNEL:
                return ct.cast(ptr, PVSDAT_p), err
            elif chan_type == CSOUND_AUDIO_CHANNEL:
                length = libcsound.csoundGetKsmps(self.cs)
            array_type = np.ctypeslib.ndpointer(MYFLT, 1, (length,), 'C_CONTIGUOUS')
            p = ct.cast(ptr, array_type)
            return array_from_pointer(p), err

        if ret == CSOUND_MEMORY:
            err = 'Not enough memory for allocating channel'
        elif ret == CSOUND_ERROR:
            err = 'The specified channel name or type is not valid'
        elif ret == CSOUND_CONTROL_CHANNEL:
            err = 'A control channel named {} already exists'.format(name)
        elif ret == CSOUND_AUDIO_CHANNEL:
            err = 'An audio channel named {} already exists'.format(name)
        elif ret == CSOUND_STRING_CHANNEL:
            err = 'A string channel named {} already exists'.format(name)
        elif ret == CSOUND_ARRAY_CHANNEL:
            err = 'An array channel named {} already exists'.format(name)
        elif ret == CSOUND_PVS_CHANNEL:
            err = 'A PVS channel named {} already exists'.format(name)
        else:
            err = 'Unknown error'
        return None, err

    def channel_var_type_name(self, name):
        """Returns the var type for a channel name.

        Returns None if the channel was not found.
        Currently supported channel var types are 'k' (control), 'a' (audio),
        'S' (string), 'f' (pvs), and '[' (array).
        """
        ret = libcsound.csoundGetChannelVarTypeName(self.cs, cstring(name))
        if ret:
            return cstring(ret)
        return None

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
        ret = libcsound.csoundGetControlChannelHints(self.cs, cstring(name),
            ct.byref(hints))
        if ret != CSOUND_SUCCESS:
            hints = None
        return hints, ret

    def lock_channel(self, channel):
        """Locks access to the channel.

        Access to data is allowed in a threadsafe manner.
        """
        libcsound.csoundLockChannel(self.cs, cstring(channel))

    def unlock_channel(self, channel):
        """Unlocks access to the channel.

        It allows access to data from elsewhere.
        """
        libcsound.csoundUnlockChannel(self.cs, cstring(channel))

    def control_channel(self, name):
        """Retrieves the value of control channel identified by name.

        A second value is returned, which, if not None, is the error
        (or success) code finding or accessing the channel.
        """
        err = ct.c_int32(0)
        ret = libcsound.csoundGetControlChannel(self.cs, cstring(name), ct.byref(err))
        if bool(err) == False:
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

    def init_array_channel(self, name, type_, sizes):
        """Create and initialise an array channel with a given array type.

        - "a" (audio sigs): each item is a ksmps-size MYFLT array
        - "i" (init vars): each item is a MYFLT
        - "S" (strings): each item is a STRINGDAT_p (see string_data() and
          set_string_data())
        - "k" (control sigs): each item is a MYFLT
        sizes - sizes for each dimension
        returns the ARRAYDAT_p for the requested channel or None on error
        NB: if the channel exists and has already been initialised,
        this function is a non-op.
        """
        sz = np.array(sizes).astype(ct.c_int)
        sz_ptr = sz.ctypes.data_as(ct.POINTER(ct.c_int))
        return libcsound.csoundInitArrayChannel(self.cs, cstring(name),
            cstring(type_), sz.size, sz_ptr)

    def array_data_type(self, adat):
        """Get the type of data the ARRAYDAT adat.

        It returns
        - "a" (audio sigs): each item is a ksmps-size MYFLT array
        - "i" (init vars): each item is a MYFLT
        - "S" (strings): each item is a STRINGDAT (see string_data() and
          set_string_data()
        - "k" (control sigs): each item is a MYFLT
        """
        return pstring(libcsound.csoundArrayDataType(adat))

    def array_data_dimensions(self, adat):
        """Get the dimensions of the ARRAYDAT adat."""
        return libcsound.csoundArrayDataDimensions(adat)

    def array_data_sizes(self, adat):
        """Get the sizes of each dimension of the ARRAYDAT adat."""
        sizes = libcsound.csoundArrayDataSizes(adat)
        dims = libcsound.csoundArrayDataDimensions(adat)
        array_type = np.ctypeslib.ndpointer(ct.c_int, 1, (dims,), 'C_CONTIGUOUS')
        p = ct.cast(sizes, array_type)
        return array_from_pointer(p)

    def set_array_data(self, adat, data):
        """Set the data in the ARRAYDAT adat."""
        libcsound.csoundSetArrayData(adat, data)

    def array_data(self, adat):
        """Get the data from the ARRAYDAT adat."""
        return libcsound.csoundGetArrayData(adat)

    # These two functions are using c void * for the data.
    # Not very useful in Python. To be refined.
    def string_data(self, sdata):
        """Get a null-terminated string from a STRINGDAT structure."""
        return pstring(libcsound.csoundGetStringData(self.cs, sdata))

    def set_string_data(self, sdata, string):
        """Set a STRINGDAT structure with a null-terminated string."""
        libcsound.csoundSetStringData(self.cs, sdata, cstring(string))

    def init_pvs_channel(self, name, size, overlap, winsize, wintype, format):
        """Create/initialise an Fsig channel.

        size - FFT analysis size
        overlap - analysis overlap size
        winsize - analysis window size
        wintype - analysis window type (see pvsdat types enumeration)
        format - analysis data format (see pvsdat format enumeration)
        returns the PVSDAT for the requested channel or None on error.
        NB: if the channel exists and has already been initialised,
        this function is a non-op.
        """
        return libcsound.csoundInitPvsChannel(self.cs, cstring(name),
            size, overlap, winsize, wintype, format)

    def pvs_data_fft_size(self, pvsdat):
        """Get the analysis FFT size used by the PVSDAT pvsdat."""
        return libcsound.csoundPvsDataFFTSize(pvsdat)

    def pvs_data_overlap(self, pvsdat):
        """Get the analysis overlap size used by the PVSDAT pvsdat."""
        return libcsound.csoundPvsDataOverlap(pvsdat)

    def pvs_data_window_size(self, pvsdat):
        """Get the analysis window size used by the PVSDAT pvsdat."""
        return libcsound.csoundPvsDataWindowSize(pvsdat)

    def pvs_data_format(self, pvsdat):
        """Get the analysis data format used by the PVSDAT pvsdat."""
        return libcsound.csoundPvsDataFormat(pvsdat)

    def pvs_data_framecount(self, pvsdat):
        """Get the current framecount from PVSDAT pvsdat."""
        return libcsound.csoundPvsDataFramecount(pvsdat)

    # These two functions are using c float * for the frame data.
    # Not very useful in Python. To be refined.
    def pvs_data(self, pvsdat):
        """Get the analysis data frame from the PVSDAT pvsdat."""
        return libcsound.csoundGetPvsData(pvsdat)

    def set_pvs_data(self, pvsdat, frame):
        """Set the analysis data frame in the PVSDAT pvsdat."""
        libcsound.csoundSetPvsData(pvsdat, frame)

    def channel_datasize(self, name):
        """Returns the size of data stored in a channel."""
        return libcsound.csoundGetChannelDatasize(self.cs, cstring(name))

    def set_input_channel_callback(self, function):
        """Sets the function to call whenever the invalue opcode is used."""
        self.input_channel_cb_ref = CHANNELFUNC(function)
        libcsound.csoundSetInputChannelCallback(self.cs, self.input_channel_cb_ref)

    def set_output_channel_callback(self, function):
        """Sets the function to call whenever the outvalue opcode is used."""
        self.output_channel_cb_ref = CHANNELFUNC(function)
        libcsound.csoundSetOutputChannelCallback(self.cs, self.output_channel_cb_ref)

    def event(self, type_, params, async_=False):
        """Send a new event. 'type_' is the event type.

        type_ 0 - instrument instance     CS_INSTR_EVENT
        type_ 1 - function table instance CS_TABLE_EVENT
        type_ 2 - end event               CS_END_EVENT
        event parameters is a tuple, a list, or an ndarray of MYFLTs with all
        the pfields for this event parameters (p-fields)
        optionally run asynchronously (async_ = True)
        """
        p = np.asarray(params, dtype=MYFLT)
        ptr = p.ctypes.data_as(ct.POINTER(MYFLT))
        n_fields = ct.c_int32(p.size)
        libcsound.csoundEvent(self.cs, ct.c_int32(type_), ptr, n_fields,
            ct.c_int32(async_))

    def event_string(self, message, async_=False):
        """Schedule new score or realtime event(s) as a string.

       Two operation modes are supported:
       - Score events: any calls before start() add the string events to
       - the score (before pre-processing) (async_ should be set to False).
       - Realtime events: after the engine starts, string events are added to
         the realtime event queue.

        Multiple events separated by newlines are possible
        and score preprocessing (carry, etc) is applied.
        Optionally run asynchronously (async_ = True).
        """
        libcsound.csoundEventString(self.cs, cstring(message),
            ct.c_int32(async_))

    def instr_number(self, name):
        """Get the instrument number for a given instrument name string.

        For use in numeric parameters list (event()).
        Returns the instrument number or -1 if not found.
        """
        return int(libcsound.csoundGetInstrNumber(self.cs, cstring(name)))

    def key_press(self, c):
        """Sets the ASCII code of the most recent key pressed.

        This value is used by the sensekey opcode if a callback for
        returning keyboard events is not set (see
        register_keyboard_callback()).
        """
        libcsound.csoundKeyPress(self.cs, cchar(c))

    def register_keyboard_callback(self, function, user_data, type_):
        """Registers general purpose callback functions for keyboard events.

        These callbacks will be called to query keyboard events. They
        are called on every control period by the sensekey opcode.

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
        return libcsound.csoundRegisterKeyboardCallback(self.cs,
            KEYBOARDFUNC(function),
            ct.py_object(user_data), ct.c_uint(type_))

    def remove_keyboard_callback(self, function):
        """Removes a callback previously set with register_keyboard_callback()."""
        libcsound.csoundRemoveKeyboardCallback(self.cs, KEYBOARDFUNC(function))

    #
    # Tables
    #
    def table_length(self, table):
        """Returns the length of a function table. (Not including the guard point).

        If the table does not exist, returns -1.
        """
        return libcsound.csoundTableLength(self.cs, ct.c_int32(table))

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
        return array_from_pointer(p)

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
        return array_from_pointer(p)

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
        """Sets whether Csound score events are performed or not.

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
        libcsound.csoundSetScorePending(self.cs, ct.c_int32(pending))

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
    # Table Display
    #
    def set_is_graphable(self, is_graphable):
        """Tells Csound whether external graphic table display is supported.

        Return the previously set value (initially False).
        """
        ret = libcsound.csoundSetIsGraphable(self.cs, ct.c_int32(is_graphable))
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
    # Circular Buffer Functions
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

    def set_open_sound_file_callback(self, function):
        """Sets a callback for opening a sound file.

        The callback is made when a sound file is going to be opened.
        The following information is passed to the callback:

        string
            pathname of the file; either full or relative to current dir
        int
            access flags for the file.
        ptr
            sound file info of the file.

        Pass None to disable the callback.
        This callback is retained after a reset() call.
        """
        self.open_sound_file_cb_ref = OPENSOUNDFILEFUNC(function)
        libcsound.csoundSetOpenSoundFileCallback(self.cs, self.open_sound_file_cb_ref)

    def set_open_file_callback(self, function):
        """Sets a callback for opening a file.

        The callback is made when a file is going to be opened.
        The following information is passed to the callback:

        string
            pathname of the file; either full or relative to current dir
        string
            access mode of the file.

        Pass None to disable the callback.
        This callback is retained after a reset() call.
        """
        self.open_file_cb_ref = OPENFILEFUNC(function)
        libcsound.csoundSetOpenFileCallback(self.cs, self.open_file_cb_ref)


CSOUNDPERFTHREAD_p = ct.c_void_p

libcspt = libcsound
libcspt.csoundCreatePerformanceThread.restype = CSOUNDPERFTHREAD_p
libcspt.csoundCreatePerformanceThread.argtypes = [CSOUND_p]
libcspt.csoundDestroyPerformanceThread.argtypes = [CSOUNDPERFTHREAD_p]
libcspt.csoundPerformanceThreadIsRunning.restype = ct.c_int32
libcspt.csoundPerformanceThreadIsRunning.argtypes = [CSOUNDPERFTHREAD_p]
PROCESSFUNC = ct.CFUNCTYPE(None, ct.c_void_p)
libcspt.csoundPerformanceThreadGetProcessCB.restype = ct.c_void_p
libcspt.csoundPerformanceThreadGetProcessCB.argtypes = [CSOUNDPERFTHREAD_p]
libcspt.csoundPerformanceThreadSetProcessCB.argtypes = [CSOUNDPERFTHREAD_p, PROCESSFUNC, ct.c_void_p]
libcspt.csoundPerformanceThreadGetCsound.restype = CSOUND_p
libcspt.csoundPerformanceThreadGetCsound.argtypes = [CSOUNDPERFTHREAD_p]
libcspt.csoundPerformanceThreadGetStatus.restype = ct.c_int32
libcspt.csoundPerformanceThreadGetStatus.argtypes = [CSOUNDPERFTHREAD_p]
libcspt.csoundPerformanceThreadPlay.argtypes = [CSOUNDPERFTHREAD_p]
libcspt.csoundPerformanceThreadPause.argtypes = [CSOUNDPERFTHREAD_p]
libcspt.csoundPerformanceThreadTogglePause.argtypes = [CSOUNDPERFTHREAD_p]
libcspt.csoundPerformanceThreadStop.argtypes = [CSOUNDPERFTHREAD_p]
libcspt.csoundPerformanceThreadRecord.argtypes = [CSOUNDPERFTHREAD_p, ct.c_char_p, ct.c_int32, ct.c_int32]
libcspt.csoundPerformanceThreadStopRecord.argtypes = [CSOUNDPERFTHREAD_p]
libcspt.csoundPerformanceThreadScoreEvent.argtypes = [CSOUNDPERFTHREAD_p, ct.c_int32, ct.c_char, ct.c_int32, ct.POINTER(MYFLT)]
libcspt.csoundPerformanceThreadInputMessage.argtypes = [CSOUNDPERFTHREAD_p, ct.c_char_p]
libcspt.csoundPerformanceThreadSetScoreOffsetSeconds.argtypes = [CSOUNDPERFTHREAD_p, ct.c_double]
libcspt.csoundPerformanceThreadJoin.restype = ct.c_int32
libcspt.csoundPerformanceThreadJoin.argtypes = [CSOUNDPERFTHREAD_p]
libcspt.csoundPerformanceThreadFlushMessageQueue.argtypes = [CSOUNDPERFTHREAD_p]


class CsoundPerformanceThread:
    """Runs Csound in a separate thread.

    The playback (which is paused by default) is stopped by calling
    stop(), or if an error occurs.
    The constructor takes a Csound instance pointer as argument; it assumes
    that ctcsound.compile_() was called successfully before creating
    the performance thread. Once the playback is stopped for one of the above
    mentioned reasons, the performance thread returns.
    """
    def __init__(self, csp):
        self.cpt = libcspt.csoundCreatePerformanceThread(csp)

    def __del__(self):
        libcspt.csoundDestroyPerforma#
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

    def is_running(self):
        """Returns True if the performance thread is running, False otherwise."""
        return libcspt.csoundPerformanceThreadIsRunning(self.cpt) != 0

    def process_cb(self):
        """Returns the process callback."""
        return PROCESSFUNC(libcspt.csoundPerformanceThreadGetProcessCB(self.cpt))

    def set_process_cb(self, function, data):
        """Sets the process callback."""
        libcspt.csoundPerformanceThreadSetProcessCB(self.cpt, PROCESSFUNC(function), ct.byref(data))

    def csound(self):
        """Returns the Csound instance pointer."""
        return libcspt.csoundPerformanceThreadGetCsound(self.cpt)

    def status(self):
        """Returns the current status.

        Zero if still playing, positive if the end of score was reached or
        performance was stopped, and negative if an error occured.
        """
        return libcspt.csoundPerformanceThreadGetStatus(self.cpt)

    def play(self):
        """Continues performance if it was paused."""
        libcspt.csoundPerformanceThreadPlay(self.cpt)

    def pause(self):
        """Pauses performance (can be continued by calling play())."""
        libcspt.csoundPerformanceThreadPause(self.cpt)

    def toggle_pause(self):
        """Pauses or continues performance, depending on current state."""
        libcspt.csoundPerformanceThreadtogglePause(self.cpt)

    def stop(self):
        """Stops performance (cannot be continued)."""
        libcspt.csoundPerformanceThreadStop(self.cpt)

    def record(self, filename, samplebits, numbufs):
        """Starts recording the output from Csound.

        The sample rate and number of channels are taken directly from the
        running Csound instance.
        """
        libcspt.csoundPerformanceThreadRecord(self.cpt, cstring(filename), samplebits, numbufs)

    def stop_record(self):
        """Stops recording and closes audio file."""
        libcspt.csoundPerformanceThreadStopRecord(self.cpt)

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
        libcspt.csoundPerformanceThreadScoreEvent(self.cpt, ct.c_int32(absp2mode), cchar(opcod), numFields, ptr)

    def input_message(self, s):
        """Sends a score event as a string, similarly to line events (-L)."""
        libcspt.csoundPerformanceThreadInputMessage(self.cpt, cstring(s))

    def set_score_offset_seconds(self, timeVal):
        """Sets the playback time pointer to the specified value (in seconds)."""
        libcspt.csoundPerformanceThreadSetScoreOffsetSeconds(self.cpt, ct.c_double(timeVal))

    def join(self):
        """Waits until the performance is finished or fails.

        Returns a positive value if the end of score was reached or
        stop() was called, and a negative value if an error occured.
        Also releases any resources associated with the performance thread
        object.
        """
        return libcspt.csoundPerformanceThreadJoin(self.cpt)

    def flush_message_queue(self):
        """Waits until all pending messages are actually received.

        (pause, send score event, etc.)
        """
        libcspt.csoundPerformanceThreadFlushMessageQueue(self.cpt)


# =============================================================================
#  UGen API wrapper classes
# =============================================================================

class UgenFactory:
    """Creates and manages UGen (opcode) instances.

    The Csound instance should be configured for sr and ksmps before
    creating a factory.

    Example::

        cs = Csound()
        cs.compile_orc("instr 1\\nendin", 0)
        cs.start()
        factory = UgenFactory(cs)
        osc = factory.new_ugen("oscils", "a", "iiio")
        ...
        del factory  # or factory.delete()
    """

    def __init__(self, csound):
        """Create a new UgenFactory from a Csound instance.

        Args:
            csound: A Csound instance (the ctcsound.Csound object).
        """
        self.cs = csound
        self.factory = libcsound.csoundUgenFactoryNew(csound.cs)
        if not self.factory:
            raise RuntimeError("Failed to create UGEN_FACTORY")

    def __del__(self):
        self.delete()

    def delete(self):
        """Free the factory. Safe to call multiple times."""
        if self.factory:
            libcsound.csoundUgenFactoryDelete(self.factory)
            self.factory = None

    def new_ugen(self, op_name, out_types, in_types):
        """Create a new Ugen for the named opcode.

        Args:
            op_name:   Opcode name (e.g. "oscils").
            out_types: Output type string (e.g. "a").
            in_types:  Input type string (e.g. "iiio").

        Returns:
            A Ugen instance, or None if the opcode/types could not be resolved.
        """
        ptr = libcsound.csoundUgenNew(
            self.factory,
            cstring(op_name), cstring(out_types), cstring(in_types))
        return Ugen(ptr, self) if ptr else None

    def list_opcodes(self):
        """Return a list of available opcodes as dicts.

        Each dict has keys: opname, outypes, intypes, dsblksiz, flags.
        """
        info_p = ct.POINTER(UgenOpcodeInfo)()
        count = ct.c_int32(0)
        ret = libcsound.csoundUgenListOpcodes(
            self.factory, ct.byref(info_p), ct.byref(count))
        if ret != 0:
            return []
        result = []
        for i in range(count.value):
            entry = info_p[i]
            result.append({
                'opname': pstring(entry.opname) if entry.opname else '',
                'outypes': pstring(entry.outypes) if entry.outypes else '',
                'intypes': pstring(entry.intypes) if entry.intypes else '',
                'dsblksiz': entry.dsblksiz,
                'flags': entry.flags,
            })
        libcsound.csoundUgenFreeOpcodeList(self.factory, info_p)
        return result

    def find_opcode(self, op_name, out_types=None, in_types=None):
        """Check if an opcode with the given name and types exists.

        Args:
            op_name:   Opcode name.
            out_types: Output type string, or None for any.
            in_types:  Input type string, or None for any.

        Returns:
            True if the opcode exists, False otherwise.
        """
        return libcsound.csoundUgenFindOpcode(
            self.factory,
            cstring(op_name),
            cstring(out_types) if out_types else None,
            cstring(in_types) if in_types else None)

    def new_graph(self):
        """Create a new empty UgenGraph."""
        ptr = libcsound.csoundUgenGraphNew(self.factory)
        return UgenGraph(ptr, self) if ptr else None

    def new_context(self):
        """Create a new UgenContext for instrument-like state."""
        ptr = libcsound.csoundUgenContextNew(self.factory)
        return UgenContext(ptr) if ptr else None


class Ugen:
    """A single instantiated Csound opcode (unit generator).

    Created via UgenFactory.new_ugen(). Do not instantiate directly.
    """

    def __init__(self, ptr, factory=None):
        self.ugen = ptr
        self._factory = factory  # prevent GC of factory while ugen alive
        self._context = None     # set by set_context()

    def __del__(self):
        self.delete()

    def delete(self):
        """Free the ugen. Safe to call multiple times."""
        if self.ugen:
            libcsound.csoundUgenDelete(self.ugen)
            self.ugen = None

    # Argument handling: by pointer (zero-copy)

    def set_output(self, index, ptr):
        """Set output argument pointer (zero-copy wiring)."""
        return libcsound.csoundUgenSetOutput(self.ugen, index, ptr)

    def set_input(self, index, ptr):
        """Set input argument pointer (zero-copy wiring)."""
        return libcsound.csoundUgenSetInput(self.ugen, index, ptr)

    # Argument handling: by value (copy)

    def set_output_value(self, index, value):
        """Copy a scalar MYFLT value into the output argument at index.

        Only valid for i-rate and k-rate arguments.

        Raises:
            ValueError: If the argument at *index* is audio-rate.
        """
        if libcsound.csoundUgenGetOutType(self.ugen, index) == UGEN_ARG_TYPE_A:
            raise ValueError(
                f"set_output_value() cannot be used on a-rate output {index}; "
                "use set_output() with a buffer pointer instead")
        v = MYFLT(value)
        return libcsound.csoundUgenSetOutputValue(self.ugen, index, ct.byref(v))

    def set_input_value(self, index, value):
        """Copy a scalar MYFLT value into the input argument at index.

        Only valid for i-rate and k-rate arguments.

        Raises:
            ValueError: If the argument at *index* is audio-rate.
        """
        if libcsound.csoundUgenGetInType(self.ugen, index) == UGEN_ARG_TYPE_A:
            raise ValueError(
                f"set_input_value() cannot be used on a-rate input {index}; "
                "use set_input() with a buffer pointer instead")
        v = MYFLT(value)
        return libcsound.csoundUgenSetInputValue(self.ugen, index, ct.byref(v))

    def get_output_value(self, index):
        """Read a scalar output argument value at index.

        Returns the value as a float, or None on error.

        Raises:
            ValueError: If the argument at *index* is audio-rate.
        """
        if libcsound.csoundUgenGetOutType(self.ugen, index) == UGEN_ARG_TYPE_A:
            raise ValueError(
                f"get_output_value() cannot be used on a-rate output {index}; "
                "use get_output_buffer() instead")
        v = MYFLT(0)
        sz = libcsound.csoundUgenGetOutputValue(self.ugen, index, ct.byref(v))
        return float(v.value) if sz > 0 else None

    def get_input_value(self, index):
        """Read a scalar input argument value at index.

        Returns the value as a float, or None on error.

        Raises:
            ValueError: If the argument at *index* is audio-rate.
        """
        if libcsound.csoundUgenGetInType(self.ugen, index) == UGEN_ARG_TYPE_A:
            raise ValueError(
                f"get_input_value() cannot be used on a-rate input {index}; "
                "use get_input_buffer() instead")
        v = MYFLT(0)
        sz = libcsound.csoundUgenGetInputValue(self.ugen, index, ct.byref(v))
        return float(v.value) if sz > 0 else None

    def get_output_buffer(self, index, ksmps):
        """Read an audio-rate output buffer at index.

        Args:
            index: output argument index.
            ksmps: number of samples per k-cycle.

        Returns:
            A list of floats (length ksmps), or None on error.
        """
        buf = (MYFLT * ksmps)()
        sz = libcsound.csoundUgenGetOutputValue(self.ugen, index, ct.byref(buf))
        return list(buf) if sz > 0 else None

    def get_input_buffer(self, index, ksmps):
        """Read an audio-rate input buffer at index.

        Args:
            index: input argument index.
            ksmps: number of samples per k-cycle.

        Returns:
            A list of floats (length ksmps), or None on error.
        """
        buf = (MYFLT * ksmps)()
        sz = libcsound.csoundUgenGetInputValue(self.ugen, index, ct.byref(buf))
        return list(buf) if sz > 0 else None

    # Query

    @property
    def in_count(self):
        """Number of input arguments."""
        return libcsound.csoundUgenGetInCount(self.ugen)

    @property
    def out_count(self):
        """Number of output arguments."""
        return libcsound.csoundUgenGetOutCount(self.ugen)

    def get_in_type(self, index):
        """Get the UGEN_ARG_TYPE for input argument at index."""
        return libcsound.csoundUgenGetInType(self.ugen, index)

    def get_out_type(self, index):
        """Get the UGEN_ARG_TYPE for output argument at index."""
        return libcsound.csoundUgenGetOutType(self.ugen, index)

    def get_in_arg_size(self, index):
        """Get the byte size of input argument at index."""
        return libcsound.csoundUgenGetInArgSize(self.ugen, index)

    def get_out_arg_size(self, index):
        """Get the byte size of output argument at index."""
        return libcsound.csoundUgenGetOutArgSize(self.ugen, index)

    # Context

    def set_context(self, context):
        """Associate this ugen with a UgenContext.

        Must be called before init() if the opcode needs
        instrument-like state (hold, release, MIDI, etc.).

        Args:
            context: A UgenContext instance.
        """
        self._context = context  # prevent GC of context while ugen alive
        return libcsound.csoundUgenSetContext(self.ugen, context.ctx)

    # Init / Perform

    def init(self):
        """Run the opcode's init-pass."""
        return libcsound.csoundUgenInit(self.ugen)

    def perform(self):
        """Run the opcode's perf-pass (one ksmps block)."""
        return libcsound.csoundUgenPerform(self.ugen)


class UgenContext:
    """Provides instrument-like context (hold/release state) for UGENs.

    Created via UgenFactory.new_context(). Do not instantiate directly.
    """

    def __init__(self, ptr):
        self.ctx = ptr

    def __del__(self):
        self.delete()

    def delete(self):
        """Free the context. Safe to call multiple times."""
        if self.ctx:
            libcsound.csoundUgenContextDelete(self.ctx)
            self.ctx = None

    def set_on_ugen(self, ugen):
        """Associate this context with a Ugen."""
        return libcsound.csoundUgenSetContext(ugen.ugen, self.ctx)


class UgenGraph:
    """A graph of connected UGENs that can be performed together.

    Created via UgenFactory.new_graph(). Do not instantiate directly.
    """

    def __init__(self, ptr, factory):
        self.graph = ptr
        self.factory = factory  # prevent GC of factory
        self._ugens = []  # track Ugen wrappers added to this graph

    def __del__(self):
        self.delete()

    def delete(self):
        """Delete the graph (does NOT delete individual UGENs)."""
        if self.graph:
            libcsound.csoundUgenGraphDelete(self.graph)
            self.graph = None

    def delete_all(self):
        """Delete the graph AND all UGENs it contains."""
        if self.graph:
            libcsound.csoundUgenGraphDeleteAll(self.graph)
            self.graph = None
            # Prevent Python Ugen.__del__ from double-freeing
            for u in self._ugens:
                u.ugen = None
            self._ugens.clear()

    def add(self, ugen):
        """Add a Ugen to the graph. Returns its index, or -1 on error.

        After adding, the graph owns the UGEN memory.  Do NOT call
        ugen.delete() manually if you plan to use graph.delete_all().
        """
        idx = libcsound.csoundUgenGraphAdd(self.graph, ugen.ugen)
        if idx >= 0:
            self._ugens.append(ugen)
        return idx

    @staticmethod
    def connect(source, out_idx, dest, in_idx):
        """Wire source's output[out_idx] to dest's input[in_idx].

        Args:
            source:  Ugen instance (or raw UGEN_p).
            out_idx: Index of the output argument on source.
            dest:    Ugen instance (or raw UGEN_p).
            in_idx:  Index of the input argument on dest.
        """
        src_p = source.ugen if isinstance(source, Ugen) else source
        dst_p = dest.ugen if isinstance(dest, Ugen) else dest
        return libcsound.csoundUgenGraphConnect(src_p, out_idx, dst_p, in_idx)

    def init(self):
        """Initialize all UGENs in graph order."""
        return libcsound.csoundUgenGraphInit(self.graph)

    def perform(self):
        """Perform one ksmps block for all UGENs."""
        return libcsound.csoundUgenGraphPerform(self.graph)

