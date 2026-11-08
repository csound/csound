<CsTest>
description = "ceps rejects negative coefficient counts"

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
  kMagnitudes[] init 65
  kCepstrum[] ceps kMagnitudes, -1
endin
</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>
