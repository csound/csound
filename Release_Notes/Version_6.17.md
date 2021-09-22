
# CSOUND VERSION 6.17 RELEASE NOTES - DRAFT - DRAFT - DRAFT - DRAFT 


-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- cntDelete deletes a counter object

### New gen

### Orchestra

### Score

### Options

### Modified Opcodes and Gens

### Utilities

### Frontends

### General Usage

## Bugs Fixed

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

commit b4e97606695cc99385f4eaff5a1fcdf2cd5cdafa
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Sep 9 12:04:23 2021 +0100

    some other message to errormsg

commit 8f23eea767cf6a9ab1a9f1d57885297c943e86e1
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Sep 9 11:37:18 2021 +0100

    moved message calls  to errormsg

commit a0a6dd2ac9a944436474b39f872ed8983bd0b3b4
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Sep 9 10:33:17 2021 +0100

    converted some messages to csoundErrorMessage

commit d18f394af052edb66b45d7964e4adbfc4bbe9284
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 16:17:08 2021 +0100

    making sure InitError and PerfError use ErrorMsg

commit d635ccba86340ff8bbb6a3b3eda822deecff3ec7
Merge: a5f93cbb8 602c040e9
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 16:05:55 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit a5f93cbb843c517f138374ce59a9ef3d3c41e547
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 16:05:51 2021 +0100

    formatting of engine param messages

commit 602c040e9a15ba63d47647c476ddea6f54bf29dc
Merge: 3a441a305 41608cd10
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Wed Sep 8 14:21:10 2021 +0100

    Merge pull request #1523 from gesellkammer/develop
    
    Some fixes regarding comman line options and conditional  printing

commit 3a441a305e91a032d2f3064852775acd51e908f1
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 14:06:39 2021 +0100

    use error messages

commit 41608cd10b095e517598d16f748f54648fcfe25e
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Sep 8 14:54:25 2021 +0200

    fix --version; fix samplerate mismatch in jack with --get-system-sr; change some warning messages to Warning; make more printing depend on msglevel

commit eb0dcb240d890b277b402d0c72bcf30292c45d2d
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 12:21:45 2021 +0100

    suppressing error messages with -m0 and not -v

commit db0f02810ad7a7f0d7555b36c455fd8c5152ff06
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 11:02:15 2021 +0100

    adding messages back to utilities

commit 88fd43578e4095b184dc0d99ffd5c12fca4ce2f4
Merge: 158946772 868dbee9f
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Sep 8 11:53:23 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 158946772ef488326ee77dcdc8f51ccee65535e3
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Sep 8 11:53:07 2021 +0200

    Revert "add env variable CSNOMESSAGES to avoid output prior to argdecode"
    
    This reverts commit d4ff16fddeb4acb2e29f71b79e1d326d3f30d4a0.

commit 868dbee9fc820e1f6950f3137151db13ed50e1b4
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 10:45:30 2021 +0100

    suppressing messages

commit f4c7ed006348ded11b63b166b4175efd6f28ad57
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 01:02:49 2021 +0100

    fixed windows build by removing pcr.dll

commit 0020ef735f3d579fedff787d16420f3f0ccf6169
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 00:48:08 2021 +0100

    fixed windows build by removing intl-8.dll

commit 38bd86e8034b9212b88d92eb2585dfc18a7f2314
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 8 00:36:42 2021 +0100

    fixed windows build by removing iconv

commit e3cb0f17873e6ac8f10642596a5ad418ee86d7ac
Merge: d4ff16fdd fbc05ef13
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Sep 8 01:23:47 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit d4ff16fddeb4acb2e29f71b79e1d326d3f30d4a0
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Sep 8 01:23:34 2021 +0200

    add env variable CSNOMESSAGES to avoid output prior to argdecode

commit fbc05ef133f06e0d1a0dfbeb382598bf864cd699
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Sep 7 23:33:54 2021 +0100

    fixed windows build by removing glib

commit ad924f478a322bc6ebbceb69316a817ebc118082
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Sep 7 22:33:24 2021 +0100

    removed fluid opcodes

commit 3e8ae7b84d48b8d625be6c9017fc2518487b2b03
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Sep 4 11:51:29 2021 +0100

    put jack header back in as it was removed by mistake

commit b35907392c31efce59c730a8b48f7c9fc74a3536
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Sep 4 11:50:03 2021 +0100

    moved jack out

commit 29f907049413cbf5a0adfe927cc7437cac322fac
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Tue Aug 31 01:43:27 2021 +0200

    Install vcpkg dependencies via cmake instlal

commit a421ead992cf82700b1b13776e4fec185d02f8f0
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Sat Aug 28 19:21:59 2021 +0200

    Remove old appveyor windows installer

commit f2412eaaf36a4ea5132194415d6c380ab78b5d5c
Author: John ffitch <jpff@codemist.co.uk>
Date:   Thu Sep 2 16:10:07 2021 +0100

    small editing error

commit c86941a8b20a661c893c7b591d74a46baaf5096f
Merge: daad3ef9b 3783d3b04
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Sep 2 11:53:43 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit daad3ef9b02914b7bfbaa73d53d3bbed36615879
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Sep 2 11:53:36 2021 +0200

    fix emugens

