<!---

To maintain this document use the following markdown:

# First level heading
## Second level heading
### Third level heading

- First level bullet point
 - Second level bullet point
  - Third level bullet point
  
`inline code`

```
preformatted text
etc.
```

[hyperlink](url for the hyperlink)

Any valid HTML can also be used.

--->
# CSOUND VERSION 6.08 RELEASE NOTES

As usual there are a number of opcode fixes and improvements, but the
major changes are in the language structures.  First the score language
has all-new treatment of macros and preprocessing, bringing it in line
with those of the orchestra.  The parsing of the orchestra has had a
number of fixes as outlined below.

A major, and not totally compatible change as been made in reading
and writing array elements.  The rate of the index determines the
time of processing.  This simplifies much  code and is explicable; the
earlier ad hoc code had many anomalies.

Also as usual there are a number of new opcodes and internal fixes
to memory leaks and more robust code.
  
-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- dct -- Discrete Cosine Transform of a sample array (type-II DCT)

- getftargs -- copy arguments of a gen to an array

- mfb -- implements a mel-frequency filterbank for an array of input magnitudes

### New Gen and Macros

- quadbezier generating Bezier curves in a table

### Orchestra

- The character ¬ is now correctly treated as a variant of ~ for bitwise not

- Lexing bug which could corrupt strings fixed

- Ensure no newlines in string-lexing

- Small improvement in reported line numbers

- Better checking of macro syntax

- Improved parsing of setting of labels

- Added error handling for unmatched brackets for UDO arg specification

- Check that `#included` file is not a directory

- Deeply nested macro calls better policed

- For years Csound has fixed the pitch of A4 at 440Hz.  Now this can be set in the header using the new r-variable A4, and also read with that variable

- Floating point values can use e or E for exponent

- For array reading of indexed values the rate of the index controls the rate of the action.  ***NOTE THIS IS AN INCOMPATIBLE CHANGE*** but fixes many anomalies

### Score

- New code to handle macros and other preprocessor commands. Brings it into line with orchestra code

### Options

- The tempo setting can now be a floating point value (previously fixed to integer)

### Modified Opcodes and Gens

- Problems in centroid fixed.

- Better treatment of rounding in printks

- OSC extended to include multicast

- Faust opcodes brought up to date with faust

- oscil1 and oscili can take a negative duration

- fout opcode documentation clarified

- Release time in mxadsr fixed

- centroid opcode  extended to take array inputs in addition

- ptable opcodes are now identical to table family

- ftgen now as array input option

### Utilities

- pvlook now always prints explicit analysis window name
     
### Frontends

- ctcsound.py: All new python frontend

- icsound:

- csound~:

- csdebugger:
  
- HTML5

 - csound.node: Implemented for Linux, minor API fix.

 - pnacl: Added compileCsdText method to csound object

 - Emscripten:

- CsoundQT:

### General Usage

- Checking of valid macro names improved

- ```#undef``` fixed

## Bugs Fixed

- Fix to prints in format use

- jitter2 reworked to make it more like the manual.

- oscbank has had multiple fixes and now works as advertised

- bformdec1 with arrays and type 4 fixed

- Bug in pvsceps fixed

- In various formatted print opcodes extra trash characters might appear -- fixed

- Assigning variables with --sample-accurate could give unexpected results; this is believed fixed now

- padsynth square profile fix, and opcode prints less depending on warn level

- gen31 fixed

- gen41 fixed

- Bug in sensekey fixed
    
- A number of issues in centroid fixed
    
## SYSTEM LEVEL CHANGES

### System Changes

- New score lexing and preprocessor

- MAC line endings now work again

- System information messages (system sampling rate, etc) are now directed to stdout

### API

- Now supports named gens

- fterror now in API

- API functions SetOutput and GetOutputFormat fixed

- Many API functions now use const where appropriate

- Messages can now be directed to stdout from the API by using CSOUNDMSG_STDOUT attribute
    
- New Lisp CFFI and FFI interfaces tested with Steel Bank Common Lisp (64 bit CPU architecture), runs in separate thread

- Messages can now be directed to stdout from the API by using CSOUNDMSG_STDOUT attribute

### Platform Specific

- iOS

 - 

- Android

 - Multichannel input and output allowed

- Windows

 - csound64.lib import library added to Windows installer
    
- OSX

 - 

- GNU/Linux

<pre>
========================================================================
commit 766f2c51cc3a62ea9a8381bb57d1c3fefebab92f
Date:   Mon Oct 17 14:55:29 2016 +0100
=======
</pre>
