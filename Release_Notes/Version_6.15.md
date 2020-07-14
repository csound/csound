<!---

To maintain this document use the following markdown:

# First level heading
## Second level heading
### Third level heading

- First level bullet point
 - Second level bullet point
  - Third level bullet point

`inline code`

``` pre-formatted text etc.  ```

[hyperlink](url for the hyperlink)

Any valid HTML can also be used.

 --->

# CSOUND VERSION 6.15 RELEASE NOTES

-- The Developers

## USER-LEVEL CHANGES

### New opcodes


### New Gen and Macros

### Orchestra


### Score

  
### Options


### Modified Opcodes and Gens


### Utilities


### Frontends

- Belacsound:

- CsoundQt:


### General Usage


## Bugs Fixed



# SYSTEM LEVEL CHANGES

-

### System Changes


### Translations

### API


### Platform Specific

- WebAudio: 

- iOS

- Android

- Windows

- MacOS

- GNU/Linux

- Haiku port

- Bela


==END==
commit 1e64bf60c65cd0bf63e2b6e05f019cfeacaca9ee
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Jul 14 16:10:50 2020 +0100

    issue #1300

commit a2806d6af1b7acff488979c94b4d452349bf16e4
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Jul 14 15:45:48 2020 +0100

    issue #1349

commit 55f588a9e6f1c766e63ada1e31b5066b8284a2c9
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Jul 14 13:09:42 2020 +0100

    issue #1232

commit a0ad9bccb06712c45edd9a7bcc0d5b939d9d158e
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Jul 14 12:56:09 2020 +0100

    fixed #1348


    issue #1358

commit c1811a1dc3fd1e2d1f8e820f0b24c1ca897e9d2d
Merge: 262c76ac0 cdc66607d
Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon Jul 13 17:09:19 2020 -0400

    make lastcycle add an extra k-cycle to make sure that it's not skipped; this only happens if there is no other time extending opcode; it is still possible to skip it if the instr is turned off without release (like turnoff or turnoff2, _, 0

commit 13f57b2a6372364fbed00ee966be1724382735c6
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Jul 13 19:11:48 2020 +0200

    fix ftset init only version

commit 262c76ac0dcb9caf998a79dd031dc9f20b6bdd4b
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Jul 11 19:18:20 2020 -0400

    restored not installing of python interface on macOS; not sure why this was done previously but this should fix Homebrew

commit 8d1898a8d7bb0cdfe0317691fe16f73382795a51
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Jul 11 09:50:39 2020 -0400

    attempting fix for default opcode dir after LIBRARY_INSTALL_DIR change

commit dc49ce04d21e3758e8ae9fbddd4205bf9d2dfefa
Author: Steven Yi <stevenyi@gmail.com>
Date:   Fri Jul 10 13:05:53 2020 -0400

    fixes for install directories to get building on Linux Mint 20

commit fb38881bcd79f643ce57c48bd35ea5a9636d4e4f
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Jul 7 17:46:24 2020 +0100

    updated android makefile

commit a2feda74480294601f219ea77ee1191d2dd81780
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Jul 7 17:33:56 2020 +0100

    made lufs internal

commit a3591d8d182b10121133a6728b01a3c5ba8f54d8
Author: gleb812 <gleb.rogozinsky@gmail.com>
Date:   Tue Jul 7 15:47:13 2020 +0300

    added lufs opcode

commit cc7348ca4862457be5c76b7507647a77579d47e1
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Jul 6 16:44:48 2020 +0100

    B score sstatement

commit 02294d46467022817da3e9c080a584dcaf9aa8c5
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Jul 5 21:47:08 2020 +0100

    updated instructions to include setting release config

commit 7a7170fe7b2f636189b093dfa8b467b5ce4d844d
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Jul 5 21:43:26 2020 +0100

    not hardcoding release

commit 1fd81c2297ac4d5846dfa0c5310e8e9df39cd5ca
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Jul 5 21:31:57 2020 +0100

    improved status messages

commit e99e0002fe258b5ddd687710c2b7949b9fbc08d9
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Jul 5 20:51:26 2020 +0100

    added Release config to crosscompile

commit 4f468fe83c540318cdd95e93e264e6d464d2ba08
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jul 3 21:46:22 2020 +0100

    bumped lpcfilter delay data to doubles

commit d79e32e6a11a0f8b90448f6f617f6c9c466d9fc7
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jul 3 14:46:46 2020 +0100

    fixed ticket 1341

commit 5756ace0bf2b29d708bc214bdbc65650da33193e
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Jul 1 17:21:13 2020 +0100

    adjust #include of a url

commit f56f9ce02000b7a1e385245ddeeb543d84df9a6c
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jun 28 21:49:25 2020 +0100

    bob filter plus many smalltidies

commit c5a27b777fdbb65a5914173c3b95cd94915eddcf
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jun 28 17:33:59 2020 +0100

    Small fix to sterrain

commit 833014296c3140d931606718bed85baec12f68b0
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Sat Jun 27 21:12:14 2020 -0400

    Fixed compilation error in fluidOpcodes.cpp, added two convenience methods in OpcodeBase.hpp.

commit 428c3100191368a537dbf095cc40918f626a4d01
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Jun 27 21:42:31 2020 +0100

    reinstate change to syntax check only

commit fae46b32c929d89b4ccc1126cea4340806882a70
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Jun 27 22:16:58 2020 +0200

    add volatile

commit 75b37e10590600cc02c17d88e8a58b313541614a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Jun 27 20:54:11 2020 +0100

    New option to name score.srt elsewhere

commit c8da310a2a2d0ad8ab24cfccee37439b263d4915
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Jun 27 20:11:40 2020 +0100

    Add tracing to src_conv

commit d9ae5fd77efeaec6c66e6bf9dddcf161c930b70e
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jun 27 18:17:08 2020 +0100

    option

commit 4e4f336336b0b0f43b79bc06f0da1ece4d08774c
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Jun 27 01:44:04 2020 +0200

    make semantics of lastcycle clearer; tied notes with no extra time result in error

commit 48f33c718c857d318c1151c37da094a6e7effd08
Author: Steven Yi <stevenyi@gmail.com>
Date:   Fri Jun 26 17:57:10 2020 -0400

    attempt to fix windows build

commit 819f4329219eba9a04f8b02369e773a739395e92
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jun 26 18:03:11 2020 +0100

    Option --print_version

commit 2197ab56cfe3ee9e934d2a17821266a05479ad58
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Jun 26 06:00:07 2020 +0200

    Let syntax-check-only return an error if syntax failed

commit d8194f628ab7cb5abd75c5070d5b166ed71552f1
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Jun 24 20:11:28 2020 +0100

    super terrain synthesis

commit 1b939b93213bd777963997309fbed163264315fc
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Wed Jun 24 09:44:20 2020 +0200

    Added to ctcsound.py a new API function to load plugins

commit 4fc053bf29637e701bbf2698d721cdee3fcf8b72
Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon Jun 22 12:56:24 2020 -0400

    added ignore for address of packed struct warning given by GCC 9

commit 449e689b275950a22e5f64eb133055100fa1f467
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jun 21 17:44:21 2020 +0100

    Add i-rate counter

commit 77dbd4d6ddae244347c992a551ff1b7487b3c2d0
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jun 20 22:28:54 2020 +0100

    cmdline option to load all plugins from a given dir

commit d7560d1ec8899ee77e2769a81ec20a670885e467
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jun 20 22:19:48 2020 +0100

    added LoadPlugins to c++ interface

commit 1a9b7091ffafa1b8eff271a5e22ce5bafff72457
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jun 20 22:06:40 2020 +0100

    new API function to load plugins

commit e4def76a6723b82cc38fbc9a774aec4117d3d0a2
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Sat Jun 20 21:54:50 2020 +0200

    Updated ctcsound.py with new API func setOpcodedir

commit b6667a35346710cbf798910c024affd5f1405870
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jun 19 23:43:42 2020 +0100

    set opcodedir

commit fa6a68f8b8a7158f623e849b66818def8e8aef06
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jun 19 22:58:16 2020 +0100

    opcodedir experimental feature

commit f5be763f32073d15a281fbc447628c4a4aba15a1
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jun 19 21:48:37 2020 +0100

    More counter opcodes

commit 3b4e4a246a124f5b82ca1d03e9fed60c1b23f313
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jun 19 16:09:45 2020 +0100

    Experimental counter family
                                                <<<<<<<<<<<<HERE<<<<<<<<<<<<<<
commit 24cb4665c9dbb949bc1eb70cc8cbbb7752576750
Merge: 4ba6626bd b50ad4822
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jun 19 15:10:34 2020 +0100

    Merge pull request #1340 from gesellkammer/develop
    
    better error for dispfft

commit b50ad48221841f5abc9540b2f9c5dddbde1a67ff
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Jun 19 10:07:11 2020 +0200

    better error for dispfft

commit 4ba6626bd324d836c6606c4c759564ff6743c3da
Merge: d7350600a c026573cd
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Jun 17 18:01:14 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit d7350600ad4e76bdb6c3c4a8056119ef28284898
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Jun 17 18:01:05 2020 +0100

    allowing some opcodes to use functional syntax if mono output

commit c026573cd65c51924e0c3d2649c8bd0038731d4f
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Jun 17 17:13:45 2020 +0100

    Better handkng of end of file in pre lexer

commit 7eebbc5982a303713a3bba9cb03ed14d4cafe2af
Merge: 9a3415299 57f1a3413
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jun 14 20:42:03 2020 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 9a341529909aa046147ea6543eb7ebb39d7f3d70
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jun 14 20:41:44 2020 +0100

    looking at missing newlines and small changes; allow 31 date streams in arduinoRead

commit 57f1a341308b639dee3bf8589780a5b6c23cb5d6
Merge: 67bc3ec38 5da38e1e9
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jun 14 19:54:59 2020 +0100

    Merge pull request #1338 from joachimheintz/patch-2
    
    increase MAXLINE to 8192

commit 5da38e1e9db74e4d9fa0a686dda3dcc4b19b572e
Author: joachimheintz <2196394+joachimheintz@users.noreply.github.com>
Date:   Sun Jun 14 20:29:31 2020 +0200

    increase MAXLINE to 8192
    
    i ran into issues in reading long lines from het.  and as far as i see, this would coincide with the limit in fprints as set in fout.c, line 1284.

commit 67bc3ec38b748a68ccaed37565de12cddc57a8ad
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jun 12 23:08:21 2020 +0100

    include port filter in arduinoRead

commit 0c506fcef4cb23cbdfff4d8c916e46ec2fe91a13
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jun 12 15:46:48 2020 +0100

    Revised/extended sale opcode+ tweaks

commit 672a1625d721d0e1cc8a382f20cc29ec151abb82
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jun 5 13:25:31 2020 +0100

    Another attempt at windows arduino

commit 9b6f1f00965ee27a234545c19517d2e5d6f2063d
Author: John ffitch <jpff@codemist.co.uk>
Date:   Thu Jun 4 21:45:09 2020 +0100

    maybe windows arduin code

commit 1de9479a19fc95efe9ffc7f3975e85f11b4ea71a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Thu Jun 4 19:36:29 2020 +0100

    maye windows arduin code

commit 0dad8304c78efba48b4f405f4757f3e0a7e339c0
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Jun 1 18:44:46 2020 +0100

    init

commit c2acb452220e3fe164d4cc0a4e655f0ae0d8c7d4
Merge: 164abacfd 876c40c1b
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Jun 1 13:24:19 2020 +0100

    resolving conflicts

