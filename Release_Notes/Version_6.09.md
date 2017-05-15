d !---

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
# CSOUND VERSION 6.09 RELEASE NOTES

A mixed bag of new opcodes and many fixes and improvements.

Also as usual there are a number of internal changes, including many
memory leaks fixed and more robust code.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- select -- sample-by-sample comparison of audio selecting the output.

- midiarp opcode -- generates arpeggios based on currently held MIDI notes.

- hilbert2 --  a DFT-based implementation of a Hilbert transformer.

- Ableton Link opcodes -- for synchronizing tempo and beat across local area networks.

- pvstrace -- retain only the N loudest bins.

- several new unary functions/opcodes for k-rate and i-time numeric
arrays: ceil, floor, round, int, frac, powoftwo, abs, log2, log10,
log, exp, sqrt, cos, sin, tan, acos, asin, atan, sinh, cosh, tanh,
cbrt, limit1.

- several new binary functions/opcodes for k-rate and i-time numeric
arrays: atan2, pow,hypot, fmod, fmax, fmin.

- limit -- numeric limiting within a given range (for arrays).

- tvconv -- a time-varying convolution (FIR filter) opcode.

- bpf, xyscale, ntom, mton -- (from SuperCollider?).

- OSCsendA -- asynchronous version of OSCsend,

- OSCsend -- now implemented directly using system sockets. Old version
using liblo has been kept as OSCsend_lo.

- OSCraw -- to listen for all OSC messages at a given port.

- New implementation of OSCsend not using liblo, with previous version
  now called OSCsend_lo,

- sorta and sortd -- sort elements of an array.

- dot -- calculates the dot product of two arrays.

- zero delay filters -- zdf_1pole_mode, zdf_2pole_mode, zdf_ladder,
  zdf_1pole and zdf_2pole.xml, diode_ladder, z35_hpf and K35_lpf.

- product -- takes a numeric array (k or i-rate) and calculates its product.

- supercollider ugens -- sc_phasor, sc_lag, sc_lagud, sc_trig added.

### New Gen and Macros

-

### Orchestra

- Including a directory of UDO files no longer fails if more than about 20 entries.

- It was possible for kr, sr, and ksmps to be inconsistent in one case, no more.

- Macro names better policed and bracket matching.

- Octal values as \000 can be in strings

### Score

- Improved line number reporting in r opcode and case with no macro implemented.

- m and n opcodes fixed.

- Expansion of [...] corrected and improved.

- Strings in scores improved

- The ) character can be in a macro argument if it is escaped with \.

- Use of the characters  e or s could lead to errors; now fixed.

- Macro names better policed, and bracket matching.

- p2 and p3 are now at higher precision, no longer truncated to 6 decimal places.

- new opcode d to switch off infinite notes (denote); same as i with negative p1.

- named instruments can be turned off with i if a - follows the "

### Options

- jack midi module now can report available devices under --midi-devices.

### Modified Opcodes and Gens

- ftgentmp improved string arguments.

- hdf5read opcode now reads entire data sets when dataset name string
  is suffixed with an asterisk.

- use of non power-of-two lengths now acceptable where before it was inconsistent.

- ampmidid optionally can be aware of 0dbfs.

- dust and dust2 at k-rate now conform to the manual (NOTE: this is an
incompatible change).

- In prints the format %% now prints one %.

- OSClisten can be used with no data outputs.

- GEN18 corrected to write to requested range.

- sockrev now can read strings.

- vbap system can in some cases allow arbitrary number of speakers via
  arrays (work in progress).

- Websocket server can only accept one protocol output, so limiting
  intype to just a single argument.

- sum opcode will also sum elements of an array.

- Overloaded pvs2tab and tab2pvs now can create and use split
magnitude and phase arrays.

### Utilities

- dnoise fixed.

### Frontends

- icsound:

- csound~:

- csdebugger:

- HTML5

 - Removed HTML5 Csound editor which has quit working.

- Emscripten:

- CsoundQT:  CsoundQt 0.9.4 is announced:
https://github.com/CsoundQt/CsoundQt/blob/develop/release_notes/Release%20notes%200.9.4.md.

- Windows installer with CsoundQt includes PythonQt.

### General Usage


## Bugs Fixed

- pwd works on OSX.

- Fencepost error in sensLine fixed.

- OSCsend corrected for caching of host name.

- Bug in push/pop opcodes fixed (this opcode is now a plugin and deprecated).

- Bug in sprintf removed.

- Bug in soundin removed.

- losci/losci3 fixed in case of long tables.

- inrg was broke for a while.

- Partikkel channelmask panning laws had an indexing error, now fixed.

- jack audio module now allows for independent numbers of in and out channels.

- Bug in string copying fixed.

- Bug in hdf5read where if two hdf5read opcodes were placed in series
  in an instrument, the argument names of the second opcode instance
  would be incorrect due to directly changing the last string
  character of the first when reading an entire dataset. 

## SYSTEM LEVEL CHANGES

### System Changes

- soundin now uses the diskin2 code.

- out family of opcodes reworked to reduce interleaving costs and to
  take proper regard if nchnls value.

### API

- New `csound_threaded.hpp` header-only facility, obviating need for 
`csPerfThread.cpp` object in some projects.

- Added GetA4 function.

- New framework for plugin opcode development in C++ using Csound's allocator.

- Added StrDup function.

- Boost dependencies removed from Csound interfaces, CsoundAC, and CsoundVST.

### Platform Specific

- iOS

 - iPad portrait SplitView fix+animation, info popover resizing, stop
   button fix in Soundfile Pitch Shifter. 

 - Csound-iOS API updates; Examples cleaned up, enhanced/expanded, and reordered. 
   Manual revised, expanded, updated. Updates to API and examples
   support iOS 10 and Xcode 8. 

- Android

 - Multichannel input and output allowed.

- Windows

 - csound64.lib import library added to Windows installer.

- OSX

- GNU/Linux

==END==
========================================================================
UNDOCUMENTED/UNDELETED

Author: jpff <jpff@codemist.co.uk>
Date:   Sun May 7 15:54:43 2017 +0100


commit 3814b45a7c804b09ac1944f76eeac52615c7a88c
Author: Steven Yi <stevenyi@gmail.com>

Date:   Sat Apr 1 18:05:13 2017 -0400

    fix for pmidi.c and csoundLock/UnLock: add include of csGblMtx.h, fix setting of HAVE_PTHREAD for all targets instead of just for libcsound64
