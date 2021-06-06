
# CSOUND VERSION 6.16 RELEASE NOTES - DRAFT - DRAFT - DRAFT - DRAFT 

This delayed release was mainly a bug fixing release but there
are significant new opcodes, including support for simpler use of MIDI
controls and a new opcode to connect to an Arduino.  Also there is an
optional limiter in the sound output chain.

Also there are a number of new filters including including the final
opcode written by the 'father of computer music' Max Mathews (with
Julius Smith)! ported by Joel Ross.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- cntDelete deletes a counter object

- lfsr, a linear feedback shift register opcode for pseudo random
  number generation.

- ctrlsave stores the current values of MIDI controllers in an array.

- turnoff3 extends tuning off to remove instrument instances that are
  queued via scheduling but not yet active.

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

- vclpf is a 4-pole resonant lowpass linear filter based on a typical
analogue filter configuration.

- spf is a second-order multimode filter based on the Steiner-Parker
configuration with separate lowpass, highpass, and bandpass inputs
and a single output.

- skf is a second-order lowpass or highpass filter based on a
linear model of the Sallen-Key analogue filter.

- svn is a non-linear state variable filter with overdrive control
and optional user-defined non-linear map.

- autocorr computes the autocorrelation of a sequence stored in an array.

- turnoff2_i is init-time version of turnoff2.

- scanu2 is a revision of scanu to make it closer to the original
  concept.  Damping now a positive value and plucking initialisation
  totally rewritten, plus various code improvements.

- st2ms and ms2st are stereo to MS and vice-versa conversion opcodes
with stereo width control.

- mvmfilter is a filter with pronounced resonance and controllable decay time.

###New gen

- gen44 allows the writing of stiffness matrices for scanu/scanu2 in a
  textual format.

### Orchestra

- The operations += and -= now work for i and k arrays. 

-  k-rate array operators are now only processed at k-rate.
    
### Score

- An instance of a named instrument can have a decimal fraction added
  to the name to allow for tied notes and other facilities that were
  only avaliable for numbered instruments.

### Options

- New options --limiter and --limiter=num (where num is in range (0,1]
inserts a tanh limiter (see clip version2) at the end of each k-cycle.
This can be used when experimenting or trying some alien inputs to save
your ears or speakers.  The default value in the first form is 0.5

- A typing error meant that the tag <CsShortLicense> was not recognised,
although the English spelling (CsSortLicence) was.  Corrected.

- New option --default-ksmps=num changes the default value from the
internal fixed number.

- New environment variable `CS_USER_PLUGINDIR` has been added to
indicate a user plugin dir path (in addition to the system plugin
directory). This added search path defaults to standard locations
on different platforms (documented in the manual).


### Modified Opcodes and Gens

- slicearray_i now works for string arrays.

- OSCsend always runs on the first call regardless of the value of kwhen.

- pvadd can access the internal ftable -1.

- pan2 efficiency improved in many cases.

- Add version of pow for the case kout[] = kx ^ ivalues[]

- expcurve and logcurve now incorporate range checks and corrects end
  values.
  
- Streaming LPC opcodes have had a major improvement in performance
  (>10x speedup for some cases), due to a new autocorrelation routine.

- Restriction on size of directory name size in ftsamplebank removed.

- If a non string is passed to sprintf to be formatted as a %s an error
  is signalled.

- readk family of opcodes now support comments in the input which is ignored.

- Added iflag parameter to sflooper.

- Opcodes beosc, beadsynt, tabrowl, and getrowlin removed.

### Utilities

### Frontends

### General Usage

- Csound no longer supports Python2 opcodes following end of life for Python 2.

## Bugs Fixed

- The wterrain2 opcode was not correctly compiled on some platforms.

- fprintks opcode crashed when format contains one %s.

- Bug in rtjack when number of outputs differed from the number in
  inputs.

- FLsetVal now works properly with "inverted range" FLsliders.

- Conditional expressions with a-rate output now work correctly.

- Bug in --opcode-dir option fixed.

- sfpassign failed to remember the number of presets causing
  confusion.  This also affects sfplay ad sfinstrplay.

- midiarp opcode fixed (issue 1365).

- A bug in moogladder where the value of 0dbfs affected the output is
  now fixed.
 
- Bugs in several filters where istor was defaulting to 1 instead of 0
  as described in the manual have been fixed.

- Bug in assigning numbers to named instruments fixed.  This
  particularly affected dynamic definitions of instruments.

- Use of %s format in sprintf crashed if the corresponding item was not a
  string.  Thus now gives an error.

- Fix bug in ftprint where trigger was not working as advertised.

- Asynchronous use of diskin2 fixed.

- pvs2tab does not crash in the sliding case but gives a error.

- In some circumstances turnoff2 could cause the silencing of another
  instrument for one k-cycle. This is now fixed.

- timeinstk behaved differently in a UDO to normal use.  This has been
  corrected.

- Fixed midiarp init method as it was causing an issue with one of the arp modes.

- Fixed scaling of attack stage of xadsr.

- Fix printarray behaviour for string arrays (default is to print at
  every cycle); added a variant without trigger.

- Bug with named instrument merging in new compilations fixed.

- ATSA opcode atsinnoi could cause memory problems.  The 6.16 release is better but not yet verified.

# SYSTEM LEVEL CHANGES

### System Changes

 - New autocorrelation routine can compute in the frequency or
in the time domain. Thanks to Patrick Ropohl for the improvement
suggestion.

- Minimum cmake version bumped to 3.5.

- Image opcodes removed, now in plugin repo.

- faust opcodes removed, now in plugin repo.

- Python opcodes are now in plugin repo.

- Ableton Link opcodes moved to plugins repo.

### Translations

### API

### Platform Specific

- WebAudio
 
- iOS

- Android

- Windows

- MacOS:
    
   Some opcode libs with dependencies have been removed from release. Image
   opcodes, Python opcodes, Faust opcodes, and FLTK Widget opcodes have
   been moved to a separate repository and are not included anymore.
   
- GNU/Linux

- Haiku port

- Bela

==END==
    
