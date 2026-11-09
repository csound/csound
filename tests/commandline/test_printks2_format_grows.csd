<CsTest>
description = "printks2 follows its format variable, even when the text outgrows its buffer"

[expect]
exit = 0
output = ["after, in a string long enough that the variable has to grow", "shorter again 8"]
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

gSformat init "before %.0f\n"

instr 1
  kCycle timeinstk
  printks2 gSformat, kCycle
endin

instr 2
  ; Past the 64 bytes the variable started with, so its buffer moves.
  gSformat = "after, in a string long enough that the variable has to grow past the sixty-four bytes it started with %.0f\n"
endin

instr 3
  ; A text that fits stays in the buffer it grew to.
  gSformat = "shorter again %.0f\n"
endin
</CsInstruments>
<CsScore>
i 1 0 .25
i 2 .1 .01
i 3 .2 .01
e
</CsScore>
</CsoundSynthesizer>
