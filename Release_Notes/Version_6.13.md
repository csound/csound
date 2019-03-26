<!---

To maintain this document use the following markdown:

# First level heading
## Second level heading
### Third level heading

- First level bullet point
 - Second level bullet point
  - Third level bullet point

`inline code`

``` preformatted text etc.  ```

[hyperlink](url for the hyperlink)

Any valid HTML can also be used.

 --->

========== DRAFT ========== DRAFT ========== DRAFT ========== DRAFT ========
# CSOUND VERSION 6.13 RELEASE NOTES

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- 

### New Gen and Macros

- string2array is a variant of fillarray with he data coming from a
  string of with space separated values.

- nstrstr returns the name string of an instrument number or an empty
  string if the nuber does not refer to a named instrument.
  
- ntof converts notename to frequency at i- and k-time.

### Orchestra

- The consistency of kr, sr and ksmps reworked especially when there
  is overriding.
  
- corrected default 0bdfs usage.

- Resolving the path for #include files reworked to be more liberal.

- reading ad writing to multidimensional arrays was very wrong.  Tis
  is now correct.
  
### Score

- The characters n and m could erroneously get ignored in scores.

- Resolving the path for #include files reworked to be more liberal.

- After an error the backtrace of files and macros incorrectly read
  the information for orchestra rather thanscores; fixed.
  
- The end of an r (repeated) section was not always correct.

- Nested {} oarts of a score could lead to errors.

### Options

-

### Modified Opcodes and Gens

- Sending failure in OSC is now a warming rather than an error.

- passign can now have an array as the target

- version of bpf/bpfcos addd to allow points defined via arrays

- grain can now use tables of any size, which was only a power of 2.

- Changing colurs in FL widgets now works (it previosly did nor redraw
  the colour). 
  
- fillarray can read from a file of values overcoming the argument
  limit.
  
- sumarray now works for audio arrays as well as forr scalar values.

- assignment of an audio value to an audio array now works.

- monitor was broken in the array form.

- gendyc now repects sample-accurate mode.

- mtof and ftom nowhave array versions.

- sc_lag and sc_lagud now use the first k- or a-rate input when no
  initial value is given.
  
- printarray now works for string arrays.

- changed2 noew works for strings.

- diskgrain, syncgrain and syncloop now can do sample rate scaling.

- GEN01 correctly reads raw audio files when requested.

- ftaudio can now rtake twp atdotional optional argumens for the start
  and end of the table data being written to file.
  
- sensekey recoded in the 'key down' mode.

- loscilx can return an audio array.

### Utilities

- hetro had a number of fixes and improvements.


### Frontends

- Belacsound:

- CsoundQt:

### General Usage

- There have been a number of improvements in the semantics for multicore;
  most of these are corrections with a few efficiency gains. 

- There is a maximum number of arguments for an opcode which was
  neither explicit nor policed.  Attempts to use too many arguments
  now gives a syntax error.
  

## Bugs Fixed

- flgetsnap fixed

- directory fixed regarding file extensions.

- FLsetText reused a string incorrectly which led to incorrect values.

- fmb3 failed to initialise the lfo rate in some cases.

- ftaudio at i-rate was totally broken.

# SYSTEM LEVEL CHANGES

- 

### System Changes

- 

### Translations

### API

- find_opcode_new and find_opcode_exact now exposed in API.

### Platform Specific

- WebAudio:

- iOS

- Android

- Windows

- MacOS

- GNU/Linux

- Bela
 - allow analog in and out with different channel numbers.

==END==
------------------------------------------------------------------------
commit 1b8a1a7cc3c89a44d193d93b873482dc2e5807d2
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Mar 25 15:07:13 2019 +0000

commit 8276f5097ff8847617eec4e48d6191b829e42763
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Mar 22 13:23:41 2019 +0100

    fix handling of %d in printarray, plus other bugs


commit 0b4c6edb9f8631b2a1d207b25432326802518a04
Author: Felipe Sateler <fsateler@gmail.com>
Date:   Sun Feb 3 18:54:08 2019 -0300

    perfThread: wait the recording thread before waiting for the performance thread
    
    If we don't tell the recording thread to stop, we might enter a deadlock as the perf thread waits for the record
    thread but it has not been stopped yet.
    
    Fixes #1103

Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Jan 9 11:24:23 2019 +0000

    fixing midi ctl opcodes

commit 742fc16511f7511761f13aad9059e1b56cdda1b3
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Dec 3 09:35:36 2018 +0000

    midiVelocityAmp printing

commit da9e67c3a17c1be6dc5b1d842fdf7394dd018ebb
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Nov 21 17:25:50 2018 +0000

    fixing ampmidid

commit 9d783d93de00ab3d6aeb1ea978ea66da9e59fc34
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Oct 24 22:46:10 2018 +0100

    fixing MIDI methods

