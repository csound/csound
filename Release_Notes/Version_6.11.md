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

# CSOUND VERSION 6.11 RELEASE NOTES

There has been a great amount of internal reorganisation, which should
not affect most users.  Some components are now independently managed
and will eventually be installable via a new package manager.  The
realtime option is now considered stable and has the "experimental"
tag removed.  There have been more steps towards completing the
arithmetic operations involving a-arrays.

Note that changes to GEN01 and diskin2 may not be backward copatible
if a none zero value is given for the format.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- loscilphs, loscil3phs are like loscil but return the phase in
addition to the audio.

- more arithmetic, between a-rate arrays and a-rate values; this
  completes the arithmetic where one or more argument is an audio
  array.

- balance2 is like balance but scales the output at a-rate, rather than
  k-rate.

### New Gen and Macros

- 

### Orchestra

- 

### Score

- Characters following a \ in a score string are treated as escaped, so
  \n is a newline rather than two characters.   It handles escaped a,
  b, f, n, r, t and v.  Other characters following a \ ignore the \.

### Options

- 

### Modified Opcodes and Gens

- print, printk2 now take an additional optional argument which if
  non-zero causes the k-variable name to be printed as well as the value.

- getrow, setrow, getcol, and setcol can now act on i-rate arrays.

- diskin2 was incorrectly described in the manual with respect to the
  iformat parameter.  Now if iformat is zero the file is expected to
  have an audio header; if in the range 1-10 (rather than 0-9 as
  before) then it is opened as raw with the specified sample format.
  ***THIS MAY BE INCOMPATIBLE***.  For most users the value of zero will
  be correct.

- GEN01 now uses format 0 to get the file type from the header; any
  other value indicates a raw file. ***THIS MAY BE INCOMPATIBLE***.  For
  most users the value of zero will be correct.

- GEN01 was incorrectly documented with respect to the format
  argment.  There are 9 raw formats whereas the earlier manual stated 6,

- Small change in slicearray should allow it to be used in fuctional
  form in most cases.

### Utilities

-

### Frontends

- icsound:

- csound~:

- csdebugger:

- Emscripten: 

- CsoundQt: 

### General Usage

## Bugs Fixed

- linen was reworked to allow for long durations and overlaps.

- resetting csound caused a crash if Lua opcodes were used; fixed.

- The poscil family of opcodes could give incorrect results if used in
  multi-core orchestras AND another instrument changed the f-table.
  This is now corrected.

- use of out with an audio array did not check that the array
  dimension was not greater than the number of channels, which caused
  a crash.  It is now checked and truncated if too large with a
  warning.

- Bug in stereo versions of loscil introduced distortion; now fixed.

- Fencepost error in phs fixed.

- gen49 read deleted memory if the file was not found; fixed

- Loading of LADSPA plugins when relying on search paths was wrong and
  mangled the name; now fixed.


## SYSTEM LEVEL CHANGES

- OPCODE6DIR{64} now can contain a colon-separated list of directories.

### System Changes

- The somewhat curious distinction between k-rate and a-rate perf-time
  has been removed throughput, so only threads 1, 2 and 3 are used,
  and the s-type output is not used and some x-type inputs, having
  been changed for direct polymorphism.  This only matters for opcode
  writers as the s, and x codes and threads 4,5,6 and 7 will be
  removed in a while.

### Translations

- As ever the French translations are complete.

- 

### API

- 

### Platform Specific

- iOS

 -

- Android

 -

- Windows

- OSX

- GNU/Linux



==END==


========================================================================
