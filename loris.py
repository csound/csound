# This file was created automatically by SWIG.
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _loris

def _swig_setattr(self,class_type,name,value):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    self.__dict__[name] = value

def _swig_getattr(self,class_type,name):
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types



channelize = _loris.channelize

createFreqReference = _loris.createFreqReference

dilate = _loris.dilate

distill = _loris.distill

exportAiff = _loris.exportAiff

exportSdif = _loris.exportSdif

exportSpc = _loris.exportSpc

importSdif = _loris.importSdif

importSpc = _loris.importSpc

synthesize = _loris.synthesize

crop = _loris.crop

copyLabeled = _loris.copyLabeled

extractLabeled = _loris.extractLabeled

removeLabeled = _loris.removeLabeled

resample = _loris.resample

shiftTime = _loris.shiftTime

sift = _loris.sift

sortByLabel = _loris.sortByLabel

version = _loris.version
class Marker(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Marker, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Marker, name)
    def __repr__(self):
        return "<C Marker instance at %s>" % (self.this,)
    def __init__(self, *args):
        _swig_setattr(self, Marker, 'this', _loris.new_Marker(*args))
        _swig_setattr(self, Marker, 'thisown', 1)
    def name(*args): return _loris.Marker_name(*args)
    def time(*args): return _loris.Marker_time(*args)
    def setName(*args): return _loris.Marker_setName(*args)
    def setTime(*args): return _loris.Marker_setTime(*args)
    def __del__(self, destroy=_loris.delete_Marker):
        try:
            if self.thisown: destroy(self)
        except: pass

class MarkerPtr(Marker):
    def __init__(self, this):
        _swig_setattr(self, Marker, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, Marker, 'thisown', 0)
        _swig_setattr(self, Marker,self.__class__,Marker)
_loris.Marker_swigregister(MarkerPtr)

morph = _loris.morph

scaleAmp = _loris.scaleAmp

scaleBandwidth = _loris.scaleBandwidth

scaleFrequency = _loris.scaleFrequency

scaleNoiseRatio = _loris.scaleNoiseRatio

shiftPitch = _loris.shiftPitch

class AiffFile(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, AiffFile, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, AiffFile, name)
    def __repr__(self):
        return "<C AiffFile instance at %s>" % (self.this,)
    def __del__(self, destroy=_loris.delete_AiffFile):
        try:
            if self.thisown: destroy(self)
        except: pass
    def sampleRate(*args): return _loris.AiffFile_sampleRate(*args)
    def midiNoteNumber(*args): return _loris.AiffFile_midiNoteNumber(*args)
    def sampleFrames(*args): return _loris.AiffFile_sampleFrames(*args)
    def addPartial(*args): return _loris.AiffFile_addPartial(*args)
    def setMidiNoteNumber(*args): return _loris.AiffFile_setMidiNoteNumber(*args)
    def write(*args): return _loris.AiffFile_write(*args)
    def __init__(self, *args):
        _swig_setattr(self, AiffFile, 'this', _loris.new_AiffFile(*args))
        _swig_setattr(self, AiffFile, 'thisown', 1)
    def samples(*args): return _loris.AiffFile_samples(*args)
    def channels(*args): return _loris.AiffFile_channels(*args)
    def addPartials(*args): return _loris.AiffFile_addPartials(*args)
    def numMarkers(*args): return _loris.AiffFile_numMarkers(*args)
    def getMarker(*args): return _loris.AiffFile_getMarker(*args)
    def removeMarker(*args): return _loris.AiffFile_removeMarker(*args)
    def addMarker(*args): return _loris.AiffFile_addMarker(*args)

class AiffFilePtr(AiffFile):
    def __init__(self, this):
        _swig_setattr(self, AiffFile, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, AiffFile, 'thisown', 0)
        _swig_setattr(self, AiffFile,self.__class__,AiffFile)
_loris.AiffFile_swigregister(AiffFilePtr)

