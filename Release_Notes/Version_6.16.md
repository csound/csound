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

- triglinseg and trigexpseg are triggered versions of linseg and
expseg

-  vclpf is a 4-pole resonant lowpass linear filter based on a tipical
analogue filter configuration.

- spf is a second-order multimode filter based on the Steiner-Parker
configuration with separate lowpass, highpass, and bandpass inputs
and a single output.

- skf is a second-order lowpass or highpass filter based on a
linear model of the Sallen-Key analogue filter.

- svn is a non-linear state variable filter with overdrive control
and optional user-defined non-linear map.

- autocorr computes the autocorrelation of a sequence stored in an array.

## New Gen and Macros

### Orchestra

- The operations += and -= now work for i and k arrays. 

### Score


### Options

- New options --limiter and --limiter=num (where num is in range (0,1]
inserts a tanh limiter (see clip version2) at the end of each k-cycle.
This can be used wen exerimenting or trying some alien inputs to save
your ears or speakers.  The default value in the first form is 0.5

- A typing error meant that the tag <CsShortLicense> was not recognised,
although the English spelling (CsSortLicence) was.  Corrected.

### Modified Opcodes and Gens

- slicearray_i now works for string arrays.

- OSCsend aways runs on the first call regardless of the value of kwhen.

- pvadd can access the interal ftable -1.

- pan2 efficiency improved in many cases.

- add version of pow for the case kout[] = kx ^ ivalues[]

- expcurve and logcurve now incorporate range checks and corrects end
  values.
  
- streaming lpc opcodes have had a major improvement in performance (>10x speedup for some cases),
due to a new autocorrelation routine.

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

- a bug in moogladder where the value of 0dbfs affected the outout is
  now fixed.

- bugs in several filters where istor was defaulting to 1 instead of 0 as described in the manual have 
been fixed.

# SYSTEM LEVEL CHANGES


### System Changes

 - new autocorrelation routine can compute in the frequency or
in the time domain. Thanks to Patrick Ropohl for the improvement suggestion.

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

commit 25c1e6bf2a0077b89a2859b92a529fc0c77e4919 (HEAD -> develop, origin/develop
, origin/HEAD)
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Sun Feb 7 09:50:56 2021 +0100

    Updated French translation



**END**

