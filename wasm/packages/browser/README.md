# @csound/browser
[![npm (scoped with tag)](https://shields.shivering-isles.com/npm/v/@csound/browser/latest)](https://www.npmjs.com/package/@csound/browser)
[![GitHub Workflow Status](https://shields.shivering-isles.com/github/workflow/status/csound/csound/csound_wasm)](https://github.com/csound/csound/actions?query=workflow%3Acsound_wasm)
[![styled with prettier](https://img.shields.io/badge/styled_with-prettier-ff69b4.svg)](https://github.com/prettier/prettier)


## Api Documentation

## Objects

<dl>
<dt><a href="#CsoundObj">CsoundObj</a> : <code>object</code></dt>
<dd><p>CsoundObj API.</p>
</dd>
</dl>

## Functions

<dl>
<dt><a href="#Csound">Csound([params])</a> ⇒ <code>Promise.&lt;(CsoundObj|undefined)&gt;</code></dt>
<dd><p>The default entry for @csound/wasm/browser module.
If loaded successfully, it returns CsoundObj,
otherwise undefined.</p>
</dd>
<dt><a href="#getTable">getTable(tableNum)</a> ⇒ <code>Promise.&lt;(Float64Array|undefined)&gt;</code></dt>
<dd></dd>
</dl>

## Typedefs

<dl>
<dt><a href="#CSOUND_PARAMS">CSOUND_PARAMS</a></dt>
<dd></dd>
<dt><a href="#CS_MIDIDEVICE">CS_MIDIDEVICE</a></dt>
<dd></dd>
</dl>

<a name="CsoundObj"></a>

## CsoundObj : <code>object</code>
CsoundObj API.

**Kind**: global namespace  

* [CsoundObj](#CsoundObj) : <code>object</code>
    * _global_
        * [getTable(tableNum)](#getTable) ⇒ <code>Promise.&lt;(Float64Array\|undefined)&gt;</code>
    * _static_
        * [.fs](#CsoundObj.fs) : <code>IFs:memfs</code>
        * [.eventNames()](#CsoundObj.eventNames) ⇒ <code>Array.&lt;string&gt;</code>
        * [.listenerCount()](#CsoundObj.listenerCount) ⇒ <code>number</code>
        * [.listeners(eventName)](#CsoundObj.listeners) ⇒ <code>Array.&lt;function()&gt;</code>
        * [.off(eventName, listener)](#CsoundObj.off) ⇒ <code>external:EventEmitter</code>
        * [.on(eventName, listener)](#CsoundObj.on) ⇒ <code>external:EventEmitter</code>
        * [.addListener(eventName, listener)](#CsoundObj.addListener) ⇒ <code>external:EventEmitter</code>
        * [.once(eventName, listener)](#CsoundObj.once) ⇒ <code>external:EventEmitter</code>
        * [.removeAllListeners(eventName)](#CsoundObj.removeAllListeners) ⇒ <code>external:EventEmitter</code>
        * [.removeListener(eventName, listener)](#CsoundObj.removeListener) ⇒ <code>external:EventEmitter</code>
        * [.getSr()](#CsoundObj.getSr) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.getKr()](#CsoundObj.getKr) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.getKsmps()](#CsoundObj.getKsmps) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.getNchnls()](#CsoundObj.getNchnls) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.getNchnlsInput()](#CsoundObj.getNchnlsInput) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.get0dBFS()](#CsoundObj.get0dBFS) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.getA4()](#CsoundObj.getA4) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.getCurrentTimeSamples()](#CsoundObj.getCurrentTimeSamples) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.getSizeOfMYFLT()](#CsoundObj.getSizeOfMYFLT) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.setOption(option)](#CsoundObj.setOption) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.setParams(csoundParams)](#CsoundObj.setParams) ⇒ <code>Promise.&lt;undefined&gt;</code>
        * [.getParams()](#CsoundObj.getParams) ⇒ [<code>Promise.&lt;CSOUND\_PARAMS&gt;</code>](#CSOUND_PARAMS)
        * [.getDebug()](#CsoundObj.getDebug) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.setDebug(debug)](#CsoundObj.setDebug) ⇒ <code>Promise.&lt;undefined&gt;</code>
        * [.inputMessage(scoreEvent)](#CsoundObj.inputMessage) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.inputMessageAsync(scoreEvent)](#CsoundObj.inputMessageAsync) ⇒ <code>Promise.&lt;number&gt;</code>
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
        * [.getMIDIDevList(isOutput)](#CsoundObj.getMIDIDevList) ⇒ [<code>Promise.&lt;CS\_MIDIDEVICE&gt;</code>](#CS_MIDIDEVICE)
        * [.getRtMidiName()](#CsoundObj.getRtMidiName) ⇒ <code>Promise.&lt;string&gt;</code>
        * [.midiMessage(midi, midi, midi)](#CsoundObj.midiMessage) ⇒ <code>Promise.&lt;void&gt;</code>
        * [.isScorePending()](#CsoundObj.isScorePending) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.setScorePending(pending)](#CsoundObj.setScorePending) ⇒ <code>Promise.&lt;undefined&gt;</code>
        * [.readScore(score)](#CsoundObj.readScore) ⇒ <code>Promise.&lt;undefined&gt;</code>
        * [.getScoreTime()](#CsoundObj.getScoreTime) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.getScoreOffsetSeconds()](#CsoundObj.getScoreOffsetSeconds) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.setScoreOffsetSeconds(time)](#CsoundObj.setScoreOffsetSeconds) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.rewindScore()](#CsoundObj.rewindScore) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.tableLength(tableNum)](#CsoundObj.tableLength) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.tableGet(tableNum, tableIndex)](#CsoundObj.tableGet) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.tableSet(tableNum, tableIndex, value)](#CsoundObj.tableSet) ⇒ <code>Promise.&lt;undefined&gt;</code>
        * [.tableCopyIn(tableNum, tableIndex, array)](#CsoundObj.tableCopyIn) ⇒ <code>Promise.&lt;undefined&gt;</code>
        * [.tableCopyOut(tableNum)](#CsoundObj.tableCopyOut) ⇒ <code>Promise.&lt;(Float64Array\|undefined)&gt;</code>
        * [.getTableArgs(tableNum)](#CsoundObj.getTableArgs) ⇒ <code>Promise.&lt;(Float64Array\|undefined)&gt;</code>
        * [.isNamedGEN(tableNum)](#CsoundObj.isNamedGEN) ⇒ <code>Promise.&lt;number&gt;</code>
        * [.getNamedGEN(tableNum)](#CsoundObj.getNamedGEN) ⇒ <code>Promise.&lt;(string\|undefined)&gt;</code>

<a name="getTable"></a>

### CsoundObjgetTable(tableNum) ⇒ <code>Promise.&lt;(Float64Array\|undefined)&gt;</code>
**Kind**: global method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| tableNum | <code>string</code> | 

<a name="CsoundObj.fs"></a>

### CsoundObj.fs : <code>IFs:memfs</code>
The in-browser filesystem based on nodejs's
built-in module "fs"

**Kind**: static property of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.eventNames"></a>

### CsoundObj.eventNames() ⇒ <code>Array.&lt;string&gt;</code>
Returns an array listing the events for which the emitter has registered listeners.
The values in the array are strings.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.listenerCount"></a>

### CsoundObj.listenerCount() ⇒ <code>number</code>
Returns the number of listeners listening to the event named eventName.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.listeners"></a>

### CsoundObj.listeners(eventName) ⇒ <code>Array.&lt;function()&gt;</code>
Returns a copy of the array of listeners for the event named eventName.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| eventName | [<code>PublicEvents</code>](#PublicEvents) | 

<a name="CsoundObj.off"></a>

### CsoundObj.off(eventName, listener) ⇒ <code>external:EventEmitter</code>
Alias for removeListener()

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| eventName | [<code>PublicEvents</code>](#PublicEvents) | 
| listener | <code>function</code> | 

<a name="CsoundObj.on"></a>

### CsoundObj.on(eventName, listener) ⇒ <code>external:EventEmitter</code>
Adds the listener function to the end of the listeners array for the event named eventName.
No checks are made to see if the listener has already been added.
Multiple calls passing the same combination of eventName and listener
will result in the listener being added, and called, multiple times.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| eventName | [<code>PublicEvents</code>](#PublicEvents) | 
| listener | <code>function</code> | 

<a name="CsoundObj.addListener"></a>

### CsoundObj.addListener(eventName, listener) ⇒ <code>external:EventEmitter</code>
Alias for "on"

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| eventName | [<code>PublicEvents</code>](#PublicEvents) | 
| listener | <code>function</code> | 

<a name="CsoundObj.once"></a>

### CsoundObj.once(eventName, listener) ⇒ <code>external:EventEmitter</code>
Adds a one-time listener function for the event named eventName.
The next time eventName is triggered, this listener is removed and then invoked.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| eventName | [<code>PublicEvents</code>](#PublicEvents) | 
| listener | <code>function</code> | 

<a name="CsoundObj.removeAllListeners"></a>

### CsoundObj.removeAllListeners(eventName) ⇒ <code>external:EventEmitter</code>
Removes all listeners, or those of the specified eventName.
It is bad practice to remove listeners added elsewhere in the code,
particularly when the EventEmitter instance was created by some other
component or module.
Returns a reference to the EventEmitter, so that calls can be chained.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| eventName | [<code>PublicEvents</code>](#PublicEvents) | 

<a name="CsoundObj.removeListener"></a>

### CsoundObj.removeListener(eventName, listener) ⇒ <code>external:EventEmitter</code>
Removes the specified listener from the listener array for the event named eventName.
removeListener() will remove, at most, one instance of a listener from the listener array.
If any single listener has been added multiple times to the listener array for the specified eventName,
then removeListener() must be called multiple times to remove each instance.
Removes the specified listener from the listener array for the event named eventName.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| eventName | [<code>PublicEvents</code>](#PublicEvents) | 
| listener | <code>function</code> | 

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

### CsoundObj.setOption(option) ⇒ <code>Promise.&lt;number&gt;</code>
Set a single csound option (flag),
no spaces are allowed in the string.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| option | <code>string</code> | 

<a name="CsoundObj.setParams"></a>

### CsoundObj.setParams(csoundParams) ⇒ <code>Promise.&lt;undefined&gt;</code>
Configure Csound with a given set of
parameters defined in the CSOUND_PARAMS structure.
These parameters are the part of the OPARMS struct
that are configurable through command line flags.
The CSOUND_PARAMS structure can be obtained using
csoundGetParams().
These options should only be changed before
performance has started.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type | Description |
| --- | --- | --- |
| csoundParams | [<code>CSOUND\_PARAMS</code>](#CSOUND_PARAMS) | csoundParams object |

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

### CsoundObj.inputMessage(scoreEvent) ⇒ <code>Promise.&lt;number&gt;</code>
Inputs an immediate score event
without any pre-process parsing

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| scoreEvent | <code>string</code> | 

<a name="CsoundObj.inputMessageAsync"></a>

### CsoundObj.inputMessageAsync(scoreEvent) ⇒ <code>Promise.&lt;number&gt;</code>
Inputs an immediate score event
without any pre-process parsing

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| scoreEvent | <code>string</code> | 

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
<a name="CsoundObj.getMIDIDevList"></a>

### CsoundObj.getMIDIDevList(isOutput) ⇒ [<code>Promise.&lt;CS\_MIDIDEVICE&gt;</code>](#CS_MIDIDEVICE)
This function can be called to obtain a list of available input or output midi devices.
If list is NULL, the function will only return the number of devices
(isOutput=1 for out devices, 0 for in devices).

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| isOutput | <code>number</code> | 

<a name="CsoundObj.getRtMidiName"></a>

### CsoundObj.getRtMidiName() ⇒ <code>Promise.&lt;string&gt;</code>
This function can be called to obtain a list of available input or output midi devices.
If list is NULL, the function will only return the number of devices
(isOutput=1 for out devices, 0 for in devices).

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.midiMessage"></a>

### CsoundObj.midiMessage(midi, midi, midi) ⇒ <code>Promise.&lt;void&gt;</code>
Emit a midi message with a given triplet of values
in the range of 0 to 127.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type | Description |
| --- | --- | --- |
| midi | <code>number</code> | status value |
| midi | <code>number</code> | data1 |
| midi | <code>number</code> | data2 |

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

<a name="CsoundObj.readScore"></a>

### CsoundObj.readScore(score) ⇒ <code>Promise.&lt;undefined&gt;</code>
Read, preprocess, and load a score from an ASCII string It can be called repeatedly,
with the new score events being added to the currently scheduled ones.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| score | <code>string</code> | 

<a name="CsoundObj.getScoreTime"></a>

### CsoundObj.getScoreTime() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the current score time in seconds since the beginning of performance.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.getScoreOffsetSeconds"></a>

### CsoundObj.getScoreOffsetSeconds() ⇒ <code>Promise.&lt;number&gt;</code>
Returns the score time beginning at which score events will actually immediately be performed

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.setScoreOffsetSeconds"></a>

### CsoundObj.setScoreOffsetSeconds(time) ⇒ <code>Promise.&lt;number&gt;</code>
Csound score events prior to the specified time are not performed,
and performance begins immediately at the specified time
(real-time events will continue to be performed as they are received).
Can be used by external software, such as a VST host, to begin
score performance midway through a Csound score,
for example to repeat a loop in a sequencer,
or to synchronize other events with the Csound score.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| time | <code>number</code> | 

<a name="CsoundObj.rewindScore"></a>

### CsoundObj.rewindScore() ⇒ <code>Promise.&lt;number&gt;</code>
Rewinds a compiled Csound score to the time specified with csoundObj.setScoreOffsetSeconds().

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  
<a name="CsoundObj.tableLength"></a>

### CsoundObj.tableLength(tableNum) ⇒ <code>Promise.&lt;number&gt;</code>
Returns the length of a function table
(not including the guard point),
or -1 if the table does not exist.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| tableNum | <code>string</code> | 

<a name="CsoundObj.tableGet"></a>

### CsoundObj.tableGet(tableNum, tableIndex) ⇒ <code>Promise.&lt;number&gt;</code>
Returns the value of a slot in a function table.
The table number and index are assumed to be valid.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| tableNum | <code>string</code> | 
| tableIndex | <code>string</code> | 

<a name="CsoundObj.tableSet"></a>

### CsoundObj.tableSet(tableNum, tableIndex, value) ⇒ <code>Promise.&lt;undefined&gt;</code>
Sets the value of a slot in a function table.
The table number and index are assumed to be valid.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| tableNum | <code>string</code> | 
| tableIndex | <code>string</code> | 
| value | <code>string</code> | 

<a name="CsoundObj.tableCopyIn"></a>

### CsoundObj.tableCopyIn(tableNum, tableIndex, array) ⇒ <code>Promise.&lt;undefined&gt;</code>
Copy the contents of an Array or TypedArray from javascript into a given csound function table.
The table number is assumed to be valid, and the table needs to have sufficient space
to receive all the array contents.
The table number and index are assumed to be valid.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| tableNum | <code>string</code> | 
| tableIndex | <code>string</code> | 
| array | <code>Array.&lt;number&gt;</code> \| <code>ArrayLike.&lt;number&gt;</code> | 

<a name="CsoundObj.tableCopyOut"></a>

### CsoundObj.tableCopyOut(tableNum) ⇒ <code>Promise.&lt;(Float64Array\|undefined)&gt;</code>
Copies the contents of a function table from csound into Float64Array.
The function returns a Float64Array if the table exists, otherwise
it returns undefined.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| tableNum | <code>string</code> | 

<a name="CsoundObj.getTableArgs"></a>

### CsoundObj.getTableArgs(tableNum) ⇒ <code>Promise.&lt;(Float64Array\|undefined)&gt;</code>
Copies the contents of a function table from csound into Float64Array.
The function returns a Float64Array if the table exists, otherwise
it returns undefined.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| tableNum | <code>string</code> | 

<a name="CsoundObj.isNamedGEN"></a>

### CsoundObj.isNamedGEN(tableNum) ⇒ <code>Promise.&lt;number&gt;</code>
Checks if a given GEN number num is a named GEN if so,
it returns the string length (excluding terminating NULL char).
Otherwise it returns 0.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| tableNum | <code>string</code> | 

<a name="CsoundObj.getNamedGEN"></a>

### CsoundObj.getNamedGEN(tableNum) ⇒ <code>Promise.&lt;(string\|undefined)&gt;</code>
Gets the GEN name from a number num, if this is a named GEN.
If the table number doesn't represent a named GEN, it will
return undefined.

**Kind**: static method of [<code>CsoundObj</code>](#CsoundObj)  

| Param | Type |
| --- | --- |
| tableNum | <code>string</code> | 

<a name="PublicEvents"></a>

## PublicEvents : <code>enum</code>
**Kind**: global enum  
**Read only**: true  
**Properties**

| Name | Type | Description |
| --- | --- | --- |
| "play" | <code>string</code> | called anytime performance goes from pause/stop to a running state. |
| "pause" | <code>string</code> | called after any successful csound.pause() calls. |
| "stop" | <code>string</code> | called after end of performance or after a successful csound.stop(). |
| "realtimePerformanceStarted" | <code>string</code> | called at the start of realtime performance but not on resume or render. |
| "realtimePerformancePaused" | <code>string</code> | only called if csound.pause() was successfully called during performance. |
| "realtimePerformanceResumed" | <code>string</code> | only called if csound.resume() was successfully called after a pause. |
| "realtimePerformanceEnded" | <code>string</code> | called after end of performance or after a successful csound.stop(). |
| "renderStarted" | <code>string</code> | called at the start of offline/non-realtime render to disk. |
| "renderEnded" | <code>string</code> | called at the end of offline/non-realtime render to disk. |
| "onAudioNodeCreated" | <code>string</code> | called when an audioNode is created from the AudioContext before realtime performance. the event callback will include the audioNode itself, which is needed if autoConnect is set to false. |
| "message" | <code>string</code> | the main entrypoint to csound's messaging (-m) system, a default event listener will print the message to the browser console, this default listener can be removed by the user. |

<a name="Csound"></a>

## Csound([params]) ⇒ <code>Promise.&lt;(CsoundObj\|undefined)&gt;</code>
The default entry for @csound/wasm/browser module.
If loaded successfully, it returns CsoundObj,
otherwise undefined.

**Kind**: global function  

| Param | Type | Default | Description |
| --- | --- | --- | --- |
| [params] | <code>Object</code> |  | Initialization parameters |
| [params.audioContext] | <code>AudioContext</code> |  | Optional AudioContext to use; if none given, an AudioContext will be created. |
| [params.inputChannelCount] | <code>Number</code> |  | Optional input channel count for AudioNode used with WebAudio graph. Defaults to the value of nchnls_i in useWorker but 2 otherwise. |
| [params.outputChannelCount] | <code>Number</code> |  | Optional output channel count AudioNode used with WebAudio graph. Defaults to the value of nchnls in useWorker but 2 otherwise. |
| [params.autoConnect] | <code>Boolean</code> | <code>true</code> | Set to configure Csound to automatically connect to the audioContext.destination output. |
| [params.withPlugins] | <code>Array.&lt;Object&gt;</code> |  | Array of WebAssembly Csound plugin libraries to use with Csound. |
| [params.useWorker] | <code>Boolean</code> | <code>false</code> | Configure to use backend using Web Workers to run Csound in a thread separate from audio callback. |
| [params.useSAB] | <code>Boolean</code> | <code>true</code> | Configure to use SharedArrayBuffers for WebWorker communications if platform supports it. |
| [params.useSPN] | <code>Boolean</code> | <code>false</code> | Configure to use explicitly request ScriptProcessorNode rather than AudioWorklet. Recommended only for debug testing purposes. |

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

<a name="CS_MIDIDEVICE"></a>

## CS\_MIDIDEVICE
**Kind**: global typedef  
**Properties**

| Name | Type |
| --- | --- |
| device_name | <code>string</code> | 
| interface_name | <code>string</code> | 
| device_id | <code>string</code> | 
| midi_module | <code>string</code> | 
| isOutput | <code>number</code> | 


