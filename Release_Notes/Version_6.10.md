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
# CSOUND VERSION 6.10 RELEASE NOTES

This is mostly a bugfix release, including a major bug recently introduced in
loscil.  New and improved opcodes and a long orphaned gen
(53) are here, as well as many small internal improvements.  Internal
changes have removed a number of memory leaks.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- midiout_i which is like midiout, but works at i-rate.

- chngetks and chnsetks -- versions of chnget and chnset for string
channels that **only** run at perf-time.

- squinewave is a mostly bandlimited shape-shifting
  square-pulse-saw-sinewave oscillator with hardsync.

- The experimental opcode OSCsendA has been removed; use OSCsend instead.

### New Gen and Macros

- gen53 (which has been in the code but not documented for years) is
  now acknowledged.  It creates a linear-phase or minimum-phase
  impulse response table from a source table containing a frequency
  response or an impulse response.

### Orchestra

- Incorrect use of k-rate if..then.. in instrument 0 is now treated as i-rate.

- Incorrect use of k-rate operations in instrument 0 are no longer
  treated as an error but a warning.

- In a csd file commented-out tags were acted upon in some cases,
  leading to truncated orchestras.  This is now fixed.

- Arrays can be assigned from i-rate and k-rate to krate and i-rate;
  previously rates had to match.

- The use of ! as a Boolean operation (meaning negation) is now supported,
  where previously the parser accepted it but did not use it.

- Constant folding now implemented on a wide range of arithmetic.

- Attempts to use an undefined macro produce a syntax error now.

- Missing " (or other terminator) in #include is noticed and the #include is 
  ignored.

### Score

- In a csd file commented-out tags were acted upon in some cases,
  leading to truncated scores. This is now fixed.

- The evaluation form [..] can now be nested.

- The extract feature (-x from command line) now works.

- Use of the score opcode x could case spurious error messages which are now 
  suppressed.

- After calling a undefined macro the rest of the line is ignored.

- A couple of bugs in repeated sections (r opcode) have been removed.

- Missing " (or other terminator) in #include is noticed and the #include is 
  ignored.

### Options

- the --tempo (and -t) option now can be fractional; was previously
  limited to an integer.

- new option: --udp-console=address:port redirects console to a remote
  address:port.

- new option: --udp-mirror-console=address:port mirrors the console to
  a remote address:port.

- new option: --udp-echo echoes messages sent to the UDP server

- new option: --aft-zero sets initial after-touch value to zero rather than 127.

### Modified Opcodes and Gens

- loscil/loscil3 accept floating point increment.

- OSCraw closes socket after use.

- fout can now generate ogg format, as well as accepting -1 to mean the
  same format as -o uses.

- bitwise and opcode (&) at a-rate corrected for sample-accurate mode.

- slicearray has an optional additional argument to give a stride to the slice.

- chnset now can have variable channel names.

- a-rate arrays may be added, subtracted, multiplied and scaled.  This
  is a start on a-rate array arithmetic.

- dssiinit improved removing some crashes.

- partials improved to remove a fencepost issue.

- vco2ift fixed when an existing table is used.

- The formatted printing opcodes now check for insufficient provided arguments.

- FLbox and FLsetText again accept an ivar as first argument to give a
  string via strset (as a alternative to a string).

- Better checking in prints should stop some crashes.

### Utilities

-

### Frontends

- icsound:

- csound~:

- csdebugger:

- Emscripten: Now compiled as WebAssembly (runs in all major browsers). API now
  somewhat more conformed to other HTML5 APIs.

- CsoundQt: Now built from master branch for improved stability.

### General Usage

## Bugs Fixed

- The optionality of the last argument in sc_phasor now works.

- Freezing in dconv fixed.

- looptseg no longer crashes if presented with too few arguments.

- schedule etc now work correctly with double-quoted strings within {{
  }} strings.

- problem with CLI frontend interrupt handler fixed.

- outs2 was broken (always wrote to channel 1 like outs1).

- Various errors in the DSSI/ladspa system fixed.

- vbap was broken in all cases except 4-speakers, now corrected.

- Live evaluation of Csound Orchestra code code could result in hard to
  diagnose, odd errors (e.g., crashes, division by zeros, NaNs). This was due
  to a bug in merging of newly found constants into the global constant pool. 

## SYSTEM LEVEL CHANGES

### System Changes

- The GNU Lesser General Public License, version 2.1, for CsoundVST
and the vst4cs opcodes has been modified to grant an exception for
compiling and linking with the VST2 SDK, which is available from
[https://github.com/steinbergmedia/vst3sdk]. For more information, see
[https://github.com/csound/csound/blob/develop/Opcodes/vst4cs/licensing_considerations_for_csoundvst_and_vst4cs.md].

- UDP Server now accepts some new commands, which are
prefixed by an opcode. These include support for
events (&<event>) and scores ($<score>); setting control channels
(@<channel> <value>); setting string channels (%<channel> <string>));
getting control channel values via UDP (:@<channel> <dest-address>
<dest-port>) and string channel contents (:%<channel> <dest-address>
<dest-port>).

### Translations

- As ever the French translations are complete.

- The Italian translations of messages are greatly improved in scope;
  about a half of error and warning messages are now done.

- Some progress as been made in German translations.

### API

- CompileCsdText now always returns a value indicating success/failure.

- Eight new asynchronous versions of API functions now available:
  csoundCompileTreeAsync(), csoundCompileOrcAsync(),
  csoundReadScoreAsync(), csoundInputMessageAsync(),
  csoundScoreEventAsync(), csoundScoreEventAbsoluteAsync(),
  csoundTableCopyOutAsync(), and csoundTableCopyInAsync().

- For server use, three new API functions: csoundUDPServerStart,
  csoundUDPServerStatus and csoundUDPServerClose.

### Platform Specific

- iOS

 -

- Android

 -

- Windows

 - Now compiles with Microsoft Visual Studio 2015 or later.

 - Continuous integration for Windows with AppVeyor (Visual Studio 2017).

<!---
- The AppVeyor build and installer now includes CsoundVST and the vst4cs
   opcodes that enable hosting VST plugins in Csound. The LGPL v2.1
   license for that code has been modified, with permission of Hermann
   Seib the original author of the VSTHost code, to permit use with
   the separately downloaded VST2 SDK from Steinberg.
--->
- OSX

- GNU/Linux

  ALSA MIDI backend now ignores some spurious -ENOENT error codes.

==END==


========================================================================
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Nov 9 12:27:38 2017 +0000

Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon Nov 6 16:53:08 2017 -0500

    added NULL check for prints opcode to prevent crash when number given as initial arg

Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon Nov 6 15:55:40 2017 -0500

    added test for prints to not crash if number given (should just report error)

