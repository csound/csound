CSOUND VERSION 6.18 RELEASE NOTES

Mainly a bug-fixing release.  Major new facility is MP3 (MPEG) audio
files support is now implemented for both input and output with libsndfile.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- scanmap reads internal state of scanu

- elapsedcycles, elapsedtime, eventcycles, and eventtime. See under
  "bugs fixed" for details.

### New gen

### Orchestra

### Score

### Options

### Modified Opcodes and Gens

- Fixed midi list printing to stdout part of the list.

### Utilities


### Frontends


## Bugs Fixed

- Fixed bug in sum.

- 'pitchamd' opcode causes segmentation fault (macOS) (#1599)

- Fixed crash on channel setting with empty channel name

- Fixed bug in ftloadk (#1611).

- Now, qnan works even when compiled with fast arithmetic.

- Fixed the flanger opcode for sample-accurate mode, where the xdel input was
not being correctly offset.

- elapsedcycles, elapsedtime, eventcycles, and eventtime (introduced as
  fixed versions of timek, times, timeinstk and timeinsts) now return
  the correct values instead of being one cycle late.  This preserves
  backward compatability.

# SYSTEM LEVEL CHANGES

### System Changes

-  Fix for issue #1613:
   Fixes the rtauhal module for all cases of multichannel output. Previously the
   module would not work correctly with some devices.

-  Csound now supports MP3 files for input and output.

### Translations

### API


### External Plugin Code


### Platform Specific

Many changes to Bela csound.  See bela web site for details

==END==

========================================================================
commit 730cf5f30b841bc04c4846dc0fd4a39ab9c0e44d
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Oct 9 11:43:15 2022 +0100

    protect against types not given as strings

commit d84279798623570bf9f546c097bfba51e52e1de6
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Oct 9 11:14:42 2022 +0100

    fixed OSClisten and OSCsend_lo to work with zero-data messages

commit fbc54056eb777e47966e278123c8129f348dd1bc
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sun Oct 9 10:51:15 2022 +0100

    changing OSCsend to match manual

commit 15d56c13c79caebefef3db47e504651b5f169905
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Oct 7 09:45:28 2022 +0100

    fixed #1638

commit ac38f27d1c5d6ee0cacc1bc719e96be3a2d93a15
Author: gesellkammer <eduardo.moguillansky@gmail.com>
Date:   Mon Sep 26 23:27:42 2022 +0200

    fix sum

commit 0202b14ac91c4e297e99462333116a537cb4790b
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Sep 26 14:41:54 2022 +0100

    commented fix for issue #1633

commit 73ea379cfec9f71ed2ef0769987fff05fd0b4291
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Mon Sep 26 14:39:07 2022 +0100

    fixed #1633

commit ad5f4196ce7fc6fb5ae4be4b652581aff41bab8f
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Sat Sep 24 13:57:19 2022 +0100

    instr0 can't be freed and this won't be changed in 6.*.Added comments to that effect

commit 6ee8fd8ff8bfedbc9807cd582bdefc40201f398c
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Sep 23 14:24:24 2022 +0100

    fixed issue #1631

