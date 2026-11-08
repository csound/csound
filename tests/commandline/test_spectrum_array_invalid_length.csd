<CsTest>
description = "Spectrum extraction rejects an odd packed input length"

[expect]
exit = "nonzero"
stderr = ["expected a one-dimensional packed real spectrum of even length"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 32
instr 1
  kInput[] init 8
  kCycle timeinstk
  if kCycle == 2 then
    trim kInput, 3
  endif
  kOut[] pows kInput
endin
</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>
