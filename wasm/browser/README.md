## Objects

<dl>
<dt><a href="#CsoundObj">CsoundObj</a> : <code>object</code></dt>
<dd><p>CsoundObj API.</p>
</dd>
</dl>

## Functions

<dl>
<dt><a href="#Csound">Csound()</a> ⇒ <code>Promise.&lt;(CsoundObj|undefined)&gt;</code></dt>
<dd><p>The default entry for @csound/wasm/browser module.
If loaded successfully, it returns CsoundObj,
otherwise undefined.</p>
</dd>
</dl>

## Typedefs

<dl>
<dt><a href="#CSOUND_PARAMS">CSOUND_PARAMS</a></dt>
<dd></dd>
</dl>

<a name="CsoundObj"></a>

## CsoundObj : <code>object</code>
CsoundObj API.

**Kind**: global namespace  

* [CsoundObj](#CsoundObj) : <code>object</code>
    * [.removeListener(eventName, listener)](#CsoundObj.removeListener) ⇒ <code>external:EventEmitter</code>
    * [.eventNames()](#CsoundObj.eventNames) ⇒ <code>Array.&lt;string&gt;</code>
    * [.listeners(eventName)](#CsoundObj.listeners) ⇒ <code>Array.&lt;function()&gt;</code>
    * [.getSr()](#CsoundObj.getSr) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getKr()](#CsoundObj.getKr) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getKsmps()](#CsoundObj.getKsmps) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getNchnls()](#CsoundObj.getNchnls) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getNchnlsInput()](#CsoundObj.getNchnlsInput) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.get0dBFS()](#CsoundObj.get0dBFS) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getA4()](#CsoundObj.getA4) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getCurrentTimeSamples()](#CsoundObj.getCurrentTimeSamples) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getSizeOfMYFLT()](#CsoundObj.getSizeOfMYFLT) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.setOption()](#CsoundObj.setOption) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.setParams()](#CsoundObj.setParams) ⇒ <code>Promise.&lt;undefined&gt;</code>
    * [.getParams()](#CsoundObj.getParams) ⇒ [<code>Promise.&lt;CSOUND\_PARAMS&gt;</code>](#CSOUND_PARAMS)
    * [.getDebug()](#CsoundObj.getDebug) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.setDebug(debug)](#CsoundObj.setDebug) ⇒ <code>Promise.&lt;undefined&gt;</code>
    * [.inputMessage()](#CsoundObj.inputMessage) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.inputMessageAsync()](#CsoundObj.inputMessageAsync) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getControlChannel(channelName)](#CsoundObj.getControlChannel) ⇒ <code>Promise.&lt;undefined&gt;</code>
    * [.setControlChannel(channelName, value)](#CsoundObj.setControlChannel) ⇒ <code>Promise.&lt;undefined&gt;</code>
    * [.getStringChannel(channelName)](#CsoundObj.getStringChannel) ⇒ <code>Promise.&lt;undefined&gt;</code>
    * [.setStringChannel(channelName, value)](#CsoundObj.setStringChannel) ⇒ <code>Promise.&lt;undefined&gt;</code>
    * [.getOutputName()](#CsoundObj.getOutputName) ⇒ <code>Promise.&lt;string&gt;</code>
    * [.getInputName()](#CsoundObj.getInputName) ⇒ <code>Promise.&lt;string&gt;</code>
    * [.destroy()](#CsoundObj.destroy) ⇒ <code>Promise.&lt;undefined&gt;</code>
    * [.getAPIVersion()](#CsoundObj.getAPIVersion) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getVersion()](#CsoundObj.getVersion) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.initialize()](#CsoundObj.initialize) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.parseOrc(orc)](#CsoundObj.parseOrc) ⇒ <code>Promise.&lt;object&gt;</code>
    * [.compileTree(tree)](#CsoundObj.compileTree) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.compileOrc(orc)](#CsoundObj.compileOrc) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.evalCode(orc)](#CsoundObj.evalCode) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.start()](#CsoundObj.start) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.compileCsd(path)](#CsoundObj.compileCsd) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.compileCsdText(orc)](#CsoundObj.compileCsdText) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.perform()](#CsoundObj.perform) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.performKsmps()](#CsoundObj.performKsmps) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.performBuffer()](#CsoundObj.performBuffer) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.stop()](#CsoundObj.stop) ⇒ <code>Promise.&lt;undefined&gt;</code>
    * [.cleanup()](#CsoundObj.cleanup) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.reset()](#CsoundObj.reset) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getInputBufferSize()](#CsoundObj.getInputBufferSize) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getOutputBufferSize()](#CsoundObj.getOutputBufferSize) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getInputBuffer()](#CsoundObj.getInputBuffer) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getOutputBuffer()](#CsoundObj.getOutputBuffer) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getSpin()](#CsoundObj.getSpin) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.getSpout()](#CsoundObj.getSpout) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.isScorePending()](#CsoundObj.isScorePending) ⇒ <code>Promise.&lt;number&gt;</code>
    * [.setScorePending(pending)](#CsoundObj.setScorePending) ⇒ <code>Promise.&lt;undefined&gt;</code>

<a name="CsoundObj.removeListener"></a>

### CsoundObj.removeListener(eventName, listener) ⇒ <code>external:EventEmitter</code>
Removes the specified listener from the listener array for the event named eventName.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| eventName | [<code>PublicEvents</code>](#PublicEvents) | 
| listener | <code>function</code> | 

<a name="CsoundObj.eventNames"></a>

### CsoundObj.eventNames() ⇒ <code>Array.&lt;string&gt;</code>
Returns an array listing the events for which the emitter has registered listeners.
The values in the array are strings.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.listeners"></a>

### CsoundObj.listeners(eventName) ⇒ <code>Array.&lt;function()&gt;</code>
Returns a copy of the array of listeners for the event named eventName.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| eventName | [<code>PublicEvents</code>](#PublicEvents) | 

<a name="CsoundObj.getSr"></a>

### CsoundObj.getSr() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the sample rate from Csound instance

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getKr"></a>

### CsoundObj.getKr() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the control rate from Csound instance

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getKsmps"></a>

### CsoundObj.getKsmps() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the ksmps value (kr/sr) from Csound instance

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getNchnls"></a>

### CsoundObj.getNchnls() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the number of output channels from Csound instance

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getNchnlsInput"></a>

### CsoundObj.getNchnlsInput() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the number of input channels from Csound instance

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.get0dBFS"></a>

### CsoundObj.get0dBFS() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the value of csoundGet0dBFS

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getA4"></a>

### CsoundObj.getA4() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the A4 frequency reference

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getCurrentTimeSamples"></a>

### CsoundObj.getCurrentTimeSamples() ⇒ <code>Promise.&lt;number&gt;</code>
Return the current performance time in samples

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getSizeOfMYFLT"></a>

### CsoundObj.getSizeOfMYFLT() ⇒ <code>Promise.&lt;number&gt;</code>
Return the size of MYFLT in number of bytes

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.setOption"></a>

### CsoundObj.setOption() ⇒ <code>Promise.&lt;number&gt;</code>
Set a single csound option (flag),
no spaces are allowed in the string.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.setParams"></a>

### CsoundObj.setParams() ⇒ <code>Promise.&lt;undefined&gt;</code>
Configure Csound with a given set of
parameters defined in the CSOUND_PARAMS structure.
These parameters are the part of the OPARMS struct
that are configurable through command line flags.
The CSOUND_PARAMS structure can be obtained using
csoundGetParams().
These options should only be changed before
performance has started.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Type | Description |
| --- | --- |
| [<code>CSOUND\_PARAMS</code>](#CSOUND_PARAMS) | csoundParams object |

<a name="CsoundObj.getParams"></a>

### CsoundObj.getParams() ⇒ [<code>Promise.&lt;CSOUND\_PARAMS&gt;</code>](#CSOUND_PARAMS)
Get the current set of parameters
from a Csound instance
in a CSOUND_PARAMS structure.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
**Returns**: [<code>Promise.&lt;CSOUND\_PARAMS&gt;</code>](#CSOUND_PARAMS) - - CSOUND_PARAMS object  
<a name="CsoundObj.getDebug"></a>

### CsoundObj.getDebug() ⇒ <code>Promise.&lt;number&gt;</code>
Returns whether Csound is set to print debug messages
sent through the DebugMsg() internal API function.
Anything different to 0 means true.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.setDebug"></a>

### CsoundObj.setDebug(debug) ⇒ <code>Promise.&lt;undefined&gt;</code>
Return the size of MYFLT in number of bytes

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| debug | <code>number</code> | 

<a name="CsoundObj.inputMessage"></a>

### CsoundObj.inputMessage() ⇒ <code>Promise.&lt;number&gt;</code>
Inputs an immediate score event
without any pre-process parsing

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.inputMessageAsync"></a>

### CsoundObj.inputMessageAsync() ⇒ <code>Promise.&lt;number&gt;</code>
Inputs an immediate score event
without any pre-process parsing

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getControlChannel"></a>

### CsoundObj.getControlChannel(channelName) ⇒ <code>Promise.&lt;undefined&gt;</code>
Retrieves the value of control channel identified by channelName.
If the err argument is not NULL, the error (or success) code finding
or accessing the channel is stored in it.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| channelName | <code>string</code> | 

<a name="CsoundObj.setControlChannel"></a>

### CsoundObj.setControlChannel(channelName, value) ⇒ <code>Promise.&lt;undefined&gt;</code>
Sets the value of control channel identified by channelName

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| channelName | <code>string</code> | 
| value | <code>number</code> | 

<a name="CsoundObj.getStringChannel"></a>

### CsoundObj.getStringChannel(channelName) ⇒ <code>Promise.&lt;undefined&gt;</code>
Retrieves the string channel identified by channelName

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| channelName | <code>string</code> | 

<a name="CsoundObj.setStringChannel"></a>

### CsoundObj.setStringChannel(channelName, value) ⇒ <code>Promise.&lt;undefined&gt;</code>
Sets the string channel value identified by channelName

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| channelName | <code>string</code> | 
| value | <code>string</code> | 

<a name="CsoundObj.getOutputName"></a>

### CsoundObj.getOutputName() ⇒ <code>Promise.&lt;string&gt;</code>
Returns the audio output name (-o)

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getInputName"></a>

### CsoundObj.getInputName() ⇒ <code>Promise.&lt;string&gt;</code>
Returns the audio input name (-i)

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.destroy"></a>

### CsoundObj.destroy() ⇒ <code>Promise.&lt;undefined&gt;</code>
Destroys an instance of Csound and frees memory

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getAPIVersion"></a>

### CsoundObj.getAPIVersion() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the API version as int

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getVersion"></a>

### CsoundObj.getVersion() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the Csound version as int

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.initialize"></a>

### CsoundObj.initialize() ⇒ <code>Promise.&lt;number&gt;</code>
Initialise Csound with specific flags.
This function is called internally by csoundCreate(),
so there is generally no need to use it explicitly
unless you need to avoid default initilization that
sets signal handlers and atexit() callbacks.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
**Returns**: <code>Promise.&lt;number&gt;</code> - - Return value is zero on success,
    positive if initialisation was done already, and negative on error.  
<a name="CsoundObj.parseOrc"></a>

### CsoundObj.parseOrc(orc) ⇒ <code>Promise.&lt;object&gt;</code>
Parses a csound orchestra string

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| orc | <code>string</code> | 

<a name="CsoundObj.compileTree"></a>

### CsoundObj.compileTree(tree) ⇒ <code>Promise.&lt;number&gt;</code>
Compiles AST tree

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| tree | <code>object</code> | 

<a name="CsoundObj.compileOrc"></a>

### CsoundObj.compileOrc(orc) ⇒ <code>Promise.&lt;number&gt;</code>
Compiles a csound orchestra string

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| orc | <code>string</code> | 

<a name="CsoundObj.evalCode"></a>

### CsoundObj.evalCode(orc) ⇒ <code>Promise.&lt;number&gt;</code>
Compiles a csound orchestra string

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| orc | <code>string</code> | 

<a name="CsoundObj.start"></a>

### CsoundObj.start() ⇒ <code>Promise.&lt;number&gt;</code>
Prepares Csound for performance

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.compileCsd"></a>

### CsoundObj.compileCsd(path) ⇒ <code>Promise.&lt;number&gt;</code>
Compiles a Csound input file but does not perform it.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| path | <code>string</code> | 

<a name="CsoundObj.compileCsdText"></a>

### CsoundObj.compileCsdText(orc) ⇒ <code>Promise.&lt;number&gt;</code>
Compiles a CSD string but does not perform it.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| orc | <code>string</code> | 

<a name="CsoundObj.perform"></a>

### CsoundObj.perform() ⇒ <code>Promise.&lt;number&gt;</code>
Performs(plays) audio until end is reached

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.performKsmps"></a>

### CsoundObj.performKsmps() ⇒ <code>Promise.&lt;number&gt;</code>
Performs(plays) 1 ksmps worth of sample(s)

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.performBuffer"></a>

### CsoundObj.performBuffer() ⇒ <code>Promise.&lt;number&gt;</code>
Performs(plays) 1 buffer worth of audio

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.stop"></a>

### CsoundObj.stop() ⇒ <code>Promise.&lt;undefined&gt;</code>
Stops a csoundPerform

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.cleanup"></a>

### CsoundObj.cleanup() ⇒ <code>Promise.&lt;number&gt;</code>
Prints information about the end of a performance,
and closes audio and MIDI devices.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.reset"></a>

### CsoundObj.reset() ⇒ <code>Promise.&lt;number&gt;</code>
Prints information about the end of a performance,
and closes audio and MIDI devices.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getInputBufferSize"></a>

### CsoundObj.getInputBufferSize() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the number of samples in Csound's input buffer.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getOutputBufferSize"></a>

### CsoundObj.getOutputBufferSize() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the number of samples in Csound's output buffer.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getInputBuffer"></a>

### CsoundObj.getInputBuffer() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the address of the Csound audio input buffer.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getOutputBuffer"></a>

### CsoundObj.getOutputBuffer() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the address of the Csound audio output buffer.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getSpin"></a>

### CsoundObj.getSpin() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the address of the Csound audio input working buffer (spin).
Enables external software to write audio into Csound before calling csoundPerformKsmps.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getSpout"></a>

### CsoundObj.getSpout() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the address of the Csound audio output working buffer (spout).
Enables external software to read audio from Csound after calling csoundPerformKsmps.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.isScorePending"></a>

### CsoundObj.isScorePending() ⇒ <code>Promise.&lt;number&gt;</code>
Sees whether Csound score events are performed or not,
independently of real-time MIDI events

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.setScorePending"></a>

### CsoundObj.setScorePending(pending) ⇒ <code>Promise.&lt;undefined&gt;</code>
Sets whether Csound score events are performed or not
(real-time events will continue to be performed).
Can be used by external software, such as a VST host,
to turn off performance of score events (while continuing to perform real-time events),
for example to mute a Csound score while working on other tracks of a piece,
or to play the Csound instruments live.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| pending | <code>number</code> | 

<a name="PublicEvents"></a>

## PublicEvents : <code>enum</code>
**Kind**: global enum  
**Read only**: true  
**Properties**

| Name | Type | Description |
| --- | --- | --- |
| "play" | <code>string</code> | called anytime performance goes from pause/stop to a running state |
| "pause" | <code>string</code> | called after any successful csound.pause() calls |
| "stop" | <code>string</code> | called after end of performance or after a successful csound.stop() |
| "realtimePerformanceStarted" | <code>string</code> | called at the start of realtime performance but not on resume or render |
| "realtimePerformancePaused" | <code>string</code> | only called if csound.pause() was successfully called during performance |
| "realtimePerformanceResumed" | <code>string</code> | only called if csound.resume() was successfully called after a pause |
| "realtimePerformanceEnded" | <code>string</code> | called after end of performance or after a successful csound.stop() |
| "renderStarted" | <code>string</code> | called at the start of offline/non-realtime render to disk |
| "renderEnded" | <code>string</code> | called at the end of offline/non-realtime render to disk |

<a name="Csound"></a>

## Csound() ⇒ <code>Promise.&lt;(CsoundObj\|undefined)&gt;</code>
The default entry for @csound/wasm/browser module.
If loaded successfully, it returns CsoundObj,
otherwise undefined.

**Kind**: global function  
<a name="CSOUND_PARAMS"></a>

## CSOUND\_PARAMS
**Kind**: global typedef  
**Properties**

| Name | Type |
| --- | --- |
| debug_mode | <code>number</code> | 
| buffer_frames | <code>number</code> | 
| hardware_buffer_frames | <code>number</code> | 
| displays | <code>number</code> | 
| ascii_graphs | <code>number</code> | 
| postscript_graphs | <code>number</code> | 
| message_level | <code>number</code> | 
| tempo | <code>number</code> | 
| ring_bell | <code>number</code> | 
| use_cscore | <code>number</code> | 
| terminate_on_midi | <code>number</code> | 
| heartbeat | <code>number</code> | 
| defer_gen01_load | <code>number</code> | 
| midi_key | <code>number</code> | 
| midi_key_cps | <code>number</code> | 
| midi_key_oct | <code>number</code> | 
| midi_key_pch | <code>number</code> | 
| midi_velocity | <code>number</code> | 


