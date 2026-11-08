<CsTest>
description = "c2r rejects a complex array with an incomplete pair"

[expect]
exit = "nonzero"
stderr = ["c2r: expected a one-dimensional array of complex pairs"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 32
instr 1
  kInput[] init 4
  kCycle timeinstk
  if kCycle == 2 then
    trim kInput, 3
  endif
  kOut[] c2r kInput
endin
</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>
