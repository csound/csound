<CsTest>
description = "GEN20 centers odd and even windows and handles the sinc origin"

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
  iLength = ftlen(p4)
  iIndex = 0
  while iIndex < iLength do
    iPosition = iIndex / iLength - .5
    if p5 == 3 then
      iExpected = 1 - 2 * abs(iPosition)
    elseif p5 == 6 then
      iExpected = exp(-18 * iPosition * iPosition)
    else
      iX = 2 * $M_PI * iPosition * p6
      iExpected = (iX == 0 ? 1 : sin(iX) / iX)
    endif
    iActual table iIndex, p4
    if !(abs(iActual - iExpected) < .00001) then
      prints "GEN20 type %g, length %g, index %g: %g, expected %g\n", p5, iLength, iIndex, iActual, iExpected
      exitnow(-1)
    endif
    iIndex += 1
  od
endin
</CsInstruments>
<CsScore>
f 1 0 -5 -20 3 1
f 2 0 -5 -20 6 1 2
f 3 0 -5 -20 9 1 1
f 4 0 8 -20 3 1
f 5 0 8 -20 6 1 2
f 6 0 8 -20 9 1 1
f 7 0 8 -20 9 1 0
i 1 0 .01 1 3
i 1 0 .01 2 6
i 1 0 .01 3 9 1
i 1 0 .01 4 3
i 1 0 .01 5 6
i 1 0 .01 6 9 1
i 1 0 .01 7 9 0
</CsScore>
</CsoundSynthesizer>
