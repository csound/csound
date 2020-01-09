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
======== DRAFT ======== DRAFT ======== DRAFT ======== DRAFT ======== DRAFT ========

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
  files.

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
  the time.  This is mainly a small performance problem, and its now
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

### General Usage

- // comments at the start of a line now accepted in CsOptions
  section of a csd file. 

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

- iOS

- Android

- Windows
 - stsend reworked for winsock library

- MacOS

- GNU/Linux

- Haiku port

- Bela

==END==

-----------------------------------------------------------------------
------------------------------------------------------------------------

commit bf1f61f7df4ec8b98e4e62116991b5555a539ee1 (HEAD -> develop, origin/develop, origin/HEAD)
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jan 3 21:04:57 2020 +0000

commit ad5e171bb1e9c0a5e4d5419531344db0b32352dd
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Dec 3 11:02:15 2019 +0100

    expose csoundCompile with extra commandline arg to allow overriding -o if it is in CsOptions

commit a4706809398daf2eddb0b778bfe4e2c57d3dce64
Merge: c04f371afd dbd36d56e2
Author: Pete Goodeve <pete.goodeve@computer.org>
Date:   Tue Oct 29 17:16:21 2019 -0700

    resolve sfont.c confusion

commit 683eeb5a6605e6b179894a2438e496d6b4a9f3dc
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Sep 24 20:38:20 2019 +0100

    added some more functionality to CPOF

commit 797f3098efdc06e587fe8d45a4b9b6b4fc4daec0
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu Sep 19 13:53:55 2019 -0400

    added getPlayState(), addPlayStateListener(), and other methods for querying and listening to the playing state of Csound

