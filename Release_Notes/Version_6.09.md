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
# CSOUND VERSION 6.09 RELEASE NOTES

Also as usual there are a number of new opcodes and internal fixes
to memory leaks and more robust code.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- select -- sample-by-sample comparison of audio selecting the output

- midiarp opcode generates arpeggios based on currently held MIDI notes.

- hilbert2 --  a DFT-based implementation of a Hilbert transformer.

- Ableton Link opcodes for synchronizing tempo and beat across local area networks.

### New Gen and Macros

-

### Orchestra

-


### Score

-
### Options

-

### Modified Opcodes and Gens

- ftgentmp impoved string arguments

- hdf5read opcode now reads entire data sets when dataset name string
  is suffixed with an asterisk

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

-

## Bugs Fixed

- pwd works on OSX

- fencepost error in sensLine fixed

- OSCsend corrected for caching of host name

## SYSTEM LEVEL CHANGES

### System Changes

- soundin now uses te diskin2 code

- out family of opcodes reworked to reduce interleaving costs and to
  take proper regard if nchnls value.

### API

- New `csound_threaded.hpp` header obviating need for `csPerfThread.cpp` object in some projects.

### Platform Specific

- iOS

- Android

 - Multichannel input and output allowed

- Windows

 - csound64.lib import library added to Windows installer

- OSX

- GNU/Linux

==END==
========================================================================
commit 53c92166642f209911d8d2e077fdc78cf0c30ae7
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Mar 20 19:59:05 2017 +0000

    uint->uint32_t

commit 782ae1a06611d34a35cc7ecb20e92623cbbaa11e
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Mar 19 17:43:11 2017 +0000

    tvconv with freeze params

commit e21c43911cac6de8733f23e283926eec4335d31f
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Mar 19 17:03:47 2017 +0000

    dust and dust2 at krate

commit 2cb6c3b70a16d8ccaf9a3c5cf2493f1d9774fd1a
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Mar 19 11:34:36 2017 +0000

    tvconv dconv option

commit a9a298fa377e02707721a1dc9334b34f67e94f9b
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Mar 19 00:19:43 2017 +0000

    tvconv partitioned

commit 865c35cad745027f65292b6c56854a7e691f35f5
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Mar 18 16:41:40 2017 +0000

    tvconv


Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Mar 16 16:51:29 2017 +0000

    dssi issues

commit eaca4bbc4616ac0294d6c9f550e11b84dcb44bd0
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Mar 13 15:21:13 2017 +1100

    Removing HTML5 Csound editor which has quit working.

commit 53ca79c27e31922b85476e970836fca185ccd920
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Mar 7 20:30:53 2017 +0000

    kill extra % in prints I hope

commit e048a670e9cd1f14c3cede68e5a86d1bfc5312db
Author: Edward Costello <phasereset@gmail.com>
Date:   Mon Mar 6 22:54:31 2017 +0000

    Fixed HDF5 opcodes

commit 424d9de6a90d742c99771ed50601e0f69b1d6377
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Mar 6 15:00:15 2017 +0000

    reversed counts

commit 1348e0c47a7ca69e400d9db93a357df2f45f5f94
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Mar 6 13:58:59 2017 +0000

    argument counting for plugin class

commit 74c5dd7445ae3e654960eba89bad43f728e1e99a
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Feb 28 17:06:41 2017 +0000

    attempt to stop memory leak

commit 5f5fa37d9c71788cd5897147f7dc482a41cbfed9
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Fri Feb 24 10:14:37 2017 +1100

    Option to use 0dBFS in ampmidid.

Author: jpff <jpff@codemist.co.uk>
Date:   Thu Feb 23 17:29:02 2017 +0000

    fix include of udos

commit d9877c9b94c197969bfe1c4eabdbc4ba48ff897e
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Feb 23 16:04:59 2017 +0000

    Check power of 2 table legth and adjust

commit 28d2c608e28849670155d432d9b8a07046569403
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Feb 22 23:25:56 2017 +0000

    push/pop fixed

commit 6aa08cc950f99d434e11486d73a7f202ca985b46
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Feb 21 12:40:00 2017 +0000

    sa code fixed

commit 40892fc005d6faa21d90dfcfaf4ffc67e1d336e6
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Feb 20 20:37:42 2017 +0000

    const iterators

commit 400c72d543a888795b0db175c7b6088d7efefd1c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Feb 19 21:26:44 2017 +0000

    support for audio sig vars

Author: gogins <michael.gogins@gmail.com>
Date:   Sun Feb 12 16:47:18 2017 -0500

    Added Ableton Link opcodes to the Csound for Android build.

commit 4c4536752dae1363ee11d74f22c01341e40413dc
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Feb 12 16:38:56 2017 +0000

    fix to e and s in scores

