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

- several new unary functions/opcodes for k-rate and i-time numeric arrays: ceil, floor, round, int,
frac, powoftwo, abs, log2, log10, log, exp, sqrt, cos, sin, tan, acos, asin, atan, sinh, cosh, tanh,
cbrt.

- several new binary functions/opcodes for k-rate and i-time numeric arrays: atan2, pow,hypot, fmod,
fmax, fmin.

- tvconv -- a time-varying convolution (FIR filter) opcode

- bpf, xyscale, ntom, mton (from SuperCollider?)

- OSCsendA asynchronous version of OSCsend

- OSCraw to listen for all OSC messages at a given port.

- new implemetation of OSCsend not using liblo, with previous version
  now called OSCsehd_lo

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

- p2 and p3 are now at higher precision and not truncated to 6 decimal places

- new opcode d to switch off infinite otes (deote)l same as i wit egative p1

- named instruments can be turned off with i if a - follows the "


### Options

-

### Modified Opcodes and Gens

- ftgentmp improved string arguments

- hdf5read opcode now reads entire data sets when dataset name string
  is suffixed with an asterisk

- use of non power-of-two lengths now acceptable where before it was inconsistent

- ampmidid optionally can be aware of 0dbfs

- dust and dust2 at k-rate now conform to the manual (NOTE: this is an
incompatible change)

- In prints the format %% now prints one %

- OSClisten can be used with no data outputs

- GEN18 corrected to write to requested range

- sockrev now can read strings

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

- bug in sprintf removed

- bug in soundin removed

## SYSTEM LEVEL CHANGES

### System Changes

- soundin now uses the diskin2 code

- out family of opcodes reworked to reduce interleaving costs and to
  take proper regard if nchnls value.

### API

- New `csound_threaded.hpp` header obviating need for `csPerfThread.cpp` object in some projects.

- added GetA4 function

- New framework for plugin opcode development in C++.

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
UNDOCUMENTED/UNDELETED

commit 0438640cc796ef220ab31ed10a890eb9d9636d36
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Apr 12 20:56:17 2017 +0100

    fix to score opcode d

commit 7366e3b24038edeeede20907bf70eca09a9db54c
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Apr 12 19:55:35 2017 +0100

    loscil changed to fpt indexing

commit 9dd10ea83e7609e0d1d4ffb54106cbeb793796d5
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Apr 11 22:31:02 2017 +0100

    trying to set non-blocking mode on windows

commit cdf7a90968d4559d79d56c61984d9429e7019c24
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Apr 11 21:27:52 2017 +0100

    Strdup to Csound

commit 1323b26c63fb2435967d542d042de7b7218564d8
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Apr 8 21:58:58 2017 +0100

    vbap has arbitrary number of speakers/channels

commit fd14e9437b6358a5927e3ce1d0f341438c65a3db
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu Apr 6 18:51:46 2017 -0400

    added zdf_1pole filter

commit 88b1db83f81655420375109302e7aa6ead4f8839
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Apr 6 22:27:31 2017 +0100

    D OSC type

commit 503913df2468332f837c21e0c7a9cddb2174686d
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Apr 6 17:17:13 2017 +0100

    possible fix to vbap

commit c95e92fb2560fb37757209bf4a2ad263c91d7b44
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Apr 6 15:22:13 2017 +0100

    vbapinit with array

commit 193b3838dc14dd11ead838f89eb255260e790c65
commit 6323888c28515c4365dd1ca429a968bf97862d38
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Apr 6 06:28:48 2017 +0100

    trying to deal with bundles again

commit 8e391a1b0f5962a19dbec3ca5ca63363b802ad1a
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Apr 5 19:00:04 2017 -0400

    implemented diode_ladder, fixed skip handling for zdf_ladder

commit 2fff5566ada754c47ad443c3b47d03ebe389eb95
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Apr 5 21:45:11 2017 +0100

    trying to deal with bundles

commit 9526f92ac0a1908be632017dc9034a997f49d97e
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Apr 5 18:33:47 2017 +0100

    ignoring bundles 2

commit 92de2b90b6ea260cef6a8d714e91921d99c17307
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Apr 3 20:31:07 2017 +0100

    G type fixed

commit 3814b45a7c804b09ac1944f76eeac52615c7a88c
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Apr 1 18:05:13 2017 -0400

    fix for pmidi.c and csoundLock/UnLock: add include of csGblMtx.h, fix setting of HAVE_PTHREAD for all targets instead of just for libcsound64

commit 6305e51a33ce6c7ceb7a0cfe59e8d0f345745f46
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Mar 28 14:08:56 2017 +0100

    sockrec string version seems to be working

commit 6305e51a33ce6c7ceb7a0cfe59e8d0f345745f46
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Mar 28 14:08:56 2017 +0100

    sockrec string version seems to be working

commit 58801753aabbd705ec8a32b89d8533003930ce6a
Author: U-HF-31335\Administrator <obrandts@gmail.com>
Date:   Fri Mar 24 15:16:05 2017 -0700

    fix partikkel channelmask panning curves

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

commit 284a0a9e4852df4a2c20aa96bb2a54772f39298f
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Jan 29 15:38:11 2017 -0500

    Resolved difficulties with stopping and restarting in CsoundThreaded hosts.

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