commit 3783d3b044c03f99320078061c2a8aea777fd9c9
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Sep 1 18:07:41 2021 +0100

    Updating for new plugins directory

commit 3db468b510dab5a4fc817bd668c0216373c467cf
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 1 17:07:49 2021 +0100

    fixed kflag parameter

commit aad335820086ece2c86743dbf6af482b0f10b03b
Merge: 9dd7184f4 8c6bd9ef7
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 1 06:51:45 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 9dd7184f463c4f0b0cec269909291f9181495a8f
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 1 06:51:37 2021 +0100

    typo fixed

commit 672aa9805c692a2cf31339243e72e528410c1e7c
Merge: e998601e9 8c6bd9ef7
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Tue Aug 31 11:10:18 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 8c6bd9ef7b3a8d9dcc3f17391d41f66517b14076
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Aug 30 21:02:12 2021 +0100

    deprecate xscan opcodes

commit 8dffe026d7d85879fccb8ae55285c34f85734ea9
Author: Rory Walsh <rorywalsh@ear.ie>
Date:   Mon Aug 30 13:44:58 2021 +0100

    adding lower case aliases to trigExpseg, trigLinseg

commit e998601e99fc9e987079e15e85c4e8e84d5a9640
Merge: c7be0f449 622dfb3b1
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Aug 30 13:44:49 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 622dfb3b1552bba357d8ab6bf4cdc6adaef2a7bf
Merge: 3ca4bbd2f 01433ca11
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 21:25:44 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 3ca4bbd2fb7a7200798b883200ae4d891f6edbcc
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 21:25:33 2021 +0100

    added instructions for builtin linkage

commit 01433ca110c90e3ce36b7e9d763b6ed52ef9e7f8
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Sun Aug 29 21:08:55 2021 +0100

    Update README.md

commit 397aa8583afd2545e343095c542ef90162941e24
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 21:04:55 2021 +0100

    removed emugens CMakeLists.txt

commit 3c74e383824d4302a321f8b54fcaaef595f734cb
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 20:48:12 2021 +0100

    emugens and scugens are internal now

commit eb1fbc0379f9e4753a8e6b92e874f192f0ed55b7
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 16:08:32 2021 +0100

    deprecated opcode lib

commit e4a4656399e742616e054f8206fc087d5a09b7f4
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 14:36:59 2021 +0100

    trying to fix winsock2 issues

commit 72d6bfd2ebb045909f2c1614c7a3cfe9794de76c
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 14:25:31 2021 +0100

    pvsgendy.c was already added

commit c6d34a7cc4097845b5ea1abfd49ab789fa6c3ad8
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 14:08:30 2021 +0100

    pvsgendy now internal

commit d350675afc58a70f25bf1b338422ad7222e00386
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 14:02:23 2021 +0100

    platerev now internal

commit 603152fc68199ed89ee0a26270e972b479af5968
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 13:56:15 2021 +0100

    counter is now internal

commit 60b367fe101838ca54fc48ffdc8ee2813b61c5ee
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 13:49:55 2021 +0100

    serial opcodes now internal

commit eddec99dbceed45d70e375469991f2cc0db83ab6
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 12:54:23 2021 +0100

    select is now internal

commit 6acb69a5702d175c0c42c3e9ff23801f39f7a817
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 12:48:52 2021 +0100

    buchla now internal

commit 2578719f75442c1607ec83b610e808570d388342
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 12:43:40 2021 +0100

    exciter is internal now

commit d824b1bc20f1661975896d526062b503ce3c1f81
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 12:38:15 2021 +0100

    cell now internal

commit fdf1db72bae76031a4fec66bdad5d20e9359c3ca
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 12:32:24 2021 +0100

    framebuffer now internal

commit 71255745d5d634b3f45aecfcdd0369a9c2d2f9e7
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 12:31:42 2021 +0100

    framebuffer removed from installer

commit e62a79be4720380df7a94a2a1e208a063a1f158e
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 12:11:56 2021 +0100

    moved fareygens to internal

commit f1ac14b2a8a45bd107021111c5b3a0c10543661d
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 12:05:51 2021 +0100

    gtf now internal

commit e0c47fa36700b9fddccc4a4171151e081b484d4a
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 11:59:36 2021 +0100

    adjusting android build script

commit 6dc7a3d824e0e81ecf614bc073604db7f85c97a0
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 11:56:33 2021 +0100

    liveconv now internal

commit a07177fcfca01bd3aacd8e01b89e64ab8c483a77
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 11:50:49 2021 +0100

    getftargs internal now

commit 33d069a313bec6203c5569e4ff534567989eafdc
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 11:33:39 2021 +0100

    date etc now internal

commit 4130ee45ac5df1a064e164631e87addb029b7d67
Merge: 37cea45f6 fca54c834
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 10:55:02 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 37cea45f60208a46c0df4e8b048a0054cb82a879
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Aug 29 10:54:52 2021 +0100

    sterrain is now an internal opcode

commit fca54c8347ea82699b51d9f30f0d0c94b1f6da70
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Sat Aug 28 19:16:34 2021 +0200

    Add in missing files from installer

commit 55d92b7fe0395586bb1a1a23c3d585fdfe0aadde
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 27 03:40:48 2021 +0200

    Add dist folder to gitignore

