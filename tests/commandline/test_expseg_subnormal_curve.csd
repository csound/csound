<CsTest>
description = "expseg advances when its unscaled update would underflow"

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

instr 1
  iStart = 2^(-140)
  kOut expseg iStart, 1, 2*iStart
  kSample init 0
  if kSample == 512 then
    if !(abs(kOut/iStart - sqrt(2)) < .001) then
      printks "expseg lost the update to underflow\n", 0
      exitnowk -1
    endif
    turnoff
  endif
  kSample += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .6 55
</CsScore>
</CsoundSynthesizer>
