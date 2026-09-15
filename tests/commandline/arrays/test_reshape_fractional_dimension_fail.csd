<CsTest>
description = "reject fractional reshape dimensions"

[expect]
exit = "nonzero"
stderr = ["reshapearray: dimension 0 must be a positive integer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

instr 1
  values:i[] init 2
  reshapearray values, 2.5
endin

</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
