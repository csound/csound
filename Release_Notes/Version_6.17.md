
# CSOUND VERSION 6.17 RELEASE NOTES - DRAFT - DRAFT - DRAFT - DRAFT 

Mainly a bug-fixing release but also a major reorgnisation of the
libraries to move all opcodes with dependencies into a plugins
drctory.  This is in part a prearation for Csound7.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- scanmap and scansmap are like the xscanmap/xscansmap opcodes but
  work with the mainstream scan opcodes.

### New gen

### Orchestra

### Score

### Options

### Modified Opcodes and Gens

### Utilities

### Frontends

### General Usage

## Bugs Fixed

- fareylen called a non-existent functionleading to a crash.  Removed typo.

# SYSTEM LEVEL CHANGES

### System Changes

### Translations

### API

### Platform Specific

==END==
    
========================================================================
commit 15fed58bd13353197c401d90eb47b79e46743498
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Sep 22 21:27:56 2021 +0100

    Strings

commit 91b3480cafe6209b1ded101762114df98b6ed969
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 22 18:18:15 2021 +0100

    fixed order

commit 37e39766fbeb1112c60244d3e9a450c487b2261b
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 22 16:31:16 2021 +0100

    memory allocation size

commit 3c784ffb6e0053608a189875176be972f78c3820
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 22 14:18:06 2021 +0100

    fixed bug in resetting framecount on pvscfs

commit cc282166c70d4d6839335707ea535125b08f9825
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Sep 20 09:07:36 2021 +0100

    printing version

commit af28d890a7edd4af01e283429ebbf89ac5f7ed8a
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Sep 14 17:20:08 2021 +0100

    fixing messages

Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Sep 8 14:54:25 2021 +0200

    fix --version; fix samplerate mismatch in jack with --get-system-sr; change some warning messages to Warning; make more printing depend on msglevel

commit eb0dcb240d890b277b402d0c72bcf30292c45d2d
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 12:21:45 2021 +0100

    suppressing error messages with -m0 and not -v

commit ad924f478a322bc6ebbceb69316a817ebc118082
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Sep 7 22:33:24 2021 +0100

    removed fluid opcodes

commit daad3ef9b02914b7bfbaa73d53d3bbed36615879
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Sep 2 11:53:36 2021 +0200

    fix emugens


commit 3db468b510dab5a4fc817bd668c0216373c467cf
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 1 17:07:49 2021 +0100

    fixed kflag parameter

commit 8c6bd9ef7b3a8d9dcc3f17391d41f66517b14076
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Aug 30 21:02:12 2021 +0100

    deprecate xscan opcodes

commit 8dffe026d7d85879fccb8ae55285c34f85734ea9
Author: Rory Walsh <rorywalsh@ear.ie>
Date:   Mon Aug 30 13:44:58 2021 +0100

    adding lower case aliases to trigExpseg, trigLinseg

commit 1a7a98b533a6972a4c4cd253793d120d616cc1aa
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Aug 27 14:27:43 2021 +0100

    fixed bug in pvcross

commit fce81057804660605b69eb800310bc74ac31db6c
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Aug 27 10:06:01 2021 +0100

    event now does not bail out when an instr is not found


    removal of winsound

commit a488a56a4825c91fae1936eda269b54950521db1
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 15:11:25 2021 +0100

    Merge pull request #1517 from gesellkammer/develop
    
    Add channel count to list_audio_devices

commit 26b9d8d8ee9bd511e21e51c9fc79ee95e6f9fc8a
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Tue Aug 24 11:18:06 2021 +0200

    add channel count to list_audio_devices (called when the flag --devices is used so that it can be parsed by frontends

commit 2e0cb60338f87fefd0f2e9d2dad413b381fd201f
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Aug 21 11:21:47 2021 +0100

    new API function for thread creation


commit 45290c093470a7713669dd100f78069b8ed99c70
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Aug 12 12:12:17 2021 +0100

    named instrument async behaviour

commit 69af43889980b6e0c3342c878b8d02f064d8cc4a
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Jul 28 18:06:51 2021 +0100

    fix for hrtfmove

commit 0307d537276411a710e36364262797b5e0f6037a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jul 25 18:32:49 2021 +0100

    Quieten midi messages

commit a5d9b9e30e52a2cd399aa4b32ddd31aff45e94c6
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jul 23 21:46:33 2021 +0100

    fix one problem in ATS cde but may have revrtd an earlier fix

commit ba16b7555a25d58997ba677e91da239256cbcf2c
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Jul 14 09:17:32 2021 +0200

    optimize sum

commit 2f2e91ac9f79b5d3fe8be09e5807aa962309caa2
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Jul 12 21:01:10 2021 +0100

    sample-accurate fix

commit f6637398c999060acd36e1d8eb032c73d979590b
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Jul 4 19:40:47 2021 +0100

    issue #1488

commit b2355f4544456e49868de5e0eecce2c8584ad56d
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jul 4 17:40:02 2021 +0100

    cvanal to SADIR

Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Jul 3 20:24:46 2021 +0200

    fix bpf binary search

Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jun 27 19:45:57 2021 +0100

    Cleaner contro oer lpslots

commit 4406446e51177388ba783ec42a2be9d3175a4aa2
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jun 26 23:12:57 2021 +0100

    silencing lpread warning messages

commit ab5656771af70cd603f7ccacc3da4d2672189abc
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jun 26 20:34:50 2021 +0100

    fix of turnoff issue

commit 0c4ff5b489950f9523600215d872b7d18ffb0c1a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jun 25 21:48:12 2021 +0100

    Better fix to lpslots in lpinter

commit e653f5e094ba6e1b3b4a3b88ecc164a6d7131007
Merge: 7e71c899c 67c32d7ad
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Fri Jun 25 09:56:00 2021 +0100

    Merge pull request #1494 from gesellkammer/develop
    
    Add an optional prefix to soundfont instruments printed via sfilist

commit 6aa12adc63e8bde47c4830e24214f39337efd95d
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Jun 22 17:01:37 2021 +0100

    fix to init method in cntReset

commit aa87598a50490ffa13212dc3425a69740746fa28
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jun 20 17:20:07 2021 +0100

    possible fix to turnoff3

commit 41edad1bf663b2c303c36b92fa26b3fcf309d6a9
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jun 18 17:46:18 2021 +0100

    fixed issue #1490


commit 35f83dacf126d0d8c2011f716d3177725778e4c9
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jun 18 09:50:58 2021 +0100

    fixed issue #1489

