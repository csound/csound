<CsTest>
description = "GEN24 rescales constant and varying source tables"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  iIndex = 0
  while iIndex < 16 do
    iValue table iIndex, p4
    iExpected = p5 + iIndex * p6
    if !(abs(iValue - iExpected) < .00001) then
      prints "GEN24 table %g at index %g: %g, expected %g\n", p4, iIndex, iValue, iExpected
      exitnow(-1)
    endif
    iIndex += 1
  od
endin
</CsInstruments>
<CsScore>
f 1 0 16 -7 2 16 2
f 2 0 16 -24 1 -1 1
f 3 0 16 -24 1 0 0
f 4 0 16 -7 -2 16 2
f 5 0 16 -24 4 3 7
i 1 0 .01 2 -1 0
i 1 0 .01 3 0 0
i 1 0 .01 5 3 [4/15]
</CsScore>
</CsoundSynthesizer>
