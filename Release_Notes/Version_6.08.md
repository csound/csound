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
# CSOUND VERSION 6.08 RELEASE NOTES

As usual there are a number of opcode fixes and improvements, but the
major changes are in the language structures.  First the score language
has all-new treatment of macros and preprocessing, bringing it in line
with those of the orchestra.  The parsing of the orchestra has had a
number of fixes as outlined below.

A major, and not totally compatible change as been made in reading and
writing array elements.  The rate of the index now often determines
the time of processing; check the entry below under *Orchestra*.  This
simplifies much code and seems to capture expectations; the earlier ad
hoc code had many anomalies.

Also as usual there are a number of new opcodes and internal fixes
to memory leaks and more robust code.
  
-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- dct -- Discrete Cosine Transform of a sample array (type-II DCT)

- getftargs -- copy arguments of a gen to an S-variable

- mfb -- implements a mel-frequency filter-bank for an array of input
  magnitudes 

### New Gen and Macros

- quadbezier -- generating Bezier curves in a table

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

- Array access semantics have been clarified:

	- i[i]  => reading at i-time and perf-time, writing at i-time
    only.
	- i[k] =>  reading at perf-time, writing yields a runtime error
	- k[i], k[k] =>  reading at perf-time, writing at perf-time
	- a[i], a[k] => reading at perf-time, writing at perf-time
    - other (S[], f[]) => reading and writing according to index type (i,k).
   
In particular, i(k[i]) will continue not to work, as before, but the new operator
i(k[],i) is provided to cover this case.

- xout validation no longer fails when constants are given

### Score

- New code to handle macros and other preprocessor commands. Brings it
  into line with orchestra code

- New score opcode C introduced as a way of switching automatic carry off (`C 0`)
  or on (default) (`C 1`)

### Options

- The tempo setting can now be a floating point value (previously fixed to integer)

- New option --version prints version information and exits

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

- subinstr can now have string arguments

- the i() format is extended to work on k-rate arrays with the first
  argument being an array, followed by the indices

- monitor opcode can now write to an array

### Utilities

- pvlook now always prints explicit analysis window name
     
### Frontends

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

- Fixes to prints in format use

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

- An internal miscalculation of variable sizes that only affected 32bit
  architectures fixed
    
## SYSTEM LEVEL CHANGES

### System Changes

- New score lexing and preprocessor

- MAC line endings now work again

- System information messages (system sampling rate, etc) are now directed to stdout

- rtjack reworked to deal with names and numbers

- The version printing now includes the commit as so the developers
  know which patches have been applied 

### API

- API version now 4.0

- Now supports named gens

- fterror now in API

- API functions SetOutput and GetOutputFormat fixed

- Many API functions now use const where appropriate

- Messages can now be directed to stdout from the API by using CSOUNDMSG_STDOUT attribute
    
- New Lisp CFFI and FFI interfaces tested with Steel Bank Common Lisp (64 bit CPU architecture), runs in separate thread

- ctcsound.py, a new FFI interface for Python was introduced in version 6.07. It is now the recommended interface for Python,
  csnd6.py being deprecated.

### Platform Specific

- iOS

- Android

 - Multichannel input and output allowed

- Windows

 - csound64.lib import library added to Windows installer
    
- OSX

 - Minor issues with installer fixed 

- GNU/Linux

==END==