commit 164abacfd1b4561848cb15ce2c10472ea5e94a39
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Jun 1 13:18:20 2020 +0100

    resonbnk and apoleparams

commit 876c40c1b3d1048c8048268df1c8bb572ebfe549
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun May 31 20:27:03 2020 +0100

    fix to multicore + etter code

commit 5d1868b128866f66922c67a1f21403df4d90237a
Merge: 4987a50e3 1bab3716b
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun May 31 10:44:39 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 4987a50e3f9a05d1fb966cc054075187e70a441e
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun May 31 10:44:32 2020 +0100

    resonator bank

commit 1bab3716b2a203e0e23341c2aa5ead0c5da331ba
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat May 30 20:01:40 2020 +0100

    fix prepiano meory issue

commit 20835dc5dcf2d16cc770b66168c8598ba1cbd0f6
Merge: 69b2e4728 df240b589
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 30 18:48:23 2020 +0100

    merge failure

commit 69b2e4728765751d74278d125836b214d40f20a8
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 30 18:47:16 2020 +0100

    allpoleb opcode

commit df240b589489d4f659581316112772ab01da48f3
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat May 30 18:36:05 2020 +0100

    Nearly there

commit 0804278dd67597064e03a2157efe441546956673
Merge: b3992282a ff590dfdc
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat May 30 18:33:41 2020 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit b3992282aa39f9c4bfc07b8ae13fc6d250ddd5a7
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat May 30 18:33:31 2020 +0100

    Still trying to fix git

commit daf888e6b65a0114ad0af097143553537c383006
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat May 30 18:28:46 2020 +0100

    Still trying to fix git

commit 010e0ea39167c16b3b8d3bf7f150957de4649dd6
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat May 30 18:24:32 2020 +0100

    trying to mend git

commit b1bb9538d0c1c9d90b14730d4701c1ac0ab59373
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat May 30 18:22:41 2020 +0100

    Revert "minor"
    
    This reverts commit 03d813c84172f980f034c761a1657059dda330b8.

commit ff590dfdcf1157d584a0e416f9371b134610e568
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 30 18:13:52 2020 +0100

    coeffs to params fix

commit 95ad9f3bd87d0a356a8cb3e09433afec4758e565
Merge: 03d813c84 ab8c930d0
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat May 30 18:13:07 2020 +0100

    fix prepiano

commit 03d813c84172f980f034c761a1657059dda330b8
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat May 30 17:46:13 2020 +0100

    minor

commit a21391e2a338dd39431cf92335667f1f34a546a8
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat May 30 17:44:59 2020 +0100

    minor

commit ab8c930d00c16a7e63b93c23240688df9624dd35
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 30 17:43:31 2020 +0100

    coeffs to params

commit 3df3dfe17b8b9525706f81ed301d28b88c824764
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 30 15:58:41 2020 +0100

    pvscfs opcode complete

commit fe03abd650c739bde2e6f8fcb3fe05dea22d7ce3
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 30 15:15:59 2020 +0100

    duplicate line

commit e5b50e548ef8ad54ff0d0369737e9384946081f3
Merge: e0b647c01 af30837ed
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 30 15:15:00 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit e0b647c01f4729d4ea5c6e0ac59af0dfcf894f22
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 30 15:14:52 2020 +0100

    added lpred.c to android build script

commit af30837edf48c2b3ace477d14b1540792ac7b2ab
Merge: 8a2355db9 f3d15f3d2
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat May 30 10:07:58 2020 -0400

    Merge branch 'develop' of github.com:/csound/csound into develop

commit 8a2355db930a70ce9654738d19ae8ba10290cf51
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat May 30 10:07:51 2020 -0400

    added missing source file for Android build

commit f3d15f3d2a4d63a7a421e52b928692e671fe04d9
Merge: ad226acb5 6ecec1676
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 30 10:43:18 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit ad226acb57c682636e7de9b8ae805349bc4b6b40
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 30 10:43:09 2020 +0100

    pole to coeff and stabilisation code

commit 6ecec1676fb5ea4ef582c94f6c66e543f41276c6
Author: Steven Yi <stevenyi@gmail.com>
Date:   Fri May 29 22:22:31 2020 -0400

    fix formatting

commit d975e4bd03b0eda09175741a2d3ebded6f39712c
Author: Steven Yi <stevenyi@gmail.com>
Date:   Fri May 29 22:19:42 2020 -0400

    make csound build with Github Actions on pushes to develop and master

commit ef124fe2d93364843d1c785da5271bc2a2ddf08b
Merge: fd938599f 1e6e765e5
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri May 29 21:51:32 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit fd938599f80f9d471a1c55019cc2afb43011f5d0
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri May 29 21:51:25 2020 +0100

    pthread exit does not exist on win32

commit 1e6e765e5cda0bf8522ad2fe30a00bf6c2345f64
Merge: 61156154b 05dfdb419
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 29 21:49:09 2020 +0100

    corrections

commit 05dfdb41917d1f7af88ceca0b414d6ebb8b776a1
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri May 29 21:15:31 2020 +0100

    fixed typo on win

commit 61156154b9dece115fe897e8b4c62b24fd2d284a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 29 20:51:03 2020 +0100

    Windows  versionsof arduino coe not yet working

commit a7bdd937cd2bde1fdbdaf6a9babe75177f5cdbb1
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri May 29 20:28:21 2020 +0100

    csound lock functions correctly from struct now

commit 659ac06397901c61c933e2242c329b7a3c466ef3
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 29 17:15:11 2020 +0100

    experimental arduino code

commit 56efcfe64107462e6e88f55f1758ac09397da28c
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 29 16:46:30 2020 +0100

    experimental arduino coge

commit 8a4414c041d0a1c3722f1e9132a867e8e4fd5093
Merge: 92878f72f ebeb2a5e0
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 29 16:31:29 2020 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 92878f72ffc1a7c22d3a5e9ef1238aed78a4713c
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 29 16:30:55 2020 +0100

    experimental ardyino coe

commit ebeb2a5e0e6637c176885c5e6eb3e5b35a82599a
Merge: 0e036d7c3 dfada23e6
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 29 16:29:40 2020 +0100

    Merge pull request #1332 from gesellkammer/scugens_rename
    
    Scugens rename

commit 0e036d7c3f70ff110516b193894c4360547bdff6
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri May 29 13:34:53 2020 +0100

    added the Durbin method to lpanal

commit dfada23e6634851c045f82d01ccae80cc8d38dd1
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu May 28 13:02:41 2020 +0200

    missing ,

commit 5202666db785d02da5246edfbb5944d079453708
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu May 28 11:30:38 2020 +0200

    Alias for sc_ opcodes: sc_lag -> lag, sc_lagud -> lagud, sc_trig -> trigholf, sc_phasor -> phasortrig

commit 7d515122e26a4459f790c9a65469cd3e62b1bb94
Merge: dbbc994e5 3e6c7ab20
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu May 28 11:19:54 2020 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 3e6c7ab20bfce8bd6f77de2dd91f3e614e991c1b
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 22 20:26:50 2020 +0100

    correct line number in error od unknown varable

commit 7a20da5cc99dd73354347d5ece45c473e31f6838
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 22 19:48:58 2020 +0100

    table opcodes were wrngwrt non powerof2 cases

commit dbbc994e52db76ff5cab2d5fec53e157c6b6473b
Merge: 3b9a2a849 33a2e67ec
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu May 21 10:40:20 2020 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 33a2e67ecfb63eff4489090224c7fb95fe6391ea
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed May 20 21:05:14 2020 +0100

    changed name

commit 15afc17007b641dec390dca3671e6d7f41ba64da
Merge: c587c9cc2 26ea4b575
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed May 20 20:29:42 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit c587c9cc29377ae5fb2164e7a25c066008116b61
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed May 20 20:29:28 2020 +0100

    lpcanal to pvs

commit 3b9a2a8493bd51f672faadceb74a85f55b6e2bd7
Merge: 73856e996 26ea4b575
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed May 20 01:15:32 2020 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 26ea4b575bdad234469f97e60206ca28b726fd03
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue May 19 17:50:37 2020 -0400

    fix for reinstall command and homebrew

commit d5c185a9df7d2412e08ffd72d69a76b59701fa39
Merge: 6af27ca80 aea908000
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 21:02:04 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 6af27ca8046c3d2ce75884c6fff4c9f854831b1a
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 21:01:53 2020 +0100

    removing kprd restriction

commit 5577f451992f1e28a66b43031d3076546b812ed9
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 20:59:46 2020 +0100

    removing kprd restriction

commit aea90800096f9964d50c9c48b5d0d4289f2eb756
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue May 19 20:53:42 2020 +0100

    Altrnative fixto line number counting after a label

commit 2a54a09c74e7b3c4b9e38039be5f31a62b1474a7
Merge: dbe54203a cda19abbf
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 19:20:42 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit dbe54203a066c943117d592a472446939419e70d
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 19:20:31 2020 +0100

    changed opcode name

commit cda19abbf3f64d02de24f3a923538e68ccef6b13
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue May 19 14:03:58 2020 -0400

    added information about reinstall for homebrew

commit 49f5402826ea332b26ee097c403d35b222f89782
Merge: 502990fab d5e9193c7
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 17:56:02 2020 +0100

    fixed merge conflicts

commit 502990fab5c8b9a09f79bd11b7523ee71e54311f
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 17:54:13 2020 +0100

    added lpc signal analysis

commit d5e9193c7772fe01fe3a4cfa7ef35d06e157cdaf
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue May 19 17:43:02 2020 +0100

    strings

commit 7c7563dc1d7a51710e2e0717ff6730793eaa1b88
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue May 19 17:40:44 2020 +0100

    strings

commit 9227b3853332e7509b7711cf4000ed64f60fc6d5
Merge: 37cbf93b1 8f53cef79
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue May 19 17:39:39 2020 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 37cbf93b188f459e88aa46c401ce9ffe05e8c589
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue May 19 17:39:31 2020 +0100

    strings

commit 8f53cef799b274ff0a138611ac9b6eedbc969d2c
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 17:20:23 2020 +0100

    initialisation

commit e7df87db714afabe85093b9e579c598dd075aa76
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue May 19 17:00:53 2020 +0100

    uninitialised variable

commit 98707d01694d3bf766525f6699eade22c556fc42
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 14:03:26 2020 +0100

    cps corrected

commit c03f27c5dc624cd2bbe8d1a61673940db51fd95f
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 13:27:54 2020 +0100

    allpole filter

commit 4961d7e524bc37b72e8256fc80c3c48bfb2cbe6b
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 11:01:07 2020 +0100

    fixed filter scaling

commit 3e72687131bf55765bf91e14b3264821f7f464b5
Merge: 7946ccda7 596f64988
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 09:20:02 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 7946ccda73cc3ba7051c7361213aeb63d5eebf1e
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue May 19 09:19:23 2020 +0100

    rms computation

commit 596f6498808f3fc0a1dfb4658c12c0859e49d950
Author: François PINOT <fggpinot@gmail.com>
Date:   Tue May 19 10:10:29 2020 +0200

    Updated French transation

commit 73856e9962765941fb0559ac12445f0e42acf177
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Tue May 19 08:48:04 2020 +0200

    ftslice: init version must be a different opcode (ftslicei)

commit ac30b89be33cef569cfb72cee4377005f9bc6143
Merge: 796af4f35 4fc49c373
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Tue May 19 08:34:42 2020 +0200

    Merge branch 'max_i' into develop

commit 4fc49c3734a1eb521ea24625b3ee348fe02b1418
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Tue May 19 08:33:57 2020 +0200

    ftslice init version and all k version

