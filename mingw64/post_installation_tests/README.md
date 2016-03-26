# Post-Installation Tests

This directory contains simple all-in-one test pieces as a sanity check after
installation, for all major runtime environments. The tests were created to
_quickly_ test the Windows installer for 64 bit CPU architecture, but they
might prove useful on other platforms. And of course the tests can be useful
for debugging Csound.

## Test Pieces

There is a test piece for each of the major runtime environments. Some of the
pieces work for more than one runtime environment. All test pieces work with
either MIDI or score input. Each test produces a half a minute
or so of real-time audio. The HTML tests produce visuals as well.

1. osc_server.csd should always be running in the background,
   start_osc_server.cmd starts it.
1. test.csd for the Csound command line.
2. test_vst.csd for CsoundVST.
1. test_with_html.csd for CsoundQt, NW.js, and Android.
1. test_in_html.html for NW.js.
1. test.py to test the Python interface.
1. test.lua to test the Lua interface.
1. test.java to test the Java interface.

## Features Tested

These tests exercise the following features, but all features are not
necessarily tested in each test piece.

1. The OSC opcodes.
1. The signal flow graph opcodes.
1. The fluidsynth opcodes.
1. The STK opcodes.
1. The Lua opcodes.
1. The Python opcodes.
1. The vst4cs opcodes (requires the 64 bit Pianoteq VST plugin).
