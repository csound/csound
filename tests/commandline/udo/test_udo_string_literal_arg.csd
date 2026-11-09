<CsTest>
description = "A string literal passed to a new-style UDO keeps its text whatever the UDO assigns to its argument"

[expect]
exit = 0
output = ["string literal checks done"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

; Far past the 64 bytes a string variable starts with, and past any literal here.
#define LONG #"a text that replaces the literal, long enough that a variable has to grow to hold all of it, and it keeps going a good while further"#
#define MEDIUM #"a literal longer than the three letters that are written over it"#

giChecks init 0
gkPerformed init 0

opcode Fail(Swhat:S):void
  prints "FAILED: %s\n", Swhat
  exitnow -1
endop

; Each of these starts by checking what it was given, so a second call shows
; what the first one left behind.

opcode GrowArgument(Sx:S):void
  if strcmp(Sx, "short") != 0 then
    Fail("a call was given what an earlier one wrote")
  endif
  Sx = $LONG
  if strcmp(Sx, $LONG) != 0 then
    Fail("the assignment did not reach the argument")
  endif
endop

opcode OverwriteArgument(Sx:S):void
  if strcmp(Sx, $MEDIUM) != 0 then
    Fail("a call was given what an earlier one wrote over a literal")
  endif
  Sx = "fit"
endop

opcode SprintfArgument(Sx:S):void
  Sx sprintf "%s", $LONG
endop

opcode Inner(Sy:S):void
  Sy = "fit"
endop

opcode Outer(Sx:S):void
  Inner(Sx)
  if strcmp(Sx, "fit") != 0 then
    Fail("a nested call did not write its caller's argument")
  endif
endop

opcode PassThrough(Sx:S):S
  Sx = $LONG
  xout Sx
endop

opcode PassThroughFit(Sx:S):S
  Sx = "fit"
  xout Sx
endop

opcode ReadPerf(Sx:S):void
  ; Each k-cycle starts from the literal, as a classic UDO's input does.
  if strcmpk(Sx, $LONG) != 0 then
    printks "FAILED: the argument did not read the literal at performance\n", 0
    exitnowk -1
  endif
  Sx strcpyk "fit"
  if strcmpk(Sx, "fit") != 0 then
    printks "FAILED: a performance-time write did not reach the argument\n", 0
    exitnowk -1
  endif
endop

; a variable stays by reference
opcode Assign(Sx:S):void
  Sx = $LONG
endop

instr 1
  ; Copies made before anything can write through a literal.
  Sshort strcpy "short"
  Smedium strcpy $MEDIUM

  GrowArgument("short")
  GrowArgument("short")
  if strcmp("short", Sshort) != 0 then
    Fail("a grown argument changed the literal")
  endif

  OverwriteArgument($MEDIUM)
  OverwriteArgument($MEDIUM)
  if strcmp($MEDIUM, Smedium) != 0 then
    Fail("a text that fits rewrote the literal")
  endif

  SprintfArgument("short")
  if strcmp("short", Sshort) != 0 then
    Fail("sprintf wrote through the literal")
  endif

  Outer($MEDIUM)
  if strcmp($MEDIUM, Smedium) != 0 then
    Fail("a nested call wrote through the literal")
  endif

  Sback1 = PassThrough("short")
  if strcmp(Sback1, $LONG) != 0 then
    Fail("a pass-through output was wrong")
  endif
  Sback2 = PassThrough("short")
  if strcmp(Sback2, $LONG) != 0 then
    Fail("a pass-through output was wrong the second time")
  endif
  Sback3 = PassThroughFit($MEDIUM)
  if strcmp(Sback3, "fit") != 0 then
    Fail("a pass-through output was wrong when the text fits")
  endif
  if strcmp("short", Sshort) != 0 || strcmp($MEDIUM, Smedium) != 0 then
    Fail("a pass-through wrote back over the literal")
  endif

  Svar strcpy "a variable"
  Assign(Svar)
  if strcmp(Svar, $LONG) != 0 then
    Fail("an assignment did not reach the caller's variable")
  endif

  giChecks += 1
endin

instr 2
  ; Written at every control cycle: the literal is read back at the last one.
  Sbefore strcpy $LONG
  ReadPerf($LONG)
  kc timeinstk
  if kc == 4 then
    if strcmpk($LONG, Sbefore) != 0 then
      printks "FAILED: a performance-time write changed the literal\n", 0
      exitnowk -1
    endif
    gkPerformed = 1
  endif
endin

instr 99
  if giChecks != 2 || i(gkPerformed) != 1 then
    prints "FAILED: the checks did not all run\n"
    exitnow -1
  endif
  prints "string literal checks done\n"
endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 1 .1 .1
i 2 .2 .25
i 99 .5 .01
e
</CsScore>
</CsoundSynthesizer>
