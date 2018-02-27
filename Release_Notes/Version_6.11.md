
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

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- loscilphs, loscil3phs are like loscil but return the phase in
addition to the audio.

- more arithmetic, between a-rate arrays and a-rate values; this
  completes the arithmetic where one or more argument is an audio
  array. 

### New Gen and Macros

- 

### Orchestra

- 

### Score

- 

### Options

- 

### Modified Opcodes and Gens

- print, printk2 now take an additional optional argument which if
  non-zero causes the k-variable name to be printed as well as the value.

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

- linen was reworked to allow for long durations and overlaps

- resetting csound caused a crash if Lua opcodes were used; fixed

- The poscil family of opcodes could give incorrect results if used in
  multi-core orchestras AND another instrument changed the f-table.
  This is now corrected.

- use of out with an audio array did not check that the array
  dimension was not greater than the number of channels, which caused
  a crash.  It is now checked and truncated if too large with a
  warning.

- Bug in steeo versions of loscil introduced distortionl now fixed


## SYSTEM LEVEL CHANGES

- OPCODE6DIR{64} now can contain a colon-separated list of directories.

### System Changes

- The somewhat curious distinction between k-rate and a-rate perf-time
  has been removed throughput, so only threads 1, 2 and 3 are used,
  and the s-type output and x-type input are not used, having been
  changed for direct polymorphism.  This only matters for opcode
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
commit 91b9efa9f849d8cf8e28fbd6a173ea55034d0cbe (HEAD -> develop, origin/develop
)
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Feb 22 18:22:39 2018 +0000

**end**
