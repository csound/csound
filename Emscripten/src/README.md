The WebAudio Csound classes
----------------------

The fundamental API for WebAudio Csound is CsoundObj. This allows developers
to create applications using Csound with little or no knowledge of the Web Audio
API. Built on top of this, the Csound frontend API provides a convenient wrapper
to the most common operations (compiling, running, sending control data and MIDI,
copying files to the sandboxed filesystem etc.), for which a plug-and-play
csound frontend object is available.

For close integration with the Web Audio API, developers may employ the Csound
engine nodes directly: CsoundNode (available wherever the AudioWorklet
interface is present) and/or CsoundScriptProcessorNode. These objects can
be accessed inside a CsoundObj object, or created directly (e.g. through
the CsoundNodeFactory or CsoundScriptProcessorNodeFactory).
