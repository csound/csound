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

## SYSTEM LEVEL CHANGES

### System Changes

-

### API

- CompileCsdText now always returns a value indicating success/failure.

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

==END==


========================================================================
commit 1191a12cf38048437286295297b55f9997e101e0 (HEAD -> develop, origin/develop)
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Oct 14 12:30:28 2017 +0100

    server commands

commit 6a708242f7f5061145bdb35b184f9d00a76d7f35
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Oct 14 10:14:17 2017 +0100

    faust opcodes build


commit 418fd15f591d5333958c0bddf6417d02cd5a1316
Author: Steven Yi <stevenyi@gmail.com>
Date:   Fri Oct 13 16:15:06 2017 -0400

    applied Albert Graef's change for ignoring of -ENOENT error code

commit 321f81f7a8a763bf32b16690caae921785292474
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Oct 13 16:14:03 2017 +0100

    return value from async functions

commit 135361d543605efff197afd26531b932c2e90a58
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Oct 13 13:39:58 2017 +0100

    lock free code completed, not fully tested, Async API functions also supplied

commit 7a417d79b7aa4b1a8181f7eb524c36956456e6e7
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Oct 13 09:37:29 2017 +0100

    asynchronous mechanism for compile and kill instance in place

commit cb96b4e8f179be941398856d10587ae16e381249
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Oct 12 14:12:40 2017 +0100

    async API functions

Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Oct 12 11:43:42 2017 +0100

    memory model for atomic in queue

commit 9eaeecf51c3ef9d80e340cb1576d48f50e9089e4
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Oct 3 11:44:44 2017 +0100

    chnget/set with k strings

commit e39f570503b5628bf77034de9a3590fbf87bcd5c
Author: gogins <michael.gogins@gmail.com>
Date:   Sat Sep 23 21:29:08 2017 -0400

    Trying to get VST features building on AppVeyor (#793).

commit da9dfebca45abfc80b41260725bd4ff17becfcce
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Sep 23 11:09:29 2017 -0400

    Updated with license exception in VST related code.

commit d51c9346336cc2e77b01644281c706d99cfe5818
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Aug 3 15:43:44 2017 +0100

    fixed csound.js putting the handleMessage message handling setting back into place

commit 8b21e6dcc6eea69c14bf96861d9a37d2657a0e24
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Aug 2 18:01:00 2017 +0100

    added setOption to CsoundObj.js

commit 73db533a551dec4616c6f3195467b231eaeb033c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Jul 18 11:52:38 2017 +0100

    interrupt handler in CLI csound fixed