commit 796af4f352ea1f7eb5fa03acf1089875794d796d
Merge: ebf454156 60871371c
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon May 18 22:28:21 2020 +0100

    fixed conflict

commit ebf454156404148362e793f2be9805ec81a1baca
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon May 18 22:23:55 2020 +0100

    added flag argument, fixed scaling

commit 60871371cbbd2025e203310a310d39138ea6c566
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon May 18 18:21:24 2020 +0100

    revert label syntax so line numbers are correct

commit 38ee69256101679732b5fc5a5b7dba155aa44b1f
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon May 18 14:39:12 2020 +0100

    compiler warnings removed

commit 1dcd90ae1661e3cd0a4b1b4e4e289f364e04dd27
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon May 18 14:38:36 2020 +0100

    compiler warnings removed

commit b128e88ff1e58c7e506ace4027f9349a3457da77
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon May 18 14:34:39 2020 +0100

    string/space

commit 0088a22046bf48704056fc1349f794bb2bec81ab
Merge: e8d7e8314 7f874034e
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon May 18 14:30:34 2020 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit e8d7e8314943b59f366167e3b0e68e8b2ffde7ba
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon May 18 14:30:08 2020 +0100

    new files

commit 7f874034ec5c94b4e221cbe04dec99ba579ff339
Merge: f1696497f 8f1e470af
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon May 18 13:01:41 2020 +0100

    Merge pull request #1326 from gesellkammer/max_i
    
     fix max.k / max.i order of declaration (i variants should appear first)

commit 8f1e470afeebb3a89d82895d31078856e5d71908
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon May 18 12:03:33 2020 +0200

    fix max.k / max.i order of declaration (i variants should appear always first)

commit a2097c061118ce76e3ce785a5a21fbe678753641
Merge: bfcf3919e f1696497f
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon May 18 11:34:46 2020 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit f1696497f552fefb0b85bc26d4d5917e841f9a05
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun May 17 21:39:04 2020 +0100

    update lpcfilter

commit 72db7e6fa55d2a8b9ecfb065dc72ceeb337754b7
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun May 17 21:01:48 2020 +0100

    fixed lpcfilter v2

commit c227cb26ed5d3ef9e121a3088187915f54f8058e
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun May 17 18:23:54 2020 +0100

    fixed comparison

commit 2422adae5833de84a0143b1d293122f1ce423a8b
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun May 17 18:19:14 2020 +0100

    added new lpc opcode

commit 43d8bddf15b9eb0900380c23c602a4f64213eeb8
Merge: 022c4122a 438d0425c
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun May 17 18:02:42 2020 +0100

    fix compilr warnings

commit 022c4122aef01c5c16e2e26baef8a82da810e544
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun May 17 17:37:12 2020 +0100

    fix compilr warnings

commit 438d0425c31beb51bbc52b0db8b18670f815bcd7
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun May 17 17:34:40 2020 +0100

    indent

commit 82a20d133be7ffe4cc0f56f7b0fba54ee27a2f15
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun May 17 17:18:08 2020 +0100

    move lpc code to its own file

commit 03896eb029d3a3d4f2495fc66445e6cfd88755b0
Merge: eb78069fc 84e4f4c1a
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun May 17 16:42:13 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit eb78069fc2b48e9d7d1714a7c498f721aefab4af
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun May 17 16:42:01 2020 +0100

    fixed lpread code and test opcode

commit 84e4f4c1a112132ea2411af934774ad9eb42847e
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Sun May 17 07:55:08 2020 +0200

    Updated French translation

commit de3cd45684426f462a7ca1f9f88053eb9e7718ee
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 16 22:18:43 2020 +0100

    working on lpc

commit 0c33b36279b263d947a590f2026d6877b01816a7
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 16 11:01:34 2020 +0100

    new linear prediction functions

commit 411addb2e47205b533b12287c061ad2a07808e9a
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat May 16 10:35:43 2020 +0100

    linear prediction functions

commit 3543181f4d4469da727d9b3eceb2f8dbe0a5f290
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 15 15:51:05 2020 +0100

    derecate ptable opcodes

commit 12e5676912b05de886118aaeb8ab6ac6e7eca225
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed May 13 20:59:39 2020 +0100

    Adjust for rounding error in GEN11

commit f82e67c22c90d8924e91ded4ee26642bf79fae56
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue May 12 21:27:25 2020 +0100

    fix cps2pch/cpsxpch in table case (typo)

commit cae24805812929b31fabca259636ec9f8387974f
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue May 12 21:06:08 2020 +0100

    fix cps2pch/cpsxpch in table case

commit a739e0b3c152aa5a34bd7407cc938c8908005db0
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sun May 10 10:15:17 2020 -0400

    removed older information about Homebrew tap and added some newer instructions on how to install and/or build csound on macOS

commit 97fd57c57b1dfd3b5c732a9f9149097be393b12b
Merge: aa51ad965 582c74039
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri May 8 21:07:30 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit aa51ad9656395fbb158dfed7860ff35715665718
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri May 8 21:07:16 2020 +0100

    improved phase estimation in partial tracking

commit 582c740394c775c72a9e3ac22cf7a1abeaa3650d
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri May 8 20:52:16 2020 +0100

    Permit zrerp input channels

commit bfcf3919e1185582ef3e57723ea8ce8b060dd0cb
Merge: d67d5c899 515e9459e
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu May 7 14:01:57 2020 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 515e9459ee6b01e50b5d8181c3641ca144f0b94b
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue May 5 20:36:55 2020 +0100

    Atrins and long lines

commit b37550df86945ab166204b0abe9e9c2eb1a12dfd
Merge: 0e3b15175 6e5569aa2
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue May 5 20:27:17 2020 +0100

    Merge pull request #1321 from gesellkammer/develop
    
    new opcode ftset

commit 0e3b1517575a66f04da24c46baabbd933e25c9df
Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon May 4 10:32:36 2020 -0400

    removing old documentation for opcode removed from Csound repo

commit d412a4d95fa545f32b187d5b3e2ed78035cd3108
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Apr 25 17:40:56 2020 +0100

    New regresion test

commit fb2e61cf2bea6caf5117c30c120430b0fa4ee56c
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Apr 25 16:22:55 2020 +0100

    Suggeted fix to  gen16

commit d67d5c899dc4638e11f01d10a0dc21de4c614cb9
Merge: 6e5569aa2 c139669e2
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Apr 23 19:13:27 2020 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit c139669e21bb89770654a74a55d13a4e01bc7721
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Apr 21 15:34:21 2020 +0100

    31bit PRNG stuck on zero

commit 393bfeb2f358c2baa3e302b47337ab479b298b64
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Apr 20 21:39:36 2020 +0100

    small typo

commit effc99911d21a5a5d6104156f05c00c56c2c13fe
Merge: cef10017a d1f222df1
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Apr 20 20:49:49 2020 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit cef10017a0f3bceb2591d61882d9e7fc7e4b5093
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Apr 20 20:49:37 2020 +0100

    new gauss opcode

commit 6e5569aa2a614ef7a3994c522f84b6a9c626b9d7
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sun Apr 19 20:04:21 2020 +0200

    fix ftprint not following the manual regarding trig == -1; fix ftprint showing wrong index; fix end index in ftset

commit 20fceb6fa67e33e41191aadc2b03b12469b210c1
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sun Apr 19 01:48:15 2020 +0200

    new opcode ftset, fixed memset special case

commit 800356366d5b35e42bd1628ca758dad0ec762c62
Merge: d1f222df1 0b3a47757
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Apr 18 23:45:19 2020 +0200

    Merge branch 'printsk-fix' into develop

commit 0b3a4775797479b2345b6f5583923da05bf1ee40
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Apr 18 23:44:39 2020 +0200

    added ftset opcode

commit d1f222df1adec4559e4dee7619a517943ddbf2a4
Merge: ba2965e53 280629a82
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Apr 18 21:36:19 2020 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit ba2965e533f9e658ff00956642bbe7540e190827
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Apr 18 21:34:50 2020 +0100

    Fix conditional expressions in S case; ad rndseed opcode

commit 280629a823820c848a2832aba3869d03f5c7faa9
Merge: 4074e40dd 84618a3ed
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Apr 18 14:26:58 2020 +0100

    Merge pull request #1320 from gesellkammer/printsk-fix
    
    Fixes a crash in printsk/println when opcode's init is skipped

commit 84618a3edfb9a6961c73334f0446f7e4466f8956
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Apr 18 14:56:55 2020 +0200

    detect when the opcode is not initialized (fixes crash when opcode is skipped via i-time goto)

commit 4074e40dde3d1c6bad04e022ee25f1242b9e4cc5
Author: Rory Walsh <rorywalsh@ear.ie>
Date:   Sat Apr 18 09:48:44 2020 +0100

    Updating the MacOS Azure build
    
    - Azure dropped support OSX-10.13, these changes address that..

commit 2428e9c51595d8348068a67cfd35a2715cbf2c95
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Apr 15 21:12:52 2020 +0100

    issue #1317 fixed

commit 6550362a23a006a28a441d288f637baca845fd49
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Apr 14 11:13:03 2020 +0100

    added pvsbandwidth

commit 25ed2314418a9946407a13daccf7f4bddbe6f2f2
Merge: 24f18b680 e2662a796
Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon Apr 13 19:18:29 2020 -0400

    Merge branch 'gesellkammer-develop' into develop

commit e2662a7964f4bb48193ffd97a3fccb57bc8d3720
Merge: 24f18b680 a2c43216d
Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon Apr 13 19:15:27 2020 -0400

    Merge branch 'develop' of https://github.com/gesellkammer/csound into gesellkammer-develop

commit a2c43216d03c552d2df59b54abe310ed56c50fa0
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Apr 13 18:42:08 2020 +0200

    forgot to remove check for printf

commit dc9414cf33b4c33b61dd80b1ad51ea17e7584cb6
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Apr 13 00:00:42 2020 +0200

    removed comma handling in println, now always appends a new line; added opcode printsk, which is similar to println but does not append a newline

commit 24f18b680153cae131f6a2b5ce3a8ff6226d7d29
Merge: af255ff6d 77f964df6
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Apr 12 19:56:38 2020 +0100

    Merge pull request #1316 from joachimheintz/patch-1
    
    fix bug in GEN20,8

commit 77f964df6b8f61b259628c9312b53e3913e48c98
Author: joachimheintz <2196394+joachimheintz@users.noreply.github.com>
Date:   Sun Apr 12 19:13:25 2020 +0200

    fix bug in GEN20,8
    
    gen20, 8 always returned 1 as value for the rectangle window, for instance in
    `i0 ftgen 8, 0, 8192, -20, 8, .1`
    i think this fixes it.

commit af255ff6d09533b5dd7fec2bd46b3700dc2796e9
Merge: 2997e5706 de4a06656
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Apr 12 15:56:57 2020 +0100

    Merge pull request #1314 from dvzrv/hdf5-1.12.0
    
    Fix for HDF5 1.12.0

commit 2997e57069fb96ca5cc7ffd311c82b3414059e9a
Merge: 4175828dd ec418afe6
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Apr 12 15:37:54 2020 +0100

    Merge pull request #1315 from joachimheintz/patch-1
    
    optional parameter for GEN20, 9

