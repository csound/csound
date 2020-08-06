!---

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

# CSOUND VERSION 6.15 RELEASE NOTES

Quite a few new opcodes are in this release as well as extensions of
existing opcodes.  In particular there is the introduction of streamed
LPC which has long been requested.

Another feature if this release is a large number of internal fixes to
incorrect data access, as well as the usual tweaks and changes.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- ftset sets multiple elements of a table to a given value.

- lufs opcode calculates a momentary, integrated and short-term loudness meter.

- bob filter is a numerical simulation of the Moog analog resonant filter.

- sterrain is an enhanced version of wterrain with more possible orbits.

- wterrain2 is a alternative enhancemet of wterrain with ess  variation that sterrain.

- count, count_i, cntCreate, cntRead, cntReset, cntCycles and cntState together implement a new counter object that cycles trough a constant range, similar to in PD.

- new alias for sc_ opcodes: sc_lag -> lag, sc_lagud -> lagud, sc_trig -> trigholf, sc_phasor -> phasortrigo.

- println is similar to printf but without the trigger.

- rndseed provides a seed for rnd and birnd functions.

- arduinoStart, arduinoRead and arduinoStop provide a protocol for
transferring sensor data from an Arduino to Csound.

- lpcfilter, lpcanal, allpole, pvslpc, pvscfs, apoleparams, resonbnk:
new streaming linear prediction opcodes.

- gauss - new version accepting mean and standard deviation as
parameters, implementing the Box-Muller algorithm.

- pvsbandwidth - returns spectral bandwidth.

- vps - vector phase shaping

### New Gen and Macros

### Orchestra

- #include of a url now works again.

- the end of file case is better handled in the pre-lexer.

- corrections to reported line number in a few error cases.

- conditional expressions yielding strings fixed, and other cases.

- the sequence //* no longer is misinterpreted as starting a comment block.

- when using sample-accurate mode a new score event that was aligned to the ksmps could stop one cycle early.  Now correct.

- the maximum line length for various inputs has been increased to
  8192.

- now legal to set the number of input channels to zero.

### Score

- New score opcode B is like b but is accumulative.

- the end of file case is better handled in the pre-lexer.

### Options

- keep-sorted-score and simple-sorted-score both can take a filename in which to write the score after a =.

- print_version option prints the version of Csound used at the end of a rendering.

- syntax-check-only return an error if syntax failed.

- opcode-dir: loads all plugin opcodes from a given directory (in
  addition to the plugins loaded from the opcode plugin path).

### Modified Opcodes and Gens

- cent, semitone, dB accuracy improved.

- taninv2 now has an array version.

- ftslice has more variations.

- ptable opcodes are now deprecated as they are identical to table opcodes.

- GEN20 case 9 (sinc function) now has an optional parameter tao the x range.

- fprint(k)s now has a %s format specifier.

- lastcycle corrected and clarified.

- chn_k can now accept the mode as a string. r=1 (input), w=2 (output), rw=3 (input+output).

- trim improved.

- the HDF5 opcodes upgraded to v1.12.0.

- GEN16 is more careful about lengths of data.

- scale has additional optional arguments to specify the input range.

- schedule/schedulek can take arguments from an array.

- GEN11 improved with respect to rounding errors.

- partials has an improved method of phase estimation.
.
- ctrlinit checks that the values are in the range [0,127].

- fin as format argument changed, and is now deprecated.

- fink has the same argument change

### Utilities

- lpanal now contains a new alternative algorithm based on the Durbin
  method, in addition to the original Gauss method.

### Frontends

- Belacsound:

- CsoundQt:


### General Usage

- if using FLTK the widgets are reset on ending a run, which was not always the case earlier.


## Bugs Fixed

- setcols was very broken; fixed.

- cps2pch and cpsxpc fixed in the case of a table of frequencies.

- the 31 bit pseudo random number generator was seeded with zero then it stayed on zero.  Than it now fixed.

- gen 20 was wrong in the case of 8.

- turning off an instrument from inside a UDO now works.

- macro expansion in both orchestra and score had a bug related to uninitialised variable.

- if a UDO set a different value for ksmps any output to a multichannel device was incorrectly calculated.

- reshape array had a number of problems, now all fixed.

- ftprint had problems not following the manual regarding trig == -1 and could show the wrong index.

- part2txt/partials occasionally emitted the same track (including same track ID) multiple times for a given time point. Fixed.

- expsegr was incorrectly dependent on ksmps when sample-accurate is in force.

- table opcodes had an error when used with non power-of-two lengths.

# SYSTEM LEVEL CHANGES

- A crash when csound.evalcode was called without csound.start fixed.

### System Changes

- Many fixes to memory problems, mainly invalid reads/writes.

### Translations

### API

- new API to hard override default plugin dir.

- new API function to load plugins.

### Platform Specific

- WebAudio: 
 
  * Built using Emscripten 1.39.13 (LLVM backend)
  * New single-file release of CsoundObj.js with all classes and webassembly files combined using npm and rollup
  * Breaking: CsoundObj.importScripts() was removed and replaced with CsoundObj.initialize()
    * paths for loading other files no longer relevant with single-file
    * initialize takes in optional AudioContext, otherwise CsoundObj will create one for use
  * Breaking: CSOUND\_AUDIO\_CONTEXT moved from global namespace; instead reference CsoundObj.CSOUND\_AUDIO\_CONTEXT
  * Compile flags changed to -O3 and without debug information, per recommendations for release builds by Emscripten
  * Link-Time Optimization (LTO) enabled for reduced size

- iOS

- Android

- Windows

- MacOS

 - coreaudio now checks the number of channels and fails if there are insufficient.

- GNU/Linux

- Haiku port

- Bela
 - updated digiBelaOut and digiIOBela.


==END==

commit 2af6df492f78471769c53e42ca5e65bf1761e52e
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Jul 14 20:11:49 2020 +0100

setSpinSample

Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Jul 11 09:50:39 2020 -0400

    attempting fix for default opcode dir after LIBRARY_INSTALL_DIR change


