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

A mixed bag of new opcodes and many fixes and improvements.

Also as usual there are a number of internal changes, including many
memory leaks fixed and more robust code. 

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- select -- sample-by-sample comparison of audio selecting the output

- midiarp opcode generates arpeggios based on currently held MIDI notes.

- hilbert2 --  a DFT-based implementation of a Hilbert transformer.

- Ableton Link opcodes for synchronizing tempo and beat across local area networks.

- pvstrace -- retain only the N loudest bins.

- tvconv -- a time-varying convolution (FIR filter) opcode

- bpf, xyscale, ntom, mton (from SuperCollider?)


### New Gen and Macros

-

### Orchestra

- Including a directory of UDO files o longer fails if more than about 20 entries

- It was possible for kr, sr, and ksmps to be inconsistent in one case, no more

- Macro names better policed

- octal values as \000 can be in strings

### Score

- Improved line number reporting in r opcode and case with no macro implemented

- m and n opcodes fixed

- Expansion of [...] corrected and improved

- Strings in scores improved

- The ) character can be in a macro argument if it is escaped with \

- Use of the characters  e or s could lead to errors; now fixed

- Macro names better policed

### Options

-

### Modified Opcodes and Gens

- ftgentmp improved string arguments

- hdf5read opcode now reads entire data sets when dataset name string
  is suffixed with an asterisk

- use of non power-of-two lengths now acceptable where before it was inconsistent

- ampmidid optionally can be aware of 0dbfs

- dust and dust2 at k-rate ow conform to the manual (NOTE: this is an
incompatible change)

- In prints the format %% now prints  one %

- OSClisten can be used with no data outputs

### Utilities

- dnoise fixed

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

- bug in push/pop opcodes fixed (this opcode is now a plugin and deprecated)

- bug insprintf removed

- bug in soundin removed

## SYSTEM LEVEL CHANGES

### System Changes

- soundin now uses the diskin2 code

- out family of opcodes reworked to reduce interleaving costs and to
  take proper regard if nchnls value.

### API

- New `csound_threaded.hpp` header obviating need for `csPerfThread.cpp` object in some projects.

- added GetA4 function

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

commit eaca4bbc4616ac0294d6c9f550e11b84dcb44bd0
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Mar 13 15:21:13 2017 +1100

    Removing HTML5 Csound editor which has quit working.

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

commit 40892fc005d6faa21d90dfcfaf4ffc67e1d336e6
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Feb 20 20:37:42 2017 +0000

    const iterators

commit 400c72d543a888795b0db175c7b6088d7efefd1c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Feb 19 21:26:44 2017 +0000

    support for audio sig vars

commit a6ecadda14cf98b02d3faa4d0a95ad912765ff8b
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Feb 9 18:37:02 2017 +0000

    pvs loops straightened

commit 937745c70d881341d2ea0b9572cb9435beb1e63e
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Feb 7 21:42:32 2017 +0000

    fix compilation vector code

commit 4ed3d0303f328744ae7226751f90ff476195317f
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Feb 6 09:49:28 2017 +0000

    array offsets corrected for audio var iterator

commit c66873d96b7334dc7648ee9ae05d46a0b4e7c4a4
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Feb 2 12:29:05 2017 +0000

    added deinit support to plugin framework

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

commit df523604caf36367f285a7c8e4c087a7e87d73f9
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Mon Jan 23 10:33:54 2017 +0000

    Own ugens and initial port of some supercollider ugens

commit d227c15bea6c06da580a10ef8ee9d94c41b12a32
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Jan 23 10:06:27 2017 +0100

    -- beginning of scugens (supercollider ugens): phasor, lag, lagud, trig)

commit 5b6178ea6fa1058cd7d5425598f67fbeadde09e5
Author: Nikhil Singh <nsingh1@berklee.edu>
Date:   Fri Jan 20 20:30:49 2017 -0500

    iPad portrait SplitView fix+animation, info popover resizing, stop button fix in Soundfile Pitch Shifter.

commit 548063327b8ccf8fbf3c76151c0659b1855ba88c
Author: Nikhil Singh <nsingh1@berklee.edu>
Date:   Fri Jan 20 15:13:12 2017 -0500

    Csound-iOS API updates, deprecations+warnings addressed. Csound-iOS Examples cleaned up, enhanced/expanded, and reordered. Csound-iOS Manual revised, expanded, updated. Updates to API and examples support iOS 10 and Xcode 8.

commit d5ca5b311fa7ef077d916b3087807ea4bd39e41b
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Thu Jan 12 19:31:02 2017 +0100

    Added getA4 to ctcsound.py

commit 9a69a8554ee8e5bf112b8fb09a018be6e3e4f221
commit 6d421193416a49f6b0acdaeadc97f9ab3ebd2ef9
Author: Edward Costello <phasereset@gmail.com>
Date:   Mon Dec 19 17:11:42 2016 +0000

    Websocket server can only accept one protocol output, so limiting intype to just a single argument

commit f14212cd25ca9893aa47805fea04b79eb00e87ec
Merge: e8e6776 8a15d3c
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Tue Dec 13 16:42:38 2016 +0000

    Overloaded pvs2tab and tab2pvs, update websocket opcode

commit 95c84c040537fd5c637bf42d3ee76f4d07ce5e75
Author: Edward Costello <phasereset@gmail.com>
Date:   Wed Dec 7 00:27:47 2016 +0000

    Overloaded pvs2tab and tab2pvs so they can create and use split magnitude and phase arrays

