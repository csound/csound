# CSOUND VERSION 6.08 RELEASE NOTES

As usual there are a number of opcode fixes and improvements, but the
major changes are in the language structures.  First the score language
has all-new treatment of macros and preprocessing, bringing it in line
with those of the orchestra.  The parsing of the orchestra has had a
number of fixes as outlined below.

A major, and not totally compatible change as been made in reading
and writing array elements.  The rate of the index determines the
time of processing.  This simplifies much  code and is explicable; the
earlier ad hoc code had many anomalies.

Also as usual there are a number of new opcodes and internal fixes
to memory leaks and more robust code.
  
-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- dct -- Discrete Cosine Transform of a sample array (type-II DCT)

- getftargs -- copy arguments of a gen to an array

- mfb -- implements a mel-frequency filterbank for an array of input magnitudes

### New Gen and Macros

- quadbezier generating Bezier curves in a table

### Orchestra

- The character Â¬ is now correctly treated as a variant of ~ for bitwise not

- Lexing bug which could corrupt strings fixed

- Ensure no newlines in string-lexing

- Small improvement in reported line numbers

- Better checking of macro syntax

- Improved parsing of setting of labels

- Added error handling for unmatched brackets for UDO arg specification

- Check that `#included` file is not a directory

- Deeply nested macro calls better policed

- For years Csound has fixed the pitch of A4 at 440Hz.  Now this can be set in the header using the new r-variable A4, and also read with that variable

- Floating point values can use e or E for exponent

- For array reading of indexed values the rate of the index controls the rate of the action.  ***NOTE THIS IS AN INCOMPATIBLE CHANGE*** but fixes many anomalies

### Score

- New code to handle macros and other preprocessor commands. Brings it into line with orchestra code

### Options

- The tempo setting can now be a floating point value (previously fixed to integer)

### Modified Opcodes and Gens

- Problems in centroid fixed.

- Better treatment of rounding in printks

- OSC extended to include multicast

- Faust opcodes brought up to date with faust

- oscil1 and oscili can take a negative duration

- fout opcode documentation clarified

- Release time in mxadsr fixed

- centroid opcode  extended to take array inputs in addition

- ptable opcodes are now identical to table family

- ftgen now as arraay input option

### Utilities

- pvlook now always prints explicit analysis window name
     
### Frontends

- ctcsound.py: All new python frontend

- icsound:

- csound~:

- csdebugger:
  
- HTML5

 - csound.node: Implemented for Linux, minor API fix.

 - pnacl: Added compileCsdText method to csound object

 - Emscripten:
 - CsoundQT has its own notes at https://github.com/CsoundQt/CsoundQt/blob/develop/release_notes/Release%20N
otes%200.9.2.1.md

### General Usage

- Checking of valid macro names improved

- ```#undef``` fixed

## Bugs Fixed

- Fix to prints in format use

- jitter2 reworked to make it more like the manual.

- oscbank has had multiple fixes and now works as advertised

- bformdec1 with arrays and type 4 fixed

- Bug in pvsceps fixed

- In various formatted print opcodes extra trash characters might appear -- fixed

- Assigning variables with --sample-accurate could give unexpected results; this is believed fixed now

- padsynth square profile fix, and opcode prints less depending on warn level

- gen31 fixed

- gen41 fixed

- Bug in sensekey fixed
    
- pvlook reports actual window used

- a number of issues in centroid fixed
    
## SYSTEM LEVEL CHANGES

### System Changes

- New score lexing and preprocessor

- MAC line endings now work again

- System information messages (system sampling rate, etc) are now directed to stdout

### API

- Now supports named gens

- fterror now in API

- API functions SetOutput and GetOutputFormat fixed

- Many API functions now use const where appropriate

- Messages can now be directed to stdout from the API by using CSOUNDMSG_STDOUT attribute
    
- New Lisp CFFI and FFI interfaces tested with Steel Bank Common Lisp (64 bit CPU architecture), runs in separate thread
    
### Platform Specific

- iOS

 - 

- Android

 - Multichannel input and output allowed

- Windows

 - csound64.lib import library added to Windows installer
    
- OSX

 - 

- GNU/Linux

## GIT LOG

