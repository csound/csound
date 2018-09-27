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


DRAFT   DRAFT   DRAFT   DRAFT   DRAFT   DRAFT   DRAFT   DRAFT   DRAFT   DRAFT



# CSOUND VERSION 6.12 RELEASE NOTES

Many changes including the removal of vst2cs functionality due to a copyright issue.

The changes made in 6.11 to raw format reading have been modified so
gen1 and diskin ignore positive file formats and use the file header,
unless the format is negative when it uses the absolute value in a raw
audio file.  This should preserve most compatibility issues.

There are a number of new and improved opcodes, new facilities in
scores and many bug fixes.


-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- fluidinfo retrieves program information from a currently loaded soundfont.

- New opcode ftaudio writes a ftable to an audio file; irate and triggered
    k-rate version exist and k-rate version supports sync or async writing.

- Version of OSClisten that writes the data to a k-rate array now exists.

- OSCcount returns the number of OSC incoming messages pending

- faustplay and faustdsp are new opcodes splitting the faust DSP
  instantiation and performance.

- OSCbundle sends a collection of similar OSC messages as a single packet
  for efficiency.

### New Gen and Macros

-

### Orchestra

- New preprocessor option #includestr.  This is like #include but has
  macro expansion in the double-quote delimited string

- Use of tied notes in subinstr fixed.

- Nesting macro calls more than about 10 caused a crash; now unlimited nesting works

- runtime error message now (usually) include a line number and a file/macro trace

- multiple assignments such as ka,kb=1,2 are again supported by the
parser; it had inadvertently got lost.

### Score

- New preprocessor option #includestr.  This is like #include but has
  macro expansion in the double-quote delimited string

- 'd' score opcode for real-time performance.  This was issue #966.

- bug in np operation fixed.

- Use of [] syntax in a score could lead to a loss of precision for
  numbers over about 1 million; older version restored.

- Nesting macro calls more than about 10 caused a crash; now unlimited nesting works

- The forms for delayed ending of sections (e 5 or s 5) now work with
  fractional delays; previously only read the integer part.

### Options

-

### Modified Opcodes and Gens

- Add optional argument to ftom for rounding answer to integer

- flooper2 and syncgrain etc  now allow resampling.

- chnclear can now take a list of channels to clear instead of just one.

- printf and printf_i now are like the manual in all arguments beyond the
  format and trigger are optional

- prints and printks can take string arguments printed with %s

- GEN2 can now take a size of zero, which s interpreted as size
sufficient for the number of values provided.

- faustcompile now includes a new optional parameter to allow it to be
run in a blocking mode. Defaults (as before) to non-blocking.

### Utilities

- A coding error in mixer was fixed.  It was very broken.

### Frontends

- Belacsound:

- CsoundQt:

### General Usage

## Bugs Fixed

- A typo in p5glove meant that the command to read the button status
  as a bitmap only returned state of button A.

- diskin to array fixed and also use with small ksmps.

- in locil it sometimes failed to deal with the ibas argument; this
  has now been reworked to be correct.

- madsr could overflow an internal counter when given a negative p3.

- Fixed mapping from threads to lua_States (issue #959).

- the time calculation in flooper2, flooper, and syncgrain was corrected.

- resampling and pitch fixed in pvstanal

- Rare buffer overflow case in faust opcodes fixed.

- pvsftw had an incorrect check for fft format which led to incorrect
  claim of bad format; fixed.

- if ksmps was 1 the opcode linenr at arate failed to work; fixed.

- window opcode fixed.

- The test for compatible subtypes of f-values in a number of pvs
  opcodes was wrong, causing spurious error messages.

- cosseg was broken for more than one segment; now OK

# SYSTEM LEVEL CHANGES

- the various -zN options now reports the number of opcodes for the request, so
differs with respect to deprecated and polymorphic opcodes.

### System Changes

- Recompilation of named instruments totally reworked to avoid errors
  and memory leaks.

- The allocation of instrument names to internal numbers has been
  rewritten and should now be usable with replacements in live coding.

- Printing the number allocated to a named instrument now behaves the
  manual, not just for debug.

- if liblo version 0.29 is available csound can be built to use it (with
  a compiler flag LIBLO29) and this fixes some bugs related to
  heavy/complex use of OSClisten.  Unfortunately the older 0.28 version
  is being distributed by some Linux distros.

- The orchestra compiler has a number of new optimisations, avoiding
unnecessary assignments and doing some more expression optimisations.

### Translations

### API

- csound->ReadScore was changed so it behaves the same as a score in a
  csd or sco file.  This could incorrectly give an infinite score or not
  in unexpected cases.

### Platform Specific

- WebAudio:

- iOS

- Android

- Windows

- MacOS

- GNU/Linux

- Bela
 - allow analog in and out with different channel numbers

==END==
