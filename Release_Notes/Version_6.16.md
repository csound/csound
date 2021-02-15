<!---

To maintain this document use the following markdown:

# First level heading
## Second level heading
### Third level heading

- First level bullet point
  - Second level bullet point
    - Third level bullet point

`inline code`

``` pre-formatted text etc.  ```

[hyperlink](url for the hyperlink)

Any valid HTML can also be used.

 --->

# CSOUND VERSION 6.16 RELEASE NOTES - DRAFT - DRAFT - DRAFT - DRAFT 

This is mainly a bug fixing release but there are new opcodes, including
support for simpler use of MIDI controls and a new opcode to connect
to an Arduino.  Also there is an optional limiter in the soun outpur chain.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- cntDelete deltes a counter object

- lfsr, a linear feedback shift register opcode for psuedo random
  umber generation.

- ctrlsave stores the currrent values of MIDIcontrollers in an array.

- turnoff3 extends tuning off to remove instrument instances that are
  queued via scheduling but not yet active. *** NOT DOCUMENTED***

- ctrlprint prints the result of a ctrlsave call in a form that can be
  used in an orchestra.

- ctrlprintpresets prints the MIDI controller preset.

- ctrlselect selects a preset for MIDI controller values.

- ctrlpreset defines a preset for MIDI controllers.

- outall writes a signal to all output channels.

- scale2 is similar to scale but has different argument order and has
an optional port filter.

- aduinoReadF extends the arduino family to transfer floating point
  values.

- triglinseg and trigexpseg are triggered versions of linseg and expseg

## New Gen and Macros

### Orchestra

- The operations += and -= now work for i and k arrays. 

### Score


### Options

New options --limiter and --limiter=num (where num is in range (0,1]
inserts a tanh limiter (see clip version2) at the end of each k-cycle.
This can be used wen exerimenting or trying some alien inputs to save
your ears or speakers.  The default value in the first form is 0.5

### Modified Opcodes and Gens

- slicearray_i now works for string arrays.

- OSCsend aways runs on the first call regardless of the value of kwhen.

- pvadd can access the interal ftable -1.

- pan2 efficiency improved in many cases.

- add version of pow for the case kout[] = kx ^ ivalues[]

- expcurve and logcurve now incorporate range checks and corrects end
  values.
  
### Utilities


### Frontends


### General Usage

- Csound no longer supports Python2 opcodes follow end of life.

## Bugs Fixed

- the wterrain2 opcode was not correctly compiled on some platforms.

- fprintks opcode crashed when format contains one %s

- bug in rtjack when number of outputs differed from the number in
  inputs.

- FLsetVal now works properly with "inverted range" FLsliders.

- conditional expressiond with a-rate output now work corrctly.

- bug in --opcode-dir option fixed.

- sfpassign failed to remember the number of presets causng
  confusion.  This also affects sfplay ad sfinstrplay.

- midiarp opcode fixed (issue 1365)

- a bug in moogladdet where te value of 0dbfs affected te outout is
  now fixed.

# SYSTEM LEVEL CHANGES


### System Changes


### Translations

### API


### Platform Specific

- WebAudio: 
 
- iOS

- Android

- Windows

- MacOS

- GNU/Linux

- Haiku port

- Bela


==END==

commit 47a9cf5d8b0093f3840f122654f49a406fb1b240
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jan 29 10:00:45 2021 +0000

    fixed values of vt for moogladder 0dbfs != 32768

**END**

