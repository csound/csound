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

- randc is like radi but uses a cubic interpolation.

### New Gen and Macros

### Orchestra

- The conditional expresson syntax a?b:c incorrectly always
  calculated b and c before selecting which to return.  Ths coulfd
  give incorrect devision by zero errors or cause unexpecte multiple
  evaluations of opcodes.  It now impemens te sommon C-like semantics.

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

### Utilities

- 

### Frontends

- Belacsound:

- CsoundQt:

### General Usage

- 

## Bugs Fixed

- shiftin fixed.

- exitnow delivers the return code as documented.

- fixed bug in beosc, where gaussian noise generator was not being
  initialised.

- OSCraw fixed.

- ftkloadk could select incorrect interal code causing a crash.

- GEN01 when used to read a single channel of a mlti-channel file got
  the length incorrect.


# SYSTEM LEVEL CHANGES

-

### System Changes

 - New plugin class for opcodes with no outputs written.

### Translations

### API

- 

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
commit 89c088842aa70d1c27ab289bca8a80f1dd12390e
Author: John ffitch <jpff@codemist.co.uk>
Date:   Tue Oct 8 15:57:32 2019 +0100

commit 683eeb5a6605e6b179894a2438e496d6b4a9f3dc
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Sep 24 20:38:20 2019 +0100

    added some more functionality to CPOF

commit 797f3098efdc06e587fe8d45a4b9b6b4fc4daec0
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu Sep 19 13:53:55 2019 -0400

    added getPlayState(), addPlayStateListener(), and other methods for querying and listening to the playing state of Csound

commit cbc3af7c5b80b5073f187604bf3b60a00539c6d6
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu Sep 12 10:56:37 2019 -0400

    store temp to prevent dangling pointer [issue #1181]

commit d1260b62724bdce9281a9a6e046b3443b0f4544e
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Sep 4 15:58:56 2019 -0400

    fix for message printing when using ScriptProcessorNode [fixes #1174]


commit 8eca5dda97c20118b85db97943f6a53e62e9aa74
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jul 28 21:41:19 2019 +0100

    fixed opcode registration in CPOF