class Analyzer(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Analyzer, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Analyzer, name)
    def __repr__(self):
        return "<C Analyzer instance at %s>" % (self.this,)
    def __init__(self, *args):
        _swig_setattr(self, Analyzer, 'this', _loris.new_Analyzer(*args))
        _swig_setattr(self, Analyzer, 'thisown', 1)
    def copy(*args): return _loris.Analyzer_copy(*args)
    def analyze(*args): return _loris.Analyzer_analyze(*args)
    def freqResolution(*args): return _loris.Analyzer_freqResolution(*args)
    def ampFloor(*args): return _loris.Analyzer_ampFloor(*args)
    def windowWidth(*args): return _loris.Analyzer_windowWidth(*args)
    def sidelobeLevel(*args): return _loris.Analyzer_sidelobeLevel(*args)
    def freqFloor(*args): return _loris.Analyzer_freqFloor(*args)
    def hopTime(*args): return _loris.Analyzer_hopTime(*args)
    def freqDrift(*args): return _loris.Analyzer_freqDrift(*args)
    def cropTime(*args): return _loris.Analyzer_cropTime(*args)
    def bwRegionWidth(*args): return _loris.Analyzer_bwRegionWidth(*args)
    def setFreqResolution(*args): return _loris.Analyzer_setFreqResolution(*args)
    def setAmpFloor(*args): return _loris.Analyzer_setAmpFloor(*args)
    def setWindowWidth(*args): return _loris.Analyzer_setWindowWidth(*args)
    def setSidelobeLevel(*args): return _loris.Analyzer_setSidelobeLevel(*args)
    def setFreqFloor(*args): return _loris.Analyzer_setFreqFloor(*args)
    def setFreqDrift(*args): return _loris.Analyzer_setFreqDrift(*args)
    def setHopTime(*args): return _loris.Analyzer_setHopTime(*args)
    def setCropTime(*args): return _loris.Analyzer_setCropTime(*args)
    def setBwRegionWidth(*args): return _loris.Analyzer_setBwRegionWidth(*args)
    def __del__(self, destroy=_loris.delete_Analyzer):
        try:
            if self.thisown: destroy(self)
        except: pass

class AnalyzerPtr(Analyzer):
    def __init__(self, this):
        _swig_setattr(self, Analyzer, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, Analyzer, 'thisown', 0)
        _swig_setattr(self, Analyzer,self.__class__,Analyzer)
_loris.Analyzer_swigregister(AnalyzerPtr)

class BreakpointEnvelope(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, BreakpointEnvelope, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, BreakpointEnvelope, name)
    def __repr__(self):
        return "<C BreakpointEnvelope instance at %s>" % (self.this,)
    def __init__(self, *args):
        _swig_setattr(self, BreakpointEnvelope, 'this', _loris.new_BreakpointEnvelope(*args))
        _swig_setattr(self, BreakpointEnvelope, 'thisown', 1)
    def __del__(self, destroy=_loris.delete_BreakpointEnvelope):
        try:
            if self.thisown: destroy(self)
        except: pass
    def copy(*args): return _loris.BreakpointEnvelope_copy(*args)
    def insertBreakpoint(*args): return _loris.BreakpointEnvelope_insertBreakpoint(*args)
    def valueAt(*args): return _loris.BreakpointEnvelope_valueAt(*args)

class BreakpointEnvelopePtr(BreakpointEnvelope):
    def __init__(self, this):
        _swig_setattr(self, BreakpointEnvelope, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, BreakpointEnvelope, 'thisown', 0)
        _swig_setattr(self, BreakpointEnvelope,self.__class__,BreakpointEnvelope)
_loris.BreakpointEnvelope_swigregister(BreakpointEnvelopePtr)


BreakpointEnvelopeWithValue = _loris.BreakpointEnvelopeWithValue
class SampleVector(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, SampleVector, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, SampleVector, name)
    def __repr__(self):
        return "<C SampleVector instance at %s>" % (self.this,)
    def __init__(self, *args):
        _swig_setattr(self, SampleVector, 'this', _loris.new_SampleVector(*args))
        _swig_setattr(self, SampleVector, 'thisown', 1)
    def __del__(self, destroy=_loris.delete_SampleVector):
        try:
            if self.thisown: destroy(self)
        except: pass
    def clear(*args): return _loris.SampleVector_clear(*args)
    def resize(*args): return _loris.SampleVector_resize(*args)
    def size(*args): return _loris.SampleVector_size(*args)
    def copy(*args): return _loris.SampleVector_copy(*args)
    def getAt(*args): return _loris.SampleVector_getAt(*args)
    def setAt(*args): return _loris.SampleVector_setAt(*args)

class SampleVectorPtr(SampleVector):
    def __init__(self, this):
        _swig_setattr(self, SampleVector, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, SampleVector, 'thisown', 0)
        _swig_setattr(self, SampleVector,self.__class__,SampleVector)
_loris.SampleVector_swigregister(SampleVectorPtr)

