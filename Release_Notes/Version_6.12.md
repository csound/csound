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

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- fluidinfo retrieves program information from a currently loaded soundfont. 

- New opcode ftaudio writes a ftable to an audio file; irate and triggered
    k-rate version exist and k-rate version supports sync or async writing.

### New Gen and Macros

- 

### Orchestra

- New preprocessor option #includestr.  This is like #include but has
  macro expansion in the double-quote delimited string

- Use of tied notes in subinstr fixed.


### Score

- New preprocessor option #includestr.  This is like #include but has
  macro expansion in the double-quote delimited string

- 'd' score opcode for real-time performanceb.  This was issue #966.

-  bug in np operation fixed.

### Options

- 

### Modified Opcodes and Gens

- Add optional argument to ftom for rounding answer to integer

- flooper2 and syncgrain etc  now allow resampling. {IN MANUAL??]

### Utilities

- 

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

- window opcoe fixed.

# SYSTEM LEVEL CHANGES

- 

### System Changes

- Recompilation of named instruments totally reworked to avoid errors
  and memory leaks.

- The allocation of instrument names to internal numbers has been
  rewritten and should now be usable with replacements in live coding.

- Printing the number allocated to a named instrument now behaves the
  manual, not just for debug.

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


========================================================================


========================================================================
commit 84b777591e72b288eb0558ca0cb6f58a8af0b263
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Jul 13 11:57:37 2018 +0100

    fixed window opcode