commit fb20ea20135abe5f9a52c95e60618e49ed4539e5
Author: gogins <michael.gogins@gmail.com>
Date:   Sat Feb 11 20:05:52 2017 -0500

    Fixed Lua score generator example.

commit 8c33099b918ca145587bb5865ccee312395680d7
Author: gogins <michael.gogins@gmail.com>
Date:   Sat Feb 11 13:58:06 2017 -0500

    Updates for hybrid build system for Lua opcodes.

commit 6c0b2f5baf6397cdb4210e0af11888556e47fb81
Author: gogins <michael.gogins@gmail.com>
Date:   Sat Feb 11 13:55:04 2017 -0500

    Updates for hybrid build system for Lua opcodes.

commit a6ecadda14cf98b02d3faa4d0a95ad912765ff8b
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Feb 9 18:37:02 2017 +0000

    pvs loops straightened

commit 937745c70d881341d2ea0b9572cb9435beb1e63e
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Feb 7 21:42:32 2017 +0000

    fix compilation vector code

commit 2064211c149fa24dcce9be6b894a97ef93fa8bc8
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Feb 6 21:21:10 2017 +0000

    fix score r without macro

commit 86356b0442ef3f6c171213829a1ed5b566725f34
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Feb 6 15:45:18 2017 +0000

    inconsistency kr sr ksmps fix

commit 4ed3d0303f328744ae7226751f90ff476195317f
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Feb 6 09:49:28 2017 +0000

    array offsets corrected for audio var iterator

commit c66873d96b7334dc7648ee9ae05d46a0b4e7c4a4
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Feb 2 12:29:05 2017 +0000

    added deinit support to plugin framework

commit 8adbd2d919811c422ccb01199b7cbd8e72cf3016
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Jan 30 21:25:40 2017 +0000

    pvstrace opcode

commit 8d91c2c4537099c315250d83f5f8dd7165c71dd5
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Jan 30 12:46:06 2017 +0000

    pvsbin methods

commit 648adab4cfead2af603b29f70cb288f424ad354c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Jan 30 09:52:27 2017 +0000

    binops for arrays

commit 284a0a9e4852df4a2c20aa96bb2a54772f39298f
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Jan 29 15:38:11 2017 -0500

    Resolved difficulties with stopping and restarting in CsoundThreaded hosts.

commit cc7a903118a65e03f0b67eea5cacf31ee16b157d
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jan 29 19:05:41 2017 +0000

    array operators completed

commit 71ef2536ba4c87c422d67447b55bcd9bcf2d6f32
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Jan 28 22:11:45 2017 +0000

    array operators

Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Jan 28 18:08:28 2017 +0000

    added csd and support for arrays to CPOF

commit 7263c82a9a9648adbc0d985e04c117df0b35f0d1
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Jan 28 17:47:33 2017 +0000

    added a comment about divide by zero

commit 1fb13a751927e4ffd1875eff8e9cd09e83a7a630
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Jan 27 18:14:15 2017 +0000

    AuxAllocAsync

commit a7270c581be57ff61e6428003bde35a7907e43c2
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jan 26 19:14:34 2017 -0500

    Added CsoundThreaded class in csound_threaded.hpp.

commit bad7b270bc2d9b2c879f118331f59237d30cc286
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Jan 26 23:24:58 2017 +0000

    plugin interface

commit f132c54efcd2617e648abcf2c099cd2f9961280f
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Jan 26 12:09:48 2017 +0000

    overriding sr issue

commit df523604caf36367f285a7c8e4c087a7e87d73f9
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Mon Jan 23 10:33:54 2017 +0000

    Own ugens and initial port of some supercollider ugens

commit d227c15bea6c06da580a10ef8ee9d94c41b12a32
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Jan 23 10:06:27 2017 +0100

    added emugems: bpf, xyscale, ntom, mton, etc. -- beginning of scugens (supercollider ugens): phasor, lag, lagud, trig)

commit 4429b88f5d3cfeb6e2399cc176e23716485edd3
Author: Nikhil Singh <nsingh1@berklee.edu>
Date:   Sat Jan 21 21:01:46 2017 -0500

    Cleanup, fixes, small feature additions, multiple CSDs added to console output

commit fa9cfeeee3e4d89ecf760eb136066c944a80a18d
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Jan 21 21:04:41 2017 +0000

    fixing the lexer for octal in strings

commit 5b6178ea6fa1058cd7d5425598f67fbeadde09e5
Author: Nikhil Singh <nsingh1@berklee.edu>
Date:   Fri Jan 20 20:30:49 2017 -0500

    iPad portrait SplitView fix+animation, info popover resizing, stop button fix in Soundfile Pitch Shifter.