commit 42eb406577b9923498da3ba068e23e3c853fed35
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Aug 27 21:02:03 2021 +0100

    Implement scan(s)map opodes

commit cac6584abbf7f45e7fba7f60bec28c804b632167
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Aug 27 14:52:16 2021 +0100

    moved wiimote out

commit 9500220c78f5fd6275af0e93ea026e8efe19dfd3
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Aug 27 14:33:27 2021 +0100

    moved Opcodes/lame.c to the plugins repo as mp3/mp3out.c

commit 1a7a98b533a6972a4c4cd253793d120d616cc1aa
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Aug 27 14:27:43 2021 +0100

    fixed bug in pvcross

commit f93470c5006710eaf584e5fbf8703d31deaabb7d
Merge: fce810578 4fec517b2
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Aug 27 10:06:08 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit fce81057804660605b69eb800310bc74ac31db6c
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Aug 27 10:06:01 2021 +0100

    event now does not bail out when an instr is not found

commit 4fec517b28319f43f336ba73f0a16122bb85fbc0
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 27 00:04:10 2021 +0200

    Changed artifact names

commit 6d40572d6642ace93f595568344756f01417f2b0
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Aug 26 21:45:21 2021 +0100

    moved linear algebra opcodes out

commit 2082b38a96092665deb2e57b8d7c35ee884df22d
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Aug 26 18:45:52 2021 +0100

    tidying up

commit fafc5ef94064b329e22fff31940cafeef09abf9c
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Aug 26 18:07:43 2021 +0100

    commented out zlib1.dll

commit 6af36ca4c6eb7c7476438e63866b6ec884969f99
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Aug 26 17:43:06 2021 +0100

    moved hdf5 opcodes out

commit c6fd02c18bcd1315b32d2027ede081f2138b21e9
Merge: 0e1584c29 42df9e775
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Aug 26 17:37:39 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 0e1584c297748c891dbb19b51b6bbebf7d8514f9
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Aug 26 17:36:49 2021 +0100

    moved websocket opcodes out

commit 42df9e775fdba634f0adaead4a3a09483c75ec29
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 17:26:00 2021 +0200

    Remove appveyor config

commit 1c000244c2e53d87c445ebf5b3fc60847e0cc630
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 17:11:28 2021 +0200

    Switching to use context instead of env variable for run number

commit 8ed92f22a96fd18cf4183710c222626bb9cdef55
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 17:02:04 2021 +0200

    Added schmea to vcpkg manifest to aid autocompletion

commit 5464522e20135030c7c9c3e65fbb2f50c5c1b95d
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 16:47:52 2021 +0200

    Another fix attempt at syntax

commit 14de09e77f552365949cfc7c0be79922c0cf034f
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 16:03:19 2021 +0200

    Remove unused syntax

commit 4b49d962be883caffca50acd2837431fd1f3698d
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 16:02:33 2021 +0200

    Revert syntax

commit bbedc19d9da2ecceeeccd850922cb5bf2e93713d
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 15:48:49 2021 +0200

    Another attempt at getting the syntax right

commit 2cfeaef388c1702472fa0fe1a6baedaef53d491c
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 13:32:40 2021 +0200

    Fix syntax for env var

commit 00f5955cc056c96d7d75014f8139c0b7b8d8c95b
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 03:43:21 2021 +0200

    Fix name and fix include directory included for binary zip

commit 0cf68bff7cf5341213e250909d4a862393281519
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 03:22:50 2021 +0200

    Set paths as relative in artifact upload

commit 0577374dbc176a024b57714d71d7846bdbc29687
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 02:58:13 2021 +0200

    Another attempt at fixing the wildcard

commit 9af8f4d29c9bbb09d621056737f7e51a25943b02
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 02:40:22 2021 +0200

    Fixed syntax

commit dc6ed883d343242f91ca29095162b5807f8cdf4d
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 02:39:10 2021 +0200

    Another attempt at fixing the wildcard

commit 911b7efcb00fb2051ca4c271335083b18e03ddb0
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 02:19:22 2021 +0200

    Fix wildcard for zip artifact

commit 7b137a53fff32e211817be5753781a722ce967f3
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 01:50:14 2021 +0200

    Fix windows msvc build instructions link

commit cffc1abe0a65a0608121c9e7ea28944b9861dc5c
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 01:44:29 2021 +0200

    Generate binary zip in upload action directly to avoid double zip artifact

commit e05d1c4f7cb69bf5757be68236528eb9836d1e13
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 01:41:47 2021 +0200

    Updated csound version in github action workflow

commit 15aeae29a8b4f12783d01d7b2b287915f308796c
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 01:39:46 2021 +0200

    Fixed build count for zipped binaries. Should be run_number not ID

commit 45988c1fd854b24cd6f02663035a99b84f959e7d
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 01:35:38 2021 +0200

    Swig is included on github action runner so no need to install it

commit e4fd16a156f10e6d065cb8e51211e5eab0a02dd7
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 01:10:17 2021 +0200

    Updated windows build readme

commit e07d1ef476d639692dba055ce403e256fd73a4ce
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 26 00:08:15 2021 +0200

    Deleted MSVC folder but moved README to new platform dir

