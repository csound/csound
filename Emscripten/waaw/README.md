Web Assembly Audio Worklet Csound Examples
===============

In this directory, we are building a collection of step-by-step examples using WAAW Csound. To run these,
once the latest library is built, use `update-scripts.sh` to copy the relevant code to the local `js` directory.
WASM code needs to be served via http or https protocols, the python script `httpd.py` can be used to run
a simple local web server for this purpose.

* [Random note generator](https://github.com/csound/csound/blob/develop/emscripten/waaw/RandomGenerator.html):
This example demonstrates how to load the WASM Csound library and use the CsoundObj class to compile and perform 
simple synthesis code that is embedded in the HTML page.

* [Sliders](https://github.com/csound/csound/blob/develop/emscripten/waaw/Sliders.html):
This example demonstrates how to use channels to send control data into Csound from HTML 5
sliders.

* [Minimal](https://github.com/csound/csound/blob/develop/emscripten/waaw/minimal.html):
This example demonstrates the csound.js javascript frontend that can
be used to port PNaCl Csound examples more conveniently. 

* [Canvas](https://github.com/csound/csound/blob/develop/emscripten/waaw/canvas.html):
Canvas widget example ported from the PNaCl Csound collection.

* [MIDIPlayer](https://github.com/csound/csound/blob/develop/emscripten/waaw/midiplayer.html):
GM MIDI file player ported from the PNaCl Csound collection.

* [CSDPlayer](https://github.com/csound/csound/blob/develop/emscripten/waaw/csdplayer.html):
Offline render example ported from the PNaCl Csound collection.

* [Stria](https://github.com/csound/csound/blob/develop/emscripten/waqw/stria.html):
Stria by J Chowning CSD performance example ported from the PNaCl
Csound collection.



