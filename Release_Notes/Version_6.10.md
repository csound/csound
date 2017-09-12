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
loscil recently.  New ad improved opcodes and a long orphaned gen (53)
are here, as well as many small internal improvements.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- midiout_i which is like midiout but works at i-rate

### New Gen and Macros

- gen53 (which has been in the code but not documented for years) is
  now acknowledged.  It creates a linear-phase or minimum-phase
  impulse response table from a source table containing a frequency
  response or an impulse response.


### Orchestra

- Incorrect use of  k-rate if..the.. in instrument 0 is now treated as i-rate.

- incorrect use of k-rate operations in instrument 0 are no longer
  treated as an error but a warning

- In a csd file commented-out tags were acted upon in some cases,
  leading to truncated orchestras.  This is now fixed.

- Arrays can be assigned from i-rate and k-rate to krate and i-rate;
  previously rates had to match.

### Score

- In a csd file commented-out tags were acted upon in some cases,
  leading to truncated scores.  This is now fixed.

-

### Options

- the --tempo (and -t) option now can be fractional; was previously
  limited to an integer.

### Modified Opcodes and Gens

- loscil/loscil3 accept floating point increment.

- OSCraw closes socket after use.

- fout can now generate ogg format, as well as accepting -1 to mean the
  same format as -o uses.

- bitwise and opcode (&) at a-rate corrected for sample-accurate mode.

### Utilities

- 

### Frontends

- icsound:

- csound~:

- csdebugger:

- HTML5


- Emscripten:

- CsoundQT:

### General Usage


## Bugs Fixed

- The optionality of the last argument in sc_phasor now works.

- Freezing in dconv fixed

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

 - Now compiled with MSVC

- OSX

- GNU/Linux

==END==


========================================================================
commit b8a71c9be032c9db150e2aed63614ca2469ecc20
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Sep 11 15:14:49 2017 +0100

commit d952e764cc2513ca49b5c934dacef69608b3cd1c
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Sep 2 11:06:26 2017 -0400

    added ALLOW_MEMORY_GROWTH=1 to address issue with sample memory limits

commit 78a91450fc97681864dcc39e39354bd7501b68c3
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Sep 2 11:00:15 2017 +0100

    chnset now can have variable channel names

commit a5b29e123ba910934989a2491b83ad70bdcf97dc
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Aug 26 07:43:09 2017 -0400

    Configure for Qt SDK version 5.9.

commit d40519bd2fe50a159b0e869ec82f1cb8eb2ea1db
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Aug 25 12:08:11 2017 +0100

    updated tvconv code

commit 6dde38f56df76ac06ad92b1a0988fdc57515a84e
Author: Felipe Sateler <fsateler@gmail.com>
Date:   Tue Aug 22 18:52:35 2017 -0300

    Simplify gmm by skipping check and instead just define our local version that doesn't conflict

commit d51c9346336cc2e77b01644281c706d99cfe5818
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Aug 3 15:43:44 2017 +0100

    fixed csound.js putting the handleMessage message handling setting back into place

commit 8b21e6dcc6eea69c14bf96861d9a37d2657a0e24
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Aug 2 18:01:00 2017 +0100

    added setOption to CsoundObj.js

commit a3c647e5695563b6b7a630782157c59da48989be
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jul 29 13:25:25 2017 -0400

    A little API compatibility for WASM.

commit 73db533a551dec4616c6f3195467b231eaeb033c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Jul 18 11:52:38 2017 +0100

    interrupt handler in CLI csound fixed


