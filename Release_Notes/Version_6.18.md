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

- Fixed bug where freeing instr 0 caused memory issues in new
  compilations. Instr 0 is not freed until reset now.

- Fixed a bug where opcode directory search was crashing Csound if HOME
  variable not set.

- Fixed pol2rect array size setting.

- OSCsend, OSCsend_lo and OSClisten can now work with OSC messages
  carrying no payload (as per spec and manual).

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