class SdifFile(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, SdifFile, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, SdifFile, name)
    def __repr__(self):
        return "<C SdifFile instance at %s>" % (self.this,)
    def __del__(self, destroy=_loris.delete_SdifFile):
        try:
            if self.thisown: destroy(self)
        except: pass
    def write(*args): return _loris.SdifFile_write(*args)
    def write1TRC(*args): return _loris.SdifFile_write1TRC(*args)
    def __init__(self, *args):
        _swig_setattr(self, SdifFile, 'this', _loris.new_SdifFile(*args))
        _swig_setattr(self, SdifFile, 'thisown', 1)
    def partials(*args): return _loris.SdifFile_partials(*args)
    def addPartials(*args): return _loris.SdifFile_addPartials(*args)
    def numMarkers(*args): return _loris.SdifFile_numMarkers(*args)
    def getMarker(*args): return _loris.SdifFile_getMarker(*args)
    def removeMarker(*args): return _loris.SdifFile_removeMarker(*args)
    def addMarker(*args): return _loris.SdifFile_addMarker(*args)

class SdifFilePtr(SdifFile):
    def __init__(self, this):
        _swig_setattr(self, SdifFile, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, SdifFile, 'thisown', 0)
        _swig_setattr(self, SdifFile,self.__class__,SdifFile)
_loris.SdifFile_swigregister(SdifFilePtr)

class SpcFile(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, SpcFile, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, SpcFile, name)
    def __repr__(self):
        return "<C SpcFile instance at %s>" % (self.this,)
    def __del__(self, destroy=_loris.delete_SpcFile):
        try:
            if self.thisown: destroy(self)
        except: pass
    def sampleRate(*args): return _loris.SpcFile_sampleRate(*args)
    def midiNoteNumber(*args): return _loris.SpcFile_midiNoteNumber(*args)
    def addPartial(*args): return _loris.SpcFile_addPartial(*args)
    def setMidiNoteNumber(*args): return _loris.SpcFile_setMidiNoteNumber(*args)
    def setSampleRate(*args): return _loris.SpcFile_setSampleRate(*args)
    def write(*args): return _loris.SpcFile_write(*args)
    def __init__(self, *args):
        _swig_setattr(self, SpcFile, 'this', _loris.new_SpcFile(*args))
        _swig_setattr(self, SpcFile, 'thisown', 1)
    def partials(*args): return _loris.SpcFile_partials(*args)
    def addPartials(*args): return _loris.SpcFile_addPartials(*args)
    def numMarkers(*args): return _loris.SpcFile_numMarkers(*args)
    def getMarker(*args): return _loris.SpcFile_getMarker(*args)
    def removeMarker(*args): return _loris.SpcFile_removeMarker(*args)
    def addMarker(*args): return _loris.SpcFile_addMarker(*args)

class SpcFilePtr(SpcFile):
    def __init__(self, this):
        _swig_setattr(self, SpcFile, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, SpcFile, 'thisown', 0)
        _swig_setattr(self, SpcFile,self.__class__,SpcFile)
_loris.SpcFile_swigregister(SpcFilePtr)

class NewPlistIterator(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, NewPlistIterator, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, NewPlistIterator, name)
    def __init__(self): raise RuntimeError, "No constructor defined"
    def __repr__(self):
        return "<C NewPlistIterator instance at %s>" % (self.this,)
    def atEnd(*args): return _loris.NewPlistIterator_atEnd(*args)
    def next(*args): return _loris.NewPlistIterator_next(*args)
    def partial(*args): return _loris.NewPlistIterator_partial(*args)

class NewPlistIteratorPtr(NewPlistIterator):
    def __init__(self, this):
        _swig_setattr(self, NewPlistIterator, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, NewPlistIterator, 'thisown', 0)
        _swig_setattr(self, NewPlistIterator,self.__class__,NewPlistIterator)
_loris.NewPlistIterator_swigregister(NewPlistIteratorPtr)

class NewPartialIterator(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, NewPartialIterator, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, NewPartialIterator, name)
    def __init__(self): raise RuntimeError, "No constructor defined"
    def __repr__(self):
        return "<C NewPartialIterator instance at %s>" % (self.this,)
    def atEnd(*args): return _loris.NewPartialIterator_atEnd(*args)
    def hasNext(*args): return _loris.NewPartialIterator_hasNext(*args)
    def next(*args): return _loris.NewPartialIterator_next(*args)

class NewPartialIteratorPtr(NewPartialIterator):
    def __init__(self, this):
        _swig_setattr(self, NewPartialIterator, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, NewPartialIterator, 'thisown', 0)
        _swig_setattr(self, NewPartialIterator,self.__class__,NewPartialIterator)
_loris.NewPartialIterator_swigregister(NewPartialIteratorPtr)

