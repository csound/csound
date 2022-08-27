
# CSOUND VERSION 6.18 RELEASE NOTES

Mainly a bug-fixing release 

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- 

### New gen

### Orchestra

-

### Score

### Options

### Modified Opcodes and Gens

- 

### Utilities

- 

### Frontends

### General Usage

## Bugs Fixed

-

# SYSTEM LEVEL CHANGES

### System Changes

-

### Translations

### API


### External Plugin Code


### Platform Specific

==END==

========================================================================
scommit 162977998a8ca81f4963ecda3d1a3bfccafa08ad
Merge: b016904e2 ed56e6d47
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Wed Aug 24 14:11:06 2022 +0100

    reverted changes in timeinstk and timeinsts; fixed order of .k and .i in times and timek; provided two sets of opcodes elapsedcycles, elapsedtime and eventcycles and eventtime which act like timek, times, timeinstk and timeinsts but return the correct values instead of being one cycle late

    Bela changes and improvements - csound6 edition

    Bela: communicate Bela's blocksize to csound. Error and explain if ksmps is incompatible with it

commit 1e89fbd070ddf2520fa49ec863621ab8d2e44a60
Author: Giulio Moro <giuliomoro@yahoo.it>
Date:   Tue Oct 20 17:20:38 2020 +0000

    Bela: refactored initialisation

    Bela: Trill: fixed signedness and initialisation

    Bela: do not let the linker wrap symbols. This also gives us back Bela's command-line options.
    

    Bela: cleaner end of program (no more high-pitched whine)

    Bela: only run (and faster) the auxiliary task if there are some Trills connected

commit ac314fe630c2b67e3442f285adfe70e70fe326ba
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Aug 17 19:00:16 2022 -0400

    fix check for SF_FORMAT_MPEG to use check_c_source_compiles()

commit 5ed74b4cdcfe28c05015f0af82d352a906120ba5
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Aug 17 17:53:03 2022 -0400

    check if SF_FORMAT_MPEG symbol is available in sndfile.h and disable USE_MP3 if not found

commit 9698528d5e716baf14ae732dccbea5c0ff130e37
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Tue Aug 9 00:01:22 2022 +0200

    Move USE_MP3 configuration

    MP3 seemsworking for output

    mp3 output


    Moving to allow MP3 files like OGG

    wasm: fix max stack size bug

    wasm: increase render performance, dont hold the event loop

    wasm: fix renderEnded event trigger

    fix timeinstk and timeinsts to match timek and times


    fixing #1613

commit 5b26278a48917a7d851a0e54a5e82d30fa1ce214
Author: Victor Lazzarini <victor.lazzarini@mu.ie>
Date:   Wed Jul 6 13:04:26 2022 +0100

    fixing code


commit 2c51e053ab8c931df7fdbd3ff1e8a396caf7f07b
Author: Victor Lazzarini <victor.lazzarini@mu.ie>
Date:   Sun Jul 3 22:16:41 2022 +0100

    trying to fix auhal multichannel

commit 1eb4d1237a49e135261edd5c3ceeea6a211d678a
Author: Victor Lazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jun 4 15:06:02 2022 +0100

    fixed #1611

    fix qnan even when compiled with fast-math

commit 73a287c190cc96be0c51fcb4a79eeb3de90b6c3a
Author: Hlöðver Sigurðsson <hlolli@gmail.com>
Date:   Wed May 4 13:34:05 2022 +0200

    add a fix for static wasm build and publish wasm@6.17.1

commit 4baf2f0cf339ae674454e0ca5d918cefb9b4c08a
Author: Hlöðver Sigurðsson <hlolli@gmail.com>
Date:   Tue May 3 23:36:21 2022 +0200

    fix dlload plugins with wasm


commit 6609d8cc50db40cd8fcdc62a4c95e4c8074ee04c
Author: Hlöðver Sigurðsson <hlolli@gmail.com>
Date:   Mon Apr 18 22:29:58 2022 +0200

    new csound/browser beta version

commit 66cd7a78e6b38824812037d323c6ecd85805da8c
Author: Hlöðver Sigurðsson <hlolli@gmail.com>
Date:   Mon Apr 18 22:28:43 2022 +0200

    fix sampleRate for single-thread worklet

commit 1d09317c66489bebe5ad7aa91651882ff4550f56
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Apr 16 21:16:01 2022 +0100

    remove duplicate entrt for dust r k-rate


Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Fri Apr 15 18:02:38 2022 +0100

    offset the xdel input in sample accurate

commit b579528f52194286d0a10b4f3cb52b5d49269424
Author: Victor Lazzarini <victor.lazzarini@mu.ie>
Date:   Fri Apr 15 12:31:18 2022 +0100

    finally fixing sum bug correctly

commit 27fa530c68b8de2a2e3209a53e7abce51f2ba805
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Apr 12 18:30:12 2022 +0100

    add gains to scanmap vector form

commit c1f52ed12c2e9770f95ca678b6d6e205bfab83bf
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Mar 29 20:57:57 2022 +0100

    fixes to scanmap

commit 512984db5dbb637fc53b3a50ff8480dbf715d6a1
Author: Victor Lazzarini <victor.lazzarini@mu.ie>
Date:   Wed Mar 16 09:48:56 2022 +0000

    fixed crash on channel setting with empty channel name

commit 8f14c35b0c220b418378d7559856128b54ca214f
Author: Victor Lazzarini <victor.lazzarini@mu.ie>
Date:   Mon Mar 14 08:58:23 2022 +0000

    issue #1599 fixed

commit 0445ba806c12df66e987b8e66e862cce7ff06b00
Author: gesellkammer <eduardo.moguillansky@gmail.com>
Date:   Fri Feb 25 15:50:51 2022 +0100

    fix typo in ctcsound

commit 6138d70504dfdc83ef0b3f2b8d85e5640e91d1e2
Author: gesellkammer <eduardo.moguillansky@gmail.com>
Date:   Fri Feb 25 13:18:53 2022 +0100

    fix midi list printing to stdout part of the list

commit 83f4f96e43773baea7d0ba4d8932f06c01d54f64
Author: Victor Lazzarini <victor.lazzarini@mu.ie>
Date:   Thu Feb 17 12:01:10 2022 +0000

    bug in sum

commit 5413528b452a5b53d3f3a30453dac7b36875a38a
Author: Victor Lazzarini <victor.lazzarini@mu.ie>
Date:   Fri Feb 4 16:09:57 2022 +0000

    removed Emscripten build

    Merge tag '6.17.0' into develop
    