commit ec418afe695eb133fd4eeee496f9366cba42021a
Author: joachimheintz <2196394+joachimheintz@users.noreply.github.com>
Date:   Sun Apr 12 13:56:44 2020 +0200

    optional parameter for GEN20, 9
    
    the sinc function is currently set to a fixed range from -pi to +pi.
    the proposed change adds an optional multiplier for the x-range (1 by default).
    so  `f1   0   4096   -20   9   1` would result in the current behaviour, whereas `f1   0   4096   -20   9   1 3` would generate a sinc function from -3pi to +3pi.
    
    i tested the change and it seems to work.  i don't understand why there is no check in the code for x=0.  perhaps someone can review.
    
    once it is merged, i can add it to the manual.  i also have new figures for https://csound.com/docs/manual/MiscWindows.html.

commit de4a066566430e8742989dc41638242f5d07fa04
Author: David Runge <dave@sleepmap.de>
Date:   Sat Apr 11 22:28:05 2020 +0200

    Fix for HDF5 1.12.0
    
    Opcodes/hdf5/HDF5IO.c:
    The HDF5 update to 1.12.0 introduced breaking API changes [1]
    Particularly H5Oget_info() got removed.
    Instead H5Oget_info1() can be called, which provides the same
    signature.
    For HDF5 prior to 1.12.0 the call to H5Oget_info() remains.
    
    Closes #1313
    
    [1] https://portal.hdfgroup.org/display/HDF5/API+Compatibility+Reports+for+1.12
    
    Adding definitions for hdf5 version.
    
    Opcodes/hdf5/CMakeLists.txt:
    Splitting the HDF5_VERSION string into MAJOR, MINOR, PATCH to be able to
    add definitions for HDF5_VERSION_MAJOR, HDF5_VERSION_MINOR and
    HDF5_VERSION_PATCH to be used during preprocessing.

commit 2688f88f965510bd6d1bb366e784171616180e94
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Apr 11 14:49:41 2020 +0200

    avoid comparing to the adress of the type itself to satisfy the gods of windows linking

commit 11e7b8d803041fa0c8cab2ac31a4657327447757
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Apr 10 20:41:24 2020 +0200

    rename echo to println

commit 21425dd6778c1cd5a654fa7458bec1e49fff888c
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Apr 10 19:15:40 2020 +0200

    new opcode echo, similar to printf but without trigger. Also no allocations at performance time, all memory is reused from previous instances

commit 151b4a0ea7a3158596a2cbc0ed91b0892fa74c27
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Apr 9 12:04:07 2020 +0200

    remove errormsg

