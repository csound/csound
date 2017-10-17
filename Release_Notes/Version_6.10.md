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
# CSOUND VERSION 6.10 RELEASE NOTES

This is mostly a bugfix release, including a major bug introduced in
loscil recently.  New and improved opcodes and a long orphaned gen (53)
are here, as well as many small internal improvements.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- midiout_i which is like midiout, but works at i-rate.

- chngetks and chnsetks -- versions of chnget and chnset for string
channels that **only** run at perf-time.

### New Gen and Macros

- gen53 (which has been in the code but not documented for years) is
  now acknowledged.  It creates a linear-phase or minimum-phase
  impulse response table from a source table containing a frequency
  response or an impulse response.


### Orchestra

- Incorrect use of k-rate if..the.. in instrument 0 is now treated as i-rate.

- Incorrect use of k-rate operations in instrument 0 are no longer
  treated as an error but a warning.

- In a csd file commented-out tags were acted upon in some cases,
  leading to truncated orchestras.  This is now fixed.

- Arrays can be assigned from i-rate and k-rate to krate and i-rate;
  previously rates had to match.

- The use of ! as a Boolean operation (meaning negation) is now supported,
were previously the parser accepted it but did not use it.

### Score

- In a csd file commented-out tags were acted upon in some cases,
  leading to truncated scores. This is now fixed.

- The evauation form [..] can now be nested.

- The extraxt feature (-x from command line) now works

- Use of the score opcode x could case spurious error messages which are
  now suppressed.

### Options

- the --tempo (and -t) option now can be fractional; was previously
  limited to an integer.

### Modified Opcodes and Gens

- loscil/loscil3 accept floating point increment.

- OSCraw closes socket after use.

- fout can now generate ogg format, as well as accepting -1 to mean the
  same format as -o uses.

- bitwise and opcode (&) at a-rate corrected for sample-accurate mode.

- slicearray as an optional additional argument to give a stride to the slice.

- chnset now can have variable channel names.

- a-rate arrays may be added, subtracted, multiplied and scaled.  This
  is a start on a-rate array arithmetic.

### Utilities

-

### Frontends

- icsound:

- csound~:

- csdebugger:

- Emscripten: Now compiled as WebAssembly (runs in all major browsers). API now
  somewhat more conformed to other HTML5 APIs.

- CsoundQT: Now built with HTML5 support.

### General Usage


## Bugs Fixed

- The optionality of the last argument in sc_phasor now works.

- Freezing in dconv fixed.

- looptseg no longer crashes if presented with too few arguments.

- schedule etc now work correctly with double-quoted strings within {{
}} strings.

- problem with CLI frontend interrupt handler fixed.

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

### API

- CompileCsdText now always returns a value indicating
success/failure.

- Eight new asynchronous versions of API functions now available:
csoundCompileTreeAsync(), csoundCompileOrcAsync(),
csoundReadScoreAsync(), csoundInputMessageAsync(),
csoundScoreEventAsync(), csoundScoreEventAbsoluteAsync(),
csoundTableCopyOutAsync(), and csoundTableCopyInAsync()

### Platform Specific

- iOS

 -

- Android

 -

- Windows

 - Now compiled with MSVC.

 - Continuous integration for Windows with AppVeyor.

 - The AppVeyor build and installer now includes CsoundVST and the vst4cs
   opcodes that enable hosting VST plugins in Csound. The LGPL v2.1
   license for that code has been modified, with permission of Hermann
   Seib the original author of the VSTHost code, to permit use with
   the separately downloaded VST2 SDK from Steinberg.

- OSX

- GNU/Linux
  ALSA MIDI backend now ignores some spurious -ENOENT error codes.

==END==


========================================================================
commit d51c9346336cc2e77b01644281c706d99cfe5818
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Aug 3 15:43:44 2017 +0100

    fixed csound.js putting the handleMessage message handling setting back into place

commit 8b21e6dcc6eea69c14bf96861d9a37d2657a0e24
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Aug 2 18:01:00 2017 +0100

    added setOption to CsoundObj.js

