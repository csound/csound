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

========== DRAFT ========== DRAFT ========== DRAFT ========== DRAFT ========
# CSOUND VERSION 6.13 RELEASE NOTES

Not many new opcodes but there are a significant number of opcodes being
extended to use arrays in a variety of ways, widening the options for
users.  There have been many fixes to the core code as well as opcodes.
-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- string2array is a variant of fillarray with he data coming from a
  string of space separated values.

- nstrstr returns the name string of an instrument number or an empty
  string if the number does not refer to a named instrument.

- ntof converts notename to frequency at i- and k-time.

### New Gen and Macros

### Orchestra

- The consistency of kr, sr and ksmps reworked especially when there
  is overriding.

- corrected default 0bdfs usage.

- Resolving the path for #include files reworked to be more liberal.

- reading and writing to multidimensional arrays was very wrong.  This
  is now correct.

- Better checking for unknown arrray types (issue #1124)

### Score

- The characters n and m could erroneously get ignored in scores.

- Resolving the path for #include files reworked to be more liberal.

- After an error the backtrace of files and macros incorrectly read
  the information for orchestra rather than scores; fixed.

- The end of an r (repeated) section was not always correct.

- Nested {} parts of a score could lead to errors.

### Options

-

### Modified Opcodes and Gens

- Sending failure in OSC is now a warning rather than an error.

- passign can now have an array as the target.

- version of bpf/bpfcos added to allow points defined via arrays.

- grain can now use tables of any size, which was only a power of 2.

- Changing colours in FL widgets now works (it previously did not redraw
  the colour).

- fillarray can read from a file of values overcoming the argument
  limit.

- sumarray now works for audio arrays as well as for scalar values.

- assignment of an audio value to an audio array now works.

- monitor was broken in the array form.

- gendyc now respects sample-accurate mode.

- mtof and ftom now have array versions.

- sc_lag and sc_lagud now use the first k- or a-rate input when no
  initial value is given.

- printarray now works for string arrays.

- changed2 now works for strings.

- diskgrain, syncgrain and syncloop now can do sample rate scaling.

- GEN01 correctly reads raw audio files when requested.

- ftaudio can now take two additional optional argument`s for the start
  and end of the table data being written to file.

- sensekey recoded in the 'key down' mode.

- loscilx can return an audio array.

- schdule opcode reports undefined instruments in all cases.

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

- FLgetsnap fixed.

- directory fixed regarding file extensions.

- FLsetText reused a string incorrectly which led to incorrect values.

- fmb3 failed to initialise the lfo rate in some cases.

- ftaudio at i-rate was totally broken.

- following a reinit printks could be skipped; fixed

# SYSTEM LEVEL CHANGES

- Hash Table implementation modified to expand on load for better performance
  when map contains large number of entries

### System Changes

- plugin GEN functions can have a zero length, but the code must check
  for this and act accordingly.  This allows for deferred allocations.

### Translations

### API

- find_opcode_new and find_opcode_exact now exposed in API.

- After a reset a default message string callback handle is configured.

- New function csoundSystemSr addd to the API to read hardware-imposed samplr rate.

### Platform Specific

- WebAudio: libsndfile now compiled with FLAC and OGG support.

- iOS

- Android

- Windows

- MacOS

- GNU/Linux

- Haiku port now available.

- Bela
 - allow analog in and out with different channel numbers.

==END==

-----------------------------------------------------------------------
The following may need an entry above
------------------------------------------------------------------------
commit ac824f002e7f0e28c664600ac4d7d700ce9113a8 (HEAD -> develop, origin/develop, origin/HEAD)
Date:   Sun Apr 14 06:23:26 2019 +0200

commit 9004981506b4639c4e58b6bd59f5e654bcca42ba
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Apr 12 21:40:30 2019 +0100

    sample-accurate for array arithmetic and assignments

commit 3be0e5eed52785b80b9bf7fac7909245a28dd192
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Apr 9 08:43:18 2019 +0100

    fixed CPOF midi functions


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
