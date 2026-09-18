<CsTest>
description = "tab2array stops before writing when a slice outgrows its output"
[expect]
exit = "nonzero"
stderr = ["Array too small"]
output = ["Output unchanged"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1

giTable ftgen 0, 0, -4, -2, 1, 2, 3, 4
gkValues[] init 0

instr 1
  kStart init 0
  kEnd init 2
  gkValues tab2array giTable, kStart, kEnd
  ; The next cycle needs three slots, but the output only has two.
  kStart = 1
  kEnd = 4
endin

instr 2
  if lenarray(gkValues) == 2 then
    if gkValues[0] == 1 && gkValues[1] == 2 then
      printks "Output unchanged\n", 0
    endif
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125
i 2 .03125 .015625
e
</CsScore>
</CsoundSynthesizer>
