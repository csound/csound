
# CSOUND VERSION 6.17 RELEASE NOTES - DRAFT - DRAFT - DRAFT - DRAFT 

Mainly a bug-fixing release but also a major re-organisation of the
libraries to move all opcodes with dependencies into a plugins
directory.  This is in part a preparation for Csound7.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- scanmap and scansmap are like the xscanmap/xscansmap opcodes but
  work with the mainstream scan opcodes.

- trigexpseg, triglinseg are aliases for trigExpseg, trigLinseg.

- xscan opcodes are deprecated as they add nothing to the scan opcodes

### New gen

### Orchestra

- Message printing has been revised so -m0 suppresses (nearly) all
  messages

- add channel count to list_audio_devices (called when the flag
  --devices is used so that it can be parsed by frontends.

### Score

### Options

### Modified Opcodes and Gens

- event opcode does not bail out if the instrument called does not exist.

- Added an optional prefix to soundfont instruments printed via sfilist.

- lpslots reworked with better control.

### Utilities

- cvanal now uses the SADIR environment to look for analysis files

### Frontends

### General Usage

## Bugs Fixed

- fareylen called a non-existent function leading to a crash.  Removed typo.

- turnoff could cause clicks in some cases; fixed

- turnoff3 improved

- cntReset fixed; had a false initialisation code

- binary search in bpf fixed.

- --sample-accurate fixed in a-rateform of tabsum.

- Problem in atssinnoi fixe; did read ousideallocated memory.

- hrtfmove fixed; it could use the wrong value for sr.

- Named instrumentscould use wrong structurein redefinition.

# SYSTEM LEVEL CHANGES

### System Changes

- winsoud has been removed

### Translations

### API

CreateThread2 is a e API functio.

### Platform Specific

==END==
    
========================================================================
commit 15fed58bd13353197c401d90eb47b79e46743498
Author: John ffitch <jpff@codemist.co.uk>
Date:   Wed Sep 22 21:27:56 2021 +0100

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

Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Sep 8 14:54:25 2021 +0200

    fix --version; fix samplerate mismatch in jack with --get-system-sr; change some warning messages to Warning; make more printing depend on msglevel

Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Thu Sep 2 11:53:36 2021 +0200

    fix emugens


commit 3db468b510dab5a4fc817bd668c0216373c467cf
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Wed Sep 1 17:07:49 2021 +0100

    fixed kflag parameter

commit 1a7a98b533a6972a4c4cd253793d120d616cc1aa
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Fri Aug 27 14:27:43 2021 +0100

    fixed bug in pvcross