class PartialList(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, PartialList, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, PartialList, name)
    def __repr__(self):
        return "<C PartialList instance at %s>" % (self.this,)
    def __init__(self, *args):
        _swig_setattr(self, PartialList, 'this', _loris.new_PartialList(*args))
        _swig_setattr(self, PartialList, 'thisown', 1)
    def __del__(self, destroy=_loris.delete_PartialList):
        try:
            if self.thisown: destroy(self)
        except: pass
    def clear(*args): return _loris.PartialList_clear(*args)
    def size(*args): return _loris.PartialList_size(*args)
    def timeSpan(*args): return _loris.PartialList_timeSpan(*args)
    def iterator(*args): return _loris.PartialList_iterator(*args)
    def __iter__(*args): return _loris.PartialList___iter__(*args)
    def append(*args): return _loris.PartialList_append(*args)
    def begin(*args): return _loris.PartialList_begin(*args)
    def end(*args): return _loris.PartialList_end(*args)
    def erase(*args): return _loris.PartialList_erase(*args)
    def splice(*args): return _loris.PartialList_splice(*args)
    def insert(*args): return _loris.PartialList_insert(*args)
    def copy(*args): return _loris.PartialList_copy(*args)

class PartialListPtr(PartialList):
    def __init__(self, this):
        _swig_setattr(self, PartialList, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, PartialList, 'thisown', 0)
        _swig_setattr(self, PartialList,self.__class__,PartialList)
_loris.PartialList_swigregister(PartialListPtr)

class Partial(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Partial, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Partial, name)
    def __repr__(self):
        return "<C Partial instance at %s>" % (self.this,)
    def __init__(self, *args):
        _swig_setattr(self, Partial, 'this', _loris.new_Partial(*args))
        _swig_setattr(self, Partial, 'thisown', 1)
    def __del__(self, destroy=_loris.delete_Partial):
        try:
            if self.thisown: destroy(self)
        except: pass
    def label(*args): return _loris.Partial_label(*args)
    def initialPhase(*args): return _loris.Partial_initialPhase(*args)
    def startTime(*args): return _loris.Partial_startTime(*args)
    def endTime(*args): return _loris.Partial_endTime(*args)
    def duration(*args): return _loris.Partial_duration(*args)
    def numBreakpoints(*args): return _loris.Partial_numBreakpoints(*args)
    def setLabel(*args): return _loris.Partial_setLabel(*args)
    def frequencyAt(*args): return _loris.Partial_frequencyAt(*args)
    def amplitudeAt(*args): return _loris.Partial_amplitudeAt(*args)
    def bandwidthAt(*args): return _loris.Partial_bandwidthAt(*args)
    def phaseAt(*args): return _loris.Partial_phaseAt(*args)
    def iterator(*args): return _loris.Partial_iterator(*args)
    def __iter__(*args): return _loris.Partial___iter__(*args)
    def begin(*args): return _loris.Partial_begin(*args)
    def end(*args): return _loris.Partial_end(*args)
    def erase(*args): return _loris.Partial_erase(*args)
    def insert(*args): return _loris.Partial_insert(*args)
    def findAfter(*args): return _loris.Partial_findAfter(*args)
    def findNearest(*args): return _loris.Partial_findNearest(*args)
    def copy(*args): return _loris.Partial_copy(*args)
    def equals(*args): return _loris.Partial_equals(*args)

class PartialPtr(Partial):
    def __init__(self, this):
        _swig_setattr(self, Partial, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, Partial, 'thisown', 0)
        _swig_setattr(self, Partial,self.__class__,Partial)
_loris.Partial_swigregister(PartialPtr)

class Breakpoint(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Breakpoint, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Breakpoint, name)
    def __repr__(self):
        return "<C Breakpoint instance at %s>" % (self.this,)
    def __init__(self, *args):
        _swig_setattr(self, Breakpoint, 'this', _loris.new_Breakpoint(*args))
        _swig_setattr(self, Breakpoint, 'thisown', 1)
    def __del__(self, destroy=_loris.delete_Breakpoint):
        try:
            if self.thisown: destroy(self)
        except: pass
    def frequency(*args): return _loris.Breakpoint_frequency(*args)
    def amplitude(*args): return _loris.Breakpoint_amplitude(*args)
    def bandwidth(*args): return _loris.Breakpoint_bandwidth(*args)
    def phase(*args): return _loris.Breakpoint_phase(*args)
    def setFrequency(*args): return _loris.Breakpoint_setFrequency(*args)
    def setAmplitude(*args): return _loris.Breakpoint_setAmplitude(*args)
    def setBandwidth(*args): return _loris.Breakpoint_setBandwidth(*args)
    def setPhase(*args): return _loris.Breakpoint_setPhase(*args)
    def copy(*args): return _loris.Breakpoint_copy(*args)
    def equals(*args): return _loris.Breakpoint_equals(*args)

