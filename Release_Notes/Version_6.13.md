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

# CSOUND VERSION 6.13 RELEASE NOTES

Not many new opcodes but there are a significant number of opcodes being
extended to use arrays in a variety of ways, widening the options for
users.  There have been many fixes to the core code as well as opcodes.
-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- string2array is a variant of fillarray with the data coming from a
  string of space separated values.

- nstrstr returns the name string of an instrument number or an empty
  string if the number does not refer to a named instrument.

- ntof converts notename to frequency at i- and k-time.

- ampmidicurve is a new opcode that maps an input MIDI velocity number to an
  output gain factor with a maximum value of 1, modifying the output gain by
  a dynamic range and a shaping exponent.

### New Gen and Macros

### Orchestra

- The consistency of kr, sr and ksmps reworked especially when there
  is overriding.

- corrected default 0bdfs usage.

- Resolving the path for #include files reworked to be more liberal.

- reading and writing to multidimensional arrays was very wrong.  This
  is now correct.

- Better checking for unknown array types (issue #1124)

- In all array operations the size of an array is determined at init time and no 
  allocation happens at perf time.
  
- array arithmetic now respects --sample-accurate.

### Score

- The characters n and m could erroneously get ignored in scores.

- Resolving the path for #include files reworked to be more liberal.

- After an error the backtrace of files and macros incorrectly read
  the information for orchestra rather than scores; fixed.

- The end of an r (repeated) section was not always correct.

- Nested {} parts of a score could lead to errors.

- After an s statement a newline was required; no longer needed.

- The {} score loops have been reworked to allow macros and expressions
  in the loop count.
  
### Options

- The new option --use-system-sr set the sample rate to the hardware/system value.

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

- schedule opcode reports undefined instruments in all cases.

- event_i now accepts tagged instrument numbers.

- printarray treats %d correctly.

- beadsynt now works with i arrays as well as k arrays as in the manual.

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

- printks fixed so it prints at correct times.

- tabrowlin and getrowlin would calculate wrong size under certain conditions.

# SYSTEM LEVEL CHANGES

- Hash Table implementation modified to expand on load for better performance
  when map contains large number of entries

### System Changes

- plugin GEN functions can have a zero length, but the code must check
  for this and act accordingly.  This allows for deferred allocations.

- schedule reports undefined instr numbers/names and continues, rather than 
  causing an error.
  
- allow multiple calls to midi out controls.

### Translations

### API

- find_opcode_new and find_opcode_exact now exposed in API.

- After a reset a default message string callback handle is configured.

- New function csoundSystemSr added to the API to read hardware-imposed sample rate.

### Platform Specific

- WebAudio: libsndfile now compiled with FLAC and OGG support.

- iOS

- Android

- Windows
 - In both orchestra and score the path tracking of #include expects a \ separator.

- MacOS

- GNU/Linux

- Haiku port now available.

- Bela
 - allow analog in and out with different channel numbers.

==END==

