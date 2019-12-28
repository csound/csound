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
======== DRAFT ======== DRAFT ======== DRAFT ======== DRAFT ======== DRAFT ========

# CSOUND VERSION 6.14 RELEASE NOTES

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- randc is like randi but uses a cubic interpolation.

- mp3out is an experimental mplementation of writing an mp3 file.  It
  may be replaced  by the current work in libsndfile to deal with MPEG
  files.

- metro2 is likre metro but with added controllable swing.

- ftexists reports whether a numbered ftable exists.

- schedulek is a k-time opcode ust ike schedule.



### New Gen and Macros

### Orchestra

- The conditional expresson syntax a?b:c incorrectly always
  calculated b and c before selecting which to return.  This could
  give incorrect devision by zero errors or cause unexpected multiple
  evaluations of opcodes.  It now impements the common C-like semantics.

- Orchestra macros are now persistent, so they apply in every
  compilation after thet are definded unti they are undefined.  It has
  been changed because of the need of live coding in particular.  A
  correct orchestra should no be affected.


### Score

- 
  
### Options

- New option simple-sorted-score creates file score.srt in a more
  user-friendly format

- Revise treatment of CsOptions wrt double quotes and spaces which need escaping.

- Setting the 1024 bit in -m suppresses printing of messages about
using deprecated opcodes.  This option is itself deprecated.

### Modified Opcodes and Gens

- squinewave now handles optional a or k rate argument.

- pindex opcode handls string fields as well as numeric ones.

- sflooper reworked to avoid a crash and provide warnings.

- event_i and schedule can rake fractional p1.

- in sound font opcodes better checking.   Also no longer will load
  multiple copies of a sound font, but reuses existing load.

- fluidControl has a new optional argument to control printing
  messages.

- bpf has an audio version now.

- stsend/stecv can work with unmatched k-rates.

- pvstrace has new optional arguments.

- lpfreson checks number of poles.

- syncloop had a small typing error that caused crashes.

- bpfcs has new array versions.

- zacl can omit second argument, defaults to clearing only the given channel.

### Utilities

- lpanal now checks that suficient poles are requested.

### Frontends

- Belacsound:

- CsoundQt:

### General Usage

- // comments at rthe start of a line now accepted in CsOptions
  section of a csd file. 

## Bugs Fixed

- shiftin fixed.

- exitnow delivers the return code as documented.

- fixed bug in beosc, where gaussian noise generator was not being
  initialised.

- OSCraw fixed.

- ftkloadk could select incorrect interal code causing a crash.

- GEN01 when used to read a single channel of a mlti-channel file got
  the length incorrect.

- ftgenonce had a fencepost problem so it could overwrite a table in
  use.

- a race condition in Jacko opcodes improved (issue #1182).

- syncloop had a small typing error that caused crashes.

- lowresx was incomplete and did not work as intended; rewritte (issue #1199)

# SYSTEM LEVEL CHANGES

-

### System Changes

- New plugin class for opcodes with no outputs written.
erform time errors and int errors are also reorte in te retur code of
the command line system.  The new API function GetErrorCnt is
available to do somting similar in other varants.
- 

### Translations

### API

- Function GetErrorCnt gives the number of perf-time errors, and adds
  in the init-time errors at the end of a rendering.

- Function FTnp2Find no longer print smessages if the table is not
found, but just returns NULL.  Previous behavour is available  as
FTnp2Finde.

- csoundGetInstrument() added

### Platform Specific

- WebAudio: 

- iOS

- Android

- Windows
 - stsend reworked for winsock library

- MacOS

- GNU/Linux

- Haiku port

- Bela

==END==

-----------------------------------------------------------------------
------------------------------------------------------------------------

commit d4980cd2fe6dd1106e4b72d31f106f7361739df8 (HEAD -> develop, origin/develop, origin/HEAD)
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Dec 24 13:08:59 2019 +0000

    port mapping messages

commit 65670eb0c378ad8a8b42881d099928688395de81
Author: vlazzarini <victor.lazzarini@mu.ie>
Date:   Tue Dec 24 12:42:24 2019 +0000

    implemented port mapping mechanism in Csound and portmidi m device id to use it

commit 526b0ca658da993e55c467629d01d3d8092ffb9a
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Dec 4 16:42:00 2019 +0100

    outvalue: koutval is called only once during the init phase. Also the order of
    the declaration of the opcode variants in Engine/entry1.c was changed to allow
    outvalue.Si to take precedence over outvalue.k for when used "outvalue Schan, ivalue"
    (outvalue.k is still used for outvalue Schan, kvar)

commit ad5e171bb1e9c0a5e4d5419531344db0b32352dd
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Dec 3 11:02:15 2019 +0100

    expose csoundCompile with extra commandline arg to allow overriding -o if it is in CsOptions

commit 88c1f746891cbb121691b8ebbccda48b4862daac
Author: roryywalsh <rorywalsh@ear.ie>
Date:   Wed Nov 13 10:33:07 2019 +0000

    adding k-time checking for channel strings in chnget and chnset opcodes

commit a4706809398daf2eddb0b778bfe4e2c57d3dce64
Merge: c04f371afd dbd36d56e2
Author: Pete Goodeve <pete.goodeve@computer.org>
Date:   Tue Oct 29 17:16:21 2019 -0700

    resolve sfont.c confusion

commit 683eeb5a6605e6b179894a2438e496d6b4a9f3dc
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Sep 24 20:38:20 2019 +0100

    added some more functionality to CPOF

commit 797f3098efdc06e587fe8d45a4b9b6b4fc4daec0
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu Sep 19 13:53:55 2019 -0400

    added getPlayState(), addPlayStateListener(), and other methods for querying and listening to the playing state of Csound

