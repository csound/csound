
# CSOUND VERSION 6.17 RELEASE NOTES - DRAFT - DRAFT - DRAFT - DRAFT 

Mainly a bug-fixing release but also a major re-organisation of the
libraries to move all opcodes with dependencies into a separate plugins
repository. The only exception to this is the OSC opcodes (which
depend on liblo). This is in part a preparation for Csound7. This is
the last release using the current parser (v.2).

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- scanmap and scansmap are like the xscanmap/xscansmap opcodes but
  work with the mainstream scan opcodes.

- trigexpseg, triglinseg are aliases for trigExpseg, trigLinseg.

- xscan opcodes are deprecated as they add nothing to the scan opcodes.

- bformdec2 provides more ambisonic decoding of bformat audio.

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

- pvscfs frame counting issue fixed.

- --sample-accurate fixed in a-rate form of tabsum.

- Problem in atssinnoi fixed; did read outside allocated memory.

- hrtfmove fixed; it could use the wrong value for sr.

- Named instruments could use wrong structure in redefinition.

- fixes in the jack backend relating to --get-system-sr.

- pvcross frequency warp mode fixed.

- lpcfilter/lpcanal bug where processing could not be restarted after
  a freeze fixed.

- the emugens collection of opcode, a large group incorporation mainly
  array operations were not made available due to a small editing
  error.

- If using a float build (rather than a double) array access could be
  wrong.

# SYSTEM LEVEL CHANGES

### System Changes

- winsound has been removed

### Translations

### API

CreateThread2 is a new API function that allows threads to be given a
user-defined stack size.

### External Plugin Code

For a variety of reasons, including licences, plugin opcodes that
allows csound to be a host for VST plugins is available from
https://github.com/gogins/csound-vst3-opcodes.git where installation
instructions can be found.  It has been shown to work on Linux, MacOS
and Windows.


### Platform Specific

==END==
    
========================================================================




