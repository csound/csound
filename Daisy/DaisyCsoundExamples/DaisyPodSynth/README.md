# DaisyPodSynth

This example provides a basic template for Csound MIDI synth on the Daisy Pod.
It uses DaisyPod-specific code to access controls. Unlike the basic MIDI example,
it takes all control data through software bus channels:

- toggle1, toggle2, and etoggle: toggle data from switches 1 and 2, and encoder push button. 
- pressed1, pressed2, and epressed: button data from the same controls as above.
- pot1 and pot2: normalised (0 - 1) analogue data from knobs 1 and 2 (linear scale).
- encoder: encoder data, normalised (0 - 1).
- leds 1 and 2 indicate the state of toggle1 and toggle2

The C++ code  in this example is generic enough to be re-used in a variety of Csound synths by
replacing the header file containing the Csound code.