commit 548063327b8ccf8fbf3c76151c0659b1855ba88c
Author: Nikhil Singh <nsingh1@berklee.edu>
Date:   Fri Jan 20 15:13:12 2017 -0500

    Csound-iOS API updates, deprecations+warnings addressed. Csound-iOS Examples cleaned up, enhanced/expanded, and reordered. Csound-iOS Manual revised, expanded, updated. Updates to API and examples support iOS 10 and Xcode 8.

commit 3e0b441b55fd8e07d70b0908da8165b889feb883
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Jan 16 16:22:18 2017 +0000

    escaping ) case in macro calls

commit a42d90e24c450866fd699b655aeeb178c327ab50
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Jan 16 14:28:58 2017 +0000

    escaping chars in macro args

commit 9328274078204903a91f0b36d33582f216fe9eb3
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Jan 16 12:50:30 2017 +0000

    fix to score strings

commit 1d0765e6a5cacc998537904119c874cc00ac4027
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Jan 15 17:41:17 2017 +0000

    fix [..] expansion in scores

commit 5d6b4983125a40ddace744d143e017cd8d3789e0
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Jan 15 16:59:46 2017 +0000

    fix skipped line after score n opcode

commit 732135e92ecafbf36e7f67dc6f43397dd3e1136c
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Jan 14 17:33:48 2017 +0000

    fix to n/m in score

commit 59859b231326f6510b123b4e4c7a9e0d82dc39c8
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jan 12 15:50:07 2017 -0500

    Ableton Link opcodes

commit d5ca5b311fa7ef077d916b3087807ea4bd39e41b
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Thu Jan 12 19:31:02 2017 +0100

    Added getA4 to ctcsound.py

commit 59ece0a0e24ab1bc0aa216cf96ec29b28a8abd5a
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Jan 12 15:44:04 2017 +0000

    line numbers in score r opcode

commit 9a69a8554ee8e5bf112b8fb09a018be6e3e4f221
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Jan 12 15:11:33 2017 +0000

    added GetA4

commit a78e6b09c8512ca7c56ce330f68f66a1591c045d
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Jan 10 15:26:16 2017 +0000

    fix score r opcode; remove memory leak

commit 728d9c4ab0179b27404691dca29ae9eae2d7a746
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Jan 8 21:07:23 2017 +0000

    fix for m/n in sxore; r still broken and exta tracing

commit 1d43cc3a4a290c4a1dbb94902878d4b20bfb02a2
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Jan 2 21:24:32 2017 +0000

    macro names restricted

commit a02db25c3d2bc3ab0d89d4f9a6487bf325d8446e
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Dec 28 21:27:04 2016 -0500

    fix crashing/hanging problem when csoundReset called multiple times

commit 3bf16bfa77dc15d4114459fb9588a2f2492427d4
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Dec 21 22:19:34 2016 +0000

    fixed dnoise

commit 6d421193416a49f6b0acdaeadc97f9ab3ebd2ef9
Author: Edward Costello <phasereset@gmail.com>
Date:   Mon Dec 19 17:11:42 2016 +0000

    Websocket server can only accept one protocol output, so limiting intype to just a single argument

commit f14212cd25ca9893aa47805fea04b79eb00e87ec
Merge: e8e6776 8a15d3c
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Tue Dec 13 16:42:38 2016 +0000

    Overloaded pvs2tab and tab2pvs, update websocket opcode

commit 1faad96f6adc27eca7f6a5b6ce7cb89ca72f7c97
Author: Edward Costello <phasereset@gmail.com>
Date:   Sun Dec 11 22:18:46 2016 +0000

    Fixed pvs2tab

commit 2d4872f9d1de7b94dda5988042124e249d873912
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Dec 8 11:53:36 2016 +0000

    finished chasing mallocs in opcodes and engine

commit a0f45dc3576ec71e622ca75a782bc85be3a30be8
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Dec 7 17:19:54 2016 +0000

    fixes to sprintf

commit 95c84c040537fd5c637bf42d3ee76f4d07ce5e75
Author: Edward Costello <phasereset@gmail.com>
Date:   Wed Dec 7 00:27:47 2016 +0000

    Overloaded pvs2tab and tab2pvs so they can create and use split magnitude and phase arrays

commit 501c536c70682a9e115368ccb432cc0df8ff6a60
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Dec 6 16:21:50 2016 +0000

    OSC change and tanslations

commit 7a812be7525165cb9313c1eb605345e890946b94
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Nov 26 07:54:21 2016 +0000

    fix rtjack

commit 69a5d1df0db22a42ef85a4c3f61351bf1487b809
Merge: 1b08942 c1678ef
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu Nov 24 18:42:18 2016 -0500

    Merge tag '6.08.0' into develop

    6.08.0