```
commit 766f2c51cc3a62ea9a8381bb57d1c3fefebab92f
Merge: 3d9db1b af85998
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Oct 17 14:55:29 2016 +0100

    more notes -- typos

commit 3d9db1bc196c9d0f4baabe40f9b610dbba6ae021
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Oct 17 14:33:58 2016 +0100

    more notes -- typos

commit af859981166838b03a6c783b261c8c121126649d
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Oct 16 14:01:59 2016 -0400

    Update Verson_6.08.md

commit 5fdf20519683fe921d1702f770b669fbebc21404
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Oct 16 13:59:51 2016 -0400

    Update Verson_6.08.md
    
    Changed format from plain text to markdown for greater consistency and for greater online readability.

commit f7681de0b9c7ea6f0d7dcd9650f70a3cc4c62e74
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Oct 16 13:42:16 2016 -0400

    Changed release notes to markdown format.

commit 186b4430b24f8dd0c6f1aedd35b02dd1b1dcddf9
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Oct 16 13:41:11 2016 -0400

    Update Version_6.08

commit 24790ceca77e3a9f1ae43afff2e861b0af639cc7
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Oct 16 17:20:51 2016 +0100

    more on draft of release notes

commit d52050cdd3f4e445f4c4bf0149f9de7cac4cb5a8
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Oct 16 15:47:47 2016 +0100

    first draft at release notes

commit 623092e2fab1b8a47ab7a2ed38ca859a1ef06a58
Merge: aa5c04f 2e4e390
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Oct 14 09:37:01 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit aa5c04f343e0260e88b2ea7ef6ec6e203683337b
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Oct 14 09:36:53 2016 +0100

    fixed mxadsr release time

commit 2e4e390dbfb9af82090cb45f12f8eadd2f78fe98
Merge: 2549fc3 cfcd93a
Author: gogins <michael.gogins@gmail.com>
Date:   Tue Oct 11 22:06:52 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 2549fc31888484de81b446171ae073f8aa350cdc
Author: gogins <michael.gogins@gmail.com>
Date:   Tue Oct 11 22:06:29 2016 -0400

    Slight fix to csound.node interface.

commit cfcd93a39f1c9e110ac1f9d549cd7551e5436463
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Oct 11 19:51:01 2016 +0100

    tidy up fout

commit d73c32f93f8cb1672fc10069961c17f54a4527d7
Merge: 73541fd 34d13f3
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Oct 11 11:51:18 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 73541fda5c5c5beee31302f8cb1b473405a96bee
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Oct 11 11:48:58 2016 +0100

    fixed non-interp modes in oscbnk

commit 34d13f3bc961c84e7e022857eb5bd889096f5cd0
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Sat Oct 8 23:49:46 2016 +0200

    Correction for keeping callback refs in ctcsound.py

commit 8ce20244c13833ad31b7f7426e144f12b6814a67
Merge: 319bab5 674d797
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Oct 8 12:27:33 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 319bab59024279777e0f2fc3c70eeb7d48d3bb2b
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Oct 8 11:20:37 2016 +0100

    oscnbank blow up fix

commit 674d797a12c3530d895656c6dfa669fa71e04d20
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Oct 7 21:28:53 2016 +0100

    new array rate model again

commit 5bc8d04827660c72553719b40440d268effe1c5c
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Oct 7 21:14:56 2016 +0100

    new array rate model

commit 282a0500b7616b52e5d861fd288aae96df67d1ed
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Oct 7 15:28:32 2016 +0100

    oscil1 & i now can take negative duration

commit 00eb57bbaa06a910a202618e9125dc5975d18756
Merge: 99ca385 93964a4
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Oct 7 14:57:27 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 99ca385b78842f7fec31e6761fa89ac92eb2b3c7
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Oct 7 14:57:17 2016 +0100

    fsig init function

commit 93964a4a83e915ceea1f3b4f5285e26316229000
Author: Felipe Sateler <fsateler@gmail.com>
Date:   Wed Oct 5 19:27:19 2016 -0300

    po: custom target name and .mo file were ambiguous, resolve that
    
    This caused problems for the ninja generator

commit 81e20ede83e06a01625b6b1c78ab9d1824af9f7e
Author: Felipe Sateler <fsateler@gmail.com>
Date:   Wed Oct 5 18:15:38 2016 -0300

    rtalsa: define _DEFAULT_SOURCE
    
    This silences a warning as _BSD_SOURCE is deprecated in favor of the new name.
    Do not drop the old name to keep compatibility with glibc < 2.19

commit eed09ecf7d845996d98d298ee6c66f0a23fc3d64
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Oct 4 14:58:58 2016 +0100

    defensive programming in OSC deinit

commit 5a0fd39905a386b09705b3439e3a1dadd7cb120a
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Oct 4 13:09:37 2016 +0100

    less use of Die

commit 3b54405182b872834f9e8ff89d4aec8f32e6167b
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Oct 4 13:07:43 2016 +0100

    less use of Die

commit bce534adf891b4666652cfeff486a5c03e9514b6
Merge: e836d67 c2c72ae
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Oct 4 13:06:03 2016 +0100

    less use of Die

commit e836d672cd1eb68786213ac197fb712c48fbd77f
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Oct 4 13:04:13 2016 +0100

    less use of Die

commit c2c72aed47934dd096cf50c7971ce5419cffcc01
Merge: 5427520 ef37604
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Oct 3 08:53:07 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 5427520395cede451a7fc8182c549395c72eedb6
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Oct 3 08:52:23 2016 +0100

    csoundGetOutputFormat

commit ef37604b497a47665ab7b3c898f0481d2f24c1fc
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Mon Oct 3 03:31:43 2016 +0200

    Typo

commit 11419d121fcaa91aa0473f102f113a64aa95c0f6
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Mon Oct 3 03:28:54 2016 +0200

    Added new outputFormat function to ctcsound.py

commit c4e7b2e8199c0a32cfd1b2fc671945bd3d994c38
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Mon Oct 3 02:56:45 2016 +0200

    Updated French translation

commit b2ebf671cd5acb3d9db3099f007f5475b38ee561
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Oct 2 20:43:33 2016 +0100

    typo

commit 26a292f1ac699f2afba8ee00bb2ddd3eacb377b1
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Oct 2 20:39:01 2016 +0100

    reverting changes to die

commit 9a93107aae43e0579b26a1c5cec13303fea64c56
Merge: 7c60ef9 00ddd8a
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Oct 2 18:22:35 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 7c60ef9268f1f61ed12aa831e31306396dbf3325
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Oct 2 18:22:17 2016 +0100

    fixed csoundGetOutputFormat

commit 00ddd8ae8cd76385623eaf82ea02b1f6b2bc29e6
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Oct 2 18:01:39 2016 +0100

    deprecated Die replacing with exitjmp

commit 7b7367f41af6c9f282116715c8c74c9f8f99e0e8
Merge: 30e864d b244ef5
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Oct 1 14:20:04 2016 +0100

    conflict resolved

commit 30e864dc27297a99e9157c4c702e4133195e73da
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sat Oct 1 14:15:40 2016 +0100

    new api function and other const char fixes

commit b244ef53ad139b957ae8cac88d63959ac4cd15bb
Author: Steven Yi <stevenyi@gmail.com>
Date:   Fri Sep 30 18:03:04 2016 -0400

    fixed issue with cs_hash_table_remove: may have leaked if first item in bucket was freed but further items existed in bucket; also prevents crash on Windows when OSC_reset was called

commit eb52af96bfdc07e5a2e20cd5c964f5c31d08eedc
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Sep 30 14:50:10 2016 +0100

    fix errors introduced by const

commit 996c043e535c8b9ef92aa007dcf3903b9ad99741
Merge: 2445e1e cba176e
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Sep 30 13:25:45 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 2445e1e3ddcff7d7bebfe4a1b5a16effdd7fb95b
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Sep 30 13:25:32 2016 +0100

    strings and layout

commit cba176e0fa24c3b173a314122b7961a18e4b5648
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Sep 29 13:39:01 2016 -0400

    Use GetNchnls_i... to get input channels! for Android.

commit b0a31614fbbe22e1c3f117a3a3e571ca17dc72a8
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Sep 29 12:39:34 2016 -0400

    Further changes to implement const correctness.

commit 7f72a388ea0db770280e4d5802d2e287c9e0cf94
Merge: eebeb9a 0f0367b
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Sep 29 12:22:12 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit eebeb9ae51b7dd9db16c70e6c95272b61fb61056
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Sep 29 12:21:36 2016 +0100

    made relevant char* and char** const in the API

commit 0f0367bb38bde275771d1af3b231aa2b9156529a
Merge: afc1456 85b9ca9
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Sep 29 00:57:10 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit afc1456ccdeee1728f68e08881d772f27bbcfac3
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Sep 29 00:56:51 2016 +0100

    tweakedcod

commit 85b9ca9b5c048a243185bfd48926b0eff0a68191
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Sep 28 19:10:51 2016 -0400

    Updating OpenSL.

commit fa2152bb2888c3dd484edd65b5ea3d3006faa6f3
Merge: 7ad62d9 6f1d8ed
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Sep 28 19:07:35 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 7ad62d99b0b93877ac74a2eee71a78f75d4b5f3f
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Sep 28 19:06:20 2016 +0100

    setting android input to multichannel

commit 6f1d8ed097ae48c0d9c7d617126ba9709887cc4d
Merge: f24a44c 3373449
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Sep 28 16:36:03 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit f24a44cfbba5006faea5bdd26db0ef7e4d00516e
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Sep 28 16:34:46 2016 +0100

    ..and e or E in fpt in potential new scorelexer

commit 33734490503b76653ab85a236e4559fe9019bfc9
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Sep 27 12:30:46 2016 -0400

    Added 3D score view in Silencio.js for examples.

commit 710ef8d20562a9bb9eab319f4a203e9bbe1ad092
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Sep 27 15:18:28 2016 +0100

    allow E in fpt constants

commit ceab68911fcee64e3af0ac01d7437dae5b58e5ea
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Sep 27 14:58:43 2016 +0100

    allow cmdine tempo ti be fpt value

commit 790954de2d9325683ca9ecbc1b8bce4cc80148d8
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Sep 23 10:00:36 2016 +0100

    extending libpy hack to OSX

commit 110e3caf3091d10de70225f9d12f48484cb3f084
Merge: a563922 dd866f0
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Sep 21 08:57:24 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit a5639225b7ef265d8e3cdfdc6e138f295750d2a2
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Sep 21 08:57:19 2016 +0100

    fixes to faustopcodes and to csound starting with no score

commit dd866f0807812d97915bd0be63f021032f8300a6
Merge: 5133688 09b4518
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Sep 20 10:41:18 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 5133688fab95e1a904ec915f3aafef053d54a60b
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Sep 20 10:41:06 2016 -0400

    Conforming JavaScript files with Silencio.

commit 09b451839c49a3f913983df2d38f441b74570dad
Merge: 67b10a9 2ef3d7c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Sep 20 08:41:08 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 67b10a9b4c441b97a6e447f097b28f2c409f9baa
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Sep 20 08:41:04 2016 +0100

    fixed csoundSetOutput

commit 2ef3d7cd3f0a910122a61bf45d7fd7b1f22724fa
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Fri Sep 16 17:33:43 2016 -0400

    Finally a clang build of the Csound for Android app. Had to change minSdkVersion and APP_PLATFORM to android-21 for paulstretch.

commit 9f4c7c5a53e71ed22ac8c654d6f531c0c14486bc
Merge: 87f2826 59a27a4
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Sep 15 11:27:29 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 87f28269df3a954817b601c1d3bc0d8d7747b0b5
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Sep 15 11:27:16 2016 +0100

    fixed sensekey bug

commit 59a27a43fc80b6aaa69b400d5f2a20987ef91bb7
Author: gogins <michael.gogins@gmail.com>
Date:   Wed Sep 14 14:08:22 2016 -0400

    Trying to build CsForAndroid on 2nd Linux system.

commit 77fc44fee521e3bf3bebfedc843cef8f119f2a64
Merge: 47597c1 c1310a1
Author: gogins <michael.gogins@gmail.com>
Date:   Wed Sep 14 13:37:48 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 47597c188986f3d444de1871283c5bfbbbde3be5
Author: gogins <michael.gogins@gmail.com>
Date:   Wed Sep 14 13:37:29 2016 -0400

    Trying to build on 2nd Linux machine...

commit c1310a1851a7e77189d6ed0750d443b247497bc1
Author: Felipe Sateler <fsateler@gmail.com>
Date:   Mon Sep 12 15:16:53 2016 -0300

    cmake: drop duplicate inclusion of Custom.cmake
    
    Fixes #698

commit 8cc27206c937f2857980e6158e6f645450ab5538
Merge: 8735edd b43a7c8
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Sep 12 16:20:49 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 8735edd9f131ee216b38ab08611ebfc25226b264
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Sep 12 16:20:33 2016 +0100

    added CSOUNDMSG_STDOUT attribute

commit b43a7c889dcbd11f75b1bcaed02f77df28a79007
Author: gogins <michael.gogins@gmail.com>
Date:   Mon Sep 12 11:10:47 2016 -0400

    Clamp a for padsynth_gen.

commit af45f8accd61cfce83f3013d30e6622e8ae2b046
Merge: 4e38fc1 22f6f37
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Sep 12 15:22:21 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 22f6f374cd55773a950bf6673a047083bb7e08df
Author: gogins <michael.gogins@gmail.com>
Date:   Sun Sep 11 21:38:15 2016 -0400

    Trying to fix the 'square' profile.

commit 5b02dc68cae97eb00570f737a16429d97f4cd350
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Sep 11 15:54:16 2016 +0100

    attempt to fix gen41

commit e63e23d2ec71f7cd2b816baf7ff450df37ef7e14
Merge: 301dede 7ce1267
Author: gogins <michael.gogins@gmail.com>
Date:   Sat Sep 10 16:25:32 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 301dedee339be24e5430b2490493149557784817
Author: gogins <michael.gogins@gmail.com>
Date:   Sat Sep 10 16:24:44 2016 -0400

    Changes for Csound 6.08, mostly updated dependencies.

commit 7ce126798d1dd722e26d9627a403a21c2390cc8a
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Sep 10 20:41:41 2016 +0100

    fix to gen31

commit d0ef8e7712c7a7df1c3859b433e23ff78fa2d9b7
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Sep 10 17:50:28 2016 +0100

    quieter padsynth controlled by warn level

commit 484f6aaf8a9dd0f71b292d04cb018b0655f42e75
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Sep 6 16:05:57 2016 -0400

    Fixed bug in padsynth for 'square' profile.

commit 6f1b49bd00047fc024787cd15c01a4e1feb968a1
Merge: c7a91b7 f855fdb
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Sep 6 14:32:43 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit c7a91b717ca9507c0f9a060bc4a756398263deea
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Sep 6 14:31:51 2016 -0400

    Added STK opcodes to Android build. On app startup rawwaves are copied to music/assets and new setEnv() is called.

commit f855fdba9ff21a3f98444c39336308bb2dc05925
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Sep 6 14:36:51 2016 +0100

    quieter tracing in SCORE_PARSER

commit 4e38fc1737569611c8c7693c5966ee6bf4f045cf
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Sep 5 22:50:23 2016 +0100

    chnages

commit ab41ed8fb0e70887e58e91f3bb88c3760b1e6aa5
Merge: 37efc10 4d3eae1
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Sep 5 17:05:24 2016 -0400

    Favor jpff OSC.c.

commit 37efc10595c349b956cd75b625f7ae76d5d89f6c
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Sep 5 16:39:41 2016 -0400

    Undoing prior 2 commits with directory deletes, re-committing restored directories and needed changes.

commit 302173dc67e3b1edb5c92f1d19e3652be9fb7940
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Sep 5 11:00:16 2016 -0400

    Revert "Changes to enable building Csound for Android on Linux."
    
    This reverts commit df36b33e2e6fd918e19164c2cb8d04123ab97223.

commit 6a13db2851022fdfd27f77dc9a6dc91261df909c
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Sep 5 10:59:48 2016 -0400

    Revert "Added Cs icon to be like Play store."
    
    This reverts commit 5b32d3f72df83eeecf6077ea256e6153473065af.

commit 4d3eae14ad1206e0abdcd5148168c67912772749
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Sep 4 21:40:24 2016 +0100

    tidy up; ksmps_override

commit 5b32d3f72df83eeecf6077ea256e6153473065af
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Sep 4 14:50:41 2016 -0400

    Added Cs icon to be like Play store.

commit 4019bf70ce39c359e767901da300053f48db6165
Merge: df36b33 96e0a7d
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Sep 3 12:44:27 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit df36b33e2e6fd918e19164c2cb8d04123ab97223
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Sep 3 12:07:16 2016 -0400

    Changes to enable building Csound for Android on Linux.

commit 96e0a7d410d9c2a3d5538bc9e7dfeaf098eaacd2
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Sat Sep 3 10:39:47 2016 +0200

    Updated ctcsound.py

commit 5d10796c6af8b9703d9f4ad27464d174a01194f8
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Sep 2 15:18:47 2016 +0100

    betr A4 use -- no works

commit 9f545b1b7e7b5c3587ec44722192e65a7b1d2eb3
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Sep 1 11:32:32 2016 +0100

    introduce A4 r-variabe

commit dddf71b867cf90c684b84c20e1d8d667165c65de
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Aug 31 19:20:38 2016 +0100

    make 440 a variable

commit df47cffa3a31c2fbfadc3cf070079ea1826f96bc
Merge: 7d62dd7 af6b1b7
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Aug 29 21:53:51 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 7d62dd7aa8f90079760fa421a25414e4b0271685
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Aug 29 21:53:36 2016 +0100

    another step to the new score reader

commit af6b1b76a949955df7de5a9bb926e9e7b1b9618c
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Aug 29 11:42:09 2016 -0400

    Coverity housekeeping.

commit 8b058b6dd54cc972cc563e8af38a2c85c176161a
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Aug 28 21:55:40 2016 +0100

    copying of OPARMS

commit 8d2e090330fb66fb0108d73d0b2943db13e73ccc
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Aug 28 13:07:47 2016 +0100

    a couple f small coverity fixes

commit 08ec473baf6c7e71cea3c643071d1e5bcb993cf7
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Aug 28 12:17:25 2016 +0100

    better patch for ETX

commit 30e1ce0eceabc713d3ba81ba14b1957897445b38
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Aug 27 16:50:33 2016 +0100

    patch round ETX in strings

commit bc2326196f5e2d5a747c6732e2985131c530681c
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Aug 26 21:35:59 2016 +0100

    sample-accurate assigment issue

commit 0b5c71e562819786a2a36b1579b82af2107a3920
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu Aug 25 12:55:29 2016 -0400

    added free to avoid resource leakage [coverity #132132]

commit fb15d20b01ceabbb1a10723fdf514ddd9c71fcd5
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Aug 25 10:35:36 2016 +0100

    more coverity fixes

commit 2a7836ce5eaaa8386580d6c98513a1be81307c50
Merge: 8d439df 04fe997
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Aug 24 20:37:11 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 8d439df9352d602e7f8220d967f229bb805d39d3
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Aug 24 20:25:37 2016 +0100

    coverity fixes and sample-accurate issue

commit 04fe997e220b9d734b4e7dd195e178c4085108f0
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Aug 24 11:48:14 2016 -0700

    close resource before return to avoid leak [coverity #152302]

commit fb0d4eabfd6862c6813190ea0877821f9e33472b
Merge: 25395a0 cd9e773
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Aug 24 10:16:49 2016 -0700

    Merge branch 'develop' of github.com:/csound/csound into develop

commit 25395a02ae102c7e3d3be635d2947149c1d1ce19
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Aug 24 10:15:50 2016 -0700

    removed unnecessary NULL checks as values are dereferenced earlier
    [coverity #152299, #152301]

commit 29cc9097164cdee20be07903905debbed17f6dca
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Aug 24 10:15:43 2016 -0700

    updated test.py to work when running within mingw/msys2

commit cd9e773347816d77eebbc178430a05c44e9301e4
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Aug 24 16:21:17 2016 +0100

    various coverity

commit 13ad13de7fdc02ab81b6c4e0544c42984ab2d908
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Aug 24 15:37:47 2016 +0100

    coverity fix

commit 4dce10bdcc15b4a05c356d8453bd95ec7f78b961
Merge: c08dcda b013b6f
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Aug 24 15:36:26 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit c08dcda4d82e71f0b73d22f928c356ae4fa5e597
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Aug 24 15:36:13 2016 +0100

    coverity fix

commit b013b6faf1b91d3cd917bd67af639420a992b8cd
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Tue Aug 23 22:26:59 2016 +0200

    Updated French translation

commit acba87ec6f2f14862503f0feb90e7e4b6ced01ee
Merge: 1054018 77cc07f
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Tue Aug 23 20:56:27 2016 +0200

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 105401834bf44d7497f200bb72432623f00792ce
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Tue Aug 23 20:55:52 2016 +0200

    Updated French translation

commit 77cc07f8f46059ba85873b3376f213204856655b
Merge: 3628c0c b1e86f5
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Aug 23 19:46:41 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 3628c0cb581e1a5e11133e23c1b0c50bcfa696a4
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Aug 23 19:46:25 2016 +0100

    minor

commit b1e86f58bed3f89972717856d345bee18b7860d8
Merge: 20e2ca9 a054415
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Aug 23 08:28:49 2016 -0700

    Merge branch 'develop' of github.com:/csound/csound into develop

commit 20e2ca9481b5b3024167d989dc7997fbe976b44d
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Aug 23 08:28:41 2016 -0700

    modified code to avoid double assignment [coverity #152267]

commit a0544150585677fa020ff23a3bf28b46711c96a0
Author: fggp <fggpinot@gmail.com>
Date:   Tue Aug 23 16:07:03 2016 +0200

    Added message level getter to ctcsound.py

commit ba3cf3dfe08cda9ea006e3e98c632aeb6a6f701a
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Aug 23 06:20:18 2016 -0700

    fix resource leak [coverity #27363]

commit 3a58dfdb683d3c4d878e150351b83a922018b321
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Aug 23 06:08:12 2016 -0700

    close file handle before return to avoid resource leak [coverity #152302]

commit 2cf93bbc8baa61d2c85317f63b22838443b4262a
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Aug 22 12:08:46 2016 +0100

    better fix to deep nested macros/includes

commit ca32a982725815e2e2a90f240ffe3f1fd54fba56
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Aug 19 21:55:46 2016 +0100

    memory checks in scomacros

commit 75c3277a81c60643e19c38844ede0c999fc764a2
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Aug 19 17:44:38 2016 +0100

    memory checks in orcmacros

commit 848d4dd292185f6a719c3fb4c1096c81337407e2
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Aug 18 20:03:42 2016 +0100

    Add fterror to API

commit 84a5f3027f8b6972016d1c77a9155c70d9e31921
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Aug 18 10:45:49 2016 +0100

    Added getftargs opcode

commit 87b351c006d1ca3fedafa1c702c8756df994418b
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Aug 18 10:34:47 2016 +0100

    Added getftargs opcode

commit 6c1ec007220bbb0533f018c43abf145764f88678
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Aug 18 10:20:05 2016 +0100

    Added quadbezier gen

commit 1641c46488efdbc58fb2f45a962c4dba1799e9fc
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Aug 18 10:15:35 2016 +0100

    Added quadbezier gen

commit 75fc2952290ca2581693a48d1c8d5bc0da55adf4
Merge: 7e6ea95 7653c7e
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Aug 16 12:55:46 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 7653c7e79adc63328a2235b1a56a41315dab149d
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Aug 15 22:30:08 2016 -0400

    No comment.

commit 7e6ea9523e3e3bf82e5cddda369cabb4718f9141
Merge: 0a4ee64 00cdb4b
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Aug 7 17:00:11 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 00cdb4b19af3372b5b59194696449bc80224cbc9
Merge: 970f28f 9c7fa02
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Aug 6 07:16:00 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 970f28f0ade03647d5e80425a33f772d6275b84a
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Aug 6 07:15:42 2016 -0400

    Conditional loading of csound library for sb-alien.

commit 9c7fa0294f91386d2f228652b86c301c1db36c4c
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Fri Aug 5 14:57:34 2016 -0400

    Update README.md

commit 0a4ee64fd26e6221cf2db8d72a22de05931e4fe4
Merge: 6a59cbc c609994
Author: jpff <jpff@codemist.co.uk>
Date:   Thu Aug 4 13:08:57 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit c609994ab214ab21c5b4cbe6c0111c1128614eab
Merge: f2bd4f9 12173ec
Author: gogins <michael.gogins@gmail.com>
Date:   Wed Aug 3 19:00:24 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit f2bd4f9fafc7abdb0fca875e2b2e75d3af41c675
Author: gogins <michael.gogins@gmail.com>
Date:   Wed Aug 3 19:00:14 2016 -0400

    pvlook now always prints explicit analysis window name.

commit 6a59cbcb2edfb1d30c2de5b33fcf0e178a107fe7
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Aug 3 13:06:29 2016 +0100

    fix ReadScore

commit 12173ecb71ec6d1a12a67a93292e19f30bf1e693
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Aug 3 12:34:52 2016 +0100

    save work on new score lexer

commit c0534ac4dd3e15a93548426911281b43790f2489
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Aug 1 08:55:54 2016 -0400

    Update README.md
    
    Fixed broken references to resources.

commit 8996a80c2db8a1815fde13ceba9a80bcd95de1fc
Merge: 017dc5a 8b1355a
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 28 17:17:07 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 017dc5abd5534583c7e857bf6f7324e17b93f053
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 28 17:16:26 2016 -0400

    Refinements for Lisp wrapper and examples, enables running in separate thread now, performances ends when CSD ends.

commit 8b1355a4f1dbe8e3feae9fdfc3d03b438e307d20
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Jul 27 17:46:58 2016 -0400

    defensive programming to prevent segfaults due to NULL pointers

commit 7389b023dd59ab53117fb63704931b463220653c
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Jul 27 18:16:10 2016 +0100

    fix termination error in printing

commit 452bfce7450a6ca772ec537d5a06ac78e4cfe79b
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Jul 27 12:29:59 2016 +0100

    small error in pvsceps

commit a5a24a20445d90417a6d96b6c1ecb8fbed98fe98
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jul 26 19:03:35 2016 -0400

    Minor changes to scales example.

commit 7f8fdce957e076d6afca4c3855dc3028baec1c3b
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Jul 24 10:59:44 2016 -0400

    Update README.md

commit 4d1d01a814ec5a705ce8b5a520ce46554129475a
Merge: 7ccd14e 6e28971
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Jul 23 20:40:05 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 7ccd14ec735d676b7d9386513d0ec66baa4508bf
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Jul 23 20:39:36 2016 +0100

    revert treatment of \r in corfiles

commit 6e28971bbbac6c1394fe266d45f464721b7e3ca2
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jul 23 14:28:50 2016 -0400

    Update README.md

commit e2d84700c1683cb184a9bb685fa1a15c50e4f433
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jul 23 14:27:55 2016 -0400

    Update README.md

commit b48415089ab4ce529778814b14f1641b35ffca41
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jul 23 14:27:16 2016 -0400

    Update README.md
    
    sb-alien only for now.

commit ade45968d9e76cd5994e9d4c414d55e03d005043
Merge: 3e2bfae c31b97d
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jul 23 14:26:11 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 3e2bfae8c1a83483c1e996463d00162c664fde24
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jul 23 14:25:53 2016 -0400

    Updated Lisp examples for sb-alien.

commit c31b97dd991665db6f8e6f1037cc3c1818219e68
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jul 23 14:21:15 2016 -0400

    Update README.md

commit ff3821cf4ca39bc9b8583c12d0042497fff16a5c
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jul 23 13:06:59 2016 -0400

    Update README.md

commit 66f3fc3f627b9a9064e77ae7c05686f6a7f451ca
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Fri Jul 22 20:47:58 2016 -0400

    Fully working Lisp integration for Steel Bank Common Lisp.

commit 73c26f6beb2225035ba826027d8ffbcaa89c8e06
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Jul 22 18:33:09 2016 +0100

    restre mac line endings

commit b138e2aeefc5c67e7a36a030bc51a38e86dbc20f
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jul 20 21:06:34 2016 -0400

    sb-alien versions of Lisp stuff that actually work in the Catskills.

commit ab5d85f255011182ca63f2c067bf8a1028f824bd
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sun Jul 17 20:47:48 2016 -0400

    Ensure VECLIB_PATH is set to default /System/Library/Framworks if not set by user

commit d82cc071b4cb1fee8aa029664c4a15f50f4d7010
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Jul 17 21:07:36 2016 +0100

    Various small changes

commit c5db5c0d8d3958571d1957062390631144c4ce31
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jul 17 10:13:07 2016 +0100

    protected for windows

commit 2c35eee84980975a417008cb287b60c24736644e
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jul 17 10:01:48 2016 +0100

    typo fix

commit 56ee3e8a281cef3a929404f0dac3b589fc4c70d1
Merge: b818e3a 80e7f20
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jul 17 09:54:44 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit b818e3a20f76eb9a408ebc7669111f832762ce98
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jul 17 09:53:48 2016 +0100

    added check for directory

commit 80e7f203cc5e4d45af1714388702c4b567a7472f
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Jul 16 23:53:04 2016 +0100

    quieer

commit 230564c6382b4ebb1c159e124dfeeb19735d3db1
Merge: 8613ba8 1e0a045
Author: gogins <michael.gogins@gmail.com>
Date:   Thu Jul 14 21:33:19 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 8613ba81a0b01231cd635e1b3400bf8dbe069e34
Author: gogins <michael.gogins@gmail.com>
Date:   Thu Jul 14 21:33:11 2016 -0400

    Beginning direct Csound rendering.

commit 1e0a04593a8203a371d556f77106c649978cf2fe
Author: gogins <michael.gogins@gmail.com>
Date:   Thu Jul 14 21:10:25 2016 -0400

    Updates.

commit dc4bfe96e8f667693d624ce4c785edfc82d01681
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 14 14:43:46 2016 -0400

    Update README.md

commit 260137f89e594b5141ae6691dfc91ce1312bdc7c
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 14 11:18:28 2016 -0400

    Update README.md

commit f7956665fb8a41ff4d775d9f8970af4902bfae30
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 14 10:34:02 2016 -0400

    Update README.md

commit 95d64d5515bb1b0aefe4ab6a86570cfc63024310
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 14 10:31:44 2016 -0400

    Update README.md

commit cd60fe0b131fd9a69f974944886be21b9c1d7c01
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 14 09:40:21 2016 -0400

    Update README.md

commit 40e1cef1d2af947a2d636431bb8e1c5d1af1e033
Merge: bc5869d e2746bc
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Jul 13 20:05:59 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit bc5869dad5c4242a172e51dc3e91bb7b92ad08a4
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Jul 13 20:05:41 2016 +0100

    stupid error in bformdec1 array type 4

commit e2746bcf0d306e0eb5993246966f7503f3400867
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jul 13 10:31:06 2016 -0400

    Update README.md

commit 92c3574d4d3aae2f46f856cded19dcc6413f5228
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jul 13 10:27:06 2016 -0400

    Update README.md

commit 5e86d09692e6e6e9ec887fbcce050aaddeb7ebb5
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jul 12 21:13:51 2016 -0400

    More explicit and reliable Lisp examples.

commit 1bf9a53268054d728f2d077a9338934948e5f61e
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Jul 12 14:35:52 2016 +0100

    patch to EQ part of oscbnk

commit d28f61580d8c5e813dd52a7ec941754e2767ffda
Merge: 3f99b72 71b3a6b
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Jul 11 09:00:50 2016 -0400

    Added asd file for csound.lisp wrapper.

commit 3f99b72b8956918d95af599a317d60c8fe9c68be
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Jul 11 08:58:22 2016 -0400

    Added asd file for csound.lisp wrapper.

commit 71b3a6b387c3dd62946b6211ade1741db557ee67
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Jul 11 08:55:26 2016 -0400

    Update README.md

commit 35a4e00895d90f24b707bd3af6b96c1f0d807ad1
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Jul 10 12:07:54 2016 -0400

    Update README.md

commit 9715acf95f53f900d06ccda408ae4fc28e346a38
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jul 9 07:41:41 2016 -0400

    Update README.md

commit 10d856f4ec93025aab4bb7364315b3062089b424
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Fri Jul 8 20:09:54 2016 -0400

    Update README.md

commit 9785a118e425e1e67236c1dd9c7817acf0f83c11
Merge: 3746717 396eef1
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Jul 8 16:49:35 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 37467173e9197e449a7f7bb2abd7cb73b23b3bd4
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Jul 8 16:49:29 2016 +0100

    version updated

commit 396eef1f336c32d468b9c593cd3411d478216943
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 7 21:07:22 2016 -0400

    Update README.md

commit b538bd36e725ee9d14bef58b7ef3162ba3548b57
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 7 12:33:56 2016 -0400

    Update README.md

commit e55933aab68706b25a416b33df9884bde70d8098
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 7 12:08:51 2016 -0400

    Update README.md

commit 03904ee6bb1586e36134d6db6bd28fab8d5ef0d1
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 7 12:06:24 2016 -0400

    Update README.md

commit d7c8a384c763c944a5577ac7b125c6a9fbb56943
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 7 11:23:26 2016 -0400

    Update README.md

commit 760d1b4efdbee00d16d7f038c712dee3d5a2521e
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 7 11:02:36 2016 -0400

    Update README.md

commit 86be9828f7ede669695cc6f078c24b27de7c3708
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 7 10:47:11 2016 -0400

    Update README.md

commit 44e42e3f94a5c5f563dc0dcea8ba2a1678e30acf
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jul 7 10:42:04 2016 -0400

    Update README.md

commit e123547e2940968cb57c998b61f2f3440224c384
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jul 6 20:06:44 2016 -0400

    Update README.md

commit 5752ed32dcb8a31e05e5ac78074acda6a882dcb9
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jul 6 18:37:44 2016 -0400

    Update README.md

commit 008459f691cf708571f75826d02d49f1934d2836
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jul 6 18:30:23 2016 -0400

    Update README.md

commit a4e63810bfc4d45dbea705302fd296ceb615abf4
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jul 6 18:22:50 2016 -0400

    Update README.md

commit b8ac3e357d2cf2686a24a6f4611223878ef06f7a
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jul 6 17:46:18 2016 -0400

    Create README.md

commit 951500078a7b078e1c0698f9d91536b04ae1e537
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jul 6 17:00:54 2016 -0400

    Added example using wrapper.

commit 15aa743d82d958ede3b09a53c37f6515dced296e
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jul 6 14:44:30 2016 -0400

    Added some stuff to the Lisp interface.

commit 7845874bc3bd590723d68254b88c1d37cfcadba3
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jul 5 19:10:19 2016 -0400

    Fixed paths.

commit 7015638b7f73e7fa484209eb2b745300f21f6137
Merge: ddfbd5c 1f79f78
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jul 5 14:43:07 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit ddfbd5c977bbecaa5c9287b9a970f835fe14a354
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jul 5 14:42:49 2016 -0400

    Add some C++ plugin opcodes to the PNaCl build.

commit 1f79f78a4d11f78c343b2dbe734589ffe53709ad
Merge: 76c99b0 45e40c4
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Jul 3 21:23:50 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 76c99b0cc4a37e552f2ff2c5d3461dcf0267e275
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Jul 3 21:23:39 2016 +0100

    enhanced jitter2

commit 45e40c4a1a151a41cce2c68746288f909dde9c61
Merge: 4fbf522 46a0a99
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Jul 3 13:06:22 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 4fbf5222ff23cc4cf6e4109ac6cdf2ca0c228b8d
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sun Jul 3 13:05:49 2016 -0400

    PNaCl: Implemented playing and rendering CSD text. Lisp: Updated example and CFFI binding.

commit 46a0a99bf4b497ab36d9ff1cc0f60db270fd9f62
Author: Felipe Sateler <fsateler@gmail.com>
Date:   Sun Jun 19 23:31:31 2016 -0400

    travis: Use the trusty environment
    
    No need to update gcc and friends anymore

commit 78a9f6296b9c5849f24980a8b0f948d992b57e2e
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Fri Jul 1 13:14:43 2016 -0400

    Fixed bug in message.

commit e26034f26810c654e0251347e48e2150c6ef8ffb
Merge: 65206cb d670204
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Jul 1 16:51:30 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 65206cba6f2fbb834186d11c8a78e5444ffe74bc
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Jul 1 16:51:22 2016 +0100

    ftgen reading from array

commit d67020445e810719fcaa5428e357bb336c660b21
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu Jun 30 17:31:26 2016 -0400

    added check after splitArgs() call to prevent further processing and segfault [fixes #671]

commit c1614d1509c5b5a012f03272df3f95ab8c51f540
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Jun 29 14:27:14 2016 -0400

    added error handling for unmatched brackets for UDO arg specification [issue #671]

commit 30aea7837ad6c53e35642426cac4a002c4b21c29
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed Jun 29 12:21:39 2016 -0400

    Fixed logic bugs in atsa, added method to nacl.

commit c4a8dbb5d7b0077306373ccf1c373728d067308d
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jun 26 16:53:59 2016 +0100

    lexing fix

commit 7923c1f18b4b80f2394e53a9b425fdc476e99506
Merge: 050f33a 83b2c3a
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jun 26 10:57:56 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 050f33ab42c5cf415164bc81ba8cc224dd22833e
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jun 26 10:57:37 2016 +0100

    fixed issue on opcode arg check and removed old obsolete code

commit 83b2c3a8d277d51d21547454afafd26be1f94651
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Sat Jun 25 09:39:49 2016 +0200

    Updated French translation

commit 57a58e07f5341994d83106db9c73975f67bb1c0d
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jun 23 20:56:22 2016 -0400

    Update README.md

commit de5be64810cd5ebf882074d217b15e71f5a7a0da
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jun 23 10:47:26 2016 -0400

    Update README.md

commit 659656c16e53cc08643cf89f62fa5779c0ece04a
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Jun 23 10:43:34 2016 -0400

    Update README.md

commit 8008f64a4b0495af3c92921afdc58c066e9b206c
Merge: 6825f2c 92382f3
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Jun 23 10:55:58 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 6825f2c28150b6cd827a9dd2027289783ad7ad90
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Jun 23 10:55:47 2016 +0100

    prints fix

commit 92382f396de801fe1f004f2fef25f053604b2a76
Author: fggp <fggpinot@gmail.com>
Date:   Thu Jun 23 10:58:12 2016 +0200

    Added new named GEN functions

commit 44eb16202e4c4e8dcd50b7b50a491974878bbf35
Merge: 22828fb 944341d
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jun 21 14:55:52 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 22828fb347a0997ad731155ba37892b7bdf6b2ff
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jun 21 14:55:27 2016 -0400

    Updated csound.node for Linux. Works!

commit 944341dd09e9f50bf52ec10e6011b9180c4a3243
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jun 21 14:53:16 2016 -0400

    Update README.md

commit 7c6c824976090ecae4dcf5918ad0b9235fb693a7
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jun 21 10:01:56 2016 -0400

    Update README.md

commit 4d7ceeaf35be040619a2ec4541def74a8baeff32
Merge: 19f733e ce3ceab
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jun 21 09:45:57 2016 -0400

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 19f733ed48a870496862d9d1c2a219882cb8111f
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Tue Jun 21 09:43:54 2016 -0400

    Updated csound.node for Linux build.

commit ce3ceab6b7783b3d1fe569f59033210b587f4667
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Jun 21 14:35:23 2016 +0100

    checks on number of macro args

commit a11face6d2be3f204d15502e4bf7e42b6187a6ce
Merge: 5a99081 a203227
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Jun 20 20:41:10 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 5a99081c64f9cf243744902663157f58da83da2b
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Jun 20 20:39:48 2016 +0100

    fix to make_label

commit a2032277c3b275b3aafe405ae8ba860c6f3f30ef
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Jun 20 19:27:37 2016 +0100

    fixed named fgen code

commit c442299341372b6a42c397c5b42f8859189a4bf5
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Jun 20 11:54:31 2016 +0100

    named gen api suppoprt

commit 9962cea738af377468652b7fb6876e3e4c4a1e3c
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Jun 19 17:13:25 2016 +0100

    Attempt to tidy dependencies

commit 6003dd762a46f86094f663bc15a44e65a32b5e19
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Jun 19 16:32:06 2016 +0100

    Fix comments in scsore +better orc pre-lexing

commit 01928e8be891011e59595be188c734d2a2165bb4
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jun 19 12:12:13 2016 +0100

    fix for semantic confusion between labels and opcode return typing

commit 3a155d562a2c66bef718e6b30bc6fdb08eced28f
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Jun 18 17:04:56 2016 +0100

    better checking of orc macros

commit 58d4abfeea9731a5ce218331a994d11959c7c8e0
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Jun 18 15:55:37 2016 +0100

    Different lexing of strings and file rename

commit 8a0ee46a2dbdd0a03865f2deb36d401d7d3bbb6b
Author: jpff <jpff@codemist.co.uk>
Date:   Fri Jun 17 17:44:43 2016 +0100

    remove bad string pattern

commit 2a7e642e82478b94495524ef334865518045a869
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Jun 15 12:17:46 2016 +0100

    fix memory braino

commit bb9279244a48046eeefc96dd7a237b2197c06802
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Jun 15 11:10:41 2016 +0100

    multicast send and optimisation?

commit 7448e32abe4bc2d173a7edd5d1bc39c038d2ae49
Merge: e373ac9 121526f
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Jun 15 10:23:38 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit e373ac9c0268a86a0bbc0950d5e8c6bbd7e9a51a
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Jun 15 10:23:30 2016 +0100

    OSC multicast listener working

commit 121526fae54e07426e3ec669dcc32821a4412a22
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Jun 15 10:16:35 2016 +0100

    comment on OSC.c

commit 2d3f88a14cf9260c5fb8aba81410820c201b3565
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Jun 14 20:22:04 2016 +0100

    More on experimentl OSCinit<

commit 5edbedbc99bedd1643e51bc127d067d9c928670f
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Jun 14 20:20:43 2016 +0100

    Fix to continuations i strings

commit b6e8c6c6b5d23b1024c3f20326d91ff5e82fa1a4
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Jun 14 20:07:45 2016 +0100

    Experimental multicast  OSC listener

commit 5e9cccc7c7d45cdde705263735d5c275092f86ad
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Jun 13 17:30:54 2016 +0100

    #sline in strings

commit a2adbccac16e11062236f6dcdf982f4d289800f2
Author: jpff <jpff@codemist.co.uk>
Date:   Mon Jun 13 15:10:13 2016 +0100

    deal with ¬in lexing

commit 873dab75ea8cac7b267e8ea4eec5b57cdafe58bf
Merge: 3fc9465 9f53f3c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Jun 13 09:35:45 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 3fc94654052800bd2ca0d4df522c9bd57049cc83
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon Jun 13 09:35:33 2016 +0100

    trying to fix issue 653

commit 9f53f3c810e6c8d523fc79d0f8f51a46f08c3016
Author: jpff <jpff@codemist.co.uk>
Date:   Sun Jun 12 15:34:00 2016 +0100

    fix to #undef

commit 0764125d55cacd37d6b9f6b3e1834c25c67a5c64
Author: jpff <jpff@codemist.co.uk>
Date:   Sat Jun 11 20:25:33 2016 +0100

    police names of macros

commit 7950249add24373ec29595c190a49c930c7c6d24
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Jun 7 19:49:46 2016 +0100

    trap stray characters in lexer to error

commit da821b2a3edfb6e1a0b42e8c25ac25a006176a92
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Jun 7 17:44:30 2016 +0100

    trap stray characters in lexer

commit f7274b8597678fb042027e4f09bafee7ec562494
Author: jpff <jpff@codemist.co.uk>
Date:   Tue Jun 7 14:07:02 2016 +0100

    layout

commit ef8d1942c917503c5a561e33d2e3498724da9e6e
Author: gogins <michael.gogins@gmail.com>
Date:   Sun Jun 5 14:27:54 2016 -0400

    Restored from stash.

commit 66e7e08537ca6ae037ea55c0f12a75ba8d1f4e64
Merge: 770940c 9e86a7f
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Jun 2 09:37:37 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 770940c588e1430825db4c0d9c780d36fde84342
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu Jun 2 09:37:14 2016 +0100

    iOS build fixes

commit 9e86a7f5aa9ad6625e2f46e18c551bacd186f7e6
Author: jpff <jpff@codemist.co.uk>
Date:   Wed Jun 1 17:53:53 2016 +0100

    Another line number fix (fencepost)

commit ea9df0117bed4bea63f3b735e691e61b40994b18
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Jun 1 08:41:14 2016 +0100

    editing rpi instructions

commit 72fd061374300fb560d03af22a94306e1a997073
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Jun 1 08:39:57 2016 +0100

    editing rpi instructions

commit 713158ab467dd33cd8883a57e8e4adb07dcdcc6c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Jun 1 08:39:08 2016 +0100

    editing rpi instructions

commit 484af3e67140be7894b4ac8ca609edc658a23a56
Merge: ded4099 d5c0efb
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Jun 1 08:35:43 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit ded4099e27da4a0b21bfc6ed098c0122186c02aa
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Jun 1 08:35:34 2016 +0100

    editing rpi instructions

commit d5c0efb9c8161a101532fad4176fd0e7f9b58856
Author: jpff <jpff@codemist.co.uk>
Date:   Tue May 31 16:22:59 2016 +0100

    rounding errors in printks

commit b17d54e62e7bfea2a08b805ca23b835e6bffadb3
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Sat May 28 22:32:15 2016 +0200

    Updated French translation

commit f330cd3df8ebfc4895ccaf3f00cad1411320bdba
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri May 27 19:44:30 2016 +0100

    array centroid

commit 3d18a7b4bee21e8a241922b2a8243df3cd50e2b2
Merge: 3fc3533 5cc15e8
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri May 27 18:31:24 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 3fc35339be7a64c92e813ad6ac1219bb3f0369d5
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri May 27 18:30:55 2016 +0100

    centroid fixes

commit 5cc15e82bfc2ba285eda0a436b69ca479c7dd61c
Author: jpff <jpff@codemist.co.uk>
Date:   Fri May 27 18:02:56 2016 +0100

    parser error fix

commit 6631eef4eae3c9b09d2f891197ff440cc0f9a83f
Author: jpff <jpff@codemist.co.uk>
Date:   Fri May 27 17:57:57 2016 +0100

    parser error fx

commit 4ad0bcf18bc552070a9a612e4545f5679bfe6f7a
Merge: 73e9568 15f9f89
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri May 27 11:37:50 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 73e95685d0ae8f08ca59f589fc891a45583a7309
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri May 27 11:37:31 2016 +0100

    dct implemented on 3 libs

commit 15f9f8936887d0eb6f8a8778a061d46e90f0bad9
Merge: d6bd193 019325c
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu May 26 16:47:32 2016 -0400

    Merge branch 'develop' of github.com:/csound/csound into develop

commit d6bd1938bf1a890ba438e37a6e0a7a5d2971658f
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu May 26 16:47:03 2016 -0400

    removed search for pthread.h as it broke the msys2 build

commit 019325ce9505acfb40749e3d8d190910d09585a5
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu May 26 19:34:28 2016 +0100

    fix to mfb

commit 8f56994e221729b1fcbc84d72789793fad0cc5ac
Merge: d181bc3 301c57a
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu May 26 17:53:08 2016 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit d181bc307d3aefbb86cf4e7ef18d7966904bd25c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Thu May 26 17:52:57 2016 +0100

    added mfb

commit 301c57a23dcf8f211eb8d1dd5d1475e9a0453d43
Author: gogins <michael.gogins@gmail.com>
Date:   Thu May 26 10:32:08 2016 -0400

    Use implicit type cast instead of potentially problematic (for 64 bit int) explicit cast.

commit 4b9fdf23091acc7c2664f9b7741cf7325cd9a969
Merge: a4da123 3089385
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Thu May 26 07:55:49 2016 +0100

    Merge pull request #644 from Ebmtranceboy/patch-2
    
    Update BUILD.md

commit 308938530d2e823d1e91d1b8b3041cd2383eb996
Author: Ebmtranceboy <ebmtranceboy@gmail.com>
Date:   Thu May 26 02:10:05 2016 +0200

    Update BUILD.md
    
    added a note about Raspian Jessie

commit a4da12373204d728809e3c9a6467987de0d98b73
Merge: dd7e54a 79cbc9e
Author: jpff <jpff@codemist.co.uk>
Date:   Wed May 25 16:22:22 2016 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit dd7e54a17792131e985dc711b1d666ccbc0cb985
Author: jpff <jpff@codemist.co.uk>
Date:   Wed May 25 16:22:06 2016 +0100

    strings

commit 79cbc9e3df57102dd899f45a7ee9a2a2e66bd97d
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Wed May 25 11:20:11 2016 -0400

    Update README.md

commit 163e2a7cff24beac4072ac9750225020b695c48a
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed May 25 10:04:22 2016 +0100

    fixing merge of mp3in

commit f3db2bfec36cb3f1f7ecc1103a2181e5da532b1a
Merge: c2dc9e6 81de598
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed May 25 10:02:31 2016 +0100

    merge fix

commit c2dc9e66ca88cc4e52efdd597ef22a31d044a02c
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed May 25 09:57:13 2016 +0100

    added dct opcodes

commit 81de598a048706b7de6f8a86f6147feee7455ffb
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sun May 22 12:11:19 2016 -0400

    add find and inclusion of pthread.h path

commit 643640d701c434298c3254f69b890fcbb2ceab5f
Author: jpff <jpff@codemist.co.uk>
Date:   Wed May 18 19:41:59 2016 +0100

    record new file + spelling

commit a0da6558da0c49b30c5612b0e5108b78860ddc2f
Author: jpff <jpff@codemist.co.uk>
Date:   Tue May 17 13:33:43 2016 +0100

    removing tabs

commit 33e536065ffeb4621c51e31714446cdd5925619d
Merge: 266abf8 2645994
Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon May 16 17:33:38 2016 -0400

    Merge tag '6.07.0' into develop
    
    6.07.0

commit 266abf89d319b69b3139349855be3ba912f9de1b
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon May 16 20:05:45 2016 +0100

    some additions  to installer build script OSX

commit 471e11fa7f0c65e09b0842d97fd3a263e94bb5ba
Merge: 515e67f 4a170b6
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon May 16 19:32:00 2016 +0100

    fixed conflict

commit 515e67fcaab05a5c167bc1d4e63123283020d8fc
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Mon May 16 19:30:58 2016 +0100

    skipping in mp3

commit 4a170b6e822c52adf741e37a85c1bd24d10ff175
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Sun May 15 22:23:16 2016 +0200

    Updated French translation

commit 827165ecaddbfab4eca6a5e180358e2b0935c02d
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sun May 15 16:18:13 2016 -0400

    Revert "simplified finding of pthread library"
    
    This reverts commit 831b81f897377895bd50236dc8038e16fe338773.
```