commit a592477f134e923428e4f6564d9e0e967f2e050a
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 25 22:38:12 2021 +0100

    few fixes to warnings

commit dde5785b0ee0cdfdcd803e4774b726c22b8b5df9
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 25 22:08:10 2021 +0100

    fixed CMake script

commit 58321ad406235312655da4c26768c8dbac12768d
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 25 21:56:50 2021 +0100

    moved opencl opcodes out

commit d06764da4949d9ca2477e8c418e21bed1f77138b
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 25 21:47:36 2021 +0100

    move CUDA opcodes out

commit c7be0f4490a2e87100feafb3f5bcff9708be66d7
Merge: 054e92eb9 ae26536de
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Aug 25 20:13:03 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit ae26536de8ba59aab7a0db12214ec1e02621a642
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 25 14:03:44 2021 +0100

    removed appveyor badge fully

commit 58c19bd3c5e7c534b2464b5463029a63f73d5af7
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 25 13:32:39 2021 +0100

    removed travis badge fully

commit a9566164f8c59e7dc0c13c71eddac61a4f1d01c2
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 25 10:53:41 2021 +0100

    set updated version

commit ef27c3ad62ad69aa985b7d8997d4c1678d71d901
Merge: cbf244174 bb21f2230
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 25 10:51:44 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit cbf244174154dbe43496bb9a98c83e0c83664941
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 25 10:51:20 2021 +0100

    commented out coverity badge

commit bb21f22306bdb5c4ea4f6d4033624828917aee12
Merge: 42446f1ac 06ea94d10
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Aug 25 01:24:11 2021 -0400

    Merge pull request #1509 from csound/feature/vcpkgImprovements
    
    VCPKG/CI improvements

commit 06ea94d10b0874f43c950eef9dd0624b6bb9f697
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 25 02:58:37 2021 +0200

    Fix labels

commit ddac31d155cfed611731cf5f5d0731910a258240
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 25 02:42:28 2021 +0200

    Port recent changes to installer

commit 7f8945a52a04aac6634f87ee52b7dd3552ca1390
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 25 02:38:59 2021 +0200

    Fail build if no artifacts found

commit e663ff97eff97c32bcf71813a4cd54f442bbc487
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 25 02:30:04 2021 +0200

    Add labels to upload steps

commit 25e0c1e7eb85cf569708203ea6eeec1b2bfa85f6
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 25 02:18:47 2021 +0200

    Revert linux updates

commit 6ed3dc73cd35513a50ce26b95b4d1d11be7ef8c8
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 25 02:16:56 2021 +0200

    Remove commented code

commit a31a0202769c95f7539fab967f48c1f552fa4fbd
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 25 02:16:35 2021 +0200

    Move folders around

commit a44db5b28feb53744753cf585865d0c2345b56e2
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Tue Aug 24 03:02:49 2021 +0200

    Fixed wildcard for installer

commit 28dda730a04e984252834ca7470b7e7cc85a7bd7
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Aug 23 17:02:08 2021 +0200

    Another attempt to get the artifact upload working

commit f0e1af34bbd180fccb45054b6372a0a465cf3620
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Aug 23 16:18:18 2021 +0200

    Cleaned up installer output and action to find and upload

commit 7310718089056ef6de623bfe2ab15fd33176c59f
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Aug 23 02:32:27 2021 +0200

    Upload windows artifacts on build completion

commit 6a6cd48e66d46fe7dbd771c6b10d1c9d9dd2fae3
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Aug 23 02:08:01 2021 +0200

    Added custom for for portaudio that downloads asiosdk

commit dcab8a52a8b7fc131d92df9132e5857d5235e1e4
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 18:25:28 2021 +0200

    Fix github action env var

commit 49bc0a28e9b9e4492df866e094f5ee4ebbdc9ed2
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 18:06:19 2021 +0200

    Ensure building for release

commit 94d93f17947ba08faf7890be7958b76bc1c66593
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 17:49:58 2021 +0200

    Windows installer fixes

commit 6d3bda60c4a4aa88aa6d8ecb84aa4d607599e496
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 17:14:54 2021 +0200

    Action runner uses enterprise VS, not community

commit 0eda06fa16017907e2adc0b5435d9dfb2541a427
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 16:53:26 2021 +0200

    Fixed path for manual download in action

commit 39d71a9793a178d927b1f528e1cc360a9b006cf1
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 16:52:55 2021 +0200

    Add html/manual folder to git ignore

commit 7961e7ebe3b628887d29408a476552baeb00800b
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 14:59:26 2021 +0200

    Update comments in new installer script

commit 9e4ba47f67638befd81f00b1b75d0ec13eec086e
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 14:57:22 2021 +0200

    Fixed up some installer related commands

commit aa828ffe86dbc05dcd08a2ace2a698854bbc3450
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 01:50:58 2021 +0200

    Remove unneeded change directory command

commit c23d139330cd01ca1e6da3569a9d5d4e078fb458
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 01:42:11 2021 +0200

    More powershell syntax fixes

commit 38330c6d3afa1620213889f458b43891e23e3da0
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 01:34:36 2021 +0200

    Converted to powershell syntax

commit 92968e10b9ca33b89b13d533bb6de1ce17c65455
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 01:12:36 2021 +0200

    Added in VCRedist file locations to installer

