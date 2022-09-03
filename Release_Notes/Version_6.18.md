
# CSOUND VERSION 6.18 RELEASE NOTES

Mainly a bug-fixing release.  Major new facility is MP3 (MPEG) audio
files are supported both for input and output.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- scanmap reads internal state of scanu

### New gen

### Orchestra

-

### Score

### Options

### Modified Opcodes and Gens

-     fix midi list printing to stdout part of the list

### Utilities

- 

### Frontends


## Bugs Fixed

- bug in sum fixd

- 'pitchamd' opcode causes segmentation fault (macOS) (#1599)

- fixed crash on channel setting with empty channel name

- bug in ftloadk fixed (#1611)

- qnan works even when compiled with fast arithmetic

# SYSTEM LEVEL CHANGES

### System Changes

-  AuHAL module was not working properly for -odac (8 channels) (#1613)

-  Csound now supports MP3 files for input and output

### Translations

### API


### External Plugin Code


### Platform Specific

May changes to Bela csound.  See bela web site or details

==END==

========================================================================
scommit 162977998a8ca81f4963ecda3d1a3bfccafa08ad
Merge: b016904e2 ed56e6d47
Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Wed Aug 24 14:11:06 2022 +0100

    reverted changes in timeinstk and timeinsts; fixed order of .k and .i in times and timek; provided two sets of opcodes elapsedcycles, elapsedtime and eventcycles and eventtime which act like timek, times, timeinstk and timeinsts but return the correct values instead of being one cycle late

Author: Victor Lazzarini <victor.lazzarini@mu.ie>
Date:   Wed Jul 6 13:04:26 2022 +0100

    fixing code


commit 66cd7a78e6b38824812037d323c6ecd85805da8c
Author: Hlöðver Sigurðsson <hlolli@gmail.com>
Date:   Mon Apr 18 22:28:43 2022 +0200

    fix sampleRate for single-thread worklet

Author: vlazzarini <victor.lazzarini@nuim.ie>
Date:   Fri Apr 15 18:02:38 2022 +0100

    offset the xdel input in sample accurate




