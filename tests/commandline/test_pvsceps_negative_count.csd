<CsTest>
description = "pvsceps rejects negative coefficient counts"

[expect]
exit = "nonzero"
stderr = ["cepstrum coefficient count must be nonnegative"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  kFrame[] init 130
  fInput tab2pvs kFrame, 32
  kCepstrum[] pvsceps fInput, -1
endin
</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>