commit 041833a11c1ecf3b105072a29457d91b5939e437
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 01:11:22 2021 +0200

    Added installer script for github action

commit 42b82cf1ccfde3b112d0f725c5539a7c40e7a4f2
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 00:18:54 2021 +0200

    Fixed powershell call. Remove custom appveyor call

commit c4d379e637ee0f03325878eab78e3d656927f1b9
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 20 00:01:00 2021 +0200

    Revert back to explicit manual download for now

commit 83ac1e188d7845249fce49d1de5954dfd88a0a0d
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 19 02:39:39 2021 +0200

    First attempt at linux build with vcpkg

commit e18214a1b75dd9bd436e331fa09721716341f147
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 18 13:24:14 2021 +0200

    Fix URL for action step

commit 503251aab8cd5b59709c7b0d65bef21ea8834025
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 18 04:16:19 2021 +0200

    Missing argument for action

commit 0b629f7d2867ba54c0ec31c5bf4bb2a109f0de8a
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 18 03:54:38 2021 +0200

    Use github action to download manual

commit e17a8b344ef5e23fd2d52816c0f09696f49ee579
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 18 03:27:12 2021 +0200

    Add build labels to github actions

commit ee8488c721a3ac158e90a357d2f5e54c42c0feb6
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Tue Aug 17 17:30:00 2021 +0200

    Whitespace fix

commit 9480c23419f8b36e37af47ae40d4125a5f1b379e
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Sun Aug 15 01:33:43 2021 +0100

    Add start of installer packaging for windows

commit e8f85a99c9d83ebb0e22b13f03710df86dbd038f
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 11 01:40:12 2021 +0100

    Updated triplet directory in downloadDependencies.ps1

commit 548ff07580af98566bf5c857c4d0ee60c23089ad
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 11 01:33:47 2021 +0100

    Added space in cmakelist

commit ca012045068af1bdbc16d69659501f69a9451cb1
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Aug 9 14:08:47 2021 +0100

    Revert "Remove appveyor config"
    
    This reverts commit 35081fc33d330fea49d1b868f781f06f0f746b29.

commit b1bac551a5d7c4931327e70fc2e3ab4e1c8ec4d0
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Aug 9 02:20:32 2021 +0100

    Enable vcpkg versioning by adding baseline

commit ddaa60fdb3a386b3d851d8c3415a28f9c96269c0
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Aug 9 02:07:05 2021 +0100

    Add in custom cmake for windows gh action

commit 79a5420475ac1af977efa9883ad1a48bde1d5358
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Aug 9 01:55:23 2021 +0100

    Cleaned up custom-vs file

commit 5b58b6142da737c462dfb5f88a0b34bd121714ba
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Aug 9 01:52:31 2021 +0100

    Remove explicit vcpkg triplet in gh action

commit 73c31f049980c0404f872810004223d28e0e9fcb
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Aug 9 01:51:25 2021 +0100

    Detect vcpkg triplet based on cmake environment.
    
    VCPKG must be explicitly enabled for project to avoid breaking existing builds

commit 45eb6fb639b81be1833dc3f9fc906ee858b4e6e0
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Sun Aug 8 13:26:39 2021 +0100

    Moved custom-vs.cmake to new folder

commit cdd87d7d09a6879c7963cc5d6ae91b9276d1020f
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Sun Aug 8 13:00:10 2021 +0100

    Bootstrap vcpkg manually

commit a6c236383678dea135bdd771a6735b739de608cc
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 6 02:42:58 2021 +0100

    Add in nuget binary store for vcpkg

commit 8eab475af90fb242dbc764c20ccb7ad63883281f
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Fri Aug 6 00:37:11 2021 +0100

    Update triplet path in cmakelist

commit 6980163f9944c8626ff178d734bd8fa667fe9ce8
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 5 18:42:15 2021 +0100

    Moved triplet file to new folder

commit 3b6c1175d34a72881a1b64a89a506027c103eddb
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 5 02:18:27 2021 +0100

    Download submodules for windows build action

commit 9e540a85a26287dd5502c2b9a01ca54141be053b
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 5 02:14:20 2021 +0100

    Fixed config formatting

commit 6f1fe6439e65352774418cebcbc2004eae49ed73
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 5 01:44:44 2021 +0100

    Fixed indentation for github action file

commit bf30b9e0f97bc3a39cdc5b5aa4e61fc547ca9b0e
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 5 01:44:24 2021 +0100

    Fixed editor config style for autoformatting tools. Should be 2 spaces indent for yaml

commit 980f4b566a00e996cc4349e9d786cbc7f1f99747
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Thu Aug 5 00:08:48 2021 +0100

    Remove appveyor config

commit 8bd411906629808a37691ddd2bfef803d2b7bb6b
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 4 23:29:24 2021 +0100

    Fixed CI label

commit 73fa1d60145aa27723f3b8002a95d0ec41f602d7
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 4 01:12:55 2021 +0100

    Fixed syntax

commit 6ad01bf8b9f83f7ff517971f61b39a404d4e0cbc
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Wed Aug 4 01:00:31 2021 +0100

    Added chocolatey dependencies to github action for windows