commit 5cb06cfaa9574b4958d920cb5b513e540cc63a64
Merge: 4ee93051d 4175828dd
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Apr 9 11:43:08 2020 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 4175828ddba695d85dcc727a4586e9ae0a29d32a
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Apr 7 16:43:30 2020 -0400

    make LIBRARY_INSTALL_DIR overrideable on commandline [issue #1310]

commit 1fadb88a708782fae8adcef820661d1c89e9b308
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Apr 7 18:12:16 2020 +0100

    mp3out error fixed (from PeteCA)

commit 98f11d05a4e5262da25dc77056b447731b8f13ee
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Apr 4 19:49:21 2020 +0100

    remove tracing

commit 6093d7eeb003d8f89a966fd36d3bc33a6f4e7fbe
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Apr 4 18:16:54 2020 +0100

    improvemets to simple score format

commit 4ee93051d131a8a96514a16ca70331b515a265a1
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Apr 4 14:15:43 2020 +0200

    put the error kind as the first arg

commit 3a757e64e0ef7a136f5926cc2d9e9fdee6d75801
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Apr 4 14:04:58 2020 +0200

    added opcode errormsg

commit abd0f66dd594d45a4e2210a3d0c7f37431a29773
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Apr 3 16:36:26 2020 +0100

    Neter conditional expession; adjusted for i versus k rate

commit 99d87dcc4861fdd81892adcbbe4678f660e7ab2d
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Apr 1 20:15:19 2020 +0100

    add %s to fprint(k)s

commit 9250ad19f22fe78fea2b74a5422c388b9d738609
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Apr 1 18:22:33 2020 +0100

    add %s to fprint(k)s

commit a4d9d1d714370c04bae323b8346b842c2e794b60
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Apr 1 17:22:49 2020 +0100

    string case in a?b:c

commit adfcdfeca3a9f9b330e4300c1d84a28b3658309f
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Mar 31 16:44:45 2020 -0400

    fixed setting nxtp instead of pds [fixes #1305 and #1306]

commit 025cc5191a61ee91bacbabca547a808e1fa0140e
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Mar 31 08:41:15 2020 -0400

    fix to advancing to end of OPDS chain

commit 0dc8a3dea8e05ea70520df2fe96d56001cf7a409
Merge: ff8403b0c e27558541
Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon Mar 30 16:55:51 2020 -0400

    Merge branch 'develop' of github.com:/csound/csound into develop

commit ff8403b0c0690deeca6f7d4907deaf86eb60fd52
Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon Mar 30 16:55:44 2020 -0400

    adjust which INSDS to interrogate when turning off instruments to fix when called from UDO [issue #1305]

commit e2755854118b0f26206d11fa001eb124455589ee
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Mar 30 20:40:26 2020 +0100

    warning message about doivision by zero in aa case of div

commit fd3f3cd314f1390b731096c32091dbe1149944b3
Merge: 18c574c4b 94f8017ee
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Mar 24 16:20:31 2020 +0000

    Merge branch 'develop' of github.com:csound/csound into develop

commit 18c574c4babac2a69037ea9086ffc4c457e9a27a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Mar 24 16:20:21 2020 +0000

    tabs

commit 94f8017ee406ddbcf8a376f1df0fa128735fe71d
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Tue Mar 24 16:31:29 2020 +0100

    Updated French translation

commit 93cb3ebc344043a7ee828a85191293da36200d82
Merge: ecc0a5c17 0b2c5be2c
Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon Mar 23 09:49:13 2020 -0400

    Merge branch 'gesellkammer-auhal-test' into develop

commit 0b2c5be2cc7cbb6746c69a01d3351c8701d8791e
Merge: ecc0a5c17 7625f295e
Author: Steven Yi <stevenyi@gmail.com>
Date:   Mon Mar 23 09:48:54 2020 -0400

    Merge branch 'auhal-test' of https://github.com/gesellkammer/csound into gesellkammer-auhal-test

commit ecc0a5c170d4e936c0b81978935b9886f744ac9b
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Mar 21 21:41:58 2020 +0000

    more checking in setcol

commit 2d96c2098c39ee7b680b4457b6a174c652152002
Merge: 430a99cdf 290b386a2
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Mar 21 17:42:40 2020 +0000

    Merge pull request #1304 from joachimheintz/patch-1
    
    fix setcols

commit 290b386a210d8e84e301655133464a92ead6a6f3
Author: joachimheintz <2196394+joachimheintz@users.noreply.github.com>
Date:   Sat Mar 21 15:38:18 2020 +0100

    fix setcols
    
    i think this is correct now.  example:
    
    instr 1
    iArr[][] init 3, 4
    iCol[] fillarray 1,2,3
    printarray iArr, "%d", "array at start:"
    iArr setcol iCol, 3
    printarray iArr, "%d", "array after setcol:"
    endin
    schedule(1,0,0)
    
    instr 2
    kArr[][] init 3, 4
    kCol[] fillarray 1,2,3
    printarray kArr, 1, "%d", "array at start:"
    kArr setcol kCol, 3
    printarray kArr, 1, "%d", "array after setcol:"
    turnoff
    endin
    schedule(2,0,1)
    
    what is missing: error message if desired column does not exist, e.g.
    iArr setcol iCol, 3

commit 7625f295e3eaae18fe57c8541c59ab74be326788
Merge: 33b447add 45834fb27
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Mar 20 21:23:52 2020 +0100

    fix conflict

commit 33b447add429efc780d1f70c72919ff038ad5d2c
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Mar 20 20:56:17 2020 +0100

    fix spacing conflicts

commit 6ce22a01dc9c2d5573ceb969819bc622dab50902
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Mar 16 14:26:25 2020 +0100

    replace simple char comparison by strcmp in chn_k string init

commit 308b4371408030c614132f57f15bdff7b02b4a90
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Mar 12 23:47:05 2020 -0700

    Coreaudio: check if input/output devices have enough channels.
    Fail if the number of channels is less than requested via nchnls
    or nchnls_i. Without this, coreaudio fails silently and hangs.
    With this fix coreaudio has the same behaviour as portaudio on
    macOS

commit 53cbb19fea67e7a96d9d61190ded6531fa473c78
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Mar 13 08:32:33 2020 +0100

    add warning for channel mismatch in coreaudio

commit 45834fb2704059d3b9daaf39f312d7045a36797f
Merge: c5e76d0e8 f7d18f3b5
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Mar 20 20:47:25 2020 +0100

    Merge branch 'auhal-test' of github.com:gesellkammer/csound into auhal-test

commit c5e76d0e88b12add6d4cb9462889df1e665c1705
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Mar 16 14:26:25 2020 +0100

    replace simple char comparison by strcmp in chn_k string init

commit c270902d78db558debb8d84a8828344cb9cb942d
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Mar 12 23:47:05 2020 -0700

    Coreaudio: check if input/output devices have enough channels.
    Fail if the number of channels is less than requested via nchnls
    or nchnls_i. Without this, coreaudio fails silently and hangs.
    With this fix coreaudio has the same behaviour as portaudio on
    macOS

commit 34c01e26c1367e3f085ed49bcd04312c260df6c6
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Mar 13 08:32:33 2020 +0100

    add warning for channel mismatch in coreaudio

commit 430a99cdfbb97d4d8302d0f369aaedc136e1027d
Merge: ef906df32 39576bb32
Author: Steven Yi <stevenyi@gmail.com>
Date:   Fri Mar 20 14:37:03 2020 -0400

    Merge pull request #1290 from gesellkammer/develop
    
    chn_k: add possibility to set the input/output mode as string

commit ef906df3263e010cfae2554bfa9e1a038435f723
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Mar 20 14:54:49 2020 +0000

    editing errior; restore correct version

commit b72c39e7a2a76747782f273fbaa8e0576a0f516c
Author: John ffitch <jpff@codemist.co.uk>
Date:   Thu Mar 19 18:10:32 2020 +0000

    implemnt array version of taninv2

commit 39576bb3273c153c4d163c329995a53b914f82be
Merge: 93be52a8d 2d11d3596
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Mar 19 01:46:28 2020 +0100

    fix conflicts

commit 93be52a8dc769fa3ec452d388ae3432c207a0b7c
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Mar 19 01:26:18 2020 +0100

    fixed reshapearray

commit 2d11d359649151d6a536085d63dc2822383879e2
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Mar 18 22:00:36 2020 +0000

    partial fix to setcol

commit ead3622c22225e0479500a1e4a81e8756861493d
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Mar 18 20:12:10 2020 +0000

    remove tracing

commit 4e7c9ed9bda8007aca1ce7181414d2062e70a943
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Mar 18 20:11:52 2020 +0000

    remove tracing

commit 83e5137e3adcbe0d7f912fa381c8fa16b52d52b2
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Mar 18 19:26:38 2020 +0000

    Various impromements in trim

commit 07c45451aaa0f45ee232f6b1aeff42855b80727a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Mar 16 17:10:32 2020 +0000

    removed mixed sifne/unsigned comparison

commit eb93ebf0f0587d96bbfb3fc8b552bd1e2ff9ee7f
Merge: b52f95e63 5010d2f03
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Mar 16 17:03:33 2020 +0000

    Merge pull request #1296 from iskunk/vg-fix-24
    
    Fix for Valgrind-reported invalid read

commit b52f95e63d5fe7318633df9910f6ec267279c565
Merge: e5d58878a 0656043a0
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Mar 16 17:01:41 2020 +0000

    Merge pull request #1295 from iskunk/vg-fix-23
    
    Fix for Valgrind-reported invalid read

commit b366b68f7ed71089c30c8bdd9de8c1ce47002f9f
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Mar 16 14:28:18 2020 +0100

    replace simple char comparison by strcmp in chn_k string init

commit f7d18f3b510624f4e63c5d178181daa91e010726
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Mar 16 14:26:25 2020 +0100

    replace simple char comparison by strcmp in chn_k string init

commit 60ebc95fefc4a42c2818ae7193a84becb4c38f06
Merge: b6593a6d9 e5d58878a
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Mar 14 07:39:37 2020 +0100

    Merge remote-tracking branch 'upstream/develop' into auhal-test

commit 902d7b7c733560227603672ea4a96fbed1afc383
Merge: ca8ef6b58 e5d58878a
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Mar 14 07:35:44 2020 +0100

    Merge remote-tracking branch 'upstream/develop' into develop

commit e5d58878aa3ecbf38b57ed269153fba1ce625276
Merge: 456b10b92 24bbcc00a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Mar 13 17:02:58 2020 +0000

    Merge pull request #1282 from iskunk/misc-fix-2
    
    Ensure that widget_reset() is called on reset

commit b6593a6d9d243a65ee8f4179d29e08d45af68104
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Mar 12 23:47:05 2020 -0700

    Coreaudio: check if input/output devices have enough channels.
    Fail if the number of channels is less than requested via nchnls
    or nchnls_i. Without this, coreaudio fails silently and hangs.
    With this fix coreaudio has the same behaviour as portaudio on
    macOS

commit 180eaee1f67cc23dec41310955d10bcef1536b69
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Mar 13 08:32:33 2020 +0100

    add warning for channel mismatch in coreaudio

commit 456b10b92eda36b6dd8c27367b2f83bd6c27c24d
Merge: 86c959cc6 74407ef33
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Mar 13 00:52:28 2020 +0000

    Merge pull request #1297 from iskunk/vg-fix-25
    
    Fix for Valgrind-reported jump/move dep on uninitialised values

commit 86c959cc628886e391cc912f2f9d4ccd63264489
Merge: 59f5c79ec 99a352fcd
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Mar 13 00:50:54 2020 +0000

    Merge pull request #1293 from iskunk/vg-fix-21
    
    Fix for Valgrind-reported invalid read

commit 59f5c79ec08e730ba1cbf0884806d368cd030fe9
Merge: 4fcf5f06a 23e3dbd48
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Mar 13 00:50:25 2020 +0000

    Merge pull request #1292 from iskunk/vg-fix-20
    
    Fix for Valgrind-reported invalid read

commit 4fcf5f06a638ed35be98f28c4c7bc994d173dc3b
Merge: f544db802 73a699b27
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Mar 13 00:49:55 2020 +0000

    Merge pull request #1294 from iskunk/vg-fix-22
    
    Fix for Valgrind-reported invalid read

commit 73a699b279e3ea85b550f6f7cb694d885d010fa7
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Mar 11 21:21:18 2020 -0400

    Fix for Valgrind-reported invalid read
    
    An array subscript should be checked before, not after, the array access.
    
    > 2 errors in context 1 of 1:
    > Invalid read of size 4
    >    at 0x50DA67D: trcross_process (psynth.c:1110)
    >    by 0x4E81882: kperf_nodebug (csound.c:1742)
    >    by 0x4E830ED: csoundPerform (csound.c:2265)
    >    by 0x109928: main (csound_main.c:328)
    >  Address 0x19d683dc is 12 bytes after a block of size 16,432 alloc'd
    >    at 0x4C32185: calloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x4EAA978: mcalloc (memalloc.c:113)
    >    by 0x4E87D5B: csoundAuxAlloc (auxfd.c:53)
    >    by 0x50D4247: partials_init (partials.c:157)
    >    by 0x4E9D4B6: init_pass (insert.c:117)
    >    by 0x4E9EE8C: insert_event (insert.c:479)
    >    by 0x4E9E0DF: insert (insert.c:305)
    >    by 0x4EB0227: process_score_event (musmon.c:833)
    >    by 0x4EB0D61: sensevents (musmon.c:1042)
    >    by 0x4E83031: csoundPerform (csound.c:2255)
    >    by 0x109928: main (csound_main.c:328)

commit f544db802d738bb79ccedc2e0c8b768b0c02cd1c
Author: John ffitch <jpff@codemist.co.uk>
Date:   Thu Mar 12 15:57:42 2020 +0000

    More accurate cent, semitone and dB

commit 74407ef330da9a06b7d7cef365982fde409dc78b
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Mar 11 21:43:16 2020 -0400

    Fix for Valgrind-reported jump/move dep on uninitialised values
    
    A stack-allocated struct needs zeroing out before use.
    
    > 5 errors in context 1 of 2:
    > Conditional jump or move depends on uninitialised value(s)
    >    at 0x5A031A7: psf_open_file (sndfile.c:2935)
    >    by 0x4E8CBEF: csoundFileOpenWithType (envvar.c:1104)
    >    by 0x50A80A8: filegrain_init (syncgrain.c:442)
    >    by 0x4E9D4B6: init_pass (insert.c:117)
    >    by 0x4E9EE8C: insert_event (insert.c:479)
    >    by 0x4E9E0DF: insert (insert.c:305)
    >    by 0x4EB0227: process_score_event (musmon.c:833)
    >    by 0x4EB0D61: sensevents (musmon.c:1042)
    >    by 0x4E83031: csoundPerform (csound.c:2255)
    >    by 0x109928: main (csound_main.c:328)
    >  Uninitialised value was created by a stack allocation
    >    at 0x50A7D37: filegrain_init (syncgrain.c:404)

commit 5010d2f030e92347af1ea9226bd6cf529813f80e
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Mar 11 21:37:09 2020 -0400

    Fix for Valgrind-reported invalid read
    
    A function table array is accessed without checking its size.
    
    > 2052279 errors in context 1 of 1:
    > Invalid read of size 8
    >    at 0x509FC13: sndwarpst (sndwarp.c:342)
    >    by 0x4E81882: kperf_nodebug (csound.c:1742)
    >    by 0x4E830ED: csoundPerform (csound.c:2265)
    >    by 0x109928: main (csound_main.c:328)
    >  Address 0x19d262f8 is 0 bytes after a block of size 524,328 alloc'd
    >    at 0x4C32185: calloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x4EAA978: mcalloc (memalloc.c:113)
    >    by 0x4E98D27: ftalloc (fgens.c:2384)
    >    by 0x4E8F44B: hfgens (fgens.c:275)
    >    by 0x4EB0296: process_score_event (musmon.c:845)
    >    by 0x4EB0D61: sensevents (musmon.c:1042)
    >    by 0x4E83031: csoundPerform (csound.c:2255)
    >    by 0x109928: main (csound_main.c:328)

commit 0656043a0f630eeba5537430b0b957cdbb7ac7f9
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Mar 11 21:27:07 2020 -0400

    Fix for Valgrind-reported invalid read
    
    An array is accessed without checking that the subscript is in range.
    
    > 656 errors in context 2 of 2:
    > Invalid read of size 8
    >    at 0x4FE5FE6: sprocess2 (pvlock.c:482)
    >    by 0x4E81882: kperf_nodebug (csound.c:1742)
    >    by 0x4E830ED: csoundPerform (csound.c:2265)
    >    by 0x109928: main (csound_main.c:328)
    >  Address 0x19dbd7f0 is 0 bytes after a block of size 16,416 alloc'd
    >    at 0x4C32185: calloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x4EAA978: mcalloc (memalloc.c:113)
    >    by 0x4E87D5B: csoundAuxAlloc (auxfd.c:53)
    >    by 0x4FE5316: sinit2 (pvlock.c:348)
    >    by 0x4E9D4B6: init_pass (insert.c:117)
    >    by 0x4E9EE8C: insert_event (insert.c:479)
    >    by 0x4E9E0DF: insert (insert.c:305)
    >    by 0x4EB0227: process_score_event (musmon.c:833)
    >    by 0x4EB0D61: sensevents (musmon.c:1042)
    >    by 0x4E83031: csoundPerform (csound.c:2255)
    >    by 0x109928: main (csound_main.c:328)

commit 99a352fcd0036c68f269e1296fd61566c72bf3ea
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Mar 11 21:15:25 2020 -0400

    Fix for Valgrind-reported invalid read
    
    An array subscript should be checked before, not after, the array access.
    
    > 6 errors in context 1 of 1:
    > Invalid read of size 8
    >    at 0x50D519A: Analysis (partials.c:288)
    >    by 0x50D5969: partials_process (partials.c:443)
    >    by 0x4E81882: kperf_nodebug (csound.c:1742)
    >    by 0x4E830ED: csoundPerform (csound.c:2265)
    >    by 0x109928: main (csound_main.c:328)
    >  Address 0x19ce9200 is 0 bytes after a block of size 8,032 alloc'd
    >    at 0x4C32185: calloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x4EAA978: mcalloc (memalloc.c:113)
    >    by 0x4E87D5B: csoundAuxAlloc (auxfd.c:53)
    >    by 0x50D4052: partials_init (partials.c:135)
    >    by 0x4E9D4B6: init_pass (insert.c:117)
    >    by 0x4E9EE8C: insert_event (insert.c:479)
    >    by 0x4E9E0DF: insert (insert.c:305)
    >    by 0x4EB0227: process_score_event (musmon.c:833)
    >    by 0x4EB0D61: sensevents (musmon.c:1042)
    >    by 0x4E83031: csoundPerform (csound.c:2255)
    >    by 0x109928: main (csound_main.c:328)

commit 23e3dbd484d21b6464fceffa25d466500d6f6bc3
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Mar 11 21:04:36 2020 -0400

    Fix for Valgrind-reported invalid read
    
    Load_File_() should add a trailing null to the buffer so that string reads
    of the content do not walk off the end.
    
    > 2368 errors in context 1 of 1:
    > Invalid read of size 1
    >    at 0x4C37E04: rawmemchr (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x5697351: _IO_str_init_static_internal (strops.c:41)
    >    by 0x568878C: vsscanf (iovsscanf.c:40)
    >    by 0x56821A3: sscanf (sscanf.c:32)
    >    by 0x16CF059A: scsnux_init_ (scansynx.c:354)
    >    by 0x16CF0F44: scsnux_init_S (scansynx.c:478)
    >    by 0x4E9D4B6: init_pass (insert.c:117)
    >    by 0x4E9EE8C: insert_event (insert.c:479)
    >    by 0x4E9E0DF: insert (insert.c:305)
    >    by 0x4EB0227: process_score_event (musmon.c:833)
    >    by 0x4EB0D61: sensevents (musmon.c:1042)
    >    by 0x4E83031: csoundPerform (csound.c:2255)
    >    by 0x109928: main (csound_main.c:328)
    >  Address 0x19d1526b is 0 bytes after a block of size 1,659 alloc'd
    >    at 0x4C2FDAF: malloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x4EAA7FF: mmalloc (memalloc.c:75)
    >    by 0x4EABC68: Load_File_ (memfiles.c:287)
    >    by 0x4EABEA7: ldmemfile2withCB (memfiles.c:356)
    >    by 0x16CF0346: scsnux_init_ (scansynx.c:315)
    >    by 0x16CF0F44: scsnux_init_S (scansynx.c:478)
    >    by 0x4E9D4B6: init_pass (insert.c:117)
    >    by 0x4E9EE8C: insert_event (insert.c:479)
    >    by 0x4E9E0DF: insert (insert.c:305)
    >    by 0x4EB0227: process_score_event (musmon.c:833)
    >    by 0x4EB0D61: sensevents (musmon.c:1042)
    >    by 0x4E83031: csoundPerform (csound.c:2255)
    >    by 0x109928: main (csound_main.c:328)

commit 4c7d6882d2327be60bfd183f6ac3b63b6a7327c9
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Mar 11 12:18:39 2020 +0000

    correct misedit

commit fb6627f63dd7995ecb8578af3ef210d697389149
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Mar 10 22:03:25 2020 +0000

    Fix to setksmp problems

commit ca8ef6b58a5b9539760dbb82fef4f008e4b9d928
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Tue Mar 10 13:17:53 2020 +0100

    fix wrong order for outvalue - i variant should come before k variant in the OENTRY list

commit 807b84add702314f48eab764ff59bb3ca47bfa47
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Mar 8 16:32:43 2020 +0000

    long line trimmed

commit 2d1d5eece2e532026f9b98f22e24662acb2fde90
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Mar 7 21:35:33 2020 +0000

    fix for carrying opcode name into PerfError

commit 03d5e69598a6679eb66ef94e77c808fcf40c6576
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Mar 6 00:25:27 2020 +0100

    added version of chn_k accepting the mode as string. r=1 (input), w=2 (output), rw=3 (input+output)

commit 927fb432445086873e3b25ff5004fe4ec81de332
Merge: dfd3e2fa6 a887598f3
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Mar 6 00:23:01 2020 +0100

    Merge remote-tracking branch 'upstream/develop' into develop

commit a887598f307fd94c09f25b9891fc34b84f013f56
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Mar 3 21:22:08 2020 +0000

    small code improvements

commit 308ab87fd2135b2d3492331bed0e19cd62889c61
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Mar 3 18:07:37 2020 +0000

    Police iir/per errors in correct mode and improve error messages

commit e1fcf75277ad157a8fecf8568af14e7a6af42e09
Merge: f16f2382d eba66fd32
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Mar 1 21:14:13 2020 +0000

    Merge branch 'develop' of github.com:csound/csound into develop

commit f16f2382d9dc1a685ce8e718396a478a6104c8f2
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Mar 1 21:14:00 2020 +0000

    remove ptch and fix with calls to InitError rater than erfError

commit eba66fd3210cb6da969a8a1535482438adb42a51
Merge: 7a611ab32 88371ccd7
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Sun Mar 1 20:57:35 2020 +0000

    Merge pull request #1286 from ketchupok/digiOutFix
    
    updated digiBelaOut and digiIOBela

commit 7a611ab32209fb3b79d662110582bc04e1c2c8b1
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Mar 1 19:45:35 2020 +0000

    temporary patch to avoid infinite loop

commit 88371ccd719d534206562104b0a1243e88d0914b
Author: ketchupok <ahah@gmx.net>
Date:   Sun Mar 1 18:58:56 2020 +0000

    updated digiBelaOut and digiIOBela

commit dfd3e2fa6aa17daedb31522fd920869d6509a267
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sun Mar 1 11:34:22 2020 +0100

    added missing new line in warning

commit ca9464d2d6650049dff9766d80344cedfde167ad
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Feb 28 20:30:02 2020 +0000

    Check against OK not NOTOK

commit 7c5704287c32497a22bbff7a8f231b8adad5cd69
Merge: a15e2f294 6bfcbb896
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Feb 28 19:58:22 2020 +0000

    Merge pull request #1284 from iskunk/misc-fix-3
    
    Fix segfault in "voice" opcode

commit a15e2f2945090cdaf68a3e9f0608e81854af638b
Author: François PINOT <fggpinot@gmail.com>
Date:   Fri Feb 28 15:59:31 2020 +0100

    Updated French translation

commit 6bfcbb896ef95220e4a6b4531a6cc01a39c77e84
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Thu Feb 27 14:37:51 2020 -0500

    Fix segfault in "voice" opcode
    
    The voicformset() function incorrectly checks for an error condition,
    preventing it from bailing out when a function table is missing.

commit c62a5499cc3acce0430bd0316b1619b5ff75378e
Merge: 00e054807 19aac3f59
Author: John ffitch <jpff@codemist.co.uk>
Date:   Thu Feb 27 16:49:05 2020 +0000

    Merge branch 'develop' of github.com:csound/csound into develop

commit 00e054807cad0de96b7be0949041fb61f7b97c5a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Thu Feb 27 16:48:52 2020 +0000

    remove old file

commit 19aac3f59b37e0ca6560475f408e03cfaca6da44
Merge: 42f7af7c1 c444e8f75
Author: John ffitch <jpff@codemist.co.uk>
Date:   Thu Feb 27 16:48:22 2020 +0000

    Merge pull request #1283 from laserbat/yyin
    
    Fixed issue when linking csbeats

commit 42f7af7c1a5a9d7eba6ad5af8a3143dc244fe7c6
Merge: ea8f14460 279982c32
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Thu Feb 27 05:09:43 2020 -0500

    Merge pull request #1276 from iskunk/vg-fix-15
    
    Fix for Valgrind-reported memory leak

commit c444e8f752c3acc31d364b901204e09eae2e791e
Author: Olga Ustuzhanina <me@laserbat.pw>
Date:   Thu Feb 27 08:51:32 2020 +0000

    Fixed issue when linking csbeats

commit ea8f14460c5172cea4f02153c8f724d461893c4b
Merge: ea9b50773 77a0028ca
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 26 22:52:19 2020 +0000

    Merge pull request #1281 from iskunk/vg-fix-19
    
    Fix for Valgrind-reported memory leak

commit 24bbcc00aad6a6c0ae182e19835f918b8a6bb9c8
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Feb 26 17:12:03 2020 -0500

    Ensure that widget_reset() is called on reset

commit 77a0028ca6ee6be71afcf6738e00fa1584f78ddc
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Feb 26 16:51:15 2020 -0500

    Fix for Valgrind-reported memory leak
    
    widget_reset() fails to clear widgetGlobals->AddrSetValue.
    
    > 224 bytes in 1 blocks are definitely lost in loss record 211 of 333
    >    at 0x4C3041F: operator new(unsigned long) (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x168D10FF: __gnu_cxx::new_allocator<ADDR_SET_VALUE>::allocate(unsigned long, void const*) (new_allocator.h:111)
    >    by 0x168CF8F5: std::allocator_traits<std::allocator<ADDR_SET_VALUE> >::allocate(std::allocator<ADDR_SET_VALUE>&, unsigned long) (alloc_traits.h:436)
    >    by 0x168CD4AF: std::_Vector_base<ADDR_SET_VALUE, std::allocator<ADDR_SET_VALUE> >::_M_allocate(unsigned long) (stl_vector.h:172)
    >    by 0x168CF07D: void std::vector<ADDR_SET_VALUE, std::allocator<ADDR_SET_VALUE> >::_M_realloc_insert<ADDR_SET_VALUE>(__gnu_cxx::__normal_iterator<ADDR_SET_VALUE*, std::vector<ADDR_SET_VALUE, std::allocator<ADDR_SET_VALUE> > >, ADDR_SET_VALUE&&) (vector.tcc:406)
    >    by 0x168CC8A7: void std::vector<ADDR_SET_VALUE, std::allocator<ADDR_SET_VALUE> >::emplace_back<ADDR_SET_VALUE>(ADDR_SET_VALUE&&) (vector.tcc:105)
    >    by 0x168C95A5: std::vector<ADDR_SET_VALUE, std::allocator<ADDR_SET_VALUE> >::push_back(ADDR_SET_VALUE&&) (stl_vector.h:954)
    >    by 0x168BD424: fl_joystick (widgets.cpp:3927)
    >    by 0x4E9DC05: init0 (insert.c:249)
    >    by 0x4EADD54: musmon (musmon.c:313)
    >    by 0x5033D63: csoundStart (main.c:549)
    >    by 0x5033DA7: csoundCompile (main.c:556)
    >    by 0x109913: main (csound_main.c:326)

commit 279982c329927335b01f6fba3cad4d701b27044d
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Tue Feb 25 16:01:58 2020 -0500

    Fix for Valgrind-reported memory leak
    
    Various vector objects in sfg_globals contain un-deleted objects prior
    to being cleared.
    
    Typical example:
    
    > 208 (144 direct, 64 indirect) bytes in 6 blocks are definitely lost in loss record 10 of 18
    >    at 0x4C3041F: operator new(unsigned long) (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x18B41240: csound::Inleta::init(CSOUND_*) (signalflowgraph.cpp:365)
    >    by 0x18B4A9AF: csound::OpcodeBase<csound::Inleta>::init_(CSOUND_*, void*) (OpcodeBase.hpp:129)
    >    by 0x4E9D3D4: init_pass (insert.c:116)
    >    by 0x4E9ED2A: insert_event (insert.c:472)
    >    by 0x4E9DF7D: insert (insert.c:298)
    >    by 0x4EAFF65: process_score_event (musmon.c:829)
    >    by 0x4EB0432: process_rt_event (musmon.c:926)
    >    by 0x4EB11E1: sensevents (musmon.c:1136)
    >    by 0x4E82F76: csoundPerform (csound.c:2245)
    >    by 0x109928: main (csound_main.c:328)

commit ea9b5077345bc6315b1d517f02b8a0c23ccd8fb4
Author: François PINOT <fggpinot@gmail.com>
Date:   Wed Feb 26 15:04:02 2020 +0100

    Updated ctcsound.py

commit e71eff6c9bb731b0a62500f4cd637de919c524f1
Merge: 8a1ec77a0 eb0029076
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 26 13:26:00 2020 +0000

    Merge pull request #1279 from iskunk/vg-fix-17
    
    Fix for various Valgrind-reported memory leaks

commit 8a1ec77a0dd9de0efb03181f9308a67ef4b8d8e8
Merge: 5228d1544 87ccb64ca
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 26 13:25:28 2020 +0000

    Merge pull request #1278 from iskunk/vg-fix-16
    
    Fix for Valgrind-reported memory leak

commit 5228d154477cca1d840c93118824686c70fe6292
Merge: c3c4f6051 774396d5a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 26 13:24:25 2020 +0000

    Merge pull request #1280 from iskunk/vg-fix-18
    
    Fix Valgrind-reported invalid read

commit c3c4f6051f8fba30332b090fc0ed318fc2543b5b
Merge: 31d266cb1 ffb4cd2af
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 26 13:21:01 2020 +0000

    Merge pull request #1277 from iskunk/misc-fix-1
    
    Eliminate "//*" sequences interpreted as comment-block starts

commit 774396d5a7b26f552ee1436a80d292727484d5b0
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Tue Feb 25 18:40:57 2020 -0500

    Fix Valgrind-reported invalid read
    
    test_buffer_run() pops a message before printing it, when it should be
    doing the reverse.
    
    > 410 errors in context 6 of 6:
    > Invalid read of size 1
    >    at 0x4C33384: strlen (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x5D624D2: vfprintf (vfprintf.c:1643)
    >    by 0x5D6367F: buffered_vfprintf (vfprintf.c:2329)
    >    by 0x5D60725: vfprintf (vfprintf.c:1301)
    >    by 0x5D69F25: printf (printf.c:33)
    >    by 0x14A35A: test_buffer_run (csound_message_buffer_test.c:58)
    >    by 0x4E40D36: run_single_test.constprop.5 (TestRun.c:991)
    >    by 0x4E4106F: run_single_suite.constprop.4 (TestRun.c:876)
    >    by 0x4E413BD: CU_run_all_tests (TestRun.c:367)
    >    by 0x14A43E: main (csound_message_buffer_test.c:93)
    >  Address 0xc7a99cd is 13 bytes inside a block of size 45 free'd
    >    at 0x4C30FDB: free (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x1518CD: csoundPopFirstMessage (csound.c:4319)
    >    by 0x14A342: test_buffer_run (csound_message_buffer_test.c:57)
    >    by 0x4E40D36: run_single_test.constprop.5 (TestRun.c:991)
    >    by 0x4E4106F: run_single_suite.constprop.4 (TestRun.c:876)
    >    by 0x4E413BD: CU_run_all_tests (TestRun.c:367)
    >    by 0x14A43E: main (csound_message_buffer_test.c:93)
    >  Block was alloc'd at
    >    at 0x4C2FDAF: malloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x151AD9: csoundMessageBufferCallback_1_ (csound.c:4384)
    >    by 0x14DBE1: csoundMessage (csound.c:2560)
    >    by 0x105CDA54: csoundModuleInit (rtalsa.c:1909)
    >    by 0x22FB82: csoundInitModule (csmodule.c:609)
    >    by 0x22FD3D: csoundInitModules (csmodule.c:664)
    >    by 0x231F9C: csoundStart (main.c:459)
    >    by 0x14A2E6: test_buffer_run (csound_message_buffer_test.c:50)
    >    by 0x4E40D36: run_single_test.constprop.5 (TestRun.c:991)
    >    by 0x4E4106F: run_single_suite.constprop.4 (TestRun.c:876)
    >    by 0x4E413BD: CU_run_all_tests (TestRun.c:367)
    >    by 0x14A43E: main (csound_message_buffer_test.c:93)

commit eb00290763838e155b0813468bec630dfe94ad53
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Tue Feb 25 18:14:13 2020 -0500

    Fix for various Valgrind-reported memory leaks
    
    Several functions in csound_debugger_test.c fail to call
    csoundDestroyMessageBuffer().
    
    Typical example:
    
    > 40 bytes in 1 blocks are indirectly lost in loss record 98 of 244
    >    at 0x4C2FDAF: malloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x5248F64: csoundCreateMutex (threads.c:478)
    >    by 0x50964B9: csoundCreateMessageBuffer (csound.c:4240)
    >    by 0x109286: test_debugger_init (csound_debugger_test.c:27)
    >    by 0x4E40D36: run_single_test.constprop.5 (TestRun.c:991)
    >    by 0x4E4106F: run_single_suite.constprop.4 (TestRun.c:876)
    >    by 0x4E413BD: CU_run_all_tests (TestRun.c:367)
    >    by 0x10AE33: main (csound_debugger_test.c:547)

commit 87ccb64ca32ef5f119c54301175d173a8975f1f9
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Tue Feb 25 17:37:50 2020 -0500

    Fix for Valgrind-reported memory leak
    
    listAlsaSeq() fails to close the handle returned from snd_seq_open().
    
    > 30,528 (128 direct, 30,400 indirect) bytes in 1 blocks are definitely lost in loss record 124 of 125
    >    at 0x4C32185: calloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0xCDA5DD7: snd_seq_hw_open (seq_hw.c:485)
    >    by 0xCDA60A3: _snd_seq_hw_open (seq_hw.c:563)
    >    by 0xCDA63C8: snd_seq_open_conf (seq.c:918)
    >    by 0xCDA6725: snd_seq_open_noupdate (seq.c:939)
    >    by 0xCDA68A6: snd_seq_open (seq.c:984)
    >    by 0xFF573FC: listAlsaSeq (rtalsa.c:1827)
    >    by 0xFF57778: listDevicesM (rtalsa.c:1870)
    >    by 0x14FBF8: csoundGetMIDIDevList (csound.c:2906)
    >    by 0x14A4B1: test_dev_list (io_test.c:63)
    >    by 0x4E40D36: run_single_test.constprop.5 (TestRun.c:991)
    >    by 0x4E4106F: run_single_suite.constprop.4 (TestRun.c:876)
    >    by 0x4E413BD: CU_run_all_tests (TestRun.c:367)
    >    by 0x14B3A5: main (io_test.c:324)

commit ffb4cd2afa6cd6302f06000f8624b83d52e898c4
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Tue Feb 25 16:09:38 2020 -0500

    Eliminate "//*" sequences interpreted as comment-block starts

commit 31d266cb17ed61ed44615b3ca10e07073d3fc962
Merge: 294c054f5 fd4b92aa9
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Feb 25 20:21:56 2020 +0000

    Merge pull request #1273 from laserbat/cmake
    
    Got rid of porttime library (which is now a part of portmidi)

commit fd4b92aa9f156930da6f5073c7fb2652d7514235
Author: Olga Ustuzhanina <me@laserbat.pw>
Date:   Tue Feb 25 19:34:53 2020 +0000

    Got rid of porttime library (which is now a part of portmidi)

commit 294c054f515942ff6ccd65c5e483323c503579ca
Merge: 8c36ec4df 4c81b276f
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Feb 22 13:41:28 2020 +0000

    Merge pull request #1268 from iskunk/vg-fix-13
    
    Fix for various Valgrind-reported memory leaks

commit 4c81b276f829bd19cafbfa0e56667cb4b35a36c7
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Feb 19 16:51:45 2020 -0500

    Fix for various Valgrind-reported memory leaks
    
    csoundJoinThread() does not free() the small bit of memory malloc()ed in
    csoundCreateThread() to hold the pthread_t object.
    
    The CsoundPerformanceThread destructor does not free the mutex and
    condition variable created toward the end of
    CsoundPerformanceThread::csPerfThread_constructor().
    
    A new library function is needed to destroy a condition variable.

commit 8c36ec4dfd3dff09bd32d291dbcb315f36f57b60
Merge: db9f26cae aed4f05dd
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 21:15:42 2020 +0000

    Merge pull request #1267 from iskunk/vg-fix-12
    
    Fix for Valgrind-reported invalid reads

commit db9f26cae168f0eae5b83860ce218ea4f081803d
Merge: 077194a0d 6a2f9a9b6
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 20:48:16 2020 +0000

    Merge branch 'develop' of github.com:csound/csound into develop

commit 077194a0d2ed1757a5eb126ebb0a3da04bb579a1
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 20:48:04 2020 +0000

    Use deinit callback to close /proc/cpu

commit aed4f05dd1396d2ed7bea64fef893b40be4b03dc
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Feb 19 15:47:10 2020 -0500

    Fix for Valgrind-reported invalid reads
    
    In process_debug_buffers(), the data->cur_bkpt pointer is not updated
    when the record it references is freed.
    
    In kperf_debug(), data->cur_bkpt is not NULL-checked before being
    dereferenced.
    
    > 1 errors in context 10 of 22:
    > Invalid read of size 4
    >    at 0x509013D: kperf_debug (csound.c:1935)
    >    by 0x5090AF8: csoundPerformKsmps (csound.c:2142)
    >    by 0x10AB30: test_next (csound_debugger_test.c:477)
    >    by 0x4E3FD36: ??? (in /usr/lib/x86_64-linux-gnu/libcunit.so.1.0.1)
    >    by 0x4E4006F: ??? (in /usr/lib/x86_64-linux-gnu/libcunit.so.1.0.1)
    >    by 0x4E403BD: CU_run_all_tests (in /usr/lib/x86_64-linux-gnu/libcunit.so.1.0.1)
    >    by 0x10AE33: main (csound_debugger_test.c:537)
    >  Address 0x19ea8750 is 48 bytes inside a block of size 72 free'd
    >    at 0x4C30D3B: free (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x50B87E9: mfree (memalloc.c:173)
    >    by 0x508FDCE: process_debug_buffers (csound.c:1861)
    >    by 0x509000D: kperf_debug (csound.c:1909)
    >    by 0x5090AF8: csoundPerformKsmps (csound.c:2142)
    >    by 0x10AB30: test_next (csound_debugger_test.c:477)
    >    by 0x4E3FD36: ??? (in /usr/lib/x86_64-linux-gnu/libcunit.so.1.0.1)
    >    by 0x4E4006F: ??? (in /usr/lib/x86_64-linux-gnu/libcunit.so.1.0.1)
    >    by 0x4E403BD: CU_run_all_tests (in /usr/lib/x86_64-linux-gnu/libcunit.so.1.0.1)
    >    by 0x10AE33: main (csound_debugger_test.c:537)
    >  Block was alloc'd at
    >    at 0x4C2FB0F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x50B8441: mmalloc (memalloc.c:75)
    >    by 0x523AE42: csoundSetInstrumentBreakpoint (csdebug.c:151)
    >    by 0x10AA3F: test_next (csound_debugger_test.c:455)
    >    by 0x4E3FD36: ??? (in /usr/lib/x86_64-linux-gnu/libcunit.so.1.0.1)
    >    by 0x4E4006F: ??? (in /usr/lib/x86_64-linux-gnu/libcunit.so.1.0.1)
    >    by 0x4E403BD: CU_run_all_tests (in /usr/lib/x86_64-linux-gnu/libcunit.so.1.0.1)
    >    by 0x10AE33: main (csound_debugger_test.c:537)

commit 6a2f9a9b6ecdd0ad61d78ca1dff4f1f05b12fe93
Merge: c98aad61f eee00a7ef
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 20:21:11 2020 +0000

    Merge pull request #1266 from iskunk/vg-fix-11
    
    Fix for Valgrind-reported invalid read

commit eee00a7efcc6b2b7365a9e9579339c37f7d1861b
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Feb 19 15:04:53 2020 -0500

    Fix for Valgrind-reported invalid read
    
    Code does not check for a read past the end of an array.
    
    > 9 errors in context 2 of 2:
    > Invalid read of size 8
    >    at 0x34C998: mtable_i (vectorial.c:50)
    >    by 0x167507: init_pass (insert.c:116)
    >    by 0x168E5D: insert_event (insert.c:472)
    >    by 0x1680B0: insert (insert.c:298)
    >    by 0x17A098: process_score_event (musmon.c:829)
    >    by 0x17A565: process_rt_event (musmon.c:926)
    >    by 0x17B314: sensevents (musmon.c:1136)
    >    by 0x14DC5E: csoundPerform (csound.c:2243)
    >    by 0x14A548: main (csound_main.c:328)
    >  Address 0xc28bb88 is 0 bytes after a block of size 296 alloc'd
    >    at 0x4C31B25: calloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x174805: mcalloc (memalloc.c:113)
    >    by 0x162D9F: ftalloc (fgens.c:2379)
    >    by 0x159568: hfgens (fgens.c:275)
    >    by 0x17A107: process_score_event (musmon.c:841)
    >    by 0x17ABD2: sensevents (musmon.c:1038)
    >    by 0x14DC5E: csoundPerform (csound.c:2243)
    >    by 0x14A548: main (csound_main.c:328)

commit c98aad61fe99b8a053fc887fa473408a1b2ade0e
Merge: 7d1d86c67 9200f7fd9
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 20:04:34 2020 +0000

    Merge pull request #1265 from iskunk/vg-fix-10
    
    Fix for Valgrind-reported invalid read

commit 7d1d86c67438668f58474180cd24390705fcd999
Merge: fecb2b009 e4a17e158
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 20:02:47 2020 +0000

    Merge pull request #1264 from iskunk/vg-fixes-9
    
    Fix for Valgrind-reported invalid reads

commit fecb2b009b34e3d3dfe70ebba210f22d920847c2
Merge: bd0abfa10 499315293
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 20:00:39 2020 +0000

    Merge pull request #1263 from iskunk/vg-fixes-8
    
    Fix for leaked file pointer

commit 9200f7fd9be8d9465bdaa7e02d6cc27532905c94
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Feb 19 14:17:26 2020 -0500

    Fix for Valgrind-reported invalid read
    
    Code checks for a null function table, but not soon enough.
    
    > 1 errors in context 1 of 1:
    > Invalid read of size 8
    >    at 0x37FDC3: flooper_init (sndloop.c:309)
    >    by 0x167507: init_pass (insert.c:116)
    >    by 0x168E5D: insert_event (insert.c:472)
    >    by 0x1680B0: insert (insert.c:298)
    >    by 0x17A098: process_score_event (musmon.c:829)
    >    by 0x17ABD2: sensevents (musmon.c:1038)
    >    by 0x14DC5E: csoundPerform (csound.c:2243)
    >    by 0x14A548: main (csound_main.c:328)
    >  Address 0x3ed0 is not stack'd, malloc'd or (recently) free'd

commit e4a17e1582076ec242260a9894a42cd8bb2e8fdf
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Feb 19 14:08:22 2020 -0500

    Fix for Valgrind-reported invalid reads
    
    Code does not check for a null function table.
    
    > 1 errors in context 1 of 1:
    > Invalid read of size 8
    >    at 0x2E3CC2: sprocess1 (pvlock.c:178)
    >    by 0x14C51E: kperf_nodebug (csound.c:1735)
    >    by 0x14DD1A: csoundPerform (csound.c:2253)
    >    by 0x14A548: main (csound_main.c:328)
    >  Address 0x3ed0 is not stack'd, malloc'd or (recently) free'd
    >
    [...]
    >
    > 1 errors in context 1 of 1:
    > Invalid read of size 8
    >    at 0x2E5026: sprocess2 (pvlock.c:393)
    >    by 0x14C51E: kperf_nodebug (csound.c:1735)
    >    by 0x14DD1A: csoundPerform (csound.c:2253)
    >    by 0x14A548: main (csound_main.c:328)
    >  Address 0x3ed0 is not stack'd, malloc'd or (recently) free'd

commit 4993152935cb0be70bccd6dfdfbd2c98a8de5202
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Wed Feb 19 13:53:00 2020 -0500

    Fix for leaked file pointer
    
    cpupercent_init() calls fopen() on /proc/stat, but never closes it.
    Create a Csound file handle so it is closed on program exit.
    
    > 552 bytes in 1 blocks are still reachable in loss record 4 of 8
    >    at 0x4C2FB0F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x5B73E49: __fopen_internal (iofopen.c:65)
    >    by 0x5B73E49: fopen@@GLIBC_2.2.5 (iofopen.c:89)
    >    by 0x30F617: cpupercent_init (cpumeter.c:78)
    >    by 0x167507: init_pass (insert.c:116)
    >    by 0x168E5D: insert_event (insert.c:472)
    >    by 0x1680B0: insert (insert.c:298)
    >    by 0x17A098: process_score_event (musmon.c:829)
    >    by 0x17ABD2: sensevents (musmon.c:1038)
    >    by 0x14DC5E: csoundPerform (csound.c:2243)
    >    by 0x14A548: main (csound_main.c:328)

commit bd0abfa10069a3e2016254b81f167ebf5f9c29bf
Merge: 3c6010661 f9dd9deaf
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 14:47:47 2020 +0000

    Merge pull request #1259 from iskunk/vg-fix-4
    
    Fix invalid pointer dereference

commit 3c60106615ca4ca4ebd5b3f6a0f6a880ebfce447
Merge: 970eca10e 6472a9630
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 14:38:46 2020 +0000

    Merge pull request #1262 from iskunk/vg-fix-7
    
    Fix Valgrind-reported invalid reads

commit 970eca10e185975d0f44e3c053459283ddce5ae9
Merge: 90c64b20d 44ac3f15e
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 14:37:58 2020 +0000

    Merge pull request #1261 from iskunk/vg-fix-6
    
    Fix Valgrind-reported jump/move dep on uninitialised values

commit 90c64b20da3dbfb4e9fd764e338d33f55bc8178b
Merge: 921207f49 fcba38bd5
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Feb 19 14:36:56 2020 +0000

    Merge pull request #1260 from iskunk/vg-fix-5
    
    Fix Valgrind-reported jump/move dep on uninitialised values

commit 6472a96303740b26add6e539d91054186c64bd59
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Tue Feb 18 19:33:12 2020 -0500

    Fix Valgrind-reported invalid reads
    
    The code fails to take into account that memory pointed to by a
    local variable may have moved.
    
    > 3 errors in context 4 of 4:
    > Invalid read of size 8
    >    at 0x2701EA: init_pp (bilbar.c:315)
    >    by 0x167507: init_pass (insert.c:116)
    >    by 0x168E5D: insert_event (insert.c:472)
    >    by 0x1680B0: insert (insert.c:298)
    >    by 0x17A098: process_score_event (musmon.c:829)
    >    by 0x17ABD2: sensevents (musmon.c:1038)
    >    by 0x14DC5E: csoundPerform (csound.c:2243)
    >    by 0x14A548: main (csound_main.c:328)
    >  Address 0xc269420 is 32 bytes inside a block of size 56 free'd
    >    at 0x4C30D3B: free (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x174A34: mfree (memalloc.c:173)
    >    by 0x152936: csoundAuxAlloc (auxfd.c:44)
    >    by 0x27002D: init_pp (bilbar.c:306)
    >    by 0x167507: init_pass (insert.c:116)
    >    by 0x168E5D: insert_event (insert.c:472)
    >    by 0x1680B0: insert (insert.c:298)
    >    by 0x17A098: process_score_event (musmon.c:829)
    >    by 0x17ABD2: sensevents (musmon.c:1038)
    >    by 0x14DC5E: csoundPerform (csound.c:2243)
    >    by 0x14A548: main (csound_main.c:328)
    >  Block was alloc'd at
    >    at 0x4C31B25: calloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x174805: mcalloc (memalloc.c:113)
    >    by 0x152988: csoundAuxAlloc (auxfd.c:53)
    >    by 0x26FC61: init_pp (bilbar.c:277)
    >    by 0x167507: init_pass (insert.c:116)
    >    by 0x168E5D: insert_event (insert.c:472)
    >    by 0x1680B0: insert (insert.c:298)
    >    by 0x17A098: process_score_event (musmon.c:829)
    >    by 0x17ABD2: sensevents (musmon.c:1038)
    >    by 0x14DC5E: csoundPerform (csound.c:2243)
    >    by 0x14A548: main (csound_main.c:328)

commit 44ac3f15e5fc59ac6849771c4a27442ae0d9ab3d
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Tue Feb 18 18:20:54 2020 -0500

    Fix Valgrind-reported jump/move dep on uninitialised values
    
    > 1 errors in context 1 of 1:
    > Conditional jump or move depends on uninitialised value(s)
    >    at 0x223FD8: sprints (ugrw1.c:448)
    >    by 0x224460: printsset (ugrw1.c:569)
    >    by 0x1674F2: init_pass (insert.c:116)
    >    by 0x168E48: insert_event (insert.c:472)
    >    by 0x16809B: insert (insert.c:298)
    >    by 0x17A083: process_score_event (musmon.c:829)
    >    by 0x17ABBD: sensevents (musmon.c:1038)
    >    by 0x14DC5E: csoundPerform (csound.c:2243)
    >    by 0x14A548: main (csound_main.c:328)
    >  Uninitialised value was created by a stack allocation
    >    at 0x224351: printsset (ugrw1.c:559)

commit fcba38bd53b4c2600c529ffeb20d1d0c0b540da0
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Tue Feb 18 18:00:32 2020 -0500

    Fix Valgrind-reported jump/move dep on uninitialised values
    
    > 878964 errors in context 76 of 76:
    > Conditional jump or move depends on uninitialised value(s)
    >    at 0x4EBF4E4: spoutsf (libsnd.c:83)
    >    by 0x4E81A9A: kperf_nodebug (csound.c:1790)
    >    by 0x4E830FE: csoundPerform (csound.c:2253)
    >    by 0x109953: main (csound_main.c:328)
    >  Uninitialised value was created by a heap allocation
    >    at 0x4C2FDAF: malloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
    >    by 0x4EAAE3D: mmalloc (memalloc.c:75)
    >    by 0x4E954EC: gen28 (fgens.c:1625)
    >    by 0x4E8F6BB: hfgens (fgens.c:236)
    >    by 0x4EB0A44: process_score_event (musmon.c:841)
    >    by 0x4EB1508: sensevents (musmon.c:1038)
    >    by 0x4E83036: csoundPerform (csound.c:2243)
    >    by 0x109953: main (csound_main.c:328)

commit f9dd9deaf25cba06acce963cb573451a3b8c0902
Author: Daniel Richard G <skunk@iSKUNK.ORG>
Date:   Tue Feb 18 17:04:32 2020 -0500

    Fix invalid pointer dereference
    
    The "ftp" pointer passed in to gen28() does not appear to be valid
    in some cases, and in any event, dereferencing it early serves no
    purpose as the "fp" variable is not used before being assigned
    a new value.
    
    > 1 errors in context 1 of 1:
    > Invalid read of size 8
    >    at 0x4E953E5: gen28 (fgens.c:1608)
    >    by 0x4E8F6BB: hfgens (fgens.c:236)
    >    by 0x4EB0A54: process_score_event (musmon.c:841)
    >    by 0x4EB1518: sensevents (musmon.c:1038)
    >    by 0x4E83036: csoundPerform (csound.c:2243)
    >    by 0x109953: main (csound_main.c:328)
    >  Address 0x42d8 is not stack'd, malloc'd or (recently) free'd

commit 921207f492f5937a9545694905667c81ba11b420
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Feb 18 21:51:08 2020 +0000

    A¬pply fix for uninitialied data; PR 1257 and 1258

commit 093d3fdd17ed6fc789d316e30627b371b987c404
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Feb 18 21:26:33 2020 +0000

    A¬pply fix for uninitialied data; PR 1257 and 1258

commit ae6dc653f5014bd424603b029a6a95692aee5f39
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Feb 18 20:25:15 2020 +0000

    A¬pply fix to macro expamsion wthat had an uninitialised value; PR 1256

commit 763129c2e72f203ccd4b65c621eccac905ff2a95
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Feb 18 08:31:08 2020 -0500

    fix for updated build artifact name

commit 9b1aa3557cc29b607468896b643f23f8fdbe584b
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Feb 11 17:34:43 2020 +0000

    fix to named instrumentsreverting change by VL

commit 52685783177ec3f97586eeb544df4b4a8080cabf
Merge: ba945e233 ec5d15640
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Feb 10 16:02:05 2020 +0000

    Merge pull request #1253 from giuliomoro/bela
    
    Bela: updated include folders

commit ec5d156401fbd4c65ce7c91a6c1375f9760be002
Author: Giulio Moro <giuliomoro@yahoo.it>
Date:   Sun Feb 9 23:55:27 2020 +0000

    Bela: updated include folders

commit ba945e233ad5be7fcce78773a7eb581dc72f27dc
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Feb 8 14:35:41 2020 +0000

    fix usage message re sorted scores

commit a392e5b0c14e8bbd32345cecf74f9f42fe894f28
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Feb 3 14:34:35 2020 +0000

    errors on mp3out build

