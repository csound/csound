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

- Check that #included file is not a directory

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
     
### Utilities

- pvlook now always prints explicit analysis window name
     
### Frontends

- ctcsound.py: All new python frontend

- icsound:

- csound~:

- csdebugger:
  
- HTML5

-- csound.node: Implemented for Linux, minor API fix.

-- pnacl: Added compileCsdText method to csound object

-- Emscripten:

-- CsoundQT has its own notes at https://github.com/CsoundQt/CsoundQt/blob/develop/release_notes/Release%20N
otes%200.9.2.1.md

### General Usage

- Checking of valid macro names improved

- #undef fixed

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
    
- pvlook reports actual window used
    
## SYSTEM LEVEL CHANGES

### System Changes

- New score lexing and preprocessor

- MAC line endings now work again

### API

- Now supports named gens

- fterror now in API

- API functions SetOutput and GetOutputFormat fixed

- Many API functions now use const where appropriate
    
- New Lisp CFFI and FFI interfaces tested with Steel Bank Common Lisp (64 bit CPU architecture), runs in separate thread
    
### Platform Specific

- iOS

-- 

- Android

-- Multichannel input and output allowed

- Windows

-- csound64.lib import library added to Windows installer
    
- OSX

--  

-- GNU/Linux

<pre>
========================================================================
commit aa5c04f343e0260e88b2ea7ef6ec6e203683337b
Date:   Fri Oct 14 09:36:53 2016 +0100
========================================================================
Date:   Tue Oct 11 19:51:01 2016 +0100

    tidy up fout

Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Sep 23 10:00:36 2016 +0100

    extending libpy hack to OSX

Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Sep 12 16:20:33 2016 +0100

    added CSOUNDMSG_STDOUT attribute

Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 28 17:16:26 2016 -0400

    Refinements for Lisp wrapper and examples, enables running in separate thread now, performances ends when CSD ends.

Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Fri Jul 22 20:47:58 2016 -0400

    Fully working Lisp integration for Steel Bank Common Lisp.

Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Jul 3 13:05:49 2016 -0400

    PNaCl: Implemented playing and rendering CSD text. Lisp: Updated example and CFFI binding.

Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Jul 1 16:51:22 2016 +0100

    ftgen reading from array

Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jun 29 12:21:39 2016 -0400

    Fixed logic bugs in atsa, added method to nacl.

Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jun 26 10:57:37 2016 +0100

    fixed issue on opcode arg check and removed old obsolete code

Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jun 21 14:55:27 2016 -0400

    Updated csound.node for Linux. Works!

Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri May 27 19:44:30 2016 +0100

    array centroid

Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri May 27 18:30:55 2016 +0100

    centroid fixes

Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu May 26 17:52:57 2016 +0100

    added mfb
    </pre>
