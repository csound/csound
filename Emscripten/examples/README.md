WebAudio Csound Examples
===============

In this directory, we are building a collection of step-by-step examples using WAAW Csound. To run these,
once the latest library is built, use `../update_example_libs_from_dist.shupdate-scripts.sh` to copy the
relevant code to the local `js` directory. You will also need a
http-server to serve the WebAudio Csound source files.


* [Random note generator](https://github.com/csound/csound/blob/develop/emscripten/examples/randgen.html):
This example demonstrates how to load the WASM Csound library and use the CsoundObj class to compile and perform 
simple synthesis code that is embedded in the HTML page.

* [Sliders](https://github.com/csound/csound/blob/develop/emscripten/examples/sliders.html):
This example demonstrates how to use channels to send control data into Csound from HTML 5
sliders.

* [Minimal](https://github.com/csound/csound/blob/develop/emscripten/examples/minimal.html):
This example demonstrates the csound.js javascript frontend that can
be used to port PNaCl Csound examples more conveniently. 

* [Canvas](https://github.com/csound/csound/blob/develop/emscripten/examples/canvas.html):
Canvas widget example ported from the PNaCl Csound collection.

* [StepSeq](https://github.com/csound/csound/blob/develop/emscripten/examples/tabex.html):
Step Sequencer example demonstrating the use of tables in Csound.

* [Reverb](https://github.com/csound/csound/blob/develop/emscripten/examples/reverb.html):
Reverb effect demonstrating realtime audio input (e.g. microphone).

* [MIDI](https://github.com/csound/csound/blob/develop/emscripten/examples/midi.html):
MIDI input example using HTML 5 buttons to generate MIDI data.

* [MIDIPlayer](https://github.com/csound/csound/blob/develop/emscripten/examples/midiplayer.html):
GM MIDI file player ported from the PNaCl Csound collection.

* [CSDPlayer](https://github.com/csound/csound/blob/develop/emscripten/examples/csdplayer.html):
Offline render example ported from the PNaCl Csound collection.

* [Stria](https://github.com/csound/csound/blob/develop/emscripten/examples/stria.html):
Stria by J Chowning CSD performance example ported from the PNaCl
Csound collection.