class BreakpointPtr(Breakpoint):
    def __init__(self, this):
        _swig_setattr(self, Breakpoint, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, Breakpoint, 'thisown', 0)
        _swig_setattr(self, Breakpoint,self.__class__,Breakpoint)
_loris.Breakpoint_swigregister(BreakpointPtr)

class BreakpointPosition(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, BreakpointPosition, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, BreakpointPosition, name)
    def __init__(self): raise RuntimeError, "No constructor defined"
    def __repr__(self):
        return "<C BreakpointPosition instance at %s>" % (self.this,)
    def time(*args): return _loris.BreakpointPosition_time(*args)
    def breakpoint(*args): return _loris.BreakpointPosition_breakpoint(*args)
    def frequency(*args): return _loris.BreakpointPosition_frequency(*args)
    def amplitude(*args): return _loris.BreakpointPosition_amplitude(*args)
    def bandwidth(*args): return _loris.BreakpointPosition_bandwidth(*args)
    def phase(*args): return _loris.BreakpointPosition_phase(*args)
    def setFrequency(*args): return _loris.BreakpointPosition_setFrequency(*args)
    def setAmplitude(*args): return _loris.BreakpointPosition_setAmplitude(*args)
    def setBandwidth(*args): return _loris.BreakpointPosition_setBandwidth(*args)
    def setPhase(*args): return _loris.BreakpointPosition_setPhase(*args)

class BreakpointPositionPtr(BreakpointPosition):
    def __init__(self, this):
        _swig_setattr(self, BreakpointPosition, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, BreakpointPosition, 'thisown', 0)
        _swig_setattr(self, BreakpointPosition,self.__class__,BreakpointPosition)
_loris.BreakpointPosition_swigregister(BreakpointPositionPtr)

class PartialIterator(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, PartialIterator, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, PartialIterator, name)
    def __repr__(self):
        return "<C PartialIterator instance at %s>" % (self.this,)
    def time(*args): return _loris.PartialIterator_time(*args)
    def breakpoint(*args): return _loris.PartialIterator_breakpoint(*args)
    def copy(*args): return _loris.PartialIterator_copy(*args)
    def next(*args): return _loris.PartialIterator_next(*args)
    def prev(*args): return _loris.PartialIterator_prev(*args)
    def equals(*args): return _loris.PartialIterator_equals(*args)
    def isInRange(*args): return _loris.PartialIterator_isInRange(*args)
    def __init__(self, *args):
        _swig_setattr(self, PartialIterator, 'this', _loris.new_PartialIterator(*args))
        _swig_setattr(self, PartialIterator, 'thisown', 1)
    def __del__(self, destroy=_loris.delete_PartialIterator):
        try:
            if self.thisown: destroy(self)
        except: pass

class PartialIteratorPtr(PartialIterator):
    def __init__(self, this):
        _swig_setattr(self, PartialIterator, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, PartialIterator, 'thisown', 0)
        _swig_setattr(self, PartialIterator,self.__class__,PartialIterator)
_loris.PartialIterator_swigregister(PartialIteratorPtr)

class PartialListIterator(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, PartialListIterator, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, PartialListIterator, name)
    def __repr__(self):
        return "<C PartialListIterator instance at %s>" % (self.this,)
    def copy(*args): return _loris.PartialListIterator_copy(*args)
    def next(*args): return _loris.PartialListIterator_next(*args)
    def prev(*args): return _loris.PartialListIterator_prev(*args)
    def partial(*args): return _loris.PartialListIterator_partial(*args)
    def equals(*args): return _loris.PartialListIterator_equals(*args)
    def isInRange(*args): return _loris.PartialListIterator_isInRange(*args)
    def __init__(self, *args):
        _swig_setattr(self, PartialListIterator, 'this', _loris.new_PartialListIterator(*args))
        _swig_setattr(self, PartialListIterator, 'thisown', 1)
    def __del__(self, destroy=_loris.delete_PartialListIterator):
        try:
            if self.thisown: destroy(self)
        except: pass

class PartialListIteratorPtr(PartialListIterator):
    def __init__(self, this):
        _swig_setattr(self, PartialListIterator, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, PartialListIterator, 'thisown', 0)
        _swig_setattr(self, PartialListIterator,self.__class__,PartialListIterator)
_loris.PartialListIterator_swigregister(PartialListIteratorPtr)


