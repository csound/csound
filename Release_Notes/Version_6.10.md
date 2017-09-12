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

This is mostly a bugfix release

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
  treated as an error but a warning.

### Score

-

### Options

- the --tempo option now can be fractional; was previously limited to an integer.

### Modified Opcodes and Gens

- loscil/loscil3 accept floating point increment.

- OSCraw closes socket after use.

- fout can now generate ogg format, as well as accepting -1 to mean the
  same format as -o uses.

- bitwise and opcode (&) i a-rate corrected for sample-accrate mode.

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

## SYSTEM LEVEL CHANGES

### System Changes

- 

### API

- 

### Platform Specific

- iOS

 -

- Android

 -

- Windows

 -

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

commit 3055f0357f41ed13055136f099cf08dc6bda7318
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Aug 29 23:07:01 2017 +0100

    fix to looptseg

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

commit dc2f6b5775567f9521494b223c6a6c8864dcee43
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Jul 31 12:33:40 2017 +0100

    and createScore

commit ad366d35d391cd342b633a4c2d0aa990dc80d2ff
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Jul 30 21:57:21 2017 +0100

    Better version of createOrchestra

commit a3c647e5695563b6b7a630782157c59da48989be
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jul 29 13:25:25 2017 -0400

    A little API compatibility for WASM.

commit b8762fead9e492d52bed75c1131386c4d36816b9
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Jul 26 23:18:26 2017 +0100

    dealing with comments in CSD orchestra tag

commit 95e7894b3ce12e8bab5054a549f50c033b129a27
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Jul 21 21:43:29 2017 +0100

    experimental array assigment

commit 73db533a551dec4616c6f3195467b231eaeb033c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Jul 18 11:52:38 2017 +0100

    interrupt handler in CLI csound fixed

commit 04a785def13e94cf0d92d9401e11b02dda96fd82
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Jul 18 11:07:10 2017 +0100

    csoundCompileCsdText return value