commit b89eebc33cebe142026d56af3489c019a00361a9
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Tue Aug 3 03:44:11 2021 +0100

    Fixed vcpkg in cmake lists. Commands must be passed in rather than inferred

commit 39ff22837bef69a5cabbc93a1c4ed18297be2f5e
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Tue Aug 3 03:43:25 2021 +0100

    Added first windows build commands to Github Actions

commit 527da040e79003bcab98ed2d94f294567289bdc0
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Tue Aug 3 03:01:26 2021 +0100

    Now using internal VCPKG with manifest and auto installation in CMakeLists root

commit eef29518ba1fa4ad28ecf58155fa577a38f04e9e
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Tue Aug 3 03:00:18 2021 +0100

    Added vcpkg manifest

commit 7b7fc8ffa0410af824b7aa500279e03e036e5e19
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Sun Aug 1 14:40:45 2021 +0100

    Added vcpkg as git submodule

commit 42446f1acf92cfa75f8d807be41154b2f49c0d0e
Merge: 4103ec954 318f8520f
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Aug 24 20:08:06 2021 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit 4103ec954e899bb1aedf0ad666f22bf522d9d9da
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Aug 24 20:07:31 2021 +0100

    removal of winsound

commit 318f8520f2f8f80e97ba4eba1ea5b52ed555da0d
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Tue Aug 24 17:59:37 2021 +0100

    Update Custom-vs.cmake
    
    fix typoes

commit 887c06b79df6f7c21f97c7bbeddaef15faee5e07
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Tue Aug 24 17:59:13 2021 +0100

    Update Custom-vs.cmake
    
    removing options that do not exist anymore

commit 3f023eb83400715dc285e727dd3f38c335f9295d
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 17:34:56 2021 +0100

    moving STK opcodes to plugins dir

commit a1e0153011a1c2927a4c09b37dbe274c858c73bf
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 17:14:21 2021 +0100

    removed code that has been taken to plugins

commit 974a1ac9e61199841a1fa066dfe4be656141f6e2
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 17:04:08 2021 +0100

    moved ALL fltk stuff to plugins

commit 4fa13b85869f4fa87d255845feab3df3eb207bff
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 16:45:24 2021 +0100

    fixing appveyor install 2

commit 62b193a8bfd5b2064a13e12947ebf10dfcbee0bc
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 16:32:33 2021 +0100

    fixing appveyor install

commit 3ebe5ce2dd5a4886751b9dedb2a4e9209c625b97
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 15:22:29 2021 +0100

    removed obsolete cmake function

commit 37900b5fe61558d5297346bdea3fb15e31707a81
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 15:15:54 2021 +0100

    removed chua build

commit a488a56a4825c91fae1936eda269b54950521db1
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 15:11:25 2021 +0100

    trying to make LINUX be set correctly

commit b37553c834951bfab0473685615240421f3e0899
Merge: a6a17c555 45d25a0cc
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 14:50:24 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit a6a17c5550f06c9323d02c1fcc0b09c94b5a7d63
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 14:50:11 2021 +0100

    tidying up cmake scripts

commit 45d25a0ccb6689e49020f55ae6991e0fceb99075
Merge: f8eeb9a90 26b9d8d8e
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Tue Aug 24 12:07:45 2021 +0100

    Merge pull request #1517 from gesellkammer/develop
    
    Add channel count to list_audio_devices

