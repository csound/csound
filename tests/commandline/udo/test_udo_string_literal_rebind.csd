<CsTest>
description = "Rebinding an Opcode object to a string literal its argument can hold reads it, and a variable stays by reference"

[expect]
exit = 0
output = ["all the rebinding checks ran"]
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

gSread init ""
gkChecks init 0

opcode Show(Sx:S):void
  gSread strcpyk Sx
endop

opcode Write(Sx:S):void
  Sx strcpyk "written"
endop

instr 1
  ; 63 characters and a zero fill the 64 bytes the variable has from init.
  objects:Opcode[] = create(opcoderef("Show"), 1)
  init objects[0], "short"
  perf objects[0], "the body read a sixty-three character literal, the most it fits"
  if strcmpk(gSread, "the body read a sixty-three character literal, the most it fits") != 0 then
    printks "FAILED: a literal that fits did not reach the body\n", 0
    exitnowk -1
  endif
  gkChecks = 1
endin

instr 2
  ; The size that counts is what the variable has now, not what it started with.
  objects:Opcode[] = create(opcoderef("Show"), 1)
  init objects[0], "a literal given at init, which makes the variable of the argument larger than the sixty-four bytes it starts with, and keeps it so"
  perf objects[0], "a hundred characters is more than the sixty-four bytes a variable starts with, but this one has grown"
  if strcmpk(gSread, "a hundred characters is more than the sixty-four bytes a variable starts with, but this one has grown") != 0 then
    printks "FAILED: a literal that fits a grown variable did not reach the body\n", 0
    exitnowk -1
  endif
  gkChecks = 2
endin

instr 3
  ; Rebinding between a literal and a variable: the variable is written, the
  ; literal is not.
  objects:Opcode[] = create(opcoderef("Write"), 1)
  init objects[0], "a literal at init"
  Svariable strcpy "a variable"
  Scopy strcpy "a literal rebound at performance"
  kCycle timeinstk
  if kCycle == 1 then
    perf objects[0], Svariable
    if strcmpk(Svariable, "written") != 0 then
      printks "FAILED: a variable rebound from a literal was not written\n", 0
      exitnowk -1
    endif
    Svariable strcpyk "a variable again"
  elseif kCycle == 2 then
    perf objects[0], "a literal rebound at performance"
    if strcmpk("a literal rebound at performance", Scopy) != 0 then
      printks "FAILED: a literal rebound from a variable was written\n", 0
      exitnowk -1
    endif
  elseif kCycle == 3 then
    perf objects[0], Svariable
    if strcmpk(Svariable, "written") != 0 then
      printks "FAILED: a variable rebound from a literal again was not written\n", 0
      exitnowk -1
    endif
    gkChecks = 3
  endif
endin

instr 99
  if i(gkChecks) != 3 then
    prints "FAILED: the rebinding checks did not all run\n"
    exitnow -1
  endif
  prints "all the rebinding checks ran\n"
endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .2 .1
i 3 .4 .25
i 99 .8 .01
e
</CsScore>
</CsoundSynthesizer>
