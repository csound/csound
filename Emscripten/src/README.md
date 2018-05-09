## Introduction 

The fundamental API for WebAudio Csound is CsoundObj. This allows developers
to create applications using Csound with little or no knowledge of the Web Audio
API. Built on top of this, the Csound frontend API provides a convenient wrapper
to the most common operations (compiling, running, sending control data and MIDI,
copying files to the sandboxed filesystem etc.), for which a plug-and-play
Csound frontend object is available.

For close integration with the Web Audio API, developers may employ the Csound
engine nodes directly: CsoundNode (available wherever the AudioWorklet
interface is present) and/or CsoundScriptProcessorNode. These objects can
be accessed inside a CsoundObj object, or created directly (e.g. through
the CsoundNodeFactory or CsoundScriptProcessorNodeFactory).

## Getting Started 

Using WebAudio Csound requires a series of steps: 

1. Figure out where to load Csound web files from (may be local to web app or from online)
2. Include the main CsoundObj.js file within your Web application.
3. Initialize the CsoundObj system by calling the importScripts() static method on CsoundObj.
4. Run your code to setup and start Csound. 

To use Csound in your Web application, either use an online version or copy the
contents of the "examples/js" folder, provided with the csound-web release,
into a folder accessible by your web page. This folder contains CsoundObj.js
and other files required to run Csound.

Once you know where you will load Csound web files from, you will need to include the CsoundObj.js file
in your program using:

```
<script src='js/CsoundObj.js'></script>
```

Then, using JavaScript, you will need to call the static importScripts() method
on CsoundObj.  This method will, depending on whether AudioWorklet or
ScriptProcessorNode is available, load the appropriate .js and other files and
setup the necessary WebAudio features.  The importScripts() method takes in an
optional argument for location of Csound .js and .wasm files (e.g.,
importScripts('./js/')).  

importScripts() itself returns a Promise.  When calling importScripts(), one should pass
in a callback function (typically one uses an arrow function here).  The callback function
will be executed once the importScripts process is complete and the system is ready to
for use. 
  
See the examples provided in the examples folder for demo code that shows including CsoundObj.js, 
calling importScripts(), and using a callback to setup and run Csound. 


## About the Implementations

WebAudio Csound currently has two backends: 

1. WebAssembly + AudioWorklet 
2. WebAssembly + ScriptProcesorNode 

Both backends use Csound compiled into WebAssembly. The AudioWorklet version will be used if the 
browser supports it as it is better performing for realtime audio and runs Csound in a separate thread
from the main thread.  If AudioWorklet is not found, CsoundObj will fallback to using ScriptProcessorNode.
SPN has well-known issues with breakups in audio generation due to running in the main thread, but is 
also well supported due to be available for a longer period of time. by supporting both implementations,
WebAudio Csound supports a wide variety of browsers and devices.

 