commit 054e92eb918d79e1522bc31754b2640c52330fa4
Merge: 26b9d8d8e f8eeb9a90
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Tue Aug 24 11:19:15 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 26b9d8d8ee9bd511e21e51c9fc79ee95e6f9fc8a
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Tue Aug 24 11:18:06 2021 +0200

    add channel count to list_audio_devices (called when the flag --devices is used so that it can be parsed by frontends

commit f8eeb9a907deb1b888a9a1272810366ce99623b5
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 24 00:11:19 2021 +0100

    adding fixed version.h for Android

commit f7aae52b1dc40d52feda68c867d8805fe1b983ae
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Aug 23 23:42:20 2021 +0100

    version setting from CMakeLists.txt

commit 65b3bc2894d9b36a9b991c026cb06a85b4443e59
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Aug 23 12:21:57 2021 +0100

    adding Info.plist

commit 99129eabfcca8f9ab3823c5c2b810474a984c98d
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Aug 23 10:16:48 2021 +0100

    updated installer PKG to take accountof csound version

commit 5d5054e3d3691f633ba253b9a3095e0d3377e1b5
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Sun Aug 22 08:55:32 2021 +0200

    Updated for new API thread creation function

commit 18898104f5d63976e7d3ccd553056e0b8b1e6485
Merge: 2e0cb6033 fa96a0977
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Aug 21 11:21:59 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 2e0cb60338f87fefd0f2e9d2dad413b381fd201f
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Aug 21 11:21:47 2021 +0100

    new API function for thread creation

commit fa96a097792f0c5a23c585ac94c6669f3bd726ab
Author: John ffitch <jpff@codemist.co.uk>
Date:   Thu Aug 19 21:28:34 2021 +0100

    Add castsaize_t -> int

commit 39425d95f812da05f88aa9ac6568cd8f3a59eb43
Merge: 7d56c51e4 2b1c55097
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 18 15:52:56 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 7d56c51e42f2f5c40df7d002ab72f692e4b1de23
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Aug 18 15:52:51 2021 +0100

    adding 32bit user plugin defaults

commit 2b1c55097f8b27a443dc19c661a4febba3199d59
Merge: df8557c69 3160545c8
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Wed Aug 18 10:34:35 2021 +0100

    Merge pull request #1514 from gesellkammer/jackports
    
    Fix jack autoconnect

commit df8557c6916692fc489b4690a2d9066b70122813
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Tue Aug 17 17:31:24 2021 +0200

    Add static csnd6 lib to installer

commit 296dad403e1696a9537c4ea8f2bf08f31020c5a0
Merge: c4ec4d3c6 0437ca7e0
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 17 15:30:05 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit c4ec4d3c67381b3d67938e90d7ac0a96cc7845a4
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Aug 17 15:29:56 2021 +0100

    adding static libcsnd6.a

commit 3160545c8f324e2b23d99c09c54c7946f4fac892
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Tue Aug 17 10:05:15 2021 +0200

    remove natsort (GPL), replace with alphanum (MIT)

commit 0437ca7e0dc9cf350ae7837081b9bb5b00469042
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Tue Aug 17 00:30:22 2021 +0200

    Duplicate csnd6.dll in installer, switching to include csnd6.lib in lib folder

commit ac0178344c9d579392da5dcafd03d71920e1d3b2
Merge: a8a8ff406 b5491fdbe
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Aug 16 23:16:05 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into jackports

commit a8a8ff406ced388ae770bae3b29622b4628a8671
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Aug 16 21:16:47 2021 +0200

    Better support for autoconnect to jack ports

commit cfddb7ecf8800500bffa3a2ce53c73640864f463
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Mon Aug 16 17:13:16 2021 +0200

    use regex to select ports, via jack_get_ports

commit b5491fdbe2d75916418dc275002f71dbfd5f7991
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Sat Aug 14 17:50:59 2021 +0100

    fix sha key

commit cc70aa1b8a98ba69bab21d6d6968da876b0d1f80
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Sat Aug 14 17:22:09 2021 +0100

    Update package-lock.json

commit 0ff6aa5c87339a4d98e3991ea0362c7a6a7149d9
Merge: 7584f01c5 4999e96ad
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Aug 13 00:36:49 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 4999e96ad94e822e5ce4f48406ddf2ccce0512db
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Aug 12 15:30:59 2021 +0100

    updating raspian instructions

commit 45290c093470a7713669dd100f78069b8ed99c70
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Thu Aug 12 12:12:17 2021 +0100

    named instrument async behaviour

commit 7584f01c510b6d8fb6b262a4862c01489322bbdc
Merge: b7afbac80 69af43889
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Jul 30 08:00:58 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 69af43889980b6e0c3342c878b8d02f064d8cc4a
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Jul 28 18:06:51 2021 +0100

    fix for hrtfmove

commit b7afbac80f68cb5dd3cac9b13000f372d77899b0
Merge: 1450fa2c6 0307d5372
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Jul 28 06:59:29 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 1450fa2c6259bf9a5d61c6955d5e4a279016ea64
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Jul 28 06:59:15 2021 +0200

    add libm

commit 0307d537276411a710e36364262797b5e0f6037a
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jul 25 18:32:49 2021 +0100

    Quieten midi messages

commit a5d9b9e30e52a2cd399aa4b32ddd31aff45e94c6
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jul 23 21:46:33 2021 +0100

    fix one problem in ATS cde but may have revrtd an earlier fix

commit 9427d1899012880f6f1cae2da01a1755d23affcd
Author: Francois PINOT <fggpinot@gmail.com>
Date:   Fri Jul 23 14:24:03 2021 +0200

    Minor layout fix

commit e2b41806635535145032f397683f02a27a507f1e
Merge: 872e290c9 d4011ce18
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jul 23 09:36:15 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit 872e290c9bfb9a6963c95494e875dfb9a7cee1f1
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jul 23 09:35:53 2021 +0100

    fixing name of lib

commit d4011ce18b1040e1a5c368d25a94976d84feac3d
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Jul 21 21:39:21 2021 +0100

    Fix file reding error in ATS opcodes

commit e12b6b5e4179bc8fc3bb089aafc887e2a74bb432
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Sat Jul 17 20:52:10 2021 +0100

    Update README.md

commit 745c8909061069ea1db66f77be28e48e73809e96
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Sat Jul 17 20:44:14 2021 +0100

    Update README.md

commit 3f60cd30ebd15a726ebafef318a1b6ab03396c55
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Jul 17 12:39:11 2021 -0400

    fix portaudio dep

commit 2d9969ac8311f352f4e36c3d135d319e741a9b89
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Jul 17 12:38:07 2021 -0400

    working on macos build

commit a8b1d30f85d8942b35b0807a405c275aa2fca126
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Jul 17 12:25:17 2021 -0400

    initial macos build

commit 0c9620604b388b5513893dd225dca8c96fda5d76
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Jul 17 12:04:03 2021 -0400

    fix building in build folder

commit b21eee2f166426b9a3d296ed015991ed056c58c5
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Jul 17 12:01:53 2021 -0400

    make apt-get install one-line to get around newline issue

commit 1ead73b8d0d837fd17f7c7aad64e6452b3f65c1d
Author: Steven Yi <stevenyi@gmail.com>
Date:   Sat Jul 17 11:53:46 2021 -0400

    initial work on Linux CI build with Github Actions

commit 4cddf36767c4a3d05d3787d5d0e42d35a1e6e1e1
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jul 17 10:36:06 2021 +0100

    fix version

commit 23ac32c822a3316cf4e4955e969d001025cac288
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jul 17 10:22:50 2021 +0100

    fixed merge

commit 58ec33038a56870467b068adebf09530229e4a9e
Merge: d7dbda72f fb5bdb368
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jul 17 10:07:23 2021 +0100

    fix conflict

commit fb5bdb3681e15f56c216b4e4487b45848aa6b9f4
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jul 17 09:49:23 2021 +0100

    updates to actions

commit 2fe4d95a74fb5209b16b7b398b2a5ac476014376
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jul 16 22:24:59 2021 +0100

    reverted code in CsPerfThread to fix recording issue

commit d27661c70cb904da77d0eff625354bd6a6f85987
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jul 16 21:24:34 2021 +0100

    fixing installer on windows

commit d7dbda72f136eb42176f7b539661e88bb93a36ec
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Jul 16 20:54:50 2021 +0100

    restoring csnd6.dll in installer

commit a572425120cd7383023063743e9df9d294bd8f4f
Author: John ffitch <jpff@codemist.co.uk>
Date:   Fri Jul 16 18:26:51 2021 +0100

    Fix top strcat; beter setseed

commit ba16b7555a25d58997ba677e91da239256cbcf2c
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Jul 14 09:17:32 2021 +0200

    optimize sum

commit 2f2e91ac9f79b5d3fe8be09e5807aa962309caa2
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Jul 12 21:01:10 2021 +0100

    sample-accurate fix

commit 27355b8531f67d826ebb68505e0c86d7ec5eb5eb
Merge: b3daa4574 b0e7e7b03
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Jul 11 13:01:07 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit b3daa45749b916b4b6d028f3801bca50fc66b15b
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Jul 11 13:01:02 2021 +0100

    background for installer

commit b0e7e7b0323a7a09aa4e5a9c34515de9934627b8
Merge: 3ef346a9c 4a707c81b
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Sun Jul 11 12:43:12 2021 +0100

    Merge pull request #1501 from gesellkammer/sumarray-simd
    
    Sumarray simd

commit 4a707c81b9dff3f6aef209aafcca97f90a66471f
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sun Jul 11 12:27:23 2021 +0200

    simd friendly version of sumarray for audio arrays

commit 76cc7d74b9fe6f2ec80d8e12620eef41df2d6e05
Merge: 8842d3db1 3ef346a9c
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Fri Jul 9 20:56:51 2021 +0200

    Merge remote-tracking branch 'upstream/develop' into develop

commit 3ef346a9c4342f539a06e2e372318f7001475ced
Merge: f6637398c b2355f454
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Jul 4 19:40:59 2021 +0100

    Merge branch 'develop' of https://github.com/csound/csound into develop

commit f6637398c999060acd36e1d8eb032c73d979590b
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Jul 4 19:40:47 2021 +0100

    issue #1488

commit b2355f4544456e49868de5e0eecce2c8584ad56d
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jul 4 17:40:02 2021 +0100

    cvanal to SADIR

commit a89560b28bc67d31ca5c51137d8c1d7d0a5061c2
Merge: d1134a11c b1a020538
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jul 4 17:37:23 2021 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit b1a0205388e754c3617ef8d6cebf15f9247678de
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Sat Jul 3 19:48:22 2021 +0100

    Update csound_builds.yml

commit 8842d3db157498d53334a0fe469b66800fcd4fe5
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Sat Jul 3 20:24:46 2021 +0200

    fix bpf binary search

commit d1134a11cf5dfe4a29a79b0a4d85701f7bb51f30
Merge: 057ab23b9 d606868f4
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Jun 30 19:36:48 2021 +0100

    Merge branch 'develop' of github.com:csound/csound into develop

commit d606868f4524eda9f78c6147b68de38bd7697c3b
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Wed Jun 30 10:30:31 2021 +0100

    Update csound_builds.yml
    
    update zip name

Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Jun 29 22:36:27 2021 -0400

    updated for 6.16.1

Date:   Tue Jun 29 22:45:31 2021 +0100

    6.16.1 version

Author: John ffitch <jpff@codemist.co.uk>
Date:   Sun Jun 27 19:45:57 2021 +0100

    Cleaner contro oer lpslots

commit 4406446e51177388ba783ec42a2be9d3175a4aa2
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Jun 26 23:12:57 2021 +0100

    silencing lpread warning messages

commit 878ecc00a424d7ab175941ad37c36b1bf10a70d3
Author: John ffitch <jpff@codemist.co.uk>
Date:   Sat Jun 26 20:56:37 2021 +0100

    fareylen has no init function so should have 2 not 3

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

commit b69857b847bcfe66821a02e865b08eaf84c9abde
Author: John ffitch <jpff@codemist.co.uk>
Date:   Mon Jun 21 16:57:44 2021 +0100

    move memoryy allocation to intfunction for bformdec2

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

