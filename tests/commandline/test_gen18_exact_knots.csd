<CsTest>
description = "GEN18 copies exact knots even when the adjacent slope overflows"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1

giSource ftgen 1, 0, -4, -2, 2e38, -2e38, 1, 2
giDest ftgen 2, 0, -4, -18, giSource, 1, 0, 3
instr 1
  iFirst table 0, giDest
  if !(iFirst == 2e38) then
    prints "GEN18 lost the exact source knot\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .008 55
</CsScore>
</CsoundSynthesizer>
