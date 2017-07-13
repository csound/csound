!---

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
# CSOUND VERSION 6.09.1 RELEASE NOTES

This is mostly a bugfix release of version 6.09 addressing a number of
issues, including a bug in multicore performance. The Web Assembly
platform is introduced in this release, as well as support for Swift
development on iOS. Some DLLs that were missing from the Windows
installer also have been added to it. Following Google's deprecation
of PNaCl, this platform has been dropped from the release and will not
be further developed.

These notes include the changes in 6.09 (for completeness) and 6.09.1

-- The Developers

### (For 6.09)
A mixed bag of new opcodes and many fixes and improvements.

Also as usual there are a number of internal changes, including many
memory leaks fixed and more robust code.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- select -- sample-by-sample comparison of audio selecting the output.

- midiarp opcode -- generates arpeggios based on currently held MIDI notes.

- hilbert2 --  a DFT-based implementation of a Hilbert transformer.

- Ableton Link opcodes -- for synchronizing tempo and beat across
  local area networks.

- pvstrace -- retain only the N loudest bins.

- several new unary functions/opcodes for k-rate and i-time numeric
arrays: ceil, floor, round, int, frac, powoftwo, abs, log2, log10,
log, exp, sqrt, cos, sin, tan, acos, asin, atan, sinh, cosh, tanh,
cbrt, limit1.

- several new binary functions/opcodes for k-rate and i-time numeric
  arrays: atan2, pow,  hypot, fmod, fmax, fmin.

- limit -- numeric limiting within a given range (for arrays).

- tvconv -- a time-varying convolution (FIR filter) opcode.

- liveconv -- partitioned convolution with dynamically reloadable impulse response

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

- supercollider ugens -- sc_phasor, sc_lag, sc_lagud, sc_trig

### New Gen and Macros

-

### Orchestra

- Including a directory of UDO files no longer fails if more than about 20 entries.

- It was possible for kr, sr, and ksmps to be inconsistent in one case, no more.

- Macro names better policed and bracket matching.

- Octal values as \000 can be in strings

- (from 6.09.1) In a UDO the out* opcodes now work, where before it was
working only sometimes.

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

- named instruments can be turned off with i if a - follows the ".

- (from 6.09.1) if an r-opcode section eded in e-opcode it used to sop early.

### Options

- jack midi module now can report available devices under --midi-devices.

- (from 6.09.1) defining smacros and omacros on command line only happens once.

- (from 6.09.1) defining smacros from command line now works.


### Modified Opcodes and Gens

- ftgentmp improved string arguments.

- hdf5read opcode now reads entire data sets when dataset name string
  is suffixed with an asterisk.

- use of non power-of-two lengths in gens now acceptable where before
  it was inconsistent.

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

- Emscripten: Emscripten Csound (asm.js) now requires sourcing in CsoundObj.js and
FileList.js separately from libcsound.js. This is to accommodate using the
same JS API with either asm.js or wasm backends.

- CsoundQT:  CsoundQt 0.9.4 is announced:
https://github.com/CsoundQt/CsoundQt/blob/develop/release_notes/Release%20notes%200.9.4.md.

- (from 6.09.1) Windows installer with CsoundQt does not include PythonQt.

### General Usage


## Bugs Fixed

- pwd works on OSX.

- Fencepost error in sensLine fixed.

- OSCsend corrected for caching of host name.

- Bug in push/pop opcodes fixed (this opcode is now a plugin and deprecated).

- Bug in sprintf removed.

- Bug in soundin removed.

- losci/losci3 fixed in case of long tables.

- inrg was broken for a while.

- Partikkel channelmask panning laws had an indexing error, now fixed.

- jack audio module now allows for independent numbers of in and out channels.

- Bug in string copying fixed.

- Bug in hdf5read where if two hdf5read opcodes were placed in series
  in an instrument, the argument names of the second opcode instance
  would be incorrect due to directly changing the last string
  character of the first when reading an entire dataset.

- Memory leaks fixed in some plugin opcodes.

## SYSTEM LEVEL CHANGES

### System Changes

- soundin now uses the diskin2 code.

- out family of opcodes reworked to reduce interleaving costs and to
  take proper regard if nchnls value.

- (from 6.09.1) a crash on Linux i386 removed relating to server mode.


### API

- New `csound_threaded.hpp' header-only facility, obviating need for
`csPerfThread.cpp' object in some projects.

- Added GetA4 function.

- New framework for plugin opcode development in C++ using Csound's allocator.

- Added StrDup function.

- Boost dependencies removed from Csound interfaces, CsoundAC, and CsoundVST.

- (from 6.09.1) Two new API functions, csoundSetSpinSample and csoundClearSpin.

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
