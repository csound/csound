CSOUND VERSION 6.18 RELEASE NOTES

Mainly a bug-fixing release.  Major new facility is MP3 (MPEG) audio
files are supported both for input and output in platforms where this is 
supported by libsndfile.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- scanmap reads internal state of scanu

- elapsedcycles, elapsedtime, eventcycles, and eventtime. See under
  "bugs fied" for detials.

### New gen

### Orchestra

### Score

### Options

### Modified Opcodes and Gens

- fix midi list printing to stdout part of the list.

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

- bug in sum fixed

- 'pitchamd' opcode causes segmentation fault (macOS) (#1599)

- fixed crash on channel setting with empty channel name

- bug in ftloadk fixed (#1611)

- qnan works even when compiled with fast arithmetic

- Fix the flanger opcode for sample-accurate mode, where the xdel input was
not being correctly offset.

- elapsedcycles, elapsedtime, eventcycles, and eventtime introducd as
  fixed versions of timek, times, timeinstk and timeinsts but return
  the correct values instead of being one cycle late.  This prserves
  backward compatability

# SYSTEM LEVEL CHANGES

### System Changes

-  Fix for issue #1613:
   Fixes the rtauhal module for all cases of multichannel output. Previously the
   module would not work correctly with some devices.

-  Csound now supports MP3 files for input and output through libsndfile (version 1.1.0 and higher).

### Translations

### API

### External Plugin Code

### Platform Specific

Bela:

Many changes to Bela csound.  See bela web site for details

MacOS:

- Universal build for x86_64 and arm64, backward compatible to MacOS 10.9
- Issue where the portaudio callback backend would fail to work in mono (e.g. laptop microphone) has been fixed.

WASM:

- fix sampleRate for single-thread worklet.


==END==

========================================================================


