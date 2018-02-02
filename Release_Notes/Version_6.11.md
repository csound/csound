<!---

To maintain this document use the following markdown:

# First level heading
## Second level heading
### Third level heading

- First level bullet point
 - Second level bullet point
  - Third level bullet point

`inline code`

``` preformatted text etc.  ```

[hyperlink](url for the hyperlink)

Any valid HTML can also be used.

--->
# CSOUND VERSION 6.101RELEASE NOTES

There has been a great amount of internal reorganisation, which should
not affect most users.  Some components are now independently managed
and installablevia the new package manager.  The realtime optaion is
now considered stable and has the "experim`ental" tag removed.

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- 


### New Gen and Macros

- 

### Orchestra

- 

### Score

- 

### Options

- 

### Modified Opcodes and Gens

-

### Utilities

-

### Frontends

- icsound:

- csound~:

- csdebugger:

- Emscripten: 

- CsoundQt: 

### General Usage

## Bugs Fixed

- 

## SYSTEM LEVEL CHANGES

### System Changes

-

### Translations

- As ever the French translations are complete.

- 

### API

- 

### Platform Specific

- iOS

 -

- Android

 -

- Windows

- OSX

- GNU/Linux



==END==


========================================================================
ommit 27b61d1dd736aafa54c6eda825fa88ed1cf9081d (HEAD -> develop, origin/develop)
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Feb 2 10:33:04 2018 +0000

commit 4f6d1e9dceb6400f64690d5236563d3e2c6fbcf4
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Jan 31 16:47:17 2018 +0000

    loscilphs and loscil3phs

commit da66696a34cf287fb64ce7e28a338ca5b032996a
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Jan 30 21:13:42 2018 +0000

    OPCODE6DIR{64} now can contain a colon-separated list of directories

commit be800844e805d2fb7aa4f7f4f19a78b7da8b7a82
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Jan 26 18:30:22 2018 +0000

    fixed a-rate linen overlap

commit 61993d4f68b763d90d89384c023fb1b9891124cb
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Fri Jan 26 18:13:55 2018 +0000

    fixed linen a-rate for long durations

commit 9ab171c10031c75a36826698092d0b10bd9bc9ab
Author: Michael Gogins <michael.gogins@gmail.com>
Date:   Mon Jan 22 22:10:05 2018 -0500

    Lua opcodes crashing in reset; fixed by not destroying global pointers for mutexes.

commit 6ca82da475d93a7d627cf537da8e1773f7a2507f
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Wed Jan 17 21:37:36 2018 +0000

    moved opening of MIDI out device to musmon after MIDI in

&&&&&&&&&&&&&&&&&&&&&&&
