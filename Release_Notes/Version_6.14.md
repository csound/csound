<!---

To maintain this document use the following markdown:
n
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

# CSOUND VERSION 6.14 RELEASE NOTES

A number of bug fixes and enhancements, but also potentially
significant changes are included.

For live coders the orchestra macros are now remembered between calls
to compilerstr.  This should not change the behaviour of current valid
orchestras but could be useful in live coding.

MIDI devices now include mapping multiple devices to higher channels.
The details are in the manual MIDI section.  

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- randc is like randi but uses a cubic interpolation.

- mp3out is an experimental implementation of writing an mp3 file.  It
  may be replaced by the current work in libsndfile to deal with MPEG
  files. See appendix.

- metro2 is like metro but with added controllable swing.

- ftexists reports whether a numbered ftable exists.

- schedulek is a k-time opcode just like schedule.

- new array based channel opcodes: chngeti, chngetk, chngeta, chngets,
  chnseti, chnsetk, chnseta, chnsets.

- lastcycle ientifies the last k-cycle of an instrument instance.

- strstrip removeswhitespace from both ends of a string.


### New Gen and Macros

### Orchestra

- The conditional expression syntax a?b:c incorrectly always
  calculated b and c before selecting which to return.  This could
  give incorrect division by zero errors or cause unexpected multiple
  evaluations of opcodes.  It now implements the common C-like semantics.

- Orchestra macros are now persistent, so they apply in every
  compilation after they are defined until they are undefined.  It has
  been changed because of the need of live coding in particular.  A
  correct orchestra should not be affected.

- Following a syntax error there were cases when Csound gave a
  segmentation error.  This is now fixed.

### Score

- 
  
### Options

- New option simple-sorted-score creates file score.srt in a more
  user-friendly format

- Revise treatment of CsOptions wrt double quotes and spaces which need escaping.

- Setting the 1024 bit in -m suppresses printing of messages about
using deprecated opcodes.  This option is itself deprecated.

### Modified Opcodes and Gens

- squinewave now handles optional a or k rate argument.

- pindex opcode handles string fields as well as numeric ones.

- sflooper reworked to avoid a crash and provide warnings.

- event_i and schedule can take fractional p1.

- in sound font opcodes better checking.   Also no longer will load
  multiple copies of a sound font, but reuses existing load.

- fluidControl has a new optional argument to control printing
  messages.

- bpf has an audio version now.

- stsend/stecv can work with unmatched k-rates.

- pvstrace has new optional arguments.

- lpfreson checks number of poles.

- syncloop had a small typing error that caused crashes.

- bpfcs has new array versions.

- zacl can omit second argument, defaults to clearing only the given
  channel.

- outvalue attempted to use a k-rate value which could be invalid at
  the time.  This is mainly a small performance problem, and it is now
  eliminated.

- Channel names for chnget and chnset opcodes can now be updated at k-rate so
  they can be called within a loop.

- copya2ftab now has an optional additional argument which is an
  offset into the ftable for where to copy the array.


### Utilities

- lpanal now checks that sufficient poles are requested.

### Frontends

- Belacsound:

- CsoundQt:
  Released to coincide with Csound 6.14 there is CsoundQt 0.9.7
  See Release notes [https://github.com/CsoundQt/CsoundQt/blob/master/release_notes/Release%20notes%200.9.7.md]


### General Usage

- // comments at the start of a line now accepted in CsOptions
  section of a csd file.

- Option --orc has been corrected so it runs without a score; that is for ever until an exit condition.


## Bugs Fixed

- shiftin fixed.

- exitnow delivers the return code as documented.

- fixed bug in beosc, where gaussian noise generator was not being
  initialised.

- OSCraw fixed.

- ftkloadk could select incorrect internal code causing a crash.

- GEN01 when used to read a single channel of a multi-channel file got
  the length incorrect.

- ftgenonce had a fencepost problem so it could overwrite a table in
  use.

- a race condition in Jacko opcodes improved (issue #1182).

- syncloop had a small typing error that caused crashes.

- lowresx was incomplete and did not work as intended; rewritten (issue #1199)

- if outch was incorrectly given an odd number parameters it would give
  a segmentation error.  This now gives an error message.

# SYSTEM LEVEL CHANGES

-

### System Changes

- New plugin class for opcodes with no outputs written.
perform time errors and init errors are also reported in the return code of
the command line system.  The new API function GetErrorCnt is
available to do something similar in other variants.
- 

### Translations

### API

- Function GetErrorCnt gives the number of perf-time errors, and adds
  in the init-time errors at the end of a rendering.

- Function FTnp2Find no longer prints messages if the table is not
found, but just returns NULL.  Previous behaviour is available  as
FTnp2Finde.

- csoundGetInstrument() added

### Platform Specific

- WebAudio: 
 - added csoundCompile to CsoundObj that adds commandline args so that one can
   override CsOptions values in CSDs 
 - added getPlayState(), addPlayStateListener(), and other methods to CsoundObj
   for querying and listening to changes of play state

- iOS

- Android

- Windows
 - stsend reworked for winsock library

- MacOS

- GNU/Linux

- Haiku port

- Bela

### Appendix: Experimental mp3out

Simple example:
```
<CsoundSynthesizer>

<CsInstruments>
ksmps = 1000

instr 1
  aa diskin "fox.wav", 1
  mp3out  aa,aa,"test.mp3"
  endin
</CsInstruments>

<CsScore>
i1 0 3
e
</CsScore>

</CsoundSynthesizer>
```

The syntax is `mp3out aleft,aright, Sfilename` with three optional arguments `mode` (0=stero, 1=Jointstereo (default), 3=Mono),  `bitrate` defaulting to 256, and `quality` (in range 1 to 7) defaulting to 2 (high quality).


==END==
